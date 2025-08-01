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

/*
 * This file is based on WME Lite.
 * http://dead-code.org/redir.php?target=wmelite
 * Copyright (c) 2011 Jan Nedoma
 */

#include "engines/wintermute/base/base.h"
#include "engines/wintermute/base/base_game.h"
#include "engines/wintermute/base/base_engine.h"
#include "engines/wintermute/base/base_parser.h"
#include "engines/wintermute/base/base_dynamic_buffer.h"

namespace Wintermute {

//////////////////////////////////////////////////////////////////////
BaseClass::BaseClass(BaseGame *gameOwner) {
	_gameRef = gameOwner;
	_persistable = true;
}


//////////////////////////////////////////////////////////////////////////
BaseClass::BaseClass() {
	_gameRef = nullptr;
	_persistable = true;
}


//////////////////////////////////////////////////////////////////////
BaseClass::~BaseClass() {
	_editorProps.clear();
}


//////////////////////////////////////////////////////////////////////////
Common::String BaseClass::getEditorProp(const Common::String &propName, const Common::String &initVal) {
	_editorPropsIter = _editorProps.find(propName);
	if (_editorPropsIter != _editorProps.end()) {
		return _editorPropsIter->_value.c_str();
	} else {
		return initVal; // Used to be NULL
	}
}


//////////////////////////////////////////////////////////////////////////
bool BaseClass::setEditorProp(const Common::String &propName, const Common::String &propValue) {
	if (propName.size() == 0) {
		return STATUS_FAILED;
	}

	if (propValue.size() == 0) {
		_editorProps.erase(propName);
	} else {
		_editorProps[propName] = propValue;
	}
	return STATUS_OK;
}



TOKEN_DEF_START
TOKEN_DEF(EDITOR_PROPERTY)
TOKEN_DEF(NAME)
TOKEN_DEF(VALUE)
TOKEN_DEF_END
//////////////////////////////////////////////////////////////////////////
bool BaseClass::parseEditorProperty(char *buffer, bool complete) {
	TOKEN_TABLE_START(commands)
	TOKEN_TABLE(EDITOR_PROPERTY)
	TOKEN_TABLE(NAME)
	TOKEN_TABLE(VALUE)
	TOKEN_TABLE_END


	if (!_gameRef->_editorMode) {
		return STATUS_OK;
	}


	char *params;
	int cmd;
	BaseParser parser;

	if (complete) {
		if (parser.getCommand(&buffer, commands, &params) != TOKEN_EDITOR_PROPERTY) {
			BaseEngine::LOG(0, "'EDITOR_PROPERTY' keyword expected.");
			return STATUS_FAILED;
		}
		buffer = params;
	}

	char *propName = nullptr;
	char *propValue = nullptr;

	while ((cmd = parser.getCommand(&buffer, commands, &params)) > 0) {
		switch (cmd) {
		case TOKEN_NAME: {
			delete[] propName;
			size_t propNameSize = strlen(params) + 1;
			propName = new char[propNameSize];
			Common::strcpy_s(propName, propNameSize, params);
			break;
		}
		case TOKEN_VALUE: {
			delete[] propValue;
			size_t propValueSize = strlen(params) + 1;
			propValue = new char[propValueSize];
			Common::strcpy_s(propValue, propValueSize, params);
			break;
		}
		default:
			break;
		}

	}
	if (cmd == PARSERR_TOKENNOTFOUND) {
		delete[] propName;
		delete[] propValue;
		BaseEngine::LOG(0, "Syntax error in EDITOR_PROPERTY definition");
		return STATUS_FAILED;
	}
	if (cmd == PARSERR_GENERIC || propName == nullptr || propValue == nullptr) {
		delete[] propName;
		delete[] propValue;
		BaseEngine::LOG(0, "Error loading EDITOR_PROPERTY definition");
		return STATUS_FAILED;
	}


	setEditorProp(propName, propValue);

	delete[] propName;
	delete[] propValue;

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool BaseClass::saveAsText(BaseDynamicBuffer *buffer, int indent) {
	_editorPropsIter = _editorProps.begin();
	while (_editorPropsIter != _editorProps.end()) {
		buffer->putTextIndent(indent, "EDITOR_PROPERTY\n");
		buffer->putTextIndent(indent, "{\n");
		buffer->putTextIndent(indent + 2, "NAME=\"%s\"\n", _editorPropsIter->_key.c_str());
		buffer->putTextIndent(indent + 2, "VALUE=\"%s\"\n", _editorPropsIter->_value.c_str());
		buffer->putTextIndent(indent, "}\n\n");

		_editorPropsIter++;
	}
	return STATUS_OK;
}

} // End of namespace Wintermute
