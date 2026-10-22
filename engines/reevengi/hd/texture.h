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

#ifndef REEVENGI_HD_TEXTURE_H
#define REEVENGI_HD_TEXTURE_H

#include "common/array.h"
#include "common/str.h"
#include "graphics/surface.h"

namespace Reevengi {
namespace HD {

// Decode mip zero of a single-image, 2D TEX v157 BC1 (19/20) or BC3 (24/31/43) texture.
// Channels are retained without normal reconstruction or shader color conversion.
// TEX bit fields follow Albam's tex-157.ksy. Output is RGBA32; failure frees it.
bool decodeTexture(const Common::Array<byte> &data, Graphics::Surface &surface, Common::String &error);

struct DepthTexture {
	uint width = 0;
	uint height = 0;
	uint uncoveredSamples = 0;
	uint beyondFarSamples = 0;
	Common::Array<float> values;
};

// Observed TEX v157 format 14: one float32 depth value per texel, one mip.
// Negative infinity is an uncovered sentinel. Other non-finite/negative values
// are rejected; finite values above one are retained. Failure resets the output.
bool decodeDepthTexture(const Common::Array<byte> &data, DepthTexture &depth, Common::String &error);

} // namespace HD
} // namespace Reevengi
#endif
