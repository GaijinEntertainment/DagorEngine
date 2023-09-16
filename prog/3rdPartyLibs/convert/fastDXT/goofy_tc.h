// goofy_tc.h v1.0
// Realtime BC1/ETC1 encoder by Sergey Makeev <sergeymakeev@hotmail.com>
//
// LICENSE:
//  MIT license at the end of this file.

namespace goofy
{
    int compressDXT1(unsigned char* result, const unsigned char* input, unsigned int width, unsigned int height, unsigned int stride);
    int compressETC1(unsigned char* result, const unsigned char* input, unsigned int width, unsigned int height, unsigned int stride);
}

#include <stdint.h>

// Enable SSE2 codec
#define GOOFY_SSE2 (1)

#define goofy_restrict __restrict
#define goofy_inline __forceinline

#define goofy_align16(x) alignas(16) x

#ifdef GOOFY_SSE2
#include <emmintrin.h>  // SSE2
#else
#include <string> // memset/memcpy
#endif

#ifdef GOOFYTC_IMPLEMENTATION
namespace goofy
{

// constants
goofy_align16(static const uint32_t gConstEight[4]) = { 0x08080808, 0x08080808, 0x08080808, 0x08080808 };
goofy_align16(static const uint32_t gConstSixteen[4]) = { 0x10101010, 0x10101010, 0x10101010, 0x10101010 };
goofy_align16(static const uint32_t gConstMaxInt[4]) = { 0x7f7f7f7f, 0x7f7f7f7f, 0x7f7f7f7f, 0x7f7f7f7f };

#ifdef GOOFY_SSE2
typedef __m128i uint8x16_t;
#else

struct uint8x16_t
{
    union
    {
        uint8_t data[16];

        int8_t m128i_i8[16];
        uint8_t m128i_u8[16];

        struct
        {
            uint8_t r0;
            uint8_t g0;
            uint8_t b0;
            uint8_t a0;

            uint8_t r1;
            uint8_t g1;
            uint8_t b1;
            uint8_t a1;

            uint8_t r2;
            uint8_t g2;
            uint8_t b2;
            uint8_t a2;

            uint8_t r3;
            uint8_t g3;
            uint8_t b3;
            uint8_t a3;
        };

        struct
        {
            uint16_t s0;
            uint16_t s1;
            uint16_t s2;
            uint16_t s3;
            uint16_t s4;
            uint16_t s5;
            uint16_t s6;
            uint16_t s7;
        };

        struct
        {
            uint32_t u0;
            uint32_t u1;
            uint32_t u2;
            uint32_t u3;
        };

        struct
        {
            uint64_t l0;
            uint64_t l1;
        };
    };
};

#endif


// 2x16xU8
struct uint8x16x2_t
{
    // rows
    uint8x16_t r0;
    uint8x16_t r1;
};

// 3x16xU8
struct uint8x16x3_t
{
    // rows
    uint8x16_t r0;
    uint8x16_t r1;
    uint8x16_t r2;
};

// 4x16xU8
struct uint8x16x4_t
{
    // rows
    uint8x16_t r0;
    uint8x16_t r1;
    uint8x16_t r2;
    uint8x16_t r3;
};

// 2xU64
struct uint64x2_t
{
    uint64_t r0;
    uint64_t r1;
};


namespace simd
{
// SSE2 implementation
#ifdef GOOFY_SSE2

    goofy_inline uint8x16_t zero()
    {
        return _mm_setzero_si128();
    }

    goofy_inline uint8x16_t fetch(const void* p)
    {
        return _mm_load_si128((const __m128i*)p);
    }

    goofy_inline void extract_uint64(uint64_t &to, const uint8x16_t& a)
    {
      #if __i386__ || __i386 || _M_IX86
        _mm_storel_epi64((__m128i*)&to, a);
      #else
        to = _mm_cvtsi128_si64(a);
      #endif
    }
    goofy_inline uint64x2_t getAsUInt64x2(const uint8x16_t& a)
    {
        uint64x2_t res;
        extract_uint64(res.r0, a);
        extract_uint64(res.r1, _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2)));
        return res;
    }

    goofy_inline uint8x16_t simd_or(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_or_si128(a, b);
    }

    goofy_inline uint8x16_t simd_and(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_and_si128(a, b);
    }

    goofy_inline uint8x16_t simd_andnot(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_andnot_si128(a, b);
    }

    goofy_inline uint8x16_t select(const uint8x16_t& mask, const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_or_si128(_mm_and_si128(mask, a), _mm_andnot_si128(mask, b));
    }

    goofy_inline uint8x16_t minu(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_min_epu8(a, b);
    }

    goofy_inline uint8x16_t maxu(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_max_epu8(a, b);
    }

    goofy_inline uint8x16_t avg(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_avg_epu8(a, b);
    }

    goofy_inline uint8x16_t replicateU0000(const uint8x16_t& a)
    {
        return _mm_shuffle_epi32(a, _MM_SHUFFLE(0, 0, 0, 0));
    }

    goofy_inline uint8x16_t replicateU1111(const uint8x16_t& a)
    {
        return _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 1, 1, 1));
    }

    goofy_inline uint8x16_t replicateU2222(const uint8x16_t& a)
    {
        return _mm_shuffle_epi32(a, _MM_SHUFFLE(2, 2, 2, 2));
    }

    goofy_inline uint8x16_t replicateU3333(const uint8x16_t& a)
    {
        return _mm_shuffle_epi32(a, _MM_SHUFFLE(3, 3, 3, 3));
    }

    goofy_inline uint8x16_t cmpeqi(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_cmpeq_epi8(a, b);
    }

    goofy_inline uint8x16_t cmplti(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_cmplt_epi8(a, b);
    }

    goofy_inline uint8x16_t addsatu(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_adds_epu8(a, b);
    }

    goofy_inline uint8x16_t subsatu(const uint8x16_t& a, const uint8x16_t& b)
    {
        return _mm_subs_epu8(a, b);
    }

    goofy_inline uint8x16x4_t transposeAs4x4(const uint8x16x4_t& v)
    {
        uint8x16_t tr0 = _mm_unpacklo_epi32(v.r0, v.r1);
        uint8x16_t tr1 = _mm_unpacklo_epi32(v.r2, v.r3);
        uint8x16_t tr2 = _mm_unpackhi_epi32(v.r0, v.r1);
        uint8x16_t tr3 = _mm_unpackhi_epi32(v.r2, v.r3);

        uint8x16x4_t res;
        res.r0 = _mm_unpacklo_epi64(tr0, tr1);
        res.r1 = _mm_unpackhi_epi64(tr0, tr1);
        res.r2 = _mm_unpacklo_epi64(tr2, tr3);
        res.r3 = _mm_unpackhi_epi64(tr2, tr3);
        return res;
    }

    goofy_inline uint8x16x3_t deinterleaveRGB(const uint8x16x4_t& v)
    {
        uint8x16_t s0a = _mm_unpacklo_epi8(v.r0, v.r1);
        uint8x16_t s0b = _mm_unpackhi_epi8(v.r0, v.r1);
        uint8x16_t s0c = _mm_unpacklo_epi8(v.r2, v.r3);
        uint8x16_t s0d = _mm_unpackhi_epi8(v.r2, v.r3);
        uint8x16_t s1a = _mm_unpacklo_epi8(s0a, s0b);
        uint8x16_t s1b = _mm_unpackhi_epi8(s0a, s0b);
        uint8x16_t s1c = _mm_unpacklo_epi8(s0c, s0d);
        uint8x16_t s1d = _mm_unpackhi_epi8(s0c, s0d);
        uint8x16_t s2a = _mm_unpacklo_epi8(s1a, s1b);
        uint8x16_t s2b = _mm_unpackhi_epi8(s1a, s1b);
        uint8x16_t s2c = _mm_unpacklo_epi8(s1c, s1d);
        uint8x16_t s2d = _mm_unpackhi_epi8(s1c, s1d);

        uint8x16x3_t res;
        res.r0 = _mm_unpacklo_epi64(s2a, s2c);   // red
        res.r1 = _mm_unpackhi_epi64(s2a, s2c);   // green
        res.r2 = _mm_unpacklo_epi64(s2b, s2d);   // blue
        //res.r3 = _mm_unpackhi_epi64(s2b, s2d); // alpha
        return res;
    }

    goofy_inline uint8x16x4_t deinterleaveRGBA(const uint8x16x4_t& v)
    {
        uint8x16_t s0a = _mm_unpacklo_epi8(v.r0, v.r1);
        uint8x16_t s0b = _mm_unpackhi_epi8(v.r0, v.r1);
        uint8x16_t s0c = _mm_unpacklo_epi8(v.r2, v.r3);
        uint8x16_t s0d = _mm_unpackhi_epi8(v.r2, v.r3);
        uint8x16_t s1a = _mm_unpacklo_epi8(s0a, s0b);
        uint8x16_t s1b = _mm_unpackhi_epi8(s0a, s0b);
        uint8x16_t s1c = _mm_unpacklo_epi8(s0c, s0d);
        uint8x16_t s1d = _mm_unpackhi_epi8(s0c, s0d);
        uint8x16_t s2a = _mm_unpacklo_epi8(s1a, s1b);
        uint8x16_t s2b = _mm_unpackhi_epi8(s1a, s1b);
        uint8x16_t s2c = _mm_unpacklo_epi8(s1c, s1d);
        uint8x16_t s2d = _mm_unpackhi_epi8(s1c, s1d);

        uint8x16x4_t res;
        res.r0 = _mm_unpacklo_epi64(s2a, s2c);   // red
        res.r1 = _mm_unpackhi_epi64(s2a, s2c);   // green
        res.r2 = _mm_unpacklo_epi64(s2b, s2d);   // blue
        res.r3 = _mm_unpackhi_epi64(s2b, s2d);   // alpha
        return res;
    }

    // transpose as four single channel 4x4 blocks at once
    //
    // in:
    //
    // R0  = | bl0.abcd | bl0.efgh | bl0.ijkl | bl0.mnop |
    // R1  = | bl1.abcd | bl1.efgh | bl1.ijkl | bl1.mnop |
    // R2  = | bl2.abcd | bl2.efgh | bl2.ijkl | bl2.mnop |
    // R3  = | bl3.abcd | bl3.efgh | bl3.ijkl | bl3.mnop |
    //
    // out:
    //
    // R1  = | bl0.aeim | bl0.bfjo | bl0.cgko | bl0.dhkl |
    // R2  = | bl1.aeim | bl1.bfjo | bl1.cgko | bl1.dhkl |
    // R3  = | bl2.aeim | bl2.bfjo | bl2.cgko | bl2.dhkl |
    // R0  = | bl3.aeim | bl3.bfjo | bl3.cgko | bl3.dhkl |
    //
    //  +---+---+---+---+           +---+---+---+---+
    //  | A | B | C | D |           | A | E | I | M |
    //  +---+---+---+---+           +---+---+---+---+
    //  | E | F | G | H |           | B | F | J | O |
    //  +---+---+---+---+    -->    +---+---+---+---+
    //  | I | J | K | L |           | C | G | K | O |
    //  +---+---+---+---+           +---+---+---+---+
    //  | M | N | O | P |           | D | H | K | L |
    //  +---+---+---+---+           +---+---+---+---+
    //
    goofy_inline uint8x16x4_t transposeAs4x4x4(const uint8x16x4_t& v)
    {
        const uint8x16_t s0a = _mm_unpacklo_epi8(v.r0, v.r1);
        const uint8x16_t s0b = _mm_unpackhi_epi8(v.r0, v.r1);
        const uint8x16_t s0c = _mm_unpacklo_epi8(v.r2, v.r3);
        const uint8x16_t s0d = _mm_unpackhi_epi8(v.r2, v.r3);
        const uint8x16_t s1a = _mm_unpacklo_epi8(s0a, s0b);
        const uint8x16_t s1b = _mm_unpackhi_epi8(s0a, s0b);
        const uint8x16_t s1c = _mm_unpacklo_epi8(s0c, s0d);
        const uint8x16_t s1d = _mm_unpackhi_epi8(s0c, s0d);
        const uint8x16_t s2a = _mm_unpacklo_epi8(s1a, s1b);
        const uint8x16_t s2b = _mm_unpackhi_epi8(s1a, s1b);
        const uint8x16_t s2c = _mm_unpacklo_epi8(s1c, s1d);
        const uint8x16_t s2d = _mm_unpackhi_epi8(s1c, s1d);

        const uint8x16_t s3a = _mm_unpacklo_epi32(s2a, s2b);
        const uint8x16_t s3b = _mm_unpackhi_epi32(s2a, s2b);
        const uint8x16_t s3c = _mm_unpacklo_epi32(s2c, s2d);
        const uint8x16_t s3d = _mm_unpackhi_epi32(s2c, s2d);

        const uint8x16_t s4a = _mm_unpacklo_epi64(s3a, s3b);
        const uint8x16_t s4b = _mm_unpackhi_epi64(s3a, s3b);
        const uint8x16_t s4c = _mm_unpacklo_epi64(s3c, s3d);
        const uint8x16_t s4d = _mm_unpackhi_epi64(s3c, s3d);

        uint8x16x4_t res;
        res.r0 = _mm_shuffle_epi32(s4a, _MM_SHUFFLE(2, 0, 3, 1));
        res.r1 = _mm_shuffle_epi32(s4b, _MM_SHUFFLE(2, 0, 3, 1));
        res.r2 = _mm_shuffle_epi32(s4c, _MM_SHUFFLE(2, 0, 3, 1));
        res.r3 = _mm_shuffle_epi32(s4d, _MM_SHUFFLE(2, 0, 3, 1));
        return res;
    }

    goofy_inline uint8x16x4_t zipU4x2(const uint8x16_t& a, const uint8x16_t& b, const uint8x16_t& c, const uint8x16_t& d)
    {
        const uint8x16x4_t res = {
            _mm_unpacklo_epi32(a, b),
            _mm_unpackhi_epi32(a, b),
            _mm_unpacklo_epi32(c, d),
            _mm_unpackhi_epi32(c, d),
        };

        return res;
    }

    goofy_inline uint8x16x2_t zipU4(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16x2_t res;
        res.r0 = _mm_unpacklo_epi32(a, b);
        res.r1 = _mm_unpackhi_epi32(a, b);
        return res;
    }

    goofy_inline uint32_t moveMaskMSB(const uint8x16_t& v)
    {
        return (uint32_t)_mm_movemask_epi8(v);
    }

    goofy_inline uint8x16x2_t zipB16(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16x2_t res;
        res.r0 = _mm_unpacklo_epi8(a, b);
        res.r1 = _mm_unpackhi_epi8(a, b);
        return res;
    }

    goofy_inline uint8x16_t simd_not(const uint8x16_t& v)
    {
        return _mm_xor_si128(v, _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()));
    }

