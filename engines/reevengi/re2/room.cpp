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
#include "math/vector2d.h"

#include "engines/reevengi/gfx/gfx_base.h"
#include "engines/reevengi/re2/room.h"

namespace Reevengi {

/*--- Defines ---*/

#define RDT2_OFFSET_COLLISION 6
#define RDT2_OFFSET_CAMERAS	7
#define RDT2_OFFSET_CAM_SWITCHES	8
#define RDT2_OFFSET_CAM_LIGHTS	9
#define RDT2_OFFSET_TEXT_LANG1	13
#define RDT2_OFFSET_TEXT_LANG2	14
#define RDT2_OFFSET_INIT_SCRIPT	16
#define RDT2_OFFSET_ROOM_SCRIPT	17
#define RDT2_OFFSET_ANIMS	18

#include "rdt_scd_defs.gen.h"

/*--- Types ---*/

typedef struct {
	uint8	unknown0;
	uint8	numCameras;
	uint8	unknown1[6];
	uint32	offsets[21];
} rdt2_header_t;

/* Collisions, offset 6 */

typedef struct {
	int16 cx, cz;
	uint32 count;
	int32 ceiling;
	uint32 dummy;	/* constant, 0xc5c5c5c5 */
} rdt2_sca_header_t;

typedef struct {
	int16 x,z;
	uint16 w,h;
	int16 type, floor;
	uint32 flags;
} rdt2_sca_element_t;

/* Cameras and masks, offset 7 */

typedef struct {
	uint16 unk0;
	uint16 const0; /* 0x683c, or 0x73b7 */
	/* const0>>7 used for engine */
	int32 fromX, fromY, fromZ;
	int32 toX, toY, toZ;
	uint32 priOffset;
} rdt2_rid_t;

typedef struct {
	uint16 numOffset;
	uint16 numMasks;
} rdt2_pri_header_t;

typedef struct {
	uint16 count;
	uint16 unknown;
	uint16 dstX, dstY;
} rdt2_pri_offset_t;

typedef struct {
	uint8 srcX, srcY;
	uint8 dstX, dstY;
	uint16 depth, size;
} rdt2_pri_square_t;

typedef struct {
	uint8 srcX, srcY;
	uint8 dstX, dstY;
	uint16 depth, zero;
	uint16 width, height;
} rdt2_pri_rect_t;

/* Cameras switches, offset 8 */

typedef struct {
	uint16 const0; /* 0xff01 */
	uint8 fromCam,toCam;
	int16 x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	int16 x2,y2;
	int16 x3,y3;
	int16 x4,y4;
} rdt2_rvd_t;

/* Lights, offset 9 */

typedef struct {
	uint8 r,g,b;
} rdt2_lit_col_t;

typedef struct {
	int16 x,y,z;
} rdt2_lit_pos_t;

typedef struct {
	uint16 type[2];
	rdt2_lit_col_t col[3];
	rdt2_lit_col_t ambient;
	rdt2_lit_pos_t pos[3];
	uint16 brightness[3];
} rdt2_lit_t;

typedef struct {
	uint8 opcode;
	uint8 length;
} script_inst_len_t;

#include "rdt_scd_types.gen.h"

/*--- Variables ---*/

#include "rdt_scd_lengths.gen.c"

/*--- Class --- */

RE2Room::RE2Room(Common::SeekableReadStream *stream): Room(stream) {
	//
}

void *RE2Room::getRdtSection(int numSection) {
	if (!_roomPtr)
		return nullptr;
	if (numSection<0)
		return nullptr;

	uint32 offset = FROM_LE_32( ((rdt2_header_t *) _roomPtr)->offsets[numSection] );

	return (void *)
		(&((char *) (_roomPtr))[offset]);
}

int RE2Room::getNumCameras(void) {
	if (!_roomPtr)
		return 0;

	return ((rdt2_header_t *) _roomPtr)->numCameras;
}

void RE2Room::getCameraPos(int numCamera, RdtCameraPos_t *cameraPos) {
	if (!_roomPtr)
		return;

	rdt2_rid_t *cameraPosArray = (rdt2_rid_t *) getRdtSection(RDT2_OFFSET_CAMERAS);

	cameraPos->fromX = FROM_LE_32( cameraPosArray[numCamera].fromX );
	cameraPos->fromY = FROM_LE_32( cameraPosArray[numCamera].fromY );
	cameraPos->fromZ = FROM_LE_32( cameraPosArray[numCamera].fromZ );

	cameraPos->toX = FROM_LE_32( cameraPosArray[numCamera].toX );
	cameraPos->toY = FROM_LE_32( cameraPosArray[numCamera].toY );
	cameraPos->toZ = FROM_LE_32( cameraPosArray[numCamera].toZ );
}

int RE2Room::checkCamSwitch(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos) {
	if (!_roomPtr)
		return -1;

	rdt2_rvd_t *camSwitchArray = (rdt2_rvd_t *) getRdtSection(RDT2_OFFSET_CAM_SWITCHES);
	int prevFrom = -1;

	while (FROM_LE_16(camSwitchArray->const0) != 0xffff) {
		bool boundary = false;

		if (prevFrom != camSwitchArray->fromCam) {
			prevFrom = camSwitchArray->fromCam;
			boundary = true;
		}

		if (boundary && (camSwitchArray->toCam==0)) {
			/* boundary, not a switch */
		} else {
			/* Check objet triggered camera switch */
			Math::Vector2d quad[4];
			quad[0] = Math::Vector2d( (int16) FROM_LE_16(camSwitchArray->x1), (int16) FROM_LE_16(camSwitchArray->y1));
			quad[1] = Math::Vector2d( (int16) FROM_LE_16(camSwitchArray->x2), (int16) FROM_LE_16(camSwitchArray->y2));
			quad[2] = Math::Vector2d( (int16) FROM_LE_16(camSwitchArray->x3), (int16) FROM_LE_16(camSwitchArray->y3));
			quad[3] = Math::Vector2d( (int16) FROM_LE_16(camSwitchArray->x4), (int16) FROM_LE_16(camSwitchArray->y4));

			if ((curCam==camSwitchArray->fromCam) && !isInside(fromPos, quad) && isInside(toPos, quad)) {
				return camSwitchArray->toCam;
			}
		}

		++camSwitchArray;
	}

	return -1;
}

void RE2Room::drawCamSwitch(int curCam) {
	if (!_roomPtr)
		return;

	g_driver->setColor(1.0, 0.75, 0.25);

	rdt2_rvd_t *camSwitchArray = (rdt2_rvd_t *) getRdtSection(RDT2_OFFSET_CAM_SWITCHES);
	int prevFrom = -1;

	while (FROM_LE_16(camSwitchArray->const0) != 0xffff) {
		bool boundary = false;

		if (prevFrom != camSwitchArray->fromCam) {
			prevFrom = camSwitchArray->fromCam;
			boundary = true;
		}

		if (boundary && (camSwitchArray->toCam==0)) {
			/* boundary, not a switch */
		} else {
			Math::Vector3d quad[4];
			quad[0] = Math::Vector3d( (int16) FROM_LE_16(camSwitchArray->x1), 0, (int16) FROM_LE_16(camSwitchArray->y1));
			quad[1] = Math::Vector3d( (int16) FROM_LE_16(camSwitchArray->x2), 0, (int16) FROM_LE_16(camSwitchArray->y2));
			quad[2] = Math::Vector3d( (int16) FROM_LE_16(camSwitchArray->x3), 0, (int16) FROM_LE_16(camSwitchArray->y3));
			quad[3] = Math::Vector3d( (int16) FROM_LE_16(camSwitchArray->x4), 0, (int16) FROM_LE_16(camSwitchArray->y4));

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

bool RE2Room::checkCamBoundary(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos) {
	if (!_roomPtr)
		return false;

	rdt2_rvd_t *camBoundaryArray = (rdt2_rvd_t *) getRdtSection(RDT2_OFFSET_CAM_SWITCHES);
	int prevFrom = -1;

	while (FROM_LE_16(camBoundaryArray->const0) != 0xffff) {
		bool boundary = false;

		if (prevFrom != camBoundaryArray->fromCam) {
			prevFrom = camBoundaryArray->fromCam;
			boundary = true;
		}

		if (boundary && (camBoundaryArray->toCam==0)) {
			/* Check objet got outside boundary */
			Math::Vector2d quad[4];
			quad[0] = Math::Vector2d( (int16) FROM_LE_16(camBoundaryArray->x1), (int16) FROM_LE_16(camBoundaryArray->y1));
			quad[1] = Math::Vector2d( (int16) FROM_LE_16(camBoundaryArray->x2), (int16) FROM_LE_16(camBoundaryArray->y2));
			quad[2] = Math::Vector2d( (int16) FROM_LE_16(camBoundaryArray->x3), (int16) FROM_LE_16(camBoundaryArray->y3));
			quad[3] = Math::Vector2d( (int16) FROM_LE_16(camBoundaryArray->x4), (int16) FROM_LE_16(camBoundaryArray->y4));

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

void RE2Room::drawCamBoundary(int curCam) {
	if (!_roomPtr)
		return;

	g_driver->setColor(1.0, 0.0, 0.0);

	rdt2_rvd_t *camBoundaryArray = (rdt2_rvd_t *) getRdtSection(RDT2_OFFSET_CAM_SWITCHES);
	int prevFrom = -1;

	while (FROM_LE_16(camBoundaryArray->const0) != 0xffff) {
		bool boundary = false;

		if (prevFrom != camBoundaryArray->fromCam) {
			prevFrom = camBoundaryArray->fromCam;
			boundary = true;
		}

		if (boundary && (camBoundaryArray->toCam==0)) {
			Math::Vector3d quad[4];
			quad[0] = Math::Vector3d( (int16) FROM_LE_16(camBoundaryArray->x1), 0, (int16) FROM_LE_16(camBoundaryArray->y1));
			quad[1] = Math::Vector3d( (int16) FROM_LE_16(camBoundaryArray->x2), 0, (int16) FROM_LE_16(camBoundaryArray->y2));
			quad[2] = Math::Vector3d( (int16) FROM_LE_16(camBoundaryArray->x3), 0, (int16) FROM_LE_16(camBoundaryArray->y3));
			quad[3] = Math::Vector3d( (int16) FROM_LE_16(camBoundaryArray->x4), 0, (int16) FROM_LE_16(camBoundaryArray->y4));

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

void RE2Room::drawMasks(int numCamera) {
	if (!_roomPtr)
		return;

	rdt2_rid_t *cameraPosArray = (rdt2_rid_t *) getRdtSection(RDT2_OFFSET_CAMERAS);

	int32 offset = FROM_LE_32( cameraPosArray[numCamera].priOffset );
	if (offset == -1)
		return;

	rdt2_pri_header_t *maskHeaderArray = (rdt2_pri_header_t *) ((byte *) &_roomPtr[offset]);
	int16 countOffsets = FROM_LE_16(maskHeaderArray->numOffset);
	if (countOffsets == -1)
		return;
	offset += sizeof(rdt2_pri_header_t);

	rdt2_pri_offset_t *maskOffsetArray = (rdt2_pri_offset_t *) ((byte *) &_roomPtr[offset]);
	offset += sizeof(rdt2_pri_offset_t) * countOffsets;

	for (int numOffset=0; numOffset<countOffsets; numOffset++) {
		int16 maskCount = FROM_LE_16(maskOffsetArray[numOffset].count);
		int16 maskDstX = FROM_LE_16(maskOffsetArray[numOffset].dstX);
		int16 maskDstY = FROM_LE_16(maskOffsetArray[numOffset].dstY);

		for (int numMask=0; numMask<maskCount; numMask++) {
			rdt2_pri_square_t *squareMask = (rdt2_pri_square_t *) ((byte *) &_roomPtr[offset]);

			int srcX,srcY, width,height, dstX=maskDstX,dstY=maskDstY, depth;

			if (squareMask->size == 0) {
				/* Rect mask */
				rdt2_pri_rect_t *rectMask = (rdt2_pri_rect_t *) squareMask;

				srcX = rectMask->srcX;
				srcY = rectMask->srcY;
				dstX += rectMask->dstX;
				dstY += rectMask->dstY;
				width = FROM_LE_16(rectMask->width);
				height = FROM_LE_16(rectMask->height);
				depth = FROM_LE_16(rectMask->depth);

				offset += sizeof(rdt2_pri_rect_t);
			} else {
				/* Square mask */

				srcX = squareMask->srcX;
				srcY = squareMask->srcY;
				dstX += squareMask->dstX;
				dstY += squareMask->dstY;
				width = height = FROM_LE_16(squareMask->size);
				depth = FROM_LE_16(squareMask->depth);

				offset += sizeof(rdt2_pri_square_t);
			}

			g_driver->drawMaskedFrame(srcX,srcY, dstX,dstY, width,height, 32*depth);
		}
	}
}

void RE2Room::scenePrepareInit(void) {
	Room::scenePrepareInit();

	_scriptPtr = (byte *) getRdtSection(RDT2_OFFSET_INIT_SCRIPT);
	_scriptLen = getSceneScriptLength(RDT2_OFFSET_INIT_SCRIPT);
	_scriptInst = getSceneScriptStart();

	debug(3, "re2: sceneInit at offset 0x%08x, length 0x%08x", FROM_LE_32( ((rdt2_header_t *) _roomPtr)->offsets[RDT2_OFFSET_INIT_SCRIPT] ), _scriptLen);
}

void RE2Room::scenePrepareRun(void) {
	Room::scenePrepareRun();

	_scriptPtr = (byte *) getRdtSection(RDT2_OFFSET_ROOM_SCRIPT);
	_scriptLen = getSceneScriptLength(RDT2_OFFSET_ROOM_SCRIPT);
	_scriptInst = getSceneScriptStart();

	debug(3, "re2: sceneRun at offset 0x%08x, length 0x%08x", FROM_LE_32( ((rdt2_header_t *) _roomPtr)->offsets[RDT2_OFFSET_ROOM_SCRIPT] ), _scriptLen);
}

int RE2Room::getSceneScriptLength(int numScript) {
	uint32 smaller_offset, offset;

	if (!_scriptPtr)
		return 0;

	/* Search smaller offset after script to calc length */
	offset = FROM_LE_32( ((rdt2_header_t *) _roomPtr)->offsets[numScript] );
	smaller_offset = _roomSize;
	for (int i=0; i<21; i++) {
		uint32 next_offset = FROM_LE_32( ((rdt2_header_t *) _roomPtr)->offsets[i] );
		if ((next_offset>0) && (next_offset<smaller_offset) && (next_offset>offset)) {
			smaller_offset = next_offset;
		}
	}

	return smaller_offset - offset;
}

byte *RE2Room::getSceneScriptStart(void) {
	if (!_scriptPtr)
		return nullptr;

	/* Start of script is an array of offsets to the various script functions
	 * The first offset also gives the first instruction to execute
	 */
	uint16 *funcArrayPtr = (uint16 *) _scriptPtr;

	_scriptPC = FROM_LE_16( funcArrayPtr[0] );
	return &_scriptPtr[_scriptPC];
}

bool RE2Room::sceneExecInst(void) {
	script_inst_t *inst = (script_inst_t *) _scriptInst;

	if (!inst)
		return true;

	switch(inst->opcode) {
		case INST_DOOR_AOT_SET:
			{
				int16 x,y,w,h, next_x,next_y,next_z, next_dir;
				int next_stage, next_room, next_camera;

				script_inst_door_aot_set_t *doorSet = (script_inst_door_aot_set_t *) inst;

				x = FROM_LE_16(doorSet->x);
				y = FROM_LE_16(doorSet->z);
				w = FROM_LE_16(doorSet->w);
				h = FROM_LE_16(doorSet->h);

				next_x = FROM_LE_16(doorSet->next_x);
				next_y = FROM_LE_16(doorSet->next_y);
				next_z = FROM_LE_16(doorSet->next_z);
				next_dir = FROM_LE_16(doorSet->next_dir);

				next_stage = doorSet->next_stage+1;
				next_room = doorSet->next_room;
				next_camera = doorSet->next_camera;

				debug(3, "0x%04x: INST_DOOR_AOT_SET x=%d,y=%d,w=%d,h=%d", _scriptPC, x,y,w,h);
			}
			break;
	}

	return false;
}

int RE2Room::sceneInstLen(void) {
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
