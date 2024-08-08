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

#include "engines/advancedDetector.h"

#include "engines/reevengi/reevengi.h"
#include "engines/reevengi/detection.h"
#include "engines/reevengi/re1/re1.h"
#include "engines/reevengi/re2/re2.h"
#include "engines/reevengi/re3/re3.h"

namespace Reevengi {

class ReevengiMetaEngine : public AdvancedMetaEngine<ReevengiGameDescription> {
public:
	const char *getName() const override {
		return "reevengi";
	}

	Common::Error createInstance(OSystem *syst, Engine **engine, const ReevengiGameDescription *desc) const override;
};

Common::Error ReevengiMetaEngine::createInstance(OSystem *syst, Engine **engine, const ReevengiGameDescription *desc) const {
	switch(desc->gameType) {
		case RType_None:
			break;
		case RType_RE1:
			*engine = new RE1Engine(syst, desc);
			break;
		case RType_RE2_LEON:
		case RType_RE2_CLAIRE:
			*engine = new RE2Engine(syst, desc);
			break;
		case RType_RE3:
			*engine = new RE3Engine(syst, desc);
			break;
	}

	return (engine != nullptr
		? Common::kNoError
		: Common::Error(Common::kUnsupportedGameidError, "REEVENGI: Unknown game data to initialize engine")
	);
}

} // End of namespace Reevengi

#if PLUGIN_ENABLED_DYNAMIC(REEVENGI)
	REGISTER_PLUGIN_DYNAMIC(REEVENGI, PLUGIN_TYPE_ENGINE, Reevengi::ReevengiMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(REEVENGI, PLUGIN_TYPE_ENGINE, Reevengi::ReevengiMetaEngine);
#endif
