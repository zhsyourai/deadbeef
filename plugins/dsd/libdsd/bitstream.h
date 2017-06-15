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

#ifndef LIBDSD_BIT_STREAM_H_INCLUDED
#define LIBDSD_BIT_STREAM_H_INCLUDED

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BitStream;
typedef struct BitStream BitStream;

typedef int32_t (*BitStreamReadCallback)(char buffer[], size_t *bytes, void *client_data);
typedef int32_t (*BitStreamWriteCallback)(const char buffer[], size_t bytes, void *client_data);

BitStream *BitStream_new(void);
void BitStream_delete(BitStream *br);
int32_t BitStream_init(BitStream *br, BitStreamReadCallback read_cb, BitStreamWriteCallback write_cb, void *client_data);
void BitStream_free(BitStream *br);
int32_t BitStream_clear(BitStream *br);

int32_t BitStream_read_raw_uint32(BitStream *br, uint32_t *val, unsigned bits);
int32_t BitStream_read_raw_int32(BitStream *br, int32_t *val, unsigned bits);
int32_t BitStream_read_raw_uint64(BitStream *br, uint64_t *val, unsigned bits);
int32_t BitStream_read_uint32_little_endian(BitStream *br, uint32_t *val); 
int32_t BitStream_skip_bits(BitStream *br, unsigned bits);
int32_t BitStream_skip_byte_block_aligned(BitStream *br, unsigned nvals);
int32_t BitStream_read_byte_block_aligned(BitStream *br, char *val, unsigned nvals);
int32_t BitStream_read_unary_unsigned(BitStream *br, unsigned *val);
int32_t BitStream_read_rice_signed(BitStream *br, int *val, unsigned parameter);
int32_t BitStream_read_rice_signed_block(BitStream *br, int vals[], unsigned nvals, unsigned parameter);

#ifdef __cplusplus
}
#endif

#endif