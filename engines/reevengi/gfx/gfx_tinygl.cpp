/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
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
#include "common/scummsys.h"
#include "common/system.h"
#include "math/glmath.h"

#include "engines/reevengi/gfx/gfx_base.h"
#include "engines/reevengi/gfx/gfx_tinygl.h"

namespace Reevengi {

#define BITMAP_TEXTURE_SIZE 256

GfxBase *CreateGfxTinyGL() {
	return new GfxTinyGL();
}

GfxTinyGL::GfxTinyGL() {
}

GfxTinyGL::~GfxTinyGL() {
}

byte *GfxTinyGL::setupScreen(int screenW, int screenH, bool fullscreen) {
	//Graphics::PixelBuffer buf = g_system->getScreenPixelBuffer();
	byte *buffer = nullptr; //buf.getRawBuffer();

	_screenWidth = screenW;
	_screenHeight = screenH;
	_scaleW = _screenWidth / (float)_gameWidth;
	_scaleH = _screenHeight / (float)_gameHeight;

	debug(3, "%dx%d -> %dx%d", _gameWidth, _gameHeight, _screenWidth, _screenHeight);

	g_system->showMouse(!fullscreen);

	_pixelFormat = g_system->getScreenFormat();
	debug("INFO: TinyGL front buffer pixel format: %s", _pixelFormat.toString().c_str());
	_zb = new TinyGL::FrameBuffer(screenW, screenH, _pixelFormat);
	TinyGL::glInit(_zb, 256);
	tglEnableDirtyRects(ConfMan.getBool("dirtyrects"));

	_storedDisplay.create(_pixelFormat, _gameWidth * _gameHeight, DisposeAfterUse::YES);
	_storedDisplay.clear(_gameWidth * _gameHeight);

	//_currentShadowArray = nullptr;

	TGLfloat ambientSource[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	tglLightModelfv(TGL_LIGHT_MODEL_AMBIENT, ambientSource);
	TGLfloat diffuseReflectance[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	tglMaterialfv(TGL_FRONT, TGL_DIFFUSE, diffuseReflectance);

	return buffer;
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
	TinyGL::tglPresentBuffer();
	g_system->copyRectToScreen(_zb->getPixelBuffer(), _zb->linesize,
	                           0, 0, _zb->xsize, _zb->ysize);
	g_system->updateScreen();
}

bool GfxTinyGL::isHardwareAccelerated() {
	return false;
}

bool GfxTinyGL::supportsShaders() {
	return false;
}

void GfxTinyGL::prepareMovieFrame(Graphics::Surface *frame) {
	if (_smushImage == nullptr)
		_smushImage = Graphics::tglGenBlitImage();
	Graphics::tglUploadBlitImage(_smushImage, *frame, 0, false);

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
		Graphics::tglBlitFast(_smushImage, offsetX, offsetY);
		return;
	}

	Graphics::BlitTransform bltTransform(offsetX, offsetY);
	bltTransform.scale(movW, movH);

	Graphics::tglBlit(_smushImage, bltTransform);
}

void GfxTinyGL::releaseMovieFrame() {
	Graphics::tglDeleteBlitImage(_smushImage);
}

void GfxTinyGL::prepareMaskedFrame(Graphics::Surface *frame, uint16* timPalette) {
	int height = frame->h;
	int width = frame->w;
	byte *bitmap = (byte *)frame->getPixels();

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

	// remove if already exist
	if (_maskNumTex > 0) {
		tglDeleteTextures(_maskNumTex, _maskTexIds);
		delete[] _maskTexIds;
		_maskNumTex = 0;
	}

	// create texture
	_maskTexPitch = ((width + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_maskNumTex = _maskTexPitch *
				   ((height + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_maskTexIds = new TGLuint[_maskNumTex];
	tglGenTextures(_maskNumTex, _maskTexIds);
	for (int i = 0; i < _maskNumTex; i++) {
		tglBindTexture(TGL_TEXTURE_2D, _maskTexIds[i]);
		tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MAG_FILTER, TGL_NEAREST);
		tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MIN_FILTER, TGL_NEAREST);
		tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_S, TGL_CLAMP);
		tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_T, TGL_CLAMP);
		tglTexImage2D(TGL_TEXTURE_2D, 0, TGL_RGBA, BITMAP_TEXTURE_SIZE, BITMAP_TEXTURE_SIZE, 0, format, dataType, nullptr);
	}

	tglPixelStorei(TGL_UNPACK_ALIGNMENT, bytesPerPixel); // 16 bit RGB 565 bitmap/32 bit BGR
	tglPixelStorei(TGL_UNPACK_ROW_LENGTH, width);

#if 1
	// FIXME: Handle paletted texture
#else
	// Upload palette
	if ((bytesPerPixel==1) && timPalette) {
		TGLfloat mapR[256], mapG[256], mapB[256], mapA[256];
		Graphics::PixelFormat fmtTimPal(2, 5, 5, 5, 1, 11, 6, 1, 0);

		memset(mapR, 0, sizeof(mapR));
		memset(mapG, 0, sizeof(mapG));
		memset(mapB, 0, sizeof(mapB));
		memset(mapA, 0, sizeof(mapA));
		for (int i=0; i<256; i++) {
			byte r, g, b, a;
			uint16 color = *timPalette++;
			fmtTimPal.colorToARGB(color, a, r, g, b);

			mapR[i] = r / 255.0f;
			mapG[i] = g / 255.0f;
			mapB[i] = b / 255.0f;
			mapA[i] = a / 255.0f;
		}
		tglPixelTransferi(TGL_MAP_COLOR, TGL_TRUE);
		tglPixelMapfv(TGL_PIXEL_MAP_I_TO_R, 256, mapR);
		tglPixelMapfv(TGL_PIXEL_MAP_I_TO_G, 256, mapG);
		tglPixelMapfv(TGL_PIXEL_MAP_I_TO_B, 256, mapB);
		tglPixelMapfv(TGL_PIXEL_MAP_I_TO_A, 256, mapA);
	}

	int curTexIdx = 0;
	for (int y = 0; y < height; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < width; x += BITMAP_TEXTURE_SIZE) {
			int t_width = (x + BITMAP_TEXTURE_SIZE >= width) ? (width - x) : BITMAP_TEXTURE_SIZE;
			int t_height = (y + BITMAP_TEXTURE_SIZE >= height) ? (height - y) : BITMAP_TEXTURE_SIZE;
			tglBindTexture(TGL_TEXTURE_2D, _maskTexIds[curTexIdx]);
			tglTexSubImage2D(TGL_TEXTURE_2D, 0, 0, 0, t_width, t_height, format, dataType, bitmap + (y * bytesPerPixel * width) + (bytesPerPixel * x));
			curTexIdx++;
		}
	}

	tglPixelTransferi(TGL_MAP_COLOR, TGL_FALSE);
#endif

	tglPixelStorei(TGL_UNPACK_ALIGNMENT, 4);
	tglPixelStorei(TGL_UNPACK_ROW_LENGTH, 0);

	_maskWidth = width; //(int)(width * _scaleW);
	_maskHeight = height; //(int)(height * _scaleH);
}

void GfxTinyGL::drawMaskedFrame(int srcX, int srcY, int dstX, int dstY, int w, int h, int depth) {
	//debug(3, "tglMask: %d,%d %dx%d %d", rect.top, rect.left, rect.width(), rect.height(), depth);

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

void GfxTinyGL::rotate(float angle, float ax, float ay, float az) {
	tglRotatef(angle, ax, ay, az);
}

void GfxTinyGL::translate(float tx, float ty, float tz) {
	tglTranslatef(tx, ty, tz);
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
