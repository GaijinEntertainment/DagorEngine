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
    template <> struct WrapType<Time> { enum { value = false }; typedef time_t type; typedef time_t rettype; };

    __forceinline bool time_equal ( Time a, Time b ) { return a.time==b.time; }
    __forceinline bool time_nequal ( Time a, Time b ) { return a.time!=b.time; }
    __forceinline bool time_gtequal ( Time a, Time b ) { return a.time>=b.time; }
    __forceinline bool time_ltequal ( Time a, Time b ) { return a.time<=b.time; }
    __forceinline bool time_gt ( Time a, Time b ) { return a.time>b.time; }
    __forceinline bool time_lt ( Time a, Time b ) { return a.time<b.time; }
    __forceinline double time_sub ( Time a, Time b ) { return difftime(a.time, b.time); }

    static inline int64_t cast_int64(Time t) { return int64_t(t.time); }
    Time builtin_clock();
    Time builtin_mktime(int year, int month, int mday, int hour, int min, int sec);
    void builtin_sleep ( uint32_t msec );
    void builtin_exit ( int32_t ec );
}
