#include "crc32.hpp"
#include <cstdio>
#if defined(__x86_64__) && !defined(_MSC_VER)
#define CRC32C_INTRINSICS_ENABLED
#endif

inline bool ____is__aligned(const uint8_t* buffer, const int align) noexcept {
	return (((uintptr_t) buffer) & (align-1)) == 0;
}

#ifdef CRC32C_INTRINSICS_ENABLED
/* Generated by https://github.com/corsix/fast-crc32/ using: */
/* ./generate -i avx512 -p crc32c -a v9s3x4e */
/* MIT licensed */

#include <stddef.h>
#include <stdint.h>
#include <nmmintrin.h>
#include <wmmintrin.h>
#include <immintrin.h>

#if defined(_MSC_VER)
#define CRC_AINLINE static __forceinline
#define CRC_ALIGN(n) __declspec(align(n))
#else
#define CRC_AINLINE static __inline __attribute__((always_inline))
#define CRC_ALIGN(n) __attribute__((aligned(n)))
#endif

#define clmul_lo(a, b) (_mm_clmulepi64_si128((a), (b), 0))
#define clmul_hi(a, b) (_mm_clmulepi64_si128((a), (b), 17))

__attribute__ ((target ("avx512vl,pclmul")))
CRC_AINLINE __m128i clmul_scalar(uint32_t a, uint32_t b) {
  return _mm_clmulepi64_si128(_mm_cvtsi32_si128(a), _mm_cvtsi32_si128(b), 0);
}

__attribute__ ((target ("avx512vl,pclmul")))
static uint32_t xnmodp(uint64_t n) /* x^n mod P, in log(n) time */ {
  uint64_t stack = ~(uint64_t)1;
  uint32_t acc, low;
  for (; n > 191; n = (n >> 1) - 16) {
    stack = (stack << 1) + (n & 1);
  }
  stack = ~stack;
  acc = ((uint32_t)0x80000000) >> (n & 31);
  for (n >>= 5; n; --n) {
    acc = _mm_crc32_u32(acc, 0);
  }
  while ((low = stack & 1), stack >>= 1) {
    __m128i x = _mm_cvtsi32_si128(acc);
    uint64_t y = _mm_cvtsi128_si64(_mm_clmulepi64_si128(x, x, 0));
    acc = _mm_crc32_u64(0, y << low);
  }
  return acc;
}

__attribute__ ((target ("avx512vl,pclmul")))
CRC_AINLINE __m128i crc_shift(uint32_t crc, size_t nbytes) {
  return clmul_scalar(crc, xnmodp(nbytes * 8 - 33));
}

