/* libdsd - Direct Stream Digital library
 * Copyright (C) 2017-2017  Hobson Zhu
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include "../include/libdsd.h"
#include "../libdstdec/types.h"
#include "../libdstdec/dst_init.h"
#include "../libdstdec/dst_fram.h"
#include "include/dsf_format.h"
#include "include/dsdiff_format.h"
#include "include/output.h"
#include "include/libdsd_endian.h"

#define DSD_MAX_CHANNELS 10

typedef struct _DSDDecoderPrivate {
  DSDIOCallbacks callbacks;
  DSDDecoderState state;
  void *client_data;
  uint32_t is_seeking;
  DSD_STREAM_TYPE stream_type;
  DSD_OUTPUT_TYPE output_type;
  DSDStreamInfo streamInfo;
  OutputPackage *output_package;
  uint8_t *output[DSD_MAX_CHANNELS];
  bool_t has_metadata;
  uint64_t metadata_offset;
  uint64_t target_sample;
  uint64_t remaining_sample;
  uint64_t stream_start;
  ebunch dstEbunch;
} DSDDecoderPrivate;

static DSDDecoderReadStatus _read_audio_frame_from_client(LIBDSDHandle handle, uint8_t *buffer, size_t bytes) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);

  if (decoder->_private->callbacks.eof
      && decoder->_private->callbacks.eof(decoder->_private->client_data) == DSD_DECODER_EOF_STATUS_REACH) {
    decoder->_private->state = DSD_DECODER_END_OF_STREAM;
    return DSD_DECODER_READ_STATUS_END_OF_STREAM;
  } else if (bytes > 0) {
    const DSDDecoderReadStatus status =
        decoder->_private->callbacks.read(buffer, &bytes, decoder->_private->client_data);
    if (status == DSD_DECODER_READ_STATUS_ABORT) {
      decoder->_private->state = DSD_DECODER_ABORTED;
      return DSD_DECODER_READ_STATUS_ABORT;
    } else if (bytes == 0) {
      if (status == DSD_DECODER_READ_STATUS_END_OF_STREAM ||
          (decoder->_private->callbacks.eof
              && decoder->_private->callbacks.eof(decoder->_private->client_data) == DSD_DECODER_EOF_STATUS_REACH)
          ) {
        decoder->_private->state = DSD_DECODER_END_OF_STREAM;
        return DSD_DECODER_READ_STATUS_END_OF_STREAM;
      } else
        return DSD_DECODER_READ_STATUS_CONTINUE;
    } else
      return DSD_DECODER_READ_STATUS_CONTINUE;
  } else {
    decoder->_private->state = DSD_DECODER_ABORTED;
    return DSD_DECODER_READ_STATUS_ABORT;
  }
}

static DSDDecoderWriteStatus _write_audio_frame_to_client(LIBDSDHandle handle,
                                                          const uint8_t *const *buffer,
                                                          uint32_t bytes) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  OutputPackage *output_package = decoder->_private->output_package;
  uint32_t channels = decoder->_private->streamInfo.channels;
  uint32_t buf_len = 0;
  uint32_t *out_buf = NULL;
  if (decoder->_private->output_type == DSD_STREAM_OUTPUT_DOP) {
    buf_len = bytes / 2 * channels;
    out_buf = calloc(sizeof(uint32_t), buf_len);
    OutputPackage_dop(output_package, buffer, bytes, out_buf);
  } else if (decoder->_private->output_type == DSD_STREAM_OUTPUT_NATIVE) {
    buf_len = bytes / 4 * channels;
    out_buf = calloc(sizeof(uint32_t), buf_len);
    OutputPackage_native(output_package, buffer, bytes, out_buf);
  } else {
    assert(0);
    return DSD_DECODER_WRITE_STATUS_ABORT;
  }
  DSDDecoderWriteStatus ret =
      decoder->_private->callbacks.write(out_buf, buf_len, decoder->_private->client_data);
  free(out_buf);
  return ret;
}

static bool_t _read_stream_head(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);

  if (decoder->_private->stream_type == DSD_STREAM_DSF) {
    DSDChunk chunk;
    if (_read_audio_frame_from_client(handle, (uint8_t *) &chunk, sizeof(chunk)) != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    if (memcmp(&chunk.ckID, &DSF_ID, sizeof(ID)) != 0)
      return false;

    if (chunk.ckSize != sizeof(DSDChunk))
      return false;

    decoder->_private->metadata_offset = chunk.ckPtMetadataChunk;

    if (chunk.ckPtMetadataChunk != 0) {
      decoder->_private->has_metadata = true;
    }
  } else if (decoder->_private->stream_type == DSD_STREAM_DSDIFF) {
    FormDSDChunk chunk;
    if (_read_audio_frame_from_client(handle, (uint8_t *) &chunk, sizeof(chunk)) != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    if (memcmp(&chunk.ckID, &DSDIFF_ID, sizeof(ID)) != 0)
      return false;

    if (memcmp(&chunk.formType, &FORMTYPE_ID, sizeof(ID)) != 0)
      return false;
  } else {
    decoder->_private->state = DSD_DECODER_NOT_SUPPORT;
    return false;
  }
  decoder->_private->state = DSD_DECODER_READ_STREAMINFO;
  return true;
}

static bool_t _alloc_output(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  for (int i = 0; i < DSD_MAX_CHANNELS; i++) {
    if (0 != decoder->_private->output[i]) {
      free(decoder->_private->output[i]);
      decoder->_private->output[i] = 0;
    }
  }
  int32_t channels = decoder->_private->streamInfo.channels;
  for (int i = 0; i < channels; i++) {
    uint8_t *tmp = calloc(sizeof(uint8_t), decoder->_private->streamInfo.block_bytes_per_channel);
    if (tmp == 0) {
      decoder->_private->state = DSD_DECODER_MEMORY_ALLOCATION_ERROR;
      return false;
    }
    decoder->_private->output[i] = tmp;
  }
  return true;
}

static bool_t _read_streaminfo(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);

  if (decoder->_private->stream_type == DSD_STREAM_DSF) {
    FormatChunk chunk;
    if (_read_audio_frame_from_client(handle, (uint8_t *) &chunk, sizeof(chunk)) != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    if (memcmp(&chunk.ckID, &DSF_FORMAT_ID, sizeof(ID)) != 0)
      return false;

    if (chunk.ckSize != sizeof(FormatChunk))
      return false;

    if (chunk.ckFmtVersion != 1)
      return false;

    decoder->_private->streamInfo.channels = chunk.ckChannelNum;
    decoder->_private->streamInfo.bits_per_sample = chunk.ckBitPerSample;
    decoder->_private->streamInfo.is_lsb = chunk.ckBitPerSample == 1;
    decoder->_private->streamInfo.sample_rate = chunk.ckSampleRate;
    decoder->_private->streamInfo.block_bytes_per_channel = chunk.ckBlockSizePerChannel;
    decoder->_private->streamInfo.block_bytes_all_channel = chunk.ckBlockSizePerChannel * chunk.ckChannelNum;
    decoder->_private->streamInfo.total_samples = chunk.ckSampleCount;
    decoder->_private->remaining_sample = chunk.ckSampleCount;

    DataChunk dataChunk;
    if (_read_audio_frame_from_client(handle, (uint8_t *) &dataChunk, sizeof(dataChunk))
        != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    if (memcmp(&dataChunk.ckID, &DSF_DATA_ID, sizeof(ID)) != 0)
      return false;

    if (!_alloc_output(handle))
      return false;
  } else if (decoder->_private->stream_type == DSD_STREAM_DSDIFF) {
    /**
     * The local chunks are :
     *   Sample Rate Chunk
     *   Channels Chunk
     *   Compression Type Chunk
     *   Absolute Start Time Chunk
     *   Loudspeaker Configuration Chunk
     */
    FormatVersionChunk version_chunk;
    if (_read_audio_frame_from_client(handle, (uint8_t *) &version_chunk, sizeof(version_chunk))
        != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    if (memcmp(&version_chunk.ckID, &FVER_ID, sizeof(ID)) != 0)
      return false;

    int32_t v = ENDSWAP_32(version_chunk.version);
    if (v != 0x01050000 && v != 0x01040000)
      return false;

    int64_t remaining_size;
    PropertyChunk property_chunk;
    if (_read_audio_frame_from_client(handle, (uint8_t *) &property_chunk, sizeof(property_chunk))
        != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    if (memcmp(&property_chunk.ckID, &PROP_ID, sizeof(ID)) != 0)
      return false;

    if (memcmp(&property_chunk.propType, &PROP_SND_TYPE_ID, sizeof(ID)) != 0)
      return false;

    remaining_size = (int64_t) ENDSWAP_64(property_chunk.ckDataSize) - sizeof(ID);

    SampleRateChunk sample_rate_chunk;
    if (_read_audio_frame_from_client(handle, (uint8_t *) &sample_rate_chunk, sizeof(sample_rate_chunk))
        != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    if (memcmp(&sample_rate_chunk.ckID, &PROP_FS_ID, sizeof(ID)) != 0)
      return false;

    remaining_size -= sizeof(sample_rate_chunk);
    decoder->_private->streamInfo.sample_rate = ENDSWAP_32(sample_rate_chunk.sampleRate);

    ChannelsChunk channels_chunk;
    if (_read_audio_frame_from_client(handle, (uint8_t *) &channels_chunk, sizeof(channels_chunk))
        != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    if (memcmp(&channels_chunk.ckID, &PROP_CHNL_ID, sizeof(ID)) != 0)
      return false;

    remaining_size -= sizeof(channels_chunk);
    decoder->_private->streamInfo.channels = (uint32_t) ENDSWAP_16(channels_chunk.numChannels);

    ID dummy;
    for (int i = 0; i < decoder->_private->streamInfo.channels; ++i) {
      if (_read_audio_frame_from_client(handle, (uint8_t *) &dummy, sizeof(dummy))
          != DSD_DECODER_READ_STATUS_CONTINUE)
        return false;
      remaining_size -= sizeof(dummy);
    }

    CompressionTypeChunk compression_type_chunk;
    if (_read_audio_frame_from_client(handle, (uint8_t *) &compression_type_chunk, sizeof(compression_type_chunk))
        != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    if (memcmp(&compression_type_chunk.ckID, &PROP_CMPR_ID, sizeof(ID)) != 0)
      return false;

    remaining_size -= sizeof(compression_type_chunk);
    if (memcmp(&compression_type_chunk.compressionType, &PROP_CMPR_DST_ID, sizeof(ID)) == 0)
      decoder->_private->streamInfo.is_compress = true;
    else
      decoder->_private->streamInfo.is_compress = false;

    uint8_t *dummy_1 = malloc(compression_type_chunk.count + 1);

    if (_read_audio_frame_from_client(handle, dummy_1, (uint32_t) (compression_type_chunk.count + 1))
        != DSD_DECODER_READ_STATUS_CONTINUE)
      return false;

    remaining_size -= compression_type_chunk.count + 1;
    DST_InitDecoder(&decoder->_private->dstEbunch, decoder->_private->streamInfo.channels,
                    decoder->_private->streamInfo.sample_rate / 44100);

    if (remaining_size) {
      AbsoluteStartTimeChunk absolute_start_time_chunk;
      if (_read_audio_frame_from_client(handle, (uint8_t *) &absolute_start_time_chunk,
                                        sizeof(absolute_start_time_chunk)) != DSD_DECODER_READ_STATUS_CONTINUE)
        return false;

      if (memcmp(&absolute_start_time_chunk.ckID, &PROP_ABSS_ID, sizeof(ID)) != 0)
        return false;

      remaining_size -= sizeof(absolute_start_time_chunk);
    }

    if (remaining_size) {
      LoudspeakerConfigurationChunk loud_config_chunk;
      if (_read_audio_frame_from_client(handle, (uint8_t *) &loud_config_chunk, sizeof(loud_config_chunk))
          != DSD_DECODER_READ_STATUS_CONTINUE)
        return false;

      if (memcmp(&loud_config_chunk.ckID, &PROP_LSCO_ID, sizeof(ID)) != 0)
        return false;

      remaining_size -= sizeof(loud_config_chunk);
    }

    if (remaining_size < 0) {
      assert(0);
      return false;
    }

    if (!decoder->_private->streamInfo.is_compress) {
      decoder->_private->streamInfo.bits_per_sample = 1;
      decoder->_private->streamInfo.is_lsb = false;
      decoder->_private->streamInfo.block_bytes_per_channel = 512;
      decoder->_private->streamInfo.block_bytes_all_channel =
          (uint32_t) (decoder->_private->streamInfo.block_bytes_per_channel * decoder->_private->streamInfo.channels);

      DSDSoundDataChunk dsd_data_chunk;
      if (_read_audio_frame_from_client(handle, (uint8_t *) &dsd_data_chunk, sizeof(dsd_data_chunk))
          != DSD_DECODER_READ_STATUS_CONTINUE)
        return false;

      if (memcmp(&dsd_data_chunk.ckID, &DSD_SOUND_DATA_ID, sizeof(ID)) != 0)
        return false;

      decoder->_private->streamInfo.total_samples =
          ENDSWAP_64(dsd_data_chunk.ckDataSize) / decoder->_private->streamInfo.channels * 8;
      decoder->_private->remaining_sample = decoder->_private->streamInfo.total_samples;

      if (!_alloc_output(handle))
        return false;
    } else {
      DSTSoundDataChunk dst_data_chunk;
      if (_read_audio_frame_from_client(handle, (uint8_t *) &dst_data_chunk, sizeof(dst_data_chunk))
          != DSD_DECODER_READ_STATUS_CONTINUE)
        return false;

      if (memcmp(&dst_data_chunk.ckID, &DST_SOUND_DATA_ID, sizeof(ID)) != 0)
        return false;

      DSTFrameInformationChunk frame_info_chunk;
      if (_read_audio_frame_from_client(handle, (uint8_t *) &frame_info_chunk, sizeof(frame_info_chunk))
          != DSD_DECODER_READ_STATUS_CONTINUE)
        return false;

      if (memcmp(&frame_info_chunk.ckID, &DST_FRAME_INFO_ID, sizeof(ID)) != 0)
        return false;

      decoder->_private->streamInfo.frame_rate = (uint32_t) ENDSWAP_16(frame_info_chunk.frameRate);
      decoder->_private->streamInfo.total_frames = ENDSWAP_32(frame_info_chunk.numFrames);

      decoder->_private->streamInfo.bits_per_sample = 1;
      decoder->_private->streamInfo.is_lsb = false;
      decoder->_private->streamInfo.block_bytes_per_channel =
          decoder->_private->streamInfo.sample_rate / ENDSWAP_16(frame_info_chunk.frameRate) / 8;
      decoder->_private->streamInfo.block_bytes_all_channel =
          (uint32_t) (decoder->_private->streamInfo.block_bytes_per_channel * decoder->_private->streamInfo.channels);

      decoder->_private->streamInfo.total_samples =
          decoder->_private->streamInfo.total_frames * decoder->_private->streamInfo.block_bytes_per_channel * 8;
      decoder->_private->remaining_sample = decoder->_private->streamInfo.total_samples;
      if (!_alloc_output(handle))
        return false;
    }

  } else {
    decoder->_private->state = DSD_DECODER_NOT_SUPPORT;
    return false;
  }
  if (!OutputPackage_init(decoder->_private->output_package, &decoder->_private->streamInfo)) {
    decoder->_private->state = DSD_DECODER_ABORTED;
    return false;
  }

  decoder->_private->callbacks.tell(&decoder->_private->stream_start, decoder->_private->client_data);
  decoder->_private->state = DSD_DECODER_READ_BLOCK;
  return true;
}

static bool_t _read_frame(LIBDSDHandle handle, bool_t *got) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  *got = false;

  if (decoder->_private->is_seeking)
    return true;

  int32_t channels = decoder->_private->streamInfo.channels;
  if (decoder->_private->stream_type == DSD_STREAM_DSF) {
    if (decoder->_private->remaining_sample) {
      for (int i = 0; i < channels; i++) {
        if (_read_audio_frame_from_client(handle,
                                          decoder->_private->output[i],
                                          decoder->_private->streamInfo.block_bytes_per_channel)
            != DSD_DECODER_READ_STATUS_CONTINUE)
          return false;
      }

      decoder->_private->remaining_sample -= decoder->_private->streamInfo.block_bytes_per_channel * 8;

      const uint8_t *revise_buffer[DSD_MAX_CHANNELS];

      for (int i = 0; i < channels; i++)
        revise_buffer[i] = decoder->_private->output[i] + decoder->_private->target_sample;

      if (_write_audio_frame_to_client(decoder, revise_buffer,
                                       (uint32_t) (
                                           decoder->_private->streamInfo.block_bytes_per_channel
                                               - decoder->_private->target_sample))
          != DSD_DECODER_WRITE_STATUS_CONTINUE) {
        decoder->_private->state = DSD_DECODER_ABORTED;
        return false;
      }

      decoder->_private->target_sample = 0;

      *got = true;
    } else {
      decoder->_private->state = DSD_DECODER_END_OF_STREAM;
      return true;
    }
  } else if (decoder->_private->stream_type == DSD_STREAM_DSDIFF) {
    if (!decoder->_private->streamInfo.is_compress) {
      if (decoder->_private->remaining_sample) {
        uint8_t *tmp = calloc(sizeof(uint8_t), decoder->_private->streamInfo.block_bytes_all_channel);
        if (_read_audio_frame_from_client(handle, tmp,
                                          decoder->_private->streamInfo.block_bytes_all_channel)
            != DSD_DECODER_READ_STATUS_CONTINUE)
          return false;

        for (int i = 0; i < channels; i++) {
          for (int j = 0; j < decoder->_private->streamInfo.block_bytes_per_channel; j++) {
            decoder->_private->output[i][j] = tmp[i + j * channels];
          }
        }
        free(tmp);

        decoder->_private->remaining_sample -= decoder->_private->streamInfo.block_bytes_per_channel * 8;

        const uint8_t *revise_buffer[DSD_MAX_CHANNELS];

        for (int i = 0; i < channels; i++)
          revise_buffer[i] = decoder->_private->output[i] + decoder->_private->target_sample;

        if (_write_audio_frame_to_client(decoder, revise_buffer,
                                         (uint32_t) (
                                             decoder->_private->streamInfo.block_bytes_per_channel
                                                 - decoder->_private->target_sample))
            != DSD_DECODER_WRITE_STATUS_CONTINUE) {
          decoder->_private->state = DSD_DECODER_ABORTED;
          return false;
        }

        decoder->_private->target_sample = 0;

        *got = true;
      } else {
        decoder->_private->state = DSD_DECODER_END_OF_STREAM;
        return true;
      }
    } else {
      if (decoder->_private->remaining_sample) {
        DSTFrameDataChunk dstf;
        if (_read_audio_frame_from_client(handle, (uint8_t *) &dstf, sizeof(dstf)) != DSD_DECODER_READ_STATUS_CONTINUE)
          return false;

        if (memcmp(&dstf.ckID, &DST_DSTF_ID, sizeof(ID)) != 0)
          return false;

        uint64_t dst_framesize_origin = ENDSWAP_64(dstf.ckDataSize);
        uint64_t dst_framesize = dst_framesize_origin;
        dst_framesize += dst_framesize % 2;
        uint8_t *dst_data = calloc(sizeof(uint8_t), dst_framesize);
        if (_read_audio_frame_from_client(handle, dst_data, (uint32_t) (dst_framesize))
            != DSD_DECODER_READ_STATUS_CONTINUE)
          return false;

        uint8_t *tmp = calloc(sizeof(uint8_t), decoder->_private->streamInfo.block_bytes_all_channel);
        if (DST_FramDSTDecode(dst_data, tmp, (int) dst_framesize_origin, (int) decoder->_private->streamInfo.total_frames,
                              &decoder->_private->dstEbunch))
          return false;
        free(dst_data);

        for (int i = 0; i < channels; i++) {
          for (int j = 0; j < decoder->_private->streamInfo.block_bytes_per_channel; j++) {
            decoder->_private->output[i][j] = tmp[i + j * channels];
          }
        }
        free(tmp);

        decoder->_private->remaining_sample -= decoder->_private->streamInfo.block_bytes_per_channel * 8;

        const uint8_t *revise_buffer[DSD_MAX_CHANNELS];

        for (int i = 0; i < channels; i++)
          revise_buffer[i] = decoder->_private->output[i] + decoder->_private->target_sample;

        if (_write_audio_frame_to_client(decoder, revise_buffer,
                                         (uint32_t) (
                                             decoder->_private->streamInfo.block_bytes_per_channel
                                                 - decoder->_private->target_sample))
            != DSD_DECODER_WRITE_STATUS_CONTINUE) {
          decoder->_private->state = DSD_DECODER_ABORTED;
          return false;
        }

        decoder->_private->target_sample = 0;

        *got = true;
      } else {
        decoder->_private->state = DSD_DECODER_END_OF_STREAM;
        return true;
      }
    }
  } else {
    decoder->_private->state = DSD_DECODER_NOT_SUPPORT;
    return false;
  }
  return true;
}

static bool_t _seek_to_absolute_sample(LIBDSDHandle handle, uint64_t length, uint64_t sample) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);

  if (decoder->_private->stream_type == DSD_STREAM_DSF
      || (decoder->_private->stream_type == DSD_STREAM_DSDIFF && !decoder->_private->streamInfo.is_compress)) {
    uint64_t seek_pos = 0;
    uint32_t one_block_samples = decoder->_private->streamInfo.block_bytes_per_channel * 8;
    uint32_t block_index = 0;
    uint32_t sample_offset_in_block = 0;
    const uint32_t all_channel_block_size = decoder->_private->streamInfo.block_bytes_all_channel;

    block_index = (uint32_t) (sample / one_block_samples);
    sample_offset_in_block = (uint32_t) (sample % one_block_samples);

    seek_pos = decoder->_private->stream_start + all_channel_block_size * block_index;

    if (decoder->_private->callbacks.seek(seek_pos, 0, decoder->_private->client_data) != DSD_DECODER_SEEK_STATUS_OK)
      return false;

    decoder->_private->target_sample = sample_offset_in_block / 8;
    if (decoder->_private->output_type == DSD_STREAM_OUTPUT_DOP)
      decoder->_private->target_sample -= decoder->_private->target_sample % 2;
    else if (decoder->_private->output_type == DSD_STREAM_OUTPUT_NATIVE)
      decoder->_private->target_sample -= decoder->_private->target_sample % 4;

    decoder->_private->remaining_sample = decoder->_private->streamInfo.total_samples - one_block_samples * block_index;
  } else {
    uint64_t seek_pos = 0;
    uint32_t one_block_samples = decoder->_private->streamInfo.block_bytes_per_channel * 8;
    uint32_t frame_index = 0;
    uint32_t sample_offset_in_block = 0;
    const uint32_t all_channel_block_size = decoder->_private->streamInfo.block_bytes_all_channel;

    frame_index = (uint32_t) (sample / one_block_samples);
    sample_offset_in_block = (uint32_t) (sample % one_block_samples);

    seek_pos = decoder->_private->stream_start + all_channel_block_size * frame_index;

    if (decoder->_private->callbacks.seek(seek_pos, 0, decoder->_private->client_data) != DSD_DECODER_SEEK_STATUS_OK)
      return false;

    decoder->_private->target_sample = sample_offset_in_block / 8;
    if (decoder->_private->output_type == DSD_STREAM_OUTPUT_DOP)
      decoder->_private->target_sample -= decoder->_private->target_sample % 2;
    else if (decoder->_private->output_type == DSD_STREAM_OUTPUT_NATIVE)
      decoder->_private->target_sample -= decoder->_private->target_sample % 4;

    decoder->_private->remaining_sample = decoder->_private->streamInfo.total_samples - one_block_samples * frame_index;
  }

  return true;
}

