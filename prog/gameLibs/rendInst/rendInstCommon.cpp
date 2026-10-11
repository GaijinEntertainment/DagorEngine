// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstAccess.h>
#include "riGen/riGenData.h"
#include "render/genRender.h"

#include <drv/3d/dag_driver.h>
#include <osApiWrappers/dag_miscApi.h>
#include <EASTL/fixed_vector.h>
#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>


namespace rendinst
{

//! reasonable default max 64K/cell (required for net sync in gameLibs/gamePhys/phys/rendinstDestr.cpp)
int maxRiGenPerCell = 0x10000, maxRiExPerCell = 0x10000;
bool tmInst12x32bit = false;

bool rendinstClipmapShadows = true;
bool rendinstGlobalShadows = false;
bool rendinstSecondaryLayer = true;
DataBlock ri_lod_ranges_ovr;

void preloadTexturesToBuildRendinstShadows()
{
  if (!is_managed_textures_streaming_load_on_demand())
    return;

  eastl::fixed_vector<TEXTUREID, 64> tmpList;

  uint32_t rmask = (rendinstClipmapShadows ? rendinst::rgRenderMaskCMS : 0) | (rendinstGlobalShadows ? rgRenderMaskDS : 0);
  for (int i = rgLayer.size() - 1, b = 1 << i; i >= 0; i--, b >>= 1)
    if (RendInstGenData *rgl = (b & rmask) ? rgLayer[i] : nullptr)
    {
      auto texIds = rgl->rtData->riImpTexIds;
      if (DAGOR_LIKELY(i == 0 && tmpList.empty()))
      {
        prefetch_and_wait_managed_textures_loaded(texIds);
        return;
      }
      else
        tmpList.insert(tmpList.end(), texIds.begin(), texIds.end());
    }

  if (!tmpList.empty())
    prefetch_and_wait_managed_textures_loaded(tmpList);
}

static void onSettingsChanged(RendInstGenData *rgl, const Point3 &sun_dir_0)
{
  if (!rgl || !rgl->rtData)
    return;

  bool hasClipShadows = false;
  bool hasGlobalShadows = false;
  for (unsigned int poolNo = 0; poolNo < rgl->rtData->rtPoolData.size(); poolNo++)
  {
    if (!rgl->rtData->rtPoolData[poolNo])
      continue;
    rendinst::render::RtPoolData &pool = *rgl->rtData->rtPoolData[poolNo];

    if (pool.rendinstClipmapShadowTex)
      hasClipShadows = true;

    if (pool.rendinstGlobalShadowTex)
      hasGlobalShadows = true;

    if (!rendinstClipmapShadows && pool.rendinstClipmapShadowTex)
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(pool.rendinstClipmapShadowTexId, pool.rendinstClipmapShadowTex);

    if (!rendinstGlobalShadows && pool.rendinstGlobalShadowTex)
      pool.rendinstGlobalShadowTex.close();
  }

  if (rendinstClipmapShadows && !hasClipShadows)
    render::renderRIGenClipmapShadowsToTextures(sun_dir_0, false); // The parameter cannot be changed by user so there is no need to
                                                                   // update SLI.

  if (rendinstGlobalShadows && !hasGlobalShadows)
    render::renderRIGenGlobalShadowsToTextures(sun_dir_0);

  if (rendinstGlobalShadows != hasGlobalShadows)
    rebuildRgRenderMasks();
}

// The only writer of the derived dist muls; call after any distMulInputs change.
static void recomputeDistMuls()
{
  using namespace rendinst::render;
  const DistMulInputs &in = distMulInputs;
  const auto effClamp = [](float v, const DistMulBounds &b) { return clamp(v, b.effectiveMin, b.effectiveMax); };

  riGenLodDistMul = effClamp(in.dynamicMul * in.riGenSettingsMul + in.dynamicOfs, in.riGenBounds);
  riGenCullDistMul = effClamp(in.dynamicMul * max(in.riGenSettingsMul, in.minCullMul) + in.dynamicOfs, in.riGenBounds);

  // riExtra draw distance is floored at the High preset value: lower presets shorten
  // lod switches but not the last lod end
  constexpr float RIEXTRA_MIN_SETTINGS_CULL_MUL = 1.f;
  float exLodMul = in.riExtraScaled ? effClamp(in.dynamicMul * in.riExtraSettingsMul + in.dynamicOfs, in.riExtraBounds) : 1.f;
  float exCullMul =
    in.riExtraScaled
      ? effClamp(in.dynamicMul * max(in.riExtraSettingsMul, RIEXTRA_MIN_SETTINGS_CULL_MUL) + in.dynamicOfs, in.riExtraBounds)
      : 1.f;
  // the traversal scale keeps the riGen cull formula: texture streaming distances and
  // access queries use it without the compare muls
  float exBaseMul =
    in.riExtraScaled ? effClamp(in.dynamicMul * max(in.riExtraSettingsMul, in.minCullMul) + in.dynamicOfs, in.riExtraBounds) : 1.f;
  rendinst::riExtraDistSqScale = 1.f / (exBaseMul * exBaseMul);
  // lodsShiftMul and riExtraAddLodMul scale a squared distance once: sqrt strength on riExtra
  float lodRatio = exBaseMul / exLodMul, cullRatio = exBaseMul / exCullMul;
  riExtraLodCompareMul = in.lodsShiftMul * in.riExtraAddLodMul * lodRatio * lodRatio;
  riExtraCullCompareMul = in.lodsShiftMul * in.riExtraAddLodMul * cullRatio * cullRatio;

  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      rgl->rtData->setDistMul(clamp(riGenLodDistMul, rgAttr[_layer].minDistMul, rgAttr[_layer].maxDistMul),
        clamp(riGenCullDistMul, rgAttr[_layer].minDistMul, rgAttr[_layer].maxDistMul), true, true, false, in.impostorsFarDistAddMul);
}

void setDynamicDistMul(float mul, float ofs, float impostors_far_dist_additional_mul)
{
  rendinst::render::distMulInputs.dynamicMul = mul;
  rendinst::render::distMulInputs.dynamicOfs = ofs;
  rendinst::render::distMulInputs.impostorsFarDistAddMul = impostors_far_dist_additional_mul;
  recomputeDistMuls();
}

// level init: the dynamic scale resets to identity, the other inputs stay
void resetDynamicDistMul()
{
  rendinst::render::distMulInputs.dynamicMul = 1.f;
  rendinst::render::distMulInputs.dynamicOfs = 0.f;
  recomputeDistMuls();
}

void setPerFrameDistMuls(float mul, float ofs, float min_cull_mul, float lods_shift_mul, float impostors_far_dist_additional_mul)
{
  auto &in = rendinst::render::distMulInputs;
  in.dynamicMul = mul;
  in.dynamicOfs = ofs;
  in.minCullMul = min_cull_mul;
  in.lodsShiftMul = lods_shift_mul;
  in.impostorsFarDistAddMul = impostors_far_dist_additional_mul;
  recomputeDistMuls();
}

void updateSettingsDistMul()
{
  const DataBlock *graphicsSettings = ::dgs_get_settings()->getBlockByNameEx("graphics");
  float distMul = graphicsSettings->getReal("rendinstDistMul", 1.f);
  updateSettingsRiGenDistMul(distMul);
  // riExtra follows rendinstDistMul when not configured separately
  updateSettingsRiExtraDistMul(graphicsSettings->getReal("rendinstExDistMul", distMul));
}

void updateSettingsDistMul(float v)
{
  updateSettingsRiGenDistMul(v);
  updateSettingsRiExtraDistMul(v);
}

void updateSettingsRiGenDistMul(float v)
{
  auto &in = rendinst::render::distMulInputs;
  in.riGenSettingsMul = clamp(v, in.riGenBounds.settingsMin, in.riGenBounds.settingsMax);
  recomputeDistMuls();
}

void updateSettingsRiExtraDistMul(float v)
{
  auto &in = rendinst::render::distMulInputs;
  in.riExtraSettingsMul = clamp(v, in.riExtraBounds.settingsMin, in.riExtraBounds.settingsMax);
  recomputeDistMuls();
}

void setSettingsDistMulMin(float v)
{
  auto &in = rendinst::render::distMulInputs;
  in.riGenBounds.settingsMin = min(v, in.riGenBounds.settingsMax);
  in.riExtraBounds.settingsMin = min(v, in.riExtraBounds.settingsMax);
  recomputeDistMuls();
}

void setEffectiveDistMulMin(float v)
{
  auto &in = rendinst::render::distMulInputs;
  in.riGenBounds.effectiveMin = min(v, in.riGenBounds.effectiveMax);
  in.riExtraBounds.effectiveMin = min(v, in.riExtraBounds.effectiveMax);
  recomputeDistMuls();
}

void setRiGenDistMulMax(float v)
{
  auto &b = rendinst::render::distMulInputs.riGenBounds;
  b.settingsMax = max(v, b.settingsMin);
  b.effectiveMax = max(v, b.effectiveMin);
  recomputeDistMuls();
}

void setRiExtraDistMulMax(float v)
{
  auto &b = rendinst::render::distMulInputs.riExtraBounds;
  b.settingsMax = max(v, b.settingsMin);
  b.effectiveMax = max(v, b.effectiveMin);
  recomputeDistMuls();
}

float getSettingsDistMul() { return rendinst::render::distMulInputs.riGenSettingsMul; }

float getSettingsRiExtraDistMul() { return rendinst::render::distMulInputs.riExtraSettingsMul; }

void updateRIExtraMulScale()
{
  rendinst::render::distMulInputs.riExtraScaled =
    ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("riExtraMulScale", 1.0f) > 0.f;
  recomputeDistMuls();
}

void setLodsShiftDistMul()
{
  float value = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("lodsShiftDistMul", 1.f);
  float clampedValue = clamp(value, 1.f, 1.3f);
  if (value != clampedValue)
    logerr("lodsShiftDistMul value (%f) clamped to %f", value, clampedValue);
  setLodsShiftDistMul(clampedValue);
}

void setLodsShiftDistMul(float mul)
{
  rendinst::render::distMulInputs.lodsShiftMul = mul;
  recomputeDistMuls();
}

void setAdditionalRiExtraLodDistMul(float mul)
{
  rendinst::render::distMulInputs.riExtraAddLodMul = mul;
  recomputeDistMuls();
}

void setImpostorsDistAddMul(float impostors_dist_additional_mul)
{
  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      rgl->rtData->setImpostorsDistAddMul(impostors_dist_additional_mul);
}

void setImpostorsFarDistAddMul(float impostors_far_dist_additional_mul)
{
  rendinst::render::distMulInputs.impostorsFarDistAddMul = impostors_far_dist_additional_mul;
  recomputeDistMuls();
}

void setImpostorsMinRange(float impostors_min_range)
{
  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      rgl->rtData->setImpostorsMinRange(impostors_min_range);
}

void onSettingsChanged(const Point3 &sun_dir_0)
{
  updateSettingsDistMul();
  updateRIExtraMulScale();
  setLodsShiftDistMul();
  FOR_EACH_RG_LAYER_DO (rgl)
    onSettingsChanged(rgl, sun_dir_0);
}

Point3 preloadPointForRendInsts(0, 0, 0);
int preloadPointForRendInstsExpirationTimeMs = -1;
void setPreloadPointForRendInsts(const Point3 &pointAt)
{
  preloadPointForRendInsts = pointAt;
  preloadPointForRendInstsExpirationTimeMs = ::get_time_msec() + 5000;
}

bool DestroyedPoolData::isInRange(uint32_t offs) const
{
  for (int i = 0; i < destroyedInstances.size(); ++i)
    if (destroyedInstances[i].endOffs > offs)
      return offs >= destroyedInstances[i].startOffs;
  return false;
}

const DestroyedPoolData *DestroyedCellData::getPool(uint16_t pool_idx) const
{
  for (int i = 0; i < destroyedPoolInfo.size(); ++i)
    if (destroyedPoolInfo[i].poolIdx == pool_idx)
      return &destroyedPoolInfo[i];
  return nullptr;
}

void debug_print_destrs(const Tab<rendinst::DestroyedCellData> &cellsNewDestrInfo)
{
  for (int i = 0; i < cellsNewDestrInfo.size(); ++i)
  {
    const rendinst::DestroyedCellData &cell = cellsNewDestrInfo[i];
    if (cell.destroyedPoolInfo.empty())
      continue;
    logerr("  cell.cellId %i", cell.cellId);
    for (int j = 0; j < cell.destroyedPoolInfo.size(); ++j)
    {
      const rendinst::DestroyedPoolData &pool = cell.destroyedPoolInfo[j];
      if (pool.destroyedInstances.empty())
        continue;
      logerr("    pool.poolIdx %i", pool.poolIdx);
      int stride = rendinst::getRIGenStride(0, cell.cellId, pool.poolIdx);
      for (int k = 0; k < pool.destroyedInstances.size(); ++k)
      {
        const rendinst::DestroyedInstanceRange &range = pool.destroyedInstances[k];
        logerr("      range %u elems %u", range.startOffs, (range.endOffs - range.startOffs) / stride);
      }
    }
  }
}
} // namespace rendinst

RendInstGenData *RendInstGenData::getGenDataByLayer(const rendinst::RendInstDesc &desc) { return rendinst::getRgLayer(desc.layer); }
