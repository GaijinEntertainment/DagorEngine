// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riGen/landClassData.h"
#include "riGen/genObjByDensGridMask.h"
#include "riGen/genObjByMask.h"
#include "riGen/genObjUtil.h"
#include "render/genRender.h"
#include "riGen/riGenExtra.h"
#include "riGen/riRotationPalette.h"
#include "riGen/riGenData.h"

#include <generic/dag_sort.h>
#include <util/dag_finally.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/gpuObjects.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <regExp/regExp.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_stdGameRes.h>
#include <math/dag_Point4.h>
#include <math/dag_vecMathCompatibility.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_btagCompr.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <memory/dag_framemem.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_mathUtils.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/unique_ptr.h>
#include <supp/dag_alloca.h>


#define debug(...) logmessage(_MAKE4C('RGEN'), __VA_ARGS__)


static constexpr unsigned UNLOAD_JOB_TAG = 0x40000000;
static constexpr unsigned REGEN_JOB_TAG = 0x20000000;

namespace rendinst
{
std::atomic<bool> isRiGenLoaded = false;
extern int maxRiGenPerCell, maxRiExPerCell;
extern bool forceRiExtra;
} // namespace rendinst

static inline IGenLoad &ptr_to_ref(IGenLoad *crd) { return *crd; }

static int riGenJobId = -1;
static int externalJobId = -1;
static int riCellPoolSz = 16;
using rendinst::regCollCb;
using rendinst::unregCollCb;
static Tab<TMatrix> full_sweep_box_itm_list(inimem);
static Tab<BBox2> full_sweep_wbox_list(inimem);
static bool riGenSecLayerEnabled = true;
static FastNameMap riExtraSubstNames;
static bool rendinstExtraAutoSubst = false;
static int riExtraSubstFaceThres = 4000;
static float riExtraSubstFaceDensThres = 1e6;
static int riExtraSubstPregenThres = 32;

static FastNameMap riFullLodsNames, riHideNames, riHideMode0Names, riHideMode1Names;
static const int numHideModes = 3;
static carray<Tab<RegExp *>, numHideModes> riHideModeNamesRE;
static carray<Tab<RegExp *>, numHideModes> riHideModeNamesExclRE;
static carray<float, numHideModes> riHideModeMinRad2 = {0};
static eastl::unique_ptr<rendinst::gen::RotationPaletteManager> rotationPaletteManager;

static bool riAllowGenJobsQueueReorder = false;

rendinst::gen::RotationPaletteManager *rendinst::gen::get_rotation_palette_manager() { return rotationPaletteManager.get(); }

static void checkAndAddAutoRiExtra(RenderableInstanceLodsResource *res, const char *name, int /*res_idx*/,
  dag::ConstSpan<RendInstGenData::Cell> /*cells*/, dag::ConstSpan<RendInstGenData::LandClassRec> /*landCls*/)
{
  if (!name || !rendinstExtraAutoSubst)
    return;

  if (riExtraSubstNames.getNameId(name) >= 0)
    return;
  const DataBlock *b = gameres_rendinst_desc.getBlockByName(name);
  if (!b && res)
  {
    DataBlock *bnew = gameres_rendinst_desc.addBlock(name);
    bnew->setInt("lods", res->lods.size());
    bnew->setBool("hasImpostor", res->hasImpostor());
    bnew->setPoint3("bbox0", res->bbox[0]);
    bnew->setPoint3("bbox1", res->bbox[1]);
    bnew->setPoint4("bsph", Point4::xyzV(res->bsphCenter, res->bsphRad));
    rendinst::addRiExtraRefs(bnew, nullptr, name);

    b = bnew;
    debug("checkAndAddAutoRiExtra(%s) was not found in riDesc.bin, use res=%p: %d lods %s", name, res, b->getInt("lods", 0),
      b->getBool("hasImpostor", false) ? "(impostor)" : "");
  }
  if (!b)
    return;

  int has_imp = b->getBool("hasImpostor", false);
  if (!has_imp)
    riExtraSubstNames.addNameId(name);
}

bool rendinst::is_ri_extra_for_inst_count_only(const char *name)
{
  const DataBlock *b = gameres_rendinst_desc.getBlockByName(name);
  if (!b)
    return false;

  int lods_count = b->getInt("lods", 0);
  int face_cnt = b->getInt("faces", 0);
  int has_imp = b->getBool("hasImpostor", false);
  Point3 box_wd = b->getPoint3("bbox1", Point3(1, 1, 1)) - b->getPoint3("bbox0", Point3(0, 0, 0));
  float dens = safediv(face_cnt, (box_wd.x * box_wd.y * box_wd.z));

  if (lods_count > rendinst::RIEXTRA_LODS_THRESHOLD)
    return false;

  if (RenderableInstanceLodsResource::get_skip_first_lods_count &&
      RenderableInstanceLodsResource::get_skip_first_lods_count(name, has_imp, lods_count) == 0)
    return true;

  if (has_imp)
    return true;

  if (face_cnt > riExtraSubstFaceThres || dens > riExtraSubstFaceDensThres)
    return false;

  return !rendinst::isRIGenExtraUsedInDestr(name);
}

int rendinst::get_skip_nearest_lods(const char *name, bool has_impostors, int total_lods)
{
  return !has_impostors && total_lods > 1 && riFullLodsNames.getNameId(name) < 0 ? (riHideNames.getNameId(name) < 0 ? 1 : total_lods)
                                                                                 : 0;
}

static void gather_sweep_boxes(Tab<const TMatrix *> &list, float x0, float z0, float dx, float dz)
{
  BBox2 bb(x0, z0, x0 + dx, z0 + dz);

  list.clear();
  for (int i = 0; i < full_sweep_box_itm_list.size(); i++)
    if (bb & full_sweep_wbox_list[i])
      list.push_back(&full_sweep_box_itm_list[i]);
  // DEBUG_CTX("gathered %d/%d (%.3f,%.3f)-(%.3f,%.3f)", list.size(), full_sweep_box_itm_list.size(), x0, z0, x1, z1);
}

static void gather_and_set_sweep_boxes(Tab<const TMatrix *> &list, float x0, float z0, float dx, float dz,
  dag::ConstSpan<const TMatrix *> src_m)
{
  BBox2 bb(x0, z0, x0 + dx, z0 + dz);

  list.clear();
  for (int i = 0; i < src_m.size(); i++)
    if (bb & full_sweep_wbox_list[src_m[i] - full_sweep_box_itm_list.data()])
      list.push_back(src_m[i]);

  rendinst::gen::SingleEntityPool::sweep_boxes_itm.set(list);
  // DEBUG_CTX("  gathered %d/%d (%.3f,%.3f)-(%.3f,%.3f) %p,%d", list.size(), src_m.size(), x0, z0, x1, z1,
  //   rendinst::gen::SingleEntityPool::sweep_boxes_itm.data(), rendinst::gen::SingleEntityPool::sweep_boxes_itm.size());
}

RendInstGenData *RendInstGenData::create(IGenLoad &crd, int layer_idx)
{
  G_ASSERT(rendinst::gen::get_rotation_palette_manager());
  RendInstGenData *dump = nullptr;
  unsigned fmt = 0;
  crd.beginBlock(&fmt);
  {
    IGenLoad *zcrd_p = nullptr;
    if (fmt == btag_compr::OODLE)
    {
      int src_sz = crd.readInt();
      zcrd_p = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE) OodleLoadCB(crd, crd.getBlockRest(), src_sz);
    }
    else if (fmt == btag_compr::ZSTD)
      zcrd_p = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(crd, crd.getBlockRest());
    else
      zcrd_p = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(crd, crd.getBlockRest());
    IGenLoad &zcrd = *zcrd_p;

    int dump_sz = zcrd.readInt();
    dump = (RendInstGenData *)midmem->tryAlloc(dump_sz);
    if (!dump)
    {
      logerr("%s can't allocate dump of %dK", __FUNCTION__, dump_sz >> 10);
    create_fail:
      zcrd.ceaseReading();
      zcrd.~IGenLoad();
      del_it(dump);
      crd.endBlock();
      return nullptr;
    }
    zcrd.read(dump, dump_sz);

    dump->landCls.patch(dump);
    dump->cells.patch(dump);
    dump->pregenEnt.patch(dump);
    for (int i = 0; i < dump->cells.size(); i++)
    {
      dump->cells[i].cls.patch(dump);
      for (int j = 0; j < SUBCELL_DIV * SUBCELL_DIV + 1; j++)
        dump->cells[i].entCnt[j].patch(dump);
    }
    for (int i = 0; i < dump->pregenEnt.size(); i++)
      dump->pregenEnt[i].riName.patch(dump);

    if (rendinst::isRgLayerPrimary(layer_idx))
    {
      dump->sweepMask = RoHugeHierBitMap2d<4, 3>::create(zcrd);
      if (!dump->sweepMask)
        goto create_fail;
    }
    else
      dump->sweepMask = nullptr;
    for (int i = 0; i < dump->landCls.size(); i++)
    {
      dump->landCls[i].landClassName.patch(dump);
      dump->landCls[i].mask = RoHugeHierBitMap2d<4, 3>::create(zcrd);
      if (!dump->landCls[i].mask)
        goto create_fail;
    }
    zcrd.ceaseReading();
    zcrd.~IGenLoad();
  }
  crd.endBlock();

  crd.beginBlock();
  if (dump->pregenEnt.size())
  {
    dump->fpLevelBin = df_open(crd.getTargetName(), DF_READ);
    dump->pregenDataBaseOfs = crd.tell();
    debug("%s %08X sz=%d -> %p", crd.getTargetName(), crd.tell(), crd.getBlockRest(), dump->fpLevelBin);
  }
  crd.endBlock();

  if (rendinst::isRgLayerPrimary(layer_idx))
  {
    rendinst::gen::lcmapExcl.initExplicit(dump->world0x(), dump->world0z(), dump->sweepMaskScale, dump->sweepMask);
    rendinst::gen::destrExcl.clear();
  }
  return dump;
}

RendInstGenData::~RendInstGenData()
{
  if (rtData)
    rtData->riRwCs.lockWrite();
  for (int i = 0; i < landCls.size(); i++)
  {
    if (landCls[i].asset != &rendinst::gen::land::AssetData::emptyLandStub)
      release_game_resource((GameResource *)landCls[i].asset);
    landCls[i].asset = nullptr;
    delete landCls[i].mask;
  }
  for (int i = 0; i < cells.size(); i++)
    del_it(cells[i].rtData);

  if (unregCollCb && rtData)
    for (int i = 0; i < rtData->riCollRes.size(); i++)
      unregCollCb(rtData->riCollRes[i].handle);
  for (int i = 0; i < pregenEnt.size(); i++)
  {
    release_game_resource((GameResource *)pregenEnt[i].riRes.get());
    if (rtData && rtData->riCollRes.size())
    {
      G_ASSERT(i < rtData->riCollRes.size());
      release_game_resource((GameResource *)rtData->riCollRes[i].collRes);
    }
  }
  pregenEnt.init(nullptr, 0);

  del_it(sweepMask);
  if (rtData)
    rtData->clear();

  if (fpLevelBin)
    df_close(fpLevelBin);
  fpLevelBin = 0;
  pregenDataBaseOfs = 0;

  RtData *rt = rtData;
  if (rtData)
  {
    rt->updateVbResetCS.lock("riGen::vbLock");

    rtData = nullptr;
    rt->updateVbResetCS.unlock();
    rt->riRwCs.unlockWrite();
  }
  del_it(rt);
}

int rendinst::getPersistentPackType(RenderableInstanceLodsResource *res, int def)
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    intptr_t idx = rgl->rtData ? find_value_idx(rgl->rtData->riRes, res) : -1;
    if (idx >= 0)
    {
      if (riExtraSubstNames.getNameId(rgl->rtData->riResName[idx]) >= 0)
        return 2;
      else
        return rgl->rtData->riPosInst[idx] ? 1 : 0;
    }
  }
  return def;
}

struct SortByAtest
{
  dag::ConstSpan<RendInstGenData::ElemMask> mask;
  dag::ConstSpan<const char *> names;
  SortByAtest(dag::ConstSpan<RendInstGenData::ElemMask> mask, dag::ConstSpan<const char *> names_) : mask(mask), names(names_) {}
  bool operator()(const uint32_t a, const uint32_t b) const
  {
    int atestA = mask[a * rendinst::MAX_LOD_COUNT + 0].atest ? 1 : 0;
    int atestB = mask[b * rendinst::MAX_LOD_COUNT + 0].atest ? 1 : 0;
    int cullA = mask[a * rendinst::MAX_LOD_COUNT + 0].cullN ? 1 : 0;
    int cullB = mask[b * rendinst::MAX_LOD_COUNT + 0].cullN ? 1 : 0;
    for (int i = 1; i < rendinst::MAX_LOD_COUNT; ++i)
    {
      atestA |= mask[a * rendinst::MAX_LOD_COUNT + i].atest ? 1 : 0;
      atestB |= mask[b * rendinst::MAX_LOD_COUNT + i].atest ? 1 : 0;
      cullA |= mask[a * rendinst::MAX_LOD_COUNT + i].cullN ? 1 : 0;
      cullB |= mask[b * rendinst::MAX_LOD_COUNT + i].cullN ? 1 : 0;
    }
    if (atestA == atestB)
    {
      if (cullA == cullB)
        return (!names[a] || !names[b]) ? a < b : stricmp(names[a], names[b]) < 0;
      return cullA < cullB;
    }
    return atestA < atestB;
  }
};

namespace
{
struct PregenEntCounterOld
{
  uint32_t riResIdxLow : 10, riCount : 20, riResIdxHigh : 2;
};
} // namespace

