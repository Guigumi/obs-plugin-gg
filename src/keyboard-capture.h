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

#pragma once

#include <stdbool.h>

enum keyboard_overlay_key {
	KEYBOARD_KEY_W,
	KEYBOARD_KEY_A,
	KEYBOARD_KEY_S,
	KEYBOARD_KEY_D,
	KEYBOARD_KEY_SPACE,
	KEYBOARD_KEY_SHIFT,
	KEYBOARD_KEY_CTRL,
	KEYBOARD_KEY_Q,
	KEYBOARD_KEY_E,
	KEYBOARD_KEY_R,
	KEYBOARD_KEY_F,
	KEYBOARD_KEY_TAB,
	KEYBOARD_KEY_CAPS,
	KEYBOARD_KEY_1,
	KEYBOARD_KEY_2,
	KEYBOARD_KEY_3,
	KEYBOARD_KEY_4,
	KEYBOARD_KEY_5,
	KEYBOARD_KEY_COUNT,
};

enum keyboard_capture_layout {
	KEYBOARD_LAYOUT_WASD,
	KEYBOARD_LAYOUT_ESDF,
	KEYBOARD_LAYOUT_ARROWS,
	KEYBOARD_LAYOUT_NUMPAD,
	KEYBOARD_LAYOUT_CUSTOM,
	KEYBOARD_LAYOUT_COUNT,
};

struct keyboard_capture_snapshot {
	bool pressed[KEYBOARD_KEY_COUNT];
	long press_sequences[KEYBOARD_KEY_COUNT];
};

bool keyboard_capture_initialize(void);
void keyboard_capture_shutdown(void);

/* Every source receives the same state and press sequences. With Raw Input, a
 * sequence also preserves a complete press that occurs between source ticks. */
void keyboard_capture_sample(enum keyboard_capture_layout layout, bool arrow_aliases,
				     struct keyboard_capture_snapshot *snapshot);
