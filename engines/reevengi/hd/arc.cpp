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

#include "engines/reevengi/hd/arc.h"
#include "common/compression/deflate.h"

namespace Reevengi {
namespace HD {

bool ArcIndex::load(Common::SeekableReadStream &stream, Common::String &error) {
	_entries.clear();
	error.clear();
	if (!stream.seek(0) || stream.size() < 8 || stream.readUint32BE() != MKTAG('A', 'R', 'C', 0)) {
		error = "Missing ARC header";
		return false;
	}
	if (stream.readUint16LE() != 7) {
		error = "Only ARC version 7 is supported";
		return false;
	}
	const uint16 count = stream.readUint16LE();
	const uint64 tableEnd = 8 + uint64(count) * 80;
	if (tableEnd > uint64(stream.size())) {
		error = "Truncated ARC index";
		return false;
	}

	Common::Array<ArcEntry> entries;
	entries.reserve(count);
	for (uint i = 0; i < count; ++i) {
		char path[64];
		if (stream.read(path, sizeof(path)) != sizeof(path) || !memchr(path, 0, sizeof(path)) || !path[0]) {
			error = "Invalid ARC resource name";
			return false;
		}
		ArcEntry entry;
		entry.path = path;
		entry.type = stream.readUint32LE();
		entry.packedSize = stream.readUint32LE();
		const uint32 sizeAndFlags = stream.readUint32LE();
		entry.unpackedSize = sizeAndFlags & 0x1fffffff;
		entry.flags = sizeAndFlags >> 29;
		entry.offset = stream.readUint32LE();
		if (stream.err() || entry.offset < tableEnd ||
			uint64(entry.offset) + entry.packedSize > uint64(stream.size())) {
			error = "ARC payload lies outside the file or overlaps the index";
			return false;
		}
		entries.push_back(entry);
	}
	_entries = entries;
	return true;
}

bool ArcIndex::readEntry(Common::SeekableReadStream &stream, uint index,
						 Common::Array<byte> &output, Common::String &error) const {
	output.clear();
	error.clear();
	if (index >= _entries.size()) {
		error = "ARC entry index out of range";
		return false;
	}
	const ArcEntry &entry = _entries[index];
	// A resource limit, not a format limit. Check before allocating either buffer.
	const uint32 maxResourceSize = 64 * 1024 * 1024;
	if (!entry.packedSize || !entry.unpackedSize ||
		entry.packedSize > maxResourceSize || entry.unpackedSize > maxResourceSize) {
		error = "ARC resource exceeds reader limits or is empty";
		return false;
	}
	Common::Array<byte> packed;
	packed.resize(entry.packedSize);
	if (!stream.seek(entry.offset) || stream.read(packed.data(), packed.size()) != packed.size()) {
		error = "Truncated ARC payload";
		return false;
	}
	Common::Array<byte> unpacked;
	unpacked.resize(entry.unpackedSize);
	unsigned long actualSize = entry.unpackedSize;
	if (!Common::inflateZlib(unpacked.data(), &actualSize, packed.data(), packed.size()) ||
		actualSize != entry.unpackedSize) {
		error = "ARC zlib payload is invalid or has a different unpacked size";
		return false;
	}
	output = unpacked;
	return true;
}

} // namespace HD
} // namespace Reevengi
