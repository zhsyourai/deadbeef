#include "bitstream.h"

static FLAC__bool _bitreader_read_from_callback(BitStream *stream) {
  unsigned start, end;
  size_t bytes;
  FLAC__byte *target;

  /* first shift the unconsumed buffer data toward the front as much as possible
   */
  if (stream->consumed_words > 0) {
    start = stream->consumed_words;
    end = stream->words + (stream->bytes ? 1 : 0);
    memmove(stream->buffer, stream->buffer + start,
            FLAC__BYTES_PER_WORD * (end - start));

    stream->words -= start;
    stream->consumed_words = 0;
  }

  /*
   * set the target for reading, taking into account word alignment and
   * endianness
   */
  bytes =
      (stream->capacity - stream->words) * FLAC__BYTES_PER_WORD - stream->bytes;
  if (bytes == 0)
    return false; /* no space left, buffer is too small; see note for
                     BitStream_DEFAULT_CAPACITY  */
  target = ((FLAC__byte *)(stream->buffer + stream->words)) + stream->bytes;

/* before reading, if the existing reader looks like this (say streamword is 32
 * bits
 * wide)
 *   bitstream :  11 22 33 44 55            stream->words=1 stream->bytes=1
 * (partial
 * tail word is left-justified)
 *   buffer[BE]:  11 22 33 44 55 ?? ?? ??   (shown layed out as bytes
 * sequentially in memory)
 *   buffer[LE]:  44 33 22 11 ?? ?? ?? 55   (?? being don't-care)
 *                               ^^-------target, bytes=3
 * on LE machines, have to byteswap the odd tail word so nothing is
 * overwritten:
 */
#if WORDS_BIGENDIAN
#else
  if (stream->bytes)
    stream->buffer[stream->words] =
        SWAP_BE_WORD_TO_HOST(stream->buffer[stream->words]);
#endif

  /* now it looks like:
   *   bitstream :  11 22 33 44 55            stream->words=1 stream->bytes=1
   *   buffer[BE]:  11 22 33 44 55 ?? ?? ??
   *   buffer[LE]:  44 33 22 11 55 ?? ?? ??
   *                               ^^-------target, bytes=3
   */

  /* read in the data; note that the callback may return a smaller number of
   * bytes */
  if (!stream->read_callback(target, &bytes, stream->client_data))
    return false;

/* after reading bytes 66 77 88 99 AA BB CC DD EE FF from the client:
 *   bitstream :  11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
 *   buffer[BE]:  11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF ??
 *   buffer[LE]:  44 33 22 11 55 66 77 88 99 AA BB CC DD EE FF ??
 * now have to byteswap on LE machines:
 */
#if WORDS_BIGENDIAN
#else
  end = (stream->words * FLAC__BYTES_PER_WORD + stream->bytes +
         (unsigned)bytes + (FLAC__BYTES_PER_WORD - 1)) /
        FLAC__BYTES_PER_WORD;
  for (start = stream->words; start < end; start++)
    stream->buffer[start] = SWAP_BE_WORD_TO_HOST(stream->buffer[start]);
#endif

  /* now it looks like:
   *   bitstream :  11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
   *   buffer[BE]:  11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF ??
   *   buffer[LE]:  44 33 22 11 88 77 66 55 CC BB AA 99 ?? FF EE DD
   * finally we'll update the reader values:
   */
  end = stream->words * FLAC__BYTES_PER_WORD + stream->bytes + (unsigned)bytes;
  stream->words = end / FLAC__BYTES_PER_WORD;
  stream->bytes = end % FLAC__BYTES_PER_WORD;

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

FLAC__bool BitStream_init(BitStream *stream,
                          BitStreamReadCallback read_callback,
                          BitStreamWriteCallback write_callback,
                          void *client_data) {
  assert(0 != stream);

  stream->words = stream->bytes = 0;
  stream->consumed_words = stream->consumed_bits = 0;
  stream->capacity = BitStream_DEFAULT_CAPACITY;
  stream->buffer = malloc(sizeof(streamword) * stream->capacity);
  if (stream->buffer == 0)
    return false;
  stream->read_callback = rcb;
  stream->client_data = cd;

  return true;
}

void BitStream_free(BitStream *stream) {
  assert(0 != stream);

  if (0 != stream->buffer)
    free(stream->buffer);
  stream->buffer = 0;
  stream->capacity = 0;
  stream->words = stream->bytes = 0;
  stream->consumed_words = stream->consumed_bits = 0;
  stream->read_callback = 0;
  stream->client_data = 0;
}

FLAC__bool BitStream_clear(BitStream *stream) {
  stream->words = stream->bytes = 0;
  stream->consumed_words = stream->consumed_bits = 0;
  return true;
}

inline FLAC__bool BitStream_is_consumed_byte_aligned(const BitStream *stream) {
  return ((stream->consumed_bits & 7) == 0);
}

inline unsigned
BitStream_bits_left_for_byte_alignment(const BitStream *stream) {
  return 8 - (stream->consumed_bits & 7);
}

inline unsigned BitStream_get_input_bits_unconsumed(const BitStream *stream) {
  return (stream->words - stream->consumed_words) * FLAC__BITS_PER_WORD +
         stream->bytes * 8 - stream->consumed_bits;
}

FLAC__bool BitStream_read_raw_uint32(BitStream *stream, uint32_t *val,
                                     unsigned bits) {
  FLAC__ASSERT(0 != stream);
  FLAC__ASSERT(0 != stream->buffer);

  FLAC__ASSERT(bits <= 32);
  FLAC__ASSERT((stream->capacity * FLAC__BITS_PER_WORD) * 2 >= bits);
  FLAC__ASSERT(stream->consumed_words <= stream->words);

  /* WATCHOUT: code does not work with <32bit words; we can make things much
   * faster with this assertion */
  FLAC__ASSERT(FLAC__BITS_PER_WORD >= 32);

  if (bits == 0) { /* OPT: investigate if this can ever happen, maybe change to
                      assertion */
    *val = 0;
    return true;
  }

  while ((stream->words - stream->consumed_words) * FLAC__BITS_PER_WORD +
             stream->bytes * 8 - stream->consumed_bits <
         bits) {
    if (!_bitreader_read_from_callback(stream))
      return false;
  }
  if (stream->consumed_words <
      stream->words) { /* if we've not consumed up to a partial tail word... */
    /* OPT: taking out the consumed_bits==0 "else" case below might make things
     * faster if less code allows the compiler to inline this function */
    if (stream->consumed_bits) {
      /* this also works when consumed_bits==0, it's just a little slower than
       * necessary for that case */
      const unsigned n = FLAC__BITS_PER_WORD - stream->consumed_bits;
      const brword word = stream->buffer[stream->consumed_words];
      if (bits < n) {
        *val = (uint32_t)(
            (word & (FLAC__WORD_ALL_ONES >> stream->consumed_bits)) >>
            (n - bits)); /* The result has <= 32 non-zero bits */
        stream->consumed_bits += bits;
        return true;
      }
      /* (FLAC__BITS_PER_WORD - stream->consumed_bits <= bits) ==>
       * (FLAC__WORD_ALL_ONES >> stream->consumed_bits) has no more than 'bits'
       * non-zero bits */
      *val = (uint32_t)(word & (FLAC__WORD_ALL_ONES >> stream->consumed_bits));
      bits -= n;
      crc16_update_word_(stream, word);
      stream->consumed_words++;
      stream->consumed_bits = 0;
      if (bits) { /* if there are still bits left to read, there have to be less
                     than 32 so they will all be in the next word */
        *val <<= bits;
        *val |= (uint32_t)(stream->buffer[stream->consumed_words] >>
                           (FLAC__BITS_PER_WORD - bits));
        stream->consumed_bits = bits;
      }
      return true;
    } else { /* stream->consumed_bits == 0 */
      const brword word = stream->buffer[stream->consumed_words];
      if (bits < FLAC__BITS_PER_WORD) {
        *val = (uint32_t)(word >> (FLAC__BITS_PER_WORD - bits));
        stream->consumed_bits = bits;
        return true;
      }
      /* at this point bits == FLAC__BITS_PER_WORD == 32; because of previous
       * assertions, it can't be larger */
      *val = (uint32_t)word;
      crc16_update_word_(stream, word);
      stream->consumed_words++;
      return true;
    }
  } else {
    /* in this case we're starting our read at a partial tail word;
     * the reader has guaranteed that we have at least 'bits' bits
     * available to read, which makes this case simpler.
     */
    /* OPT: taking out the consumed_bits==0 "else" case below might make things
     * faster if less code allows the compiler to inline this function */
    if (stream->consumed_bits) {
      /* this also works when consumed_bits==0, it's just a little slower than
       * necessary for that case */
      FLAC__ASSERT(stream->consumed_bits + bits <= stream->bytes * 8);
      *val = (uint32_t)((stream->buffer[stream->consumed_words] &
                         (FLAC__WORD_ALL_ONES >> stream->consumed_bits)) >>
                        (FLAC__BITS_PER_WORD - stream->consumed_bits - bits));
      stream->consumed_bits += bits;
      return true;
    } else {
      *val = (uint32_t)(stream->buffer[stream->consumed_words] >>
                        (FLAC__BITS_PER_WORD - bits));
      stream->consumed_bits += bits;
      return true;
    }
  }
}

int32_t BitStream_read_raw_int32(BitStream *stream, int32_t *val,
                                 uint32_t bits) {
  uint32_t uval, mask;
  /* OPT: inline raw uint32 code here, or make into a macro if possible in the
   * .h file */
  if (!BitStream_read_raw_uint32(br, &uval, bits))
    return false;
  /* sign-extend *val assuming it is currently bits wide. */
  /* From: https://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
   */
  mask = 1u << (bits - 1);
  *val = (uval ^ mask) - mask;
  return true;
}

int32_t BitStream_read_raw_uint64(BitStream *stream, uint64_t *val,
                                  uint32_t bits) {
  uint32_t hi, lo;

  if (bits > 32) {
    if (!BitStream_read_raw_uint32(br, &hi, bits - 32))
      return false;
    if (!BitStream_read_raw_uint32(br, &lo, 32))
      return false;
    *val = hi;
    *val <<= 32;
    *val |= lo;
  } else {
    if (!BitStream_read_raw_uint32(br, &lo, bits))
      return false;
    *val = lo;
  }
  return true;
}
int32_t BitStream_read_raw_int64(BitStream *stream, uint64_t *val,
                                 uint32_t bits) {}
int32_t BitStream_skip_bits(BitStream *stream, uint32_t bits) {
  /*
         * OPT: a faster implementation is possible but probably not that useful
         * since this is only called a couple of times in the metadata readers.
         */
  FLAC__ASSERT(0 != br);
  FLAC__ASSERT(0 != br->buffer);

  if (bits > 0) {
    const unsigned n = br->consumed_bits & 7;
    unsigned m;
    FLAC__uint32 x;

    if (n != 0) {
      m = flac_min(8 - n, bits);
      if (!BitStream_read_raw_uint32(br, &x, m))
        return false;
      bits -= m;
    }
    m = bits / 8;
    if (m > 0) {
      if (!BitStream_skip_byte_block_aligned_no_crc(br, m))
        return false;
      bits %= 8;
    }
    if (bits > 0) {
      if (!BitStream_read_raw_uint32(br, &x, bits))
        return false;
    }
  }

  return true;
}