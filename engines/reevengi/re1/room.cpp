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
#include "common/memstream.h"
#include "common/stream.h"

#include "engines/reevengi/reevengi.h"
#include "engines/reevengi/game/door.h"
#include "engines/reevengi/gfx/gfx_base.h"
#include "engines/reevengi/re1/room.h"

namespace Reevengi {

/*--- Defines ---*/

#define RDT1_OFFSET_CAM_SWITCHES	0
#define RDT1_OFFSET_COLLISION		1
#define RDT1_OFFSET_INIT_SCRIPT		6
#define RDT1_OFFSET_ROOM_SCRIPT		7
#define RDT1_OFFSET_EVENTS		8
#define RDT1_OFFSET_TEXT		11

#define RDT1_RVD_BOUNDARY 9

#include "rdt_scd_defs.gen.h"

/*--- Types ---*/

typedef struct {
	int32	x,y,z;
	uint32	unknown[2];
} rdt1_header_part_t;

typedef struct {
	uint8	unknown0;
	uint8	numCameras;
	uint8	unknown1[4];
	uint16	unknown2[3];
	rdt1_header_part_t	unknown3[3];
	uint32	offsets[19];
} rdt1_header_t;

/* Cameras and masks */

typedef struct {
	uint32 priOffset;
	uint32 timOffset;
	int32 fromX, fromY, fromZ;
	int32 toX, toY, toZ;
	uint32 unknown[3];
} rdt1_rid_t;

typedef struct {
	uint16 numOffset;
	uint16 numMasks;
} rdt1_pri_header_t;

typedef struct {
	uint16 count;
	uint16 unknown;
	int16 dstX, dstY;
} rdt1_pri_offset_t;

typedef struct {
	uint8 srcX, srcY;
	uint8 dstX, dstY;
	uint16 depth;
	uint8 unknown;
	uint8 size;
} rdt1_pri_square_t;

typedef struct {
	uint8 srcX, srcY;
	uint8 dstX, dstY;
	uint16 depth, zero;
	uint16 width, height;
} rdt1_pri_rect_t;

/* Cameras switches, offset 0 */

typedef struct {
	uint16 toCam, fromCam;	/* to = RDT_RVD_BOUNDARY if boundary, not camera switch */
	int16 x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	int16 x2,y2;
	int16 x3,y3;
	int16 x4,y4;
} rdt1_rvd_t;

typedef struct {
	uint8 opcode;
	uint8 length;
} script_inst_len_t;

#include "rdt_scd_types.gen.h"

/*--- Variables ---*/

#include "rdt_scd_lengths.gen.c"

/*--- Class ---*/

RE1Room::RE1Room(ReevengiEngine *game, Common::SeekableReadStream *stream): Room(game, stream) {
	//
}

int RE1Room::getNumCameras(void) {
	if (!_roomPtr)
		return 0;

	return ((rdt1_header_t *) _roomPtr)->numCameras;
}

void RE1Room::getCameraPos(int numCamera, RdtCameraPos_t *cameraPos) {
	if (!_roomPtr)
		return;

	rdt1_rid_t *cameraPosArray = (rdt1_rid_t *) ((byte *) &_roomPtr[sizeof(rdt1_header_t)]);

	cameraPos->fromX = FROM_LE_32( cameraPosArray[numCamera].fromX );
	cameraPos->fromY = FROM_LE_32( cameraPosArray[numCamera].fromY );
	cameraPos->fromZ = FROM_LE_32( cameraPosArray[numCamera].fromZ );

	cameraPos->toX = FROM_LE_32( cameraPosArray[numCamera].toX );
	cameraPos->toY = FROM_LE_32( cameraPosArray[numCamera].toY );
	cameraPos->toZ = FROM_LE_32( cameraPosArray[numCamera].toZ );
}

int RE1Room::checkCamSwitch(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos) {
	if (!_roomPtr)
		return -1;

	int32 offset = FROM_LE_32( ((rdt1_header_t *) _roomPtr)->offsets[RDT1_OFFSET_CAM_SWITCHES] );
	rdt1_rvd_t *camSwitchArray = (rdt1_rvd_t *) ((byte *) &_roomPtr[offset]);

	while (FROM_LE_16(camSwitchArray->toCam) != 0xffff) {
		bool isBoundary = (FROM_LE_16(camSwitchArray->toCam) == RDT1_RVD_BOUNDARY);

		if (isBoundary) {
			/* boundary, not a switch */
		} else {
			/* Check objet triggered camera switch */
			Math::Vector2d quad[4];
			quad[0] = Math::Vector2d(FROM_LE_16(camSwitchArray->x1), FROM_LE_16(camSwitchArray->y1));
			quad[1] = Math::Vector2d(FROM_LE_16(camSwitchArray->x2), FROM_LE_16(camSwitchArray->y2));
			quad[2] = Math::Vector2d(FROM_LE_16(camSwitchArray->x3), FROM_LE_16(camSwitchArray->y3));
			quad[3] = Math::Vector2d(FROM_LE_16(camSwitchArray->x4), FROM_LE_16(camSwitchArray->y4));

			if ((curCam==camSwitchArray->fromCam) && !isInside(fromPos, quad) && isInside(toPos, quad)) {
				return camSwitchArray->toCam;
			}
		}

		++camSwitchArray;
	}

	return -1;
}

void RE1Room::drawCamSwitch(int curCam) {
	if (!_roomPtr)
		return;

	g_driver->setColor(1.0, 0.75, 0.5);

	int32 offset = FROM_LE_32( ((rdt1_header_t *) _roomPtr)->offsets[RDT1_OFFSET_CAM_SWITCHES] );
	rdt1_rvd_t *camSwitchArray = (rdt1_rvd_t *) ((byte *) &_roomPtr[offset]);

	while (FROM_LE_16(camSwitchArray->toCam) != 0xffff) {
		bool isBoundary = (FROM_LE_16(camSwitchArray->toCam) == RDT1_RVD_BOUNDARY);

		if (isBoundary) {
			/* boundary, not a switch */
		} else {
			Math::Vector3d quad[4];
			quad[0] = Math::Vector3d(FROM_LE_16(camSwitchArray->x1), 0, FROM_LE_16(camSwitchArray->y1));
			quad[1] = Math::Vector3d(FROM_LE_16(camSwitchArray->x2), 0, FROM_LE_16(camSwitchArray->y2));
			quad[2] = Math::Vector3d(FROM_LE_16(camSwitchArray->x3), 0, FROM_LE_16(camSwitchArray->y3));
			quad[3] = Math::Vector3d(FROM_LE_16(camSwitchArray->x4), 0, FROM_LE_16(camSwitchArray->y4));

			if (curCam==camSwitchArray->fromCam) {
				g_driver->line(quad[0], quad[1]);
				g_driver->line(quad[1], quad[2]);
				g_driver->line(quad[2], quad[3]);
				g_driver->line(quad[3], quad[0]);
			}
		}

		++camSwitchArray;
	}
}

bool RE1Room::checkCamBoundary(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos) {
	if (!_roomPtr)
		return false;

	int32 offset = FROM_LE_32( ((rdt1_header_t *) _roomPtr)->offsets[RDT1_OFFSET_CAM_SWITCHES] );
	rdt1_rvd_t *camBoundaryArray = (rdt1_rvd_t *) ((byte *) &_roomPtr[offset]);

	while (FROM_LE_16(camBoundaryArray->toCam) != 0xffff) {
		bool isBoundary = (FROM_LE_16(camBoundaryArray->toCam) == RDT1_RVD_BOUNDARY);

		if (isBoundary) {
			/* Check objet got outside boundary */
			Math::Vector2d quad[4];
			quad[0] = Math::Vector2d(FROM_LE_16(camBoundaryArray->x1), FROM_LE_16(camBoundaryArray->y1));
			quad[1] = Math::Vector2d(FROM_LE_16(camBoundaryArray->x2), FROM_LE_16(camBoundaryArray->y2));
			quad[2] = Math::Vector2d(FROM_LE_16(camBoundaryArray->x3), FROM_LE_16(camBoundaryArray->y3));
			quad[3] = Math::Vector2d(FROM_LE_16(camBoundaryArray->x4), FROM_LE_16(camBoundaryArray->y4));

			if ((curCam==camBoundaryArray->fromCam) && isInside(fromPos, quad) && !isInside(toPos, quad)) {
				return true;
			}
		} else {
			/* Switch, not a boundary */
		}

		++camBoundaryArray;
	}

	return false;
}

void RE1Room::drawCamBoundary(int curCam) {
	if (!_roomPtr)
		return;

	g_driver->setColor(1.0, 0.0, 0.0);

	int32 offset = FROM_LE_32( ((rdt1_header_t *) _roomPtr)->offsets[RDT1_OFFSET_CAM_SWITCHES] );
	rdt1_rvd_t *camBoundaryArray = (rdt1_rvd_t *) ((byte *) &_roomPtr[offset]);

	while (FROM_LE_16(camBoundaryArray->toCam) != 0xffff) {
		bool isBoundary = (FROM_LE_16(camBoundaryArray->toCam) == RDT1_RVD_BOUNDARY);

		if (isBoundary) {
			Math::Vector3d quad[4];
			quad[0] = Math::Vector3d(FROM_LE_16(camBoundaryArray->x1), 0, FROM_LE_16(camBoundaryArray->y1));
			quad[1] = Math::Vector3d(FROM_LE_16(camBoundaryArray->x2), 0, FROM_LE_16(camBoundaryArray->y2));
			quad[2] = Math::Vector3d(FROM_LE_16(camBoundaryArray->x3), 0, FROM_LE_16(camBoundaryArray->y3));
			quad[3] = Math::Vector3d(FROM_LE_16(camBoundaryArray->x4), 0, FROM_LE_16(camBoundaryArray->y4));

			if (curCam==camBoundaryArray->fromCam) {
				g_driver->line(quad[0], quad[1]);
				g_driver->line(quad[1], quad[2]);
				g_driver->line(quad[2], quad[3]);
				g_driver->line(quad[3], quad[0]);
			}
		} else {
			/* Switch, not a boundary */
		}

		++camBoundaryArray;
	}
}

void RE1Room::drawMasks(int numCamera) {
	if (!_roomPtr)
		return;

	rdt1_rid_t *cameraPosArray = (rdt1_rid_t *) ((byte *) &_roomPtr[sizeof(rdt1_header_t)]);
	int32 offset = FROM_LE_32( cameraPosArray[numCamera].priOffset );
	if (offset == -1)
		return;

	rdt1_pri_header_t *maskHeaderArray = (rdt1_pri_header_t *) ((byte *) &_roomPtr[offset]);
	int16 countOffsets = FROM_LE_16(maskHeaderArray->numOffset);
	if (countOffsets == -1)
		return;
	offset += sizeof(rdt1_pri_header_t);

	rdt1_pri_offset_t *maskOffsetArray = (rdt1_pri_offset_t *) ((byte *) &_roomPtr[offset]);
	offset += sizeof(rdt1_pri_offset_t) * countOffsets;

	for (int numOffset=0; numOffset<countOffsets; numOffset++) {
		int16 maskCount = FROM_LE_16(maskOffsetArray[numOffset].count);
		int16 maskDstX = FROM_LE_16(maskOffsetArray[numOffset].dstX);
		int16 maskDstY = FROM_LE_16(maskOffsetArray[numOffset].dstY);

		for (int numMask=0; numMask<maskCount; numMask++) {
			rdt1_pri_square_t *squareMask = (rdt1_pri_square_t *) ((byte *) &_roomPtr[offset]);

			int srcX,srcY, width,height, dstX=maskDstX,dstY=maskDstY, depth;

			if (squareMask->size == 0) {
				/* Rect mask */
				rdt1_pri_rect_t *rectMask = (rdt1_pri_rect_t *) squareMask;

				srcX = rectMask->srcX;
				srcY = rectMask->srcY;
				dstX += rectMask->dstX;
				dstY += rectMask->dstY;
				width = FROM_LE_16(rectMask->width);
				height = FROM_LE_16(rectMask->height);
				depth = FROM_LE_16(rectMask->depth);

				offset += sizeof(rdt1_pri_rect_t);
			} else {
				/* Square mask */

				srcX = squareMask->srcX;
				srcY = squareMask->srcY;
				dstX += squareMask->dstX;
				dstY += squareMask->dstY;
				width = height = FROM_LE_16(squareMask->size);
				depth = FROM_LE_16(squareMask->depth);

				offset += sizeof(rdt1_pri_square_t);
			}

			g_driver->drawMaskedFrame(srcX,srcY, dstX,dstY, width,height, 16*depth);
		}
	}
}

Common::SeekableReadStream *RE1Room::getTimMask(int numCamera) {
	if (!_roomPtr)
		return nullptr;

	rdt1_rid_t *cameraPosArray = (rdt1_rid_t *) ((byte *) &_roomPtr[sizeof(rdt1_header_t)]);
	int32 offset = FROM_LE_32( cameraPosArray[numCamera].timOffset );
	if (offset == 0)
		return nullptr;

	return new Common::MemoryReadStream(&_roomPtr[offset], _roomSize);
}

void RE1Room::scenePrepareInit(void) {
	Room::scenePrepareInit();

	if (!_roomPtr)
		return;

	int32 offset = FROM_LE_32( ((rdt1_header_t *) _roomPtr)->offsets[RDT1_OFFSET_INIT_SCRIPT] );
	_scriptPtr = & (((byte *) _roomPtr)[offset]);
	if (!_scriptPtr)
		return;

	_scriptLen = FROM_LE_16( *((int16 *) _scriptPtr) );
	_scriptPC = 2;
	_scriptInst = &_scriptPtr[_scriptPC];

	debug(3, "re1: sceneInit at offset 0x%08x, length 0x%08x", offset, _scriptLen);
}

void RE1Room::scenePrepareRun(void) {
	Room::scenePrepareRun();

	if (!_roomPtr)
		return;

	int32 offset = FROM_LE_32( ((rdt1_header_t *) _roomPtr)->offsets[RDT1_OFFSET_ROOM_SCRIPT] );
	_scriptPtr = & (((byte *) _roomPtr)[offset]);
	if (!_scriptPtr)
		return;

	_scriptLen = FROM_LE_16( *((int16 *) _scriptPtr) );
	_scriptPC = 2;
	_scriptInst = &_scriptPtr[_scriptPC];

	debug(3, "re1: sceneRun at offset 0x%08x, length 0x%08x", offset, _scriptLen);
}

bool RE1Room::sceneExecInst(void) {
	script_inst_t *inst = (script_inst_t *) _scriptInst;

	if (!inst)
		return true;

	switch(inst->opcode) {
		case INST_DOOR_SET:
			{
				script_inst_door_set_t *doorSet = (script_inst_door_set_t *) inst;

				Door *new_door = new Door();
				new_door->_x = FROM_LE_16(doorSet->x);
				new_door->_y = FROM_LE_16(doorSet->y);
				new_door->_w = FROM_LE_16(doorSet->w);
				new_door->_h = FROM_LE_16(doorSet->h);

				new_door->_nextX = FROM_LE_16(doorSet->next_x);
				new_door->_nextY = FROM_LE_16(doorSet->next_y);
				new_door->_nextZ = FROM_LE_16(doorSet->next_z);
				new_door->_nextDir = FROM_LE_16(doorSet->next_dir);

				new_door->_nextStage = doorSet->next_stage_and_room>>5;
				switch(new_door->_nextStage) {
					case 0:
					default:
						new_door->_nextStage = _game->_stage;
						break;
					case 1:
						new_door->_nextStage = _game->_stage-1;
						break;
					case 2:
						new_door->_nextStage = _game->_stage+1;
						break;
				}
				new_door->_nextRoom = doorSet->next_stage_and_room & 31;
				new_door->_nextCamera = 0 /*doorSet->next_camera & 7*/;

				this->_doors.push_back(new_door);

				debug(3, "0x%04x: INST_DOOR_SET x=%d,y=%d,w=%d,h=%d", _scriptPC,
					new_door->_x,new_door->_y,new_door->_w,new_door->_h);
			}
			break;
		default:
			debug(3, "0x%04x: 0x%02x", _scriptPC, inst->opcode);
			break;
	}

	return false;
}

int RE1Room::sceneInstLen(void) {
	if (!_scriptInst)
		return 0;

	for (unsigned int i=0; i< sizeof(inst_length)/sizeof(script_inst_len_t); i++) {
		if (inst_length[i].opcode == _scriptInst[0]) {
			return inst_length[i].length;
		}
	}

	/* Variable length instructions */
	switch(_scriptInst[0]) {
		case INST_17:
			switch(_scriptInst[4]) {
				case 0:
					/* fields used */
					return 6+4;
				case 1:
				case 2:
				case 3:
					/* fields not used */
					return 6+4;
				default:
					return 6;
			}
			break;
		case INST_28:
			switch(_scriptInst[2]) {
				case 0:
				case 2:
				case 3:
				case 5:
				case 9:
				case 10:
					return 6;
				case 1:
					return 8;
				case 6:
				case 8:
					return 4;
				case 4:
				default:
					break;
			}
			break;
		case INST_33:
			switch(_scriptInst[1]) {
				case 0:
				case 4:
				case 6:
				case 7:
					return 2;
				case 1:
				case 3:
				case 5:
				case 8:
				case 9:
				case 10:
					return 4;
				case 2:
				default:
					break;
			}
			break;
		default:
			break;
	}

	return 0;
}

} // End of namespace Reevengi
