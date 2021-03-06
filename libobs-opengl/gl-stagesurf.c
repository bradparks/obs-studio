/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "gl-subsystem.h"

static bool create_pixel_pack_buffer(struct gs_stage_surface *surf)
{
	GLsizeiptr size;
	bool success = true;

	if (!gl_gen_buffers(1, &surf->pack_buffer))
		return false;

	if (!gl_bind_buffer(GL_PIXEL_PACK_BUFFER, surf->pack_buffer))
		return false;

	size  = surf->width * surf->bytes_per_pixel;
	size  = (size+3) & 0xFFFFFFFC; /* align width to 4-byte boundry */
	size *= surf->height;

	glBufferData(GL_PIXEL_PACK_BUFFER, size, 0, GL_DYNAMIC_READ);
	if (!gl_success("glBufferData"))
		success = false;

	if (!gl_bind_buffer(GL_PIXEL_PACK_BUFFER, 0))
		success = false;

	return success;
}

static bool gl_init_stage_surface(struct gs_stage_surface *surf)
{
	bool success = true;

	if (!gl_gen_textures(1, &surf->texture))
		return false;
	if (!gl_bind_texture(GL_TEXTURE_2D, surf->texture))
		return false;

	if (!gl_init_face(GL_TEXTURE_2D, surf->gl_type, 1, surf->gl_format,
			surf->gl_internal_format, false,
			surf->width, surf->height, 0, NULL))
		success = false;

	if (!gl_bind_texture(GL_TEXTURE_2D, 0))
		success = false;

	if (success)
		success = create_pixel_pack_buffer(surf);

	return success;
}

stagesurf_t device_create_stagesurface(device_t device, uint32_t width,
		uint32_t height, enum gs_color_format color_format)
{
	struct gs_stage_surface *surf;
	surf = bmalloc(sizeof(struct gs_stage_surface));
	memset(surf, 0, sizeof(struct gs_stage_surface));

	surf->format             = color_format;
	surf->width              = width;
	surf->height             = height;
	surf->gl_format          = convert_gs_format(color_format);
	surf->gl_internal_format = convert_gs_internal_format(color_format);
	surf->gl_type            = get_gl_format_type(color_format);
	surf->bytes_per_pixel    = gs_get_format_bpp(color_format)/8;

	if (!gl_init_stage_surface(surf)) {
		blog(LOG_ERROR, "device_create_stagesurface (GL) failed");
		stagesurface_destroy(surf);
		return NULL;
	}

	return surf;
}

void stagesurface_destroy(stagesurf_t stagesurf)
{
	if (stagesurf) {
		if (stagesurf->pack_buffer)
			gl_delete_buffers(1, &stagesurf->pack_buffer);

		if (stagesurf->texture)
			gl_delete_textures(1, &stagesurf->texture);

		bfree(stagesurf);
	}
}

static bool can_stage(struct gs_stage_surface *dst, struct gs_texture_2d *src)
{
	if (!src) {
		blog(LOG_ERROR, "Source texture is NULL");
		return false;
	}

	if (src->base.type != GS_TEXTURE_2D) {
		blog(LOG_ERROR, "Source texture must be a 2D texture");
		return false;
	}

	if (!dst) {
		blog(LOG_ERROR, "Destination surface is NULL");
		return false;
	}

	if (src->base.format != dst->format) {
		blog(LOG_ERROR, "Source and destination formats do not match");
		return false;
	}

	if (src->width != dst->width || src->height != dst->height) {
		blog(LOG_ERROR, "Source and destination must have the same "
		                "dimensions");
		return false;
	}

	return true;
}

void device_stage_texture(device_t device, stagesurf_t dst, texture_t src)
{
	struct gs_texture_2d *tex2d = (struct gs_texture_2d*)src;
	if (!can_stage(dst, tex2d))
		goto failed;

	if (!gl_copy_texture(device, dst->texture, GL_TEXTURE_2D,
				tex2d->base.texture, GL_TEXTURE_2D,
				dst->width, dst->height))
		goto failed;

	if (!gl_bind_buffer(GL_TEXTURE_2D, dst->texture))
		goto failed;
	if (!gl_bind_buffer(GL_PIXEL_PACK_BUFFER, dst->pack_buffer))
		goto failed;

	glGetTexImage(GL_TEXTURE_2D, 0, dst->gl_format, dst->gl_type, 0);
	if (!gl_success("glGetTexImage"))
		goto failed;

	gl_bind_buffer(GL_PIXEL_PACK_BUFFER, 0);
	gl_bind_texture(GL_TEXTURE_2D, 0);
	return;

failed:
	gl_bind_buffer(GL_PIXEL_PACK_BUFFER, 0);
	gl_bind_texture(GL_TEXTURE_2D, 0);
	blog(LOG_ERROR, "device_stage_texture (GL) failed");
}

uint32_t stagesurface_getwidth(stagesurf_t stagesurf)
{
	return stagesurf->width;
}

uint32_t stagesurface_getheight(stagesurf_t stagesurf)
{
	return stagesurf->height;
}

enum gs_color_format stagesurface_getcolorformat(stagesurf_t stagesurf)
{
	return stagesurf->format;
}

bool stagesurface_map(stagesurf_t stagesurf, const void **data,
		uint32_t *row_bytes)
{
	if (!gl_bind_buffer(GL_PIXEL_PACK_BUFFER, stagesurf->pack_buffer))
		goto fail;

	*data = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (!gl_success("glMapBuffer"))
		goto fail;

	gl_bind_buffer(GL_PIXEL_PACK_BUFFER, 0);

	*row_bytes = stagesurf->bytes_per_pixel * stagesurf->width;
	return true;

fail:
	blog(LOG_ERROR, "stagesurf_map (GL) failed");
	return false;
}

void stagesurface_unmap(stagesurf_t stagesurf)
{
	if (!gl_bind_buffer(GL_PIXEL_PACK_BUFFER, stagesurf->pack_buffer))
		return;

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	gl_success("glUnmapBuffer");

	gl_bind_buffer(GL_PIXEL_PACK_BUFFER, 0);
}
