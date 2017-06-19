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
#ifndef LIBDSD_TYPEDEFS_H_INCLUDED
#define LIBDSD_TYPEDEFS_H_INCLUDED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *LIBDSDHandle;

typedef uint32_t ID;

typedef int bool_t;

#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif
#ifndef __cplusplus
#define true 1
#define false 0
#endif

#ifndef DSD_CPU_X86_64
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) ||           \
    defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define DSD_CPU_X86_64
#endif
#endif

#ifndef DSD_CPU_X86
#if defined(__i386__) || defined(__i486__) || defined(__i586__) ||             \
    defined(__i686__) || defined(__i386) || defined(_M_IX86)
#define DSD_CPU_X86
#endif
#endif

#ifndef DSD_CPU_ARM
#if defined(__ARM__)
#define DSD_CPU_ARM
#endif
#endif

#define DSD_U64L(x) x##ULL

#define DSD_BYTE_ALL_ONES ((uint8_t)0xff)

#ifdef DSD_CPU_X86

typedef uint32_t dsd_word;
#define DSD_BYTES_PER_WORD 4
#define DSD_BITS_PER_WORD 32
#define DSD_WORD_ALL_ONES ((uint32_t)0xffffffff)

#else

typedef uint64_t dsd_word;
#define DSD_BYTES_PER_WORD 8
#define DSD_BITS_PER_WORD 64
#define DSD_WORD_ALL_ONES ((uint64_t)DSD_U64L(0xffffffffffffffff))
#endif

#if defined(__GNUC__) &&                                                       \
    (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))

#define DSD_MXN_(A, B) A##B
#define MXN_IMPL(A, B, L, R)                                                   \
  ({                                                                           \
    __typeof__(A) DSD_MXN_(__a, L) = (A);                                      \
    __typeof__(B) DSD_MXN_(__b, L) = (B);                                      \
    DSD_MXN_(__a, L) R DSD_MXN_(__b, L) ? DSD_MXN_(__a, L) : DSD_MXN_(__b, L); \
  })

#define dsd_max(a, b) MXN_IMPL(A, B, __COUNTER__, >)
#define dsd_min(A, B) MXN_IMPL(A, B, __COUNTER__, <)

/* Whatever other unix that has sys/param.h */
#elif defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#define dsd_max(a, b) MAX(a, b)
#define dsd_min(a, b) MIN(a, b)

/* Windows VS has them in stdlib.h.. XXX:Untested */
#elif defined(_MSC_VER)
#include <stdlib.h>
#define dsd_max(a, b) __max(a, b)
#define dsd_min(a, b) __min(a, b)
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifdef __cplusplus
}
#endif

#endif