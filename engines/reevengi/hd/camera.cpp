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

#include "engines/reevengi/hd/camera.h"
#include "common/endian.h"
#include "common/memstream.h"

namespace Reevengi {
namespace HD {

static bool checkClass(const Common::Array<byte> &data, uint64 offset, uint64 dataStart,
					   const char *const *names, const uint16 *types, uint count) {
	if (offset < 32 || offset + 8 + count * 24 > dataStart ||
		READ_LE_UINT32(data.data() + offset + 4) != count)
		return false;
	for (uint i = 0; i < count; ++i) {
		const byte *field = data.data() + offset + 8 + i * 24;
		const uint64 nameOffset = uint64(READ_LE_UINT32(field)) + 24;
		const uint length = strlen(names[i]) + 1;
		const uint16 size = types[i] == 20 ? 16 : (types[i] == 3 ? 1 : 4);
		if (nameOffset + length > dataStart || READ_LE_UINT16(field + 4) != types[i] ||
			READ_LE_UINT16(field + 6) != size ||
			memcmp(data.data() + nameOffset, names[i], length))
			return false;
	}
	return true;
}

static bool readValues(Common::SeekableReadStream &stream, float *values, uint count) {
	if (stream.readUint32LE() != 1)
		return false;
	for (uint i = 0; i < count; ++i) {
		values[i] = stream.readFloatLE();
		// This also rejects NaN and infinity, without platform-specific math calls.
		if (!(values[i] >= -100000000.0f && values[i] <= 100000000.0f))
			return false;
	}
	return !stream.err() && !stream.eos();
}

bool decodeCameras(const Common::Array<byte> &data, Common::Array<RoomCamera> &cameras, Common::String &error) {
	cameras.clear();
	error = "Unsupported or malformed XFS camera array";
	if (data.size() < 32 || READ_BE_UINT32(data.data()) != MKTAG('X', 'F', 'S', 0) ||
		READ_LE_UINT16(data.data() + 4) != 15 || READ_LE_UINT32(data.data() + 16) != 2)
		return false;
	const uint64 start = uint64(READ_LE_UINT32(data.data() + 20)) + 24;
	const uint64 class0 = uint64(READ_LE_UINT32(data.data() + 24)) + 24;
	const uint64 class1 = uint64(READ_LE_UINT32(data.data() + 28)) + 24;
	if (start + 17 > data.size() || class0 >= start || class1 >= start)
		return false;
	static const char *const rootNames[] = {"mAutoDelete", "mpArray"};
	static const uint16 rootTypes[] = {3, 0xa002};
	static const char *const cameraNames[] = {"mCameraPos", "mTargetPos", "mRoll", "mFovy"};
	static const uint16 cameraTypes[] = {20, 20, 12, 12};
	if (!checkClass(data, class0, start, rootNames, rootTypes, 2) ||
		!checkClass(data, class1, start, cameraNames, cameraTypes, 4))
		return false;

	Common::MemoryReadStream stream(data.data(), data.size());
	stream.seek(start);
	if (stream.readUint16LE() != 1 || stream.readUint16LE() != 0)
		return false;
	const uint32 rootSize = stream.readUint32LE();
	if (start + 4 + rootSize != data.size() || stream.readUint32LE() != 1)
		return false;
	if (stream.readByte() > 1)
		return false;
	const uint32 count = stream.readUint32LE();
	if (!count || count > 256 || uint64(stream.pos()) + uint64(count) * 64 != data.size() ||
		READ_LE_UINT32(data.data() + 8) != count + 1)
		return false;
	Common::Array<RoomCamera> result;
	for (uint i = 0; i < count; ++i) {
		if (stream.readUint16LE() != 3 || stream.readUint16LE() != i + 1 || stream.readUint32LE() != 60)
			return false;
		RoomCamera camera;
		if (!readValues(stream, camera.position, 4) || !readValues(stream, camera.target, 4) ||
			!readValues(stream, &camera.roll, 1) || !readValues(stream, &camera.fovY, 1) ||
			!(camera.fovY > 0 && camera.fovY < 180))
			return false;
		result.push_back(camera);
	}
	cameras = result;
	error.clear();
	return true;
}

bool decodeCutScreenParams(const Common::Array<byte> &data, CutScreenParams &params, Common::String &error) {
	params = CutScreenParams();
	error = "Unsupported or malformed rCutScrParam";
	if (data.size() != 44 || READ_LE_UINT32(data.data()) != 1)
		return false;
	for (uint i = 4; i <= 20; i += 4)
		if (READ_LE_UINT32(data.data() + i) > 1)
			return false;
	const uint32 layers = READ_LE_UINT32(data.data() + 24);
	const uint32 scroll = READ_LE_UINT32(data.data() + 28);
	const float offset = READ_LE_FLOAT32(data.data() + 32), range = READ_LE_FLOAT32(data.data() + 36), bias = READ_LE_FLOAT32(data.data() + 40);
	if (scroll > 1 || !(offset >= -10000 && offset <= 10000) ||
		!(range >= 0 && range < 1) || !(bias >= -10 && bias <= 10))
		return false;
	// Preserve the resource's flags and count. Renderer limits and supported
	// effect combinations belong to the consumer, not to this metadata reader.
	params.movie = READ_LE_UINT32(data.data() + 4) != 0;
	params.depth = READ_LE_UINT32(data.data() + 8) != 0;
	params.shadow = READ_LE_UINT32(data.data() + 12) != 0;
	params.mirrorModel = READ_LE_UINT32(data.data() + 16) != 0;
	params.mirrorMask = READ_LE_UINT32(data.data() + 20) != 0;
	params.layers = layers;
	params.playerScroll = scroll != 0;
	params.offsetY = offset;
	params.scrollRange = range;
	params.rangeOffset = bias;
	error.clear();
	return true;
}

} // namespace HD
} // namespace Reevengi
