/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This file is based on, or a modified version of code from TinyGL (C) 1997-1998 Fabrice Bellard,
 * which is licensed under the zlib-license (see LICENSE).
 * It also has modifications by the ResidualVM-team, which are covered under the GPLv2 (or later).
 */

// Texture Manager

#include "common/endian.h"

#include "graphics/tinygl/zgl.h"

namespace TinyGL {

static GLTexture *find_texture(GLContext *c, uint h) {
	GLTexture *t;

	t = c->shared_state.texture_hash_table[h % TEXTURE_HASH_TABLE_SIZE];
	while (t) {
		if (t->handle == h)
			return t;
		t = t->next;
	}
	return nullptr;
}

void GLContext::free_texture(uint h) {
	free_texture(find_texture(this, h));
}

void GLContext::free_texture(GLTexture *t) {
	GLTexture **ht;
	GLImage *im;

	if (!t->prev) {
		ht = &shared_state.texture_hash_table[t->handle % TEXTURE_HASH_TABLE_SIZE];
		*ht = t->next;
	} else {
		t->prev->next = t->next;
	}
	if (t->next)
		t->next->prev = t->prev;

	for (int i = 0; i < MAX_TEXTURE_LEVELS; i++) {
		im = &t->images[i];
		if (im->pixmap) {
			delete im->pixmap;
			im->pixmap = nullptr;
		}
	}

	gl_free(t);
}

GLTexture *GLContext::alloc_texture(uint h) {
	GLTexture *t, **ht;

	t = (GLTexture *)gl_zalloc(sizeof(GLTexture));

	ht = &shared_state.texture_hash_table[h % TEXTURE_HASH_TABLE_SIZE];

	t->next = *ht;
	t->prev = nullptr;
	if (t->next)
		t->next->prev = t;
	*ht = t;

	t->handle = h;
	t->disposed = false;
	t->versionNumber = 0;

	return t;
}

void GLContext::glInitTextures() {
	texture_2d_enabled = 0;
	current_texture = find_texture(this, 0);
	maxTextureName = 0;
	texture_mag_filter = TGL_LINEAR;
	texture_min_filter = TGL_NEAREST_MIPMAP_LINEAR;
#if defined(SCUMM_LITTLE_ENDIAN)
	colorAssociationList.push_back({Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24), TGL_RGBA, TGL_UNSIGNED_BYTE});
	colorAssociationList.push_back({Graphics::PixelFormat(3, 8, 8, 8, 0, 0, 8, 16, 0),  TGL_RGB,  TGL_UNSIGNED_BYTE});
#else
	colorAssociationList.push_back({Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0), TGL_RGBA, TGL_UNSIGNED_BYTE});
	colorAssociationList.push_back({Graphics::PixelFormat(3, 8, 8, 8, 0, 16, 8, 0, 0),  TGL_RGB,  TGL_UNSIGNED_BYTE});
#endif
	colorAssociationList.push_back({Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0),  TGL_RGB,  TGL_UNSIGNED_SHORT_5_6_5});
	colorAssociationList.push_back({Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0),  TGL_RGBA, TGL_UNSIGNED_SHORT_5_5_5_1});
	colorAssociationList.push_back({Graphics::PixelFormat(2, 4, 4, 4, 4, 12, 8, 4, 0),  TGL_RGBA, TGL_UNSIGNED_SHORT_4_4_4_4});
}

void GLContext::glopBindTexture(GLParam *p) {
	int target = p[1].i;
	int texture = p[2].i;
	GLTexture *t;

	assert(target == TGL_TEXTURE_2D && texture >= 0);

	t = find_texture(this, texture);
	if (!t) {
		t = alloc_texture(texture);
	}
	current_texture = t;
}

