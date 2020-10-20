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

#include "engines/advancedDetector.h"

#include "engines/reevengi/reevengi.h"
#include "engines/reevengi/re1/re1.h"
#include "engines/reevengi/re2/re2.h"
#include "engines/reevengi/re3/re3.h"

namespace Reevengi {

struct ReevengiGameDescription {
	ADGameDescription desc;
	ReevengiGameType gameType;
};

class ReevengiMetaEngine : public AdvancedMetaEngine {
public:
	const char *getName() const override {
		return "Reevengi";
	}

	const char *getEngineId() const override {
		return "reevengi";
	}

	const char *getOriginalCopyright() const /*override*/ {
		return "(C) Capcom";
	}

	bool createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const /*override*/ {
		const ReevengiGameDescription *gd = (const ReevengiGameDescription *)desc;

		switch(gd->gameType) {
			case RType_None:
				break;
			case RType_RE1:
				*engine = new RE1Engine(syst, gd->gameType, &(gd->desc));
				break;
			case RType_RE2_LEON:
			case RType_RE2_CLAIRE:
				*engine = new RE2Engine(syst, gd->gameType, &(gd->desc));
				break;
			case RType_RE3:
				*engine = new RE3Engine(syst, gd->gameType, &(gd->desc));
				break;
		}

		return engine != nullptr;
	}
};

} // End of namespace Reevengi

#if PLUGIN_ENABLED_DYNAMIC(REEVENGI)
	REGISTER_PLUGIN_DYNAMIC(REEVENGI, PLUGIN_TYPE_ENGINE, Reevengi::ReevengiMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(REEVENGI, PLUGIN_TYPE_ENGINE, Reevengi::ReevengiMetaEngine);
#endif
