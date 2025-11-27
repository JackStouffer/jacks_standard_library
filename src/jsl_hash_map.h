#pragma once

#include "jsl_core.h"

// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
static inline uint64_t murmur3_fmix_u64(uint64_t x, uint64_t seed)
{
    uint64_t z = x ^ seed;
    z ^= z >> 33;
    z *= 0xff51afd7ed558ccdULL;
    z ^= z >> 33;
    z *= 0xc4ceb9fe1a85ec53ULL;
    z ^= z >> 33;
    return z;
}

/*
* rapidhash V3 - Very fast, high quality, platform-independent hashing algorithm.
*
* Based on 'wyhash', by Wang Yi <godspeed_china@yeah.net>
*
* Copyright (C) 2025 Nicolas De Carli
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* You can contact the author at:
*   - rapidhash source repository: https://github.com/Nicoshev/rapidhash
*/

#if defined(_MSC_VER)
# include <intrin.h>
# if defined(_M_X64) && !defined(_M_ARM64EC)
#   pragma intrinsic(_umul128)
# endif
#endif

/*
*  C/C++ macros.
*/

#ifdef _MSC_VER
# define RAPIDHASH_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__)
# define RAPIDHASH_ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
# define RAPIDHASH_ALWAYS_INLINE inline
#endif

#ifdef __cplusplus
# define RAPIDHASH_NOEXCEPT noexcept
# define RAPIDHASH_CONSTEXPR constexpr
# ifndef RAPIDHASH_INLINE
#   define RAPIDHASH_INLINE RAPIDHASH_ALWAYS_INLINE
# endif
# if __cplusplus >= 201402L && !defined(_MSC_VER)
#   define RAPIDHASH_INLINE_CONSTEXPR RAPIDHASH_ALWAYS_INLINE constexpr
# else
#   define RAPIDHASH_INLINE_CONSTEXPR RAPIDHASH_ALWAYS_INLINE
# endif
#else
# define RAPIDHASH_NOEXCEPT
# define RAPIDHASH_CONSTEXPR static const
# ifndef RAPIDHASH_INLINE
#   define RAPIDHASH_INLINE static RAPIDHASH_ALWAYS_INLINE
# endif
# define RAPIDHASH_INLINE_CONSTEXPR RAPIDHASH_INLINE
#endif

/*
*  Likely and unlikely macros.
*/
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
# define _likely_(x)  __builtin_expect(x,1)
# define _unlikely_(x)  __builtin_expect(x,0)
#else
# define _likely_(x) (x)
# define _unlikely_(x) (x)
#endif

/*
*  Endianness macros.
*/
#ifndef RAPIDHASH_LITTLE_ENDIAN
# if defined(_WIN32) || defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#   define RAPIDHASH_LITTLE_ENDIAN
# elif defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#   define RAPIDHASH_BIG_ENDIAN
# else
#   warning "could not determine endianness! Falling back to little endian."
#   define RAPIDHASH_LITTLE_ENDIAN
# endif
#endif

/*
*  Default secret parameters.
*/
RAPIDHASH_CONSTEXPR uint64_t jsl__rapid_secret[8] = {
    0x2d358dccaa6c78a5ull,
    0x8bb84b93962eacc9ull,
    0x4b33a62ed433d4a3ull,
    0x4d5a2da51de1aa47ull,
    0xa0761d6478bd642full,
    0xe7037ed1a0b428dbull,
    0x90ed1765281c388cull,
    0xaaaaaaaaaaaaaaaaull};

/*
*  64*64 -> 128bit multiply function.
*
*  @param A  Address of 64-bit number.
*  @param B  Address of 64-bit number.
*
*  Calculates 128-bit C = *A * *B.
*
*  When RAPIDHASH_FAST is defined:
*  Overwrites A contents with C's low 64 bits.
*  Overwrites B contents with C's high 64 bits.
*
*  When RAPIDHASH_PROTECTED is defined:
*  Xors and overwrites A contents with C's low 64 bits.
*  Xors and overwrites B contents with C's high 64 bits.
*/
RAPIDHASH_INLINE_CONSTEXPR void jsl__rapid_mum(uint64_t *A, uint64_t *B) RAPIDHASH_NOEXCEPT {
    #if defined(__SIZEOF_INT128__)
    __uint128_t r=*A; r*=*B;
    *A=(uint64_t)r; *B=(uint64_t)(r>>64);
    #elif defined(_MSC_VER) && (defined(_WIN64) || defined(_M_HYBRID_CHPE_ARM64))
    #if defined(_M_X64)
        *A=_umul128(*A,*B,B);
    #else
        uint64_t c = __umulh(*A, *B);
        *A = *A * *B;
        *B = c;
    #endif
    #else
    uint64_t ha=*A>>32, hb=*B>>32, la=(uint32_t)*A, lb=(uint32_t)*B;
    uint64_t rh=ha*hb, rm0=ha*lb, rm1=hb*la, rl=la*lb, t=rl+(rm0<<32), c=t<rl;
    uint64_t lo=t+(rm1<<32);
    c+=lo<t;
    uint64_t hi=rh+(rm0>>32)+(rm1>>32)+c;
    *A=lo;  *B=hi;
    #endif
}

