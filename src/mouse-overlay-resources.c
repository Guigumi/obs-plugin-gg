#include "mouse-overlay-resources.h"

#include <graphics/vec4.h>
#include <util/threading.h>

struct mouse_image_entry {
	gs_image_file_t *image;
	const char *path;
};

static void mouse_resources_load_image(gs_image_file_t *image, const char *relative_path)
{
	char *path = obs_module_file(relative_path);
	if (!path) {
		blog(LOG_WARNING, "Failed to resolve image path: %s", relative_path);
		return;
	}
	gs_image_file_init(image, path);
	if (!image->loaded)
		blog(LOG_WARNING, "Failed to load image: %s", path);
	bfree(path);
}

static void mouse_resources_init_images(struct mouse_overlay_resources *resources)
{
	const struct mouse_image_entry images[] = {
		{&resources->cursor_image, "images/cursor-main.png"},
		{&resources->trail_image, "images/cursor-trail.png"},
	};
	for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); i++)
		mouse_resources_load_image(images[i].image, images[i].path);
}

static void mouse_resources_free_images(struct mouse_overlay_resources *resources)
{
	gs_image_file_t *images[] = {
		&resources->cursor_image,
		&resources->trail_image,
	};
	for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); i++)
		gs_image_file_free(images[i]);
}

static gs_effect_t *mouse_resources_load_effect(const char *relative_path)
{
	char *path = obs_module_file(relative_path);
	if (!path) {
		blog(LOG_ERROR, "Failed to resolve effect path: %s", relative_path);
		return NULL;
	}
	char *errors = NULL;
	obs_enter_graphics();
	gs_effect_t *effect = gs_effect_create_from_file(path, &errors);
	obs_leave_graphics();
	if (!effect)
		blog(LOG_ERROR, "Failed to load effect %s: %s", path, errors ? errors : "unknown error");
	bfree(errors);
	bfree(path);
	return effect;
}

static bool mouse_resources_validate_effect_entry(const char *effect_name, const char *entry_name, const void *entry)
{
	if (entry)
		return true;
	blog(LOG_ERROR, "%s is missing required entry: %s", effect_name, entry_name);
	return false;
}

static void mouse_resources_init_effects(struct mouse_overlay_resources *resources)
{
	resources->trail_effect = mouse_resources_load_effect("trail.effect");
	if (resources->trail_effect) {
		resources->trail_technique = gs_effect_get_technique(resources->trail_effect, "DrawTrail");
		resources->trail_effect_image = gs_effect_get_param_by_name(resources->trail_effect, "image");
		resources->trail_effect_opacity = gs_effect_get_param_by_name(resources->trail_effect, "opacity");
		resources->trail_effect_tint = gs_effect_get_param_by_name(resources->trail_effect, "tint");
		mouse_resources_validate_effect_entry("trail.effect", "DrawTrail", resources->trail_technique);
		mouse_resources_validate_effect_entry("trail.effect", "image", resources->trail_effect_image);
		mouse_resources_validate_effect_entry("trail.effect", "opacity", resources->trail_effect_opacity);
		mouse_resources_validate_effect_entry("trail.effect", "tint", resources->trail_effect_tint);
	}

	resources->click_effect = mouse_resources_load_effect("click.effect");
	if (resources->click_effect) {
		resources->cursor_technique = gs_effect_get_technique(resources->click_effect, "DrawCursor");
		resources->click_technique = gs_effect_get_technique(resources->click_effect, "DrawClick");
		resources->click_effect_image = gs_effect_get_param_by_name(resources->click_effect, "image");
		resources->click_effect_opacity = gs_effect_get_param_by_name(resources->click_effect, "opacity");
		resources->click_effect_tint = gs_effect_get_param_by_name(resources->click_effect, "tint");
		resources->click_effect_side = gs_effect_get_param_by_name(resources->click_effect, "click_side");
		resources->click_effect_size = gs_effect_get_param_by_name(resources->click_effect, "sprite_size");
		mouse_resources_validate_effect_entry("click.effect", "DrawCursor", resources->cursor_technique);
		mouse_resources_validate_effect_entry("click.effect", "DrawClick", resources->click_technique);
		mouse_resources_validate_effect_entry("click.effect", "image", resources->click_effect_image);
		mouse_resources_validate_effect_entry("click.effect", "opacity", resources->click_effect_opacity);
		mouse_resources_validate_effect_entry("click.effect", "tint", resources->click_effect_tint);
		mouse_resources_validate_effect_entry("click.effect", "click_side", resources->click_effect_side);
		mouse_resources_validate_effect_entry("click.effect", "sprite_size", resources->click_effect_size);
	}
}

