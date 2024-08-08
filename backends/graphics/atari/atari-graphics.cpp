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

#define FORBIDDEN_SYMBOL_EXCEPTION_FILE // atari-graphics.h's unordered_set

#include "atari-graphics.h"

#include <mint/cookie.h>
#include <mint/falcon.h>
#include <mint/osbind.h>
#include <mint/sysvars.h>

#include "backends/platform/atari/dlmalloc.h"
#include "backends/keymapper/action.h"
#include "backends/keymapper/keymap.h"
#include "common/config-manager.h"
#include "common/str.h"
#include "common/textconsole.h"	// for warning() & error()
#include "common/translation.h"
#include "engines/engine.h"
#include "graphics/blit.h"
#include "gui/ThemeEngine.h"

#include "atari-graphics-superblitter.h"

#define SCREEN_ACTIVE

bool g_unalignedPitch = false;
mspace g_mspace = nullptr;

static const Graphics::PixelFormat PIXELFORMAT_CLUT8 = Graphics::PixelFormat::createFormatCLUT8();
static const Graphics::PixelFormat PIXELFORMAT_RGB332 = Graphics::PixelFormat(1, 3, 3, 2, 0, 5, 2, 0, 0);
static const Graphics::PixelFormat PIXELFORMAT_RGB121 = Graphics::PixelFormat(1, 1, 2, 1, 0, 3, 1, 0, 0);

static bool s_shrinkVidelVisibleArea;

static void shrinkVidelVisibleArea() {
	// Active VGA screen area consists of 960 half-lines, i.e. 480 raster lines.
	// In case of 320x240, the number is still 480 but data is fetched
	// only for 240 lines so it doesn't make a difference to us.

	if (hasSuperVidel()) {
		const int vOffset = ((480 - 400) / 2) * 2;	// *2 because of half-lines

		// VDB = VBE = VDB + paddding/2
		*((volatile uint16*)0xFFFF82A8) = *((volatile uint16*)0xFFFF82A6) = *((volatile uint16*)0xFFFF82A8) + vOffset;
		// VDE = VBB = VDE - padding/2
		*((volatile uint16*)0xFFFF82AA) = *((volatile uint16*)0xFFFF82A4) = *((volatile uint16*)0xFFFF82AA) - vOffset;
	} else {
		// 31500/60.1 = 524 raster lines
		// vft = 524 * 2 + 1 = 1049 half-lines
		// 480 visible lines = 960 half-lines
		// 1049 - 960 = 89 half-lines reserved for borders
		// we want 400 visible lines = 800 half-lines
		// vft = 800 + 89 = 889 half-lines in total ~ 70.1 Hz vertical frequency
		int16 vft = *((volatile int16*)0xFFFF82A2);
		int16 vss = *((volatile int16*)0xFFFF82AC);	// vss = vft - vss_sync
		vss -= vft;	// -vss_sync
		*((volatile int16*)0xFFFF82A2) = 889;
		*((volatile int16*)0xFFFF82AC) = 889 + vss;
	}
}

static bool s_tt;
static int s_shakeXOffset;
static int s_shakeYOffset;
static int s_aspectRatioCorrectionYOffset;
static Graphics::Surface *s_screenSurf;

static void VblHandler() {
	if (s_screenSurf) {
#ifdef SCREEN_ACTIVE
		uintptr p = (uintptr)s_screenSurf->getBasePtr(0, MAX_V_SHAKE + s_shakeYOffset + s_aspectRatioCorrectionYOffset);

		if (!s_tt) {
			const int bitsPerPixel = (s_screenSurf->format == PIXELFORMAT_RGB121 ? 4 : 8);

			s_shakeXOffset = -s_shakeXOffset;

			if (s_shakeXOffset >= 0) {
				p += MAX_HZ_SHAKE;
				*((volatile char *)0xFFFF8265) = s_shakeXOffset;
			} else {
				*((volatile char *)0xFFFF8265) = MAX_HZ_SHAKE + s_shakeXOffset;
			}

			// subtract 4 or 8 words if scrolling
			*((volatile short *)0xFFFF820E) = s_shakeXOffset == 0
			   ? (2 * MAX_HZ_SHAKE * bitsPerPixel / 8) / 2
			   : (2 * MAX_HZ_SHAKE * bitsPerPixel / 8) / 2 - bitsPerPixel;
		}

		union { byte c[4]; uintptr p; } sptr;
		sptr.p = p;

		*((volatile byte *)0xFFFF8201) = sptr.c[1];
		*((volatile byte *)0xFFFF8203) = sptr.c[2];
		*((volatile byte *)0xFFFF820D) = sptr.c[3];
#endif
		s_screenSurf = nullptr;
	}

	if (s_shrinkVidelVisibleArea) {
		shrinkVidelVisibleArea();
		s_shrinkVidelVisibleArea = false;
	}
}

static uint32 InstallVblHandler() {
	uint32 installed = 0;
	*vblsem = 0;  // lock vbl

	for (int i = 0; i < *nvbls; ++i) {
		if (!(*_vblqueue)[i]) {
			(*_vblqueue)[i] = VblHandler;
			installed = 1;
			break;
		}
	}

	*vblsem = 1;  // unlock vbl
	return installed;
}

static uint32 UninstallVblHandler() {
	uint32 uninstalled = 0;
	*vblsem = 0;  // lock vbl

	for (int i = 0; i < *nvbls; ++i) {
		if ((*_vblqueue)[i] == VblHandler) {
			(*_vblqueue)[i] = NULL;
			uninstalled = 1;
			break;
		}
	}

	*vblsem = 1;  // unlock vbl
	return uninstalled;
}

static int  s_oldRez = -1;
static int  s_oldMode = -1;
static void *s_oldPhysbase = nullptr;
static Palette s_oldPalette;

