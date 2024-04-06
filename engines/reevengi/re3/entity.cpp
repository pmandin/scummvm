/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/debug.h"
//#include "common/file.h"
#include "common/memstream.h"

#include "engines/reevengi/gfx/gfx_base.h"
#include "engines/reevengi/re3/entity.h"

namespace Reevengi {

/*--- Types ---*/

typedef struct {
	uint32 offset;
	uint32 length;
} emd_header_t;

typedef struct {
	uint16	count;
	uint16	offset;
	uint16	unknown[2];
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
	uint16	unknown[3];
	int16	pos[1];
	/*int16	speed[3];*/

	/* 12 bits values for angles following */
	/*angles[]; */
} emd_skel_anim_t;

typedef struct {
	int16 x,y,z,w;
} emd_vertex_t;

typedef struct {
	uint8 tu0,tv0;
	uint8 page, dummy1;

	uint8 tu1,tv1;
	uint8 clutid, v0;

	uint8 tu2,tv2;
	uint8 v1,v2;
} emd_triangle_t;

typedef struct {
	uint8 tu0,tv0;
	uint8 page, dummy1;

	uint8 tu1,tv1;
	uint8 clutid, dummy3;

	uint8 tu2,tv2;
	uint8 v0,v1;

	uint8 tu3,tv3;
	uint8 v2,v3;
} emd_quad_t;

typedef struct {
	uint16	vtx_offset;
	uint16	dummy0;
	uint16	nor_offset;
	uint16	dummy1;
	uint16	vtx_count;
	uint16	dummy2;
	uint16	tri_offset;
	uint16	dummy3;
	uint16	quad_offset;
	uint16	dummy4;
	uint16	tri_count;
	uint16	quad_count;
} emd_mesh_t;

typedef struct {
	uint32 length;
	uint32 num_objects;
} emd_mesh_header_t;

RE3Entity::RE3Entity(Common::SeekableReadStream *stream): Entity(stream) {
	//
}

int RE3Entity::getNumSection(int sectionType) {
	return -1;
}

void *RE3Entity::getEmdSection(int numSection) {
	emd_header_t *emd_header;
	uint32 *hdr_offsets;

	if (!_emdPtr)
		return nullptr;
	if (numSection<0)
		return nullptr;

	emd_header = (emd_header_t *) _emdPtr;

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[FROM_LE_32(emd_header->offset)]);

	return (void *)
		(&((char *) (_emdPtr))[FROM_LE_32(hdr_offsets[numSection])]);
}

int RE3Entity::getNumAnims(void) {
	emd_anim_header_t *emd_anim_header;

	emd_anim_header = (emd_anim_header_t *) getEmdSection(getNumSection(ENTITY_ANIM_FRAMES));
	if (!emd_anim_header)
		return 0;

	return (FROM_LE_16(emd_anim_header->offset) / sizeof(emd_anim_header_t));
}

int RE3Entity::getNumAnimFrames(void) {
	emd_anim_header_t *emd_anim_header;

	emd_anim_header = (emd_anim_header_t *) getEmdSection(getNumSection(ENTITY_ANIM_FRAMES));
	if (!emd_anim_header)
		return 1;

	return (FROM_LE_16(emd_anim_header[_numAnim].count));
}

void RE3Entity::getAnimAngles(int numMesh, int *x, int *y, int *z) {
	emd_anim_header_t *emd_anim_header;
	emd_skel_header_t *emd_skel_header;
	uint16 *ptr_skel_frame;
	uint8 *ptr_angles;
	int start_byte, num_skel_frame, max_frames;

	Entity::getAnimAngles(numMesh, x, y, z);

	if (!_emdPtr)
		return;
	if (_numAnim>=getNumAnims())
		return;
	max_frames = getNumAnimFrames();

	emd_anim_header = (emd_anim_header_t *) getEmdSection(getNumSection(ENTITY_ANIM_FRAMES));
	ptr_skel_frame = (uint16 *)
		(&((char *) (emd_anim_header))[FROM_LE_16(emd_anim_header[_numAnim].offset)]);
	num_skel_frame = FROM_LE_16(ptr_skel_frame[_numFrame % max_frames]) & ((1<<8)-1);

	emd_skel_header = (emd_skel_header_t *) getEmdSection(getNumSection(ENTITY_SKELETON));
	ptr_angles = (uint8 *)
		(&((char *) (emd_skel_header))[
			FROM_LE_16(emd_skel_header->anim_offset)
			+ num_skel_frame*FROM_LE_16(emd_skel_header->size)
			+ sizeof(emd_skel_anim_t)
		]);

	start_byte = (numMesh>>1) * 9;
	if ((numMesh & 1)==0) {
		/* XX, YX, YY, ZZ, -Z */
		*x = ptr_angles[start_byte] + ((ptr_angles[start_byte+1] & 15)<<8);
		*y = (ptr_angles[start_byte+1]>>4) + (ptr_angles[start_byte+2]<<4);
		*z = ptr_angles[start_byte+3] + ((ptr_angles[start_byte+4] & 15)<<8);
	} else {
		/* X-, XX, YY, ZY, ZZ */
		ptr_angles += 4;
		*x = (ptr_angles[start_byte]>>4) + (ptr_angles[start_byte+1]<<4);
		*y = ptr_angles[start_byte+2] + ((ptr_angles[start_byte+3] & 15)<<8);
		*z = (ptr_angles[start_byte+3]>>4) + (ptr_angles[start_byte+4]<<4);
	}
}

