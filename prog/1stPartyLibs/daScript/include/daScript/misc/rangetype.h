#pragma once

#include "daScript/misc/vectypes.h"

namespace das
{
    template <typename TT>
    struct RangeType {
        using baseType = TT;
        union {
            TT  from;   //  [from,to) range
            TT  x;
        };
        union {
            TT  to;
            TT  y;
        };
        __forceinline RangeType() {}
        __forceinline RangeType( TT t ) : from(0), to(t) {}
        __forceinline RangeType( TT f, TT t ) : from(f), to(t) {}
        __forceinline RangeType(vec4f t) : from(vec_extract<TT>::x(t)), to(vec_extract<TT>::y(t)) {}
        template <typename AP>
        __forceinline friend StringWriter<AP> & operator<< (StringWriter<AP> & stream, const RangeType<TT> & vec) {
            stream << vec.from << DAS_PRINT_VEC_SEPARATROR << vec.to;
            return stream;
        }
        __forceinline bool operator == ( const RangeType<TT> & vec ) const {
            return from==vec.from && to==vec.to;
        }
        __forceinline operator vec4f() const { return v_ldu_half((float *)this); };
    };

    typedef struct RangeType<int32_t>  range;
    typedef struct RangeType<uint32_t> urange;
    typedef struct RangeType<int64_t>  range64;
    typedef struct RangeType<uint64_t> urange64;

    __forceinline range mk_range ( int32_t i ) { return range(i); }
    __forceinline urange mk_urange ( uint32_t i ) { return urange(i); }
    __forceinline range64 mk_range64 ( int64_t i ) { return range64(i); }
    __forceinline urange64 mk_urange64 ( uint64_t i ) { return urange64(i); }
}
