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

#include "common/stream.h"
#include "common/substream.h"

#include "engines/reevengi/formats/ard.h"

namespace Reevengi {

ArdArchive::ArdArchive(Common::SeekableReadStream *stream): _stream(stream) {
	_stream->seek(0);

	_stream->skip(4);	// File length
	_numFiles = _stream->readUint32LE();
}

ArdArchive::~ArdArchive() {
	_stream = nullptr;
}

Common::SeekableReadStream *ArdArchive::createReadStreamForMember(int numFile) const {
	int32 fileOffset = 0x800, fileLength;

	if (!_stream || (numFile >= _numFiles))
		return nullptr;

	_stream->seek(8);
	fileLength = _stream->readUint32LE();
	_stream->skip(4);

	for (int i=0; i<numFile; i++) {
		fileOffset += fileLength;
		fileOffset |= 0x7ff;
		fileOffset++;

		fileLength = _stream->readUint32LE();
		_stream->skip(4);
	}

	Common::SeekableSubReadStream *subStream = new Common::SeekableSubReadStream(_stream,
		fileOffset, fileOffset + fileLength);

	return subStream;
}

} // End of namespace Reevengi