void RendInstGenData::beforeReleaseRtData()
{
  if (!rtData || !rtData->entCntOldPtr)
    return;
  G_ASSERT(!(dataFlags & DFLG_PREGEN_ENT32_CNT32));
  auto oldBase = (PregenEntCounterOld *)rtData->entCntOldPtr;
  for (int i = 0; i < cells.size(); i++)
    for (int j = 0; j < SUBCELL_DIV * SUBCELL_DIV + 1; j++)
      cells[i].entCnt[j].setPtr(oldBase + (cells[i].entCnt[j].get() - rtData->entCntForOldFmt.data()));
  rtData->entCntOldPtr = nullptr;
  clear_and_shrink(rtData->entCntForOldFmt);
}
void RendInstGenData::prepareRtData(int layer_idx)
{
  rtData = new RtData(layer_idx);
  if (!(dataFlags & DFLG_PREGEN_ENT32_CNT32) && cells.size())
  {
    auto oldPtr = (PregenEntCounterOld *)cells.front().entCnt.front().get();
    auto oldCnt = (PregenEntCounterOld *)cells.back().entCnt.back().get() - oldPtr;
    rtData->entCntOldPtr = oldPtr;
    rtData->entCntForOldFmt.resize(oldCnt);
    auto newPtr = rtData->entCntForOldFmt.data();
    for (unsigned j = 0; j < oldCnt; j++)
      newPtr[j].set((oldPtr[j].riResIdxHigh << 10) + oldPtr[j].riResIdxLow, oldPtr[j].riCount);

    for (int i = 0; i < cells.size(); i++)
      for (int j = 0; j < SUBCELL_DIV * SUBCELL_DIV + 1; j++)
      {
        auto p = ((PregenEntCounterOld *)cells[i].entCnt[j].get());
        cells[i].entCnt[j].setPtr(newPtr + (p - oldPtr));
      }
    if (rtData->entCntForOldFmt.size())
      logwarn("using rtData->entCntForOldFmt.size()=%d (extra %d bytes) for cells.size()=%d due to older RIGz format",
        rtData->entCntForOldFmt.size(), data_size(rtData->entCntForOldFmt), cells.size());
  }

  int nReserve = pregenEnt.size();
  for (auto &lc : landCls)
    nReserve += lc.asset ? lc.asset->riRes.size() : 0;
  rtData->riRes.reserve(nReserve);
  rtData->riCollRes.reserve(nReserve);
  rtData->riPosInst.reserve(nReserve);
  rtData->riPaletteRotation.reserve(nReserve);
  rtData->riZeroInstSeeds.reserve(nReserve);
  rtData->riColPair.reserve(nReserve * 2);
  rtData->riResName.reserve(nReserve);
  rtData->preloadDistance = 64 + rendinst::rgAttr[layer_idx].poiRad;
  rtData->transparencyDeltaRcp = rtData->preloadDistance * 0.1f;

  for (int i = 0; i < pregenEnt.size(); i++)
  {
    PatchablePtr<RenderableInstanceLodsResource> &riRes = pregenEnt[i].riRes;
    if (!riRes)
    {
      riRes = (RenderableInstanceLodsResource *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(pregenEnt[i].riName),
        RendInstGameResClassId);
      if (!riRes)
      {
        logerr("pregen[%d].riName=<%s> missing", i, pregenEnt[i].riName);
        riRes = rendinst::get_stub_res();
      }

      if (riRes)
      {
        checkAndAddAutoRiExtra(riRes, pregenEnt[i].riName, i, cells, landCls);

        if (riRes->lods.size() > rendinst::MAX_LOD_COUNT)
        {
          if (riExtraSubstNames.getNameId(pregenEnt[i].riName) < 0)
            logwarn("Invalid number of LODs (%d) in '%s' (only %d will be used).", riRes->lods.size(), pregenEnt[i].riName,
              rendinst::MAX_LOD_COUNT);
          else if (riRes->lods.size() > rendinst::RiExtraPool::MAX_LODS)
            logwarn("Invalid number of LODs (%d) in '%s' (only %d will be used).", riRes->lods.size(), pregenEnt[i].riName,
              rendinst::RiExtraPool::MAX_LODS);
        }
      }
    }
    // debug("ri[%d] <%s> =%p (%08X-%08X  %d)",
    //   i, pregenEnt[i].riName, riRes, pregenEnt[i].colPair[0].u, pregenEnt[i].colPair[1].u, pregenEnt[i].posInst);

    // It still might nullptr on the dedicated.
    // The dedicated does not have rendinst::get_stub_res() - it's render only thing.
    if (riRes)
      riRes->loadImpostorData(pregenEnt[i].riName);

    rtData->riRes.push_back(riRes);
    rtData->riPosInst.push_back(pregenEnt[i].posInst);
    rtData->riPaletteRotation.push_back(pregenEnt[i].paletteRotation);
    rtData->riZeroInstSeeds.push_back((perInstDataDwords != 0) ? pregenEnt[i].zeroInstSeeds : 0);
    G_ASSERT(rtData->riPosInst.size() == rtData->riPaletteRotation.size());
    G_ASSERT(rtData->riPosInst.size() == rtData->riZeroInstSeeds.size());
    append_items(rtData->riColPair, 2, pregenEnt[i].colPair.data());
    rtData->riResName.push_back(pregenEnt[i].riName);

    if (!pregenEnt[i].riName.get() || !*pregenEnt[i].riName.get())
    {
      memset(&rtData->riCollRes.push_back(), 0, sizeof(CollisionResData));
      continue;
    }

    String coll_name(128, "%s" RI_COLLISION_RES_SUFFIX, pregenEnt[i].riName.get());
    FATAL_CONTEXT_AUTO_SCOPE(coll_name);
    CollisionResource *collRes =
      get_resource_type_id(coll_name) == CollisionGameResClassId
        ? (CollisionResource *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(coll_name), CollisionGameResClassId)
        : nullptr;
    if (!collRes && pregenEnt[i].posInst)
      logwarn("missing tree-RI-collRes: %s", coll_name);
    if (collRes && collRes->getAllNodes().empty())
    {
      release_game_resource((GameResource *)collRes);
      collRes = nullptr;
    }
    if (collRes)
      collRes->collapseAndOptimize(USE_GRID_FOR_RI);
    CollisionResData &collResData = rtData->riCollRes.push_back();
    collResData.collRes = collRes;
    collResData.handle = nullptr;
    if (collRes && regCollCb)
      collResData.handle = regCollCb(collRes, coll_name.str());
  }

  if (!landCls.size() && !pregenEnt.size())
    return;

  for (int i = 0; i < landCls.size(); i++)
  {
    G_ASSERTF(landCls[i].asset && landCls[i].asset != &rendinst::gen::land::AssetData::emptyLandStub,
      "land class asset <%s> was not exported correctly for generation.\n"
      "Add export {type:t=\"land\"} to application.blk and run daBuild or switch off projectDefaults{riMgr {gen {'PLATFORM':b = "
      "true}}}",
      landCls[i].landClassName.get());
    int cnt = landCls[i].asset->riRes.size();
    int base = append_items(rtData->riResMapStor, cnt);
    for (int j = 0; j < cnt; j++)
    {
      checkAndAddAutoRiExtra(landCls[i].asset->riRes[j], landCls[i].asset->riResName[j], -1,
        make_span<RendInstGenData::Cell>(nullptr, 0), make_span<RendInstGenData::LandClassRec>(nullptr, 0));

      if ((rtData->riResMapStor[base + j] = find_value_idx(rtData->riRes, landCls[i].asset->riRes[j])) < 0)
      {
        rtData->riResMapStor[base + j] = append_items(rtData->riRes, 1, &landCls[i].asset->riRes[j]);

        CollisionResData &collResData = rtData->riCollRes.push_back();
        collResData.collRes = landCls[i].asset->riCollRes[j];
        if (landCls[i].asset->riCollRes[j] && landCls[i].asset->riCollRes[j]->getAllNodes().empty())
          collResData.collRes = nullptr;
        collResData.handle = regCollCb ? regCollCb(collResData.collRes, landCls[i].asset->riResName[j]) : nullptr;

        rtData->riPosInst.push_back(landCls[i].asset->riPosInst[j]);
        rtData->riPaletteRotation.push_back(landCls[i].asset->riPaletteRotation[j]);
        rtData->riZeroInstSeeds.push_back(false);
        append_items(rtData->riColPair, 2, &landCls[i].asset->riColPair[j * 2]);
        rtData->riResName.push_back(landCls[i].asset->riResName[j]);
        if (landCls[i].asset->riPaletteRotation[j] && !landCls[i].asset->riPosInst[j])
          logerr("Currently only pos_inst instances support palette rotations <%s>", pregenEnt[i].riName);
      }
      else
      {
        int idx = rtData->riResMapStor[base + j];
        if (rtData->riColPair[idx * 2 + 0] != landCls[i].asset->riColPair[j * 2 + 0] ||
            rtData->riColPair[idx * 2 + 1] != landCls[i].asset->riColPair[j * 2 + 1])
        {
          logwarn("RIRES <%s> in landClass <%s> has changed colors! %08X-%08X -> %08X-%08X", landCls[i].asset->riResName[j],
            landCls[i].landClassName.get(), rtData->riColPair[idx * 2].u, rtData->riColPair[idx * 2 + 1].u,
            landCls[i].asset->riColPair[j * 2].u, landCls[i].asset->riColPair[j * 2 + 1].u);
          rtData->riColPair[idx * 2 + 0] = landCls[i].asset->riColPair[j * 2 + 0];
          rtData->riColPair[idx * 2 + 1] = landCls[i].asset->riColPair[j * 2 + 1];
        }
      }
    }
  }
  rtData->riResMapStor.shrink_to_fit();
  rtData->riRes.shrink_to_fit();
  rtData->riCollRes.shrink_to_fit();
  rtData->riResName.shrink_to_fit();
  rtData->riColPair.shrink_to_fit();

  int16_t *p = rtData->riResMapStor.data();
  for (int i = 0; i < landCls.size(); p += landCls[i].asset->riRes.size(), i++)
    landCls[i].riResMap = p;

  rendinst::gen::RotationPaletteManager::RendintsInfo riInfo;
  riInfo.landClassCount = landCls.size();
  riInfo.landClasses = landCls.begin();
  riInfo.rtData = rtData;
  riInfo.pregenEntCount = pregenEnt.size();
  riInfo.pregenEnt = pregenEnt.data();
  rotationPaletteManager->createPalette(riInfo);

  if (RendInstGenData::renderResRequired)
  {
    bool consistentBinary = true;
    for (int i = 0; i < rtData->riPaletteRotation.size(); ++i)
    {
      if (!rtData->riRes[i] || !rtData->riRes[i]->hasImpostor())
      {
        if (rtData->riPaletteRotation[i])
        {
          debug("Rotation palettes: inconsistent binary due to asset without impostor having a rotation palette <%s>",
            rtData->riResName[i]);
          consistentBinary = false, rtData->riPaletteRotation[i] = 0;
        }
        continue;
      }
      if (rtData->riPaletteRotation[i] && !rtData->riRes[i]->isBakedImpostor())
      {
        debug("Rotation palettes: inconsistent binary due to runtime impostor having rotation palette <%s>", rtData->riResName[i]);
        consistentBinary = false;
        rtData->riPaletteRotation[i] = 0;
      }
      else if (!rtData->riPaletteRotation[i] && rtData->riRes[i]->isBakedImpostor())
      {
        debug("Rotation palettes: inconsistent binary due to baked impostor not having a rotation palette <%s>", rtData->riResName[i]);
        consistentBinary = false;
        rtData->riPaletteRotation[i] = 1;
      }
      if (rtData->riPaletteRotation[i])
        G_ASSERT(rotationPaletteManager->getPalette({rtData->layerIdx, i}).count != 0);
    }
    if (!consistentBinary)
      logerr(
        "Rotation palettes from the level binary are not consistent with impostor types. Newly added baked impostors will not rotate."
        " Re-export current level binary to resolve this.");
  }

  clear_and_resize(rtData->riResElemMask, rtData->riRes.size() * rendinst::MAX_LOD_COUNT);
  mem_set_0(rtData->riResElemMask);
  for (int i = 0; i < countof(rtData->riResIdxPerStage); i++)
    rtData->riResIdxPerStage[i].clear();
  if (RendInstGenData::renderResRequired)
  {
    static int atest_variable_id = VariableMap::getVariableId("atest");
    static int pn_variable_id = VariableMap::getVariableId("material_pn_triangulation");
    static int use_cross_dissolve_variable_id = VariableMap::getVariableId("use_cross_dissolve");
    ska::flat_hash_set<uint16_t> riResIdxPerStageUniqs[countof(rtData->riResIdxPerStage)];

    for (int i = 0, ie = rtData->riRes.size(); i < ie; i++)
      for (int l = 0; l < rendinst::MAX_LOD_COUNT && l < rtData->riRes[i]->lods.size(); l++)
      {
        ShaderMesh *m = rtData->riResLod(i, l).scene->getMesh()->getMesh()->getMesh();
        if (m->getElems(m->STG_opaque, m->STG_imm_decal).size())
          riResIdxPerStageUniqs[get_layer_index(rendinst::LayerFlag::Opaque)].insert(i);
        if (m->getElems(m->STG_trans).size())
          riResIdxPerStageUniqs[get_layer_index(rendinst::LayerFlag::Transparent)].insert(i);
        if (m->getElems(m->STG_decal).size())
          riResIdxPerStageUniqs[get_layer_index(rendinst::LayerFlag::Decals)].insert(i);
        if (m->getElems(m->STG_distortion).size())
          riResIdxPerStageUniqs[get_layer_index(rendinst::LayerFlag::Distortion)].insert(i);

        dag::Span<ShaderMesh::RElem> elems = m->getElems(m->STG_opaque, m->STG_decal);
        uint32_t mask = 0;
        uint32_t cmask = 0;
        uint32_t plodMask = 0;
        G_STATIC_ASSERT(sizeof(mask) == sizeof(rtData->riResElemMask[0].atest));
        G_STATIC_ASSERT(sizeof(rtData->riResElemMask[0].atest) == sizeof(rtData->riResElemMask[0].cullN));
        G_ASSERTF(elems.size() <= sizeof(mask) * 8, "elems.size()=%d > mask=%d bits for RI[%d]=%s", elems.size(), sizeof(mask) * 8, i,
          rtData->riResName[i]);

        for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++)
        {
          if (elems[elemNo].vertexData)
          {
            int atest = 0;
            int pnMat = 0;
            int crossDissolve = 0;

            if (strstr(elems[elemNo].mat->getInfo().str(), "rendinst_plod"))
              plodMask |= 1 << elemNo;

            if (rtData->riRes[i]->hasImpostor() &&
                (/*impostor lod*/ l == rtData->riRes[i]->lods.size() - 1 ||
                  /*transition lod*/
                  (elems[elemNo].mat->getIntVariable(use_cross_dissolve_variable_id, crossDissolve) && crossDissolve > 0)))
            {
              mask |= 1 << elemNo;
              cmask |= 1 << elemNo;
              continue;
            }
            if (strstr(elems[elemNo].mat->getInfo().str(), "facing_leaves") ||
                strstr(elems[elemNo].mat->getInfo().str(), "rendinst_tree") || // Not atest but must use separate shader to apply the
                                                                               // wind.
                (elems[elemNo].mat->getIntVariable(atest_variable_id, atest) && atest > 0) ||
                (elems[elemNo].mat->getIntVariable(pn_variable_id, pnMat) && pnMat > 0))
              mask |= 1 << elemNo;
            if ((elems[elemNo].mat->get_flags() & SHFLG_2SIDED) && !(elems[elemNo].mat->get_flags() & SHFLG_REAL2SIDED))
              cmask |= 1 << elemNo;
#if DAGOR_DBGLEVEL > 0
            if (rendinst::shader_material_validation_cb)
              rendinst::shader_material_validation_cb(elems[elemNo].mat, rtData->riResName[i]);

            if (!(mask & (1 << elemNo)) && elems[elemNo].e)
            {
              dag::ConstSpan<ShaderChannelId> channels = elems[elemNo].e->getChannels();
              int pos_offset = -1, pos_type = -1;
              int normal_offset = -1, normal_type = -1;
              for (int j = 0, coffset = 0; j < channels.size(); ++j)
              {
                unsigned chSize = 0;
                if (channels[j].u == SCUSAGE_POS && channels[j].ui == 0)
                {
                  pos_offset = coffset;
                  pos_type = channels[j].t;
                }
                if (channels[j].u == SCUSAGE_NORM && channels[j].ui == 0)
                {
                  normal_offset = coffset;
                  normal_type = channels[j].t;
                }
                if (channel_size(channels[j].t, chSize))
                  coffset += chSize;
              }
              if (pos_offset != 0 || pos_type != SCTYPE_FLOAT3)
              {
                G_ASSERTF(pos_type == SCTYPE_FLOAT3, "shader <%s> has invalid type", elems[elemNo].mat->getInfo().str());
                G_ASSERTF(pos_offset == 0, "shader <%s> has position with offset %d and not 0", elems[elemNo].mat->getInfo().str(),
                  pos_offset);
              }
              // normal check is actually NOT needed, as we should be rendering opaque only with position.
              // however, it is not so YET
              // to be updated later
              if (rendinst::render::normal_type == -1)
                rendinst::render::normal_type = normal_type;
              if (normal_offset != 12 || normal_type != rendinst::render::normal_type ||
                  (normal_type != SCTYPE_SHORT4N && normal_type != SCTYPE_E3DCOLOR))
              {
                G_ASSERTF((normal_type == SCTYPE_SHORT4N || normal_type == SCTYPE_E3DCOLOR),
                  "shader <%s> has invalid normal type: should be either short4n or color", elems[elemNo].mat->getInfo().str());
                G_ASSERTF(normal_type == rendinst::render::normal_type, "shader <%s> has different normal type than other RI shaders",
                  elems[elemNo].mat->getInfo().str());
                G_ASSERTF(normal_offset == 12, "shader <%s> has normal with offset %d and not 12", elems[elemNo].mat->getInfo().str(),
                  normal_offset);
              }
            }
#endif
          }
        }
        rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + l].atest = mask;
        rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + l].cullN = cmask;
        rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + l].plod = plodMask;
      }
    rendinst::render::init_depth_VDECL();
    for (uint16_t i = 0; i < eastl::size(riResIdxPerStageUniqs); i++)
      for (uint16_t const &v : riResIdxPerStageUniqs[i])
        rtData->riResIdxPerStage[i].push_back(v);
  }

  G_ASSERT(rtData->riRes.size() < (1 << (sizeof(rtData->riResOrder[0]) * 8)));
  rtData->riResOrder.resize(rtData->riRes.size());
  for (int i = 0; i < rtData->riResOrder.size(); ++i)
    rtData->riResOrder[i] = i;
  if (RendInstGenData::renderResRequired)
    eastl::sort(rtData->riResOrder.data(), rtData->riResOrder.data() + rtData->riResOrder.size(),
      SortByAtest(rtData->riResElemMask, rtData->riResName));


  // for (int i = 0; i < rtData->riResOrder.size(); ++i)
  //   debug("sorted %d: %s", i, rtData->riResName[rtData->riResOrder[i]]);

  clear_and_resize(rtData->riResHideMask, rtData->riRes.size());
  if (RendInstGenData::renderResRequired)
    for (int i = 0, ie = rtData->riResHideMask.size(); i < ie; i++)
      rtData->riResHideMask[i] = rendinst::getResHideMask(rtData->riResName[i], rtData->riRes[i] ? &rtData->riRes[i]->bbox : nullptr);
  else
    mem_set_0(rtData->riResHideMask);

  clear_and_resize(rtData->riResBb, rtData->riRes.size());
  clear_and_resize(rtData->riCollResBb, rtData->riRes.size());
  for (int i = 0, ie = rtData->riResBb.size(); i < ie; i++)
  {
    if (rtData->riCollRes[i].collRes)
      rtData->riCollResBb[i] = rtData->riCollRes[i].collRes->vFullBBox;
    else
      rtData->riCollResBb[i].bmin = rtData->riCollResBb[i].bmax = v_zero();

    if (RendInstGenData::renderResRequired && rtData->riRes[i])
    {
      as_point4(&rtData->riResBb[i].bmin).set_xyz1(rtData->riRes[i]->bbox[0]);
      as_point4(&rtData->riResBb[i].bmax).set_xyz1(rtData->riRes[i]->bbox[1]);
    }
    else if (rtData->riCollRes[i].collRes)
      rtData->riResBb[i] = rtData->riCollResBb[i];
    else if (rtData->riResName[i])
    {
      const DataBlock *b = gameres_rendinst_desc.getBlockByName(rtData->riResName[i]);
      if (!b)
        goto missing;
      as_point4(&rtData->riResBb[i].bmin).set_xyz1(b->getPoint3("bbox0", Point3(-1, -1, -1)));
      as_point4(&rtData->riResBb[i].bmax).set_xyz1(b->getPoint3("bbox1", Point3(1, 1, 1)));
    }
    else
    {
    missing:
      as_point4(&rtData->riResBb[i].bmin).set(-0.1, -0.1, -0.1, 1);
      as_point4(&rtData->riResBb[i].bmax).set(0.1, 0.1, 0.1, 1);
    }
    if (!rtData->riCollRes[i].collRes && RendInstGenData::renderResRequired && rtData->riRes[i])
      rtData->riCollResBb[i] = rtData->riResBb[i];

#if RI_VERBOSE_OUTPUT
    G_STATIC_ASSERT(rendinst::MAX_LOD_COUNT >= 5); // only for the debug message
    BBox3 riResBb, riCollResBb;
    v_stu_bbox3(riResBb, rtData->riResBb[i]);
    v_stu_bbox3(riCollResBb, rtData->riCollResBb[i]);
    debug("ri[%d] <%s> =%p (%d) %08X-%08X (%@ %@) (%@ %@) am=(%04X %04X %04X %04X %04X) cm=(%04X %04X %04X %04X %04X) hm=%02X", i,
      rtData->riResName[i], rtData->riRes[i], (bool)rtData->riPosInst[i], rtData->riColPair[i * 2 + 0].u,
      rtData->riColPair[i * 2 + 1].u, riResBb.lim[0], riResBb.lim[1], riCollResBb.lim[0], riCollResBb.lim[1],
      rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 0].atest, rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 1].atest,
      rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 2].atest, rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 3].atest,
      rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 4].atest, rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 0].cullN,
      rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 1].cullN, rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 2].cullN,
      rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 3].cullN, rtData->riResElemMask[i * rendinst::MAX_LOD_COUNT + 4].cullN,
      rtData->riResHideMask[i]);
