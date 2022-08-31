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

#ifndef REEVENGI_RE2ENTITY_PLD_H
#define REEVENGI_RE2ENTITY_PLD_H

#include "common/stream.h"

#include "engines/reevengi/re2/entity.h"

namespace Reevengi {

class RE2EntityPld : public RE2Entity {
public:
	RE2EntityPld(Common::SeekableReadStream *stream);

protected:
	int getNumSection(int sectionType) override;
};

} // End of namespace Reevengi

#endif
