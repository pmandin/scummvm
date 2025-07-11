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

#include "common/array.h"
#include "common/debug.h"
#include "common/endian.h"
#include "common/str.h"
#include "common/stream.h"
#include "common/formats/winexe_pe.h"

namespace Common {

PEResources::PEResources() {
	_exe = nullptr;
	_disposeFileHandle = DisposeAfterUse::YES;
}

PEResources::~PEResources() {
	clear();
}

void PEResources::clear() {
	_sections.clear();
	_resources.clear();
	if (_exe) {
		if (_disposeFileHandle == DisposeAfterUse::YES)
			delete _exe;
		_exe = nullptr;
	}
}

bool PEResources::loadFromEXE(SeekableReadStream *stream, DisposeAfterUse::Flag disposeFileHandle) {
	clear();

	_exe = stream;
	_disposeFileHandle = disposeFileHandle;

	if (!stream)
		return false;

	if (stream->readUint16BE() != MKTAG16('M', 'Z'))
		return false;

	stream->skip(58);

	uint32 peOffset = stream->readUint32LE();

	if (!peOffset || peOffset >= (uint32)stream->size())
		return false;

	stream->seek(peOffset);

	if (stream->readUint32BE() != MKTAG('P','E',0,0))
		return false;

	stream->skip(2);
	uint16 sectionCount = stream->readUint16LE();
	stream->skip(12);
	uint16 optionalHeaderSize = stream->readUint16LE();
	stream->skip(optionalHeaderSize + 2);

	// Read in all the sections
	for (uint16 i = 0; i < sectionCount; i++) {
		char sectionName[9];
		stream->read(sectionName, 8);
		sectionName[8] = 0;

		Section section;
		stream->skip(4);
		section.virtualAddress = stream->readUint32LE();
		section.size = stream->readUint32LE();
		section.offset = stream->readUint32LE();
		stream->skip(16);

		_sections[sectionName] = section;
	}

	// Currently, we require loading a resource section
	if (!_sections.contains(".rsrc")) {
		clear();
		return false;
	}

	Section &resSection = _sections[".rsrc"];
	parseResourceLevel(resSection, resSection.offset, 0);

	return true;
}

void PEResources::parseResourceLevel(Section &section, uint32 offset, int level) {
	_exe->seek(offset + 12);

	uint16 namedEntryCount = _exe->readUint16LE();
	uint16 intEntryCount = _exe->readUint16LE();

	for (uint32 i = 0; i < (uint32)(namedEntryCount + intEntryCount); i++) {
		uint32 value = _exe->readUint32LE();

		WinResourceID id;

		if (value & 0x80000000) {
			value &= 0x7fffffff;

			uint32 startPos = _exe->pos();
			_exe->seek(section.offset + (value & 0x7fffffff));

			// Read in the name, truncating from unicode to ascii
			String name;
			uint16 nameLength = _exe->readUint16LE();
			while (nameLength--)
				name += (char)(_exe->readUint16LE() & 0xff);

			_exe->seek(startPos);

			id = name;
		} else {
			id = value;
		}

		uint32 nextOffset = _exe->readUint32LE();
		uint32 lastOffset = _exe->pos();

		if (level == 0)
			_curType = id;
		else if (level == 1)
			_curID = id;
		else if (level == 2)
			_curLang = id;

		if (level < 2) {
			// Time to dive down further
			parseResourceLevel(section, section.offset + (nextOffset & 0x7fffffff), level + 1);
		} else {
			_exe->seek(section.offset + nextOffset);

			Resource resource;
			resource.offset = _exe->readUint32LE() + section.offset - section.virtualAddress;
			resource.size = _exe->readUint32LE();

			debug(4, "Found resource '%s' '%s' '%s' at %d of size %d", _curType.toString().c_str(),
					_curID.toString().c_str(), _curLang.toString().c_str(), resource.offset, resource.size);

			_resources[_curType][_curID][_curLang] = resource;
		}

		_exe->seek(lastOffset);
	}
}

const Array<WinResourceID> PEResources::getTypeList() const {
	Array<WinResourceID> array;

	if (!_exe)
		return array;

	for (const auto &resource : _resources)
		array.push_back(resource._key);

	return array;
}

const Array<WinResourceID> PEResources::getIDList(const WinResourceID &type) const {
	Array<WinResourceID> array;

	if (!_exe || !_resources.contains(type))
		return array;

	const IDMap &idMap = _resources[type];

	for (const auto &id : idMap)
		array.push_back(id._key);

	return array;
}

const Array<WinResourceID> PEResources::getLangList(const WinResourceID &type, const WinResourceID &id) const {
	Array<WinResourceID> array;

	if (!_exe || !_resources.contains(type))
		return array;

	const IDMap &idMap = _resources[type];

	if (!idMap.contains(id))
		return array;

	const LangMap &langMap = idMap[id];

	for (const auto &lang : langMap)
		array.push_back(lang._key);

	return array;
}

SeekableReadStream *PEResources::getResource(const WinResourceID &type, const WinResourceID &id) {
	Array<WinResourceID> langList = getLangList(type, id);

	if (langList.empty())
		return nullptr;

	const Resource &resource = _resources[type][id][langList[0]];
	_exe->seek(resource.offset);
	return _exe->readStream(resource.size);
}

SeekableReadStream *PEResources::getResource(const WinResourceID &type, const WinResourceID &id, const WinResourceID &lang) {
	if (!_exe || !_resources.contains(type))
		return nullptr;

	const IDMap &idMap = _resources[type];

	if (!idMap.contains(id))
		return nullptr;

	const LangMap &langMap = idMap[id];

	if (!langMap.contains(lang))
		return nullptr;

	const Resource &resource = langMap[lang];
	_exe->seek(resource.offset);
	return _exe->readStream(resource.size);
}

String PEResources::loadString(uint32 stringID) {
	String string;
	SeekableReadStream *stream = getResource(kWinString, (stringID >> 4) + 1);

	if (!stream)
		return string;

	// Skip over strings we don't care about
	uint32 startString = stringID & ~0xF;

	for (uint32 i = startString; i < stringID; i++)
		stream->skip(stream->readUint16LE() * 2);

	// HACK: Truncate UTF-16 down to ASCII
	byte size = stream->readUint16LE();
	while (size--)
		string += (char)(stream->readUint16LE() & 0xFF);

	delete stream;
	return string;
}

WinResources::VersionInfo *PEResources::parseVersionInfo(SeekableReadStream *res) {
	VersionInfo *info = new VersionInfo;

	while (res->pos() < res->size() && !res->eos()) {
		while (res->pos() % 4 && !res->eos()) // Pad to 4
			res->readByte();

		/* uint16 len = */ res->readUint16LE();
		uint16 valLen = res->readUint16LE();
		uint16 type = res->readUint16LE();
		uint16 c;

		if (res->eos())
			break;

		Common::U32String key;
		while ((c = res->readUint16LE()) != 0 && !res->eos())
			key += c;

		while (res->pos() % 4 && !res->eos()) // Pad to 4
			res->readByte();

		if (res->eos())
			break;

		if (type != 0) {	// text
			Common::U32String value;
			for (int j = 0; j < valLen; j++)
				value += res->readUint16LE();

			info->hash.setVal(key.encode(), value);
		} else {
			if (key == "VS_VERSION_INFO") {
				if (!info->readVSVersionInfo(res))
					return info;
			}
		}
	}

	return info;
}

} // End of namespace Common
