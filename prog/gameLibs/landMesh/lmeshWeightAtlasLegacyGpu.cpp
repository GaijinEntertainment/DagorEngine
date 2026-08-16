// Copyright (C) Gaijin Games KFT.  All rights reserved.

// The GPU path: no CPU pixel work at all, the driver decodes each cell's
// exported textures and land_weight_pack draws them into the atlas.
// See lmeshWeightAtlasLegacy.h for why this file exists.

#include <landMesh/lmeshWeightAtlas.h>
#include <landMesh/lmeshManager.h>
#include "lmeshWeightAtlasLegacy.h"
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_lock.h>
#include <ioSys/dag_memIo.h>
#include <math/dag_mathBase.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/unique_ptr.h>

static constexpr int DET_NUM = LandWeightAtlas::DET_NUM;

#define GLOBAL_VARS_LIST         \
  VAR(land_weight_src_const_no)  \
  VAR(land_weight_src2_const_no) \
  VAR(land_weight_pack_tc)       \
  VAR(land_weight_pack_ch)       \
  VAR(land_weight_pack_num_tex)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

// without the weights on the CPU there is nothing to pack, only pages to claim
// for the render pass that fills them
void LandWeightAtlas::setCellRecord(int index, const uint8_t *det_tex_ids, int num_tex)
{
  writeRecord(index, det_tex_ids, num_tex, nullptr);
}

// renders the cell's pages from its legacy weight textures; the sampler clamp
// extends the cell edge into the page border (no neighbor weights on this path)
bool LandWeightAtlas::renderCell(int index, int num_tex, BaseTexture *tex1, BaseTexture *tex2, int src_size)
{
  if (!tex || !tex1 || uint32_t(index) >= uint32_t(cellsX * cellsY) || src_size <= 0)
    return false;
  const int srcSlot = land_weight_src_const_noVarId.get_int(), src2Slot = land_weight_src2_const_noVarId.get_int();
  if (srcSlot <= 0 || src2Slot <= 0)
  {
    logerr("land weight atlas: land_weight_pack is not in this shader dump, so the pages cannot be rendered");
    return false;
  }
  if (!packRenderer)
    packRenderer = new PostFxRenderer("land_weight_pack");

  // cells with up to 4 landclasses ship no second texture; unbound reads as 0,
  // which is what those channels are worth. The sampler comes with the shader.
  d3d::set_tex(STAGE_PS, srcSlot, tex1);
  d3d::set_tex(STAGE_PS, src2Slot, tex2);
  land_weight_pack_tcVarId.set_float4((float)pageW / src_size, (float)pageW / src_size, -2.f / src_size, -2.f / src_size);
  land_weight_pack_num_texVarId.set_int(num_tex);

  // a page is simply the cell's next three channels, and the shader sees a
  // channel the cell does not blend as a weight of zero
  const uint32_t rec = records[index * WORDS_PER_CELL];
  for (int p = 0, n = LandWeightAtlas::pagesFor(clamp(num_tex, 1, (int)DET_NUM)); p < n; p++)
  {
    const int page = (rec >> (p * LAND_WEIGHT_PAGE1_SHIFT)) & LAND_WEIGHT_PAGE_MASK;
    const int firstCh = p * LAND_WEIGHT_CHANNELS_PER_PAGE;
    land_weight_pack_chVarId.set_float4(firstCh, firstCh + 1, firstCh + 2, 0);
    d3d::setview((page % pagesPerRow) * pageW, (page / pagesPerRow) * pageW, pageW, pageW, 0, 1);
    packRenderer->render();
  }
  return true;
}

// pages are rendered from the legacy textures the driver decodes for us
LandWeightAtlas *render_land_weight_atlas(dag::Span<Tab<uint8_t>> records, int cells_x, int cells_y, int tex_size, int elem_size)
{
  static constexpr int HDR = LandMeshManager::DET_TEX_NUM + 8;
  int64_t reft = ref_time_ticks();
  eastl::unique_ptr<LandWeightAtlas> a(new LandWeightAtlas(cells_x, cells_y, elem_size, /*tex_cflg*/ 0));
  SmallTab<uint8_t, TmpmemAlloc> numTex;
  clear_and_resize(numTex, records.size());
  for (int i = 0; i < records.size(); ++i)
  {
    numTex[i] = 0;
    for (int ch = 0; ch < LandMeshManager::DET_TEX_NUM; ch++)
      if (records[i].size() && records[i][ch] != 0xFF)
        numTex[i]++;
    a->setCellRecord(i, records[i].size() ? records[i].data() : nullptr, numTex[i]);
  }
  if (!a->upload()) // sized to the pages the entries claimed, then rendered into
    return nullptr;

  d3d::GpuAutoLock gpuLock;
  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;
  d3d::set_render_target({}, DepthAccess::RW, {{a->tex, 0, 0}});
  d3d::clearview(CLEAR_TARGET, 0x00000000, 1.f, 0); // pages of cells that fail below are never drawn into
  Tab<int> undecoded(framemem_ptr());
  for (int i = 0; i < records.size(); ++i)
  {
    if (!records[i].size())
      continue;
    int len = 0, tex2Offset = 0;
    memcpy(&len, &records[i][LandMeshManager::DET_TEX_NUM], 4);
    memcpy(&tex2Offset, &records[i][LandMeshManager::DET_TEX_NUM + 4], 4);
    if (!len)
      continue;
    InPlaceMemLoadCB crd1(records[i].data() + HDR, tex2Offset ? tex2Offset : len);
    Texture *t1 = d3d::create_ddsx_tex(crd1, TEXCF_RGB, 0, 0, "land_weight_src");
    Texture *t2 = nullptr;
    bool decoded = t1;
    if (tex2Offset && len - tex2Offset > 0)
    {
      InPlaceMemLoadCB crd2(records[i].data() + HDR + tex2Offset, len - tex2Offset);
      t2 = d3d::create_ddsx_tex(crd2, TEXCF_RGB, 0, 0, "land_weight_src2");
      // instead of drawing without t2 when it is required, the cell is flagged as undecoded
      // The decision has to happen before del_d3dres(), which nulls what it frees.
      decoded &= bool(t2);
    }
    const bool packed = decoded && a->renderCell(i, numTex[i], t1, t2, tex_size);
    del_d3dres(t1);
    del_d3dres(t2);
    if (!decoded)
      undecoded.push_back(i);
    else if (!packed) // no pass can fill any page, so no atlas rather than garbage weights
      return nullptr;
  }
  if (undecoded.size())
  {
    // Their pages were claimed before we knew, and nothing drew into them; a
    // record of one landclass reads none of them, which is the same fallback
    // the CPU path gives a cell it cannot decode.
    for (int i : undecoded)
      a->setCellRecord(i, records[i].data(), 1);
    a->upload(); // republishes the records; the atlas itself is already there
    logerr("land weight atlas: %d of %d cells have weight textures the driver cannot read and render as a "
           "single landclass; re-export the location",
      (int)undecoded.size(), (int)records.size());
  }
  debug("land weight atlas: %dx%d cells, page %d (elem %d + border), %d pages, rendered in %d us", cells_x, cells_y,
    a->getCellTexSize(), elem_size, a->getPageCount(), (int)get_time_usec(reft));
  return a.release();
}