DSD_API LIBDSDHandle DSD_decoder_new() {
  DSDDecoder *decoder;

  decoder = calloc(1, sizeof(DSDDecoder));
  if (decoder == 0) {
    return 0;
  }

  decoder->_private = calloc(1, sizeof(DSDDecoderPrivate));
  if (decoder->_private == 0) {
    free(decoder);
    return 0;
  }

  decoder->_private->output_package = OutputPackage_new();

  decoder->_private->state = DSD_DECODER_UNINITIALIZED;

  return (LIBDSDHandle) decoder;
}

DSD_API void DSD_decoder_delete(LIBDSDHandle handle) {
  if (handle == NULL)
    return;
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);

  DST_CloseDecoder(&decoder->_private->dstEbunch);
  free(decoder->_private);
  free(decoder);
}

DSD_API DSDDecoderState DSD_decoder_get_state(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  return decoder->_private->state;
}

DSD_API uint64_t DSD_decoder_get_total_samples(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  return decoder->_private->streamInfo.total_samples;
}

DSD_API uint32_t DSD_decoder_get_channels(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  return decoder->_private->streamInfo.channels;
}

DSD_API uint32_t DSD_decoder_get_bits_per_sample(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  return decoder->_private->streamInfo.bits_per_sample;
}

DSD_API uint32_t DSD_decoder_get_block_size_per_channel(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  return decoder->_private->streamInfo.block_bytes_per_channel;
}

