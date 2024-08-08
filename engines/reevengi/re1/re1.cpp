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
#include "common/stream.h"

#include "engines/reevengi/detection.h"
#include "engines/reevengi/formats/pak.h"
#include "engines/reevengi/formats/bss.h"
#include "engines/reevengi/movie/movie.h"
#include "engines/reevengi/re1/re1.h"
#include "engines/reevengi/re1/entity.h"
#include "engines/reevengi/re1/room.h"

namespace Reevengi {

/*--- Defines ---*/

#define NUM_COUNTRIES 9

/*--- Constant ---*/

static const char *re1_country[NUM_COUNTRIES]={
	"horr/usa",
	"horr/ger",
	"horr/jpn",
	"horr/fra",
	"usa",
	"ger",
	"jpn",
	"fra",
	""
};

static const char *re1pc_movies[] = {
	"horr/usa/movie/capcom.avi",
	"horr/usa/movie/dm1.avi",
	"horr/usa/movie/dm2.avi",
	"horr/usa/movie/dm3.avi",
	"horr/usa/movie/dm4.avi",
	"horr/usa/movie/dm6.avi",
	"horr/usa/movie/dm7.avi",
	"horr/usa/movie/dm8.avi",
	"horr/usa/movie/dmb.avi",
	"horr/usa/movie/dmc.avi",
	"horr/usa/movie/dmd.avi",
	"horr/usa/movie/dme.avi",
	"horr/usa/movie/dmf.avi",
	"horr/usa/movie/ed1.avi",
	"horr/usa/movie/ed2.avi",
	"horr/usa/movie/ed3.avi",
	"horr/usa/movie/ed6.avi",
	"horr/usa/movie/ed7.avi",
	"horr/usa/movie/ed8.avi",
	"horr/usa/movie/eu4.avi",
	"horr/usa/movie/eu5.avi",
	"horr/usa/movie/ou.avi",
	"horr/usa/movie/pu.avi",
	"horr/usa/movie/staf_r.avi",
	"horr/usa/movie/stfc_r.avi",
	"horr/usa/movie/stfj_r.avi",
	"horr/usa/movie/stfz_r.avi"
};

static const char *re1ps1_movies[] = {
	"psx/movie/capcom.str",
	"psx/movie/dm1.str",
	"psx/movie/dm2.str",
	"psx/movie/dm3.str",
	"psx/movie/dm4.str",
	"psx/movie/dm6.str",
	"psx/movie/dm7.str",
	"psx/movie/dm8.str",
	"psx/movie/dmb.str",
	"psx/movie/dmc.str",
	"psx/movie/dmd.str",
	"psx/movie/dme.str",
	"psx/movie/dmf.str",
	"psx/movie/ed1.str",
	"psx/movie/ed2.str",
	"psx/movie/ed3.str",
	"psx/movie/ed4.str",
	"psx/movie/ed5.str",
	"psx/movie/ed6.str",
	"psx/movie/ed7.str",
	"psx/movie/ed8.str",
	"psx/movie/oj.str",
	"psx/movie/pj.str",
	"psx/movie/stfc.str",
	"psx/movie/stfj.str"
};

static const char *RE1_ROOM = "%s%s/stage%d/room%d%02x0.rdt";

static const char *RE1PCGAME_BG = "%s/stage%d/rc%d%02x%d.pak";

static const char *RE1PSX_BG = "psx%s/stage%d/room%d%02x.bss";

static const char *RE1_MODEL1 = "%s%s/enemy/char1%d.emd";
static const char *RE1_MODEL2 = "%s%s/enemy/em10%02x.emd";
static const char *RE1_MODEL3 = "%s%s/enemy/em11%02x.emd";

RE1Engine::RE1Engine(OSystem *syst, const ReevengiGameDescription *desc) :
		ReevengiEngine(syst, desc) {
	_room = 6;
	_camera = 6;

	_country = 8;

	/* Default entity */
	_defEntity = 0;
	_defIsPlayer = 1;
}

RE1Engine::~RE1Engine() {
}

void RE1Engine::initPreRun(void) {
	char filePath[32];

	/* Country detect */
	for (int i=0; i<NUM_COUNTRIES; i++) {
		snprintf(filePath, sizeof(filePath), "%s/data/capcom.ptc", re1_country[i]);

		if (SearchMan.hasFile(filePath)) {
			_country = i;
			debug(3, "re1: country %d", i);
			break;
		}
	}

	/* Dual shock USA ? */
	if (SearchMan.hasFile("slus_007.47")) {
		_country = 4;
	}
}

void RE1Engine::loadBgImage(void) {
	//debug(3, "re1: loadBgImage");

	/* Stages 6,7 use images from stages 1,2 */
	int stage = ((_stage % 8)>5 ? _stage-5 : _stage);

	if ((_gameDesc.flags & ADGF_DEMO)==ADGF_DEMO) {
		if (_stage>2) { _stage=1; }
	}

	/* Images for shaking scenes have 4 pixels less, force dimensions */
	int width = 320, height = 240;
	if (stage==2) {
		if (_room==0) {
			if (_camera==0) {
				width -= 4; height -= 4;
			}
		}
	} else if (stage==3) {
		if (_room==6) {
			if (_camera!=2) {
				width -= 4; height -= 4;
			}
		} else if (_room==7) {
			/* All cameras angles for this room */
			width -= 4; height -= 4;
		} else if (_room==0x0b) {
			/* All cameras angles for this room */
			width -= 4; height -= 4;
		} else if (_room==0x0f) {
			/* All cameras angles for this room */
			width -= 4; height -= 4;
		}
	} else if (stage==5) {
		if (_room==0x0d) {
			width -= 4; height -= 4;
		}
		if (_room==0x15) {
			width -= 4; height -= 4;
		}
	}

	switch(_gameDesc.platform) {
		case Common::kPlatformWindows:
			{
				loadBgImagePc(stage, width, height);
			}
			break;
		case Common::kPlatformPSX:
			{
				loadBgImagePsx(stage, width, height);
			}
			break;
		default:
			return;
	}

	ReevengiEngine::loadBgImage();
}

void RE1Engine::loadBgImagePc(int stage, int width, int height) {
	char filePath[64];

	snprintf(filePath, sizeof(filePath), RE1PCGAME_BG, re1_country[_country], stage, stage, _room, _camera);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		_bgImage = new PakDecoder();
		if ((width!=320) || (height!=240)) {
			((PakDecoder *) _bgImage)->setSize(width, height);
		}
		((PakDecoder *) _bgImage)->loadStream(*stream);
	}
	delete stream;
}

