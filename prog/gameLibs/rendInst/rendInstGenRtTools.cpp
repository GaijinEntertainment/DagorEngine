// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstGenRtTools.h>
#include <rendInst/rendInstGenRender.h>
#include "riGen/riGenData.h"
#include "riGen/genObjUtil.h"
#include "riGen/landClassData.h"
#include "riGen/riUtil.h"
#include "riGen/riRotationPalette.h"

#include <drv/3d/dag_lock.h>
#include <3d/dag_texPackMgr2.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_Point4.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>


static constexpr const unsigned riPoolBits = 16u;
static constexpr const unsigned riPoolBitsMask = (1u << riPoolBits) - 1u;
static bool rigenNeedSyncPrepare = true, pendingReinit = false;
static unsigned ri_future_pregen_cnt[16] = {0};
static struct RiGenLastSetSweepMask
{
  rendinst::gen::EditableHugeBitmask *bm;
  float ox, oz, scale;
} lastSweep = {nullptr, 0, 0, 1};
static Point3 curSunDir(0, -1, 0);
static bool globalShadowsNeeded = false;

static rendinst::rigen_prepare_pools_t riGenPreparePools = nullptr;

inline void assign_res(PatchablePtr<RenderableInstanceLodsResource> &dest, RenderableInstanceLodsResource *res)
{
  if (res)
    res->addRef();
  if (dest)
    dest->delRef();
  dest = res;
}

static void updateGlobalShadows()
{
  if (!globalShadowsNeeded || !rendinst::rendinstGlobalShadows)
    return;
  debug("update shadows for RI: %@", curSunDir);
  if (!is_managed_textures_streaming_load_on_demand())
    ddsx::tex_pack2_perform_delayed_data_loading();

  d3d::GpuAutoLock gpuLock;
  SCOPE_VIEW_PROJ_MATRIX;
  rendinst::render::renderRIGenGlobalShadowsToTextures(curSunDir);
}

static bool check_ht_tight_enough(RendInstGenData *rgl, int idx, RendInstGenData::CellRtData *crt)
{
  if (!crt->bbox.size())
    return true;
  float miny = floor(as_point3(&crt->bbox[0].bmin).y - 32);
  float maxy = ceil(as_point3(&crt->bbox[0].bmax).y + 64);
  float hdelta = maxy - miny;
  if (hdelta >= 256)
    hdelta = ceilf(hdelta / 256.0f) * 256.0;
  else
    for (int i = 1; i <= 256; i *= 2)
      if (hdelta < i)
      {
        hdelta = ceilf(hdelta / i) * i;
        break;
      }
  if (fabs(rgl->cells[idx].htMin - miny) > 0 || fabs(rgl->cells[idx].htDelta - hdelta) > 0)
  {
    rgl->cells[idx].htMin = as_point4(&crt->cellOrigin).y = miny;
    rgl->cells[idx].htDelta = crt->cellHeight = hdelta;
    return false;
  }
  return true;
}
static RendInstGenData::CellRtData *validate_generated_cell(RendInstGenData *rgl, RendInstGenData::CellRtData *crt, int idx, int cx,
  int cz)
{
  for (int try_cnt = 3; !check_ht_tight_enough(rgl, idx, crt) && try_cnt > 0; try_cnt--)
  {
    crt->sysMemData.reset();
    crt = rgl->generateCell(cx, cz);
  }
  return crt;
}

static int get_cell_idx_at(int layer_idx, float x, float z)
{
  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  if (!rgl || !rgl->cellNumW || !rgl->cellNumH)
    return -1;
  int cx = clamp<int>((x - rgl->world0x()) / (rgl->grid2world * rgl->cellSz), 0, rgl->cellNumW - 1);
  int cz = clamp<int>((z - rgl->world0z()) / (rgl->grid2world * rgl->cellSz), 0, rgl->cellNumH - 1);
  return cz * rgl->cellNumW + cx;
}
static RendInstGenData::CellRtData *get_cell_rt_at(int layer_idx, float x, float z)
{
  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  if (!rgl)
    return nullptr;
  int idx = get_cell_idx_at(layer_idx, x, z);
  if (idx < 0)
    return nullptr;
  return rgl->cells[idx].rtData;
}

static void rendinst_alloc_coll_props(int layer_idx)
{
  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  rgl->rtData->riProperties.resize(rgl->rtData->riRes.size());
  mem_set_0(rgl->rtData->riProperties);
  rgl->rtData->riDestr.resize(rgl->rtData->riRes.size());
  mem_set_0(rgl->rtData->riDestr);
}