void AtariGraphicsShutdown() {
	Supexec(UninstallVblHandler);

	if (s_oldRez != -1) {
		Setscreen(SCR_NOCHANGE, s_oldPhysbase, s_oldRez);

		EsetPalette(0, s_oldPalette.entries, s_oldPalette.tt);
	} else if (s_oldMode != -1) {
		static _RGB black[256];
		VsetRGB(0, 256, black);

		VsetScreen(SCR_NOCHANGE, s_oldPhysbase, SCR_NOCHANGE, SCR_NOCHANGE);

		if (hasSuperVidel()) {
			// SuperVidel XBIOS does not restore those (unlike TOS/EmuTOS)
			long ssp = Super(SUP_SET);
			//*((volatile char *)0xFFFF8265) = 0;
			*((volatile short *)0xFFFF820E) = 0;
			Super(ssp);

			VsetMode(SVEXT | SVEXT_BASERES(0) | COL80 | BPS8C);	// resync to proper 640x480
		}
		VsetMode(s_oldMode);

		VsetRGB(0, s_oldPalette.entries, s_oldPalette.falcon);
	}
}

AtariGraphicsManager::AtariGraphicsManager() {
	debug("AtariGraphicsManager()");

	enum {
		VDO_NO_ATARI_HW = 0xffff,
		VDO_ST = 0,
		VDO_STE,
		VDO_TT,
		VDO_FALCON,
		VDO_MILAN
	};

	long vdo = VDO_NO_ATARI_HW<<16;
	Getcookie(C__VDO, &vdo);
	vdo >>= 16;

	_tt = (vdo == VDO_TT);
	s_tt = _tt;

	if (!_tt)
		_vgaMonitor = VgetMonitor() == MON_VGA;

	// no BDF scaling please
	ConfMan.registerDefault("gui_disable_fixed_font_scaling", true);

	// make the standard GUI renderer default (!DISABLE_FANCY_THEMES implies anti-aliased rendering in ThemeEngine.cpp)
	// (and without DISABLE_FANCY_THEMES we can't use 640x480 themes)
	const char *standardThemeEngineName = GUI::ThemeEngine::findModeConfigName(GUI::ThemeEngine::kGfxStandard);
	if (!ConfMan.hasKey("gui_renderer"))
		ConfMan.set("gui_renderer", standardThemeEngineName);

	// make the built-in theme default to avoid long loading times
	if (!ConfMan.hasKey("gui_theme"))
		ConfMan.set("gui_theme", "builtin");

#ifndef DISABLE_FANCY_THEMES
	// make "themes" the default theme path
	if (!ConfMan.hasKey("themepath"))
		ConfMan.setPath("themepath", "themes");
#endif

	ConfMan.flushToDisk();

	// Generate RGB332/RGB121 palette for the overlay
	const Graphics::PixelFormat &format = getOverlayFormat();
	const int paletteSize = getOverlayPaletteSize();
	for (int i = 0; i < paletteSize; i++) {
		if (_tt) {
			// Bits 15-12    Bits 11-8     Bits 7-4      Bits 3-0
			// Reserved      Red           Green         Blue
			_overlayPalette.tt[i] =  ((i >> format.rShift) & format.rMax()) << (8 + (format.rLoss - 4));
			_overlayPalette.tt[i] |= ((i >> format.gShift) & format.gMax()) << (4 + (format.gLoss - 4));
			_overlayPalette.tt[i] |= ((i >> format.bShift) & format.bMax()) << (0 + (format.bLoss - 4));
		} else {
			_overlayPalette.falcon[i].red    = ((i >> format.rShift) & format.rMax()) << format.rLoss;
			_overlayPalette.falcon[i].green |= ((i >> format.gShift) & format.gMax()) << format.gLoss;
			_overlayPalette.falcon[i].blue  |= ((i >> format.bShift) & format.bMax()) << format.bLoss;
		}
	}

	if (_tt) {
		s_oldRez = Getrez();
		// EgetPalette / EsetPalette doesn't care about current resolution's number of colors
		s_oldPalette.entries = 256;
		EgetPalette(0, 256, s_oldPalette.tt);
	} else {
		s_oldMode = VsetMode(VM_INQUIRE);
		switch (s_oldMode & NUMCOLS) {
		case BPS1:
			s_oldPalette.entries = 2;
			break;
		case BPS2:
			s_oldPalette.entries = 4;
			break;
		case BPS4:
			s_oldPalette.entries = 16;
			break;
		case BPS8:
		case BPS8C:
			s_oldPalette.entries = 256;
			break;
		default:
			s_oldPalette.entries = 0;
		}
		VgetRGB(0, s_oldPalette.entries, s_oldPalette.falcon);
	}
	s_oldPhysbase = Physbase();

	if (!Supexec(InstallVblHandler)) {
		error("VBL handler was not installed");
	}

	g_system->getEventManager()->getEventDispatcher()->registerObserver(this, 10, false);
}

AtariGraphicsManager::~AtariGraphicsManager() {
	debug("~AtariGraphicsManager()");

	g_system->getEventManager()->getEventDispatcher()->unregisterObserver(this);

	AtariGraphicsShutdown();
}

bool AtariGraphicsManager::hasFeature(OSystem::Feature f) const {
	switch (f) {
	case OSystem::Feature::kFeatureAspectRatioCorrection:
		//debug("hasFeature(kFeatureAspectRatioCorrection): %d", !_tt);
		return !_tt;
	case OSystem::Feature::kFeatureCursorPalette:
		// FIXME: pretend to have cursor palette at all times, this function
		// can get (and it is) called any time, before and after showOverlay()
		// (overlay cursor uses the cross if kFeatureCursorPalette returns false
		// here too soon)
		//debug("hasFeature(kFeatureCursorPalette): %d", isOverlayVisible());
		//return isOverlayVisible();
		return true;
	default:
		return false;
	}

	// TODO: kFeatureDisplayLogFile?, kFeatureClipboardSupport, kFeatureSystemBrowserDialog
}

void AtariGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
	if (!hasFeature(f))
		return;
	
	switch (f) {
	case OSystem::Feature::kFeatureAspectRatioCorrection:
		//debug("setFeatureState(kFeatureAspectRatioCorrection): %d", enable);
		_pendingState.aspectRatioCorrection = enable;

		if (_currentState.aspectRatioCorrection != _pendingState.aspectRatioCorrection)
			_pendingState.change |= GraphicsState::kAspectRatioCorrection;
		break;
	default:
		break;
	}
}

bool AtariGraphicsManager::getFeatureState(OSystem::Feature f) const {
	switch (f) {
	case OSystem::Feature::kFeatureAspectRatioCorrection:
		//debug("getFeatureState(kFeatureAspectRatioCorrection): %d", _aspectRatioCorrection);
		return _currentState.aspectRatioCorrection;
	case OSystem::Feature::kFeatureCursorPalette:
		//debug("getFeatureState(kFeatureCursorPalette): %d", isOverlayVisible());
		//return isOverlayVisible();
		return true;
	default:
		return false;
	}
}

bool AtariGraphicsManager::setGraphicsMode(int mode, uint flags) {
	debug("setGraphicsMode: %d, %d", mode, flags);

	_pendingState.mode = (GraphicsMode)mode;

	if (_currentState.mode != _pendingState.mode)
		_pendingState.change |= GraphicsState::kScreenAddress;

	// this doesn't seem to be checked anywhere
	return true;
}

void AtariGraphicsManager::initSize(uint width, uint height, const Graphics::PixelFormat *format) {
	debug("initSize: %d, %d, %d", width, height, format ? format->bytesPerPixel : 1);

	_pendingState.width = width;
	_pendingState.height = height;
	_pendingState.format = format ? *format : PIXELFORMAT_CLUT8;

	if ((_pendingState.width > 0 && _pendingState.height > 0)
		&& (_currentState.width != _pendingState.width || _currentState.height != _pendingState.height)) {
		_pendingState.change |= GraphicsState::kVideoMode;
	}
}

void AtariGraphicsManager::beginGFXTransaction() {
	debug("beginGFXTransaction");

	// these serve as a flag whether we are launching a game; if not, they will be always zeroed
	_pendingState.width = 0;
	_pendingState.height = 0;
	_pendingState.change &= ~GraphicsState::kVideoMode;
}

OSystem::TransactionError AtariGraphicsManager::endGFXTransaction() {
	debug("endGFXTransaction");

	int error = OSystem::TransactionError::kTransactionSuccess;

	if (_pendingState.mode < GraphicsMode::DirectRendering || _pendingState.mode > GraphicsMode::TripleBuffering)
		error |= OSystem::TransactionError::kTransactionModeSwitchFailed;

	if (_pendingState.format != PIXELFORMAT_CLUT8)
		error |= OSystem::TransactionError::kTransactionFormatNotSupported;

	if (_pendingState.width > 0 && _pendingState.height > 0) {
		if (_pendingState.width > getMaximumScreenWidth() || _pendingState.height > getMaximumScreenHeight())
			error |= OSystem::TransactionError::kTransactionSizeChangeFailed;

		if (_pendingState.width % 16 != 0 && !hasSuperVidel()) {
			warning("Requested width not divisible by 16, please report");
			error |= OSystem::TransactionError::kTransactionSizeChangeFailed;
		}
	}

	if (error != OSystem::TransactionError::kTransactionSuccess) {
		warning("endGFXTransaction failed: %02x", (int)error);
		// all our errors are fatal as we don't support rollback so make sure that
		// initGraphicsAny() fails (note: setupGraphics() doesn't check errors at all)
		error |= OSystem::TransactionError::kTransactionSizeChangeFailed;
		return static_cast<OSystem::TransactionError>(error);
	}

	// don't exit overlay unless there is real video mode to be set
	if (_pendingState.width == 0 || _pendingState.height == 0) {
		_ignoreHideOverlay = true;
		return OSystem::kTransactionSuccess;
	} else if (_overlayVisible) {
		// that's it, really. updateScreen() will take care of everything.
		_ignoreHideOverlay = false;
		_overlayVisible = false;
		// if being in the overlay, reset everything (same as hideOverlay() does)
		_pendingState.change |= GraphicsState::kAll;
	}

	_chunkySurface.init(_pendingState.width, _pendingState.height, _pendingState.width,
		_chunkySurface.getPixels(), _pendingState.format);

	_screen[FRONT_BUFFER]->reset(_pendingState.width, _pendingState.height, 8, true);
	_screen[BACK_BUFFER1]->reset(_pendingState.width, _pendingState.height, 8, true);
	_screen[BACK_BUFFER2]->reset(_pendingState.width, _pendingState.height, 8, true);
	_workScreen = _screen[_pendingState.mode <= GraphicsMode::SingleBuffering ? FRONT_BUFFER : BACK_BUFFER1];

	_palette.clear();
	_pendingState.change |= GraphicsState::kPalette;

	// no point of setting this in updateScreen(), it would only complicate code
	_currentState = _pendingState;
	// currently there is no use for this
	_currentState.change = GraphicsState::kNone;

	// apply new screen changes
	updateScreen();

	return OSystem::kTransactionSuccess;
}

void AtariGraphicsManager::setPalette(const byte *colors, uint start, uint num) {
	//debug("setPalette: %d, %d", start, num);

	if (_tt) {
		uint16 *pal = &_palette.tt[start];
		for (uint i = 0; i < num; ++i) {
			// Bits 15-12    Bits 11-8     Bits 7-4      Bits 3-0
			// Reserved      Red           Green         Blue
			pal[i]  = ((colors[i * 3 + 0] >> 4) & 0x0f) << 8;
			pal[i] |= ((colors[i * 3 + 1] >> 4) & 0x0f) << 4;
			pal[i] |= ((colors[i * 3 + 2] >> 4) & 0x0f);
		}
	} else {
		_RGB *pal = &_palette.falcon[start];
		for (uint i = 0; i < num; ++i) {
			pal[i].red   = colors[i * 3 + 0];
			pal[i].green = colors[i * 3 + 1];
			pal[i].blue  = colors[i * 3 + 2];
		}
	}

	_pendingState.change |= GraphicsState::kPalette;
}

