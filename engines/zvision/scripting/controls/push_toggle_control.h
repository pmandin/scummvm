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

#ifndef ZVISION_PUSH_TOGGLE_CONTROL_H
#define ZVISION_PUSH_TOGGLE_CONTROL_H

#include "common/array.h"
#include "common/events.h"
#include "common/rect.h"
#include "zvision/scripting/control.h"

namespace ZVision {

class PushToggleControl : public Control {
public:
	PushToggleControl(ZVision *engine, uint32 key, Common::SeekableReadStream &stream);
	~PushToggleControl() override;

	/**
	 * Called when LeftMouse is pushed. Default is NOP.
	 *
	 * @param screenSpacePos             The position of the mouse in screen space
	 * @param backgroundImageSpacePos    The position of the mouse in background image space
	 */
	bool onMouseDown(const Common::Point &screenSpacePos, const Common::Point &backgroundImageSpacePos) override;
	/**
	 * Called when LeftMouse is lifted. Calls ScriptManager::setStateValue(_key, 1);
	 *
	 * @param screenSpacePos             The position of the mouse in screen space
	 * @param backgroundImageSpacePos    The position of the mouse in background image space
	 */
	bool onMouseUp(const Common::Point &screenSpacePos, const Common::Point &backgroundImageSpacePos) override;
	/**
	 * Called on every MouseMove. Tests if the mouse is inside _hotspot, and if so, sets the cursor.
	 *
	 * @param engine                     The base engine
	 * @param screenSpacePos             The position of the mouse in screen space
	 * @param backgroundImageSpacePos    The position of the mouse in background image space
	 * @return                           Was the cursor changed?
	 */
	bool onMouseMove(const Common::Point &screenSpacePos, const Common::Point &backgroundImageSpacePos) override;

private:
	/**
	 * The area that will trigger the event
	 * This is in image space coordinates, NOT screen space
	 */
	Common::Array<Common::Rect> _hotspots;
	/** The cursor to use when hovering over _hotspot */
	int _cursor;
	/** Button maximal values count */
	uint _countTo;

	Common::EventType _event;

	bool contain(const Common::Point &point);
};

} // End of namespace ZVision

#endif
