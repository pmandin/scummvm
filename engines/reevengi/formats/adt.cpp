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

#include "engines/reevengi/formats/adt.h"

namespace Reevengi {

/*--- Consts ---*/

#define NODE_LEFT 0
#define NODE_RIGHT 1

/*--- Class ---*/

AdtDecoder::AdtDecoder() {
	_dstPointer = 0;
	_dstBufLen = 0;
}

AdtDecoder::~AdtDecoder() {
	destroy();
}

void AdtDecoder::destroy() {
	if (_dstPointer) {
		free(_dstPointer);
		_dstPointer = nullptr;
	}

	_dstBufLen = 0;

	TimDecoder::destroy();
}

bool AdtDecoder::loadStream(Common::SeekableReadStream &adt) {
	destroy();

	depack(adt);
	if (!_dstPointer) {
		return false;
	}
/*
	Common::DumpFile adf;
	adf.open("img.bin");
	adf.write(_dstPointer, _dstBufLen);
	adf.close();
*/
	Common::SeekableReadStream *mem_str = new Common::MemoryReadStream(_dstPointer, _dstBufLen);
	if (!mem_str) {
		return false;
	}

	if (!TimDecoder::loadStream(*mem_str)) {
		ProcessRawImage();
	}

	return true;
/*
	if (_dstBufLen == 320*256*2) {
		ProcessRawImage();
		return true;
	}

	return TimDecoder::loadStream(*mem_str);
*/
}

bool AdtDecoder::loadStreamNumber(Common::SeekableReadStream &adt, int numFile) {
	if (numFile==0)
		return loadStream(adt);

	if (numFile!=1)
		return false;

	/* File following first raw image 320x256x16 bits is TIM file */
	destroy();

	depack(adt);
	if (!_dstPointer) {
		return false;
	}
/*
	Common::DumpFile adf;
	adf.open("re2.tim");
	adf.write(&_dstPointer[0x28000], _dstBufLen-0x28000);
	adf.close();
*/
	Common::SeekableReadStream *mem_str = new Common::MemoryReadStream(&_dstPointer[0x28000], _dstBufLen-0x28000);
	if (!mem_str) {
		return false;
	}

	if (!TimDecoder::loadStream(*mem_str)) {
		ProcessRawImage();
	}

	return true;
}

/*--- Raw image and reorganize ---*/

void AdtDecoder::ProcessRawImage()
{
	int x, y;
	uint16 *surface_line, *src_line, *srcBuffer = (uint16 *) _dstPointer;
	uint16 *dstBuffer;
	Graphics::PixelFormat fmt = Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0);

	TimDecoder::CreateTimSurface(320, 240, fmt);
	dstBuffer = (uint16 *) (getSurface()->getBasePtr(0,0));

	/* First 256x256 block */
	surface_line = dstBuffer;
	src_line = srcBuffer;
	for (y=0; y<240; y++) {
		uint16 *surface_col = surface_line;
		for (x=0; x<256; x++) {
			uint16 col = *src_line++;
			*surface_col++ = readPixel(col);
		}
		surface_line += 320;
	}

	/* Then 2x64x128 inside 128x128 */
	surface_line = dstBuffer;
	surface_line += 256;
	src_line = &srcBuffer[256*256];
	for (y=0; y<128; y++) {
		uint16 *surface_col = surface_line;
		uint16 *src_col = src_line;
		for (x=0; x<64; x++) {
			uint16 col = *src_col++;
			*surface_col++ = readPixel(col);
		}

		surface_line += 320;
		src_line += 128;
	}

	surface_line = dstBuffer;
	surface_line += (320*128)+256;
	src_line = &srcBuffer[256*256+64];
	for (y=0; y<240-128; y++) {
		uint16 *surface_col = surface_line;
		uint16 *src_col = src_line;
		for (x=0; x<64; x++) {
			uint16 col = *src_col++;
			*surface_col++ = readPixel(col);
		}

		surface_line += 320;
		src_line += 128;
	}
}

