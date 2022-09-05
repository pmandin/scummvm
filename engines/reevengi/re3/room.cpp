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
#include "common/endian.h"
#include "common/stream.h"

#include "engines/reevengi/reevengi.h"
#include "engines/reevengi/game/door.h"
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

RE3Room::RE3Room(ReevengiEngine *game, Common::SeekableReadStream *stream): RE2Room(game, stream) {
	//
}

bool RE3Room::sceneExecInst(void) {
	script_inst_t *inst = (script_inst_t *) _scriptInst;

	if (!inst)
		return true;

	switch(inst->opcode) {
		case INST_DOOR_AOT_SET:
			{
				script_inst_door_aot_set_t *doorSet = (script_inst_door_aot_set_t *) inst;

				Door *new_door = new Door();
				new_door->_x = FROM_LE_16(doorSet->x);
				new_door->_y = FROM_LE_16(doorSet->z);
				new_door->_w = FROM_LE_16(doorSet->w);
				new_door->_h = FROM_LE_16(doorSet->h);

				new_door->_nextX = FROM_LE_16(doorSet->next_x);
				new_door->_nextY = FROM_LE_16(doorSet->next_y);
				new_door->_nextZ = FROM_LE_16(doorSet->next_z);
				new_door->_nextDir = FROM_LE_16(doorSet->next_dir);

				new_door->_nextStage = doorSet->next_stage+1;
				new_door->_nextRoom = doorSet->next_room;
				new_door->_nextCamera = doorSet->next_camera;

				this->_doors.push_back(new_door);

				debug(3, "0x%04x: DOOR_AOT_SET x=%d,y=%d,w=%d,h=%d", _scriptPC,
					new_door->_x,new_door->_y,new_door->_w,new_door->_h);
			}
			break;
		case INST_POS_SET:
			{
				script_inst_pos_set_t *posSet = (script_inst_pos_set_t *) inst;

				_game->_playerX = (int16) FROM_LE_16(posSet->x);
				_game->_playerY = (int16) FROM_LE_16(posSet->y);
				_game->_playerZ = (int16) FROM_LE_16(posSet->z);

				debug(3, "0x%04x: POS_SET x=%.3f,y=%.3f,z=%.3f", _scriptPC,
					_game->_playerX, _game->_playerY, _game->_playerZ);
			}
			break;
		case INST_DIR_SET:
			{
				script_inst_dir_set_t *dirSet = (script_inst_dir_set_t *) inst;

				_game->_playerA = FROM_LE_16(dirSet->cdir_y);

				debug(3, "0x%04x: DIR_SET a=%.3f", _scriptPC,
					_game->_playerA);
			}
			break;
		default:
			debug(3, "0x%04x: 0x%02x", _scriptPC, inst->opcode);
			break;
	}

	return false;
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