DSD_API uint32_t DSD_decoder_get_sample_rate(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  return decoder->_private->streamInfo.sample_rate;
}

DSD_API uint32_t DSD_decoder_get_decode_position(LIBDSDHandle handle,
                                                 uint64_t *position) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);
  if (decoder->_private->callbacks.tell(position, decoder->_private->client_data) != DSD_DECODER_TELL_STATUS_OK)
    return false;
  return true;
}

DSD_API DSDDecoderInitStatus DSD_decoder_init_stream(LIBDSDHandle handle, DSDIOCallbacks callbacks,
                                                     DSD_STREAM_TYPE file_type, DSD_OUTPUT_TYPE output_type,
                                                     void *client_data) {
  DSDDecoder *decoder = (DSDDecoder *) handle;
  if (decoder->_private->state != DSD_DECODER_UNINITIALIZED)
    return DSD_DECODER_INIT_STATUS_ALREADY_INITIALIZED;
  decoder->_private->client_data = client_data;
  decoder->_private->is_seeking = 0;
  decoder->_private->has_metadata = false;
  decoder->_private->callbacks = callbacks;
  decoder->_private->stream_type = file_type;
  decoder->_private->output_type = output_type;
  decoder->_private->state = DSD_DECODER_READ_STREAMHEAD;
  return DSD_DECODER_INIT_STATUS_OK;
}

