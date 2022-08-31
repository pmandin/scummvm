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

#ifndef REEVENGI_RE1ROOM_H
#define REEVENGI_RE1ROOM_H

#include "engines/reevengi/game/room.h"

namespace Reevengi {

class Room;

class RE1Room: public Room {
public:
	RE1Room(Common::SeekableReadStream *stream);

	int getNumCameras(void);
	void getCameraPos(int numCamera, RdtCameraPos_t *cameraPos);

	int checkCamSwitch(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos);
	void drawCamSwitch(int curCam);

	bool checkCamBoundary(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos);
	void drawCamBoundary(int curCam);

	void drawMasks(int numCamera);
	Common::SeekableReadStream *getTimMask(int numCamera);
};

} // End of namespace Reevengi

#endif
