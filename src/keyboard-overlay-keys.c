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
	"keyboard_character_w",
	"keyboard_character_a",
	"keyboard_character_s",
	"keyboard_character_d",
	"keyboard_character_space",
	"keyboard_character_shift",
	"keyboard_character_ctrl",
	"keyboard_character_q",
	"keyboard_character_e",
	"keyboard_character_r",
	"keyboard_character_f",
	"keyboard_character_tab",
	"keyboard_character_caps",
	"keyboard_character_1",
	"keyboard_character_2",
	"keyboard_character_3",
	"keyboard_character_4",
	"keyboard_character_5",
};

static const char *const default_characters[KEYBOARD_KEY_COUNT] = {
	"W", "A", "S", "D", "Space", "Shift", "Ctrl", "Q", "E", "R", "F", "Tab", "Caps", "1", "2", "3", "4", "5",
};
static const char *const key_character_labels[KEYBOARD_KEY_COUNT] = {
	"KeyboardCharacterW", "KeyboardCharacterA", "KeyboardCharacterS", "KeyboardCharacterD",
	"KeyboardCharacterSpace", "KeyboardCharacterShift", "KeyboardCharacterCtrl", "KeyboardCharacterQ",
	"KeyboardCharacterE", "KeyboardCharacterR", "KeyboardCharacterF", "KeyboardCharacterTab",
	"KeyboardCharacterCaps", "KeyboardCharacter1", "KeyboardCharacter2", "KeyboardCharacter3",
	"KeyboardCharacter4", "KeyboardCharacter5",
};
static const char *const arrows_default_characters[4] = {"Up", "Left", "Down", "Right"};
static const char *const numpad_default_characters[5] = {"1", "2", "3", "4", "5"};
static const char *const esdf_default_characters[KEYBOARD_KEY_COUNT] = {
	"E", "S", "D", "F", "Space", "Shift", "Ctrl", "Q", "E", "R", "F", "Tab", "Caps", "1", "2", "3", "4", "5",
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
	if (layout == KEYBOARD_LAYOUT_CUSTOM)
		return true;
	if (layout == KEYBOARD_LAYOUT_NUMPAD)
		return index >= KEYBOARD_KEY_1 && index <= KEYBOARD_KEY_5;
	return index < KEYBOARD_KEY_COUNT && (layout == KEYBOARD_LAYOUT_WASD || layout == KEYBOARD_LAYOUT_ESDF ||
						layout == KEYBOARD_LAYOUT_ARROWS) && index < 4;
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
	if (keyboard->layout_preset == KEYBOARD_LAYOUT_ESDF &&
	    !obs_data_has_user_value(settings, keyboard->keys[index].character_setting))
		return esdf_default_characters[index];
	if (keyboard->layout_preset == KEYBOARD_LAYOUT_ARROWS && index < 4 &&
	    !obs_data_has_user_value(settings, keyboard->keys[index].character_setting))
		return arrows_default_characters[index];
	if (keyboard->layout_preset == KEYBOARD_LAYOUT_NUMPAD && index >= KEYBOARD_KEY_1 && index <= KEYBOARD_KEY_5 &&
	    !obs_data_has_user_value(settings, keyboard->keys[index].character_setting))
		return numpad_default_characters[index - KEYBOARD_KEY_1];
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
	obs_data_set_default_double(settings, "keyboard_fade_duration", KEY_COLOR_FADE_DURATION_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_pulse_duration", KEY_PULSE_DURATION_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_rotation", KEY_ROTATION_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_tint_color", KEY_TINT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_font_color", KEY_FONT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_feedback", KEY_FEEDBACK_DEFAULT);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		obs_data_set_default_string(settings, key_character_settings[i], default_characters[i]);
	}
}

void keyboard_keys_add_properties(obs_properties_t *props)
{
	obs_properties_t *keyboard = obs_properties_create();
	obs_properties_add_group(props, "keyboard_enabled", obs_module_text("Keyboard"), OBS_GROUP_CHECKABLE, keyboard);
	obs_properties_add_color(keyboard, "keyboard_tint_color", obs_module_text("KeyboardTintColor"));
	obs_properties_add_font(keyboard, "keyboard_font", obs_module_text("KeyboardFont"));
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
	obs_properties_add_float_slider(keyboard, "keyboard_fade_duration", obs_module_text("KeyboardFadeDuration"),
					KEY_COLOR_FADE_DURATION_MIN, KEY_COLOR_FADE_DURATION_MAX, 0.01f);
	obs_properties_add_float_slider(keyboard, "keyboard_pulse_duration", obs_module_text("KeyboardPulseDuration"),
					KEY_PULSE_DURATION_MIN, KEY_PULSE_DURATION_MAX, 0.05f);
	obs_property_t *preset = obs_properties_add_list(keyboard, "keyboard_layout_preset",
								 obs_module_text("KeyboardLayoutPreset"), OBS_COMBO_TYPE_LIST,
								 OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(preset, obs_module_text("KeyboardLayoutWASD"), KEYBOARD_LAYOUT_WASD);
	obs_property_list_add_int(preset, obs_module_text("KeyboardLayoutESDF"), KEYBOARD_LAYOUT_ESDF);
	obs_property_list_add_int(preset, obs_module_text("KeyboardLayoutArrows"), KEYBOARD_LAYOUT_ARROWS);
	obs_property_list_add_int(preset, obs_module_text("KeyboardLayoutNumpad"), KEYBOARD_LAYOUT_NUMPAD);
	obs_property_list_add_int(preset, obs_module_text("KeyboardLayoutCustom"), KEYBOARD_LAYOUT_CUSTOM);
	obs_properties_add_color(keyboard, "keyboard_font_color", obs_module_text("KeyboardFontColor"));
	obs_properties_t *custom = obs_properties_create();
	obs_properties_add_group(keyboard, "keyboard_key_configuration", obs_module_text("KeyboardKeyConfiguration"),
				 OBS_GROUP_NORMAL, custom);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		obs_properties_add_text(custom, key_character_settings[i], obs_module_text(key_character_labels[i]),
					OBS_TEXT_DEFAULT);
	}

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
	keyboard->pulse_duration = (float)obs_data_get_double(settings, "keyboard_pulse_duration");
	keyboard->rotation_deg = (float)obs_data_get_double(settings, "keyboard_rotation");
	keyboard->tint_color = (uint32_t)obs_data_get_int(settings, "keyboard_tint_color");
	keyboard->font_color = obs_data_has_user_value(settings, "keyboard_font_color")
				       ? (uint32_t)obs_data_get_int(settings, "keyboard_font_color")
				       : KEY_FONT_COLOR_DEFAULT;
	keyboard->feedback = KEYBOARD_FEEDBACK_BOTH;
	if (keyboard->layout_preset < KEYBOARD_LAYOUT_WASD || keyboard->layout_preset >= KEYBOARD_LAYOUT_COUNT)
		keyboard->layout_preset = KEYBOARD_LAYOUT_WASD;
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		keyboard->keys[i].visible = keyboard_key_is_visible_by_preset(keyboard->layout_preset, i);
		keyboard->keys[i].normalized_x = (float)(i % 5) / 4.0f;
		keyboard->keys[i].normalized_y = (float)(i / 5) / 3.0f;
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
	if (!isfinite(keyboard->pulse_duration))
		keyboard->pulse_duration = KEY_PULSE_DURATION_DEFAULT;
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
