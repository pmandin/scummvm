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

#ifndef REEVENGI_RE3_ENGINE_H
#define REEVENGI_RE3_ENGINE_H

#include "engines/advancedDetector.h"

#include "engines/reevengi/reevengi.h"

namespace Reevengi {

class RE3Engine : public ReevengiEngine {
public:
	RE3Engine(OSystem *syst, const ReevengiGameDescription *desc);
	virtual ~RE3Engine();

protected:
	void initPreRun(void) override;
	void loadBgImage(void) override;
	void loadBgMaskImage(void) override;
	void loadRoom(void) override;
	void loadMovie(unsigned int numMovie) override;
	Entity *loadEntity(int numEntity, int isPlayer) override;

private:
	char _country;

	void loadBgImagePc(void);
	void loadBgImagePsx(void);

	void loadBgMaskImagePc(void);
	void loadBgMaskImagePsx(void);

	void loadRoomPc(void);
	void loadRoomPsx(void);

	Entity *loadEntityPc(int numEntity, int isPlayer);
	Entity *loadEntityPsx(int numEntity, int isPlayer);
};

} // end of namespace Reevengi

#endif