#endif
  }
  rtData->lastPoi = Point2(-1000000, 1000000);
}
void RendInstGenData::addRIGenExtraSubst(const char *ri_res_name)
{
  G_ASSERT(rtData);
  for (int i = 0; i < rtData->riResName.size(); i++)
    if (rtData->riResName[i] && strcmp(rtData->riResName[i], ri_res_name) == 0)
    {
      for (int j = 0; j < rtData->riExtraIdxPair.size(); j += 2)
        if (rtData->riExtraIdxPair[j] == i)
          return;
      rtData->riExtraIdxPair.push_back(i);
      rtData->riExtraIdxPair.push_back(
        rendinst::addRIGenExtraResIdx(ri_res_name, i, rtData->layerIdx, rendinst::AddRIFlag::UseShadow));
      // debug("RI-subst: added %s, riExtraIdxPair=%d", ri_res_name, rtData->riExtraIdxPair.size());
      return;
    }
}

bool RendInstGenData::Cell::hasFarVisiblePregenInstances(const RendInstGenData *rgl, float thres_dist,
  dag::Span<float> riMaxDist) const
{
  for (const LandClassCoverage &c : cls)
    for (int riResIdx : make_span(rgl->landCls[c.landClsIdx].riResMap.get(), rgl->landCls[c.landClsIdx].asset->riRes.size()))
      if (riResIdx < riMaxDist.size() && riMaxDist[riResIdx] > thres_dist)
        return true;

  for (int s = 0; s + 1 < entCnt.size(); s++)
    for (const PregenEntCounter *p = entCnt[s], *pe = entCnt[s + 1]; p < pe; p++)
      if (p->riResIdx < riMaxDist.size() && riMaxDist[p->riResIdx] > thres_dist)
        return true;
  return false;
}

// NOTE: This code is way too complicated for static thread safety analysis to understand it.
int RendInstGenData::precomputeCell(RendInstGenData::CellRtData &crt, int x, int z) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  int cellIdx = x + z * cellNumW;
  RendInstGenData::Cell &cell = cells[cellIdx];
  Tab<rendinst::gen::SingleEntityPool> pools(tmpmem);
  float world2grid = 1.0 / grid2world;
  float box_sz = grid2world * cellSz / SUBCELL_DIV;

  clear_and_resize(crt.pools, rtData->riRes.size());
  mem_set_0(crt.pools);
  dag::ConstSpan<uint16_t> riExtraIdxPair = rtData->riExtraIdxPair;

  for (int i = 0; i < riExtraIdxPair.size(); i += 2)
  {
    crt.pools[riExtraIdxPair[i]].avail = -riExtraIdxPair[i + 1] - 1;
    crt.pools[riExtraIdxPair[i]].baseOfs = riExtraIdxPair[i];
  }
  rendinst::gen::SingleEntityPool::ri_col_pair.set(rtData->riColPair);

  float landMin = cell.htMin, landMax = cell.htMin + cell.htDelta;
  rendinst::gen::custom_get_land_min_max(BBox2(world0x() + x * grid2world * cellSz, world0z() + z * grid2world * cellSz,
                                           world0x() + (x + 1) * grid2world * cellSz, world0z() + (z + 1) * grid2world * cellSz),
    landMin, landMax);
  inplace_max(landMax, cell.htMin + cell.htDelta); //< or we have big troubles when placing RI on prefabs high above land!

  as_point4(&crt.cellOrigin).set(world0x() + x * grid2world * cellSz, landMin, world0z() + z * grid2world * cellSz, 1);
  crt.cellHeight = landMax - landMin;

  Tab<const TMatrix *> sbm_cell(tmpmem), sbm_subcell(tmpmem);
  gather_sweep_boxes(sbm_cell, world0x() + x * grid2world * cellSz, world0z() + z * grid2world * cellSz, grid2world * cellSz,
    grid2world * cellSz);

  if (RendInstGenData::riGenPrepareAddPregenCB)
    RendInstGenData::riGenPrepareAddPregenCB(crt, rtData->layerIdx, perInstDataDwords, world0x() + x * cellSz * grid2world, cell.htMin,
      world0z() + z * cellSz * grid2world, grid2world * cellSz, cell.htDelta ? cell.htDelta : 8192);

  LFileGeneralLoadCB fcrd(fpLevelBin);
  IGenLoad *zcrd = nullptr;
  SmallTab<uint8_t, TmpmemAlloc> pregenUnpData;

  vec3f v_cell_add = v_zero();
  vec3f v_cell_mul = v_zero();

  rendinst::gen::SingleEntityPool::ox = world0x() + x * SUBCELL_DIV * box_sz;
  rendinst::gen::SingleEntityPool::oy = v_extract_y(crt.cellOrigin);
  rendinst::gen::SingleEntityPool::oz = world0z() + z * SUBCELL_DIV * box_sz;
  rendinst::gen::SingleEntityPool::cell_xz_sz = SUBCELL_DIV * box_sz;
  rendinst::gen::SingleEntityPool::cell_y_sz = crt.cellHeight;
  rendinst::gen::SingleEntityPool::per_pool_local_bb = rtData->riResBb.data();
  rendinst::gen::SingleEntityPool::cur_cell_id = cellIdx;
  rendinst::gen::SingleEntityPool::cur_ri_extra_ord = 0;
  rendinst::gen::SingleEntityPool::per_inst_data_dwords = perInstDataDwords;

  if (cell.riDataRelOfs >= 0)
  {
    bool need_unpack_pregen = false;
    for (int s = 0; s < SUBCELL_DIV * SUBCELL_DIV; s++)
    {
      for (PregenEntCounter *p = cell.entCnt[s], *pe = cell.entCnt[s + 1]; p < pe; p++)
        if (crt.pools[p->riResIdx].avail < 0)
        {
          need_unpack_pregen = true;
          break;
        }
      if (need_unpack_pregen)
        break;
    }
    if (need_unpack_pregen)
    {
      fcrd.seekto(pregenDataBaseOfs + cell.riDataRelOfs);
      unsigned fmt = 0;
      if (!fcrd.beginBlock(&fmt))
        fcrd.endBlock();
      else if (fmt == btag_compr::ZSTD)
        zcrd = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(fcrd, fcrd.getBlockRest());
      else
        zcrd = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(fcrd, fcrd.getBlockRest());

      v_cell_add = v_make_vec4f(v_extract_x(crt.cellOrigin), cell.htMin, v_extract_z(crt.cellOrigin), 0);
      v_cell_mul =
        v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(rendinst::gen::SingleEntityPool::cell_xz_sz,
                                             cell.htDelta ? cell.htDelta : 8192, rendinst::gen::SingleEntityPool::cell_xz_sz, 0));
    }
  }

  rtData->riRwCs.lockRead();
  int cellExtraDestrIdx = -1; // intentionally not ptr, since riDestrCellData might be reallocated within add_destroyed_data in other
                              // thread
  for (int i = 0; i < rtData->riDestrCellData.size(); ++i)
  {
    if (rtData->riDestrCellData[i].cellId != -(cellIdx + 1))
      continue;
    cellExtraDestrIdx = i;
    break;
  }
  if (cellExtraDestrIdx < 0)
    rtData->riRwCs.unlockRead();

  // null-generate to determine capacity requirements
  for (int s = 0; s < SUBCELL_DIV * SUBCELL_DIV; s++)
  {
    float x0 = (x * SUBCELL_DIV + (s % SUBCELL_DIV)) * box_sz;
    float z0 = (z * SUBCELL_DIV + (s / SUBCELL_DIV)) * box_sz;
    gather_and_set_sweep_boxes(sbm_subcell, x0 + world0x(), z0 + world0z(), box_sz, box_sz, sbm_cell);

    if (zcrd)
      for (PregenEntCounter *p = cell.entCnt[s], *pe = cell.entCnt[s + 1]; p < pe; p++)
      {
        EntPool &pool = crt.pools[p->riResIdx];
        bool pos_inst = rtData->riPosInst[p->riResIdx];
        bool palette_rotation = rtData->riPaletteRotation[p->riResIdx];
        bool zeroInstSeeds = rtData->riZeroInstSeeds[p->riResIdx];
        int cnt = p->riCount;
        int stride = (pos_inst ? 2 * 4 : (rendinst::tmInst12x32bit ? 4 * 12 : 2 * 12)) + (zeroInstSeeds ? 0 : 4 * perInstDataDwords);

        if (pool.avail >= 0)
        {
          crt.pools[p->riResIdx].total += cnt;
          zcrd->seekrel(cnt * stride);
          continue;
        }

        if (pregenUnpData.size() < cnt * stride)
          clear_and_resize(pregenUnpData, cnt * stride);
        zcrd->read(pregenUnpData.data(), cnt * stride);

        int ri_idx = -pool.avail - 1;
        const rendinst::DestroyedPoolData *poolExtraData = nullptr;
        if (cellExtraDestrIdx >= 0)
          poolExtraData = rtData->riDestrCellData[cellExtraDestrIdx].getPool(ri_idx);
        int x_ofs = pos_inst ? 2 * 0 : (rendinst::tmInst12x32bit ? 4 * 3 + 2 : 2 * 3);
        int y_ofs = pos_inst ? 2 * 1 : (rendinst::tmInst12x32bit ? 4 * 7 + 2 : 2 * 7);
        int z_ofs = pos_inst ? 2 * 2 : (rendinst::tmInst12x32bit ? 4 * 11 + 2 : 2 * 11);
        int curDestrRange = 0;
        int originalRiPool = rendinst::riExtra[ri_idx].riPoolRef;
        bool canBeExcluded = useDestrExclForPregenAdd && rtData->riDestr.size() && originalRiPool >= 0 && // debris inited
                             rendinst::getRgLayer(rendinst::riExtra[ri_idx].riPoolRefLayer) == this &&
                             !rtData->riProperties[originalRiPool].immortal && !rtData->riDestr[originalRiPool].destructable;
        for (uint8_t *ptr = pregenUnpData.data(), *ptr_e = ptr + stride * cnt; ptr < ptr_e; ptr += stride)
        {
          float px =
            *(int16_t *)(ptr + x_ofs) * rendinst::gen::SingleEntityPool::cell_xz_sz / 32767.0f + rendinst::gen::SingleEntityPool::ox;
          float pz =
            *(int16_t *)(ptr + z_ofs) * rendinst::gen::SingleEntityPool::cell_xz_sz / 32767.0f + rendinst::gen::SingleEntityPool::oz;
          bool isInDestr = false;
          if (poolExtraData)
          {
            for (; curDestrRange < poolExtraData->destroyedInstances.size(); ++curDestrRange)
            {
              if (poolExtraData->destroyedInstances[curDestrRange].endOffs > rendinst::gen::SingleEntityPool::cur_ri_extra_ord)
              {
                isInDestr |=
                  rendinst::gen::SingleEntityPool::cur_ri_extra_ord >= poolExtraData->destroyedInstances[curDestrRange].startOffs;
                break;
              }
            }
          }
          rendinst::RiExtraPool &riPool = rendinst::riExtra[ri_idx];
          if ((canBeExcluded && rendinst::gen::destrExcl.isMarked(px, pz)) ||
              rendinst::gen::SingleEntityPool::intersectWithSweepBoxes(px, pz) || (isInDestr && riPool.destrDepth == 0))
          {
            if (pos_inst)
              *(int16_t *)(ptr + 2 * 3) = 0;
            else if (!rendinst::tmInst12x32bit)
              *(int16_t *)(ptr + 2 * 0) = *(int16_t *)(ptr + 2 * 1) = *(int16_t *)(ptr + 2 * 2) = *(int16_t *)(ptr + 2 * 4) =
                *(int16_t *)(ptr + 2 * 5) = *(int16_t *)(ptr + 2 * 6) = *(int16_t *)(ptr + 2 * 8) = *(int16_t *)(ptr + 2 * 9) =
                  *(int16_t *)(ptr + 2 * 10) = 0;
            else
              *(int *)(ptr + 4 * 0) = *(int *)(ptr + 4 * 1) = *(int *)(ptr + 4 * 2) = *(int *)(ptr + 4 * 4) = *(int *)(ptr + 4 * 5) =
                *(int *)(ptr + 4 * 6) = *(int *)(ptr + 4 * 8) = *(int *)(ptr + 4 * 9) = *(int *)(ptr + 4 * 10) = 0;
            rendinst::gen::SingleEntityPool::cur_ri_extra_ord += 16 * (riPool.destrDepth + 1);
            continue;
          }
          mat44f tm;
          if (pos_inst)
            rendinst::gen::unpack_tm_pos(tm, (int16_t *)ptr, v_cell_add, v_cell_mul, palette_rotation);
          else if (!rendinst::tmInst12x32bit)
            rendinst::gen::validate_unpack_tm_full(tm, (int16_t *)ptr, v_cell_add, v_cell_mul);
          else
            rendinst::gen::unpack_tm32_full(tm, (int32_t *)ptr, v_cell_add, v_cell_mul);
          tm.col3 = rendinst::gen::custom_update_pregen_pos_y(tm.col3, (int16_t *)(ptr + y_ofs),
            rendinst::gen::SingleEntityPool::cell_y_sz, rendinst::gen::SingleEntityPool::oy);

          v_bbox3_add_transformed_box(crt.pregenRiExtraBbox, tm, riPool.lbb);

          int curRiPoolIdx = ri_idx;
          if (isInDestr)
          {
            rendinst::gen::SingleEntityPool::cur_ri_extra_ord += 16;
            for (curRiPoolIdx = riPool.destroyedRiIdx; curRiPoolIdx >= 0;
                 curRiPoolIdx = rendinst::riExtra[curRiPoolIdx].destroyedRiIdx)
            {
              poolExtraData = rtData->riDestrCellData[cellExtraDestrIdx].getPool(curRiPoolIdx);
              if (!poolExtraData || !poolExtraData->isInRange(rendinst::gen::SingleEntityPool::cur_ri_extra_ord))
                break;
              rendinst::gen::SingleEntityPool::cur_ri_extra_ord += 16;
            }
          }
          if (curRiPoolIdx >= 0)
          {
            const int32_t *per_inst_data =
              (perInstDataDwords && !zeroInstSeeds) ? (int *)(ptr + stride - 4 * perInstDataDwords) : nullptr;
            // debug("add riEx(%d : %d) seed=%08X  %.3f, %.3f", p->riResIdx, curRiPoolIdx, *per_inst_data, px, pz);

            // addRIGenExtra44 does write lock of ccExtra within, do unlock layer's rw lock to avoid
            // relying on lock order of AutoLockReadPrimaryAndExtra (fragile) and also code that use it (e.g. traces) to continue
            if (cellExtraDestrIdx >= 0)
              rtData->riRwCs.unlockRead();
            rendinst::addRIGenExtra44(curRiPoolIdx, false, tm, true, cellIdx, rendinst::gen::SingleEntityPool::cur_ri_extra_ord,
              zeroInstSeeds ? 0 : perInstDataDwords, per_inst_data, true /*on loading*/);
            if (cellExtraDestrIdx >= 0)
              rtData->riRwCs.lockRead();
            rendinst::gen::SingleEntityPool::cur_ri_extra_ord += 16 * (rendinst::riExtra[curRiPoolIdx].destrDepth + 1);
          }
        }
      }
    else if (cell.riDataRelOfs >= 0)
      for (PregenEntCounter *p = cell.entCnt[s], *pe = cell.entCnt[s + 1]; p < pe; p++)
        if (crt.pools[p->riResIdx].avail >= 0)
          crt.pools[p->riResIdx].total += p->riCount;

    if (crt.pregenAdd)
      for (PregenEntCounter *p = crt.pregenAdd->cntStor.data() + crt.pregenAdd->scCntIdx[s],
                            *pe = crt.pregenAdd->cntStor.data() + crt.pregenAdd->scCntIdx[s + 1];
           p < pe; p++)
        if (crt.pools[p->riResIdx].avail >= 0)
          crt.pools[p->riResIdx].total += p->riCount;

    for (int i = 0; i < cell.cls.size(); i++)
    {
      int cls_idx = cell.cls[i].landClsIdx;
      rendinst::gen::land::AssetData &land = *landCls[cls_idx].asset;

#if _TARGET_PC_TOOLS_BUILD
      int bx0 = x * cellSz + (s % SUBCELL_DIV) * cellSz / SUBCELL_DIV;
      int bz = z * cellSz + (s / SUBCELL_DIV) * cellSz / SUBCELL_DIV;
      int bsz = (cellSz + SUBCELL_DIV - 1) / SUBCELL_DIV;
      for (int bz1 = bz + bsz; bz < bz1; bz++)
        for (int bx = bx0, bx1 = bx0 + bsz; bx < bx1; bx++)
          if (landCls[cls_idx].mask->get(bx, bz))
            goto generate;
      continue;
    generate:
#endif

      pools.resize(land.riRes.size());
      mem_set_0(pools);
      if (riExtraIdxPair.size())
        for (int j = 0; j < pools.size(); j++)
        {
          int k = landCls[cls_idx].riResMap[j];
          if (crt.pools[k].avail < 0)
            pools[j].avail = crt.pools[k].avail, pools[j].shortage = crt.pools[k].baseOfs;
        }

      if (land.tiled)
        rendinst::gen::generateTiledEntitiesInMaskedRect(*land.tiled, make_span(pools), *landCls[cls_idx].mask, world2grid, x0, z0,
          box_sz, world0x(), world0z(), landCls[cls_idx].riResMap, densMapPivotX, densMapPivotZ);

      if (land.planted)
        rendinst::gen::generatePlantedEntitiesInMaskedRect(*land.planted, land.layerIdx, make_span(pools), *landCls[cls_idx].mask,
          world2grid, x0, z0, box_sz, world0x(), world0z(), 0, landCls[cls_idx].riResMap, densMapPivotX, densMapPivotZ);

      for (int j = 0; j < pools.size(); j++)
        if (pools[j].avail >= 0)
          crt.pools[landCls[cls_idx].riResMap[j]].total += pools[j].shortage;
    }
  }

  if (cellExtraDestrIdx >= 0)
    rtData->riRwCs.unlockRead();

  rendinst::gen::SingleEntityPool::sweep_boxes_itm.reset();

  if (zcrd)
  {
    zcrd->ceaseReading();
    zcrd->~IGenLoad();
    fcrd.endBlock();
  }

  // compute VB size
  crt.vbSize = 0;
  for (int i = 0; i < crt.pools.size(); i++)
    crt.vbSize += RIGEN_STRIDE_B(rtData->riPosInst[i], rtData->riZeroInstSeeds[i], perInstDataDwords) * crt.pools[i].total;

  // arrange base offsets
  int ri_count = 0, ofs = 0;
  for (int i = 0; i < crt.pools.size();
       ofs += RIGEN_STRIDE_B(rtData->riPosInst[i], rtData->riZeroInstSeeds[i], perInstDataDwords) * crt.pools[i].total, i++)
  {
    crt.pools[i].baseOfs = ofs, ri_count += crt.pools[i].total;
  }
  G_ASSERT(ofs <= crt.vbSize);
  if (rendinst::rgAttr[rtData->layerIdx].needNetSync)
  {
#if _TARGET_PC_TOOLS_BUILD
    if (rendinst::forceRiExtra && crt.vbSize > rendinst::maxRiGenPerCell * 8)
    {
      int ri_gen_per_cell = 0, ri_ex_per_cell = 0;
      for (int i = 0; i < crt.pools.size(); i++)
        if (rtData->riPosInst[i])
          ri_gen_per_cell += crt.pools[i].total;
        else
          ri_ex_per_cell += crt.pools[i].total;

      G_ASSERTF(ri_gen_per_cell <= rendinst::maxRiGenPerCell,
        "crt.vbSize=%d, ri_gen_per_cell=%d > %d, ri_ex_per_cell=%d; cell[%d,%d] at (%.1f,%.1f) sz=%.1f", crt.vbSize, ri_gen_per_cell,
        rendinst::maxRiGenPerCell, ri_ex_per_cell, x, z, rendinst::gen::SingleEntityPool::ox, rendinst::gen::SingleEntityPool::oz,
        rendinst::gen::SingleEntityPool::cell_xz_sz);
      G_ASSERTF(rendinst::gen::SingleEntityPool::cur_ri_extra_ord / 16 + ri_ex_per_cell <= rendinst::maxRiExPerCell,
        "riExCnt=%d+%d > %d (ri_count=%d); cell[%d,%d] at (%.1f,%.1f) sz=%.1f", rendinst::gen::SingleEntityPool::cur_ri_extra_ord / 16,
        ri_ex_per_cell, rendinst::maxRiExPerCell, ri_count, x, z, rendinst::gen::SingleEntityPool::ox,
        rendinst::gen::SingleEntityPool::oz, rendinst::gen::SingleEntityPool::cell_xz_sz);
      G_UNUSED(ri_gen_per_cell);
      G_UNUSED(ri_ex_per_cell);
    }
    else
    {
#endif
#if DAGOR_DBGLEVEL > 0
      if (crt.vbSize > rendinst::maxRiGenPerCell * 8)
      {
        debug("cell[%d,%d], %d RI:", x, z, crt.pools.size());
        for (int i = 0; i < crt.pools.size(); i++)
        {
          BBox3 riResBb;
          v_stu_bbox3(riResBb, rtData->riResBb[i]);
          debug(" [%2d] %4d - %s  [%s] sz=%@%s", i, crt.pools[i].total, rtData->riResName[i], rtData->riPosInst[i] ? "pos" : "tm",
            i < rtData->riResBb.size() ? riResBb.lim[1] - riResBb.lim[0] : Point3(0, 0, 0),
            (i < rtData->riCollRes.size() && rtData->riCollRes[i].collRes) ? "" : " non-collidable");
        }
      }
#endif
      G_ASSERTF(crt.vbSize <= rendinst::maxRiGenPerCell * 8, "crt.vbSize=%d > %d (ri_count=%d); cell[%d,%d] at (%.1f,%.1f) sz=%.1f",
        crt.vbSize, rendinst::maxRiGenPerCell * 8, ri_count, x, z, rendinst::gen::SingleEntityPool::ox,
        rendinst::gen::SingleEntityPool::oz, rendinst::gen::SingleEntityPool::cell_xz_sz);
      G_ASSERTF(rendinst::gen::SingleEntityPool::cur_ri_extra_ord / 16 <= rendinst::maxRiExPerCell,
        "riExCnt=%d > %d (ri_count=%d); cell[%d,%d] at (%.1f,%.1f) sz=%.1f", rendinst::gen::SingleEntityPool::cur_ri_extra_ord / 16,
        rendinst::maxRiExPerCell, ri_count, x, z, rendinst::gen::SingleEntityPool::ox, rendinst::gen::SingleEntityPool::oz,
        rendinst::gen::SingleEntityPool::cell_xz_sz);
#if _TARGET_PC_TOOLS_BUILD
    }
#endif
  }

  if (!ri_count)
  {
    G_ASSERT(!crt.vbSize);
    G_ASSERT(!crt.sysMemData);
    clear_and_shrink(crt.pools);
  }
  // debug("cell %4d,%4d: alloc %7d pools=%d (cnt=%d)", x, z, crt.vbSize, crt.pools.size(), ri_count);
  return ri_count;
}

