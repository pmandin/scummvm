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

#ifndef REEVENGI_ROOM_H
#define REEVENGI_ROOM_H

#include "common/stream.h"
#include "math/vector2d.h"

#include "engines/reevengi/game/door.h"

namespace Reevengi {

typedef struct {
	int32 fromX, fromY, fromZ;
	int32 toX, toY, toZ;
} RdtCameraPos_t;

class ReevengiEngine;

class Room {
public:
	Room(ReevengiEngine *game, Common::SeekableReadStream *stream);
	virtual ~Room();

	virtual int getNumCameras(void);
	virtual void getCameraPos(int numCamera, RdtCameraPos_t *cameraPos);

	virtual int checkCamSwitch(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos);
	virtual void drawCamSwitch(int curCam);

	virtual bool checkCamBoundary(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos);
	virtual void drawCamBoundary(int curCam);

	virtual void drawMasks(int numCamera);

	// Checks if pos in door zone
	//	Returns matched door
	Door *checkDoors(Math::Vector2d pos);

	// Prepare scene initialization script
	virtual void scenePrepareInit(void);
	// Prepare scene run script
	virtual void scenePrepareRun(void);
	// Execute script
	void sceneRunScript(void);
	// Execute instruction for current scene script
	//	Returns true if can continue executing with next instruction
	//	Returns false if any pause in script
	virtual bool sceneExecInst(void);
	// Returns instruction length
	virtual int sceneInstLen(void);
	// Update program counter for next instruction
	void sceneUpdatePC(void);

protected:
	ReevengiEngine *_game;

	// raw data file for room
	byte *_roomPtr;
	int32 _roomSize;

	byte *_scriptPtr;
	int _scriptLen;		// Script length
	byte *_scriptInst;	// Current instruction = &_scriptPtr[_scriptOffset]
	int _scriptPC;		// Program counter in script
	bool _scriptInit;	// Run init script (true), room script (false)

	Common::List<Door *> _doors;	// Doors list;

	virtual void*getRdtSection(int numSection);

	bool isInside(Math::Vector2d pos, Math::Vector2d quad[4]);
};

} // End of namespace Reevengi

#endif
