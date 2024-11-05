// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_renderScene.h>

// mark visible elems for basic render with (no shader lods)
struct RenderScene::OptimizedScene::MarkElems
{
  static inline void mark_visible(void *p, uint16_t first, uint16_t last, int, const bbox3f &)
  {
    RenderScene::VisibilityData &vd = *(RenderScene::VisibilityData *)p;
    RenderScene::OptimizedScene &s = *vd.scn;
    short *__restrict ep = &s.elemIdxStor[first], *__restrict ep_end = &s.elemIdxStor[last];
    unsigned *ue = vd.usedElems.data();
    vd.lastElemNum[0] += last - first + 1;

    for (; ep <= ep_end; ep++)
      ue[*ep / 32] |= 0x80000000 >> (*ep & 0x1F);
  }
};
void RenderScene::prepareVisibilitySimple(VisibilityData *vd, int render_id, unsigned render_flags_mask, const VisibilityFinder &vf)
{
  optScn.visTree.prepareVisibility<OptimizedScene::MarkElems>(vd, render_id, render_flags_mask, vf);
}