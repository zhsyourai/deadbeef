/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    - Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    - Neither the name of the DeaDBeeF Player nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "../../deadbeef.h"
#include "../../strdupa.h"
#include "libdsd/include/libdsd.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt, ...)

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#define BUFFERSIZE 100000

typedef struct {
  DB_fileinfo_t info;
  LIBDSDHandle *decoder;
  char *buffer;
  int remaining; // bytes remaining in buffer from last read
  int64_t startsample;
  int64_t endsample;
  int64_t currentsample;
  int64_t totalsamples;
  DB_FILE *file;

  // used only on insert
  ddb_playlist_t *plt;
  DB_playItem_t *after;
  DB_playItem_t *last;
  DB_playItem_t *it;
  const char *fname;
  int bitrate;
  int set_bitrate;
} dsd_info_t;

// callbacks
DSDDecoderReadStatus dsd_read_cb(void *ptr, size_t *bytes, void *client_data) {
  dsd_info_t *info = (dsd_info_t *)client_data;
  size_t r = deadbeef->fread(ptr, 1, *bytes, info->file);
  *bytes = r;
  if (r == 0) {
    return DSD_DECODER_READ_STATUS_END_OF_STREAM;
  }
  return DSD_DECODER_READ_STATUS_CONTINUE;
}

DSDDecoderSeekStatus dsd_seek_cb(uint64_t offset, int whence,
                                 void *client_data) {
  dsd_info_t *info = (dsd_info_t *)client_data;
  int r = deadbeef->fseek(info->file, offset, SEEK_SET);
  if (r) {
    return DSD_DECODER_SEEK_STATUS_ERROR;
  }
  return DSD_DECODER_SEEK_STATUS_OK;
}

DSDDecoderTellStatus dsd_tell_cb(uint64_t *position, void *client_data) {
  dsd_info_t *info = (dsd_info_t *)client_data;
  size_t r = deadbeef->ftell(info->file);
  *position = r;
  return DSD_DECODER_TELL_STATUS_OK;
}

DSDDecoderLengthStatus dsd_length_cb(uint64_t *length, void *client_data) {
  dsd_info_t *info = (dsd_info_t *)client_data;
  size_t pos = deadbeef->ftell(info->file);
  deadbeef->fseek(info->file, 0, SEEK_END);
  *length = deadbeef->ftell(info->file);
  deadbeef->fseek(info->file, pos, SEEK_SET);
  return DSD_DECODER_LENGTH_STATUS_OK;
}

DSDDecoderEOFStatus dsd_eof_cb(void *client_data) {
  dsd_info_t *info = (dsd_info_t *)client_data;
  int64_t pos = deadbeef->ftell(info->file);
  return pos == deadbeef->fgetlength(info->file)
             ? DSD_DECODER_EOF_STATUS_REACH
             : DSD_DECODER_EOF_STATUS_NOT_REACH;
}

