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

#include "common/endian.h"
#include "common/util.h"
#include "engines/reevengi/hd/texture.h"
#include <cxxtest/TestSuite.h>

class HDTextureTestSuite : public CxxTest::TestSuite {
private:
	Common::Array<byte> fixture(uint width = 4, uint height = 4) {
		Common::Array<byte> data;
		data.resize(28);
		memset(data.data(), 0, data.size());
		memcpy(data.data(), "TEX", 3);
		data[4] = 157;
		data[7] = 0x20;
		WRITE_LE_UINT32(data.data() + 8, 1 | (width << 6) | (height << 19));
		WRITE_LE_UINT32(data.data() + 12, 1 | (20 << 8) | (1 << 16));
		WRITE_LE_UINT32(data.data() + 16, 20);
		WRITE_LE_UINT16(data.data() + 20, 0xf800);
		WRITE_LE_UINT16(data.data() + 22, 0x001f);
		WRITE_LE_UINT32(data.data() + 24, 0xe4e4e4e4);
		return data;
	}
	void checkPixel(const Graphics::Surface &surface, uint x, uint y, byte a, byte r, byte g, byte b) {
		TS_ASSERT_EQUALS(READ_UINT32(surface.getBasePtr(x, y)), surface.format.ARGBToColor(a, r, g, b));
	}

public:
	void testOriginalNormalMapFormat31RetainsBC3AlphaAndGreen() {
		Common::Array<byte> data = fixture();
		data.resize(36);
		data[13] = 31;
		data[20] = 127;
		data[21] = 127;
		for (uint i = 22; i < 28; ++i)
			data[i] = 0;
		WRITE_LE_UINT16(data.data() + 28, 0x07e0);
		WRITE_LE_UINT16(data.data() + 30, 0x07e0);
		WRITE_LE_UINT32(data.data() + 32, 0);
		Graphics::Surface surface;
		Common::String error;
		TS_ASSERT(Reevengi::HD::decodeTexture(data, surface, error));
		checkPixel(surface, 0, 0, 127, 0, 255, 0);
		surface.free();
		data.pop_back();
		TS_ASSERT(!Reevengi::HD::decodeTexture(data, surface, error));
	}
	void testBC3AlphaModesAndFourColorRule() {
		for (uint mode = 0; mode < 2; ++mode) {
			Common::Array<byte> data = fixture();
			data.resize(36);
			data[13] = 24;
			data[20] = mode ? 10 : 255;
			data[21] = mode ? 110 : 0;
			uint64 indices = 0;
			for (uint i = 0; i < 16; ++i)
				indices |= uint64(i % 8) << (i * 3);
			for (uint i = 0; i < 6; ++i)
				data[22 + i] = (indices >> (i * 8)) & 255;
			// BC3 always uses four colors, even when endpoint 0 <= endpoint 1.
			WRITE_LE_UINT16(data.data() + 28, 0);
			WRITE_LE_UINT16(data.data() + 30, 0xffff);
			WRITE_LE_UINT32(data.data() + 32, 0xffffffffU);
			Graphics::Surface surface;
			Common::String error;
			TS_ASSERT(Reevengi::HD::decodeTexture(data, surface, error));
			const byte alphaHigh[] = {255, 0, 218, 182, 145, 109, 72, 36};
			const byte alphaLow[] = {10, 110, 30, 50, 70, 90, 0, 255};
			for (uint i = 0; i < 16; ++i)
				checkPixel(surface, i % 4, i / 4, mode ? alphaLow[i % 8] : alphaHigh[i % 8], 170, 170, 170);
			surface.free();
			data.pop_back();
			TS_ASSERT(!Reevengi::HD::decodeTexture(data, surface, error));
		}
	}

	void testBC1FourColorBlock() {
		const Common::Array<byte> data = fixture();
		Graphics::Surface surface;
		Common::String error;
		TS_ASSERT(Reevengi::HD::decodeTexture(data, surface, error));
		TS_ASSERT_EQUALS(surface.w, 4);
		TS_ASSERT_EQUALS(surface.h, 4);
		for (uint y = 0; y < 4; ++y) {
			checkPixel(surface, 0, y, 255, 255, 0, 0);
			checkPixel(surface, 1, y, 255, 0, 0, 255);
			checkPixel(surface, 2, y, 255, 170, 0, 85);
			checkPixel(surface, 3, y, 255, 85, 0, 170);
		}
		surface.free();
	}

	void testBC1TransparencyAndCroppedBlock() {
		Common::Array<byte> data = fixture(3, 2);
		WRITE_LE_UINT16(data.data() + 20, 0);
		WRITE_LE_UINT16(data.data() + 22, 0xffff);
		WRITE_LE_UINT32(data.data() + 24, 0xfffffffe);
		Graphics::Surface surface;
		Common::String error;
		TS_ASSERT(Reevengi::HD::decodeTexture(data, surface, error));
		TS_ASSERT_EQUALS(surface.w, 3);
		TS_ASSERT_EQUALS(surface.h, 2);
		checkPixel(surface, 0, 0, 255, 127, 127, 127);
		checkPixel(surface, 1, 0, 0, 0, 0, 0);
		checkPixel(surface, 2, 1, 0, 0, 0, 0);
		surface.free();
	}

