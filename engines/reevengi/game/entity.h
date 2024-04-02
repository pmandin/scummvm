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

#ifndef REEVENGI_ENTITY_H
#define REEVENGI_ENTITY_H

#include "common/stream.h"
#include "math/vector2d.h"

#include "engines/reevengi/formats/tim.h"

namespace Reevengi {

class Entity {
public:
	int _numAnim;	/* Animation in entity */
	int _numFrame;	/* Frame in animation */

	Entity(Common::SeekableReadStream *stream);
	virtual ~Entity();

	TimDecoder *timTexture;

	void draw(int x, int y, int z, int a);

	virtual int getNumAnims(void);	/* Number of animations for this entity */
	virtual int getNumAnimFrames(void);	/* Number of frames for a given animation */

protected:
	// raw data file for entity
	byte *_emdPtr;
	int32 _emdSize;

	void setTexture(int numTexId);

	virtual void *getEmdSection(int numSection);

	virtual int getNumChildren(int numMesh);
	virtual int getChild(int numMesh, int numChild);

	virtual void drawMesh(int numMesh);

	virtual void drawNode(int numMesh);

private:
	uint32 _texId[4];
};

} // End of namespace Reevengi

#endif
