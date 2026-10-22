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
#include "common/memstream.h"
#include "common/util.h"
#include "engines/reevengi/hd/arc.h"

class HDArcTestSuite : public CxxTest::TestSuite {
private:
	Common::Array<byte> fixture() {
		// zlib-compressed "XFS\0test-room"; synthetic data, no game content.
		static const byte packed[] = {
			0x78, 0x9c, 0x8b, 0x70, 0x0b, 0x66, 0x28, 0x49, 0x2d, 0x2e, 0xd1,
			0x2d, 0xca, 0xcf, 0xcf, 0x05, 0x00, 0x1d, 0xb8, 0x04, 0x9c};
		Common::Array<byte> data;
		data.resize(88 + sizeof(packed));
		memset(data.data(), 0, data.size());
		memcpy(data.data(), "ARC", 3);
		WRITE_LE_UINT16(data.data() + 4, 7);
		WRITE_LE_UINT16(data.data() + 6, 1);
		memcpy(data.data() + 8, "room\\test", 9);
		WRITE_LE_UINT32(data.data() + 72, 0x53a2970e);
		WRITE_LE_UINT32(data.data() + 76, sizeof(packed));
		WRITE_LE_UINT32(data.data() + 80, 13 | (2U << 29));
		WRITE_LE_UINT32(data.data() + 84, 88);
		memcpy(data.data() + 88, packed, sizeof(packed));
		return data;
	}

	void assertRejected(const Common::Array<byte> &data) {
		Common::MemoryReadStream stream(data.data(), data.size());
		Reevengi::HD::ArcIndex arc;
		Common::String error;
		TS_ASSERT(!arc.load(stream, error));
		TS_ASSERT(!error.empty());
		TS_ASSERT(arc.entries().empty());
	}

public:
	void testIndexAndDecompression() {
		const Common::Array<byte> data = fixture();
		Common::MemoryReadStream stream(data.data(), data.size());
		Reevengi::HD::ArcIndex arc;
		Common::String error;
		TS_ASSERT(arc.load(stream, error));
		TS_ASSERT_EQUALS(arc.entries().size(), 1U);
		TS_ASSERT_EQUALS(arc.entries()[0].path, "room\\test");
		TS_ASSERT_EQUALS(arc.entries()[0].flags, 2);
		TS_ASSERT_EQUALS(arc.entries()[0].unpackedSize, 13U);
		Common::Array<byte> output;
		TS_ASSERT(arc.readEntry(stream, 0, output, error));
		TS_ASSERT_EQUALS(output.size(), 13U);
		TS_ASSERT_SAME_DATA(output.data(), "XFS\0test-room", 13);
		TS_ASSERT(error.empty());
		TS_ASSERT(!arc.readEntry(stream, 1, output, error));
		TS_ASSERT(output.empty());
	}

	void testShortHeadersAndWrongVersions() {
		Common::Array<byte> data = fixture();
		data.resize(7);
		assertRejected(data);
		data = fixture();
		data[0] = 'B';
		assertRejected(data);
		data = fixture();
		WRITE_LE_UINT16(data.data() + 4, 17);
		assertRejected(data);
	}

	void testTruncatedIndexAndUnterminatedName() {
		Common::Array<byte> data = fixture();
		WRITE_LE_UINT16(data.data() + 6, 2);
		assertRejected(data);
		data = fixture();
		memset(data.data() + 8, 'a', 64);
		assertRejected(data);
	}

	void testPayloadBoundsAndOverflow() {
		Common::Array<byte> data = fixture();
		WRITE_LE_UINT32(data.data() + 84, 8);
		assertRejected(data);
		data = fixture();
		WRITE_LE_UINT32(data.data() + 84, 0xfffffff0U);
		WRITE_LE_UINT32(data.data() + 76, 32);
		assertRejected(data);
		data = fixture();
		data.pop_back();
		assertRejected(data);
	}

	void testInvalidCompressionAndDeclaredSize() {
		for (uint mode = 0; mode < 3; ++mode) {
			Common::Array<byte> data = fixture();
			if (mode == 0)
				data[88] = 0;
			else if (mode == 1)
				WRITE_LE_UINT32(data.data() + 80, 14 | (2U << 29));
			else
				WRITE_LE_UINT32(data.data() + 80, (65U * 1024 * 1024) | (2U << 29));
			Common::MemoryReadStream stream(data.data(), data.size());
			Reevengi::HD::ArcIndex arc;
			Common::String error;
			Common::Array<byte> output;
			TS_ASSERT(arc.load(stream, error));
			TS_ASSERT(!arc.readEntry(stream, 0, output, error));
			TS_ASSERT(output.empty());
			TS_ASSERT(!error.empty());
		}
	}

	void testFailedReloadClearsOldIndex() {
		Common::Array<byte> data = fixture();
		Common::MemoryReadStream valid(data.data(), data.size());
		Reevengi::HD::ArcIndex arc;
		Common::String error;
		TS_ASSERT(arc.load(valid, error));
		Common::MemoryReadStream invalid(data.data(), 2);
		TS_ASSERT(!arc.load(invalid, error));
		TS_ASSERT(arc.entries().empty());
	}

	void testEmptyArchiveAndEmptyName() {
		Common::Array<byte> data = fixture();
		data.resize(8);
		WRITE_LE_UINT16(data.data() + 6, 0);
		Common::MemoryReadStream stream(data.data(), data.size());
		Reevengi::HD::ArcIndex arc;
		Common::String error;
		TS_ASSERT(arc.load(stream, error));
		TS_ASSERT(arc.entries().empty());
		TS_ASSERT(error.empty());
		data = fixture();
		data[8] = 0;
		assertRejected(data);
	}

	void testEmptyAndUndersizedDeclaredPayloadsClearOutput() {
		const uint32 sizes[] = {0, 12};
		for (uint i = 0; i < ARRAYSIZE(sizes); ++i) {
			Common::Array<byte> data = fixture();
			WRITE_LE_UINT32(data.data() + 80, sizes[i] | (2U << 29));
			Common::MemoryReadStream stream(data.data(), data.size());
			Reevengi::HD::ArcIndex arc;
			Common::String error;
			Common::Array<byte> output;
			output.push_back(42);
			TS_ASSERT(arc.load(stream, error));
			TS_ASSERT(!arc.readEntry(stream, 0, output, error));
			TS_ASSERT(output.empty());
			TS_ASSERT(!error.empty());
		}
	}

	void testShortReadClearsOutput() {
		const Common::Array<byte> data = fixture();
		Common::MemoryReadStream stream(data.data(), data.size());
		Reevengi::HD::ArcIndex arc;
		Common::String error;
		TS_ASSERT(arc.load(stream, error));
		Common::MemoryReadStream truncated(data.data(), data.size() - 1);
		Common::Array<byte> output;
		output.push_back(42);
		TS_ASSERT(!arc.readEntry(truncated, 0, output, error));
		TS_ASSERT(output.empty());
		TS_ASSERT(!error.empty());
	}
};
