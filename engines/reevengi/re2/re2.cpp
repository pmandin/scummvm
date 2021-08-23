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

#include "common/debug.h"
#include "common/file.h"
#include "common/memstream.h"
#include "common/stream.h"

#include "engines/reevengi/formats/adt.h"
#include "engines/reevengi/formats/bss.h"
#include "engines/reevengi/formats/bss_sld.h"
#include "engines/reevengi/re2/re2.h"
#include "engines/reevengi/re2/entity.h"
#include "engines/reevengi/re2/entity_emd.h"
#include "engines/reevengi/re2/entity_pld.h"
#include "engines/reevengi/re2/room.h"

namespace Reevengi {

/*--- Constant ---*/

static const char *RE2_ROOM = "pl%d/rd%c/room%d%02x%d.rdt";

static const char *RE2PCDEMO_BG = "common/stage%d/rc%d%02x%1x.adt";
static const char *RE2PCDEMO_BGMASK = "common/stage%d/rs%d%02x%1x.adt";

static const char *RE2PSX_BG = "common/bss/room%d%02x.bss";

static const char *RE2PC_MODEL1 = "pl%d/pld/pl%02x.pld";
static const char *RE2PC_MODEL2 = "pl%d/emd%d/em%d%02x.%s";

static const char *RE2PSX_MODEL1 = "pl%d/pld/pl%02x.pld";

RE2Engine::RE2Engine(OSystem *syst, ReevengiGameType gameType, const ADGameDescription *desc) :
		ReevengiEngine(syst, gameType, desc), _country('u') {
	if (gameType == RType_RE2_CLAIRE) {
		_character = 1;
	}

	/* Game B: start in room 4 */

	/* Demo: only stage 1 and 2 */
}

RE2Engine::~RE2Engine() {
}

void RE2Engine::initPreRun(void) {
	char filePath[64];

	/* Country detection */
	sprintf(filePath, RE2_ROOM, _character, 'p', 1, 0, _character);
	if (SearchMan.hasFile(filePath)) {
		_country = 'p';
	}
	sprintf(filePath, RE2_ROOM, _character, 's', 1, 0, _character);
	if (SearchMan.hasFile(filePath)) {
		_country = 's';
	}
	sprintf(filePath, RE2_ROOM, _character, 'f', 1, 0, _character);
	if (SearchMan.hasFile(filePath)) {
		_country = 'f';
	}
	sprintf(filePath, RE2_ROOM, _character, 't', 1, 0, _character);
	if (SearchMan.hasFile(filePath)) {
		_country = 't';
	}

}

void RE2Engine::loadBgImage(void) {
	//debug(3, "re2: loadBgImage");

	if ((_gameDesc.flags & ADGF_DEMO)==ADGF_DEMO) {
		if (_stage>2) { _stage=1; }
	}

	switch(_gameDesc.platform) {
		case Common::kPlatformWindows:
			{
				if ((_gameDesc.flags & ADGF_DEMO)==ADGF_DEMO) {
					loadBgImagePcDemo();
				} else {
					loadBgImagePcGame();
				}
			}
			break;
		case Common::kPlatformPSX:
			{
				loadBgImagePsx();
			}
			break;
		default:
			return;
	}

	ReevengiEngine::loadBgImage();
}

void RE2Engine::loadBgImagePcDemo(void) {
	char filePath[64];

	sprintf(filePath, RE2PCDEMO_BG, _stage, _stage, _room, _camera);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		_bgImage = new AdtDecoder();
		((AdtDecoder *) _bgImage)->loadStream(*stream);
	}
	delete stream;
}

void RE2Engine::loadBgImagePcGame(void) {
	int num_image, max_images;
	int32 archiveLen;
	uint32 streamPos[2], imageLen;
	byte *imgBuffer;

	num_image = (_stage-1)*512 + _room*16 + _camera;

	Common::SeekableReadStream *arcStream = SearchMan.createReadStreamForMember("common/bin/roomcut.bin");
	archiveLen = arcStream->size();

	max_images = arcStream->readUint32LE() >> 2;
	if (num_image<max_images) {
		arcStream->seek(num_image<<2);

		streamPos[0] = arcStream->readUint32LE();
		streamPos[1] = (arcStream->pos() == (max_images<<2) ? archiveLen : arcStream->readUint32LE());
		imageLen = streamPos[1]-streamPos[0];

		imgBuffer = new byte[imageLen];
		arcStream->seek(streamPos[0]);
		arcStream->read(imgBuffer, imageLen);

		Common::SeekableReadStream *imgStream = new Common::MemoryReadStream(imgBuffer, imageLen,
			DisposeAfterUse::YES
		);
		if (imgStream) {
			_bgImage = new AdtDecoder();
			((AdtDecoder *) _bgImage)->loadStream(*imgStream);
		}
		delete imgStream;
	}

	delete arcStream;
}