__attribute__ ((target ("avx512vl,pclmul")))
static uint32_t crc32c_clmul_long(uint32_t crc0, const char* buf, size_t len) {
  crc0 = ~crc0;
  for (; len && ((uintptr_t)buf & 7); --len) {
    crc0 = _mm_crc32_u8(crc0, *buf++);
  }
  if (((uintptr_t)buf & 8) && len >= 8) {
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)buf);
    buf += 8;
    len -= 8;
  }
  if (len >= 240) {
    const char* end = buf + len;
    size_t blk = (len - 0) / 240;
    size_t klen = blk * 32;
    const char* buf2 = buf + klen * 3;
    const char* limit = buf + klen - 64;
    uint32_t crc1 = 0;
    uint32_t crc2 = 0;
    __m128i vc0;
    __m128i vc1;
    __m128i vc2;
    uint64_t vc;
    /* First vector chunk. */
    __m128i x0 = _mm_loadu_si128((const __m128i*)buf2), y0;
    __m128i x1 = _mm_loadu_si128((const __m128i*)(buf2 + 16)), y1;
    __m128i x2 = _mm_loadu_si128((const __m128i*)(buf2 + 32)), y2;
    __m128i x3 = _mm_loadu_si128((const __m128i*)(buf2 + 48)), y3;
    __m128i x4 = _mm_loadu_si128((const __m128i*)(buf2 + 64)), y4;
    __m128i x5 = _mm_loadu_si128((const __m128i*)(buf2 + 80)), y5;
    __m128i x6 = _mm_loadu_si128((const __m128i*)(buf2 + 96)), y6;
    __m128i x7 = _mm_loadu_si128((const __m128i*)(buf2 + 112)), y7;
    __m128i x8 = _mm_loadu_si128((const __m128i*)(buf2 + 128)), y8;
    __m128i k;
    k = _mm_setr_epi32(0x7e908048, 0, 0xc96cfdc0, 0);
    buf2 += 144;
    /* Main loop. */
    while (buf <= limit) {
      y0 = clmul_lo(x0, k), x0 = clmul_hi(x0, k);
      y1 = clmul_lo(x1, k), x1 = clmul_hi(x1, k);
      y2 = clmul_lo(x2, k), x2 = clmul_hi(x2, k);
      y3 = clmul_lo(x3, k), x3 = clmul_hi(x3, k);
      y4 = clmul_lo(x4, k), x4 = clmul_hi(x4, k);
      y5 = clmul_lo(x5, k), x5 = clmul_hi(x5, k);
      y6 = clmul_lo(x6, k), x6 = clmul_hi(x6, k);
      y7 = clmul_lo(x7, k), x7 = clmul_hi(x7, k);
      y8 = clmul_lo(x8, k), x8 = clmul_hi(x8, k);
      x0 = _mm_ternarylogic_epi64(x0, y0, _mm_loadu_si128((const __m128i*)buf2), 0x96);
      x1 = _mm_ternarylogic_epi64(x1, y1, _mm_loadu_si128((const __m128i*)(buf2 + 16)), 0x96);
      x2 = _mm_ternarylogic_epi64(x2, y2, _mm_loadu_si128((const __m128i*)(buf2 + 32)), 0x96);
      x3 = _mm_ternarylogic_epi64(x3, y3, _mm_loadu_si128((const __m128i*)(buf2 + 48)), 0x96);
      x4 = _mm_ternarylogic_epi64(x4, y4, _mm_loadu_si128((const __m128i*)(buf2 + 64)), 0x96);
      x5 = _mm_ternarylogic_epi64(x5, y5, _mm_loadu_si128((const __m128i*)(buf2 + 80)), 0x96);
      x6 = _mm_ternarylogic_epi64(x6, y6, _mm_loadu_si128((const __m128i*)(buf2 + 96)), 0x96);
      x7 = _mm_ternarylogic_epi64(x7, y7, _mm_loadu_si128((const __m128i*)(buf2 + 112)), 0x96);
      x8 = _mm_ternarylogic_epi64(x8, y8, _mm_loadu_si128((const __m128i*)(buf2 + 128)), 0x96);
      crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)buf);
      crc1 = _mm_crc32_u64(crc1, *(const uint64_t*)(buf + klen));
      crc2 = _mm_crc32_u64(crc2, *(const uint64_t*)(buf + klen * 2));
      crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)(buf + 8));
      crc1 = _mm_crc32_u64(crc1, *(const uint64_t*)(buf + klen + 8));
      crc2 = _mm_crc32_u64(crc2, *(const uint64_t*)(buf + klen * 2 + 8));
      crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)(buf + 16));
      crc1 = _mm_crc32_u64(crc1, *(const uint64_t*)(buf + klen + 16));
      crc2 = _mm_crc32_u64(crc2, *(const uint64_t*)(buf + klen * 2 + 16));
      crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)(buf + 24));
      crc1 = _mm_crc32_u64(crc1, *(const uint64_t*)(buf + klen + 24));
      crc2 = _mm_crc32_u64(crc2, *(const uint64_t*)(buf + klen * 2 + 24));
      buf += 32;
      buf2 += 144;
    }
    /* Reduce x0 ... x8 to just x0. */
    k = _mm_setr_epi32(0xf20c0dfe, 0, 0x493c7d27, 0);
    y0 = clmul_lo(x0, k), x0 = clmul_hi(x0, k);
    x0 = _mm_ternarylogic_epi64(x0, y0, x1, 0x96);
    x1 = x2, x2 = x3, x3 = x4, x4 = x5, x5 = x6, x6 = x7, x7 = x8;
    y0 = clmul_lo(x0, k), x0 = clmul_hi(x0, k);
    y2 = clmul_lo(x2, k), x2 = clmul_hi(x2, k);
    y4 = clmul_lo(x4, k), x4 = clmul_hi(x4, k);
    y6 = clmul_lo(x6, k), x6 = clmul_hi(x6, k);
    x0 = _mm_ternarylogic_epi64(x0, y0, x1, 0x96);
    x2 = _mm_ternarylogic_epi64(x2, y2, x3, 0x96);
    x4 = _mm_ternarylogic_epi64(x4, y4, x5, 0x96);
    x6 = _mm_ternarylogic_epi64(x6, y6, x7, 0x96);
    k = _mm_setr_epi32(0x3da6d0cb, 0, 0xba4fc28e, 0);
    y0 = clmul_lo(x0, k), x0 = clmul_hi(x0, k);
    y4 = clmul_lo(x4, k), x4 = clmul_hi(x4, k);
    x0 = _mm_ternarylogic_epi64(x0, y0, x2, 0x96);
    x4 = _mm_ternarylogic_epi64(x4, y4, x6, 0x96);
    k = _mm_setr_epi32(0x740eef02, 0, 0x9e4addf8, 0);
    y0 = clmul_lo(x0, k), x0 = clmul_hi(x0, k);
    x0 = _mm_ternarylogic_epi64(x0, y0, x4, 0x96);
    /* Final scalar chunk. */
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)buf);
    crc1 = _mm_crc32_u64(crc1, *(const uint64_t*)(buf + klen));
    crc2 = _mm_crc32_u64(crc2, *(const uint64_t*)(buf + klen * 2));
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)(buf + 8));
    crc1 = _mm_crc32_u64(crc1, *(const uint64_t*)(buf + klen + 8));
    crc2 = _mm_crc32_u64(crc2, *(const uint64_t*)(buf + klen * 2 + 8));
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)(buf + 16));
    crc1 = _mm_crc32_u64(crc1, *(const uint64_t*)(buf + klen + 16));
    crc2 = _mm_crc32_u64(crc2, *(const uint64_t*)(buf + klen * 2 + 16));
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)(buf + 24));
    crc1 = _mm_crc32_u64(crc1, *(const uint64_t*)(buf + klen + 24));
    crc2 = _mm_crc32_u64(crc2, *(const uint64_t*)(buf + klen * 2 + 24));
    vc0 = crc_shift(crc0, klen * 2 + blk * 144);
    vc1 = crc_shift(crc1, klen + blk * 144);
    vc2 = crc_shift(crc2, 0 + blk * 144);
    vc = _mm_extract_epi64(_mm_ternarylogic_epi64(vc0, vc1, vc2, 0x96), 0);
    /* Reduce 128 bits to 32 bits, and multiply by x^32. */
    crc0 = _mm_crc32_u64(0, _mm_extract_epi64(x0, 0));
    crc0 = _mm_crc32_u64(crc0, vc ^ _mm_extract_epi64(x0, 1));
    buf = buf2;
    len = end - buf;
  }
  for (; len >= 8; buf += 8, len -= 8) {
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t*)buf);
  }
  for (; len; --len) {
    crc0 = _mm_crc32_u8(crc0, *buf++);
  }
  return ~crc0;
}

