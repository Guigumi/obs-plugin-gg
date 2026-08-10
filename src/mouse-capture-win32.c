/*
 Mouse
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
#define GAME_DETECTION_CACHE_MS 100u
#define GAME_DETECTION_HYSTERESIS_MS 2000u
#define RAW_INPUT_RECENT_MS 500u
#define RAW_INPUT_OWNERSHIP_CACHE_MS 1000u
#define RAW_INPUT_WINDOW_CLASS L"MouseRawInput"

static volatile long shared_button_state;
static volatile long shared_left_click_sequence;
static volatile long shared_right_click_sequence;
static SRWLOCK relative_input_lock = SRWLOCK_INIT;
static int64_t relative_total_x;
static int64_t relative_total_y;
static uint64_t relative_last_input_time;
static HANDLE raw_input_thread;
static HANDLE raw_input_ready_event;
static DWORD raw_input_thread_id;
static void *volatile raw_input_window;
static volatile LONG raw_input_available;
static volatile LONG raw_input_registered;
static SRWLOCK raw_input_ownership_lock = SRWLOCK_INIT;
static uint64_t raw_input_ownership_time;

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
static SRWLOCK game_detection_lock = SRWLOCK_INIT;
static uint64_t game_detection_time;
static bool game_detection_result;
static HWND game_detection_window;
static uint64_t game_detection_last_positive;

static bool query_registered_raw_mouse(RAWINPUTDEVICE *result, bool *found)
{
	*found = false;
	UINT count = 0;
	if (GetRegisteredRawInputDevices(NULL, &count, sizeof(RAWINPUTDEVICE)) != 0)
		return false;
	if (!count)
		return true;
	RAWINPUTDEVICE *devices = HeapAlloc(GetProcessHeap(), 0, count * sizeof(*devices));
	if (!devices)
		return false;
	const UINT found_count = GetRegisteredRawInputDevices(devices, &count, sizeof(*devices));
	bool success = found_count != (UINT)-1;
	if (found_count != (UINT)-1) {
		for (UINT i = 0; i < found_count; i++) {
			if (devices[i].usUsagePage == 0x01 && devices[i].usUsage == 0x02) {
				*result = devices[i];
				*found = true;
				break;
			}
		}
	}
	HeapFree(GetProcessHeap(), 0, devices);
	return success;
}

static LRESULT CALLBACK raw_input_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	UNUSED_PARAMETER(wparam);
	if (message == WM_INPUT) {
		RAWINPUT input;
		UINT size = sizeof(input);
		if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) == size &&
		    input.header.dwType == RIM_TYPEMOUSE && !(input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
			AcquireSRWLockExclusive(&relative_input_lock);
			relative_total_x += input.data.mouse.lLastX;
			relative_total_y += input.data.mouse.lLastY;
			relative_last_input_time = GetTickCount64();
			ReleaseSRWLockExclusive(&relative_input_lock);
		}
		return DefWindowProcW(window, message, wparam, lparam);
	}
	if (message == WM_CLOSE) {
		DestroyWindow(window);
		return 0;
	}
	if (message == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(window, message, wparam, lparam);
}

static DWORD WINAPI raw_input_thread_proc(void *param)
{
	UNUSED_PARAMETER(param);
	const HINSTANCE instance = GetModuleHandleW(NULL);
	WNDCLASSW window_class = {0};
	window_class.lpfnWndProc = raw_input_window_proc;
	window_class.hInstance = instance;
	window_class.lpszClassName = RAW_INPUT_WINDOW_CLASS;
	const ATOM class_atom = RegisterClassW(&window_class);
	if (!class_atom) {
		SetEvent(raw_input_ready_event);
		return 1;
	}

	HWND window =
		CreateWindowExW(0, RAW_INPUT_WINDOW_CLASS, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, instance, NULL);
	InterlockedExchangePointer(&raw_input_window, window);
	if (window) {
		RAWINPUTDEVICE existing_device;
		bool existing_device_found;
		RAWINPUTDEVICE device = {0};
		device.usUsagePage = 0x01;
		device.usUsage = 0x02;
		device.dwFlags = RIDEV_INPUTSINK;
		device.hwndTarget = window;
		if (query_registered_raw_mouse(&existing_device, &existing_device_found) && !existing_device_found &&
		    RegisterRawInputDevices(&device, 1, sizeof(device))) {
			InterlockedExchange(&raw_input_registered, 1);
			InterlockedExchange(&raw_input_available, 1);
		}
	}
	SetEvent(raw_input_ready_event);

	MSG message;
	while (GetMessageW(&message, NULL, 0, 0) > 0) {
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}

	RAWINPUTDEVICE current_device;
	bool current_device_found;
	if (InterlockedCompareExchange(&raw_input_registered, 0, 0) &&
	    query_registered_raw_mouse(&current_device, &current_device_found) && current_device_found &&
	    current_device.hwndTarget == window) {
		RAWINPUTDEVICE remove_device = {0};
		remove_device.usUsagePage = 0x01;
		remove_device.usUsage = 0x02;
		remove_device.dwFlags = RIDEV_REMOVE;
		RegisterRawInputDevices(&remove_device, 1, sizeof(remove_device));
	}
	InterlockedExchange(&raw_input_registered, 0);
	if (window && IsWindow(window))
		DestroyWindow(window);
	InterlockedExchangePointer(&raw_input_window, NULL);
	InterlockedExchange(&raw_input_available, 0);
	UnregisterClassW(RAW_INPUT_WINDOW_CLASS, instance);
	return 0;
}

bool mouse_capture_initialize(void)
{
	if (raw_input_thread)
		return InterlockedCompareExchange(&raw_input_available, 0, 0) != 0;
	raw_input_ready_event = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (!raw_input_ready_event)
		return false;
	raw_input_thread = CreateThread(NULL, 0, raw_input_thread_proc, NULL, 0, &raw_input_thread_id);
	if (!raw_input_thread) {
		CloseHandle(raw_input_ready_event);
		raw_input_ready_event = NULL;
		return false;
	}
	WaitForSingleObject(raw_input_ready_event, INFINITE);
	return InterlockedCompareExchange(&raw_input_available, 0, 0) != 0;
}

void mouse_capture_shutdown(void)
{
	if (!raw_input_thread)
		return;
	const HWND window = InterlockedCompareExchangePointer(&raw_input_window, NULL, NULL);
	if (window)
		PostMessageW(window, WM_CLOSE, 0, 0);
	PostThreadMessageW(raw_input_thread_id, WM_QUIT, 0, 0);
	WaitForSingleObject(raw_input_thread, INFINITE);
	CloseHandle(raw_input_thread);
	raw_input_thread = NULL;
	raw_input_thread_id = 0;
	if (raw_input_ready_event) {
		CloseHandle(raw_input_ready_event);
		raw_input_ready_event = NULL;
	}
}

void mouse_capture_get_relative_totals(int64_t *x, int64_t *y)
{
	AcquireSRWLockShared(&relative_input_lock);
	*x = relative_total_x;
	*y = relative_total_y;
	ReleaseSRWLockShared(&relative_input_lock);
}

bool mouse_capture_relative_available(void)
{
	if (!InterlockedCompareExchange(&raw_input_registered, 0, 0))
		return false;

	const uint64_t now = GetTickCount64();
	AcquireSRWLockExclusive(&raw_input_ownership_lock);
	if (!raw_input_ownership_time || now - raw_input_ownership_time >= RAW_INPUT_OWNERSHIP_CACHE_MS) {
		RAWINPUTDEVICE current_device;
		bool current_device_found;
		const HWND window = InterlockedCompareExchangePointer(&raw_input_window, NULL, NULL);
		const bool owns_registration = window &&
					       query_registered_raw_mouse(&current_device, &current_device_found) &&
					       current_device_found && current_device.hwndTarget == window;
		InterlockedExchange(&raw_input_available, owns_registration);
		raw_input_ownership_time = now;
	}
	const bool available = InterlockedCompareExchange(&raw_input_available, 0, 0) != 0;
	ReleaseSRWLockExclusive(&raw_input_ownership_lock);
	if (!available)
		return false;

	AcquireSRWLockShared(&relative_input_lock);
	const bool relative_input_seen = relative_last_input_time != 0;
	ReleaseSRWLockShared(&relative_input_lock);
	return relative_input_seen;
}

static bool detect_game_mode_uncached(HWND *foreground_window, bool *raw_input_recent)
{
	const HWND foreground = GetForegroundWindow();
	*foreground_window = foreground;
	*raw_input_recent = false;
	if (!foreground || foreground == GetShellWindow() || IsIconic(foreground))
		return false;
	DWORD process_id = 0;
	GetWindowThreadProcessId(foreground, &process_id);
	if (!process_id || process_id == GetCurrentProcessId())
		return false;

	RECT window_rect;
	MONITORINFO monitor_info = {0};
	monitor_info.cbSize = sizeof(monitor_info);
	const HMONITOR monitor = MonitorFromWindow(foreground, MONITOR_DEFAULTTONEAREST);
	if (!GetWindowRect(foreground, &window_rect) || !GetMonitorInfoW(monitor, &monitor_info))
		return false;

	const int window_width = window_rect.right - window_rect.left;
	const int window_height = window_rect.bottom - window_rect.top;
	const int monitor_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
	const int monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
	const bool fullscreen = window_rect.left <= monitor_info.rcMonitor.left + 2 &&
				window_rect.top <= monitor_info.rcMonitor.top + 2 &&
				window_rect.right >= monitor_info.rcMonitor.right - 2 &&
				window_rect.bottom >= monitor_info.rcMonitor.bottom - 2;

	RECT clip_rect;
	const RECT virtual_rect = {
		GetSystemMetrics(SM_XVIRTUALSCREEN),
		GetSystemMetrics(SM_YVIRTUALSCREEN),
		GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN),
		GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN),
	};
	RECT clipped_to_window;
	const bool confined = GetClipCursor(&clip_rect) && !EqualRect(&clip_rect, &virtual_rect) &&
			      IntersectRect(&clipped_to_window, &clip_rect, &window_rect) &&
			      EqualRect(&clipped_to_window, &clip_rect);
	CURSORINFO cursor_info = {0};
	cursor_info.cbSize = sizeof(cursor_info);
	const bool cursor_hidden = GetCursorInfo(&cursor_info) && !(cursor_info.flags & CURSOR_SHOWING);
	const bool large_window = window_width >= monitor_width / 2 && window_height >= monitor_height / 2;
	AcquireSRWLockShared(&relative_input_lock);
	*raw_input_recent = relative_last_input_time &&
			    GetTickCount64() - relative_last_input_time <= RAW_INPUT_RECENT_MS;
	ReleaseSRWLockShared(&relative_input_lock);
	return cursor_hidden && (confined || (fullscreen && large_window));
}

bool mouse_capture_detect_game_mode(void)
{
	const uint64_t now = GetTickCount64();
	AcquireSRWLockExclusive(&game_detection_lock);
	if (!game_detection_time || now - game_detection_time >= GAME_DETECTION_CACHE_MS) {
		HWND foreground = GetForegroundWindow();
		bool raw_input_recent = false;
		const bool game_signals = mouse_capture_relative_available() &&
					  detect_game_mode_uncached(&foreground, &raw_input_recent);
		const bool detected = game_signals && (raw_input_recent ||
						       (game_detection_result && foreground == game_detection_window));
		if (detected) {
			game_detection_window = foreground;
			game_detection_last_positive = now;
			game_detection_result = true;
		} else {
			game_detection_result = foreground && foreground == game_detection_window &&
						now - game_detection_last_positive < GAME_DETECTION_HYSTERESIS_MS;
		}
		game_detection_time = now;
	}
	const bool result = game_detection_result;
	ReleaseSRWLockExclusive(&game_detection_lock);
	return result;
}

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
