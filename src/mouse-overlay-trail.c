#include "mouse-overlay-trail.h"

#include "mouse-overlay-cursor.h"
#include "mouse-overlay-resources.h"

#define TRAIL_POINTS_MIN 1u
#define TRAIL_POINTS_MAX MOUSE_TRAIL_POINTS_CAPACITY
#define TRAIL_POINTS_DEFAULT 25u
#define TRAIL_DURATION_DEFAULT 0.25f
#define TRAIL_DURATION_MIN 0.05f
#define TRAIL_DURATION_MAX 30.0f
#define TRAIL_SPACING_DEFAULT 20.0f
#define TRAIL_SPACING_MIN 1.0f
#define TRAIL_SPACING_MAX 256.0f
#define TRAIL_SIZE_DEFAULT_PCT 100
#define TRAIL_SIZE_MIN_PCT 10
#define TRAIL_SIZE_MAX_PCT 200
#define TRAIL_OPACITY_DEFAULT_PCT 50
#define TRAIL_FADE_LINEAR 0
#define TRAIL_FADE_SMOOTH 1
#define TRAIL_FADE_EXPONENTIAL 2
#define TRAIL_FADE_DEFAULT TRAIL_FADE_SMOOTH

void mouse_trail_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "trail_points", (int)TRAIL_POINTS_DEFAULT);
	obs_data_set_default_double(settings, "trail_duration", TRAIL_DURATION_DEFAULT);
	obs_data_set_default_double(settings, "trail_spacing", TRAIL_SPACING_DEFAULT);
	obs_data_set_default_bool(settings, "trail_enabled", true);
	obs_data_set_default_bool(settings, "trail_shrink", true);
	obs_data_set_default_int(settings, "trail_size", TRAIL_SIZE_DEFAULT_PCT);
	obs_data_set_default_int(settings, "trail_opacity", TRAIL_OPACITY_DEFAULT_PCT);
	obs_data_set_default_int(settings, "trail_tint_color", MOUSE_TINT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "trail_fade", TRAIL_FADE_DEFAULT);
}

