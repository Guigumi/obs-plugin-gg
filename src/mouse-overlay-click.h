#pragma once

#include "mouse-overlay-internal.h"

void mouse_click_defaults(obs_data_t *settings);
void mouse_click_add_properties(obs_properties_t *props);
void mouse_click_update(struct mouse_click_state *click, obs_data_t *settings);
void mouse_click_sync_sequences(struct mouse_click_state *click);
void mouse_click_reset(struct mouse_click_state *click);
void mouse_click_tick(struct mouse_click_state *click, const struct mouse_cursor_state *cursor, uint64_t now_ns);
bool mouse_click_can_render(const struct mouse_click_state *click, const struct mouse_cursor_state *cursor,
			    const struct mouse_overlay_resources *resources);
void mouse_click_render(const struct mouse_click_state *click, const struct mouse_cursor_state *cursor,
			struct mouse_overlay_resources *resources, uint64_t now_ns);