static RendInstGenData *alloc_rigen_data(float ofs_x, float ofs_z, float grid2world, int cell_sz, int cell_num_x, int cell_num_z,
  dag::ConstSpan<rendinst::gen::land::AssetData *> land_cls, int layer_idx, int per_inst_data_dwords, float dens_map_px,
  float dens_map_pz)
{
  size_t ofs0 = sizeof(RendInstGenData);
  size_t ofs1 = ofs0 + sizeof(RendInstGenData::Cell) * cell_num_x * cell_num_z;
  size_t ofs2 = ofs1 + sizeof(RendInstGenData::LandClassRec) * land_cls.size();
  size_t ofs3 = ofs2 + sizeof(RendInstGenData::Cell::LandClassCoverage) * land_cls.size() * cell_num_x * cell_num_z;
  size_t ofs4 = ofs3 + sizeof(RendInstGenData::PregenEntPoolDesc) * rendinst::MAX_PREGEN_RI;
  size_t sz = ofs4;
  float invGridCellSzV = 1.f / (grid2world * cell_sz);
  char *mem = (char *)memalloc(sz, midmem);
  memset(mem, 0, sz);

  RendInstGenData *ri = new (mem, _NEW_INPLACE) RendInstGenData;
  ri->cellNumW = cell_num_x;
  ri->cellNumH = cell_num_z;
  ri->cellSz = cell_sz;
  ri->dataFlags = 0;
  ri->perInstDataDwords = per_inst_data_dwords;
  ri->sweepMaskScale = 1.0;
  ri->cells.init(mem + ofs0, cell_num_x * cell_num_z);
  ri->landCls.init(mem + ofs1, 0);
  ri->grid2world = grid2world;
  ri->densMapPivotX = dens_map_px;
  ri->densMapPivotZ = dens_map_pz;
  ri->pregenDataBaseOfs = -1;
  ri->world0Vxz = v_make_vec4f(ofs_x, ofs_z, ofs_x, ofs_z);
  ri->invGridCellSzV = v_splats(invGridCellSzV);
  ri->lastCellXZXZ = v_perm_xaxa(v_splats(cell_num_x - 0.9), v_splats(cell_num_z - 0.9));
  ri->pregenEnt.init(mem + ofs3, 0);

  for (int i = 0, idx = 0; i < land_cls.size(); i++)
  {
    if (!land_cls[i] || land_cls[i]->layerIdx != layer_idx)
      continue;
    ri->landCls[idx].asset = land_cls[i];
    ri->landCls[idx].mask = EditableHugeBitMap2d::create(cell_sz * cell_num_x, cell_sz * cell_num_z);
    idx++;
    ri->landCls.init(mem + ofs1, idx);
  }
  for (int i = 0; i < cell_num_x * cell_num_z; i++)
  {
    ri->cells[i].cls.init(mem + ofs2 + i * sizeof(RendInstGenData::Cell::LandClassCoverage) * ri->landCls.size(), ri->landCls.size());
    ri->cells[i].htMin = 0;
    ri->cells[i].htDelta = 128;
    ri->cells[i].riDataRelOfs = -1;
    for (int j = 0, je = ri->cells[i].cls.size(); j < je; j++)
      ri->cells[i].cls[j].landClsIdx = j;
  }
  return ri;
}
static void clearRigenRtdataAndReinitPregenEnt(RendInstGenData *rgl, unsigned new_pregen_ent_size)
{
  if (!rgl->rtData)
    return;

  {
    ScopedLockWrite lock(rgl->rtData->riRwCs);
    // addref riRes in future pregenEnt
    for (auto *p = rgl->pregenEnt.data(), *pe = p + new_pregen_ent_size; p < pe; p++)
      if (p->riRes)
        p->riRes->addRef();

    for (int i = 0; i < rgl->cells.size(); i++)
      del_it(rgl->cells[i].rtData);
    if (rendinst::unregCollCb)
      for (RendInstGenData::CollisionResData &collResData : rgl->rtData->riCollRes)
        rendinst::unregCollCb(collResData.handle);
    G_ASSERTF(rgl->pregenEnt.size() <= rgl->rtData->riCollRes.size(), "rtData=%p rgl->pregenEnt.size()=%d rtData->riCollRes.size()=%d",
      rgl->rtData, rgl->pregenEnt.size(), rgl->rtData->riCollRes.size());
    for (int i = 0; i < rgl->pregenEnt.size(); i++) // we release only collres acquired by pregenEnt (not ones from land classes!)
    {
      // release riRes in currently inited pregenEnt
      if (rgl->pregenEnt[i].riRes)
        rgl->pregenEnt[i].riRes->delRef();
      release_game_resource((GameResource *)rgl->rtData->riCollRes[i].collRes);
    }
    rgl->rtData->riCollRes.clear();
    rgl->rtData->clear();
    rgl->beforeReleaseRtData();
  }
  del_it(rgl->rtData);
  // update pregenEnt size
  rgl->pregenEnt.init(rgl->pregenEnt.data(), new_pregen_ent_size);
}
int rendinst::register_rt_pregen_ri(RenderableInstanceLodsResource *ri_res, int layer_idx, E3DCOLOR cf, E3DCOLOR ct,
  const char *res_nm)
{
  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  if (!rgl || !ri_res)
    return -1;

  if (ri_future_pregen_cnt[layer_idx] < rgl->pregenEnt.size())
    ri_future_pregen_cnt[layer_idx] = rgl->pregenEnt.size();
  auto pregenEnt = make_span(rgl->pregenEnt.data(), ri_future_pregen_cnt[layer_idx]);
  for (int i = 0; i < pregenEnt.size(); i++)
    if (pregenEnt[i].riRes == ri_res)
    {
      pregenEnt[i].colPair[0] = cf;
      pregenEnt[i].colPair[1] = ct;
      return i | (layer_idx << riPoolBits);
    }

  if (pregenEnt.size() < MAX_PREGEN_RI)
  {
    while (!rendinst::isRIGenPrepareFinished())
      sleep_msec(10);

    int idx = pregenEnt.size();
    pregenEnt = make_span(pregenEnt.data(), idx + 1);

    auto &new_ent = pregenEnt[idx];
    new_ent.riRes = ri_res;
    new_ent.riName = res_nm;
    new_ent.colPair[0] = cf;
    new_ent.colPair[1] = ct;
    new_ent.posInst = ri_res->hasImpostor();
    new_ent.paletteRotation = ri_res->isBakedImpostor() ? 1 : 0;
    new_ent.zeroInstSeeds = 0;
    new_ent._resv29 = 0;
    new_ent.paletteRotationCount = ri_res->getRotationPaletteSize();

    ri_future_pregen_cnt[layer_idx] = pregenEnt.size();
    pendingReinit = true;
    return idx | (layer_idx << riPoolBits);
  }
  else
    logwarn("pregenEnt exceeds limit %d", MAX_PREGEN_RI);
  return -MAX_PREGEN_RI - 2;
}

void rendinst::update_rt_pregen_ri(int pregen_id, RenderableInstanceLodsResource &ri_res)
{
  if (pregen_id == -1)
    return;

  int layerIdx = pregen_id >> riPoolBits;
  int idx = pregen_id & riPoolBitsMask;

  G_ASSERTF(layerIdx < rendinst::rgLayer.size(), "pregen_id=0x%x layerIdx=%d rendinst::rgLayer.size()=%d", pregen_id, layerIdx,
    rendinst::rgLayer.size());
  RendInstGenData *rgl = rendinst::rgLayer[layerIdx];
  if (!rgl)
    return;

  if (ri_future_pregen_cnt[layerIdx] < rgl->pregenEnt.size())
    ri_future_pregen_cnt[layerIdx] = rgl->pregenEnt.size();
  auto pregenEnt = make_span(rgl->pregenEnt.data(), ri_future_pregen_cnt[layerIdx]);
  G_ASSERTF(idx < pregenEnt.size(), "pregen_id=0x%x idx=%d pregenEnt.size()=%d rgl->pregenEnt.size()=%d", pregen_id, idx,
    pregenEnt.size(), rgl->pregenEnt.size());

  if (idx < rgl->pregenEnt.size())
    assign_res(pregenEnt[idx].riRes, &ri_res);
  else
    pregenEnt[idx].riRes = &ri_res; // it is not inited yet so we replace it without delref/addref
  pendingReinit = true;
}

