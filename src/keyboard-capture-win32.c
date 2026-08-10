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

#include "keyboard-capture.h"

#include <windows.h>

#include <stdint.h>
#include <string.h>

#define RAW_INPUT_WINDOW_CLASS L"MouseKeyboardRawInput"
#define RAW_INPUT_RECONCILE_TIMER 1u
#define RAW_INPUT_RECONCILE_MS 250u
#define RAW_INPUT_OWNERSHIP_TIMER 2u
#define RAW_INPUT_OWNERSHIP_MS 1000u
#define RAW_INPUT_EVENT_TIMEOUT_MS 100u
#define KEYBOARD_FALLBACK_POLL_MS 5u
#define RAW_INPUT_DEVICE_MAX 32u

static int keyboard_key_virtual_key(enum keyboard_overlay_key key)
{
	switch (key) {
	case KEYBOARD_KEY_W: return 'W';
	case KEYBOARD_KEY_A: return 'A';
	case KEYBOARD_KEY_S: return 'S';
	case KEYBOARD_KEY_D: return 'D';
	case KEYBOARD_KEY_SPACE: return VK_SPACE;
	case KEYBOARD_KEY_SHIFT: return VK_LSHIFT;
	case KEYBOARD_KEY_CTRL: return VK_LCONTROL;
	case KEYBOARD_KEY_Q: return 'Q';
	case KEYBOARD_KEY_E: return 'E';
	case KEYBOARD_KEY_R: return 'R';
	case KEYBOARD_KEY_F: return 'F';
	case KEYBOARD_KEY_TAB: return VK_TAB;
	case KEYBOARD_KEY_CAPS: return VK_CAPITAL;
	case KEYBOARD_KEY_1: return '1';
	case KEYBOARD_KEY_2: return '2';
	case KEYBOARD_KEY_3: return '3';
	case KEYBOARD_KEY_4: return '4';
	case KEYBOARD_KEY_5: return '5';
	case KEYBOARD_KEY_ESCAPE: return VK_ESCAPE;
	case KEYBOARD_KEY_F1: return VK_F1;
	case KEYBOARD_KEY_F2: return VK_F2;
	case KEYBOARD_KEY_F3: return VK_F3;
	case KEYBOARD_KEY_F4: return VK_F4;
	case KEYBOARD_KEY_F5: return VK_F5;
	case KEYBOARD_KEY_F6: return VK_F6;
	case KEYBOARD_KEY_F7: return VK_F7;
	case KEYBOARD_KEY_F8: return VK_F8;
	case KEYBOARD_KEY_F9: return VK_F9;
	case KEYBOARD_KEY_F10: return VK_F10;
	case KEYBOARD_KEY_F11: return VK_F11;
	case KEYBOARD_KEY_F12: return VK_F12;
	case KEYBOARD_KEY_PRINT_SCREEN: return VK_SNAPSHOT;
	case KEYBOARD_KEY_SCROLL_LOCK: return VK_SCROLL;
	case KEYBOARD_KEY_PAUSE: return VK_PAUSE;
	case KEYBOARD_KEY_GRAVE: return VK_OEM_3;
	case KEYBOARD_KEY_6: return '6';
	case KEYBOARD_KEY_7: return '7';
	case KEYBOARD_KEY_8: return '8';
	case KEYBOARD_KEY_9: return '9';
	case KEYBOARD_KEY_0: return '0';
	case KEYBOARD_KEY_MINUS: return VK_OEM_MINUS;
	case KEYBOARD_KEY_EQUAL: return VK_OEM_PLUS;
	case KEYBOARD_KEY_BACKSPACE: return VK_BACK;
	case KEYBOARD_KEY_T: return 'T';
	case KEYBOARD_KEY_Y: return 'Y';
	case KEYBOARD_KEY_U: return 'U';
	case KEYBOARD_KEY_I: return 'I';
	case KEYBOARD_KEY_O: return 'O';
	case KEYBOARD_KEY_P: return 'P';
	case KEYBOARD_KEY_LEFT_BRACKET: return VK_OEM_4;
	case KEYBOARD_KEY_RIGHT_BRACKET: return VK_OEM_6;
	case KEYBOARD_KEY_BACKSLASH: return VK_OEM_5;
	case KEYBOARD_KEY_G: return 'G';
	case KEYBOARD_KEY_H: return 'H';
	case KEYBOARD_KEY_J: return 'J';
	case KEYBOARD_KEY_K: return 'K';
	case KEYBOARD_KEY_L: return 'L';
	case KEYBOARD_KEY_SEMICOLON: return VK_OEM_1;
	case KEYBOARD_KEY_APOSTROPHE: return VK_OEM_7;
	case KEYBOARD_KEY_ENTER: return VK_RETURN;
	case KEYBOARD_KEY_Z: return 'Z';
	case KEYBOARD_KEY_X: return 'X';
	case KEYBOARD_KEY_C: return 'C';
	case KEYBOARD_KEY_V: return 'V';
	case KEYBOARD_KEY_B: return 'B';
	case KEYBOARD_KEY_N: return 'N';
	case KEYBOARD_KEY_M: return 'M';
	case KEYBOARD_KEY_COMMA: return VK_OEM_COMMA;
	case KEYBOARD_KEY_PERIOD: return VK_OEM_PERIOD;
	case KEYBOARD_KEY_SLASH: return VK_OEM_2;
	case KEYBOARD_KEY_RIGHT_SHIFT: return VK_RSHIFT;
	case KEYBOARD_KEY_LEFT_ALT: return VK_LMENU;
	case KEYBOARD_KEY_LEFT_WINDOWS: return VK_LWIN;
	case KEYBOARD_KEY_RIGHT_ALT: return VK_RMENU;
	case KEYBOARD_KEY_RIGHT_WINDOWS: return VK_RWIN;
	case KEYBOARD_KEY_MENU: return VK_APPS;
	case KEYBOARD_KEY_RIGHT_CTRL: return VK_RCONTROL;
	case KEYBOARD_KEY_INSERT: return VK_INSERT;
	case KEYBOARD_KEY_HOME: return VK_HOME;
	case KEYBOARD_KEY_PAGE_UP: return VK_PRIOR;
	case KEYBOARD_KEY_DELETE: return VK_DELETE;
	case KEYBOARD_KEY_END: return VK_END;
	case KEYBOARD_KEY_PAGE_DOWN: return VK_NEXT;
	case KEYBOARD_KEY_UP: return VK_UP;
	case KEYBOARD_KEY_LEFT: return VK_LEFT;
	case KEYBOARD_KEY_DOWN: return VK_DOWN;
	case KEYBOARD_KEY_RIGHT: return VK_RIGHT;
	case KEYBOARD_KEY_NUM_LOCK: return VK_NUMLOCK;
	case KEYBOARD_KEY_NUMPAD_DIVIDE: return VK_DIVIDE;
	case KEYBOARD_KEY_NUMPAD_MULTIPLY: return VK_MULTIPLY;
	case KEYBOARD_KEY_NUMPAD_SUBTRACT: return VK_SUBTRACT;
	case KEYBOARD_KEY_NUMPAD_7: return VK_NUMPAD7;
	case KEYBOARD_KEY_NUMPAD_8: return VK_NUMPAD8;
	case KEYBOARD_KEY_NUMPAD_9: return VK_NUMPAD9;
	case KEYBOARD_KEY_NUMPAD_ADD: return VK_ADD;
	case KEYBOARD_KEY_NUMPAD_4: return VK_NUMPAD4;
	case KEYBOARD_KEY_NUMPAD_5: return VK_NUMPAD5;
	case KEYBOARD_KEY_NUMPAD_6: return VK_NUMPAD6;
	case KEYBOARD_KEY_NUMPAD_1: return VK_NUMPAD1;
	case KEYBOARD_KEY_NUMPAD_2: return VK_NUMPAD2;
	case KEYBOARD_KEY_NUMPAD_3: return VK_NUMPAD3;
	case KEYBOARD_KEY_NUMPAD_0: return VK_NUMPAD0;
	case KEYBOARD_KEY_NUMPAD_DECIMAL: return VK_DECIMAL;
	case KEYBOARD_KEY_NUMPAD_ENTER: return VK_RETURN;
	default: return -1;
	}
}

