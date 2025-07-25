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
//#include "audio/decoders/3do.h"

#include "image/codecs/cinepak.h"

#include "engines/reevengi/movie/cpk_decoder.h"
#include "engines/reevengi/movie/movie.h"

// for Test-Code
#include "common/system.h"
#include "common/events.h"
#include "common/keyboard.h"
#include "engines/engine.h"
#include "engines/util.h"
#include "graphics/pixelformat.h"
#include "graphics/surface.h"

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
	uint32 chunkTag, videoChunkTag, data_offset;
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

		_audio_samplerate = FROM_LE_16(film_description.audio_samplerate);
		_audio_channels = film_description.audio_channels;
		_audio_bits = film_description.audio_bits;

		if ((film_description.audio_format == 2) && (_audio_channels > 0)) {
			_audio_type = CPK_ACODEC_ADPCM_ADX;
		} else if (_audio_channels > 0) {
			if (_audio_bits == 8) {
				_audio_type = CPK_ACODEC_PCM_S8_PLANAR;
			} else if (_audio_bits == 16) {
				_audio_type = CPK_ACODEC_PCM_S16BE_PLANAR;
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
		videoChunkTag = chunkTag;

		debug(3, "video %dx%d", videoWidth, videoHeight );
	}

	/* Initialize audio */
	if (_audio_type != CPK_CODEC_NONE) {
#if 0
		st = avformat_new_stream(s, NULL);
		if (!st)
			return AVERROR(ENOMEM);
		_audio_stream_index = st->index;
		st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
		st->codecpar->codec_id = _audio_type;
		st->codecpar->codec_tag = 1;
		st->codecpar->ch_layout.nb_channels = _audio_channels;
		st->codecpar->sample_rate = _audio_samplerate;

		if (_audio_type == CPK_ACODEC_ADPCM_ADX) {
			st->codecpar->bits_per_coded_sample = 18 * 8 / 32;
			st->codecpar->block_align = _audio_channels * 18;
			ffstream(st)->need_parsing = AVSTREAM_PARSE_FULL;
		} else {
			st->codecpar->bits_per_coded_sample = _audio_bits;
			st->codecpar->block_align = _audio_channels *
				st->codecpar->bits_per_coded_sample / 8;
		}

		st->codecpar->bit_rate = _audio_channels * st->codecpar->sample_rate *
			st->codecpar->bits_per_coded_sample;
#endif
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

		//debug(3, "pos 0x%08lx, stream %d", _sample_table[i].sample_offset, (int) _sample_table[i].stream );
	}

	if (_audio_type != CPK_CODEC_NONE)
		_audio_duration = audio_frame_counter;

	if (_video_type != CPK_CODEC_NONE) {
		_videoTrack = new StreamVideoTrack(videoWidth, videoHeight, videoChunkTag, video_frame_counter);
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
#if 0
	int32 chunkOffset     = 0;
	int32 dataStartOffset = 0;
	int32 nextChunkOffset = 0;
	uint32 chunkTag       = 0;
	uint32 chunkSize      = 0;

	uint32 videoSubType   = 0;
	uint32 audioSubType   = 0;
	uint32 audioBytes     = 0;
	uint32 audioTrackId   = 0;
#endif
	uint32 videoFrameSize = 0;
	uint32 videoTimeStamp = 0;

	bool videoGotFrame = false;
	bool videoDone     = false;
	bool audioDone     = false;
/*
	if (wantedAudioQueued <= _audioTracks[0]->getTotalAudioQueued()) {
		// already got enough audio queued up
		audioDone = true;
	}
*/
	while (1) {
		if (_stream->eos())
			break;

		_stream->seek(_sample_table[_current_sample].sample_offset, SEEK_SET);

		switch (_sample_table[_current_sample].stream) {
			case CPK_STREAM_VIDEO:
				{
					if (!videoDone) {
						if (!videoGotFrame) {
							// We haven't decoded any frame yet, so do so now
							//_stream->readUint32BE();
							//videoFrameSize = _stream->readUint32BE();
							videoFrameSize = _sample_table[_current_sample].sample_size;
							videoTimeStamp = _sample_table[_current_sample].pts;

							_videoTrack->decodeFrame(_stream->readStream(videoFrameSize), videoTimeStamp);

							++_current_sample;
							videoGotFrame = true;

						} else {
							// Already decoded a frame, so get timestamp of follow-up frame
							// and then we are done with video

							// Calculate next frame time
							uint32 currentFrameStartTime = _videoTrack->getNextFrameStartTime();
							uint32 nextFrameStartTime = videoTimeStamp * 1000 / _base_clock;
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
					++_current_sample;
				}
				break;
		}

		if ((videoDone) /*&& (audioDone)*/) {
			return;
		}
	}
#if 0
		switch (chunkTag) {
		case MKTAG('F','I','L','M'):
			videoTimeStamp = _stream->readUint32BE();
			_stream->skip(4); // Unknown
			videoSubType = _stream->readUint32BE();

			switch (videoSubType) {
			case MKTAG('F', 'H', 'D', 'R'):
				// Ignore video header
				break;

			case MKTAG('F', 'R', 'M', 'E'):
				// Found frame data
				if (_streamVideoOffset <= chunkOffset) {
					// We are at an offset that is still relevant to video decoding
					if (!videoDone) {
						if (!videoGotFrame) {
							// We haven't decoded any frame yet, so do so now
							_stream->readUint32BE();
							videoFrameSize = _stream->readUint32BE();
							_videoTrack->decodeFrame(_stream->readStream(videoFrameSize), videoTimeStamp);

							_streamVideoOffset = nextChunkOffset;
							videoGotFrame = true;

						} else {
							// Already decoded a frame, so get timestamp of follow-up frame
							// and then we are done with video

							// Calculate next frame time
							// 3DO clock time for movies runs at 240Hh, that's why timestamps are based on 240.
							uint32 currentFrameStartTime = _videoTrack->getNextFrameStartTime();
							uint32 nextFrameStartTime = videoTimeStamp * 1000 / 240;
							assert(currentFrameStartTime <= nextFrameStartTime);
							(void)currentFrameStartTime;
							_videoTrack->setNextFrameStartTime(nextFrameStartTime);

							// next time we want to start at the current chunk
							_streamVideoOffset = chunkOffset;
							videoDone = true;
						}
					}
				}
				break;

			default:
				error("3DO movie: Unknown subtype inside FILM packet");
				break;
			}
			break;

		case MKTAG('S','N','D','S'):
			_stream->readUint32BE();
			audioTrackId = _stream->readUint32BE();
			audioSubType = _stream->readUint32BE();

			switch (audioSubType) {
			case MKTAG('S', 'H', 'D', 'R'):
				// Ignore the audio header
				break;

			case MKTAG('S', 'S', 'M', 'P'):
				// Got audio chunk
				if (_streamAudioOffset <= chunkOffset) {
					// We are at an offset that is still relevant to audio decoding
					if (!audioDone) {
						uint queued = 0;
						audioBytes = _stream->readUint32BE();
						for (uint i = 0; i < _audioTracks.size(); i++)
							if (_audioTracks[i]->matchesId(audioTrackId)) {
								_audioTracks[i]->queueAudio(_stream, audioBytes);
								queued = _audioTracks[i]->getTotalAudioQueued();
							}

						_streamAudioOffset = nextChunkOffset;
						if (wantedAudioQueued <= queued) {
							// Got enough audio
							audioDone = true;
						}
					}
				}
				break;

			default:
				error("3DO movie: Unknown subtype inside SNDS packet");
				break;
			}
			break;

		case MKTAG('C','T','R','L'):
		case MKTAG('F','I','L','L'): // filler chunk, fills to certain boundary
		case MKTAG('D','A','C','Q'):
		case MKTAG('J','O','I','N'): // add cel data (not used in sherlock)
			// Ignore these chunks
			break;

		case MKTAG('S','H','D','R'):
			// Happens for EA logo, seems to be garbage data right at the start of the file
			break;

		default:
			error("Unknown chunk-tag '%s' inside 3DO movie", tag2str(chunkTag));
		}

		// Always seek to end of chunk
		// Sometimes not all of the chunk is filled with audio
		_stream->seek(nextChunkOffset);

		if ((videoDone) && (audioDone)) {
			return;
		}
	}
#endif
}

CpkMovieDecoder::StreamVideoTrack::StreamVideoTrack(uint32 width, uint32 height, uint32 codecTag, uint32 frameCount) {
	_width = width;
	_height = height;
	_frameCount = frameCount;
	_curFrame = -1;
	_nextFrameStartTime = 0;

	// Create the Cinepak decoder, if we're using it
	if (codecTag == MKTAG('c', 'v', 'i', 'd'))
		_codec = new Image::CinepakDecoder();
	else
		error("Unsupported CPK movie video codec tag '%s'", tag2str(codecTag));
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

CpkMovieDecoder::StreamAudioTrack::StreamAudioTrack(uint32 codecTag, uint32 sampleRate, uint32 channels, Audio::Mixer::SoundType soundType, uint32 trackId) :
		AudioTrack(soundType) {
#if 0
			switch (codecTag) {
	case MKTAG('A','D','P','4'):
	case MKTAG('S','D','X','2'):
		// ADP4 + SDX2 are both allowed
		break;

	default:
		error("Unsupported 3DO movie audio codec tag '%s'", tag2str(codecTag));
	}

	_totalAudioQueued = 0; // currently 0 milliseconds queued

	_codecTag    = codecTag;
	_sampleRate  = sampleRate;
	_trackId     = trackId;
	switch (channels) {
	case 1:
		_stereo = false;
		break;
	case 2:
		_stereo = true;
		break;
	default:
		error("Unsupported 3DO movie audio channels %d", channels);
	}

	_audioStream = Audio::makeQueuingAudioStream(sampleRate, _stereo);

	// reset audio decoder persistent spaces
	memset(&_ADP4_PersistentSpace, 0, sizeof(_ADP4_PersistentSpace));
	memset(&_SDX2_PersistentSpace, 0, sizeof(_SDX2_PersistentSpace));
#endif

		}

CpkMovieDecoder::StreamAudioTrack::~StreamAudioTrack() {
//	delete _audioStream;
//	free(_ADP4_PersistentSpace);
//	free(_SDX2_PersistentSpace);
}

//bool CpkMovieDecoder::StreamAudioTrack::matchesId(uint tid) {
//	return true; // _trackId == tid;
//}

void CpkMovieDecoder::StreamAudioTrack::queueAudio(Common::SeekableReadStream *stream, uint32 size) {
#if 0
	Common::SeekableReadStream *compressedAudioStream = 0;
	Audio::RewindableAudioStream *audioStream = 0;
	uint32 audioLengthMSecs = 0;

	// Read the specified chunk into memory
	compressedAudioStream = stream->readStream(size);

	switch(_codecTag) {
	case MKTAG('A','D','P','4'):
		audioStream = Audio::make3DO_ADP4AudioStream(compressedAudioStream, _sampleRate, _stereo, &audioLengthMSecs, DisposeAfterUse::YES, &_ADP4_PersistentSpace);
		break;
	case MKTAG('S','D','X','2'):
		audioStream = Audio::make3DO_SDX2AudioStream(compressedAudioStream, _sampleRate, _stereo, &audioLengthMSecs, DisposeAfterUse::YES, &_SDX2_PersistentSpace);
		break;
	default:
		break;
	}
	if (audioStream) {
		_totalAudioQueued += audioLengthMSecs;
		_audioStream->queueAudioStream(audioStream, DisposeAfterUse::YES);
	} else {
		// in case there was an error
		delete compressedAudioStream;
	}
#endif

}

Audio::AudioStream *CpkMovieDecoder::StreamAudioTrack::getAudioStream() const {
	return _audioStream;
}

} // End of namespace Reevengi