#else
    // generic CPU implementation
    namespace detail
    {
        goofy_inline uint8x16_t unpacklo16(const uint8x16_t& a, const uint8x16_t& b)
        {
            uint8x16_t res;
            res.s0 = a.s0;
            res.s1 = b.s0;
            res.s2 = a.s1;
            res.s3 = b.s1;
            res.s4 = a.s2;
            res.s5 = b.s2;
            res.s6 = a.s3;
            res.s7 = b.s3;
            return res;
        }

        goofy_inline uint8x16_t unpackhi16(const uint8x16_t& a, const uint8x16_t& b)
        {
            uint8x16_t res;
            res.s0 = a.s4;
            res.s1 = b.s4;
            res.s2 = a.s5;
            res.s3 = b.s5;
            res.s4 = a.s6;
            res.s5 = b.s6;
            res.s6 = a.s7;
            res.s7 = b.s7;
            return res;
        }

        goofy_inline uint8x16_t unpacklo8(const uint8x16_t& a, const uint8x16_t& b)
        {
            uint8x16_t res;
            res.data[0] = a.data[0];
            res.data[1] = b.data[0];
            res.data[2] = a.data[1];
            res.data[3] = b.data[1];
            res.data[4] = a.data[2];
            res.data[5] = b.data[2];
            res.data[6] = a.data[3];
            res.data[7] = b.data[3];
            res.data[8] = a.data[4];
            res.data[9] = b.data[4];
            res.data[10] = a.data[5];
            res.data[11] = b.data[5];
            res.data[12] = a.data[6];
            res.data[13] = b.data[6];
            res.data[14] = a.data[7];
            res.data[15] = b.data[7];
            return res;
        }

        goofy_inline uint8x16_t unpackhi8(const uint8x16_t& a, const uint8x16_t& b)
        {
            uint8x16_t res;
            res.data[0] = a.data[8];
            res.data[1] = b.data[8];
            res.data[2] = a.data[9];
            res.data[3] = b.data[9];
            res.data[4] = a.data[10];
            res.data[5] = b.data[10];
            res.data[6] = a.data[11];
            res.data[7] = b.data[11];
            res.data[8] = a.data[12];
            res.data[9] = b.data[12];
            res.data[10] = a.data[13];
            res.data[11] = b.data[13];
            res.data[12] = a.data[14];
            res.data[13] = b.data[14];
            res.data[14] = a.data[15];
            res.data[15] = b.data[15];
            return res;
        }

        goofy_inline uint8x16_t unpacklo64(const uint8x16_t& a, const uint8x16_t& b)
        {
            uint8x16_t res;
            res.l0 = a.l0;
            res.l1 = b.l0;
            return res;
        }

        goofy_inline uint8x16_t unpackhi64(const uint8x16_t& a, const uint8x16_t& b)
        {
            uint8x16_t res;
            res.l0 = a.l1;
            res.l1 = b.l1;
            return res;
        }

        goofy_inline uint8x16_t unpacklo32(const uint8x16_t& a, const uint8x16_t& b)
        {
            uint8x16_t res;
            res.u0 = a.u0;
            res.u1 = b.u0;
            res.u2 = a.u1;
            res.u3 = b.u1;
            return res;
        }

        goofy_inline uint8x16_t unpackhi32(const uint8x16_t& a, const uint8x16_t& b)
        {
            uint8x16_t res;
            res.u0 = a.u2;
            res.u1 = b.u2;
            res.u2 = a.u3;
            res.u3 = b.u3;
            return res;
        }

        goofy_inline uint8x16_t replicateU0011(const uint8x16_t& a)
        {
            uint8x16_t res;
            res.u0 = a.u0;
            res.u1 = a.u0;
            res.u2 = a.u1;
            res.u3 = a.u1;
            return res;
        }

        goofy_inline uint8x16_t replicateU2233(const uint8x16_t& a)
        {
            uint8x16_t res;
            res.u0 = a.u2;
            res.u1 = a.u2;
            res.u2 = a.u3;
            res.u3 = a.u3;
            return res;
        }

        goofy_inline uint8x16_t swizzleU1302(const uint8x16_t& a)
        {
            uint8x16_t res;
            res.u0 = a.u1;
            res.u1 = a.u3;
            res.u2 = a.u0;
            res.u3 = a.u2;
            return res;
        }

    } //detail

    goofy_inline uint8x16_t zero()
    {
        uint8x16_t r;
        memset(&r, 0, sizeof(uint8x16_t));
        return r;
    }

    goofy_inline uint8x16_t fetch(const void* p)
    {
        uint8x16_t r;
        memcpy(&r, p, sizeof(uint8x16_t));
        return r;
    }

    goofy_inline uint64x2_t getAsUInt64x2(const uint8x16_t& a)
    {
        uint64x2_t res;
        res.r0 = a.l0;
        res.r1 = a.l1;
        return res;
    }

    // bit or
    goofy_inline uint8x16_t simd_or(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            res.data[i] = a.data[i] | b.data[i];
        }
        return res;
    }

    // bit and
    goofy_inline uint8x16_t simd_and(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            res.data[i] = a.data[i] & b.data[i];
        }
        return res;
    }

    goofy_inline uint8x16_t simd_andnot(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            res.data[i] = (~a.data[i]) & b.data[i];
        }
        return res;
    }

    goofy_inline uint32_t moveMaskMSB(const uint8x16_t& v)
    {
        uint32_t res = 0;
        for (uint32_t i = 0; i < 16; i++)
        {
            uint32_t msb = ((v.data[i] & 0x80) >> 7);
            res = res | (msb << i);
        }
        return res;
    }

    //
    // if (maskA) {
    //  return a;
    // }
    // else {
    //  return b;
    // }
    goofy_inline uint8x16_t select(const uint8x16_t& mask, const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            unsigned char msk = mask.data[i];
            res.data[i] = (msk & a.data[i]) | ((~msk) & b.data[i]); // _mm_or_si128(_mm_and_si128(mask, a), _mm_andnot_si128(mask, b))
        }
        return res;
    }

    goofy_inline uint8x16_t minu(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            res.data[i] = (a.data[i] < b.data[i]) ? a.data[i] : b.data[i];
        }
        return res;
    }

    goofy_inline uint8x16_t maxu(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            res.data[i] = (a.data[i] > b.data[i]) ? a.data[i] : b.data[i];
        }
        return res;
    }

    goofy_inline uint8x16_t avg(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            uint32_t t = (a.data[i] + b.data[i]) + 1;
            res.data[i] = (uint8_t)(t >> 1);
        }
        return res;
    }

    goofy_inline uint8x16_t replicateU0000(const uint8x16_t& a)
    {
        uint8x16_t res;
        res.u0 = a.u0;
        res.u1 = a.u0;
        res.u2 = a.u0;
        res.u3 = a.u0;
        return res;
    }

    goofy_inline uint8x16_t replicateU1111(const uint8x16_t& a)
    {
        uint8x16_t res;
        res.u0 = a.u1;
        res.u1 = a.u1;
        res.u2 = a.u1;
        res.u3 = a.u1;
        return res;
    }

    goofy_inline uint8x16_t replicateU2222(const uint8x16_t& a)
    {
        uint8x16_t res;
        res.u0 = a.u2;
        res.u1 = a.u2;
        res.u2 = a.u2;
        res.u3 = a.u2;
        return res;
    }

    goofy_inline uint8x16_t replicateU3333(const uint8x16_t& a)
    {
        uint8x16_t res;
        res.u0 = a.u3;
        res.u1 = a.u3;
        res.u2 = a.u3;
        res.u3 = a.u3;
        return res;
    }

    // cmp equal (signed)
    goofy_inline uint8x16_t cmpeqi(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            res.data[i] = ((char)a.data[i] == (char)b.data[i]) ? 0xFF : 0x00;
        }
        return res;
    }

    // cmp less (signed)
    goofy_inline uint8x16_t cmplti(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            res.data[i] = ((char)a.data[i] < (char)b.data[i]) ? 0xFF : 0x00;
        }
        return res;
    }

    // add unsigned saturate
    goofy_inline uint8x16_t addsatu(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            int32_t diff = ((unsigned char)a.data[i] + (unsigned char)b.data[i]);
            if (diff > 255)
                diff = 255;
            res.data[i] = (uint8_t)diff;
        }
        return res;
    }

    // sub unsigned saturate
    goofy_inline uint8x16_t subsatu(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16_t res;
        for (uint32_t i = 0; i < 16; i++)
        {
            int32_t diff = ((unsigned char)a.data[i] - (unsigned char)b.data[i]);
            if (diff < 0)
                diff = 0;
            res.data[i] = (uint8_t)diff;
        }
        return res;
    }

    // transpose as one 4x4 RGBA block
    //
    // in:
    //
    // R0  = | a0.rgba | a1.rgba | a2.rgba | a3.rgba |
    // R1  = | b0.rgba | b1.rgba | b2.rgba | b3.rgba |
    // R2  = | c0.rgba | c1.rgba | c2.rgba | c3.rgba |
    // R3  = | d0.rgba | d1.rgba | d2.rgba | d3.rgba |
    //
    // out:
    //
    // R0  = | a0.rgba | b0.rgba | c0.rgba | d0.rgba |
    // R1  = | a1.rgba | b1.rgba | c1.rgba | d1.rgba |
    // R2  = | a2.rgba | b2.rgba | c2.rgba | d2.rgba |
    // R3  = | a3.rgba | b3.rgba | c3.rgba | d3.rgba |
    //
    goofy_inline uint8x16x4_t transposeAs4x4(const uint8x16x4_t& v)
    {
        // a0, b0, a1, b1
        const uint8x16_t tr0 = detail::unpacklo32(v.r0, v.r1);
        // c0, d0, c1, d1
        const uint8x16_t tr1 = detail::unpacklo32(v.r2, v.r3);
        // a2, b2, a3, b3
        const uint8x16_t tr2 = detail::unpackhi32(v.r0, v.r1);
        // c2, d2, c3, d3
        const uint8x16_t tr3 = detail::unpackhi32(v.r2, v.r3);

        uint8x16x4_t res;
        // a0, b0, c0, d0
        res.r0 = detail::unpacklo64(tr0, tr1);
        // a1, b1, c1, d1
        res.r1 = detail::unpackhi64(tr0, tr1);
        // a2, b2, c2, d2
        res.r2 = detail::unpacklo64(tr2, tr3);
        // a3, b3, c3, d3
        res.r3 = detail::unpackhi64(tr2, tr3);
        return res;
    }

    // deinterleave as 4x16
    //
    // in:
    //
    // R0  = | a0.rgba | a1.rgba | a2.rgba | a3.rgba |
    // R1  = | b0.rgba | b1.rgba | b2.rgba | b3.rgba |
    // R2  = | c0.rgba | c1.rgba | c2.rgba | c3.rgba |
    // R3  = | d0.rgba | d1.rgba | d2.rgba | d3.rgba |
    //
    // out:
    //
    // R0  = | a0.r | a1.r | a2.r | a3.r | b0.r | b1.r | b2.r | b3.r | c0.r | c1.r | c2.r | c3.r | d0.r | d1.r | d2.r | d3.r |
    // R1  = | a0.g | a1.g | a2.g | a3.g | b0.g | b1.g | b2.g | b3.g | c0.g | c1.g | c2.g | c3.g | d0.g | d1.g | d2.g | d3.g |
    // R2  = | a0.b | a1.b | a2.b | a3.b | b0.b | b1.b | b2.b | b3.b | c0.b | c1.b | c2.b | c3.b | d0.b | d1.b | d2.b | d3.b |
    // R3  = | a0.a | a1.a | a2.a | a3.a | b0.a | b1.a | b2.a | b3.a | c0.a | c1.a | c2.a | c3.a | d0.a | d1.a | d2.a | d3.a |
    //
    goofy_inline uint8x16x3_t deinterleaveRGB(const uint8x16x4_t& v)
    {
        // step 1

        // | a0.r | b0.r | a0.g | b0.g | a0.b | b0.b | a0.a | b0.a | a1.r | b1.r | a1.g | b1.g | a1.b | b1.b | a1.a | b1.a |
        // | a2.r | b2.r | a2.g | b2.g | a2.b | b2.b | a2.a | b2.a | a3.r | b3.r | a3.g | b3.g | a3.b | b3.b | a3.a | b3.a |
        const uint8x16_t s0a = detail::unpacklo8(v.r0, v.r1);
        const uint8x16_t s0b = detail::unpackhi8(v.r0, v.r1);

        // | c0.r | d0.r | c0.g | d0.g | c0.b | d0.b | c0.a | d0.a | c1.r | d1.r | c1.g | d1.g | c1.b | d1.b | c1.a | d1.a |
        // | c2.r | d2.r | c2.g | d2.g | c2.b | d2.b | c2.a | d2.a | c3.r | d3.r | c3.g | d3.g | c3.b | d3.b | c3.a | d3.a |
        const uint8x16_t s0c = detail::unpacklo8(v.r2, v.r3);
        const uint8x16_t s0d = detail::unpackhi8(v.r2, v.r3);

        // step 2
        // | a0.r | a2.r | b0.r | b2.r | a0.g | a2.g | b0.g | b2.g | a0.b | a2.b | b0.b | b2.b | a0.a | a2.a | b0.a | b2.a |
        // | a1.r | a3.r | b1.r | b3.r | a1.g | a3.g | b1.g | b3.g | a1.b | a3.b | b1.b | b3.b | a1.a | a3.a | b1.a | b3.a |
        const uint8x16_t s1a = detail::unpacklo8(s0a, s0b);
        const uint8x16_t s1b = detail::unpackhi8(s0a, s0b);

        // | c0.r | c2.r | d0.r | d2.r | c0.g | c2.g | d0.g | d2.g | c0.b | c2.b | d0.b | d2.b | c0.a | c2.a | d0.a | d2.a |
        // | c1.r | c3.r | d1.r | d3.r | c1.g | c3.g | d1.g | d3.g | c1.b | c3.b | d1.b | d3.b | c1.a | c3.a | d1.a | d3.a |
        const uint8x16_t s1c = detail::unpacklo8(s0c, s0d);
        const uint8x16_t s1d = detail::unpackhi8(s0c, s0d);

        // step 3
        // | a0.r | a1.r | a2.r | a3.r | b0.r | b1.r | b2.r | b3.r | a0.g | a1.g | a2.g | a3.g | b0.g | b1.g | b2.g | b3.g |
        // | a0.b | a1.b | a2.b | a3.b | b0.b | b1.b | b2.b | b3.b | a0.a | a1.a | a2.a | a3.a | b0.a | b1.a | b2.a | b3.a |
        const uint8x16_t s2a = detail::unpacklo8(s1a, s1b);
        const uint8x16_t s2b = detail::unpackhi8(s1a, s1b);

        // | c0.r | c1.r | c2.r | c3.r | d0.r | d1.r | d2.r | d3.r | c0.g | c1.g | c2.g | c3.g | d0.g | d1.g | d2.g | d3.g |
        // | c0.b | c1.b | c2.b | c3.b | d0.b | d1.b | d2.b | d3.b | c0.a | c1.a | c2.a | c3.a | d0.a | d1.a | d2.a | d3.a |
        const uint8x16_t s2c = detail::unpacklo8(s1c, s1d);
        const uint8x16_t s2d = detail::unpackhi8(s1c, s1d);

        // step 4 (final)
        uint8x16x3_t res;
        // | a0.r | a1.r | a2.r | a3.r | b0.r | b1.r | b2.r | b3.r | c0.r | c1.r | c2.r | c3.r | d0.r | d1.r | d2.r | d3.r |
        // | a0.g | a1.g | a2.g | a3.g | b0.g | b1.g | b2.g | b3.g | c0.g | c1.g | c2.g | c3.g | d0.g | d1.g | d2.g | d3.g |
        res.r0 = detail::unpacklo64(s2a, s2c);
        res.r1 = detail::unpackhi64(s2a, s2c);

        // | a0.b | a1.b | a2.b | a3.b | b0.b | b1.b | b2.b | b3.b | c0.b | c1.b | c2.b | c3.b | d0.b | d1.b | d2.b | d3.b |
        // | a0.a | a1.a | a2.a | a3.a | b0.a | b1.a | b2.a | b3.a | c0.a | c1.a | c2.a | c3.a | d0.a | d1.a | d2.a | d3.a |
        res.r2 = detail::unpacklo64(s2b, s2d);
        //res.r3 = detail::unpackhi64(s2b, s2d);
        return res;
    }

    // transpose as four single channel 4x4 blocks at once
    //
    // in:
    //
    // R0  = | bl0.abcd | bl0.efgh | bl0.ijkl | bl0.mnop |
    // R1  = | bl1.abcd | bl1.efgh | bl1.ijkl | bl1.mnop |
    // R2  = | bl2.abcd | bl2.efgh | bl2.ijkl | bl2.mnop |
    // R3  = | bl3.abcd | bl3.efgh | bl3.ijkl | bl3.mnop |
    //
    // out:
    //         NOTE: columns are swapped!
    //
    //            3          4           0         1
    // R1  = | bl0.cgko | bl0.dhlp | bl0.aeim | bl0.bfjn |
    // R2  = | bl1.cgko | bl1.dhlp | bl1.aeim | bl1.bfjn |
    // R3  = | bl2.cgko | bl2.dhlp | bl2.aeim | bl2.bfjn |
    // R0  = | bl3.cgko | bl3.dhlp | bl3.aeim | bl3.bfjn |
    //
    //  +---+---+---+---+           +---+---+---+---+
    //  | A | B | C | D |           | C | G | K | O |
    //  +---+---+---+---+           +---+---+---+---+
    //  | E | F | G | H |           | D | H | L | P |
    //  +---+---+---+---+    -->    +---+---+---+---+
    //  | I | J | K | L |           | A | E | I | M |
    //  +---+---+---+---+           +---+---+---+---+
    //  | M | N | O | P |           | B | F | J | N |
    //  +---+---+---+---+           +---+---+---+---+
    //
    goofy_inline uint8x16x4_t transposeAs4x4x4(const uint8x16x4_t& v)
    {
        // step 1

        // | 0.a | 1.a | 0.b | 1.b | 0.c | 1.c | 0.d | 1.d | 0.e | 1.e | 0.f | 1.f | 0.g | 1.g | 0.h | 1.h |
        // | 0.i | 1.i | 0.j | 1.j | 0.k | 1.k | 0.l | 1.l | 0.m | 1.m | 0.n | 1.n | 0.o | 1.o | 0.p | 1.p |
        const uint8x16_t s0a = detail::unpacklo8(v.r0, v.r1);
        const uint8x16_t s0b = detail::unpackhi8(v.r0, v.r1);

        // | 2.a | 3.a | 2.b | 3.b | 2.c | 3.c | 2.d | 3.d | 2.e | 3.e | 2.f | 3.f | 2.g | 3.g | 2.h | 3.h |
        // | 2.i | 3.i | 2.j | 3.j | 2.k | 3.k | 2.l | 3.l | 2.m | 3.m | 2.n | 3.n | 2.o | 3.o | 2.p | 3.p |
        const uint8x16_t s0c = detail::unpacklo8(v.r2, v.r3);
        const uint8x16_t s0d = detail::unpackhi8(v.r2, v.r3);

        // step 2

        // | 0.a | 0.i | 1.a | 1.i | 0.b | 0.j | 1.b | 1.j | 0.c | 0.k | 1.c | 1.k | 0.d | 0.l | 1.d | 1.l |
        // | 0.e | 0.m | 1.e | 1.m | 0.f | 0.n | 1.f | 1.n | 0.g | 0.o | 1.g | 1.o | 0.h | 0.p | 1.h | 1.p |
        const uint8x16_t s1a = detail::unpacklo8(s0a, s0b);
        const uint8x16_t s1b = detail::unpackhi8(s0a, s0b);

        // | 2.a | 2.i | 3.a | 3.i | 2.b | 2.j | 3.b | 3.j | 2.c | 2.k | 3.c | 3.k | 2.d | 2.l | 3.d | 3.l |
        // | 2.e | 2.m | 3.e | 3.m | 2.f | 2.n | 3.f | 3.n | 2.g | 2.o | 3.g | 3.o | 2.h | 2.p | 3.h | 3.p |
        const uint8x16_t s1c = detail::unpacklo8(s0c, s0d);
        const uint8x16_t s1d = detail::unpackhi8(s0c, s0d);

        // step 3

        // | 0.a | 0.e | 0.i | 0.m | 1.a | 1.e | 1.i | 1.m | 0.b | 0.f | 0.j | 0.n | 1.b | 1.f | 1.j | 1.n |
        // | 0.c | 0.g | 0.k | 0.o | 1.c | 1.g | 1.k | 1.o | 0.d | 0.h | 0.l | 0.p | 1.d | 1.h | 1.l | 1.p |
        const uint8x16_t s2a = detail::unpacklo8(s1a, s1b);
        const uint8x16_t s2b = detail::unpackhi8(s1a, s1b);

        // | 2.a | 2.e | 2.i | 2.m | 3.a | 3.e | 3.i | 3.m | 2.b | 2.f | 2.j | 2.n | 3.b | 3.f | 3.j | 3.n |
        // | 2.c | 2.g | 2.k | 2.o | 3.c | 3.g | 3.k | 3.o | 2.d | 2.h | 2.l | 2.p | 3.d | 3.h | 3.l | 3.p |
        const uint8x16_t s2c = detail::unpacklo8(s1c, s1d);
        const uint8x16_t s2d = detail::unpackhi8(s1c, s1d);

        // step 4

        // | 0.a | 0.e | 0.i | 0.m | 0.c | 0.g | 0.k | 0.o | 1.a | 1.e | 1.i | 1.m | 1.c | 1.g | 1.k | 1.o |
        // | 0.b | 0.f | 0.j | 0.n | 0.d | 0.h | 0.l | 0.p | 1.b | 1.f | 1.j | 1.n | 1.d | 1.h | 1.l | 1.p |
        const uint8x16_t s3a = detail::unpacklo32(s2a, s2b);
        const uint8x16_t s3b = detail::unpackhi32(s2a, s2b);

        // | 2.a | 2.e | 2.i | 2.m | 2.c | 2.g | 2.k | 2.o | 3.a | 3.e | 3.i | 3.m | 3.c | 3.g | 3.k | 3.o |
        // | 2.b | 2.f | 2.j | 2.n | 2.d | 2.h | 2.l | 2.p | 3.b | 3.f | 3.j | 3.n | 3.d | 3.h | 3.l | 3.p |
        const uint8x16_t s3c = detail::unpacklo32(s2c, s2d);
        const uint8x16_t s3d = detail::unpackhi32(s2c, s2d);

        // step 5

        // | 0.a | 0.e | 0.i | 0.m | 0.c | 0.g | 0.k | 0.o | 0.b | 0.f | 0.j | 0.n | 0.d | 0.h | 0.l | 0.p |
        // | 1.a | 1.e | 1.i | 1.m | 1.c | 1.g | 1.k | 1.o | 1.b | 1.f | 1.j | 1.n | 1.d | 1.h | 1.l | 1.p |
        const uint8x16_t s4a = detail::unpacklo64(s3a, s3b);
        const uint8x16_t s4b = detail::unpackhi64(s3a, s3b);

        // | 2.a | 2.e | 2.i | 2.m | 2.c | 2.g | 2.k | 2.o | 2.b | 2.f | 2.j | 2.n | 2.d | 2.h | 2.l | 2.p |
        // | 3.a | 3.e | 3.i | 3.m | 3.c | 3.g | 3.k | 3.o | 3.b | 3.f | 3.j | 3.n | 3.d | 3.h | 3.l | 3.p |
        const uint8x16_t s4c = detail::unpacklo64(s3c, s3d);
        const uint8x16_t s4d = detail::unpackhi64(s3c, s3d);

        // step 5 (final)
        uint8x16x4_t res;
        // | 0.c | 0.g | 0.k | 0.o | 0.d | 0.h | 0.l | 0.p | 0.a | 0.e | 0.i | 0.m | 0.b | 0.f | 0.j | 0.n |
        res.r0 = detail::swizzleU1302(s4a);
        // | 1.c | 1.g | 1.k | 1.o | 1.d | 1.h | 1.l | 1.p | 1.a | 1.e | 1.i | 1.m | 1.b | 1.f | 1.j | 1.n |
        res.r1 = detail::swizzleU1302(s4b);
        // | 2.c | 2.g | 2.k | 2.o | 2.d | 2.h | 2.l | 2.p | 2.a | 2.e | 2.i | 2.m | 2.b | 2.f | 2.j | 2.n |
        res.r2 = detail::swizzleU1302(s4c);
        // | 3.c | 3.g | 3.k | 3.o | 3.d | 3.h | 3.l | 3.p | 3.a | 3.e | 3.i | 3.m | 3.b | 3.f | 3.j | 3.n |
        res.r3 = detail::swizzleU1302(s4d);
        return res;
    }

    // like ZipU4 but for two parallel zips
    //
    // in:
    //
    // A  = | a0.rgba | a1.rgba | a2.rgba | a3.rgba |
    // B  = | b0.rgba | b1.rgba | b2.rgba | b3.rgba |
    // C  = | c0.rgba | c1.rgba | c2.rgba | c3.rgba |
    // D  = | d0.rgba | d1.rgba | d2.rgba | d3.rgba |
    //
    // out:
    //
    // R0  = | a0.rgba | b0.rgba | a1.rgba | b1.rgba |
    // R1  = | a2.rgba | b2.rgba | a3.rgba | b3.rgba |
    // R2  = | c0.rgba | d0.rgba | c1.rgba | d1.rgba |
    // R3  = | c2.rgba | d2.rgba | c3.rgba | d3.rgba |
    //
    goofy_inline uint8x16x4_t zipU4x2(const uint8x16_t& a, const uint8x16_t& b, const uint8x16_t& c, const uint8x16_t& d)
    {
        const uint8x16x4_t res = {
                detail::unpacklo32(a, b),
                detail::unpackhi32(a, b),
                detail::unpacklo32(c, d),
                detail::unpackhi32(c, d),
        };
        return res;
    }

    //
    // in:
    //
    // a  = | a0.rgba | a1.rgba | a2.rgba| a3.rgba
    // b  = | b0.rgba | b1.rgba | b2.rgba| b3.rgba
    //
    // out:
    //
    // R0  = | a0.rgba | b0.rgba | a1.rgba | b1.rgba |
    // R1  = | a2.rgba | b2.rgba | a3.rgba | b3.rgba |
    //
    goofy_inline uint8x16x2_t zipU4(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16x2_t res;
        res.r0 = detail::unpacklo32(a, b);
        res.r1 = detail::unpackhi32(a, b);
        return res;
    }

    //
    // in:
    //
    // a  = | a0 | a1 | a2 | a3 | a4 | a5 | a6 | a7 | a8 | a9 | aA | aB | aC | aD | aE | aF |
    // b  = | b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7 | b8 | b9 | bA | bB | bC | bD | bE | bF |
    //
    // out:
    //
    // R0  = | a0 | b0 | a1 | b1 | a2 | b2 | a3 | b3 | a4 | b4 | a5 | b5 | a6 | b6 | a7 | b7 |
    // R1  = | a8 | b8 | a9 | b9 | aA | bA | aB | bB | aC | bC | aD | bD | aE | bE | aF | bF |
    //
    goofy_inline uint8x16x2_t zipB16(const uint8x16_t& a, const uint8x16_t& b)
    {
        uint8x16x2_t res;
        res.r0 = detail::unpacklo8(a, b);
        res.r1 = detail::unpackhi8(a, b);
        return res;
    }

    goofy_inline uint8x16_t simd_not(const uint8x16_t& v)
    {
        uint8x16_t res;
        for (int i = 0; i < 16; i++)
        {
            res.data[i] = v.data[i] ^ 0xFF;
        }
        return res;
    }