bool rendinst::create_rt_rigen_data(float ofs_x, float ofs_z, float grid2world, int cell_sz, int cell_num_x, int cell_num_z,
  int per_inst_data_dwords, dag::ConstSpan<rendinst::gen::land::AssetData *> land_cls, float dens_map_px, float dens_map_pz)
{
  FOR_EACH_RG_LAYER_DO (rgl)
    return false;

  RendInstGenData::riGenValidateGeneratedCell = &validate_generated_cell;
  RendInstGenData::useDestrExclForPregenAdd = false;
  if (!rgLayer.size())
  {
    logwarn("implicit configurateRIGen() call");
    configurateRIGen(DataBlock::emptyBlock);
  }

  for (int layer = 0; layer < rgLayer.size(); layer++)
  {
    int l_cell_num_x = cell_num_x;
    int l_cell_num_z = cell_num_z;
    int l_cell_sz = cell_sz;
    int div = rgAttr[layer].cellSizeDivisor;
    if (cell_sz > div)
    {
      l_cell_num_x *= div;
      l_cell_num_z *= div;
      l_cell_sz /= div;
    }
    else
    {
      l_cell_num_x *= cell_sz;
      l_cell_num_z *= cell_sz;
      l_cell_sz = 1;
    }
    rendinst::rgLayer[layer] = alloc_rigen_data(ofs_x, ofs_z, grid2world, l_cell_sz, l_cell_num_x, l_cell_num_z, land_cls, layer,
      per_inst_data_dwords, dens_map_px, dens_map_pz);
    ri_future_pregen_cnt[layer] = rendinst::rgLayer[layer]->pregenEnt.size();
  }

  rendinst::rebuildRgRenderMasks();
  debug("rgRenderMaskO=0x%04X rgRenderMaskDS=0x%04X rgRenderMaskCMS=0x%04X", rgRenderMaskO, rgRenderMaskDS, rgRenderMaskCMS);

  rendinst::set_rigen_sweep_mask(lastSweep.bm, lastSweep.ox, lastSweep.oz, lastSweep.scale);
  rendinst::prepareRIGen(true);
  rendinst_alloc_coll_props(0);
  updateGlobalShadows();
  rigenNeedSyncPrepare = true;
  return true;
}
void rendinst::release_rt_rigen_data()
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    for (int i = 0; i < rgl->landCls.size(); i++)
      rgl->landCls[i].asset = nullptr;
  }
  rendinst::clearRIGen();
  memset(ri_future_pregen_cnt, 0, sizeof(ri_future_pregen_cnt));
  RendInstGenData::riGenValidateGeneratedCell = nullptr;
}

int rendinst::get_rigen_data_layers() { return rendinst::rgLayer.size(); }
RendInstGenData *rendinst::get_rigen_data(int layer_idx) { return rendinst::rgLayer[layer_idx]; }
int rendinst::get_rigen_layers_cell_div(int layer_idx)
{
  return (layer_idx >= 0 && layer_idx < rendinst::rgAttr.size()) ? rendinst::rgAttr[layer_idx].cellSizeDivisor : 1;
}

EditableHugeBitMap2d *rendinst::get_rigen_bit_mask(rendinst::gen::land::AssetData *land_cls)
{
  FOR_EACH_RG_LAYER_DO (rgl)
    for (int i = 0; i < rgl->landCls.size(); i++)
      if (rgl->landCls[i].asset == land_cls)
        return static_cast<EditableHugeBitMap2d *>(rgl->landCls[i].mask);
  return nullptr;
}

void rendinst::discard_rigen_rect(int gx0, int gz0, int gx1, int gz1)
{
  if (!rendinst::rgLayer.size() || !rendinst::rgLayer[0])
    return;

  IBBox2 r_discard(IPoint2(gx0, gz0), IPoint2(gx1, gz1));
  ScopedLockAllRgLayersForWrite lock;

  FOR_EACH_RG_LAYER_DO (rgl)
  {
    int cell_stride = rgl->cellNumW;
    int cell_sz = rgl->cellSz;
    for (int i = 0; i < rgl->cells.size(); i++)
      if (rgl->cells[i].rtData)
      {
        int cx = i % cell_stride, cz = i / cell_stride;
        IBBox2 r_cell(IPoint2(cx * cell_sz, cz * cell_sz), IPoint2(cx * cell_sz + cell_sz, cz * cell_sz + cell_sz));
        if (r_discard & r_cell)
        {
          rgl->rtData->loaded.delInt(i);
          del_it(rgl->cells[i].rtData);
          rigenNeedSyncPrepare = true;
        }
      }
  }
}
void rendinst::discard_rigen_all()
{
  if (!rendinst::rgLayer.size() || !rendinst::rgLayer[0])
    return;
  rendinst::regenerateRIGen();

  rigenNeedSyncPrepare = true;
  ScopedLockAllRgLayersForWrite lock;

  FOR_EACH_RG_LAYER_DO (rgl)
    for (int i = 0; i < rgl->cells.size(); i++)
      del_it(rgl->cells[i].rtData);
}

void rendinst::set_rigen_sweep_mask(rendinst::EditableHugeBitmask *bm, float ox, float oz, float scale)
{
  rendinst::gen::destrExcl.ox = ox;
  rendinst::gen::destrExcl.oz = oz;
  rendinst::gen::destrExcl.scale = scale;
#if _TARGET_PC_TOOLS_BUILD
  rendinst::gen::destrExcl.constBm = true;
#endif
  G_STATIC_ASSERT(sizeof(*bm) == sizeof(rendinst::gen::destrExcl.bm));
  if (bm)
    memcpy(&rendinst::gen::destrExcl.bm, bm, sizeof(*bm));
  else
    memset(&rendinst::gen::destrExcl.bm, 0, sizeof(rendinst::gen::destrExcl.bm));

  lastSweep.bm = bm;
  lastSweep.ox = ox;
  lastSweep.oz = oz;
  lastSweep.scale = scale;
}
void rendinst::enable_rigen_mask_generated(bool en) { RendInstGenData::maskGeneratedEnabled = en; }