void AtariGraphicsManager::grabPalette(byte *colors, uint start, uint num) const {
	//debug("grabPalette: %d, %d", start, num);

	if (_tt) {
		const uint16 *pal = &_palette.tt[start];
		for (uint i = 0; i < num; ++i) {
			// Bits 15-12    Bits 11-8     Bits 7-4      Bits 3-0
			// Reserved      Red           Green         Blue
			*colors++ = ((pal[i] >> 8) & 0x0f) << 4;
			*colors++ = ((pal[i] >> 4) & 0x0f) << 4;
			*colors++ = ((pal[i]     ) & 0x0f) << 4;
		}
	} else {
		const _RGB *pal = &_palette.falcon[start];
		for (uint i = 0; i < num; ++i) {
			*colors++ = pal[i].red;
			*colors++ = pal[i].green;
			*colors++ = pal[i].blue;
		}
	}
}

void AtariGraphicsManager::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
	//debug("copyRectToScreen: %d, %d, %d(%d), %d", x, y, w, pitch, h);

	copyRectToScreenInternal(buf, pitch, x, y, w, h,
		PIXELFORMAT_CLUT8,
		_currentState.mode == GraphicsMode::DirectRendering,
		_currentState.mode == GraphicsMode::TripleBuffering);
}

// this is not really locking anything but it's an useful function
// to return current rendering surface :)
Graphics::Surface *AtariGraphicsManager::lockScreen() {
	//debug("lockScreen");

	if (isOverlayVisible() && !isOverlayDirectRendering())
		return &_overlaySurface;
	else if ((isOverlayVisible() && isOverlayDirectRendering()) || _currentState.mode == GraphicsMode::DirectRendering)
		return _workScreen->offsettedSurf;
	else
		return &_chunkySurface;
}

void AtariGraphicsManager::unlockScreen() {
	//debug("unlockScreen: %d x %d", _workScreen->surf.w, _workScreen->surf.h);

	const Graphics::Surface &dstSurface = *lockScreen();

	const bool directRendering = (dstSurface.getPixels() != _chunkySurface.getPixels());
	const Common::Rect rect = alignRect(0, 0, dstSurface.w, dstSurface.h);
	_workScreen->addDirtyRect(dstSurface, rect, directRendering);

	if (_currentState.mode == GraphicsMode::TripleBuffering) {
		_screen[BACK_BUFFER2]->addDirtyRect(dstSurface, rect, directRendering);
		_screen[FRONT_BUFFER]->addDirtyRect(dstSurface, rect, directRendering);
	}

	// doc says:
	// Unlock the screen framebuffer, and mark it as dirty, i.e. during the
	// next updateScreen() call, the whole screen will be updated.
	//
	// ... so no updateScreen() from here (otherwise Eco Quest's intro is crawling!)
}

void AtariGraphicsManager::fillScreen(uint32 col) {
	debug("fillScreen: %d", col);

	Graphics::Surface *screen = lockScreen();
	screen->fillRect(Common::Rect(screen->w, screen->h), col);
	unlockScreen();
}

void AtariGraphicsManager::fillScreen(const Common::Rect &r, uint32 col) {
	debug("fillScreen: %dx%d %d", r.width(), r.height(), col);

	Graphics::Surface *screen = lockScreen();
	if (screen)
		screen->fillRect(r, col);
	unlockScreen();
}

