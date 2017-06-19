/* libdsd - Direct Stream Digital library
 * Copyright (C) -  Hobson Zhu
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
 * PROFITS; OR BUSINESS int32_tERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef LIBDSD_DECODER_H_INCLUDED
#define LIBDSD_DECODER_H_INCLUDED

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  DSD_DECODER_READ_STREAMINFO,
  DSD_DECODER_READ_BLOCK,
  DSD_DECODER_END_OF_STREAM,
  DSD_DECODER_SEEK_ERROR,
  DSD_DECODER_ABORTED,
  DSD_DECODER_MEMORY_ALLOCATION_ERROR,
  DSD_DECODER_UNINITIALIZED
} DSDDecoderState;

extern FLAC_API const char *const DSDDecoderStateString[];

typedef enum {
  DSD_DECODER_INIT_STATUS_OK = 0,
  DSD_DECODER_INIT_STATUS_MEMORY_ALLOCATION_ERROR,
  DSD_DECODER_INIT_STATUS_ALREADY_INITIALIZED
} DSDDecoderInitStatus;

extern FLAC_API const char *const DSDDecoderInitStatusString[];

typedef enum {
  DSD_DECODER_READ_STATUS_CONTINUE,
  DSD_DECODER_READ_STATUS_END_OF_STREAM,
  DSD_DECODER_READ_STATUS_ABORT
} DSDDecoderReadStatus;

extern FLAC_API const char *const DSDDecoderReadStatusString[];

typedef enum {
  DSD_DECODER_SEEK_STATUS_OK,
  DSD_DECODER_SEEK_STATUS_ERROR,
  DSD_DECODER_SEEK_STATUS_UNSUPPORTED
} DSDDecoderSeekStatus;

extern FLAC_API const char *const DSDDecoderSeekStatusString[];

typedef enum {
  DSD_DECODER_TELL_STATUS_OK,
  DSD_DECODER_TELL_STATUS_ERROR,
  DSD_DECODER_TELL_STATUS_UNSUPPORTED
} DSDDecoderTellStatus;

extern FLAC_API const char *const DSDDecoderTellStatusString[];

typedef enum {
  DSD_DECODER_LENGTH_STATUS_OK,
  DSD_DECODER_LENGTH_STATUS_ERROR,
  DSD_DECODER_LENGTH_STATUS_UNSUPPORTED
} DSDDecoderLengthStatus;

extern FLAC_API const char *const DSDDecoderLengthStatusString[];

typedef enum {
  DSD_DECODER_WRITE_STATUS_CONTINUE,
  DSD_DECODER_WRITE_STATUS_ABORT
} DSDDecoderWriteStatus;

extern FLAC_API const char *const DSDDecoderWriteStatusString[];

typedef enum {
  DSD_DECODER_ERROR_STATUS_LOST_SYNC,
  DSD_DECODER_ERROR_STATUS_BAD_HEADER,
  DSD_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH,
  DSD_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM
} DSDDecoderErrorStatus;

typedef enum { DSD_FILE_DSF, DSD_FILE_DSDIFF, DSD_FILE_N } DSD_FILE_TYPE;

extern FLAC_API const char *const DSDDecoderErrorStatusString[];

struct _DSDDecoderPrivate;
typedef struct { struct _DSDDecoderPrivate *_privite; } DSDDecoder;

DSD_API LIBDSDHandle DSD_decoder_new(DSDIOCallbacks callbacks, DSD_FILE_TYPE file_type);

DSD_API void DSD_decoder_delete(LIBDSDHandle handle);

DSD_API DSDDecoderState DSD_decoder_get_state(LIBDSDHandle decoder);

DSD_API uint64_t DSD_decoder_get_total_samples(LIBDSDHandle decoder);

DSD_API uint32_t DSD_decoder_get_channels(LIBDSDHandle decoder);

DSD_API uint32_t DSD_decoder_get_bits_per_sample(LIBDSDHandle decoder);

DSD_API uint32_t DSD_decoder_get_block_size_per_channel(LIBDSDHandle decoder);

DSD_API uint32_t DSD_decoder_get_sample_rate(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_get_decode_position(LIBDSDHandle decoder,
                                                uint64_t *position);

DSD_API DSDDecoderInitStatus DSD_decoder_init_stream(LIBDSDHandle decoder,
                                                     void *client_data);

DSD_API int32_t DSD_decoder_finish(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_flush(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_reset(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_process_single(LIBDSDHandle decoder);

DSD_API int32_t
DSD_decoder_process_until_end_of_streaminfo(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_process_until_end_of_stream(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_seek_absolute(LIBDSDHandle decoder,
                                          uint64_t sample);

#ifdef __cplusplus
}
#endif

#endif