void rendinst::prepare_rt_rigen_data_render(const Point3 &pos, const TMatrix &view_itm, const mat44f &proj_tm)
{
  if (pendingReinit)
  {
    rigenNeedSyncPrepare = true;
    FOR_EACH_RG_LAYER_DO (rgl)
      clearRigenRtdataAndReinitPregenEnt(rgl, ri_future_pregen_cnt[_layer]);

    rendinst::set_rigen_sweep_mask(lastSweep.bm, lastSweep.ox, lastSweep.oz, lastSweep.scale);
    rendinst::prepareRIGen();
    updateGlobalShadows();
    rendinst_alloc_coll_props(0);
    pendingReinit = false;
  }
  if (rigenNeedSyncPrepare)
  {
    FOR_EACH_RG_LAYER_DO (rgl)
      if (rgl->rtData)
        rgl->rtData->lastPoi.set(-1e6, 1e6);
    if (riGenPreparePools)
      riGenPreparePools(true);
  }

  rendinst::scheduleRIGenPrepare(make_span_const(&pos, 1));
  static int from_sun_directionVarId = get_shader_variable_id("from_sun_direction", true);
  Point3 sunDir(0, 1, 0);
  float dist = 0;
  if (from_sun_directionVarId >= 0)
  {
    Color4 dir = ShaderGlobal::get_color4_fast(from_sun_directionVarId);
    sunDir = Point3(dir.r, dir.g, dir.b);
    dist = 1000;
  }
  rendinst::updateRIGenImpostors(dist, sunDir, view_itm, proj_tm);
  if (rigenNeedSyncPrepare)
  {
    int64_t reft = ref_time_ticks();
    while (!rendinst::isRIGenPrepareFinished())
      sleep_msec(10);
    if (get_time_usec(reft) > 500000)
      debug("rigenNeedSyncPrepare: %d msec", get_time_usec(reft) / 1000);
    if (riGenPreparePools)
      riGenPreparePools(false);
  }
  rigenNeedSyncPrepare = false;
  rendinst::updateHeapVb();
}
bool rendinst::compute_rt_rigen_tight_ht_limits(int layer_idx, int cx, int cz, float &out_hmin, float &out_hdelta)
{
  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  out_hmin = 0;
  out_hdelta = 8192;
  if (!rgl)
    return false;
  int idx = cx + cz * rgl->cellNumW;
  G_ASSERT(idx >= 0 && idx < rgl->cells.size());
  del_it(rgl->cells[idx].rtData);
  {
    RendInstGenData::CellRtData *crt = rgl->generateCell(cx, cz);
    if (RendInstGenData::riGenValidateGeneratedCell && crt && crt->bbox.size())
      crt = RendInstGenData::riGenValidateGeneratedCell(rgl, crt, idx, cx, cz);
    del_it(crt);
    del_it(rgl->cells[idx].rtData);
    rgl->rtData->loaded.delInt(idx);
  }
  out_hmin = rgl->cells[idx].htMin;
  out_hdelta = rgl->cells[idx].htDelta;
  return true;
}

static void gen_rt_rigen_cells(RendInstGenData *rgl, const BBox3 &area)
{
  DEBUG_CP();
  float csz = rgl->grid2world * rgl->cellSz;
  float ox = rgl->world0x();
  float oz = rgl->world0z();

  ScopedLockWrite lock(rgl->rtData->riRwCs);
  for (int cz = floorf((area[0].z - oz) / csz), cze = ceilf((area[1].z - oz) / csz); cz < cze; cz++)
    for (int cx = floorf((area[0].x - ox) / csz), cxe = cx = ceilf((area[1].x - ox) / csz); cx < cxe; cx++)
    {
      int idx = cx + cz * rgl->cellNumW;
      if (rgl->cells[idx].isLoaded())
        continue;

      debug("generate %d,%d", cx, cz);
      RendInstGenData::CellRtData *crt = rgl->generateCell(cx, cz);
      if (RendInstGenData::riGenValidateGeneratedCell && crt && crt->bbox.size())
        crt = RendInstGenData::riGenValidateGeneratedCell(rgl, crt, idx, cx, cz);

      rgl->updateVb(*crt, idx);
      rgl->cells[idx].rtData = crt;
      rgl->rtData->loaded.addInt(idx);
      rgl->rtData->toLoad.delInt(idx);
      if (rgl->cells[idx].isReady())
        rgl->rtData->loadedCellsBBox += IPoint2(cx, cz);
    }
  DEBUG_CP();
}
void rendinst::generate_rt_rigen_main_cells(const BBox3 &area)
{
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      gen_rt_rigen_cells(rgl, area);
}
static void calc_box_extension_for_objects_in_grid(RendInstGenData *rgl, bbox3f &out_bbox)
{
  vec4f expandCellsRange = v_neg(v_perm_xzac(v_max(v_sub(rgl->rtData->maxCellBbox.bmax, v_splats(rgl->cellSz)), v_zero()),
    v_min(rgl->rtData->maxCellBbox.bmin, v_zero())));

  out_bbox.bmin = v_min(out_bbox.bmin, v_and(v_perm_xxyy(expandCellsRange), v_cast_vec4f(V_CI_MASK1010)));
  out_bbox.bmax = v_max(out_bbox.bmax, v_and(v_perm_zzww(expandCellsRange), v_cast_vec4f(V_CI_MASK1010)));
}
void rendinst::calculate_box_extension_for_objects_in_grid(bbox3f &out_bbox)
{
  v_bbox3_init_empty(out_bbox);
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      calc_box_extension_for_objects_in_grid(rgl, out_bbox);
}

static rendinst::rigen_gather_pos_t riGenGatherPosCB = nullptr;
static rendinst::rigen_gather_tm_t riGenGatherTmCB = nullptr;
static rendinst::rigen_calculate_mapping_t riGenCalculateIndicesCB = nullptr;

