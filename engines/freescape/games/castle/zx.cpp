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

#include "common/file.h"

#include "freescape/freescape.h"
#include "freescape/games/castle/castle.h"
#include "freescape/language/8bitDetokeniser.h"

namespace Freescape {

void CastleEngine::initZX() {
	_viewArea = Common::Rect(64, 36, 256, 148);
	_yminValue = -1;
	_ymaxValue = 1;

	_soundIndexShoot = 5;
	_soundIndexCollide = -1;
	_soundIndexFall = -1;
	_soundIndexClimb = -1;
	_soundIndexMenu = -1;
	_soundIndexStart = 6;
	_soundIndexAreaChange = 5;
}

Graphics::ManagedSurface *CastleEngine::loadFrameWithHeader(Common::SeekableReadStream *file, int pos, uint32 front, uint32 back) {
	Graphics::ManagedSurface *surface = new Graphics::ManagedSurface();
	file->seek(pos);
	int16 width = file->readByte();
	int16 height = file->readByte();
	surface->create(width * 8, height, _gfx->_texturePixelFormat);

	/*byte mask =*/ file->readByte();

	surface->fillRect(Common::Rect(0, 0, width * 8, height), back);
	/*int frameSize =*/ file->readUint16LE();
	return loadFrame(file, surface, width, height, front);
}

Common::Array<Graphics::ManagedSurface *> CastleEngine::loadFramesWithHeader(Common::SeekableReadStream *file, int pos, int numFrames, uint32 front, uint32 back) {
	Graphics::ManagedSurface *surface = nullptr;
	file->seek(pos);
	int16 width = file->readByte();
	int16 height = file->readByte();
	/*byte mask =*/ file->readByte();

	/*int frameSize =*/ file->readUint16LE();
	Common::Array<Graphics::ManagedSurface *> frames;
	for (int i = 0; i < numFrames; i++) {
		surface = new Graphics::ManagedSurface();
		surface->create(width * 8, height, _gfx->_texturePixelFormat);
		surface->fillRect(Common::Rect(0, 0, width * 8, height), back);
		frames.push_back(loadFrame(file, surface, width, height, front));
	}

	return frames;
}


Graphics::ManagedSurface *CastleEngine::loadFrame(Common::SeekableReadStream *file, Graphics::ManagedSurface *surface, int width, int height, uint32 front) {
	for (int i = 0; i < width * height; i++) {
		byte color = file->readByte();
		for (int n = 0; n < 8; n++) {
			int y = i / width;
			int x = (i % width) * 8 + (7 - n);
			if ((color & (1 << n)))
				surface->setPixel(x, y, front);
		}
	}
	return surface;
}

void CastleEngine::loadAssetsZXFullGame() {
	Common::File file;
	uint8 r, g, b;

	file.open("castlemaster.zx.title");
	if (file.isOpen()) {
		_title = loadAndCenterScrImage(&file);
	} else
		error("Unable to find castlemaster.zx.title");

	file.close();
	file.open("castlemaster.zx.border");
	if (file.isOpen()) {
		_border = loadAndCenterScrImage(&file);
	} else
		error("Unable to find castlemaster.zx.border");
	file.close();

	file.open("castlemaster.zx.data");
	if (!file.isOpen())
		error("Failed to open castlemaster.zx.data");

	loadMessagesVariableSize(&file, 0x4bd, 71);
	switch (_language) {
		case Common::ES_ESP:
			loadRiddles(&file, 0x1470 - 4 - 2 - 9 * 2, 9);
			loadMessagesVariableSize(&file, 0xf3d, 71);
			load8bitBinary(&file, 0x6aab - 2, 16);
			loadSpeakerFxZX(&file, 0xca0, 0xcdc);
			loadFonts(&file, 0x1218 + 16, _font);
			break;
		case Common::EN_ANY:
			loadRiddles(&file, 0x145c - 2 - 9 * 2, 9);
			load8bitBinary(&file, 0x6a3b, 16);
			loadSpeakerFxZX(&file, 0xc91, 0xccd);
			loadFonts(&file, 0x1219, _font);
			break;
		default:
			error("Language not supported");
			break;
	}

	loadColorPalette();
	_gfx->readFromPalette(2, r, g, b);
	uint32 red = _gfx->_texturePixelFormat.ARGBToColor(0xFF, r, g, b);

	_gfx->readFromPalette(7, r, g, b);
	uint32 white = _gfx->_texturePixelFormat.ARGBToColor(0xFF, r, g, b);

	_keysBorderFrames.push_back(loadFrameWithHeader(&file, 0xdf7, white, red));

	uint32 green = _gfx->_texturePixelFormat.ARGBToColor(0xFF, 0, 0xff, 0);
	_spiritsMeterIndicatorFrame = loadFrameWithHeader(&file, _language == Common::ES_ESP ? 0xe5e : 0xe4f, green, white);

	_gfx->readFromPalette(4, r, g, b);
	uint32 front = _gfx->_texturePixelFormat.ARGBToColor(0xFF, r, g, b);

	int backgroundWidth = 16;
	int backgroundHeight = 18;
	Graphics::ManagedSurface *background = new Graphics::ManagedSurface();
	background->create(backgroundWidth * 8, backgroundHeight, _gfx->_texturePixelFormat);
	background->fillRect(Common::Rect(0, 0, backgroundWidth * 8, backgroundHeight), 0);

	file.seek(_language == Common::ES_ESP ? 0xfd3 : 0xfc4);
	_background = loadFrame(&file, background, backgroundWidth, backgroundHeight, front);

	_gfx->readFromPalette(6, r, g, b);
	uint32 yellow = _gfx->_texturePixelFormat.ARGBToColor(0xFF, r, g, b);
	uint32 black = _gfx->_texturePixelFormat.ARGBToColor(0xFF, 0, 0, 0);
	_strenghtBackgroundFrame = loadFrameWithHeader(&file, _language == Common::ES_ESP ? 0xee6 : 0xed7, yellow, black);
	_strenghtBarFrame = loadFrameWithHeader(&file, _language == Common::ES_ESP ? 0xf72 : 0xf63, yellow, black);

	Graphics::ManagedSurface *bar = new Graphics::ManagedSurface();
	bar->create(_strenghtBarFrame->w - 4, _strenghtBarFrame->h, _gfx->_texturePixelFormat);
	_strenghtBarFrame->copyRectToSurface(*bar, 4, 0, Common::Rect(4, 0, _strenghtBarFrame->w - 4, _strenghtBarFrame->h));
	_strenghtBarFrame->free();
	delete _strenghtBarFrame;
	_strenghtBarFrame = bar;

	_strenghtWeightsFrames = loadFramesWithHeader(&file, _language == Common::ES_ESP ? 0xf92 : 0xf83, 4, yellow, black);

	_flagFrames = loadFramesWithHeader(&file, 0x10e4, 4, green, black);

	int thunderWidth = 4;
	int thunderHeight = 43;
	_thunderFrame = new Graphics::ManagedSurface();
	_thunderFrame->create(thunderWidth * 8, thunderHeight, _gfx->_texturePixelFormat);
	_thunderFrame->fillRect(Common::Rect(0, 0, thunderWidth * 8, thunderHeight), 0);
	_thunderFrame = loadFrame(&file, _thunderFrame, thunderWidth, thunderHeight, front);

	_riddleTopFrame = new Graphics::ManagedSurface(loadBundledImage("castle_riddle_top_frame"));
	_riddleTopFrame->convertToInPlace(_gfx->_texturePixelFormat);

	_riddleBackgroundFrame = new Graphics::ManagedSurface(loadBundledImage("castle_riddle_background_frame"));
	_riddleBackgroundFrame->convertToInPlace(_gfx->_texturePixelFormat);

	_riddleBottomFrame = new Graphics::ManagedSurface(loadBundledImage("castle_riddle_bottom_frame"));
	_riddleBottomFrame->convertToInPlace(_gfx->_texturePixelFormat);

	for (auto &it : _areaMap) {
		it._value->addStructure(_areaMap[255]);

		it._value->addObjectFromArea(164, _areaMap[255]);
		it._value->addObjectFromArea(174, _areaMap[255]);
		for (int16 id = 136; id < 140; id++) {
			it._value->addObjectFromArea(id, _areaMap[255]);
		}

		for (int16 id = 214; id < 228; id++) {
			it._value->addObjectFromArea(id, _areaMap[255]);
		}
	}
	addGhosts();
	_areaMap[1]->addFloor();
	_areaMap[2]->addFloor();

	// Discard the first three global conditions
	// It is unclear why they hide/unhide objects that formed the spirits
	for (int i = 0; i < 3; i++) {
		debugC(kFreescapeDebugParser, "Discarding condition %s", _conditionSources[0].c_str());
		_conditions.remove_at(0);
		_conditionSources.remove_at(0);
	}

	_timeoutMessage = _messagesList[1];
	// Shield is unused in Castle Master
	_noEnergyMessage = _messagesList[1];
	_fallenMessage = _messagesList[4];
	_crushedMessage = _messagesList[3];
	_outOfReachMessage = _messagesList[7];
	_noEffectMessage = _messagesList[8];
}

void CastleEngine::drawZXUI(Graphics::Surface *surface) {
	uint32 color = 5;
	uint8 r, g, b;

	_gfx->readFromPalette(color, r, g, b);
	uint32 front = _gfx->_texturePixelFormat.ARGBToColor(0xFF, r, g, b);

	color = 0;
	_gfx->readFromPalette(color, r, g, b);
	uint32 black = _gfx->_texturePixelFormat.ARGBToColor(0xFF, r, g, b);

	Common::Rect backRect(123, 179, 242 + 5, 188);
	surface->fillRect(backRect, black);

	Common::String message;
	int deadline;
	getLatestMessages(message, deadline);
	if (deadline <= _countdown) {
		//debug("deadline: %d countdown: %d", deadline, _countdown);
		drawStringInSurface(message, 120, 179, front, black, surface);
		_temporaryMessages.push_back(message);
		_temporaryMessageDeadlines.push_back(deadline);
	} else
		drawStringInSurface(_currentArea->_name, 120, 179, front, black, surface);

	for (int k = 0; k < int(_keysCollected.size()); k++) {
		surface->copyRectToSurface((const Graphics::Surface)*_keysBorderFrames[0], 99 - k * 4, 177, Common::Rect(0, 0, 6, 11));
	}

	uint32 green = _gfx->_texturePixelFormat.ARGBToColor(0xFF, 0, 0xff, 0);

	surface->fillRect(Common::Rect(152, 156, 216, 164), green);
	surface->copyRectToSurface((const Graphics::Surface)*_spiritsMeterIndicatorFrame, 140 + _spiritsMeterPosition, 156, Common::Rect(0, 0, 15, 8));
	drawEnergyMeter(surface, Common::Point(63, 154));

	int flagFrameIndex = (_ticks / 10) % 4;
	surface->copyRectToSurface(*_flagFrames[flagFrameIndex], 264, 9, Common::Rect(0, 0, _flagFrames[flagFrameIndex]->w, _flagFrames[flagFrameIndex]->h));
}

} // End of namespace Freescape
