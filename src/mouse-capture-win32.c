/*
Mouse Overlay GG
Copyright (C) 2026 GUI

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "mouse-capture.h"

#include <windows.h>

#include <util/threading.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define BOUND_CACHE_MS 2000u
#define MONITOR_CACHE_MAX 32u
#define MOUSE_BUTTON_LEFT 0x01L
#define MOUSE_BUTTON_RIGHT 0x02L

static volatile long shared_button_state;
static volatile long shared_left_click_sequence;
static volatile long shared_right_click_sequence;

struct desktop_bounds {
	int left;
	int top;
	int width;
	int height;
};

struct monitor_entry {
	struct desktop_bounds bounds;
	char name[MOUSE_CAPTURE_MONITOR_NAME_MAX];
};

struct monitor_cache {
	struct monitor_entry entries[MONITOR_CACHE_MAX];
	size_t count;
	uint64_t last_refresh;
};

static SRWLOCK monitor_cache_lock = SRWLOCK_INIT;
static struct monitor_cache monitors;

static BOOL CALLBACK enumerate_monitor(HMONITOR monitor, HDC dc, LPRECT rect, LPARAM param)
{
	UNUSED_PARAMETER(dc);
	UNUSED_PARAMETER(rect);
	struct monitor_cache *cache = (struct monitor_cache *)param;
	if (cache->count >= MONITOR_CACHE_MAX)
		return FALSE;

	MONITORINFOEXA info = {0};
	info.cbSize = sizeof(info);
	if (!GetMonitorInfoA(monitor, (MONITORINFO *)&info))
		return TRUE;

	struct monitor_entry *entry = &cache->entries[cache->count++];
	entry->bounds.left = info.rcMonitor.left;
	entry->bounds.top = info.rcMonitor.top;
	entry->bounds.width = info.rcMonitor.right - info.rcMonitor.left;
	entry->bounds.height = info.rcMonitor.bottom - info.rcMonitor.top;
	snprintf(entry->name, sizeof(entry->name), "%s", info.szDevice);
	return TRUE;
}

static void refresh_monitor_cache(void)
{
	const uint64_t now = GetTickCount64();
	if (monitors.count > 0 && now - monitors.last_refresh < BOUND_CACHE_MS)
		return;

	monitors.count = 0;
	EnumDisplayMonitors(NULL, NULL, enumerate_monitor, (LPARAM)&monitors);
	monitors.last_refresh = now;
}

static struct desktop_bounds virtual_desktop_bounds(void)
{
	struct desktop_bounds bounds;
	bounds.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	bounds.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	bounds.width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	bounds.height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	return bounds;
}

size_t mouse_capture_get_monitor_count(void)
{
	AcquireSRWLockExclusive(&monitor_cache_lock);
	refresh_monitor_cache();
	const size_t count = monitors.count;
	ReleaseSRWLockExclusive(&monitor_cache_lock);
	return count;
}

bool mouse_capture_get_monitor_name(size_t index, char *name, size_t name_size)
{
	if (!name || !name_size)
		return false;

	AcquireSRWLockExclusive(&monitor_cache_lock);
	refresh_monitor_cache();
	const bool found = index < monitors.count;
	if (found)
		snprintf(name, name_size, "%s", monitors.entries[index].name);
	ReleaseSRWLockExclusive(&monitor_cache_lock);
	if (!found)
		name[0] = '\0';
	return found;
}

bool mouse_capture_sample_position(int monitor_index, float *x, float *y)
{
	*x = 0.5f;
	*y = 0.5f;

	POINT point;
	if (!GetCursorPos(&point))
		return false;

	struct desktop_bounds bounds = virtual_desktop_bounds();
	bool selected_monitor_found = false;
	if (monitor_index >= 0) {
		AcquireSRWLockExclusive(&monitor_cache_lock);
		refresh_monitor_cache();
		if ((size_t)monitor_index < monitors.count) {
			bounds = monitors.entries[monitor_index].bounds;
			selected_monitor_found = true;
		}
		ReleaseSRWLockExclusive(&monitor_cache_lock);
	}

	if (bounds.width <= 0 || bounds.height <= 0)
		return false;
	if (selected_monitor_found && (point.x < bounds.left || point.x >= bounds.left + bounds.width ||
				       point.y < bounds.top || point.y >= bounds.top + bounds.height))
		return false;

	float nx = (float)(point.x - bounds.left) / (float)bounds.width;
	float ny = (float)(point.y - bounds.top) / (float)bounds.height;

	*x = fminf(fmaxf(nx, 0.0f), 1.0f);
	*y = fminf(fmaxf(ny, 0.0f), 1.0f);
	return true;
}

void mouse_capture_get_button_sequences(long *left, long *right)
{
	*left = os_atomic_load_long(&shared_left_click_sequence);
	*right = os_atomic_load_long(&shared_right_click_sequence);
}

void mouse_capture_sample_button_sequences(long *left, long *right, bool *left_down, bool *right_down)
{
	*left_down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	*right_down = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	const long button_state = (*left_down ? MOUSE_BUTTON_LEFT : 0) | (*right_down ? MOUSE_BUTTON_RIGHT : 0);
	const long previous_state = os_atomic_exchange_long(&shared_button_state, button_state);

	if ((button_state & MOUSE_BUTTON_LEFT) && !(previous_state & MOUSE_BUTTON_LEFT))
		os_atomic_inc_long(&shared_left_click_sequence);
	if ((button_state & MOUSE_BUTTON_RIGHT) && !(previous_state & MOUSE_BUTTON_RIGHT))
		os_atomic_inc_long(&shared_right_click_sequence);

	mouse_capture_get_button_sequences(left, right);
}