static void prepare_add_pregen(RendInstGenData::CellRtData &crt, int layer_idx, int per_inst_data_dwords, float ox, float oy, float oz,
  float cell_xz_sz, float cell_y_sz)
{
  static constexpr int SUBCELL_DIV = RendInstGenData::SUBCELL_DIV;

  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  Tab<Point4> poolP4(tmpmem);
  Tab<TMatrix> poolTM(tmpmem);
  Tab<int> poolPerInstDataP4, poolPerInstDataTM(tmpmem);
  Tab<char> sc_idx(tmpmem);
  int per_sc[SUBCELL_DIV * SUBCELL_DIV];
  int flg = layer_idx << riPoolBits;

  if (!crt.pregenAdd)
    crt.pregenAdd = new RendInstGenData::PregenEntRtAdd;

  crt.pregenAdd->needsUpdate = false;
  crt.pregenAdd->dataStor.clear();
  crt.pregenAdd->cntStor.clear();
  mem_set_0(crt.pregenAdd->scCntIdx);
  if (rgl->pregenEnt.size() > crt.pools.size())
  {
    logwarn("rgl->pregenEnt=%d > crt.pools=%d, skip prepare_add_pregen", rgl->pregenEnt.size(), crt.pools.size());
    return;
  }
  poolP4.reserve(1024);
  poolTM.reserve(1024);
  poolPerInstDataP4.reserve(1024 * per_inst_data_dwords);
  poolPerInstDataTM.reserve(1024 * per_inst_data_dwords);

  float box_sz = rgl->grid2world * rgl->cellSz / SUBCELL_DIV;
  Tab<Tab<int>> riLdCnt(tmpmem);
  riLdCnt.resize(rgl->pregenEnt.size() * SUBCELL_DIV * SUBCELL_DIV);

  Tab<int> riIndices(tmpmem);
  riIndices.resize(rgl->pregenEnt.size(), -1);
  riGenCalculateIndicesCB(riIndices, flg, riPoolBitsMask);

  for (int i = 0; i < rgl->pregenEnt.size(); i++)
    if (rgl->pregenEnt[i].riRes)
    {
      bool posInst = rgl->pregenEnt[i].posInst;
      bool paletteRotation = rgl->pregenEnt[i].paletteRotation;
      bool zeroInstSeeds = rgl->pregenEnt[i].zeroInstSeeds;
      int base = i * SUBCELL_DIV * SUBCELL_DIV;
      memset(per_sc, 0, sizeof(per_sc));
      if (posInst && !paletteRotation)
      {
        int start = poolP4.size();
        riGenGatherPosCB(poolP4, poolPerInstDataP4, !zeroInstSeeds, riIndices[i], i | flg, ox, oz, ox + cell_xz_sz, oz + cell_xz_sz);
        for (int j = start; j < poolP4.size(); j++)
        {
          int cx = clamp<int>((poolP4[j].x - ox) / box_sz, 0, SUBCELL_DIV - 1);
          int cz = clamp<int>((poolP4[j].z - oz) / box_sz, 0, SUBCELL_DIV - 1);
          riLdCnt[base + cz * SUBCELL_DIV + cx].push_back(j);
        }
      }
      else
      {
        int start = poolTM.size();
        riGenGatherTmCB(poolTM, poolPerInstDataTM, !zeroInstSeeds, riIndices[i], i | flg, ox, oz, ox + cell_xz_sz, oz + cell_xz_sz);
        for (int j = start; j < poolTM.size(); j++)
        {
          int cx = clamp<int>((poolTM[j][3][0] - ox) / box_sz, 0, SUBCELL_DIV - 1);
          int cz = clamp<int>((poolTM[j][3][2] - oz) / box_sz, 0, SUBCELL_DIV - 1);
          riLdCnt[base + cz * SUBCELL_DIV + cx].push_back(j);
        }
      }
    }
  if (poolP4.size() + poolTM.size() == 0)
    return;

  bool need_one_more_pass = false;
  // per_inst_data_dwords is manually added
  rendinst::gen::InstancePackData packData{ox, oy, oz, cell_xz_sz, cell_y_sz, 0};
  for (int s = 0; s < SUBCELL_DIV * SUBCELL_DIV; s++)
  {
    crt.pregenAdd->scCntIdx[s] = crt.pregenAdd->cntStor.size();
    for (int j = 0; j < rgl->pregenEnt.size(); j++)
      if (riLdCnt[j * SUBCELL_DIV * SUBCELL_DIV + s].size())
      {
        dag::ConstSpan<int> idx = riLdCnt[j * SUBCELL_DIV * SUBCELL_DIV + s];
        RendInstGenData::PregenEntCounter cnt;
        bool posInst = rgl->pregenEnt[j].posInst;
        bool paletteRotation = rgl->pregenEnt[j].paletteRotation;
        bool zeroInstSeeds = rgl->pregenEnt[j].zeroInstSeeds;
        int stride = (posInst ? 4 : (rendinst::tmInst12x32bit ? 24 : 12)) + (zeroInstSeeds ? 0 : per_inst_data_dwords * 2);
        int stor_s = crt.pregenAdd->dataStor.size();
        cnt.set(j, idx.size());

        crt.pregenAdd->cntStor.push_back(cnt);
        crt.pregenAdd->dataStor.reserve(cnt.riCount * stride);
        if (paletteRotation)
        {
          for (int k = 0; k < cnt.riCount; k++)
          {
            int32_t palette_id = -1;
            const TMatrix &tm = poolTM[idx[k]];
            const Point4 posScale = Point4::xyzV(tm.getcol(3), tm.getcol(1).length());
            rendinst::gen::RotationPaletteManager::Palette palette =
              rendinst::gen::get_rotation_palette_manager()->getPalette({layer_idx, j});
            G_ASSERT(palette.count > 0);
            rendinst::gen::RotationPaletteManager::clamp_euler_angles(palette, tm, &palette_id);
            G_ASSERTF(palette_id >= 0, "Inconsistent palette rotation for pregen ri #%d, <%s>", j, rgl->pregenEnt[j].riName.get());
            size_t size = rendinst::gen::pack_entity_pos_inst_16(packData, Point3::xyz(posScale), posScale.w, palette_id, nullptr);
            G_ASSERT(size == 4 * sizeof(int16_t));
            float y = posScale.y, q_y = (y - oy) / cell_y_sz;
            if (q_y < -32000.0 / 32767.0 || q_y > 1)
            {
              inplace_min(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmin).y, y);
              inplace_max(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmax).y, y);
              if (y > -11e3 && y < 10e3)
                need_one_more_pass = true;
              else
                continue;
            }
            else if (y < oy)
              inplace_min(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmin).y, y);
            int id = append_items(crt.pregenAdd->dataStor, size / sizeof(int16_t));
            rendinst::gen::pack_entity_pos_inst_16(packData, Point3::xyz(posScale), posScale.w, palette_id,
              &crt.pregenAdd->dataStor[id]);
            if (per_inst_data_dwords && !zeroInstSeeds)
              append_items(crt.pregenAdd->dataStor, per_inst_data_dwords * 2, (int16_t *)&poolPerInstDataTM[idx[k]]);
          }
        }
        else if (rgl->pregenEnt[j].posInst)
        {
          for (int k = 0; k < cnt.riCount; k++)
          {
            Point4 &p = poolP4[idx[k]];
            size_t size = rendinst::gen::pack_entity_pos_inst_16(packData, Point3(p.x, p.y, p.z), p.w, -1, nullptr);
            G_ASSERT(size == 4 * sizeof(int16_t));
            float y = p.y, q_y = (y - oy) / cell_y_sz;
            if (q_y < -32000.0 / 32767.0 || q_y > 1)
            {
              inplace_min(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmin).y, y);
              inplace_max(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmax).y, y);
              if (y > -11e3 && y < 10e3)
                need_one_more_pass = true;
              else
                continue;
            }
            else if (y < oy)
              inplace_min(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmin).y, y);
            int id = append_items(crt.pregenAdd->dataStor, size / sizeof(int16_t));
            rendinst::gen::pack_entity_pos_inst_16(packData, Point3(p.x, p.y, p.z), p.w, -1, &crt.pregenAdd->dataStor[id]);
            if (per_inst_data_dwords && !zeroInstSeeds)
              append_items(crt.pregenAdd->dataStor, per_inst_data_dwords * 2, (int16_t *)&poolPerInstDataP4[idx[k]]);
          }
        }
        else if (!rendinst::tmInst12x32bit)
        {
          for (int k = 0; k < cnt.riCount; k++)
          {
            TMatrix &tm = poolTM[idx[k]];
            size_t size = rendinst::gen::pack_entity_tm_16(packData, tm, nullptr);
            G_ASSERT(size == 12 * sizeof(int16_t));
            float y = tm.m[3][1], q_y = (y - oy) / cell_y_sz;
            if (q_y < -32000.0 / 32767.0 || q_y > 1)
            {
              inplace_min(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmin).y, y);
              inplace_max(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmax).y, y);
              if (y > -11e3 && y < 10e3)
                need_one_more_pass = true;
              else
                continue;
            }
            else if (y < oy)
              inplace_min(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmin).y, y);

            int id = append_items(crt.pregenAdd->dataStor, size / sizeof(int16_t));
            rendinst::gen::pack_entity_tm_16(packData, tm, &crt.pregenAdd->dataStor[id]);
            if (per_inst_data_dwords && !zeroInstSeeds)
              append_items(crt.pregenAdd->dataStor, per_inst_data_dwords * 2, (int16_t *)&poolPerInstDataTM[idx[k]]);
          }
        }
        else
        {
          for (int k = 0; k < cnt.riCount; k++)
          {
            TMatrix &tm = poolTM[idx[k]];
            size_t size = rendinst::gen::pack_entity_tm_32(packData, tm, nullptr);
            G_ASSERT(size == 12 * sizeof(int32_t));
            G_UNUSED(size);
            float y = tm.m[3][1], q_y = (y - oy) / cell_y_sz;
            if (q_y < -32000.0 / 32767.0 || q_y > 1)
            {
              inplace_min(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmin).y, y);
              inplace_max(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmax).y, y);
              if (y > -11e3 && y < 10e3)
                need_one_more_pass = true;
              else
                continue;
            }
            else if (y < oy)
              inplace_min(as_point3(&rendinst::gen::SingleEntityPool::bbox.bmin).y, y);

            int id = append_items(crt.pregenAdd->dataStor, 24);
            rendinst::gen::pack_entity_tm_32(packData, tm, &crt.pregenAdd->dataStor[id]);
            if (per_inst_data_dwords && !zeroInstSeeds)
              append_items(crt.pregenAdd->dataStor, per_inst_data_dwords * 2, (int16_t *)&poolPerInstDataTM[idx[k]]);
          }
        }
        if (!crt.pregenAdd->cntStor.empty())
          crt.pregenAdd->cntStor.back().riCount = (crt.pregenAdd->dataStor.size() - stor_s) / stride;
      }
  }
  crt.pregenAdd->scCntIdx[SUBCELL_DIV * SUBCELL_DIV] = crt.pregenAdd->cntStor.size();
  debug("crt.pregenAdd->cntStor=%d crt.pregenAdd->dataStor=%d rgl->pregenEnt=%d need_one_more_pass=%d (at %.0f,%.0f  sz=%.0f)",
    crt.pregenAdd->cntStor.size(), crt.pregenAdd->dataStor.size(), rgl->pregenEnt.size(), (int)need_one_more_pass, ox, oz, cell_xz_sz);
  if (need_one_more_pass)
  {
    crt.pregenAdd->needsUpdate = true;
    rigenNeedSyncPrepare = true;
  }
}

