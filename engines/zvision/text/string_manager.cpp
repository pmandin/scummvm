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

#include "common/debug.h"
#include "common/file.h"
#include "common/scummsys.h"
#include "common/tokenizer.h"
#include "graphics/fontman.h"
#include "zvision/zvision.h"
#include "zvision/text/string_manager.h"
#include "zvision/text/text.h"

namespace ZVision {

StringManager::StringManager(ZVision *engine)
	: _engine(engine) {
}

StringManager::~StringManager() {

}

void StringManager::initialize(ZVisionGameId gameId) {
	if (gameId == GID_NEMESIS)
		loadStrFile("nemesis.str");
	else if (gameId == GID_GRANDINQUISITOR)
		loadStrFile("inquis.str");
}

void StringManager::loadStrFile(const Common::Path &fileName) {
	Common::File file;
	if (!file.open(fileName))
		error("%s does not exist. String parsing failed", fileName.toString().c_str());

	uint lineNumber = 0;
	while (!file.eos()) {
		_lines[lineNumber] = readWideLine(file).encode();

		lineNumber++;
		assert(lineNumber <= NUM_TEXT_LINES);
	}
}

const Common::String StringManager::getTextLine(uint stringNumber) {
	return _lines[stringNumber];
}

} // End of namespace ZVision
