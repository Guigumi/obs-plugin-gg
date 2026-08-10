#include "test_harness.h"

#include "keyboard-overlay-layout.h"

#include <string.h>

static void test_layout_wasd_positions(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.key_size = 64.0f;
	keyboard.spacing = 8.0f;

	keyboard_layout_update(&keyboard);

	const float step = 72.0f;
	const float half = 32.0f;
	ASSERT_FLOAT_NEAR(step + half, keyboard.keys[KEYBOARD_KEY_W].x, 0.001f);
	ASSERT_FLOAT_NEAR(half, keyboard.keys[KEYBOARD_KEY_W].y, 0.001f);
	ASSERT_FLOAT_NEAR(half, keyboard.keys[KEYBOARD_KEY_A].x, 0.001f);
	ASSERT_FLOAT_NEAR(step + half, keyboard.keys[KEYBOARD_KEY_A].y, 0.001f);
	ASSERT_FLOAT_NEAR(step + half, keyboard.keys[KEYBOARD_KEY_S].x, 0.001f);
	ASSERT_FLOAT_NEAR(step + half, keyboard.keys[KEYBOARD_KEY_S].y, 0.001f);
	ASSERT_FLOAT_NEAR(step * 2.0f + half, keyboard.keys[KEYBOARD_KEY_D].x, 0.001f);
	ASSERT_FLOAT_NEAR(step + half, keyboard.keys[KEYBOARD_KEY_D].y, 0.001f);
}

static void test_layout_base_dimensions(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.key_size = 64.0f;
	keyboard.spacing = 8.0f;

	float width = 0.0f;
	float height = 0.0f;
	keyboard_layout_get_base_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(208.0f, width, 0.001f);
	ASSERT_FLOAT_NEAR(136.0f, height, 0.001f);
}

static void test_layout_rotation_dimensions(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.key_size = 64.0f;
	keyboard.spacing = 8.0f;

	float width = 0.0f;
	float height = 0.0f;
	keyboard.rotation_deg = 0.0f;
	keyboard_layout_get_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(208.0f, width, 0.01f);
	ASSERT_FLOAT_NEAR(136.0f, height, 0.01f);
	keyboard.rotation_deg = 90.0f;
	keyboard_layout_get_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(136.0f, width, 0.01f);
	ASSERT_FLOAT_NEAR(208.0f, height, 0.01f);
	keyboard.rotation_deg = 180.0f;
	keyboard_layout_get_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(208.0f, width, 0.01f);
	ASSERT_FLOAT_NEAR(136.0f, height, 0.01f);
}

static void test_layout_full_keyboard_positions(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.key_size = 64.0f;
	keyboard.spacing = 8.0f;
	keyboard.layout_preset = KEYBOARD_LAYOUT_100;
	keyboard_layout_update(&keyboard);
	ASSERT_FLOAT_NEAR(32.0f, keyboard.keys[KEYBOARD_KEY_ESCAPE].x, 0.001f);
	ASSERT_FLOAT_NEAR(32.0f, keyboard.keys[KEYBOARD_KEY_ESCAPE].y, 0.001f);
	ASSERT_FLOAT_NEAR(128.0f, keyboard.keys[KEYBOARD_KEY_BACKSPACE].width, 0.001f);
	ASSERT_FLOAT_NEAR(128.0f, keyboard.keys[KEYBOARD_KEY_TAB].width, 0.001f);
	ASSERT_FLOAT_NEAR(128.0f, keyboard.keys[KEYBOARD_KEY_CAPS].width, 0.001f);
	ASSERT_FLOAT_NEAR(400.0f, keyboard.keys[KEYBOARD_KEY_SPACE].width, 0.001f);
	ASSERT_FLOAT_NEAR(896.0f, keyboard.keys[KEYBOARD_KEY_F12].x, 0.001f);
	ASSERT_FLOAT_NEAR(8.0f,
				 keyboard.keys[KEYBOARD_KEY_RIGHT_ALT].x - keyboard.keys[KEYBOARD_KEY_RIGHT_ALT].width / 2.0f -
					 (keyboard.keys[KEYBOARD_KEY_SPACE].x + keyboard.keys[KEYBOARD_KEY_SPACE].width / 2.0f),
				 0.001f);
}

static void test_layout_full_keyboard_dimensions(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.key_size = 64.0f;
	keyboard.spacing = 8.0f;
	keyboard.layout_preset = KEYBOARD_LAYOUT_100;
	keyboard_layout_update(&keyboard);
	float width = 0.0f;
	float height = 0.0f;
	keyboard_layout_get_base_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(1064.0f, width, 0.001f);
	ASSERT_FLOAT_NEAR(424.0f, height, 0.001f);
}

static void test_layout_extras_positions_and_dimensions(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.key_size = 64.0f;
	keyboard.spacing = 8.0f;
	keyboard.layout_preset = KEYBOARD_LAYOUT_EXTRAS;
	keyboard_layout_update(&keyboard);
	ASSERT_FLOAT_NEAR(32.0f, keyboard.keys[KEYBOARD_KEY_INSERT].x, 0.001f);
	ASSERT_FLOAT_NEAR(104.0f, keyboard.keys[KEYBOARD_KEY_UP].x, 0.001f);
	ASSERT_FLOAT_NEAR(320.0f, keyboard.keys[KEYBOARD_KEY_LEFT].y, 0.001f);
	ASSERT_FLOAT_NEAR(32.0f, keyboard.keys[KEYBOARD_KEY_PRINT_SCREEN].x, 0.001f);
	float width = 0.0f;
	float height = 0.0f;
	keyboard_layout_get_base_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(496.0f, width, 0.001f);
	ASSERT_FLOAT_NEAR(352.0f, height, 0.001f);
}