static SRWLOCK capture_lock = SRWLOCK_INIT;
static bool physical_states[256];
static bool logical_states[KEYBOARD_KEY_COUNT];
static bool alias_states[4];
static long logical_press_sequences[KEYBOARD_KEY_COUNT];
static long alias_press_sequences[4];
struct raw_keyboard_state {
	HANDLE device;
	bool physical_states[256];
};
static struct raw_keyboard_state raw_keyboards[RAW_INPUT_DEVICE_MAX];
static HANDLE raw_input_thread;
static HANDLE raw_input_ready_event;
static DWORD raw_input_thread_id;
static void *volatile raw_input_window;
static volatile LONG raw_input_available;
static volatile LONG raw_input_registered;
static volatile LONG64 raw_input_last_event_time;

static bool key_down(int virtual_key)
{
	return (GetAsyncKeyState(virtual_key) & 0x8000) != 0;
}

static void update_logical_states(void)
{
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		const int virtual_key = keyboard_key_virtual_key((enum keyboard_overlay_key)i);
		const bool down = virtual_key >= 0 && physical_states[virtual_key];
		if (down && !logical_states[i])
			logical_press_sequences[i]++;
		logical_states[i] = down;
	}

	const int wasd_virtual_keys[4] = {'W', 'A', 'S', 'D'};
	const int arrow_virtual_keys[4] = {VK_UP, VK_LEFT, VK_DOWN, VK_RIGHT};
	for (size_t i = 0; i < 4; i++) {
		const bool alias_down = physical_states[wasd_virtual_keys[i]] || physical_states[arrow_virtual_keys[i]];
		if (alias_down && !alias_states[i])
			alias_press_sequences[i]++;
		alias_states[i] = alias_down;
	}
}

