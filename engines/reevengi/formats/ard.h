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

#ifndef REEVENGI_ARD_H
#define REEVENGI_ARD_H

#include "common/stream.h"

namespace Reevengi {

class ArdArchive {
public:
	static const int kRdtFile = 8;
	static const int kVbFile = 9;

	ArdArchive(Common::SeekableReadStream *stream);
	virtual ~ArdArchive();

	Common::SeekableReadStream *createReadStreamForMember(int numFile) const;

private:
	Common::SeekableReadStream *_stream;
	int _numFiles;
};

} // End of namespace Reevengi

#endif
