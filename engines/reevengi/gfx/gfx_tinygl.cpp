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

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "math/glmath.h"

#include "graphics/surface.h"

#include "engines/reevengi/gfx/gfx_base.h"
#include "engines/reevengi/gfx/gfx_tinygl.h"

namespace Reevengi {

#define BITMAP_TEXTURE_SIZE 256

GfxBase *CreateGfxTinyGL() {
	return new GfxTinyGL();
}

GfxTinyGL::GfxTinyGL(): _smushImage(nullptr),
	_maskNumTex(0) {
	//
}

GfxTinyGL::~GfxTinyGL() {
	releaseMaskedFrame();
	releaseMovieFrame();
}

void GfxTinyGL::setupScreen(int screenW, int screenH) {
	_screenWidth = screenW;
	_screenHeight = screenH;
	_scaleW = _screenWidth / (float)_gameWidth;
	_scaleH = _screenHeight / (float)_gameHeight;

	g_system->showMouse(false);

	_pixelFormat = g_system->getScreenFormat();
	debug("INFO: TinyGL front buffer pixel format: %s", _pixelFormat.toString().c_str());
	TinyGL::createContext(screenW, screenH, _pixelFormat, 256, true, ConfMan.getBool("dirtyrects"));

	_storedDisplay = new Graphics::Surface;
	_storedDisplay->create(_gameWidth, _gameHeight, _pixelFormat);

	/* _currentShadowArray = nullptr; */

	TGLfloat ambientSource[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	tglLightModelfv(TGL_LIGHT_MODEL_AMBIENT, ambientSource);
	TGLfloat diffuseReflectance[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	tglMaterialfv(TGL_FRONT, TGL_DIFFUSE, diffuseReflectance);
}

const char *GfxTinyGL::getVideoDeviceName() {
	return "TinyGL Software Renderer";
}

void GfxTinyGL::clearScreen() {
	tglClear(TGL_COLOR_BUFFER_BIT | TGL_DEPTH_BUFFER_BIT);
}

void GfxTinyGL::clearDepthBuffer() {
	tglClear(TGL_DEPTH_BUFFER_BIT);
}

void GfxTinyGL::flipBuffer() {
	Common::List<Common::Rect> dirtyAreas;
	TinyGL::presentBuffer(dirtyAreas);

	Graphics::Surface glBuffer;
	TinyGL::getSurfaceRef(glBuffer);

	if (!dirtyAreas.empty()) {
		for (Common::List<Common::Rect>::iterator itRect = dirtyAreas.begin(); itRect != dirtyAreas.end(); ++itRect) {
			g_system->copyRectToScreen(glBuffer.getBasePtr((*itRect).left, (*itRect).top), glBuffer.pitch,
			                           (*itRect).left, (*itRect).top, (*itRect).width(), (*itRect).height());
		}
	}

	g_system->updateScreen();
}

bool GfxTinyGL::isHardwareAccelerated() {
	return false;
}

bool GfxTinyGL::supportsShaders() {
	return false;
}

void GfxTinyGL::prepareMovieFrame(Graphics::Surface *frame) {
	if (!_smushImage)
		_smushImage = tglGenBlitImage();
	tglUploadBlitImage(_smushImage, *frame, 0, false);

	_smushWidth = frame->w;
	_smushHeight = frame->h;
}

void GfxTinyGL::drawMovieFrame(int offsetX, int offsetY) {
	int sysW = g_system->getWidth();
	int sysH = g_system->getHeight();

	float movScale = MIN<float>((float) sysW / _smushWidth, (float) sysH / _smushHeight);
	int movW = _smushWidth * movScale;
	int movH = _smushHeight * movScale;

	offsetX = (int)(offsetX * movScale);
	offsetY = (int)(offsetY * movScale);

	offsetX += (sysW-movW)>>1;
	offsetY += (sysH-movH)>>1;

	if ((movW==_smushWidth) && (movH==_smushHeight)) {
		tglBlitFast(_smushImage, offsetX, offsetY);
		return;
	}

	TinyGL::BlitTransform bltTransform(offsetX, offsetY);
	//FIXME bltTransform.scale no more available
	// bltTransform.scale(movW, movH);

	tglBlit(_smushImage, bltTransform);
}

void GfxTinyGL::releaseMovieFrame() {
	tglDeleteBlitImage(_smushImage);
}

void GfxTinyGL::prepareMaskedFrame(Graphics::Surface *frame, uint16* timPalette) {
	int height = frame->h;
	int width = frame->w;
	uint16 *maskBitmap;

	TGLenum format;
	TGLenum dataType;
	int bytesPerPixel = frame->format.bytesPerPixel;

	// Aspyr Logo format
	if (frame->format == Graphics::PixelFormat(4, 8, 8, 8, 0, 8, 16, 24, 0)) {
#if !defined(__amigaos4__)
		format = TGL_BGRA;
		dataType = TGL_UNSIGNED_INT_8_8_8_8;
#else
		// AmigaOS' MiniGL does not understand GL_UNSIGNED_INT_8_8_8_8 yet.
		format = TGL_BGRA;
		dataType = TGL_UNSIGNED_BYTE;
#endif
	} else if (frame->format == Graphics::PixelFormat(4, 8, 8, 8, 0, 16, 8, 0, 0)) {
		format = TGL_BGRA;
		dataType = TGL_UNSIGNED_INT_8_8_8_8_REV;
	} else if (frame->format == Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0)) {
		format = TGL_RGB;
		dataType = TGL_UNSIGNED_SHORT_5_6_5;
	/*} else if (frame->format == Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0)) {
		format = TGL_RGBA;
		dataType = TGL_UNSIGNED_SHORT_5_5_5_1;*/
	} else if (frame->format == Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24)) {
		format = TGL_RGBA;
		dataType = TGL_UNSIGNED_INT_8_8_8_8_REV;
	} else if (frame->format == Graphics::PixelFormat(3, 8, 8, 8, 0, 0, 8, 16, 0)) {
		format = TGL_RGB;
		dataType = TGL_UNSIGNED_BYTE;
 	} else if (frame->format == Graphics::PixelFormat(1, 0, 0, 0, 0, 0, 0, 0, 0)) {
		format = TGL_COLOR_INDEX;
		dataType = TGL_UNSIGNED_BYTE;
	} else {
		error("Unknown pixelformat: Bpp: %d RBits: %d GBits: %d BBits: %d ABits: %d RShift: %d GShift: %d BShift: %d AShift: %d",
			frame->format.bytesPerPixel,
			-(frame->format.rLoss - 8),
			-(frame->format.gLoss - 8),
			-(frame->format.bLoss - 8),
			-(frame->format.aLoss - 8),
			frame->format.rShift,
			frame->format.gShift,
			frame->format.bShift,
			frame->format.aShift);
	}

	releaseMaskedFrame();

	// create texture
	_maskTexPitch = ((width + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_maskNumTex = _maskTexPitch *
				   ((height + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_maskTexIds = new TGLuint[_maskNumTex];
	tglGenTextures(_maskNumTex, _maskTexIds);

	int i = 0;
	for (int y = 0; y < height; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < width; x += BITMAP_TEXTURE_SIZE) {
			int t_width = (x + BITMAP_TEXTURE_SIZE >= width) ? (width - x) : BITMAP_TEXTURE_SIZE;
			int t_height = (y + BITMAP_TEXTURE_SIZE >= height) ? (height - y) : BITMAP_TEXTURE_SIZE;
			TGLenum txFormat = TGL_RGBA;
			TGLenum txDataType = TGL_UNSIGNED_SHORT_5_5_5_1;
			Graphics::PixelFormat dstFormat(2, 5, 5, 5, 1, 11, 6, 1, 0);

			tglBindTexture(TGL_TEXTURE_2D, _maskTexIds[i]);
			tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MAG_FILTER, TGL_NEAREST);
			tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MIN_FILTER, TGL_NEAREST);
			tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_S, TGL_CLAMP);
			tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_T, TGL_CLAMP);

			maskBitmap = new uint16[BITMAP_TEXTURE_SIZE * BITMAP_TEXTURE_SIZE];

			if (format == TGL_COLOR_INDEX) {
				// Does not work with paletted texture, convert to some argb texture
				Graphics::PixelFormat fmtTimPal(2, 5, 5, 5, 1, 11, 6, 1, 0);

				/* Convert part of texture, we only need Alpha channel */
				uint16 *dstBitmap = maskBitmap;
				uint8 *srcBitmap = (uint8 *) frame->getBasePtr(0, 0);
				for (int sy=0; sy<t_height; sy++) {
					uint8 *srcLine = srcBitmap;
					uint16 *dstLine = dstBitmap;
					for (int sx=0; sx<t_width; sx++) {
						byte r, g, b, a;
						uint16 color = timPalette[ *srcLine++ ];
						fmtTimPal.colorToARGB(color, a, r, g, b);
						*dstLine++ = dstFormat.ARGBToColor(a, r, g, b);
					}
					srcBitmap += frame->pitch;
					dstBitmap += BITMAP_TEXTURE_SIZE;
				}
			} else {
				// FIXME: Copy part of texture from frame to maskBitmap
			}

			tglTexImage2D(TGL_TEXTURE_2D, 0, TGL_RGBA, BITMAP_TEXTURE_SIZE, BITMAP_TEXTURE_SIZE, 0, txFormat, txDataType, maskBitmap);
			delete[] maskBitmap;

			i++;
		}
	}

