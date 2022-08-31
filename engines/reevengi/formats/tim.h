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

#ifndef REEVENGI_TIM_H
#define REEVENGI_TIM_H

#include "graphics/surface.h"
#include "image/image_decoder.h"

namespace Common {
class SeekableReadStream;
}

namespace Reevengi {

class TimDecoder : public Image::ImageDecoder {
public:
	TimDecoder();
	virtual ~TimDecoder();

	void setSize(int w, int h);

	void CreateTimSurface(int w, int h, Graphics::PixelFormat &fmt);
	uint16 *getTimPalette(void) const { return _timPalette; };

	// ImageDecoder API
	virtual void destroy();
	virtual bool loadStream(Common::SeekableReadStream &tim);
	virtual const Graphics::Surface *getSurface() const { return &_surface; }
	virtual const byte *getPalette() const { return _colorMap; }
	virtual const byte *getPalette(int numColorMap) const { return &_colorMap[_colorMapLength * numColorMap]; }
	virtual uint16 getPaletteColorCount() const { return _colorMapLength; }

protected:
	uint16 readPixel(Common::SeekableReadStream &tim);
	uint16 readPixel(uint16 color);

private:
	int _forcedW, _forcedH;
	uint16 *_timPalette;	/* Original palette, needed for masking using alpha component */

	// Color-map:
	byte *_colorMap;
	int16 _colorMapCount;	/* Number of color maps */
	int16 _colorMapLength;	/* Number of colors per color map */

	Graphics::PixelFormat _format;
	Graphics::Surface _surface;

	// Loading helpers
	bool readHeader(Common::SeekableReadStream &tim, byte &imageType);
	bool readData(Common::SeekableReadStream &tim, byte imageType);
	bool readColorMap(Common::SeekableReadStream &tim, byte imageType);
};

} // End of namespace Reevengi

#endif
