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

#include "graphics/surface.h"

#include "common/system.h"
#include "common/timer.h"

#include "engines/reevengi/movie/movie.h"
#include "engines/reevengi/reevengi.h"
//#include "engines/grim/debug.h"
//#include "engines/grim/savegame.h"

namespace Reevengi {

MoviePlayer *g_movie;

MoviePlayer::MoviePlayer() {
	_fname = "";
	_stream = nullptr;
	_channels = -1;
	_freq = 22050;
	_videoFinished = false;
	_videoLooping = false;
	_videoPause = true;
	_updateNeeded = false;
	_showSubtitles = true;
	_movieTime = 0;
	_frame = -1;
	_videoDecoder = nullptr;
	_internalSurface = nullptr;
	_externalSurface = new Graphics::Surface();
	_timerStarted = false;
}

MoviePlayer::~MoviePlayer() {
	// Remove the callback immediately, so we're sure timerCallback() doesn't get called
	// after the deinit() or the deletes.
	if (_timerStarted)
		g_system->getTimerManager()->removeTimerProc(&timerCallback);

	deinit();
	delete _stream;
	delete _videoDecoder;
	delete _externalSurface;
}

void MoviePlayer::pause(bool p) {
	Common::StackLock lock(_frameMutex);
	_videoPause = p;
	_videoDecoder->pauseVideo(p);
}

void MoviePlayer::stop() {
	Common::StackLock lock(_frameMutex);
	deinit();
	//g_grim->setMode(GrimEngine::NormalMode);
}

void MoviePlayer::timerCallback(void *instance) {
	MoviePlayer *movie = static_cast<MoviePlayer *>(instance);
	Common::StackLock lock(movie->_frameMutex);
	if (movie->prepareFrame())
		movie->postHandleFrame();
}

bool MoviePlayer::prepareFrame() {
	if (!_videoLooping && _videoDecoder->endOfVideo()) {
		_videoFinished = true;
	}

	if (_videoPause) {
		return false;
	}

	if (_videoFinished) {
		//if (g_grim->getMode() == GrimEngine::SmushMode) {
		//	g_grim->setMode(GrimEngine::NormalMode);
		//}
		_videoPause = true;
		return false;
	}

	if (_videoDecoder->getTimeToNextFrame() > 0)
		return false;

	handleFrame();
	_internalSurface = _videoDecoder->decodeNextFrame();
	if (_frame != _videoDecoder->getCurFrame()) {
		_updateNeeded = true;
	}

	_movieTime = _videoDecoder->getTime();
	_frame = _videoDecoder->getCurFrame();

	return true;
}

Graphics::Surface *MoviePlayer::getDstSurface() {
	Common::StackLock lock(_frameMutex);
	if (_updateNeeded && _internalSurface) {
		_externalSurface->copyFrom(*_internalSurface);
	}

	return _externalSurface;
}

void MoviePlayer::init() {
	if (!_timerStarted) {
		g_system->getTimerManager()->installTimerProc(&timerCallback, 10000, this, "movieLoop");
		_timerStarted = true;
	}

	_frame = -1;
	_movieTime = 0;
	_updateNeeded = false;
	_videoFinished = false;
}

void MoviePlayer::deinit() {
	//Debug::debug(Debug::Movie, "Deinitting video '%s'.\n", _fname.c_str());

	if (_videoDecoder)
		_videoDecoder->close();

	_internalSurface = nullptr;

	if (_externalSurface)
		_externalSurface->free();

	_videoPause = false;
	_videoFinished = true;
}

bool MoviePlayer::play(const Common::String &filename, bool looping, bool start, bool showSubtitles) {
	Common::StackLock lock(_frameMutex);
	deinit();
	_fname = filename;
	_videoLooping = looping;
	_showSubtitles = showSubtitles;

	if (!loadFile(_fname))
		return false;

	//Debug::debug(Debug::Movie, "Playing video '%s'.\n", filename.c_str());

	init();
	_internalSurface = nullptr;

	if (start) {
		_videoDecoder->start();

		// Get the first frame immediately
		timerCallback(this);
	}

	return true;
}

bool MoviePlayer::loadFile(const Common::Path &filename) {
	return _videoDecoder->loadFile(filename);
}

bool MoviePlayer::play(Common::SeekableReadStream *stream, bool looping, bool start, bool showSubtitles) {
	Common::StackLock lock(_frameMutex);
	deinit();
	_stream = stream;
	_videoLooping = looping;
	_showSubtitles = showSubtitles;

	if (!loadStream(_stream))
		return false;

	//Debug::debug(Debug::Movie, "Playing video '%s'.\n", filename.c_str());

	init();
	_internalSurface = nullptr;

	if (start) {
		_videoDecoder->start();

		// Get the first frame immediately
		timerCallback(this);
	}

	return true;
}

bool MoviePlayer::loadStream(Common::SeekableReadStream *stream) {
	return _videoDecoder->loadStream(stream);
}

#if 0
void MoviePlayer::saveState(SaveGame *state) {
	Common::StackLock lock(_frameMutex);
	state->beginSection('SMUS');

	state->writeString(_fname);

	state->writeLESint32(_frame);
	state->writeFloat(_movieTime);
	state->writeBool(_videoFinished);
	state->writeBool(_videoLooping);

	save(state);

	state->endSection();
}

void MoviePlayer::restoreState(SaveGame *state) {
	Common::StackLock lock(_frameMutex);
	state->beginSection('SMUS');

	_fname = state->readString();

	int32 frame = state->readLESint32();
	float movieTime = state->readFloat();
	bool videoFinished = state->readBool();
	bool videoLooping = state->readBool();

	if (!videoFinished && !_fname.empty()) {
		play(_fname.c_str(), videoLooping, false);
	}
	_frame = frame;
	_movieTime = movieTime;

	restore(state);

	state->endSection();
}
#endif

#if !defined(USE_MPEG2) || !defined(USE_BINK)
#define NEED_NULLPLAYER
#endif

// Temporary fix while reworking codecs:
#ifndef NEED_NULLPLAYER
#define NEED_NULLPLAYER
#endif

// Fallback for when USE_MPEG2 / USE_BINK isnt defined

#ifdef NEED_NULLPLAYER
class NullPlayer : public MoviePlayer {
public:
	NullPlayer(const char *codecID) {
		warning("%s-playback not compiled in, but needed", codecID);
		_videoFinished = true; // Rigs all movies to be completed.
	}
	~NullPlayer() {}
	bool play(const Common::String &filename, bool looping, bool start = true, bool showSubtitles = false) override { return true; }
	bool loadFile(const Common::Path &filename) override { return true; }
	void stop() override {}
	void pause(bool p) override {}
	void saveState(SaveGame *state) {}
	void restoreState(SaveGame *state) {}
private:
	static void timerCallback(void *ptr) {}
	void handleFrame() override {}
	bool prepareFrame() override { return false; }
	void init() override {}
	void deinit() override {}
};
#endif

#ifndef USE_MPEG2
MoviePlayer *CreateMpegPlayer() {
	return new NullPlayer("MPEG2");
}
#endif

#ifndef USE_BINK
MoviePlayer *CreateBinkPlayer(bool demo) {
	return new NullPlayer("BINK");
}
#endif

} // end of namespace Reevengi
