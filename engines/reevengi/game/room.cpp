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

#include "engines/reevengi/game/room.h"

namespace Reevengi {

Room::Room(Common::SeekableReadStream *stream) {
	stream->seek(0);
	_roomSize = stream->size();

	_roomPtr = new byte[_roomSize];
	stream->read(_roomPtr, _roomSize);
/*
	Common::DumpFile adf;
	adf.open("room.rdt");
	adf.write(_roomPtr, _roomSize);
	adf.close();
*/
	postLoad();
}

Room::~Room() {
	delete _roomPtr;
	_roomPtr = nullptr;
	_roomSize = 0;
}

int Room::getNumCameras(void) {
	return 0;
}

void Room::getCameraPos(int numCamera, RdtCameraPos_t *cameraPos) {
	//
}

int Room::checkCamSwitch(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos) {
	return -1;
}

void Room::drawCamSwitch(int curCam) {
	//
}

bool Room::checkCamBoundary(int curCam, Math::Vector2d fromPos, Math::Vector2d toPos) {
	return false;
}

void Room::drawCamBoundary(int curCam) {
	//
}

void Room::drawMasks(int numCamera) {
	//
}

void Room::postLoad(void) {
	//
}

bool Room::isInside(Math::Vector2d pos, Math::Vector2d quad[4]) {
	float dx1,dy1,dx2,dy2;

	for (int i=0; i<4; i++) {
		dx1 = quad[(i+1) & 3].getX() - quad[i].getX();
		dy1 = quad[(i+1) & 3].getY() - quad[i].getY();

		dx2 = pos.getX() - quad[i].getX();
		dy2 = pos.getY() - quad[i].getY();

		if (dx1*dy2-dy1*dx2 >= 0) {
			return false;
		}
	}

	return true;
}

} // End of namespace Reevengi
