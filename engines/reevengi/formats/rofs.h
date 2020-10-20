/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef REEVENGI_ROFS_H
#define REEVENGI_ROFS_H

#include "common/archive.h"
#include "common/bitstream.h"
#include "common/scummsys.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/str.h"

namespace Reevengi {

struct RofsFileEntry {
	bool compressed;
	uint32 uncompressedSize;
	uint32 compressedSize;
	uint32 offset;
};

class RofsArchive : public Common::Archive {
public:
	RofsArchive();
	~RofsArchive();

	bool open(const Common::String &filename);
	void close();
	bool isOpen() const { return _stream != 0; }

	// Common::Archive API implementation
	bool hasFile(const Common::String &name) const override;
	int listMembers(Common::ArchiveMemberList &list) const override;
	const Common::ArchiveMemberPtr getMember(const Common::String &name) const override;
	Common::SeekableReadStream *createReadStreamForMember(const Common::String &name) const override;

private:
	Common::SeekableReadStream *_stream;

	typedef Common::HashMap<Common::String, RofsFileEntry, Common::IgnoreCase_Hash, Common::IgnoreCase_EqualTo> FileMap;
	FileMap _map;

	// Archive parsing
	bool isArchive(void);
	void readFilename(char *filename, int nameLen);
	void enumerateFiles(Common::String &dirPrefix);
	void readFileHeader(RofsFileEntry &entry);

};

class RofsFileStream: public Common::SeekableReadStream {
public:
	Common::SeekableReadStream *_arcStream;
	RofsFileEntry _entry;
	uint32 _pos;
	byte *_fileBuffer;

	RofsFileStream(const RofsFileEntry *entry, Common::SeekableReadStream *subStream);
	~RofsFileStream();

private:
	uint32 read(void *dataPtr, uint32 dataSize);

	bool eos() const { return _pos>=_entry.uncompressedSize; }

	int32 pos() const { return _pos; }
	int32 size() const { return _entry.uncompressedSize; }

	bool seek(int32 offs, int whence = SEEK_SET);

	// Decrypt file
	void decryptFile(void);
	void decryptBlock(byte *src, uint32 key, uint32 length);
	uint8 nextKey(uint32 *key);

	// Depack file
	void depackFile(void);
	void depackBlock(uint8 *dst, int dstLength, uint8 *src, int srcLength);
};

} // End of namespace Reevengi

#endif