static inline void copy_interlaced_buf(int cnt, uint8_t *dest_top, int dest_stride, const uint8_t *sptr, int src_step,
  int src_add_stride, int dest_add_stride = 0)
{
  uint8_t *dptr = dest_top - cnt * dest_stride;
  while (dptr < dest_top)
  {
    for (uint8_t *dptr_end = dptr + dest_stride - dest_add_stride; dptr < dptr_end; dptr += 2, sptr += src_step)
      *(int16_t *)dptr = *(int16_t *)sptr;
    sptr += src_add_stride;
    dptr += dest_add_stride;
  }
}

RendInstGenData::CellRtData *RendInstGenData::generateCell(int x, int z)
{
  TIME_PROFILE_DEV(generateCell);
  int cellId = x + z * cellNumW;
  RendInstGenData::Cell &cell = cells[cellId];
  CellRtData *crt_ptr = cell.rtData;
  if (!crt_ptr)
  {
    crt_ptr = new CellRtData(rtData->riRes.size(), rtData);
    precomputeCell(*crt_ptr, x, z);
  }
  else
    interlocked_release_store_ptr(cell.rtData, (RendInstGenData::CellRtData *)nullptr); // mark as not ready

  RendInstGenData::CellRtData &crt = *crt_ptr;
  if (crt.vbSize <= 0)
    return crt_ptr;

  uint8_t *vbPtr = crt.sysMemData.get();
  Finally setSysMemData([&] {
    crt.sysMemData.release(); // This makes compiler omit free code (b/c we know that it's null)
    crt.sysMemData.reset(vbPtr);
  }); // Delayed as it's influences cell "readyness"
  if (!vbPtr)
  {
    // allocate sysmem data copy and storage for SubCell slices
    vbPtr = new uint8_t[crt.vbSize];
    G_ASSERT(crt.pools.size() <= (1ull << (sizeof(decltype(crt.scsRemap)::value_type) * CHAR_BIT)));
    clear_and_resize(crt.scsRemap, crt.pools.size());
    clear_and_resize(crt.bbox, 1 + SUBCELL_DIV * SUBCELL_DIV);
  }
  else
    setSysMemData.release();

  for (int i = 0; i < crt.pools.size(); i++)
  {
    crt.pools[i].avail = crt.pools[i].total;
    crt.pools[i].topOfs = crt.pools[i].baseOfs;
  }
  dag::ConstSpan<uint16_t> riExtraIdxPair = rtData->riExtraIdxPair;
  for (int i = 0; i < riExtraIdxPair.size(); i += 2)
    crt.pools[riExtraIdxPair[i]].avail = 0;

  Tab<rendinst::gen::SingleEntityPool> pools(tmpmem);
  float world2grid = 1.0 / grid2world;
  float box_sz = grid2world * cellSz / SUBCELL_DIV;
  int p_cnt = crt.pools.size();

  rendinst::gen::SingleEntityPool::ox = world0x() + x * SUBCELL_DIV * box_sz;
  rendinst::gen::SingleEntityPool::oy = v_extract_y(crt.cellOrigin);
  rendinst::gen::SingleEntityPool::oz = world0z() + z * SUBCELL_DIV * box_sz;
  rendinst::gen::SingleEntityPool::cell_xz_sz = SUBCELL_DIV * box_sz;
  rendinst::gen::SingleEntityPool::cell_y_sz = crt.cellHeight;
  rendinst::gen::SingleEntityPool::per_pool_local_bb = rtData->riResBb.data();
  rendinst::gen::SingleEntityPool::per_inst_data_dwords = perInstDataDwords;
  v_bbox3_init_empty(rendinst::gen::SingleEntityPool::bbox);

  if (RendInstGenData::riGenPrepareAddPregenCB && crt.pregenAdd && crt.pregenAdd->needsUpdate)
    RendInstGenData::riGenPrepareAddPregenCB(crt, rtData->layerIdx, perInstDataDwords, rendinst::gen::SingleEntityPool::ox,
      rendinst::gen::SingleEntityPool::oy, rendinst::gen::SingleEntityPool::oz, rendinst::gen::SingleEntityPool::cell_xz_sz,
      rendinst::gen::SingleEntityPool::cell_y_sz);

  LFileGeneralLoadCB fcrd(fpLevelBin);
  IGenLoad *zcrd = nullptr;

  if (cell.riDataRelOfs >= 0)
  {
    fcrd.seekto(pregenDataBaseOfs + cell.riDataRelOfs);
    unsigned fmt = 0;
    if (!fcrd.beginBlock(&fmt))
      fcrd.endBlock();
    else if (fmt == btag_compr::ZSTD)
      zcrd = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(fcrd, fcrd.getBlockRest());
    else
      zcrd = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(fcrd, fcrd.getBlockRest());
  }

  Tab<CellRtData::SubCellSlice> scsLocal(tmpmem);
  scsLocal.resize(SUBCELL_DIV * SUBCELL_DIV * crt.pools.size());
  mem_set_0(scsLocal);
  mem_set_0(crt.scsRemap);
  crt.bbox[0] = rendinst::gen::SingleEntityPool::bbox;
  vec3f v_cell_add = v_make_vec4f(v_extract_x(crt.cellOrigin), cell.htMin, v_extract_z(crt.cellOrigin), 0);
  vec3f v_cell_mul =
    v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(rendinst::gen::SingleEntityPool::cell_xz_sz, cell.htDelta ? cell.htDelta : 8192,
                                         rendinst::gen::SingleEntityPool::cell_xz_sz, 0));

  Tab<const TMatrix *> sbm_cell(tmpmem), sbm_subcell(tmpmem);
  gather_sweep_boxes(sbm_cell, rendinst::gen::SingleEntityPool::ox, rendinst::gen::SingleEntityPool::oz,
    rendinst::gen::SingleEntityPool::cell_xz_sz, rendinst::gen::SingleEntityPool::cell_xz_sz);

  char *pregenData = crt.pregenAdd ? (char *)crt.pregenAdd->dataStor.data() : nullptr;

  // debug("cell[%d,%d] origin=(%.3f,%.3f,%.3f) + sz=(%.3f,%.3f,%.3f)", x, z, P3D(as_point4(&crt.cellOrigin)),
  //   rendinst::gen::SingleEntityPool::cell_xz_sz, rendinst::gen::SingleEntityPool::cell_y_sz,
  //   rendinst::gen::SingleEntityPool::cell_xz_sz);
  Tab<uint8_t> buf_tm32;
  Tab<uint8_t> buf_w16;
  for (int s = 0; s < SUBCELL_DIV * SUBCELL_DIV; s++)
  {
    float x0 = (x * SUBCELL_DIV + (s % SUBCELL_DIV)) * box_sz;
    float z0 = (z * SUBCELL_DIV + (s / SUBCELL_DIV)) * box_sz;
    gather_and_set_sweep_boxes(sbm_subcell, x0 + world0x(), z0 + world0z(), box_sz, box_sz, sbm_cell);

    v_bbox3_init_empty(rendinst::gen::SingleEntityPool::bbox);
    if (zcrd)
      for (PregenEntCounter *p = cell.entCnt[s], *pe = cell.entCnt[s + 1]; p < pe; p++)
      {
        EntPool &pool = crt.pools[p->riResIdx];
        bool pos_inst = rtData->riPosInst[p->riResIdx];
        bool palette_rotation = rtData->riPaletteRotation[p->riResIdx];
        bool zeroInstSeeds = rtData->riZeroInstSeeds[p->riResIdx];
        int cnt = p->riCount;
        int stride = (pos_inst ? 2 * 4 : 2 * 12) + (zeroInstSeeds ? 0 : 4 * perInstDataDwords);
        uint8_t *ptr = pool.topPtr(vbPtr);

        scsLocal[crt.getCellIndex(p->riResIdx, s)].ofs = pool.topOfs + 1;

        int dest_stride = stride - (zeroInstSeeds ? 0 : 4 * perInstDataDwords);
        buf_tm32.clear();
        buf_w16.clear();
        if (!pos_inst && rendinst::tmInst12x32bit)
        {
          stride += 4 * 12 - 2 * 12;
          buf_tm32.resize(stride * cnt);
          ptr = buf_tm32.data();
        }
        else if (perInstDataDwords && !zeroInstSeeds)
        {
          buf_w16.resize(stride * cnt);
          ptr = buf_w16.data();
        }
        if (pool.avail > cnt)
        {
          zcrd->read(ptr, cnt * stride);
          pool.topOfs += cnt * dest_stride;
          pool.avail -= cnt;
        }
        else if (pool.avail)
        {
          zcrd->read(ptr, pool.avail * stride);
          zcrd->seekrel((cnt - pool.avail) * stride);
          pool.topOfs += pool.avail * dest_stride;
          cnt = pool.avail;
          pool.avail = 0;
        }
        else
        {
          zcrd->seekrel(cnt * stride);
          cnt = 0;
        }

        if (!cnt)
          continue;
        bbox3f lbb = rtData->riResBb[p->riResIdx];
        bool canBeExcluded = useDestrExclForPregenAdd && rtData->riDestr.size() && // debris inited
                             !rtData->riProperties[p->riResIdx].immortal && !rtData->riDestr[p->riResIdx].destructable;
        int x_ofs = pos_inst ? 2 * 0 : (rendinst::tmInst12x32bit ? 4 * 3 + 2 : 2 * 3);
        int y_ofs = pos_inst ? 2 * 1 : (rendinst::tmInst12x32bit ? 4 * 7 + 2 : 2 * 7);
        int z_ofs = pos_inst ? 2 * 2 : (rendinst::tmInst12x32bit ? 4 * 11 + 2 : 2 * 11);
        for (uint8_t *ptr_e = ptr + stride * cnt; ptr < ptr_e; ptr += stride)
        {
          float px =
            *(int16_t *)(ptr + x_ofs) * rendinst::gen::SingleEntityPool::cell_xz_sz / 32767.0f + rendinst::gen::SingleEntityPool::ox;
          float pz =
            *(int16_t *)(ptr + z_ofs) * rendinst::gen::SingleEntityPool::cell_xz_sz / 32767.0f + rendinst::gen::SingleEntityPool::oz;
          if ((canBeExcluded && rendinst::gen::destrExcl.isMarked(px, pz)) ||
              rendinst::gen::SingleEntityPool::intersectWithSweepBoxes(px, pz))
          {
            if (pos_inst)
              *(int16_t *)(ptr + 2 * 3) = 0;
            else
              *(int16_t *)(ptr + 2 * 0) = *(int16_t *)(ptr + 2 * 1) = *(int16_t *)(ptr + 2 * 2) = *(int16_t *)(ptr + 2 * 4) =
                *(int16_t *)(ptr + 2 * 5) = *(int16_t *)(ptr + 2 * 6) = *(int16_t *)(ptr + 2 * 8) = *(int16_t *)(ptr + 2 * 9) =
                  *(int16_t *)(ptr + 2 * 10) = 0;
            continue;
          }
          if (pos_inst)
          {
            vec4f pos, scale;
            rendinst::gen::unpack_tm_pos(pos, scale, (int16_t *)ptr, v_cell_add, v_cell_mul, palette_rotation);
            pos = rendinst::gen::custom_update_pregen_pos_y(pos, (int16_t *)(ptr + y_ofs), rendinst::gen::SingleEntityPool::cell_y_sz,
              rendinst::gen::SingleEntityPool::oy);
            *((int16_t *)(ptr + y_ofs)) =
              int16_t(clamp((v_extract_y(pos) - rendinst::gen::SingleEntityPool::oy) / rendinst::gen::SingleEntityPool::cell_y_sz,
                        -1.0f, 1.0f) *
                      32767.0);
            v_bbox3_add_pt(rendinst::gen::SingleEntityPool::bbox, v_madd(lbb.bmin, scale, pos));
            v_bbox3_add_pt(rendinst::gen::SingleEntityPool::bbox, v_madd(lbb.bmax, scale, pos));
          }
          else
          {
            mat44f tm;
            if (!rendinst::tmInst12x32bit)
              rendinst::gen::validate_unpack_tm_full(tm, (int16_t *)ptr, v_cell_add, v_cell_mul);
            else
            {
              rendinst::gen::unpack_tm32_full(tm, (int32_t *)ptr, v_cell_add, v_cell_mul);
              *(int16_t *)(ptr + y_ofs - 2) = 0;
            }
            tm.col3 = rendinst::gen::custom_update_pregen_pos_y(tm.col3, (int16_t *)(ptr + y_ofs),
              rendinst::gen::SingleEntityPool::cell_y_sz, rendinst::gen::SingleEntityPool::oy);
            *((int16_t *)(ptr + y_ofs)) =
              int16_t(clamp((v_extract_y(tm.col3) - rendinst::gen::SingleEntityPool::oy) / rendinst::gen::SingleEntityPool::cell_y_sz,
                        -1.0f, 1.0f) *
                      32767.0);
            v_bbox3_add_transformed_box(rendinst::gen::SingleEntityPool::bbox, tm, lbb);
          }
        }

        if (buf_tm32.size())
          copy_interlaced_buf(cnt, pool.topPtr(vbPtr), dest_stride, buf_tm32.data() + 2, 4, stride - dest_stride * 2);
        else if (buf_w16.size())
          copy_interlaced_buf(cnt, pool.topPtr(vbPtr), dest_stride, buf_w16.data(), 2, stride - dest_stride);
      }

    if (pregenData)
      for (PregenEntCounter *p = crt.pregenAdd->cntStor.data() + crt.pregenAdd->scCntIdx[s],
                            *pe = crt.pregenAdd->cntStor.data() + crt.pregenAdd->scCntIdx[s + 1];
           p < pe; p++)
      {
        G_ASSERTF_CONTINUE(p->riResIdx < crt.pools.size(), "p->riResIdx=%d crt.pools.size()=%d s=%d idx=%d", p->riResIdx,
          crt.pools.size(), s, p - (crt.pregenAdd->cntStor.data() + crt.pregenAdd->scCntIdx[s]));

        EntPool &pool = crt.pools[p->riResIdx];
        bool pos_inst = rtData->riPosInst[p->riResIdx];
        bool palette_rotation = rtData->riPaletteRotation[p->riResIdx];
        bool zeroInstSeeds = rtData->riZeroInstSeeds[p->riResIdx];
        int cnt = p->riCount;
        int stride = (pos_inst ? 2 * 4 : 2 * 12) + (zeroInstSeeds ? 0 : 4 * perInstDataDwords);
        uint8_t *ptr = pool.topPtr(vbPtr);

        scsLocal[crt.getCellIndex(p->riResIdx, s)].ofs = pool.topOfs + 1;

        int dest_stride = RIGEN_STRIDE_B(pos_inst, zeroInstSeeds, perInstDataDwords);
        int dest_adata = pos_inst ? 0 : RIGEN_ADD_STRIDE_PER_INST_B(zeroInstSeeds, perInstDataDwords);
        int src_adata = zeroInstSeeds ? 0 : perInstDataDwords * 4;
        buf_tm32.clear();
        buf_w16.clear();
        if (!pos_inst && rendinst::tmInst12x32bit)
        {
          stride += 4 * 12 - 2 * 12;
          buf_tm32.resize(stride * cnt);
          ptr = buf_tm32.data();
        }
        else if (perInstDataDwords && !zeroInstSeeds)
        {
          buf_w16.resize(stride * cnt);
          ptr = buf_w16.data();
        }

        if (pool.avail > cnt)
        {
          memcpy(ptr, pregenData, cnt * stride);
          pregenData += cnt * stride;
          pool.topOfs += cnt * dest_stride;
          pool.avail -= cnt;
        }
        else if (pool.avail)
        {
          memcpy(ptr, pregenData, pool.avail * stride);
          pregenData += cnt * stride;
          pool.topOfs += pool.avail * dest_stride;
          cnt = pool.avail;
          pool.avail = 0;
        }
        else
        {
          pregenData += cnt * stride;
          cnt = 0;
        }

        if (!cnt)
          continue;
        bbox3f lbb = rtData->riResBb[p->riResIdx];
        bool canBeExcluded = useDestrExclForPregenAdd && rtData->riDestr.size() && // debris inited
                             !rtData->riProperties[p->riResIdx].immortal && !rtData->riDestr[p->riResIdx].destructable;
        int x_ofs = pos_inst ? 2 * 0 : (rendinst::tmInst12x32bit ? 4 * 3 + 2 : 2 * 3);
        int y_ofs = pos_inst ? 2 * 1 : (rendinst::tmInst12x32bit ? 4 * 7 + 2 : 2 * 7);
        int z_ofs = pos_inst ? 2 * 2 : (rendinst::tmInst12x32bit ? 4 * 11 + 2 : 2 * 11);
        for (uint8_t *ptr_e = ptr + stride * cnt; ptr < ptr_e; ptr += stride)
        {
          float px =
            *(int16_t *)(ptr + x_ofs) * rendinst::gen::SingleEntityPool::cell_xz_sz / 32767.0f + rendinst::gen::SingleEntityPool::ox;
          float pz =
            *(int16_t *)(ptr + z_ofs) * rendinst::gen::SingleEntityPool::cell_xz_sz / 32767.0f + rendinst::gen::SingleEntityPool::oz;
          if ((canBeExcluded && rendinst::gen::destrExcl.isMarked(px, pz)) ||
              rendinst::gen::SingleEntityPool::intersectWithSweepBoxes(px, pz))
          {
            if (pos_inst)
              *(int16_t *)(ptr + 2 * 3) = 0;
            else
              *(int16_t *)(ptr + 2 * 0) = *(int16_t *)(ptr + 2 * 1) = *(int16_t *)(ptr + 2 * 2) = *(int16_t *)(ptr + 2 * 4) =
                *(int16_t *)(ptr + 2 * 5) = *(int16_t *)(ptr + 2 * 6) = *(int16_t *)(ptr + 2 * 8) = *(int16_t *)(ptr + 2 * 9) =
                  *(int16_t *)(ptr + 2 * 10) = 0;
            continue;
          }
          if (pos_inst)
          {
            vec4f pos, scale;
            rendinst::gen::unpack_tm_pos(pos, scale, (int16_t *)ptr, v_cell_add, v_cell_mul, palette_rotation);
            pos = rendinst::gen::custom_update_pregen_pos_y(pos, (int16_t *)(ptr + y_ofs), rendinst::gen::SingleEntityPool::cell_y_sz,
              rendinst::gen::SingleEntityPool::oy);
            *((int16_t *)(ptr + y_ofs)) = int16_t(
              roundf(clamp((v_extract_y(pos) - rendinst::gen::SingleEntityPool::oy) / rendinst::gen::SingleEntityPool::cell_y_sz,
                       -1.0f, 1.0f) *
                     32767.0f));
            v_bbox3_add_pt(rendinst::gen::SingleEntityPool::bbox, v_madd(lbb.bmin, scale, pos));
            v_bbox3_add_pt(rendinst::gen::SingleEntityPool::bbox, v_madd(lbb.bmax, scale, pos));
          }
          else
          {
            mat44f tm;
            if (!rendinst::tmInst12x32bit)
              rendinst::gen::validate_unpack_tm_full(tm, (int16_t *)ptr, v_cell_add, v_cell_mul);
            else
            {
              rendinst::gen::unpack_tm32_full(tm, (int32_t *)ptr, v_cell_add, v_cell_mul);
              *(int16_t *)(ptr + y_ofs - 2) = 0;
            }
            tm.col3 = rendinst::gen::custom_update_pregen_pos_y(tm.col3, (int16_t *)(ptr + y_ofs),
              rendinst::gen::SingleEntityPool::cell_y_sz, rendinst::gen::SingleEntityPool::oy);
            *((int16_t *)(ptr + y_ofs)) = int16_t(
              roundf(clamp((v_extract_y(tm.col3) - rendinst::gen::SingleEntityPool::oy) / rendinst::gen::SingleEntityPool::cell_y_sz,
                       -1.0f, 1.0f) *
                     32767.0f));
            v_bbox3_add_transformed_box(rendinst::gen::SingleEntityPool::bbox, tm, lbb);
          }
        }

        // Copy only base data without additional
        if (buf_tm32.size())
          copy_interlaced_buf(cnt, pool.topPtr(vbPtr), dest_stride, buf_tm32.data() + 2, 4, src_adata, dest_adata);
        else if (buf_w16.size())
          copy_interlaced_buf(cnt, pool.topPtr(vbPtr), dest_stride, buf_w16.data(), 2, src_adata, dest_adata);

#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS // use additional data as hashVal only when in Tools (De3X, AV2)
        // We need second pass because in the first we used high 16bits of 32bits values, but for additional we need low 16bits
        int dest_shift = dest_stride - dest_adata;
        int src_shift = stride - src_adata;
        int skip_stride = dest_stride - (zeroInstSeeds ? 0 : perInstDataDwords * sizeof(short));
        if (buf_tm32.size())
          copy_interlaced_buf(cnt, pool.topPtr(vbPtr) + dest_shift, dest_stride, buf_tm32.data() + src_shift, 4, src_shift,
            skip_stride);
        else if (buf_w16.size() && dest_adata > 0)
        {
          // using 4 for src_step, because hash values are stored as dwords in buf_w16
          copy_interlaced_buf(cnt, pool.topPtr(vbPtr) + dest_shift, dest_stride, buf_w16.data() + src_shift, 4, src_shift,
            skip_stride);
        }
#endif
      }

    if (maskGeneratedEnabled)
      for (int i = 0; i < cell.cls.size(); i++)
      {
        int cls_idx = cell.cls[i].landClsIdx;
        rendinst::gen::land::AssetData &land = *landCls[cls_idx].asset;

#if _TARGET_PC_TOOLS_BUILD
        int bx0 = x * cellSz + (s % SUBCELL_DIV) * cellSz / SUBCELL_DIV;
        int bz = z * cellSz + (s / SUBCELL_DIV) * cellSz / SUBCELL_DIV;
        int bsz = (cellSz + SUBCELL_DIV - 1) / SUBCELL_DIV;
        for (int bz1 = bz + bsz; bz < bz1; bz++)
          for (int bx = bx0, bx1 = bx0 + bsz; bx < bx1; bx++)
            if (landCls[cls_idx].mask->get(bx, bz))
              goto generate;
        continue;
      generate:
#endif

        pools.resize(land.riRes.size());
        for (int j = 0; j < pools.size(); j++)
        {
          int idx = landCls[cls_idx].riResMap[j];
          if (crt.pools[idx].avail < 0)
          {
            pools[idx].topPtr = pools[idx].basePtr = nullptr;
            pools[idx].avail = pools[idx].shortage = 0;
            continue;
          }
          if (!scsLocal[crt.getCellIndex(idx, s)].ofs)
            scsLocal[crt.getCellIndex(idx, s)].ofs = crt.pools[idx].topOfs + 1;
          pools[j].topPtr = pools[j].basePtr = crt.pools[idx].topPtr(vbPtr);
          pools[j].avail = crt.pools[idx].avail;
          pools[j].shortage = 0;
        }

#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS
        G_ASSERT(perInstDataDwords <= 4);
// We assume it is <= 4 in tools
// Take a look into genObjUtil.h to fix it (see per_inst_data_dwords)
#endif

        if (land.tiled)
          rendinst::gen::generateTiledEntitiesInMaskedRect(*land.tiled, make_span(pools), *landCls[cls_idx].mask, world2grid, x0, z0,
            box_sz, world0x(), world0z(), landCls[cls_idx].riResMap, densMapPivotX, densMapPivotZ);

        if (land.planted)
          rendinst::gen::generatePlantedEntitiesInMaskedRect(*land.planted, land.layerIdx, make_span(pools), *landCls[cls_idx].mask,
            world2grid, x0, z0, box_sz, world0x(), world0z(), 0, landCls[cls_idx].riResMap, densMapPivotX, densMapPivotZ);

        for (int j = 0; j < pools.size(); j++)
        {
          int idx = landCls[cls_idx].riResMap[j];
          crt.pools[idx].avail = pools[j].avail;
          crt.pools[idx].topOfs = (uint8_t *)pools[j].topPtr - vbPtr;
        }
      }

    for (int i = 0; i < p_cnt; i++)
    {
      CellRtData::SubCellSlice &scs = scsLocal[crt.getCellIndex(i, s)];
      if (scs.ofs)
      {
        scs.ofs--;
        scs.sz = crt.pools[i].topOfs - scs.ofs;
      }
      else
      {
        scs.ofs = s > 0 ? scsLocal[crt.getCellIndex(i, s - 1)].ofs + scsLocal[crt.getCellIndex(i, s - 1)].sz : 0;
      }
      G_ASSERTF(scs.ofs >= 0 && scs.sz >= 0, "scs.ofs=%d scs.sz=%d", scs.ofs, scs.sz);
    }

    v_bbox3_add_box(crt.bbox[0], crt.bbox[s + 1] = rendinst::gen::SingleEntityPool::bbox);
    // debug("sc[%d] box: (%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f)", s, P3D(as_point4(&crt.bbox[s+1].bmin)),
    // P3D(as_point4(&crt.bbox[s+1].bmax)));
  }
