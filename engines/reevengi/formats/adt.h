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

#ifndef REEVENGI_ADT_H
#define REEVENGI_ADT_H

#include "graphics/surface.h"
#include "image/image_decoder.h"

#include "reevengi/formats/tim.h"

namespace Common {
class SeekableReadStream;
}

namespace Reevengi {

/*--- Types ---*/

typedef struct {
	uint32 start;
	uint32 length;
} unpackArray8_t;

typedef struct {
	int32 nodes[2];
} node_t;

typedef struct {
	uint32 start;
	uint32 length;
	uint32 *ptr4;
	unpackArray8_t *ptr8;
	node_t *tree;
} unpackArray_t;

/*--- Class ---*/

class AdtDecoder : public TimDecoder {
public:
	AdtDecoder();
	virtual ~AdtDecoder();

	// ImageDecoder API
	virtual void destroy() override;
	virtual bool loadStream(Common::SeekableReadStream &adt) override;
	virtual const Graphics::Surface *getSurface() const { return TimDecoder::getSurface(); } /*override;*/

	virtual bool loadStreamNumber(Common::SeekableReadStream &adt, int numFile = 0);

private:
	// Reorganize raw image
	void ProcessRawImage();

	// Hufmman like depacker
	uint8 *_dstPointer;
	int _dstBufLen, _dstOffset;

	uint8 *_srcPointer;
	int _srcOffset;
	unsigned char _srcByte;
	int _srcNumBit;

	uint8 *_tmp32k, *_tmp16k;
	int _tmp32kOffset, _tmp16kOffset;

	unpackArray_t _array1, _array2, _array3;
	unsigned short _freqArray[17];

	void depack(Common::SeekableReadStream &adt);
	void initTmpArray(unpackArray_t *array, int start, int length);
	void initTmpArrayData(unpackArray_t *array);

	int readSrcBits(Common::SeekableReadStream &adt, int numBits);
	int readSrcOneBit(Common::SeekableReadStream &adt);
	int readSrcBitfieldArray(Common::SeekableReadStream &adt, unpackArray_t *array, uint32 curIndex);
	int readSrcBitfield(Common::SeekableReadStream &adt);

	void initUnpackBlock(Common::SeekableReadStream &adt);
	void initUnpackBlockArray(unpackArray_t *array);
	int initUnpackBlockArray2(unpackArray_t *array);

};

} // End of namespace Reevengi

#endif

