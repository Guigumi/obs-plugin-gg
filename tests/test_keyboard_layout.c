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

int main(void)
{
	printf("--- Testes de Layout do Teclado ---\n");
	RUN_TEST(test_layout_wasd_positions);
	RUN_TEST(test_layout_base_dimensions);
	RUN_TEST(test_layout_rotation_dimensions);
	TEST_MAIN_END();
}