/*--- Hufmman like depacker ---*/

/* Initialize temporary tables, read each block and depack it */
void AdtDecoder::depack(Common::SeekableReadStream &adt)
{
	int blockLength;

	_srcPointer = _dstPointer = nullptr;
	_srcByte = _srcNumBit = _srcOffset = _dstOffset = _dstBufLen =
		_tmp32kOffset = _tmp16kOffset = 0;

	_tmp32k = (uint8 *) malloc(32768);
	if (!_tmp32k) {
		return;
	}

	_tmp16k = (uint8 *) malloc(16384);
	if (!_tmp16k) {
		free(_tmp32k);
		return;
	}

	adt.seek(4, SEEK_CUR);

	initTmpArray(&_array1, 8, 16);
	initTmpArray(&_array2, 8, 512);
	initTmpArray(&_array3, 8, 16);

	initTmpArrayData(&_array1);
	initTmpArrayData(&_array2);
	initTmpArrayData(&_array3);

	memset(_tmp16k, 0, 16384);

	blockLength = readSrcBits(adt, 8);
	blockLength |= readSrcBits(adt, 8)<<8;
	while (blockLength>0) {
		int tmpBufLen, tmpBufLen1, curBlockLength;

		initUnpackBlock(adt);

		tmpBufLen = initUnpackBlockArray2(&_array2);
		tmpBufLen1 = initUnpackBlockArray2(&_array3);

		curBlockLength = 0;
		while (curBlockLength < blockLength) {
			int curBitfield = readSrcBitfieldArray(adt, &_array2, tmpBufLen);

			if (curBitfield < 256) {
				/* Realloc if needed */
				if (_dstOffset+1 > _dstBufLen) {
					_dstBufLen += 0x8000;
					_dstPointer = (uint8 *) realloc(_dstPointer, _dstBufLen);
				}

				_dstPointer[_dstOffset++] =
					_tmp16k[_tmp16kOffset++] = curBitfield;
				_tmp16kOffset &= 0x3fff;
			} else {
				int i;
				int numValues = curBitfield - 0xfd;
				int startOffset;
				curBitfield = readSrcBitfieldArray(adt, &_array3, tmpBufLen1);
				if (curBitfield != 0) {
					int numBits = curBitfield-1;
					curBitfield = readSrcBits(adt, numBits) & 0xffff;
					curBitfield += 1<<numBits;
				}

				/* Realloc if needed */
				if (_dstOffset+numValues > _dstBufLen) {
					_dstBufLen += 0x8000;
					_dstPointer = (uint8 *) realloc(_dstPointer, _dstBufLen);
				}

				startOffset = (_tmp16kOffset-curBitfield-1) & 0x3fff;
				for (i=0; i<numValues; i++) {
					_dstPointer[_dstOffset++] = _tmp16k[_tmp16kOffset++] =
						_tmp16k[startOffset++];
					startOffset &= 0x3fff;
					_tmp16kOffset &= 0x3fff;
				}
			}

			curBlockLength++;
		}

		blockLength = readSrcBits(adt, 8);
		blockLength |= readSrcBits(adt, 8)<<8;
	}

	free(_tmp16k);
	free(_tmp32k);

	if (!_dstPointer) {
		_dstBufLen = _dstOffset;
		_dstPointer = (uint8 *) realloc(_dstPointer, _dstBufLen);
	}
}

void AdtDecoder::initTmpArray(unpackArray_t *array, int start, int length)
{
	array->start = start;

	array->length = length;

	array->tree = (node_t *) &_tmp32k[_tmp32kOffset];
	_tmp32kOffset += length * 2 * sizeof(node_t);

	array->ptr8 = (unpackArray8_t *) &_tmp32k[_tmp32kOffset];
	_tmp32kOffset += length * sizeof(unpackArray8_t);

	array->ptr4 = (uint32 *) &_tmp32k[_tmp32kOffset];
	_tmp32kOffset += length * sizeof(uint32);
}

