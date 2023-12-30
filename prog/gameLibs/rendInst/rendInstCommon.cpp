#include <rendInst/rendInstGen.h>
#include "riGen/riGenData.h"
#include "render/genRender.h"

#include <3d/dag_drv3d.h>
#include <osApiWrappers/dag_miscApi.h>
#include <EASTL/fixed_vector.h>
#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>


namespace rendinst
{

static float unitDistMul = 1.0f;
static float unitDistOfs = 0.0f;

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
      if (EASTL_LIKELY(i == 0 && tmpList.empty()))
        return prefetch_and_wait_managed_textures_loaded(texIds);
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
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(pool.rendinstGlobalShadowTexId, pool.rendinstGlobalShadowTex);
  }

  if (rendinstClipmapShadows && !hasClipShadows)
    render::renderRIGenClipmapShadowsToTextures(sun_dir_0, false); // The parameter cannot be changed by user so there is no need to
                                                                   // update SLI.

  if (rendinstGlobalShadows && !hasGlobalShadows)
    render::renderRIGenGlobalShadowsToTextures(sun_dir_0);

  if (rendinstGlobalShadows != hasGlobalShadows)
    rebuildRgRenderMasks();
}

void setRiExtraDistMul(float mul)
{
  rendinst::riExtraLodDistSqMul = 1.0f / ((rendinst::render::riExtraMulScale > 0.0f ? rendinst::render::globalDistMul : 1.f) * mul);
  rendinst::riExtraCullDistSqMul =
    1.0f / ((rendinst::render::riExtraMulScale > 0.0f ? rendinst::render::globalCullDistMul : 1.f) * mul);

  rendinst::riExtraLodDistSqMul *= rendinst::riExtraLodDistSqMul;
  rendinst::riExtraCullDistSqMul *= rendinst::riExtraCullDistSqMul;

  rendinst::render::riExtraLodsShiftDistMul = rendinst::render::lodsShiftDistMul * rendinst::render::additionalRiExtraLodDistMul *
                                              rendinst::riExtraLodDistSqMul / rendinst::riExtraCullDistSqMul;
}

void setDistMul(float distMul, float distOfs, bool force_impostors_and_mul, float impostors_far_dist_additional_mul) // 0.2353, 0.0824
                                                                                                                     // will remap 0.5
                                                                                                                     // .. 2.2 to 0.2
                                                                                                                     // .. 0.6
{
  unitDistMul = distMul;
  unitDistOfs = distOfs;
  rendinst::render::globalDistMul = clamp(unitDistMul * rendinst::render::settingsDistMul + unitDistOfs,
    MIN_EFFECTIVE_RENDINST_DIST_MUL, MAX_EFFECTIVE_RENDINST_DIST_MUL);

  rendinst::render::globalCullDistMul =
    clamp(unitDistMul * max(rendinst::render::settingsDistMul, rendinst::render::settingsMinCullDistMul) + unitDistOfs,
      MIN_EFFECTIVE_RENDINST_DIST_MUL, MAX_EFFECTIVE_RENDINST_DIST_MUL);

  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      rgl->rtData->setDistMul(clamp(rendinst::render::globalDistMul, rgAttr[_layer].minDistMul, rgAttr[_layer].maxDistMul),
        clamp(rendinst::render::globalCullDistMul, rgAttr[_layer].minDistMul, rgAttr[_layer].maxDistMul), true, true,
        force_impostors_and_mul, impostors_far_dist_additional_mul);

  setRiExtraDistMul(1.f);

  rendinst::render::forceImpostors = force_impostors_and_mul;
}

void updateSettingsDistMul()
{
  updateSettingsDistMul(::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("rendinstDistMul", 1.f));
}

void updateSettingsDistMul(float v)
{
  rendinst::render::settingsDistMul = clamp(v, MIN_SETTINGS_RENDINST_DIST_MUL, MAX_SETTINGS_RENDINST_DIST_MUL);
  setDistMul(unitDistMul, unitDistOfs);
}

float getSettingsDistMul() { return rendinst::render::settingsDistMul; }

void updateMinCullSettingsDistMul(float v) { rendinst::render::settingsMinCullDistMul = v; }

void updateRIExtraMulScale()
{
  rendinst::render::riExtraMulScale = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("riExtraMulScale", 1.0f);
  setDistMul(unitDistMul, unitDistOfs);
}

static float defaultLodsShiftDistMul = 1.0;
void setLodsShiftDistMult()
{
  rendinst::render::lodsShiftDistMul = defaultLodsShiftDistMul =
    clamp(::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("lodsShiftDistMul", 1.f), 1.f, 1.3f);
}

void setLodsShiftDistMult(float mul) { rendinst::render::lodsShiftDistMul = mul; }
void setAdditionalRiExtraLodDistMul(float mul) { rendinst::render::additionalRiExtraLodDistMul = mul; }

void resetLodsShiftDistMult() { rendinst::render::lodsShiftDistMul = defaultLodsShiftDistMul; }

void setZoomDistMul(float mul, bool scale_lod1, float impostors_far_dist_additional_mul)
{
  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      rgl->rtData->setDistMul(clamp(rendinst::render::globalDistMul, rgAttr[_layer].minDistMul, rgAttr[_layer].maxDistMul) * mul,
        clamp(rendinst::render::globalCullDistMul, rgAttr[_layer].minDistMul, rgAttr[_layer].maxDistMul) * mul, scale_lod1, false,
        false, impostors_far_dist_additional_mul);

  setRiExtraDistMul(mul);
}

void setImpostorsDistAddMul(float impostors_dist_additional_mul)
{
  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      rgl->rtData->setImpostorsDistAddMul(impostors_dist_additional_mul);
}

void setImpostorsFarDistAddMul(float impostors_far_dist_additional_mul)
{
  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      rgl->rtData->setImpostorsFarDistAddMul(impostors_far_dist_additional_mul);
}

void onSettingsChanged(const Point3 &sun_dir_0)
{
  updateSettingsDistMul();
  updateRIExtraMulScale();
  setLodsShiftDistMult();
  setDistMul(unitDistMul, unitDistOfs);
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
} // namespace rendinst

RendInstGenData *RendInstGenData::getGenDataByLayer(const rendinst::RendInstDesc &desc) { return rendinst::getRgLayer(desc.layer); }
