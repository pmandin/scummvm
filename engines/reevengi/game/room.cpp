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

#include "engines/reevengi/game/room.h"

namespace Reevengi {

Room::Room(Common::SeekableReadStream *stream): _scriptPtr(nullptr), _scriptLen(0),
	_scriptInst(nullptr), _scriptPC(0), _scriptInit(true) {
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
}

Room::~Room() {
	delete _roomPtr;
	_roomPtr = nullptr;
	_roomSize = 0;
}

void *Room::getRdtSection(int numSection) {
	return nullptr;
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

void Room::scenePrepareInit(void) {
	_scriptPtr = _scriptInst = nullptr;
	_scriptLen = _scriptPC = 0;
}

void Room::scenePrepareRun(void) {
	_scriptPtr = _scriptInst = nullptr;
	_scriptLen = _scriptPC = 0;
}

void Room::sceneRunScript(void) {
	/* Run init script once */
	if (_scriptInit) {
		scenePrepareInit();
		while (_scriptInst) {
			sceneExecInst();
		}
		_scriptInit = false;

		scenePrepareRun();
	}

	/* Room script */
	bool paused = false;
	while (_scriptInst && !paused) {
		paused = sceneExecInst();
	}
}

bool Room::sceneExecInst(void) {
	// FIXME
	_scriptInst = nullptr;
	return true;
}

int Room::sceneInstLen(void) {
	return 0;
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