#if _TARGET_PC_TOOLS_BUILD
  {
    // Disabling this code may cause some bugs with RiGen collision in game,
    // but they weren't noticed, so we just disable potential overhead
    bbox3f b = crt.bbox[0];
    b.bmin = v_sub(b.bmin, crt.cellOrigin);
    b.bmax = v_sub(b.bmax, crt.cellOrigin);
    v_bbox3_add_box(rtData->maxCellBbox, b);
  }
#endif

  rendinst::gen::SingleEntityPool::sweep_boxes_itm.reset();
  clear_and_shrink(buf_tm32);
  // debug("cell box: (%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f)", P3D(as_point4(&crt.bbox[0].bmin)), P3D(as_point4(&crt.bbox[0].bmax)));
  for (int s = SUBCELL_DIV * SUBCELL_DIV - 2; s >= 0; s--)
  {
    for (int i = 0; i < p_cnt; i++)
    {
      CellRtData::SubCellSlice &scs = scsLocal[crt.getCellIndex(i, s)];
      if (!scs.ofs && !scs.sz)
      {
        scs.ofs = scsLocal[crt.getCellIndex(i, s + 1)].ofs;
      }
    }
  }

  uint32_t nonEmptyPools = 1;
  for (int i = 0; i < p_cnt; i++)
  {
    uint32_t total = 0;
    for (int s = 0; s < SUBCELL_DIV * SUBCELL_DIV; s++)
      total += scsLocal[crt.getCellIndex(i, s)].sz;

    crt.scsRemap[i] = total > 0 ? nonEmptyPools : 0;
    nonEmptyPools += total > 0 ? 1 : 0;
  }

  clear_and_resize(crt.scs, SUBCELL_DIV * SUBCELL_DIV * nonEmptyPools);
  mem_set_0(crt.scs);

  CellRtData::SubCellSlice *target = crt.scs.data();
  for (int i = 0; i < p_cnt; i++)
  {
    if (crt.scsRemap[i])
      memcpy(target + crt.scsRemap[i] * SUBCELL_DIV * SUBCELL_DIV, scsLocal.data() + i * SUBCELL_DIV * SUBCELL_DIV,
        sizeof(CellRtData::SubCellSlice) * SUBCELL_DIV * SUBCELL_DIV);
  }

#if DAGOR_DBGLEVEL > 0
  for (int i = 0; i < p_cnt; i++)
  {
    for (int s = 0; s < SUBCELL_DIV * SUBCELL_DIV; s++)
    {
      auto src = scsLocal[crt.getCellIndex(i, s)];
      auto dst = crt.getCellSlice(i, s);
      G_ASSERT(src.sz == dst.sz);
      if (src.sz)
        G_ASSERT(src.ofs == dst.ofs);
    }
  }
