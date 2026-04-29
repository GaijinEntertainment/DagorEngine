#ifndef TSR_COEFFS_HLSLI
#define TSR_COEFFS_HLSLI

#define TSR_MAX_PHASES 256
#define TSR_TAPS 3

// padded to 48 bytes
struct Weights
{
  float w[TSR_TAPS*TSR_TAPS];
  float sum;
  float padding[2];
};

#endif