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

/*
Based on original Python version
https://forums.qhimm.com/index.php?topic=11225.0

# SEGA PRS Decompression (LZS variant)
# Credits:
# based on information/comparing output with
# Nemesis/http://www.romhacking.net/utils/671/
# puyotools/http://code.google.com/p/puyotools/
# fuzziqer software prs/http://www.fuzziqersoftware.com/projects.php
*/

#include "common/stream.h"
#include "common/memstream.h"

#include "engines/reevengi/formats/prs.h"

namespace Reevengi {

/*--- Defines ---*/

#define RE1PRS_CHUNK_SIZE 32768

PrsDecoder::PrsDecoder(): _srcPointer(nullptr), _srcLen(0), _dstPointer(nullptr),
  _dstLen(0)
{
	//
}

PrsDecoder::~PrsDecoder() {
	delete _srcPointer;
	_srcPointer = nullptr;
}

Common::SeekableReadStream *PrsDecoder::createReadStream(Common::SeekableReadStream *stream) {
	if (!stream)
		return nullptr;

	/* Read whole stream in memory */
	_srcLen = stream->size();
	_srcPointer = new uint8[_srcLen];
	if (!_srcPointer)
		return nullptr;

	if (stream->read(_srcPointer, _srcLen) != _srcLen)
		return nullptr;

	depack();

	if (!_dstPointer)
		return nullptr;

	Common::SeekableReadStream *mem_str = new Common::MemoryReadStream(_dstPointer, _dstLen);

	return mem_str;
}

void PrsDecoder::depack(void) {
	int cmd, t, offset, amount, start, j, a, b;

	_srcPos = _srcBit = _dstOffset = _cmd = 0;

	while (_srcPos<_srcLen) {
		cmd = read_bit();
		if (cmd) {
			write_dest(_srcPointer[_srcPos++]);
			continue;
		}

		t = read_bit();
		if (t) {

			a = _srcPointer[_srcPos++];
			b = _srcPointer[_srcPos++];

			offset = ((b << 8) | a) >> 3;
			amount = a & 7;

			if (_srcPos<_srcLen) {
				if (amount == 0) {
					amount = _srcPointer[_srcPos++] + 1;
				} else {
					amount += 2;
				}
			}

			start = _dstOffset - 0x2000 + offset;
		} else {
			amount = 0;

			for (j=0; j<2; j++) {
				amount <<= 1;
				amount |= read_bit();
			}
			offset = _srcPointer[_srcPos++];
			amount += 2;

			start = _dstOffset - 0x100 + offset;
		}

		for(j=0; j<amount; j++) {
			if (start < 0) {
				write_dest(0);
			} else if ( ((uint32) start) < _dstOffset) {
				write_dest( _dstPointer[start] );
			} else {
				write_dest(0);
			}
			start++;
		}
	}

	_dstLen = _dstOffset;
}

int PrsDecoder::read_bit(void)
{
	int retvalue;

	if (_srcBit==0) {
		_cmd = _srcPointer[_srcPos++];
		_srcBit = 8;
	}

	retvalue = _cmd & 1;
	_cmd >>= 1;
	_srcBit--;

	return retvalue;
}

void PrsDecoder::write_dest(uint8 value)
{
	if (!_dstPointer || (_dstOffset>=_dstLen)) {
		_dstLen += RE1PRS_CHUNK_SIZE;
		_dstPointer = (uint8 *) realloc(_dstPointer, _dstLen);

		if (!_dstPointer) {
			//fprintf(stderr, "prs: can not allocate %d bytes\n", _dstLen);
			return;
		}
	}

	_dstPointer[_dstOffset++] = value;
}

} // End of namespace Reevengi