int RE3Entity::getNumChildren(int numMesh) {
	emd_skel_header_t *emd_skel_header;
	emd_skel_data_t *emd_skel_data;

	if (!_emdPtr)
		return 0;

	/* Offset 2: Skeleton */
	emd_skel_header = (emd_skel_header_t *) getEmdSection(getNumSection(ENTITY_SKELETON));

	emd_skel_data = (emd_skel_data_t *)
		(&((char *) (emd_skel_header))[FROM_LE_16(emd_skel_header->relpos_offset)]);

	return FROM_LE_16(emd_skel_data[numMesh].num_mesh);
}

int RE3Entity::getChild(int numMesh, int numChild) {
	emd_skel_header_t *emd_skel_header;
	emd_skel_data_t *emd_skel_data;
	uint8 *mesh_numbers;

	if (!_emdPtr)
		return 0;

	/* Offset 2: Skeleton */
	emd_skel_header = (emd_skel_header_t *) getEmdSection(getNumSection(ENTITY_SKELETON));

	emd_skel_data = (emd_skel_data_t *)
		(&((char *) (emd_skel_header))[FROM_LE_16(emd_skel_header->relpos_offset)]);

	mesh_numbers = (uint8 *) emd_skel_data;
	return mesh_numbers[FROM_LE_16(emd_skel_data[numMesh].offset)+numChild];
}

