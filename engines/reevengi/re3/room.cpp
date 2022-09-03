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

#include "common/stream.h"

#include "engines/reevengi/re3/room.h"

namespace Reevengi {

/*--- Defines ---*/

#include "rdt_scd_defs.gen.h"

/*--- Types ---*/

typedef struct {
	uint8 opcode;
	uint8 length;
} script_inst_len_t;

#include "rdt_scd_types.gen.h"

/*--- Variables ---*/

#include "rdt_scd_lengths.gen.c"

/*--- Class ---*/

RE3Room::RE3Room(Common::SeekableReadStream *stream): RE2Room(stream) {
	//
}

bool RE3Room::sceneExecInst(void) {
	// FIXME
	return Room::sceneExecInst();
}

int RE3Room::sceneInstLen(void) {
	if (!_scriptInst)
		return 0;

	for (unsigned int i=0; i< sizeof(inst_length)/sizeof(script_inst_len_t); i++) {
		if (inst_length[i].opcode == _scriptInst[0]) {
			return inst_length[i].length;
		}
	}

	return 0;
}

} // End of namespace Reevengi
