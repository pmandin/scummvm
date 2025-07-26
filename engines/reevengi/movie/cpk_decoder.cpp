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
 * Sega FILM Format (CPK) Demuxer
 * Copyright (c) 2003 The FFmpeg project
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Sega FILM (.cpk) file demuxer
 * by Mike Melanson (melanson@pcisys.net)
 * For more information regarding the Sega FILM file format, visit:
 *   http://www.pcisys.net/~melanson/codecs/
 */

#include "common/scummsys.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "audio/audiostream.h"
#include "audio/decoders/raw.h"

#include "image/codecs/cinepak.h"

#include "engines/reevengi/movie/cpk_decoder.h"
#include "engines/reevengi/movie/movie.h"

namespace Reevengi {

typedef struct {
	uint32	tag;
	uint32	data_offset;
	uint32	version;
	uint32	dummy;
} film_header_t;

typedef struct {
	uint32	tag;
	uint32	chunk_length;
	uint32	video_tag;
	uint32	video_height;

	uint32	video_width;
	byte	video_bits;
	byte	audio_channels;
	byte	audio_bits;
	byte	audio_format;
	uint16	audio_samplerate;
	uint16	dummy5;
	uint32	dummy6;
} film_fdsc_t;

typedef struct {
	uint32	tag;
	uint32	chunk_length;
	uint32	base_clock;
	uint32	sample_count;
} film_stab_t;

typedef struct {
	uint32	offset;
	uint32	size;
	uint32	pts;
	uint32	dummy4;
} film_stabdata_t;

CpkMovieDecoder::CpkMovieDecoder()
{
#if 0
		_stream(0), _videoTrack(0) {
	_streamVideoOffset = 0;
	_streamAudioOffset = 0;
#endif
	_sample_table = nullptr;
}

CpkMovieDecoder::~CpkMovieDecoder() {
	close();

	free(_sample_table);
	_sample_table = nullptr;
}

/*Video::VideoDecoder::AudioTrack* CpkMovieDecoder::getAudioTrack(int index) {
	//return _audioTracks[index];
}*/

bool CpkMovieDecoder::loadStream(Common::SeekableReadStream *stream) {
	film_header_t film_header;
	film_fdsc_t film_description;
	film_stab_t	film_sampletable;
	uint32 chunkTag, data_offset;
	uint32 audio_frame_counter;
	uint32 video_frame_counter;
	uint32 videoWidth = 0, videoHeight = 0;
	unsigned int i;

	_stream = stream;
	_video_type = CPK_CODEC_NONE;
	_audio_type = CPK_CODEC_NONE;

	/* Read header */
	_stream->read(&film_header, sizeof(film_header_t));
	if (FROM_BE_32(film_header.tag) != MKTAG('F','I','L','M')) {
		close();
		return false;
	}

	/* Read description */
	if (FROM_BE_32(film_header.version) == 0) {
		/* special case for Lemmings .film files; 20-byte header */
		_stream->read(&film_description, 20);

		/* make some assumptions about the audio parameters */
		_audio_type = CPK_ACODEC_PCM_S8;
		_audio_samplerate = 22050;
		_audio_channels = 1;
		_audio_bits = 8;
	} else {
		/* normal Saturn .cpk files; 32-byte header */
		_stream->read(&film_description, sizeof(film_fdsc_t));

		_audio_samplerate = FROM_BE_16(film_description.audio_samplerate);
		_audio_channels = film_description.audio_channels;
		_audio_bits = film_description.audio_bits;

		if ((film_description.audio_format == 2) && (_audio_channels > 0)) {
			_audio_type = CPK_ACODEC_ADPCM_ADX;
			debug(3, "cpk: audio: adpcm");
		} else if (_audio_channels > 0) {
			if (_audio_bits == 8) {
				_audio_type = CPK_ACODEC_PCM_S8_PLANAR;
				debug(3, "cpk: audio: pcm s8");
			} else if (_audio_bits == 16) {
				_audio_type = CPK_ACODEC_PCM_S16BE_PLANAR;
				debug(3, "cpk: audio: pcm s16be");
			}
		}
	}

	chunkTag = FROM_BE_32(film_description.video_tag);
	if (chunkTag == MKTAG('c','v','i','d')) {
		_video_type = CPK_VCODEC_CINEPAK;
	} else if (chunkTag == MKTAG('r','a','w',' ')) {
		_video_type = CPK_VCODEC_RAW;
	}

	if (_video_type == CPK_CODEC_NONE && _audio_type == CPK_CODEC_NONE) {
		close();
		return false;
	}

	/* Initialize video */
	if (_video_type != CPK_CODEC_NONE) {
		if (film_description.video_bits != 24) {
			/* Raw video not supported */
			close();
			return false;
		}

		videoHeight = FROM_BE_32(film_description.video_height);
		videoWidth = FROM_BE_32(film_description.video_width);

		debug(3, "cpk: video %dx%d", videoWidth, videoHeight );
	}

	/* Initialize video */
	if (_audio_type != CPK_CODEC_NONE) {
		debug(3, "cpk: audio %d Hz", _audio_samplerate );
	}

	/* load the sample table */
	_stream->read(&film_sampletable, sizeof(film_stab_t));
	chunkTag = FROM_BE_32(film_sampletable.tag);
	if (chunkTag != MKTAG('S','T','A','B')) {
		close();
		return false;
	}

	_base_clock = FROM_BE_32(film_sampletable.base_clock);
	_sample_count = FROM_BE_32(film_sampletable.sample_count);

	_sample_table = (film_sample *) calloc(_sample_count, sizeof(film_sample));
	if (!_sample_table) {
//		return AVERROR(ENOMEM);
		close();
		return false;
	}

	data_offset = FROM_BE_32(film_header.data_offset);
	audio_frame_counter = video_frame_counter = 0;
	for (i = 0; i < _sample_count; i++) {
		film_stabdata_t	film_stabdata;
		uint32 pts;

		/* load the next sample record and transfer it to an internal struct */
		_stream->read(&film_stabdata, sizeof(film_stabdata));
		_sample_table[i].sample_offset =
			data_offset + FROM_BE_32(film_stabdata.offset);
		_sample_table[i].sample_size = FROM_BE_32(film_stabdata.size);
		if (_sample_table[i].sample_size > INT_MAX / 4) {
			/*return AVERROR_INVALIDDATA;*/
			close();
			return false;
		}

		pts = FROM_BE_32(film_stabdata.pts);
		if (pts == 0xFFFFFFFF) {
			_sample_table[i].stream = CPK_STREAM_AUDIO;
			_sample_table[i].pts = audio_frame_counter;

			if (_audio_type == CPK_ACODEC_ADPCM_ADX)
				audio_frame_counter += (_sample_table[i].sample_size * 32 /
					(18 * _audio_channels));
			else if (_audio_type != CPK_CODEC_NONE)
				audio_frame_counter += (_sample_table[i].sample_size /
					(_audio_channels * _audio_bits / 8));
		} else {
			_sample_table[i].stream = CPK_STREAM_VIDEO;
			_sample_table[i].pts = pts & 0x7FFFFFFF;
			_sample_table[i].keyframe = (pts & 0x80000000) ? 0 : 1 /*AVINDEX_KEYFRAME*/;
			video_frame_counter++;
		}

//		debug(3, "pos 0x%08lx, stream %d, pts %d", _sample_table[i].sample_offset
//			, (int) _sample_table[i].stream, _sample_table[i].pts );
	}

	if (_audio_type != CPK_CODEC_NONE) {
		_audioTrack = new StreamAudioTrack(_audio_type, _audio_samplerate, _audio_channels,
			_audio_bits, audio_frame_counter, getSoundType() );
		addTrack(_audioTrack);
	}

	if (_video_type != CPK_CODEC_NONE) {
		_videoTrack = new StreamVideoTrack(_video_type, videoWidth, videoHeight, video_frame_counter);
		addTrack(_videoTrack);
	}

	_current_sample = 0;

	return true;
}

void CpkMovieDecoder::close() {
	Video::VideoDecoder::close();

	/*delete _stream;*/ _stream = 0;
	_videoTrack = 0;
}

// We try to at least decode 1 frame
// and also try to get at least 0.5 seconds of audio queued up
void CpkMovieDecoder::readNextPacket() {
	uint32 currentMovieTime = getTime();
	uint32 wantedAudioQueued  = currentMovieTime + 500; // always try to be 0.500 seconds in front of movie time
	uint32 frameSize = 0;
	uint32 timeStamp = 0;

	bool videoGotFrame = false;
	bool videoDone     = false;
	bool audioDone     = false;

	if (wantedAudioQueued <= _audioTrack->getTotalAudioQueued(_audio_samplerate)) {
		// already got enough audio queued up
		audioDone = true;
	}

	while (1) {
		if (_stream->eos())
			break;
		if (_current_sample>=_sample_count)
			break;

		_stream->seek(_sample_table[_current_sample].sample_offset, SEEK_SET);

		frameSize = _sample_table[_current_sample].sample_size;
		timeStamp = _sample_table[_current_sample].pts;

		switch (_sample_table[_current_sample].stream) {
			case CPK_STREAM_VIDEO:
				{
					if (!videoDone) {
						if (!videoGotFrame) {
							// We haven't decoded any frame yet, so do so now
							_videoTrack->decodeFrame(_stream->readStream(frameSize), timeStamp);

							++_current_sample;
							videoGotFrame = true;

						} else {
							// Already decoded a frame, so get timestamp of follow-up frame
							// and then we are done with video

							// Calculate next frame time
							uint32 currentFrameStartTime = _videoTrack->getNextFrameStartTime();
							uint32 nextFrameStartTime = timeStamp * 1000 / _base_clock;
							assert(currentFrameStartTime <= nextFrameStartTime);
							(void)currentFrameStartTime;
							_videoTrack->setNextFrameStartTime(nextFrameStartTime);

							// next time we want to start at the current chunk
							videoDone = true;
						}
					}
				}
				break;
			case CPK_STREAM_AUDIO:
				{
					// We are at an offset that is still relevant to audio decoding
					/*if (!audioDone)*/ {
						_audioTrack->queueAudio(_stream->readStream(frameSize), frameSize);
						uint queued = _audioTrack->getTotalAudioQueued(_audio_samplerate);

						++_current_sample;

						if (wantedAudioQueued <= queued) {
							// Got enough audio
							audioDone = true;
						}
					}
				}
				break;
		}

		if ((videoDone) /*&& (audioDone)*/) {
			return;
		}
	}
}

CpkMovieDecoder::StreamVideoTrack::StreamVideoTrack(CpkCodecId codec, uint32 width, uint32 height, uint32 frameCount) {
	_width = width;
	_height = height;
	_frameCount = frameCount;
	_curFrame = -1;
	_nextFrameStartTime = 0;

	// Create the Cinepak decoder, if we're using it
	if (codec == CPK_VCODEC_CINEPAK)
		_codec = new Image::CinepakDecoder();
	else
		error("Unsupported CPK movie video codec");
}

CpkMovieDecoder::StreamVideoTrack::~StreamVideoTrack() {
	delete _codec;
}

bool CpkMovieDecoder::StreamVideoTrack::endOfTrack() const {
	return getCurFrame() >= getFrameCount() - 1;
}

Graphics::PixelFormat CpkMovieDecoder::StreamVideoTrack::getPixelFormat() const {
	return _codec->getPixelFormat();
}

bool CpkMovieDecoder::StreamVideoTrack::setOutputPixelFormat(const Graphics::PixelFormat &format) {
	return _codec->setOutputPixelFormat(format);
}

void CpkMovieDecoder::StreamVideoTrack::decodeFrame(Common::SeekableReadStream *stream, uint32 videoTimeStamp) {
	_surface = _codec->decodeFrame(*stream);
	_curFrame++;
}

CpkMovieDecoder::StreamAudioTrack::StreamAudioTrack(CpkCodecId codec, uint32 sampleRate, uint32 channels,
	uint32 bits, uint32 frameCount, Audio::Mixer::SoundType soundType) : AudioTrack(soundType) {

	byte flags = 0;
	if (bits == 2)
		flags |= Audio::FLAG_16BITS;
	if (channels == 2)
		flags |= Audio::FLAG_STEREO;

	switch(codec) {
		case CPK_ACODEC_PCM_S8_PLANAR:
		case CPK_ACODEC_PCM_S16BE_PLANAR:
			{
				_audioStream = Audio::makePacketizedRawStream(sampleRate, flags);
			}
			break;
		default:
			{
				error("Unsupported CPK audio codec");
			}
			break;
	}
}

CpkMovieDecoder::StreamAudioTrack::~StreamAudioTrack() {
	delete _audioStream;
}

void CpkMovieDecoder::StreamAudioTrack::queueAudio(Common::SeekableReadStream *stream, uint32 size) {
	_totalAudioQueued += size;

	_audioStream->queuePacket(stream);
}

Audio::AudioStream *CpkMovieDecoder::StreamAudioTrack::getAudioStream() const {
	return _audioStream;
}

} // End of namespace Reevengi