void AtariGraphicsManager::updateScreen() {
	//debug("updateScreen");

	// avoid falling into the debugger (screen may not not initialized yet)
	Common::setErrorHandler(nullptr);

	if (_checkUnalignedPitch) {
		const Common::ConfigManager::Domain *activeDomain = ConfMan.getActiveDomain();
		if (activeDomain) {
			// FIXME: Some engines are too bound to linear surfaces that it is very
			// hard to repair them. So instead of polluting the engine with
			// Surface::init() & delete[] Surface::getPixels() just use this hack.
			const Common::String engineId = activeDomain->getValOrDefault("engineid");
			const Common::String gameId = activeDomain->getValOrDefault("gameid");

			debug("checking %s/%s", engineId.c_str(), gameId.c_str());

			if (engineId == "composer"
				|| engineId == "hypno"
				|| engineId == "mohawk"
				|| engineId == "parallaction"
				|| engineId == "private"
				|| (engineId == "sci"
					&& (gameId == "phantasmagoria" || gameId == "shivers"))
				|| engineId == "sherlock"
				|| engineId == "teenagent"
				|| engineId == "tsage") {
				g_unalignedPitch = true;
			}
		}

		_checkUnalignedPitch = false;
	}

	_workScreen->cursor.update();

	bool screenUpdated = false;

	if (isOverlayVisible()) {
		assert(_workScreen == _screen[OVERLAY_BUFFER]);
		if (isOverlayDirectRendering())
			screenUpdated = updateScreenInternal(Graphics::Surface());
		else
			screenUpdated = updateScreenInternal(_overlaySurface);
	} else {
		switch (_currentState.mode) {
		case GraphicsMode::DirectRendering:
			assert(_workScreen == _screen[FRONT_BUFFER]);
			screenUpdated = updateScreenInternal(Graphics::Surface());
			break;
		case GraphicsMode::SingleBuffering:
			assert(_workScreen == _screen[FRONT_BUFFER]);
			screenUpdated = updateScreenInternal(_chunkySurface);
			break;
		case GraphicsMode::TripleBuffering:
			assert(_workScreen == _screen[BACK_BUFFER1]);
			screenUpdated = updateScreenInternal(_chunkySurface);
			break;
		default:
			warning("Unknown graphics mode %d", (int)_currentState.mode);
		}
	}

	_workScreen->clearDirtyRects();

	if (!_overlayPending && (_pendingState.width == 0 || _pendingState.height == 0)) {
		return;
	}

	if (screenUpdated
		&& !isOverlayVisible()
		&& _currentState.mode == GraphicsMode::TripleBuffering) {
		// Triple buffer:
		// - alternate BACK_BUFFER1 and BACK_BUFFER2
		// - check if FRONT_BUFFER has been displayed for at least one frame
		// - display the most recent buffer (BACK_BUFFER2 in our case)
		// - alternate BACK_BUFFER2 and FRONT_BUFFER (only if BACK_BUFFER2
		//   has been updated)

		set_sysvar_to_short(vblsem, 0);  // lock vbl

		static long old_vbclock = get_sysvar(_vbclock);
		long curr_vbclock = get_sysvar(_vbclock);

		if (old_vbclock != curr_vbclock) {
			// at least one vbl has passed since setting new video base
			// guard BACK_BUFFER2 from overwriting while presented
			Screen *tmp = _screen[BACK_BUFFER2];
			_screen[BACK_BUFFER2] = _screen[FRONT_BUFFER];
			_screen[FRONT_BUFFER] = tmp;

			old_vbclock = curr_vbclock;
		}

		// swap back buffers
		Screen *tmp = _screen[BACK_BUFFER1];
		_screen[BACK_BUFFER1] = _screen[BACK_BUFFER2];
		_screen[BACK_BUFFER2] = tmp;

		// queue BACK_BUFFER2 with the most recent frame content
		s_screenSurf = &_screen[BACK_BUFFER2]->surf;

		set_sysvar_to_short(vblsem, 1);  // unlock vbl

		_workScreen = _screen[BACK_BUFFER1];
		// BACK_BUFFER2: now contains finished frame
		// FRONT_BUFFER is displayed and still contains previously finished frame
	}

	const GraphicsState oldPendingState = _pendingState;
	if (_overlayPending) {
		debug("Forcing overlay pending state");
		_pendingState.change = GraphicsState::kAll;
	}

	bool doShrinkVidelVisibleArea = false;
	bool doSuperVidelReset = false;
	if (_pendingState.change & GraphicsState::kAspectRatioCorrection) {
		assert(_workScreen->mode != -1);

		if (_pendingState.aspectRatioCorrection && _currentState.height == 200 && !isOverlayVisible()) {
			// apply machine-specific aspect ratio correction
			if (!_vgaMonitor) {
				_workScreen->mode &= ~PAL;
				// 60 Hz
				_workScreen->mode |= NTSC;
				_pendingState.change |= GraphicsState::kVideoMode;
			} else {
				Screen *screen = _screen[FRONT_BUFFER];
				s_aspectRatioCorrectionYOffset = (screen->surf.h - 2*MAX_V_SHAKE - screen->offsettedSurf->h) / 2;
				_pendingState.change |= GraphicsState::kShakeScreen;

				if (_pendingState.change & GraphicsState::kVideoMode)
					doShrinkVidelVisibleArea = true;
				else
					s_shrinkVidelVisibleArea = true;
			}
		} else {
			// reset back to default mode
			if (!_vgaMonitor) {
				_workScreen->mode &= ~NTSC;
				// 50 Hz
				_workScreen->mode |= PAL;
				_pendingState.change |= GraphicsState::kVideoMode;
			} else {
				s_aspectRatioCorrectionYOffset = 0;
				s_shrinkVidelVisibleArea = false;

				if (hasSuperVidel())
					doSuperVidelReset = true;
				_pendingState.change |= GraphicsState::kVideoMode;
			}
		}

		_pendingState.change &= ~GraphicsState::kAspectRatioCorrection;
	}

#ifdef SCREEN_ACTIVE
	if (_pendingState.change & GraphicsState::kVideoMode) {
		if (_workScreen->rez != -1) {
			// unfortunately this reinitializes VDI, too
			Setscreen(SCR_NOCHANGE, SCR_NOCHANGE, _workScreen->rez);

			// strictly speaking, this is necessary only if kScreenAddress is set but makes code easier
			static uint16 black[256];
			// Vsync();	// done by Setscreen() above
			EsetPalette(0, isOverlayVisible() ? 16 : 256, black);
		} else if (_workScreen->mode != -1) {
			// VsetMode() must be called first: it resets all hz/v, scrolling and line width registers
			// so even if kScreenAddress wasn't scheduled, we have to set new s_screenSurf to refresh them
			static _RGB black[256];
			VsetRGB(0, 256, black);
			// Vsync();	// done by (either) VsetMode() below

			if (doSuperVidelReset) {
				VsetMode(SVEXT | SVEXT_BASERES(0) | COL80 | BPS8C);	// resync to proper 640x480
				doSuperVidelReset = false;
			}

			debug("VsetMode: %04x", _workScreen->mode);
			VsetMode(_workScreen->mode);
		}

		// due to implied Vsync() above
		assert(s_screenSurf == nullptr);

		// refresh Videl register settings
		s_screenSurf = isOverlayVisible() ? &_screen[OVERLAY_BUFFER]->surf : &_screen[FRONT_BUFFER]->surf;
		s_shrinkVidelVisibleArea = doShrinkVidelVisibleArea;

		// keep kVideoMode for resetting the palette later
		_pendingState.change &= ~(GraphicsState::kScreenAddress | GraphicsState::kShakeScreen);
	}

	if (_pendingState.change & GraphicsState::kScreenAddress) {
		// takes effect in the nearest VBL interrupt but we always wait for Vsync() in this case
		Vsync();
		assert(s_screenSurf == nullptr);

		s_screenSurf = isOverlayVisible() ? &_screen[OVERLAY_BUFFER]->surf : &_screen[FRONT_BUFFER]->surf;
		_pendingState.change &= ~GraphicsState::kScreenAddress;
	}

	if (_pendingState.change & GraphicsState::kShakeScreen) {
		// takes effect in the nearest VBL interrupt
		if (!s_screenSurf)
			s_screenSurf = isOverlayVisible() ? &_screen[OVERLAY_BUFFER]->surf : &_screen[FRONT_BUFFER]->surf;
		_pendingState.change &= ~GraphicsState::kShakeScreen;
	}

	if (_pendingState.change & (GraphicsState::kVideoMode | GraphicsState::kPalette)) {
		if (!_tt) {
			// takes effect in the nearest VBL interrupt
			VsetRGB(0, isOverlayVisible() ? getOverlayPaletteSize() : 256, _workScreen->palette->falcon);
		} else {
			// takes effect immediatelly (it's possible that Vsync() hasn't been called: that's expected,
			// don't cripple framerate only for a palette change)
			EsetPalette(0, isOverlayVisible() ? getOverlayPaletteSize() : 256, _workScreen->palette->tt);
		}
		_pendingState.change &= ~(GraphicsState::kVideoMode | GraphicsState::kPalette);
	}
#endif

	if (_overlayPending) {
		_pendingState = oldPendingState;
		_overlayPending = false;
	}

	//debug("end of updateScreen");
}