static void update_physical_key(unsigned int virtual_key, bool down)
{
	if (virtual_key >= 256 || physical_states[virtual_key] == down)
		return;
	physical_states[virtual_key] = down;
	update_logical_states();
}

static unsigned int keyboard_normalize_raw_virtual_key(const RAWKEYBOARD *keyboard)
{
	const bool extended = (keyboard->Flags & RI_KEY_E0) != 0;
	if (keyboard->VKey == VK_SHIFT)
		return keyboard->MakeCode == 0x36 ? VK_RSHIFT : VK_LSHIFT;
	if (keyboard->VKey == VK_CONTROL)
		return extended ? VK_RCONTROL : VK_LCONTROL;
	if (keyboard->VKey == VK_MENU)
		return extended ? VK_RMENU : VK_LMENU;
	return keyboard->VKey;
}

static void update_raw_key(HANDLE device, unsigned int virtual_key, bool down)
{
	if (virtual_key >= 256)
		return;

	AcquireSRWLockExclusive(&capture_lock);
	if (!device) {
		update_physical_key(virtual_key, down);
		ReleaseSRWLockExclusive(&capture_lock);
		return;
	}

	size_t slot = RAW_INPUT_DEVICE_MAX;
	size_t free_slot = RAW_INPUT_DEVICE_MAX;
	for (size_t i = 0; i < RAW_INPUT_DEVICE_MAX; i++) {
		if (raw_keyboards[i].device == device) {
			slot = i;
			break;
		}
		if (!raw_keyboards[i].device && free_slot == RAW_INPUT_DEVICE_MAX)
			free_slot = i;
	}
	if (slot == RAW_INPUT_DEVICE_MAX)
		slot = free_slot;
	if (slot == RAW_INPUT_DEVICE_MAX) {
		update_physical_key(virtual_key, down);
		ReleaseSRWLockExclusive(&capture_lock);
		return;
	}

	raw_keyboards[slot].device = device;
	raw_keyboards[slot].physical_states[virtual_key] = down;
	bool aggregate_down = false;
	for (size_t i = 0; i < RAW_INPUT_DEVICE_MAX && !aggregate_down; i++)
		aggregate_down = raw_keyboards[i].device && raw_keyboards[i].physical_states[virtual_key];
	update_physical_key(virtual_key, aggregate_down);
	ReleaseSRWLockExclusive(&capture_lock);
}

static void reset_pressed_states(void)
{
	AcquireSRWLockExclusive(&capture_lock);
	memset(physical_states, 0, sizeof(physical_states));
	memset(logical_states, 0, sizeof(logical_states));
	memset(alias_states, 0, sizeof(alias_states));
	memset(raw_keyboards, 0, sizeof(raw_keyboards));
	ReleaseSRWLockExclusive(&capture_lock);
}

static void reconcile_wasd_states(void)
{
	AcquireSRWLockExclusive(&capture_lock);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		const int virtual_key = keyboard_key_virtual_key((enum keyboard_overlay_key)i);
		if (virtual_key >= 0)
			update_physical_key((unsigned int)virtual_key, key_down(virtual_key));
	}
	update_physical_key(VK_UP, key_down(VK_UP));
	update_physical_key(VK_LEFT, key_down(VK_LEFT));
	update_physical_key(VK_DOWN, key_down(VK_DOWN));
	update_physical_key(VK_RIGHT, key_down(VK_RIGHT));
	ReleaseSRWLockExclusive(&capture_lock);
}

