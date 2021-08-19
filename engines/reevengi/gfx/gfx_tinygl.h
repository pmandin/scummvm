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

#ifndef REEVENGI_GFX_TINYGL_H
#define REEVENGI_GFX_TINYGL_H

#include "engines/reevengi/gfx/gfx_base.h"

#include "graphics/tinygl/zgl.h"

namespace Reevengi {

class GfxTinyGL : public GfxBase {
public:
	GfxTinyGL();
	virtual ~GfxTinyGL();

	byte *setupScreen(int screenW, int screenH, bool fullscreen) override;

	const char *getVideoDeviceName() /*override*/;

	void clearScreen() override;
	void clearDepthBuffer() /*override*/;
	void flipBuffer() override;

	bool isHardwareAccelerated() /*override*/;
	bool supportsShaders() /*override*/;

	void prepareMovieFrame(Graphics::Surface *frame) override;
	void drawMovieFrame(int offsetX, int offsetY) override;
	void releaseMovieFrame() override;

	void prepareMaskedFrame(Graphics::Surface *frame, uint16* timPalette = nullptr) override;
	void drawMaskedFrame(int srcX, int srcY, int dstX, int dstY, int w, int h, int depth) override;
	void releaseMaskedFrame(void) override;

	void setProjection(float angle, float aspect, float zNear, float zFar) override;
	void setModelview(float fromX, float fromY, float fromZ,
		float toX, float toY, float toZ,
		float upX, float upY, float upZ) override;
	void loadIdentity(void) override;
	void pushMatrix(void) override;
	void popMatrix(void) override;
	void rotate(float angle, float ax, float ay, float az) override;
	void translate(float tx, float ty, float tz) override;

	void beginTriangles(void) override;
	void beginQuads(void) override;
	void normal3f(float x, float y, float z) override;
	void vertex3f(float x, float y, float z) override;
	void endPrim(void) override;

	void setBlending(bool enable) override;
	void setColor(float r, float g, float b) override;
	void setColorMask(bool enable) override;
	void setDepth(bool enable) override;

	void line(Math::Vector3d v0, Math::Vector3d v1) override;

private:
	TinyGL::FrameBuffer *_zb;
	Graphics::PixelFormat _pixelFormat;
	Graphics::PixelBuffer _storedDisplay;
	Graphics::BlitImage *_smushImage;

	int _maskNumTex, _maskTexPitch;
	TGLuint *_maskTexIds;
};

} // End of namespace Reevengi

#endif
