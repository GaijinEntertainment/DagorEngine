#pragma once
#ifndef SQ_SAFE_SHIFT_H
#define SQ_SAFE_SHIFT_H

#include <sqconfig.h>

#include <limits>
#include <type_traits>

static constexpr unsigned SQ_INTEGER_BITS = std::numeric_limits<SQUnsignedInteger>::digits;
static constexpr unsigned SQ_INTEGER_MASK = SQ_INTEGER_BITS - 1;


inline SQInteger sq_safe_shift_left(SQInteger x, SQInteger y)
{
    SQUnsignedInteger ux = (SQUnsignedInteger)x;
    SQUnsignedInteger uy = (SQUnsignedInteger)y;
    SQUnsignedInteger neg = uy >> (SQ_INTEGER_BITS - 1);
    SQUnsignedInteger neg_mask = ~neg + 1u;  // == -neg without C4146
    SQUnsignedInteger sh  = (uy ^ neg_mask) + neg;

    SQUnsignedInteger overflow = ~SQUnsignedInteger(sh >= SQ_INTEGER_BITS) + 1u;

    SQUnsignedInteger l = ux << (sh & SQ_INTEGER_MASK);
    SQUnsignedInteger r = ux >> (sh & SQ_INTEGER_MASK);

    // Arithmetic sign extension for reversed right shift (negative y)
    SQUnsignedInteger sign = ~SQUnsignedInteger(x < 0) + 1u;
    SQUnsignedInteger sign_ext = sign ^ (sign >> (sh & SQ_INTEGER_MASK));
    r |= sign_ext;

    SQUnsignedInteger res = (l & ~neg_mask) | (r & neg_mask);

    // On overflow: 0 for left shift (positive y), sign-fill for reversed right shift (negative y)
    SQUnsignedInteger fill = sign & neg_mask & overflow;
    res = (res & ~overflow) | fill;

    return (SQInteger)res;
}


inline SQInteger sq_safe_shift_right(SQInteger x, SQInteger y)
{
    SQUnsignedInteger ux = (SQUnsignedInteger)x;
    SQUnsignedInteger uy = (SQUnsignedInteger)y;

    SQUnsignedInteger neg = uy >> (SQ_INTEGER_BITS - 1);
    SQUnsignedInteger neg_mask = ~neg + 1u;
    SQUnsignedInteger sh  = (uy ^ neg_mask) + neg;

    SQUnsignedInteger overflow = ~SQUnsignedInteger(sh >= SQ_INTEGER_BITS) + 1u;

    SQUnsignedInteger r = ux >> (sh & SQ_INTEGER_MASK);
    SQUnsignedInteger l = ux << (sh & SQ_INTEGER_MASK);

    // Arithmetic sign extension: fill top sh bits of r with sign bit
    SQUnsignedInteger sign = ~SQUnsignedInteger(x < 0) + 1u;
    SQUnsignedInteger sign_ext = sign ^ (sign >> (sh & SQ_INTEGER_MASK));
    r |= sign_ext;

    SQUnsignedInteger dir = (r & ~neg_mask) | (l & neg_mask);

    SQUnsignedInteger fill = sign & overflow;

    return (SQInteger)((dir & ~overflow) | fill);
}


inline SQInteger sq_safe_unsigned_shift_right(SQInteger x, SQInteger y)
{
    SQUnsignedInteger ux = (SQUnsignedInteger)x;
    SQUnsignedInteger uy = (SQUnsignedInteger)y;

    SQUnsignedInteger neg = uy >> (SQ_INTEGER_BITS - 1);
    SQUnsignedInteger neg_mask = ~neg + 1u;
    SQUnsignedInteger sh  = (uy ^ neg_mask) + neg;

    SQUnsignedInteger overflow = ~SQUnsignedInteger(sh >= SQ_INTEGER_BITS) + 1u;

    SQUnsignedInteger r = ux >> (sh & SQ_INTEGER_MASK);
    SQUnsignedInteger l = ux << (sh & SQ_INTEGER_MASK);

    SQUnsignedInteger res = (r & ~neg_mask) | (l & neg_mask);

    res &= ~overflow;

    return (SQInteger)res;
}

#endif // SQ_SAFE_SHIFT_H