__attribute__ ((target ("sse4.2")))
uint32_t crc32c_sse42(uint32_t hash, const uint8_t* buffer, size_t len)
{
	// 8-bits until 4-byte aligned
	while (!____is__aligned(buffer, 4) && len > 0) {
		hash = _mm_crc32_u8(hash, *buffer); buffer++; len--;
	}
	// 16 bytes at a time
	while (len >= 16) {
		hash = _mm_crc32_u32(hash, *(uint32_t*) (buffer +  0));
		hash = _mm_crc32_u32(hash, *(uint32_t*) (buffer +  4));
		hash = _mm_crc32_u32(hash, *(uint32_t*) (buffer +  8));
		hash = _mm_crc32_u32(hash, *(uint32_t*) (buffer + 12));
		buffer += 16; len -= 16;
	}
	// 4 bytes at a time
	while (len >= 4) {
		hash = _mm_crc32_u32(hash, *(uint32_t*) buffer);
		buffer += 4; len -= 4;
	}
	// remaining bytes
	if (len & 2) {
		hash = _mm_crc32_u16(hash, *(uint16_t*) buffer);
		buffer += 2;
	}
	if (len & 1) {
		hash = _mm_crc32_u8(hash, *buffer);
	}
	return hash;
}
#endif

namespace riscv
{
	uint32_t crc32c(uint32_t crc, const void* data, size_t len)
	{
	#ifdef CRC32C_INTRINSICS_ENABLED
		if (__builtin_cpu_supports ("avx512vl"))
		{
			return crc32c_clmul_long(crc, (const char*)data, len);
		}
		else if (__builtin_cpu_supports ("sse4.2"))
		{
			return ~crc32c_sse42(~crc, (const uint8_t*)data, len);
		}
	#endif
		return ~crc32<0x82F63B78>(~crc, data, len);
	}

	uint32_t crc32c(const void* data, size_t len)
	{
	#ifdef CRC32C_INTRINSICS_ENABLED
		if (__builtin_cpu_supports ("avx512vl"))
		{
			return crc32c_clmul_long(0, (const char*)data, len);
		}
		else if (__builtin_cpu_supports ("sse4.2"))
		{
			return ~crc32c_sse42(0xFFFFFFFF, (const uint8_t *)data, len);
		}
	#endif
		return ~crc32<0x82F63B78>(0xFFFFFFFF, data, len);
	}
}