void AtariGraphicsManager::setShakePos(int shakeXOffset, int shakeYOffset) {
	//debug("setShakePos: %d, %d", shakeXOffset, shakeYOffset);

	if (_tt) {
		// as TT can't horizontally shake anything, do it at least vertically
		s_shakeYOffset = (shakeYOffset == 0 && shakeXOffset != 0) ? shakeXOffset : shakeYOffset;
	} else {
		s_shakeXOffset = shakeXOffset;
		s_shakeYOffset = shakeYOffset;
	}

	_pendingState.change |= GraphicsState::kShakeScreen;
}

void AtariGraphicsManager::showOverlay(bool inGUI) {
	debug("showOverlay (visible: %d)", _overlayVisible);

	if (_overlayVisible)
		return;

	if (_currentState.mode == GraphicsMode::DirectRendering) {
		_workScreen->cursor.restoreBackground(Graphics::Surface(), true);
	}

	_oldWorkScreen = _workScreen;
	_workScreen = _screen[OVERLAY_BUFFER];

	// do not cache dirtyRects and oldCursorRect
	_workScreen->reset(getOverlayWidth(), getOverlayHeight(), getBitsPerPixel(getOverlayFormat()), false);

	_overlayVisible = true;

	assert(_pendingState.change == GraphicsState::kNone);
	_overlayPending = true;
	updateScreen();
}

void AtariGraphicsManager::hideOverlay() {
	debug("hideOverlay (ignore: %d, visible: %d)", _ignoreHideOverlay, _overlayVisible);

	if (!_overlayVisible)
		return;

	if (_ignoreHideOverlay) {
		// faster than _workScreen->reset()
		_workScreen->clearDirtyRects();
		_workScreen->cursor.reset();
		return;
	}

	_workScreen = _oldWorkScreen;
	_oldWorkScreen = nullptr;

	// FIXME: perhaps there's a better way but this will do for now
	_checkUnalignedPitch = true;

	_overlayVisible = false;

	assert(_pendingState.change == GraphicsState::kNone);
	_pendingState.change = GraphicsState::kAll;
	updateScreen();
}

Graphics::PixelFormat AtariGraphicsManager::getOverlayFormat() const {
#ifndef DISABLE_FANCY_THEMES
	return _tt ? PIXELFORMAT_RGB121 : PIXELFORMAT_RGB332;
#else
	return PIXELFORMAT_RGB121;
#endif
}

void AtariGraphicsManager::clearOverlay() {
	if (isOverlayDirectRendering())
		return;

	debug("clearOverlay");

	if (!_overlayVisible)
		return;

	const Graphics::Surface &sourceSurface =
		_currentState.mode == GraphicsMode::DirectRendering ? *_screen[FRONT_BUFFER]->offsettedSurf : _chunkySurface;

	const bool upscale = _overlaySurface.w / sourceSurface.w >= 2 && _overlaySurface.h / sourceSurface.h >= 2;

	const int w = upscale ? sourceSurface.w * 2 : sourceSurface.w;
	const int h = upscale ? sourceSurface.h * 2 : sourceSurface.h;

	const int hzOffset = (_overlaySurface.w - w) / 2;
	const int vOffset  = (_overlaySurface.h - h) / 2;

	const int srcPadding = sourceSurface.pitch - sourceSurface.w;
	const int dstPadding = hzOffset * 2 + (upscale ? _overlaySurface.pitch : 0);

	// Transpose from game palette to RGB332/RGB121 (overlay palette)
	const byte *src = (const byte*)sourceSurface.getPixels();
	byte *dst = (byte *)_overlaySurface.getBasePtr(hzOffset, vOffset);

	// for TT: 8/4/0 + (xLoss - 4) + xShift
	static const int rShift = (_tt ? (8 - 4) : 0)
		+ _overlaySurface.format.rLoss - _overlaySurface.format.rShift;
	static const int gShift = (_tt ? (4 - 4) : 0)
		+ _overlaySurface.format.gLoss - _overlaySurface.format.gShift;
	static const int bShift = (_tt ? (0 - 4) : 0)
		+ _overlaySurface.format.bLoss - _overlaySurface.format.bShift;

	static const int rMask = _overlaySurface.format.rMax() << _overlaySurface.format.rShift;
	static const int gMask = _overlaySurface.format.gMax() << _overlaySurface.format.gShift;
	static const int bMask = _overlaySurface.format.bMax() << _overlaySurface.format.bShift;

	for (int y = 0; y < sourceSurface.h; y++) {
		for (int x = 0; x < sourceSurface.w; x++) {
			byte pixel;

			if (_tt) {
				// Bits 15-12    Bits 11-8     Bits 7-4      Bits 3-0
				// Reserved      Red           Green         Blue
				const uint16 &col = _palette.tt[*src++];
				pixel = ((col >> rShift) & rMask)
					  | ((col >> gShift) & gMask)
					  | ((col >> bShift) & bMask);
			} else {
				const _RGB &col = _palette.falcon[*src++];
				pixel = ((col.red   >> rShift) & rMask)
					  | ((col.green >> gShift) & gMask)
					  | ((col.blue  >> bShift) & bMask);
			}

			if (upscale) {
				*(dst + _overlaySurface.pitch) = pixel;
				*dst++ = pixel;
				*(dst + _overlaySurface.pitch) = pixel;
			}
			*dst++ = pixel;
		}

		src += srcPadding;
		dst += dstPadding;
	}

	// top rect
	memset(_overlaySurface.getBasePtr(0, 0), 0, vOffset * _overlaySurface.pitch);
	// bottom rect
	memset(_overlaySurface.getBasePtr(0, _overlaySurface.h - vOffset), 0, vOffset * _overlaySurface.pitch);
	// left rect
	_overlaySurface.fillRect(Common::Rect(0, vOffset, hzOffset, _overlaySurface.h - vOffset), 0);
	// right rect
	_overlaySurface.fillRect(Common::Rect(_overlaySurface.w - hzOffset, vOffset, _overlaySurface.w, _overlaySurface.h - vOffset), 0);

	_screen[OVERLAY_BUFFER]->addDirtyRect(_overlaySurface, Common::Rect(_overlaySurface.w, _overlaySurface.h), false);
}

