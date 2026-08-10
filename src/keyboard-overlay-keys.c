#include "keyboard-overlay-keys.h"

#include "keyboard-capture.h"

#include <util/platform.h>

#include <float.h>
#include <math.h>
#include <string.h>

#define KEY_SIZE_DEFAULT 64.0f
#define KEY_SIZE_MIN 8.0f
#define KEY_SIZE_MAX 256.0f
#define KEY_SPACING_DEFAULT 8.0f
#define KEY_SPACING_MIN 0.0f
#define KEY_SPACING_MAX 64.0f
#define KEY_IDLE_OPACITY_DEFAULT 35.0f
#define KEY_ACTIVE_OPACITY_DEFAULT 100.0f
#define KEY_PULSE_DURATION_DEFAULT 0.35f
#define KEY_PULSE_DURATION_MIN 0.1f
#define KEY_PULSE_DURATION_MAX 2.0f
#define KEY_COLOR_FADE_DURATION_DEFAULT 0.03f
#define KEY_COLOR_FADE_DURATION_MIN 0.01f
#define KEY_COLOR_FADE_DURATION_MAX 0.50f
#define KEY_QUICK_PRESS_DISPLAY_SECONDS 0.10f
#define KEY_ROTATION_DEFAULT 0.0f
#define KEY_ROTATION_MIN -180.0f
#define KEY_ROTATION_MAX 180.0f
#define KEY_TINT_COLOR_DEFAULT 0xFFFFFF
#define KEY_FONT_COLOR_DEFAULT 0xFFFFFF
#define KEY_LABEL_FONT_SIZE_DEFAULT 36
#define KEY_FEEDBACK_DEFAULT KEYBOARD_FEEDBACK_BOTH

static const char *const key_character_settings[KEYBOARD_KEY_COUNT] = {
	"keyboard_character_w", "keyboard_character_a", "keyboard_character_s", "keyboard_character_d",
	"keyboard_character_space", "keyboard_character_shift", "keyboard_character_ctrl", "keyboard_character_q",
	"keyboard_character_e", "keyboard_character_r", "keyboard_character_f", "keyboard_character_tab",
	"keyboard_character_caps", "keyboard_character_1", "keyboard_character_2", "keyboard_character_3",
	"keyboard_character_4", "keyboard_character_5", "keyboard_character_escape", "keyboard_character_f1",
	"keyboard_character_f2", "keyboard_character_f3", "keyboard_character_f4", "keyboard_character_f5",
	"keyboard_character_f6", "keyboard_character_f7", "keyboard_character_f8", "keyboard_character_f9",
	"keyboard_character_f10", "keyboard_character_f11", "keyboard_character_f12", "keyboard_character_print_screen",
	"keyboard_character_scroll_lock", "keyboard_character_pause", "keyboard_character_grave", "keyboard_character_6",
	"keyboard_character_7", "keyboard_character_8", "keyboard_character_9", "keyboard_character_0",
	"keyboard_character_minus", "keyboard_character_equal", "keyboard_character_backspace", "keyboard_character_t",
	"keyboard_character_y", "keyboard_character_u", "keyboard_character_i", "keyboard_character_o",
	"keyboard_character_p", "keyboard_character_left_bracket", "keyboard_character_right_bracket",
	"keyboard_character_backslash", "keyboard_character_g", "keyboard_character_h", "keyboard_character_j",
	"keyboard_character_k", "keyboard_character_l", "keyboard_character_semicolon", "keyboard_character_apostrophe",
	"keyboard_character_enter", "keyboard_character_z", "keyboard_character_x", "keyboard_character_c",
	"keyboard_character_v", "keyboard_character_b", "keyboard_character_n", "keyboard_character_m",
	"keyboard_character_comma", "keyboard_character_period", "keyboard_character_slash",
	"keyboard_character_right_shift", "keyboard_character_left_alt", "keyboard_character_left_windows",
	"keyboard_character_right_alt", "keyboard_character_right_windows", "keyboard_character_menu",
	"keyboard_character_right_ctrl", "keyboard_character_insert", "keyboard_character_home",
	"keyboard_character_page_up", "keyboard_character_delete", "keyboard_character_end", "keyboard_character_page_down",
	"keyboard_character_up", "keyboard_character_left", "keyboard_character_down", "keyboard_character_right",
	"keyboard_character_num_lock", "keyboard_character_numpad_divide", "keyboard_character_numpad_multiply",
	"keyboard_character_numpad_subtract", "keyboard_character_numpad_7", "keyboard_character_numpad_8",
	"keyboard_character_numpad_9", "keyboard_character_numpad_add", "keyboard_character_numpad_4",
	"keyboard_character_numpad_5", "keyboard_character_numpad_6", "keyboard_character_numpad_1",
	"keyboard_character_numpad_2", "keyboard_character_numpad_3", "keyboard_character_numpad_0",
	"keyboard_character_numpad_decimal", "keyboard_character_numpad_enter",
};