#endif

  if (!rtData->rtPoolData.empty()) // Resized for render only (see `RendInstGenData::initRender)
  {
    rtData->riRwCs.lockRead();
    auto mrange = make_span_const(vbPtr, crt.vbSize);
    for (const auto &cellDestr : rtData->riDestrCellData)
    {
      if (cellDestr.cellId != cellId)
        continue;
      for (const auto &poolDestr : cellDestr.destroyedPoolInfo)
      {
        auto pRtPoolData = safe_at(rtData->rtPoolData, poolDestr.poolIdx);
        if (DAGOR_UNLIKELY(!pRtPoolData)) // Destroy data from previous level?
        {
          logerr("Ignore attempt to destroy invalid pool %d ouf of %d on generating cell %d @ (%d, %d)", poolDestr.poolIdx,
            rtData->rtPoolData.size(), cellId, x, z);
          break;
        }
        if (!*pRtPoolData) // Missing res?
          continue;

        bool posInst = rtData->riPosInst[poolDestr.poolIdx];
        int stride_w = RIGEN_STRIDE_B(posInst, rtData->riZeroInstSeeds[poolDestr.poolIdx], perInstDataDwords) / 2;
        uint8_t *basePoolPtr = vbPtr + crt.pools[poolDestr.poolIdx].baseOfs;
        for (auto &range : poolDestr.destroyedInstances)
        {
          for (int16_t *data = (int16_t *)(basePoolPtr + range.startOffs), *data_e = (int16_t *)(basePoolPtr + range.endOffs);
               data < data_e; data += stride_w)
          {
            if (posInst)
              rendinst::destroy_pos_rendinst_data(data, mrange);
            else
              rendinst::destroy_tm_rendinst_data(data, mrange);
          }
        }
      }
      break;
    }
    rtData->riRwCs.unlockRead();
  }

  if (zcrd)
  {
    zcrd->ceaseReading();
    zcrd->~IGenLoad();
    fcrd.endBlock();
  }

  // int ri_count = 0;
  // for (int i = 0; i < p_cnt; i ++)
  //   ri_count +=  crt.pools[i].total - crt.pools[i].avail;
  // debug("cell (%2d,%2d): %d inst", x, z, ri_count);
  crt_ptr->cellStateFlags &= ~RendInstGenData::CellRtData::STATIC_SHADOW_RENDERED;
  return crt_ptr;
}

void rendinst::configurateRIGen(const DataBlock &riSetup)
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    DAG_FATAL("%s should be called only when all riGen layers are destroyed, but rgLayer[%d]=%p", __FUNCTION__, _layer, rgl);
    return;
  }
  rgLayer.resize(riSetup.getInt("layersCnt", 2));
  mem_set_0(rgLayer);
  rgAttr.resize(rgLayer.size());
  mem_set_0(rgAttr);
  rgPrimaryLayers = riSetup.getInt("primaryLayersCnt", 1);
  if (rgPrimaryLayers > rgLayer.size())
  {
    G_ASSERTF(rgPrimaryLayers <= rgLayer.size(), "rgPrimaryLayers=%d rgLayer.size()=%d", rgPrimaryLayers, rgLayer.size());
    rgPrimaryLayers = rgLayer.size();
  }

  for (int i = 0; i < rgLayer.size(); i++)
  {
    const DataBlock &b = *riSetup.getBlockByNameEx(String(0, "layer%d", i + 1));
    rgAttr[i].minDistMul = b.getReal("minDistMul", 0.1f);
    rgAttr[i].maxDistMul = b.getReal("maxDistMul", isRgLayerPrimary(i) ? 10.0f : 0.75f);
    rgAttr[i].poiRad = b.getReal("poiRad", 512.0f);
    rgAttr[i].cellSizeDivisor = b.getInt("cellSizeDivisor", i == 0 ? 1 : 8);
    rgAttr[i].needNetSync = b.getBool("needNetSync", i == 0 && isRgLayerPrimary(i));
    rgAttr[i].render = b.getBool("render", true);
    rgAttr[i].depthShadows = b.getBool("depthShadows", true);
    rgAttr[i].clipmapShadows = b.getBool("clipmapShadows", isRgLayerPrimary(i));
    debug("rgLayer.%d: %s div=%d defPoiRad=%.0f distMulRange=(%.2f, %.2f) %s %s %s %s", i + 1, isRgLayerPrimary(i) ? "PRIM" : "sec ",
      rgAttr[i].cellSizeDivisor, rgAttr[i].poiRad, rgAttr[i].minDistMul, rgAttr[i].maxDistMul,
      rgAttr[i].needNetSync ? "needNetSync" : "", rgAttr[i].render ? "render" : "", rgAttr[i].depthShadows ? "depthShadows" : "",
      rgAttr[i].clipmapShadows ? "clipmapShadows" : "");
  }
  rebuildRgRenderMasks();
}

void rendinst::initRIGen(bool need_render, int cell_pool_sz, float poi_radius, ri_register_collision_cb reg_coll_cb,
  ri_unregister_collision_cb unreg_coll_cb, int job_manager_id, float sec_layer_poi_radius, bool simplified_render,
  bool should_init_gpu_objects)
{
  if (!rendinst::rgLayer.size())
  {
    logwarn("implicit configurateRIGen() call");
    configurateRIGen(DataBlock::emptyBlock);
  }

  rendinst::maxExtraRiCount = ::dgs_get_game_params()->getInt("rendinstExtraMaxCnt", 4000);
  RendInstGenData::renderResRequired = need_render;
  if (need_render && !ShaderGlobal::get_int_by_name("per_instance_data_no", rendinst::render::instancingTexRegNo))
  {
    rendinst::render::instancingTexRegNo = 14;
    logerr("\"per_instance_data_no\" shader var not exist, using 14 as fallback");
  }

  externalJobId = job_manager_id;
  if (externalJobId < 0)
  {
    using namespace cpujobs;
    riGenJobId = create_virtual_job_manager(256 << 10, WORKER_THREADS_AFFINITY_MASK, "riGenJobs", DEFAULT_THREAD_PRIORITY,
      DEFAULT_IDLE_TIME_SEC / 2);
  }
  else
    riGenJobId = externalJobId;

  riCellPoolSz = cell_pool_sz;
  for (int _layer = 0; _layer < rendinst::rgLayer.size(); _layer++)
  {
    float rad = (_layer < rendinst::rgPrimaryLayers) ? poi_radius : sec_layer_poi_radius;
    if (rad > -100)
      rendinst::rgAttr[_layer].poiRad = rad;
  }
  regCollCb = reg_coll_cb;
  unregCollCb = unreg_coll_cb;

  if (RendInstGenData::renderResRequired)
    RendInstGenData::initRenderGlobals(!simplified_render, should_init_gpu_objects);

  const DataBlock &optBlk = *::dgs_get_game_params()->getBlockByNameEx("rendinstOpt");
  riFullLodsNames.reset();
  riHideNames.reset();
  riHideMode0Names.reset();
  riHideMode1Names.reset();
  for (int modeNo = 0; modeNo < numHideModes; modeNo++)
  {
    clear_all_ptr_items(riHideModeNamesRE[modeNo]);
    clear_all_ptr_items(riHideModeNamesExclRE[modeNo]);
    riHideModeMinRad2[modeNo] = 0;
  }

  if (const DataBlock *b = optBlk.getBlockByName("leaveLod0"))
    for (int i = 0; i < b->paramCount(); i++)
      if (b->getParamType(i) == b->TYPE_STRING)
        riFullLodsNames.addNameId(b->getStr(i));

  if (simplified_render)
  {
    debug("using simplified render");
    if (const DataBlock *b = optBlk.getBlockByName("hideForAll"))
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == b->TYPE_STRING)
          riHideNames.addNameId(b->getStr(i));
    if (const DataBlock *b = optBlk.getBlockByName("hideForMode0"))
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == b->TYPE_STRING)
          riHideMode0Names.addNameId(b->getStr(i));
    if (const DataBlock *b = optBlk.getBlockByName("hideForMode1"))
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == b->TYPE_STRING)
          riHideMode1Names.addNameId(b->getStr(i));

    debug("leaveLod0.count=%d", riFullLodsNames.nameCount());
    debug("hideForAll.count=%d", riHideNames.nameCount());
    debug("hideForMode0.count=%d", riHideMode0Names.nameCount());
    debug("hideForMode1.count=%d", riHideMode1Names.nameCount());
    RenderableInstanceLodsResource::get_skip_first_lods_count = get_skip_nearest_lods;
  }
  else
  {
    if (const DataBlock *b = optBlk.getBlockByName("hideForMode1"))
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == b->TYPE_STRING)
          riHideMode1Names.addNameId(b->getStr(i));
    if (const DataBlock *b = optBlk.getBlockByName("hideForMode0"))
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == b->TYPE_STRING)
          if (riHideMode1Names.getNameId(b->getStr(i)) < 0)
            riHideMode0Names.addNameId(b->getStr(i));
    riHideMode1Names.reset();
    debug("hideForMode0.count=%d", riHideMode0Names.nameCount());
  }

  {
    rendinst::RiExtraPool::defLodLimits &= 0x000000FF;
    rendinst::RiExtraPool::defLodLimits |= (simplified_render ? 1 : 0);
    int inc_nid = optBlk.getNameId("incRE");
    int exc_nid = optBlk.getNameId("excRE");
    for (int modeNo = 0; modeNo < numHideModes; modeNo++)
    {
      int minLod = clamp(max(optBlk.getInt(String(0, "minLodForMode%d", modeNo), 0), (simplified_render ? 1 : 0)), 0, 15);
      int maxLod = clamp(optBlk.getInt(String(0, "maxLodForMode%d", modeNo), 15), 0, 15);
      rendinst::RiExtraPool::defLodLimits |= (minLod | (maxLod << 4)) << (modeNo * 8 + 8);

      riHideModeMinRad2[modeNo] = sqr(optBlk.getReal(String(0, "minBoundRadForMode%d", modeNo), 0));

      if (const DataBlock *b = optBlk.getBlockByName(String(0, "hideForMode%dre", modeNo)))
        for (int i = 0; i < b->paramCount(); i++)
          if (b->getParamType(i) == b->TYPE_STRING && (b->getParamNameId(i) == inc_nid || b->getParamNameId(i) == exc_nid))
          {
            RegExp *re = new RegExp;
            if (re->compile(b->getStr(i), "i"))
              (b->getParamNameId(i) == inc_nid ? riHideModeNamesRE[modeNo] : riHideModeNamesExclRE[modeNo]).push_back(re);
            else
              delete re;
          }
    }
  }
#if !_TARGET_PC_TOOLS_BUILD
  if (!RendInstGenData::renderResRequired && rgLayer.size() > rgPrimaryLayers)
  {
    debug("removing %d secondary layer(s) due to RendInstGenData::renderResRequired=false", rgLayer.size() - rgPrimaryLayers);
    rgLayer.resize(rgPrimaryLayers);
    rgAttr.resize(rgPrimaryLayers);
  }
#endif
  rotationPaletteManager = eastl::make_unique<rendinst::gen::RotationPaletteManager>();

  if (!render::meshRElemsUpdatedCbToken)
    render::meshRElemsUpdatedCbToken =
      unitedvdata::riUnitedVdata.on_mesh_relems_updated.subscribe(rendinst::render::on_ri_mesh_relems_updated);
}

void rendinst::termRIGen()
{
  if (riGenJobId >= 0)
  {
    if (externalJobId < 0)
    {
      cpujobs::reset_job_queue(riGenJobId, false);
      spin_wait([&] { return cpujobs::is_job_manager_busy(riGenJobId); });
      cpujobs::destroy_virtual_job_manager(riGenJobId, true);
    }
    riGenJobId = -1;
  }
  render::meshRElemsUpdatedCbToken = {};
  rendinst::termRIGenExtra();
  rendinst::clearRIGen();
  RendInstGenData::termRenderGlobals();
  rotationPaletteManager.reset();
  riFullLodsNames.reset();
  riHideNames.reset();
  riHideMode0Names.reset();
  riHideMode1Names.reset();
  for (int modeNo = 0; modeNo < numHideModes; modeNo++)
  {
    clear_all_ptr_items_and_shrink(riHideModeNamesRE[modeNo]);
    clear_all_ptr_items_and_shrink(riHideModeNamesExclRE[modeNo]);
  }
  rendinst::term_stub_res();
}

void rendinst::setPoiRadius(float poi_radius)
{
  for (int _layer = 0; _layer < min((unsigned)rendinst::rgLayer.size(), rendinst::rgPrimaryLayers); _layer++)
    rendinst::rgAttr[_layer].poiRad = poi_radius;
}

uint8_t rendinst::getResHideMask(const char *res_name, const BBox3 *lbox)
{
  if (!res_name)
    return 0;

  bool mode0 = true, mode1 = true, mode2 = true;
  if (lbox)
  {
    float rad2 = lbox->width().lengthSq() / 4;
    mode0 = rad2 > riHideModeMinRad2[0];
    mode1 = rad2 > riHideModeMinRad2[1];
    mode2 = rad2 > riHideModeMinRad2[2];
    /*
    if (!mode0)
      debug("hide(%s)[%d] due to rad2=%.3f", res_name, 0, rad2);
    if (!mode1)
      debug("hide(%s)[%d] due to rad2=%.3f", res_name, 1, rad2);
    */
  }

  if (RenderableInstanceLodsResource::get_skip_first_lods_count == get_skip_nearest_lods)
    if (riHideNames.getNameId(res_name) >= 0)
    {
      // debug("hide(%s)[all] due to names", res_name, 0);
      return 0xFF;
    }

  if (mode0)
    mode0 = riHideMode0Names.getNameId(res_name) < 0;
  if (mode0)
    for (int i = 0; i < riHideModeNamesRE[0].size(); i++)
      if (riHideModeNamesRE[0][i]->test(res_name))
      {
        mode0 = false;
        // if (!mode0)
        //   debug("hide(%s)[%d] due to re", res_name, 0);
        break;
      }
  if (mode1)
    mode1 = riHideMode1Names.getNameId(res_name) < 0;
  if (mode1)
    for (int i = 0; i < riHideModeNamesRE[1].size(); i++)
      if (riHideModeNamesRE[1][i]->test(res_name))
      {
        mode1 = false;
        // if (!mode1)
        //   debug("hide(%s)[%d] due to re", res_name, 1);
        break;
      }
  if (!mode0)
    for (int i = 0; i < riHideModeNamesExclRE[0].size(); i++)
      if (riHideModeNamesExclRE[0][i]->test(res_name))
      {
        mode0 = true;
        break;
      }
  if (!mode1)
    for (int i = 0; i < riHideModeNamesExclRE[1].size(); i++)
      if (riHideModeNamesExclRE[1][i]->test(res_name))
      {
        mode1 = true;
        break;
      }

  for (int i = 0; i < riHideModeNamesRE[2].size(); i++)
    if (riHideModeNamesRE[2][i]->test(res_name))
    {
      mode2 = false;
      break;
    }
  if (!mode2)
  {
    for (int i = 0; i < riHideModeNamesExclRE[2].size(); i++)
      if (riHideModeNamesExclRE[2][i]->test(res_name))
      {
        mode2 = true;
        break;
      }
  }

  return (mode0 ? 0 : 1) | (mode1 ? 0 : 2) | (mode2 ? 0 : 4);
}

void rendinst::registerRIGenSweepAreas(dag::ConstSpan<TMatrix> box_itm_list)
{
  ScopedLockAllRgLayersForWrite lock;

  bool changed = full_sweep_box_itm_list.size() || box_itm_list.size();

  full_sweep_box_itm_list = box_itm_list;
  full_sweep_wbox_list.resize(full_sweep_box_itm_list.size());
  for (int i = 0; i < full_sweep_box_itm_list.size(); i++)
  {
    TMatrix tm = inverse(box_itm_list[i]);
    full_sweep_wbox_list[i].setempty();
    full_sweep_wbox_list[i] += Point2::xz(tm * Point3(-1, 0, -1));
    full_sweep_wbox_list[i] += Point2::xz(tm * Point3(-1, 0, 1));
    full_sweep_wbox_list[i] += Point2::xz(tm * Point3(1, 0, 1));
    full_sweep_wbox_list[i] += Point2::xz(tm * Point3(1, 0, -1));
  }

  FOR_EACH_RG_LAYER_DO (rgl)
    if (changed && rgl->rtData->loaded.getList().size())
    {
      dag::ConstSpan<int> ld = rgl->rtData->loaded.getList();
      debug("reset %d loaded cells (due to sweep areas changed)", ld.size());
      for (auto ldi : ld)
        if (RendInstGenData::CellRtData *crt = rgl->cells[ldi].rtData)
          crt->clear();

      rgl->rtData->loaded.reset();
      rgl->rtData->lastPoi.set(-1e6, 1e6);
    }
}

void rendinst::regenerateRIGen()
{
  ScopedLockAllRgLayersForWrite lock;

  if (!rendinst::gen::destrExcl.constBm)
    rendinst::gen::destrExcl.clear();
  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData->loaded.getList().size())
    {
      dag::ConstSpan<int> ld = rgl->rtData->loaded.getList();
      debug("reset %d loaded cells", ld.size());
      for (auto ldi : ld)
        if (RendInstGenData::CellRtData *crt = rgl->cells[ldi].rtData)
          crt->clear();

      rgl->rtData->loaded.reset();
      rgl->rtData->lastPoi = Point2(-1000000, 1000000);
    }
}

struct RiGenCellSortPredicate
{
  dag::ConstSpan<Point3> poi;
  int poiCount;
  vec4f w0xz, csz;
  int cell_stride;

  RiGenCellSortPredicate(dag::ConstSpan<Point3> p, int pc, const RendInstGenData *rgl) : poi(p), poiCount(pc)
  {
    csz = v_splats(rgl->grid2world * rgl->cellSz);
    w0xz = rgl->getWorld0Vxz();
    cell_stride = rgl->cellNumW;
  }

