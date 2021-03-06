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
#ifndef LIBDSD_META_DATA_H_INCLUDED
#define LIBDSD_META_DATA_H_INCLUDED

#include "export.h"
#include "typedefs.h"
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t sample_rate;
  uint32_t channels;
  uint32_t bits_per_sample;
  uint64_t total_samples;
  uint32_t block_bytes_per_channel;
  uint32_t block_bytes_all_channel;
  bool_t is_compress;
  bool_t is_lsb;
  uint32_t frame_rate;
  uint64_t total_frames;
} DSDStreamInfo;

DSD_API int DSD_metadata_get_title(LIBDSDHandle handle, char *str,
                                   size_t length);

DSD_API int DSD_metadata_get_artist(LIBDSDHandle handle, char *str,
                                    size_t length);

#ifdef __cplusplus
}
#endif

#endif