/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
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

#include "hypno/grammar.h"
#include "hypno/hypno.h"

namespace Hypno {

void SpiderEngine::showConversation() {
	debugC(1, kHypnoDebugScene, "Showing conversation");
	uint32 x = 0;
	uint32 y = 0;
	Graphics::Surface *speaker = decodeFrame("dialog/speaker3.smk", 0);
	bool activeFound = false;

	// First iteration on the talk commands
	Videos videos;
	for (Actions::iterator itt = _conversation.begin(); itt != _conversation.end(); ++itt) {
		Talk *a = (Talk *)*itt;
		if (a->boxPos != Common::Point(0, 0)) {
			if (!(x == 0 && x == y))
				error("Multiple BOX positions found");

			x = a->boxPos.x;
			y = a->boxPos.y;
		}
		if (!a->intro.empty() && !_intros.contains(a->intro)) {
			videos.push_back(MVideo(a->intro, a->introPos, false, false, false));
			_intros[a->intro] = true;
		}
	}

	if (videos.size() > 0) {
		runIntros(videos);
		videos.clear();
	}

	if (x == 0 && x == y)
		error("BOX position not found");

	// Second iteration on the talk commands
	for (Actions::const_iterator itt = _conversation.begin(); itt != _conversation.end(); ++itt) {
		Talk *a = (Talk *)*itt;
		if (a->active) {
			uint32 frame;
			Common::String path;
			for (TalkCommands::const_iterator it = a->commands.begin(); it != a->commands.end(); ++it) {
				if (it->command == "F") {
					frame = it->num;
				} else if (it->command == "G") {
					path = it->path;
				}
			}
			if (!path.empty()) {
				activeFound = true;
				frame = frame;
				Graphics::Surface *surf = decodeFrame("dialog/" + path, frame);

				drawImage(*speaker, x, y, false);
				drawImage(*surf, x + speaker->w, y, false);
				a->rect = Common::Rect(x + speaker->w, y, x + surf->w, y + surf->h);
				y = y + surf->h;
				
				surf->free();
				delete surf;
			}
		}
	}
	if (!activeFound) {
		debugC(1, kHypnoDebugScene, "No active item was found in the current conversation");
		// Final iteration on the talk commands
		bool shouldEscape = false;
		for (Actions::const_iterator it = _conversation.begin(); it != _conversation.end(); ++it) {
			Talk *a = (Talk *)*it;
			if (!a->second.empty()) {
				debugC(1, kHypnoDebugScene, "Adding %s to play after the conversation ends", a->second.c_str());
				videos.push_back(MVideo(a->second, a->secondPos, false, false, false));
			}
			if (a->escape) {
				shouldEscape = true;
			}
		}

		if (videos.size() > 0) {
			runIntros(videos);
			videos.clear();
		}

		debugC(1, kHypnoDebugScene, "Clearing conversation");
		_conversation.clear();

		if (shouldEscape) {
			runIntros(_escapeSequentialVideoToPlay);
			_escapeSequentialVideoToPlay.clear();
		}

		drawScreen();
	} 
	speaker->free();
	delete speaker;
}

void SpiderEngine::leftClickedConversation(const Common::Point &mousePos) {
	Talk *t;
	Videos videos;
	for (Actions::const_iterator itt = _conversation.begin(); itt != _conversation.end(); ++itt) {
		Talk *a = (Talk *)*itt;
		if (a->active && a->rect.contains(mousePos)) {
			a->active = false;
			for (TalkCommands::const_iterator it = a->commands.begin(); it != a->commands.end(); ++it) {
				if (it->command == "A") {
					debugC(1, kHypnoDebugScene, "Adding option %d", it->num);
					t = (Talk *)_conversation[it->num];
					t->active = true;
					_refreshConversation = true;
				} else if (it->command == "D") {
					debugC(1, kHypnoDebugScene, "Disabling option %d", it->num);
					t = (Talk *)_conversation[it->num];
					t->active = false;
					_refreshConversation = true;
				} else if (it->command == "P") {
					debugC(1, kHypnoDebugScene, "Playing %s", it->path.c_str());
					videos.push_back(MVideo(it->path, it->position, false, false, false));
					_refreshConversation = true;
				} else if (it->command == "S") {
					debugC(1, kHypnoDebugScene, "Enabling variable %s", it->variable.c_str());
					_sceneState[it->variable] = 1;
					_refreshConversation = true;
				} else if (it->command == "L") {
					Common::String variable = "GS_LEVELCOMPLETE";
					debugC(1, kHypnoDebugScene, "Enabling variable %s", variable.c_str());
					_sceneState[variable] = 1;
					_refreshConversation = true;
				}

			}
		}
		if (!a->background.empty()) {
			loadImage(a->background, a->backgroundPos.x, a->backgroundPos.y, false);
		}
	}
	if (videos.size() > 0)
		runIntros(videos);
}

void SpiderEngine::rightClickedConversation(const Common::Point &mousePos) {
	Videos videos;
	for (Actions::const_iterator itt = _conversation.begin(); itt != _conversation.end(); ++itt) {
		Talk *a = (Talk *)*itt;
		if (a->active && a->rect.contains(mousePos)) {
			for (TalkCommands::const_iterator it = a->commands.begin(); it != a->commands.end(); ++it) {
				if (it->command == "I") {
					debugC(1, kHypnoDebugScene, "Playing %s", it->path.c_str());
					// Not sure why position is 50, 50 since there is only one pixel
					videos.push_back(MVideo(it->path, Common::Point(0, 0), false, false, false));
				}
			}
		}
	}
	if (videos.size() > 0)
		runIntros(videos);
}

} // End of namespace Hypno
