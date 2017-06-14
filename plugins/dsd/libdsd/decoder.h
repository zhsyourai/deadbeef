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

DSD_API DSSDecoderState DSD_decoder_get_state(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_get_md_checking(LIBDSDHandle decoder);

DSD_API uint64_t DSD_decoder_get_total_samples(LIBDSDHandle decoder);

DSD_API uint32_t DSD_decoder_get_channels(LIBDSDHandle decoder);

DSD_API uint32_t DSD_decoder_get_bits_per_sample(LIBDSDHandle decoder);

DSD_API uint32_t DSD_decoder_get_sample_rate(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_get_decode_position(LIBDSDHandle decoder, uint64_t *position);


DSD_API LIBDSDHandleInitStatus DSD_decoder_init_stream(LIBDSDHandle decoder, void *client_data);

DSD_API int32_t DSD_decoder_finish(LIBDSDHandle decoder);
  
DSD_API int32_t DSD_decoder_flush(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_reset(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_process_single(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_process_until_end_of_streaminfo(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_process_until_end_of_stream(LIBDSDHandle decoder);

DSD_API int32_t DSD_decoder_seek_absolute(LIBDSDHandle decoder, uint64_t sample);

#ifdef __cplusplus
}
#endif

#endif