void mouse_trail_add_properties(obs_properties_t *props)
{
	obs_properties_add_int_slider(props, "trail_points", obs_module_text("TrailPoints"), (int)TRAIL_POINTS_MIN,
				      (int)TRAIL_POINTS_MAX, 1);
	obs_properties_add_int_slider(props, "trail_size", obs_module_text("TrailSize"), TRAIL_SIZE_MIN_PCT,
				      TRAIL_SIZE_MAX_PCT, 1);
	obs_properties_add_int_slider(props, "trail_opacity", obs_module_text("TrailOpacity"), 0, 100, 1);
	obs_properties_add_float_slider(props, "trail_duration", obs_module_text("TrailDuration"), TRAIL_DURATION_MIN,
					TRAIL_DURATION_MAX, 0.05f);
	obs_properties_add_float_slider(props, "trail_spacing", obs_module_text("TrailSpacing"), TRAIL_SPACING_MIN,
					TRAIL_SPACING_MAX, 1.0f);
	obs_properties_add_bool(props, "trail_shrink", obs_module_text("TrailShrink"));
	obs_properties_add_color(props, "trail_tint_color", obs_module_text("TrailTintColor"));
	obs_property_t *fade = obs_properties_add_list(props, "trail_fade", obs_module_text("TrailFade"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(fade, obs_module_text("TrailFadeLinear"), TRAIL_FADE_LINEAR);
	obs_property_list_add_int(fade, obs_module_text("TrailFadeSmooth"), TRAIL_FADE_SMOOTH);
	obs_property_list_add_int(fade, obs_module_text("TrailFadeExponential"), TRAIL_FADE_EXPONENTIAL);
}

void mouse_trail_update(struct mouse_trail_state *trail, obs_data_t *settings)
{
	trail->enabled = obs_data_get_bool(settings, "trail_enabled");
	trail->shrink = obs_data_get_bool(settings, "trail_shrink");
	trail->point_limit = (uint32_t)mouse_settings_get_int(settings, "trail_points", (int)TRAIL_POINTS_MIN,
							      (int)TRAIL_POINTS_MAX, (int)TRAIL_POINTS_DEFAULT);
	trail->size_scale = (float)mouse_settings_get_int(settings, "trail_size", TRAIL_SIZE_MIN_PCT,
							  TRAIL_SIZE_MAX_PCT, TRAIL_SIZE_DEFAULT_PCT) /
			    100.0f;
	trail->opacity = mouse_settings_get_opacity(settings, "trail_opacity", TRAIL_OPACITY_DEFAULT_PCT);
	trail->tint_color = mouse_settings_get_color(settings, "trail_tint_color");
	trail->fade = mouse_settings_get_enum(settings, "trail_fade", TRAIL_FADE_LINEAR, TRAIL_FADE_EXPONENTIAL,
					      TRAIL_FADE_DEFAULT);
	const float duration = mouse_settings_get_float(settings, "trail_duration", TRAIL_DURATION_MIN,
							TRAIL_DURATION_MAX, TRAIL_DURATION_DEFAULT);
	trail->duration_ns = mouse_seconds_to_ns(duration);
	trail->spacing = mouse_settings_get_float(settings, "trail_spacing", TRAIL_SPACING_MIN, TRAIL_SPACING_MAX,
						  TRAIL_SPACING_DEFAULT);
	trail->dirty = true;
}

void mouse_trail_reset(struct mouse_trail_state *trail)
{
	trail->count = 0;
	trail->start = 0;
	mouse_trail_reset_continuity(trail);
}

void mouse_trail_reset_continuity(struct mouse_trail_state *trail)
{
	trail->distance_since_point = 0.0f;
	trail->has_sample = false;
}

static void mouse_trail_prune(struct mouse_trail_state *trail, uint64_t now_ns)
{
	while (trail->count > 0) {
		const struct mouse_trail_point *point = &trail->points[trail->start];
		if (now_ns - point->time_ns < trail->duration_ns)
			break;
		trail->start = (trail->start + 1) % MOUSE_TRAIL_POINTS_CAPACITY;
		trail->count--;
	}
}

static void mouse_trail_push(struct mouse_trail_state *trail, float x, float y, uint64_t now_ns)
{
	const size_t next = (trail->start + trail->count) % MOUSE_TRAIL_POINTS_CAPACITY;
	struct mouse_trail_point *point = &trail->points[next];
	point->x = x;
	point->y = y;
	point->time_ns = now_ns;

	if (trail->count >= trail->point_limit)
		trail->start = (trail->start + 1) % MOUSE_TRAIL_POINTS_CAPACITY;
	else
		trail->count++;
}

static void mouse_trail_sample(struct mouse_trail_state *trail, float x, float y, uint64_t now_ns)
{
	if (!trail->has_sample) {
		trail->last_sample_x = x;
		trail->last_sample_y = y;
		trail->has_sample = true;
		mouse_trail_push(trail, x, y, now_ns);
		return;
	}

	const float dx = x - trail->last_sample_x;
	const float dy = y - trail->last_sample_y;
	const float distance = sqrtf(dx * dx + dy * dy);
	if (distance <= 0.0f)
		return;

	const float previous_remainder = trail->distance_since_point;
	const float total_distance = previous_remainder + distance;
	size_t point_count = (size_t)floorf(total_distance / trail->spacing);
	trail->distance_since_point = fmodf(total_distance, trail->spacing);
	const size_t max_points = trail->point_limit < MOUSE_TRAIL_POINTS_CAPACITY ? trail->point_limit
										   : MOUSE_TRAIL_POINTS_CAPACITY;
	const size_t skipped_points = point_count > max_points ? point_count - max_points : 0;
	point_count -= skipped_points;
	float segment_distance = trail->spacing - previous_remainder + (float)skipped_points * trail->spacing;

	for (size_t i = 0; i < point_count; i++, segment_distance += trail->spacing) {
		const float progress = segment_distance / distance;
		mouse_trail_push(trail, trail->last_sample_x + dx * progress, trail->last_sample_y + dy * progress,
				 now_ns);
	}

	trail->last_sample_x = x;
	trail->last_sample_y = y;
}

void mouse_trail_tick(struct mouse_trail_state *trail, const struct mouse_cursor_state *cursor, uint64_t now_ns)
{
	if (trail->dirty) {
		trail->dirty = false;
		if (!trail->enabled || !trail->point_limit) {
			mouse_trail_reset(trail);
			return;
		}
		if (trail->count > trail->point_limit) {
			const size_t drop = trail->count - trail->point_limit;
			trail->start = (trail->start + drop) % MOUSE_TRAIL_POINTS_CAPACITY;
			trail->count = trail->point_limit;
		}
	}

	mouse_trail_prune(trail, now_ns);
	if (!trail->count)
		trail->has_sample = false;
	if (!cursor->visible || !trail->enabled || !trail->point_limit)
		return;
	mouse_trail_sample(trail, cursor->x, cursor->y, now_ns);
}

bool mouse_trail_can_render(const struct mouse_trail_state *trail, const struct mouse_overlay_resources *resources)
{
	return trail->enabled && trail->count > 0 && resources->trail_image.texture && resources->trail_technique &&
	       resources->trail_effect_image && resources->trail_effect_opacity && resources->trail_effect_tint;
}

void mouse_trail_render(const struct mouse_trail_state *trail, const struct mouse_cursor_state *cursor,
			struct mouse_overlay_resources *resources, uint64_t now_ns)
{
	if (!mouse_trail_can_render(trail, resources))
		return;
	mouse_resources_set_tint(resources->trail_effect_tint, trail->tint_color);
	gs_effect_set_texture(resources->trail_effect_image, resources->trail_image.texture);
	gs_effect_set_float(resources->trail_effect_opacity, trail->opacity);
	const size_t passes = gs_technique_begin(resources->trail_technique);
	for (size_t i = 0; i < passes; i++) {
		gs_technique_begin_pass(resources->trail_technique, i);
		for (size_t j = 0; j < trail->count; j++) {
			const size_t index = (trail->start + j) % MOUSE_TRAIL_POINTS_CAPACITY;
			const struct mouse_trail_point *point = &trail->points[index];
			float remaining = 1.0f - (float)(now_ns - point->time_ns) / (float)trail->duration_ns;
			if (remaining <= 0.0f)
				continue;
			if (remaining > 1.0f)
				remaining = 1.0f;

			float fade = remaining;
			if (trail->fade == TRAIL_FADE_SMOOTH)
				fade = remaining * remaining * (3.0f - 2.0f * remaining);
			else if (trail->fade == TRAIL_FADE_EXPONENTIAL)
				fade = remaining * remaining;

			float size = cursor->size * trail->size_scale;
			if (trail->shrink)
				size *= remaining;
			if (size < 1.0f)
				continue;
			float positions[4][2];
			const size_t position_count =
				mouse_cursor_draw_positions(cursor, point->x, point->y, size, positions);
			for (size_t k = 0; k < position_count; k++)
				mouse_resources_draw_sprite(resources->trail_effect_image,
							    resources->trail_effect_opacity,
							    resources->trail_image.texture, positions[k][0],
							    positions[k][1], size, fade * trail->opacity);
		}
		gs_technique_end_pass(resources->trail_technique);
	}
	gs_technique_end(resources->trail_technique);
}
