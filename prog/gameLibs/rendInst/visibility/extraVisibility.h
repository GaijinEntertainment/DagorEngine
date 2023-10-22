#pragma once

#include "riGen/riExtraPool.h"
#include "render/extraRender.h"

#include <generic/dag_smallTab.h>


constexpr uint32_t INVALID_VB_EXTRA_GEN = 0;
struct RiGenExtraVisibility
{
  static constexpr int LARGE_LOD_CNT = rendinst::RiExtraPool::MAX_LODS / 2; // sort only first few lods

  int forcedExtraLod = -1;
  struct Order
  {
    int d;
    uint32_t id;
    bool operator<(const Order &rhs) const { return d < rhs.d; }
  };
  struct UVec2
  {
    uint32_t x, y;
  };
  SmallTab<vec4f> largeTempData;                      // temp array
  SmallTab<SmallTab<Order>> riexLarge[LARGE_LOD_CNT]; // temp array
  SmallTab<SmallTab<vec4f>> riexData[rendinst::RiExtraPool::MAX_LODS];
  SmallTab<float> minSqDistances[rendinst::RiExtraPool::MAX_LODS];
  SmallTab<IPoint2> vbOffsets[rendinst::RiExtraPool::MAX_LODS]; // after copied to buffer
  SmallTab<unsigned short> riexPoolOrder;

  rendinst::render::VbExtraCtx *vbexctx = nullptr;
  volatile uint32_t vbExtraGeneration = INVALID_VB_EXTRA_GEN;

  uint32_t riExLodNotEmpty = 0;
  int riexInstCount = 0;

  struct PerInstanceElem
  {
    uint32_t lod : 4, instanceId : 28;
    uint16_t poolId, poolOrder;
    int vbOffset;
    float dist2;
  };
  SmallTab<PerInstanceElem> sortedTransparentElems;
};
