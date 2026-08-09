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

#ifndef MOUSE_CAPTURE_H
#define MOUSE_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>

#define MOUSE_CAPTURE_MONITOR_ALL -1
#define MOUSE_CAPTURE_MONITOR_NAME_MAX 64

/* Monitor indices are zero-based and names use the Win32 display identifier. */
size_t mouse_capture_get_monitor_count(void);
bool mouse_capture_get_monitor_name(size_t index, char *name, size_t name_size);

/* Stores the cursor position normalized to [0, 1] for the selected monitor.
 * Returns false when the cursor is unavailable or outside that monitor. An
 * invalid index falls back to the complete virtual desktop. */
bool mouse_capture_sample_position(int monitor_index, float *x, float *y);

/* Returns the latest shared click sequences without sampling input. */
void mouse_capture_get_button_sequences(long *left, long *right);

/* Samples both buttons and returns monotonically increasing sequences.
 * A sequence changes once per press regardless of how many source instances
 * sample it, so each instance can consume the same edge exactly once. */
void mouse_capture_sample_button_sequences(long *left, long *right, bool *left_down, bool *right_down);

#endif
