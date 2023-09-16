#pragma once

#include <ctime>

namespace das {

    struct Time {
        time_t time;
    };

    template <>
    struct cast <Time> {
        static __forceinline Time to ( vec4f x )               { union { Time t; vec4f vec; } T; T.vec = x; return T.t; }
        static __forceinline vec4f from ( Time x )             { union { Time t; vec4f vec; } T; T.t = x; return T.vec; }
    };
    template <> struct WrapType<Time> { enum { value = false }; typedef time_t type; };

    template<>
    struct SimPolicy<Time> {
        static __forceinline auto to_time ( vec4f a ) {
            return cast<Time>::to(a).time;
        }
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_time(a) == to_time(b);
        }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_time(a) != to_time(b);
        }
        static __forceinline bool GtEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_time(a) >= to_time(b);
        }
        static __forceinline bool LessEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_time(a) <= to_time(b);
        }
        static __forceinline bool Gt  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_time(a) > to_time(b);
        }
        static __forceinline bool Less  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return to_time(a) < to_time(b);
        }
        static __forceinline double Sub  ( vec4f a, vec4f b, Context &, LineInfo * ) {
            return difftime(to_time(a), to_time(b));
        }
    };

    static inline int64_t cast_int64(Time t) { return int64_t(t.time); }
    Time builtin_clock();
    void builtin_sleep ( uint32_t msec );
    void builtin_exit ( int32_t ec );
}
