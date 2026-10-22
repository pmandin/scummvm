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

#include <cxxtest/TestSuite.h>

#include "common/endian.h"
#include "common/util.h"
#include "engines/reevengi/hd/camera.h"

class HDCameraReaderTestSuite : public CxxTest::TestSuite {
private:
	enum {
		kRootClass = 32,
		kCameraClass = 88,
		kNamePool = 192,
		kPropertyBytes = 24,
		kRootHeaderBytes = 17,
		kCameraBytes = 64
	};

	Common::Array<byte> fixture(uint count = 1) {
		// Invented values serialized with the observed two-class XFS schema.
		Common::Array<byte> data;
		data.resize(kNamePool);
		memset(data.data(), 0, data.size());
		memcpy(data.data(), "XFS", 3);
		WRITE_LE_UINT16(data.data() + 4, 15);
		WRITE_LE_UINT16(data.data() + 6, 1);
		WRITE_LE_UINT32(data.data() + 8, count + 1);
		WRITE_LE_UINT32(data.data() + 16, 2);
		WRITE_LE_UINT32(data.data() + 24, kRootClass - 24);
		WRITE_LE_UINT32(data.data() + 28, kCameraClass - 24);
		WRITE_LE_UINT32(data.data() + kRootClass + 4, 2);
		WRITE_LE_UINT32(data.data() + kCameraClass + 4, 4);
		const char *const names[] = {"mAutoDelete", "mpArray", "mCameraPos", "mTargetPos", "mRoll", "mFovy"};
		const uint16 types[] = {3, 0xa002, 20, 20, 12, 12};
		const uint16 sizes[] = {1, 4, 16, 16, 4, 4};
		for (uint i = 0; i < ARRAYSIZE(names); ++i) {
			const uint field = i < 2 ? kRootClass + 8 + i * kPropertyBytes : kCameraClass + 8 + (i - 2) * kPropertyBytes;
			const uint offset = data.size();
			data.resize(offset + strlen(names[i]) + 1);
			memcpy(data.data() + offset, names[i], strlen(names[i]) + 1);
			WRITE_LE_UINT32(data.data() + field, offset - 24);
			WRITE_LE_UINT16(data.data() + field + 4, types[i]);
			WRITE_LE_UINT16(data.data() + field + 6, sizes[i]);
		}
		const uint root = data.size();
		data.resize(root + kRootHeaderBytes + count * kCameraBytes);
		memset(data.data() + root, 0, data.size() - root);
		WRITE_LE_UINT32(data.data() + 20, root - 24);
		WRITE_LE_UINT16(data.data() + root, 1);
		WRITE_LE_UINT32(data.data() + root + 4, data.size() - root - 4);
		WRITE_LE_UINT32(data.data() + root + 8, 1);
		data[root + 12] = 1;
		WRITE_LE_UINT32(data.data() + root + 13, count);
		for (uint camera = 0; camera < count; ++camera) {
			const uint object = root + kRootHeaderBytes + camera * kCameraBytes;
			WRITE_LE_UINT16(data.data() + object, 3);
			WRITE_LE_UINT16(data.data() + object + 2, camera + 1);
			WRITE_LE_UINT32(data.data() + object + 4, 60);
			const uint widths[] = {4, 4, 1, 1};
			const float values[] = {float(camera + 1), 2, 3, 0, 4, 5, 6, 0, float(camera), 55};
			uint value = 0;
			uint pos = object + 8;
			for (uint field = 0; field < ARRAYSIZE(widths); ++field) {
				WRITE_LE_UINT32(data.data() + pos, 1);
				pos += 4;
				for (uint i = 0; i < widths[field]; ++i, pos += 4)
					WRITE_LE_FLOAT32(data.data() + pos, values[value++]);
			}
		}
		return data;
	}

	uint dataStart(const Common::Array<byte> &data) {
		return 24 + READ_LE_UINT32(data.data() + 20);
	}

	uint cameraStart(const Common::Array<byte> &data, uint index = 0) {
		return dataStart(data) + kRootHeaderBytes + index * kCameraBytes;
	}