void RE1Engine::loadBgImagePsx(int stage, int width, int height) {
	char filePath[64];

	snprintf(filePath, sizeof(filePath), RE1PSX_BG, re1_country[_country], stage, stage, _room);

	Common::SeekableReadStream *arcStream = SearchMan.createReadStreamForMember(filePath);
	if (arcStream) {
		byte *imgBuffer = new byte[32768];
		memset(imgBuffer, 0, 32768);

		arcStream->seek(32768 * _camera);
		arcStream->read(imgBuffer, 32768);

		Common::BitStreamMemoryStream *imgStream = new Common::BitStreamMemoryStream(imgBuffer, 32768, DisposeAfterUse::YES);
		if (imgStream) {
			/* FIXME: Would be simpler to call PSXStreamDecoder::PSXVideoTrack::decodeFrame() on imgBuffer
			   instead of duplicating implementation in formats/bss.[cpp,h] */
			PSXVideoTrack *vidDecoder = new PSXVideoTrack(/*imgStream,*/ 1, 16);
			vidDecoder->decodeFrame(imgStream, 16);

			const Graphics::Surface *frame = vidDecoder->decodeNextFrame();
			if (frame) {
				Graphics::PixelFormat fmt;
				memcpy(&fmt, &(frame->format), sizeof(Graphics::PixelFormat));

				int frameW = width;
				int frameH = height;

				_bgImage = new TimDecoder();
				((TimDecoder *)_bgImage)->CreateTimSurface(frameW, frameH, fmt);

				const Graphics::Surface *dstFrame = _bgImage->getSurface();

				const byte *src = (const byte *) frame->getPixels();
				byte *dst = (byte *) dstFrame->getPixels();
				for (int y = frameH; y > 0; --y) {
					memcpy(dst, src, frameW * fmt.bytesPerPixel);
					src += /*frame->pitch*/ frameW * fmt.bytesPerPixel;
					dst += dstFrame->pitch;
				}
			}

			delete vidDecoder;
		}
		delete imgStream;
	}
	delete arcStream;
}

void RE1Engine::loadBgMaskImage(void) {
	//debug(3, "re1: loadBgMaskImage");

	if (_roomScene) {
		Common::SeekableReadStream *stream = ((RE1Room *)_roomScene)->getTimMask(_camera);
		if (stream) {
			_bgMaskImage = new TimDecoder();
			((TimDecoder *) _bgMaskImage)->loadStream(*stream);
		}
		delete stream;
	}

	ReevengiEngine::loadBgMaskImage();
}

void RE1Engine::loadRoom(void) {
	char filePath[64];
	bool isPsx = (_gameDesc.platform == Common::kPlatformPSX);

	snprintf(filePath, sizeof(filePath), RE1_ROOM, isPsx ? "psx" : "", re1_country[_country], _stage, _stage, _room);

	debug(3, "re1: loadRoom(\"%s\")", filePath);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		//debug(3, "loaded %s", filePath);
		_roomScene = new RE1Room(this, stream);
	}
	delete stream;
}

Entity *RE1Engine::loadEntity(int numEntity, int isPlayer) {
	char filePath[64];
	const char *filename = RE1_MODEL1;
	bool isPsx = (_gameDesc.platform == Common::kPlatformPSX);
	Entity *newEntity = nullptr;

	debug(3, "re1: loadEntity(%d, %d)", numEntity, isPlayer);

	if (!isPlayer) {
		filename = RE1_MODEL2;
		if (numEntity>=0x40) {
			filename = RE1_MODEL3;
			numEntity -= 0x40;
		}
	}

	snprintf(filePath, sizeof(filePath), filename, isPsx ? "psx" : "", re1_country[_country], numEntity);

	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);
	if (stream) {
		//debug(3, "re1: loaded %s", filePath);
		newEntity = (Entity *) new RE1Entity(stream);
	}
	delete stream;

	return newEntity;
}

void RE1Engine::loadMovie(unsigned int numMovie) {
	char filePath[64];
	bool isPsx = (_gameDesc.platform == Common::kPlatformPSX);

	ReevengiEngine::loadMovie(numMovie);

	if (isPsx) {
		// PS1
		if (numMovie >= sizeof(re1ps1_movies)) {
			return;
		}

		snprintf(filePath, sizeof(filePath), re1ps1_movies[numMovie]);

		g_movie = CreatePsxPlayer();
	} else {
		// PC
		if (numMovie >= sizeof(re1pc_movies)) {
			return;
		}

		snprintf(filePath, sizeof(filePath), re1pc_movies[numMovie]);

		g_movie = CreateAviPlayer();
	}

	debug(3, "re1: loadMovie(%d): %s", numMovie, filePath);
	g_movie->play(filePath, false, 0, 0);
}

} // end of namespace Reevengi

