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

#include "got/got.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "common/translation.h"
#include "engines/util.h"
#include "got/console.h"
#include "got/game/init.h"
#include "got/game/main.h"
#include "got/game/move.h"
#include "got/gfx/image.h"
#include "got/utils/res_archive.h"
#include "got/views/game_content.h"
#include "graphics/paletteman.h"

namespace Got {

#define SAVEGAME_VERSION 1

GotEngine *g_engine;

GotEngine::GotEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
																		 _gameDescription(gameDesc), _randomSource("Got") {
	g_engine = this;
}

GotEngine::~GotEngine() {
	_mixer->stopAll();
}

uint32 GotEngine::getFeatures() const {
	return _gameDescription->flags;
}

bool GotEngine::isDemo() const {
	return (_gameDescription->flags & ADGF_DEMO) != 0;
}

Common::String GotEngine::getGameId() const {
	return _gameDescription->gameId;
}

Common::Error GotEngine::run() {
	// Initialize 320x240 paletted graphics mode. Note that the original
	// main menu/dialogs ran at 320x200, but the game ran at 320x240.
	initGraphics(320, 240);

	// Set the engine's debugger console
	setDebugger(new Console());

	// Initialize resources and variables
	resInit();
	_vars.load();

	// General initialization
	if (_G(demo))
		initialize_game();
	syncSoundSettings();

	runGame();

	return Common::kNoError;
}

Common::Error GotEngine::saveGameStream(Common::WriteStream *stream, bool isAutosave) {
	stream->writeByte(SAVEGAME_VERSION);
	Common::Serializer s(nullptr, stream);
	s.setVersion(SAVEGAME_VERSION);

	return syncGame(s);
}

Common::Error GotEngine::loadGameStream(Common::SeekableReadStream *stream) {
	byte version = stream->readByte();
	if (version != SAVEGAME_VERSION)
		error("Invalid savegame version");

	Common::Serializer s(stream, nullptr);
	s.setVersion(version);

	return syncGame(s);
}

Common::Error GotEngine::syncGame(Common::Serializer &s) {
	char title[32];
	Common::fill(title, title + 32, 0);
	Common::strcpy_s(title, _G(playerName).c_str());
	s.syncBytes((byte *)title, 32);
	if (s.isLoading())
		_G(playerName) = title;

	_G(setup).sync(s);

	if (s.isLoading()) {
		// For savegames loaded directly from the ScummVM launcher,
		// take care of initializing game defaults before rest of loading
		if (!firstView() || firstView()->getName() != "Game")
			initialize_game();

		int area = _G(setup).area;
		if (area == 0)
			area = 1;

		g_vars->setArea(area);
	}

	_G(thor_info).sync(s);
	_G(sd_data).sync(s);

	if (s.isLoading())
		savegameLoaded();

	return Common::kNoError;
}

void GotEngine::savegameLoaded() {
	_G(current_area) = _G(thor_info).last_screen;

	_G(thor)->x = (_G(thor_info).last_icon % 20) * 16;
	_G(thor)->y = ((_G(thor_info).last_icon / 20) * 16) - 1;
	if (_G(thor)->x < 1)
		_G(thor)->x = 1;
	if (_G(thor)->y < 0)
		_G(thor)->y = 0;
	_G(thor)->dir = _G(thor_info).last_dir;
	_G(thor)->last_dir = _G(thor_info).last_dir;
	_G(thor)->health = _G(thor_info).last_health;
	_G(thor)->num_moves = 1;
	_G(thor)->vunerable = 60;
	_G(thor)->show = 60;
	_G(thor)->speed_count = 6;
	load_new_thor();

	g_vars->resetEndgameFlags();

	if (!_G(music_flag))
		_G(setup).music = 0;
	if (!_G(sound_flag))
		_G(setup).dig_sound = 0;
	if (_G(setup).music == 1) {
		if (GAME1 && _G(current_area) == 59) {
			music_play(5, true);
		} else {
			music_play(_G(level_type), true);
		}
	} else {
		_G(setup).music = 1;
		music_pause();
		_G(setup).music = 0;
	}

	_G(game_over) = _G(setup).game_over != 0;
	_G(slow_mode) = _G(setup).speed != 0;

	g_events->replaceView("Game", true);
	setup_load();
}

bool GotEngine::canLoadGameStateCurrently(Common::U32String *msg) {
	if (_G(demo)) {
		*msg = _("Savegames are not available in demo mode");
		return false;
	}

	// Only allow if not in the middle of area transition, dying, etc.
	return _G(gameMode) == MODE_NORMAL;
}

bool GotEngine::canSaveGameStateCurrently(Common::U32String *msg) {
	if (_G(demo)) {
		*msg = _("Savegames are not available in demo mode");
		return false;
	}

	// Don't allowing saving when not in-game
	if (!firstView() || firstView()->getName() != "Game" || _G(game_over))
		return false;

	// Only allow if not in the middle of area transition, dying, etc.
	return _G(gameMode) == MODE_NORMAL;
}

void GotEngine::syncSoundSettings() {
	Engine::syncSoundSettings();

	bool allSoundIsMuted = ConfMan.getBool("mute");

	_mixer->muteSoundType(Audio::Mixer::kSFXSoundType,
						  ConfMan.getBool("sfx_mute") || allSoundIsMuted);
	_mixer->muteSoundType(Audio::Mixer::kMusicSoundType,
						  ConfMan.getBool("music_mute") || allSoundIsMuted);
}

void GotEngine::pauseEngineIntern(bool pause) {
	g_vars->clearKeyFlags();
	if (_G(gameMode) == MODE_LIGHTNING)
		_G(gameMode) = MODE_NORMAL;

	if (_G(tornado_used)) {
		_G(tornado_used) = false;
		actor_destroyed(&_G(actor[2]));
	}

	if (_G(shield_on)) {
		_G(actor[2]).dead = 2;
		_G(actor[2]).used = 0;
		_G(shield_on) = false;
	}

	_G(lightning_used) = false;
	_G(thunder_flag) = 0;
	_G(hourglass_flag) = 0;

	Engine::pauseEngineIntern(pause);
}

Common::String GotEngine::getHighScoresSaveName() const {
	return Common::String::format("%s-scores.dat", _targetName.c_str());
}

} // End of namespace Got