void RE3Entity::drawMesh(int numMesh) {
	emd_skel_header_t *emd_skel_header;
	emd_skel_relpos_t *emd_skel_relpos;
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_t *emd_mesh_array;
	emd_mesh_t *emd_mesh;
	uint i, tx_w, tx_h;
	int angles[3];

	if (!_emdPtr)
		return;

	/* Offset 0: Skeleton */
	emd_skel_header = (emd_skel_header_t *) getEmdSection(getNumSection(ENTITY_SKELETON));
	emd_skel_relpos = (emd_skel_relpos_t *) &emd_skel_header[1];
	emd_skel_relpos = &emd_skel_relpos[numMesh];

	g_driver->translate(
		(int16) FROM_LE_16(emd_skel_relpos->x),
		(int16) FROM_LE_16(emd_skel_relpos->y),
		(int16) FROM_LE_16(emd_skel_relpos->z)
	);

	// rotate for current animation frame
	getAnimAngles(numMesh, &angles[0], &angles[1], &angles[2]);
	g_driver->rotate((angles[0] * 360.0f) / 4096.0f, 1.0f, 0.0f, 0.0f);
	g_driver->rotate((angles[1] * 360.0f) / 4096.0f, 0.0f, 1.0f, 0.0f);
	g_driver->rotate((angles[2] * 360.0f) / 4096.0f, 0.0f, 0.0f, 1.0f);

	/* Offset 7: Meshes */
	emd_mesh_header = (emd_mesh_header_t *) getEmdSection(getNumSection(ENTITY_MESHES));
	emd_mesh_array = (emd_mesh_t *) &emd_mesh_header[1];
	emd_mesh = (emd_mesh_t *) &emd_mesh_array[numMesh];

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

	/*debug(3, "mesh: %d vtx (%04x)", FROM_LE_32(emd_mesh->vtx_count), FROM_LE_32(emd_mesh->vtx_offset));
	debug(3, "mesh:    nor (%04x)", FROM_LE_32(emd_mesh->nor_offset));
	debug(3, "mesh: %d tri (%04x)", FROM_LE_32(emd_mesh->tri_count), FROM_LE_32(emd_mesh->tri_offset));
	debug(3, "mesh: %d quad (%04x)", FROM_LE_32(emd_mesh->quad_count), FROM_LE_32(emd_mesh->quad_offset));*/

	/* Draw triangles */
	for (i=0; i<FROM_LE_32(emd_mesh->tri_count); i++) {
		emd_vertex_t *emd_vtx, *emd_nor;
		emd_triangle_t *emd_tri;
		int idx_vtx, idx_nor, tx_page;

		/* Vertex array */
		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_mesh_array)[FROM_LE_32(emd_mesh->vtx_offset)]);

		/* Normal array */
		emd_nor = (emd_vertex_t *)
			(&((char *) emd_mesh_array)[FROM_LE_32(emd_mesh->nor_offset)]);

		/* Vertices,normal index, texcoord */
		emd_tri = (emd_triangle_t *)
			(&((char *) emd_mesh_array)[FROM_LE_32(emd_mesh->tri_offset)]);

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
		idx_vtx = idx_nor = FROM_LE_16(emd_tri[i].v0);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->texCoord2f(
			(float) (emd_tri[i].tu1 + tx_page) / tx_w,
			(float) emd_tri[i].tv1 / tx_h
		);
		idx_vtx = idx_nor = FROM_LE_16(emd_tri[i].v1);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->texCoord2f(
			(float) (emd_tri[i].tu2 + tx_page) / tx_w,
			(float) emd_tri[i].tv2 / tx_h
		);
		idx_vtx = idx_nor = FROM_LE_16(emd_tri[i].v2);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->endPrim();
	}

	/* Draw quads */
	for (i=0; i<FROM_LE_32(emd_mesh->quad_count); i++) {
		emd_vertex_t *emd_vtx, *emd_nor;
		emd_quad_t *emd_quad;
		int idx_vtx, idx_nor, tx_page;

		/* Vertex array */
		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_mesh_array)[FROM_LE_32(emd_mesh->vtx_offset)]);

		/* Normal array */
		emd_nor = (emd_vertex_t *)
			(&((char *) emd_mesh_array)[FROM_LE_32(emd_mesh->nor_offset)]);

		/* Vertices,normal index, texcoord */
		emd_quad = (emd_quad_t *)
			(&((char *) emd_mesh_array)[FROM_LE_32(emd_mesh->quad_offset)]);

		tx_page = (FROM_LE_16(emd_quad[i].page)<<1) & 0xff;

		new_clutid = FROM_LE_16(emd_quad[i].clutid) & 3;
		if (clutid != new_clutid) {
			setTexture(new_clutid);
			clutid = new_clutid;
		}

		g_driver->beginQuads();

		g_driver->texCoord2f(
			(float) (emd_quad[i].tu0 + tx_page) / tx_w,
			(float) emd_quad[i].tv0 / tx_h
		);
		idx_vtx = idx_nor = FROM_LE_16(emd_quad[i].v0);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->texCoord2f(
			(float) (emd_quad[i].tu1 + tx_page) / tx_w,
			(float) emd_quad[i].tv1 / tx_h
		);
		idx_vtx = idx_nor = FROM_LE_16(emd_quad[i].v1);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->texCoord2f(
			(float) (emd_quad[i].tu3 + tx_page) / tx_w,
			(float) emd_quad[i].tv3 / tx_h
		);
		idx_vtx = idx_nor = FROM_LE_16(emd_quad[i].v3);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->texCoord2f(
			(float) (emd_quad[i].tu2 + tx_page) / tx_w,
			(float) emd_quad[i].tv2 / tx_h
		);
		idx_vtx = idx_nor = FROM_LE_16(emd_quad[i].v2);
		g_driver->normal3f(
			(int16) FROM_LE_16(emd_nor[idx_nor].x),
			(int16) FROM_LE_16(emd_nor[idx_nor].y),
			(int16) FROM_LE_16(emd_nor[idx_nor].z)
		);
		g_driver->vertex3f(
			(int16) FROM_LE_16(emd_vtx[idx_vtx].x),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].y),
			(int16) FROM_LE_16(emd_vtx[idx_vtx].z)
		);

		g_driver->endPrim();
	}
}

} // End of namespace Reevengi
