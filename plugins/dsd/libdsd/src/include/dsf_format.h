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

#ifndef LIBDSD_DSF_H_INCLUDED
#define LIBDSD_DSF_H_INCLUDED

#include <stddef.h>
#include "../../include/typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************
 *           CHUNK DEFINE          *
 ***********************************/
typedef struct {
  ID ckID;                    // chunk id 'DSD '
  uint64_t ckSize;            // chunk size, in bytes
  uint64_t ckTotalFileSize;   // total filo size, in bytes
  uint64_t ckPtMetadataChunk; // point to metadata chunk if not exist set to 0
} __attribute((packed)) DSDChunk;

const ID DSF_ID = {'D', 'S', 'D', ' '};

typedef struct {
  ID ckID;                        // chunk id 'fmt '
  uint64_t ckSize;                // chunk size, in bytes
  uint32_t ckFmtVersion;          // version of this file format
  uint32_t ckFmtID;               // this file format ID. 0: DSD raw
  uint32_t ckChannelType;         // channel type
                                  // 1: mono
                                  // 2: stereo
                                  // 3: 3 channels
                                  // 4: quad
                                  // 5: 4 channels
                                  // 6: 5 channels
                                  // 7: 5.1 channels
  uint32_t ckChannelNum;          // channel number
  uint32_t ckSampleRate;          // sampling frequency, in Hz
  uint32_t ckBitPerSample;        // bits per sample 1: LSB, 8: MSB
  uint64_t ckSampleCount;         // sampling count is the num per channel.
  uint32_t ckBlockSizePerChannel; // block size per channel, in bytes (fixed 4096)
  uint8_t reserved[4];            // reserved 4 bytes
} __attribute((packed)) FormatChunk;

const ID DSF_FORMAT_ID = {'f', 'm', 't', ' '};

typedef struct {
  ID ckID;                 // chunk id 'data'
  uint64_t ckSize;         // chunk size, in bytes
  uint8_t ckSampleData[0]; // sample data array
} __attribute((packed)) DataChunk;

const ID DSF_DATA_ID = {'d', 'a', 't', 'a'};

#ifdef __cplusplus
}
#endif

#endif