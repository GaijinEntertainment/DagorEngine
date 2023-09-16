// MODIFICATION BY GAIJIN
//
// PlayStation time.h doesn't have gmtime_r and localtime_r functions, but have the _s variants with the following
// signatures:
// struct tm *gmtime_s(const time_t *_Restrict, struct tm *_Restrict);
// struct tm *localtime_s(const time_t *_Restrict, struct tm *_Restrict);
#if _TARGET_C1 || _TARGET_C2



#endif
