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

#ifndef LIBDSD_DSDIFF_H_INCLUDED
#define LIBDSD_DSDIFF_H_INCLUDED

#include "../../include/typedefs.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************
 *           CHUNK DEFINE          *
 ***********************************/

#define CHUNK_ID "FRM8"

#define CHUNK_FVER "FVER"
#define CHUNK_PROD "PROD"
#define CHUNK_DSD "DSD "
#define CHUNK_DST "DST "
#define CHUNK_DSTI "DSTI"
#define CHUNK_COMT "COMT"
#define CHUNK_DIIN "DIIN"
#define CHUNK_MANF "MANF"

typedef struct {
  ID ckID;             // chunk id
  uint64_t ckDataSize; // chunk data size, in bytes (not include ckID and this
                       // field)
  uint8_t ckData[0];   // data
} __attribute((packed)) Chunk;

/***********************************
 *         FORM DSD CHUNK          *
 ***********************************/

typedef struct {
  ID ckID;             // 'FRM8'
  uint64_t ckDataSize; // FORM's data size, in bytes (not include ckID and this
                       // field)
  ID formType;         // 'DSD '
  Chunk frm8Chunks[0]; // chunks
} __attribute((packed)) FormDSDChunk;

const ID DSDIFF_ID = {'F', 'R', 'M', '8'};
const ID FORMTYPE_ID = {'D', 'S', 'D', ' '};


/***********************************
 *      FORMAT VERSION CHUNK       *
 ***********************************/

typedef struct {
  ID ckID;             // 'FVER'
  uint64_t ckDataSize; // 4
  uint32_t version;    // 0x01050000 version 1.5.0.0 DSDIFF
} __attribute((packed)) FormatVersionChunk;

const ID FVER_ID = {'F', 'V', 'E', 'R'};

/***********************************
 *         PROPERTY CHUNK          *
 ***********************************/

typedef struct {
  ID ckID;             // 'PROP'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  ID propType;         // 'SND '
  Chunk propChunks[0]; // local chunks
} __attribute((packed)) PropertyChunk;

const ID PROP_ID = {'P', 'R', 'O', 'P'};

const ID PROP_SND_TYPE_ID = {'S', 'N', 'D', ' '};

typedef struct {
  ID ckID;             // 'FS '
  uint64_t ckDataSize; // 4
  uint32_t sampleRate; // sample rate in [Hz]
} __attribute((packed)) SampleRateChunk;

const ID PROP_FS_ID = {'F', 'S', ' ', ' '};


#define PREDEF_CHID_SLFT "SLFT"
#define PREDEF_CHID_SRGT "SRGT"
#define PREDEF_CHID_MLFT "MLFT"
#define PREDEF_CHID_MRGT "MRGT"
#define PREDEF_CHID_LS "LS "
#define PREDEF_CHID_RS "RS "
#define PREDEF_CHID_C "C "
#define PREDEF_CHID_LFE "LFE "

typedef struct {
  ID ckID;              // 'CHNL'
  uint64_t ckDataSize;  // data size, in bytes (not include ckID and this field)
  uint16_t numChannels; // number of audio channels
  ID chID[0];           // channels ID's
} __attribute((packed)) ChannelsChunk;

const ID PROP_CHNL_ID = {'C', 'H', 'N', 'L'};

typedef struct {
  ID ckID;             // 'CMPR'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  ID compressionType;  // compression ID code
  uint8_t count;       // length of the compression name
  uint8_t compressionName[0]; // human readable type name
} __attribute((packed)) CompressionTypeChunk;

const ID PROP_CMPR_ID = {'C', 'M', 'P', 'R'};
const ID PROP_CMPR_DSD_ID = {'D', 'S', 'D', ' '};
const ID PROP_CMPR_DST_ID = {'D', 'S', 'T', ' '};

typedef struct {
  ID ckID;             // 'ABSS'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  uint16_t hours;      // hours
  uint8_t minutes;     // minutes
  uint8_t seconds;     // seconds
  uint32_t samples;    // samples
} __attribute((packed)) AbsoluteStartTimeChunk;

const ID PROP_ABSS_ID = {'A', 'B', 'S', 'S'};

typedef struct {
  ID ckID;             // 'LSCO'
  uint64_t ckDataSize; // 2
  uint16_t lsConfig;   // loudspeaker configuration
} __attribute((packed)) LoudspeakerConfigurationChunk;

const ID PROP_LSCO_ID = {'L', 'S', 'C', 'O'};

/***********************************
 *      DSD SOUND DATA CHUNK       *
 ***********************************/

typedef struct {
  ID ckID;             // 'DSD '
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  uint8_t DSDsoundData[0]; // (interleaved) DSD data
} __attribute((packed)) DSDSoundDataChunk;