	_maskWidth = width; //(int)(width * _scaleW);
	_maskHeight = height; //(int)(height * _scaleH);
}

void GfxTinyGL::drawMaskedFrame(int srcX, int srcY, int dstX, int dstY, int w, int h, int depth) {
	int sysW = _screenWidth;
	int sysH = _screenHeight;

	float movScale = MIN<float>((float) sysW / 320, (float) sysH / 240);
	int movW = 320 * movScale;
	int movH = 240 * movScale;

	float bitmapDepth= 1.0f - (kRenderZNear / (float) depth);
	bitmapDepth *= kRenderZFar / (kRenderZFar - kRenderZNear);

 	tglViewport(_screenViewport.left, _screenViewport.top, _screenWidth, _screenHeight);

	// prepare view
	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrtho(0, _screenWidth, _screenHeight, 0, 0, 1);

	tglMatrixMode(TGL_TEXTURE);
	tglLoadIdentity();
	tglScalef(1.0f / BITMAP_TEXTURE_SIZE, 1.0f / BITMAP_TEXTURE_SIZE, 1.0f);

	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();
	tglScalef(movScale, movScale, 1.0f);
	tglTranslatef(0.0f, 0.0f, -bitmapDepth);

	// A lot more may need to be put there : disabling Alpha test, blending, ...
	// For now, just keep this here :-)

	tglDisable(TGL_LIGHTING);
	tglEnable(TGL_TEXTURE_2D);
	// draw
	tglEnable(TGL_DEPTH_TEST);
	tglDepthMask(TGL_TRUE);
	//glEnable(TGL_SCISSOR_TEST);
	tglColorMask(TGL_FALSE, TGL_FALSE, TGL_FALSE, TGL_FALSE);
	setBlending(true);

	dstX += (sysW-movW)>>1;
	dstY += (sysH-movH)>>1;

	//glScissor(offsetX, _screenHeight - (offsetY + movH), movW, movH);

	int y=0;
	while (y<h) {
		int rowTexNum = (srcY+y) / BITMAP_TEXTURE_SIZE;
		int ty = (srcY+y) % BITMAP_TEXTURE_SIZE;
		int th = h-y;
		if (ty+th>BITMAP_TEXTURE_SIZE) {
			th = BITMAP_TEXTURE_SIZE-ty;	// Need to render extra part with another texture
		}

		int x=0;
		while (x<w) {
			int colTexNum = (srcX+x) / BITMAP_TEXTURE_SIZE;
			int tx = (srcX+x) % BITMAP_TEXTURE_SIZE;
			int tw = w-x;
			if (tx+tw>BITMAP_TEXTURE_SIZE) {
				tw = BITMAP_TEXTURE_SIZE-tx;	// Need to render extra part with another texture
			}

			tglBindTexture(TGL_TEXTURE_2D, _maskTexIds[rowTexNum*_maskTexPitch + colTexNum]);

			tglBegin(TGL_QUADS);
			tglTexCoord2f(tx+0.5, ty+0.5);
			tglVertex2f(dstX+x, dstY+y);
			tglTexCoord2f(tx-0.5 + tw, ty+0.5);
			tglVertex2f(dstX+x + tw, dstY+y);
			tglTexCoord2f(tx-0.5 + tw, ty-0.5 + th);
			tglVertex2f(dstX+x + tw, dstY+y + th);
			tglTexCoord2f(tx+0.5, ty-0.5 + th);
			tglVertex2f(dstX+x, dstY+y + th);
			tglEnd();

			x += tw;
		}

		y += th;
	}

	setBlending(false);
	//glDisable(TGL_SCISSOR_TEST);
	tglDisable(TGL_TEXTURE_2D);
	tglEnable(TGL_LIGHTING);
	tglColorMask(TGL_TRUE, TGL_TRUE, TGL_TRUE, TGL_TRUE);
}

