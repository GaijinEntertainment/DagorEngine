#ifndef SPHERICAL_HARMONICS_CONSTS_INCLUDED
#define SPHERICAL_HARMONICS_CONSTS_INCLUDED 1

#define THREADS_PER_GROUP_AXIS 16
#define HARM_COEFS_COUNT 9
#define FINAL_REDUCTION_HIGH_THREADS_COUNT 128
#define MAX_TEXTURE_SAMPLING_RESOLUTION 128 //it is actually sqrt(FINAL_REDUCTION_HIGH_THREADS_COUNT * THREADS_PER_GROUP_AXIS^2 * 2 * 2 / 6)
//two 2 takes from compute shader algo where we sample 2 point or two index in one thread. 6 - it is cubemap dimension
struct HarmCoefs
{
  float4 coeficients[HARM_COEFS_COUNT];
};

#endif