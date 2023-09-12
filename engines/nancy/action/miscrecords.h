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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef NANCY_ACTION_RECORDTYPES_H
#define NANCY_ACTION_RECORDTYPES_H

#include "engines/nancy/action/actionrecord.h"

namespace Nancy {

class NancyEngine;

namespace Action {

class Unimplemented : public ActionRecord {
	void execute() override;
};

class PaletteThisScene : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	byte _paletteID;
	byte _unknownEnum; // enum w values 1-3
	uint16 _paletteStart;
	uint16 _paletteSize;

protected:
	Common::String getRecordTypeName() const override { return "PaletteThisScene"; }
};

class PaletteNextScene : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	byte _paletteID;

protected:
	Common::String getRecordTypeName() const override { return "PaletteNextScene"; }
};

class LightningOn : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	int16 _distance;
	uint16 _pulseTime;
	int16 _rgbPercent;

protected:
	Common::String getRecordTypeName() const override { return "LightningOn"; }
};

class SpecialEffect : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	byte _type = 1;
	uint16 _fadeToBlackTime = 0;
	uint16 _frameTime = 0;

protected:
	Common::String getRecordTypeName() const override { return "SpecialEffect"; }
};

class TextBoxWrite : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	Common::String _text;

protected:
	Common::String getRecordTypeName() const override { return "TextBoxWrite"; }
};

class TextboxClear : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

protected:
	Common::String getRecordTypeName() const override { return "TextboxClear"; }
};

class BumpPlayerClock : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	byte _relative;
	uint16 _hours;
	uint16 _minutes;

protected:
	Common::String getRecordTypeName() const override { return "BumpPlayerClock"; }
};

class SaveContinueGame : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

protected:
	Common::String getRecordTypeName() const override { return "SaveContinueGame"; }
};

class TurnOffMainRendering : public Unimplemented {
public:
	void readData(Common::SeekableReadStream &stream) override;

protected:
	Common::String getRecordTypeName() const override { return "TurnOffMainRendering"; }
};

class TurnOnMainRendering : public Unimplemented {
public:
	void readData(Common::SeekableReadStream &stream) override;

protected:
	Common::String getRecordTypeName() const override { return "TurnOnMainRendering"; }
};

class ResetAndStartTimer : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

protected:
	Common::String getRecordTypeName() const override { return "ResetAndStartTimer"; }
};

class StopTimer : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

protected:
	Common::String getRecordTypeName() const override { return "StopTimer"; }
};

class EventFlags : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	MultiEventFlagDescription _flags;

protected:
	Common::String getRecordTypeName() const override { return "EventFlags"; }
};

class EventFlagsMultiHS : public EventFlags {
public:
	EventFlagsMultiHS(bool isCursor) : _isCursor(isCursor) {}
	virtual ~EventFlagsMultiHS() {}

	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	CursorManager::CursorType getHoverCursor() const override { return _hoverCursor; }

	CursorManager::CursorType _hoverCursor = CursorManager::kHotspot;
	Common::Array<HotspotDescription> _hotspots;

	bool _isCursor;

protected:
	bool canHaveHotspot() const override { return true; }
	Common::String getRecordTypeName() const override { return _isCursor ? "EventFlagsCursorHS" : "EventFlagsMultiHS"; }
};

class LoseGame : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

protected:
	Common::String getRecordTypeName() const override { return "LoseGame"; }
};

class PushScene : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

protected:
	Common::String getRecordTypeName() const override { return "PushScene"; }
};

class PopScene : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

protected:
	Common::String getRecordTypeName() const override { return "PopScene"; }
};

class WinGame : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

protected:
	Common::String getRecordTypeName() const override { return "WinGame"; }
};

class AddInventoryNoHS : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	uint _itemID;

protected:
	Common::String getRecordTypeName() const override { return "AddInventoryNoHS"; }
};

class RemoveInventoryNoHS : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	uint _itemID;

protected:
	Common::String getRecordTypeName() const override { return "RemoveInventoryNoHS"; }
};

class DifficultyLevel : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	uint16 _difficulty = 0;
	FlagDescription _flag;

protected:
	Common::String getRecordTypeName() const override { return "DifficultyLevel"; }
};

class ShowInventoryItem : public RenderActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	ShowInventoryItem() : RenderActionRecord(9) {}
	virtual ~ShowInventoryItem() { _fullSurface.free(); }

	void init() override;

	uint16 _objectID = 0;
	Common::String _imageName;
	Common::Array<BitmapDescription> _bitmaps;

	int16 _drawnFrameID = -1;
	Graphics::ManagedSurface _fullSurface;

protected:
	bool canHaveHotspot() const override { return true; }
	Common::String getRecordTypeName() const override { return "ShowInventoryItem"; }
	bool isViewportRelative() const override { return true; }
};

class HintSystem : public ActionRecord {
public:
	void readData(Common::SeekableReadStream &stream) override;
	void execute() override;

	byte _characterID; // 0x00
	SoundDescription _genericSound; // 0x01

	const Hint *selectedHint;
	int16 _hintID;

	void selectHint();

protected:
	Common::String getRecordTypeName() const override { return "HintSystem"; }
};

} // End of namespace Action
} // End of namespace Nancy

#endif // NANCY_ACTION_RECORDTYPES_H
