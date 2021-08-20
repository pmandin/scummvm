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

#ifndef REEVENGI_RE2ENTITY_H
#define REEVENGI_RE2ENTITY_H

#include "common/stream.h"
#include "math/vector2d.h"

#include "engines/reevengi/game/entity.h"

namespace Reevengi {

class RE2Entity : public Entity {
public:
	RE2Entity(Common::SeekableReadStream *stream);

protected:
	void *getEmdSection(int numSection) override;

	int getNumAnims(void) override;
	int getNumChildren(int numMesh) override;
	int getChild(int numMesh, int numChild) override;

	void drawMesh(int numMesh) override;
};

} // End of namespace Reevengi

#endif
