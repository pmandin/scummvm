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

#ifndef REEVENGI_SLD_H
#define REEVENGI_SLD_H

#include "common/stream.h"

namespace Reevengi {

class SldArchive {
public:
	SldArchive(Common::SeekableReadStream *stream);
	virtual ~SldArchive();

	Common::SeekableReadStream *createReadStreamForMember(int numFile) const;

private:
	Common::SeekableReadStream *_stream;
};

class SldFileStream: public Common::SeekableReadStream {
public:
	Common::SeekableReadStream *_arcStream;
	uint32 _pos, _size;
	byte *_fileBuffer;

	SldFileStream(Common::SeekableReadStream *subStream);
	~SldFileStream();

private:
	uint32 read(void *dataPtr, uint32 dataSize);

	bool eos() const { return _pos>=_size; }

	int64 pos() const { return _pos; }
	int64 size() const { return _size; }

	bool seek(int64 offs, int whence = SEEK_SET);

	// Depack file
	void depackFile(void);
};

} // End of namespace Reevengi

#endif