/*
*  Multiply and xor mix function.
*
*  @param A  64-bit number.
*  @param B  64-bit number.
*
*  Calculates 128-bit C = A * B.
*  Returns 64-bit xor between high and low 64 bits of C.
*/
RAPIDHASH_INLINE_CONSTEXPR uint64_t jsl__rapid_mix(uint64_t A, uint64_t B) RAPIDHASH_NOEXCEPT { jsl__rapid_mum(&A,&B); return A^B; }

/*
*  Read functions.
*/
#ifdef RAPIDHASH_LITTLE_ENDIAN
RAPIDHASH_INLINE uint64_t jsl__rapid_read64(const uint8_t *p) RAPIDHASH_NOEXCEPT { uint64_t v; JSL_MEMCPY(&v, p, sizeof(uint64_t)); return v;}
RAPIDHASH_INLINE uint64_t jsl__rapid_read32(const uint8_t *p) RAPIDHASH_NOEXCEPT { uint32_t v; JSL_MEMCPY(&v, p, sizeof(uint32_t)); return v;}
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
RAPIDHASH_INLINE uint64_t jsl__rapid_read64(const uint8_t *p) RAPIDHASH_NOEXCEPT { uint64_t v; JSL_MEMCPY(&v, p, sizeof(uint64_t)); return __builtin_bswap64(v);}
RAPIDHASH_INLINE uint64_t jsl__rapid_read32(const uint8_t *p) RAPIDHASH_NOEXCEPT { uint32_t v; JSL_MEMCPY(&v, p, sizeof(uint32_t)); return __builtin_bswap32(v);}
#elif defined(_MSC_VER)
RAPIDHASH_INLINE uint64_t jsl__rapid_read64(const uint8_t *p) RAPIDHASH_NOEXCEPT { uint64_t v; JSL_MEMCPY(&v, p, sizeof(uint64_t)); return _byteswap_uint64(v);}
RAPIDHASH_INLINE uint64_t jsl__rapid_read32(const uint8_t *p) RAPIDHASH_NOEXCEPT { uint32_t v; JSL_MEMCPY(&v, p, sizeof(uint32_t)); return _byteswap_ulong(v);}
#else
RAPIDHASH_INLINE uint64_t jsl__rapid_read64(const uint8_t *p) RAPIDHASH_NOEXCEPT {
    uint64_t v; JSL_MEMCPY(&v, p, 8);
    return (((v >> 56) & 0xff)| ((v >> 40) & 0xff00)| ((v >> 24) & 0xff0000)| ((v >>  8) & 0xff000000)| ((v <<  8) & 0xff00000000)| ((v << 24) & 0xff0000000000)| ((v << 40) & 0xff000000000000)| ((v << 56) & 0xff00000000000000));
}
RAPIDHASH_INLINE uint64_t jsl__rapid_read32(const uint8_t *p) RAPIDHASH_NOEXCEPT {
    uint32_t v; JSL_MEMCPY(&v, p, 4);
    return (((v >> 24) & 0xff)| ((v >>  8) & 0xff00)| ((v <<  8) & 0xff0000)| ((v << 24) & 0xff000000));
}
#endif