void AtariGraphicsManager::grabOverlay(Graphics::Surface &surface) const {
	debug("grabOverlay: %d(%d), %d", surface.w, surface.pitch, surface.h);

	if (isOverlayDirectRendering()) {
		memset(surface.getPixels(), 0, surface.h * surface.pitch);
	} else {
		assert(surface.w >= _overlaySurface.w);
		assert(surface.h >= _overlaySurface.h);
		assert(surface.format.bytesPerPixel == _overlaySurface.format.bytesPerPixel);

		const byte *src = (const byte *)_overlaySurface.getPixels();
		byte *dst = (byte *)surface.getPixels();
		Graphics::copyBlit(dst, src, surface.pitch,
			_overlaySurface.pitch, _overlaySurface.w, _overlaySurface.h, _overlaySurface.format.bytesPerPixel);
	}
}

void AtariGraphicsManager::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
	//debug("copyRectToOverlay: %d, %d, %d(%d), %d", x, y, w, pitch, h);

	copyRectToScreenInternal(buf, pitch, x, y, w, h,
		getOverlayFormat(),
		isOverlayDirectRendering(),
		false);
}

bool AtariGraphicsManager::showMouse(bool visible) {
	//debug("showMouse: %d", visible);

	bool last = _workScreen->cursor.setVisible(visible);

	if (!isOverlayVisible() && _currentState.mode == GraphicsMode::TripleBuffering) {
		_screen[BACK_BUFFER2]->cursor.setVisible(visible);
		_screen[FRONT_BUFFER]->cursor.setVisible(visible);
	}

	// don't rely on engines to call it (if they don't it confuses the cursor restore logic)
	updateScreen();

	return last;
}

void AtariGraphicsManager::warpMouse(int x, int y) {
	//debug("warpMouse: %d, %d", x, y);

	_workScreen->cursor.setPosition(x, y);

	if (!isOverlayVisible() && _currentState.mode == GraphicsMode::TripleBuffering) {
		_screen[BACK_BUFFER2]->cursor.setPosition(x, y);
		_screen[FRONT_BUFFER]->cursor.setPosition(x, y);
	}
}

void AtariGraphicsManager::setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor,
										  bool dontScale, const Graphics::PixelFormat *format, const byte *mask) {
	//debug("setMouseCursor: %d, %d, %d, %d, %d, %d", w, h, hotspotX, hotspotY, keycolor, format ? format->bytesPerPixel : 1);

	if (mask)
		warning("AtariGraphicsManager::setMouseCursor: Masks are not supported");

	if (format)
		assert(*format == PIXELFORMAT_CLUT8);

	_workScreen->cursor.setSurface(buf, (int)w, (int)h, hotspotX, hotspotY, keycolor);

	if (!isOverlayVisible() && _currentState.mode == GraphicsMode::TripleBuffering) {
		_screen[BACK_BUFFER2]->cursor.setSurface(buf, (int)w, (int)h, hotspotX, hotspotY, keycolor);
		_screen[FRONT_BUFFER]->cursor.setSurface(buf, (int)w, (int)h, hotspotX, hotspotY, keycolor);
	}
}

void AtariGraphicsManager::setCursorPalette(const byte *colors, uint start, uint num) {
	debug("setCursorPalette: %d, %d", start, num);

	_workScreen->cursor.setPalette(colors, start, num);

	if (!isOverlayVisible() && _currentState.mode == GraphicsMode::TripleBuffering) {
		// avoid copying the same (shared) palette two more times...
		_screen[BACK_BUFFER2]->cursor.setPalette(nullptr, 0, 0);
		_screen[FRONT_BUFFER]->cursor.setPalette(nullptr, 0, 0);
	}
}

void AtariGraphicsManager::updateMousePosition(int deltaX, int deltaY) {
	_workScreen->cursor.updatePosition(deltaX, deltaY);

	if (!isOverlayVisible() && _currentState.mode == GraphicsMode::TripleBuffering) {
		_screen[BACK_BUFFER2]->cursor.updatePosition(deltaX, deltaY);
		_screen[FRONT_BUFFER]->cursor.updatePosition(deltaX, deltaY);
	}
}

