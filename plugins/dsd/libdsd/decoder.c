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

#include "decoder.h"

FLAC_API const char *const DSDDecoderStateString[] = {
    "DSD_DECODER_SEARCH_FOR_METADATA",
    "DSD_DECODER_READ_METADATA",
    "DSD_DECODER_SEARCH_FOR_FRAME_SYNC",
    "DSD_DECODER_READ_FRAME",
    "DSD_DECODER_END_OF_STREAM",
    "DSD_DECODER_OGG_ERROR",
    "DSD_DECODER_SEEK_ERROR",
    "DSD_DECODER_ABORTED",
    "DSD_DECODER_MEMORY_ALLOCATION_ERROR",
    "DSD_DECODER_UNINITIALIZED"};

FLAC_API const char *const DSDDecoderInitStatusString[] = {
    "DSD_DECODER_INIT_STATUS_OK",
    "DSD_DECODER_INIT_STATUS_UNSUPPORTED_CONTAINER",
    "DSD_DECODER_INIT_STATUS_INVALID_CALLBACKS",
    "DSD_DECODER_INIT_STATUS_MEMORY_ALLOCATION_ERROR",
    "DSD_DECODER_INIT_STATUS_ERROR_OPENING_FILE",
    "DSD_DECODER_INIT_STATUS_ALREADY_INITIALIZED"};

FLAC_API const char *const DSDDecoderReadStatusString[] = {
    "DSD_DECODER_READ_STATUS_CONTINUE", "DSD_DECODER_READ_STATUS_END_OF_STREAM",
    "DSD_DECODER_READ_STATUS_ABORT"};

FLAC_API const char *const DSDDecoderSeekStatusString[] = {
    "DSD_DECODER_SEEK_STATUS_OK", "DSD_DECODER_SEEK_STATUS_ERROR",
    "DSD_DECODER_SEEK_STATUS_UNSUPPORTED"};

FLAC_API const char *const DSDDecoderTellStatusString[] = {
    "DSD_DECODER_TELL_STATUS_OK", "DSD_DECODER_TELL_STATUS_ERROR",
    "DSD_DECODER_TELL_STATUS_UNSUPPORTED"};

FLAC_API const char *const DSDDecoderLengthStatusString[] = {
    "DSD_DECODER_LENGTH_STATUS_OK", "DSD_DECODER_LENGTH_STATUS_ERROR",
    "DSD_DECODER_LENGTH_STATUS_UNSUPPORTED"};

FLAC_API const char *const DSDDecoderWriteStatusString[] = {
    "DSD_DECODER_WRITE_STATUS_CONTINUE", "DSD_DECODER_WRITE_STATUS_ABORT"};

FLAC_API const char *const DSDDecoderErrorStatusString[] = {
    "DSD_DECODER_ERROR_STATUS_LOST_SYNC", "DSD_DECODER_ERROR_STATUS_BAD_HEADER",
    "DSD_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH",
    "DSD_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM"};

typedef struct _DSDDecoderPrivate {
  DSDIOCallbacks callbacks;
  DSDDecoderState state;
  void *client_data;
  uint64_t samples_decoded;
  uint32_t is_seeking;
  DSD_FILE_TYPE file_type;
} DSDDecoderPrivate;

DSD_API LIBDSDHandle DSD_decoder_new(DSDIOCallbacks callbacks, DSD_FILE_TYPE file_type) {
  DSDDecoder *decoder;

  decoder = calloc(1, sizeof(DSDDecoder));
  if (decoder == 0) {
    return 0;
  }

  decoder->_privite = calloc(1, sizeof(DSDDecoderPrivate));
  if (decoder->_privite == 0) {
    free(coder);
    return 0;
  }

  decoder->_privite->callbacks = callbacks;
  decoder->_privite->file_type = file_type;

  return (LIBDSDHandle)coder;
}

DSD_API void DSD_decoder_delete(LIBDSDHandle handle) {
  if (handle == NULL)
    return;
  DSDDecoder *decoder = (DSDDecoder *)handle;
  assert(0 != decoder->_privite);

  free(decoder->_privite);
  free(decoder);
}

DSD_API DSDDecoderState DSD_decoder_get_state(DSDLIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *)handle;
  assert(0 != decoder->_private);
  return decoder->state;
}

DSD_API uint64_t DSD_decoder_get_total_samples(DSDLIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *)handle;
  assert(0 != decoder->_private);
  return decoder->state;
}

DSD_API uint32_t DSD_decoder_get_channels(DSDLIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *)handle;
  assert(0 != decoder->_private);
  return decoder->state;
}

DSD_API uint32_t DSD_decoder_get_bits_per_sample(DSDLIBDSDHandle handle) {}

DSD_API uint32_t DSD_decoder_get_block_size_per_channel(LIBDSDHandle decoder) {}

DSD_API uint32_t DSD_decoder_get_sample_rate(DSDLIBDSDHandle handle) {}

DSD_API int32_t DSD_decoder_get_decode_position(DSDLIBDSDHandle handle,
                                                uint64_t *position) {}

DSD_API DSDDecoderInitStatus DSD_decoder_init_stream(DSDLIBDSDHandle handle,
                                                     void *client_data) {
  DSDDecoder *decoder = (DSDDecoder *)handle;
  decoder->_private->decoder.client_data = client_data;
  decoder->_private->decoder.is_seeking = 0;
  decoder->_private->decoder.samples_decoded = 0;

  return DSD_DECODER_INIT_STATUS_OK;
}