static void sample_fallback(void)
{
	reconcile_wasd_states();
}

static bool query_registered_raw_keyboard(RAWINPUTDEVICE *result, bool *found)
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
	const bool success = found_count != (UINT)-1;
	if (success) {
		for (UINT i = 0; i < found_count; i++) {
			if (devices[i].usUsagePage == 0x01 && devices[i].usUsage == 0x06) {
				*result = devices[i];
				*found = true;
				break;
			}
		}
	}
	HeapFree(GetProcessHeap(), 0, devices);
	return success;
}

static void refresh_raw_input_registration(HWND window)
{
	RAWINPUTDEVICE existing_device;
	bool existing_device_found;
	if (!query_registered_raw_keyboard(&existing_device, &existing_device_found))
		return;

	if (existing_device_found) {
		const bool owns_registration = existing_device.hwndTarget == window;
		InterlockedExchange(&raw_input_registered, owns_registration);
		if (!owns_registration && InterlockedExchange(&raw_input_available, 0)) {
			AcquireSRWLockExclusive(&capture_lock);
			memset(raw_keyboards, 0, sizeof(raw_keyboards));
			ReleaseSRWLockExclusive(&capture_lock);
			reconcile_wasd_states();
		} else if (owns_registration)
			InterlockedExchange(&raw_input_available, 1);
		return;
	}

	RAWINPUTDEVICE device = {0};
	device.usUsagePage = 0x01;
	device.usUsage = 0x06;
	device.dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
	device.hwndTarget = window;
	const bool registered = RegisterRawInputDevices(&device, 1, sizeof(device)) != FALSE;
	InterlockedExchange(&raw_input_registered, registered);
	InterlockedExchange(&raw_input_available, registered);
	if (registered) {
		AcquireSRWLockExclusive(&capture_lock);
		memset(raw_keyboards, 0, sizeof(raw_keyboards));
		ReleaseSRWLockExclusive(&capture_lock);
		reconcile_wasd_states();
	}
}