void mouse_resources_init(struct mouse_overlay_resources *resources)
{
	mouse_resources_init_images(resources);
	mouse_resources_init_effects(resources);
}

void mouse_resources_free(struct mouse_overlay_resources *resources)
{
	mouse_resources_free_images(resources);
	gs_effect_destroy(resources->trail_effect);
	gs_effect_destroy(resources->click_effect);
}

void mouse_resources_prepare(struct mouse_overlay_resources *resources)
{
	if (os_atomic_exchange_bool(&resources->images_dirty, false)) {
		mouse_resources_free_images(resources);
		mouse_resources_init_images(resources);
	}

	gs_image_file_t *images[] = {
		&resources->cursor_image,
		&resources->trail_image,
	};
	for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); i++) {
		if (!images[i]->texture && images[i]->loaded)
			gs_image_file_init_texture(images[i]);
	}
}

void mouse_resources_clear_textures(struct mouse_overlay_resources *resources)
{
	if (resources->trail_effect_image)
		gs_effect_set_texture(resources->trail_effect_image, NULL);
	if (resources->click_effect_image)
		gs_effect_set_texture(resources->click_effect_image, NULL);
}

bool mouse_resources_reload_images(obs_properties_t *props, obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	if (data)
		os_atomic_set_bool(&((struct mouse_overlay_gg_data *)data)->resources.images_dirty, true);
	return true;
}

static bool mouse_resources_draw_clipped_quad(float x, float y, float size)
{
	const float left = x - size / 2.0f;
	const float top = y - size / 2.0f;
	const float right = left + size;
	const float bottom = top + size;
	const float clipped_left = fmaxf(left, 0.0f);
	const float clipped_top = fmaxf(top, 0.0f);
	const float clipped_right = fminf(right, MOUSE_OVERLAY_WIDTH);
	const float clipped_bottom = fminf(bottom, MOUSE_OVERLAY_HEIGHT);
	if (clipped_left >= clipped_right || clipped_top >= clipped_bottom)
		return true;
	if (clipped_left == left && clipped_top == top && clipped_right == right && clipped_bottom == bottom)
		return false;

	const float u0 = (clipped_left - left) / size;
	const float v0 = (clipped_top - top) / size;
	const float u1 = (clipped_right - left) / size;
	const float v1 = (clipped_bottom - top) / size;
	gs_render_start(true);
	gs_texcoord(u0, v0, 0);
	gs_vertex2f(clipped_left, clipped_top);
	gs_texcoord(u1, v0, 0);
	gs_vertex2f(clipped_right, clipped_top);
	gs_texcoord(u0, v1, 0);
	gs_vertex2f(clipped_left, clipped_bottom);
	gs_texcoord(u1, v1, 0);
	gs_vertex2f(clipped_right, clipped_bottom);
	gs_render_stop(GS_TRISTRIP);
	return true;
}

void mouse_resources_draw_sprite(gs_eparam_t *image_param, gs_eparam_t *opacity_param, gs_texture_t *texture, float x,
				 float y, float size, float opacity)
{
	gs_effect_set_float(opacity_param, opacity);
	gs_effect_set_texture(image_param, texture);
	if (mouse_resources_draw_clipped_quad(x, y, size))
		return;
	gs_matrix_push();
	gs_matrix_translate3f(x - size / 2.0f, y - size / 2.0f, 0.0f);
	gs_draw_sprite(texture, 0, (uint32_t)size, (uint32_t)size);
	gs_matrix_pop();
}

void mouse_resources_draw_procedural(gs_eparam_t *opacity_param, float x, float y, float size, float opacity)
{
	gs_effect_set_float(opacity_param, opacity);
	if (mouse_resources_draw_clipped_quad(x, y, size))
		return;
	gs_matrix_push();
	gs_matrix_translate3f(x - size / 2.0f, y - size / 2.0f, 0.0f);
	gs_matrix_scale3f(size, size, 1.0f);
	gs_draw_sprite(NULL, 0, 1, 1);
	gs_matrix_pop();
}

void mouse_resources_set_tint(gs_eparam_t *param, uint32_t color)
{
	struct vec4 tint;
	vec4_from_rgba(&tint, color | 0xFF000000);
	gs_effect_set_vec4(param, &tint);
}
