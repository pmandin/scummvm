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

#ifndef REEVENGI_HD_ARC_H
#define REEVENGI_HD_ARC_H

#include "common/array.h"
#include "common/str.h"
#include "common/stream.h"

namespace Reevengi {
namespace HD {

struct ArcEntry {
	Common::String path;
	uint32 type;
	uint32 packedSize;
	uint32 unpackedSize;
	uint32 offset;
	byte flags;
};

// Read-only MT Framework ARC v7 index. The caller retains the source stream.
// Failed loads clear the index; failed reads clear the output buffer.
class ArcIndex {
public:
	bool load(Common::SeekableReadStream &stream, Common::String &error);
	bool readEntry(Common::SeekableReadStream &stream, uint index,
				   Common::Array<byte> &output, Common::String &error) const;
	const Common::Array<ArcEntry> &entries() const { return _entries; }

private:
	Common::Array<ArcEntry> _entries;
};

} // namespace HD
} // namespace Reevengi

#endif
