#include "mouse-overlay-click.h"

#include "mouse-capture.h"
#include "mouse-overlay-cursor.h"
#include "mouse-overlay-resources.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CLICK_DURATION_DEFAULT 0.25f
#define CLICK_DURATION_MIN 0.05f
#define CLICK_DURATION_MAX 5.0f
#define CLICK_OPACITY_DEFAULT_PCT 25
#define CLICK_ANIM_EXPAND 0
#define CLICK_ANIM_CONTRACT 1
#define CLICK_ANIM_PULSE 2
#define CLICK_ANIM_DEFAULT CLICK_ANIM_PULSE

void mouse_click_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "click_enabled", true);
	obs_data_set_default_bool(settings, "click_left", true);
	obs_data_set_default_bool(settings, "click_right", true);
	obs_data_set_default_double(settings, "click_duration", CLICK_DURATION_DEFAULT);
	obs_data_set_default_int(settings, "click_opacity", CLICK_OPACITY_DEFAULT_PCT);
	obs_data_set_default_int(settings, "click_tint_color", MOUSE_TINT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "click_anim", CLICK_ANIM_DEFAULT);
}

void mouse_click_add_properties(obs_properties_t *props)
{
	obs_properties_add_bool(props, "click_left", obs_module_text("ClickLeft"));
	obs_properties_add_bool(props, "click_right", obs_module_text("ClickRight"));
	obs_properties_add_int_slider(props, "click_opacity", obs_module_text("ClickOpacity"), 0, 100, 1);
	obs_properties_add_float_slider(props, "click_duration", obs_module_text("ClickDuration"), CLICK_DURATION_MIN,
					CLICK_DURATION_MAX, 0.05f);
	obs_property_t *animation = obs_properties_add_list(props, "click_anim", obs_module_text("ClickAnim"),
							    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(animation, obs_module_text("ClickAnimExpand"), CLICK_ANIM_EXPAND);
	obs_property_list_add_int(animation, obs_module_text("ClickAnimContract"), CLICK_ANIM_CONTRACT);
	obs_property_list_add_int(animation, obs_module_text("ClickAnimPulse"), CLICK_ANIM_PULSE);
	obs_properties_add_color(props, "click_tint_color", obs_module_text("ClickTintColor"));
}

void mouse_click_update(struct mouse_click_state *click, obs_data_t *settings)
{
	click->enabled = obs_data_get_bool(settings, "click_enabled");
	click->left_enabled = obs_data_get_bool(settings, "click_left");
	click->right_enabled = obs_data_get_bool(settings, "click_right");
	const float duration = mouse_settings_get_float(settings, "click_duration", CLICK_DURATION_MIN,
							CLICK_DURATION_MAX, CLICK_DURATION_DEFAULT);
	click->duration_ns = mouse_seconds_to_ns(duration);
	click->opacity = mouse_settings_get_opacity(settings, "click_opacity", CLICK_OPACITY_DEFAULT_PCT);
	click->tint_color = mouse_settings_get_color(settings, "click_tint_color");
	click->animation = mouse_settings_get_enum(settings, "click_anim", CLICK_ANIM_EXPAND, CLICK_ANIM_PULSE,
						   CLICK_ANIM_DEFAULT);
	if (!click->enabled)
		mouse_click_reset(click);
}

void mouse_click_sync_sequences(struct mouse_click_state *click)
{
	mouse_capture_get_button_sequences(&click->seen_left_click_sequence, &click->seen_right_click_sequence);
}

void mouse_click_reset(struct mouse_click_state *click)
{
	click->count = 0;
	click->start = 0;
}

static void mouse_click_prune(struct mouse_click_state *click, uint64_t now_ns)
{
	size_t kept_count = 0;
	const size_t original_count = click->count;
	for (size_t i = 0; i < original_count; i++) {
		const size_t read_index = (click->start + i) % MOUSE_CLICK_EVENTS_CAPACITY;
		const struct mouse_click_event event = click->events[read_index];
		if (!event.tracks_hold && now_ns - event.release_time_ns >= click->duration_ns)
			continue;
		const size_t write_index = (click->start + kept_count) % MOUSE_CLICK_EVENTS_CAPACITY;
		click->events[write_index] = event;
		kept_count++;
	}
	click->count = kept_count;
	if (!click->count)
		click->start = 0;
}

static void mouse_click_remove(struct mouse_click_state *click, size_t logical_index)
{
	for (size_t i = logical_index; i + 1 < click->count; i++) {
		const size_t destination = (click->start + i) % MOUSE_CLICK_EVENTS_CAPACITY;
		const size_t source = (click->start + i + 1) % MOUSE_CLICK_EVENTS_CAPACITY;
		click->events[destination] = click->events[source];
	}
	click->count--;
	if (!click->count)
		click->start = 0;
}

static void mouse_click_make_room(struct mouse_click_state *click)
{
	if (click->count < MOUSE_CLICK_EVENTS_CAPACITY)
		return;
	size_t oldest_fade = SIZE_MAX;
	uint64_t oldest_release_time = UINT64_MAX;
	for (size_t i = 0; i < click->count; i++) {
		const size_t index = (click->start + i) % MOUSE_CLICK_EVENTS_CAPACITY;
		const struct mouse_click_event *event = &click->events[index];
		if (!event->tracks_hold && event->release_time_ns < oldest_release_time) {
			oldest_fade = i;
			oldest_release_time = event->release_time_ns;
		}
	}
	mouse_click_remove(click, oldest_fade != SIZE_MAX ? oldest_fade : 0);
}

static void mouse_click_push(struct mouse_click_state *click, bool left, float x, float y, uint64_t now_ns)
{
	mouse_click_make_room(click);
	click->count++;
	const size_t next = (click->start + click->count - 1) % MOUSE_CLICK_EVENTS_CAPACITY;
	struct mouse_click_event *event = &click->events[next];
	event->x = x;
	event->y = y;
	event->time_ns = now_ns;
	event->release_time_ns = 0;
	event->left = left;
	event->tracks_hold = true;
}

static void mouse_click_release_event(struct mouse_click_event *event, uint64_t now_ns, float x, float y)
{
	if (!event->tracks_hold)
		return;
	event->tracks_hold = false;
	event->release_time_ns = now_ns;
	event->x = x;
	event->y = y;
}

static void mouse_click_end_previous_hold(struct mouse_click_state *click, bool left, uint64_t now_ns, float x, float y)
{
	for (size_t i = 0; i < click->count; i++) {
		const size_t index = (click->start + i) % MOUSE_CLICK_EVENTS_CAPACITY;
		struct mouse_click_event *event = &click->events[index];
		if (event->left == left)
			mouse_click_release_event(event, now_ns, x, y);
	}
}

static bool mouse_click_has_tracked_hold(const struct mouse_click_state *click, bool left)
{
	for (size_t i = 0; i < click->count; i++) {
		const size_t index = (click->start + i) % MOUSE_CLICK_EVENTS_CAPACITY;
		const struct mouse_click_event *event = &click->events[index];
		if (event->left == left && event->tracks_hold)
			return true;
	}
	return false;
}

void mouse_click_tick(struct mouse_click_state *click, const struct mouse_cursor_state *cursor, uint64_t now_ns)
{
	long left_sequence;
	long right_sequence;
	mouse_capture_sample_button_sequences(&left_sequence, &right_sequence, &click->left_down, &click->right_down);
	if (click->seen_left_click_sequence != left_sequence) {
		click->seen_left_click_sequence = left_sequence;
		mouse_click_end_previous_hold(click, true, now_ns, cursor->x, cursor->y);
		if (cursor->visible && click->enabled && click->left_enabled)
			mouse_click_push(click, true, cursor->x, cursor->y, now_ns);
	}
	if (click->seen_right_click_sequence != right_sequence) {
		click->seen_right_click_sequence = right_sequence;
		mouse_click_end_previous_hold(click, false, now_ns, cursor->x, cursor->y);
		if (cursor->visible && click->enabled && click->right_enabled)
			mouse_click_push(click, false, cursor->x, cursor->y, now_ns);
	}
	for (size_t i = 0; i < click->count; i++) {
		const size_t index = (click->start + i) % MOUSE_CLICK_EVENTS_CAPACITY;
		struct mouse_click_event *event = &click->events[index];
		const bool still_held = event->left ? click->left_enabled && click->left_down
						    : click->right_enabled && click->right_down;
		if (event->tracks_hold && !still_held)
			mouse_click_release_event(event, now_ns, cursor->x, cursor->y);
	}
	if (cursor->visible && click->enabled && click->left_enabled && click->left_down &&
	    !mouse_click_has_tracked_hold(click, true))
		mouse_click_push(click, true, cursor->x, cursor->y, now_ns);
	if (cursor->visible && click->enabled && click->right_enabled && click->right_down &&
	    !mouse_click_has_tracked_hold(click, false))
		mouse_click_push(click, false, cursor->x, cursor->y, now_ns);

	mouse_click_prune(click, now_ns);
	if (!cursor->visible)
		mouse_click_reset(click);
}

static float mouse_click_animation_scale(int animation, float progress)
{
	if (animation == CLICK_ANIM_CONTRACT)
		return 1.0f - 0.5f * sinf((float)M_PI * progress);
	if (animation == CLICK_ANIM_PULSE)
		return 0.75f + 0.25f * sinf((float)M_PI * 0.5f * progress);
	return 0.5f + 0.5f * progress;
}

bool mouse_click_can_render(const struct mouse_click_state *click, const struct mouse_cursor_state *cursor,
			    const struct mouse_overlay_resources *resources)
{
	const bool left_held = cursor->visible && click->enabled && click->left_enabled && click->left_down;
	const bool right_held = cursor->visible && click->enabled && click->right_enabled && click->right_down;
	return click->enabled && (click->count > 0 || left_held || right_held) && resources->click_technique &&
	       resources->click_effect_opacity && resources->click_effect_tint && resources->click_effect_side &&
	       resources->click_effect_size;
}

static void mouse_click_draw(const struct mouse_cursor_state *cursor, struct mouse_overlay_resources *resources,
			     bool left, float x, float y, float size, float opacity)
{
	gs_effect_set_float(resources->click_effect_side, left ? -1.0f : 1.0f);
	gs_effect_set_float(resources->click_effect_size, size);
	float positions[4][2];
	const size_t position_count = mouse_cursor_draw_positions(cursor, x, y, size, positions);
	for (size_t i = 0; i < position_count; i++)
		mouse_resources_draw_procedural(resources->click_effect_opacity, positions[i][0], positions[i][1], size,
						opacity);
}

void mouse_click_render(const struct mouse_click_state *click, const struct mouse_cursor_state *cursor,
			struct mouse_overlay_resources *resources, uint64_t now_ns)
{
	if (!mouse_click_can_render(click, cursor, resources))
		return;
	bool left_animating = false;
	bool right_animating = false;
	for (size_t i = 0; i < click->count; i++) {
		const size_t index = (click->start + i) % MOUSE_CLICK_EVENTS_CAPACITY;
		const struct mouse_click_event *event = &click->events[index];
		if (!event->tracks_hold && now_ns - event->release_time_ns >= click->duration_ns)
			continue;
		if (event->left)
			left_animating = true;
		else
			right_animating = true;
	}
	const bool left_held = cursor->visible && click->left_enabled && click->left_down && !left_animating;
	const bool right_held = cursor->visible && click->right_enabled && click->right_down && !right_animating;
	mouse_resources_set_tint(resources->click_effect_tint, click->tint_color);

	const size_t passes = gs_technique_begin(resources->click_technique);
	for (size_t i = 0; i < passes; i++) {
		gs_technique_begin_pass(resources->click_technique, i);
		if (left_held)
			mouse_click_draw(cursor, resources, true, cursor->x, cursor->y, cursor->size, click->opacity);
		if (right_held)
			mouse_click_draw(cursor, resources, false, cursor->x, cursor->y, cursor->size, click->opacity);

		for (size_t j = 0; j < click->count; j++) {
			const size_t index = (click->start + j) % MOUSE_CLICK_EVENTS_CAPACITY;
			const struct mouse_click_event *event = &click->events[index];
			const bool held = event->tracks_hold &&
					  (event->left ? click->left_enabled && click->left_down
						       : click->right_enabled && click->right_down);
			const float animation_progress =
				mouse_clampf((float)(now_ns - event->time_ns) / (float)click->duration_ns, 0.0f, 1.0f);
			const float fade_progress =
				held ? 0.0f : (float)(now_ns - event->release_time_ns) / (float)click->duration_ns;
			if (fade_progress >= 1.0f)
				continue;

			mouse_click_draw(cursor, resources, event->left, held ? cursor->x : event->x,
					 held ? cursor->y : event->y,
					 cursor->size *
						 mouse_click_animation_scale(click->animation, animation_progress),
					 (1.0f - mouse_clampf(fade_progress, 0.0f, 1.0f)) * click->opacity);
		}
		gs_technique_end_pass(resources->click_technique);
	}
	gs_technique_end(resources->click_technique);
}
