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

#ifndef REEVENGI_CLOCK_H
#define REEVENGI_CLOCK_H

namespace Reevengi {

class Clock;

class Clock {
public:
	Clock(Clock *parent = nullptr);
	virtual ~Clock();

	void pause(void);
	void unpause(void);

	uint32 getRunningTime(bool _unPausedTime = false);
	uint32 getGameTic(bool _unPausedTime = false);	// 1 tic = 1/30s, mostly for animations

	// Pause till game tics elapsed, using unpaused time
	void waitGameTic(int elapsedTics = 1);

private:
	Clock *_parent;

	uint32 runningTime, startPause, endPause;
	bool paused;

	uint32 getParentTime(void);
};

} // End of namespace Reevengi

#endif
