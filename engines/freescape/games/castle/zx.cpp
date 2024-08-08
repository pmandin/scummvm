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
}

Graphics::Surface *CastleEngine::loadFramesWithHeader(Common::SeekableReadStream *file, int pos, int numFrames, uint32 back) {
	Graphics::Surface *surface = new Graphics::Surface();
	file->seek(pos);
	int16 width = file->readByte();
	int16 height = file->readByte();
	surface->create(width * 8, height, _gfx->_texturePixelFormat);

	/*byte mask =*/ file->readByte();

	uint8 r, g, b;
	_gfx->readFromPalette(7, r, g, b);
	uint32 white = _gfx->_texturePixelFormat.ARGBToColor(0xFF, r, g, b);

	surface->fillRect(Common::Rect(0, 0, width * 8, height), white);
	/*int frameSize =*/ file->readUint16LE();
	return loadFrames(file, surface, width, height, back);
}


Graphics::Surface *CastleEngine::loadFrames(Common::SeekableReadStream *file, Graphics::Surface *surface, int width, int height, uint32 back) {
	for (int i = 0; i < width * height; i++) {
		byte color = file->readByte();
		for (int n = 0; n < 8; n++) {
			int y = i / width;
			int x = (i % width) * 8 + (7 - n);
			if ((color & (1 << n)))
				surface->setPixel(x, y, back);
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
			loadRiddles(&file, 0xcf0 - 1 - 1 - 1, 8);
			loadMessagesVariableSize(&file, 0xf3d, 71);
			load8bitBinary(&file, 0x6aab - 2, 16);
			loadSpeakerFxZX(&file, 0xca0, 0xcdc);
			loadFonts(&file, 0x1218 + 16, _font);
			break;
		case Common::EN_ANY:
			loadRiddles(&file, 0x1460 - 1 - 3, 8);
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
	_keysFrame = loadFramesWithHeader(&file, 0xdf7, 1, red);

	uint32 green = _gfx->_texturePixelFormat.ARGBToColor(0xFF, 0, 0xff, 0);
	_spiritsMeterIndicatorFrame = loadFramesWithHeader(&file, _language == Common::ES_ESP ? 0xe5e : 0xe4f, 1, green);

	Graphics::Surface *background = new Graphics::Surface();

	_gfx->readFromPalette(4, r, g, b);
	uint32 front = _gfx->_texturePixelFormat.ARGBToColor(0xFF, r, g, b);

	int backgroundWidth = 16;
	int backgroundHeight = 18;
	background->create(backgroundWidth * 8, backgroundHeight, _gfx->_texturePixelFormat);
	background->fillRect(Common::Rect(0, 0, backgroundWidth * 8, backgroundHeight), 0);

	file.seek(_language == Common::ES_ESP ? 0xfd3 : 0xfc4);
	_background = loadFrames(&file, background, backgroundWidth, backgroundHeight, front);

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
		_conditions.remove_at(i);
		_conditionSources.remove_at(i);
	}

	_timeoutMessage = _messagesList[1];
	// Shield is unused in Castle Master
	_noEnergyMessage = _messagesList[1];
	_fallenMessage = _messagesList[4];
	_crushedMessage = _messagesList[3];
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
		drawStringInSurface(message, 120, 179, front, black, surface);
		_temporaryMessages.push_back(message);
		_temporaryMessageDeadlines.push_back(deadline);
	} else
		drawStringInSurface(_currentArea->_name, 120, 179, front, black, surface);

	for (int k = 0; k < _numberKeys; k++) {
		surface->copyRectToSurface((const Graphics::Surface)*_keysFrame, 99 - k * 4, 177, Common::Rect(0, 0, 6, 11));
	}

	uint32 green = _gfx->_texturePixelFormat.ARGBToColor(0xFF, 0, 0xff, 0);

	surface->fillRect(Common::Rect(152, 156, 216, 164), green);
	surface->copyRectToSurface((const Graphics::Surface)*_spiritsMeterIndicatorFrame, 140 + _spiritsMeterPosition, 156, Common::Rect(0, 0, 15, 8));
	//drawEnergyMeter(surface);
}

} // End of namespace Freescape
