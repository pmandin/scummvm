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

#include "common/rect.h"
#include "common/system.h"

#include "reevengi/gfx/gfx_base.h"

namespace Reevengi {

GfxBase::GfxBase() : _gameWidth(320), _gameHeight(240), _smushWidth(0), _smushHeight(0),
	_maskWidth(0), _maskHeight(0) {
}

bool GfxBase::computeScreenViewport(void) {
	int32 screenWidth = g_system->getWidth();
	int32 screenHeight = g_system->getHeight();

	Common::Rect viewport;
	if (true /*g_system->getFeatureState(OSystem::kFeatureAspectRatioCorrection)*/) {
		// Aspect ratio correction
		int32 viewportWidth = MIN<int32>(screenWidth, (screenHeight * _gameWidth) / _gameHeight);
		int32 viewportHeight = MIN<int32>(screenHeight, (screenWidth * _gameHeight) / _gameWidth);
		viewport = Common::Rect(viewportWidth, viewportHeight);

		// Pillarboxing
		viewport.translate((screenWidth - viewportWidth)>>1,
			(screenHeight - viewportHeight)>>1);
	} else {
		// Aspect ratio correction disabled, just stretch
		viewport = Common::Rect(screenWidth, screenHeight);
	}

	_screenWidth = viewport.width();
	_screenHeight = viewport.height();
	_scaleW = _screenWidth / (float)_gameWidth;
	_scaleH = _screenHeight / (float)_gameHeight;

	if (viewport == _screenViewport) {
		return false;
	}

	_screenViewport = viewport;
	return true;
}

} // end of namespace Reevengi
