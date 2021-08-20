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

#include "common/debug.h"
//#include "common/file.h"
#include "common/memstream.h"

#include "engines/reevengi/gfx/gfx_base.h"
#include "engines/reevengi/re2/entity.h"

namespace Reevengi {

/*--- Defines ---*/

#define EMD_ANIM_FRAMES 1
#define EMD_SKELETON 2
#define EMD_MESHES 7

/*--- Types ---*/

typedef struct {
	uint32 offset;
	uint32 length;
} emd_header_t;

typedef struct {
	uint16	count;
	uint16	offset;
} emd_anim_header_t;

typedef struct {
	int16	x,y,z;
} emd_skel_relpos_t;

typedef struct {
	uint16	num_mesh;
	uint16	offset;
} emd_skel_data_t;

typedef struct {
	uint16	relpos_offset;
	uint16	anim_offset;
	uint16	count;
	uint16	size;
} emd_skel_header_t;

typedef struct {
	int16	pos[3];
	int16	speed[3];

	/* 12 bits values for angles following */
	/* char angles[74];	 */
} emd_skel_anim_t;

typedef struct {
	int16 x,y,z,w;
} emd_vertex_t;

typedef struct {
	int16	n0,v0;
	int16	n1,v1;
	int16	n2,v2;
} emd_triangle_t;

typedef struct {
	uint8 u0,v0;
	uint16 page;
	uint8 u1,v1;
	uint16 clutid;
	uint8 u2,v2;
	uint16 dummy;
} emd_triangle_tex_t;

typedef struct {
	uint16	n0,v0;
	uint16	n1,v1;
	uint16	n2,v2;
	uint16	n3,v3;
} emd_quad_t;

typedef struct {
	uint8 u0,v0;
	uint16 page;
	uint8 u1,v1;
	uint16 clutid;
	uint8 u2,v2;
	uint16 dummy0;
	uint8 u3,v3;
	uint16 dummy1;
} emd_quad_tex_t;

typedef struct {
	uint32	vtx_offset;
	uint32	vtx_count;
	uint32	nor_offset;
	uint32	nor_count;
	uint32	mesh_offset;
	uint32	mesh_count;
	uint32	tex_offset;
} emd_mesh_t;

typedef struct {
	uint32 length;
	uint32 dummy;
	uint32 num_objects;
} emd_mesh_header_t;

typedef struct {
	emd_mesh_t triangles;
	emd_mesh_t quads;
} emd_mesh_object_t;

RE2Entity::RE2Entity(Common::SeekableReadStream *stream): Entity(stream) {
	//
}

void *RE2Entity::getEmdSection(int numSection) {
	emd_header_t *emd_header;
	uint32 *hdr_offsets;

	if (!_emdPtr)
		return nullptr;

	emd_header = (emd_header_t *) _emdPtr;

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[FROM_LE_32(emd_header->offset)]);

	return (void *)
		(&((char *) (_emdPtr))[FROM_LE_32(hdr_offsets[numSection])]);
}

int RE2Entity::getNumAnims(void) {
	emd_anim_header_t *emd_anim_header;

	if (!_emdPtr)
		return 0;

	emd_anim_header = (emd_anim_header_t *) getEmdSection(EMD_ANIM_FRAMES);

	return (FROM_LE_16(emd_anim_header->offset) / sizeof(emd_anim_header_t));
}

int RE2Entity::getNumChildren(int numMesh) {
	emd_skel_header_t *emd_skel_header;
	emd_skel_data_t *emd_skel_data;

	if (!_emdPtr)
		return 0;

	/* Offset 2: Skeleton */
	emd_skel_header = (emd_skel_header_t *) getEmdSection(EMD_SKELETON);

	emd_skel_data = (emd_skel_data_t *)
		(&((char *) (emd_skel_header))[FROM_LE_16(emd_skel_header->relpos_offset)]);

	return FROM_LE_16(emd_skel_data[numMesh].num_mesh);
}

int RE2Entity::getChild(int numMesh, int numChild) {
	emd_skel_header_t *emd_skel_header;
	emd_skel_data_t *emd_skel_data;
	uint8 *mesh_numbers;

	if (!_emdPtr)
		return 0;

	/* Offset 2: Skeleton */
	emd_skel_header = (emd_skel_header_t *) getEmdSection(EMD_SKELETON);

	emd_skel_data = (emd_skel_data_t *)
		(&((char *) (emd_skel_header))[FROM_LE_16(emd_skel_header->relpos_offset)]);

	mesh_numbers = (uint8 *) emd_skel_data;
	return mesh_numbers[FROM_LE_16(emd_skel_data[numMesh].offset)+numChild];
}

