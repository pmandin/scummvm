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

#ifndef REEVENGI_RE3_ENGINE_H
#define REEVENGI_RE3_ENGINE_H

#include "engines/advancedDetector.h"

#include "engines/reevengi/reevengi.h"

namespace Reevengi {

class RE3Engine : public ReevengiEngine {
public:
	RE3Engine(OSystem *syst, ReevengiGameType gameType, const ADGameDescription *desc);
	virtual ~RE3Engine();

protected:
	void initPreRun(void);
	void loadBgImage(void);
	void loadBgMaskImage(void);
	void loadRoom(void);

private:
	char _country;

	void loadBgImagePc(void);
	void loadBgImagePsx(void);

	void loadBgMaskImagePc(void);
	void loadBgMaskImagePsx(void);

	void loadRoomPc(void);
	void loadRoomPsx(void);
};

} // end of namespace Reevengi

#endif
