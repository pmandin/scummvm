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
#include "engines/reevengi/re2/entity_pld.h"

namespace Reevengi {

/*--- Defines ---*/

#define PLD_ANIM_FRAMES 0
#define PLD_SKELETON 1
#define PLD_MESHES 2
#define PLD_TEXTURE 3

/*--- Types ---*/

typedef struct {
	uint32 offset;
	uint32 length;
} emd_header_t;

RE2EntityPld::RE2EntityPld(Common::SeekableReadStream *stream): RE2Entity(stream) {
	emd_header_t *emd_header;
	uint32 *hdr_offsets, tim_offset;
	byte *tim_ptr;

	if (!_emdPtr)
		return;

	emd_header = (emd_header_t *) _emdPtr;

	hdr_offsets = (uint32 *)
		(&((char *) (_emdPtr))[FROM_LE_32(emd_header->offset)]);

	/* Offset 3: TIM image */
	tim_offset = FROM_LE_32(hdr_offsets[PLD_TEXTURE]);

	tim_ptr = &_emdPtr[tim_offset];

	Common::SeekableReadStream *mem_str = new Common::MemoryReadStream(tim_ptr, (_emdSize-16) - tim_offset);
	if (mem_str) {
		timTexture = new TimDecoder();
		timTexture->loadStream(*mem_str);
	}
}

int RE2EntityPld::getNumSection(int sectionType) {
	switch (sectionType) {
		case ENTITY_ANIM_FRAMES:
			return PLD_ANIM_FRAMES;
		case ENTITY_SKELETON:
			return PLD_SKELETON;
		case ENTITY_MESHES:
			return PLD_MESHES;
		case ENTITY_TEXTURE:
			return PLD_TEXTURE;
	}

	return -1;
}

} // End of namespace Reevengi
