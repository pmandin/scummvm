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

#include "common/archive.h"
#include "common/debug.h"
#include "common/memstream.h"
#include "common/stream.h"
#include "image/jpeg.h"

#include "engines/reevengi/formats/ard.h"
#include "engines/reevengi/formats/bss.h"
#include "engines/reevengi/formats/bss_sld.h"
#include "engines/reevengi/formats/rofs.h"
#include "engines/reevengi/formats/sld.h"
#include "engines/reevengi/formats/tim.h"
#include "engines/reevengi/re3/re3.h"
#include "engines/reevengi/re3/room.h"

namespace Reevengi {

/*--- Constant ---*/

static const char *RE3PCROFS_DAT = "rofs%d.dat";
static const char *RE3PC_BG = "data_a/bss/r%d%02x%02x.jpg";
static const char *RE3PC_BGMASK = "data_a/bss/r%d%02x.sld";
static const char *RE3PC_ROOM = "data_%c/rdt/r%d%02x.rdt";

static const char *RE3PSX_BG = "cd_data/stage%d/r%d%02x.bss";
static const char *RE3PSX_ROOM = "cd_data/stage%d/r%d%02x.ard";

RE3Engine::RE3Engine(OSystem *syst, ReevengiGameType gameType, const ADGameDescription *desc) :
		ReevengiEngine(syst, gameType, desc), _country('u') {
	_room = 13;
}

RE3Engine::~RE3Engine() {
}

void RE3Engine::initPreRun(void) {
	char filePath[32];

	switch(_gameDesc.platform) {
		case Common::kPlatformWindows:
			{
				_gameDesc.flags &= ~ADGF_DEMO;

				/* Use all ROFS<n>.DAT files as archives */
				for (int i=1;i<16;i++) {
					sprintf(filePath, RE3PCROFS_DAT, i);

					RofsArchive *archive = new RofsArchive();
					if (archive->open(filePath))
						SearchMan.add(filePath, archive, 0, true);
					else {
						if (i==2) {
							/* no rofs2.dat for demo */
							_gameDesc.flags |= ADGF_DEMO;
						}
						delete archive;
					}
				}

				if (SearchMan.hasFile("data_e/etc2/died00e.tim")) {
					_country = 'e';
				}
				if (SearchMan.hasFile("data_f/etc2/died00f.tim")) {
					_country = 'f';
				}
			}
			break;
		case Common::kPlatformPSX:
			{
			}
			break;
		default:
			break;
	}
}

void RE3Engine::loadBgImage(void) {
	//debug(3, "re3: loadBgImage");

	switch(_gameDesc.platform) {
		case Common::kPlatformWindows:
			{
				loadBgImagePc();
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

void RE3Engine::loadBgImagePc(void) {
	char filePath[64];

	sprintf(filePath, RE3PC_BG, _stage, _room, _camera);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		_bgImage = new Image::JPEGDecoder();
		((Image::JPEGDecoder *) _bgImage)->loadStream(*stream);
	}
	delete stream;
}

void RE3Engine::loadBgImagePsx(void) {
	char filePath[64];

	sprintf(filePath, RE3PSX_BG, _stage, _stage, _room);

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

void RE3Engine::loadBgMaskImage(void) {
//	debug(3, "re3: loadBgMaskImage");

	switch(_gameDesc.platform) {
		case Common::kPlatformWindows:
			{
				loadBgMaskImagePc();
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

void RE3Engine::loadBgMaskImagePc(void) {
	char filePath[64];

	sprintf(filePath, RE3PC_BGMASK, _stage, _room);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		SldArchive *sldArchive = new SldArchive(stream);
		if (sldArchive) {
			Common::SeekableReadStream *imgStream = sldArchive->createReadStreamForMember(_camera);
			if (imgStream) {
				_bgMaskImage = new TimDecoder();
				((TimDecoder *) _bgMaskImage)->loadStream(*imgStream);
			}
			delete imgStream;
		}
		delete sldArchive;
	}
	delete stream;
}

void RE3Engine::loadBgMaskImagePsx(void) {
	char filePath[64];
	const char blkEnd[8]={0x01,0x00,0x00,0x00, 0x00,0x00,0x00,0x00};

	sprintf(filePath, RE3PSX_BG, _stage, _stage, _room);

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

			//debug(3, "re3: offset 0x%08x, length %d", fileOffset, imageLen);
/*
			Common::DumpFile adf;
			adf.open("re3comptim.bin");
			adf.write(imgBuffer, imageLen);
			adf.close();
*/
			Common::SeekableReadStream *imgStream = new Common::MemoryReadStream(imgBuffer, imageLen,
				DisposeAfterUse::YES
			);

			if (imgStream) {
				_bgMaskImage = new BssSldDecoder(3);
				((BssSldDecoder *) _bgMaskImage)->loadStream(*imgStream);
			}
			delete imgStream;
		}

		delete blkBuffer;
	}
	delete arcStream;
}

void RE3Engine::loadRoom(void) {
	//debug(3, "re3: loadRoom");

	switch(_gameDesc.platform) {
		case Common::kPlatformWindows:
			{
				loadRoomPc();
			}
			break;
		case Common::kPlatformPSX:
			{
				loadRoomPsx();
			}
			break;
		default:
			break;
	}
}

void RE3Engine::loadRoomPc(void) {
	char filePath[64];

	sprintf(filePath, RE3PC_ROOM, _country, _stage, _room);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		_roomScene = new RE3Room(stream);
	}
	delete stream;
}

void RE3Engine::loadRoomPsx(void) {
	char filePath[64];

	sprintf(filePath, RE3PSX_ROOM, _stage, _stage, _room);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		ArdArchive *ard = new ArdArchive(stream);
		if (ard) {
			Common::SeekableReadStream *rdtStream = ard->createReadStreamForMember(ArdArchive::kRdtFile);
			if (rdtStream) {
				_roomScene = new RE3Room(rdtStream);
			}
			delete rdtStream;
		}
		delete ard;
	}
	delete stream;
}

} // end of namespace Reevengi

