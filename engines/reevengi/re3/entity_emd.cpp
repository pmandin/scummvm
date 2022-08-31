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
#include "engines/reevengi/re3/entity_emd.h"

namespace Reevengi {

/*--- Defines ---*/

#define EMD_ANIM_FRAMES 2
#define EMD_SKELETON 3
#define EMD_MESHES 14

RE3EntityEmd::RE3EntityEmd(Common::SeekableReadStream *stream): RE3Entity(stream) {
	//
}

int RE3EntityEmd::getNumSection(int sectionType) {
	switch (sectionType) {
		case ENTITY_ANIM_FRAMES:
			return EMD_ANIM_FRAMES;
		case ENTITY_SKELETON:
			return EMD_SKELETON;
		case ENTITY_MESHES:
			return EMD_MESHES;
	}

	return -1;
}

} // End of namespace Reevengi