static const char *const default_characters[KEYBOARD_KEY_COUNT] = {
	"W", "A", "S", "D", "Space", "Shift", "Ctrl", "Q", "E", "R", "F", "Tab", "Caps", "1", "2", "3", "4", "5",
	"Esc", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "PrtSc", "Scroll", "Pause",
	"`", "6", "7", "8", "9", "0", "-", "=", "Backspace", "T", "Y", "U", "I", "O", "P", "[", "]", "\\",
	"G", "H", "J", "K", "L", ";", "'", "Enter", "Z", "X", "C", "V", "B", "N", "M", ",", ".", "/", "RShift",
	"LAlt", "LWin", "RAlt", "RWin", "Menu", "RCtrl", "Ins", "Home", "PgUp", "Del", "End", "PgDn", "Up", "Left", "Down", "Right",
	"NumLock", "/", "*", "-", "7", "8", "9", "+", "4", "5", "6", "1", "2", "3", "0", ".", "Enter",
};
static const char *const key_character_labels[KEYBOARD_KEY_COUNT] = {
	"KeyboardCharacterW", "KeyboardCharacterA", "KeyboardCharacterS", "KeyboardCharacterD",
	"KeyboardCharacterSpace", "KeyboardCharacterShift", "KeyboardCharacterCtrl", "KeyboardCharacterQ",
	"KeyboardCharacterE", "KeyboardCharacterR", "KeyboardCharacterF", "KeyboardCharacterTab",
	"KeyboardCharacterCaps", "KeyboardCharacter1", "KeyboardCharacter2", "KeyboardCharacter3",
	"KeyboardCharacter4", "KeyboardCharacter5", "KeyboardCharacterEscape", "KeyboardCharacterF1",
	"KeyboardCharacterF2", "KeyboardCharacterF3", "KeyboardCharacterF4", "KeyboardCharacterF5",
	"KeyboardCharacterF6", "KeyboardCharacterF7", "KeyboardCharacterF8", "KeyboardCharacterF9",
	"KeyboardCharacterF10", "KeyboardCharacterF11", "KeyboardCharacterF12", "KeyboardCharacterPrintScreen",
	"KeyboardCharacterScrollLock", "KeyboardCharacterPause", "KeyboardCharacterGrave", "KeyboardCharacter6",
	"KeyboardCharacter7", "KeyboardCharacter8", "KeyboardCharacter9", "KeyboardCharacter0",
	"KeyboardCharacterMinus", "KeyboardCharacterEqual", "KeyboardCharacterBackspace", "KeyboardCharacterT",
	"KeyboardCharacterY", "KeyboardCharacterU", "KeyboardCharacterI", "KeyboardCharacterO",
	"KeyboardCharacterP", "KeyboardCharacterLeftBracket", "KeyboardCharacterRightBracket",
	"KeyboardCharacterBackslash", "KeyboardCharacterG", "KeyboardCharacterH", "KeyboardCharacterJ",
	"KeyboardCharacterK", "KeyboardCharacterL", "KeyboardCharacterSemicolon", "KeyboardCharacterApostrophe",
	"KeyboardCharacterEnter", "KeyboardCharacterZ", "KeyboardCharacterX", "KeyboardCharacterC",
	"KeyboardCharacterV", "KeyboardCharacterB", "KeyboardCharacterN", "KeyboardCharacterM",
	"KeyboardCharacterComma", "KeyboardCharacterPeriod", "KeyboardCharacterSlash", "KeyboardCharacterRightShift",
	"KeyboardCharacterLeftAlt", "KeyboardCharacterLeftWindows", "KeyboardCharacterRightAlt",
	"KeyboardCharacterRightWindows", "KeyboardCharacterMenu", "KeyboardCharacterRightCtrl",
	"KeyboardCharacterInsert", "KeyboardCharacterHome", "KeyboardCharacterPageUp", "KeyboardCharacterDelete",
	"KeyboardCharacterEnd", "KeyboardCharacterPageDown", "KeyboardCharacterUp", "KeyboardCharacterLeft",
	"KeyboardCharacterDown", "KeyboardCharacterRight", "KeyboardCharacterNumLock", "KeyboardCharacterNumpadDivide",
	"KeyboardCharacterNumpadMultiply", "KeyboardCharacterNumpadSubtract", "KeyboardCharacterNumpad7",
	"KeyboardCharacterNumpad8", "KeyboardCharacterNumpad9", "KeyboardCharacterNumpadAdd", "KeyboardCharacterNumpad4",
	"KeyboardCharacterNumpad5", "KeyboardCharacterNumpad6", "KeyboardCharacterNumpad1", "KeyboardCharacterNumpad2",
	"KeyboardCharacterNumpad3", "KeyboardCharacterNumpad0", "KeyboardCharacterNumpadDecimal",
	"KeyboardCharacterNumpadEnter",
};

