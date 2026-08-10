#include "test_harness.h"

#include "mouse-overlay-trail.h"

#include <string.h>

static void test_mouse_trail_reset_state(void)
{
	struct mouse_trail_state trail;
	memset(&trail, 0, sizeof(trail));
	trail.count = 10;
	trail.start = 5;
	trail.distance_since_point = 15.0f;
	trail.has_sample = true;
	mouse_trail_reset(&trail);
	ASSERT_EQ_INT(0, trail.count);
	ASSERT_EQ_INT(0, trail.start);
	ASSERT_FLOAT_NEAR(0.0f, trail.distance_since_point, 0.001f);
	ASSERT_FALSE(trail.has_sample);
}

static void test_mouse_trail_tick_sampling(void)
{
	struct mouse_trail_state trail;
	memset(&trail, 0, sizeof(trail));
	trail.enabled = true;
	trail.point_limit = 50;
	trail.spacing = 10.0f;
	trail.duration_ns = 1000000000ULL;
	struct mouse_cursor_state cursor;
	memset(&cursor, 0, sizeof(cursor));
	cursor.visible = true;
	cursor.x = 0.0f;
	cursor.y = 0.0f;
	const uint64_t now = 1000ULL;
	mouse_trail_tick(&trail, &cursor, now);
	ASSERT_EQ_INT(1, trail.count);
	cursor.x = 25.0f;
	mouse_trail_tick(&trail, &cursor, now + 100);
	ASSERT_EQ_INT(3, trail.count);
	mouse_trail_tick(&trail, &cursor, now + 2000000000ULL);
	ASSERT_EQ_INT(1, trail.count);
	ASSERT_FLOAT_NEAR(25.0f, trail.points[trail.start].x, 0.001f);
	ASSERT_FLOAT_NEAR(0.0f, trail.points[trail.start].y, 0.001f);
	ASSERT_EQ_INT(now + 2000000000ULL, trail.points[trail.start].time_ns);
}

int main(void)
{
	printf("--- Testes de Rastro do Mouse ---\n");
	RUN_TEST(test_mouse_trail_reset_state);
	RUN_TEST(test_mouse_trail_tick_sampling);
	TEST_MAIN_END();
}
