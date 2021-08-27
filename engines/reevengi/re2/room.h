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

#ifndef REEVENGI_RE2ROOM_H
#define REEVENGI_RE2ROOM_H

#include "engines/reevengi/game/room.h"

namespace Reevengi {

class Room;

class RE2Room: public Room {
public:
	RE2Room(Common::SeekableReadStream *stream);

	int getNumCameras(void);
	void getCameraPos(int numCamera, RdtCameraPos_t *cameraPos);

	int checkCamSwitch(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos);
	void drawCamSwitch(int curCam);

	bool checkCamBoundary(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos);
	void drawCamBoundary(int curCam);

	void drawMasks(int numCamera);

protected:
	void *getRdtSection(int numSection) override;
};

} // End of namespace Reevengi

#endif
