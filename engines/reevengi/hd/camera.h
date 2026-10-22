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

#ifndef REEVENGI_HD_CAMERA_H
#define REEVENGI_HD_CAMERA_H

#include "common/array.h"
#include "common/str.h"

namespace Reevengi {
namespace HD {

struct RoomCamera {
	float position[4] = {};
	float target[4] = {};
	float roll = 0;
	float fovY = 0;
};

// Limited to the observed XFS v15 camera-array schema, with 1..256 cameras.
// Vectors retain their fourth component; this reader does not build a projection.
// Failure clears the previous camera array and supplies an error message.
bool decodeCameras(const Common::Array<byte> &data, Common::Array<RoomCamera> &cameras, Common::String &error);

// Serialized version-1 rCutScrParam metadata, independent of render support.
struct CutScreenParams {
	bool movie = false;
	bool depth = false;
	bool shadow = false;
	bool mirrorModel = false;
	bool mirrorMask = false;
	uint32 layers = 1;
	bool playerScroll = true;
	float offsetY = 90;
	float scrollRange = 0.3f;
	float rangeOffset = 0;
};

// Failure resets the output to its default values and supplies an error message.
bool decodeCutScreenParams(const Common::Array<byte> &data, CutScreenParams &params, Common::String &error);

} // namespace HD
} // namespace Reevengi

#endif