#endif
}

static_assert(sizeof(uint8x16_t) == 16, "Incorrect byte8x16 sizeof");
static_assert(sizeof(uint8x16x2_t) == 32, "Incorrect byte8x16x1 sizeof");
static_assert(sizeof(uint8x16x3_t) == 48, "Incorrect byte8x16x2 sizeof");
static_assert(sizeof(uint8x16x4_t) == 64, "Incorrect byte8x16x4 sizeof");
static_assert(sizeof(uint64x2_t) == 16, "Incorrect uint64x2_t sizeof");


// Block brightness variance to ETC control byte
static const uint32_t etc1BrighnessRangeTocontrolByte[256] = {
    0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000,
    0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x03000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000,
    0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x27000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000,
    0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000,
    0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x4B000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000,
    0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000,
    0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x6F000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000,
    0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000,
    0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000,
    0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0x93000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000,
    0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000,
    0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xB7000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000,
    0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000,
    0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000,
    0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000,
    0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xDB000000, 0xFF000000, 0xFF000000
};


enum GoofyCodecType
{
    GOOFY_DXT1,
    GOOFY_ETC1,
};

//
// Encode 4 DXT1/ETC1 at once
//

template<GoofyCodecType CODEC_TYPE>
goofy_inline void goofySimdEncode(const unsigned char* goofy_restrict inputRGBA, size_t inputStride, unsigned char* goofy_restrict pResult)
{
    // Fetch 16x4 pixels from the buffer(four DX blocks)
    // 16 pixels wide is better for the CPU cache utilization (64 bytes per line) and it is better for SIMD lane utilization
    // -----------------------------------------------------------
    uint8x16x4_t bl0;
    uint8x16x4_t bl1;
    uint8x16x4_t bl2;
    uint8x16x4_t bl3;
    bl0.r0 = simd::fetch(inputRGBA);
    bl1.r0 = simd::fetch(inputRGBA + 16);
    bl2.r0 = simd::fetch(inputRGBA + 32);
    bl3.r0 = simd::fetch(inputRGBA + 48);
    inputRGBA += inputStride;
    bl0.r1 = simd::fetch(inputRGBA);
    bl1.r1 = simd::fetch(inputRGBA + 16);
    bl2.r1 = simd::fetch(inputRGBA + 32);
    bl3.r1 = simd::fetch(inputRGBA + 48);
    inputRGBA += inputStride;
    bl0.r2 = simd::fetch(inputRGBA);
    bl1.r2 = simd::fetch(inputRGBA + 16);
    bl2.r2 = simd::fetch(inputRGBA + 32);
    bl3.r2 = simd::fetch(inputRGBA + 48);
    inputRGBA += inputStride;
    bl0.r3 = simd::fetch(inputRGBA);
    bl1.r3 = simd::fetch(inputRGBA + 16);
    bl2.r3 = simd::fetch(inputRGBA + 32);
    bl3.r3 = simd::fetch(inputRGBA + 48);

    // Find min block colors
    // -----------------------------------------------------------
    const uint8x16x4_t blMin = {
        simd::minu(simd::minu(bl0.r0, bl0.r1), simd::minu(bl0.r2, bl0.r3)), // min0_clmn0.rgba | min0_clmn1.rgba | min0_clmn2.rgba | min0_clmn3.rgba
        simd::minu(simd::minu(bl1.r0, bl1.r1), simd::minu(bl1.r2, bl1.r3)), // min1_clmn0.rgba | min1_clmn1.rgba | min1_clmn2.rgba | min1_clmn3.rgba
        simd::minu(simd::minu(bl2.r0, bl2.r1), simd::minu(bl2.r2, bl2.r3)), // min2_clmn0.rgba | min2_clmn1.rgba | min2_clmn2.rgba | min2_clmn3.rgba
        simd::minu(simd::minu(bl3.r0, bl3.r1), simd::minu(bl3.r2, bl3.r3))  // min3_clmn0.rgba | min3_clmn1.rgba | min3_clmn2.rgba | min3_clmn3.rgba
    };

    // blMinTr (transposed blMin)
    // min0_clmn0.rgba | min1_clmn0.rgba | min2_clmn0.rgba | min3_clmn0.rgba
    // min0_clmn1.rgba | min1_clmn1.rgba | min2_clmn1.rgba | min3_clmn1.rgba
    // min0_clmn2.rgba | min1_clmn2.rgba | min2_clmn2.rgba | min3_clmn2.rgba
    // min0_clmn3.rgba | min1_clmn3.rgba | min2_clmn3.rgba | min3_clmn3.rgba
    const uint8x16x4_t blMinTr = simd::transposeAs4x4(blMin);

    // Per-block min colors
    // min0.rgba | min1.rgba | min2.rgba | min3.rgba
    const uint8x16_t minColors = simd::minu(
        simd::minu(blMinTr.r0, blMinTr.r1),
        simd::minu(blMinTr.r2, blMinTr.r3)
    );

    // Same to find max block colors
    // -----------------------------------------------------------
    const uint8x16x4_t blMax = {
        simd::maxu(simd::maxu(bl0.r0, bl0.r1), simd::maxu(bl0.r2, bl0.r3)),
        simd::maxu(simd::maxu(bl1.r0, bl1.r1), simd::maxu(bl1.r2, bl1.r3)),
        simd::maxu(simd::maxu(bl2.r0, bl2.r1), simd::maxu(bl2.r2, bl2.r3)),
        simd::maxu(simd::maxu(bl3.r0, bl3.r1), simd::maxu(bl3.r2, bl3.r3))
    };

    const uint8x16x4_t blMaxTr = simd::transposeAs4x4(blMax);

    // Per-block max colors
    // max0.rgba | max1.rgba | max2.rgba | max3.rgba
    const uint8x16_t maxColors = simd::maxu(
        simd::maxu(blMaxTr.r0, blMaxTr.r1),
        simd::maxu(blMaxTr.r2, blMaxTr.r3)
    );

    // Find min/max brigtness
    // -----------------------------------------------------------

    // Note: some SSE lanes wasted, it is not ideal, but seems OK-ish?
    //
    // min0.rgba | min0.rgba | min1.rgba | min1.rgba
    // min2.rgba | min2.rgba | min3.rgba | min3.rgba
    // max0.rgba | max0.rgba | max1.rgba | max1.rgba
    // max2.rgba | max2.rgba | max3.rgba | max3.rgba
    const uint8x16x4_t blMinMax = simd::zipU4x2(minColors, minColors, maxColors, maxColors);

    // Deinterleave
    // min0.rr | min1.rr | min2.rr | min3.rr | max0.rr | max1.rr | max2.rr | max3.rr
    // min0.gg | min1.gg | min2.gg | min3.gg | max0.gg | max1.gg | max2.gg | max3.gg
    // min0.bb | min1.bb | min2.bb | min3.bb | max0.bb | max1.bb | max2.bb | max3.bb
    const uint8x16x3_t blMinMaxDi = simd::deinterleaveRGB(blMinMax);

    // Get Y component of YCoCg color-model (perceptual brightness)
    // https://en.wikipedia.org/wiki/YCoCg
    // Y = 0.25 * R + 0.25 * B + 0.5 * G
    //   We can rewrite equation above using the following form
    // Y = (((R + B) / 2) + G) / 2
    //
    // Y = min0.yy | min1.yy | min2.yy | min3.yy | max0.yy | max1.yy | max2.yy | max3.yy
    const uint8x16_t Y = simd::avg(simd::avg(blMinMaxDi.r0, blMinMaxDi.r2), blMinMaxDi.r1);

    // Min/max brightness per block
    // R0 = min0.yyyy | min1.yyyy | min2.yyyy | min3.yyyy
    // R1 = max0.yyyy | max1.yyyy | max2.yyyy | max3.yyyy
    const uint8x16x2_t blMinMaxY = simd::zipB16(Y, Y);

    // Clamp to min brightness
    const uint8x16_t constEight = simd::fetch(&gConstEight);
    // range0.yyyy | range1.yyyy | range2.yyyy | range3.yyyy
    const uint8x16_t blRangeY = simd::maxu(simd::subsatu(blMinMaxY.r1, blMinMaxY.r0), constEight);

    // mid0.yyyy | mid1.yyyy | mid2.yyyy | mid3.yyyy
    const uint8x16_t blMidY = simd::avg(blMinMaxY.r0, blMinMaxY.r1);

    // Approximate multiplication by 0.375 to get quantization thresholds
    const uint8x16_t constZero = simd::zero();

    const uint8x16_t blHalfRangeY = simd::avg(blRangeY, constZero);
    const uint8x16_t blQuarterRangeY = simd::avg(blHalfRangeY, constZero);
    const uint8x16_t blEighthsRangeY = simd::avg(blQuarterRangeY, constZero);

    // Threshold = (quarter + eights) = (0.25 + 0.125) ~= (range * 0.375)
    // qt0.yyyy | qt1.yyyy | qt2.yyyy | qt3.yyyy
    const uint8x16_t blQThreshold = simd::addsatu(blQuarterRangeY, blEighthsRangeY);

    // Quantization (generate indices)
    // -----------------------------------------------------------
    const uint8x16_t constMaxInt = simd::fetch(&gConstMaxInt);

    //  block 0
    //
    // p0.r p1.r p2.r p3.r p4.r p5.r p6.r p7.r p8.r p9.r p10.r p11.r p12.r p13.r p14.r p15.r
    // p0.g p1.g p2.g p3.g p4.g p5.g p6.g p7.g p8.g p9.g p10.g p11.g p12.g p13.g p14.g p15.g
    // p0.b p1.b p2.b p3.b p4.b p5.b p6.b p7.b p8.b p9.b p10.b p11.b p12.b p13.b p14.b p15.b
    const uint8x16x3_t bl0Di = simd::deinterleaveRGB(bl0);

    // Convert RGB to brightness
    // per-pixel block brightness
    const uint8x16_t bl0Y = simd::avg(simd::avg(bl0Di.r0, bl0Di.r2), bl0Di.r1);

    // Block brightness to compare with
    const uint8x16_t bl0MidY = simd::replicateU0000(blMidY);

    // Brightness difference (per-pixel in block)
    // NOTE: we need to clamp difference to max signed int8, because of the signed comparison later
    const uint8x16_t bl0PosDiffY = simd::minu(simd::subsatu(bl0Y, bl0MidY), constMaxInt);
    const uint8x16_t bl0NegDiffY = simd::minu(simd::subsatu(bl0MidY, bl0Y), constMaxInt);
    // Greater or Equal to zero mask
    const uint8x16_t bl0GezMask = simd::cmpeqi(bl0NegDiffY, constZero);

    // Absolute diffference of brightness (per-pixel in block)
    const uint8x16_t bl0AbsDiffY = simd::simd_or(bl0PosDiffY, bl0NegDiffY);

    // get quantization threshold for current block
    const uint8x16_t bl0QThreshold = simd::replicateU0000(blQThreshold);

    // Less than Quantization Threshold mask
    const uint8x16_t bl0LqtMask = simd::cmplti(bl0AbsDiffY, bl0QThreshold);

    // Here we've got two bitmasks
    //
    // GezMask = greater or equal than zero (per pixel)
    // LqtMask = less than quantization threshold (per pixel)
    //
    //
    //  min       qt          qt        max
    //   x---------x-----+-----x---------x
    //                   0
    //
    //                   |---------------| greater or equal than zero (GezMask)
    //
    //             |-----------| less than quantization threshold (LqtMask)
    //

    //  block 1
    const uint8x16x3_t bl1Di = simd::deinterleaveRGB(bl1);
    const uint8x16_t bl1Y = simd::avg(simd::avg(bl1Di.r0, bl1Di.r2), bl1Di.r1);
    const uint8x16_t bl1MidY = simd::replicateU1111(blMidY);
    const uint8x16_t bl1PosDiffY = simd::minu(simd::subsatu(bl1Y, bl1MidY), constMaxInt);
    const uint8x16_t bl1NegDiffY = simd::minu(simd::subsatu(bl1MidY, bl1Y), constMaxInt);
    const uint8x16_t bl1GezMask = simd::cmpeqi(bl1NegDiffY, constZero);
    const uint8x16_t bl1AbsDiffY = simd::simd_or(bl1PosDiffY, bl1NegDiffY);
    const uint8x16_t bl1QThreshold = simd::replicateU1111(blQThreshold);
    const uint8x16_t bl1LqtMask = simd::cmplti(bl1AbsDiffY, bl1QThreshold);

    //  block 2
    const uint8x16x3_t bl2Di = simd::deinterleaveRGB(bl2);
    const uint8x16_t bl2Y = simd::avg(simd::avg(bl2Di.r0, bl2Di.r2), bl2Di.r1);
    const uint8x16_t bl2MidY = simd::replicateU2222(blMidY);
    const uint8x16_t bl2PosDiffY = simd::minu(simd::subsatu(bl2Y, bl2MidY), constMaxInt);
    const uint8x16_t bl2NegDiffY = simd::minu(simd::subsatu(bl2MidY, bl2Y), constMaxInt);
    const uint8x16_t bl2GezMask = simd::cmpeqi(bl2NegDiffY, constZero);
    const uint8x16_t bl2AbsDiffY = simd::simd_or(bl2PosDiffY, bl2NegDiffY);
    const uint8x16_t bl2QThreshold = simd::replicateU2222(blQThreshold);
    const uint8x16_t bl2LqtMask = simd::cmplti(bl2AbsDiffY, bl2QThreshold);

    //  block 3
    const uint8x16x3_t bl3Di = simd::deinterleaveRGB(bl3);
    const uint8x16_t bl3Y = simd::avg(simd::avg(bl3Di.r0, bl3Di.r2), bl3Di.r1);
    const uint8x16_t bl3MidY = simd::replicateU3333(blMidY);
    const uint8x16_t bl3PosDiffY = simd::minu(simd::subsatu(bl3Y, bl3MidY), constMaxInt);
    const uint8x16_t bl3NegDiffY = simd::minu(simd::subsatu(bl3MidY, bl3Y), constMaxInt);
    const uint8x16_t bl3GezMask = simd::cmpeqi(bl3NegDiffY, constZero);
    const uint8x16_t bl3AbsDiffY = simd::simd_or(bl3PosDiffY, bl3NegDiffY);
    const uint8x16_t bl3QThreshold = simd::replicateU3333(blQThreshold);
    const uint8x16_t bl3LqtMask = simd::cmplti(bl3AbsDiffY, bl3QThreshold);

    // Finalize blocks
    // -----------------------------------------------------------
    if (CODEC_TYPE == GOOFY_DXT1)
    {
        // Generate DXT indices using given masks

        // DXT indices order
        // -------------------------
        //        C0(max)     C2        C3      C1(min)
        // DEC: |    0    |    2    |    3    |    1    |
        // BIN: |   00b   |   10b   |   11b   |   01b   |
        //
        //      |      GezMask      |
        //                          |      LqtMask      |

        // Zip two masks to match DX bits order
        // Gez0 | Lqt0 | Gez1 | Lqt1 | Gez2 | Lqt2 | Gez3 | Lqt3 | Gez4 | Lqt4 | Gez5 | Lqt5 | Gez6 | Lqt6 | Gez7 | Lqt7
        // Gez8 | Lqt8 | Gez9 | Lqt9 | GezA | LqtA | GezB | LqtB | GezC | LqtC | GezD | LqtD | GezE | LqtE | GezF | LqtF
        const uint8x16x2_t bl0RawIndices = simd::zipB16(simd::simd_not(bl0GezMask), bl0LqtMask);
        const uint8x16x2_t bl3RawIndices = simd::zipB16(simd::simd_not(bl3GezMask), bl3LqtMask);
        const uint8x16x2_t bl2RawIndices = simd::zipB16(simd::simd_not(bl2GezMask), bl2LqtMask);
        const uint8x16x2_t bl1RawIndices = simd::zipB16(simd::simd_not(bl1GezMask), bl1LqtMask);

        // Bytes to bits
        uint32_t bl0Indices = simd::moveMaskMSB(bl0RawIndices.r0) | (simd::moveMaskMSB(bl0RawIndices.r1) << 16);
        uint32_t bl1Indices = simd::moveMaskMSB(bl1RawIndices.r0) | (simd::moveMaskMSB(bl1RawIndices.r1) << 16);
        uint32_t bl2Indices = simd::moveMaskMSB(bl2RawIndices.r0) | (simd::moveMaskMSB(bl2RawIndices.r1) << 16);
        uint32_t bl3Indices = simd::moveMaskMSB(bl3RawIndices.r0) | (simd::moveMaskMSB(bl3RawIndices.r1) << 16);

        // Convert rgb888 to rgb555

        // We can't shift right by 3 using SIMD, but we can shift right by 1 three times instead
        // We need to sub eight before, because avg is (a+b+1) >> 1

        // max555_0.rgba | max555_1.rgba | max555_2.rgba | max555_3.rgba
        const uint8x16_t maxColors555 = simd::avg(simd::avg(simd::avg(simd::subsatu(maxColors, constEight), constZero), constZero), constZero);
        // min555_0.rgba | min555_1.rgba | min555_2.rgba | min555_3.rgba
        const uint8x16_t minColors555 = simd::avg(simd::avg(simd::avg(simd::subsatu(minColors, constEight), constZero), constZero), constZero);

        // max555_0.rgba | min555_0.rgba | max555_1.rgba | min555_1.rgba
        // max555_2.rgba | min555_2.rgba | max555_3.rgba | min555_3.rgba
        const uint8x16x2_t maxMinColors555 = simd::zipU4(maxColors555, minColors555);

        const uint64x2_t maxMin01 = simd::getAsUInt64x2(maxMinColors555.r0);
        const uint64x2_t maxMin23 = simd::getAsUInt64x2(maxMinColors555.r1);

        // R0
        // AAAAAAAA000000000000000000000000AAAAAAAA000000000000000000011111b << 11 = 0000000000000000 1111100000000000b
        // AAAAAAAA000000000000000000000000AAAAAAAA000000000001111100000000b >> 2  = 0000000000000000 0000011111000000b
        // AAAAAAAA000000000000000000000000AAAAAAAA000111110000000000000000b >> 16 = 0000000000000000 0000000000011111b

        // R1
        // AAAAAAAA000000000000000000011111AAAAAAAA000000000000000000000000b >> 5 =  1111100000000000 0000000000000000b
        // AAAAAAAA000000000001111100000000AAAAAAAA000000000000000000000000b >> 18 = 0000011111000000 0000000000000000b
        // AAAAAAAA000111110000000000000000AAAAAAAA000000000000000000000000b >> 32 = 0000000000011111 0000000000000000b

        // 0x20                                                                    = 0000000000000000 0000000000100000b

        uint32_t* goofy_restrict pDest = (uint32_t* goofy_restrict)pResult;

        uint32_t block0a = (uint32_t)(0x20 | // max color green channel LSB (to avoid switching to DXT1 3-color mode)
            (maxMin01.r0 & 0x1Full) << 0ull | (maxMin01.r0 & 0x1F00ull) >> 2ull | (maxMin01.r0 & 0x1F0000ull) >> 5ull |  // max color
            (maxMin01.r0 & 0x1F00000000ull) >> 16ull | (maxMin01.r0 & 0x1F0000000000ull) >> 18ull | (maxMin01.r0 & 0x1F000000000000ull) >> 21ull); // min color
        //uint32_t block0b = bl0Indices << 32ull;  // indices
        *pDest = block0a; pDest++; *pDest = bl0Indices;

        uint32_t block1a = (uint32_t)(0x20 |
            (maxMin01.r1 & 0x1Full) << 0ull | (maxMin01.r1 & 0x1F00ull) >> 2ull | (maxMin01.r1 & 0x1F0000ull) >> 5ull |  // max color
            (maxMin01.r1 & 0x1F00000000ull) >> 16ull | (maxMin01.r1 & 0x1F0000000000ull) >> 18ull | (maxMin01.r1 & 0x1F000000000000ull) >> 21ull); // min color
        //uint32_t block1b = bl1Indices << 32ull;
        pDest++; *pDest = block1a; pDest++; *pDest = bl1Indices;

        uint32_t block2a = (uint32_t)(0x20 |
            (maxMin23.r0 & 0x1Full) << 0ull | (maxMin23.r0 & 0x1F00ull) >> 2ull | (maxMin23.r0 & 0x1F0000ull) >> 5ull |  // max color
            (maxMin23.r0 & 0x1F00000000ull) >> 16ull | (maxMin23.r0 & 0x1F0000000000ull) >> 18ull | (maxMin23.r0 & 0x1F000000000000ull) >> 21ull); // min color
            //bl2Indices << 32ull;
        pDest++; *pDest = block2a; pDest++; *pDest = bl2Indices;

        uint32_t block3a = (uint32_t)(0x20 |
            (maxMin23.r1 & 0x1Full) << 0ull | (maxMin23.r1 & 0x1F00ull) >> 2ull | (maxMin23.r1 & 0x1F0000ull) >> 5ull |  // max color
            (maxMin23.r1 & 0x1F00000000ull) >> 16ull | (maxMin23.r1 & 0x1F0000000000ull) >> 18ull | (maxMin23.r1 & 0x1F000000000000ull) >> 21ull); // min color
            //bl3Indices << 32ull;
        pDest++; *pDest = block3a; pDest++; *pDest = bl3Indices;
    }
    #if SUPPORT_ETC1
    else
    //else if (CODEC_TYPE == GOOFY_ETC1)
    {
        // Combined masks (major bit = GreaterEqualZero  other 7 bits = LessQuantizationThreshold)
        const uint8x16x4_t blMasks = {
            simd::simd_or(simd::simd_andnot(constMaxInt, bl0GezMask), simd::simd_and(bl0LqtMask, constMaxInt)),
            simd::simd_or(simd::simd_andnot(constMaxInt, bl1GezMask), simd::simd_and(bl1LqtMask, constMaxInt)),
            simd::simd_or(simd::simd_andnot(constMaxInt, bl2GezMask), simd::simd_and(bl2LqtMask, constMaxInt)),
            simd::simd_or(simd::simd_andnot(constMaxInt, bl3GezMask), simd::simd_and(bl3LqtMask, constMaxInt))
        };

        //  +---+---+---+---+           +---+---+---+---+
        //  | A | B | C | D |           | C | G | K | O |
        //  +---+---+---+---+           +---+---+---+---+
        //  | E | F | G | H |           | D | H | L | P |
        //  +---+---+---+---+    -->    +---+---+---+---+
        //  | I | J | K | L |           | A | E | I | M |
        //  +---+---+---+---+           +---+---+---+---+
        //  | M | N | O | P |           | B | F | J | N |
        //  +---+---+---+---+           +---+---+---+---+
        const uint8x16x4_t blMasksTr = simd::transposeAs4x4x4(blMasks);

        // Unpack masks and copy from bytes to bits
        const uint32_t bl0PosOrZero = simd::moveMaskMSB(blMasksTr.r0);
        const uint32_t bl1PosOrZero = simd::moveMaskMSB(blMasksTr.r1);
        const uint32_t bl2PosOrZero = simd::moveMaskMSB(blMasksTr.r2);
        const uint32_t bl3PosOrZero = simd::moveMaskMSB(blMasksTr.r3);

        uint8x16_t bl0LessThanQtMask = simd::simd_and(blMasksTr.r0, constMaxInt);
        uint8x16_t bl1LessThanQtMask = simd::simd_and(blMasksTr.r1, constMaxInt);
        uint8x16_t bl2LessThanQtMask = simd::simd_and(blMasksTr.r2, constMaxInt);
        uint8x16_t bl3LessThanQtMask = simd::simd_and(blMasksTr.r3, constMaxInt);
        bl0LessThanQtMask = simd::addsatu(bl0LessThanQtMask, bl0LessThanQtMask);
        bl1LessThanQtMask = simd::addsatu(bl1LessThanQtMask, bl1LessThanQtMask);
        bl2LessThanQtMask = simd::addsatu(bl2LessThanQtMask, bl2LessThanQtMask);
        bl3LessThanQtMask = simd::addsatu(bl3LessThanQtMask, bl3LessThanQtMask);

        const uint32_t bl0LessThanQt = simd::moveMaskMSB(bl0LessThanQtMask);
        const uint32_t bl1LessThanQt = simd::moveMaskMSB(bl1LessThanQtMask);
        const uint32_t bl2LessThanQt = simd::moveMaskMSB(bl2LessThanQtMask);
        const uint32_t bl3LessThanQt = simd::moveMaskMSB(bl3LessThanQtMask);

#if 1
        // Keep chromatic component from the average color, but override brightness
        // NOTE: This is slightly slower but gets slightly better quality

        // Find average blocks color
        const uint8x16x4_t blAvg = {
            simd::avg(simd::avg(bl0.r0, bl0.r1), simd::avg(bl0.r2, bl0.r3)),
            simd::avg(simd::avg(bl1.r0, bl1.r1), simd::avg(bl1.r2, bl1.r3)),
            simd::avg(simd::avg(bl2.r0, bl2.r1), simd::avg(bl2.r2, bl2.r3)),
            simd::avg(simd::avg(bl3.r0, bl3.r1), simd::avg(bl3.r2, bl3.r3))
        };

        const uint8x16x4_t blAvgTr = simd::transposeAs4x4(blAvg);

        const uint8x16_t blAvgColors = simd::avg(
            simd::avg(blAvgTr.r0, blAvgTr.r1),
            simd::avg(blAvgTr.r2, blAvgTr.r3)
        );

        // Note: a lot of SSE lanes wasted, it is not ideal, TODO?
        //
        // avg0.rgba | avg0.rgba | avg1.rgba | avg1.rgba
        // avg2.rgba | avg2.rgba | avg3.rgba | avg3.rgba
        // avg0.rgba | avg0.rgba | avg1.rgba | avg1.rgba
        // avg2.rgba | avg2.rgba | avg3.rgba | avg3.rgba
        const uint8x16x4_t blAvg4 = simd::zipU4x2(blAvgColors, blAvgColors, blAvgColors, blAvgColors);

        // Deinterleave
        // avg0.rr | avg1.rr | avg2.rr | avg3.rr | avg0.rr | avg1.rr | avg2.rr | avg3.rr
        // avg0.gg | avg1.gg | avg2.gg | avg3.gg | avg0.gg | avg1.gg | avg2.gg | avg3.gg
        // avg0.bb | avg1.bb | avg2.bb | avg3.bb | avg0.bb | avg1.bb | avg2.bb | avg3.bb
        const uint8x16x3_t blAvg4Di = simd::deinterleaveRGB(blAvg4);

        // Y = avg0.yy | avg1.yy | avg2.yy | avg3.yy | avg0.yy | avg1.yy | avg2.yy | avg3.yy
        const uint8x16_t Y = simd::avg(simd::avg(blAvg4Di.r0, blAvg4Di.r2), blAvg4Di.r1);

        // Min/max brightness per block
        // R0 = avg0.yyyy | avg1.yyyy | avg2.yyyy | avg3.yyyy
        // R1 = avg0.yyyy | avg1.yyyy | avg2.yyyy | avg3.yyyy    // NOTE: not used!
        const uint8x16x2_t blAvgY = simd::zipB16(Y, Y);

        const uint8x16_t blPosCorrectionY = simd::minu(simd::subsatu(blMidY, blAvgY.r0), constMaxInt);
        const uint8x16_t blNegCorrectionY = simd::minu(simd::subsatu(blAvgY.r0, blMidY), constMaxInt);
        const uint8x16_t blCorrectionYGezMask = simd::cmpeqi(blNegCorrectionY, constZero);
        const uint8x16_t blCorrectionYAbs = simd::simd_or(blPosCorrectionY, blNegCorrectionY);

        // Get the color in the middle between  min/max colors of the block.
        // NOTE: this is not the same as an average block color.

        const uint8x16_t blBaseColorsPos = simd::addsatu(blAvgColors, blCorrectionYAbs);
        const uint8x16_t blBaseColorsNeg = simd::subsatu(blAvgColors, blCorrectionYAbs);

        const uint8x16_t blBaseColors = simd::select(blCorrectionYGezMask, blBaseColorsPos, blBaseColorsNeg);
#else
        // Get the color in the middle between  min/max colors of the block.
        // NOTE: this is not the same as an average block color.
        const uint8x16_t blBaseColors = simd::avg(minColors, maxColors);
#endif

        // Convert rgb888 to rgb555

        // We can't shift right by 3 using SIMD, but we can shift right by 1 three times instead
        // We need to sub eight before, because avg is (a+b+1) >> 1

        // mid555_0.rgba | mid555_1.rgba | mid555_2.rgba | mid555_3.rgba
        const uint8x16_t baseColors555 = simd::avg(simd::avg(simd::avg(simd::subsatu(blBaseColors, constEight), constZero), constZero), constZero);

        const uint64x2_t baseColors = simd::getAsUInt64x2(baseColors555);

        // R0
        // AAAAAAAA000000000000000000000000AAAAAAAA000000000000000000011111b << 3  = 00000000 00000000 11111000b
        // AAAAAAAA000000000000000000000000AAAAAAAA000000000001111100000000b << 3  = 00000000 11111000 00000000b
        // AAAAAAAA000000000000000000000000AAAAAAAA000111110000000000000000b << 3  = 11111000 00000000 00000000b

        // R1
        // AAAAAAAA000000000000000000011111AAAAAAAA000000000000000000000000b >> 29 = 00000000 00000000 11111000b
        // AAAAAAAA000000000001111100000000AAAAAAAA000000000000000000000000b >> 29 = 00000000 11111000 00000000b
        // AAAAAAAA000111110000000000000000AAAAAAAA000000000000000000000000b >> 29 = 11111000 00000000 00000000b

        uint32_t* goofy_restrict pDest = (uint32_t* goofy_restrict)pResult;

        const uint32_t block0a = etc1BrighnessRangeTocontrolByte[blRangeY.m128i_u8[0]] | ((baseColors.r0 << 3ull) & 0xFFFFFF);
        const uint32_t block0b = ~(bl0PosOrZero | (bl0LessThanQt << 16));
        *pDest = block0a; pDest++; *pDest = block0b;

        const uint32_t block1a = etc1BrighnessRangeTocontrolByte[blRangeY.m128i_u8[4]] | ((baseColors.r0 >> 29ull) & 0xFFFFFF);
        const uint32_t block1b = ~(bl1PosOrZero | (bl1LessThanQt << 16));
        pDest++; *pDest = block1a; pDest++; *pDest = block1b;

        const uint32_t block2a = etc1BrighnessRangeTocontrolByte[blRangeY.m128i_u8[8]] | ((baseColors.r1 << 3ull) & 0xFFFFFF);
        const uint32_t block2b = ~(bl2PosOrZero | (bl2LessThanQt << 16));
        pDest++; *pDest = block2a; pDest++; *pDest = block2b;

        const uint32_t block3a = etc1BrighnessRangeTocontrolByte[blRangeY.m128i_u8[12]] | ((baseColors.r1 >> 29ull) & 0xFFFFFF);
        const uint32_t block3b = ~(bl3PosOrZero | (bl3LessThanQt << 16));
        pDest++; *pDest = block3a; pDest++; *pDest = block3b;
    }
    #else
      static_assert(CODEC_TYPE == GOOFY_DXT1, "unsupported codec");
    #endif
}

