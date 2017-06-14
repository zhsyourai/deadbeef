#ifndef LIBDSD_DSDIFF_H_INCLUDED
#define LIBDSD_DSDIFF_H_INCLUDED

#include "typedefs.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************
 *           CHUNK DEFINE          *
 ***********************************/
typedef struct {
  ID ckID;                    // chunkid 'dsd '
  uint64_t ckSize;            // chunk size, in bytes
  uint64_t ckTotalfileSize;   // total filoe size, in bytes
  uint64_t ckPtMetadataChunk; // point to metadata chunk if not exist set to 0
} __attribute((packed)) DSDChunk;

typedef struct {
  ID ckID;                        // chunkid 'fmt '
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
  uint64_t ckSampleCount;         // sampling count is the num per 1 channel.
  uint32_t ckBlockSizePerChannel; // block size per channel, in bytes (fixed 4096)
  uint8_t reserved[4]
} __attribute((packed)) FormatChunk;

typedef struct {
  ID ckID;                 // chunkid 'data'
  uint64_t ckSize;         // chunk size, in bytes
  uint8_t ckSampleData[0]; // sample data array
} __attribute((packed)) DataChunk;

#ifdef __cplusplus
}
#endif

#endif