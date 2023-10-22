#include "device.h"

#if D3D_HAS_RAY_TRACING && 0 // VK_NV_ray_tracing
#include "nv/pipeline.cpp"
#else
#include "stub/pipeline.cpp"
#endif