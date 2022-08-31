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

//#include "common/file.h"
#include "common/memstream.h"
#include "common/stream.h"
#include "common/substream.h"

#include "engines/reevengi/formats/sld.h"

namespace Reevengi {

/*--- Types ---*/

typedef struct {
	uint32 flag;	// 0=no file, 1=has file
	uint32 length;
} sld_header_t;

SldArchive::SldArchive(Common::SeekableReadStream *stream): _stream(stream) {
}

SldArchive::~SldArchive() {
	_stream = nullptr;
}

Common::SeekableReadStream *SldArchive::createReadStreamForMember(int numFile) const {
	sld_header_t sldHeader;
	int curFile = 0;

	if (!_stream)
		return nullptr;

	_stream->seek(0);
	memset(&sldHeader, 0, sizeof(sldHeader));
	while (!_stream->eos()) {
		_stream->read(&sldHeader, sizeof(sldHeader));

		if (curFile==numFile)
			break;

		if (FROM_LE_32(sldHeader.flag)) {
			_stream->seek(FROM_LE_32(sldHeader.length) - sizeof(sldHeader), SEEK_CUR);
		}
		++curFile;
	}
	if (FROM_LE_32(sldHeader.flag)==0)
		return nullptr;

	int32 fileOffset = _stream->pos();
	int32 compSize = FROM_LE_32(sldHeader.length);
	Common::SeekableSubReadStream subStream(_stream, fileOffset, fileOffset+compSize);

	return new SldFileStream(&subStream);
}

// SldFileStream

SldFileStream::SldFileStream(Common::SeekableReadStream *subStream):
	_pos(0), _size(0), _arcStream(subStream) {

	_arcStream->seek(0);
	depackFile();
/*
	Common::DumpFile adf;
	adf.open("img0.tim");
	adf.write(_fileBuffer, _size);
	adf.close();
*/
	_arcStream = nullptr;
}

SldFileStream::~SldFileStream() {
	free(_fileBuffer);
	_fileBuffer = nullptr;
}

uint32 SldFileStream::read(void *dataPtr, uint32 dataSize) {
	int32 sizeRead = MIN<int32>(dataSize, _size-_pos);

	memcpy(dataPtr, &_fileBuffer[_pos], sizeRead);
	_pos += sizeRead;

	return sizeRead;
}

bool SldFileStream::seek(int64 offs, int whence) {
	switch(whence) {
		case SEEK_SET:
			_pos = offs;
			break;
		case SEEK_END:
			_pos = offs+_size;
			break;
		case SEEK_CUR:
			_pos += offs;
			break;
	}

	return true;
}

void SldFileStream::depackFile(void) {
	int32 numBlocks = _arcStream->readUint32LE();
	uint32 count, offset, dstIndex = 0;
	uint8 start;

	_size = 65536;
	_fileBuffer = (byte *) malloc(_size);

	for(int i=0; i<numBlocks; i++) {
		_arcStream->read(&start, 1);

		if (start & 0x80) {
			count = start & 0x7f;

			if (dstIndex+count>_size) {
				_size += 65536;
				_fileBuffer = (byte *) realloc(_fileBuffer, _size);
			}
			_arcStream->read(&_fileBuffer[dstIndex], count);

			dstIndex += count;
		} else {
			int tmp = start<<8;

			_arcStream->read(&start, 1);
			tmp |= start;

			offset = (tmp & 0x7ff)+4;
			count = (tmp>>11)+2;

			if (dstIndex+count>_size) {
				_size += 65536;
				_fileBuffer = (byte *) realloc(_fileBuffer, _size);
			}

			for (uint32 j=0; j<count; j++) {
				_fileBuffer[dstIndex+j] = _fileBuffer[dstIndex+j-offset];
			}
			dstIndex += count;
		}

	}

	_size = dstIndex;
	_fileBuffer = (byte *) realloc(_fileBuffer, _size);
}

} // End of namespace Reevengi