DSD_API bool_t DSD_decoder_finish(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *) handle;
  assert(0 != decoder->_private);

  if (decoder->_private->state == DSD_DECODER_UNINITIALIZED)
    return true;

  decoder->_private->is_seeking = false;
  decoder->_private->state = DSD_DECODER_UNINITIALIZED;
  return true;
}

DSD_API bool_t DSD_decoder_flush(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  if (decoder->_private->state == DSD_DECODER_UNINITIALIZED)
    return false;

  decoder->_private->state = DSD_DECODER_READ_BLOCK;

  return true;
}

DSD_API bool_t DSD_decoder_reset(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  if (!DSD_decoder_flush(decoder)) {
    /* above call sets the state for us */
    return false;
  }

  decoder->_private->state = DSD_DECODER_READ_STREAMHEAD;
  return true;
}

DSD_API bool_t DSD_decoder_process_single(LIBDSDHandle handle) {
  bool_t got_a_frame;
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  while (1) {
    switch (decoder->_private->state) {
      case DSD_DECODER_READ_STREAMHEAD:
        if (!_read_stream_head(decoder))
          return false; /* above function sets the status for us */
        else
          return true;
      case DSD_DECODER_READ_STREAMINFO:
        if (!_read_streaminfo(decoder))
          return false; /* above function sets the status for us */
        else
          return true;
      case DSD_DECODER_READ_BLOCK:
        if (!_read_frame(decoder, &got_a_frame))
          return false; /* above function sets the status for us */
        if (got_a_frame)
          return true; /* above function sets the status for us */
        break;
      case DSD_DECODER_END_OF_STREAM:
      case DSD_DECODER_ABORTED:return true;
      default:assert(0);
        return false;
    }
  }
}