const ID DSD_SOUND_DATA_ID = {'D', 'S', 'D', ' '};

/***********************************
 *      DST SOUND DATA CHUNK       *
 ***********************************/

typedef struct {
  ID ckID;             // 'DST '
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  Chunk chunks[0];  // container
} __attribute((packed)) DSTSoundDataChunk;

const ID DST_SOUND_DATA_ID = {'D', 'S', 'T', ' '};

typedef struct {
  ID ckID;             // 'FRTE'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  uint32_t numFrames;  // number of DST frames.
  uint16_t frameRate;  // DST frame rate per second
} __attribute((packed)) DSTFrameInformationChunk;

const ID DST_FRAME_INFO_ID = {'F', 'R', 'T', 'E'};

typedef struct {
  ID ckID;             // 'DSTF'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  uint8_t soundData[0]; // The DST data for one frame
} __attribute((packed)) DSTFrameDataChunk;

const ID DST_DSTF_ID = {'D', 'S', 'T', 'F'};

typedef struct {
  ID ckID; // 'DSTC'
  uint64_t ckDataSize;
  uint8_t crcData[0]; // the value of the CRC
} __attribute((packed)) DSTFrameCrcChunk;

const ID DST_DSTC_ID = {'D', 'S', 'T', 'C'};

/***********************************
 *      DST SOUND INDEX CHUNK      *
 ***********************************/
typedef struct {
  uint64_t offset; // offset in the file [in bytes] of the sound in the DST
  // Sound Data Chunk
  uint32_t length; // length of the sound in bytes
} __attribute((packed)) DSTFrameIndex;

typedef struct {
  ID ckID;             // 'DSTI'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  DSTFrameIndex indexData[0]; // array of index structs
} __attribute((packed)) DSTSoundIndexChunk;

/***********************************
 *         COMMENTS CHUNK          *
 ***********************************/
typedef struct {
  uint16_t timeStampYear;   // creation year
  uint8_t timeStampMonth;   // creation month
  uint8_t timeStampDay;     // creation day
  uint8_t timeStampHour;    // creation hour
  uint8_t timeStampMinutes; // creation minutes
  uint16_t cmtType;         // comment type
  uint16_t cmtRef;          // comment reference
  uint32_t count;           // string length
  int8_t commentText[0];    // text
} __attribute((packed)) Comment;

typedef struct {
  ID ckID;              // 'COMT'
  uint64_t ckDataSize;  // data size, in bytes (not include ckID and this field)
  uint16_t numComments; // number of comments
  Comment comments[0];  // the concatenated comments
} __attribute((packed)) CommentsChunk;

/***********************************
 * EDITED MASTER INFORMATION CHUNK *
 ***********************************/

typedef struct {
  ID ckID;             // 'DIIN'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  Chunk EmChunks[0];   // container
} __attribute((packed)) EditedMasterInformationChunk;

typedef struct {
  ID ckID;             // 'EMID'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  int8_t emid[0];      // unique sequence of bytes
} __attribute((packed)) EditedMasterIDChunk;

typedef struct {
  ID ckID;              // 'MARK'
  uint64_t ckDataSize;  // data size, in bytes (not include ckID and this field)
  uint16_t hours;       // marker position in hours
  uint8_t minutes;      // marker position in minutes
  uint8_t seconds;      // marker position in seconds
  uint32_t samples;     // marker position in samples
  int32_t offset;       // marker offset in samples
  uint16_t markType;    // type of marker
  uint16_t markChannel; // channel reference
  uint16_t TrackFlags;  // special purpose flags
  uint32_t count;       // string length
  int8_t markerText[0]; // description
} __attribute((packed)) MarkerChunk;

typedef struct {
  ID ckID;              // 'DIAR'
  uint64_t ckDataSize;  // data size, in bytes (not include ckID and this field)
  uint32_t count;       // string length
  int8_t artistText[0]; // description
} __attribute((packed)) ArtistChunk;

typedef struct {
  ID ckID;             // 'DITI'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  uint32_t count;      // string length
  int8_t titleText[0]; // description
} __attribute((packed)) TitleChunk;

/***********************************
 *   MANUFACTURER SPECIFIC CHUNK   *
 ***********************************/

typedef struct {
  ID ckID;             // 'MANF'
  uint64_t ckDataSize; // data size, in bytes (not include ckID and this field)
  ID manID;            // unique manufacturer ID [4 characters]
  uint8_t manData[0];  // manufacturer specific data
} __attribute((packed)) ManufacturerSpecificChunk;

#ifdef __cplusplus
}
#endif

#endif