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

#include "common/crc.h"
#include "common/file.h"
#include "common/macresman.h"
#include "common/stuffit.h"
#include "common/winexe.h"

#include "graphics/maccursor.h"
#include "graphics/wincursor.h"

#include "mtropolis/boot.h"
#include "mtropolis/detection.h"
#include "mtropolis/runtime.h"

#include "mtropolis/plugin/obsidian.h"
#include "mtropolis/plugin/standard.h"
#include "mtropolis/plugins.h"

namespace MTropolis {

namespace Boot {

enum FileCategory {
	kFileCategoryPlayer,
	kFileCategoryExtension,
	kFileCategoryProjectAdditionalSegment,
	kFileCategoryProjectMainSegment,

	kFileCategorySpecial,

	kFileCategoryUnknown,
};

struct FileIdentification {
	Common::String fileName;
	FileCategory category;

	uint32 macType;
	uint32 macCreator;
	Common::SharedPtr<Common::MacResManager> resMan;
	Common::SharedPtr<Common::SeekableReadStream> stream;
};

static void initResManForFile(FileIdentification &f) {
	if (!f.resMan) {
		f.resMan.reset(new Common::MacResManager());
		if (!f.resMan->open(f.fileName))
			error("Failed to open resources of file '%s'", f.fileName.c_str());
	}
}

class GameDataHandler {
public:
	virtual ~GameDataHandler();

	virtual void unpackAdditionalFiles(Common::Array<Common::SharedPtr<ProjectPersistentResource> > &persistentResources, Common::Array<FileIdentification> &files);
	virtual void categorizeSpecialFiles(Common::Array<FileIdentification> &files);
	virtual void addPlugIns(ProjectDescription &projectDesc, const Common::Array<FileIdentification> &files);
};

GameDataHandler::~GameDataHandler() {
}

void GameDataHandler::unpackAdditionalFiles(Common::Array<Common::SharedPtr<ProjectPersistentResource> > &persistentResources, Common::Array<FileIdentification> &files) {
}

void GameDataHandler::categorizeSpecialFiles(Common::Array<FileIdentification> &files) {
}

void GameDataHandler::addPlugIns(ProjectDescription &projectDesc, const Common::Array<FileIdentification> &files) {
	Common::SharedPtr<MTropolis::PlugIn> standardPlugIn = PlugIns::createStandard();
	projectDesc.addPlugIn(standardPlugIn);
}

template<class T>
class PersistentResource : public ProjectPersistentResource {
public:
	explicit PersistentResource(const Common::SharedPtr<T> &item);
	const Common::SharedPtr<T> &getItem();

	static Common::SharedPtr<ProjectPersistentResource> wrap(const Common::SharedPtr<T> &item);

private:
	Common::SharedPtr<T> _item;
};

template<class T>
PersistentResource<T>::PersistentResource(const Common::SharedPtr<T> &item) : _item(item) {
}

template<class T>
const Common::SharedPtr<T> &PersistentResource<T>::getItem() {
	return _item;
}

template<class T>
Common::SharedPtr<ProjectPersistentResource> PersistentResource<T>::wrap(const Common::SharedPtr<T> &item) {
	return Common::SharedPtr<ProjectPersistentResource>(new PersistentResource<T>(item));
}

class ObsidianGameDataHandler : public GameDataHandler {
public:
	explicit ObsidianGameDataHandler(const MTropolisGameDescription &gameDesc);

	void unpackAdditionalFiles(Common::Array<Common::SharedPtr<ProjectPersistentResource> > &persistentResources, Common::Array<FileIdentification> &files) override;
	void categorizeSpecialFiles(Common::Array<FileIdentification> &files) override;
	void addPlugIns(ProjectDescription &projectDesc, const Common::Array<FileIdentification> &files) override;

private:
	bool _isMac;
	bool _isRetail;
	bool _isEnglish;

	void unpackMacRetailInstaller(Common::Array<Common::SharedPtr<ProjectPersistentResource> > &persistentResources, Common::Array<FileIdentification> &files);
	Common::SharedPtr<Obsidian::WordGameData> loadWinWordGameData();
	Common::SharedPtr<Obsidian::WordGameData> loadMacWordGameData();

