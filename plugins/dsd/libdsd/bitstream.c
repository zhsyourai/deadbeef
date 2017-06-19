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

#include "bitstream.h"
#include <stdlib.h>
#include <string.h>

static const unsigned BITSTREAM_DEFAULT_CAPACITY = 65536u / 8; /* in words */

struct BitStream {
  uint8_t *buffer;
  uint32_t capacity; // in bytes
  uint32_t bytes;
  uint32_t consumed_bytes;
  uint32_t consumed_bits;
  BitStreamReadCallback read_callback;
  BitStreamWriteCallback write_callback;
  bool_t is_bigendian;
  void *client_data;
};

static int32_t _bitreader_read_from_callback(BitStream *stream) {
  uint8_t start, end;
  size_t bytes;
  uint8_t *target;

  if (stream->consumed_bytes > 0) {
    start = stream->consumed_bytes;
    end = stream->bytes;
    memmove(stream->buffer, stream->buffer + start, (end - start));

    stream->bytes -= start;
    stream->consumed_bytes = 0;
  }

  bytes = stream->capacity - stream->bytes;
  if (bytes == 0)
    return false;
  target = ((uint8_t *)(stream->buffer + stream->bytes));

  if (!stream->read_callback(target, &bytes, stream->client_data))
    return false;

  stream->bytes += (uint32_t)bytes;
  return true;
}

BitStream *BitStream_new(void) {
  BitStream *stream = calloc(1, sizeof(BitStream));
  return stream;
}

void BitStream_delete(BitStream *stream) {
  assert(0 != stream);

  BitStream_free(stream);
  free(stream);
}

bool_t BitStream_init(BitStream *stream, BitStreamReadCallback read_callback,
                      BitStreamWriteCallback write_callback,
                      bool_t is_bigendian, void *client_data) {
  assert(0 != stream);
  assert(DSD_BITS_PER_WORD >= 32);

  stream->bytes = 0;
  stream->consumed_bytes = stream->consumed_bits = 0;
  stream->capacity = BITSTREAM_DEFAULT_CAPACITY;
  stream->buffer = malloc(stream->capacity);
  if (stream->buffer == 0)
    return false;
  stream->read_callback = read_callback;
  stream->write_callback = write_callback;
  stream->client_data = client_data;
  stream->is_bigendian = is_bigendian;
  return true;
}

void BitStream_free(BitStream *stream) {
  assert(0 != stream);

  if (0 != stream->buffer)
    free(stream->buffer);
  stream->buffer = 0;
  stream->capacity = 0;
  stream->bytes = 0;
  stream->consumed_bytes = stream->consumed_bits = 0;
  stream->read_callback = 0;
  stream->write_callback = 0;
  stream->client_data = 0;
}

bool_t BitStream_clear(BitStream *stream) {
  stream->bytes = 0;
  stream->consumed_bytes = stream->consumed_bits = 0;
  return true;
}

inline int32_t BitStream_is_consumed_byte_aligned(const BitStream *stream) {
  return ((stream->consumed_bits & 7) == 0);
}

inline uint32_t
BitStream_bits_left_for_byte_alignment(const BitStream *stream) {
  return 8 - (stream->consumed_bits & 7);
}

inline uint32_t BitStream_get_input_bits_unconsumed(const BitStream *stream) {
  return (stream->bytes - stream->consumed_bytes) * 8 - stream->consumed_bits;
}