DSD_API bool_t
DSD_decoder_process_until_end_of_streaminfo(LIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  while (1) {
    switch (decoder->_private->state) {
      case DSD_DECODER_READ_STREAMHEAD:
        if (!_read_stream_head(decoder))
          return false; /* above function sets the status for us */
        break;
      case DSD_DECODER_READ_STREAMINFO:
        if (!_read_streaminfo(decoder))
          return false; /* above function sets the status for us */
        break;
      case DSD_DECODER_READ_BLOCK:
      case DSD_DECODER_END_OF_STREAM:
      case DSD_DECODER_ABORTED:return true;
      default:assert(0);
        return false;
    }
  }
}

DSD_API bool_t
DSD_decoder_process_until_end_of_stream(LIBDSDHandle handle) {
  int32_t dummy;
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  while (1) {
    switch (decoder->_private->state) {
      case DSD_DECODER_READ_STREAMHEAD:
        if (!_read_stream_head(decoder))
          return false; /* above function sets the status for us */
        break;
      case DSD_DECODER_READ_STREAMINFO:
        if (!_read_streaminfo(decoder))
          return false; /* above function sets the status for us */
        break;
      case DSD_DECODER_READ_BLOCK:
        if (!_read_frame(decoder, &dummy))
          return false; /* above function sets the status for us */
        break;
      case DSD_DECODER_END_OF_STREAM:
      case DSD_DECODER_ABORTED:return true;
      default:assert(0);
        return false;
    }
  }
}

