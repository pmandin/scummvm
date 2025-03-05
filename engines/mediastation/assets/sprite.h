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

#ifndef MEDIASTATION_SPRITE_H
#define MEDIASTATION_SPRITE_H

#include "common/rect.h"
#include "common/array.h"

#include "mediastation/asset.h"
#include "mediastation/assetheader.h"
#include "mediastation/datafile.h"
#include "mediastation/bitmap.h"
#include "mediastation/mediascript/operand.h"
#include "mediastation/mediascript/scriptconstants.h"

namespace MediaStation {

class SpriteFrameHeader : public BitmapHeader {
public:
	SpriteFrameHeader(Chunk &chunk);
	virtual ~SpriteFrameHeader() override;

	uint _index;
	Common::Point *_boundingBox;
};

class SpriteFrame : public Bitmap {
public:
	SpriteFrame(Chunk &chunk, SpriteFrameHeader *header);
	virtual ~SpriteFrame() override;

	uint32 left();
	uint32 top();
	Common::Point topLeft();
	Common::Rect boundingBox();
	uint32 index();

private:
	SpriteFrameHeader *_bitmapHeader = nullptr;
};

// Sprites are somewhat like movies, but they strictly show one frame at a time
// and don't have sound. They are intended for background/recurrent animations.
class Sprite : public Asset {
friend class Context;

public:
	Sprite(AssetHeader *header);
	~Sprite();

	virtual Operand callMethod(BuiltInMethod methodId, Common::Array<Operand> &args) override;
	virtual void process() override;
	virtual void redraw(Common::Rect &rect) override;

	virtual void readChunk(Chunk &chunk) override;

private:
	Common::Array<SpriteFrame *> _frames;
	SpriteFrame *_activeFrame = nullptr;
	bool _isShowing = false;
	bool _isPlaying = false;
	uint _currentFrameIndex = 0;
	uint _nextFrameTime = 0;

	// Method implementations.
	void spatialShow();
	void spatialHide();
	void timePlay();
	void timeStop();
	void movieReset();
	void setCurrentClip();

	void updateFrameState();
	void showFrame(SpriteFrame *frame);
	Common::Rect getActiveFrameBoundingBox();
};

} // End of namespace MediaStation

#endif