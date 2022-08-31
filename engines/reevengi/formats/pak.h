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

#ifndef REEVENGI_PAK_H
#define REEVENGI_PAK_H

#include "graphics/surface.h"
#include "image/image_decoder.h"

#include "reevengi/formats/tim.h"

namespace Common {
class SeekableReadStream;
}

namespace Reevengi {

/*--- Defines ---*/

#define RE1PAK_CHUNK_SIZE 32768

#define RE1PAK_DECODE_SIZE 35024

/*--- Types ---*/

typedef struct {
	uint32 flag;
	uint32 index;
	uint32 value;
} re1_pack_t;

class PakDecoder : public TimDecoder {
public:
	PakDecoder();
	virtual ~PakDecoder();

	void setSize(int w, int h);

	// ImageDecoder API
	virtual void destroy() override;
	virtual bool loadStream(Common::SeekableReadStream &pak) override;
	virtual const Graphics::Surface *getSurface() const { return TimDecoder::getSurface(); } /*override;*/

private:
	int _forcedW, _forcedH;

	// LZW depacker
	uint8 *_dstPointer;
	int _dstBufLen, _dstOffset;

	uint8 _srcByte;
	int _tmpMask;

	re1_pack_t _tmpArray2[RE1PAK_DECODE_SIZE];
	uint8 _decodeStack[RE1PAK_DECODE_SIZE];

	void depack(Common::SeekableReadStream &pak);
	uint32 read_bits(Common::SeekableReadStream &pak, int num_bits);
	int decodeString(int decodeStackOffset, uint32 code);
	void write_dest(uint8 value);

};

} // End of namespace Reevengi

#endif