void GfxTinyGL::releaseMaskedFrame(void) {
	if (_maskNumTex > 0) {
		tglDeleteTextures(_maskNumTex, _maskTexIds);
		delete[] _maskTexIds;
		_maskNumTex = 0;
	}
}

void GfxTinyGL::setProjection(float angle, float aspect, float zNear, float zFar) {
	Math::Matrix4 mProjection = Math::makePerspectiveMatrix(angle, aspect, zNear, zFar);

	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglMultMatrixf(mProjection.getData());

	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();
}

void GfxTinyGL::setModelview(float fromX, float fromY, float fromZ,
	float toX, float toY, float toZ, float upX, float upY, float upZ) {
	Math::Matrix4 mLookAt = Math::makeLookAtMatrix(Math::Vector3d(fromX, fromY, fromZ),
		Math::Vector3d(toX, toY, toZ), Math::Vector3d(upX, upY, upZ));

	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();
	tglMultMatrixf(mLookAt.getData());

	tglTranslatef(-fromX, -fromY, -fromZ);
}

void GfxTinyGL::MatrixModeModelview(void) {
	tglMatrixMode(TGL_MODELVIEW);
}

void GfxTinyGL::MatrixModeTexture(void) {
	tglMatrixMode(TGL_TEXTURE);
}

