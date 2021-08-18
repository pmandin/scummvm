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

#ifndef AGS_ENGINE_AC_GAME_SETUP_H
#define AGS_ENGINE_AC_GAME_SETUP_H

#include "ags/engine/main/graphics_mode.h"
#include "ags/shared/util/string.h"

namespace AGS3 {

// Mouse control activation type
enum MouseControlWhen {
	kMouseCtrl_Never,       // never control mouse (track system mouse position)
	kMouseCtrl_Fullscreen,  // control mouse in fullscreen only
	kMouseCtrl_Always,      // always control mouse (fullscreen and windowed)
	kNumMouseCtrlOptions
};

// Mouse speed definition, specifies how the speed setting is applied to the mouse movement
enum MouseSpeedDef {
	kMouseSpeed_Absolute,       // apply speed multiplier directly
	kMouseSpeed_CurrentDisplay, // keep speed/resolution relation based on current system display mode
	kNumMouseSpeedDefs
};

using AGS::Shared::String;

// TODO: reconsider the purpose of this struct.
// Earlier I was trying to remove the uses of this struct from the engine
// and restrict it to only config/init stage, while applying its values to
// respective game/engine subcomponents at init stage.
// However, it did not work well at all times, and consequently I thought
// that engine may use a "config" object or combo of objects to store
// current user config, which may also be changed from script, and saved.
struct GameSetup {
	int    audio_backend; // abstract option, currently only works as on/off
	int textheight; // text height used on the certain built-in GUI // TODO: move out to game class?
	bool  no_speech_pack;
	bool  enable_antialiasing;
	bool  disable_exception_handling;
	String startup_dir; // directory where the default game config is located (usually same as main_data_dir)
	String main_data_dir; // main data directory
	String main_data_file; // full path to main data file
	// Following 4 optional dirs are currently for compatibility with Editor only (debug runs)
	// This is bit ugly, but remain so until more flexible configuration is designed
	String install_dir; // optional custom install dir path (also used as extra data dir)
	String opt_data_dir; // optional data dir number 2
	String opt_audio_dir; // optional custom install audio dir path
	String opt_voice_dir; // optional custom install voice-over dir path
	//
	String conf_path; // explicitly set path to config
	bool   local_user_conf; // search for user config in the game directory
	String user_data_dir; // directory to write savedgames and user files to
	String shared_data_dir; // directory to write shared game files to
	String translation;
	bool  mouse_auto_lock;
	int   override_script_os;
	int8_t override_multitasking;
	bool  override_upscale;
	float mouse_speed;
	MouseControlWhen mouse_ctrl_when;
	bool  mouse_ctrl_enabled;
	MouseSpeedDef mouse_speed_def;
	bool  RenderAtScreenRes; // render sprites at screen resolution, as opposed to native one
	int   Supersampling;

	ScreenSetup Screen;

	GameSetup();
};

} // namespace AGS3

#endif
