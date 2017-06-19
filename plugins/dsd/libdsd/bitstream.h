/* libdsd - Direct Stream Digital listreamary
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

#ifndef LIBDSD_BIT_STREAM_H_INCLUDED
#define LIBDSD_BIT_STREAM_H_INCLUDED

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BitStream;
typedef struct BitStream BitStream;

typedef int32_t (*BitStreamReadCallback)(char buffer[], size_t *bytes,
                                         void *client_data);
typedef int32_t (*BitStreamWriteCallback)(const char buffer[], size_t bytes,
                                          void *client_data);

BitStream *BitStream_new(void);

void BitStream_delete(BitStream *stream);

bool_t BitStream_init(BitStream *stream, BitStreamReadCallback read_callback,
                       BitStreamWriteCallback write_callback, bool_t is_bigendian,
                       void *client_data);

void BitStream_free(BitStream *stream);

bool_t BitStream_clear(BitStream *stream);

bool_t BitStream_is_consumed_byte_aligned(const BitStream *stream);

uint32_t BitStream_bits_left_for_byte_alignment(const BitStream *stream);

uint32_t BitStream_get_input_bits_unconsumed(const BitStream *stream);

bool_t BitStream_read_raw(BitStream *stream, uint8_t *val, uint32_t bits);

bool_t BitStream_read_uint32(BitStream *stream, uint32_t *val);

bool_t BitStream_read_int32(BitStream *stream, int32_t *val);

bool_t BitStream_read_uint64(BitStream *stream, uint64_t *val);

bool_t BitStream_read_int64(BitStream *stream, uint64_t *val);

bool_t BitStream_skip_bits(BitStream *stream, uint32_t bits);

#ifdef __cplusplus
}
#endif

#endif