static LRESULT CALLBACK raw_input_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_INPUT) {
		RAWINPUT input;
		UINT size = sizeof(input);
		if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) == size &&
		    input.header.dwType == RIM_TYPEKEYBOARD && input.data.keyboard.VKey != 0xFF) {
			const bool down = (input.data.keyboard.Flags & RI_KEY_BREAK) == 0;
			InterlockedExchange64(&raw_input_last_event_time, (LONG64)GetTickCount64());
			update_raw_key(input.header.hDevice, keyboard_normalize_raw_virtual_key(&input.data.keyboard), down);
		}
		return DefWindowProcW(window, message, wparam, lparam);
	}
	if (message == WM_INPUT_DEVICE_CHANGE && wparam == GIDC_REMOVAL) {
		AcquireSRWLockExclusive(&capture_lock);
		for (size_t i = 0; i < RAW_INPUT_DEVICE_MAX; i++) {
			if (raw_keyboards[i].device == (HANDLE)lparam) {
				memset(&raw_keyboards[i], 0, sizeof(raw_keyboards[i]));
				break;
			}
		}
		ReleaseSRWLockExclusive(&capture_lock);
		reconcile_wasd_states();
		return 0;
	}
	if (message == WM_TIMER && wparam == RAW_INPUT_RECONCILE_TIMER) {
		if (InterlockedCompareExchange(&raw_input_available, 0, 0))
			reconcile_wasd_states();
		return 0;
	}
	if (message == WM_TIMER && wparam == RAW_INPUT_OWNERSHIP_TIMER) {
		refresh_raw_input_registration(window);
		return 0;
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
	(void)param;
	const HINSTANCE instance = GetModuleHandleW(NULL);
	WNDCLASSW window_class = {0};
	window_class.lpfnWndProc = raw_input_window_proc;
	window_class.hInstance = instance;
	window_class.lpszClassName = RAW_INPUT_WINDOW_CLASS;
	if (!RegisterClassW(&window_class)) {
		SetEvent(raw_input_ready_event);
		return 1;
	}

	const HWND window =
		CreateWindowExW(0, RAW_INPUT_WINDOW_CLASS, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, instance, NULL);
	InterlockedExchangePointer(&raw_input_window, window);
	if (window) {
		const UINT_PTR reconcile_timer =
			SetTimer(window, RAW_INPUT_RECONCILE_TIMER, RAW_INPUT_RECONCILE_MS, NULL);
		const UINT_PTR ownership_timer =
			SetTimer(window, RAW_INPUT_OWNERSHIP_TIMER, RAW_INPUT_OWNERSHIP_MS, NULL);
		if (reconcile_timer && ownership_timer) {
			refresh_raw_input_registration(window);
		} else {
			if (reconcile_timer)
				KillTimer(window, RAW_INPUT_RECONCILE_TIMER);
			if (ownership_timer)
				KillTimer(window, RAW_INPUT_OWNERSHIP_TIMER);
		}
	}
	SetEvent(raw_input_ready_event);

	bool running = true;
	while (running) {
		uint64_t last_raw_event;
		bool raw_event_stale;
		const DWORD wait_result = MsgWaitForMultipleObjectsEx(0, NULL, KEYBOARD_FALLBACK_POLL_MS, QS_ALLINPUT,
								      MWMO_INPUTAVAILABLE);
		if (wait_result == WAIT_TIMEOUT) {
			last_raw_event = (uint64_t)InterlockedCompareExchange64(&raw_input_last_event_time, 0, 0);
			raw_event_stale = !last_raw_event ||
					  GetTickCount64() - last_raw_event >= RAW_INPUT_EVENT_TIMEOUT_MS;
			if (!InterlockedCompareExchange(&raw_input_available, 0, 0) || raw_event_stale)
				reconcile_wasd_states();
			continue;
		}

		MSG message;
		while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT) {
				running = false;
				break;
			}
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
	}

	RAWINPUTDEVICE current_device;
	bool current_device_found;
	if (InterlockedCompareExchange(&raw_input_registered, 0, 0) &&
	    query_registered_raw_keyboard(&current_device, &current_device_found) && current_device_found &&
	    current_device.hwndTarget == window) {
		RAWINPUTDEVICE remove_device = {0};
		remove_device.usUsagePage = 0x01;
		remove_device.usUsage = 0x06;
		remove_device.dwFlags = RIDEV_REMOVE;
		RegisterRawInputDevices(&remove_device, 1, sizeof(remove_device));
	}
	InterlockedExchange(&raw_input_registered, 0);
	if (window && IsWindow(window)) {
		KillTimer(window, RAW_INPUT_RECONCILE_TIMER);
		KillTimer(window, RAW_INPUT_OWNERSHIP_TIMER);
		DestroyWindow(window);
	}
	InterlockedExchangePointer(&raw_input_window, NULL);
	InterlockedExchange(&raw_input_available, 0);
	reset_pressed_states();
	UnregisterClassW(RAW_INPUT_WINDOW_CLASS, instance);
	return 0;
}

bool keyboard_capture_initialize(void)
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

void keyboard_capture_shutdown(void)
{
	if (!raw_input_thread)
		return;
	const HWND window = InterlockedCompareExchangePointer(&raw_input_window, NULL, NULL);
	if (window) {
		PostMessageW(window, WM_CLOSE, 0, 0);
		PostThreadMessageW(raw_input_thread_id, WM_QUIT, 0, 0);
	} else if (WaitForSingleObject(raw_input_thread, 0) == WAIT_TIMEOUT) {
		PostThreadMessageW(raw_input_thread_id, WM_QUIT, 0, 0);
	}
	WaitForSingleObject(raw_input_thread, INFINITE);
	CloseHandle(raw_input_thread);
	raw_input_thread = NULL;
	raw_input_thread_id = 0;
	if (raw_input_ready_event) {
		CloseHandle(raw_input_ready_event);
		raw_input_ready_event = NULL;
	}
}

void keyboard_capture_sample(enum keyboard_capture_layout layout, bool arrow_aliases,
				     struct keyboard_capture_snapshot *snapshot)
{
	if (!InterlockedCompareExchange(&raw_input_available, 0, 0))
		sample_fallback();

	AcquireSRWLockShared(&capture_lock);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		if (layout == KEYBOARD_LAYOUT_WASD && i < 4 && arrow_aliases) {
			snapshot->pressed[i] = alias_states[i];
			snapshot->press_sequences[i] = alias_press_sequences[i];
		} else {
			snapshot->pressed[i] = logical_states[i];
			snapshot->press_sequences[i] = logical_press_sequences[i];
		}
	}
	ReleaseSRWLockShared(&capture_lock);
}