static void keyboard_keys_sync_capture(struct keyboard_overlay_gg_data *keyboard)
{
	struct keyboard_capture_snapshot snapshot;
	keyboard_capture_sample(keyboard->layout_preset, keyboard->arrow_aliases, &snapshot);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		struct keyboard_overlay_key_data *key = &keyboard->keys[i];
		key->press_sequence = snapshot.press_sequences[key->capture_key];
	}
	keyboard->capture_snapshot_initialized = true;
}

static bool keyboard_key_is_visible_by_preset(enum keyboard_capture_layout layout, size_t index)
{
	if (layout == KEYBOARD_LAYOUT_WASD)
		return index < 4;
	if (layout == KEYBOARD_LAYOUT_100)
		return index < KEYBOARD_KEY_PRINT_SCREEN || (index >= KEYBOARD_KEY_GRAVE && index < KEYBOARD_KEY_INSERT);
	if (layout == KEYBOARD_LAYOUT_EDITING)
		return (index >= KEYBOARD_KEY_PRINT_SCREEN && index <= KEYBOARD_KEY_PAUSE) ||
		       (index >= KEYBOARD_KEY_INSERT && index <= KEYBOARD_KEY_RIGHT);
	return index >= KEYBOARD_KEY_NUM_LOCK;
}

void keyboard_keys_initialize(struct keyboard_overlay_gg_data *keyboard)
{
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		keyboard->keys[i].capture_key = (enum keyboard_overlay_key)i;
		keyboard->keys[i].character_setting = key_character_settings[i];
		keyboard->keys[i].visible = true;
	}
}

const char *keyboard_keys_get_character(const struct keyboard_overlay_gg_data *keyboard, obs_data_t *settings,
					 size_t index)
{
	if (index >= KEYBOARD_KEY_COUNT)
		return "";
	const char *configured = obs_data_get_string(settings, keyboard->keys[index].character_setting);
	if (!obs_data_has_user_value(settings, keyboard->keys[index].character_setting))
		return default_characters[index];
	return configured ? configured : "";
}

void keyboard_keys_defaults(obs_data_t *settings)
{
	obs_data_t *font = obs_data_create();
	obs_data_set_default_string(font, "face", "Arial");
	obs_data_set_default_int(font, "size", KEY_LABEL_FONT_SIZE_DEFAULT);
	obs_data_set_default_int(font, "flags", OBS_FONT_BOLD);
	obs_data_set_default_obj(settings, "keyboard_font", font);
	obs_data_release(font);

	obs_data_set_default_bool(settings, "keyboard_enabled", true);
	obs_data_set_default_bool(settings, "keyboard_arrow_aliases", true);
	obs_data_set_default_int(settings, "keyboard_layout_preset", KEYBOARD_LAYOUT_WASD);
	obs_data_set_default_double(settings, "keyboard_key_size", KEY_SIZE_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_spacing", KEY_SPACING_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_idle_opacity", KEY_IDLE_OPACITY_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_active_opacity", KEY_ACTIVE_OPACITY_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_rotation", KEY_ROTATION_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_tint_color", KEY_TINT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_font_color", KEY_FONT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_feedback", KEY_FEEDBACK_DEFAULT);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		obs_data_set_default_string(settings, key_character_settings[i], default_characters[i]);
	}
}

