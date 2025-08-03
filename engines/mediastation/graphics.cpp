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
#include "common/util.h"
#include "engines/util.h"

#include "mediastation/assets/palette.h"
#include "mediastation/bitmap.h"
#include "mediastation/debugchannels.h"
#include "mediastation/dissolvepatterns.h"
#include "mediastation/graphics.h"
#include "mediastation/mediastation.h"

namespace MediaStation {

VideoDisplayManager::VideoDisplayManager(MediaStationEngine *vm) : _vm(vm) {
	initGraphics(SCREEN_WIDTH, SCREEN_HEIGHT);
	_screen = new Graphics::Screen();
}

VideoDisplayManager::~VideoDisplayManager() {
	delete _screen;
	_screen = nullptr;
	_vm = nullptr;
}

void VideoDisplayManager::effectTransition(Common::Array<ScriptValue> &args) {
	if (args.empty()) {
		warning("effectTransition: Script args cannot be empty");
		return;
	}

	TransitionType transitionType = static_cast<TransitionType>(args[0].asParamToken());
	switch (transitionType) {
	case kTransitionFadeToBlack:
		fadeToBlack(args);
		break;

	case kTransitionFadeToPalette:
		fadeToRegisteredPalette(args);
		break;

	case kTransitionSetToPalette:
		setToRegisteredPalette(args);
		break;

	case kTransitionSetToBlack:
		setToBlack(args);
		break;

	case kTransitionFadeToColor:
		fadeToColor(args);
		break;

	case kTransitionSetToColor:
		setToColor(args);
		break;

	case kTransitionSetToPercentOfPalette:
		setToPercentOfPalette(args);
		break;

	case kTransitionFadeToPaletteObject:
		fadeToPaletteObject(args);
		break;

	case kTransitionSetToPaletteObject:
		setToPaletteObject(args);
		break;

	case kTransitionSetToPercentOfPaletteObject:
		setToPercentOfPaletteObject(args);
		break;

	case kTransitionColorShiftCurrentPalette:
		colorShiftCurrentPalette(args);
		break;

	case kTransitionCircleOut:
		circleOut(args);
		break;

	default:
		warning("effectTransition: Got unknown transition type %d", static_cast<uint>(transitionType));
	}
}

void VideoDisplayManager::doTransitionOnSync() {
	if (!_scheduledTransitionOnSync.empty()) {
		effectTransition(_scheduledTransitionOnSync);
		_scheduledTransitionOnSync.clear();
	}
}

void VideoDisplayManager::fadeToBlack(Common::Array<ScriptValue> &args) {
	double fadeTime = DEFAULT_FADE_TRANSITION_TIME_IN_SECONDS;
	uint startIndex = DEFAULT_PALETTE_TRANSITION_START_INDEX;
	uint colorCount = DEFAULT_PALETTE_TRANSITION_COLOR_COUNT;

	if (args.size() >= 2) {
		fadeTime = args[1].asTime();
	}
	if (args.size() >= 4) {
		startIndex = static_cast<uint>(args[2].asFloat());
		colorCount = static_cast<uint>(args[3].asFloat());
	}

	_fadeToColor(0, 0, 0, fadeTime, startIndex, colorCount);
}

void VideoDisplayManager::fadeToRegisteredPalette(Common::Array<ScriptValue> &args) {
	double fadeTime = DEFAULT_FADE_TRANSITION_TIME_IN_SECONDS;
	uint startIndex = DEFAULT_PALETTE_TRANSITION_START_INDEX;
	uint colorCount = DEFAULT_PALETTE_TRANSITION_COLOR_COUNT;

	if (args.size() >= 2) {
		fadeTime = args[1].asTime();
	}
	if (args.size() >= 4) {
		startIndex = static_cast<uint>(args[2].asFloat());
		colorCount = static_cast<uint>(args[3].asFloat());
	}

	_fadeToRegisteredPalette(fadeTime, startIndex, colorCount);
}

void VideoDisplayManager::setToRegisteredPalette(Common::Array<ScriptValue> &args) {
	uint startIndex = DEFAULT_PALETTE_TRANSITION_START_INDEX;
	uint colorCount = DEFAULT_PALETTE_TRANSITION_COLOR_COUNT;

	if (args.size() >= 3) {
		startIndex = static_cast<uint>(args[1].asFloat());
		colorCount = static_cast<uint>(args[2].asFloat());
	}

	_setToRegisteredPalette(startIndex, colorCount);
}

void VideoDisplayManager::setToBlack(Common::Array<ScriptValue> &args) {
	uint startIndex = DEFAULT_PALETTE_TRANSITION_START_INDEX;
	uint colorCount = DEFAULT_PALETTE_TRANSITION_COLOR_COUNT;

	if (args.size() >= 3) {
		startIndex = static_cast<uint>(args[1].asFloat());
		colorCount = static_cast<uint>(args[2].asFloat());
	}

	_setToColor(0, 0, 0, startIndex, colorCount);
}

void VideoDisplayManager::fadeToColor(Common::Array<ScriptValue> &args) {
	byte r = 0, g = 0, b = 0;
	double fadeTime = DEFAULT_FADE_TRANSITION_TIME_IN_SECONDS;
	uint startIndex = DEFAULT_PALETTE_TRANSITION_START_INDEX;
	uint colorCount = DEFAULT_PALETTE_TRANSITION_COLOR_COUNT;

	if (args.size() >= 5) {
		r = static_cast<byte>(args[1].asFloat());
		g = static_cast<byte>(args[2].asFloat());
		b = static_cast<byte>(args[3].asFloat());
		fadeTime = args[4].asTime();
	}
	if (args.size() >= 7) {
		fadeTime = args[5].asTime();
		startIndex = static_cast<uint>(args[6].asFloat());
		colorCount = static_cast<uint>(args[7].asFloat());
	}

	_fadeToColor(r, g, b, fadeTime, startIndex, colorCount);
}

void VideoDisplayManager::setToColor(Common::Array<ScriptValue> &args) {
	if (args.size() < 6) {
		error("setToColor: Too few script args");
	}

	byte r = static_cast<byte>(args[1].asFloat());
	byte g = static_cast<byte>(args[2].asFloat());
	byte b = static_cast<byte>(args[3].asFloat());
	uint startIndex = static_cast<uint>(args[4].asFloat());
	uint colorCount = static_cast<uint>(args[5].asFloat());

	_setToColor(r, g, b, startIndex, colorCount);
}

void VideoDisplayManager::setToPercentOfPalette(Common::Array<ScriptValue> &args) {
	if (args.size() < 7) {
		error("setToPercentOfPalette: Too few script args");
	}

	double percent = args[1].asFloat();
	byte r = static_cast<byte>(args[2].asFloat());
	byte g = static_cast<byte>(args[3].asFloat());
	byte b = static_cast<byte>(args[4].asFloat());
	uint startIndex = static_cast<uint>(args[5].asFloat());
	uint colorCount = static_cast<uint>(args[6].asFloat());

	_setPercentToColor(percent, r, g, b, startIndex, colorCount);
}

void VideoDisplayManager::fadeToPaletteObject(Common::Array<ScriptValue> &args) {
	uint paletteId = 0;
	double fadeTime = DEFAULT_FADE_TRANSITION_TIME_IN_SECONDS;
	uint startIndex = DEFAULT_PALETTE_TRANSITION_START_INDEX;
	uint colorCount = DEFAULT_PALETTE_TRANSITION_COLOR_COUNT;

	if (args.size() >= 2) {
		paletteId = args[1].asAssetId();
	} else {
		warning("fadeToPaletteObject: Too few script args");
		return;
	}
	if (args.size() >= 3) {
		fadeTime = args[2].asFloat();
	}
	if (args.size() >= 5) {
		startIndex = static_cast<uint>(args[3].asFloat());
		colorCount = static_cast<uint>(args[4].asFloat());
	}

	_fadeToPaletteObject(paletteId, fadeTime, startIndex, colorCount);
}

void VideoDisplayManager::setToPaletteObject(Common::Array<ScriptValue> &args) {
	uint paletteId = 0;
	uint startIndex = DEFAULT_PALETTE_TRANSITION_START_INDEX;
	uint colorCount = DEFAULT_PALETTE_TRANSITION_COLOR_COUNT;

	if (args.size() >= 2) {
		paletteId = args[1].asAssetId();
	} else {
		warning("fadeToPaletteObject: Too few script args");
		return;
	}
	if (args.size() >= 4) {
		startIndex = static_cast<uint>(args[2].asFloat());
		colorCount = static_cast<uint>(args[3].asFloat());
	}

	_setToPaletteObject(paletteId, startIndex, colorCount);
}

void VideoDisplayManager::setToPercentOfPaletteObject(Common::Array<ScriptValue> &args) {
	uint paletteId = 0;
	double percent = 0.0;
	uint startIndex = DEFAULT_PALETTE_TRANSITION_START_INDEX;
	uint colorCount = DEFAULT_PALETTE_TRANSITION_COLOR_COUNT;

	if (args.size() >= 3) {
		percent = args[1].asFloat();
		paletteId = args[2].asAssetId();
	} else {
		error("fadeToPaletteObject: Too few script args");
		return;
	}
	if (args.size() >= 5) {
		startIndex = static_cast<uint>(args[3].asFloat());
		colorCount = static_cast<uint>(args[4].asFloat());
	}

	_setPercentToPaletteObject(percent, paletteId, startIndex, colorCount);
}

void VideoDisplayManager::colorShiftCurrentPalette(Common::Array<ScriptValue> &args) {
	if (args.size() < 4) {
		warning("colorShiftCurrentPalette: Too few script args");
		return;
	}

	uint shift = static_cast<uint>(args[1].asFloat());
	uint startIndex = static_cast<uint>(args[2].asFloat());
	uint colorCount = static_cast<uint>(args[3].asFloat());

	_colorShiftCurrentPalette(startIndex, shift, colorCount);
}

void VideoDisplayManager::circleOut(Common::Array<ScriptValue> &args) {
	warning("STUB: circleOut");
}

void VideoDisplayManager::_setPalette(Graphics::Palette &palette, uint startIndex, uint colorCount) {
	// We can't use the Palette::set method directly because it assumes the
	// data we want to copy is at the start of the data pointer, but in this
	// case we want to copy some range not necessarily right at the start. Thus,
	// we need to calculate some pointers manually.
	_limitColorRange(startIndex, colorCount);
	uint startOffset = 3 * startIndex;
	const byte *startPointer = palette.data() + startOffset;
	_screen->setPalette(startPointer, startIndex, colorCount);
}

void VideoDisplayManager::_setPaletteToColor(Graphics::Palette &targetPalette, byte r, byte g, byte b) {
	for (uint colorIndex = 0; colorIndex < Graphics::PALETTE_COUNT; colorIndex++) {
		targetPalette.set(colorIndex, r, g, b);
	}
}

uint VideoDisplayManager::_limitColorRange(uint &startIndex, uint &colorCount) {
	CLIP<uint>(startIndex, 0, Graphics::PALETTE_COUNT - 1);
	uint endColorIndex = startIndex + colorCount;
	CLIP<uint>(endColorIndex, 0, Graphics::PALETTE_COUNT);
	colorCount = endColorIndex - startIndex;
	return endColorIndex;
}

byte VideoDisplayManager::_interpolateColorComponent(byte source, byte target, double progress) {
	// The original scaled to 1024 to convert to an integer, but we will just
	// do floating-point interpolation.
	double result = source + ((target - source) * progress);
	return static_cast<byte>(CLIP<uint>(result, 0, Graphics::PALETTE_COUNT));
}

void VideoDisplayManager::_fadeToColor(byte r, byte g, byte b, double fadeTime, uint startIndex, uint colorCount) {
	// Create a temporary palette that is all one color.
	Graphics::Palette tempPalette(Graphics::PALETTE_COUNT);
	_setPaletteToColor(tempPalette, r, g, b);
	_fadeToPalette(fadeTime, tempPalette, startIndex, colorCount);
}

void VideoDisplayManager::_setToColor(byte r, byte g, byte b, uint startIndex, uint colorCount) {
	Graphics::Palette tempPalette = _screen->getPalette();
	uint endIndex = _limitColorRange(startIndex, colorCount);
	for (uint colorIndex = startIndex; colorIndex < endIndex; colorIndex++) {
		tempPalette.set(colorIndex, r, g, b);
	}

	_setPalette(tempPalette, startIndex, colorCount);
}

void VideoDisplayManager::_setPercentToColor(double percent, byte r, byte g, byte b, uint startIndex, uint colorCount) {
	// Create a temporary palette that is all one color.
	Graphics::Palette tempPalette(Graphics::PALETTE_COUNT);
	_setPaletteToColor(tempPalette, r, g, b);

	_setToPercentPalette(percent, *_registeredPalette, tempPalette, startIndex, colorCount);
}

void VideoDisplayManager::_setToPercentPalette(double percent, Graphics::Palette &currentPalette, Graphics::Palette &targetPalette, uint startIndex, uint colorCount) {
	if (percent < 0.0 || percent > 1.0) {
		warning("_setToPercentPalette: Got invalid palette percent value %f", percent);
		percent = CLIP<double>(percent, 0.0, 1.0);
	}

	uint endIndex = _limitColorRange(startIndex, colorCount);

	Graphics::Palette blendedPalette = currentPalette;
	for (uint colorIndex = startIndex; colorIndex < endIndex; colorIndex++) {
		byte redSource, greenSource, blueSource, redTarget, greenTarget, blueTarget;
		currentPalette.get(colorIndex, redSource, greenSource, blueSource);
		targetPalette.get(colorIndex, redTarget, greenTarget, blueTarget);

		byte newRed = _interpolateColorComponent(redSource, redTarget, percent);
		byte newGreen = _interpolateColorComponent(greenSource, greenTarget, percent);
		byte newBlue = _interpolateColorComponent(blueSource, blueTarget, percent);

		blendedPalette.set(colorIndex, newRed, newGreen, newBlue);
	}

	_setPalette(blendedPalette, startIndex, colorCount);
}

void VideoDisplayManager::_fadeToPalette(double fadeTime, Graphics::Palette &targetPalette, uint startIndex, uint colorCount) {
	if (fadeTime <= 0.0) {
		// Set the fade time to something reasonable so we can continue.
		warning("_fadeToPalette: Got invalid fade time %f", fadeTime);
		fadeTime = 0.1;
	}

	// Gamma correction can be set via a script function, but I haven't seen
	// that be set to anything other than 1 (the default), so it's not
	// implemented for now.
	Graphics::Palette currentPalette = _screen->getPalette();
	Graphics::Palette intermediatePalette(Graphics::PALETTE_COUNT);
	uint endIndex = _limitColorRange(startIndex, colorCount);
	uint fadeTimeMillis = static_cast<uint>(fadeTime * 1000);
	uint startTimeMillis = g_system->getMillis();
	uint endTimeMillis = startTimeMillis + fadeTimeMillis;

	while (g_system->getMillis() < endTimeMillis) {
		uint currentTimeMillis = g_system->getMillis();
		double progress = MIN<double>(static_cast<double>(currentTimeMillis - startTimeMillis) / fadeTimeMillis, 1.0);

		for (uint colorIndex = startIndex; colorIndex < endIndex; colorIndex++) {
			byte sourceR, sourceG, sourceB, targetR, targetG, targetB;

			currentPalette.get(colorIndex, sourceR, sourceG, sourceB);
			targetPalette.get(colorIndex, targetR, targetG, targetB);
			byte newR = _interpolateColorComponent(sourceR, targetR, progress);
			byte newG = _interpolateColorComponent(sourceG, targetG, progress);
			byte newB = _interpolateColorComponent(sourceB, targetB, progress);

			intermediatePalette.set(colorIndex, newR, newG, newB);
		}

		_setPalette(intermediatePalette, startIndex, colorCount);
		g_system->updateScreen();
		g_system->delayMillis(5);
	}

	// Ensure we end with exactly the target palette
	_setPalette(targetPalette, startIndex, colorCount);
}

void VideoDisplayManager::_fadeToRegisteredPalette(double fadeTime, uint startIndex, uint colorCount) {
	_fadeToPalette(fadeTime, *_registeredPalette, startIndex, colorCount);
}

void VideoDisplayManager::_setToRegisteredPalette(uint startIndex, uint colorCount) {
	_setPalette(*_registeredPalette, startIndex, colorCount);
}

void VideoDisplayManager::_colorShiftCurrentPalette(uint startIndex, uint shiftAmount, uint colorCount) {
	uint endIndex = _limitColorRange(startIndex, colorCount);
	Graphics::Palette currentPalette = _screen->getPalette();
	Graphics::Palette shiftedPalette = currentPalette;

	for (uint i = startIndex; i < endIndex; i++) {
		// Calculate target index with wraparound.
		// We convert to a zero-based index, wrap that index to stay in bounds,
		// and then convert back to the actual palette index range.
		uint targetIndex = ((i + shiftAmount - startIndex) % colorCount) + startIndex;

		byte r, g, b;
		currentPalette.get(i, r, g, b);
		shiftedPalette.set(targetIndex, r, g, b);
	}

	_setPalette(shiftedPalette, startIndex, colorCount);
}

void VideoDisplayManager::_fadeToPaletteObject(uint paletteId, double fadeTime, uint startIndex, uint colorCount) {
	Asset *asset = _vm->getAssetById(paletteId);
	if (asset == nullptr) {
		error("Got null target palette");
	} else if (asset->type() != kAssetTypePalette) {
		error("Asset %d is not a palette", paletteId);
	}

	Graphics::Palette *palette = static_cast<Palette *>(asset)->_palette;
	_fadeToPalette(fadeTime, *palette, startIndex, colorCount);
}

void VideoDisplayManager::_setToPaletteObject(uint paletteId, uint startIndex, uint colorCount) {
	Asset *asset = _vm->getAssetById(paletteId);
	if (asset == nullptr) {
		error("Got null target palette");
	} else if (asset->type() != kAssetTypePalette) {
		error("Asset %d is not a palette", paletteId);
	}

	Graphics::Palette *palette = static_cast<Palette *>(asset)->_palette;
	_setPalette(*palette, startIndex, colorCount);
}

void VideoDisplayManager::_setPercentToPaletteObject(double percent, uint paletteId, uint startIndex, uint colorCount) {
	Asset *asset = _vm->getAssetById(paletteId);
	if (asset == nullptr) {
		error("Got null target palette");
	} else if (asset->type() != kAssetTypePalette) {
		error("Asset %d is not a palette", paletteId);
	}

	Graphics::Palette *targetPalette = static_cast<Palette *>(asset)->_palette;
	_setToPercentPalette(percent, *_registeredPalette, *targetPalette, startIndex, colorCount);
}

void VideoDisplayManager::imageBlit(
	const Common::Point &destinationPoint,
	const Bitmap *sourceImage,
	const double dissolveFactor,
	const Common::Array<Common::Rect> &dirtyRegion,
	Graphics::ManagedSurface *targetImage) {

	byte blitFlags = kClipEnabled;
	switch (sourceImage->getCompressionType()) {
	case kUncompressedBitmap:
		break;

	case kRle8BitmapCompression:
		blitFlags |= kRle8Blit;
		break;

	case kCccBitmapCompression:
		blitFlags |= kCccBlit;
		break;

	case kCccTransparentBitmapCompression:
		blitFlags |= kCccTransparentBlit;
		break;

	case kUncompressedTransparentBitmap:
		blitFlags |= kUncompressedTransparentBlit;
		break;

	default:
		error("imageBlit: Got unknown bitmap compression type %d",
			static_cast<uint>(sourceImage->getCompressionType()));
	}

	if (dissolveFactor > 1.0 || dissolveFactor < 0.0) {
		warning("imageBlit: Got out-of-range dissolve factor: %f", dissolveFactor);
		CLIP(dissolveFactor, 0.0, 1.0);
	} else if (dissolveFactor == 0.0) {
		// If the image is fully transparent, there is nothing to draw, so we can return now.
		return;
	} else if (dissolveFactor != 1.0) {
		blitFlags |= kPartialDissolve;
	}
	uint integralDissolveFactor = static_cast<uint>(dissolveFactor * 100 + 0.5);

	if (targetImage == nullptr) {
		targetImage = _screen;
	}

	// In the disasm, this whole function has complex blit flag logic
	// throughout, including a lot of switch cases in the statement below
	// that were unreachable with the flags actually available, as defined above.
	// The apparently unreachable switch cases are excluded from the statement below.
	switch (blitFlags) {
	case kClipEnabled:
	case kUncompressedTransparentBlit | kClipEnabled:
		// The original had two different methods for transparent versus
		// non-transparent blitting, but we will just use simpleBlitFrom in both
		// cases. It will pick the better method if there is no transparent
		// color set.
		blitRectsClip(targetImage, destinationPoint, sourceImage->_image, dirtyRegion);
		break;

	case kRle8Blit | kClipEnabled:
		rleBlitRectsClip(targetImage, destinationPoint, sourceImage, dirtyRegion);
		break;

	case kCccBlit | kClipEnabled:
	case kCccTransparentBlit | kClipEnabled:
		// CCC blitting is unimplemented for now because few, if any, titles actually use it.
		error("imageBlit: CCC blitting not implemented yet");
		break;

	case kPartialDissolve | kClipEnabled:
	case kPartialDissolve | kUncompressedTransparentBlit | kClipEnabled:
	case kPartialDissolve | kRle8Blit | kClipEnabled:
		// The original had a separate function (rleDissolveBlitRectsClip) for decompressing the RLE and
		// applying the dissolve at the same time that we are decompressing, but I thought that was too
		// complex. Instead, we just check the compression type in the same
		// method and decompress beforehand if necessary.
		dissolveBlitRectsClip(targetImage, destinationPoint, sourceImage, dirtyRegion, integralDissolveFactor);
		break;

	default:
		error("imageBlit: Got invalid blit mode: 0x%x", blitFlags);
	}
}

void VideoDisplayManager::blitRectsClip(
	Graphics::ManagedSurface *dest,
	const Common::Point &destLocation,
	const Graphics::ManagedSurface &source,
	const Common::Array<Common::Rect> &dirtyRegion) {

	for (const Common::Rect &dirtyRect : dirtyRegion) {
		Common::Rect destRect(destLocation, source.w, source.h);
		Common::Rect areaToRedraw = dirtyRect.findIntersectingRect(destRect);

		if (!areaToRedraw.isEmpty()) {
			// Calculate source coordinates (relative to source image).
			Common::Point originOnScreen(areaToRedraw.origin());
			areaToRedraw.translate(-destLocation.x, -destLocation.y);
			dest->simpleBlitFrom(source, areaToRedraw, originOnScreen);
		}
	}
}

void VideoDisplayManager::rleBlitRectsClip(
	Graphics::ManagedSurface *dest,
	const Common::Point &destLocation,
	const Bitmap *source,
	const Common::Array<Common::Rect> &dirtyRegion) {

	Graphics::ManagedSurface surface = decompressRle8Bitmap(source);
	Common::Rect destRect(destLocation, source->width(), source->height());
	for (const Common::Rect &dirtyRect : dirtyRegion) {
		Common::Rect areaToRedraw = dirtyRect.findIntersectingRect(destRect);

		if (!areaToRedraw.isEmpty()) {
			// Calculate source coordinates (relative to source image).
			Common::Point originOnScreen(areaToRedraw.origin());
			areaToRedraw.translate(-destLocation.x, -destLocation.y);
			dest->simpleBlitFrom(surface, areaToRedraw, originOnScreen);
		}
	}
}

void VideoDisplayManager::dissolveBlitRectsClip(
	Graphics::ManagedSurface *dest,
	const Common::Point &destPos,
	const Bitmap *source,
	const Common::Array<Common::Rect> &dirtyRegion,
	const uint integralDissolveFactor) {

	byte dissolveIndex = DISSOLVE_PATTERN_COUNT;
	if (integralDissolveFactor != 50) {
		dissolveIndex = ((integralDissolveFactor + 2) / 4) - 1;
		CLIP<byte>(dissolveIndex, 0, (DISSOLVE_PATTERN_COUNT - 1));
	}

	Common::Rect destRect(Common::Rect(destPos, source->width(), source->height()));
	for (const Common::Rect &dirtyRect : dirtyRegion) {
		Common::Rect areaToRedraw = dirtyRect.findIntersectingRect(destRect);
		if (!areaToRedraw.isEmpty()) {
			// Calculate source coordinates (relative to source image).
			Common::Point originOnScreen(areaToRedraw.origin());
			areaToRedraw.translate(-destPos.x, -destPos.y);
			dissolveBlit1Rect(dest, areaToRedraw, originOnScreen, source, dirtyRect, DISSOLVE_PATTERNS[dissolveIndex]);
		}
	}
}

void VideoDisplayManager::dissolveBlit1Rect(
	Graphics::ManagedSurface *dest,
	const Common::Rect &areaToRedraw,
	const Common::Point &originOnScreen,
	const Bitmap *source,
	const Common::Rect &dirtyRegion,
	const DissolvePattern &pattern) {

	Graphics::ManagedSurface sourceSurface;
	const Graphics::ManagedSurface *srcPtr = nullptr;
	switch (source->getCompressionType()) {
	case kRle8BitmapCompression:
		sourceSurface = decompressRle8Bitmap(source);
		srcPtr = &sourceSurface;
		break;

	case kUncompressedBitmap:
	case kUncompressedTransparentBitmap:
		srcPtr = &source->_image;
		break;

	default:
		error("dissolveBlit1Rect: Unsupported compression type for dissolve blit: %d",
			static_cast<uint>(source->getCompressionType()));
	}

	Common::Point patternStartPos(originOnScreen.x % pattern.widthHeight, originOnScreen.y % pattern.widthHeight);
	Common::Point currentPatternPos;
	for (int y = 0; y < areaToRedraw.height(); y++) {
		currentPatternPos.y = (patternStartPos.y + y) % pattern.widthHeight;

		for (int x = 0; x < areaToRedraw.width(); x++) {
			currentPatternPos.x = (patternStartPos.x + x) % pattern.widthHeight;
			uint patternIndex = currentPatternPos.y * pattern.widthHeight + currentPatternPos.x;

			bool shouldDrawPixel = pattern.data[patternIndex] == pattern.threshold;
			if (shouldDrawPixel) {
				// Even if the pattern indicates we should draw here, only
				// copy non-transparent source pixels (value != 0 for transparent bitmaps).
				Common::Point sourcePos(areaToRedraw.left + x, areaToRedraw.top + y);

				bool sourceXInBounds = sourcePos.x >= 0 && sourcePos.x < srcPtr->w;
				bool sourceYInBounds = sourcePos.y >= 0 && sourcePos.y < srcPtr->h;
				if (sourceXInBounds && sourceYInBounds) {
					byte sourcePixel = srcPtr->getPixel(sourcePos.x, sourcePos.y);
					if (sourcePixel != 0) {
						Common::Point destPos(originOnScreen.x + x, originOnScreen.y + y);
						bool destXInBounds = destPos.x >= 0 && destPos.x < dest->w;
						bool destYInBounds = destPos.y >= 0 && destPos.y < dest->h;
						if (destXInBounds && destYInBounds) {
							dest->setPixel(destPos.x, destPos.y, sourcePixel);
						} else {
							warning("dissolveBlit1Rect: Dest out of bounds");
						}
					}
				} else {
					warning("dissolveBlit1Rect: Source out of bounds");
				}
			}
		}
	}
}

void VideoDisplayManager::imageDeltaBlit(
	const Common::Point &deltaFramePos,
	const Common::Point &keyFrameOffset,
	const Bitmap *deltaFrame,
	const Bitmap *keyFrame,
	const double dissolveFactor,
	const Common::Array<Common::Rect> &dirtyRegion) {

	if (deltaFrame->getCompressionType() != kRle8BitmapCompression) {
		error("imageDeltaBlit: Unsupported delta frame compression type for delta blit: %d",
			static_cast<uint>(keyFrame->getCompressionType()));
	} else if (dissolveFactor != 1.0) {
		warning("imageDeltaBlit: Delta blit does not support dissolving");
	}

	switch (keyFrame->getCompressionType()) {
	case kUncompressedBitmap:
	case kUncompressedTransparentBitmap:
		deltaRleBlitRectsClip(_screen, deltaFramePos, deltaFrame, keyFrame, dirtyRegion);
		break;

	case kRle8BitmapCompression:
		fullDeltaRleBlitRectsClip(_screen, deltaFramePos, keyFrameOffset, deltaFrame, keyFrame, dirtyRegion);
		break;

	default:
		error("imageDeltaBlit: Unsupported keyframe image type for delta blit: %d",
			static_cast<uint>(deltaFrame->getCompressionType()));
	}
}

void VideoDisplayManager::fullDeltaRleBlitRectsClip(
	Graphics::ManagedSurface *destinationImage,
	const Common::Point &deltaFramePos,
	const Common::Point &keyFrameOffset,
	const Bitmap *deltaFrame,
	const Bitmap *keyFrame,
	const Common::Array<Common::Rect> &dirtyRegion) {

	Graphics::ManagedSurface surface = decompressRle8Bitmap(deltaFrame, &keyFrame->_image, &keyFrameOffset);
	for (const Common::Rect &dirtyRect : dirtyRegion) {
		// The original has a fullDeltaRleBlit1Rect function, but given that we do
		// the delta application when we decompress the keyframe above, we really
		// don't need a separate function for this.
		Common::Rect destRect(deltaFramePos, deltaFrame->width(), deltaFrame->height());
		Common::Rect areaToRedraw = dirtyRect.findIntersectingRect(destRect);

		if (!areaToRedraw.isEmpty()) {
			// Calculate source coordinates (relative to source image).
			Common::Point originOnScreen(areaToRedraw.origin());
			areaToRedraw.translate(-deltaFramePos.x, -deltaFramePos.y);
			destinationImage->simpleBlitFrom(surface, areaToRedraw, originOnScreen);
		}
	}
}

void VideoDisplayManager::deltaRleBlitRectsClip(
	Graphics::ManagedSurface *destinationImage,
	const Common::Point &deltaFramePos,
	const Bitmap *deltaFrame,
	const Bitmap *keyFrame,
	const Common::Array<Common::Rect> &dirtyRegion) {

	Common::Rect deltaFrameBounds = Common::Rect(deltaFramePos, deltaFrame->width(), deltaFrame->height());
	for (const Common::Rect &dirtyRect : dirtyRegion) {
		if (deltaFrameBounds.intersects(dirtyRect)) {
			deltaRleBlit1Rect(destinationImage, deltaFramePos, deltaFrame, keyFrame, dirtyRect);
		}
	}
}

void VideoDisplayManager::deltaRleBlit1Rect(
	Graphics::ManagedSurface *destinationImage,
	const Common::Point &destinationPoint,
	const Bitmap *deltaFrame,
	const Bitmap *keyFrame,
	const Common::Rect &dirtyRect) {

	// This is a very complex function that attempts to decompress the keyframe
	// and delta frame at the same time, assuming the keyframe is also
	// compressed. However, real titles don't seem to use it, instead
	// decompressing the keyframe separately and then passng it in.
	// So this is left unimplemented until it's actually needed.
	warning("STUB: deltaRleBlit1Rect");
}

Graphics::ManagedSurface VideoDisplayManager::decompressRle8Bitmap(
	const Bitmap *source,
	const Graphics::ManagedSurface *keyFrame,
	const Common::Point *keyFrameOffset) {

	// Create a surface to hold the decompressed bitmap.
	Graphics::ManagedSurface dest;
	dest.create(source->width(), source->height(), Graphics::PixelFormat::createFormatCLUT8());
	dest.setTransparentColor(0);
	int destSizeInBytes = source->width() * source->height();

	Common::SeekableReadStream *chunk = source->_compressedStream;
	chunk->seek(0);

	bool imageFullyRead = false;
	Common::Point sourcePos;
	while (sourcePos.y < source->height()) {
		sourcePos.x = 0;
		while (true) {
			if (sourcePos.y >= source->height()) {
				break;
			}

			byte operation = chunk->readByte();
			if (operation == 0x00) {
				operation = chunk->readByte();
				if (operation == 0x00) {
					// Mark the end of the line.
					// Also check if the image is finished being read.
					if (chunk->eos()) {
						imageFullyRead = true;
					}
					break;

				} else if (operation == 0x01) {
					// Mark the end of the image.
					imageFullyRead = true;
					break;

				} else if (operation == 0x02) {
					// Copy from the keyframe region.
					assert((keyFrame != nullptr) && (keyFrameOffset != nullptr));
					byte xToCopy = chunk->readByte();
					byte yToCopy = chunk->readByte();

					// If we requested to copy multiple lines, do that first.
					for (int lineOffset = 0; lineOffset < yToCopy; lineOffset++) {
						Common::Point keyFramePos = sourcePos - *keyFrameOffset + Common::Point(0, lineOffset);
						Common::Point destPos = sourcePos + Common::Point(0, lineOffset);

						bool sourceXInBounds = (keyFramePos.x >= 0) && (keyFramePos.x + xToCopy <= keyFrame->w);
						bool sourceYInBounds = (keyFramePos.y >= 0) && (keyFramePos.y < keyFrame->h);
						bool destInBounds = (destPos.y * dest.w) + (destPos.x + xToCopy) <= destSizeInBytes;
						if (sourceXInBounds && sourceYInBounds && destInBounds) {
							const byte *srcPtr = static_cast<const byte *>(keyFrame->getBasePtr(keyFramePos.x, keyFramePos.y));
							byte *destPtr = static_cast<byte *>(dest.getBasePtr(destPos.x, destPos.y));
							memcpy(destPtr, srcPtr, xToCopy);
						} else {
							warning("decompressRle8Bitmap: Keyframe copy (multi-line) exceeds bounds");
						}
					}

					// Then copy the pixels in the same line.
					Common::Point keyFramePos = sourcePos - *keyFrameOffset;
					bool sourceXInBounds = (keyFramePos.x >= 0) && (keyFramePos.x + xToCopy <= keyFrame->w);
					bool sourceYInBounds = (keyFramePos.y >= 0) && (keyFramePos.y < keyFrame->h);
					bool destInBounds = (sourcePos.y * dest.w) + (sourcePos.x + xToCopy) <= destSizeInBytes;
					if (sourceXInBounds && sourceYInBounds && destInBounds) {
						const byte *srcPtr = static_cast<const byte *>(keyFrame->getBasePtr(keyFramePos.x, keyFramePos.y));
						byte *destPtr = static_cast<byte *>(dest.getBasePtr(sourcePos.x, sourcePos.y));
						memcpy(destPtr, srcPtr, xToCopy);
					} else {
						warning("decompressRle8Bitmap: Keyframe copy (same line) exceeds bounds");
					}

					sourcePos += Common::Point(xToCopy, yToCopy);

				} else if (operation == 0x03) {
					// Adjust the pixel position.
					sourcePos.x += chunk->readByte();
					sourcePos.y += chunk->readByte();

				} else if (operation >= 0x04) {
					// Read a run of uncompressed pixels.
					// The bounds check is structured this way because the run can extend across scanlines.
					byte runLength = operation;
					uint maxAllowedRun = destSizeInBytes - (sourcePos.y * dest.w + sourcePos.x);
					CLIP<uint>(runLength, 0, maxAllowedRun);

					byte *destPtr = static_cast<byte *>(dest.getBasePtr(sourcePos.x, sourcePos.y));
					chunk->read(destPtr, runLength);
					if (chunk->pos() % 2 == 1) {
						chunk->readByte();
					}

					sourcePos.x += runLength;
				}
			} else {
				// Read a run of length encoded pixels.
				byte colorIndexToRepeat = chunk->readByte();
				byte repetitionCount = operation;
				uint maxAllowedCount = destSizeInBytes - (sourcePos.y * dest.w + sourcePos.x);
				CLIP<uint>(repetitionCount, 0, maxAllowedCount);

				byte *destPtr = static_cast<byte *>(dest.getBasePtr(sourcePos.x, sourcePos.y));
				memset(destPtr, colorIndexToRepeat, repetitionCount);
				sourcePos.x += repetitionCount;
			}
		}

		sourcePos.y++;
		if (imageFullyRead) {
			break;
		}
	}

	return dest;
}

} // End of namespace MediaStation