void rendinst::set_rt_pregen_gather_cb(rigen_gather_pos_t cb_pos, rigen_gather_tm_t cb_tm, rigen_calculate_mapping_t cb_ind,
  rigen_prepare_pools_t cb_prep)
{
  RendInstGenData::riGenPrepareAddPregenCB = (cb_pos || cb_tm) ? prepare_add_pregen : nullptr;
  riGenGatherPosCB = cb_pos;
  riGenGatherTmCB = cb_tm;
  riGenCalculateIndicesCB = cb_ind;
  riGenPreparePools = cb_prep;
}
void rendinst::notify_ri_moved(int ri_idx, float ox, float oz, float nx, float nz)
{
  int layer_idx = (ri_idx >> riPoolBits) & 0xF;
  ri_idx &= riPoolBitsMask;

  int idx0 = get_cell_idx_at(layer_idx, ox, oz);
  int idx1 = get_cell_idx_at(layer_idx, nx, nz);
  if (idx0 < 0 && idx1 < 0)
    return;

  RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
  if (idx0 == idx1)
  {
    ScopedLockWrite lock(rgl->rtData->riRwCs);
    if (rgl->rtData->loaded.hasInt(idx0))
      rgl->rtData->loaded.delInt(idx0);
    if (rgl->cells[idx0].rtData)
    {
      rgl->cells[idx0].rtData->clear();
      if (rgl->cells[idx0].rtData->pregenAdd)
        rgl->cells[idx0].rtData->pregenAdd->needsUpdate = true;
    }
  }
  else
  {
    ScopedLockWrite lock(rgl->rtData->riRwCs);
    if (idx0 >= 0)
    {
      if (rgl->rtData->loaded.hasInt(idx0))
        rgl->rtData->loaded.delInt(idx0);
      del_it(rgl->cells[idx0].rtData);
    }
    if (idx1 >= 0)
    {
      if (rgl->rtData->loaded.hasInt(idx1))
        rgl->rtData->loaded.delInt(idx1);
      del_it(rgl->cells[idx1].rtData);
    }
  }
  rigenNeedSyncPrepare = true;
}

