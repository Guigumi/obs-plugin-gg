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

/*
 * Queries the current cursor position and stores it normalized to
 * [0, 1] in *x/*y using the virtual desktop bounds. If the position
 * cannot be obtained the values default to (0.5, 0.5).
 *
 * The desktop bounds are only refreshed a few times per second, so
 * this call stays cheap in the common frame path.
 */
void mouse_capture_sample_position(float *x, float *y);

/*
 * Queries the pressed state of the left and right mouse buttons.
 * Values are true while the button is held down (edge detection must
 * be done by the caller).
 */
void mouse_capture_sample_buttons(bool *left, bool *right);

#endif