	void assertRejected(const Common::Array<byte> &data) {
		Common::Array<Reevengi::HD::RoomCamera> cameras;
		cameras.push_back(Reevengi::HD::RoomCamera());
		Common::String error;
		TS_ASSERT(!Reevengi::HD::decodeCameras(data, cameras, error));
		TS_ASSERT(cameras.empty());
		TS_ASSERT(!error.empty());
	}

public:
	void testCameraValuesAndErrorReset() {
		Common::Array<byte> data = fixture();
		const uint object = cameraStart(data);
		// Preserve the fourth components rather than interpreting them here.
		WRITE_LE_FLOAT32(data.data() + object + 24, 1);
		WRITE_LE_FLOAT32(data.data() + object + 44, 2);
		WRITE_LE_FLOAT32(data.data() + object + 52, -7);
		Common::Array<Reevengi::HD::RoomCamera> cameras;
		Common::String error = "Previous error";
		TS_ASSERT(Reevengi::HD::decodeCameras(data, cameras, error));
		TS_ASSERT_EQUALS(cameras.size(), 1U);
		if (cameras.size() != 1)
			return;
		const float position[] = {1, 2, 3, 1};
		const float target[] = {4, 5, 6, 2};
		for (uint i = 0; i < 4; ++i) {
			TS_ASSERT_EQUALS(cameras[0].position[i], position[i]);
			TS_ASSERT_EQUALS(cameras[0].target[i], target[i]);
		}
		TS_ASSERT_EQUALS(cameras[0].roll, -7.0f);
		TS_ASSERT_EQUALS(cameras[0].fovY, 55.0f);
		TS_ASSERT(error.empty());
	}

	void testMultipleCamerasKeepSerializedOrder() {
		Common::Array<Reevengi::HD::RoomCamera> cameras;
		Common::String error;
		TS_ASSERT(Reevengi::HD::decodeCameras(fixture(3), cameras, error));
		TS_ASSERT_EQUALS(cameras.size(), 3U);
		for (uint i = 0; i < cameras.size(); ++i) {
			TS_ASSERT_EQUALS(cameras[i].position[0], float(i + 1));
			TS_ASSERT_EQUALS(cameras[i].roll, float(i));
		}
	}

	void testMaximumCameraCount() {
		Common::Array<Reevengi::HD::RoomCamera> cameras;
		Common::String error;
		TS_ASSERT(Reevengi::HD::decodeCameras(fixture(256), cameras, error));
		TS_ASSERT_EQUALS(cameras.size(), 256U);
		if (cameras.size() == 256)
			TS_ASSERT_EQUALS(cameras[255].position[0], 256.0f);
		assertRejected(fixture(0));
		assertRejected(fixture(257));
	}

	void testUnsupportedHeadersAndRelativeOffsets() {
		const uint headerWords[] = {8, 16, 20, 24, 28};
		for (uint i = 0; i < ARRAYSIZE(headerWords); ++i) {
			Common::Array<byte> data = fixture();
			WRITE_LE_UINT32(data.data() + headerWords[i], 0xfffffff0U);
			assertRejected(data);
		}
		Common::Array<byte> data = fixture();
		data.resize(31);
		assertRejected(data);
		data = fixture();
		data[0] = 'B';
		assertRejected(data);
		data = fixture();
		WRITE_LE_UINT16(data.data() + 4, 14);
		assertRejected(data);
		data = fixture();
		WRITE_LE_UINT32(data.data() + 24, 0);
		assertRejected(data);
		data = fixture();
		WRITE_LE_UINT32(data.data() + 28, dataStart(data) - 24);
		assertRejected(data);
	}

	void testClassDescriptorsMustMatchSupportedSchema() {
		Common::Array<byte> data = fixture();
		WRITE_LE_UINT32(data.data() + kRootClass + 4, 3);
		assertRejected(data);
		data = fixture();
		WRITE_LE_UINT32(data.data() + kCameraClass + 4, 3);
		assertRejected(data);
		for (uint i = 0; i < 6; ++i) {
			const uint field = i < 2 ? kRootClass + 8 + i * kPropertyBytes : kCameraClass + 8 + (i - 2) * kPropertyBytes;
			data = fixture();
			WRITE_LE_UINT32(data.data() + field, 0xfffffff0U);
			assertRejected(data);
			data = fixture();
			WRITE_LE_UINT16(data.data() + field + 4, 0);
			assertRejected(data);
			data = fixture();
			WRITE_LE_UINT16(data.data() + field + 6, 0);
			assertRejected(data);
			data = fixture();
			data[24 + READ_LE_UINT32(data.data() + field)] = 'x';
			assertRejected(data);
		}
	}

