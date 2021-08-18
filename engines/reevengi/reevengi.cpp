/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/error.h"
#include "common/events.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/pixelbuffer.h"
#include "graphics/renderer.h"
#include "graphics/screen.h"
#include "graphics/surface.h"
#include "image/bmp.h"
#include "image/jpeg.h"
#include "math/vector3d.h"

#ifdef USE_OPENGL
#include "graphics/opengl/context.h"
#endif

#include "engines/reevengi/reevengi.h"
#include "engines/reevengi/formats/adt.h"
#include "engines/reevengi/formats/pak.h"
#include "engines/reevengi/formats/tim.h"
#include "engines/reevengi/game/clock.h"
#include "engines/reevengi/game/entity.h"
#include "engines/reevengi/game/room.h"
#include "engines/reevengi/gfx/gfx_base.h"
#include "engines/reevengi/gfx/gfx_opengl.h"
#include "engines/reevengi/gfx/gfx_tinygl.h"
#include "engines/reevengi/movie/movie.h"

namespace Reevengi {

GfxBase *g_driver = nullptr;

ReevengiEngine::ReevengiEngine(OSystem *syst, ReevengiGameType gameType, const ADGameDescription *gameDesc) :
	Engine(syst), _gameType(gameType), _character(0), _softRenderer(true),
	_stage(1), _room(0), _camera(0), _bgImage(nullptr),_bgMaskImage(nullptr), _roomScene(nullptr),
	_playerX(0), _playerY(0), _playerZ(0), _playerA(0), _playerMove(0), _playerTic(0) {
	memcpy(&_gameDesc, gameDesc, sizeof(_gameDesc));
	g_movie = nullptr;
	_clock = new Clock();

	_playerTic = _clock->getGameTic();
}

ReevengiEngine::~ReevengiEngine() {
	destroyBgImage();
	destroyBgMaskImage();
	destroyRoom();

	delete g_movie;
	g_movie = nullptr;

	delete _clock;
	_clock = nullptr;
}

bool ReevengiEngine::hasFeature(EngineFeature f) const {
	return (f == kSupportsArbitraryResolutions);
}

GfxBase *ReevengiEngine::createRenderer(int screenW, int screenH, bool fullscreen) {
	Common::String rendererConfig = ConfMan.get("renderer");
	Graphics::RendererType desiredRendererType = Graphics::parseRendererTypeCode(rendererConfig);
	Graphics::RendererType matchingRendererType = Graphics::getBestMatchingAvailableRendererType(desiredRendererType);

	_softRenderer = matchingRendererType == Graphics::kRendererTypeTinyGL;
	initGraphics3d(screenW, screenH); //, fullscreen, !_softRenderer);

#if defined(USE_OPENGL)
	// Check the OpenGL context actually supports shaders
	if (matchingRendererType == Graphics::kRendererTypeOpenGLShaders && !OpenGLContext.shadersSupported) {
		matchingRendererType = Graphics::kRendererTypeOpenGL;
	}
#endif

	if (matchingRendererType != desiredRendererType && desiredRendererType != Graphics::kRendererTypeDefault) {
		// Display a warning if unable to use the desired renderer
		warning("Unable to create a '%s' renderer", rendererConfig.c_str());
	}

	GfxBase *renderer = nullptr;
/*#if defined(USE_GLES2) || defined(USE_OPENGL_SHADERS)
	if (matchingRendererType == Graphics::kRendererTypeOpenGLShaders) {
		renderer = CreateGfxOpenGLShader();
	}
#endif*/
#if defined(USE_OPENGL) /*&& !defined(USE_GLES2)*/
	if (matchingRendererType == Graphics::kRendererTypeOpenGL) {
		renderer = CreateGfxOpenGL();
	}
#endif
	if (matchingRendererType == Graphics::kRendererTypeTinyGL) {
		renderer = CreateGfxTinyGL();
	}

	if (!renderer) {
		error("Unable to create a '%s' renderer", rendererConfig.c_str());
	}

	renderer->setupScreen(screenW, screenH, fullscreen);
	return renderer;
}

void ReevengiEngine::initPreRun(void) {
}

Common::Error ReevengiEngine::run() {
	initPreRun();

	bool fullscreen = ConfMan.getBool("fullscreen");
	g_driver = createRenderer(640, 480, fullscreen);

	//TimDecoder *my_image = testLoadImage();
	//testLoadMovie();
	loadRoom();
#if 1
	if (_roomScene) {
		RdtCameraPos_t camera;
		_roomScene->getCameraPos(_camera, &camera);

		/* Reset player pos */
		_playerX = (camera.toX + camera.fromX) / 2;
		_playerY = (camera.toY + camera.fromY) / 2;
		_playerZ = (camera.toZ + camera.fromZ) / 2;

		debug(3, "%d cameras, pos %.3f,%.3f,%.3f", _roomScene->getNumCameras(), _playerX,_playerY,_playerZ);
	}
#endif
	loadBgImage();
	loadBgMaskImage();

	while (!shouldQuit()) {
		g_driver->clearScreen();

		if (_bgImage) {
			testDisplayImage(_bgImage);
		}
		if (_bgMaskImage) {
			testDisplayMaskImage(_bgMaskImage);
		}
		//testDisplayImage(my_image);
		//testPlayMovie();
		testView3DBegin();
		testDrawGrid();
		testDrawOrigin();
		testDrawPlayer();
		testView3DEnd();

		// Tell the system to update the screen.
		g_driver->flipBuffer();

		// Get new events from the event manager so the window doesn't appear non-responsive.
		processEvents();

		// FIXME: Continue processing input events, till game tic elapsed
		_clock->waitGameTic();
	}

	g_driver->releaseMovieFrame();
	//delete my_image;

	return Common::kNoError;
}

void ReevengiEngine::processEvents(void) {
	Common::Event e;
	while (g_system->getEventManager()->pollEvent(e)) {
		// Handle any buttons, keys and joystick operations

		/*if (isPaused()) {
			// Only pressing key P to resume the game is allowed when the game is paused
			if (e.type == Common::EVENT_KEYDOWN && e.kbd.keycode == Common::KEYCODE_p) {
				pauseEngine(false);
			}
			continue;
		}*/

		if (e.type == Common::EVENT_KEYDOWN) {
			processEventsKeyDownRepeat(e);

			if (e.kbdRepeat) {
				continue;
			}

			processEventsKeyDown(e);

			if (e.kbd.keycode == Common::KEYCODE_d && (e.kbd.hasFlags(Common::KBD_CTRL))) {
				/*_console->attach();*/
				/*_console->onFrame();*/
			} else if ((e.kbd.keycode == Common::KEYCODE_RETURN || e.kbd.keycode == Common::KEYCODE_KP_ENTER)
						&& e.kbd.hasFlags(Common::KBD_ALT)) {
					//StarkGfx->toggleFullscreen();
			} else if (e.kbd.keycode == Common::KEYCODE_p) {
				/*if (StarkUserInterface->isInGameScreen()) {
					pauseEngine(true);
					debug("The game is paused");
				}*/
			} else {
				//StarkUserInterface->handleKeyPress(e.kbd);
			}

		} else if (e.type == Common::EVENT_SCREEN_CHANGED) {
			//debug(3, "onScreenChanged");
			onScreenChanged();
		}
	}
}

void ReevengiEngine::processEventsKeyDownRepeat(Common::Event e) {
	int curTic = _clock->getGameTic();
	bool updatePlayer = false;
	float nPlayerX=_playerX, nPlayerZ=_playerZ;

	/* Move player */
	if (e.kbd.keycode == Common::KEYCODE_UP) {
		// forward
		if (!_playerMove) {
			_playerTic = curTic;
			_playerMove = 1;
		}

		nPlayerX += cos((_playerA*M_PI)/2048.0f)*0.5f*(curTic-_playerTic);
		nPlayerZ -= sin((_playerA*M_PI)/2048.0f)*0.5f*(curTic-_playerTic);
		updatePlayer = true;
	}
	if (e.kbd.keycode == Common::KEYCODE_DOWN) {
		// backward
		if (!_playerMove) {
			_playerTic = curTic;
			_playerMove = 1;
		}

		nPlayerX -= cos((_playerA*M_PI)/2048.0f)*0.5f*(curTic-_playerTic);
		nPlayerZ += sin((_playerA*M_PI)/2048.0f)*0.5f*(curTic-_playerTic);
		updatePlayer = true;
	}
	if (e.kbd.keycode == Common::KEYCODE_LEFT) {
		// turn left
		if (!_playerMove) {
			_playerTic = curTic;
			_playerMove = 1;
		}

		_playerA -= 0.3f*(curTic-_playerTic);
		while (_playerA < 0.0f) {
			_playerA += 4096.0f;
		}
		updatePlayer = true;
	}
	if (e.kbd.keycode == Common::KEYCODE_RIGHT) {
		// turn right
		if (!_playerMove) {
			_playerTic = curTic;
			_playerMove = 1;
		}

		_playerA += 0.3f*(curTic-_playerTic);
		while (_playerA > 4096.0f) {
			_playerA -= 4096.0f;
		}
		updatePlayer = true;
	}
	if (!updatePlayer) {
		_playerMove = 0;
	}

	if (_roomScene) {
		Math::Vector2d fromPos(_playerX, _playerZ);
		Math::Vector2d toPos(nPlayerX, nPlayerZ);

		if (_roomScene->checkCamBoundary(_camera, fromPos, toPos)) {
			//debug(3, "reached boundary for this camera");
			return;
		}

		int newCamera = _roomScene->checkCamSwitch(_camera, fromPos, toPos);
		if (newCamera != -1) {
			_camera = newCamera;
			destroyBgImage();
			loadBgImage();

			destroyBgMaskImage();
			loadBgMaskImage();
		}
	}

	_playerX = nPlayerX;
	_playerZ = nPlayerZ;
}

void ReevengiEngine::processEventsKeyDown(Common::Event e) {
	bool updateBgImage = false;
	bool updateRoom = false;

	/* Depend on game/demo */
	if (e.kbd.keycode == Common::KEYCODE_z) {
		--_stage;
		if (_stage<1) {
			_stage=7;
		}
		updateBgImage = true;
		updateRoom = true;
	}
	if (e.kbd.keycode == Common::KEYCODE_s) {
		++_stage;
		if (_stage>7) {
			_stage=1;
		}
		updateBgImage = true;
		updateRoom = true;
	}
	if (e.kbd.keycode == Common::KEYCODE_x) {
		_stage=1;
		updateBgImage = true;
		updateRoom = true;
	}

	/* Depend on game/stage */
	if (e.kbd.keycode == Common::KEYCODE_e) {
		--_room;
		if (_room<0) {
			_room=0x1c;
		}
		updateBgImage = true;
		updateRoom = true;
	}
	if (e.kbd.keycode == Common::KEYCODE_d) {
		++_room;
		if (_room>0x1c) {
			_room=0;
		}
		updateBgImage = true;
		updateRoom = true;
	}
	if (e.kbd.keycode == Common::KEYCODE_c) {
		_room=0;
		updateBgImage = true;
		updateRoom = true;
	}

	/* Room dependant */
	if (e.kbd.keycode == Common::KEYCODE_r) {
		--_camera;
		if (_roomScene) {
			if (_camera<0) {
				_camera = _roomScene->getNumCameras()-1;
			}
		}
		updateBgImage = true;
	}
	if (e.kbd.keycode == Common::KEYCODE_f) {
		++_camera;
		if (_roomScene) {
			if (_camera>=_roomScene->getNumCameras()) {
				_camera = 0;
			}
		}
		updateBgImage = true;
	}
	if (e.kbd.keycode == Common::KEYCODE_v) {
		_camera=0;
		updateBgImage = true;
	}

	debug(3, "switch to stage %d, room %d, camera %d", _stage, _room, _camera);

	if (updateRoom) {
		destroyRoom();
		loadRoom();
		if (_roomScene) {
			RdtCameraPos_t camera;
			_roomScene->getCameraPos(_camera, &camera);

			/* Reset player pos */
			_playerX = (camera.toX + camera.fromX) / 2;
			_playerY = (camera.toY + camera.fromY) / 2;
			_playerZ = (camera.toZ + camera.fromZ) / 2;

			debug(3, "%d cameras, pos %.3f,%.3f,%.3f", _roomScene->getNumCameras(), _playerX,_playerY,_playerZ);
		}
	}
	if (updateBgImage) {
		destroyBgImage();
		loadBgImage();

		destroyBgMaskImage();
		loadBgMaskImage();
	}
}

void ReevengiEngine::onScreenChanged(void) {
	bool changed = g_driver->computeScreenViewport();
}

void ReevengiEngine::destroyBgImage(void) {
	delete _bgImage;
	_bgImage = nullptr;
}

void ReevengiEngine::loadBgImage(void) {
	if (!_bgImage) {
		return;
	}

	Graphics::Surface *bgSurf = (Graphics::Surface *) _bgImage->getSurface();
	if (bgSurf) {
		g_driver->prepareMovieFrame(bgSurf);
	}
}

void ReevengiEngine::destroyBgMaskImage(void) {
	delete _bgMaskImage;
	_bgMaskImage = nullptr;
}

void ReevengiEngine::loadBgMaskImage(void) {
	if (!_bgMaskImage) {
		return;
	}

	uint16 *timPalette = _bgMaskImage->getTimPalette();

	Graphics::Surface *bgSurf = (Graphics::Surface *) _bgMaskImage->getSurface();
	if (bgSurf) {
		g_driver->prepareMaskedFrame(bgSurf, timPalette);
	}
}

Entity *ReevengiEngine::loadEntity(int numEntity)
{
	return nullptr;
}

void ReevengiEngine::destroyRoom(void) {
	delete _roomScene;
	_roomScene = nullptr;
}

void ReevengiEngine::loadRoom(void) {
	//
}

TimDecoder *ReevengiEngine::testLoadImage(void) {
	/*debug(3, "loading jopt06.tim");
	Common::SeekableReadStream *s1 = SearchMan.createReadStreamForMember("jopt06.tim");
	TimDecoder *my_image1 = new TimDecoder();
	my_image1->loadStream(*s1);*/

	debug(3, "loading gwarning.adt");
	Common::SeekableReadStream *s3 = SearchMan.createReadStreamForMember("common/datp/gwarning.adt");
	AdtDecoder *my_image1 = new AdtDecoder();
	my_image1->loadStream(*s3);

	/*debug(3, "loading rc1060.pak");
	Common::SeekableReadStream *s2 = SearchMan.createReadStreamForMember("rc1060.pak");
	PakDecoder *my_image1 = new PakDecoder();
	my_image1->loadStream(*s2);*/

	if (my_image1) {
		Graphics::Surface *surf = (Graphics::Surface *) my_image1->getSurface();
		if (surf) {
			g_driver->prepareMovieFrame(surf);
		}
	}

	return my_image1;
}

void ReevengiEngine::testDisplayImage(Image::ImageDecoder *img) {
	g_driver->drawMovieFrame(0, 0);
}

void ReevengiEngine::testDisplayMaskImage(Image::ImageDecoder *img) {
	if (!_roomScene)
		return;

	_roomScene->drawMasks(_camera);
}

void ReevengiEngine::testLoadMovie(void) {
	g_movie = CreatePsxPlayer();
	g_movie->play("capcom.str", false, 0, 0);
	//g_movie = CreateAviPlayer();
	//g_movie->play("sample.avi", false, 0, 0);
	//g_movie = CreateMpegPlayer();
	//g_movie->play("zmovie/roopne.dat", false, 0, 0);
}

void ReevengiEngine::testPlayMovie(void) {

	//if (g_movie->getMovieTime()>1000) { g_movie->pause(true); }

	if (g_movie->isPlaying() /*&& _movieSetup == _currSet->getCurrSetup()->_name*/) {
		//_movieTime = g_movie->getMovieTime();
		if (g_movie->isUpdateNeeded()) {
			Graphics::Surface *frame = g_movie->getDstSurface();
			if (frame) {
				//g_driver->clearScreen();
				g_driver->prepareMovieFrame(frame);
			}
			g_movie->clearUpdateNeeded();
		}
		if (g_movie->getFrame() >= 0)
			g_driver->drawMovieFrame(0, 0);
		else
			g_driver->releaseMovieFrame();
	}
}

void ReevengiEngine::testView3DBegin(void) {
	if (!_roomScene)
		return;

	RdtCameraPos_t camera;
	_roomScene->getCameraPos(_camera, &camera);

	g_driver->setProjection(60.0f, 4.0f/3.0f, (float) GfxBase::kRenderZNear, (float) GfxBase::kRenderZFar);
	g_driver->setModelview(
		camera.fromX, camera.fromY, camera.fromZ,
		camera.toX, camera.toY, camera.toZ,
		0.0f, -1.0f, 0.0f
	);

	g_driver->setDepth(true);
}

void ReevengiEngine::testDrawOrigin(void) {
	Math::Vector3d v0(0, 0, 0);

	g_driver->setColor(1.0, 0.0, 0.0);	/* x red */
	{
		Math::Vector3d v1(3000, 0, 0);
		g_driver->line(v0, v1);
	}

	g_driver->setColor(0.0, 1.0, 0.0);	/* y green */
	{
		Math::Vector3d v1(0, 3000, 0);
		g_driver->line(v0, v1);
	}

	g_driver->setColor(0.0, 0.0, 1.0);	/* z blue */
	{
		Math::Vector3d v1(0, 0, 3000);
		g_driver->line(v0, v1);
	}
}

void ReevengiEngine::testDrawGrid(void) {
	if (!_roomScene)
		return;

	RdtCameraPos_t camera;
	_roomScene->getCameraPos(_camera, &camera);

	g_driver->setColor(0.75, 0.75, 0.75);

	float i, px = camera.toX, pz = camera.toY;

	for (i=-20000.0f; i<=20000.0f; i+=2000.0f) {
		Math::Vector3d v0(px-20000.0f, 0.0f, pz+i);
		Math::Vector3d v1(px+20000.0f, 0.0f, pz+i);
		g_driver->line(v0, v1);

		Math::Vector3d v2(px+i, 0.0f, pz-20000.0f);
		Math::Vector3d v3(px+i, 0.0f, pz+20000.0f);
		g_driver->line(v2, v3);
	}

	// Draw camera boundary and switches
	_roomScene->drawCamBoundary(_camera);
	_roomScene->drawCamSwitch(_camera);
}

void ReevengiEngine::testDrawPlayer(void) {
	const float radius = 2000.0f;
	float x = _playerX;
	float y = _playerZ;
	float angle = _playerA;

	Math::Vector3d v1(
		x + radius * cos((-angle * M_PI) / 2048.0f) * 0.5f,
		0.0f,
		y + radius * sin((-angle * M_PI) / 2048.0f) * 0.5f
	);

	g_driver->setColor(0, 1.0, 0);

	{
		Math::Vector3d v0(
			x - radius * cos((-angle * M_PI) / 2048.0f) * 0.5f,
			0.0f,
			y - radius * sin((-angle * M_PI) / 2048.0f) * 0.5f
		);
		g_driver->line(v0, v1);
	}

	{
		Math::Vector3d v0(
			x + radius * cos((-(angle-224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f,
			0.0f,
			y + radius * sin((-(angle-224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f
		);
		g_driver->line(v0, v1);
	}

	{
		Math::Vector3d v0(
			x + radius * cos((-(angle+224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f,
			0.0f,
			y + radius * sin((-(angle+224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f
		);
		g_driver->line(v0, v1);
	}
}

void ReevengiEngine::testView3DEnd(void) {
	g_driver->setColor(1.0, 1.0, 1.0);
}

} // End of namespace Reevengi