bool AtariGraphicsManager::notifyEvent(const Common::Event &event) {
	switch (event.type) {
	case Common::EVENT_RETURN_TO_LAUNCHER:
	case Common::EVENT_QUIT:
		if (isOverlayVisible()) {
			_ignoreHideOverlay = true;
			return false;
		}
		break;

	case Common::EVENT_CUSTOM_BACKEND_ACTION_START:
		switch ((CustomEventAction) event.customType) {
		case kActionToggleAspectRatioCorrection:
			if (hasFeature(OSystem::Feature::kFeatureAspectRatioCorrection)) {
				_pendingState.aspectRatioCorrection = !_pendingState.aspectRatioCorrection;

				if (_currentState.aspectRatioCorrection != _pendingState.aspectRatioCorrection) {
					_pendingState.change |= GraphicsState::kAspectRatioCorrection;

					// would be updated in updateScreen() anyway
					_currentState.aspectRatioCorrection = _pendingState.aspectRatioCorrection;
					updateScreen();
				}
				return true;
			}
			break;
		}
		break;

	default:
		return false;
	}

	return false;
}

Common::Keymap *AtariGraphicsManager::getKeymap() const {
	Common::Keymap *keymap = new Common::Keymap(Common::Keymap::kKeymapTypeGlobal, "atari-graphics", _("Graphics"));
	Common::Action *act;

	if (hasFeature(OSystem::kFeatureAspectRatioCorrection)) {
		act = new Common::Action("ASPT", _("Toggle aspect ratio correction"));
		act->addDefaultInputMapping("C+A+a");
		act->setCustomBackendActionEvent(kActionToggleAspectRatioCorrection);
		keymap->addAction(act);
	}

	return keymap;
}

void AtariGraphicsManager::allocateSurfaces() {
	for (int i : { FRONT_BUFFER, BACK_BUFFER1, BACK_BUFFER2 }) {
		_screen[i] = new Screen(this, getMaximumScreenWidth(), getMaximumScreenHeight(), PIXELFORMAT_CLUT8, &_palette);
	}

	// overlay is the default screen upon start
	_workScreen = _screen[OVERLAY_BUFFER] = new Screen(this, getOverlayWidth(), getOverlayHeight(), getOverlayFormat(), &_overlayPalette);
	_workScreen->reset(getOverlayWidth(), getOverlayHeight(), getBitsPerPixel(getOverlayFormat()), true);

	_chunkySurface.create(getMaximumScreenWidth(), getMaximumScreenHeight(), PIXELFORMAT_CLUT8);
	_overlaySurface.create(getOverlayWidth(), getOverlayHeight(), getOverlayFormat());
}

void AtariGraphicsManager::freeSurfaces() {
	for (int i : { FRONT_BUFFER, BACK_BUFFER1, BACK_BUFFER2, OVERLAY_BUFFER }) {
		delete _screen[i];
		_screen[i] = nullptr;
	}
	_workScreen = nullptr;

	_chunkySurface.free();
	_overlaySurface.free();
}

bool AtariGraphicsManager::updateScreenInternal(const Graphics::Surface &srcSurface) {
	//debug("updateScreenInternal");

	const Screen::DirtyRects &dirtyRects = _workScreen->dirtyRects;
	Graphics::Surface *dstSurface        = _workScreen->offsettedSurf;
	Cursor &cursor                       = _workScreen->cursor;

	const bool directRendering           = srcSurface.getPixels() == nullptr;
	const int dstBitsPerPixel            = getBitsPerPixel(dstSurface->format);

	bool updated = false;

	const bool cursorDrawEnabled = cursor.isVisible();
	bool forceCursorDraw = cursorDrawEnabled && (_workScreen->fullRedraw || cursor.isChanged());

	lockSuperBlitter();

	for (auto it = dirtyRects.begin(); it != dirtyRects.end(); ++it) {
		if (cursorDrawEnabled && !forceCursorDraw)
			forceCursorDraw = cursor.intersects(*it);

		if (!directRendering) {
			copyRectToSurface(*dstSurface, dstBitsPerPixel, srcSurface, it->left, it->top, *it);
			updated |= true;
		}
	}

	updated |= cursor.restoreBackground(srcSurface, false);

	unlockSuperBlitter();

	updated |= cursor.draw(directRendering, forceCursorDraw);

	return updated;
}

void AtariGraphicsManager::copyRectToScreenInternal(const void *buf, int pitch, int x, int y, int w, int h,
													const Graphics::PixelFormat &format, bool directRendering, bool tripleBuffer) {
	Graphics::Surface &dstSurface = *lockScreen();

	const Common::Rect rect = alignRect(x, y, w, h);
	_workScreen->addDirtyRect(dstSurface, rect, directRendering);

	if (tripleBuffer) {
		_screen[BACK_BUFFER2]->addDirtyRect(dstSurface, rect, directRendering);
		_screen[FRONT_BUFFER]->addDirtyRect(dstSurface, rect, directRendering);
	}

	if (directRendering) {
		// TODO: mask the unaligned parts and copy the rest
		Graphics::Surface srcSurface;
		byte *srcBuf = (byte *)const_cast<void *>(buf);
		srcBuf -= (x - rect.left);	// HACK: this assumes pointer to a complete buffer
		srcSurface.init(rect.width(), rect.height(), pitch, srcBuf, format);

		copyRectToSurface(
			dstSurface, getBitsPerPixel(format), srcSurface,
			rect.left, rect.top,
			Common::Rect(rect.width(), rect.height()));
	} else {
		dstSurface.copyRectToSurface(buf, pitch, x, y, w, h);
	}
}

int AtariGraphicsManager::getBitsPerPixel(const Graphics::PixelFormat &format) const {
	return format == PIXELFORMAT_RGB121 ? 4 : 8;
}

bool AtariGraphicsManager::isOverlayDirectRendering() const {
	// overlay is direct rendered if in the launcher or if game is directly rendered
	// (on SuperVidel we always want to use shading/transparency but its direct rendering is fine and supported)
	return !hasSuperVidel()
#ifndef DISABLE_FANCY_THEMES
		   && (ConfMan.getActiveDomain() == nullptr || _currentState.mode == GraphicsMode::DirectRendering)
#endif
		;
}
