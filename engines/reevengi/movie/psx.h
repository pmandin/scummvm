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

#ifndef REEVENGI_PSX_PLAYER_H
#define REEVENGI_PSX_PLAYER_H

//#include "common/file.h"

#include "engines/reevengi/movie/movie.h"

namespace Reevengi {

class PsxPlayer : public MoviePlayer {
private:
	bool _emul_cd;

	bool loadFile(const Common::String &filename) override;
	//bool _demo;

public:
	PsxPlayer(bool emul_cd = true);
};

class PsxCdStream : public Common::SeekableReadStream {
private:
	Common::SeekableReadStream *_srcStream;
	byte *_bufSector;
	uint32 _size;
	uint32 _pos;
	int32 _prevSector, _curSector;
//	Common::DumpFile _adf;

public:
	PsxCdStream(Common::SeekableReadStream *srcStream);
	~PsxCdStream();

	uint32 read(void *dataPtr, uint32 dataSize);

	bool eos() const { return _srcStream->eos(); }

	int64 pos() const { return _pos; }
	int64 size() const { return _size; }

	bool seek(int64 offs, int whence = SEEK_SET);
};

} // end of namespace Reevengi

#endif
