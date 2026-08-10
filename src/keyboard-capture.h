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

#pragma once

#include <stdbool.h>

enum keyboard_overlay_key {
	KEYBOARD_KEY_W,
	KEYBOARD_KEY_A,
	KEYBOARD_KEY_S,
	KEYBOARD_KEY_D,
	KEYBOARD_KEY_COUNT,
};

struct keyboard_capture_snapshot {
	bool pressed[KEYBOARD_KEY_COUNT];
	long press_sequences[KEYBOARD_KEY_COUNT];
};

bool keyboard_capture_initialize(void);
void keyboard_capture_shutdown(void);

/* Every source receives the same state and press sequences. With Raw Input, a
 * sequence also preserves a complete press that occurs between source ticks. */
void keyboard_capture_sample_wasd(bool arrow_aliases, struct keyboard_capture_snapshot *snapshot);
