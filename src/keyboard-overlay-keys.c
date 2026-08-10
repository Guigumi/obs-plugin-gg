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
#define KEY_LABEL_FONT_SIZE_DEFAULT 36
#define KEY_FEEDBACK_DEFAULT KEYBOARD_FEEDBACK_BOTH

static const char *const key_character_settings[KEYBOARD_KEY_COUNT] = {
	"keyboard_character_w",
	"keyboard_character_a",
	"keyboard_character_s",
	"keyboard_character_d",
};

static const char *const key_visibility_settings[KEYBOARD_KEY_COUNT] = {
	"keyboard_visible_w",
	"keyboard_visible_a",
	"keyboard_visible_s",
	"keyboard_visible_d",
};

static const char *const key_visibility_labels[KEYBOARD_KEY_COUNT] = {
	"KeyboardVisibleW",
	"KeyboardVisibleA",
	"KeyboardVisibleS",
	"KeyboardVisibleD",
};

static void keyboard_keys_sync_capture(struct keyboard_overlay_gg_data *keyboard)
{
	struct keyboard_capture_snapshot snapshot;
	keyboard_capture_sample_wasd(keyboard->arrow_aliases, &snapshot);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		struct keyboard_overlay_key_data *key = &keyboard->keys[i];
		key->press_sequence = snapshot.press_sequences[key->capture_key];
	}
	keyboard->capture_snapshot_initialized = true;
}

void keyboard_keys_initialize(struct keyboard_overlay_gg_data *keyboard)
{
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		keyboard->keys[i].capture_key = (enum keyboard_overlay_key)i;
		keyboard->keys[i].character_setting = key_character_settings[i];
		keyboard->keys[i].visible = true;
	}
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
	obs_data_set_default_double(settings, "keyboard_key_size", KEY_SIZE_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_spacing", KEY_SPACING_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_idle_opacity", KEY_IDLE_OPACITY_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_active_opacity", KEY_ACTIVE_OPACITY_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_fade_duration", KEY_COLOR_FADE_DURATION_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_pulse_duration", KEY_PULSE_DURATION_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_rotation", KEY_ROTATION_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_tint_color", KEY_TINT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_feedback", KEY_FEEDBACK_DEFAULT);
	obs_data_set_default_string(settings, "keyboard_character_w", "W");
	obs_data_set_default_string(settings, "keyboard_character_a", "A");
	obs_data_set_default_string(settings, "keyboard_character_s", "S");
	obs_data_set_default_string(settings, "keyboard_character_d", "D");
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++)
		obs_data_set_default_bool(settings, key_visibility_settings[i], true);
}

void keyboard_keys_add_properties(obs_properties_t *props)
{
	obs_properties_t *keyboard = obs_properties_create();
	obs_properties_add_group(props, "keyboard_enabled", obs_module_text("Keyboard"), OBS_GROUP_CHECKABLE, keyboard);
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
	obs_properties_add_bool(keyboard, "keyboard_arrow_aliases", obs_module_text("KeyboardArrowAliases"));
	obs_properties_add_color(keyboard, "keyboard_tint_color", obs_module_text("KeyboardTintColor"));
	obs_properties_add_font(keyboard, "keyboard_font", obs_module_text("KeyboardFont"));
	obs_properties_add_text(keyboard, "keyboard_character_w", obs_module_text("KeyboardCharacterW"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(keyboard, "keyboard_character_a", obs_module_text("KeyboardCharacterA"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(keyboard, "keyboard_character_s", obs_module_text("KeyboardCharacterS"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(keyboard, "keyboard_character_d", obs_module_text("KeyboardCharacterD"),
				OBS_TEXT_DEFAULT);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++)
		obs_properties_add_bool(keyboard, key_visibility_settings[i], obs_module_text(key_visibility_labels[i]));

	obs_property_t *feedback = obs_properties_add_list(keyboard, "keyboard_feedback",
							   obs_module_text("KeyboardFeedback"), OBS_COMBO_TYPE_LIST,
							   OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(feedback, obs_module_text("KeyboardFeedbackColor"), KEYBOARD_FEEDBACK_COLOR);
	obs_property_list_add_int(feedback, obs_module_text("KeyboardFeedbackPulse"), KEYBOARD_FEEDBACK_PULSE);
	obs_property_list_add_int(feedback, obs_module_text("KeyboardFeedbackBoth"), KEYBOARD_FEEDBACK_BOTH);
}

void keyboard_keys_update(struct keyboard_overlay_gg_data *keyboard, obs_data_t *settings)
{
	const bool arrow_aliases = obs_data_get_bool(settings, "keyboard_arrow_aliases");
	const bool arrow_aliases_changed = keyboard->arrow_aliases != arrow_aliases;
	keyboard->enabled = obs_data_get_bool(settings, "keyboard_enabled");
	keyboard->arrow_aliases = arrow_aliases;
	keyboard->key_size = (float)obs_data_get_double(settings, "keyboard_key_size");
	keyboard->spacing = (float)obs_data_get_double(settings, "keyboard_spacing");
	keyboard->idle_opacity_pct = (float)obs_data_get_double(settings, "keyboard_idle_opacity");
	keyboard->active_opacity_pct = (float)obs_data_get_double(settings, "keyboard_active_opacity");
	const double configured_fade_duration = obs_data_get_double(settings, "keyboard_fade_duration");
	keyboard->pulse_duration = (float)obs_data_get_double(settings, "keyboard_pulse_duration");
	keyboard->rotation_deg = (float)obs_data_get_double(settings, "keyboard_rotation");
	keyboard->tint_color = (uint32_t)obs_data_get_int(settings, "keyboard_tint_color");
	keyboard->feedback = (int)obs_data_get_int(settings, "keyboard_feedback");
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++)
		keyboard->keys[i].visible = !obs_data_has_user_value(settings, key_visibility_settings[i]) ||
								  obs_data_get_bool(settings, key_visibility_settings[i]);

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
	if (keyboard->feedback < KEYBOARD_FEEDBACK_COLOR || keyboard->feedback > KEYBOARD_FEEDBACK_BOTH)
		keyboard->feedback = KEY_FEEDBACK_DEFAULT;
	if (arrow_aliases_changed)
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
	keyboard_capture_sample_wasd(keyboard->arrow_aliases, &snapshot);
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