void rendinst::notify_ri_deleted(int ri_idx, float ox, float oz)
{
  int layer_idx = (ri_idx >> riPoolBits) & 0xF;
  ri_idx &= riPoolBitsMask;
  if (RendInstGenData::CellRtData *crt = get_cell_rt_at(layer_idx, ox, oz))
  {
    RendInstGenData *rgl = rendinst::rgLayer[layer_idx];
    int idx = get_cell_idx_at(layer_idx, ox, oz);
    ScopedLockWrite lock(rgl->rtData->riRwCs);
    if (rgl->rtData->loaded.hasInt(idx))
      rgl->rtData->loaded.delInt(idx);
    del_it(rgl->cells[idx].rtData);
  }
  rigenNeedSyncPrepare = true;
}

void rendinst::set_sun_dir_for_global_shadows(const Point3 &sun_dir)
{
  if (memcmp(&sun_dir, &curSunDir, sizeof(curSunDir)) == 0)
    return;
  curSunDir = sun_dir;
  debug("update sundir for RI: %@", curSunDir);
  updateGlobalShadows();
}
void rendinst::set_global_shadows_needed(bool need)
{
  if (!RendInstGenData::renderResRequired)
    need = false;
  if (!globalShadowsNeeded && need)
    rendinst::render::renderRIGenGlobalShadowsToTextures(curSunDir);
  globalShadowsNeeded = need;
}

void rendinst::get_layer_idx_and_ri_idx_from_pregen_id(int pregen_id, int &layer_idx, int &ri_idx)
{
  layer_idx = (pregen_id >> riPoolBits) & 0xF;
  ri_idx = pregen_id & riPoolBitsMask;
}

rendinst::GetRendInstMatrixByRiIdxResult rendinst::get_rendinst_matrix_by_ri_idx(int layer_idx, int ri_idx, const Point3 &position,
  TMatrix &out_tm)
{
  const RendInstGenData *riGen = rendinst::rgLayer[layer_idx];
  if (!riGen)
    return rendinst::GetRendInstMatrixByRiIdxResult::Failure;

  ScopedLockRead lock(riGen->rtData->riRwCs);

  const int cellIdx = get_cell_idx_at(layer_idx, position.x, position.z);
  if (cellIdx < 0)
    return rendinst::GetRendInstMatrixByRiIdxResult::Failure;

  const RendInstGenData::Cell &cell = riGen->cells[cellIdx];
  const RendInstGenData::CellRtData *cellRt = cell.isReady();
  if (!cellRt)
    return rendinst::GetRendInstMatrixByRiIdxResult::NonReady;

  const unsigned int stride =
    RIGEN_STRIDE_B(cellRt->rtData->riPosInst[ri_idx], cellRt->rtData->riZeroInstSeeds[ri_idx], riGen->perInstDataDwords);

  if (cellRt->pools[ri_idx].total - cellRt->pools[ri_idx].avail < 1)
    return rendinst::GetRendInstMatrixByRiIdxResult::Failure;

  bool foundNear = false;
  mat44f nearTm44;

  for (int i = 0; i < cellRt->pools[ri_idx].total; ++i)
  {
    rendinst::RendInstDesc riDesc(cellIdx, i, ri_idx, 0, layer_idx);

    const int16_t *data = (int16_t *)(cellRt->sysMemData.get() + cellRt->pools[ri_idx].baseOfs + i * stride);
    mat44f tm44;
    if (!riutil::get_rendinst_matrix(riDesc, const_cast<RendInstGenData *>(riGen), data, &cell, tm44))
      continue;

    const Point3 currentPos(v_extract_x(tm44.col3), v_extract_y(tm44.col3), v_extract_z(tm44.col3));
    if (
      fabsf(position.x - currentPos.x) > 0.01f || fabsf(position.y - currentPos.y) > 0.01f || fabsf(position.z - currentPos.z) > 0.01f)
      continue;

    if (foundNear)
      return rendinst::GetRendInstMatrixByRiIdxResult::MultipleInstancesAtThePostion;

    foundNear = true;
    nearTm44 = tm44;
  }

  if (foundNear)
  {
    v_mat_43cu_from_mat44(out_tm.array, nearTm44);
    return rendinst::GetRendInstMatrixByRiIdxResult::Success;
  }

  return rendinst::GetRendInstMatrixByRiIdxResult::Failure;
}

// To emulate RendInstGenData::generateCell accurately we have to unpack Y and repack it.
// The same happens to it in generateCell because of rendinst::gen::custom_update_pregen_pos_y.
static inline void requantizeY(int16_t &int16_y, float cell_add_y, float cell_mul_y)
{
  const float floatY = ((int16_y / 32767.0f) * cell_mul_y) + cell_add_y;
  int16_y = int16_t(clamp((floatY - cell_add_y) / cell_mul_y, -1.0f, 1.0f) * 32767.0);
}

