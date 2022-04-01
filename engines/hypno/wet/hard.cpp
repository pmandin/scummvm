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

#include "common/bitarray.h"
#include "common/events.h"
#include "common/config-manager.h"
#include "common/savefile.h"

#include "hypno/hypno.h"

namespace Hypno {

void WetEngine::endCredits(Code *code) {
	showCredits();
	_nextLevel = "<main_menu>";
}

void WetEngine::runCode(Code *code) {
	changeScreenMode("320x200");
	if (code->name == "<main_menu>")
		runMainMenu(code);
	else if (code->name == "<level_menu>")
		runLevelMenu(code);
	else if (code->name == "<check_lives>")
		runCheckLives(code);
	else if (code->name == "<credits>")
		endCredits(code);
	else
		error("invalid hardcoded level: %s", code->name.c_str());
}

void WetEngine::runCheckLives(Code *code) {
	if (_lives < 0)
		_nextLevel = "<game_over>";
	else
		_nextLevel = _checkpoint;
}

void WetEngine::runLevelMenu(Code *code) {
	if (_lastLevel == 0) {
		_nextLevel = Common::String::format("c%d", _ids[0]);
		return;
	}

	Common::Event event;
	byte *palette;
	Graphics::Surface *menu = decodeFrame("c_misc/menus.smk", 20, &palette);
	loadPalette(palette, 0, 256);
	byte black[3] = {0x00, 0x00, 0x00}; // Always red?
	byte lime[3] = {0x00, 0xFF, 0x00}; // Always red?
	byte green[3] = {0x2C, 0x82, 0x28}; // Always red?
	int maxLevel = 20;
	int currentLevel = 0;
	for (int i = 0; i < maxLevel; i++)
		if (i <= _lastLevel)
			loadPalette((byte *) &green, 192+i, 1);
		else
			loadPalette((byte *) &black, 192+i, 1);

	loadPalette((byte *) &lime, 192+currentLevel, 1);
	drawImage(*menu, 0, 0, false);
	bool cont = true;
	while (!shouldQuit() && cont) {
		while (g_system->getEventManager()->pollEvent(event)) {
			// Events
			switch (event.type) {

			case Common::EVENT_QUIT:
			case Common::EVENT_RETURN_TO_LAUNCHER:
				break;

			case Common::EVENT_KEYDOWN:
				if (event.kbd.keycode == Common::KEYCODE_DOWN && currentLevel < _lastLevel) {
					playSound("sound/extra.raw", 1, 11025);
					currentLevel++;
				} else if (event.kbd.keycode == Common::KEYCODE_UP && currentLevel > 0) {
					playSound("sound/extra.raw", 1, 11025);
					currentLevel--;
				} else if (event.kbd.keycode == Common::KEYCODE_RETURN ) {
					_nextLevel = Common::String::format("c%d", _ids[currentLevel]);
					cont = false;
				}

				for (int i = 0; i < maxLevel; i++)
					if (i <= _lastLevel)
						loadPalette((byte *) &green, 192+i, 1);
					else
						loadPalette((byte *) &black, 192+i, 1);


				loadPalette((byte *) &lime, 192+currentLevel, 1);
				drawImage(*menu, 0, 0, false);
				break;
			default:
				break;
			}
		}

		drawScreen();
		g_system->delayMillis(10);
	}
	menu->free();
	delete menu;
}

void WetEngine::runMainMenu(Code *code) {
	Common::Event event;
	uint32 c = 252; // green
	byte *palette;
	Graphics::Surface *menu = decodeFrame("c_misc/menus.smk", 16, &palette);
	Graphics::Surface *overlay = decodeFrame("c_misc/menus.smk", 18, nullptr);
	loadPalette(palette, 0, 256);
	Common::Rect subName(21, 10, 159, 24);

	drawImage(*menu, 0, 0, false);
	Graphics::Surface surName = overlay->getSubArea(subName);
	drawImage(surName, subName.left, subName.top, false);
	drawString("scifi08.fgx", "ENTER NAME :", 48, 50, 100, c);
	bool cont = true;
	while (!shouldQuit() && cont) {
		while (g_system->getEventManager()->pollEvent(event)) {
			// Events
			switch (event.type) {

			case Common::EVENT_QUIT:
			case Common::EVENT_RETURN_TO_LAUNCHER:
				break;

			case Common::EVENT_KEYDOWN:
				if (event.kbd.keycode == Common::KEYCODE_BACKSPACE)
					_name.deleteLastChar();
				else if (event.kbd.keycode == Common::KEYCODE_RETURN && !_name.empty()) {
					cont = false;
				}
				else if (Common::isAlnum(event.kbd.keycode)) {
					playSound("sound/m_choice.raw", 1);
					_name = _name + char(event.kbd.keycode - 32);
				}

				drawImage(*menu, 0, 0, false);
				drawImage(surName, subName.left, subName.top, false);
				drawString("scifi08.fgx", "ENTER NAME :", 48, 50, 100, c);
				drawString("scifi08.fgx", _name, 140, 50, 170, c);
				break;


			default:
				break;
			}
		}

		drawScreen();
		g_system->delayMillis(10);
	}

	if (_name == "COOLCOLE") {
		_lastLevel = 19;
		playSound("sound/extra.raw", 1);
	} else
		_lastLevel = 0;

	_name.toLowercase();
	bool found = loadProfile(_name);

	if (found)
		return;

	saveProfile(_name, _ids[_lastLevel]);

	Common::Rect subDifficulty(20, 104, 233, 119);
	Graphics::Surface surDifficulty = overlay->getSubArea(subDifficulty);
	drawImage(*menu, 0, 0, false);
	drawImage(surDifficulty, subDifficulty.left, subDifficulty.top, false);

	Common::Rect subWet(145, 149, 179, 159);
	Graphics::Surface surWet = overlay->getSubArea(subWet);
	drawImage(surWet, subWet.left, subWet.top, false);
	playSound("sound/no_rapid.raw", 1, 11025);

	Common::Rect subDamp(62, 149, 110, 159);
	Graphics::Surface surDamp = overlay->getSubArea(subDamp);

	Common::Rect subSoaked(204, 149, 272, 159);
	Graphics::Surface surSoaked = overlay->getSubArea(subSoaked);

	Common::Array<Common::String> difficulties;
	difficulties.push_back("0");
	difficulties.push_back("1");
	difficulties.push_back("2");
	uint32 idx = 1;

	cont = true;
	while (!shouldQuit() && cont) {
		while (g_system->getEventManager()->pollEvent(event)) {
			// Events
			switch (event.type) {

			case Common::EVENT_QUIT:
			case Common::EVENT_RETURN_TO_LAUNCHER:
				break;

			case Common::EVENT_KEYDOWN:
				if (event.kbd.keycode == Common::KEYCODE_LEFT && idx > 0) {
					playSound("sound/no_rapid.raw", 1, 11025);
					idx--;
				} else if (event.kbd.keycode == Common::KEYCODE_RIGHT && idx < 2) {
					playSound("sound/no_rapid.raw", 1, 11025);
					idx++;
				} else if (event.kbd.keycode == Common::KEYCODE_RETURN)
					cont = false;

				drawImage(*menu, 0, 0, false);
				drawImage(surDifficulty, subDifficulty.left, subDifficulty.top, false);

				if (difficulties[idx] == "0")
					drawImage(surDamp, subDamp.left, subDamp.top, false);
				else if (difficulties[idx] == "1")
					drawImage(surWet, subWet.left, subWet.top, false);
				else if (difficulties[idx] == "2")
					drawImage(surSoaked, subSoaked.left, subSoaked.top, false);
				else
					error("Invalid difficulty: %s", difficulties[idx].c_str());

				break;
			default:
				break;
			}
		}
		drawScreen();
		g_system->delayMillis(10);
	}

	_difficulty = difficulties[idx];
	_nextLevel = code->levelIfWin;

}

} // End of namespace Hypno
