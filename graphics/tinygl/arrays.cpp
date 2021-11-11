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

#include "graphics/tinygl/zgl.h"

#define VERTEX_ARRAY    0x0001
#define COLOR_ARRAY     0x0002
#define NORMAL_ARRAY    0x0004
#define TEXCOORD_ARRAY  0x0008

namespace TinyGL {

void glopArrayElement(GLContext *c, GLParam *param) {
	int offset;
	int states = c->client_states;
	int idx = param[1].i;

	if (states & COLOR_ARRAY) {
		GLParam p[5];
		int size = c->color_array_size;
		offset = idx * c->color_array_stride;
		switch (c->color_array_type) {
		case TGL_UNSIGNED_BYTE:
		case TGL_BYTE: {
				TGLbyte *array = (TGLbyte *)c->color_array + offset;
				p[1].f = array[0];
				p[2].f = array[1];
				p[3].f = array[2];
				p[4].f = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_UNSIGNED_INT:
		case TGL_INT: {
				TGLint *array = (TGLint *)((TGLbyte *)c->color_array + offset);
				p[1].f = array[0];
				p[2].f = array[1];
				p[3].f = array[2];
				p[4].f = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_UNSIGNED_SHORT:
		case TGL_SHORT: {
				TGLshort *array = (TGLshort *)((TGLbyte *)c->color_array + offset);
				p[1].f = array[0];
				p[2].f = array[1];
				p[3].f = array[2];
				p[4].f = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_FLOAT: {
				TGLfloat *array = (TGLfloat *)((TGLbyte *)c->color_array + offset);
				p[1].f = array[0];
				p[2].f = array[1];
				p[3].f = array[2];
				p[4].f = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_DOUBLE: {
				TGLdouble *array = (TGLdouble *)((TGLbyte *)c->color_array + offset);
				p[1].f = array[0];
				p[2].f = array[1];
				p[3].f = array[2];
				p[4].f = size > 3 ? array[3] : 1.0f;
				break;
			}
		default:
			assert(0);
		}
		glopColor(c, p);
	}
	if (states & NORMAL_ARRAY) {
		offset = idx * c->normal_array_stride;
		c->current_normal.W = 0.0f;
		switch (c->normal_array_type) {
		case TGL_FLOAT: {
				TGLfloat *array = (TGLfloat *)((TGLbyte *)c->normal_array + offset);
				c->current_normal.X = array[0];
				c->current_normal.Y = array[1];
				c->current_normal.Z = array[2];
				break;
			}
		case TGL_DOUBLE: {
				TGLdouble *array = (TGLdouble *)((TGLbyte *)c->normal_array + offset);
				c->current_normal.X = array[0];
				c->current_normal.Y = array[1];
				c->current_normal.Z = array[2];
				break;
			}
		case TGL_INT: {
				TGLint *array = (TGLint *)((TGLbyte *)c->normal_array + offset);
				c->current_normal.X = array[0];
				c->current_normal.Y = array[1];
				c->current_normal.Z = array[2];
				break;
			}
		case TGL_SHORT: {
				TGLshort *array = (TGLshort *)((TGLbyte *)c->normal_array + offset);
				c->current_normal.X = array[0];
				c->current_normal.Y = array[1];
				c->current_normal.Z = array[2];
			break;
		}
		default:
			assert(0);
		}
	}
	if (states & TEXCOORD_ARRAY) {
		int size = c->texcoord_array_size;
		offset = idx * c->texcoord_array_stride;
		switch (c->texcoord_array_type) {
		case TGL_FLOAT: {
				TGLfloat *array = (TGLfloat *)((TGLbyte *)c->texcoord_array + offset);
				c->current_tex_coord.X = array[0];
				c->current_tex_coord.Y = array[1];
				c->current_tex_coord.Z = size > 2 ? array[2] : 0.0f;
				c->current_tex_coord.W = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_DOUBLE: {
				TGLdouble *array = (TGLdouble *)((TGLbyte *)c->texcoord_array + offset);
				c->current_tex_coord.X = array[0];
				c->current_tex_coord.Y = array[1];
				c->current_tex_coord.Z = size > 2 ? array[2] : 0.0f;
				c->current_tex_coord.W = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_INT: {
				TGLint *array = (TGLint *)((TGLbyte *)c->texcoord_array + offset);
				c->current_tex_coord.X = array[0];
				c->current_tex_coord.Y = array[1];
				c->current_tex_coord.Z = size > 2 ? array[2] : 0.0f;
				c->current_tex_coord.W = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_SHORT: {
				TGLshort *array = (TGLshort *)((TGLbyte *)c->texcoord_array + offset);
				c->current_tex_coord.X = array[0];
				c->current_tex_coord.Y = array[1];
				c->current_tex_coord.Z = size > 2 ? array[2] : 0.0f;
				c->current_tex_coord.W = size > 3 ? array[3] : 1.0f;
				break;
			}
		default:
			assert(0);
		}
	}
	if (states & VERTEX_ARRAY) {
		GLParam p[5];
		int size = c->vertex_array_size;
		offset = idx * c->vertex_array_stride;
		switch (c->vertex_array_type) {
		case TGL_FLOAT: {
				TGLfloat *array = (TGLfloat *)((TGLbyte *)c->vertex_array + offset);
				p[1].f = array[0];
				p[2].f = array[1];
				p[3].f = size > 2 ? array[2] : 0.0f;
				p[4].f = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_DOUBLE: {
				TGLdouble *array = (TGLdouble *)((TGLbyte *)c->vertex_array + offset);
				p[1].f = array[0];
				p[2].f = array[1];
				p[3].f = size > 2 ? array[2] : 0.0f;
				p[4].f = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_INT: {
				TGLint *array = (TGLint *)((TGLbyte *)c->vertex_array + offset);
				p[1].f = array[0];
				p[2].f = array[1];
				p[3].f = size > 2 ? array[2] : 0.0f;
				p[4].f = size > 3 ? array[3] : 1.0f;
				break;
			}
		case TGL_SHORT: {
				TGLshort *array = (TGLshort *)((TGLbyte *)c->vertex_array + offset);
				p[1].f = array[0];
				p[2].f = array[1];
				p[3].f = size > 2 ? array[2] : 0.0f;
				p[4].f = size > 3 ? array[3] : 1.0f;
				break;
			}
		default:
			assert(0);
		}
		glopVertex(c, p);
	}
}

void glopDrawArrays(GLContext *c, GLParam *p) {
	GLParam array_element[2];
	GLParam begin[2];

	begin[1].i = p[1].i;
	glopBegin(c, begin);
	for (int i = 0; i < p[3].i; i++) {
		array_element[1].i = p[2].i + i;
		glopArrayElement(c, array_element);
	}
	glopEnd(c, NULL);
}

void glopDrawElements(GLContext *c, GLParam *p) {
	GLParam array_element[2];
	void *indices;
	GLParam begin[2];

	indices = (char *)p[4].p;
	begin[1].i = p[1].i;

	glopBegin(c, begin);
	for (int i = 0; i < p[2].i; i++) {
		switch (p[3].i) {
		case TGL_UNSIGNED_BYTE:
			array_element[1].i = ((TGLbyte *)indices)[i];
			break;
		case TGL_UNSIGNED_SHORT:
			array_element[1].i = ((TGLshort *)indices)[i];
			break;
		case TGL_UNSIGNED_INT:
			array_element[1].i = ((TGLint *)indices)[i];
			break;
		default:
			assert(0);
			break;
		}
		glopArrayElement(c, array_element);
	}
	glopEnd(c, NULL);
}

void glopEnableClientState(GLContext *c, GLParam *p) {
	c->client_states |= p[1].i;
}

void glopDisableClientState(GLContext *c, GLParam *p) {
	c->client_states &= p[1].i;
}

void glopVertexPointer(GLContext *c, GLParam *p) {
	c->vertex_array_size = p[1].i;
	c->vertex_array_type = p[2].i;
	c->vertex_array = p[4].p;
	switch (c->vertex_array_type) {
	case TGL_FLOAT:
		c->vertex_array_stride = p[3].i != 0 ? p[3].i : c->vertex_array_size * sizeof(TGLfloat);
		break;
	case TGL_DOUBLE:
		c->vertex_array_stride = p[3].i != 0 ? p[3].i : c->vertex_array_size * sizeof(TGLdouble);
		break;
	case TGL_INT:
		c->vertex_array_stride = p[3].i != 0 ? p[3].i : c->vertex_array_size * sizeof(TGLint);
		break;
	case TGL_SHORT:
		c->vertex_array_stride = p[3].i != 0 ? p[3].i : c->vertex_array_size * sizeof(TGLshort);
		break;
	default:
		assert(0);
		break;
	}
}

void glopColorPointer(GLContext *c, GLParam *p) {
	c->color_array_size = p[1].i;
	c->color_array_type = p[2].i;
	c->color_array = p[4].p;
	switch (c->color_array_type) {
	case TGL_BYTE:
	case TGL_UNSIGNED_BYTE:
		c->color_array_stride = p[3].i != 0 ? p[3].i : c->color_array_stride * sizeof(TGLbyte);
		break;
	case TGL_SHORT:
	case TGL_UNSIGNED_SHORT:
		c->color_array_stride = p[3].i != 0 ? p[3].i : c->color_array_stride * sizeof(TGLshort);
		break;
	case TGL_INT:
	case TGL_UNSIGNED_INT:
		c->color_array_stride = p[3].i != 0 ? p[3].i : c->color_array_stride * sizeof(TGLint);
		break;
	case TGL_FLOAT:
		c->color_array_stride = p[3].i != 0 ? p[3].i : c->color_array_stride * sizeof(TGLfloat);
		break;
	case TGL_DOUBLE:
		c->color_array_stride = p[3].i != 0 ? p[3].i : c->color_array_stride * sizeof(TGLdouble);
		break;
	default:
		assert(0);
		break;
	}
}

void glopNormalPointer(GLContext *c, GLParam *p) {
	c->normal_array_type = p[1].i;
	c->normal_array = p[3].p;
	switch (p[1].i) {
	case TGL_FLOAT:
		c->normal_array_stride = p[2].i != 0 ? p[2].i : 3 * sizeof(TGLfloat);
		break;
	case TGL_DOUBLE:
		c->normal_array_stride = p[2].i != 0 ? p[2].i : 3 * sizeof(TGLdouble);
		break;
	case TGL_INT:
		c->normal_array_stride = p[2].i != 0 ? p[2].i : 3 * sizeof(TGLint);
		break;
	case TGL_SHORT:
		c->normal_array_stride = p[2].i != 0 ? p[2].i : 3 * sizeof(TGLshort);
		break;
	default:
		assert(0);
		break;
	}
}

void glopTexCoordPointer(GLContext *c, GLParam *p) {
	c->texcoord_array_size = p[1].i;
	c->texcoord_array_type = p[2].i;
	c->texcoord_array = p[4].p;
	switch (c->texcoord_array_type) {
	case TGL_FLOAT:
		c->texcoord_array_stride = p[3].i != 0 ? p[3].i : c->texcoord_array_size * sizeof(TGLfloat);
		break;
	case TGL_DOUBLE:
		c->texcoord_array_stride = p[3].i != 0 ? p[3].i : c->texcoord_array_size * sizeof(TGLdouble);
		break;
	case TGL_INT:
		c->texcoord_array_stride = p[3].i != 0 ? p[3].i : c->texcoord_array_size * sizeof(TGLint);
		break;
	case TGL_SHORT:
		c->texcoord_array_stride = p[3].i != 0 ? p[3].i : c->texcoord_array_size * sizeof(TGLshort);
		break;
	default:
		assert(0);
		break;
	}
}

} // end of namespace TinyGL

void tglArrayElement(TGLint i) {
	TinyGL::GLParam p[2];
	p[0].op = TinyGL::OP_ArrayElement;
	p[1].i = i;
	gl_add_op(p);
}

void tglDrawArrays(TGLenum mode, TGLint first, TGLsizei count) {
	TinyGL::GLParam p[4];
	p[0].op = TinyGL::OP_DrawArrays;
	p[1].i = mode;
	p[2].i = first;
	p[3].i = count;
	gl_add_op(p);
}

void tglDrawElements(TGLenum mode, TGLsizei count, TGLenum type, const TGLvoid *indices) {
	TinyGL::GLParam p[5];
	p[0].op = TinyGL::OP_DrawElements;
	p[1].i = mode;
	p[2].i = count;
	p[3].i = type;
	p[4].p = const_cast<void *>(indices);
	gl_add_op(p);
}

void tglEnableClientState(TGLenum array) {
	TinyGL::GLParam p[2];
	p[0].op = TinyGL::OP_EnableClientState;

	switch (array) {
	case TGL_VERTEX_ARRAY:
		p[1].i = VERTEX_ARRAY;
		break;
	case TGL_NORMAL_ARRAY:
		p[1].i = NORMAL_ARRAY;
		break;
	case TGL_COLOR_ARRAY:
		p[1].i = COLOR_ARRAY;
		break;
	case TGL_TEXTURE_COORD_ARRAY:
		p[1].i = TEXCOORD_ARRAY;
		break;
	default:
		assert(0);
		break;
	}
	gl_add_op(p);
}

void tglDisableClientState(TGLenum array) {
	TinyGL::GLParam p[2];
	p[0].op = TinyGL::OP_DisableClientState;

	switch (array) {
	case TGL_VERTEX_ARRAY:
		p[1].i = ~VERTEX_ARRAY;
		break;
	case TGL_NORMAL_ARRAY:
		p[1].i = ~NORMAL_ARRAY;
		break;
	case TGL_COLOR_ARRAY:
		p[1].i = ~COLOR_ARRAY;
		break;
	case TGL_TEXTURE_COORD_ARRAY:
		p[1].i = ~TEXCOORD_ARRAY;
		break;
	default:
		assert(0);
		break;
	}
	gl_add_op(p);
}

void tglVertexPointer(TGLint size, TGLenum type, TGLsizei stride, const TGLvoid *pointer) {
	TinyGL::GLParam p[5];
	p[0].op = TinyGL::OP_VertexPointer;
	p[1].i = size;
	p[2].i = type;
	p[3].i = stride;
	p[4].p = const_cast<void *>(pointer);
	gl_add_op(p);
}

void tglColorPointer(TGLint size, TGLenum type, TGLsizei stride, const TGLvoid *pointer) {
	TinyGL::GLParam p[5];
	p[0].op = TinyGL::OP_ColorPointer;
	p[1].i = size;
	p[2].i = type;
	p[3].i = stride;
	p[4].p = const_cast<void *>(pointer);
	gl_add_op(p);
}

void tglNormalPointer(TGLenum type, TGLsizei stride, const TGLvoid *pointer) {
	TinyGL::GLParam p[4];
	p[0].op = TinyGL::OP_NormalPointer;
	p[1].i = type;
	p[2].i = stride;
	p[3].p = const_cast<void *>(pointer);
	gl_add_op(p);
}

void tglTexCoordPointer(TGLint size, TGLenum type, TGLsizei stride, const TGLvoid *pointer) {
	TinyGL::GLParam p[5];
	p[0].op = TinyGL::OP_TexCoordPointer;
	p[1].i = size;
	p[2].i = type;
	p[3].i = stride;
	p[4].p = const_cast<void *>(pointer);
	gl_add_op(p);
}
