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