void RE2Engine::loadBgImagePsx(void) {
	char filePath[64];

	sprintf(filePath, RE2PSX_BG, _stage, _room);

	Common::SeekableReadStream *arcStream = SearchMan.createReadStreamForMember(filePath);
	if (arcStream) {
		byte *imgBuffer = new byte[65536];
		memset(imgBuffer, 0, 65536);

		arcStream->seek(65536 * _camera);
		arcStream->read(imgBuffer, 65536);

		Common::BitStreamMemoryStream *imgStream = new Common::BitStreamMemoryStream(imgBuffer, 65536, DisposeAfterUse::YES);
		if (imgStream) {
			/* FIXME: Would be simpler to call PSXStreamDecoder::PSXVideoTrack::decodeFrame() on imgBuffer
			   instead of duplicating implementation in formats/bss.[cpp,h] */
			PSXVideoTrack *vidDecoder = new PSXVideoTrack(/*imgStream,*/ 1, 32);
			vidDecoder->decodeFrame(imgStream, 32);

			const Graphics::Surface *frame = vidDecoder->decodeNextFrame();
			if (frame) {
				Graphics::PixelFormat fmt;
				memcpy(&fmt, &(frame->format), sizeof(Graphics::PixelFormat));

				_bgImage = new TimDecoder();
				((TimDecoder *)_bgImage)->CreateTimSurface(frame->w, frame->h, fmt);

				const Graphics::Surface *dstFrame = _bgImage->getSurface();

				const byte *src = (const byte *) frame->getPixels();
				byte *dst = (byte *) dstFrame->getPixels();
				if (frame->pitch == dstFrame->pitch) {
					memcpy(dst, src, frame->h * frame->pitch);
				} else {
					for (int y = frame->h; y > 0; --y) {
						memcpy(dst, src, frame->w * fmt.bytesPerPixel);
						src += frame->pitch;
						dst += dstFrame->pitch;
					}
				}
			}

			delete vidDecoder;
		}
		delete imgStream;
	}
	delete arcStream;
}

void RE2Engine::loadBgMaskImage(void) {
	//debug(3, "re2: loadBgMaskImage");

	if ((_gameDesc.flags & ADGF_DEMO)==ADGF_DEMO) {
		if (_stage>2) { _stage=1; }
	}

	switch(_gameDesc.platform) {
		case Common::kPlatformWindows:
			{
				if ((_gameDesc.flags & ADGF_DEMO)==ADGF_DEMO) {
					loadBgMaskImagePcDemo();
				} else {
					loadBgMaskImagePcGame();
				}
			}
			break;
		case Common::kPlatformPSX:
			{
				loadBgMaskImagePsx();
			}
			break;
		default:
			return;
	}

	ReevengiEngine::loadBgMaskImage();
}

void RE2Engine::loadBgMaskImagePcDemo(void) {
	char filePath[64];

	sprintf(filePath, RE2PCDEMO_BGMASK, _stage, _stage, _room, _camera);
	debug(3, "re2: loadBgMaskImagePcDemo(\"%s\")", filePath);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		_bgMaskImage = new AdtDecoder();
		((AdtDecoder *) _bgMaskImage)->loadStream(*stream);
	}
	delete stream;
}

void RE2Engine::loadBgMaskImagePcGame(void) {
	int num_image, max_images;
	int32 archiveLen;
	uint32 streamPos[2], imageLen;
	byte *imgBuffer;

	num_image = (_stage-1)*512 + _room*16 + _camera;

	Common::SeekableReadStream *arcStream = SearchMan.createReadStreamForMember("common/bin/roomcut.bin");
	archiveLen = arcStream->size();

	max_images = arcStream->readUint32LE() >> 2;
	if (num_image<max_images) {
		arcStream->seek(num_image<<2);

		streamPos[0] = arcStream->readUint32LE();
		streamPos[1] = (arcStream->pos() == (max_images<<2) ? archiveLen : arcStream->readUint32LE());
		imageLen = streamPos[1]-streamPos[0];

		imgBuffer = new byte[imageLen];
		arcStream->seek(streamPos[0]);
		arcStream->read(imgBuffer, imageLen);

		Common::SeekableReadStream *imgStream = new Common::MemoryReadStream(imgBuffer, imageLen,
			DisposeAfterUse::YES
		);

		if (imgStream) {
			_bgMaskImage = new AdtDecoder();
			((AdtDecoder *) _bgMaskImage)->loadStreamNumber(*imgStream, 1);
		}
		delete imgStream;
	}

	delete arcStream;
}