bool_t BitStream_read_raw(BitStream *stream, uint8_t *val, uint32_t bits) {
  assert(0 != stream);
  assert(0 != stream->buffer);
  assert((stream->capacity * 8) >= bits);
  assert(stream->consumed_bytes <= stream->bytes);
  if (bits == 0) {
    *val = 0;
    return true;
  }

  while ((stream->bytes - stream->consumed_bytes) * 8 - stream->consumed_bits <
         bits) {
    if (!_bitreader_read_from_callback(stream))
      return false;
  }

  uint32_t loop_bits = 0;
  while (bits) {
    if (bits / 8) {
      loop_bits = 8;
      bits -= 8;
    } else {
      loop_bits = bits;
      bits = 0;
    }
    if (stream->consumed_bytes < stream->bytes) {
      const uint8_t byte = stream->buffer[stream->consumed_bytes];
      if (stream->consumed_bits) {
        const uint32_t n = 8 - stream->consumed_bits;
        if (loop_bits < n) {
          *val =
              (uint8_t)((byte & (DSD_BYTE_ALL_ONES >> stream->consumed_bits)) >>
                        (n - loop_bits));
          stream->consumed_bits += loop_bits;
          return true;
        }
        *val = (uint8_t)(byte & (DSD_BYTE_ALL_ONES >> stream->consumed_bits));
        loop_bits -= n;
        stream->consumed_bytes++;
        stream->consumed_bits = 0;
        if (0 < loop_bits && loop_bits <= n) {
          *val <<= loop_bits;
          *val |= (uint8_t)(stream->buffer[stream->consumed_bytes] >>
                            (8 - loop_bits));
          stream->consumed_bits = loop_bits;
        }
        continue;
      } else {
        if (loop_bits < 8) {
          *val = (uint8_t)(byte >> (8 - loop_bits));
          stream->consumed_bits = loop_bits;
          return true;
        }
        *val = (uint8_t)byte;
        stream->consumed_bytes++;
        continue;
      }
    } else {
      if (stream->consumed_bits) {
        assert(stream->consumed_bits + loop_bits <= 8);
        *val = (uint8_t)((stream->buffer[stream->consumed_bytes] &
                          (DSD_BYTE_ALL_ONES >> stream->consumed_bits)) >>
                         (8 - stream->consumed_bits - loop_bits));
        stream->consumed_bits += loop_bits;
        return true;
      } else {
        *val = (uint8_t)(stream->buffer[stream->consumed_bytes] >>
                         (8 - loop_bits));
        stream->consumed_bits += loop_bits;
        return true;
      }
    }
  }
}

bool_t BitStream_read_uint32(BitStream *stream, uint32_t *val) {
  uint32_t uval;
  if (!BitStream_read_raw(stream, &uval, 32))
    return false;
  *val = uval;
  return true;
}

bool_t BitStream_read_int32(BitStream *stream, int32_t *val, uint32_t bits) {
  uint32_t uval;
  if (!BitStream_read_raw(stream, &uval, 32))
    return false;
  *val = uval;
  return true;
}

bool_t BitStream_read_uint64(BitStream *stream, uint64_t *val) {
  uint32_t hi, lo;
  if (!BitStream_read_raw(stream, &hi, 32))
    return false;
  if (!BitStream_read_raw(stream, &lo, 32))
    return false;
  *val = hi;
  *val <<= 32;
  *val |= lo;
  return true;
}

bool_t BitStream_read_int64(BitStream *stream, int64_t *val) {
  uint32_t hi, lo;
  if (!BitStream_read_raw(stream, &hi, 32))
    return false;
  if (!BitStream_read_raw(stream, &lo, 32))
    return false;
  *val = hi;
  *val <<= 32;
  *val |= lo;
  return true;
}

bool_t BitStream_skip_bits(BitStream *stream, uint32_t bits) {
  assert(0 != stream);
  assert(0 != stream->buffer);

  if (bits > 0) {
    const uint32_t n = stream->consumed_bits & 7;
    uint32_t m;
    uint32_t x;

    if (n != 0) {
      m = dsd_min(8 - n, bits);
      if (!BitStream_read_raw(stream, &x, m))
        return false;
      bits -= m;
    }
    m = bits / 32;
    while (m--) {
      if (!BitStream_read_raw(stream, &x, 32))
        return false;
      bits -= 32;
    }
    bits %= 32;
    if (bits > 0) {
      if (!BitStream_read_raw(stream, &x, bits))
        return false;
    }
  }

  return true;
}