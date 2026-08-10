#include "test_harness.h"

#include "keyboard-overlay-keys.h"

#include <string.h>

static void test_keyboard_keys_initialization(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard_keys_initialize(&keyboard);
	ASSERT_EQ_INT(KEYBOARD_KEY_W, keyboard.keys[0].capture_key);
	ASSERT_EQ_INT(KEYBOARD_KEY_A, keyboard.keys[1].capture_key);
	ASSERT_EQ_INT(KEYBOARD_KEY_S, keyboard.keys[2].capture_key);
	ASSERT_EQ_INT(KEYBOARD_KEY_D, keyboard.keys[3].capture_key);
}

static void test_keyboard_keys_clamping(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "keyboard_enabled", true);
	obs_data_set_double(settings, "keyboard_key_size", 999.0);
	obs_data_set_double(settings, "keyboard_spacing", -50.0);
	obs_data_set_double(settings, "keyboard_idle_opacity", 150.0);
	obs_data_set_double(settings, "keyboard_rotation", 300.0);
	keyboard_keys_update(&keyboard, settings);
	ASSERT_FLOAT_NEAR(256.0f, keyboard.key_size, 0.001f);
	ASSERT_FLOAT_NEAR(0.0f, keyboard.spacing, 0.001f);
	ASSERT_FLOAT_NEAR(100.0f, keyboard.idle_opacity_pct, 0.001f);
	ASSERT_FLOAT_NEAR(180.0f, keyboard.rotation_deg, 0.001f);
	obs_data_release(settings);
}

static void test_keyboard_fade_default_and_clamping(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "keyboard_enabled", true);
	keyboard_keys_update(&keyboard, settings);
	ASSERT_FLOAT_NEAR(0.03f, keyboard.fade_duration, 0.001f);
	obs_data_set_double(settings, "keyboard_fade_duration", 0.001);
	keyboard_keys_update(&keyboard, settings);
	ASSERT_FLOAT_NEAR(0.01f, keyboard.fade_duration, 0.001f);
	obs_data_set_double(settings, "keyboard_fade_duration", 1.0);
	keyboard_keys_update(&keyboard, settings);
	ASSERT_FLOAT_NEAR(0.50f, keyboard.fade_duration, 0.001f);
	obs_data_release(settings);
}

static void test_keyboard_invalid_values(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "keyboard_enabled", true);
	obs_data_set_double(settings, "keyboard_key_size", NAN);
	obs_data_set_double(settings, "keyboard_spacing", INFINITY);
	obs_data_set_double(settings, "keyboard_rotation", -INFINITY);
	keyboard_keys_update(&keyboard, settings);
	ASSERT_FLOAT_NEAR(64.0f, keyboard.key_size, 0.001f);
	ASSERT_FLOAT_NEAR(8.0f, keyboard.spacing, 0.001f);
	ASSERT_FLOAT_NEAR(0.0f, keyboard.rotation_deg, 0.001f);
	obs_data_release(settings);
}

static void test_keyboard_visibility(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard_keys_initialize(&keyboard);
	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "keyboard_enabled", true);
	keyboard_keys_update(&keyboard, settings);
	ASSERT_TRUE(keyboard.keys[KEYBOARD_KEY_W].visible);
	ASSERT_TRUE(keyboard.keys[KEYBOARD_KEY_A].visible);
	ASSERT_TRUE(keyboard.keys[KEYBOARD_KEY_S].visible);
	ASSERT_TRUE(keyboard.keys[KEYBOARD_KEY_D].visible);
	ASSERT_FALSE(keyboard.keys[KEYBOARD_KEY_SPACE].visible);
	obs_data_release(settings);
}

static void test_keyboard_preset_visibility(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard_keys_initialize(&keyboard);
	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "keyboard_enabled", true);
	obs_data_set_int(settings, "keyboard_layout_preset", KEYBOARD_LAYOUT_NUMPAD);
	keyboard_keys_update(&keyboard, settings);
	ASSERT_FALSE(keyboard.keys[KEYBOARD_KEY_W].visible);
	ASSERT_TRUE(keyboard.keys[KEYBOARD_KEY_1].visible);
	ASSERT_TRUE(keyboard.keys[KEYBOARD_KEY_5].visible);
	obs_data_release(settings);
}

static void test_keyboard_font_auto_size_default(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard_keys_initialize(&keyboard);
	obs_data_t *settings = obs_data_create();
	keyboard_keys_defaults(settings);
	keyboard_keys_update(&keyboard, settings);
	ASSERT_TRUE(keyboard.font_auto_size);
	ASSERT_EQ_INT(0xFFFFFF, keyboard.font_color);
	obs_data_set_bool(settings, "keyboard_font_auto_size", false);
	keyboard_keys_update(&keyboard, settings);
	ASSERT_TRUE(keyboard.font_auto_size);
	obs_data_release(settings);
}

int main(void)
{
	printf("--- Testes de Teclas e Validacao ---\n");
	RUN_TEST(test_keyboard_keys_initialization);
	RUN_TEST(test_keyboard_keys_clamping);
	RUN_TEST(test_keyboard_fade_default_and_clamping);
	RUN_TEST(test_keyboard_invalid_values);
	RUN_TEST(test_keyboard_visibility);
	RUN_TEST(test_keyboard_preset_visibility);
	RUN_TEST(test_keyboard_font_auto_size_default);
	TEST_MAIN_END();
}
