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

#include "engines/reevengi/hd/texture.h"
#include "common/endian.h"

namespace Reevengi {
namespace HD {

static void decodeRGB565(uint16 packed, byte *color) {
	const byte r = (packed >> 11) & 31;
	const byte g = (packed >> 5) & 63;
	const byte b = packed & 31;
	color[0] = (r << 3) | (r >> 2);
	color[1] = (g << 2) | (g >> 4);
	color[2] = (b << 3) | (b >> 2);
	color[3] = 255;
}

bool decodeTexture(const Common::Array<byte> &data, Graphics::Surface &surface, Common::String &error) {
	surface.free();
	error.clear();
	if (data.size() < 20 || READ_BE_UINT32(data.data()) != MKTAG('T', 'E', 'X', 0) || data[4] != 157) {
		error = "Expected a TEX v157 header";
		return false;
	}
	const uint32 dimensions = READ_LE_UINT32(data.data() + 8);
	const uint mips = dimensions & 63;
	const uint width = (dimensions >> 6) & 8191;
	const uint height = dimensions >> 19;
	const uint32 layout = READ_LE_UINT32(data.data() + 12);
	const uint images = layout & 255;
	const uint format = (layout >> 8) & 255;
	// 31 is the original BC3 normal-map storage variant (alpha/green XY).
	const bool bc3 = format == 24 || format == 31 || format == 43;
	const uint depth = (layout >> 16) & 8191;
	if ((data[7] >> 4) != 2 || images != 1 || depth != 1 ||
		(format != 19 && format != 20 && !bc3) || !mips || mips > 14 ||
		!width || !height || uint64(width) * height > 16 * 1024 * 1024) {
		error = "Unsupported TEX layout: expected a bounded single-image 2D BC1/BC3 color texture";
		return false;
	}
	const uint tableEnd = 16 + mips * 4;
	if (tableEnd > data.size()) {
		error = "Truncated TEX mip table";
		return false;
	}
	const uint32 offset = READ_LE_UINT32(data.data() + 16);
	const uint blocksX = (width + 3) / 4;
	const uint blocksY = (height + 3) / 4;
	const uint blockSize = bc3 ? 16 : 8;
	const uint64 end = uint64(offset) + uint64(blocksX) * blocksY * blockSize;
	if (offset < tableEnd || end > data.size() ||
		(mips > 1 && READ_LE_UINT32(data.data() + 20) < end)) {
		error = "TEX mip zero overlaps its header or is truncated";
		return false;
	}
	surface.create(width, height, Graphics::PixelFormat::createFormatRGBA32());
	const byte *block = data.data() + offset;
	for (uint by = 0; by < blocksY; ++by) {
		for (uint bx = 0; bx < blocksX; ++bx, block += blockSize) {
			const byte *colorBlock = block + (bc3 ? 8 : 0);
			const uint16 c0 = READ_LE_UINT16(colorBlock);
			const uint16 c1 = READ_LE_UINT16(colorBlock + 2);
			byte colors[4][4];
			decodeRGB565(c0, colors[0]);
			decodeRGB565(c1, colors[1]);
			if (c0 > c1 || bc3) {
				for (uint channel = 0; channel < 3; ++channel) {
					colors[2][channel] = (2 * colors[0][channel] + colors[1][channel]) / 3;
					colors[3][channel] = (colors[0][channel] + 2 * colors[1][channel]) / 3;
				}
				colors[2][3] = colors[3][3] = 255;
			} else {
				for (uint channel = 0; channel < 3; ++channel)
					colors[2][channel] = (colors[0][channel] + colors[1][channel]) / 2;
				colors[2][3] = 255;
				memset(colors[3], 0, sizeof(colors[3]));
			}
			byte alphas[8] = {};
			uint64 alphaIndices = 0;
			if (bc3) {
				alphas[0] = block[0];
				alphas[1] = block[1];
				if (alphas[0] > alphas[1]) {
					for (uint a = 2; a < 8; ++a)
						alphas[a] = ((8 - a) * alphas[0] + (a - 1) * alphas[1]) / 7;
				} else {
					for (uint a = 2; a < 6; ++a)
						alphas[a] = ((6 - a) * alphas[0] + (a - 1) * alphas[1]) / 5;
					alphas[6] = 0;
					alphas[7] = 255;
				}
				for (uint a = 0; a < 6; ++a)
					alphaIndices |= uint64(block[a + 2]) << (a * 8);
			}
			uint32 indices = READ_LE_UINT32(colorBlock + 4);
			for (uint y = 0; y < 4; ++y) {
				for (uint x = 0; x < 4; ++x, indices >>= 2, alphaIndices >>= 3) {
					if (bx * 4 + x >= width || by * 4 + y >= height)
						continue;
					const byte *rgba = colors[indices & 3];
					const uint32 pixel = surface.format.ARGBToColor(bc3 ? alphas[alphaIndices & 7] : rgba[3], rgba[0], rgba[1], rgba[2]);
					WRITE_UINT32(surface.getBasePtr(bx * 4 + x, by * 4 + y), pixel);
				}
			}
		}
	}
	return true;
}

bool decodeDepthTexture(const Common::Array<byte> &data, DepthTexture &depth, Common::String &error) {
	depth = DepthTexture();
	error = "Unsupported or malformed float depth texture";
	if (data.size() < 20 || READ_BE_UINT32(data.data()) != MKTAG('T', 'E', 'X', 0) ||
		data[4] != 157 || (data[7] >> 4) != 2)
		return false;
	const uint32 dimensions = READ_LE_UINT32(data.data() + 8);
	const uint width = (dimensions >> 6) & 8191;
	const uint height = dimensions >> 19;
	const uint32 layout = READ_LE_UINT32(data.data() + 12);
	const uint32 offset = READ_LE_UINT32(data.data() + 16);
	const uint64 pixels = uint64(width) * height;
	if ((dimensions & 63) != 1 || (layout & 255) != 1 || ((layout >> 8) & 255) != 14 ||
		((layout >> 16) & 8191) != 1 || !width || !height || pixels > 16 * 1024 * 1024 ||
		offset < 20 || uint64(offset) + pixels * 4 != data.size())
		return false;
	Common::Array<float> values;
	values.resize(pixels);
	uint uncovered = 0, beyondFar = 0;
	for (uint i = 0; i < pixels; ++i) {
		// Some room depth maps use negative infinity for uncovered pixels.
		// Treat this exact sentinel as uncovered; reject other invalid float data.
		if (READ_LE_UINT32(data.data() + offset + i * 4) == 0xff800000U) {
			values[i] = 0;
			++uncovered;
			continue;
		}
		values[i] = READ_LE_FLOAT32(data.data() + offset + i * 4);
		// R32F is not normalized. Retain finite samples beyond the far plane.
		const uint32 bits = READ_LE_UINT32(data.data() + offset + i * 4);
		if (!(values[i] >= 0) || (bits & 0x7f800000U) == 0x7f800000U)
			return false;
		if (values[i] > 1)
			++beyondFar;
	}
	depth.width = width;
	depth.height = height;
	depth.uncoveredSamples = uncovered;
	depth.beyondFarSamples = beyondFar;
	depth.values = values;
	error.clear();
	return true;
}

} // namespace HD
} // namespace Reevengi
