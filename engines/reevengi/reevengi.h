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

#ifndef REEVENGI_H
#define REEVENGI_H

#include "engines/advancedDetector.h"
#include "engines/engine.h"
#include "image/image_decoder.h"

#include "engines/reevengi/detection.h"

namespace Common {
class Event;
}


namespace Reevengi {

class GfxBase;
class TimDecoder;
class Clock;
class Room;
class Entity;

class ReevengiEngine : public Engine {
public:
	ReevengiEngine(OSystem *syst, const ReevengiGameDescription *gameDesc);
	~ReevengiEngine() override;

	virtual Common::Error run(void);

	int _character;
	int _stage, _room, _camera;

	float _playerX, _playerY, _playerZ, _playerA;

protected:
	ADGameDescription _gameDesc;
	ReevengiGameType _gameType;
	Clock *_clock;

	bool hasFeature(EngineFeature f) const override;
	GfxBase *createRenderer(int screenW, int screenH, bool fullscreen);
	virtual void initPreRun(void);

	// Background image
	Image::ImageDecoder *_bgImage;
	void destroyBgImage(void);
	virtual void loadBgImage(void);

	// Background mask image
	TimDecoder *_bgMaskImage;
	void destroyBgMaskImage(void);
	virtual void loadBgMaskImage(void);

	// Room data
	Room *_roomScene;
	void destroyRoom(void);
	virtual void loadRoom(void);

	// Movie
	virtual void loadMovie(unsigned int numMovie);

	// Entity
	int _defEntity, _defIsPlayer;
	virtual Entity *loadEntity(int numEntity, int isPlayer);

	// Player
	int _playerTic, _playerMove;

private:
	bool _softRenderer;

	void processEvents(void);
	void onScreenChanged(void);

	void processEventsKeyDown(Common::Event e);
	void processEventsKeyDownRepeat(Common::Event e);

	void testDisplayImage(Image::ImageDecoder *img);
	void testDisplayMaskImage(Image::ImageDecoder *img);
	void testLoadMovie(void);
	void testPlayMovie(void);

	void testView3DBegin(void);
	void testDrawOrigin(void);
	void testDrawGrid(void);
	void testDrawPlayer(void);
	void testView3DEnd(void);

};

} // End of namespace Reevengi

#endif