void RE2Entity::drawMesh(int numMesh) {
	if (!_emdPtr)
		return;
}

#if 0
void RE2Entity::drawMesh(int numMesh) {
	uint32 *hdr_offsets, skel_offset, mesh_offset;
	emd_skel_relpos_t *emd_skel_relpos;
	emd_mesh_t *emd_mesh, *emd_mesh_array;
	uint i, tx_w, tx_h;

	if (!_emdPtr)
		return;

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[_emdSize-16]);

	/* Offset 0: Skeleton */
	skel_offset = FROM_LE_32(hdr_offsets[EMD_SKELETON]);

	emd_skel_relpos = (emd_skel_relpos_t *)
		(&((char *) (_emdPtr))[skel_offset+sizeof(emd_skel_header_t)]);
	emd_skel_relpos = &emd_skel_relpos[numMesh];

	g_driver->translate(
		(int16) FROM_LE_16(emd_skel_relpos->x),
		(int16) FROM_LE_16(emd_skel_relpos->y),
		(int16) FROM_LE_16(emd_skel_relpos->z)
	);

	// TODO: Handle animation

	/* Offset 2: Meshes */
	mesh_offset = FROM_LE_32(hdr_offsets[EMD_MESHES]);
	/*emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) _emdPtr)[mesh_offset]);*/

	/*if ((uint) numMesh>=FROM_LE_32(emd_mesh_header->num_objects))
		return;*/

	mesh_offset += sizeof(emd_mesh_header_t);
	emd_mesh_array = (emd_mesh_t *)
		(&((char *) _emdPtr)[mesh_offset]);

	emd_mesh = &emd_mesh_array[numMesh];
	/*debug(3, "mesh %d: %d", numMesh, FROM_LE_32(emd_mesh->mesh_count));
	debug(3, " vtx 0x%08x", FROM_LE_32(emd_mesh->vtx_offset));
	debug(3, " nor 0x%08x", FROM_LE_32(emd_mesh->nor_offset));
	debug(3, " msh 0x%08x", FROM_LE_32(emd_mesh->mesh_offset));*/

	/* Texture dimensions */
	tx_w = 256;
	tx_h = 128;
	if (timTexture) {
		const Graphics::Surface *timSurface = timTexture->getSurface();
		if (timSurface) {
			tx_w = timSurface->w;
			tx_h = timSurface->h;
		}
	}

	uint16 clutid = 255, new_clutid;

	for (i=0; i<FROM_LE_32(emd_mesh->mesh_count); i++) {
		emd_vertex_t *emd_vtx, *emd_nor;
		emd_triangle_t *emd_tri;
		int idx_vtx, idx_nor, tx_page;

		/* Vertex array */
		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_mesh_array)[FROM_LE_32(emd_mesh->vtx_offset)]);

		/* Normal array */
		emd_nor = (emd_vertex_t *)
			(&((char *) emd_mesh_array)[FROM_LE_32(emd_mesh->nor_offset)]);

		/* Vertices,normal index  and texcoords */
		emd_tri = (emd_triangle_t *)
			(&((char *) emd_mesh_array)[FROM_LE_32(emd_mesh->mesh_offset)]);

		tx_page = (FROM_LE_16(emd_tri[i].page)<<1) & 0xff;

		new_clutid = FROM_LE_16(emd_tri[i].clutid) & 3;
		if (clutid != new_clutid) {
			setTexture(new_clutid);
			clutid = new_clutid;
		}

		g_driver->beginTriangles();

		g_driver->texCoord2f(
			(float) (emd_tri[i].tu0 + tx_page) / tx_w,
			(float) emd_tri[i].tv0 / tx_h
		);
		idx_nor = FROM_LE_16(emd_tri[i].n0);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		idx_vtx = FROM_LE_16(emd_tri[i].v0);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->texCoord2f(
			(float) (emd_tri[i].tu1 + tx_page) / tx_w,
			(float) emd_tri[i].tv1 / tx_h
		);
		idx_nor = FROM_LE_16(emd_tri[i].n1);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		idx_vtx = FROM_LE_16(emd_tri[i].v1);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->texCoord2f(
			(float) (emd_tri[i].tu2 + tx_page) / tx_w,
			(float) emd_tri[i].tv2 / tx_h
		);
		idx_nor = FROM_LE_16(emd_tri[i].n2);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		idx_vtx = FROM_LE_16(emd_tri[i].v2);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->endPrim();
	}
}
#endif

} // End of namespace Reevengi
