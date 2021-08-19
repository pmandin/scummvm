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

	/* 16 bits values for angles following */
} emd_skel_anim_t;

typedef struct {
	int16 x,y,z,w;
} emd_vertex_t;

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
	uint32	vtx_offset;
	uint32	vtx_count;
	uint32	nor_offset;
	uint32	nor_count;
	uint32	mesh_offset;
	uint32	mesh_count;
	uint32	dummy;
} emd_mesh_t;

typedef struct {
	uint32 length;
	uint32 dummy;
	uint32 num_objects;
} emd_mesh_header_t;

typedef struct {
	emd_mesh_t triangles;
} emd_mesh_object_t;

RE1Entity::RE1Entity(Common::SeekableReadStream *stream): Entity(stream) {
	//debug(3, "re1: %d anims", getNumAnims());
}

int RE1Entity::getNumAnims(void) {
	uint32 *hdr_offsets, anim_offset;
	emd_anim_header_t *emd_anim_header;

	if (!_emdPtr)
		return 0;

	/*emd_header = (emd_header_t *) this->emd_file;*/

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[_emdSize-16]);

	/* Offset 1: Animation frames */
	anim_offset = FROM_LE_32(hdr_offsets[EMD_ANIM_FRAMES]);

	emd_anim_header = (emd_anim_header_t *)
		(&((char *) (_emdPtr))[anim_offset]);

	return (FROM_LE_16((emd_anim_header->offset) / sizeof(emd_anim_header_t)));
}

int RE1Entity::getNumChildren(int numMesh) {
	uint32 *hdr_offsets, skel_offset;
	emd_skel_header_t *emd_skel_header;
	emd_skel_data_t *emd_skel_data;

	if (!_emdPtr)
		return 0;

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[_emdSize-16]);

	/* Offset 0: Skeleton */
	skel_offset = FROM_LE_32(hdr_offsets[EMD_SKELETON]);

	emd_skel_header = (emd_skel_header_t *)
		(&((char *) (_emdPtr))[skel_offset]);
	emd_skel_data = (emd_skel_data_t *)
		(&((char *) (_emdPtr))[skel_offset+FROM_LE_16(emd_skel_header->relpos_offset)]);

	return (FROM_LE_16(emd_skel_data[numMesh].num_mesh));
}

int RE1Entity::getChild(int numMesh, int numChild) {
	uint32 *hdr_offsets, skel_offset;
	emd_skel_header_t *emd_skel_header;
	emd_skel_data_t *emd_skel_data;
	uint8 *mesh_numbers;

	if (!_emdPtr)
		return 0;

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[_emdSize-16]);

	/* Offset 0: Skeleton */
	skel_offset = FROM_LE_32(hdr_offsets[EMD_SKELETON]);

	emd_skel_header = (emd_skel_header_t *)
		(&((char *) (_emdPtr))[skel_offset]);
	emd_skel_data = (emd_skel_data_t *)
		(&((char *) (_emdPtr))[skel_offset+FROM_LE_16(emd_skel_header->relpos_offset)]);

	mesh_numbers = (uint8 *) emd_skel_data;
	return mesh_numbers[FROM_LE_16(emd_skel_data[numMesh].offset)+numChild];
}

void RE1Entity::drawMesh(int numMesh) {
	uint32 *hdr_offsets, mesh_offset;
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_object_t *emd_mesh_object;
	uint i;

	if (!_emdPtr)
		return;

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[_emdSize-16]);

	/* Offset 2: Meshes */
	mesh_offset = FROM_LE_32(hdr_offsets[EMD_MESHES]);
	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) _emdPtr)[mesh_offset]);

	if ((uint) numMesh>=FROM_LE_32(emd_mesh_header->num_objects))
		return;

	mesh_offset += sizeof(emd_mesh_header_t);
	emd_mesh_object = (emd_mesh_object_t *)
		(&((char *) _emdPtr)[mesh_offset]);

	emd_mesh_object = &emd_mesh_object[numMesh];
	for (i=0; i<emd_mesh_object->triangles.mesh_count; i++) {
		g_driver->beginTriangles();
		// TODO
		g_driver->endPrim();
	}
}

} // End of namespace Reevengi