void AdtDecoder::initTmpArrayData(unpackArray_t *array)
{
	uint32 i;

	for (i=0; i<array->length; i++) {
		array->ptr4[i] =
		array->ptr8[i].start =
		array->ptr8[i].length =
		array->tree[i].nodes[NODE_LEFT] =
		array->tree[i].nodes[NODE_RIGHT] = -1;
	}

	while (i < array->length<<1) {
		array->tree[i].nodes[NODE_LEFT] =
		array->tree[i].nodes[NODE_RIGHT] = -1;
		i++;
	}
}

int AdtDecoder::readSrcBits(Common::SeekableReadStream &adt, int numBits)
{
	int orMask = 0, andMask;
	int finalValue = _srcByte;

	while (numBits > _srcNumBit) {
		numBits -= _srcNumBit;
		andMask = (1<<_srcNumBit)-1;
		andMask &= finalValue;
		andMask <<= numBits;

		_srcByte = adt.readByte();

		finalValue = _srcByte;
		_srcNumBit = 8;
		orMask |= andMask;
	}

	_srcNumBit -= numBits;
	finalValue >>= _srcNumBit;
	finalValue = (finalValue & ((1<<numBits)-1)) | orMask;

	return finalValue;
}

int AdtDecoder::readSrcOneBit(Common::SeekableReadStream &adt)
{
	_srcNumBit--;
	if (_srcNumBit<0) {
		_srcNumBit = 7;

		_srcByte = adt.readByte();
	}

	return (_srcByte>> _srcNumBit) & 1;
}

int AdtDecoder::readSrcBitfieldArray(Common::SeekableReadStream &adt, unpackArray_t *array, uint32 curIndex)
{
	do {
		if (readSrcOneBit(adt)) {
			curIndex = array->tree[curIndex].nodes[NODE_RIGHT];
		} else {
			curIndex = array->tree[curIndex].nodes[NODE_LEFT];
		}
	} while (curIndex >= array->length);

	return curIndex;
}

int AdtDecoder::readSrcBitfield(Common::SeekableReadStream &adt)
{
	int numZeroBits = 0;
	int bitfieldValue = 1;

	while (readSrcOneBit(adt)==0) {
		numZeroBits++;
	}

	while (numZeroBits>0) {
		bitfieldValue = readSrcOneBit(adt) + (bitfieldValue<<1);
		numZeroBits--;
	}

	return bitfieldValue;
}