	void testRootObjectTagsLengthsAndBoolean() {
		const uint offsets[] = {0, 2, 4, 8, 12, 13};
		for (uint i = 0; i < ARRAYSIZE(offsets); ++i) {
			Common::Array<byte> data = fixture();
			const uint root = dataStart(data);
			if (offsets[i] == 0 || offsets[i] == 2)
				WRITE_LE_UINT16(data.data() + root + offsets[i], 2);
			else if (offsets[i] == 12)
				data[root + 12] = 2;
			else
				WRITE_LE_UINT32(data.data() + root + offsets[i], 2);
			assertRejected(data);
		}
	}

	void testCameraObjectTagsLengthsAndElementCounts() {
		const uint offsets[] = {0, 2, 4, 8, 28, 48, 56};
		for (uint i = 0; i < ARRAYSIZE(offsets); ++i) {
			Common::Array<byte> data = fixture();
			const uint object = cameraStart(data);
			if (offsets[i] == 0 || offsets[i] == 2)
				WRITE_LE_UINT16(data.data() + object + offsets[i], 2);
			else
				WRITE_LE_UINT32(data.data() + object + offsets[i], 2);
			assertRejected(data);
		}
	}

	void testInvalidCameraFloatsAndFieldOfView() {
		const uint offsets[] = {12, 32, 52, 60};
		const uint32 invalid[] = {0x7fc00000U, 0x7f800000U, 0xff800000U};
		for (uint offset : offsets) {
			for (uint32 bits : invalid) {
				Common::Array<byte> data = fixture();
				WRITE_LE_UINT32(data.data() + cameraStart(data) + offset, bits);
				assertRejected(data);
			}
			const float outside[] = {-1e9f, 1e9f};
			for (float value : outside) {
				Common::Array<byte> data = fixture();
				WRITE_LE_FLOAT32(data.data() + cameraStart(data) + offset, value);
				assertRejected(data);
			}
		}
		const float fieldsOfView[] = {0, -1, 180};
		for (float fov : fieldsOfView) {
			Common::Array<byte> data = fixture();
			WRITE_LE_FLOAT32(data.data() + cameraStart(data) + 60, fov);
			assertRejected(data);
		}
	}

	void testCompleteFileLengthIsRequired() {
		Common::Array<byte> data = fixture();
		data.pop_back();
		assertRejected(data);
		data = fixture();
		data.push_back(0);
		assertRejected(data);
	}

	void testLateFailureDoesNotPublishPartialCameraArray() {
		Common::Array<byte> data = fixture(2);
		WRITE_LE_FLOAT32(data.data() + cameraStart(data, 1) + 60, 180);
		assertRejected(data);
	}
};

class HDCutScreenReaderTestSuite : public CxxTest::TestSuite {
private:
	Common::Array<byte> fixture() {
		Common::Array<byte> data;
		data.resize(44);
		memset(data.data(), 0, data.size());
		WRITE_LE_UINT32(data.data(), 1);
		WRITE_LE_UINT32(data.data() + 24, 3);
		WRITE_LE_UINT32(data.data() + 28, 1);
		WRITE_LE_FLOAT32(data.data() + 32, 110);
		WRITE_LE_FLOAT32(data.data() + 36, 0.05f);
		WRITE_LE_FLOAT32(data.data() + 40, 0.2f);
		return data;
	}

	void assertRejected(const Common::Array<byte> &data) {
		Reevengi::HD::CutScreenParams params;
		params.layers = 7;
		params.movie = params.depth = params.shadow = params.mirrorModel = params.mirrorMask = true;
		params.playerScroll = false;
		params.offsetY = params.scrollRange = params.rangeOffset = -7;
		Common::String error;
		TS_ASSERT(!Reevengi::HD::decodeCutScreenParams(data, params, error));
		TS_ASSERT(!error.empty());
		TS_ASSERT_EQUALS(params.layers, 1U);
		TS_ASSERT(!params.movie && !params.depth && !params.shadow && !params.mirrorModel && !params.mirrorMask);
		TS_ASSERT(params.playerScroll);
		TS_ASSERT_EQUALS(params.offsetY, 90.0f);
		TS_ASSERT_EQUALS(params.scrollRange, 0.3f);
		TS_ASSERT_EQUALS(params.rangeOffset, 0.0f);
	}

public:
	void testFramingFieldsAndDisabledScroll() {
		Common::Array<byte> data = fixture();
		Reevengi::HD::CutScreenParams params;
		Common::String error = "Previous error";
		TS_ASSERT(Reevengi::HD::decodeCutScreenParams(data, params, error));
		TS_ASSERT(error.empty());
		TS_ASSERT_EQUALS(params.layers, 3U);
		TS_ASSERT(params.playerScroll);
		TS_ASSERT_EQUALS(params.offsetY, 110.0f);
		TS_ASSERT_EQUALS(params.scrollRange, 0.05f);
		TS_ASSERT_EQUALS(params.rangeOffset, 0.2f);
		WRITE_LE_UINT32(data.data() + 28, 0);
		TS_ASSERT(Reevengi::HD::decodeCutScreenParams(data, params, error));
		TS_ASSERT(!params.playerScroll);
	}

