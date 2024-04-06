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
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/reevengi/formats/tim.h"

namespace Reevengi {

/*--- Defines ---*/

#define TIM_MAGIC	0x10

#define TIM_TYPE_WITHPAL	(1<<3)
#define TIM_TYPE_BPP4		(0 | TIM_TYPE_WITHPAL)
#define TIM_TYPE_BPP8		(1 | TIM_TYPE_WITHPAL)
#define TIM_TYPE_BPP15		2
#define TIM_TYPE_BPP24		3

/*--- Types ---*/

typedef struct {
	uint32	magic;
	uint32	type;
} tim_header_t;

typedef struct {
	uint32	length;
	uint32	dummy;
	uint16	palette_colors;
	uint16	nb_palettes;
} tim_header_pal_t;

typedef struct {
	uint32	length;
	uint32	dummy;
	uint16	width;
	uint16	height;
} tim_header_pix_t;

/*--- Class ---*/

TimDecoder::TimDecoder(): _colorMapCount(0), _colorMapLength(0), _colorMap(nullptr),
	_forcedW(0), _forcedH(0), _timPalette(nullptr) {
}

TimDecoder::~TimDecoder() {
	destroy();
}

void TimDecoder::destroy() {
	_surface.free();
	delete[] _colorMap;
	_colorMap = nullptr;
	delete[] _timPalette;
	_timPalette = nullptr;
}

void TimDecoder::setSize(int w, int h) {
	_forcedW = w;
	_forcedH = h;
}

void TimDecoder::CreateTimSurface(int w, int h, Graphics::PixelFormat &fmt) {
	_surface.create(w, h, fmt);
}

bool TimDecoder::loadStream(Common::SeekableReadStream &tim) {
	byte imageType;
	bool success;

	TimDecoder::destroy();

	success = readHeader(tim, imageType);
	if (success) {
		success = readData(tim, imageType);
	}
	if (tim.err() || !success) {
		//warning("Failed reading TIM-file");
		return false;
	}
	return success;
}

bool TimDecoder::readHeader(Common::SeekableReadStream &tim, byte &imageType) {
	tim_header_t hdr;
	tim_header_pal_t pal_hdr;
	uint32 str_pos;
	bool success;

	if (!tim.seek(0)) {
		//warning("Failed reading TIM-file");
		return false;
	}

	hdr.magic = tim.readUint32LE();
	hdr.type = tim.readUint32LE();

	if (hdr.magic != TIM_MAGIC) {
		return false;
	}
	imageType = hdr.type;

	if (imageType==TIM_TYPE_BPP24) {
		_format = Graphics::PixelFormat(3, 8, 8, 8, 0, 16, 8, 0, 0);
		return true;
	}
	if (imageType==TIM_TYPE_BPP15) {
		_format = Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0);
		return true;
	}

	/* Read color maps */
	_format = Graphics::PixelFormat::createFormatCLUT8();

	str_pos = tim.pos();

	pal_hdr.length = tim.readUint32LE();
	pal_hdr.dummy = tim.readUint32LE();
	pal_hdr.palette_colors = tim.readUint16LE();
	pal_hdr.nb_palettes = tim.readUint16LE();

	_colorMapCount = pal_hdr.nb_palettes;
	_colorMapLength = pal_hdr.palette_colors;

	success = readColorMap(tim, imageType);
	if (success) {
		tim.seek(str_pos + pal_hdr.length, SEEK_SET);
	}

	return success;
}

/* Convert ABGR to RGBA + set alpha correctly */
uint16 TimDecoder::readPixel(Common::SeekableReadStream &tim) {
	return readPixel(tim.readUint16LE());
}

uint16 TimDecoder::readPixel(uint16 color) {
	int r1,g1,b1,a1;

	a1 = (color>>15) & 1;
	b1 = (color>>10) & 31;
	g1 = (color>>5) & 31;
	r1 = color & 31;

	if (r1+g1+b1 == 0) {
		a1 = (a1 ? 1 : 0);
	} else {
		a1 = 1;
	}

	return (r1<<11)|(g1<<6)|(b1<<1)|a1;
}

bool TimDecoder::readColorMap(Common::SeekableReadStream &tim, byte imageType) {
	_colorMap = new byte[3 * _colorMapCount * _colorMapLength];
	byte *_colorMapFill = _colorMap;
	_timPalette = new uint16[256 * _colorMapCount];

	for (int j = 0; j < _colorMapCount; j++) {
		for (int i = 0; i < _colorMapLength; i++) {
			byte r, g, b, a;
			Graphics::PixelFormat format(2, 5, 5, 5, 1, 11, 6, 1, 0);

			uint16 color = readPixel(tim);
			_timPalette[j*256+i] = color;
			format.colorToARGB(color, a, r, g, b);

#ifdef SCUMM_LITTLE_ENDIAN
			*_colorMapFill++ = r;
			*_colorMapFill++ = g;
			*_colorMapFill++ = b;
#else
			*_colorMapFill++ = b;
			*_colorMapFill++ = g;
			*_colorMapFill++ = r;
#endif
		}
	}
	return true;
}

bool TimDecoder::readData(Common::SeekableReadStream &tim, byte imageType) {
	tim_header_pix_t pix_hdr;

	pix_hdr.length = tim.readUint32LE();
	pix_hdr.dummy = tim.readUint32LE();
	pix_hdr.width = tim.readUint16LE();
	pix_hdr.height = tim.readUint16LE();

	switch (imageType) {
		case TIM_TYPE_BPP4:
			_surface.w = pix_hdr.width << 2;
			_surface.format.bytesPerPixel = 1;
			break;
		case TIM_TYPE_BPP8:
			_surface.w = pix_hdr.width << 1;
			_surface.format.bytesPerPixel = 1;
			break;
		case TIM_TYPE_BPP15:
			_surface.w = pix_hdr.width;
			_surface.format.bytesPerPixel = 2;
			break;
		//case TIM_TYPE_BPP24:	// TODO
		default:
			return false;
	}
	_surface.h = pix_hdr.height;

	if (_forcedW) { _surface.w = _forcedW; }
	if (_forcedH) { _surface.h = _forcedH; }

	_surface.create(_surface.w, _surface.h, _format);

	switch (imageType) {
		case TIM_TYPE_BPP4:
			{
				for(int y=0; y<_surface.h; y++) {
					uint8 *dst = (uint8 *)_surface.getBasePtr(0, y);

					for (int x = 0; x < _surface.w; x++) {
						uint8 src = tim.readByte();

						*dst++ = src & 15;
						*dst++ = (src>>4) & 15;
					}
				}
			}
			break;
		case TIM_TYPE_BPP8:
			{
				for(int y=0; y<_surface.h; y++) {
					uint8 *dst = (uint8 *) _surface.getBasePtr(0, y);

					tim.read(dst, _surface.w);
				}
			}
			break;
		case TIM_TYPE_BPP15:
			{
				for(int y=0; y<_surface.h; y++) {
					uint16 *dst = (uint16 *)_surface.getBasePtr(0, y);

					for (int x = 0; x < _surface.w; x++) {
						*dst++ = readPixel(tim);
					}
				}
			}
			break;
		default:
			break;
	}

	return true;
}

} // End of namespace Reevengi
