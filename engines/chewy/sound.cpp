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

#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "audio/decoders/raw.h"
#include "audio/mods/protracker.h"
#include "common/config-manager.h"
#include "common/system.h"
#include "chewy/resource.h"
#include "chewy/sound.h"
#include "chewy/types.h"
#include "chewy/globals.h"
#include "chewy/music/tmf_stream.h"

namespace Chewy {

const uint8 Sound::TMF_MOD_SONG_NAME[] = {
	'S', 'C', 'U', 'M', 'M',
	'V', 'M', ' ', 'M', 'O',
	'D', 'U', 'L', 'E', '\0',
	'\0', '\0', '\0', '\0', '\0'};
const uint8 Sound::TMF_MOD_INSTRUMENT_NAME[] = {
	'S', 'C', 'U', 'M', 'M',
	'V', 'M', ' ', 'I', 'N',
	'S', 'T', 'R', 'U', 'M',
	'E', 'N', 'T', '\0', '\0',
	'\0', '\0'};
// TODO Verify period values used by the game; this is an educated guess.
const uint16 Sound::TMF_MOD_PERIODS[] = {
	856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113
};

Sound::Sound(Audio::Mixer *mixer) {
	_mixer = mixer;
	_speechRes = new SoundResource("speech.tvp");
	_soundRes = new SoundResource("details.tap");
}

Sound::~Sound() {
	delete _soundRes;
	delete _speechRes;
}

void Sound::playSound(int num, uint channel, bool loop) {
	SoundChunk *sound = _soundRes->getSound(num);
	uint8 *data = (uint8 *)MALLOC(sound->size);
	memcpy(data, sound->data, sound->size);

	playSound(data, sound->size, channel, loop);

	delete[] sound->data;
	delete sound;
}

void Sound::playSound(uint8 *data, uint32 size, uint channel, bool loop, DisposeAfterUse::Flag dispose) {
	Audio::AudioStream *stream = Audio::makeLoopingAudioStream(
	                                 Audio::makeRawStream(data,
	                                         size, 22050, Audio::FLAG_UNSIGNED,
	                                         dispose),
	                                 loop ? 0 : 1);

	_mixer->playStream(Audio::Mixer::kSFXSoundType, &_soundHandle[channel], stream);
}

void Sound::pauseSound(uint channel) {
	assert(channel < MAX_SOUND_EFFECTS);
	_mixer->pauseHandle(_soundHandle[channel], true);
}

void Sound::resumeSound(uint channel) {
	assert(channel < MAX_SOUND_EFFECTS);
	_mixer->pauseHandle(_soundHandle[channel], false);
}

void Sound::stopSound(uint channel) {
	assert(channel < MAX_SOUND_EFFECTS);
	_mixer->stopHandle(_soundHandle[channel]);
}

void Sound::stopAllSounds() {
	for (int i = 4; i < 8; i++)
		stopSound(i);
}

bool Sound::isSoundActive(uint channel) {
	assert(channel < MAX_SOUND_EFFECTS);
	return _mixer->isSoundHandleActive(_soundHandle[channel]);
}

void Sound::setSoundVolume(uint volume) {
	_mixer->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, volume);
}

void Sound::setSoundChannelVolume(uint channel, uint volume) {
	assert(channel < MAX_SOUND_EFFECTS);
	_mixer->setChannelVolume(_soundHandle[channel], volume);
}

void Sound::setSoundChannelBalance(uint channel, int8 balance) {
	assert(channel < MAX_SOUND_EFFECTS);
	_mixer->setChannelBalance(_soundHandle[channel], balance);
}

void Sound::playMusic(int num, bool loop) {
	uint32 musicNum = _soundRes->getChunkCount() - 1 - num;
	Chunk *chunk = _soundRes->getChunk(musicNum);
	uint8 *data = _soundRes->getChunkData(musicNum);

	playMusic(data, chunk->size, loop);

	delete[] data;
}

void Sound::playMusic(uint8 *data, uint32 size, bool loop, DisposeAfterUse::Flag dispose) {
	TMFStream *stream = new TMFStream(new Common::MemoryReadStream(data, size), 0);

	_mixer->playStream(Audio::Mixer::kMusicSoundType, &_musicHandle, stream);
}

void Sound::pauseMusic() {
	_mixer->pauseHandle(_musicHandle, true);
}

void Sound::resumeMusic() {
	_mixer->pauseHandle(_musicHandle, false);
}

void Sound::stopMusic() {
	_mixer->stopHandle(_musicHandle);
}

bool Sound::isMusicActive() {
	return _mixer->isSoundHandleActive(_musicHandle);
}

void Sound::setMusicVolume(uint volume) {
	_mixer->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, volume);
}

void Sound::playSpeech(int num, bool waitForFinish) {
	if (isSpeechActive())
		stopSpeech();

	// Get the speech data
	SoundChunk *sound = _speechRes->getSound(num);
	size_t size = sound->size;
	uint8 *data = (uint8 *)MALLOC(size);
	memcpy(data, sound->data, size);

	delete[] sound->data;
	delete sound;

	// Play it
	Audio::AudioStream *stream = Audio::makeLoopingAudioStream(
	    Audio::makeRawStream(data, size, 22050, Audio::FLAG_UNSIGNED,
		DisposeAfterUse::YES), 1);

	_mixer->playStream(Audio::Mixer::kSpeechSoundType,
		&_speechHandle, stream);

	if (waitForFinish) {
		// Wait for speech to finish
		while (isSpeechActive() && !SHOULD_QUIT) {
			setupScreen(DO_SETUP);
		}
	}
}

void Sound::pauseSpeech() {
	_mixer->pauseHandle(_speechHandle, true);
}

void Sound::resumeSpeech() {
	_mixer->pauseHandle(_speechHandle, false);
}

void Sound::stopSpeech() {
	_mixer->stopHandle(_speechHandle);
}

bool Sound::isSpeechActive() {
	return _mixer->isSoundHandleActive(_speechHandle);
}

void Sound::setSpeechVolume(uint volume) {
	_mixer->setVolumeForSoundType(Audio::Mixer::kSpeechSoundType, volume);
}

void Sound::stopAll() {
	_mixer->stopAll();
}

void Sound::waitForSpeechToFinish() {
	if (speechEnabled()) {
		while (isSpeechActive() && !SHOULD_QUIT) {
			setupScreen(DO_SETUP);
		}
	}
}

bool Sound::soundEnabled() const {
	return !ConfMan.getBool("sfx_mute");
}

void Sound::toggleSound(bool enable) {
	return ConfMan.setBool("sfx_mute", !enable);
}

bool Sound::musicEnabled() const {
	return !ConfMan.getBool("music_mute");
}

void Sound::toggleMusic(bool enable) {
	return ConfMan.setBool("music_mute", !enable);
}

bool Sound::speechEnabled() const {
	return !ConfMan.getBool("speech_mute");
}

void Sound::toggleSpeech(bool enable) {
	return ConfMan.setBool("speech_mute", !enable);
}

bool Sound::subtitlesEnabled() const {
	return ConfMan.getBool("subtitles");
}

void Sound::toggleSubtitles(bool enable) {
	return ConfMan.setBool("subtitles", enable);
}

} // namespace Chewy
