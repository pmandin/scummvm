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

#include "engines/reevengi/game/entity.h"
#include "engines/reevengi/gfx/gfx_base.h"

namespace Reevengi {

Entity::Entity(Common::SeekableReadStream *stream): _numAnim(0), _numFrame(0),
	timTexture(nullptr) {
	stream->seek(0);
	_emdSize = stream->size();

	_emdPtr = new byte[_emdSize];
	stream->read(_emdPtr, _emdSize);

	for (int i=0; i<4; i++) {
		_texId[i] = 0xffffffff;
	}

/*
	Common::DumpFile adf;
	adf.open("room.rdt");
	adf.write(_roomPtr, _roomSize);
	adf.close();
*/
}

Entity::~Entity() {
	delete _emdPtr;
	_emdPtr = nullptr;
	_emdSize = 0;
}

void Entity::draw(int x, int y, int z, int a) {
	if (!_emdPtr)
		return;

	g_driver->translate(x,y,z);
	g_driver->rotate((a * 360.0f) / 4096.0f, 0.0f, 1.0f, 0.0f);

	g_driver->setTexture2d(true);
	g_driver->MatrixModeTexture();
	g_driver->loadIdentity();
	g_driver->MatrixModeModelview();

	drawNode(0);
	g_driver->setTexture2d(false);
}

void *Entity::getEmdSection(int numSection) {
	return nullptr;
}

int Entity::getNumAnims(void) {
	return 0;
}

int Entity::getNumAnimFrames(void) {
	return 1;
}

int Entity::getNumChildren(int numMesh) {
	return 0;
}

int Entity::getChild(int numMesh, int numChild) {
	return 0;
}

void Entity::drawMesh(int numMesh) {
	//
}

void Entity::drawNode(int numMesh) {
	int i, numChildren;

	g_driver->pushMatrix();

	// render mesh
	drawMesh(numMesh);

	// render children
	numChildren = getNumChildren(numMesh);
	for (i=0; i<numChildren; i++) {
		//debug(3, "mesh %d: %d", numMesh, getChild(numMesh, i));
		drawNode(getChild(numMesh, i));
	}

	g_driver->popMatrix();
}

void Entity::setTexture(int numTexId) {
	if (!timTexture)
		return;

	if (_texId[numTexId]==0xffffffff) {
		// Initialize texture
		_texId[numTexId] = g_driver->genTexture();
		g_driver->bindTexture(_texId[numTexId]);

		uint16 *timPalette = timTexture->getTimPalette();
		g_driver->createTexture(timTexture->getSurface(), &timPalette[256*numTexId]);
	} else {
		g_driver->bindTexture(_texId[numTexId]);
	}
}

} // End of namespace Reevengi