void GLContext::glopTexImage2D(GLParam *p) {
	int target = p[1].i;
	int level = p[2].i;
	int internalformat = p[3].i;
	int width = p[4].i;
	int height = p[5].i;
	int border = p[6].i;
	uint format = (uint)p[7].i;
	uint type = (uint)p[8].i;
	byte *pixels = (byte *)p[9].p;
	GLImage *im;

	if (target != TGL_TEXTURE_2D)
		error("tglTexImage2D: target not handled");
	if (level < 0 || level >= MAX_TEXTURE_LEVELS)
		error("tglTexImage2D: invalid level");
	if (internalformat != TGL_RGBA && internalformat != TGL_RGB)
		error("tglTexImage2D: invalid internalformat");
	if (border != 0)
		error("tglTexImage2D: invalid border");

	if (current_texture == nullptr) {
		return;
	}
	current_texture->versionNumber++;
	im = &current_texture->images[level];
	im->xsize = _textureSize;
	im->ysize = _textureSize;
	if (im->pixmap) {
		delete im->pixmap;
		im->pixmap = nullptr;
	}
	if (pixels != NULL) {
		unsigned int filter;
		Graphics::PixelFormat pf;
		bool found = false;
		Common::Array<struct tglColorAssociation>::const_iterator it = colorAssociationList.begin();
		for (; it != colorAssociationList.end(); it++) {
			if (it->format == format &&
			    it->type == type) {
				pf = it->pf;
				found = true;
				break;
			}
		}
		if (!found)
			error("TinyGL texture: format 0x%04x and type 0x%04x combination not supported", format, type);
		Graphics::PixelBuffer src(pf, pixels);
		Graphics::PixelFormat internalPf;
#if defined(SCUMM_LITTLE_ENDIAN)
		if (internalformat == TGL_RGBA)
			internalPf = Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24);
		else if (internalformat == TGL_RGB)
			internalPf = Graphics::PixelFormat(3, 8, 8, 8, 0, 0, 8, 16, 0);
#else
		if (internalformat == TGL_RGBA)
			internalPf = Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0);
		else if (internalformat == TGL_RGB)
			internalPf = Graphics::PixelFormat(3, 8, 8, 8, 0, 16, 8, 0, 0);
#endif
		Graphics::PixelBuffer srcInternal(internalPf, width * height, DisposeAfterUse::YES);
		srcInternal.copyBuffer(0, width * height, src);
		if (width > _textureSize || height > _textureSize)
			filter = texture_mag_filter;
		else
			filter = texture_min_filter;
		switch (filter) {
		case TGL_LINEAR_MIPMAP_NEAREST:
		case TGL_LINEAR_MIPMAP_LINEAR:
		case TGL_LINEAR:
			im->pixmap = new Graphics::BilinearTexelBuffer(
				srcInternal,
				width, height,
				_textureSize
			);
			break;
		default:
			im->pixmap = new Graphics::NearestTexelBuffer(
				srcInternal,
				width, height,
				_textureSize
			);
			break;
		}
	}
}

// TODO: not all tests are done
void GLContext::glopTexEnv(GLParam *p) {
	int target = p[1].i;
	int pname = p[2].i;
	int param = p[3].i;

	if (target != TGL_TEXTURE_ENV) {
error:
		error("tglTexParameter: unsupported option");
	}

	if (pname != TGL_TEXTURE_ENV_MODE)
		goto error;

	if (param != TGL_DECAL)
		goto error;
}

// TODO: not all tests are done
void GLContext::glopTexParameter(GLParam *p) {
	int target = p[1].i;
	int pname = p[2].i;
	int param = p[3].i;

	if (target != TGL_TEXTURE_2D) {
error:
		error("tglTexParameter: unsupported option");
	}

	switch (pname) {
	case TGL_TEXTURE_WRAP_S:
		texture_wrap_s = param;
		break;
	case TGL_TEXTURE_WRAP_T:
		texture_wrap_t = param;
		break;
	case TGL_TEXTURE_MAG_FILTER:
		switch (param) {
		case TGL_NEAREST:
		case TGL_LINEAR:
			texture_mag_filter = param;
			break;
		default:
			goto error;
		}
		break;
	case TGL_TEXTURE_MIN_FILTER:
		switch (param) {
		case TGL_LINEAR_MIPMAP_NEAREST:
		case TGL_LINEAR_MIPMAP_LINEAR:
		case TGL_NEAREST_MIPMAP_NEAREST:
		case TGL_NEAREST_MIPMAP_LINEAR:
		case TGL_NEAREST:
		case TGL_LINEAR:
			texture_min_filter = param;
			break;
		default:
			goto error;
		}
		break;
	default:
		;
	}
}

void GLContext::glopPixelStore(GLParam *p) {
	int pname = p[1].i;
	int param = p[2].i;

	if (pname != TGL_UNPACK_ALIGNMENT || param != 1) {
		error("tglPixelStore: unsupported option");
	}
}

} // end of namespace TinyGL

void tglGenTextures(int n, unsigned int *textures) {
	TinyGL::GLContext *c = TinyGL::gl_get_context();

	for (int i = 0; i < n; i++) {
		textures[i] = c->maxTextureName + i + 1;
	}
	c->maxTextureName += n;
}

void tglDeleteTextures(int n, const unsigned int *textures) {
	TinyGL::GLContext *c = TinyGL::gl_get_context();
	TinyGL::GLTexture *t;

	for (int i = 0; i < n; i++) {
		t = TinyGL::find_texture(c, textures[i]);
		if (t) {
			if (t == c->current_texture) {
				tglBindTexture(TGL_TEXTURE_2D, 0);
			}
			t->disposed = true;
		}
	}
}