//
// Encode 4 DXT1/ETC1 at once
//
goofy_inline void goofySimdEncodeDXT5(const unsigned char* goofy_restrict inputRGBA, size_t inputStride, unsigned char* goofy_restrict pResult)
{
    // Fetch 16x4 pixels from the buffer(four DX blocks)
    // 16 pixels wide is better for the CPU cache utilization (64 bytes per line) and it is better for SIMD lane utilization
    // -----------------------------------------------------------
    uint8x16x4_t bl0;
    uint8x16x4_t bl1;
    uint8x16x4_t bl2;
    uint8x16x4_t bl3;
    bl0.r0 = simd::fetch(inputRGBA);
    bl1.r0 = simd::fetch(inputRGBA + 16);
    bl2.r0 = simd::fetch(inputRGBA + 32);
    bl3.r0 = simd::fetch(inputRGBA + 48);
    inputRGBA += inputStride;
    bl0.r1 = simd::fetch(inputRGBA);
    bl1.r1 = simd::fetch(inputRGBA + 16);
    bl2.r1 = simd::fetch(inputRGBA + 32);
    bl3.r1 = simd::fetch(inputRGBA + 48);
    inputRGBA += inputStride;
    bl0.r2 = simd::fetch(inputRGBA);
    bl1.r2 = simd::fetch(inputRGBA + 16);
    bl2.r2 = simd::fetch(inputRGBA + 32);
    bl3.r2 = simd::fetch(inputRGBA + 48);
    inputRGBA += inputStride;
    bl0.r3 = simd::fetch(inputRGBA);
    bl1.r3 = simd::fetch(inputRGBA + 16);
    bl2.r3 = simd::fetch(inputRGBA + 32);
    bl3.r3 = simd::fetch(inputRGBA + 48);

    // Find min block colors
    // -----------------------------------------------------------
    const uint8x16x4_t blMin = {
        simd::minu(simd::minu(bl0.r0, bl0.r1), simd::minu(bl0.r2, bl0.r3)), // min0_clmn0.rgba | min0_clmn1.rgba | min0_clmn2.rgba | min0_clmn3.rgba
        simd::minu(simd::minu(bl1.r0, bl1.r1), simd::minu(bl1.r2, bl1.r3)), // min1_clmn0.rgba | min1_clmn1.rgba | min1_clmn2.rgba | min1_clmn3.rgba
        simd::minu(simd::minu(bl2.r0, bl2.r1), simd::minu(bl2.r2, bl2.r3)), // min2_clmn0.rgba | min2_clmn1.rgba | min2_clmn2.rgba | min2_clmn3.rgba
        simd::minu(simd::minu(bl3.r0, bl3.r1), simd::minu(bl3.r2, bl3.r3))  // min3_clmn0.rgba | min3_clmn1.rgba | min3_clmn2.rgba | min3_clmn3.rgba
    };

    // blMinTr (transposed blMin)
    // min0_clmn0.rgba | min1_clmn0.rgba | min2_clmn0.rgba | min3_clmn0.rgba
    // min0_clmn1.rgba | min1_clmn1.rgba | min2_clmn1.rgba | min3_clmn1.rgba
    // min0_clmn2.rgba | min1_clmn2.rgba | min2_clmn2.rgba | min3_clmn2.rgba
    // min0_clmn3.rgba | min1_clmn3.rgba | min2_clmn3.rgba | min3_clmn3.rgba
    const uint8x16x4_t blMinTr = simd::transposeAs4x4(blMin);

    // Per-block min colors
    // min0.rgba | min1.rgba | min2.rgba | min3.rgba
    const uint8x16_t minColors = simd::minu(
        simd::minu(blMinTr.r0, blMinTr.r1),
        simd::minu(blMinTr.r2, blMinTr.r3)
    );

    // Same to find max block colors
    // -----------------------------------------------------------
    const uint8x16x4_t blMax = {
        simd::maxu(simd::maxu(bl0.r0, bl0.r1), simd::maxu(bl0.r2, bl0.r3)),
        simd::maxu(simd::maxu(bl1.r0, bl1.r1), simd::maxu(bl1.r2, bl1.r3)),
        simd::maxu(simd::maxu(bl2.r0, bl2.r1), simd::maxu(bl2.r2, bl2.r3)),
        simd::maxu(simd::maxu(bl3.r0, bl3.r1), simd::maxu(bl3.r2, bl3.r3))
    };

    const uint8x16x4_t blMaxTr = simd::transposeAs4x4(blMax);

    // Per-block max colors
    // max0.rgba | max1.rgba | max2.rgba | max3.rgba
    const uint8x16_t maxColors = simd::maxu(
        simd::maxu(blMaxTr.r0, blMaxTr.r1),
        simd::maxu(blMaxTr.r2, blMaxTr.r3)
    );

    // Find min/max brigtness
    // -----------------------------------------------------------

    // Note: some SSE lanes wasted, it is not ideal, but seems OK-ish?
    //
    // min0.rgba | min0.rgba | min1.rgba | min1.rgba
    // min2.rgba | min2.rgba | min3.rgba | min3.rgba
    // max0.rgba | max0.rgba | max1.rgba | max1.rgba
    // max2.rgba | max2.rgba | max3.rgba | max3.rgba
    const uint8x16x4_t blMinMax = simd::zipU4x2(minColors, minColors, maxColors, maxColors);

    // Deinterleave
    // min0.rr | min1.rr | min2.rr | min3.rr | max0.rr | max1.rr | max2.rr | max3.rr
    // min0.gg | min1.gg | min2.gg | min3.gg | max0.gg | max1.gg | max2.gg | max3.gg
    // min0.bb | min1.bb | min2.bb | min3.bb | max0.bb | max1.bb | max2.bb | max3.bb
    // min0.aa | min1.aa | min2.aa | min3.aa | max0.aa | max1.aa | max2.aa | max3.aa
    const uint8x16x4_t blMinMaxDi = simd::deinterleaveRGBA(blMinMax);
    #define _SHUFFLE_FWD(maskX, maskY, maskZ, maskW) _MM_SHUFFLE(maskW, maskZ, maskY, maskX)
    #define _SHUFFLE_ALL(maskX) _MM_SHUFFLE(maskX, maskX, maskX, maskX)
    const uint8x16_t constZero = simd::zero();
    __m128i minAlpha16 = _mm_unpacklo_epi8(blMinMaxDi.r3, constZero);
    minAlpha16 = _mm_unpacklo_epi32(
                             _mm_shufflelo_epi16(minAlpha16, _SHUFFLE_FWD(0,2,0,2)),
                              _mm_unpackhi_epi64(_mm_shufflehi_epi16(minAlpha16, _SHUFFLE_FWD(0,2,0,2)),minAlpha16));
    __m128i maxAlpha16 = _mm_unpackhi_epi8(blMinMaxDi.r3, constZero);
    maxAlpha16 = _mm_unpacklo_epi32(
                             _mm_shufflelo_epi16(maxAlpha16, _SHUFFLE_FWD(0,2,0,2)),
                              _mm_unpackhi_epi64(_mm_shufflehi_epi16(maxAlpha16, _SHUFFLE_FWD(0,2,0,2)),maxAlpha16));
    //blMinMaxDi.r3

    // Get Y component of YCoCg color-model (perceptual brightness)
    // https://en.wikipedia.org/wiki/YCoCg
    // Y = 0.25 * R + 0.25 * B + 0.5 * G
    //   We can rewrite equation above using the following form
    // Y = (((R + B) / 2) + G) / 2
    //
    // Y = min0.yy | min1.yy | min2.yy | min3.yy | max0.yy | max1.yy | max2.yy | max3.yy
    const uint8x16_t Y = simd::avg(simd::avg(blMinMaxDi.r0, blMinMaxDi.r2), blMinMaxDi.r1);

    // Min/max brightness per block
    // R0 = min0.yyyy | min1.yyyy | min2.yyyy | min3.yyyy
    // R1 = max0.yyyy | max1.yyyy | max2.yyyy | max3.yyyy
    const uint8x16x2_t blMinMaxY = simd::zipB16(Y, Y);

    // Clamp to min brightness
    const uint8x16_t constEight = simd::fetch(&gConstEight);
    // range0.yyyy | range1.yyyy | range2.yyyy | range3.yyyy
    const uint8x16_t blRangeY = simd::maxu(simd::subsatu(blMinMaxY.r1, blMinMaxY.r0), constEight);

    // mid0.yyyy | mid1.yyyy | mid2.yyyy | mid3.yyyy
    const uint8x16_t blMidY = simd::avg(blMinMaxY.r0, blMinMaxY.r1);

    // Approximate multiplication by 0.375 to get quantization thresholds

    const uint8x16_t blHalfRangeY = simd::avg(blRangeY, constZero);
    const uint8x16_t blQuarterRangeY = simd::avg(blHalfRangeY, constZero);
    const uint8x16_t blEighthsRangeY = simd::avg(blQuarterRangeY, constZero);

    // Threshold = (quarter + eights) = (0.25 + 0.125) ~= (range * 0.375)
    // qt0.yyyy | qt1.yyyy | qt2.yyyy | qt3.yyyy
    const uint8x16_t blQThreshold = simd::addsatu(blQuarterRangeY, blEighthsRangeY);

    // Quantization (generate indices)
    // -----------------------------------------------------------
    const uint8x16_t constMaxInt = simd::fetch(&gConstMaxInt);

    //  block 0
    //
    // p0.r p1.r p2.r p3.r p4.r p5.r p6.r p7.r p8.r p9.r p10.r p11.r p12.r p13.r p14.r p15.r
    // p0.g p1.g p2.g p3.g p4.g p5.g p6.g p7.g p8.g p9.g p10.g p11.g p12.g p13.g p14.g p15.g
    // p0.b p1.b p2.b p3.b p4.b p5.b p6.b p7.b p8.b p9.b p10.b p11.b p12.b p13.b p14.b p15.b
    // p0.a p1.a p2.a p3.a p4.a p5.a p6.a p7.a p8.a p9.a p10.a p11.a p12.a p13.a p14.a p15.a
    const uint8x16x4_t bl0Di = simd::deinterleaveRGBA(bl0);

    // Convert RGB to brightness
    // per-pixel block brightness
    const uint8x16_t bl0Y = simd::avg(simd::avg(bl0Di.r0, bl0Di.r2), bl0Di.r1);

    // Block brightness to compare with
    const uint8x16_t bl0MidY = simd::replicateU0000(blMidY);

    // Brightness difference (per-pixel in block)
    // NOTE: we need to clamp difference to max signed int8, because of the signed comparison later
    const uint8x16_t bl0PosDiffY = simd::minu(simd::subsatu(bl0Y, bl0MidY), constMaxInt);
    const uint8x16_t bl0NegDiffY = simd::minu(simd::subsatu(bl0MidY, bl0Y), constMaxInt);
    // Greater or Equal to zero mask
    const uint8x16_t bl0GezMask = simd::cmpeqi(bl0NegDiffY, constZero);

    // Absolute diffference of brightness (per-pixel in block)
    const uint8x16_t bl0AbsDiffY = simd::simd_or(bl0PosDiffY, bl0NegDiffY);

    // get quantization threshold for current block
    const uint8x16_t bl0QThreshold = simd::replicateU0000(blQThreshold);

    // Less than Quantization Threshold mask
    const uint8x16_t bl0LqtMask = simd::cmplti(bl0AbsDiffY, bl0QThreshold);

    // Here we've got two bitmasks
    //
    // GezMask = greater or equal than zero (per pixel)
    // LqtMask = less than quantization threshold (per pixel)
    //
    //
    //  min       qt          qt        max
    //   x---------x-----+-----x---------x
    //                   0
    //
    //                   |---------------| greater or equal than zero (GezMask)
    //
    //             |-----------| less than quantization threshold (LqtMask)
    //

    //  block 1
    const uint8x16x4_t bl1Di = simd::deinterleaveRGBA(bl1);
    const uint8x16_t bl1Y = simd::avg(simd::avg(bl1Di.r0, bl1Di.r2), bl1Di.r1);
    const uint8x16_t bl1MidY = simd::replicateU1111(blMidY);
    const uint8x16_t bl1PosDiffY = simd::minu(simd::subsatu(bl1Y, bl1MidY), constMaxInt);
    const uint8x16_t bl1NegDiffY = simd::minu(simd::subsatu(bl1MidY, bl1Y), constMaxInt);
    const uint8x16_t bl1GezMask = simd::cmpeqi(bl1NegDiffY, constZero);
    const uint8x16_t bl1AbsDiffY = simd::simd_or(bl1PosDiffY, bl1NegDiffY);
    const uint8x16_t bl1QThreshold = simd::replicateU1111(blQThreshold);
    const uint8x16_t bl1LqtMask = simd::cmplti(bl1AbsDiffY, bl1QThreshold);

    //  block 2
    const uint8x16x4_t bl2Di = simd::deinterleaveRGBA(bl2);
    const uint8x16_t bl2Y = simd::avg(simd::avg(bl2Di.r0, bl2Di.r2), bl2Di.r1);
    const uint8x16_t bl2MidY = simd::replicateU2222(blMidY);
    const uint8x16_t bl2PosDiffY = simd::minu(simd::subsatu(bl2Y, bl2MidY), constMaxInt);
    const uint8x16_t bl2NegDiffY = simd::minu(simd::subsatu(bl2MidY, bl2Y), constMaxInt);
    const uint8x16_t bl2GezMask = simd::cmpeqi(bl2NegDiffY, constZero);
    const uint8x16_t bl2AbsDiffY = simd::simd_or(bl2PosDiffY, bl2NegDiffY);
    const uint8x16_t bl2QThreshold = simd::replicateU2222(blQThreshold);
    const uint8x16_t bl2LqtMask = simd::cmplti(bl2AbsDiffY, bl2QThreshold);

    //  block 3
    const uint8x16x4_t bl3Di = simd::deinterleaveRGBA(bl3);
    const uint8x16_t bl3Y = simd::avg(simd::avg(bl3Di.r0, bl3Di.r2), bl3Di.r1);
    const uint8x16_t bl3MidY = simd::replicateU3333(blMidY);
    const uint8x16_t bl3PosDiffY = simd::minu(simd::subsatu(bl3Y, bl3MidY), constMaxInt);
    const uint8x16_t bl3NegDiffY = simd::minu(simd::subsatu(bl3MidY, bl3Y), constMaxInt);
    const uint8x16_t bl3GezMask = simd::cmpeqi(bl3NegDiffY, constZero);
    const uint8x16_t bl3AbsDiffY = simd::simd_or(bl3PosDiffY, bl3NegDiffY);
    const uint8x16_t bl3QThreshold = simd::replicateU3333(blQThreshold);
    const uint8x16_t bl3LqtMask = simd::cmplti(bl3AbsDiffY, bl3QThreshold);

    // Finalize blocks
    // -----------------------------------------------------------
    {
        // Generate DXT indices using given masks

        // DXT indices order
        // -------------------------
        //        C0(max)     C2        C3      C1(min)
        // DEC: |    0    |    2    |    3    |    1    |
        // BIN: |   00b   |   10b   |   11b   |   01b   |
        //
        //      |      GezMask      |
        //                          |      LqtMask      |

        // Zip two masks to match DX bits order
        // Gez0 | Lqt0 | Gez1 | Lqt1 | Gez2 | Lqt2 | Gez3 | Lqt3 | Gez4 | Lqt4 | Gez5 | Lqt5 | Gez6 | Lqt6 | Gez7 | Lqt7
        // Gez8 | Lqt8 | Gez9 | Lqt9 | GezA | LqtA | GezB | LqtB | GezC | LqtC | GezD | LqtD | GezE | LqtE | GezF | LqtF
        const uint8x16x2_t bl0RawIndices = simd::zipB16(simd::simd_not(bl0GezMask), bl0LqtMask);
        const uint8x16x2_t bl3RawIndices = simd::zipB16(simd::simd_not(bl3GezMask), bl3LqtMask);
        const uint8x16x2_t bl2RawIndices = simd::zipB16(simd::simd_not(bl2GezMask), bl2LqtMask);
        const uint8x16x2_t bl1RawIndices = simd::zipB16(simd::simd_not(bl1GezMask), bl1LqtMask);

        // Bytes to bits
        uint32_t bl0Indices = simd::moveMaskMSB(bl0RawIndices.r0) | (simd::moveMaskMSB(bl0RawIndices.r1) << 16);
        uint32_t bl1Indices = simd::moveMaskMSB(bl1RawIndices.r0) | (simd::moveMaskMSB(bl1RawIndices.r1) << 16);
        uint32_t bl2Indices = simd::moveMaskMSB(bl2RawIndices.r0) | (simd::moveMaskMSB(bl2RawIndices.r1) << 16);
        uint32_t bl3Indices = simd::moveMaskMSB(bl3RawIndices.r0) | (simd::moveMaskMSB(bl3RawIndices.r1) << 16);

        // Convert rgb888 to rgb555

        // We can't shift right by 3 using SIMD, but we can shift right by 1 three times instead
        // We need to sub eight before, because avg is (a+b+1) >> 1

        // max555_0.rgba | max555_1.rgba | max555_2.rgba | max555_3.rgba
        const uint8x16_t maxColors555 = simd::avg(simd::avg(simd::avg(simd::subsatu(maxColors, constEight), constZero), constZero), constZero);
        // min555_0.rgba | min555_1.rgba | min555_2.rgba | min555_3.rgba
        const uint8x16_t minColors555 = simd::avg(simd::avg(simd::avg(simd::subsatu(minColors, constEight), constZero), constZero), constZero);

        // max555_0.rgba | min555_0.rgba | max555_1.rgba | min555_1.rgba
        // max555_2.rgba | min555_2.rgba | max555_3.rgba | min555_3.rgba
        const uint8x16x2_t maxMinColors555 = simd::zipU4(maxColors555, minColors555);

        const uint64x2_t maxMin01 = simd::getAsUInt64x2(maxMinColors555.r0);
        const uint64x2_t maxMin23 = simd::getAsUInt64x2(maxMinColors555.r1);

        // R0
        // AAAAAAAA000000000000000000000000AAAAAAAA000000000000000000011111b << 11 = 0000000000000000 1111100000000000b
        // AAAAAAAA000000000000000000000000AAAAAAAA000000000001111100000000b >> 2  = 0000000000000000 0000011111000000b
        // AAAAAAAA000000000000000000000000AAAAAAAA000111110000000000000000b >> 16 = 0000000000000000 0000000000011111b

        // R1
        // AAAAAAAA000000000000000000011111AAAAAAAA000000000000000000000000b >> 5 =  1111100000000000 0000000000000000b
        // AAAAAAAA000000000001111100000000AAAAAAAA000000000000000000000000b >> 18 = 0000011111000000 0000000000000000b
        // AAAAAAAA000111110000000000000000AAAAAAAA000000000000000000000000b >> 32 = 0000000000011111 0000000000000000b

        uint32_t* goofy_restrict pDest = (uint32_t* goofy_restrict)pResult;
        emitDXT5_alpha_block(bl0Di.r3, minAlpha16, maxAlpha16, pDest);pDest+=2;

        uint32_t block0a = (uint32_t)(
            (maxMin01.r0 & 0x1Full) << 0ull | (maxMin01.r0 & 0x1F00ull) >> 2ull | (maxMin01.r0 & 0x1F0000ull) >> 5ull |  // max color
            (maxMin01.r0 & 0x1F00000000ull) >> 16ull | (maxMin01.r0 & 0x1F0000000000ull) >> 18ull | (maxMin01.r0 & 0x1F000000000000ull) >> 21ull); // min color
        //uint32_t block0b = bl0Indices << 32ull;  // indices
        *pDest = block0a; pDest++; *pDest = bl0Indices;

        pDest++;
        emitDXT5_alpha_block(bl1Di.r3, _mm_shufflelo_epi16(minAlpha16, _SHUFFLE_ALL(1)), _mm_shufflelo_epi16(maxAlpha16, _SHUFFLE_ALL(1)), pDest);pDest+=2;
        uint32_t block1a = (uint32_t)(
            (maxMin01.r1 & 0x1Full) << 0ull | (maxMin01.r1 & 0x1F00ull) >> 2ull | (maxMin01.r1 & 0x1F0000ull) >> 5ull |  // max color
            (maxMin01.r1 & 0x1F00000000ull) >> 16ull | (maxMin01.r1 & 0x1F0000000000ull) >> 18ull | (maxMin01.r1 & 0x1F000000000000ull) >> 21ull); // min color
        //uint32_t block1b = bl1Indices << 32ull;
        *pDest = block1a; pDest++; *pDest = bl1Indices;

        uint32_t block2a = (uint32_t)(
            (maxMin23.r0 & 0x1Full) << 0ull | (maxMin23.r0 & 0x1F00ull) >> 2ull | (maxMin23.r0 & 0x1F0000ull) >> 5ull |  // max color
            (maxMin23.r0 & 0x1F00000000ull) >> 16ull | (maxMin23.r0 & 0x1F0000000000ull) >> 18ull | (maxMin23.r0 & 0x1F000000000000ull) >> 21ull); // min color
        pDest++;
        emitDXT5_alpha_block(bl2Di.r3, _mm_shufflelo_epi16(minAlpha16, _SHUFFLE_ALL(2)), _mm_shufflelo_epi16(maxAlpha16, _SHUFFLE_ALL(2)), pDest);pDest+=2;
        *pDest = block2a; pDest++; *pDest = bl2Indices;

        pDest++;
        emitDXT5_alpha_block(bl3Di.r3, _mm_shufflelo_epi16(minAlpha16, _SHUFFLE_ALL(3)), _mm_shufflelo_epi16(maxAlpha16, _SHUFFLE_ALL(3)), pDest);pDest+=2;
        uint32_t block3a = (uint32_t)(
            (maxMin23.r1 & 0x1Full) << 0ull | (maxMin23.r1 & 0x1F00ull) >> 2ull | (maxMin23.r1 & 0x1F0000ull) >> 5ull |  // max color
            (maxMin23.r1 & 0x1F00000000ull) >> 16ull | (maxMin23.r1 & 0x1F0000000000ull) >> 18ull | (maxMin23.r1 & 0x1F000000000000ull) >> 21ull); // min color
            //bl3Indices << 32ull;
        *pDest = block3a; pDest++; *pDest = bl3Indices;
    }
    #undef _SHUFFLE_FWD
    #undef _SHUFFLE_ALL
}


