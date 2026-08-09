#pragma once

#include "mouse-overlay-internal.h"

void mouse_resources_init(struct mouse_overlay_resources *resources);
void mouse_resources_free(struct mouse_overlay_resources *resources);
void mouse_resources_prepare(struct mouse_overlay_resources *resources);
void mouse_resources_clear_textures(struct mouse_overlay_resources *resources);
bool mouse_resources_reload_images(obs_properties_t *props, obs_property_t *property, void *data);
void mouse_resources_draw_sprite(gs_eparam_t *image_param, gs_eparam_t *opacity_param, gs_texture_t *texture, float x,
				 float y, float size, float opacity);
void mouse_resources_draw_procedural(gs_eparam_t *opacity_param, float x, float y, float size, float opacity);
void mouse_resources_set_tint(gs_eparam_t *param, uint32_t color);