rendinst::QuantizeRendInstMatrixResult rendinst::quantize_rendinst_matrix(int layer_idx, int ri_idx, const TMatrix &in_tm,
  TMatrix &out_tm)
{
  const RendInstGenData *riGen = rendinst::rgLayer[layer_idx];
  if (!riGen)
    return rendinst::QuantizeRendInstMatrixResult::Failure;

  ScopedLockRead lock(riGen->rtData->riRwCs);

  const Point3 position = in_tm.getcol(3);
  const int cellIdx = get_cell_idx_at(layer_idx, position.x, position.z);
  if (cellIdx < 0)
    return rendinst::QuantizeRendInstMatrixResult::Failure;

  const RendInstGenData::Cell &cell = riGen->cells[cellIdx];
  const RendInstGenData::CellRtData *cellRt = cell.isReady();
  if (!cellRt)
    return rendinst::QuantizeRendInstMatrixResult::NonReady;

  const float cellSz = riGen->grid2world * riGen->cellSz;
  const vec3f v_cell_add = cellRt->cellOrigin;
  const vec3f v_cell_mul = v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(cellSz, cellRt->cellHeight, cellSz, 0));
  int16_t data[12];
  mat44f unpackedTm44;

  rendinst::gen::InstancePackData instancePackData;
  instancePackData.ox = v_extract_x(cellRt->cellOrigin);
  instancePackData.oy = v_extract_y(cellRt->cellOrigin);
  instancePackData.oz = v_extract_z(cellRt->cellOrigin);
  instancePackData.cell_xz_sz = cellSz;
  instancePackData.cell_y_sz = cellRt->cellHeight;
  instancePackData.per_inst_data_dwords = 0;

  if (riGen->pregenEnt[ri_idx].posInst)
  {
    const float scale = in_tm.getcol(1).length();
    int paletteId = -1;

    if (riGen->pregenEnt[ri_idx].paletteRotation)
    {
      rendinst::gen::RotationPaletteManager::Palette palette =
        rendinst::gen::get_rotation_palette_manager()->getPalette({layer_idx, ri_idx});
      rendinst::gen::RotationPaletteManager::clamp_euler_angles(palette, in_tm, &paletteId);

      rendinst::gen::pack_entity_pos_inst_16(instancePackData, in_tm.getcol(3), scale, paletteId, data);
      requantizeY(data[1], instancePackData.oy, instancePackData.cell_y_sz);

      vec4f unpackedPos, unpackedScale;
      vec4i unpackedPaletteId;
      rendinst::gen::unpack_tm_pos(unpackedPos, unpackedScale, data, v_cell_add, v_cell_mul, true, &unpackedPaletteId);

      quat4f rotation = rendinst::gen::RotationPaletteManager::get_quat(palette, v_extract_xi(unpackedPaletteId));
      v_mat44_compose(unpackedTm44, unpackedPos, rotation, unpackedScale);
    }
    else
    {
      rendinst::gen::pack_entity_pos_inst_16(instancePackData, in_tm.getcol(3), scale, paletteId, data);
      requantizeY(data[1], instancePackData.oy, instancePackData.cell_y_sz);
      rendinst::gen::unpack_tm_pos(unpackedTm44, data, v_cell_add, v_cell_mul, false);
    }
  }
  else
  {
    { // This is the inverse of rendinst::gen::unpack_tm_full. rendinst::gen::pack_entity_16 was not precise enough.
      mat44f tm44;
      v_mat44_make_from_43cu(tm44, in_tm.array);

      mat44f m;
      m.col0 = v_div(tm44.col0, rendinst::gen::VC_1div256);
      m.col1 = v_div(tm44.col1, rendinst::gen::VC_1div256);
      m.col2 = v_div(tm44.col2, rendinst::gen::VC_1div256);
      m.col3 = v_div(v_sub(tm44.col3, v_cell_add), v_cell_mul);

      mat43f m43;
      v_mat44_transpose_to_mat43(m43, m);

      vec4i row0 = v_cvt_floori(m43.row0);
      data[0] = v_extract_xi(row0);
      data[1] = v_extract_yi(row0);
      data[2] = v_extract_zi(row0);
      data[3] = v_extract_wi(row0);

      vec4i row1 = v_cvt_floori(m43.row1);
      data[4] = v_extract_xi(row1);
      data[5] = v_extract_yi(row1);
      data[6] = v_extract_zi(row1);
      data[7] = v_extract_wi(row1);

      vec4i row2 = v_cvt_floori(m43.row2);
      data[8] = v_extract_xi(row2);
      data[9] = v_extract_yi(row2);
      data[10] = v_extract_zi(row2);
      data[11] = v_extract_wi(row2);
    }

    requantizeY(data[7], instancePackData.oy, instancePackData.cell_y_sz);
    rendinst::gen::unpack_tm_full(unpackedTm44, data, v_cell_add, v_cell_mul);
  }

  v_mat_43cu_from_mat44(out_tm.array, unpackedTm44);
  return rendinst::QuantizeRendInstMatrixResult::Success;
}

unsigned rendinst::precompute_ri_cells_for_stats(DataBlock &out_stats)
{
  debug("%s started...", __FUNCTION__);
  int total_ri_count = 0;
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
  {
    DataBlock &blk = *out_stats.addNewBlock("rgl");
    int layer_ri_count = 0;
    blk.setIPoint2("cells", IPoint2(rgl->cellNumW, rgl->cellNumH));
    blk.setPoint2("world0", Point2(rgl->world0x(), rgl->world0z()));
    blk.setInt("cellSz", rgl->cellSz);
    blk.setReal("grid2world", rgl->grid2world);
    for (int z = 0; z < rgl->cellNumH; z++)
    {
      DataBlock &r_blk = *blk.addNewBlock("cellsRow");
      for (int x = 0; x < rgl->cellNumW; x++)
      {
        RendInstGenData::CellRtData *crt = new RendInstGenData::CellRtData(rgl->rtData->riRes.size(), rgl->rtData);
        int ri_count = rgl->precomputeCell(*crt, x, z);
        float x0 = x * rgl->grid2world * rgl->cellSz + rgl->world0x();
        float z0 = z * rgl->grid2world * rgl->cellSz + rgl->world0z();
        r_blk.addPoint4("riCntInCell", Point4(ri_count, crt->vbSize, x0, z0));
        total_ri_count += ri_count;
        layer_ri_count += ri_count;
        delete crt;
      }
    }
    blk.setInt("totalRI", layer_ri_count);
  }
  debug("%s done, ri_count=%d", __FUNCTION__, total_ri_count);
  return total_ri_count;
}