void GfxTinyGL::loadIdentity(void) {
	tglLoadIdentity();
}

void GfxTinyGL::pushMatrix(void) {
	tglPushMatrix();
}

void GfxTinyGL::popMatrix(void) {
	tglPopMatrix();
}

void GfxTinyGL::rotate(float angle, float ax, float ay, float az) {
	tglRotatef(angle, ax, ay, az);
}

void GfxTinyGL::translate(float tx, float ty, float tz) {
	tglTranslatef(tx, ty, tz);
}

void GfxTinyGL::beginTriangles(void) {
	tglBegin(TGL_TRIANGLES);
}

void GfxTinyGL::beginQuads(void) {
	tglBegin(TGL_QUADS);
}

void GfxTinyGL::normal3f(float x, float y, float z) {
	tglNormal3f(x, y, z);
}

void GfxTinyGL::texCoord2f(float s, float r) {
	tglTexCoord2f(s, r);
}

void GfxTinyGL::vertex3f(float x, float y, float z) {
	tglVertex3f(x, y, z);
}

void GfxTinyGL::endPrim(void) {
	tglEnd();
}

uint GfxTinyGL::genTexture(void) {
	TGLuint _newTexId[1];

	tglGenTextures(1, _newTexId);

	return _newTexId[0];
}

void GfxTinyGL::bindTexture(uint texId) {
	tglBindTexture(TGL_TEXTURE_2D, texId);
}

