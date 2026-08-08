/*
Mouse Overlay GG
Copyright (C) 2026 GUI

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <graphics/graphics.h>
#include <plugin-support.h>

#include <string.h>

#define OVERLAY_WIDTH 1920u
#define OVERLAY_HEIGHT 1080u
#define MARKER_SIZE 16u

struct mouse_overlay_gg_data {
	gs_texture_t *marker;
};

static const char *mouse_overlay_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("SourceName");
}

static void *mouse_overlay_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);
	UNUSED_PARAMETER(source);

	return bzalloc(sizeof(struct mouse_overlay_gg_data));
}

static void mouse_overlay_destroy(void *data)
{
	struct mouse_overlay_gg_data *shdata = data;
	if (shdata) {
		gs_texture_destroy(shdata->marker);
		bfree(shdata);
	}
}

static void mouse_overlay_build_marker(struct mouse_overlay_gg_data *shdata)
{
	if (shdata->marker)
		return;

	uint8_t texture_data[MARKER_SIZE * MARKER_SIZE * 4];
	memset(texture_data, 0, sizeof(texture_data));
	for (uint32_t y = 0; y < MARKER_SIZE; y++) {
		for (uint32_t x = 0; x < MARKER_SIZE; x++) {
			uint32_t center = MARKER_SIZE / 2;
			bool on_cross = (x == center) || (y == center) ||
					(x == center - 1) || (y == center - 1);
			if (on_cross) {
				uint32_t idx = (y * MARKER_SIZE + x) * 4;
				texture_data[idx + 0] = 0xFF;
				texture_data[idx + 1] = 0xFF;
				texture_data[idx + 2] = 0xFF;
				texture_data[idx + 3] = 0xFF;
			}
		}
	}

	const uint8_t *data = texture_data;
	shdata->marker = gs_texture_create(MARKER_SIZE, MARKER_SIZE, GS_RGBA, 1,
					   &data, 0);
	if (!shdata->marker)
		blog(LOG_ERROR, "Failed to create marker texture");
}

static uint32_t mouse_overlay_get_width(void *data)
{
	UNUSED_PARAMETER(data);
	return OVERLAY_WIDTH;
}

static uint32_t mouse_overlay_get_height(void *data)
{
	UNUSED_PARAMETER(data);
	return OVERLAY_HEIGHT;
}

static void mouse_overlay_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct mouse_overlay_gg_data *shdata = data;

	mouse_overlay_build_marker(shdata);
	if (!shdata->marker)
		return;

	gs_effect_t *const default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	if (!default_effect)
		return;

	gs_technique_t *const tech = gs_effect_get_technique(default_effect, "Draw");
	if (!tech)
		return;

	gs_eparam_t *const param = gs_effect_get_param_by_name(default_effect, "image");
	gs_effect_set_texture_srgb(param, shdata->marker);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	const size_t passes = gs_technique_begin(tech);
	for (size_t i = 0; i < passes; i++) {
		gs_technique_begin_pass(tech, i);

		gs_matrix_push();
		gs_matrix_translate3f(OVERLAY_WIDTH / 2.0f, OVERLAY_HEIGHT / 2.0f, 0.0f);
		gs_matrix_scale3f(MARKER_SIZE, MARKER_SIZE, 1.0f);
		gs_draw_sprite(shdata->marker, 0, 0, 0);
		gs_matrix_pop();

		gs_technique_end_pass(tech);
	}
	gs_technique_end(tech);

	gs_blend_state_pop();
}

struct obs_source_info mouse_overlay_gg_source_info = {
	.id = "mouse_overlay_gg",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW |
			OBS_SOURCE_SRGB,
	.get_name = mouse_overlay_get_name,
	.create = mouse_overlay_create,
	.destroy = mouse_overlay_destroy,
	.get_width = mouse_overlay_get_width,
	.get_height = mouse_overlay_get_height,
	.video_render = mouse_overlay_video_render,
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
	obs_register_source(&mouse_overlay_gg_source_info);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)",
		PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}