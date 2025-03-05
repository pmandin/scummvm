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

#ifndef MEDIASTATION_STAGE_H
#define MEDIASTATION_STAGE_H

#include "mediastation/asset.h"

namespace MediaStation {

class Camera : public Asset {
public:
	Camera(AssetHeader *header) : Asset(header) {};

	virtual Operand callMethod(BuiltInMethod methodId, Common::Array<Operand> &args) override { error("CallMethod not implemented"); };
	virtual void process() override {};
};

class Stage : public Asset {
public:
	Stage(AssetHeader *header) : Asset(header) {};

	virtual Operand callMethod(BuiltInMethod methodId, Common::Array<Operand> &args) override { error("CallMethod not implemented"); };
	virtual void process() override {};

private:
	Common::Array<Asset *> _children;
};

}

#endif