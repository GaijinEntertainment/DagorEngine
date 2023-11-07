#include "daScript/misc/platform.h"
#include "daScript/misc/performance_time.h"

#if DAS_PROFILE_SECTIONS
namespace das {
    ProfileSection::ProfileSection(const char * n) {
        name = n;
        timeStamp = ref_time_ticks();
    }
    ProfileSection::~ProfileSection() {
        int usec = get_time_usec(timeStamp);
        printf("\"%s\", %.4f\n", name ? name : "unspecified", usec / 1000000.0f);
    }
}

#endif

#ifdef _MSC_VER

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern "C" int64_t ref_time_ticks () {
    LARGE_INTEGER  t0;
    QueryPerformanceCounter(&t0);
    return t0.QuadPart;
}

extern "C" int get_time_usec ( int64_t reft ) {
    int64_t t0 = ref_time_ticks();
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return int((t0-reft)*1000000LL/freq.QuadPart);
}

extern "C" int64_t get_time_nsec ( int64_t reft ) {
    int64_t t0 = ref_time_ticks();
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return  int64_t((t0-reft)*1000000000LL/freq.QuadPart);
}

extern "C" int64_t ref_time_delta_to_usec(int64_t ref)
{
    LARGE_INTEGER freq;
    QueryPerformanceCounter(&freq);
    return ref * 1000000LL/freq.QuadPart;
}

#elif __linux__ || defined(_EMSCRIPTEN_VER) || defined __HAIKU__

#include <time.h>

const uint64_t NSEC_IN_SEC = 1000000000LL;

extern "C" int64_t ref_time_ticks () {
    timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        DAS_ASSERT(false);
        return -1;
    }

    return ts.tv_sec * NSEC_IN_SEC + ts.tv_nsec;
}

extern "C" int get_time_usec ( int64_t reft ) {
    return int((ref_time_ticks() - reft) / (NSEC_IN_SEC/1000000LL));
}

extern "C" int64_t get_time_nsec ( int64_t reft ) {
    return  ref_time_ticks() - reft;
}

extern "C" int64_t ref_time_delta_to_usec(int64_t ref) { return ref / (NSEC_IN_SEC/1000000LL); }


#else // osx

#include <time.h>

extern "C" int64_t ref_time_ticks() {
    return clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
}

extern "C" int get_time_usec ( int64_t reft ) {
    return (clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW) - reft)/1000LL;
}

extern "C" int64_t get_time_nsec ( int64_t reft ) {
    return clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW) - reft;
}

extern "C" int64_t ref_time_delta_to_usec(int64_t ref) { return ref / 1000LL; }

#endif