	Common::SharedPtr<Common::Archive> _installerArchive;
};

ObsidianGameDataHandler::ObsidianGameDataHandler(const MTropolisGameDescription &gameDesc) {
	_isMac = (gameDesc.desc.platform == Common::kPlatformMacintosh);
	_isEnglish = (gameDesc.desc.language == Common::EN_ANY);
	_isRetail = ((gameDesc.desc.flags & ADGF_DEMO) == 0);
}

struct MacInstallerUnpackRequest {
	const char *fileName;
	uint32 type;
	uint32 creator;
};

void ObsidianGameDataHandler::unpackAdditionalFiles(Common::Array<Common::SharedPtr<ProjectPersistentResource> > &persistentResources, Common::Array<FileIdentification> &files) {
	if (_isMac && _isRetail)
		unpackMacRetailInstaller(persistentResources, files);
}

void ObsidianGameDataHandler::unpackMacRetailInstaller(Common::Array<Common::SharedPtr<ProjectPersistentResource> > &persistentResources, Common::Array<FileIdentification> &files) {
	const MacInstallerUnpackRequest requests[] = {
		{"Obsidian", MKTAG('A', 'P', 'P', 'L'), MKTAG('M', 'f', 'P', 'l')},
		{"Basic.rPP", MKTAG('M', 'F', 'X', 'O'), MKTAG('M', 'f', 'P', 'l')},
		{"mCursors.cPP", MKTAG('M', 'F', 'c', 'r'), MKTAG('M', 'f', 'P', 'l')},
		{"Obsidian.cPP", MKTAG('M', 'F', 'c', 'r'), MKTAG('M', 'f', 'M', 'f')},
		{"RSGKit.rPP", MKTAG('M', 'F', 'c', 'o'), MKTAG('M', 'f', 'M', 'f')},
	};

	Common::SharedPtr<Common::MacResManager> installerResMan(new Common::MacResManager());
	persistentResources.push_back(PersistentResource<Common::MacResManager>::wrap(installerResMan));

	if (!installerResMan->open("Obsidian Installer"))
		error("Failed to open Obsidian Installer");

	if (!installerResMan->hasDataFork())
		error("Obsidian Installer has no data fork");

	Common::SeekableReadStream *installerDataForkStream = installerResMan->getDataFork();

	// Not counted/persisted because the StuffIt archive owns the stream
	_installerArchive.reset(Common::createStuffItArchive(installerDataForkStream));
	persistentResources.push_back(PersistentResource<Common::Archive>::wrap(_installerArchive));

	if (!_installerArchive) {
		delete installerDataForkStream;
		installerDataForkStream = nullptr;

		error("Failed to open Obsidian Installer archive");
	}

	debug(1, "Unpacking resource files...");

	for (const MacInstallerUnpackRequest &request : requests) {
		Common::SharedPtr<Common::MacResManager> resMan(new Common::MacResManager());

		if (!resMan->open(request.fileName, *_installerArchive))
			error("Failed to open file '%s' from installer package", request.fileName);

		FileIdentification ident;
		ident.fileName = request.fileName;
		ident.macCreator = request.creator;
		ident.macType = request.type;
		ident.resMan = resMan;
		ident.category = kFileCategoryUnknown;
		files.push_back(ident);
	}

	{
		debug(1, "Unpacking startup segment...");

		Common::SharedPtr<Common::SeekableReadStream> startupStream(_installerArchive->createReadStreamForMember("Obsidian Data 1"));

		FileIdentification ident;
		ident.fileName = "Obsidian Data 1";
		ident.macCreator = MKTAG('M', 'f', 'P', 'l');
		ident.macType = MKTAG('M', 'F', 'm', 'm');
		ident.category = kFileCategoryUnknown;
		ident.stream = startupStream;
		files.push_back(ident);
	}
}

void ObsidianGameDataHandler::categorizeSpecialFiles(Common::Array<FileIdentification> &files) {
	// Flag the installer as Special so it doesn't get detected as the player due to being an application
	// Flag RSGKit as Special so it doesn't get fed to the cursor loader
	for (FileIdentification &file : files) {
		if (file.fileName == "Obsidian Installer" || file.fileName == "RSGKit.rPP" || file.fileName == "RSGKit.r95")
			file.category = kFileCategorySpecial;
	}
}

void ObsidianGameDataHandler::addPlugIns(ProjectDescription &projectDesc, const Common::Array<FileIdentification> &files) {
	Common::SharedPtr<Obsidian::WordGameData> wgData;

	if (_isRetail && _isEnglish) {
		if (_isMac)
			wgData = loadMacWordGameData();
		else
			wgData = loadWinWordGameData();
	}

	Common::SharedPtr<Obsidian::ObsidianPlugIn> obsidianPlugIn(new Obsidian::ObsidianPlugIn(wgData));
	projectDesc.addPlugIn(obsidianPlugIn);

	Common::SharedPtr<MTropolis::PlugIn> standardPlugIn = PlugIns::createStandard();
	static_cast<Standard::StandardPlugIn *>(standardPlugIn.get())->getHacks().allowGarbledListModData = true;
	projectDesc.addPlugIn(standardPlugIn);
}

Common::SharedPtr<Obsidian::WordGameData> ObsidianGameDataHandler::loadMacWordGameData() {
	Common::ArchiveMemberPtr rsgKit = _installerArchive->getMember(Common::Path("RSGKit.rPP"));
	if (!rsgKit)
		error("Couldn't find word game file in installer archive");

	Common::SharedPtr<Obsidian::WordGameData> wgData(new Obsidian::WordGameData());

	Common::SharedPtr<Common::SeekableReadStream> stream(rsgKit->createReadStream());
	if (!stream)
		error("Failed to open word game file");

	Obsidian::WordGameLoadBucket buckets[] = {
		{0, 0},             // 0 letters
		{0xD7C8, 0xD7CC},   // 1 letter
		{0xD7CC, 0xD84D},   // 2 letter
		{0xD84D, 0xE25D},   // 3 letter
		{0x1008C, 0x12AA8}, // 4 letter
		{0x14C58, 0x19614}, // 5 letter
		{0x1C73C, 0x230C1}, // 6 letter
		{0x26D10, 0x2EB98}, // 7 letter
		{0x32ADC, 0x3AA0E}, // 8 letter
		{0x3E298, 0x45B88}, // 9 letter
		{0x48BE8, 0x4E0D0}, // 10 letter
		{0x4FFB0, 0x53460}, // 11 letter
		{0x545F0, 0x56434}, // 12 letter
		{0x56D84, 0x57CF0}, // 13 letter
		{0x58158, 0x58833}, // 14 letter
		{0x58A08, 0x58CD8}, // 15 letter
		{0x58D8C, 0x58EAD}, // 16 letter
		{0x58EF4, 0x58F72}, // 17 letter
		{0x58F90, 0x58FDC}, // 18 letter
		{0, 0},             // 19 letter
		{0x58FEC, 0x59001}, // 20 letter
		{0x59008, 0x59034}, // 21 letter
		{0x5903C, 0x59053}, // 22 letter
	};

	if (!wgData->load(stream.get(), buckets, 23, 1, false))
		error("Failed to load word game data");

	return wgData;
}

Common::SharedPtr<Obsidian::WordGameData> ObsidianGameDataHandler::loadWinWordGameData() {
	Common::File f;
	if (!f.open("RSGKit.r95")) {
		error("Couldn't open word game data file");
		return nullptr;
	}

	Common::SharedPtr<Obsidian::WordGameData> wgData(new Obsidian::WordGameData());

	Obsidian::WordGameLoadBucket buckets[] = {
		{0, 0},             // 0 letters
		{0x63D54, 0x63D5C}, // 1 letter
		{0x63BF8, 0x63CA4}, // 2 letter
		{0x627D8, 0x631E8}, // 3 letter
		{0x5C2C8, 0x60628}, // 4 letter
		{0x52F4C, 0x5919C}, // 5 letter
		{0x47A64, 0x4F2FC}, // 6 letter
		{0x3BC98, 0x43B20}, // 7 letter
		{0x2DA78, 0x38410}, // 8 letter
		{0x218F8, 0x2AA18}, // 9 letter
		{0x19D78, 0x1FA18}, // 10 letter
		{0x15738, 0x18BE8}, // 11 letter
		{0x128A8, 0x14DE8}, // 12 letter
		{0x1129C, 0x1243C}, // 13 letter
		{0x10974, 0x110C4}, // 14 letter
		{0x105EC, 0x108BC}, // 15 letter
		{0x10454, 0x105A8}, // 16 letter
		{0x103A8, 0x10434}, // 17 letter
		{0x10348, 0x10398}, // 18 letter
		{0, 0},             // 19 letter
		{0x10328, 0x10340}, // 20 letter
		{0x102EC, 0x1031C}, // 21 letter
		{0x102D0, 0x102E8}, // 22 letter
	};

	if (!wgData->load(&f, buckets, 23, 4, true)) {
		error("Failed to load word game data file");
		return nullptr;
	}

	return wgData;
}

static bool getMacTypesForMacBinary(const char *fileName, uint32 &outType, uint32 &outCreator) {
	Common::SharedPtr<Common::SeekableReadStream> stream(SearchMan.createReadStreamForMember(fileName));

	if (!stream)
		return false;

	byte mbHeader[MBI_INFOHDR];
	if (stream->read(mbHeader, MBI_INFOHDR) != MBI_INFOHDR)
		return false;

	if (mbHeader[0] != 0 || mbHeader[74] != 0)
		return false;

	Common::CRC_BINHEX crc;
	crc.init();
	uint16 checkSum = crc.crcFast(mbHeader, 124);

	if (checkSum != READ_BE_UINT16(&mbHeader[124]))
		return false;

	outType = MKTAG(mbHeader[65], mbHeader[66], mbHeader[67], mbHeader[68]);
	outCreator = MKTAG(mbHeader[69], mbHeader[70], mbHeader[71], mbHeader[72]);

	return true;
}

static uint32 getWinFileEndingPseudoTag(const Common::String &fileName) {
	byte bytes[4] = {0, 0, 0, 0};
	size_t numInserts = 4;
	if (fileName.size() < 4)
		numInserts = fileName.size();

	for (size_t i = 0; i < numInserts; i++)
		bytes[i] = static_cast<byte>(invariantToLower(fileName[fileName.size() - numInserts + i]));

	return MKTAG(bytes[0], bytes[1], bytes[2], bytes[3]);
}

static bool getMacTypesForFile(const char *fileName, uint32 &outType, uint32 &outCreator) {
	if (getMacTypesForMacBinary(fileName, outType, outCreator))
		return true;

	return false;
}

static bool fileSortCompare(const FileIdentification &a, const FileIdentification &b) {
	// If file names are mismatched then we want the first one to be shorter
	if (a.fileName.size() > b.fileName.size())
		return !fileSortCompare(b, a);

	size_t aSize = a.fileName.size();
	for (size_t i = 0; i < aSize; i++) {
		char ac = invariantToLower(a.fileName[i]);
		char bc = invariantToLower(b.fileName[i]);

		if (ac < bc)
			return true;
		if (bc < ac)
			return false;
	}

	return aSize < b.fileName.size();
}

static int resolveFileSegmentID(const Common::String &fileName) {
	size_t lengthWithoutExtension = fileName.size();

	size_t dotPos = fileName.findLastOf('.');
	if (dotPos != Common::String::npos)
		lengthWithoutExtension = dotPos;

	int numDigits = 0;
	int segmentID = 0;
	int multiplier = 1;

	for (size_t i = 0; i < lengthWithoutExtension; i++) {
		size_t charPos = lengthWithoutExtension - 1 - i;
		char c = fileName[charPos];

		if (c >= '0' && c <= '9') {
			int digit = c - '0';
			segmentID += digit * multiplier;
			multiplier *= 10;
			numDigits++;
		} else {
			break;
		}
	}

	if (numDigits == 0)
		error("Unusual segment naming scheme");

	return segmentID;
}

static void loadCursorsMac(FileIdentification &f, CursorGraphicCollection &cursorGraphics) {
	initResManForFile(f);

	const uint32 bwType = MKTAG('C', 'U', 'R', 'S');
	const uint32 colorType = MKTAG('c', 'r', 's', 'r');

	Common::MacResIDArray bwIDs = f.resMan->getResIDArray(bwType);
	Common::MacResIDArray colorIDs = f.resMan->getResIDArray(colorType);

	Common::MacResIDArray bwOnlyIDs;
	for (Common::MacResIDArray::const_iterator bwIt = bwIDs.begin(), bwItEnd = bwIDs.end(); bwIt != bwItEnd; ++bwIt) {
		bool hasColor = false;
		for (Common::MacResIDArray::const_iterator colorIt = colorIDs.begin(), colorItEnd = colorIDs.end(); colorIt != colorItEnd; ++colorIt) {
			if ((*colorIt) == (*bwIt)) {
				hasColor = true;
				break;
			}
		}

		if (!hasColor)
			bwOnlyIDs.push_back(*bwIt);
	}

	int numCursorsLoaded = 0;
	for (int cti = 0; cti < 2; cti++) {
		const uint32 resType = (cti == 0) ? bwType : colorType;
		const bool isBW = (cti == 0);
		const Common::MacResIDArray &resArray = (cti == 0) ? bwOnlyIDs : colorIDs;

		for (size_t i = 0; i < resArray.size(); i++) {
			Common::SharedPtr<Common::SeekableReadStream> resData(f.resMan->getResource(resType, resArray[i]));
			if (!resData) {
				warning("Failed to open cursor resource");
				return;
			}

			Common::SharedPtr<Graphics::MacCursor> cursor(new Graphics::MacCursor());
			// Some CURS resources are 72 bytes instead of the expected 68, make sure they load as the correct format
			if (!cursor->readFromStream(*resData, isBW, 0xff, isBW)) {
				warning("Failed to load cursor resource");
				return;
			}

			cursorGraphics.addMacCursor(resArray[i], cursor);
			numCursorsLoaded++;
		}
	}

	if (numCursorsLoaded == 0) {
		// If an extension is in detection, it should either have cursors or be categorized as Special if it has some other use.
		warning("Expected to find cursors in '%s' but there were none.", f.fileName.c_str());
	}
}

static bool loadCursorsWin(FileIdentification &f, CursorGraphicCollection &cursorGraphics) {
	Common::SharedPtr<Common::SeekableReadStream> stream = f.stream;

	if (!stream) {
		Common::SharedPtr<Common::File> file(new Common::File());
		if (!file->open(f.fileName))
			error("Failed to open file '%s'", f.fileName.c_str());

		stream = file;
	}

	Common::SharedPtr<Common::WinResources> winRes(Common::WinResources::createFromEXE(stream.get()));
	if (!winRes) {
		warning("Couldn't load resources from PE file");
		return false;
	}

	int numCursorGroupsLoaded = 0;
	Common::Array<Common::WinResourceID> cursorGroupIDs = winRes->getIDList(Common::kWinGroupCursor);
	for (Common::Array<Common::WinResourceID>::const_iterator it = cursorGroupIDs.begin(), itEnd = cursorGroupIDs.end(); it != itEnd; ++it) {
		const Common::WinResourceID &id = *it;

		Common::SharedPtr<Graphics::WinCursorGroup> cursorGroup(Graphics::WinCursorGroup::createCursorGroup(winRes.get(), *it));
		if (!winRes) {
			warning("Couldn't load cursor group");
			return false;
		}

		if (cursorGroup->cursors.size() == 0) {
			// Empty?
			continue;
		}

		cursorGraphics.addWinCursorGroup(id.getID(), cursorGroup);
		numCursorGroupsLoaded++;
	}

	if (numCursorGroupsLoaded == 0) {
		// If an extension is in detection, it should either have cursors or be categorized as Special if it has some other use.
		warning("Expected to find cursors in '%s' but there were none.", f.fileName.c_str());
	}

	return true;
}

} // namespace Boot

Common::SharedPtr<ProjectDescription> bootProject(const MTropolisGameDescription &gameDesc) {
	Common::SharedPtr<ProjectDescription> desc;

	Common::Array<Common::SharedPtr<ProjectPersistentResource>> persistentResources;

	Common::SharedPtr<Boot::GameDataHandler> gameDataHandler;

	switch (gameDesc.gameID) {
	case GID_OBSIDIAN:
		gameDataHandler.reset(new Boot::ObsidianGameDataHandler(gameDesc));
		break;
	default:
		gameDataHandler.reset(new Boot::GameDataHandler());
		break;
	}

	if (gameDesc.desc.platform == Common::kPlatformMacintosh) {
		Common::Array<Boot::FileIdentification> macFiles;

		debug(1, "Attempting to boot Macintosh game...");

		const ADGameFileDescription *fileDesc = gameDesc.desc.filesDescriptions;
		while (fileDesc->fileName) {
			const char *fileName = fileDesc->fileName;
			
			Boot::FileIdentification ident;
			ident.fileName = fileName;
			ident.category = Boot::kFileCategoryUnknown;
			if (!Boot::getMacTypesForFile(fileName, ident.macType, ident.macCreator))
				error("Couldn't determine Mac file type code for file '%s'", fileName);

			macFiles.push_back(ident);

			fileDesc++;
		}

		gameDataHandler->unpackAdditionalFiles(persistentResources, macFiles);
		gameDataHandler->categorizeSpecialFiles(macFiles);

		Common::sort(macFiles.begin(), macFiles.end(), Boot::fileSortCompare);

		// File types changed in mTropolis 2.0 in a way that MFmx and MFxm have different meaning than mTropolis 1.0.
		// So, we need to detect what variety of files we have available:
		// MT1 Mac: MFmm[+MFmx]
		// MT2 Mac: MFmm[+MFxm]
		// MT2 Cross: MFmx[+MFxx]
		bool haveAnyMFmm = false;
		bool haveAnyMFmx = false;
		//bool haveAnyMFxx = false; // Unused
		bool haveAnyMFxm = false;

		for (Boot::FileIdentification &macFile : macFiles) {
			if (macFile.category == Boot::kFileCategoryUnknown) {
				switch (macFile.macType) {
				case MKTAG('M', 'F', 'm', 'm'):
					haveAnyMFmm = true;
					break;
				case MKTAG('M', 'F', 'm', 'x'):
					haveAnyMFmx = true;
					break;
				case MKTAG('M', 'F', 'x', 'm'):
					haveAnyMFxm = true;
					break;
				case MKTAG('M', 'F', 'x', 'x'):
					//haveAnyMFxx = true; // Unused
					break;
				default:
					break;
				};
			}
		}

		bool isMT2CrossPlatform = (haveAnyMFmx && !haveAnyMFmm);
		if (isMT2CrossPlatform && haveAnyMFxm)
			error("Unexpected combination of player file types");		

		// Identify unknown files
		for (Boot::FileIdentification &macFile : macFiles) {
			if (macFile.category == Boot::kFileCategoryUnknown) {
				switch (macFile.macType) {
				case MKTAG('M', 'F', 'm', 'm'):
					macFile.category = Boot::kFileCategoryProjectMainSegment;
					break;
				case MKTAG('M', 'F', 'm', 'x'):
					macFile.category = isMT2CrossPlatform ? Boot::kFileCategoryProjectMainSegment : Boot::kFileCategoryProjectAdditionalSegment;
					break;
				case MKTAG('M', 'F', 'x', 'm'):
				case MKTAG('M', 'F', 'x', 'x'):
					macFile.category = Boot::kFileCategoryProjectAdditionalSegment;
					break;
				case MKTAG('A', 'P', 'P', 'L'):
					macFile.category = Boot::kFileCategoryPlayer;
					break;
				case MKTAG('M', 'F', 'c', 'o'):
				case MKTAG('M', 'F', 'c', 'r'):
				case MKTAG('M', 'F', 'X', 'O'):
					macFile.category = Boot::kFileCategoryExtension;
					break;
				default:
					error("Failed to categorize input file '%s'", macFile.fileName.c_str());
					break;
				};
			}
		}

		Boot::FileIdentification *mainSegmentFile = nullptr;
		Common::Array<Boot::FileIdentification *> segmentFiles;

		// Bin segments
		for (Boot::FileIdentification &macFile : macFiles) {
			switch (macFile.category) {
			case Boot::kFileCategoryPlayer:
				// Case handled below after cursor loading
				break;
			case Boot::kFileCategoryExtension:
				// Case handled below after cursor loading
				break;
			case Boot::kFileCategoryProjectMainSegment:
				mainSegmentFile = &macFile;
				break;
			case Boot::kFileCategoryProjectAdditionalSegment: {
					int segmentID = Boot::resolveFileSegmentID(macFile.fileName);
					if (segmentID < 2)
						error("Unusual segment numbering scheme");

					size_t segmentIndex = static_cast<size_t>(segmentID - 1);
					while (segmentFiles.size() <= segmentIndex)
						segmentFiles.push_back(nullptr);
					segmentFiles[segmentIndex] = &macFile;
				} break;
			case Boot::kFileCategorySpecial:
				break;
			case Boot::kFileCategoryUnknown:
				break;
			}
		}

		if (segmentFiles.size() > 0)
			segmentFiles[0] = mainSegmentFile;
		else
			segmentFiles.push_back(mainSegmentFile);

		// Load cursors
		Common::SharedPtr<CursorGraphicCollection> cursorGraphics(new CursorGraphicCollection());

		for (Boot::FileIdentification &macFile : macFiles) {
			if (macFile.category == Boot::kFileCategoryPlayer)
				Boot::loadCursorsMac(macFile, *cursorGraphics);
		}
		
		for (Boot::FileIdentification &macFile : macFiles) {
			if (macFile.category == Boot::kFileCategoryExtension)
				Boot::loadCursorsMac(macFile, *cursorGraphics);
		}

		// Create the project description
		desc.reset(new ProjectDescription(isMT2CrossPlatform ? KProjectPlatformCrossPlatform : kProjectPlatformMacintosh));

		for (Boot::FileIdentification *segmentFile : segmentFiles) {
			if (!segmentFile)
				error("Missing segment file");

			Common::SharedPtr<Common::SeekableReadStream> dataFork;

			if (segmentFile->stream)
				dataFork = segmentFile->stream;
			else {
				Boot::initResManForFile(*segmentFile);
				dataFork.reset(segmentFile->resMan->getDataFork());
				if (!dataFork)
					error("Segment file '%s' has no data fork", segmentFile->fileName.c_str());

				persistentResources.push_back(Boot::PersistentResource<Common::MacResManager>::wrap(segmentFile->resMan));
			}

			persistentResources.push_back(Boot::PersistentResource<Common::SeekableReadStream>::wrap(dataFork));

			desc->addSegment(0, dataFork.get());
		}

		gameDataHandler->addPlugIns(*desc, macFiles);

		desc->setCursorGraphics(cursorGraphics);
	} else if (gameDesc.desc.platform == Common::kPlatformWindows) {
		Common::Array<Boot::FileIdentification> winFiles;

		debug(1, "Attempting to boot Windows game...");

		const ADGameFileDescription *fileDesc = gameDesc.desc.filesDescriptions;
		while (fileDesc->fileName) {
			const char *fileName = fileDesc->fileName;

			Boot::FileIdentification ident;
			ident.fileName = fileName;
			ident.category = Boot::kFileCategoryUnknown;
			ident.macType = 0;
			ident.macCreator = 0;
			winFiles.push_back(ident);

			fileDesc++;
		}

		gameDataHandler->unpackAdditionalFiles(persistentResources, winFiles);
		gameDataHandler->categorizeSpecialFiles(winFiles);

		Common::sort(winFiles.begin(), winFiles.end(), Boot::fileSortCompare);

		bool isCrossPlatform = false;
		bool isWindows = false;
		bool isMT1 = false;
		bool isMT2 = false;

		// Identify unknown files
		for (Boot::FileIdentification &winFile : winFiles) {
			if (winFile.category == Boot::kFileCategoryUnknown) {
				switch (Boot::getWinFileEndingPseudoTag(winFile.fileName)) {
				case MKTAG('.', 'm', 'p', 'l'):
					winFile.category = Boot::kFileCategoryProjectMainSegment;
					isWindows = true;
					isMT1 = true;
					if (isMT2)
						error("Unexpected mix of file platforms");
					break;
				case MKTAG('.', 'm', 'p', 'x'):
					winFile.category = Boot::kFileCategoryProjectAdditionalSegment;
					isWindows = true;
					isMT1 = true;
					if (isMT2)
						error("Unexpected mix of file platforms");
					break;

				case MKTAG('.', 'm', 'f', 'w'):
					winFile.category = Boot::kFileCategoryProjectMainSegment;
					if (isMT1 || isCrossPlatform)
						error("Unexpected mix of file platforms");
					isWindows = true;
					isMT2 = true;
					break;

				case MKTAG('.', 'm', 'x', 'w'):
					winFile.category = Boot::kFileCategoryProjectAdditionalSegment;
					if (isMT1 || isCrossPlatform)
						error("Unexpected mix of file platforms");
					isWindows = true;
					isMT2 = true;
					break;

				case MKTAG('.', 'm', 'f', 'x'):
					winFile.category = Boot::kFileCategoryProjectMainSegment;
					if (isWindows)
						error("Unexpected mix of file platforms");
					isCrossPlatform = true;
					isMT2 = true;
					break;

				case MKTAG('.', 'm', 'x', 'x'):
					winFile.category = Boot::kFileCategoryProjectAdditionalSegment;
					if (isWindows)
						error("Unexpected mix of file platforms");
					isCrossPlatform = true;
					isMT2 = true;
					break;

				case MKTAG('.', 'c', '9', '5'):
				case MKTAG('.', 'e', '9', '5'):
				case MKTAG('.', 'r', '9', '5'):
					winFile.category = Boot::kFileCategoryExtension;
					break;

				case MKTAG('.', 'e', 'x', 'e'):
					winFile.category = Boot::kFileCategoryPlayer;
					break;

				default:
					error("Failed to categorize input file '%s'", winFile.fileName.c_str());
					break;
				};
			}
		}

		Boot::FileIdentification *mainSegmentFile = nullptr;
		Common::Array<Boot::FileIdentification *> segmentFiles;

		// Bin segments
		for (Boot::FileIdentification &macFile : winFiles) {
			switch (macFile.category) {
			case Boot::kFileCategoryPlayer:
				// Case handled below after cursor loading
				break;
			case Boot::kFileCategoryExtension:
				// Case handled below after cursor loading
				break;
			case Boot::kFileCategoryProjectMainSegment:
				mainSegmentFile = &macFile;
				break;
			case Boot::kFileCategoryProjectAdditionalSegment: {
				int segmentID = Boot::resolveFileSegmentID(macFile.fileName);
				if (segmentID < 2)
					error("Unusual segment numbering scheme");

				size_t segmentIndex = static_cast<size_t>(segmentID - 1);
				while (segmentFiles.size() <= segmentIndex)
					segmentFiles.push_back(nullptr);
				segmentFiles[segmentIndex] = &macFile;
			} break;
			case Boot::kFileCategorySpecial:
				break;
			case Boot::kFileCategoryUnknown:
				break;
			}
		}

		if (segmentFiles.size() > 0)
			segmentFiles[0] = mainSegmentFile;
		else
			segmentFiles.push_back(mainSegmentFile);

		// Load cursors
		Common::SharedPtr<CursorGraphicCollection> cursorGraphics(new CursorGraphicCollection());

		for (Boot::FileIdentification &winFile : winFiles) {
			if (winFile.category == Boot::kFileCategoryPlayer)
				Boot::loadCursorsWin(winFile, *cursorGraphics);
		}

		for (Boot::FileIdentification &winFile : winFiles) {
			if (winFile.category == Boot::kFileCategoryExtension)
				Boot::loadCursorsWin(winFile, *cursorGraphics);
		}

		// Create the project description
		desc.reset(new ProjectDescription(isCrossPlatform ? KProjectPlatformCrossPlatform : kProjectPlatformWindows));

		for (Boot::FileIdentification *segmentFile : segmentFiles) {
			if (!segmentFile)
				error("Missing segment file");

			if (segmentFile->stream) {
				persistentResources.push_back(Boot::PersistentResource<Common::SeekableReadStream>::wrap(segmentFile->stream));
				desc->addSegment(0, segmentFile->stream.get());
			} else {
				desc->addSegment(0, segmentFile->fileName.c_str());
			}
		}

		gameDataHandler->addPlugIns(*desc, winFiles);

		desc->setCursorGraphics(cursorGraphics);
	}

	Common::SharedPtr<ProjectResources> resources(new ProjectResources());
	resources->persistentResources = persistentResources;

	desc->setResources(resources);

	return desc;
}

} // End of namespace MTropolis
