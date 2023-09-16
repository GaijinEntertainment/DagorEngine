#ifndef MAX_FFT_RESOLUTION_SHIFT
  #define MAX_FFT_RESOLUTION_SHIFT 9
  //can be appication specific override
#endif

#define CS_MAX_FFT_RESOLUTION (1<<MAX_FFT_RESOLUTION_SHIFT)
#define WARP_WIDTH_SHIFT 3 // minimum number of threads which execute in lockstep
#define WARP_WIDTH (1<<WARP_WIDTH_SHIFT)// minimum number of threads which execute in lockstep

#ifndef H0_COMPRESSION
  #define H0_COMPRESSION 1
#endif
#if H0_COMPRESSION
  #define h0_type uint
#else
  #define h0_type float2
#endif

#ifndef FFT_COMPRESSION
  #define FFT_COMPRESSION 1
#endif

#if FFT_COMPRESSION
  #define dt_type uint2
  #define ht_type uint
#else
  #define dt_type float4
  #define ht_type float2
#endif
