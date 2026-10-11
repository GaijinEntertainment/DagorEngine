#include "daScript/misc/platform.h"

#include "daScript/misc/float2string.h"

#include "misc/include_fmt.h"

#include <cstring>

// the digit and layout emitter below is borrowed from the Luau programming language
// (VM/src/lnumprint.cpp), MIT License - see LUAU.LICENSE beside this file; that code is
// based on the Lua 5.x implementation, whose notice LUAU.LICENSE carries too. The digits
// come from fmt's dragonbox and the fixed/scientific crossover is fmt's, so the spelling
// is byte-identical to the fmt::format_to("{}", x) it replaces.

namespace das {

    static const char kDigitTable[] = "0001020304050607080910111213141516171819202122232425262728293031323334353637383940414243444546474849"
                                      "5051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";

    enum { kDigitRoom = 24, kOverreadSlack = 24 };

    template <typename TT> struct F2SBits;
    template <> struct F2SBits<float>  { typedef uint32_t U;  static const uint32_t expMask = 0x7f800000u; };
    template <> struct F2SBits<double> { typedef uint64_t U;  static const uint64_t expMask = 0x7ff0000000000000ull; };

    static char * printunsignedrev ( char * end, uint64_t num ) {
        while ( num >= 10000 ) {
            unsigned int tail = unsigned(num % 10000);
            memcpy(end - 4, &kDigitTable[int(tail / 100) * 2], 2);
            memcpy(end - 2, &kDigitTable[int(tail % 100) * 2], 2);
            num /= 10000;
            end -= 4;
        }
        unsigned int rest = unsigned(num);
        while ( rest >= 10 ) {
            memcpy(end - 2, &kDigitTable[int(rest % 100) * 2], 2);
            rest /= 100;
            end -= 2;
        }
        if ( rest ) {
            end[-1] = char('0' + int(rest));
            end -= 1;
        }
        return end;
    }

    static char * printexp ( char * buf, int num ) {
        *buf++ = 'e';
        *buf++ = num < 0 ? '-' : '+';
        int v = num < 0 ? -num : num;
        if ( v >= 100 ) {
            *buf++ = char('0' + (v / 100));
            v %= 100;
        }
        memcpy(buf, &kDigitTable[v * 2], 2);
        return buf + 2;
    }

#define DAS_F2S_MEMCPY(dst, src, size, sizefast)  do { DAS_ASSERT((size) <= sizefast); memcpy(dst, src, sizefast); } while (0)
#define DAS_F2S_MEMSET(dst, ch, size, sizefast)   do { DAS_ASSERT((size) <= sizefast); memset(dst, ch, sizefast); } while (0)

    template <typename TT>
    static __forceinline char * print_shortest ( char * buf, TT x ) {
        typedef typename F2SBits<TT>::U U;
        const U expMask = F2SBits<TT>::expMask;
        U bits;
        memcpy(&bits, &x, sizeof(x));
        const U signMask = U(U(1) << (sizeof(U) * 8 - 1));
        if ( bits & signMask ) {
            *buf++ = '-';
            bits &= ~signMask;
            memcpy(&x, &bits, sizeof(x));
        }
        if ( (bits & expMask) == expMask ) {
            memcpy(buf, bits != expMask ? "nan" : "inf", 3);
            return buf + 3;
        }
        if ( bits == 0 ) {
            *buf++ = '0';
            return buf;
        }
        auto dec = fmt::detail::dragonbox::to_decimal(x);
        char decbuf[kDigitRoom + kOverreadSlack];
        char * decend = decbuf + kDigitRoom;
        char * digits = printunsignedrev(decend, uint64_t(dec.significand));
        int declen = int(decend - digits);
        int output_exp = dec.exponent + declen - 1;
        if ( output_exp < -4 || output_exp >= 16 ) {
            *buf++ = digits[0];
            if ( declen > 1 ) {
                *buf++ = '.';
                DAS_F2S_MEMCPY(buf, digits + 1, declen - 1, 16);
                buf += declen - 1;
            }
            return printexp(buf, output_exp);
        }
        int dot = declen + dec.exponent;
        if ( dec.exponent >= 0 ) {
            DAS_F2S_MEMCPY(buf, digits, declen, 17);
            DAS_F2S_MEMSET(buf + declen, '0', dec.exponent, 16);
            return buf + declen + dec.exponent;
        } else if ( dot > 0 ) {
            DAS_F2S_MEMCPY(buf, digits, dot, 16);
            buf[dot] = '.';
            DAS_F2S_MEMCPY(buf + dot + 1, digits + dot, declen - dot, 16);
            return buf + declen + 1;
        } else {
            buf[0] = '0';
            buf[1] = '.';
            DAS_F2S_MEMSET(buf + 2, '0', -dot, 4);
            DAS_F2S_MEMCPY(buf + 2 - dot, digits, declen, 17);
            return buf + 2 - dot + declen;
        }
    }

#undef DAS_F2S_MEMCPY
#undef DAS_F2S_MEMSET

    char * float2string ( char * buf, float value ) {
        return print_shortest(buf, value);
    }

    char * double2string ( char * buf, double value ) {
        return print_shortest(buf, value);
    }

}