DSD_API int32_t DSD_decoder_finish(DSDLIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = (DSDDecoder *)handle;
  assert(0 != decoder->_private);

  if (decoder->_private->state == DSD_DECODER_UNINITIALIZED)
    return true;

  decoder->_private->is_seeking = false;
  decoder->_private->state = DSD_DECODER_UNINITIALIZED;
  return true;
}

DSD_API int32_t DSD_decoder_flush(DSDLIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  if (decoder->_private->state == DSD_DECODER_UNINITIALIZED)
    return false;

  decoder->_private->samples_decoded = 0;

  decoder->_private->state = DSD_DECODER_READ_BLOCK;

  return true;
}

DSD_API int32_t DSD_decoder_reset(DSDLIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  if (!DSD_decoder_flush(decoder)) {
    /* above call sets the state for us */
    return false;
  }

  decoder->_private->state = DSD_DECODER_READ_STREAMINFO;
  return true;
}

DSD_API int32_t DSD_decoder_process_single(DSDLIBDSDHandle handle) {
  int32_t got_a_frame;
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  while (1) {
    switch (decoder->_private->state) {
    case DSD_DECODER_READ_STREAMINFO:
      if (!_read_streaminfo(decoder))
        return false; /* above function sets the status for us */
      else
        return true;
    case DSD_DECODER_READ_BLOCK:
      if (!_read_block(decoder, &got_a_frame))
        return false; /* above function sets the status for us */
      if (got_a_frame)
        return true; /* above function sets the status for us */
      break;
    case DSD_DECODER_END_OF_STREAM:
    case DSD_DECODER_ABORTED:
      return true;
    default:
      assert(0);
      return false;
    }
  }
}

DSD_API int32_t
DSD_decoder_process_until_end_of_streaminfo(DSDLIBDSDHandle handle) {
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  while (1) {
    switch (decoder->_private->state) {
    case DSD_DECODER_READ_STREAMINFO:
      if (!_read_streaminfo(decoder))
        return false; /* above function sets the status for us */
      break;
    case DSD_DECODER_READ_BLOCK:
    case DSD_DECODER_END_OF_STREAM:
    case DSD_DECODER_ABORTED:
      return true;
    default:
      assert(0);
      return false;
    }
  }
}

DSD_API int32_t
DSD_decoder_process_until_end_of_stream(DSDLIBDSDHandle handle) {
  int32_t dummy;
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  while (1) {
    switch (decoder->_private->state) {
    case DSD_DECODER_READ_STREAMINFO:
      if (!_read_streaminfo(decoder))
        return false; /* above function sets the status for us */
      break;
    case DSD_DECODER_READ_BLOCK:
      if (!_read_block(decoder, &dummy))
        return false; /* above function sets the status for us */
      break;
    case DSD_DECODER_END_OF_STREAM:
    case DSD_DECODER_ABORTED:
      return true;
    default:
      assert(0);
      return false;
    }
  }
}

DSD_API int32_t DSD_decoder_seek_absolute(DSDLIBDSDHandle handle,
                                          uint64_t sample) {
  uint64_t length;
  assert(0 != handle);
  DSDDecoder *decoder = handle;
  assert(0 != decoder->_private);

  if (decoder->state != DSD_DECODER_READ_STREAMINFO &&
      decoder->state != DSD_DECODER_READ_BLOCK &&
      decoder->state != DSD_DECODER_END_OF_STREAM)
    return false;

  assert(decoder->_private->callbacks->seek_callback);
  assert(decoder->_private->callbacks->tell_callback);
  assert(decoder->_private->callbacks->length_callback);
  assert(decoder->_private->callbacks->eof_callback);

  if (DSD_decoder_get_total_samples(decoder) > 0 &&
      sample >= DSD_decoder_get_total_samples(decoder))
    return false;

  decoder->is_seeking = true;

  /* get the file length (currently our algorithm needs to know the length so
   * it's also an error to get DSD_DECODER_LENGTH_STATUS_UNSUPPORTED)
   */
  if (decoder->length_callback(decoder, &length, decoder->client_data) !=
      DSD_DECODER_LENGTH_STATUS_OK) {
    decoder->is_seeking = false;
    return false;
  }

  /* if we haven't finished processing the metadata yet, do that so we have the
   * STREAMINFO, SEEK_TABLE, and first_frame_offset */
  if (decoder->state == DSD_DECODER_READ_STREAMINFO) {
    if (!DSD_decoder_process_until_end_of_streaminfo(decoder)) {
      /* above call sets the state for us */
      decoder->is_seeking = false;
      return false;
    }
    /* check this again in case we didn't know total_samples the first time */
    if (DSD_decoder_get_total_samples(decoder) > 0 &&
        sample >= DSD_decoder_get_total_samples(decoder)) {
      decoder->is_seeking = false;
      return false;
    }
  }

  {
    const FLAC__bool ok = seek_to_absolute_sample_(decoder, length, sample);
    decoder->is_seeking = false;
    return ok;
  }
}

int32_t _read_streaminfo() {}

int32_t _read_block() {}