	void testMalformedTextureHeadersAndBounds() {
		for (uint mode = 0; mode < 16; ++mode) {
			Common::Array<byte> data = fixture();
			switch (mode) {
			case 0:
				data.resize(19);
				break;
			case 1:
				data[4] = 112;
				break;
			case 2:
				data[12] = 6;
				break;
			case 3:
				WRITE_LE_UINT32(data.data() + 16, 0xfffffff0U);
				break;
			case 4:
				WRITE_LE_UINT32(data.data() + 16, 16);
				break;
			case 5:
				data.pop_back();
				break;
			case 6:
				WRITE_LE_UINT32(data.data() + 8, 1 | (8191U << 6) | (8191U << 19));
				break;
			case 7:
				data[13] = 14;
				break;
			case 8:
				data[0] = 'B';
				break;
			case 9:
				data[7] = 0x60;
				break;
			case 10:
				WRITE_LE_UINT32(data.data() + 8, 1 | (4 << 19));
				break;
			case 11:
				WRITE_LE_UINT32(data.data() + 8, 1 | (4 << 6));
				break;
			case 12:
				data[14] = 2;
				break;
			case 13:
				data[8] &= ~63;
				break;
			case 14:
				data[8] |= 15;
				break;
			case 15:
				data[8] = (data[8] & ~63) | 14;
				break;
			}
			Graphics::Surface surface;
			Common::String error;
			TS_ASSERT(!Reevengi::HD::decodeTexture(data, surface, error));
			TS_ASSERT(!error.empty());
			TS_ASSERT(!surface.getPixels());
		}
	}

	void testBlockOrderAndMipOffset() {
		Common::Array<byte> data = fixture(8, 8);
		data.resize(64);
		WRITE_LE_UINT32(data.data() + 8, 2 | (8 << 6) | (8 << 19));
		WRITE_LE_UINT32(data.data() + 16, 24);
		WRITE_LE_UINT32(data.data() + 20, 56);
		const uint16 endpoints[] = {0xf800, 0x07e0, 0x001f, 0xffff};
		for (uint block = 0; block < 4; ++block) {
			WRITE_LE_UINT16(data.data() + 24 + block * 8, endpoints[block]);
			WRITE_LE_UINT16(data.data() + 26 + block * 8, 0);
			WRITE_LE_UINT32(data.data() + 28 + block * 8, 0);
		}
		Graphics::Surface surface;
		Common::String error;
		TS_ASSERT(Reevengi::HD::decodeTexture(data, surface, error));
		checkPixel(surface, 0, 0, 255, 255, 0, 0);
		checkPixel(surface, 7, 0, 255, 0, 255, 0);
		checkPixel(surface, 0, 7, 255, 0, 0, 255);
		checkPixel(surface, 7, 7, 255, 255, 255, 255);
		surface.free();
		WRITE_LE_UINT32(data.data() + 20, 55);
		TS_ASSERT(!Reevengi::HD::decodeTexture(data, surface, error));
	}

	void testSupportedColorFormatAliases() {
		const byte formats[] = {19, 20, 24, 31, 43};
		for (uint i = 0; i < ARRAYSIZE(formats); ++i) {
			Common::Array<byte> data = fixture();
			if (i >= 2) {
				data.resize(36);
				memmove(data.data() + 28, data.data() + 20, 8);
				memset(data.data() + 20, 0, 8);
				data[20] = 255;
			}
			data[13] = formats[i];
			Graphics::Surface surface;
			Common::String error = "Previous error";
			TS_ASSERT(Reevengi::HD::decodeTexture(data, surface, error));
			TS_ASSERT(error.empty());
			checkPixel(surface, 0, 0, 255, 255, 0, 0);
			surface.free();
		}
	}

	void testFailedDecodeFreesPreviousSurface() {
		Common::Array<byte> data = fixture();
		Graphics::Surface surface;
		Common::String error;
		TS_ASSERT(Reevengi::HD::decodeTexture(data, surface, error));
		data.pop_back();
		TS_ASSERT(!Reevengi::HD::decodeTexture(data, surface, error));
		TS_ASSERT(!surface.getPixels());
		TS_ASSERT(!error.empty());
	}
};

