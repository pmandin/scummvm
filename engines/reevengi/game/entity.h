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

#ifndef REEVENGI_ENTITY_H
#define REEVENGI_ENTITY_H

#include "common/stream.h"
#include "math/vector2d.h"

#include "engines/reevengi/formats/tim.h"

namespace Reevengi {

class Entity {
public:
	Entity(Common::SeekableReadStream *stream);
	virtual ~Entity();

	TimDecoder *timTexture;

	void draw(int x, int y, int z, int a);

protected:
	// raw data file for entity
	byte *_emdPtr;
	int32 _emdSize;

	int _numAnim;	/* Animation in entity */
	int _numFrame;	/* Frame in animation */

	virtual int getNumAnims(void);
	virtual int getNumChildren(int numMesh);
	virtual int getChild(int numMesh, int numChild);

	virtual void drawMesh(int numMesh);

	virtual void drawNode(int numMesh);

private:
	uint32 _texId;

	void setTexture(void);
};

} // End of namespace Reevengi

#endif
