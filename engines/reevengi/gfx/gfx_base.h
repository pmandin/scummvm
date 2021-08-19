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

#ifndef REEVENGI_GFX_BASE_H
#define REEVENGI_GFX_BASE_H

#include "common/rect.h"
#include "common/scummsys.h"
#include "graphics/surface.h"
#include "math/vector3d.h"

namespace Reevengi {

class GfxBase {
public:
	static const int kRenderZNear = 16;
	static const int kRenderZFar = 65536;

	GfxBase();
	virtual ~GfxBase() { ; }

	int _gameWidth, _gameHeight;

	/**
	 * Creates a render-context.
	 *
	 * @param screenW       the width of the context
	 * @param screenH       the height of the context
	 * @param fullscreen    true if fullscreen is desired, false otherwise.
	 */
	virtual byte *setupScreen(int screenW, int screenH, bool fullscreen) = 0;

	virtual void clearScreen() = 0;

	/**
	 *  Swap the buffers, making the drawn screen visible
	 */
	virtual void flipBuffer() = 0;

	/**
	 * Prepare a movie-frame for drawing
	 * performing any necessary conversion
	 *
	 * @param width         the width of the movie-frame.
	 * @param height        the height of the movie-frame.
	 * @param bitmap        a pointer to the data for the movie-frame.
	 * @see drawMovieFrame
	 * @see releaseMovieFrame
	 */
	virtual void prepareMovieFrame(Graphics::Surface *frame) = 0;
	virtual void drawMovieFrame(int offsetX, int offsetY) = 0;

	/**
	 * Release the currently prepared movie-frame, if one exists.
	 *
	 * @see drawMovieFrame
	 * @see prepareMovieFrame
	 */
	virtual void releaseMovieFrame() = 0;

	/* Draw mask (only to depth buffer), using alpha */
	virtual void prepareMaskedFrame(Graphics::Surface *frame, uint16* timPalette = nullptr) = 0;
	virtual void drawMaskedFrame(int srcX, int srcY, int dstX, int dstY, int w, int h, int depth) = 0;
	virtual void releaseMaskedFrame(void) = 0;

	bool computeScreenViewport(void);

	virtual void setProjection(float angle, float aspect, float zNear, float zFar) = 0;
	virtual void setModelview(float fromX, float fromY, float fromZ,
		float toX, float toY, float toZ, float upX, float upY, float upZ) = 0;
	virtual void loadIdentity(void) = 0;
	virtual void pushMatrix(void) = 0;
	virtual void popMatrix(void) = 0;
	virtual void rotate(float angle, float ax, float ay, float az) = 0;
	virtual void translate(float tx, float ty, float tz) = 0 ;

	virtual void beginTriangles(void) = 0;
	virtual void beginQuads(void) = 0;
	virtual void normal3f(float x, float y, float z) = 0;
	virtual void vertex3f(float x, float y, float z) = 0;
	virtual void endPrim(void) = 0;

	virtual void setBlending(bool enable) =0;
	virtual void setColor(float r, float g, float b) =0;
	virtual void setColorMask(bool enable) =0;
	virtual void setDepth(bool enable) =0;

	virtual void line(Math::Vector3d v0, Math::Vector3d v1) =0;

protected:
	float _scaleW, _scaleH;
	int _screenWidth, _screenHeight;

	int _smushWidth, _smushHeight;
	int _maskWidth, _maskHeight;

	Common::Rect _screenViewport;
};

// Factory-like functions:

GfxBase *CreateGfxOpenGL();
/*GfxBase *CreateGfxOpenGLShader();*/
GfxBase *CreateGfxTinyGL();

extern GfxBase *g_driver;

} // end of namespace Reevengi

#endif