class HDDepthTextureTestSuite : public CxxTest::TestSuite {
private:
	Common::Array<byte> fixture() {
		Common::Array<byte> data;
		data.resize(28);
		memset(data.data(), 0, data.size());
		memcpy(data.data(), "TEX", 3);
		data[4] = 157;
		data[7] = 0x20;
		WRITE_LE_UINT32(data.data() + 8, 1 | (2 << 6) | (1 << 19));
		WRITE_LE_UINT32(data.data() + 12, 1 | (14 << 8) | (1 << 16));
		WRITE_LE_UINT32(data.data() + 16, 20);
		WRITE_LE_FLOAT32(data.data() + 20, 0.9f);
		WRITE_LE_UINT32(data.data() + 24, 0xff800000U);
		return data;
	}

public:
	void testDepthSentinelAndFiniteSamples() {
		Common::Array<byte> data = fixture();
		Reevengi::HD::DepthTexture depth;
		Common::String error = "Previous error";
		TS_ASSERT(Reevengi::HD::decodeDepthTexture(data, depth, error));
		TS_ASSERT(error.empty());
		TS_ASSERT_EQUALS(depth.width, 2U);
		TS_ASSERT_EQUALS(depth.height, 1U);
		TS_ASSERT_EQUALS(depth.values.size(), 2U);
		TS_ASSERT_EQUALS(depth.uncoveredSamples, 1U);
		TS_ASSERT_EQUALS(depth.beyondFarSamples, 0U);
		TS_ASSERT_DELTA(depth.values[0], 0.9f, 0.0001f);
		TS_ASSERT_EQUALS(depth.values[1], 0.0f);
		WRITE_LE_FLOAT32(data.data() + 20, 0);
		WRITE_LE_FLOAT32(data.data() + 24, 1);
		TS_ASSERT(Reevengi::HD::decodeDepthTexture(data, depth, error));
		TS_ASSERT_EQUALS(depth.uncoveredSamples, 0U);
		TS_ASSERT_EQUALS(depth.values[0], 0.0f);
		TS_ASSERT_EQUALS(depth.values[1], 1.0f);
	}

	void testFiniteDepthBeyondFarPlaneIsRetained() {
		Common::Array<byte> data = fixture();
		Reevengi::HD::DepthTexture depth;
		Common::String error;
		const uint32 samples[] = {0x3f8020cdU, 0x40000000U, 0x7f7fffffU};
		for (uint i = 0; i < ARRAYSIZE(samples); ++i) {
			WRITE_LE_UINT32(data.data() + 20, samples[i]);
			TS_ASSERT(Reevengi::HD::decodeDepthTexture(data, depth, error));
			TS_ASSERT_EQUALS(depth.values[0], READ_LE_FLOAT32(data.data() + 20));
			TS_ASSERT_EQUALS(depth.beyondFarSamples, 1U);
			TS_ASSERT_EQUALS(depth.uncoveredSamples, 1U);
		}
	}

	void testInvalidDepthResetsPreviousOutput() {
		const uint32 samples[] = {0x7f800000U, 0x7fc00000U, 0xffc00000U, 0xbf800000U};
		for (uint i = 0; i < ARRAYSIZE(samples); ++i) {
			Common::Array<byte> data = fixture();
			Reevengi::HD::DepthTexture depth;
			Common::String error;
			TS_ASSERT(Reevengi::HD::decodeDepthTexture(data, depth, error));
			WRITE_LE_UINT32(data.data() + 24, samples[i]);
			TS_ASSERT(!Reevengi::HD::decodeDepthTexture(data, depth, error));
			TS_ASSERT(!error.empty());
			TS_ASSERT(depth.values.empty());
			TS_ASSERT_EQUALS(depth.width, 0U);
			TS_ASSERT_EQUALS(depth.height, 0U);
			TS_ASSERT_EQUALS(depth.beyondFarSamples, 0U);
			TS_ASSERT_EQUALS(depth.uncoveredSamples, 0U);
		}
	}

	void testMalformedDepthHeadersAndBounds() {
		for (uint mode = 0; mode < 14; ++mode) {
			Common::Array<byte> data = fixture();
			switch (mode) {
			case 0:
				data.resize(19);
				break;
			case 1:
				data[0] = 'B';
				break;
			case 2:
				data[4] = 112;
				break;
			case 3:
				data[7] = 0x60;
				break;
			case 4:
				data[8] = (data[8] & ~63) | 2;
				break;
			case 5:
				data[12] = 6;
				break;
			case 6:
				data[13] = 20;
				break;
			case 7:
				data[14] = 2;
				break;
			case 8:
				WRITE_LE_UINT32(data.data() + 8, 1 | (1 << 19));
				break;
			case 9:
				WRITE_LE_UINT32(data.data() + 8, 1 | (2 << 6));
				break;
			case 10:
				WRITE_LE_UINT32(data.data() + 8, 1 | (8191U << 6) | (8191U << 19));
				break;
			case 11:
				WRITE_LE_UINT32(data.data() + 16, 0xfffffff0U);
				break;
			case 12:
				WRITE_LE_UINT32(data.data() + 16, 16);
				break;
			case 13:
				data.pop_back();
				break;
			}
			Reevengi::HD::DepthTexture depth;
			Common::String error;
			TS_ASSERT(!Reevengi::HD::decodeDepthTexture(data, depth, error));
			TS_ASSERT(!error.empty());
			TS_ASSERT(depth.values.empty());
		}
	}
};
