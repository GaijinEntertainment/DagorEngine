#if NUM_POISSON_SAMPLES == 256
static const float3 POISSON_SAMPLES[NUM_POISSON_SAMPLES] =
{
#include <poisson_256_content.hlsl>
};

#elif NUM_POISSON_SAMPLES == 128

static const float3 POISSON_SAMPLES[NUM_POISSON_SAMPLES] =
{
#include <poisson_256_content.hlsl>
};

#elif NUM_POISSON_SAMPLES == 64

static const float3 POISSON_SAMPLES[NUM_POISSON_SAMPLES] =
{
#include <poisson_256_content.hlsl>
};

#elif NUM_POISSON_SAMPLES == 32

static const float3 POISSON_SAMPLES[NUM_POISSON_SAMPLES] =
{
#include <poisson_256_content.hlsl>
};

#else
  #undef NUM_POISSON_SAMPLES
#endif
