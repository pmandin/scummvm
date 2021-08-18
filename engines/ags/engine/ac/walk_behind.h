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

#ifndef AGS_ENGINE_AC_WALK_BEHIND_H
#define AGS_ENGINE_AC_WALK_BEHIND_H

namespace AGS3 {

// A method of rendering walkbehinds on screen:
// DrawAsSeparateSprite - draws whole walkbehind as a sprite; this
//     method is most simple and is optimal for 3D renderers.
// DrawOverCharSprite and DrawAsSeparateCharSprite - are alternatives
//     optimized for software render.
// DrawOverCharSprite - turns parts of the character and object sprites
//     transparent when they are covered by walkbehind (walkbehind itself
//     is not drawn separately in this case).
// DrawAsSeparateCharSprite - draws smaller *parts* of walkbehind as
//     separate sprites, only ones that cover characters or objects.
enum WalkBehindMethodEnum {
	DrawOverCharSprite,
	DrawAsSeparateSprite,
	DrawAsSeparateCharSprite
};

void update_walk_behind_images();
void recache_walk_behinds();

} // namespace AGS3

#endif
