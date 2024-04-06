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
#include "engines/reevengi/re1/entity.h"

namespace Reevengi {

/*--- Defines ---*/

#define EMD_SKELETON 0
#define EMD_ANIM_FRAMES 1
#define EMD_MESHES 2
#define EMD_TIM 3

/*--- Types ---*/

typedef struct {
	uint32 offset;
	uint32 length;
} emd_header_t;

/* Section 0 Skeleton */

typedef struct {
	uint16	bone_offset;
	uint16	anim_offset;
	uint16	count;
	uint16	size;
} emd_skel_header_t;

typedef struct {
	int16	x,y,z;
} emd_skel_relpos_t;

typedef struct {
	uint16	num_mesh;
	uint16	offset;
} emd_skel_bone_t;

/* Section 1 Animations */

typedef struct {
	uint16	count;
	uint16	offset;
} emd_anim_header_t;

typedef struct {
	int16	pos[3];
	int16	speed[3];

	/* 16 bits values for angles following */
	/* int16 angles[3*n] */
} emd_skel_anim_t;

/* Section 2 Meshes */

typedef struct {
	uint32 length;
	uint32 dummy;
	uint32 num_objects;
} emd_mesh_header_t;

typedef struct {
	uint32	vtx_offset;	/* offset to emd_vertex_t array */
	uint32	vtx_count;
	uint32	nor_offset;	/* offset to emd_vertex_t array */
	uint32	nor_count;
	uint32	mesh_offset;/* offset to emd_triangle_t array */
	uint32	mesh_count;
	uint32	dummy;
} emd_mesh_t;

typedef struct {
	uint32 id;

	uint8 tu0,tv0;
	uint16 page;
	uint8 tu1,tv1;
	uint16 clutid;
	uint8 tu2,tv2;
	uint16 dummy;

	uint16	n0,v0;
	uint16	n1,v1;
	uint16	n2,v2;
} emd_triangle_t;

typedef struct {
	int16 x,y,z,w;
} emd_vertex_t;

RE1Entity::RE1Entity(Common::SeekableReadStream *stream): Entity(stream) {
	uint32 *hdr_offsets, tim_offset;
	byte *tim_ptr;

	//debug(3, "re1: %d anims", getNumAnims());

	if (!_emdPtr)
		return;

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[_emdSize-16]);

	/* Offset 3: TIM image */
	tim_offset = FROM_LE_32(hdr_offsets[EMD_TIM]);

	tim_ptr = &_emdPtr[tim_offset];

	Common::SeekableReadStream *mem_str = new Common::MemoryReadStream(tim_ptr, (_emdSize-16) - tim_offset);
	if (mem_str) {
		timTexture = new TimDecoder();
		timTexture->loadStream(*mem_str);
	}
}

void *RE1Entity::getEmdSection(int numSection) {
	uint32 *hdr_offsets;

	if (!_emdPtr)
		return nullptr;

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[_emdSize-16]);

	return (void *)
		(&((char *) (_emdPtr))[FROM_LE_32(hdr_offsets[numSection])]);
}

int RE1Entity::getNumAnims(void) {
	emd_anim_header_t *emd_anim_header;

	/* Offset 1: Animation frames */
	emd_anim_header = (emd_anim_header_t *) getEmdSection(EMD_ANIM_FRAMES);
	if (!emd_anim_header)
		return 0;

	return (FROM_LE_16((emd_anim_header->offset) / sizeof(emd_anim_header_t)));
}

int RE1Entity::getNumAnimFrames(void) {
	emd_anim_header_t *emd_anim_header;

	emd_anim_header = (emd_anim_header_t *) getEmdSection(EMD_ANIM_FRAMES);
	if (!emd_anim_header)
		return 1;

	return (FROM_LE_16(emd_anim_header[_numAnim].count));
}

void RE1Entity::getAnimAngles(int numMesh, int *x, int *y, int *z) {
	emd_anim_header_t *emd_anim_header;
	emd_skel_header_t *emd_skel_header;
	uint32 *ptr_skel_frame;
	int16 *ptr_angles;
	int num_skel_frame, max_frames;

	Entity::getAnimAngles(numMesh, x, y, z);

	if (!_emdPtr)
		return;
	if (_numAnim>=getNumAnims())
		return;
	max_frames = getNumAnimFrames();

	emd_anim_header = (emd_anim_header_t *) getEmdSection(EMD_ANIM_FRAMES);
	ptr_skel_frame = (uint32 *)
		(&((char *) (emd_anim_header))[FROM_LE_16(emd_anim_header[_numAnim].offset)]);
	num_skel_frame = FROM_LE_32(ptr_skel_frame[_numFrame % max_frames]) & ((1<<16)-1);

	emd_skel_header = (emd_skel_header_t *) getEmdSection(EMD_SKELETON);
	ptr_angles = (int16 *)
		(&((char *) (emd_skel_header))[
			FROM_LE_16(emd_skel_header->anim_offset)
			+ num_skel_frame*FROM_LE_16(emd_skel_header->size)
			+ sizeof(emd_skel_anim_t)
		]);

	*x = FROM_LE_16(ptr_angles[numMesh*3]) & 4095;
	*y = FROM_LE_16(ptr_angles[numMesh*3+1]) & 4095;
	*z = FROM_LE_16(ptr_angles[numMesh*3+2]) & 4095;
}

int RE1Entity::getNumChildren(int numMesh) {
	emd_skel_header_t *emd_skel_header;
	emd_skel_bone_t *emd_skel_bone;

	if (!_emdPtr)
		return 0;

	/* Offset 0: Skeleton */
	emd_skel_header = (emd_skel_header_t *) getEmdSection(EMD_SKELETON);

	emd_skel_bone = (emd_skel_bone_t *)
		(&((char *) (emd_skel_header))[FROM_LE_16(emd_skel_header->bone_offset)]);

	return (FROM_LE_16(emd_skel_bone[numMesh].num_mesh));
}

int RE1Entity::getChild(int numMesh, int numChild) {
	emd_skel_header_t *emd_skel_header;
	emd_skel_bone_t *emd_skel_bone;
	uint8 *mesh_numbers;

	if (!_emdPtr)
		return 0;

	/* Offset 0: Skeleton */
	emd_skel_header = (emd_skel_header_t *) getEmdSection(EMD_SKELETON);

	emd_skel_bone = (emd_skel_bone_t *)
		(&((char *) (emd_skel_header))[FROM_LE_16(emd_skel_header->bone_offset)]);

	mesh_numbers = (uint8 *) emd_skel_bone;
	return mesh_numbers[FROM_LE_16(emd_skel_bone[numMesh].offset)+numChild];
}

void RE1Entity::drawMesh(int numMesh) {
	//uint32 *hdr_offsets, skel_offset, mesh_offset;
	emd_skel_header_t *emd_skel_header;
	emd_skel_relpos_t *emd_skel_relpos;
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_t *emd_mesh, *emd_mesh_array;
	uint i, tx_w, tx_h;
	int angles[3];

	if (!_emdPtr)
		return;

	/* Offset 0: Skeleton */
	emd_skel_header = (emd_skel_header_t *) getEmdSection(EMD_SKELETON);
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

	/* Offset 2: Meshes */
	emd_mesh_header = (emd_mesh_header_t *) getEmdSection(EMD_MESHES);
	emd_mesh_array = (emd_mesh_t *) &emd_mesh_header[1];
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

} // End of namespace Reevengi
