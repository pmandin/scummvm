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

#include "engines/reevengi/formats/pak.h"

namespace Reevengi {

PakDecoder::PakDecoder(): _forcedW(0), _forcedH(0), _dstPointer(nullptr), _dstBufLen(0) {
}

PakDecoder::~PakDecoder() {
	destroy();
}

void PakDecoder::destroy() {
	if (_dstPointer) {
		free(_dstPointer);
		_dstPointer = nullptr;
	}

	_dstBufLen = 0;

	TimDecoder::destroy();
}

void PakDecoder::setSize(int w, int h) {
	_forcedW = w;
	_forcedH = h;
}

bool PakDecoder::loadStream(Common::SeekableReadStream &pak) {
	destroy();

	depack(pak);
	if (!_dstPointer) {
		return false;
	}
/*
	Common::DumpFile adf;
	adf.open("img_pak.bin");
	adf.write(_dstPointer, _dstBufLen);
	adf.close();
*/
	Common::SeekableReadStream *mem_str = new Common::MemoryReadStream(_dstPointer, _dstBufLen);
	if (!mem_str) {
		return false;
	}

	if (_forcedW || _forcedH) {
		TimDecoder::setSize(_forcedW, _forcedH);
	}
	return TimDecoder::loadStream(*mem_str);
}

/*--- LZW depacker ---*/
/* FIXME: should be a distinct class, inherited from Bitstream */

void PakDecoder::depack(Common::SeekableReadStream &pak)
{
	int num_bits_to_read, i, stop = 0;
	uint32 lzwnew, c, lzwold, lzwnext;

	_dstPointer = nullptr;
	_dstBufLen = _dstOffset = 0;

	_tmpMask = 0x80;
	_srcByte = 0;

	memset(_tmpArray2, 0, sizeof(_tmpArray2));

	while (!stop) {
		for (i=0; i<RE1PAK_DECODE_SIZE; i++) {
			_tmpArray2[i].flag = 0xffffffff;
		}
		lzwnext = 0x103;
		num_bits_to_read = 9;

		c = lzwold = read_bits(pak, num_bits_to_read);

		if (lzwold == 0x100) {
			break;
		}

		write_dest(c);

		for(;;) {
			lzwnew = read_bits(pak, num_bits_to_read);

			if (lzwnew == 0x100) {
				stop = 1;
				break;
			}

			if (lzwnew == 0x102) {
				break;
			}

			if (lzwnew == 0x101) {
				num_bits_to_read++;
				continue;
			}

			if (lzwnew >= lzwnext) {
				_decodeStack[0] = c;
				i = decodeString(1, lzwold);
			} else {
				i = decodeString(0, lzwnew);
			}

			c = _decodeStack[i];

			while (i>=0) {
				write_dest(_decodeStack[i--]);
			}

			_tmpArray2[lzwnext].index = lzwold;
			_tmpArray2[lzwnext].value = c;
			lzwnext++;

			lzwold = lzwnew;
		}
	}

	if (!_dstPointer) {
		_dstBufLen = _dstOffset;
		_dstPointer = (uint8 *) realloc(_dstPointer, _dstBufLen);
	}
}

uint32 PakDecoder::read_bits(Common::SeekableReadStream &pak, int num_bits)
{
	uint32 value=0, mask;

	mask = 1<<(--num_bits);

	while (mask>0) {
		if (_tmpMask == 0x80) {
			_srcByte = pak.readByte();
		}

		if ((_tmpMask & _srcByte)!=0) {
			value |= mask;
		}

		_tmpMask >>= 1;
		mask >>= 1;

		if (_tmpMask == 0) {
			_tmpMask = 0x80;
		}
	}

	return value;
}

int PakDecoder::decodeString(int decodeStackOffset, uint32 code)
{
	while (code>255) {
		_decodeStack[decodeStackOffset++] = _tmpArray2[code].value;
		code = _tmpArray2[code].index;
	}
	_decodeStack[decodeStackOffset] = code;

	return decodeStackOffset;
}

void PakDecoder::write_dest(uint8 value)
{
	if ((_dstPointer==nullptr) || (_dstOffset>=_dstBufLen)) {
		_dstBufLen += RE1PAK_CHUNK_SIZE;
		_dstPointer = (uint8 *) realloc(_dstPointer, _dstBufLen);
		if (_dstPointer==NULL) {
			debug(3, "%s: can not allocate %d bytes\n", __FUNCTION__, _dstBufLen);
			return;
		}
	}

	_dstPointer[_dstOffset++] = value;
}

} // End of namespace Reevengi