/*
*  rapidhash main function.
*
*  @param key     Buffer to be hashed.
*  @param len     @key length, in bytes.
*  @param seed    64-bit seed used to alter the hash result predictably.
*  @param secret  Triplet of 64-bit secrets used to alter hash result predictably.
*
*  Returns a 64-bit hash.
*/
RAPIDHASH_INLINE_CONSTEXPR uint64_t jsl__rapidhash_internal(const void *key, size_t len, uint64_t seed, const uint64_t* secret) RAPIDHASH_NOEXCEPT {
    const uint8_t *p=(const uint8_t *)key;
    seed ^= jsl__rapid_mix(seed ^ secret[2], secret[1]);
    uint64_t a=0, b=0;
    size_t i = len;
    if (_likely_(len <= 16)) {
        if (len >= 4) {
        seed ^= len;
        if (len >= 8) {
            const uint8_t* plast = p + len - 8;
            a = jsl__rapid_read64(p);
            b = jsl__rapid_read64(plast);
        } else {
            const uint8_t* plast = p + len - 4;
            a = jsl__rapid_read32(p);
            b = jsl__rapid_read32(plast);
        }
        } else if (len > 0) {
        a = (((uint64_t)p[0])<<45)|p[len-1];
        b = p[len>>1];
        } else
        a = b = 0;
    } else {
        if (len > 112) {
        uint64_t see1 = seed, see2 = seed;
        uint64_t see3 = seed, see4 = seed;
        uint64_t see5 = seed, see6 = seed;
        do {
            seed = jsl__rapid_mix(jsl__rapid_read64(p) ^ secret[0], jsl__rapid_read64(p + 8) ^ seed);
            see1 = jsl__rapid_mix(jsl__rapid_read64(p + 16) ^ secret[1], jsl__rapid_read64(p + 24) ^ see1);
            see2 = jsl__rapid_mix(jsl__rapid_read64(p + 32) ^ secret[2], jsl__rapid_read64(p + 40) ^ see2);
            see3 = jsl__rapid_mix(jsl__rapid_read64(p + 48) ^ secret[3], jsl__rapid_read64(p + 56) ^ see3);
            see4 = jsl__rapid_mix(jsl__rapid_read64(p + 64) ^ secret[4], jsl__rapid_read64(p + 72) ^ see4);
            see5 = jsl__rapid_mix(jsl__rapid_read64(p + 80) ^ secret[5], jsl__rapid_read64(p + 88) ^ see5);
            see6 = jsl__rapid_mix(jsl__rapid_read64(p + 96) ^ secret[6], jsl__rapid_read64(p + 104) ^ see6);
            p += 112;
            i -= 112;
        } while(i > 112);

        seed ^= see1;
        see2 ^= see3;
        see4 ^= see5;
        seed ^= see6;
        see2 ^= see4;
        seed ^= see2;
        }
        if (i > 16) {
        seed = jsl__rapid_mix(jsl__rapid_read64(p) ^ secret[2], jsl__rapid_read64(p + 8) ^ seed);
        if (i > 32) {
            seed = jsl__rapid_mix(jsl__rapid_read64(p + 16) ^ secret[2], jsl__rapid_read64(p + 24) ^ seed);
            if (i > 48) {
                seed = jsl__rapid_mix(jsl__rapid_read64(p + 32) ^ secret[1], jsl__rapid_read64(p + 40) ^ seed);
                if (i > 64) {
                    seed = jsl__rapid_mix(jsl__rapid_read64(p + 48) ^ secret[1], jsl__rapid_read64(p + 56) ^ seed);
                    if (i > 80) {
                        seed = jsl__rapid_mix(jsl__rapid_read64(p + 64) ^ secret[2], jsl__rapid_read64(p + 72) ^ seed);
                        if (i > 96) {
                            seed = jsl__rapid_mix(jsl__rapid_read64(p + 80) ^ secret[1], jsl__rapid_read64(p + 88) ^ seed);
                        }
                    }
                }
            }
        }
        }
        a=jsl__rapid_read64(p+i-16) ^ i;  b=jsl__rapid_read64(p+i-8);
    }
    a ^= secret[1];
    b ^= seed;
    jsl__rapid_mum(&a, &b);
    return jsl__rapid_mix(a ^ secret[7], b ^ secret[1] ^ i);
}

/*
*  rapidhash seeded hash function.
*
*  @param key     Buffer to be hashed.
*  @param len     @key length, in bytes.
*  @param seed    64-bit seed used to alter the hash result predictably.
*
*  Calls rapidhash_internal using provided parameters and default secrets.
*
*  Returns a 64-bit hash.
*/
RAPIDHASH_INLINE_CONSTEXPR uint64_t jsl__rapidhash_withSeed(
    const void *key,
    size_t len,
    uint64_t seed
) RAPIDHASH_NOEXCEPT {
    return jsl__rapidhash_internal(key, len, seed, jsl__rapid_secret);
}

#define JSL__HASH_MAP_GET_SET_FLAG_INDEX(slot_number) slot_number >> 5L

enum JSL__HashmapFlags
{
    JSL__HASHMAP_CANT_EXPAND = JSL_MAKE_BITFLAG(0),
    JSL__HASHMAP_CANT_INSERT = JSL_MAKE_BITFLAG(1),
    JSL__HASHMAP_DUPLICATE_KEYS = JSL_MAKE_BITFLAG(2),
    JSL__HASHMAP_DUPLICATE_VALUES = JSL_MAKE_BITFLAG(3),
    JSL__HASHMAP_NULL_VALUE_SET = JSL_MAKE_BITFLAG(4)
};