static bool keyboard_layout_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	const bool wasd = obs_data_get_int(settings, "keyboard_layout_preset") == KEYBOARD_LAYOUT_WASD;
	const enum keyboard_capture_layout layout =
		(enum keyboard_capture_layout)obs_data_get_int(settings, "keyboard_layout_preset");
	obs_property_t *wasd_configuration = obs_properties_get(props, "keyboard_key_configuration_wasd");
	obs_property_t *full_configuration = obs_properties_get(props, "keyboard_key_configuration_100");
	obs_property_t *editing_configuration = obs_properties_get(props, "keyboard_key_configuration_editing");
	obs_property_t *numpad_configuration = obs_properties_get(props, "keyboard_key_configuration_numpad");
	if (wasd_configuration)
		obs_property_set_visible(wasd_configuration, wasd);
	if (full_configuration)
		obs_property_set_visible(full_configuration, layout == KEYBOARD_LAYOUT_100);
	if (editing_configuration)
		obs_property_set_visible(editing_configuration, layout == KEYBOARD_LAYOUT_EDITING);
	if (numpad_configuration)
		obs_property_set_visible(numpad_configuration, layout == KEYBOARD_LAYOUT_NUMPAD);
	return true;
}

void keyboard_keys_add_properties(obs_properties_t *props, enum keyboard_capture_layout initial_layout)
{
	obs_properties_t *keyboard = obs_properties_create();
	obs_properties_add_group(props, "keyboard_enabled", obs_module_text("Keyboard"), OBS_GROUP_CHECKABLE, keyboard);
	obs_property_t *preset = obs_properties_add_list(keyboard, "keyboard_layout_preset",
								 obs_module_text("KeyboardLayoutPreset"), OBS_COMBO_TYPE_LIST,
								 OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(preset, obs_module_text("KeyboardLayoutWASD"), KEYBOARD_LAYOUT_WASD);
	obs_property_list_add_int(preset, obs_module_text("KeyboardLayout100"), KEYBOARD_LAYOUT_100);
	obs_property_list_add_int(preset, obs_module_text("KeyboardLayoutEditing"), KEYBOARD_LAYOUT_EDITING);
	obs_property_list_add_int(preset, obs_module_text("KeyboardLayoutNumpad"), KEYBOARD_LAYOUT_NUMPAD);
	obs_property_set_modified_callback(preset, keyboard_layout_modified);
	obs_properties_add_float_slider(keyboard, "keyboard_key_size", obs_module_text("KeyboardKeySize"), KEY_SIZE_MIN,
					KEY_SIZE_MAX, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_spacing", obs_module_text("KeyboardSpacing"),
					KEY_SPACING_MIN, KEY_SPACING_MAX, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_rotation", obs_module_text("KeyboardRotation"),
					KEY_ROTATION_MIN, KEY_ROTATION_MAX, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_idle_opacity", obs_module_text("KeyboardIdleOpacity"), 0.0f,
					100.0f, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_active_opacity", obs_module_text("KeyboardActiveOpacity"),
					0.0f, 100.0f, 1.0f);
	obs_properties_add_color(keyboard, "keyboard_tint_color", obs_module_text("KeyboardTintColor"));
	obs_properties_add_font(keyboard, "keyboard_font", obs_module_text("KeyboardFont"));
	obs_properties_add_color(keyboard, "keyboard_font_color", obs_module_text("KeyboardFontColor"));
	obs_properties_t *wasd_configuration = obs_properties_create();
	obs_property_t *wasd_group = obs_properties_add_group(keyboard, "keyboard_key_configuration_wasd",
								     obs_module_text("KeyboardKeyConfigurationWASD"),
								     OBS_GROUP_NORMAL, wasd_configuration);
	for (size_t i = 0; i < 4; i++) {
		obs_properties_add_text(wasd_configuration, key_character_settings[i], obs_module_text(key_character_labels[i]),
					OBS_TEXT_DEFAULT);
	}
	obs_properties_t *full_configuration = obs_properties_create();
	obs_property_t *full_group = obs_properties_add_group(keyboard, "keyboard_key_configuration_100",
								      obs_module_text("KeyboardKeyConfiguration100"),
								      OBS_GROUP_NORMAL, full_configuration);
	for (size_t i = 0; i < KEYBOARD_KEY_PRINT_SCREEN; i++) {
		obs_properties_add_text(full_configuration, key_character_settings[i], obs_module_text(key_character_labels[i]),
					OBS_TEXT_DEFAULT);
	}
	for (size_t i = KEYBOARD_KEY_GRAVE; i < KEYBOARD_KEY_INSERT; i++) {
		obs_properties_add_text(full_configuration, key_character_settings[i], obs_module_text(key_character_labels[i]),
					OBS_TEXT_DEFAULT);
	}
	obs_properties_t *editing_configuration = obs_properties_create();
	obs_property_t *editing_group = obs_properties_add_group(keyboard, "keyboard_key_configuration_editing",
								       obs_module_text("KeyboardKeyConfigurationEditing"),
								       OBS_GROUP_NORMAL, editing_configuration);
	for (size_t i = KEYBOARD_KEY_PRINT_SCREEN; i <= KEYBOARD_KEY_PAUSE; i++) {
		obs_properties_add_text(editing_configuration, key_character_settings[i], obs_module_text(key_character_labels[i]),
					OBS_TEXT_DEFAULT);
	}
	for (size_t i = KEYBOARD_KEY_INSERT; i <= KEYBOARD_KEY_RIGHT; i++) {
		obs_properties_add_text(editing_configuration, key_character_settings[i], obs_module_text(key_character_labels[i]),
					OBS_TEXT_DEFAULT);
	}
	obs_properties_t *numpad_configuration = obs_properties_create();
	obs_property_t *numpad_group = obs_properties_add_group(keyboard, "keyboard_key_configuration_numpad",
								      obs_module_text("KeyboardKeyConfigurationNumpad"),
								      OBS_GROUP_NORMAL, numpad_configuration);
	for (size_t i = KEYBOARD_KEY_NUM_LOCK; i < KEYBOARD_KEY_COUNT; i++) {
		obs_properties_add_text(numpad_configuration, key_character_settings[i], obs_module_text(key_character_labels[i]),
					OBS_TEXT_DEFAULT);
	}
	obs_property_set_visible(wasd_group, initial_layout == KEYBOARD_LAYOUT_WASD);
	obs_property_set_visible(full_group, initial_layout == KEYBOARD_LAYOUT_100);
	obs_property_set_visible(editing_group, initial_layout == KEYBOARD_LAYOUT_EDITING);
	obs_property_set_visible(numpad_group, initial_layout == KEYBOARD_LAYOUT_NUMPAD);
}

void keyboard_keys_update(struct keyboard_overlay_gg_data *keyboard, obs_data_t *settings)
{
	const bool arrow_aliases_changed = !keyboard->arrow_aliases;
	const enum keyboard_capture_layout configured_layout =
		(enum keyboard_capture_layout)obs_data_get_int(settings, "keyboard_layout_preset");
	const bool layout_changed = keyboard->layout_preset != configured_layout;
	keyboard->enabled = obs_data_get_bool(settings, "keyboard_enabled");
	keyboard->arrow_aliases = true;
	keyboard->layout_preset = configured_layout;
	keyboard->key_size = (float)obs_data_get_double(settings, "keyboard_key_size");
	keyboard->spacing = (float)obs_data_get_double(settings, "keyboard_spacing");
	keyboard->font_auto_size = true;
	keyboard->idle_opacity_pct = (float)obs_data_get_double(settings, "keyboard_idle_opacity");
	keyboard->active_opacity_pct = (float)obs_data_get_double(settings, "keyboard_active_opacity");
	const double configured_fade_duration = obs_data_get_double(settings, "keyboard_fade_duration");
	const double configured_pulse_duration = obs_data_get_double(settings, "keyboard_pulse_duration");
	keyboard->rotation_deg = (float)obs_data_get_double(settings, "keyboard_rotation");
	keyboard->tint_color = (uint32_t)obs_data_get_int(settings, "keyboard_tint_color");
	keyboard->feedback = KEYBOARD_FEEDBACK_BOTH;
	if (keyboard->layout_preset < KEYBOARD_LAYOUT_WASD || keyboard->layout_preset >= KEYBOARD_LAYOUT_COUNT)
		keyboard->layout_preset = KEYBOARD_LAYOUT_WASD;
	keyboard->font_color = obs_data_has_user_value(settings, "keyboard_font_color")
				       ? (uint32_t)obs_data_get_int(settings, "keyboard_font_color")
				       : KEY_FONT_COLOR_DEFAULT;
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		keyboard->keys[i].visible = keyboard_key_is_visible_by_preset(keyboard->layout_preset, i);
		keyboard->keys[i].normalized_x = 0.0f;
		keyboard->keys[i].normalized_y = 0.0f;
	}

	if (!isfinite(keyboard->key_size))
		keyboard->key_size = KEY_SIZE_DEFAULT;
	keyboard->key_size = fminf(fmaxf(keyboard->key_size, KEY_SIZE_MIN), KEY_SIZE_MAX);
	if (!isfinite(keyboard->spacing))
		keyboard->spacing = KEY_SPACING_DEFAULT;
	keyboard->spacing = fminf(fmaxf(keyboard->spacing, KEY_SPACING_MIN), KEY_SPACING_MAX);
	if (!isfinite(keyboard->idle_opacity_pct))
		keyboard->idle_opacity_pct = KEY_IDLE_OPACITY_DEFAULT;
	keyboard->idle_opacity_pct = fminf(fmaxf(keyboard->idle_opacity_pct, 0.0f), 100.0f);
	if (!isfinite(keyboard->active_opacity_pct))
		keyboard->active_opacity_pct = KEY_ACTIVE_OPACITY_DEFAULT;
	keyboard->active_opacity_pct = fminf(fmaxf(keyboard->active_opacity_pct, 0.0f), 100.0f);
	if (!obs_data_has_user_value(settings, "keyboard_fade_duration") || !isfinite(configured_fade_duration) ||
	    configured_fade_duration > FLT_MAX)
		keyboard->fade_duration = KEY_COLOR_FADE_DURATION_DEFAULT;
	else
		keyboard->fade_duration = (float)configured_fade_duration;
	keyboard->fade_duration =
		fminf(fmaxf(keyboard->fade_duration, KEY_COLOR_FADE_DURATION_MIN), KEY_COLOR_FADE_DURATION_MAX);
	if (!obs_data_has_user_value(settings, "keyboard_pulse_duration") || !isfinite(configured_pulse_duration) ||
	    configured_pulse_duration > FLT_MAX)
		keyboard->pulse_duration = KEY_PULSE_DURATION_DEFAULT;
	else
		keyboard->pulse_duration = (float)configured_pulse_duration;
	keyboard->pulse_duration =
		fminf(fmaxf(keyboard->pulse_duration, KEY_PULSE_DURATION_MIN), KEY_PULSE_DURATION_MAX);
	if (!isfinite(keyboard->rotation_deg))
		keyboard->rotation_deg = KEY_ROTATION_DEFAULT;
	keyboard->rotation_deg = fminf(fmaxf(keyboard->rotation_deg, KEY_ROTATION_MIN), KEY_ROTATION_MAX);
	if (arrow_aliases_changed || layout_changed)
		keyboard->capture_snapshot_initialized = false;
	if (!keyboard->enabled) {
		for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
			keyboard->keys[i].pressed = false;
			keyboard->keys[i].press_sequence = 0;
			keyboard->keys[i].color_level = 0.0f;
			keyboard->keys[i].press_time_ns = 0;
		}
		keyboard->capture_snapshot_initialized = false;
	} else if (!keyboard->capture_snapshot_initialized)
		keyboard_keys_sync_capture(keyboard);
}

