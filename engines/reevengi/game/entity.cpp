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

#include "engines/reevengi/game/entity.h"
#include "engines/reevengi/gfx/gfx_base.h"

namespace Reevengi {

Entity::Entity(Common::SeekableReadStream *stream): _numAnim(0), _numFrame(0),
	timTexture(nullptr), _texId(0xffffffff) {
	stream->seek(0);
	_emdSize = stream->size();

	_emdPtr = new byte[_emdSize];
	stream->read(_emdPtr, _emdSize);
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
	setTexture();
	drawNode(0);
	g_driver->setTexture2d(false);
}

int Entity::getNumAnims(void) {
	return 0;
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
		drawNode(getChild(numMesh, i));
	}

	g_driver->popMatrix();
}

void Entity::setTexture(void) {
	if (!timTexture)
		return;

	g_driver->MatrixModeTexture();
	g_driver->loadIdentity();

	if (_texId==0xffffffff) {
		// Initialize texture
		_texId = g_driver->genTexture();
		g_driver->bindTexture(_texId);

		g_driver->createTexture(timTexture->getSurface(), timTexture->getTimPalette());
	} else {
		g_driver->bindTexture(_texId);
	}

	g_driver->MatrixModeModelview();
}

} // End of namespace Reevengi