  bool operator()(int a, int b) const
  {
    // Note: can int division be replaced to masking? Is it guaranteed to be pow2?
    vec4f av = v_madd(v_make_vec4f((a % cell_stride + 0.5f), (a / cell_stride + 0.5f), 0.f, 0.f), csz, w0xz);
    vec4f bv = v_madd(v_make_vec4f((b % cell_stride + 0.5f), (b / cell_stride + 0.5f), 0.f, 0.f), csz, w0xz);
    vec4f ad2 = v_set_x(1e12f), bd2 = v_set_x(1e12f);
    for (int i = 0; i < poiCount; i++)
    {
      vec4f pv = v_perm_xzxz(v_ldu_p3_safe(i < poi.size() ? &poi[i].x : &rendinst::preloadPointForRendInsts.x));
      ad2 = v_min(ad2, v_length2_sq_x(v_sub(pv, av)));
      bd2 = v_min(bd2, v_length2_sq_x(v_sub(pv, bv)));
    }
    return v_test_vec_x_lt(ad2, bd2);
  }
};

static bool isCellAdjacentToCornerByIdx(int cornerIndex, int cellIndex, int cellStride)
{
  // [cornerIndex][cornerIndex + 1]
  // [cornerIndex + cellStride ][cornerIndex + cellStride + 1]
  return cellIndex == cornerIndex || cellIndex == cornerIndex + 1 || cellIndex == cornerIndex + cellStride ||
         cellIndex == cornerIndex + cellStride + 1;
}

static int scheduleRIGenPrepare(RendInstGenData *rgl, dag::ConstSpan<Point3> poi)
{
  class UnloadRiCell final : public cpujobs::IJob
  {
    RendInstGenData *rgl;
    union
    {
      unsigned tag;
      struct
      {
        unsigned idx : 24;
        unsigned layer : 4;
        unsigned job_tag : 4;
      };
    };
    RendInstGenData::CellRtDataLoaded unloadedCellData;

  public:
    UnloadRiCell(RendInstGenData *ri_gen, int _idx, int layer_idx) : rgl(ri_gen), tag(UNLOAD_JOB_TAG | (layer_idx << 24) | _idx)
    {
      G_FAST_ASSERT(_idx == idx);
      rgl->rtData->toUnload.addInt(_idx);
    }
    virtual void doJob() override
    {
      // debug("unload %d,%d", idx%rgl->cellNumW, idx/rgl->cellNumW);
      G_ASSERT((unsigned)idx < rgl->cells.size());

      ScopedLockWrite lock(rgl->rtData->riRwCs);
      bool wasReady = rgl->cells[idx].isReady();
      RendInstGenData::CellRtData *crt = rgl->cells[idx].rtData;
      if (crt) // Note: actual data deleted in dtor (to be able safely read it from main thread)
        unloadedCellData = eastl::move((RendInstGenData::CellRtDataLoaded &)*crt);
      rgl->rtData->loaded.delInt(idx);
      rgl->rtData->toUnload.delInt(idx);
      if (wasReady)
      {
        int cell_stride = rgl->cellNumW;
        int cx = idx % cell_stride, cz = idx / cell_stride;
        if (rgl->rtData->loadedCellsBBox[0].x == cx || rgl->rtData->loadedCellsBBox[0].y == cz ||
            rgl->rtData->loadedCellsBBox[1].x == cx || rgl->rtData->loadedCellsBBox[1].y == cz) // border cell! recompute bounding
        {
          rgl->rtData->loadedCellsBBox = IBBox2(IPoint2(10000, 10000), IPoint2(-10000, -10000));
          for (int i = 0; i < rgl->rtData->loaded.size(); i++)
          {
            int cellI = rgl->rtData->loaded[i];
            if (rgl->cells[cellI].isReady())
              rgl->rtData->loadedCellsBBox += IPoint2(cellI % cell_stride, cellI / cell_stride);
          }
        }
      }
      if (crt)
        crt->clear();
    }
    virtual unsigned getJobTag() override { return tag; };
    virtual void releaseJob() override { delete this; }
  };

  class RegenRiCell final : public cpujobs::IJob
  {
    RendInstGenData *rgl;
    int idx;
    unsigned tag;
    bool isLastJob = false;

  public:
    RegenRiCell(RendInstGenData *ri_gen, int _idx, int layer_idx) :
      rgl(ri_gen), idx(_idx), tag(REGEN_JOB_TAG | idx | (layer_idx << 24))
    {
      rgl->rtData->toLoad.addInt(idx);
    }
    virtual void doJob() override
    {
#if DAGOR_DBGLEVEL > 0
      {
        ScopedLockRead lock(rgl->rtData->riRwCs);
        bool loaded = rgl->rtData->loaded.hasInt(idx);
        G_ASSERT(rgl->cells[idx].isLoaded() == loaded);
      }
#endif

      int cell_stride = rgl->cellNumW;
      int cx = idx % cell_stride, cz = idx / cell_stride;
      G_FAST_ASSERT((unsigned)idx < rgl->cells.size());

      if (rgl->cells[idx].isLoaded())
      {
        debug("ignore attempt to load already loaded cell @(%d,%d)", cx, cz);
        return;
      }

      // debug("generate %d,%d", cx, cz);

      int startTime = get_time_msec();

      RendInstGenData::CellRtData *crt = rgl->generateCell(cx, cz);
      if (RendInstGenData::riGenValidateGeneratedCell && crt && crt->bbox.size())
        crt = RendInstGenData::riGenValidateGeneratedCell(rgl, crt, idx, cx, cz);

      rgl->updateVb(*crt, idx);

      bool canceled;
      {
        ScopedLockWrite lock(rgl->rtData->riRwCs);
        rgl->rtData->loaded.addInt(idx);
        canceled = !rgl->rtData->toLoad.delInt(idx);
        if (crt->sysMemData) // if is ready
          rgl->rtData->loadedCellsBBox += IPoint2(cx, cz);
        interlocked_release_store_ptr(rgl->cells[idx].rtData, crt); // This makes cell "ready"
        isLastJob = rgl->rtData->toLoad.empty();
      }

      if (canceled)
        debug("loaded canceled cell @(%d,%d)", cx, cz);

      constexpr int maxReasonableTimeMs = 5000;
      if (get_time_msec() - startTime > maxReasonableTimeMs)
        debug("generateCell in %dms @(%d, %d)", get_time_msec() - startTime, cx, cz);

      if (isLastJob)
        rendinst::optimizeRIGenExtra();
    }
    virtual unsigned getJobTag() { return tag; };
    virtual void releaseJob()
    {
      if (isLastJob && rendinst::do_delayed_ri_extra_destruction)
        rendinst::do_delayed_ri_extra_destruction();
      delete this;
    }
  };

  if (!rgl || !rgl->rtData)
    return 0;

  int layer_idx = rgl->rtData->layerIdx;
  G_ASSERTF(layer_idx >= 0 && layer_idx < rendinst::rgLayer.size() && layer_idx < 16, "layer_idx=%d rendinst::rgLayer=%d", layer_idx,
    rendinst::rgLayer.size());

  int poiCount = (!rendinst::isRgLayerPrimary(layer_idx) || rendinst::preloadPointForRendInstsExpirationTimeMs < ::get_time_msec())
                   ? poi.size()
                   : poi.size() + 1;
  if (!poiCount)
    return 0;

  float csz = rgl->grid2world * rgl->cellSz;
  if (poiCount == 1)
    if (poi.size() && lengthSq(rgl->rtData->lastPoi - Point2::xz(poi[0])) < csz * csz * 0.0025 &&
        fabsf(rgl->rtData->preloadDistance - rgl->rtData->lastPreloadDistance) < csz * 0.05f) // 5% of each cell
      return 0;

  int cell_stride = rgl->cellNumW;
  float ox = rgl->world0x();
  float oz = rgl->world0z();
  float poi_rad = rendinst::rgAttr[layer_idx].poiRad;
  poi_rad = poi_rad < 0 ? rgl->rtData->preloadDistance : poi_rad;
  float poiDist2 = (poi_rad + csz / 2) * (poi_rad + csz / 2);
  FastIntList reqCells(framemem_ptr()), newCells(framemem_ptr());

  // We get best fitting 2x2 cells around poi and try to generate instances in them earlier,
  // so that the level does not appear empty.
  // Generation of instances in other cells may happen more than 10 seconds later.
  const bool mayNeedPrepend = rendinst::isRgLayerPrimary(layer_idx) && poi.size();
  int prependPrimaryCellX = 0;
  int prependPrimaryCellZ = 0;
  int prependPrimaryCellIndex = 0;
  if (mayNeedPrepend)
  {
    prependPrimaryCellX = (int)floorf((poi[0].x - ox) / csz - 0.5f);
    prependPrimaryCellZ = (int)floorf((poi[0].z - oz) / csz - 0.5f);
    prependPrimaryCellIndex = prependPrimaryCellZ * cell_stride + prependPrimaryCellX;
  }

  ScopedLockWrite lock(rgl->rtData->riRwCs);
  FastIntList &loaded = rgl->rtData->loaded;
  FastIntList &toLoad = rgl->rtData->toLoad;
  FastIntList &toUnload = rgl->rtData->toUnload;
  float preloadPointRadius = 1000; //!
  float preloadPointDist2 = (preloadPointRadius + csz / 2) * (preloadPointRadius + csz / 2);
  for (int i = 0; i < poiCount; i++)
  {
    float poiX, poiZ;
    poiX = i < poi.size() ? poi[i].x : rendinst::preloadPointForRendInsts.x;
    poiZ = i < poi.size() ? poi[i].z : rendinst::preloadPointForRendInsts.z;
    float poiRad = i < poi.size() ? poi_rad : preloadPointRadius;
    float dist2 = i < poi.size() ? poiDist2 : preloadPointDist2;

    // preloadPointForRendInsts
    int x0 = (int)floorf((poiX - poiRad - ox) / csz), x1 = (int)floor((poiX + poiRad - ox) / csz);
    int z = (int)floorf((poiZ - poiRad - oz) / csz), z1 = (int)floor((poiZ + poiRad - oz) / csz);
    if (x0 < 0)
      x0 = 0;
    if (x1 >= cell_stride)
      x1 = cell_stride - 1;
    if (z < 0)
      z = 0;
    if (z1 >= rgl->cellNumH)
      z1 = rgl->cellNumH - 1;
    for (; z <= z1; z++)
      for (int x = x0; x <= x1; x++)
        if ((Point2(poiX - ox, poiZ - oz) - Point2(x * csz + 0.5 * csz, z * csz + 0.5 * csz)).lengthSq() < dist2)
        {
          int idx = z * cell_stride + x;
          reqCells.addInt(idx);
          const bool mayNeedPrependThisCell = (i == 0) && riAllowGenJobsQueueReorder && mayNeedPrepend &&
                                              isCellAdjacentToCornerByIdx(prependPrimaryCellIndex, idx, cell_stride);
          if (!rgl->cells[idx].isLoaded() && toLoad.hasInt(idx) && mayNeedPrependThisCell)
          {
            const int jobsRemovedCount = cpujobs::remove_jobs_by_tag(riGenJobId, REGEN_JOB_TAG | idx | (layer_idx << 24), false);
            G_ASSERT(jobsRemovedCount <= 1); // RegenRiCell are expected to have unique tags
            if (jobsRemovedCount > 0)
              toLoad.delInt(idx);
          }
          if (!rgl->cells[idx].isLoaded() && !toLoad.hasInt(idx))
            newCells.addInt(idx);
          else if (toUnload.hasInt(idx))
          {
            cpujobs::remove_jobs_by_tag(riGenJobId, UNLOAD_JOB_TAG | idx | ((layer_idx & 0xF) << 24), false);
            toUnload.delInt(idx);
          }
        }
  }

  int extra_cnt = loaded.getList().size() + toLoad.getList().size() + newCells.getList().size() - riCellPoolSz;
  if (extra_cnt > 0)
  {
    for (int i = 0; i < loaded.getList().size(); i++)
      if (!reqCells.hasInt(loaded.getList()[i]) && !toUnload.hasInt(loaded.getList()[i]))
      {
        cpujobs::add_job(riGenJobId, new UnloadRiCell(rgl, loaded.getList()[i], layer_idx));
        extra_cnt--;
        if (extra_cnt <= 0)
          break;
      }
    for (int i = 0; i < toLoad.getList().size(); i++)
      if (!reqCells.hasInt(toLoad.getList()[i]))
      {
        int idx = toLoad.getList()[i];
        cpujobs::remove_jobs_by_tag(riGenJobId, REGEN_JOB_TAG | idx | (layer_idx << 24), false);
        toLoad.delInt(idx);
        extra_cnt--;
        if (extra_cnt <= 0)
          break;
      }
  }

  if (newCells.size())
  {
    // int64_t t0 = ref_time_ticks();
    eastl::sort((int *)newCells.getList().data(), (int *)newCells.getList().data() + newCells.size(),
      RiGenCellSortPredicate(poi, poiCount, rgl));
    // debug("sort %d cells for %d usec", newCells.size(), get_time_usec(t0));
  }

  int numCellsToPrepend = 0;
  if (mayNeedPrepend)
  {
    for (; numCellsToPrepend < newCells.getList().size(); numCellsToPrepend++)
    {
      int cellIndex = newCells.getList()[numCellsToPrepend];
      const bool prepend = isCellAdjacentToCornerByIdx(prependPrimaryCellIndex, cellIndex, cell_stride);

      if (!prepend)
        break;
    }
  }

  if (numCellsToPrepend > 0)
  {
    for (int cellNo = numCellsToPrepend - 1; cellNo >= 0; cellNo--)
      cpujobs::add_job(riGenJobId, new RegenRiCell(rgl, newCells.getList()[cellNo], layer_idx), true);
  }

  for (int cellNo = numCellsToPrepend; cellNo < newCells.getList().size(); cellNo++)
    cpujobs::add_job(riGenJobId, new RegenRiCell(rgl, newCells.getList()[cellNo], layer_idx));

  if (poi.size())
    rgl->rtData->lastPoi = Point2::xz(poi[0]);

  rgl->rtData->lastPreloadDistance = rgl->rtData->preloadDistance;

  return newCells.getList().size();
}

int rendinst::scheduleRIGenPrepare(dag::ConstSpan<Point3> poi)
{
  if (!rendinst::isRiGenLoaded.load(std::memory_order_acquire)) // check that RI is in operable state
    return 0;

  int ret = 0;
  if (riGenSecLayerEnabled)
  {
    FOR_EACH_RG_LAYER_DO (rgl)
      ret += scheduleRIGenPrepare(rgl, poi);
  }
  else
  {
    FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
      ret += scheduleRIGenPrepare(rgl, poi);
  }
  return ret;
}
bool rendinst::isRIGenPrepareFinished() { return riGenJobId < 0 || !cpujobs::is_job_manager_busy(riGenJobId); }


void rendinst::setLoadingState(bool is_loading)
{
  struct ChangeLoadingState : public DelayedAction
  {
    bool state;
    ChangeLoadingState(bool s) : state(s) {}
    virtual void performAction()
    {
      bool wasLoading = RendInstGenData::isLoading;
      RendInstGenData::isLoading = state;
      if (wasLoading != state && !state)
        rendinst::gpuobjects::flush_pending();
    }
  };
  execute_delayed_action_on_main_thread(new ChangeLoadingState(is_loading));
}

bool rendinst::isRiExtraLoaded() { return !RendInstGenData::isLoading && !riExtra.empty(); }

bool rendinst::loadRIGen(IGenLoad &crd, void (*add_resource_cb)(const char *resname), bool init_sec_ri_extra_here,
  Tab<char> *out_reserve_storage_pregen_ent_for_tools, const DataBlock *level_blk)
{
  G_ASSERTF_RETURN(RendInstGenData::isLoading || is_main_thread(), false, "isLoading=%d mainThread=%d", RendInstGenData::isLoading,
    is_main_thread());
  for (int layer = 0; layer < rendinst::rgLayer.size(); layer++)
    del_it(rendinst::rgLayer[layer]);
  rebuildRgRenderMasks();

  Tab<RendInstGenData *> rgl;
  rgl.resize(rendinst::rgLayer.size());
  mem_set_0(rgl);
  for (int layer = 0; layer < rgl.size() && crd.getBlockRest() > sizeof(RendInstGenData); layer++)
  {
    if (!rendinst::isRgLayerPrimary(layer) && !(rendinstSecondaryLayer && RendInstGenData::renderResRequired))
    {
      if (::dgs_get_settings()->getBlockByNameEx("debug")->getBool("logerrOnSecLayer", false))
        logerr("level contains secondary layers which are prohibited (logerrOnSecLayer)");
      debug("stop loading riGen at rgLayer.%d due to rendinstSecondaryLayer=%d renderResRequired=%d (layers: %d primary / %d total)",
        layer + 1, (int)rendinstSecondaryLayer, (int)RendInstGenData::renderResRequired, rendinst::rgPrimaryLayers,
        rendinst::rgLayer.size());
      break;
    }
    rgl[layer] = RendInstGenData::create(crd, layer);
    debug("loaded rgLayer.%d=%p %s (getBlockRest=%d layers=%d)", layer + 1, rgl[layer],
      rendinst::isRgLayerPrimary(layer) ? "PRIM" : "sec ", crd.getBlockRest(), rgl.size());
    if (!rgl[layer])
    {
      clear_all_ptr_items(rgl);
      return false;
    }
    debug("%dx%d cells (each cell is %dx%d grid with gridCellSz=%.1f)  area=(%.1f,%.1f)-(%.1f,%.1f)  pregenEnt=%d (ofs=0x%08X)",
      rgl[layer]->cellNumW, rgl[layer]->cellNumH, rgl[layer]->cellSz, rgl[layer]->cellSz, rgl[layer]->grid2world,
      rgl[layer]->world0x(), rgl[layer]->world0z(),
      rgl[layer]->world0x() + rgl[layer]->grid2world * rgl[layer]->cellSz * rgl[layer]->cellNumW,
      rgl[layer]->world0z() + rgl[layer]->grid2world * rgl[layer]->cellSz * rgl[layer]->cellNumH, rgl[layer]->pregenEnt.size(),
      rgl[layer]->pregenDataBaseOfs);
  }

  if (out_reserve_storage_pregen_ent_for_tools)
  {
    size_t max_pregen = MAX_PREGEN_RI;
    for (int layer = 0; layer < rgl.size(); layer++)
      if (rgl[layer] && rgl[layer]->pregenEnt.size() + 32 > max_pregen)
        max_pregen = rgl[layer]->pregenEnt.size() + 32;

    size_t resv_per_layer = sizeof(RendInstGenData::PregenEntPoolDesc) * max_pregen;
    out_reserve_storage_pregen_ent_for_tools->resize(resv_per_layer * rgl.size());
    mem_set_0(*out_reserve_storage_pregen_ent_for_tools);
    for (int layer = 0; layer < rgl.size(); layer++)
      if (rgl[layer])
      {
        mem_copy_to(rgl[layer]->pregenEnt, out_reserve_storage_pregen_ent_for_tools->data() + resv_per_layer * layer);
        rgl[layer]->pregenEnt.init(out_reserve_storage_pregen_ent_for_tools->data() + resv_per_layer * layer,
          rgl[layer]->pregenEnt.size());
      }
  }

  if (add_resource_cb)
  {
    for (int layer = 0; layer < rgl.size(); layer++)
    {
      if (!rgl[layer])
        continue;
      for (int i = 0; i < rgl[layer]->landCls.size(); i++)
        add_resource_cb(rgl[layer]->landCls[i].landClassName);
      for (int i = 0; i < rgl[layer]->pregenEnt.size(); i++)
      {
        add_resource_cb(rgl[layer]->pregenEnt[i].riName);
        String coll_name(128, "%s" RI_COLLISION_RES_SUFFIX, rgl[layer]->pregenEnt[i].riName.get());
        if (get_resource_type_id(coll_name) == CollisionGameResClassId)
          add_resource_cb(coll_name);
      }
    }
  }

  rendinst::rgLayer = rgl;
  if (!add_resource_cb)
    rendinst::prepareRIGen(init_sec_ri_extra_here, level_blk);

  rebuildRgRenderMasks();
  debug("rgRenderMaskO=0x%04X rgRenderMaskDS=0x%04X rgRenderMaskCMS=0x%04X", rgRenderMaskO, rgRenderMaskDS, rgRenderMaskCMS);
  return true;
}

