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
#include "common/memstream.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/reevengi/formats/bss_sld.h"

namespace Reevengi {

BssSldDecoder::BssSldDecoder(int depackMode): _dstPointer(nullptr), _dstBufLen(0),
	_depackMode(depackMode) {
}

BssSldDecoder::~BssSldDecoder() {
	destroy();
}

void BssSldDecoder::destroy() {
	delete[] _dstPointer;
	_dstPointer = nullptr;

	_dstBufLen = 0;

	TimDecoder::destroy();
}

bool BssSldDecoder::loadStream(Common::SeekableReadStream &bsssld) {
	destroy();

	if (_depackMode==2) {
		depack_re2(bsssld);
	}
	if (_depackMode==3) {
		depack_re3(bsssld);
	}

	if (!_dstPointer) {
		return false;
	}
/*
	Common::DumpFile adf;
	adf.open("img.tim");
	adf.write(_dstPointer, _dstBufLen);
	adf.close();
*/
	Common::SeekableReadStream *mem_str = new Common::MemoryReadStream(_dstPointer, _dstBufLen);
	if (!mem_str) {
		return false;
	}

	return TimDecoder::loadStream(*mem_str);
}

void BssSldDecoder::depack_re2(Common::SeekableReadStream &bsssld)
{
	_dstBufLen = bsssld.readUint32LE();
	_dstPointer = (uint8 *) malloc(_dstBufLen);
	memset(_dstPointer, 0, _dstBufLen);

	bsssld.skip(2);

	int count, dstPos = 0;

	while ((bsssld.pos()<bsssld.size()) && (dstPos<_dstBufLen)) {
		byte srcByte = bsssld.readByte();

		while ((srcByte & 0x10)==0) {
			count = srcByte & 0x0f;
			int srcOffset = (-256 | (srcByte & 0xe0))<<3;
			srcByte = bsssld.readByte();
			srcOffset |= srcByte;
			if (count == 0x0f) {
				srcByte = bsssld.readByte();
				count += srcByte;
			}
			count += 3;

			memcpyOverlap(&_dstPointer[dstPos], &_dstPointer[dstPos+srcOffset], count);
			dstPos += count;

			srcByte = bsssld.readByte();
		}

		if (srcByte == 0xff)
			break;

		count = ((srcByte | 0xffe0) ^ 0xffff)+1;
		if (count == 0x10) {
			srcByte = bsssld.readByte();
			count += srcByte;
		}

		bsssld.read(&_dstPointer[dstPos], count);
		dstPos += count;
	}
}

void BssSldDecoder::depack_re3(Common::SeekableReadStream &bsssld)
{
	int numBlocks = bsssld.readUint32LE();
	int count, offset, dstPos = 0;

	_dstBufLen = 65536;
	_dstPointer = (uint8 *) malloc(_dstBufLen);
	memset(_dstPointer, 0, _dstBufLen);

	for(int i=0; (i<numBlocks) && (bsssld.pos()<bsssld.size()); i++) {
		byte srcByte = bsssld.readByte();

		if ((srcByte & 0x80) != 0) {
			count = srcByte & 0x7f;

			if (dstPos+count >= _dstBufLen) {
				_dstBufLen += 65536;
				_dstPointer = (byte *) realloc(_dstPointer, _dstBufLen);
			}

			bsssld.read(&_dstPointer[dstPos], count);
			dstPos += count;
		} else {
			offset = srcByte<<8;
			srcByte = bsssld.readByte();
			offset |= srcByte;
			count = (offset>>11)+2;
			offset &= 0x7ff;

			if (dstPos+count >= _dstBufLen) {
				_dstBufLen += 65536;
				_dstPointer = (byte *) realloc(_dstPointer, _dstBufLen);
			}

			memcpyOverlap(&_dstPointer[dstPos], &_dstPointer[dstPos-(offset+4)], count);
			dstPos += count;
		}
	}

	_dstBufLen = dstPos;
	_dstPointer = (byte *) realloc(_dstPointer, _dstBufLen);
}

void BssSldDecoder::memcpyOverlap(uint8 *dest, uint8 *src, int count) {
	int i;

	for (i=0; i<count; i++) {
		dest[i] = src[i];
	}
}

} // End of namespace Reevengi
