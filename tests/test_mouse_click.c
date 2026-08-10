#include "test_harness.h"

#include "mouse-overlay-click.h"
#include "test_stubs.h"

#include <string.h>

static void test_mouse_inline_helpers(void)
{
	ASSERT_FLOAT_NEAR(5.0f, mouse_clampf(5.0f, 0.0f, 10.0f), 0.001f);
	ASSERT_FLOAT_NEAR(0.0f, mouse_clampf(-2.0f, 0.0f, 10.0f), 0.001f);
	ASSERT_FLOAT_NEAR(10.0f, mouse_clampf(15.0f, 0.0f, 10.0f), 0.001f);
	ASSERT_EQ_INT(1500000000ULL, mouse_seconds_to_ns(1.5f));
}

static void test_mouse_click_reset_and_capacity(void)
{
	struct mouse_click_state click;
	memset(&click, 0, sizeof(click));
	click.count = 25;
	click.start = 12;
	mouse_click_reset(&click);
	ASSERT_EQ_INT(0, click.count);
	ASSERT_EQ_INT(0, click.start);
}

static void test_mouse_click_press_release_and_prune(void)
{
	struct mouse_click_state click;
	memset(&click, 0, sizeof(click));
	click.enabled = true;
	click.left_enabled = true;
	click.right_enabled = false;
	click.duration_ns = 1000;
	struct mouse_cursor_state cursor;
	memset(&cursor, 0, sizeof(cursor));
	cursor.visible = true;
	cursor.x = 10.0f;
	cursor.y = 20.0f;
	test_left_click_sequence = 1;
	test_left_button_down = true;
	mouse_click_tick(&click, &cursor, 100);
	ASSERT_EQ_INT(1, click.count);
	ASSERT_TRUE(click.events[click.start].tracks_hold);
	ASSERT_FLOAT_NEAR(10.0f, click.events[click.start].x, 0.001f);
	test_left_button_down = false;
	cursor.x = 30.0f;
	cursor.y = 40.0f;
	mouse_click_tick(&click, &cursor, 200);
	ASSERT_FALSE(click.events[click.start].tracks_hold);
	ASSERT_FLOAT_NEAR(30.0f, click.events[click.start].x, 0.001f);
	ASSERT_EQ_INT(200, click.events[click.start].release_time_ns);
	mouse_click_tick(&click, &cursor, 1300);
	ASSERT_EQ_INT(0, click.count);
}

int main(void)
{
	printf("--- Testes de Cliques do Mouse ---\n");
	RUN_TEST(test_mouse_inline_helpers);
	RUN_TEST(test_mouse_click_reset_and_capacity);
	RUN_TEST(test_mouse_click_press_release_and_prune);
	TEST_MAIN_END();
}
