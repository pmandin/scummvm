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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/serializer.h"

#include "engines/nancy/nancy.h"
#include "engines/nancy/graphics.h"
#include "engines/nancy/resource.h"
#include "engines/nancy/input.h"
#include "engines/nancy/sound.h"
#include "engines/nancy/util.h"

#include "engines/nancy/action/orderingpuzzle.h"

#include "engines/nancy/state/scene.h"

namespace Nancy {
namespace Action {

void OrderingPuzzle::init() {
	for (uint i = 0; i < _destRects.size(); ++i) {
		if (i == 0) {
			_screenPosition = _destRects[i];
		} else {
			_screenPosition.extend(_destRects[i]);
		}
	}

	for (uint i = 0; i < _overlayDests.size(); ++i) {
		_screenPosition.extend(_overlayDests[i]);
	}

	g_nancy->_resource->loadImage(_imageName, _image);
	_drawSurface.create(_screenPosition.width(), _screenPosition.height(), g_nancy->_graphicsManager->getInputPixelFormat());

	if (_image.hasPalette()) {
		uint8 palette[256 * 3];
		_image.grabPalette(palette, 0, 256);
		_drawSurface.setPalette(palette, 0, 256);
	}

	setTransparent(true);
	_drawSurface.clear(_drawSurface.getTransparentColor());
	setVisible(true);

	RenderObject::init();
}

void OrderingPuzzle::readData(Common::SeekableReadStream &stream) {
	bool isPiano = _puzzleType == kPiano;
	bool isOrderItems = _puzzleType == kOrderItems;
	readFilename(stream, _imageName);
	Common::Serializer ser(&stream, nullptr);
	ser.setVersion(g_nancy->getGameType());

	uint16 numElements;
	if (ser.getVersion() == kGameTypeVampire) {
		// Hardcoded in The Vampire Diaries
		numElements = 5;
	} else {
		ser.syncAsUint16LE(numElements);
	}

	if (isOrderItems) {
		ser.syncAsByte(_hasSecondState);
		ser.syncAsByte(_itemsStayDown);
	} else if (isPiano) {
		_itemsStayDown = false;
	}

	readRectArray(stream, _down1Rects, numElements);
	ser.skip(16 * (15 - numElements), kGameTypeNancy1);

	if (isOrderItems) {
		readRectArray(stream, _up2Rects, numElements);
		ser.skip(16 * (15 - numElements));

		readRectArray(stream, _down2Rects, numElements);
		ser.skip(16 * (15 - numElements));
	}

	readRectArray(stream, _destRects, numElements);
	ser.skip(16 * (15 - numElements), kGameTypeNancy1);

	_hotspots.resize(numElements);
	if (isPiano) {
		readRectArray(stream, _hotspots, numElements);
		ser.skip(16 * (15 - numElements));
	} else {
		_hotspots = _destRects;
	}

	uint sequenceLength = 5;
	ser.syncAsUint16LE(sequenceLength, kGameTypeNancy1);

	_correctSequence.resize(sequenceLength);
	for (uint i = 0; i < sequenceLength; ++i) {
		switch (_puzzleType) {
		case kOrdering:
			ser.syncAsByte(_correctSequence[i]);
			break;
		case kPiano:
			ser.syncAsUint16LE(_correctSequence[i]);
			break;
		case kOrderItems:
			// For some reason, OrderItems labels starting from 1
			ser.syncAsUint16LE(_correctSequence[i]);
			--_correctSequence[i];
			break;
		}
	}
	ser.skip((15 - sequenceLength) * (_puzzleType == kOrdering ? 1 : 2), kGameTypeNancy1);

	if (isOrderItems) {
		uint numOverlays = 0;
		ser.syncAsUint16LE(_state2InvItem);
		ser.syncAsUint16LE(numOverlays);

		readRectArray(ser, _overlaySrcs, numOverlays);
		readRectArray(ser, _overlayDests, numOverlays);
	}

	if (ser.getVersion() > kGameTypeVampire) {
		_pushDownSound.readNormal(stream);

		if (isOrderItems) {
			_itemSound.readNormal(stream);
			_popUpSound.readNormal(stream);
		}
	}

	_solveExitScene.readData(stream, ser.getVersion() == kGameTypeVampire);
	ser.syncAsUint16LE(_solveSoundDelay);
	_solveSound.readNormal(stream);
	_exitScene.readData(stream, ser.getVersion() == kGameTypeVampire);
	readRect(stream, _exitHotspot);

	_downItems.resize(numElements, false);
	_secondStateItems.resize(numElements, false);
}

void OrderingPuzzle::execute() {
	switch (_state) {
	case kBegin:
		init();
		registerGraphics();
		if (g_nancy->getGameType() > kGameTypeVampire) {
			g_nancy->_sound->loadSound(_pushDownSound);
			if (_puzzleType == kOrderItems) {
				g_nancy->_sound->loadSound(_itemSound);
				g_nancy->_sound->loadSound(_popUpSound);
			}
		}
		_state = kRun;
		// fall through
	case kRun:
		switch (_solveState) {
		case kNotSolved:
			if (!_itemsStayDown) {
				// Clear the pushed item
				if (g_nancy->_sound->isSoundPlaying(_pushDownSound)) {
					return;
				}

				for (uint i = 0; i < _downItems.size(); ++i) {
					if (_downItems[i]) {
						popUp(i);
					}
				}
			}

			if (_puzzleType != kPiano) {
				if (_clickedSequence.size() < _correctSequence.size()) {
					return;
				}

				// Check the pressed sequence. If its length is above a certain number,
				// clear it and start anew
				if (_clickedSequence != _correctSequence) {
					if (_puzzleType == kOrdering) {
						uint maxNumPressed = 4;
						if (g_nancy->getGameType() > kGameTypeVampire) {
							if (_puzzleType == kOrderItems) {
								maxNumPressed = _correctSequence.size() - 1;
							} else {
								maxNumPressed = _correctSequence.size() + 1;
							}
						}

						if (_clickedSequence.size() > maxNumPressed) {
							clearAllElements();
						}
					} else {
						// OrderItems has a slight delay, after which it actually clears
						if (_clickedSequence.size() == _correctSequence.size()) {
							if (_solveSoundPlayTime == 0) {
								_solveSoundPlayTime = g_nancy->getTotalPlayTime() + 500;
							} else {
								if (g_nancy->getTotalPlayTime() > _solveSoundPlayTime) {
									clearAllElements();
									_solveSoundPlayTime = 0;
								}
							}
						}
					}

					
					return;
				}

				if (_puzzleType == kOrderItems) {
					if (!g_nancy->_sound->isSoundPlaying(_pushDownSound)) {
						// Draw some overlays when solved correctly (OrderItems only)
						for (uint i = 0; i < _overlaySrcs.size(); ++i) {
							Common::Rect destRect = _overlayDests[i];
							destRect.translate(-_screenPosition.left, -_screenPosition.top);

							_drawSurface.blitFrom(_image, _overlaySrcs[i], destRect);
							_needsRedraw = true;
						}
					} else {
						return;
					}
				}				
			} else {
				// Piano puzzle checks only the last few elements
				if (_clickedSequence.size() < _correctSequence.size()) {
					return;
				}

				// Arbitrary number
				if (_clickedSequence.size() > 30) {
					_clickedSequence.erase(&_clickedSequence[0], &_clickedSequence[_clickedSequence.size() - 6]);
				}

				for (uint i = 0; i < _correctSequence.size(); ++i) {
					if (_clickedSequence[_clickedSequence.size() - _correctSequence.size() + i] != (int16)_correctSequence[i]) {
						return;
					}
				}
			}

			NancySceneState.setEventFlag(_solveExitScene._flag);
			_solveSoundPlayTime = g_nancy->getTotalPlayTime() + _solveSoundDelay * 1000;
			_solveState = kPlaySound;
			// fall through
		case kPlaySound:
			if (g_nancy->getTotalPlayTime() <= _solveSoundPlayTime) {
				break;
			}

			g_nancy->_sound->loadSound(_solveSound);
			g_nancy->_sound->playSound(_solveSound);
			_solveState = kWaitForSound;
			break;
		case kWaitForSound:
			if (!g_nancy->_sound->isSoundPlaying(_solveSound)) {
				_state = kActionTrigger;
			}

			break;
		}

		break;
	case kActionTrigger:
		if (g_nancy->getGameType() == kGameTypeVampire) {
			g_nancy->_sound->stopSound("BUOK");
		} else {
			g_nancy->_sound->stopSound(_pushDownSound);
		}
		
		g_nancy->_sound->stopSound(_solveSound);

		if (_solveState == kNotSolved) {
			_exitScene.execute();
		} else {
			NancySceneState.changeScene(_solveExitScene._sceneChange);
		}

		finishExecution();
		break;
	}
}

void OrderingPuzzle::handleInput(NancyInput &input) {
	if (_solveState != kNotSolved) {
		return;
	}

	bool canClick = true;
	if (_itemsStayDown && g_nancy->_sound->isSoundPlaying(_pushDownSound)) {
		canClick = false;
	}

	if (NancySceneState.getViewport().convertViewportToScreen(_exitHotspot).contains(input.mousePos)) {
		g_nancy->_cursorManager->setCursorType(CursorManager::kExit);

		if (canClick && input.input & NancyInput::kLeftMouseButtonUp) {
			_state = kActionTrigger;
		}
		return;
	}

	for (int i = 0; i < (int)_hotspots.size(); ++i) {
		if (NancySceneState.getViewport().convertViewportToScreen(_hotspots[i]).contains(input.mousePos)) {
			g_nancy->_cursorManager->setCursorType(CursorManager::kHotspot);

			if (canClick && input.input & NancyInput::kLeftMouseButtonUp) {
				if (_puzzleType == kOrderItems) {
					if (_itemsStayDown && _downItems[i]) {
						// Button is pressed, OrderItems does not allow for depressing
						return;
					}

					if (NancySceneState.getHeldItem() == _state2InvItem) {
						// We are holding the correct inventory, set the button to its alternate (dusted) state
						setToSecondState(i);
						return;
					}
				} 

				if (_puzzleType == kPiano) {
					// Set the correct sound name for every piano key
					if (Common::isDigit(_pushDownSound.name.lastChar())) {
						_pushDownSound.name.deleteLastChar();
					}

					_pushDownSound.name.insertChar('0' + i, _pushDownSound.name.size());
					g_nancy->_sound->loadSound(_pushDownSound);
				}
				
				if (_puzzleType == kOrdering) {
					// Ordering puzzle allows for depressing buttons after they're pressed.
					// If the button is the last one the player pressed, it is removed from the order.
					// If not, the sequence is kept wrong and will be reset after enough buttons are pressed
					for (uint j = 0; j < _clickedSequence.size(); ++j) {
						if (_clickedSequence[j] == i && _downItems[i] == true) {
							popUp(i);
							if (_clickedSequence.back() == i) {
								_clickedSequence.pop_back();
							}

							return;
						}
					}
				}

				_clickedSequence.push_back(i);
				pushDown(i);
			}

			return;
		}
	}
}

Common::String OrderingPuzzle::getRecordTypeName() const {
	switch (_puzzleType) {
	case kPiano:
		return "PianoPuzzle";
	case kOrderItems:
		return "OrderItemsPuzzle";
	default:
		return "OrderingPuzzle";
	}
}

void OrderingPuzzle::pushDown(uint id) {
	if (g_nancy->getGameType() == kGameTypeVampire) {
		g_nancy->_sound->playSound("BUOK");
	} else {
		g_nancy->_sound->playSound(_pushDownSound);
	}

	_downItems[id] = true;
	Common::Rect destRect = _destRects[id];
	destRect.translate(-_screenPosition.left, -_screenPosition.top);
	_drawSurface.blitFrom(_image, _secondStateItems[id] ? _down2Rects[id] : _down1Rects[id], destRect);

	_needsRedraw = true;
}

void OrderingPuzzle::setToSecondState(uint id) {
	g_nancy->_sound->playSound(_itemSound);

	_secondStateItems[id] = true;
	Common::Rect destRect = _destRects[id];
	destRect.translate(-_screenPosition.left, -_screenPosition.top);
	_drawSurface.blitFrom(_image, _downItems[id] ? _down2Rects[id] : _up2Rects[id], destRect);

	_needsRedraw = true;
}

void OrderingPuzzle::popUp(uint id) {
	if (g_nancy->getGameType() == kGameTypeVampire) {
		g_nancy->_sound->playSound("BUOK");
	} else {
		if (_popUpSound.name.size()) {
			g_nancy->_sound->playSound(_popUpSound);
		} else {
			g_nancy->_sound->playSound(_pushDownSound);
		}
	}


	_downItems[id] = false;
	Common::Rect destRect = _destRects[id];
	destRect.translate(-_screenPosition.left, -_screenPosition.top);

	if (_secondStateItems[id] == false || _up2Rects.size() == 0) {
		_drawSurface.fillRect(destRect, _drawSurface.getTransparentColor());
	} else {
		_drawSurface.blitFrom(_image, _up2Rects[id], destRect);
	}

	_needsRedraw = true;
}

void OrderingPuzzle::clearAllElements() {
	for (uint id = 0; id < _downItems.size(); ++id) {
		popUp(id);
	}

	_clickedSequence.clear();
	return;
}

} // End of namespace Action
} // End of namespace Nancy