DSD_API bool_t DSD_decoder_seek_absolute(LIBDSDHandle handle,
                                         uint64_t sample) {
  uint64_t length;
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  if (decoder->_private->state != DSD_DECODER_READ_STREAMHEAD &&
      decoder->_private->state != DSD_DECODER_READ_STREAMINFO &&
      decoder->_private->state != DSD_DECODER_READ_BLOCK &&
      decoder->_private->state != DSD_DECODER_END_OF_STREAM)
    return false;

  assert(decoder->_private->callbacks.seek);
  assert(decoder->_private->callbacks.tell);
  assert(decoder->_private->callbacks.length);
  assert(decoder->_private->callbacks.eof);

  if (DSD_decoder_get_total_samples(decoder) > 0 &&
      sample >= DSD_decoder_get_total_samples(decoder))
    return false;

  decoder->_private->is_seeking = true;

  if (decoder->_private->callbacks.length(&length, decoder->_private->client_data) !=
      DSD_DECODER_LENGTH_STATUS_OK) {
    decoder->_private->is_seeking = false;
    return false;
  }

  if (decoder->_private->state == DSD_DECODER_READ_STREAMINFO) {
    if (!DSD_decoder_process_until_end_of_streaminfo(decoder)) {
      decoder->_private->is_seeking = false;
      return false;
    }
    if (DSD_decoder_get_total_samples(decoder) > 0 &&
        sample >= DSD_decoder_get_total_samples(decoder)) {
      decoder->_private->is_seeking = false;
      return false;
    }
  }

  const bool_t ok = _seek_to_absolute_sample(decoder, length, sample);
  decoder->_private->is_seeking = false;
  return ok;
}
