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

#ifndef AGS_ENGINE_AC_SCREEN_OVERLAY_H
#define AGS_ENGINE_AC_SCREEN_OVERLAY_H

#include "ags/shared/core/types.h"

namespace AGS3 {

// Forward declaration
namespace AGS {
namespace Shared {
class Bitmap;
class Stream;
} // namespace Shared
} // namespace AGS

namespace AGS {
namespace Engine {
class IDriverDependantBitmap;
} // namespace Engine
} // namespace AGS

using namespace AGS; // FIXME later

// Overlay class.
// TODO: currently overlay creates and stores its own bitmap, even if
// created using existing sprite. As a side-effect, changing that sprite
// (if it were a dynamic one) will not affect overlay (unlike other objects).
// For future perfomance optimization it may be desired to store sprite index
// instead; but that would mean that overlay will have to receive sprite
// changes. For backward compatibility there may be a game switch that
// forces it to make a copy.
struct ScreenOverlay {
	// Texture
	Engine::IDriverDependantBitmap *ddb = nullptr;
	// Original bitmap
	Shared::Bitmap *pic = nullptr;
	bool hasAlphaChannel = false;
	int type = 0, timeout = 0;
	// Note that x,y are overlay's properties, that define its position in script;
	// but real drawn position is x + offsetX, y + offsetY;
	int x = 0, y = 0;
	// Border/padding offset for the tiled text windows
	int offsetX = 0, offsetY = 0;
	// Width and height to stretch the texture to
	int scaleWidth = 0, scaleHeight = 0;
	int bgSpeechForChar = -1;
	int associatedOverlayHandle = 0;
	int zorder = INT_MIN;
	bool positionRelativeToScreen = false;
	bool hasSerializedBitmap = false;
	int transparency = 0;

	// Tells if Overlay has graphically changed recently
	bool HasChanged() const {
		return _hasChanged;
	}
	// Manually marks GUI as graphically changed
	void MarkChanged() {
		_hasChanged = true;
	}
	// Clears changed flag
	void ClearChanged() {
		_hasChanged = false;
	}

	void ReadFromFile(Shared::Stream *in, int32_t cmp_ver);
	void WriteToFile(Shared::Stream *out) const;

private:
	bool _hasChanged = false;
};

} // namespace AGS3

#endif