void AdtDecoder::initUnpackBlock(Common::SeekableReadStream &adt)
{
	uint32 i, j, curBitfield;
	int prevValue, curBit;
	int numValues;
	unsigned short tmp[512];
	unsigned long tmpBufLen;

	/* Initialize array 1 to unpack block */

	prevValue = 0;
	for (i=0; i<_array1.length; i++) {
		if (readSrcOneBit(adt)) {
			_array1.ptr8[i].length = readSrcBitfield(adt) ^ prevValue;
		} else {
			_array1.ptr8[i].length = prevValue;
		}
		prevValue = _array1.ptr8[i].length;
	}

	/* Count frequency of values in array 1 */
	memset(_freqArray, 0, sizeof(_freqArray));

	for (i=0; i<_array1.length; i++) {
		numValues = _array1.ptr8[i].length;
		if (numValues <= 16) {
			_freqArray[numValues]++;
		}
	}

	initUnpackBlockArray(&_array1);
	tmpBufLen = initUnpackBlockArray2(&_array1);

	/* Initialize array 2 to unpack block */

	if (_array2.length>0) {
		memset(tmp, 0, _array2.length);
	}

	curBit = readSrcOneBit(adt);
	j = 0;
	while (j < _array2.length) {
		if (curBit) {
			curBitfield = readSrcBitfield(adt);
			for (i=0; i<curBitfield; i++) {
				tmp[j+i] = readSrcBitfieldArray(adt, &_array1, tmpBufLen);
			}
			j += curBitfield;
			curBit = 0;
			continue;
		}

		curBitfield = readSrcBitfield(adt);
		if (curBitfield>0) {
			memset(&tmp[j], 0, curBitfield*sizeof(unsigned short));
			j += curBitfield;
		}
		curBit = 1;
	}

	j = 0;
	for (i=0; i<_array2.length; i++) {
		j = j ^ tmp[i];
		_array2.ptr8[i].length = j;
	}

	/* Count frequency of values in array 2 */
	memset(_freqArray, 0, sizeof(_freqArray));

	for (i=0; i<_array2.length; i++) {
		numValues = _array2.ptr8[i].length;
		if (numValues <= 16) {
			_freqArray[numValues]++;
		}
	}

	initUnpackBlockArray(&_array2);

	/* Initialize array 3 to unpack block */

	prevValue = 0;
	for (i=0; i<_array3.length; i++) {
		if (readSrcOneBit(adt)) {
			_array3.ptr8[i].length = readSrcBitfield(adt) ^ prevValue;
		} else {
			_array3.ptr8[i].length = prevValue;
		}
		prevValue = _array3.ptr8[i].length;
	}

	/* Count frequency of values in array 3 */
	memset(_freqArray, 0, sizeof(_freqArray));

	for (i=0; i<_array3.length; i++) {
		numValues = _array3.ptr8[i].length;
		if (numValues <= 16) {
			_freqArray[numValues]++;
		}
	}

	initUnpackBlockArray(&_array3);
}

void AdtDecoder::initUnpackBlockArray(unpackArray_t *array)
{
	uint16 tmp[18];
	uint32 i, j;

	memset(tmp, 0, sizeof(tmp));

	for (i=0; i<16; i++) {
		tmp[i+2] = (tmp[i+1] + _freqArray[i+1])<<1;
	}

	for (i=0;i<18;i++) {
		/*int startTmp = tmp[i];*/
		for (j=0; j<array->length; j++) {
			if (array->ptr8[j].length == i) {
				array->ptr8[j].start = tmp[i]++ & 0xffff;
			}
		}
	}
}

int AdtDecoder::initUnpackBlockArray2(unpackArray_t *array)
{
	uint32 i, j;
	int curLength = array->length;
	int curArrayIndex = curLength + 1;

	array->tree[curLength].nodes[NODE_LEFT] =
	array->tree[curLength].nodes[NODE_RIGHT] =
	array->tree[curArrayIndex].nodes[NODE_LEFT] =
	array->tree[curArrayIndex].nodes[NODE_RIGHT] = -1;

	for (i=0; i<array->length; i++) {
		int curPtr8Start = array->ptr8[i].start;
		uint32 curPtr8Length = array->ptr8[i].length;

		curLength = array->length;

		for (j=0; j<curPtr8Length; j++) {
			int curMask = 1<<(curPtr8Length-j-1);
			int arrayOffset;

			if ((curMask & curPtr8Start)!=0) {
				arrayOffset = NODE_RIGHT;
			} else {
				arrayOffset = NODE_LEFT;
			}

			if (j+1 == curPtr8Length) {
				array->tree[curLength].nodes[arrayOffset] = i;
				break;
			}

			if (array->tree[curLength].nodes[arrayOffset] == -1) {
				array->tree[curLength].nodes[arrayOffset] = curArrayIndex;
				array->tree[curArrayIndex].nodes[NODE_LEFT] =
				array->tree[curArrayIndex].nodes[NODE_RIGHT] = -1;
				curLength = curArrayIndex++;
			} else {
				curLength = array->tree[curLength].nodes[arrayOffset];
			}
		}
	}

	return array->length;
}

} // End of namespace Reevengi
