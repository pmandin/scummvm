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

#ifndef REEVENGI_DOOR_H
#define REEVENGI_DOOR_H

#include "common/stream.h"
#include "math/vector2d.h"

namespace Reevengi {

class Door {
public:
	int16 _x,_y,_w,_h;

	int16 _nextX, _nextY, _nextZ, _nextDir;
	uint8 _nextStage, _nextRoom, _nextCamera;

	Door(void);
	virtual ~Door();

	bool isInside(Math::Vector2d pos);
};

} // End of namespace Reevengi

#endif