static float impostorsDistAddMul = 1.0f;
float rendinst::getDefaultImpostorsDistAddMul() { return impostorsDistAddMul; }

static float distAddMul = 1.0f;
float rendinst::getDefaultDistAddMul() { return distAddMul; }

void rendinst::addRIGenExtraSubst(const char *ri_res_name)
{
  FOR_EACH_RG_LAYER_DO (rgl)
    rgl->addRIGenExtraSubst(ri_res_name);
}
static void prepareRIGen(RendInstGenData *rgl, int layer_idx, const DataBlock *level_blk)
{
  for (int i = 0; i < rgl->landCls.size(); i++)
  {
    if (rgl->landCls[i].asset)
      continue;

    rgl->landCls[i].asset = (rendinst::gen::land::AssetData *)::get_game_resource_ex(
      GAMERES_HANDLE_FROM_STRING(rgl->landCls[i].landClassName), rendinst::gen::land::HUID_LandClass);
    debug("land %s =%p", rgl->landCls[i].landClassName.get(), rgl->landCls[i].asset);
    if (!rgl->landCls[i].asset)
      rgl->landCls[i].asset = &rendinst::gen::land::AssetData::emptyLandStub;
  }
  rgl->prepareRtData(layer_idx);
  if (RendInstGenData::renderResRequired)
    rgl->initRender(level_blk);
}
void rendinst::prepareRIGen(bool init_sec_ri_extra_here, const DataBlock *level_blk)
{
  G_ASSERTF_RETURN(RendInstGenData::isLoading || is_main_thread(), , "isLoading=%d mainThread=%d", RendInstGenData::isLoading,
    is_main_thread());
  const DataBlock &b = *::dgs_get_game_params()->getBlockByNameEx("rendinstExtra");

  riExtraSubstNames.reset();
  rendinstExtraAutoSubst = ::dgs_get_game_params()->getBool("rendinstExtraAutoSubst", true);
  riExtraSubstFaceThres = ::dgs_get_game_params()->getInt("rendinstExtraSubstFaceThres", 4000);
  riExtraSubstFaceDensThres = ::dgs_get_game_params()->getReal("rendinstExtraSubstFaceDensThres", 1e6);
  riExtraSubstPregenThres = ::dgs_get_game_params()->getInt("rendinstExtraSubstPregenThres", 128);
  for (int i = 0; i < b.paramCount(); i++)
    if (b.getParamType(i) == b.TYPE_STRING && get_resource_type_id(b.getStr(i)) == RendInstGameResClassId)
    {
      riExtraSubstNames.addNameId(b.getStr(i));
#if RI_VERBOSE_OUTPUT
      debug("riExtraSubst: add <%s> due to mention in BLK", b.getStr(i));
#endif
    }

  rotationPaletteManager->clear();
  FOR_EACH_RG_LAYER_DO (rgl)
    prepareRIGen(rgl, _layer, level_blk);

  if (RendInstGenData::renderResRequired)
    RendInstGenData::initPaletteVb();

  float impostorsFarDistAddMul = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("rendinstImpostorsFarDistAddMul", 1.0);
  impostorsDistAddMul = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("rendinstImpostorsDistAddMul", 1.0);
  riAllowGenJobsQueueReorder = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("riAllowGenJobsQueueReorder", false);

  rendinst::setImpostorsDistAddMul(impostorsDistAddMul);
  rendinst::setImpostorsFarDistAddMul(impostorsFarDistAddMul);

  if (maxExtraRiCount && riExtraSubstNames.nameCount())
  {
    debug("initRIGenExtra: due to maxExtraRiCount=%d riExtraSubstNames.nameCount()=%d", maxExtraRiCount,
      riExtraSubstNames.nameCount());
    rendinst::initRIGenExtra(RendInstGenData::renderResRequired, level_blk);
  }

  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (!rendinst::isRgLayerPrimary(_layer))
    {
      if (init_sec_ri_extra_here && RendInstGenData::renderResRequired && rgl->rtData)
      {
#if RI_VERBOSE_OUTPUT
        debug("init riExtra subst here");
#endif
      }
      else
        continue;
    }
    iterate_names(riExtraSubstNames, [&](int, const char *name) { rgl->addRIGenExtraSubst(name); });
  }

  // check riExtra maxDist and pregenerate cells that contain RI visible over whole map (e.g., large rocks)
  DEBUG_CP();
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    dag::Span<float> riMaxDist((float *)alloca(rgl->rtData->riRes.size() * sizeof(float)), rgl->rtData->riRes.size());
    mem_set_0(riMaxDist);
    dag::ConstSpan<uint16_t> riExtraIdxPair = rgl->rtData->riExtraIdxPair;
    for (int i = 0; i < riExtraIdxPair.size(); i += 2)
      riMaxDist[riExtraIdxPair[i]] = sqrtf(rendinst::riExtra[riExtraIdxPair[i + 1]].distSqLOD[rendinst::RiExtraPool::MAX_LODS - 1]);

    for (int z = 0; z < rgl->cellNumH; z++)
      for (int x = 0; x < rgl->cellNumW; x++)
      {
        int cellId = x + z * rgl->cellNumW;
        RendInstGenData::Cell &cell = rgl->cells[cellId];
        RendInstGenData::CellRtData *crt = cell.rtData;
        if (crt)
          continue;
        if (!cell.hasFarVisiblePregenInstances(rgl, 125000.f, riMaxDist))
          continue;

        debug("precomputeCell %d,%d", x, z);
        crt = new RendInstGenData::CellRtData(rgl->rtData->riRes.size(), rgl->rtData);
        rgl->precomputeCell(*crt, x, z);
        ScopedLockWrite lock(rgl->rtData->riRwCs);
        cell.rtData = crt;
      }
  }
  DEBUG_CP();
  rendinst::isRiGenLoaded.store(true, std::memory_order_release);
}

void rendinst::precomputeRIGenCellsAndPregenerateRIExtra()
{
  debug("%s started...", __FUNCTION__);
  int ri_count = 0;
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    for (int z = 0; z < rgl->cellNumH; z++)
      for (int x = 0; x < rgl->cellNumW; x++)
      {
        int cellId = x + z * rgl->cellNumW;
        RendInstGenData::Cell &cell = rgl->cells[cellId];
        RendInstGenData::CellRtData *crt = cell.rtData;
        G_ASSERT(!crt);
        crt = new RendInstGenData::CellRtData(rgl->rtData->riRes.size(), rgl->rtData);
        ri_count += rgl->precomputeCell(*crt, x, z);
        cell.rtData = crt;
      }
  debug("%s done, ri_count=%d", __FUNCTION__, ri_count);
}

void rendinst::updateRIGen(float dt, bbox3f *movedDebrisBbox)
{
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      rgl->rtData->updateDebris(dt, movedDebrisBbox);
}
void rendinst::initRiGenDebris(const DataBlock &ri_blk, FxTypeByNameCallback get_fx_type_by_name, bool init_sec_ri_extra_here)
{
  G_ASSERTF_RETURN(RendInstGenData::isLoading || is_main_thread(), , "isLoading=%d mainThread=%d", RendInstGenData::isLoading,
    is_main_thread());
  const DataBlock *prev = rendinst::registerRIGenExtraConfig(&ri_blk);
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (!rgl->rtData)
      continue;
    if (!rendinst::isRgLayerPrimary(_layer) && (!RendInstGenData::renderResRequired || !init_sec_ri_extra_here))
      continue;

    rgl->rtData->initDebris(ri_blk, get_fx_type_by_name);
    if (!rendinst::isRgLayerPrimary(_layer) && init_sec_ri_extra_here)
    {
      debug("rgLayer.%d: init riExtra subst here", _layer + 1);
      iterate_names(riExtraSubstNames, [&](int, const char *name) { rgl->addRIGenExtraSubst(name); });
    }
  }
  rendinst::registerRIGenExtraConfig(prev);
}
void rendinst::initRiGenDebrisSecondary(const DataBlock &ri_blk, FxTypeByNameCallback get_fx_type_by_name)
{
  G_ASSERTF_RETURN(RendInstGenData::isLoading || is_main_thread(), , "isLoading=%d mainThread=%d", RendInstGenData::isLoading,
    is_main_thread());
  const DataBlock *prev = rendinst::registerRIGenExtraConfig(&ri_blk);
  FOR_EACH_RG_LAYER_DO (rgl)
    if (!rendinst::isRgLayerPrimary(_layer) && rgl->rtData)
    {
      rgl->rtData->initDebris(ri_blk, get_fx_type_by_name);
      debug("rgLayer.%d: init riExtra subst here", _layer + 1);
      iterate_names(riExtraSubstNames, [&](int, const char *name) { rgl->addRIGenExtraSubst(name); });
    }
  rendinst::registerRIGenExtraConfig(prev);
}
void rendinst::updateRiDestrFxIds(FxTypeByNameCallback get_fx_type_by_name)
{
  FOR_EACH_RG_LAYER_DO (rgl)
    if (const auto rtData = rgl->rtData)
      for (auto &riDestr : rtData->riDestr)
        riDestr.destrFxId = get_fx_type_by_name(riDestr.destrFxName);
}

void rendinst::clearRIGen()
{
  if (rendinst::maxRenderedRiEx[0] || rendinst::maxRenderedRiEx[1])
    debug("maxRenderedRiEx[0,1]=%d,%d", rendinst::maxRenderedRiEx[0], rendinst::maxRenderedRiEx[1]);
  rendinst::maxRenderedRiEx[0] = rendinst::maxRenderedRiEx[1] = 0;

  rendinst::isRiGenLoaded.store(false, std::memory_order_release);
  if (riGenJobId >= 0)
  {
    if (externalJobId < 0)
      cpujobs::reset_job_queue(riGenJobId, false);

    while (cpujobs::is_job_manager_busy(riGenJobId))
      sleep_msec(10);
  }

  for (int layer = 0; layer < rendinst::rgLayer.size(); layer++)
    del_it(rendinst::rgLayer[layer]);
  rebuildRgRenderMasks();
  rendinst::gen::lcmapExcl.clear();
  rendinst::gen::destrExcl.clear();
  rendinst::gen::SingleEntityPool::sweep_boxes_itm.reset();
  clear_and_shrink(full_sweep_box_itm_list);
  clear_and_shrink(full_sweep_wbox_list);
  rendinst::termRIGenExtra();
  riExtraSubstNames.reset(false);
}

void rendinst::clearRIGenDestrData()
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    ScopedLockWrite lock(rgl->rtData->riRwCs);
    rgl->rtData->riDestrCellData.clear();
  }
}

bool rendinst::isSecLayerEnabled() { return riGenSecLayerEnabled; }

void rendinst::enableSecLayer(bool en)
{
  riGenSecLayerEnabled = en;
  if (!rendinstSecondaryLayer || !RendInstGenData::renderResRequired)
    return;

  if (!en)
    FOR_EACH_SECONDARY_RG_LAYER_DO(rgl)
    {
      ScopedLockWrite lock(rgl->rtData->riRwCs);

      if (rgl->rtData->loaded.getList().size())
      {
        dag::ConstSpan<int> ld = rgl->rtData->loaded.getList();
        debug("reset %d loaded cells", ld.size());
        for (auto ldi : ld)
          if (RendInstGenData::CellRtData *crt = rgl->cells[ldi].rtData)
            crt->clear();

        rgl->rtData->loaded.reset();
        rgl->rtData->lastPoi.set(-1e6, 1e6);
      }
    }
}

bool RendInstGenData::updateLClassColors(const char *name, E3DCOLOR from, E3DCOLOR to)
{
  for (int i = 0; i < landCls.size(); i++)
  {
    if (!landCls[i].asset)
      continue;
    if (stricmp(landCls[i].landClassName.get(), name) != 0)
      continue;

    int cnt = landCls[i].asset->riRes.size();
    for (int j = 0; j < cnt; j++)
      for (int idx = 0; idx < rtData->riRes.size(); idx++)
        if (rtData->riResName[idx] && stricmp(rtData->riResName[idx], landCls[i].asset->riResName[j]) == 0)
        {
          if (!rtData->riPosInst[idx] && !rendinst::render::tmInstColored)
            break;
#if RI_VERBOSE_OUTPUT
          debug("res <%s> changed color %08X-%08X -> %08X-%08X", landCls[i].asset->riResName[j], rtData->riColPair[idx * 2].u,
            rtData->riColPair[idx * 2 + 1].u, landCls[i].asset->riColPair[j * 2].u, landCls[i].asset->riColPair[j * 2 + 1].u);
#endif
          rtData->riColPair[idx * 2 + 0] = from;
          rtData->riColPair[idx * 2 + 1] = to;
          break;
        }
    return true;
  }
  return false;
}


void rendinst::get_ri_color_infos(
  eastl::fixed_function<sizeof(void *) * 4, void(E3DCOLOR colorFrom, E3DCOLOR colorTo, const char *name)> cb)
{
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
  {
    for (int i = 0, ie = rgl->rtData->riRes.size(); i < ie; i++)
    {
      if (rgl->rtData->riResName[i])
      {
        if (!rgl->rtData->riPosInst[i] && !rendinst::render::tmInstColored)
          continue;

        cb(rgl->rtData->riColPair[i * 2 + 0], rgl->rtData->riColPair[i * 2 + 1], rgl->rtData->riResName[i]);
      }
    }
  }
}

void rendinst::overrideLodRanges(const DataBlock &ri_lod_ranges)
{
  ri_lod_ranges_ovr = ri_lod_ranges;
  FOR_EACH_RG_LAYER_DO (rgl)
    rgl->applyLodRanges();
  rendinst::reapplyLodRanges();
}

void rendinst::walkRIGenResourceNames(res_walk_cb cb)
{
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
  {
    for (int i = 0, ie = rgl->rtData->riRes.size(); i < ie; ++i)
      if (rgl->rtData->riResName[i])
        cb(rgl->rtData->riResName[i]);
  }
}

bool rendinst::hasRiLayer(int res_idx, LayerFlag layer)
{
  G_ASSERT_RETURN(res_idx >= 0 && res_idx < riExtra.size(), false);
  return bool(riExtra[res_idx].layers & layer);
}

void rendinst::iterate_apex_destructible_rendinst_data(rendinst::RiApexIterationCallback callback)
{
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
    if (auto rtData = rgl->rtData)
      for (int i = 0; i < rtData->riResName.size(); i++)
        if (rtData->riDestr[i].apexDestructible)
          callback(rtData->riResName[i], rtData->riDestr[i].apexDestructionOptionsPresetName.c_str());
}

bool rendinst::should_init_ri_damage_model()
{
  if (rgPrimaryLayers > 4)
    logerr("RendInst damage model attempted initialization, but the level has more than 4 primary RI layers! "
           "The ri damage model does NOT support more than 4 layers!");

  return rgPrimaryLayers > 0 && rgLayer[0] != nullptr;
}

void rendinst::dm_iter_all_ri_layers(eastl::fixed_function<16, void(int, const dag::ConstSpan<const char *> &)> cb)
{
  for (int i = 0; i < rendinst::rgPrimaryLayers; i++)
    if (const RendInstGenData *layer = rendinst::rgLayer[i])
      if (const auto *rtData = layer->rtData)
        cb(i, rtData->riResName);
}