void keyboard_keys_tick(struct keyboard_overlay_gg_data *keyboard, float seconds)
{
	if (!keyboard->enabled)
		return;

	struct keyboard_capture_snapshot snapshot;
	keyboard_capture_sample(keyboard->layout_preset, keyboard->arrow_aliases, &snapshot);
	const uint64_t now_ns = os_gettime_ns();
	const float fade_step = fminf(seconds / keyboard->fade_duration, 1.0f);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		struct keyboard_overlay_key_data *key = &keyboard->keys[i];
		const enum keyboard_overlay_key capture_key = key->capture_key;
		const bool new_press = keyboard->capture_snapshot_initialized &&
				       (snapshot.press_sequences[capture_key] != key->press_sequence ||
					snapshot.pressed[capture_key] && !key->pressed);
		if (new_press) {
			key->press_time_ns = now_ns;
			key->color_level = 1.0f;
		}
		if (snapshot.pressed[capture_key])
			key->color_level = 1.0f;
		else if (!new_press) {
			const uint64_t age_ns = key->press_time_ns ? now_ns - key->press_time_ns : UINT64_MAX;
			if (age_ns >= (uint64_t)(KEY_QUICK_PRESS_DISPLAY_SECONDS * 1e9f))
				key->color_level = fmaxf(key->color_level - fade_step, 0.0f);
		}
		key->pressed = snapshot.pressed[capture_key];
		key->press_sequence = snapshot.press_sequences[capture_key];
	}
	keyboard->capture_snapshot_initialized = true;
}
