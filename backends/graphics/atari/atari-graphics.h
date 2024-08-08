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

#ifndef BACKENDS_GRAPHICS_ATARI_H
#define BACKENDS_GRAPHICS_ATARI_H

#include "backends/graphics/graphics.h"
#include "common/events.h"

#include <mint/osbind.h>

#include "common/rect.h"
#include "graphics/surface.h"

#include "atari-cursor.h"
#include "atari-screen.h"

#define MAX_HZ_SHAKE 16 // Falcon only
#define MAX_V_SHAKE  16

class AtariGraphicsManager : public GraphicsManager, Common::EventObserver {
	friend class Cursor;
	friend class Screen;

public:
	AtariGraphicsManager();
	virtual ~AtariGraphicsManager();

	bool hasFeature(OSystem::Feature f) const override;
	void setFeatureState(OSystem::Feature f, bool enable) override;
	bool getFeatureState(OSystem::Feature f) const override;

	const OSystem::GraphicsMode *getSupportedGraphicsModes() const override {
		static const OSystem::GraphicsMode graphicsModes[] = {
			{ "direct", "Direct rendering", (int)GraphicsMode::DirectRendering },
			{ "single", "Single buffering", (int)GraphicsMode::SingleBuffering },
			{ "triple", "Triple buffering", (int)GraphicsMode::TripleBuffering },
			{ nullptr, nullptr, 0 }
		};
		return graphicsModes;
	}
	int getDefaultGraphicsMode() const override { return (int)GraphicsMode::TripleBuffering; }
	bool setGraphicsMode(int mode, uint flags = OSystem::kGfxModeNoFlags) override;
	int getGraphicsMode() const override { return (int)_currentState.mode; }

	void initSize(uint width, uint height, const Graphics::PixelFormat *format = NULL) override;

	int getScreenChangeID() const override { return 0; }

	void beginGFXTransaction() override;
	OSystem::TransactionError endGFXTransaction() override;

	int16 getHeight() const override { return _currentState.height; }
	int16 getWidth() const override { return _currentState.width; }
	void setPalette(const byte *colors, uint start, uint num) override;
	void grabPalette(byte *colors, uint start, uint num) const override;
	void copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) override;
	Graphics::Surface *lockScreen() override;
	void unlockScreen() override;
	void fillScreen(uint32 col) override;
	void fillScreen(const Common::Rect &r, uint32 col) override;
	void updateScreen() override;
	void setShakePos(int shakeXOffset, int shakeYOffset) override;
	void setFocusRectangle(const Common::Rect& rect) override {}
	void clearFocusRectangle() override {}

	void showOverlay(bool inGUI) override;
	void hideOverlay() override;
	bool isOverlayVisible() const override { return _overlayVisible; }
	Graphics::PixelFormat getOverlayFormat() const override;
	void clearOverlay() override;
	void grabOverlay(Graphics::Surface &surface) const override;
	void copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) override;
	int16 getOverlayHeight() const override { return 480; }
	int16 getOverlayWidth() const override { return _vgaMonitor ? 640 : 640*1.2; }

	bool showMouse(bool visible) override;
	void warpMouse(int x, int y) override;
	void setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor,
						bool dontScale = false, const Graphics::PixelFormat *format = NULL, const byte *mask = NULL) override;
	void setCursorPalette(const byte *colors, uint start, uint num) override;

	Common::Point getMousePosition() const { return _workScreen->cursor.getPosition(); }
	void updateMousePosition(int deltaX, int deltaY);

	bool notifyEvent(const Common::Event &event) override;
	Common::Keymap *getKeymap() const;

protected:
	typedef void* (*AtariMemAlloc)(size_t bytes);
	typedef void (*AtariMemFree)(void *ptr);

	void allocateSurfaces();
	void freeSurfaces();

private:
	enum class GraphicsMode : int {
		Unknown			= -1,
		DirectRendering = 0,
		SingleBuffering = 1,
		TripleBuffering = 3
	};

	enum CustomEventAction {
		kActionToggleAspectRatioCorrection = 100,
	};

#ifndef DISABLE_FANCY_THEMES
	int16 getMaximumScreenHeight() const { return 480; }
	int16 getMaximumScreenWidth() const { return _tt ? 320 : (_vgaMonitor ? 640 : 640*1.2); }
#else
	int16 getMaximumScreenHeight() const { return _tt ? 480 : 240; }
	int16 getMaximumScreenWidth() const { return _tt ? 320 : (_vgaMonitor ? 320 : 320*1.2); }
#endif

	bool updateScreenInternal(const Graphics::Surface &srcSurface);

	void copyRectToScreenInternal(const void *buf, int pitch, int x, int y, int w, int h,
								  const Graphics::PixelFormat &format, bool directRendering, bool tripleBuffer);

	int getBitsPerPixel(const Graphics::PixelFormat &format) const;

	bool isOverlayDirectRendering() const;

	virtual AtariMemAlloc getStRamAllocFunc() const {
		return [](size_t bytes) { return (void*)Mxalloc(bytes, MX_STRAM); };
	}
	virtual AtariMemFree getStRamFreeFunc() const {
		return [](void *ptr) { Mfree(ptr); };
	}

	virtual void copyRectToSurface(Graphics::Surface &dstSurface, int dstBitsPerPixel, const Graphics::Surface &srcSurface,
								   int destX, int destY,
								   const Common::Rect &subRect) const {
		dstSurface.copyRectToSurface(srcSurface, destX, destY, subRect);
	}

	virtual void drawMaskedSprite(Graphics::Surface &dstSurface, int dstBitsPerPixel,
								  const Graphics::Surface &srcSurface, const Graphics::Surface &srcMask,
								  int destX, int destY,
								  const Common::Rect &subRect) = 0;

	virtual Common::Rect alignRect(int x, int y, int w, int h) const = 0;

	Common::Rect alignRect(const Common::Rect &rect) const {
		return alignRect(rect.left, rect.top, rect.width(), rect.height());
	}

	int getOverlayPaletteSize() const {
#ifndef DISABLE_FANCY_THEMES
		return _tt ? 16 : 256;
#else
		return 16;
#endif
	}

	bool _vgaMonitor = true;
	bool _tt = false;
	bool _checkUnalignedPitch = false;

	struct GraphicsState {
		GraphicsMode mode = GraphicsMode::Unknown;
		int width = 0;
		int height = 0;
		Graphics::PixelFormat format;
		bool aspectRatioCorrection = false;

		enum PendingScreenChange {
			kNone					= 0,
			kVideoMode				= 1<<0,
			kScreenAddress			= 1<<1,
			kPalette				= 1<<2,
			kAspectRatioCorrection	= 1<<3,
			kShakeScreen            = 1<<4,
			kAll					= kVideoMode | kScreenAddress | kPalette | kAspectRatioCorrection | kShakeScreen,
		};
		int change = kNone;
	};
	GraphicsState _pendingState;
	GraphicsState _currentState;

	enum {
		FRONT_BUFFER,
		BACK_BUFFER1,
		BACK_BUFFER2,
		OVERLAY_BUFFER,
		BUFFER_COUNT
	};
	Screen *_screen[BUFFER_COUNT] = {};
	Screen *_workScreen = nullptr;
	Screen *_oldWorkScreen = nullptr;	// used in hideOverlay()

	Graphics::Surface _chunkySurface;

	bool _overlayVisible = true;
	bool _overlayPending = true;
	bool _ignoreHideOverlay = true;
	Graphics::Surface _overlaySurface;

	Palette _palette;
	Palette _overlayPalette;
};

#endif
