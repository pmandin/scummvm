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

bool HypnoSmackerDecoder::loadStream(Common::SeekableReadStream *stream) {
	if (!SmackerDecoder::loadStream(stream))
		return false;

	// Map audio tracks to sound types
	for (uint32 i = 0; i < 8; i++) {
		Track *t = getTrack(i);
		if (t && t->getTrackType() == Track::kTrackTypeAudio) {
			AudioTrack *audio = (AudioTrack *)t;
			audio->setMute(false);
			audio->setSoundType(i == 7 ? Audio::Mixer::kSpeechSoundType : Audio::Mixer::kSFXSoundType);
		}
	}
	return true;
}

}
