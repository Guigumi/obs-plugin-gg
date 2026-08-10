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

static const int default_virtual_keys[KEYBOARD_KEY_COUNT] = {
	'W', 'A', 'S', 'D', VK_SPACE, VK_LSHIFT, VK_LCONTROL, 'Q', 'E', 'R', 'F', VK_TAB, VK_CAPITAL,
	'1', '2', '3', '4', '5',
};
static const int esdf_virtual_keys[KEYBOARD_KEY_COUNT] = {
	'E', 'S', 'D', 'F', VK_SPACE, VK_LSHIFT, VK_LCONTROL, 'Q', 'E', 'R', 'F', VK_TAB, VK_CAPITAL,
	'1', '2', '3', '4', '5',
};
static const int arrow_virtual_keys[KEYBOARD_KEY_COUNT] = {VK_UP, VK_LEFT, VK_DOWN, VK_RIGHT};
static const int numpad_virtual_keys[KEYBOARD_KEY_COUNT] = {VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5};

static SRWLOCK capture_lock = SRWLOCK_INIT;
static bool physical_states[256];
static bool wasd_states[KEYBOARD_KEY_COUNT];
static bool esdf_states[KEYBOARD_KEY_COUNT];
static bool alias_states[KEYBOARD_KEY_COUNT];
static bool numpad_states[KEYBOARD_KEY_COUNT];
static long wasd_press_sequences[KEYBOARD_KEY_COUNT];
static long esdf_press_sequences[KEYBOARD_KEY_COUNT];
static long alias_press_sequences[KEYBOARD_KEY_COUNT];
static long numpad_press_sequences[KEYBOARD_KEY_COUNT];
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
		const bool wasd_down = physical_states[default_virtual_keys[i]];
		const bool esdf_down = physical_states[esdf_virtual_keys[i]];
		const bool alias_down = wasd_down || physical_states[arrow_virtual_keys[i]];
		const bool numpad_down = i < 5 && physical_states[numpad_virtual_keys[i]];
		if (wasd_down && !wasd_states[i])
			wasd_press_sequences[i]++;
		if (esdf_down && !esdf_states[i])
			esdf_press_sequences[i]++;
		if (alias_down && !alias_states[i])
			alias_press_sequences[i]++;
		if (numpad_down && !numpad_states[i])
			numpad_press_sequences[i]++;
		wasd_states[i] = wasd_down;
		esdf_states[i] = esdf_down;
		alias_states[i] = alias_down;
		numpad_states[i] = numpad_down;
	}
}

static void update_physical_key(unsigned int virtual_key, bool down)
{
	if (virtual_key >= 256 || physical_states[virtual_key] == down)
		return;
	physical_states[virtual_key] = down;
	update_logical_states();
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
	memset(wasd_states, 0, sizeof(wasd_states));
	memset(esdf_states, 0, sizeof(esdf_states));
	memset(alias_states, 0, sizeof(alias_states));
	memset(numpad_states, 0, sizeof(numpad_states));
	memset(raw_keyboards, 0, sizeof(raw_keyboards));
	ReleaseSRWLockExclusive(&capture_lock);
}

static void reconcile_wasd_states(void)
{
	AcquireSRWLockExclusive(&capture_lock);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		update_physical_key((unsigned int)default_virtual_keys[i], key_down(default_virtual_keys[i]));
		update_physical_key((unsigned int)esdf_virtual_keys[i], key_down(esdf_virtual_keys[i]));
		update_physical_key((unsigned int)arrow_virtual_keys[i], key_down(arrow_virtual_keys[i]));
		if (i < 5)
			update_physical_key((unsigned int)numpad_virtual_keys[i], key_down(numpad_virtual_keys[i]));
	}
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
			update_raw_key(input.header.hDevice, input.data.keyboard.VKey, down);
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
		if (layout == KEYBOARD_LAYOUT_ESDF) {
			snapshot->pressed[i] = esdf_states[i];
			snapshot->press_sequences[i] = esdf_press_sequences[i];
		} else if (layout == KEYBOARD_LAYOUT_ARROWS) {
			snapshot->pressed[i] = i < 4 && physical_states[arrow_virtual_keys[i]];
			snapshot->press_sequences[i] = alias_press_sequences[i];
		} else if (layout == KEYBOARD_LAYOUT_NUMPAD) {
			snapshot->pressed[i] = numpad_states[i];
			snapshot->press_sequences[i] = numpad_press_sequences[i];
		} else {
			snapshot->pressed[i] = arrow_aliases ? alias_states[i] : wasd_states[i];
			snapshot->press_sequences[i] = arrow_aliases ? alias_press_sequences[i] : wasd_press_sequences[i];
		}
	}
	ReleaseSRWLockShared(&capture_lock);
}