static DSDDecoderWriteStatus
cdsd_write_callback(const void *buffer, uint32_t buf_len, void *client_data) {
  dsd_info_t *info = (dsd_info_t *)client_data;
  DB_fileinfo_t *_info = &info->info;

  int channels = _info->fmt.channels;
  int bufsize = BUFFERSIZE - info->remaining;
  int bufsamples = channels * bufsize / _info->fmt.bps / 8;
  int nsamples = min(bufsamples, buf_len);

  char *bufptr = info->buffer + info->remaining;

  int readbytes = buf_len;

  unsigned bps = DSD_decoder_get_bits_per_sample(info->decoder);

  if (bps == 24) {
    for (int i = 0; i < nsamples; i++) {
      int32_t sample = ((uint32_t*)buffer)[i];
      *bufptr++ = sample & 0xff;
      *bufptr++ = (sample & 0xff00) >> 8;
      *bufptr++ = (sample & 0xff0000) >> 16;
    }
  } else if (bps == 32) {
    trace("DSD: output\n");
    for (int i = 0; i < nsamples; i++) {
      int32_t sample = ((uint32_t*)buffer)[i];
      sample <<= 8;
      *((int32_t *)bufptr) = sample;
      bufptr += 4;
    }
  }

  info->remaining = (int)(bufptr - info->buffer);

  if (readbytes > bufsize) {
    trace("flac: buffer overflow, distortion will occur\n");
    //    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }
  return DSD_DECODER_WRITE_STATUS_CONTINUE;
}

static dsd_info_t *cdsd_open_int(uint32_t hints) {
  dsd_info_t *info = calloc(1, sizeof(dsd_info_t));
  if (info && hints & DDB_DECODER_HINT_NEED_BITRATE) {
    info->set_bitrate = 1;
  }
  return info;
}

static DB_fileinfo_t *cdsd_open(uint32_t hints) {
  return (DB_fileinfo_t *)cdsd_open_int(hints);
}

static DB_fileinfo_t *cdsd_open2(uint32_t hints, DB_playItem_t *it) {
  dsd_info_t *info = cdsd_open_int(hints);
  if (!info) {
    return NULL;
  }

  deadbeef->pl_lock();
  info->file = deadbeef->fopen(deadbeef->pl_find_meta(it, ":URI"));
  if (!info->file) {
    trace("cdsd_open2 failed to open file %s\n",
          deadbeef->pl_find_meta(it, ":URI"));
  }
  deadbeef->pl_unlock();

  return (DB_fileinfo_t *)info;
}

static int cdsd_init(DB_fileinfo_t *_info, DB_playItem_t *it) {
  trace("cdsd_init %s\n", deadbeef->pl_find_meta(it, ":URI"));
  dsd_info_t *info = (dsd_info_t *)_info;

  if (!info->file) {
    deadbeef->pl_lock();
    info->file = deadbeef->fopen(deadbeef->pl_find_meta(it, ":URI"));
    deadbeef->pl_unlock();
    if (!info->file) {
      trace("cdsd_init failed to open file %s\n",
            deadbeef->pl_find_meta(it, ":URI"));
      return -1;
    }
  }

  deadbeef->pl_lock();
  const char *uri = deadbeef->pl_find_meta(it, ":URI");
  const char *ext = strrchr(uri, '.');
  if (ext) {
    ext++;
  }
  deadbeef->pl_unlock();

  int skip = 0;
  int dsf = 1;
  int dsdiff = 1;
  if (ext && (!strcasecmp(ext, "dsf") || !strcasecmp(ext, "dff"))) {
    skip = deadbeef->junk_get_leading_size(info->file);
    if (skip > 0) {
      deadbeef->fseek(info->file, skip, SEEK_SET);
    }
    char sign[4];
    if (deadbeef->fread(sign, 1, 4, info->file) != 4) {
      trace("cdsd_init failed to read signature\n");
      return -1;
    }
    dsf = strncmp(sign, "DSD ", 4);
    dsdiff = strncmp(sign, "FRM8", 4);
    if (dsf && dsdiff) {
      trace("cdsd_init bad signature\n");
      return -1;
    }
    deadbeef->fseek(info->file, -4, SEEK_CUR);
  }

  DSDDecoderInitStatus status;
  info->decoder = DSD_decoder_new();
  if (!info->decoder) {
    trace("DSD_decoder_new failed\n");
    return -1;
  }

  DSDIOCallbacks callbacks = {
      .read = dsd_read_cb,
      .seek = dsd_seek_cb,
      .tell = dsd_tell_cb,
      .length = dsd_length_cb,
      .eof = dsd_eof_cb,
      .write = cdsd_write_callback,
  };

  status = DSD_decoder_init_stream(info->decoder, callbacks,
                                   dsf ? DSD_STREAM_DSDIFF : DSD_STREAM_DSF,
                                   DSD_STREAM_OUTPUT_DOP, info);
  if (status != DSD_DECODER_INIT_STATUS_OK) {
    trace("cdsd_init bad decoder status\n");
    return -1;
  }

  if (!DSD_decoder_process_until_end_of_streaminfo(info->decoder)) {
    trace("cdsd_init metadata failed\n");
    return -1;
  }

  info->totalsamples = DSD_decoder_get_total_samples(info->decoder);
  _info->fmt.samplerate = DSD_decoder_get_sample_rate(info->decoder) / 16;
  _info->fmt.channels = DSD_decoder_get_channels(info->decoder);
  _info->fmt.bps = DSD_decoder_get_bits_per_sample(info->decoder);
  for (int i = 0; i < _info->fmt.channels; i++) {
    _info->fmt.channelmask |= 1 << i;
  }

  _info->plugin = &plugin;
  _info->readpos = 0;

  if (_info->fmt.samplerate <= 0) {
    fprintf(stderr, "corrupted/invalid dsd stream\n");
    return -1;
  }
  info->bitrate = deadbeef->pl_find_meta_int(it, ":BITRATE", -1);

  deadbeef->pl_lock();
  {
    const char *channelmask =
        deadbeef->pl_find_meta(it, "WAVEFORMAT_EXTENSIBLE_CHANNELMASK");
    if (channelmask) {
      uint32_t cm = 0;
      if (1 == sscanf(channelmask, "0x%X", &cm)) {
        _info->fmt.channelmask = cm;
      }
    }
  }
  deadbeef->pl_unlock();

  info->buffer = malloc(BUFFERSIZE);
  info->remaining = 0;
  int64_t endsample = deadbeef->pl_item_get_endsample(it);
  if (endsample > 0) {
    info->startsample = deadbeef->pl_item_get_startsample(it);
    info->endsample = endsample;
    if (plugin.seek_sample(_info, 0) < 0) {
      trace("cdsd_init failed to seek to sample 0\n");
      return -1;
    }
    trace("flac(cue): startsample=%d, endsample=%d, totalsamples=%d, "
          "currentsample=%d\n",
          info->startsample, info->endsample, info->totalsamples,
          info->currentsample);
  } else {
    info->startsample = 0;
    info->endsample = info->totalsamples - 1;
    info->currentsample = 0;
    trace("flac: startsample=%d, endsample=%d, totalsamples=%d\n",
          info->startsample, info->endsample, info->totalsamples);
  }

  return 0;
}

static void cdsd_free(DB_fileinfo_t *_info) {
  if (_info) {
    dsd_info_t *info = (dsd_info_t *)_info;
    if (info->decoder) {
      DSD_decoder_delete(info->decoder);
    }
    if (info->buffer) {
      free(info->buffer);
    }
    if (info->file) {
      deadbeef->fclose(info->file);
    }
    free(_info);
  }
}

static int cdsd_read(DB_fileinfo_t *_info, char *bytes, int size) {
  dsd_info_t *info = (dsd_info_t *)_info;
  if (info->set_bitrate &&
      info->bitrate != deadbeef->streamer_get_apx_bitrate()) {
    deadbeef->streamer_set_bitrate(info->bitrate);
  }

  int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
  if (info->endsample >= 0) {
    if (size / samplesize + info->currentsample > info->endsample) {
      size = (int)(info->endsample - info->currentsample + 1) * samplesize;
      trace("size truncated to %d bytes, cursample=%d, endsample=%d\n", size,
            info->currentsample, info->endsample);
      if (size <= 0) {
        return 0;
      }
    }
  }
  int initsize = size;
  do {
    if (info->remaining) {
      int sz = min(size, info->remaining);
      memcpy(bytes, info->buffer, sz);

      size -= sz;
      bytes += sz;
      if (sz < info->remaining) {
        memmove(info->buffer, &info->buffer[sz], info->remaining - sz);
      }
      info->remaining -= sz;
      int n = sz / samplesize;
      info->currentsample += sz / samplesize;
      _info->readpos += (float)n / _info->fmt.samplerate;
    }
    if (!size) {
      break;
    }
    if (!DSD_decoder_process_single(info->decoder)) {
      trace("DSD_decoder_process_single error\n");
      break;
    }
    if (DSD_decoder_get_state(info->decoder) == DSD_DECODER_END_OF_STREAM) {
      trace("DSD_decoder_get_state error\n");
      break;
    }
  } while (size > 0);

  return initsize - size;
}

static int cdsd_seek_sample(DB_fileinfo_t *_info, int sample) {
  dsd_info_t *info = (dsd_info_t *)_info;
  sample += info->startsample;
  info->currentsample = sample;
  info->remaining = 0;
  if (!DSD_decoder_seek_absolute(info->decoder, (uint64_t)(sample))) {
    return -1;
  }
  _info->readpos = (float)(sample - info->startsample) / _info->fmt.samplerate;
  return 0;
}

static int cdsd_seek(DB_fileinfo_t *_info, float time) {
  return cdsd_seek_sample(_info, time * _info->fmt.samplerate);
}

static void cdsd_free_temp(DB_fileinfo_t *_info) {
  if (_info) {
    dsd_info_t *info = (dsd_info_t *)_info;
    if (info->decoder) {
      DSD_decoder_delete(info->decoder);
    }
    if (info->buffer) {
      free(info->buffer);
    }
    if (info->file) {
      deadbeef->fclose(info->file);
    }
  }
}

static DB_playItem_t *cdsd_insert(ddb_playlist_t *plt, DB_playItem_t *after,
                                  const char *fname) {
  trace("flac: inserting %s\n", fname);
  DB_playItem_t *it = NULL;
  LIBDSDHandle decoder = NULL;
  dsd_info_t info;
  memset(&info, 0, sizeof(info));
  DB_fileinfo_t *_info = &info.info;
  info.fname = fname;
  info.after = after;
  info.last = after;
  info.plt = plt;
  info.file = deadbeef->fopen(fname);
  if (!info.file) {
    goto cdsd_insert_fail;
  }

  const char *ext = fname + strlen(fname);
  while (ext > fname && *ext != '/' && *ext != '.') {
    ext--;
  }
  if (*ext == '.') {
    ext++;
  } else {
    ext = NULL;
  }

  int skip = 0;
  int dsf = 1;
  int dsdiff = 1;
  if (ext && (!strcasecmp(ext, "dsf") || !strcasecmp(ext, "dff"))) {
    skip = deadbeef->junk_get_leading_size(info.file);
    if (skip > 0) {
      deadbeef->fseek(info.file, skip, SEEK_SET);
    }
    char sign[4];
    if (deadbeef->fread(sign, 1, 4, info.file) != 4) {
      trace("cdsd_init failed to read signature\n");
      goto cdsd_insert_fail;
    }
    dsf = strncmp(sign, "DSD ", 4);
    dsdiff = strncmp(sign, "FRM8", 4);
    if (dsf && dsdiff) {
      trace("cdsd_init bad signature\n");
      goto cdsd_insert_fail;
    }
    deadbeef->fseek(info.file, -4, SEEK_CUR);
  }

  DSDDecoderInitStatus status;
  decoder = DSD_decoder_new();
  if (!decoder) {
    trace("DSD_decoder_new failed\n");
    goto cdsd_insert_fail;
  }

  it = deadbeef->pl_item_alloc_init(fname, plugin.plugin.id);
  info.it = it;
  if (skip > 0) {
    deadbeef->fseek(info.file, skip, SEEK_SET);
  } else {
    deadbeef->rewind(info.file);
  }
  deadbeef->fseek(info.file, -4, SEEK_CUR);

  DSDIOCallbacks callbacks = {
      .read = dsd_read_cb,
      .seek = dsd_seek_cb,
      .tell = dsd_tell_cb,
      .length = dsd_length_cb,
      .eof = dsd_eof_cb,
  };

  status = DSD_decoder_init_stream(decoder, callbacks,
                                   dsf ? DSD_STREAM_DSDIFF : DSD_STREAM_DSF,
                                   DSD_STREAM_OUTPUT_DOP, &info);
  if (status != DSD_DECODER_INIT_STATUS_OK) {
    trace("cdsd_init bad decoder status\n");
    goto cdsd_insert_fail;
  }

  if (!DSD_decoder_process_until_end_of_streaminfo(decoder)) {
    trace("cdsd_init metadata failed\n");
    goto cdsd_insert_fail;
  }

  _info->fmt.samplerate = DSD_decoder_get_sample_rate(decoder) / 16;
  _info->fmt.channels = DSD_decoder_get_channels(decoder);
  _info->fmt.bps = DSD_decoder_get_bits_per_sample(decoder);
  info.totalsamples = DSD_decoder_get_total_samples(decoder);
  if (info.totalsamples > 0) {
    deadbeef->plt_set_item_duration(
        info.plt, it,
        info.totalsamples / (float)DSD_decoder_get_sample_rate(decoder));
  } else {
    deadbeef->plt_set_item_duration(info.plt, it, -1);
  }

  if (info.info.fmt.samplerate <= 0) {
    goto cdsd_insert_fail;
  }
  int64_t fsize = deadbeef->fgetlength(info.file);
  int is_streaming = info.file->vfs->is_streaming();

  deadbeef->pl_add_meta(it, ":FILETYPE", dsf ? "DSF" : "DSDIFF");

  char s[100];
  snprintf(s, sizeof(s), "%lld", fsize);
  deadbeef->pl_add_meta(it, ":FILE_SIZE", s);
  snprintf(s, sizeof(s), "%d", info.info.fmt.channels);
  deadbeef->pl_add_meta(it, ":CHANNELS", s);
  snprintf(s, sizeof(s), "%d", info.info.fmt.bps);
  deadbeef->pl_add_meta(it, ":BPS", s);
  snprintf(s, sizeof(s), "%d", info.info.fmt.samplerate);
  deadbeef->pl_add_meta(it, ":SAMPLERATE", s);
  if (deadbeef->pl_get_item_duration(it) > 0) {
    uint64_t position;
    if (DSD_decoder_get_decode_position(decoder, &position))
      fsize -= position;

    deadbeef->pl_set_meta_int(
        it, ":BITRATE",
        (int)roundf(fsize / deadbeef->pl_get_item_duration(it) * 8 / 1000));
  }
  DSD_decoder_delete(decoder);
  decoder = NULL;

  deadbeef->fclose(info.file);
  info.file = NULL;

  after = deadbeef->plt_insert_item(plt, after, it);
  deadbeef->pl_item_unref(it);
  cdsd_free_temp(_info);
  return after;
cdsd_insert_fail:
  if (it) {
    deadbeef->pl_item_unref(it);
  }
  cdsd_free_temp(_info);
  return NULL;
}

static const char *exts[] = {"dsf", "dff", NULL};

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 7,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stddsd",
    .plugin.name = "DSD decoder",
    .plugin.descr = "DSD decoder using libdsd",
    .plugin.copyright =
        "Copyright (C) 2009-2013 Alexey Yakovenko et al.\n"
        "Uses libFLAC (C) Copyright (C) "
        "2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson\n"
        "Uses libogg Copyright (c) 2002, Xiph.org Foundation\n"
        "\n"
        "Redistribution and use in source and binary forms, with or without\n"
        "modification, are permitted provided that the following conditions\n"
        "are met:\n"
        "\n"
        "- Redistributions of source code must retain the above copyright\n"
        "notice, this list of conditions and the following disclaimer.\n"
        "\n"
        "- Redistributions in binary form must reproduce the above copyright\n"
        "notice, this list of conditions and the following disclaimer in the\n"
        "documentation and/or other materials provided with the distribution.\n"
        "\n"
        "- Neither the name of the DeaDBeeF Player nor the names of its\n"
        "contributors may be used to endorse or promote products derived from\n"
        "this software without specific prior written permission.\n"
        "\n"
        "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
        "``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
        "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS "
        "FOR\n"
        "A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE "
        "FOUNDATION OR\n"
        "CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, "
        "SPECIAL,\n"
        "EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n"
        "PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n"
        "PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY "
        "OF\n"
        "LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n"
        "NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n"
        "SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n",
    .plugin.website = "http://deadbeef.sf.net",
    .open = cdsd_open,
    .open2 = cdsd_open2,
    .init = cdsd_init,
    .free = cdsd_free,
    .read = cdsd_read,
    .seek = cdsd_seek,
    .seek_sample = cdsd_seek_sample,
    .insert = cdsd_insert,
    .exts = exts,
};

DB_plugin_t *dsd_load(DB_functions_t *api) {
  deadbeef = api;
  return DB_PLUGIN(&plugin);
}
