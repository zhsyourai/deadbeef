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
 
#ifndef LIBDSDCALLBACKS_H_INCLUDED
#define LIBDSDCALLBACKS_H_INCLUDED

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    DSD_DECODER_READ_STATUS_CONTINUE,
    DSD_DECODER_READ_STATUS_END_OF_STREAM,
    DSD_DECODER_READ_STATUS_ABORT
} DSDDecoderReadStatus;

typedef enum {
    DSD_DECODER_SEEK_STATUS_OK,
    DSD_DECODER_SEEK_STATUS_ERROR,
    DSD_DECODER_SEEK_STATUS_UNSUPPORTED
} DSDDecoderSeekStatus;

typedef enum {
    DSD_DECODER_TELL_STATUS_OK,
    DSD_DECODER_TELL_STATUS_ERROR,
    DSD_DECODER_TELL_STATUS_UNSUPPORTED
} DSDDecoderTellStatus;

typedef enum {
    DSD_DECODER_LENGTH_STATUS_OK,
    DSD_DECODER_LENGTH_STATUS_ERROR,
    DSD_DECODER_LENGTH_STATUS_UNSUPPORTED
} DSDDecoderLengthStatus;

typedef enum {
    DSD_DECODER_WRITE_STATUS_CONTINUE,
    DSD_DECODER_WRITE_STATUS_ABORT
} DSDDecoderWriteStatus;

typedef enum {
    DSD_DECODER_ERROR_STATUS_LOST_SYNC,
    DSD_DECODER_ERROR_STATUS_BAD_HEADER,
    DSD_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH,
    DSD_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM
} DSDDecoderErrorStatus;

typedef enum {
    DSD_DECODER_EOF_STATUS_REACH,
    DSD_DECODER_EOF_STATUS_NOT_REACH,
    DSD_DECODER_EOF_STATUS_UNSUPPORTED
} DSDDecoderEOFStatus;

typedef DSDDecoderReadStatus (*DSDIOCallback_Read) (void *ptr, size_t *size, void* client_data);

typedef DSDDecoderWriteStatus (*DSDIOCallback_Write) (const void *buffer, uint32_t sample_count, void* client_data);

typedef DSDDecoderSeekStatus (*DSDIOCallback_Seek) (uint64_t offset, int whence, void* client_data);

typedef DSDDecoderTellStatus (*DSDIOCallback_Tell) (uint64_t *position, void* client_data);

typedef DSDDecoderEOFStatus (*DSDIOCallback_Eof) (void* client_data);

typedef DSDDecoderLengthStatus (*DSDIOCallback_Length)(uint64_t *length, void *client_data);

typedef struct {
    DSDIOCallback_Read read;
    DSDIOCallback_Write write;
    DSDIOCallback_Seek seek;
    DSDIOCallback_Tell tell;
    DSDIOCallback_Eof eof;
    DSDIOCallback_Length length;
} DSDIOCallbacks;

#ifdef __cplusplus
}
#endif

#endif