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

#if defined(WIN32)
#include <windows.h>
// winnt.h defines ARRAYSIZE, but we want our own one...
#undef ARRAYSIZE
#endif

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "math/glmath.h"

#if defined(USE_OPENGL_GAME)

#include "engines/reevengi/gfx/gfx_base.h"
#include "engines/reevengi/gfx/gfx_opengl.h"

namespace Reevengi {

#define BITMAP_TEXTURE_SIZE 256

GfxBase *CreateGfxOpenGL() {
	return new GfxOpenGL();
}

GfxOpenGL::GfxOpenGL() : _smushNumTex(0), _smushTexIds(nullptr),
	_maskNumTex(0), _maskTexPitch(0), _maskTexIds(nullptr) {
}

GfxOpenGL::~GfxOpenGL() {
	releaseMovieFrame();
	releaseMaskedFrame();
	//delete[] _storedDisplay;
}

void GfxOpenGL::setupScreen(int screenW, int screenH) {
	_screenWidth = screenW;
	_screenHeight = screenH;
	_scaleW = _screenWidth / (float)_gameWidth;
	_scaleH = _screenHeight / (float)_gameHeight;

	/*_globalScaleW = _screenWidth / (float)_globalWidth;
	_globalScaleH = _screenHeight / (float)_globalHeight;

	_useDepthShader = false;
	_useDimShader = false;*/

	g_system->showMouse(false);

	//int screenSize = _screenWidth * _screenHeight * 4;
	//_storedDisplay = new byte[screenSize];
	//memset(_storedDisplay, 0, screenSize);
	_smushNumTex = 0;

	/* _currentShadowArray = nullptr; */
	glViewport(0, 0, _screenWidth, _screenHeight);

	GLfloat ambientSource[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientSource);
	GLfloat diffuseReflectance[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseReflectance);
}

const char *GfxOpenGL::getVideoDeviceName() {
	return "OpenGL Renderer";
}

void GfxOpenGL::clearScreen() {
	if (_numFullClear>0) {
		glDisable(GL_SCISSOR_TEST);
		--_numFullClear;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GfxOpenGL::clearDepthBuffer() {
	glClear(GL_DEPTH_BUFFER_BIT);
}

void GfxOpenGL::flipBuffer() {
	g_system->updateScreen();
}

bool GfxOpenGL::isHardwareAccelerated() {
	return true;
}

bool GfxOpenGL::supportsShaders() {
	return false;
}

void GfxOpenGL::prepareMovieFrame(const Graphics::Surface *frame) {
	int height = frame->h;
	int width = frame->w;
	const byte *bitmap = (const byte *) frame->getPixels();

	GLenum format;
	GLenum dataType;
	int bytesPerPixel = frame->format.bytesPerPixel;

	// Aspyr Logo format
	if (frame->format == Graphics::PixelFormat(4, 8, 8, 8, 0, 8, 16, 24, 0)) {
#if !defined(__amigaos4__)
		format = GL_BGRA;
		dataType = GL_UNSIGNED_INT_8_8_8_8;
#else
		// AmigaOS' MiniGL does not understand GL_UNSIGNED_INT_8_8_8_8 yet.
		format = GL_BGRA;
		dataType = GL_UNSIGNED_BYTE;
#endif
	} else if (frame->format == Graphics::PixelFormat(4, 8, 8, 8, 0, 16, 8, 0, 0)) {
		format = GL_BGRA;
		dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
	} else if (frame->format == Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0)) {
		format = GL_RGB;
		dataType = GL_UNSIGNED_SHORT_5_6_5;
	} else if (frame->format == Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0)) {
		format = GL_RGBA;
		dataType = GL_UNSIGNED_SHORT_5_5_5_1;
	} else if (frame->format == Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24)) {
		format = GL_RGBA;
		dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
	} else if (frame->format == Graphics::PixelFormat(3, 8, 8, 8, 0, 0, 8, 16, 0)) {
		format = GL_RGB;
		dataType = GL_UNSIGNED_BYTE;
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
	if (_smushNumTex > 0) {
		glDeleteTextures(_smushNumTex, _smushTexIds);
		delete[] _smushTexIds;
		_smushNumTex = 0;
	}

	// create texture
	_smushNumTex = ((width + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE) *
				   ((height + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_smushTexIds = new GLuint[_smushNumTex];
	glGenTextures(_smushNumTex, _smushTexIds);
	for (int i = 0; i < _smushNumTex; i++) {
		glBindTexture(GL_TEXTURE_2D, _smushTexIds[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BITMAP_TEXTURE_SIZE, BITMAP_TEXTURE_SIZE, 0, format, dataType, nullptr);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, bytesPerPixel); // 16 bit RGB 565 bitmap/32 bit BGR
	glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

	int curTexIdx = 0;
	for (int y = 0; y < height; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < width; x += BITMAP_TEXTURE_SIZE) {
			int t_width = (x + BITMAP_TEXTURE_SIZE >= width) ? (width - x) : BITMAP_TEXTURE_SIZE;
			int t_height = (y + BITMAP_TEXTURE_SIZE >= height) ? (height - y) : BITMAP_TEXTURE_SIZE;
			glBindTexture(GL_TEXTURE_2D, _smushTexIds[curTexIdx]);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t_width, t_height, format, dataType, bitmap + (y * bytesPerPixel * width) + (bytesPerPixel * x));
			curTexIdx++;
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	_smushWidth = width; //(int)(width * _scaleW);
	_smushHeight = height; //(int)(height * _scaleH);
}

void GfxOpenGL::drawMovieFrame(int offsetX, int offsetY) {
	int sysW = g_system->getWidth();
	int sysH = g_system->getHeight();

	float movScale = MIN<float>((float) sysW / _smushWidth, (float) sysH / _smushHeight);
	int movW = _smushWidth * movScale;
	int movH = _smushHeight * movScale;

 	glViewport(0, 0, sysW, sysH);

	// prepare view
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, sysW, sysH, 0, 0, 1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// A lot more may need to be put there : disabling Alpha test, blending, ...
	// For now, just keep this here :-)

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	// draw
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	//glEnable(GL_SCISSOR_TEST);

	offsetX = (int)(offsetX * movScale);
	offsetY = (int)(offsetY * movScale);

	offsetX += (sysW-movW)>>1;
	offsetY += (sysH-movH)>>1;

	//glScissor(offsetX, _screenHeight - (offsetY + movH), movW, movH);

	int curTexIdx = 0;
	for (int y = 0; y < movH; y += (int)(BITMAP_TEXTURE_SIZE * movScale)) {
		for (int x = 0; x < movW; x += (int)(BITMAP_TEXTURE_SIZE * movScale)) {
			glBindTexture(GL_TEXTURE_2D, _smushTexIds[curTexIdx]);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(x + offsetX, y + offsetY);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2f(x + offsetX + BITMAP_TEXTURE_SIZE * movScale, y + offsetY);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2f(x + offsetX + BITMAP_TEXTURE_SIZE * movScale, y + offsetY + BITMAP_TEXTURE_SIZE * movScale);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2f(x + offsetX, y + offsetY + BITMAP_TEXTURE_SIZE * movScale);
			glEnd();
			curTexIdx++;
		}
	}

	//glDisable(GL_SCISSOR_TEST);
	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	glViewport(_screenViewport.left, _screenViewport.top, _screenWidth, _screenHeight);
}

void GfxOpenGL::releaseMovieFrame() {
	if (_smushNumTex > 0) {
		glDeleteTextures(_smushNumTex, _smushTexIds);
		delete[] _smushTexIds;
		_smushNumTex = 0;
	}
}

void GfxOpenGL::prepareMaskedFrame(const Graphics::Surface *frame, uint16* timPalette) {
	int height = frame->h;
	int width = frame->w;
	const byte *bitmap = (const byte *) frame->getPixels();

	GLenum format;
	GLenum dataType;
	int bytesPerPixel = frame->format.bytesPerPixel;

	// Aspyr Logo format
	if (frame->format == Graphics::PixelFormat(4, 8, 8, 8, 0, 8, 16, 24, 0)) {
#if !defined(__amigaos4__)
		format = GL_BGRA;
		dataType = GL_UNSIGNED_INT_8_8_8_8;
#else
		// AmigaOS' MiniGL does not understand GL_UNSIGNED_INT_8_8_8_8 yet.
		format = GL_BGRA;
		dataType = GL_UNSIGNED_BYTE;
#endif
	} else if (frame->format == Graphics::PixelFormat(4, 8, 8, 8, 0, 16, 8, 0, 0)) {
		format = GL_BGRA;
		dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
	} else if (frame->format == Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0)) {
		format = GL_RGB;
		dataType = GL_UNSIGNED_SHORT_5_6_5;
	} else if (frame->format == Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0)) {
		format = GL_RGBA;
		dataType = GL_UNSIGNED_SHORT_5_5_5_1;
	} else if (frame->format == Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24)) {
		format = GL_RGBA;
		dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
	} else if (frame->format == Graphics::PixelFormat(3, 8, 8, 8, 0, 0, 8, 16, 0)) {
		format = GL_RGB;
		dataType = GL_UNSIGNED_BYTE;
 	} else if (frame->format == Graphics::PixelFormat(1, 0, 0, 0, 0, 0, 0, 0, 0)) {
		format = GL_COLOR_INDEX;
		dataType = GL_UNSIGNED_BYTE;
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
		glDeleteTextures(_maskNumTex, _maskTexIds);
		delete[] _maskTexIds;
		_maskNumTex = 0;
	}

	// create texture
	_maskTexPitch = ((width + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_maskNumTex = _maskTexPitch *
				   ((height + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_maskTexIds = new GLuint[_maskNumTex];
	glGenTextures(_maskNumTex, _maskTexIds);
	for (int i = 0; i < _maskNumTex; i++) {
		glBindTexture(GL_TEXTURE_2D, _maskTexIds[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BITMAP_TEXTURE_SIZE, BITMAP_TEXTURE_SIZE, 0, format, dataType, nullptr);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, bytesPerPixel); // 16 bit RGB 565 bitmap/32 bit BGR
	glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

	// Upload palette
	if ((bytesPerPixel==1) && timPalette) {
		GLfloat mapR[256], mapG[256], mapB[256], mapA[256];
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
		glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, mapR);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, mapG);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, mapB);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, mapA);
	}

	int curTexIdx = 0;
	for (int y = 0; y < height; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < width; x += BITMAP_TEXTURE_SIZE) {
			int t_width = (x + BITMAP_TEXTURE_SIZE >= width) ? (width - x) : BITMAP_TEXTURE_SIZE;
			int t_height = (y + BITMAP_TEXTURE_SIZE >= height) ? (height - y) : BITMAP_TEXTURE_SIZE;
			glBindTexture(GL_TEXTURE_2D, _maskTexIds[curTexIdx]);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t_width, t_height, format, dataType, bitmap + (y * bytesPerPixel * width) + (bytesPerPixel * x));
			curTexIdx++;
		}
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

	_maskWidth = width; //(int)(width * _scaleW);
	_maskHeight = height; //(int)(height * _scaleH);
}

void GfxOpenGL::drawMaskedFrame(int srcX, int srcY, int dstX, int dstY, int w, int h, int depth) {
	int sysW = _screenWidth;
	int sysH = _screenHeight;

	float movScale = MIN<float>((float) sysW / 320, (float) sysH / 240);
	int movW = 320 * movScale;
	int movH = 240 * movScale;

	float bitmapDepth= 1.0f - (kRenderZNear / (float) depth);
	bitmapDepth *= kRenderZFar / (kRenderZFar - kRenderZNear);

 	glViewport(_screenViewport.left, _screenViewport.top, _screenWidth, _screenHeight);

	// prepare view
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _screenWidth, _screenHeight, 0, 0, 1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(1.0f / BITMAP_TEXTURE_SIZE, 1.0f / BITMAP_TEXTURE_SIZE, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(movScale, movScale, 1.0f);
	glTranslatef(0.0f, 0.0f, -bitmapDepth);

	// A lot more may need to be put there : disabling Alpha test, blending, ...
	// For now, just keep this here :-)

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	// draw
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	//glEnable(GL_SCISSOR_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
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

			if (rowTexNum*_maskTexPitch + colTexNum < _maskNumTex) {
				glBindTexture(GL_TEXTURE_2D, _maskTexIds[rowTexNum*_maskTexPitch + colTexNum]);

				glBegin(GL_QUADS);
				glTexCoord2f(tx+0.5, ty+0.5);
				glVertex2f(dstX+x, dstY+y);
				glTexCoord2f(tx-0.5 + tw, ty+0.5);
				glVertex2f(dstX+x + tw, dstY+y);
				glTexCoord2f(tx-0.5 + tw, ty-0.5 + th);
				glVertex2f(dstX+x + tw, dstY+y + th);
				glTexCoord2f(tx+0.5, ty-0.5 + th);
				glVertex2f(dstX+x, dstY+y + th);
				glEnd();
			}

			x += tw;
		}

		y += th;
	}

	setBlending(false);
	//glDisable(GL_SCISSOR_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void GfxOpenGL::releaseMaskedFrame(void) {
	if (_maskNumTex > 0) {
		glDeleteTextures(_maskNumTex, _maskTexIds);
		delete[] _maskTexIds;
		_maskNumTex = 0;
	}
}

void GfxOpenGL::setProjection(float angle, float aspect, float zNear, float zFar) {
	Math::Matrix4 mProjection = Math::makePerspectiveMatrix(angle, aspect, zNear, zFar);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMultMatrixf(mProjection.getData());

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void GfxOpenGL::setModelview(float fromX, float fromY, float fromZ,
	float toX, float toY, float toZ,
	float upX, float upY, float upZ) {

	Math::Matrix4 mLookAt = Math::makeLookAtMatrix(Math::Vector3d(fromX, fromY, fromZ),
		Math::Vector3d(toX, toY, toZ), Math::Vector3d(upX, upY, upZ));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(mLookAt.getData());

	glTranslatef(-fromX, -fromY, -fromZ);
}

void GfxOpenGL::MatrixModeModelview(void) {
	glMatrixMode(GL_MODELVIEW);
}

void GfxOpenGL::MatrixModeTexture(void) {
	glMatrixMode(GL_TEXTURE);
}

void GfxOpenGL::loadIdentity(void) {
	glLoadIdentity();
}

void GfxOpenGL::pushMatrix(void) {
	glPushMatrix();
}

void GfxOpenGL::popMatrix(void) {
	glPopMatrix();
}

void GfxOpenGL::rotate(float angle, float ax, float ay, float az) {
	glRotatef(angle, ax, ay, az);
}

void GfxOpenGL::translate(float tx, float ty, float tz) {
	glTranslatef(tx, ty, tz);
}

void GfxOpenGL::beginTriangles(void) {
	glBegin(GL_TRIANGLES);
}

void GfxOpenGL::beginQuads(void) {
	glBegin(GL_QUADS);
}

void GfxOpenGL::normal3f(float x, float y, float z) {
	glNormal3f(x, y, z);
}

void GfxOpenGL::texCoord2f(float s, float r) {
	glTexCoord2f(s, r);
}

void GfxOpenGL::vertex3f(float x, float y, float z) {
	glVertex3f(x, y, z);
}

void GfxOpenGL::endPrim(void) {
	glEnd();
}

uint GfxOpenGL::genTexture(void) {
	GLuint _newTexId[1];

	glGenTextures(1, _newTexId);

	return _newTexId[0];
}

void GfxOpenGL::bindTexture(uint texId) {
	glBindTexture(GL_TEXTURE_2D, texId);
}

void GfxOpenGL::createTexture(const Graphics::Surface *frame, uint16* timPalette) {
	GLenum format, dataType;

	if (!frame)
		return;

	if (frame->format == Graphics::PixelFormat(1, 0, 0, 0, 0, 0, 0, 0, 0)) {
		format = GL_COLOR_INDEX;
		dataType = GL_UNSIGNED_BYTE;
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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glPixelStorei(GL_UNPACK_ALIGNMENT, frame->format.bytesPerPixel);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->w);

	// Upload palette
	if ((frame->format.bytesPerPixel==1) && timPalette) {
		GLfloat mapR[256], mapG[256], mapB[256], mapA[256];
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
		glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, mapR);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, mapG);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, mapB);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, mapA);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame->w, frame->h, 0, format, dataType, frame->getPixels());
}

