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
#include "video/psx_decoder.h"

#include "engines/reevengi/movie/psx.h"
#include "engines/reevengi/reevengi.h"


namespace Reevengi {

MoviePlayer *CreatePsxPlayer(bool emul_cd) {
	return new PsxPlayer(emul_cd);
}

PsxPlayer::PsxPlayer(bool emul_cd) : MoviePlayer() {
	_emul_cd = emul_cd;
	_videoDecoder = new Video::PSXStreamDecoder(Video::PSXStreamDecoder::kCD2x);
}

bool PsxPlayer::loadFile(const Common::String &filename) {
	//_fname = Common::String("Video/") + filename + ".pss";
	_fname = filename;

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(_fname);
	if (!stream)
		return false;

	if (_emul_cd) {
		stream = new PsxCdStream(stream);
	}
	if (!stream)
		return false;

	_videoDecoder->setDefaultHighColorFormat(Graphics::PixelFormat(4, 8, 8, 8, 0, 8, 16, 24, 0));
	_videoDecoder->loadStream(stream);
	_videoDecoder->start();

	return true;
}

#define RAW_CD_SECTOR_SIZE	2352
#define DATA_CD_SECTOR_SIZE	2048

#define CDXA_TYPE_DATA     0x08
#define CDXA_TYPE_AUDIO    0x04

#define CD_SYNC_SIZE 12
#define CD_SEC_SIZE 4
#define CD_XA_SIZE 8
#define CD_DATA_SIZE 2048

#define STR_MAGIC 0x60010180

PsxCdStream::PsxCdStream(Common::SeekableReadStream *srcStream):
	Common::SeekableReadStream(), _prevSector(-1), _curSector(0),
	_pos(0) {

	_srcStream = srcStream;

	_srcStream->seek(0, SEEK_END);
	_size = (_srcStream->pos() / DATA_CD_SECTOR_SIZE) * RAW_CD_SECTOR_SIZE;
	_srcStream->seek(0, SEEK_SET);

	_bufSector = new byte[RAW_CD_SECTOR_SIZE];
	memset(_bufSector, 0, RAW_CD_SECTOR_SIZE);
	memset(&_bufSector[1], 0xff, 10);
	_bufSector[0x11] = 1;

	//_adf.open("audio.bin");
}

PsxCdStream::~PsxCdStream() {
	delete _bufSector;
	_bufSector = nullptr;

	//_adf.close();
}

uint32 PsxCdStream::read(void *dataPtr, uint32 dataSize) {
	uint32 readSize = 0;

	while (dataSize>0) {
		/* Read new source sector ? */
		if (_curSector != _prevSector) {
			byte *srcBuf = &_bufSector[CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE];

			_srcStream->read(srcBuf, DATA_CD_SECTOR_SIZE);
			_prevSector = _curSector;

			if (READ_BE_INT32(srcBuf) == STR_MAGIC) {
				/* Prepare video sector */
				_bufSector[0x12] = CDXA_TYPE_DATA;
				_bufSector[0x13] = 0;
			} else {
				/* Prepare audio sector */
				_bufSector[0x12] = CDXA_TYPE_AUDIO;
				_bufSector[0x13] = 1;	// Stereo, 37.8KHz
			}
		}

		int srcPos = _pos % RAW_CD_SECTOR_SIZE;
		int srcRead = MIN<int32>(dataSize, RAW_CD_SECTOR_SIZE-srcPos);

		memcpy(dataPtr, &_bufSector[srcPos], srcRead);
		dataSize -= srcRead;
		readSize += srcRead;

		_pos += srcRead;
		_curSector = _pos / RAW_CD_SECTOR_SIZE;
	}

	return readSize;
}

bool PsxCdStream::seek(int64 offs, int whence) {
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

	_curSector = _pos / RAW_CD_SECTOR_SIZE;

	/* Always seek to start of sector */
	_srcStream->seek(_curSector * DATA_CD_SECTOR_SIZE, whence);

	return true;
}

} // end of namespace Reevengi