static void test_layout_numpad_positions_and_dimensions(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.key_size = 64.0f;
	keyboard.spacing = 8.0f;
	keyboard.layout_preset = KEYBOARD_LAYOUT_NUMPAD;
	keyboard_layout_update(&keyboard);
	ASSERT_FLOAT_NEAR(32.0f, keyboard.keys[KEYBOARD_KEY_NUM_LOCK].x, 0.001f);
	ASSERT_FLOAT_NEAR(248.0f, keyboard.keys[KEYBOARD_KEY_NUMPAD_ADD].x, 0.001f);
	ASSERT_FLOAT_NEAR(136.0f, keyboard.keys[KEYBOARD_KEY_NUMPAD_ADD].height, 0.001f);
	ASSERT_FLOAT_NEAR(128.0f, keyboard.keys[KEYBOARD_KEY_NUMPAD_0].width, 0.001f);
	float width = 0.0f;
	float height = 0.0f;
	keyboard_layout_get_base_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(280.0f, width, 0.001f);
	ASSERT_FLOAT_NEAR(352.0f, height, 0.001f);
}

static void test_layout_osu_presets(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.key_size = 64.0f;
	keyboard.spacing = 8.0f;
	keyboard.layout_preset = KEYBOARD_LAYOUT_OSU_MANIA;
	keyboard_layout_update(&keyboard);
	ASSERT_FLOAT_NEAR(32.0f, keyboard.keys[KEYBOARD_KEY_D].x, 0.001f);
	ASSERT_FLOAT_NEAR(248.0f, keyboard.keys[KEYBOARD_KEY_K].x, 0.001f);
	float width = 0.0f;
	float height = 0.0f;
	keyboard_layout_get_base_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(280.0f, width, 0.001f);
	ASSERT_FLOAT_NEAR(64.0f, height, 0.001f);

	keyboard.layout_preset = KEYBOARD_LAYOUT_OSU_STANDARD;
	keyboard_layout_update(&keyboard);
	ASSERT_FLOAT_NEAR(32.0f, keyboard.keys[KEYBOARD_KEY_Z].x, 0.001f);
	ASSERT_FLOAT_NEAR(104.0f, keyboard.keys[KEYBOARD_KEY_X].x, 0.001f);
	keyboard_layout_get_base_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(136.0f, width, 0.001f);
	ASSERT_FLOAT_NEAR(64.0f, height, 0.001f);
}

static void test_layout_game_presets(void)
{
	struct keyboard_overlay_gg_data keyboard;
	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.key_size = 64.0f;
	keyboard.spacing = 8.0f;
	keyboard.layout_preset = KEYBOARD_LAYOUT_CS2;
	keyboard_layout_update(&keyboard);
	ASSERT_FLOAT_NEAR(224.0f, keyboard.keys[KEYBOARD_KEY_W].x, 0.001f);
	ASSERT_FLOAT_NEAR(64.0f, keyboard.keys[KEYBOARD_KEY_TAB].x, 0.001f);
	ASSERT_FLOAT_NEAR(64.0f, keyboard.keys[KEYBOARD_KEY_CAPS].x, 0.001f);
	ASSERT_FLOAT_NEAR(224.0f, keyboard.keys[KEYBOARD_KEY_B].y, 0.001f);
	float width = 0.0f;
	float height = 0.0f;
	keyboard_layout_get_base_dimensions(&keyboard, &width, &height);
	ASSERT_FLOAT_NEAR(448.0f, width, 0.001f);
	ASSERT_FLOAT_NEAR(320.0f, height, 0.001f);

	keyboard.layout_preset = KEYBOARD_LAYOUT_VALORANT;
	keyboard_layout_update(&keyboard);
	ASSERT_FLOAT_NEAR(352.0f, keyboard.keys[KEYBOARD_KEY_R].x, 0.001f);
	ASSERT_FLOAT_NEAR(352.0f, keyboard.keys[KEYBOARD_KEY_V].x, 0.001f);
	ASSERT_FLOAT_NEAR(160.0f, keyboard.keys[KEYBOARD_KEY_F].y, 0.001f);
}

int main(void)
{
	printf("--- Testes de Layout do Teclado ---\n");
	RUN_TEST(test_layout_wasd_positions);
	RUN_TEST(test_layout_base_dimensions);
	RUN_TEST(test_layout_rotation_dimensions);
	RUN_TEST(test_layout_full_keyboard_positions);
	RUN_TEST(test_layout_full_keyboard_dimensions);
	RUN_TEST(test_layout_extras_positions_and_dimensions);
	RUN_TEST(test_layout_numpad_positions_and_dimensions);
	RUN_TEST(test_layout_osu_presets);
	RUN_TEST(test_layout_game_presets);
	TEST_MAIN_END();
}