void GfxOpenGL::setBlending(bool enable) {
	if (enable) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
	} else {
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
	}
}

void GfxOpenGL::setColor(float r, float g, float b) {
	glColor3f(r, g, b);
}

void GfxOpenGL::setColorMask(bool enable) {
	glColorMask(enable ? GL_TRUE : GL_FALSE, enable ? GL_TRUE : GL_FALSE,
		enable ? GL_TRUE : GL_FALSE, enable ? GL_TRUE : GL_FALSE);
}

void GfxOpenGL::setDepth(bool enable) {
	if (enable) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
}

void GfxOpenGL::setTexture2d(bool enable) {
	if (enable) {
		glEnable(GL_TEXTURE_2D);
	} else {
		glDisable(GL_TEXTURE_2D);
	}
}

void GfxOpenGL::line(Math::Vector3d v0, Math::Vector3d v1) {
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	//glDisable(GL_DEPTH_TEST);

	glBegin(GL_LINES);
		glVertex3f(v0.x(), v0.y(), v0.z());
		glVertex3f(v1.x(), v1.y(), v1.z());
	glEnd();
}

} // End of namespace Reevengi

#else

namespace Reevengi {

GfxBase *CreateGfxOpenGL() {
	return nullptr;
}

}

#endif // USE_OPENGL
