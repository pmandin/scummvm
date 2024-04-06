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

#include "common/debug.h"
#include "common/memstream.h"
#include "common/substream.h"

#include "engines/reevengi/formats/rofs.h"

namespace Reevengi {

RofsArchive::RofsArchive() : Common::Archive() {
	_stream = nullptr;
}

RofsArchive::~RofsArchive() {
	close();
}

bool RofsArchive::open(const Common::Path &filename) {
	close();

	_stream = SearchMan.createReadStreamForMember(filename);

	if (!_stream)
		return false;

	if (!isArchive())
		return false;
	//debug(3, "rofs: header ok for %s", filename.c_str());

	char dir0[32], dir1[32];
	readFilename(dir0, sizeof(dir0));
	//debug(3, "rofs: dir0=%s", dir0);

	int32 dirLocation= _stream->readUint32LE() << 3;
	//debug(3, "rofs: dir location=%08x", dirLocation);

	/*int32 dirLength=*/ _stream->readUint32LE();
	//debug(3, "rofs: dir length=%d", dirLength);

	readFilename(dir1, sizeof(dir1));
	//debug(3, "rofs: dir1=%s", dir1);

	Common::String dirPrefix = dir0;
	dirPrefix += "/";
	dirPrefix += dir1;
	dirPrefix += "/";

	_stream->seek(dirLocation);
	enumerateFiles(dirPrefix);

	return true;
}

void RofsArchive::close() {
	delete _stream; _stream = nullptr;
	_map.clear();
}

bool RofsArchive::hasFile(const Common::Path &name) const {
	return (_map.find(name) != _map.end());
}

int RofsArchive::listMembers(Common::ArchiveMemberList &list) const {
	int count = 0;

	for (FileMap::const_iterator i = _map.begin(); i != _map.end(); ++i) {
		list.push_back(Common::ArchiveMemberList::value_type(new Common::GenericArchiveMember(i->_key, *this)));
		++count;
	}

	return count;
}

const Common::ArchiveMemberPtr RofsArchive::getMember(const Common::Path &name) const {
	if (!hasFile(name))
		return Common::ArchiveMemberPtr();

	return Common::ArchiveMemberPtr(new Common::GenericArchiveMember(name, *this));
}

Common::SeekableReadStream *RofsArchive::createReadStreamForMember(const Common::Path &name) const {
	FileMap::const_iterator fDesc = _map.find(name);

	if (fDesc == _map.end())
		return nullptr;

	const RofsFileEntry &entry = fDesc->_value;

	Common::SeekableSubReadStream subStream(_stream, entry.offset, entry.offset + entry.compressedSize);

	return new RofsFileStream(&entry, &subStream);
}

/* All rofs<n>.dat files start with this */
static const char rofs_id[21]={
	3, 0, 0, 0,
	1, 0, 0, 0,
	4, 0, 0, 0,
	0, 1, 1, 0,
	0, 4, 0, 0,
	0
};

bool RofsArchive::isArchive(void) {
	byte buf[21];

	_stream->read(buf, 21);

	return (memcmp(buf, rofs_id, sizeof(rofs_id)) == 0);
}

void RofsArchive::readFilename(char *filename, int nameLen) {
	char c;

	memset(filename, 0, nameLen);

	do {
		_stream->read(&c, 1);

		*filename++ = c;
	} while (c != '\0');
}

void RofsArchive::enumerateFiles(Common::String &dirPrefix) {
	int fileCount = _stream->readUint32LE();

	for(int i=0; i<fileCount; i++) {
		int32 fileOffset = _stream->readUint32LE() << 3;
		int32 fileCompSize = _stream->readUint32LE();

		char fileName[32];
		readFilename(fileName, sizeof(fileName));
		//debug(3, " entry %d: %s/%s", i, dirPrefix, fileName);

		Common::String name = dirPrefix;
		name += fileName;
		//debug(3, " entry %d: %s", i, name.c_str());

		RofsFileEntry entry;
		entry.compressed = false;
		entry.uncompressedSize = fileCompSize;
		entry.compressedSize = fileCompSize;
		entry.offset = fileOffset;

		/* Now read file header to check for compression */
		//debug(3, "file %s at 0x%08x", name.c_str(), fileOffset);

		int32 arcPos = _stream->pos();
		_stream->seek(entry.offset);
		readFileHeader(entry);
		_stream->seek(arcPos);

		_map[Common::Path(name)] = entry;
	}
}

void RofsArchive::readFileHeader(RofsFileEntry &entry) {
	char ident[8];

	_stream->skip(4);	/* Skip relative offset, num blocks */
	entry.uncompressedSize = _stream->readUint32LE();
	_stream->read(ident, 8);
	//entry.blkOffset = _stream->pos();

	for(int i=0; i<8; i++) {
		ident[i] ^= ident[7];
	}
	entry.compressed = (strcmp("Hi_Comp", ident)==0);

	//debug(3, " file %s", ident);
}

// RofsFileStream

RofsFileStream::RofsFileStream(const RofsFileEntry *entry, Common::SeekableReadStream *subStream):
	_pos(0), _arcStream(subStream) {

	_entry = *entry;
	_fileBuffer = new byte[_entry.uncompressedSize];
	memset(_fileBuffer, 0, _entry.uncompressedSize);

	decryptFile(_entry.compressed);
/*
	Common::DumpFile adf;
	adf.open("img0.jpg");
	adf.write(_fileBuffer, _entry.uncompressedSize);
	adf.close();
*/
	_arcStream = nullptr;
}

RofsFileStream::~RofsFileStream() {
	delete[] _fileBuffer;
	_fileBuffer = nullptr;
}

uint32 RofsFileStream::read(void *dataPtr, uint32 dataSize) {
	int32 sizeRead = MIN<int32>(dataSize, _entry.uncompressedSize-_pos);

	memcpy(dataPtr, &_fileBuffer[_pos], sizeRead);
	_pos += sizeRead;

	return sizeRead;
}

bool RofsFileStream::seek(int64 offs, int whence) {
	switch(whence) {
		case SEEK_SET:
			_pos = offs;
			break;
		case SEEK_END:
			_pos = offs+_entry.uncompressedSize;
			break;
		case SEEK_CUR:
			_pos += offs;
			break;
	}

	return true;
}

// RofsFileStream decryption

const unsigned short base_array[64]={
	0x00e6, 0x01a4, 0x00e6, 0x01c5,
	0x0130, 0x00e8, 0x03db, 0x008b,
	0x0141, 0x018e, 0x03ae, 0x0139,
	0x00f0, 0x027a, 0x02c9, 0x01b0,
	0x01f7, 0x0081, 0x0138, 0x0285,
	0x025a, 0x015b, 0x030f, 0x0335,
	0x02e4, 0x01f6, 0x0143, 0x00d1,
	0x0337, 0x0385, 0x007b, 0x00c6,
	0x0335, 0x0141, 0x0186, 0x02a1,
	0x024d, 0x0342, 0x01fb, 0x03e5,
	0x01b0, 0x006d, 0x0140, 0x00c0,
	0x0386, 0x016b, 0x020b, 0x009a,
	0x0241, 0x00de, 0x015e, 0x035a,
	0x025b, 0x0154, 0x0068, 0x02e8,
	0x0321, 0x0071, 0x01b0, 0x0232,
	0x02d9, 0x0263, 0x0164, 0x0290
};

void RofsFileStream::decryptFile(bool isCompressed) {
	uint16 fileOffset = _arcStream->readUint16LE();
	//debug(3, "offset 0x%08x", fileOffset);

	uint16 numBlocks = _arcStream->readUint16LE();
	//debug(3, "blocks: %d", numBlocks);

	_arcStream->skip(4+8);	// Uncompressed size + Hi_Comp/Not_Comp string

	uint32 *keyInfo = new uint32[2*numBlocks];

	/* Read key info */
	for(int i=0; i<2*numBlocks; i++) {
		keyInfo[i] = _arcStream->readUint32LE();
	}

	_arcStream->seek(fileOffset);
	int32 offset = 0;
	for(int i=0; i<numBlocks; i++) {
		uint32 blockKey = keyInfo[i];
		uint32 blockLen = keyInfo[i+numBlocks];

		//debug(3, "decrypt %d bytes with 0x%08x", blockLen, blockKey);

		_arcStream->read(&_fileBuffer[offset], blockLen);
		decryptBlock(&_fileBuffer[offset], blockKey, blockLen);

		if (isCompressed) {
			/* Depack remaining size, limited to 32K max */
			uint32 dstBlock = _entry.uncompressedSize - offset;
			if (dstBlock>32768) {
				dstBlock = 32768;
			}

			depack_block(&_fileBuffer[offset], blockLen, &dstBlock);
			if (dstBlock!=0) {
				blockLen = dstBlock;
			}
		}

		offset += blockLen;
	}

	//debug(3, "done");
	delete[] keyInfo;
}

void RofsFileStream::decryptBlock(byte *src, uint32 key, uint32 length) {
	uint8 xor_key, base_index, modulo;
	int block_index;

	xor_key = nextKey(&key);
	modulo = nextKey(&key);
	base_index = modulo % 0x3f;

	block_index = 0;
	for (uint32 i=0; i<length; i++) {
		if (block_index>base_array[base_index]) {
			modulo = nextKey(&key);
			base_index = modulo % 0x3f;
			xor_key = nextKey(&key);
			block_index = 0;
		}
		src[i] ^= xor_key;
		block_index++;
	}
}

uint8 RofsFileStream::nextKey(uint32 *key)
{
	*key *= 0x5d588b65;
	*key += 0x8000000b;

	return (*key >> 24);
}

// RofsFileStream depacking, LZSS like routine

void RofsFileStream::depack_block(uint8 *dst, uint32 srcLength, uint32 *dstLength)
{
	uint32 srcNumBit, srcIndex, tmpIndex, dstIndex;
	int i, value, value2, tmpStart, tmpLength;
	uint8 *src, *tmp4k;

	tmp4k = new uint8[4096+256];

	for (i=0; i<256; i++) {
 		memset(&tmp4k[i*16], i, 16);
	}
	memset(&tmp4k[4096], 0, 256);

	/* Copy source to a temp copy */
	src = (uint8 *) malloc(srcLength);
	if (!src) {
		error("Can not allocate memory for depacking\n");
		*dstLength = 0;
		delete tmp4k;
		return;
	}
	memcpy(src, dst, srcLength);

	/*printf("Depacking %08x to %08x, len %d\n", src,dst,length);*/

	srcNumBit = 0;
	srcIndex = 0;
	tmpIndex = 0;
	dstIndex = 0;
	while ((srcIndex<srcLength) && (dstIndex<*dstLength)) {
		srcNumBit++;

		value = src[srcIndex++] << srcNumBit;
		if (srcIndex<srcLength) {
			value |= src[srcIndex] >> (8-srcNumBit);
		}

		if (srcNumBit==8) {
			srcIndex++;
			srcNumBit = 0;
		}

		if ((value & (1<<8))==0) {
			dst[dstIndex++] = tmp4k[tmpIndex++] = value;
		} else {
			value2 = (src[srcIndex++] << srcNumBit) & 0xff;
			value2 |= src[srcIndex] >> (8-srcNumBit);

			tmpLength = (value2 & 0x0f)+2;

			tmpStart = (value2 >> 4) & 0xfff;
			tmpStart |= (value & 0xff) << 4;

			if (dstIndex+tmpLength > *dstLength) {
				tmpLength = (*dstLength)-dstIndex;
			}

			memcpy(&dst[dstIndex], &tmp4k[tmpStart], tmpLength);
			memcpy(&tmp4k[tmpIndex], &dst[dstIndex], tmpLength);

			dstIndex += tmpLength;
			tmpIndex += tmpLength;
		}

		if (tmpIndex>=4096) {
			tmpIndex = 0;
		}
	}

	/*printf("Depacked to %d len\n", dstIndex);*/

	free(src);
	*dstLength = dstIndex;
	delete[] tmp4k;
}

} // End of namespace Reevengi


