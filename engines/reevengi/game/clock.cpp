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

#include "common/system.h"

#include "engines/reevengi/game/clock.h"

namespace Reevengi {

Clock::Clock(Clock *parent): runningTime(0), paused(false) {
	_parent = parent;
	startPause = endPause = getParentTime();
}

Clock::~Clock() {
}

void Clock::pause(void) {
	if (paused) {
		return;
	}

	paused = true;
	startPause = getParentTime();
	/* endPause is end of previous pause */
	runningTime += startPause - endPause;
}

void Clock::unpause(void) {
	if (!paused) {
		return;
	}

	paused = false;
	endPause = getParentTime();
}

uint32 Clock::getRunningTime(bool _unPausedTime) {
	if (paused && !_unPausedTime) {
		return runningTime;
	}

	return runningTime + getParentTime() - endPause;
}

uint32 Clock::getGameTic(bool _unPausedTime) {
	return (getRunningTime(_unPausedTime) * 30) / 1000;
}

void Clock::waitGameTic(int elapsedTics) {
	uint32 nextMs = ((getGameTic(true)+elapsedTics) * 1000) / 30;

	g_system->delayMillis( MIN<uint32>(1, nextMs - getRunningTime(true)));
}

uint32 Clock::getParentTime(void) {
	if (_parent) {
		return _parent->getRunningTime();
	}

	return g_system->getMillis();
}

} // End of namespace Reevengi