void GfxTinyGL::createTexture(const Graphics::Surface *frame, uint16* timPalette) {
	TGLenum format = TGL_RGBA;
	TGLenum dataType = TGL_UNSIGNED_BYTE;
	uint16 *maskBitmap = nullptr;

	if (!frame)
		return;

	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MAG_FILTER, TGL_NEAREST);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MIN_FILTER, TGL_NEAREST);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_S, TGL_CLAMP);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_T, TGL_CLAMP);

	if (frame->format == Graphics::PixelFormat(1, 0, 0, 0, 0, 0, 0, 0, 0)) {
		/* Convert to R5G5B5A1 texture */
		format = TGL_RGBA;
		dataType = TGL_UNSIGNED_SHORT_5_5_5_1;
		Graphics::PixelFormat fmtTimPal(2, 5, 5, 5, 1, 11, 6, 1, 0);
		Graphics::PixelFormat dstFormat(2, 5, 5, 5, 1, 11, 6, 1, 0);

		maskBitmap = new uint16[frame->w * frame->h];

		/* Convert part of texture, we only need Alpha channel */
		uint16 *dstBitmap = maskBitmap;
		uint8 *srcBitmap = (uint8 *) frame->getPixels();
		for (int sy=0; sy<frame->h; sy++) {
			uint8 *srcLine = srcBitmap;
			uint16 *dstLine = dstBitmap;
			for (int sx=0; sx<frame->w; sx++) {
				byte r, g, b, a;
				uint16 color = timPalette[ *srcLine++ ];
				fmtTimPal.colorToARGB(color, a, r, g, b);
				*dstLine++ = dstFormat.ARGBToColor(a, r, g, b);
			}
			srcBitmap += frame->pitch;
			dstBitmap += frame->w;
		}

	} else {
		error("Unknown pixelformat: Bpp: %d RBits: %d GBits: %d BBits: %d ABits: %d RShift: %d GShift: %d BShift: %d AShift: %d",
			frame->format.bytesPerPixel,
			-(frame->format.rLoss - 8),
			-(frame->format.gLoss - 8),
			-(frame->format.bLoss - 8),
			-(frame->format.aLoss - 8),
			frame->format.rShift,
			frame->format.gShift,
			frame->format.bShift,
			frame->format.aShift);
	}

	tglTexImage2D(TGL_TEXTURE_2D, 0, TGL_RGBA, frame->w, frame->h, 0, format, dataType, maskBitmap);
	delete[] maskBitmap;
}

void GfxTinyGL::setBlending(bool enable) {
	if (enable) {
		tglEnable(TGL_BLEND);
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);

		tglEnable(TGL_ALPHA_TEST);
		tglAlphaFunc(TGL_GREATER, 0.5f);
	} else {
		tglDisable(TGL_BLEND);
		tglDisable(TGL_ALPHA_TEST);
	}
}

void GfxTinyGL::setColor(float r, float g, float b) {
	tglColor3f(r, g, b);
}

void GfxTinyGL::setColorMask(bool enable) {
	tglColorMask(enable ? TGL_TRUE : TGL_FALSE, enable ? TGL_TRUE : TGL_FALSE,
		enable ? TGL_TRUE : TGL_FALSE, enable ? TGL_TRUE : TGL_FALSE);
}

void GfxTinyGL::setDepth(bool enable) {
	if (enable) {
		tglEnable(TGL_DEPTH_TEST);
	} else {
		tglDisable(TGL_DEPTH_TEST);
	}
}

void GfxTinyGL::setTexture2d(bool enable) {
	if (enable) {
		tglEnable(TGL_TEXTURE_2D);
	} else {
		tglDisable(TGL_TEXTURE_2D);
	}
}

void GfxTinyGL::line(Math::Vector3d v0, Math::Vector3d v1) {
	tglDisable(TGL_LIGHTING);
	tglDisable(TGL_TEXTURE_2D);
	//tglDisable(TGL_DEPTH_TEST);

	tglBegin(TGL_LINES);
		tglVertex3f(v0.x(), v0.y(), v0.z());
		tglVertex3f(v1.x(), v1.y(), v1.z());
	tglEnd();
}

} // End of namespace Reevengi
