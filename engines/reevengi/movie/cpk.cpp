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

#include "common/system.h"
#include "engines/advancedDetector.h"

#include "engines/reevengi/movie/cpk.h"
#include "engines/reevengi/movie/cpk_decoder.h"
#include "engines/reevengi/movie/movie.h"
#include "engines/reevengi/reevengi.h"


namespace Reevengi {

MoviePlayer *CreateCpkPlayer(Common::SeekableReadStream *stream) {
	return new CpkPlayer(stream);
}

CpkPlayer::CpkPlayer(Common::SeekableReadStream *stream) : MoviePlayer() {
	_stream = stream;
	_videoDecoder = new CpkMovieDecoder();
}

CpkPlayer::~CpkPlayer() {
	MoviePlayer::~MoviePlayer();

	delete _stream;
}

bool CpkPlayer::loadFile(const Common::Path &filename) {
	//_fname = Common::String("Video/") + filename + ".pss";
	_fname = filename;

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(_fname);
	if (!stream)
		return false;

	return loadStream(stream);
}

bool CpkPlayer::loadStream(Common::SeekableReadStream *stream) {

	_videoDecoder->setOutputPixelFormat(Graphics::PixelFormat(4, 8, 8, 8, 0, 8, 16, 24, 0));
	_videoDecoder->loadStream(stream);
	_videoDecoder->start();

	return true;
}

} // end of namespace Reevengi
