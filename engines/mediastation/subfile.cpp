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

#include "mediastation/subfile.h"
#include "mediastation/debugchannels.h"

namespace MediaStation {

Subfile::Subfile() : _stream(nullptr) {}

Subfile::Subfile(Common::SeekableReadStream *stream) : _stream(stream) {
	// VERIFY FILE SIGNATURE.
	debugC(5, kDebugLoading, "\n*** Subfile::Subfile(): Got new subfile (@0x%llx) ***", static_cast<long long int>(_stream->pos()));
	_rootChunk = nextChunk();
	if (_rootChunk._id != MKTAG('R', 'I', 'F', 'F'))
		// TODO: These need to be interpreted as ASCII.
		error("Subfile::Subfile(): Expected \"RIFF\" chunk, got %s (@0x%llx)", tag2str(_rootChunk._id), static_cast<long long int>(_stream->pos()));
	_stream->skip(4); // IMTS

	// READ RATE CHUNK.
	// This chunk shoudl always contain just one piece of data - the "rate"
	// (whatever that is). Usually it is zero.
	// TODO: Figure out what this actually is.
	Chunk rateChunk = nextChunk();
	if (rateChunk._id != MKTAG('r', 'a', 't', 'e'))
		error("Subfile::Subfile(): Expected \"rate\" chunk, got %s (@0x%llx)", tag2str(_rootChunk._id), static_cast<long long int>(_stream->pos()));
	_rate = _stream->readUint32LE();

	// READ PAST LIST CHUNK.
	nextChunk();

	// QUEUE UP THE FIRST DATA CHUNK.
	if (_stream->readUint32BE() != MKTAG('d', 'a', 't', 'a'))
		error("Subfile::Subfile(): Expected \"data\" as first bytes of subfile, got %s @0x%llx)", tag2str(rateChunk._id), static_cast<long long int>(_stream->pos()));
}

Chunk Subfile::nextChunk() {
	// Chunks always start on even-indexed bytes.
	if (_stream->pos() & 1)
		_stream->skip(1);
	_currentChunk = Chunk(_stream);
	return _currentChunk;
}

bool Subfile::atEnd() {
	// TODO: Is this the best place to put this and approach to use?
	return _rootChunk.bytesRemaining() == 0;
}

} // End of namespace MediaStation
