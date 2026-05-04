#ifndef CHOP_WATER_DEFINES_HLSL
#define CHOP_WATER_DEFINES_HLSL

#define CHOP_TILING_OVERLAP 0.95

#if defined(__SSE2__) || defined(_M_IX86) || defined(_M_X64)
#define CHOP_WATER_USE_SSE 1
#endif

#endif