int compressDXT1(unsigned char* result, const unsigned char* input, unsigned int width, unsigned int height, unsigned int stride)
{
    // those checks are required because of 4x1 block window inside the compressor
    if (width % 16 != 0)
    {
        return -1;
    }

    if (height % 4 != 0)
    {
        return -2;
    }

    unsigned int blockW = width >> 2;
    unsigned int blockH = height >> 2;

    size_t inputStride = stride;
    for (uint32_t y = 0; y < blockH; y++)
    {
        const unsigned char* goofy_restrict encoderPos = input;
        for (uint32_t x = 0; x < blockW; x += 4)
        {
            goofySimdEncode<GOOFY_DXT1>(encoderPos, inputStride, result);
            encoderPos += 64; // 16 rgba pixels (4 DXT blocks) = 16 * 4 = 64
            result += 32;     // 4 DXT1 blocks = 8 * 4 = 32
        }
        input += inputStride * 4; // 4 lines
    }
    return 0;
}

#if SUPPORT_ETC1
int compressETC1(unsigned char* result, const unsigned char* input, unsigned int width, unsigned int height, unsigned int stride)
{
    // those checks are required because of 4x1 block window inside the compressor
    if (width % 16 != 0)
    {
        return -1;
    }

    if (height % 4 != 0)
    {
        return -2;
    }

    unsigned int blockW = width >> 2;
    unsigned int blockH = height >> 2;

    size_t inputStride = stride;
    for (uint32_t y = 0; y < blockH; y++)
    {
        const unsigned char* goofy_restrict encoderPos = input;
        for (uint32_t x = 0; x < blockW; x += 4)
        {
            goofySimdEncode<GOOFY_ETC1>(encoderPos, inputStride, result);
            encoderPos += 64; // 16 rgba pixels (4 DXT blocks) = 16 * 4 = 64
            result += 32;     // 4 DXT1 blocks = 8 * 4 = 32
        }
        input += inputStride * 4; // 4 lines
    }
    return 0;
}
#endif


#undef goofy_restrict
#undef goofy_inline
#undef goofy_align16
}
#endif





// Copyright (c) 2020 Sergey Makeev <sergeymakeev@hotmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to	deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