void RE2Engine::loadBgMaskImagePsx(void) {
	char filePath[64];
	const char blkEnd[8]={0x01,0x00,0x00,0x00, 0x00,0x00,0x00,0x00};

	sprintf(filePath, RE2PSX_BG, _stage, _room);

	Common::SeekableReadStream *arcStream = SearchMan.createReadStreamForMember(filePath);
	if (arcStream) {
		byte *blkBuffer = new byte[65536];
		memset(blkBuffer, 0, 65536);

		arcStream->seek(65536 * _camera);
		arcStream->read(blkBuffer, 65536);

		/* Search markers for pointer to TIM image, start from end of block */
		/* Should be better if we could skip compressed size of PSX video stream image */
		unsigned int fileOffset = 65536-sizeof(blkEnd);
		bool fileFound = false;
		while (fileOffset>0) {
			if (memcmp(&blkBuffer[fileOffset], blkEnd, sizeof(blkEnd))==0) {
				fileFound = true;
				break;
			}
			--fileOffset;
		}

		if (fileFound) {
			/* Go back to start of header, offset.le32, count.le32 */
			unsigned int endFileOffset = fileOffset-4;

			/* Read compressed TIM offset for mask */
			fileOffset = FROM_LE_32( *((uint32 *) &blkBuffer[endFileOffset]) );

			int imageLen = endFileOffset-fileOffset;
			byte *imgBuffer = new byte[imageLen];
			memcpy(imgBuffer, &blkBuffer[fileOffset], imageLen);

			//debug(3, "re2: offset 0x%08x, length %d", fileOffset, imageLen);
/*
			Common::DumpFile adf;
			adf.open("re2comptim.bin");
			adf.write(imgBuffer, imageLen);
			adf.close();
*/
			Common::SeekableReadStream *imgStream = new Common::MemoryReadStream(imgBuffer, imageLen,
				DisposeAfterUse::YES
			);

			if (imgStream) {
				_bgMaskImage = new BssSldDecoder(2);
				((BssSldDecoder *) _bgMaskImage)->loadStream(*imgStream);
			}
			delete imgStream;
		}

		delete blkBuffer;
	}
	delete arcStream;
}

void RE2Engine::loadRoom(void) {
	char filePath[64];

	debug(3, "re2: loadRoom");

	sprintf(filePath, RE2_ROOM, _character, _country, _stage, _room, _character);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		_roomScene = new RE2Room(stream);
	}
	delete stream;
}

Entity *RE2Engine::loadEntity(int numEntity, int isPlayer) {
	Entity *newEntity = nullptr;

	debug(3, "re2: loadEntity(%d,%d)", numEntity, isPlayer);

	switch(_gameDesc.platform) {
		case Common::kPlatformWindows:
			{
				newEntity = loadEntityPc(numEntity, isPlayer);
			}
			break;
		case Common::kPlatformPSX:
			{
				newEntity = loadEntityPsx(numEntity, isPlayer);
			}
			break;
		default:
			break;
	}

	return newEntity;
}

Entity *RE2Engine::loadEntityPc(int numEntity, int isPlayer) {
	char filePath[64];
	Common::SeekableReadStream *stream;
	Entity *newEntity = nullptr;

	// Load EMD model

	if (isPlayer) {
		sprintf(filePath, RE2PC_MODEL1, _character, numEntity);
	} else {
		sprintf(filePath, RE2PC_MODEL2, _character, _character, _character, numEntity, "emd");
	}
	debug(3, "re2: loadEntityPc(\"%s\")", filePath);

	stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		if (isPlayer) {
			newEntity = (Entity *) new RE2EntityPld(stream);
		} else {
			newEntity = (Entity *) new RE2EntityEmd(stream);
		}
	}
	delete stream;

	// Load TIM texture

	if (isPlayer) {
		// TIM file embedded in pld file
	} else {
		sprintf(filePath, RE2PC_MODEL2, _character, _character, _character, numEntity, "tim");
		debug(3, "re2: loadEntityPc(\"%s\")", filePath);

		stream = SearchMan.createReadStreamForMember(filePath);
		if (newEntity && stream) {
			newEntity->timTexture = new TimDecoder();
			newEntity->timTexture->loadStream(*stream);
		}
		delete stream;
	}

	return newEntity;
}

Entity *RE2Engine::loadEntityPsx(int numEntity, int isPlayer) {
	char filePath[64];
	Common::SeekableReadStream *stream;
	Entity *newEntity = nullptr;

	// Load EMD model

	if (isPlayer) {
		sprintf(filePath, RE2PSX_MODEL1, _character, numEntity);
	} else {
		// TODO: Handle ems archive
		return nullptr;
	}
	debug(3, "re2: loadEntityPsx(\"%s\")", filePath);

	stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		if (isPlayer) {
			newEntity = (Entity *) new RE2EntityPld(stream);
		} else {
			// TODO: Handle ems archive
		}
	}
	delete stream;

	// Load TIM texture

	if (isPlayer) {
		// TIM file embedded in pld file
	} else {
		// TODO: Handle ems archive
	}

	return newEntity;
}

} // end of namespace Reevengi

