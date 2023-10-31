#pragma once
#include <landMesh/landRayTracer.h>
class EditorLandRayTracer : public BaseLandRayTracer<uint32_t, uint32_t>
{
public:
  EditorLandRayTracer(IMemAlloc *allocator = midmem) : BaseLandRayTracer<uint32_t, uint32_t>(allocator) {}
};
