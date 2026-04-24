// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh::dagdp
{
struct BVHInstanceMapper : ::BVHInstanceMapper
{
  BVHInstanceMapper(ContextId) {};
  void mapRendinst(const RenderableInstanceLodsResource *, int, int, IPoint2 &out_blas, uint32_t &out_meta) const override
  {
    out_blas.x = 0;
    out_blas.y = 0;
    out_meta = 0;
  }
  void submitBuffers(Sbuffer *, Sbuffer *) override {}
  void requestStaticBLAS(const RenderableInstanceLodsResource *) override {}
  bool isInStaticBLASQueue(const RenderableInstanceLodsResource *) override { return false; }
};

::BVHInstanceMapper *get_mapper(ContextId) { return nullptr; }

void init(ContextId) {}

void teardown(ContextId) {}

::BVHInstanceMapper::InstanceBuffers get_buffers(ContextId) { return {}; }

} // namespace bvh::dagdp