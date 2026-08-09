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

#define BOUND_CACHE_MS 2000u
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

static void refresh_bounds(struct desktop_bounds *b)
{
	b->left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	b->top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	b->width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	b->height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

void mouse_capture_sample_position(float *x, float *y)
{
	*x = 0.5f;
	*y = 0.5f;

	POINT point;
	if (!GetCursorPos(&point))
		return;

	static struct desktop_bounds bounds;
	static uint64_t last_refresh = 0;

	const uint64_t now = GetTickCount64();
	if (bounds.width <= 0 || (now - last_refresh) >= BOUND_CACHE_MS) {
		refresh_bounds(&bounds);
		last_refresh = now;
	}

	if (bounds.width <= 0 || bounds.height <= 0)
		return;

	float nx = (float)(point.x - bounds.left) / (float)bounds.width;
	float ny = (float)(point.y - bounds.top) / (float)bounds.height;

	*x = fminf(fmaxf(nx, 0.0f), 1.0f);
	*y = fminf(fmaxf(ny, 0.0f), 1.0f);
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