	void testScreenEffectFlagsAreRetainedIndividually() {
		using Reevengi::HD::CutScreenParams;
		bool CutScreenParams::*const flags[] = {&CutScreenParams::movie, &CutScreenParams::depth, &CutScreenParams::shadow, &CutScreenParams::mirrorModel, &CutScreenParams::mirrorMask};
		for (uint selected = 0; selected < ARRAYSIZE(flags); ++selected) {
			Common::Array<byte> data = fixture();
			WRITE_LE_UINT32(data.data() + 4 + selected * 4, 1);
			CutScreenParams params;
			Common::String error;
			TS_ASSERT(Reevengi::HD::decodeCutScreenParams(data, params, error));
			for (uint i = 0; i < ARRAYSIZE(flags); ++i)
				TS_ASSERT_EQUALS(params.*flags[i], i == selected);
		}
	}

	void testLayerCountsAndEffectCombinationsDoNotImposeRendererPolicy() {
		const uint32 layers[] = {0, 30, 0xffffffffU};
		for (uint32 count : layers) {
			Common::Array<byte> data = fixture();
			for (uint offset = 4; offset <= 20; offset += 4)
				WRITE_LE_UINT32(data.data() + offset, 1);
			WRITE_LE_UINT32(data.data() + 24, count);
			Reevengi::HD::CutScreenParams params;
			Common::String error;
			TS_ASSERT(Reevengi::HD::decodeCutScreenParams(data, params, error));
			TS_ASSERT_EQUALS(params.layers, count);
			TS_ASSERT(params.movie && params.depth && params.shadow && params.mirrorModel && params.mirrorMask);
		}
	}

	void testInvalidRecordSizeVersionAndBooleans() {
		Common::Array<byte> data = fixture();
		data.pop_back();
		assertRejected(data);
		data = fixture();
		data.push_back(0);
		assertRejected(data);
		data = fixture();
		WRITE_LE_UINT32(data.data(), 2);
		assertRejected(data);
		const uint booleanOffsets[] = {4, 8, 12, 16, 20, 28};
		for (uint offset : booleanOffsets) {
			data = fixture();
			WRITE_LE_UINT32(data.data() + offset, 2);
			assertRejected(data);
		}
	}

	void testNonFiniteAndOutOfRangeFramingValues() {
		const uint offsets[] = {32, 36, 40};
		const uint32 invalid[] = {0x7fc00000U, 0x7f800000U, 0xff800000U};
		for (uint offset : offsets) {
			for (uint32 bits : invalid) {
				Common::Array<byte> data = fixture();
				WRITE_LE_UINT32(data.data() + offset, bits);
				assertRejected(data);
			}
		}
		const struct {
			uint offset;
			float value;
		} outside[] = {{32, -10001}, {32, 10001}, {36, -0.01f}, {36, 1}, {40, -10.1f}, {40, 10.1f}};
		for (const auto &sample : outside) {
			Common::Array<byte> data = fixture();
			WRITE_LE_FLOAT32(data.data() + sample.offset, sample.value);
			assertRejected(data);
		}
	}

	void testFramingAcceptanceBoundaries() {
		const struct {
			float offset, range, bias;
		} samples[] = {{-10000, 0, -10}, {10000, 0.999f, 10}};
		for (const auto &sample : samples) {
			Common::Array<byte> data = fixture();
			WRITE_LE_FLOAT32(data.data() + 32, sample.offset);
			WRITE_LE_FLOAT32(data.data() + 36, sample.range);
			WRITE_LE_FLOAT32(data.data() + 40, sample.bias);
			Reevengi::HD::CutScreenParams params;
			Common::String error;
			TS_ASSERT(Reevengi::HD::decodeCutScreenParams(data, params, error));
			TS_ASSERT_EQUALS(params.offsetY, sample.offset);
			TS_ASSERT_EQUALS(params.scrollRange, sample.range);
			TS_ASSERT_EQUALS(params.rangeOffset, sample.bias);
		}
	}
};
