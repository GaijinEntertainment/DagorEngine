// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFx/dafx.h>
#include <fx/dag_baseFxClasses.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <3d/dag_texMgr.h>
#include <shaders/dag_shaders.h>
#include <math/random/dag_random.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/hash_map.h>
#include <EASTL/fixed_vector.h>
#include "dafxCompound_decl.h"
#include "dafxSystemDesc.h"
#include "dafxQuality.h"
#include <math/dag_TMatrix4.h>
#include "modfx/modfx_flags_decl.hlsli"
#include "dafxSparkModules/dafx_spark_flags_decl.hlsli"

static const int g_last_ref_slot = 16;
static dafx::ContextId g_dafx_ctx;

enum
{
  HUID_ACES_RESET = 0xA781BF24u
};

enum
{
  RGROUP_DEFAULT = 0,
  RGROUP_LOWRES = 1,
  RGROUP_HIGHRES = 2,
  RGROUP_DISTORTION = 3,
  RGROUP_WATER_PROJ_ADVANCED = 4,
  RGROUP_WATER_PROJ = 5,
  RGROUP_UNDERWATER = 6,
};

enum
{
  PLACEMENT_DEFAULT = 0,
  PLACEMENT_SPHERE = 1,
  PLACEMENT_SPHERE_SECTOR = 2,
  PLACEMENT_CYLINDER = 3,
};

extern void dafx_report_broken_res(int game_res_id, const char *fx_type);

void gather_sub_textures(const dafx::SystemDesc &desc, eastl::vector<TEXTUREID> &out)
{
  auto fetch = [](auto &list, auto &o) {
    for (auto [tid, a] : list)
      if (tid != BAD_TEXTUREID)
        o.push_back(tid);
  };

  fetch(desc.texturesVs, out);
  fetch(desc.texturesPs, out);
  fetch(desc.texturesCs, out);

  for (const dafx::SystemDesc &sub : desc.subsystems)
    gather_sub_textures(sub, out);
}

__forceinline void _rnd_fvec4(int &seed, float &x, float &y, float &z, float &w)
{
  x = _frnd(seed), y = _frnd(seed), z = _frnd(seed), w = _frnd(seed);
}

static void sphere_placement(int &seed, CompositePlacement const &placement, Point3 &pos, Point3 &dir)
{
  float latVal, longVal, xVol, yVol, zVol;
  _rnd_fvec4(seed, longVal, xVol, yVol, zVol);
  int seed2 = _rnd(seed);
  latVal = _frnd(seed2);

  float latitude = acos(2.0 * latVal - 1.0) - M_PI / 2.0;
  float longitude = 2.0 * M_PI * longVal;

  pos.x = placement.placement_radius * (placement.place_in_volume ? xVol : 1.0f) * cos(latitude) * cos(longitude);
  pos.y = placement.placement_radius * (placement.place_in_volume ? yVol : 1.0f) * cos(latitude) * sin(longitude);
  pos.z = placement.placement_radius * (placement.place_in_volume ? zVol : 1.0f) * sin(latitude);

  dir = pos;
  dir.normalize();
}

static void cylinder_placement(int &seed, CompositePlacement const &placement, Point3 &pos, Point3 &dir)
{
  float randDegree, xVol, yVol, zVol;
  _rnd_fvec4(seed, randDegree, xVol, yVol, zVol);

  float theta = 2.0 * M_PI * randDegree;

  pos.x = placement.placement_radius * (placement.cylinder.use_whole_surface ? xVol : 1.0f) * cos(theta);
  pos.y = placement.cylinder.placement_height * (placement.place_in_volume ? yVol : 1.0f);
  pos.z = placement.placement_radius * (placement.cylinder.use_whole_surface ? zVol : 1.0f) * sin(theta);

  dir = Point3(0, 1, 0);
}

static void sphere_sector_placement(int &seed, CompositePlacement const &placement, Point3 &pos, Point3 &dir)
{
  float randDegree1, randDegree2, xVol, yVol, zVol;
  _rnd_fvec4(seed, randDegree1, xVol, yVol, zVol);
  int seed2 = _rnd(seed);
  randDegree2 = _frnd(seed2);

  float sectorAngle = placement.sphere_sector.sector_angle * M_PI / 180.0f;
  float phi = M_PI / 2 - sectorAngle * (randDegree1 - 0.5f);
  float theta = randDegree2 * M_PI * 2;

  pos.x = placement.placement_radius * (placement.place_in_volume ? xVol : 1.0f) * cos(theta) * cos(phi);
  pos.y = placement.placement_radius * (placement.place_in_volume ? yVol : 1.0f) * sin(phi);
  pos.z = placement.placement_radius * (placement.place_in_volume ? zVol : 1.0f) * sin(theta) * cos(phi);

  dir = normalize(pos);
}

struct SubEffect
{
  struct ValueOffset
  {
    int fxTm;
    int parentVelocity;
    int localGravity;
    int gravityTm;
    int lightPos;
    int lightColor;
    int lightRadius;
    int colorMult;
    int flags;
    int velocityScaleMin;
    int velocityScaleMax;
    int radiusMin;
    int radiusMax;
    int windCoeff;
    int fakeBrightnessBackgroundPos;
    int splinePosData;
  };

  ValueOffset offsets;
  Point2 fxRadiusRange;
  TMatrix subTransform;
  dafx_ex::TransformType transformType;
  float transformScalingFactor;
  float currentCalcdDistanceScale;
  dafx_ex::SystemInfo::DistanceScale distanceScale;
#if DAFX_USE_GRAVITY_ZONE
  Point3 gravityUp;
  Matrix3 rotateToLocalSpace;
#endif
  uint32_t rflags;
  uint32_t sflags;
};

struct DafxCompound : BaseParticleEffect
{
  struct DistanceLag
  {
    constexpr static int historySize = 10;
    bool valid = false;
    bool enabled = false;
    float stepSq = 0.f;
    float distance = 0.f;
    float distanceSq = 0.f;
    int historyIdx = 0;
    eastl::array<Point3, historySize> history;
  };

  struct PlacementData
  {
    Point3 offset = Point3::ZERO;
    Point3 scale = Point3::ONE;
    Point3 rotation = Point3::ZERO;
  };

  struct SharedData
  {
    dafx::SystemDesc pdesc;
    eastl::vector<TEXTUREID> textures;
    eastl::vector<PlacementData> placementData; // optional for fx, so no fixed_vector
    TMatrix lightTm = TMatrix::IDENT;
    CompositePlacement compositePlacement = {};
    int maxInstances = 0;
    int playerReserved = 0;
    float onePointNumber = 0;
    float onePointRadius = 0;
    int gameResId = 0;
    bool lightOnly = false;

    ~SharedData()
    {
      for (TEXTUREID tid : textures)
        release_managed_tex(tid);
    }
  };

  dafx::ContextId ctx;
  dafx::SystemId sid;
  dafx::InstanceId iid;
  eastl::shared_ptr<SharedData const> shared;
  eastl::fixed_vector<SubEffect, 8, true> subEffects;
  eastl::vector<DistanceLag> distanceLagData; // optional for fx, so no fixed_vector

  float fxScale = 1.f;
  float distanceToCamOnSpawn = 0;
  Point2 fxScaleRange = Point2(1.f, 1.f);
  BaseEffectObject *lightFx = nullptr;

  bool hasModFxSubEffects = false; // for an optimization, to know when to compute scale factor

  bool resLoaded = false;
  int rndSeed = 0; // It will get its value during loadParamsDataInternal

  DafxCompound() {}

  DafxCompound(const DafxCompound &r) :
    sid(r.sid),
    iid(r.iid),
    ctx(r.ctx),
    subEffects(r.subEffects),
    lightFx(r.lightFx ? (BaseEffectObject *)r.lightFx->clone() : nullptr),
    distanceLagData(r.distanceLagData),
    hasModFxSubEffects(r.hasModFxSubEffects),
    resLoaded(r.resLoaded),
    shared(r.shared),
    fxScale(r.fxScale),
    fxScaleRange(r.fxScaleRange)
  {
    G_ASSERT_RETURN(shared, );
    rndSeed = generate_seed(uintptr_t(&shared->pdesc), shared->gameResId);
    if (shared->compositePlacement.enabled && shared->compositePlacement.copies_number > 0)
      regeneratePosAndOrientation();
  }

  ~DafxCompound()
  {
    if (iid && g_dafx_ctx)
      dafx::destroy_instance(g_dafx_ctx, iid);

    if (lightFx)
      delete lightFx;
  }

  int generate_seed(uint32_t val1, uint32_t val2)
  {
    int combined_val = val1 ^ val2;
    return _rnd(combined_val) ^ rndSeed;
  }

  void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb) override
  {
    int gameResId = load_cb->getSelfGameResId();
    resLoaded = false;
    if (loadParamsDataInternal(ptr, len, gameResId, load_cb))
      resLoaded = true;
    else
      dafx_report_broken_res(gameResId, "dafx_compound");
  }

  bool loadParamsDataInternal(const char *ptr, int len, int game_res_id, BaseParamScriptLoadCB *load_cb)
  {
    CHECK_FX_VERSION_OPT(ptr, len, 3);

    if (!g_dafx_ctx)
    {
      logwarn("fx: compound: failed to load params data, context was not initialized");
      return false;
    }

    auto builtSharedData = eastl::make_shared<SharedData>();

    builtSharedData->pdesc.gameResId = builtSharedData->gameResId = game_res_id;
    subEffects.clear();
    distanceLagData.clear();

    CompModfx modfxList;
    if (!modfxList.load(ptr, len, load_cb))
      return false;

    LightfxParams parLight;
    if (!parLight.load(ptr, len, load_cb))
      return false;

    CFxGlobalParams parGlobals;
    if (!parGlobals.load(ptr, len, load_cb))
      return false;

    builtSharedData->maxInstances = parGlobals.max_instances;
    builtSharedData->playerReserved = parGlobals.player_reserved;
    builtSharedData->pdesc.emitterData.spawnRangeLimit = -1;
    builtSharedData->onePointNumber = parGlobals.one_point_number;
    builtSharedData->onePointRadius = parGlobals.one_point_radius;

    if (modfxList.instance_life_time_min > 0 || modfxList.instance_life_time_max > 0)
    {
      if (modfxList.instance_life_time_min > 0 && modfxList.instance_life_time_max > 0)
      {
        builtSharedData->pdesc.emitterData.globalLifeLimitMin = max(modfxList.instance_life_time_min, 0.f);
        builtSharedData->pdesc.emitterData.globalLifeLimitMax =
          max(modfxList.instance_life_time_max, modfxList.instance_life_time_min);
      }
      else
        logerr("dafx: instance_life_time_min/max should either be 0 or both be > 0");
    }

    void **refFirst = &modfxList.fx1;

    // light
    lightFx = nullptr;
    void *ref = (parLight.ref_slot >= 1 && parLight.ref_slot <= g_last_ref_slot) ? refFirst[parLight.ref_slot - 1] : nullptr;
    if (ref)
    {
      BaseEffectObject *fx = static_cast<BaseEffectObject *>(ref);

      Color4 value;
      if (fx->getParam(HUID_LIGHT_PARAMS, &value))
      {
        if ((modfxList.instance_life_time_min + modfxList.instance_life_time_max) > 0)
          parLight.life_time = (parLight.life_time > 0) //
                                 ? min(parLight.life_time, modfxList.instance_life_time_min)
                                 : modfxList.instance_life_time_min;

        parLight.mod_scale = max(parLight.mod_scale, 0.f);
        parLight.mod_radius = max(parLight.mod_radius, 0.f);

        lightFx = (BaseEffectObject *)fx->clone();
        lightFx->setParam(_MAKE4C('LFXS'), &parLight.mod_scale);
        lightFx->setParam(_MAKE4C('LFXR'), &parLight.mod_radius);
        lightFx->setParam(_MAKE4C('LFXL'), &parLight.life_time);
        lightFx->setParam(_MAKE4C('LFXF'), &parLight.fade_time);
        lightFx->setParam(_MAKE4C('LFXO'), &parLight.allow_game_override);
        if (parLight.override_shadow)
          lightFx->setParam(_MAKE4C('FXSH'), &parLight.shadow);
        builtSharedData->lightTm = TMatrix::IDENT;
        builtSharedData->lightTm.setcol(3, parLight.offset);
      }
    }

    int subIdx = 0;

    fxScaleRange.x = modfxList.fx_scale_min;
    fxScaleRange.y = modfxList.fx_scale_max;

    auto placement_func = sphere_placement;
    switch (parGlobals.procedural_placement.placement_type)
    {
      default:
      case PLACEMENT_SPHERE: placement_func = sphere_placement; break;
      case PLACEMENT_SPHERE_SECTOR: placement_func = sphere_sector_placement; break;
      case PLACEMENT_CYLINDER: placement_func = cylinder_placement; break;
    }

    dag::Vector<dafx::SystemDesc *> loaded_subsystems;

    Point3 pos = Point3::ZERO;
    Point3 dir = Point3(0, 1, 0);
    rndSeed = generate_seed(uintptr_t(this), builtSharedData->gameResId);
    placement_func(rndSeed, parGlobals.procedural_placement, pos, dir);

    // Load all subeffects for the compound fx
    for (int i = 0; i < modfxList.array.size(); ++i)
    {
      const ModfxParams &par = modfxList.array[i];
      if (par.ref_slot < 1 || par.ref_slot > g_last_ref_slot)
        continue;

      auto pData = builtSharedData->placementData.push_back();
      pData.offset = par.offset;
      pData.scale = par.scale;
      pData.rotation = par.rotation;

      void *ref = refFirst[par.ref_slot - 1];
      if (!ref)
      {
        debug("dafx compound: invalid sub fx reference: %d, skipping", i);
        continue;
      }

      BaseEffectObject *fx = static_cast<BaseEffectObject *>(ref);

      dafx::SystemDesc *sdesc = nullptr; // will be null on failed subfx load
      if (!fx->getParam(_MAKE4C('PFXQ'), &sdesc) || !sdesc)
        continue;

      loadSubEffect(*builtSharedData, subIdx, par, fx, sdesc, pos, dir, parGlobals.procedural_placement.enabled,
        parGlobals.spawn_range_limit);
      loaded_subsystems.push_back(sdesc);
    }

    builtSharedData->compositePlacement = parGlobals.procedural_placement;
    if (builtSharedData->pdesc.emitterData.spawnRangeLimit < 0)
      builtSharedData->pdesc.emitterData.spawnRangeLimit = 0;

    // Apply procedural placement
    if (builtSharedData->compositePlacement.enabled)
    {
      // Less than copies_number because the first one is already loaded
      G_ASSERT(builtSharedData->compositePlacement.copies_number > 0);
      for (int i = 0; i < builtSharedData->compositePlacement.copies_number - 1; i++)
      {
        placement_func(rndSeed, builtSharedData->compositePlacement, pos, dir);

        addSubEffects(*builtSharedData, subIdx, loaded_subsystems, modfxList.array, pos, dir);
      }
    }

    if (builtSharedData->pdesc.subsystems.empty())
    {
      if (!lightFx)
      {
        logerr("dafx compound fx is empty");
        builtSharedData = nullptr;
      }
      else
      {
        builtSharedData->lightOnly = true;
      }
    }
    else
    {
      gather_sub_textures(builtSharedData->pdesc, builtSharedData->textures);
      for (TEXTUREID tid : builtSharedData->textures)
        acquire_managed_tex(tid);

      builtSharedData->pdesc.emitterData.type = dafx::EmitterType::FIXED;
      builtSharedData->pdesc.emitterData.fixedData.count = 0;

      builtSharedData->pdesc.emissionData.type = dafx::EmissionType::NONE;
      builtSharedData->pdesc.simulationData.type = dafx::SimulationType::NONE;
    }

    release_game_resource_ex(modfxList.fx1, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx2, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx3, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx4, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx5, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx6, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx7, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx8, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx9, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx10, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx11, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx12, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx13, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx14, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx15, EffectGameResClassId);
    release_game_resource_ex(modfxList.fx16, EffectGameResClassId);

    shared = eastl::move(builtSharedData);

    return true;
  }

  void regeneratePosAndOrientation()
  {
    bool isSubEffectSizeCompatible = (subEffects.size() % shared->placementData.size() == 0) &&
                                     (subEffects.size() / shared->placementData.size() == shared->compositePlacement.copies_number);
    G_ASSERTF_RETURN(isSubEffectSizeCompatible, ,
      "dafx compound: Composite placement is enabled, but the number of subeffects (%d) is not compatible with the number of "
      "placement data entries (%d) or copies number (%d).",
      subEffects.size(), shared->placementData.size(), shared->compositePlacement.copies_number);

    auto placement_func = sphere_placement;
    switch (shared->compositePlacement.placement_type)
    {
      default:
      case PLACEMENT_SPHERE: placement_func = sphere_placement; break;
      case PLACEMENT_SPHERE_SECTOR: placement_func = sphere_sector_placement; break;
      case PLACEMENT_CYLINDER: placement_func = cylinder_placement; break;
    }

    int uniqueSubEffectsNr = subEffects.size() / shared->compositePlacement.copies_number;

    for (int i = 0; i < shared->compositePlacement.copies_number; i++)
    {
      Point3 pos = Point3::ZERO;
      Point3 dir = Point3(0, 1, 0);
      placement_func(rndSeed, shared->compositePlacement, pos, dir);
      for (int j = 0; j < uniqueSubEffectsNr; j++)
      {
        int idx = i * uniqueSubEffectsNr + j;
        SubEffect &subEffect = subEffects[idx];
        auto const &pData = shared->placementData[j];
        subEffect.subTransform = calculateSubEffectTransform(pData.offset, pData.scale * fxScale * subEffect.currentCalcdDistanceScale,
          pData.rotation, pos, dir, true);
        subEffect.transformScalingFactor = subEffect.subTransform.getScalingFactor();
      }
    }
  }

  TMatrix calculateSubEffectTransform(const Point3 &offset, const Point3 &scale, const Point3 &rotation, const Point3 &pos,
    const Point3 &dir, bool usePosDir = true)
  {
    TMatrix scl = TMatrix::IDENT;
    scl.m[0][0] = scale.x;
    scl.m[1][1] = scale.y;
    scl.m[2][2] = scale.z;

    TMatrix wtm;
    TMatrix placementTm = TMatrix::IDENT;
    Point3 localOffset = offset;

    if (usePosDir)
    {
      Point3 up = dir;
      Point3 right = abs(up.y) > 0.99f ? normalize(cross(up, Point3(1, 0, 0))) : normalize(cross(up, Point3(0, 1, 0)));
      Point3 forward = normalize(cross(right, up));
      placementTm.setcol(0, right);
      placementTm.setcol(1, up);
      placementTm.setcol(2, forward);

      localOffset = localOffset.x * right + localOffset.y * up + localOffset.z * forward + pos;
    }

    wtm = placementTm * rotxTM(rotation.x * DEG_TO_RAD) * rotyTM(rotation.y * DEG_TO_RAD) * rotzTM(rotation.z * DEG_TO_RAD) * scl;
    wtm.setcol(3, localOffset);

    return wtm;
  }

  void addSubEffects(SharedData &sd, int &subIdx, dag::Vector<dafx::SystemDesc *> &subsystems, const SmallTab<ModfxParams> &parArray,
    const Point3 &pos, const Point3 &dir)
  {
    for (int i = 0; i < subsystems.size(); i++)
    {
      SubEffect &subEffect = subEffects.push_back();
      subEffect = subEffects[i];

      sd.pdesc.subsystems.push_back(*subsystems[i]);
      const ModfxParams &par = parArray[i];

      auto sdesc = &sd.pdesc.subsystems.back();
      dafx::SystemDesc *ddesc = sdesc->subsystems.empty() ? nullptr : &sdesc->subsystems[0];
      G_ASSERT_CONTINUE(ddesc);

      ddesc->renderSortDepth = subIdx++;

      if (par.distance_lag > 0)
      {
        distanceLagData.resize(subIdx);
        DistanceLag &v = distanceLagData.back();
        v.enabled = true;
        v.distance = par.distance_lag;
        v.distanceSq = v.distance * v.distance;
        v.stepSq = (par.distance_lag / DistanceLag::historySize);
        v.stepSq *= v.stepSq;
      }

      TMatrix wtm =
        calculateSubEffectTransform(par.offset, par.scale * fxScale * subEffect.currentCalcdDistanceScale, par.rotation, pos, dir);
      subEffect.subTransform = wtm;
      subEffect.transformScalingFactor = subEffect.subTransform.getScalingFactor();
    }
  }

  void loadSubEffect(SharedData &sd, int &subIdx, const ModfxParams &par, BaseEffectObject *fx, dafx::SystemDesc *sdesc,
    const Point3 &pos, const Point3 &dir, bool usePosDir, float spawn_range_limit)
  {
    SubEffect &subEffect = subEffects.push_back();
    SubEffect::ValueOffset &offsets = subEffect.offsets;

    sd.pdesc.subsystems.push_back(*sdesc);
    sdesc = &sd.pdesc.subsystems.back();
    sdesc->qualityFlags = fx_apply_quality_bits(par.quality, sdesc->qualityFlags);

    dafx::SystemDesc *ddesc = sdesc->subsystems.empty() ? nullptr : &sdesc->subsystems[0];
    G_ASSERT_RETURN(ddesc, );

    ddesc->renderSortDepth = subIdx++;
    ddesc->emitterData.delay += par.delay;

    if (spawn_range_limit > 0 && ddesc->emitterData.spawnRangeLimit <= 0)
      ddesc->emitterData.spawnRangeLimit = spawn_range_limit;

    if (ddesc->emitterData.spawnRangeLimit == 0)
      sd.pdesc.emitterData.spawnRangeLimit = 0;
    else if (sd.pdesc.emitterData.spawnRangeLimit != 0)
      sd.pdesc.emitterData.spawnRangeLimit = max(sd.pdesc.emitterData.spawnRangeLimit, ddesc->emitterData.spawnRangeLimit);

    if (par.mod_part_count != 1 && par.mod_part_count > 0)
    {
      if (ddesc->emitterData.type == dafx::EmitterType::LINEAR)
      {
        ddesc->emitterData.linearData.countMin =
          ddesc->emitterData.linearData.countMin > 0 ? max(ddesc->emitterData.linearData.countMin * par.mod_part_count, 1.f) : 0;
        ddesc->emitterData.linearData.countMax = max(ddesc->emitterData.linearData.countMax * par.mod_part_count, 1.f);
      }
      else if (ddesc->emitterData.type == dafx::EmitterType::BURST)
      {
        ddesc->emitterData.burstData.countMin =
          ddesc->emitterData.burstData.countMin > 0 ? max(ddesc->emitterData.burstData.countMin * par.mod_part_count, 1.f) : 0;
        ddesc->emitterData.burstData.countMax = max(ddesc->emitterData.burstData.countMax * par.mod_part_count, 1.f);
      }
      else if (ddesc->emitterData.type == dafx::EmitterType::FIXED)
      {
        ddesc->emitterData.fixedData.count = max(ddesc->emitterData.fixedData.count * par.mod_part_count, 1.f);
      }
    }

    if (par.render_group != RGROUP_DEFAULT)
    {
      for (auto &i : ddesc->renderDescs)
      {
        if (strcmp(i.tag.c_str(), dafx_ex::renderTags[dafx_ex::RTAG_LOWRES]) != 0 &&
            strcmp(i.tag.c_str(), dafx_ex::renderTags[dafx_ex::RTAG_HIGHRES]) != 0 &&
            // strcmp(i.tag.c_str(), dafx_ex::renderTags[dafx_ex::RTAG_DISTORTION]) != 0 &&
            // strcmp(i.tag.c_str(), dafx_ex::renderTags[dafx_ex::RTAG_WATER_PROJ]) != 0 &&
            strcmp(i.tag.c_str(), dafx_ex::renderTags[dafx_ex::RTAG_UNDERWATER]) != 0)
          continue;

        int t = dafx_ex::RTAG_LOWRES;
        switch (par.render_group)
        {
          case RGROUP_LOWRES: t = dafx_ex::RTAG_LOWRES; break;
          case RGROUP_HIGHRES: t = dafx_ex::RTAG_HIGHRES; break;
          case RGROUP_DISTORTION: t = dafx_ex::RTAG_DISTORTION; break;
          case RGROUP_WATER_PROJ_ADVANCED: t = dafx_ex::RTAG_WATER_PROJ_ADVANCED; break;
          case RGROUP_WATER_PROJ: t = dafx_ex::RTAG_WATER_PROJ; break;
          case RGROUP_UNDERWATER: t = dafx_ex::RTAG_UNDERWATER; break;
          default: G_ASSERT(false);
        }
        i.tag = dafx_ex::renderTags[t];
      }
    }

    ddesc->emitterData.globalLifeLimitMin = ddesc->emitterData.globalLifeLimitMax = par.global_life_time_min;
    if (par.global_life_time_min > 0 || par.global_life_time_max > 0)
    {
      if (par.global_life_time_min > 0 && par.global_life_time_max > 0)
        ddesc->emitterData.globalLifeLimitMax = par.global_life_time_max;
      else
        logerr("dafx: global_life_time_min/max should either be 0 or both be > 0");
    }

    if (par.mod_life != 1 && par.mod_life > 0)
    {
      // min/max ratio still be the same
      ddesc->emitterData.linearData.lifeLimit *= par.mod_life;
      ddesc->emitterData.burstData.lifeLimit *= par.mod_life;
      ddesc->emitterData.distanceBasedData.lifeLimit *= par.mod_life;
    }

    if (par.distance_lag > 0)
    {
      distanceLagData.resize(subIdx);
      DistanceLag &v = distanceLagData.back();
      v.enabled = true;
      v.distance = par.distance_lag;
      v.distanceSq = v.distance * v.distance;
      v.stepSq = (par.distance_lag / DistanceLag::historySize);
      v.stepSq *= v.stepSq;
    }

    int renderRefDataSize = sdesc->emissionData.renderRefData.size();
    unsigned char *simulationRefData = sdesc->emissionData.simulationRefData.data();

    if (!(float_nonzero(par.scale.x) && float_nonzero(par.scale.y) && float_nonzero(par.scale.z)))
    {
      String resName;
      get_game_resource_name(sd.gameResId, resName);
      logerr("dafx: scale must be non-zero! ('%s' sub-fx slot: %d)", resName.c_str(), subIdx);
    }

    dafx_ex::TransformType trtype = (dafx_ex::TransformType)par.transform_type;
    G_ASSERT(trtype <= dafx_ex::TRANSFORM_LOCAL_SPACE);

    offsets.lightPos = -1;
    offsets.lightRadius = -1;
    offsets.lightColor = -1;
    offsets.parentVelocity = -1;
    offsets.localGravity = -1;
    offsets.gravityTm = -1;
    offsets.fxTm = -1;
    offsets.colorMult = -1;
    offsets.flags = -1;
    offsets.velocityScaleMin = -1;
    offsets.velocityScaleMax = -1;
    offsets.radiusMin = -1;
    offsets.radiusMax = -1;
    offsets.windCoeff = -1;
    offsets.fakeBrightnessBackgroundPos = -1;
    offsets.splinePosData = -1;

    subEffect.currentCalcdDistanceScale = 1.f;

    dafx_ex::SystemInfo *sinfo = nullptr;
    if (fx->getParam(_MAKE4C('PFVR'), &sinfo) && sinfo)
    {
      auto patch_mul_f_v = [=](int n, eastl::vector_map<int, int> &table, unsigned char *dst, float v) {
        eastl::vector_map<int, int>::iterator it = table.find(n);
        if (it != table.end())
        {
          int ofs = it->second;
          if (ofs >= renderRefDataSize)
            ofs -= renderRefDataSize;
          *(float *)(dst + ofs) *= v;
        }
      };

      auto patch_mul_v3_v = [=](int n, eastl::vector_map<int, int> &table, unsigned char *dst, float v) {
        eastl::vector_map<int, int>::iterator it = table.find(n);
        if (it != table.end())
        {
          int ofs = it->second;
          if (ofs >= renderRefDataSize)
            ofs -= renderRefDataSize;
          *(float *)(dst + ofs) *= v;
          *(float *)(dst + ofs + 1 * sizeof(float)) *= v;
          *(float *)(dst + ofs + 2 * sizeof(float)) *= v;
        }
      };

      auto patch_mul_color_v = [=](int n, eastl::vector_map<int, int> &table, unsigned char *dst, E3DCOLOR v) {
        eastl::vector_map<int, int>::iterator it = table.find(n);
        if (it != table.end())
        {
          int ofs = it->second;
          if (ofs >= renderRefDataSize)
            ofs -= renderRefDataSize;

          *(E3DCOLOR *)(dst + ofs) = e3dcolor_mul(*(E3DCOLOR *)(dst + ofs), v);
        }
      };

      float mod_rad = par.mod_radius;
      if (mod_rad != 1 && mod_rad > 0)
      {
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_RADIUS_MIN, sinfo->valueOffsets, simulationRefData, mod_rad);
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_RADIUS_MAX, sinfo->valueOffsets, simulationRefData, mod_rad);
      }

      if (par.mod_rotation_speed != 1)
      {
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_ROTATION_MIN, sinfo->valueOffsets, simulationRefData, par.mod_rotation_speed);
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_ROTATION_MAX, sinfo->valueOffsets, simulationRefData, par.mod_rotation_speed);
      }

      if (par.mod_velocity_start != 1)
      {
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_VELOCITY_START_MIN, sinfo->valueOffsets, simulationRefData, par.mod_velocity_start);
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_VELOCITY_START_MAX, sinfo->valueOffsets, simulationRefData, par.mod_velocity_start);
        patch_mul_v3_v(dafx_ex::SystemInfo::VAL_VELOCITY_START_VEC3, sinfo->valueOffsets, simulationRefData, par.mod_velocity_start);
      }

      if (par.mod_velocity_add != 1)
      {
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_VELOCITY_ADD_MIN, sinfo->valueOffsets, simulationRefData, par.mod_velocity_add);
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_VELOCITY_ADD_MAX, sinfo->valueOffsets, simulationRefData, par.mod_velocity_add);
        patch_mul_v3_v(dafx_ex::SystemInfo::VAL_VELOCITY_ADD_VEC3, sinfo->valueOffsets, simulationRefData, par.mod_velocity_add);
      }

      if (par.mod_velocity_wind_scale != 1)
      {
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_VELOCITY_WIND_COEFF, sinfo->valueOffsets, simulationRefData,
          par.mod_velocity_wind_scale);
      }

      if (par.mod_velocity_mass != 1)
      {
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_MASS, sinfo->valueOffsets, simulationRefData, par.mod_velocity_mass);
      }

      if (par.mod_velocity_drag != 1)
      {
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_DRAG_COEFF, sinfo->valueOffsets, simulationRefData, par.mod_velocity_drag);
      }

      if (par.mod_velocity_drag_to_rad != 1)
      {
        patch_mul_f_v(dafx_ex::SystemInfo::VAL_DRAG_TO_RAD, sinfo->valueOffsets, simulationRefData, par.mod_velocity_drag_to_rad);
      }

      patch_mul_color_v(dafx_ex::SystemInfo::VAL_COLOR_MIN, sinfo->valueOffsets, simulationRefData, par.mod_color);
      patch_mul_color_v(dafx_ex::SystemInfo::VAL_COLOR_MAX, sinfo->valueOffsets, simulationRefData, par.mod_color);

      auto it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_GRAVITY_ZONE_TM);
      if (it != sinfo->valueOffsets.end())
        offsets.gravityTm = it->second;
    }

    // Default values
    subEffect.fxRadiusRange = Point2(1.f, 1.f);
    if (sinfo && trtype == dafx_ex::TRANSFORM_DEFAULT)
      trtype = sinfo->transformType;
    subEffect.transformType = trtype;
    if (sinfo)
    {
      subEffect.distanceScale = sinfo->distanceScale;
      subEffect.currentCalcdDistanceScale = subEffect.distanceScale.calcScale(distanceToCamOnSpawn);
    }
#if DAFX_USE_GRAVITY_ZONE
    subEffect.gravityUp = Point3(0, 1, 0);
    subEffect.rotateToLocalSpace = Matrix3::IDENT;
#endif

    TMatrix wtm = calculateSubEffectTransform(par.offset, par.scale * fxScale * subEffect.currentCalcdDistanceScale, par.rotation, pos,
      dir, usePosDir);
    subEffect.subTransform = wtm;
    subEffect.transformScalingFactor = subEffect.subTransform.getScalingFactor();

    int isModfx = 0;
    if (fx->getParam(_MAKE4C('PFXM'), &isModfx) && isModfx)
    {
      G_ASSERT_RETURN(sinfo, );

      offsets.flags = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_FLAGS];
      offsets.fxTm = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_TM];

      hasModFxSubEffects |= offsets.flags >= 0;

      auto it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_LIGHT_POS);

      if (it != sinfo->valueOffsets.end())
      {
        offsets.lightPos = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_LIGHT_POS];
        offsets.lightColor = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_LIGHT_COLOR];
        offsets.lightRadius = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_LIGHT_RADIUS];
        G_ASSERT(offsets.lightRadius == offsets.lightColor + sizeof(float) * 3); // to allow batch set
      }

      it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_VELOCITY_START_MIN);
      if (it != sinfo->valueOffsets.end())
        offsets.velocityScaleMin = it->second;

      it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_VELOCITY_START_MAX);
      if (it != sinfo->valueOffsets.end())
        offsets.velocityScaleMax = it->second;

      it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_VELOCITY_START_ADD);
      if (it != sinfo->valueOffsets.end())
        offsets.parentVelocity = it->second;

      it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_VELOCITY_WIND_COEFF);
      if (it != sinfo->valueOffsets.end())
        offsets.windCoeff = it->second;

      it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_LOCAL_GRAVITY_VEC);
      if (it != sinfo->valueOffsets.end())
        offsets.localGravity = it->second;

      it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_COLOR_MUL);
      if (it != sinfo->valueOffsets.end())
        offsets.colorMult = it->second;

      it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_FAKE_BRIGHTNESS_BACKGROUND_POS);
      if (it != sinfo->valueOffsets.end())
        offsets.fakeBrightnessBackgroundPos = it->second;

      it = sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_POS_SPLINE_DATA);
      if (it != sinfo->valueOffsets.end())
        offsets.splinePosData = it->second;

      G_ASSERT(sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_RADIUS_MIN) != sinfo->valueOffsets.end());
      G_ASSERT(sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_RADIUS_MAX) != sinfo->valueOffsets.end());
      offsets.radiusMin = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_RADIUS_MIN];
      offsets.radiusMax = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_RADIUS_MAX];

      subEffect.fxRadiusRange.x = *(float *)(simulationRefData + offsets.radiusMin - renderRefDataSize);
      subEffect.fxRadiusRange.y = *(float *)(simulationRefData + offsets.radiusMax - renderRefDataSize);

      uint32_t flg = sinfo->rflags;
      if (lightFx)
        flg |= 1 << MODFX_RFLAG_OMNI_LIGHT_ENABLED;

      *(uint32_t *)(sdesc->emissionData.renderRefData.data() + offsets.flags) = flg;
      subEffect.rflags = flg;
      subEffect.sflags = sinfo->sflags;
    }
    else // sparks
    {
      G_ASSERT_RETURN(sinfo, );

      offsets.fxTm = 0;
      offsets.parentVelocity = sdesc->emissionData.renderRefData.size();
      subEffect.rflags = sinfo->rflags;
      subEffect.sflags = 0;
    }
  }

  void setParam(unsigned id, void *value) override
  {
    if (DAGOR_LIKELY(id != _MAKE4C('PFXR') && id != _MAKE4C('PFXE')) && !iid)
      return;
    switch (id)
    {
      case _MAKE4C('PFXR'):
      {
        // context was recreated and all systems was released
        if (ctx != g_dafx_ctx)
        {
          G_ASSERT(!sid && !iid);
          sid = dafx::SystemId();
          iid = dafx::InstanceId();
          ctx = g_dafx_ctx;

          rndSeed = generate_seed(uintptr_t(this), uint32_t(iid));
        }

        if (resLoaded && (!shared || shared->lightOnly))
          break;

        if (iid)
          dafx::destroy_instance(g_dafx_ctx, iid);

        String resName;
        get_game_resource_name(shared->gameResId, resName);

        eastl::string name;
        name.append_sprintf("%s__%d", resName.c_str(), shared->gameResId);

        bool forceRecreate = value && *(bool *)value;
        if (resLoaded && shared)
          sid = dafx::get_system_by_name(g_dafx_ctx, name, forceRecreate ? nullptr : &shared->pdesc);
        if (sid && forceRecreate)
        {
          dafx::release_system(g_dafx_ctx, sid);
          sid = dafx::SystemId();
        }

        if (!sid)
        {
          sid = resLoaded ? dafx::register_system(g_dafx_ctx, shared->pdesc, name) : dafx::get_dummy_system_id(g_dafx_ctx);
          if (!sid)
            logerr("fx: modfx: failed to register system");
        }

        if (sid && forceRecreate)
        {
          iid = dafx::create_instance(g_dafx_ctx, sid);
          G_ASSERT_RETURN(iid, );
          rndSeed = generate_seed(uintptr_t(this), uint32_t(iid));
          dafx::set_instance_status(g_dafx_ctx, iid, false);
          recalcFxScale();
        }
        break;
      }
      case _MAKE4C('PFXE'):
      {
        if (sid && !iid)
        {
          iid = dafx::create_instance(g_dafx_ctx, sid);
          G_ASSERT_RETURN(iid, );
          recalcFxScale();
        }
        if (iid)
        {
          G_ASSERT(value);
          BaseEffectEnabled *dafxe = static_cast<BaseEffectEnabled *>(value);
          dafx::set_instance_status(g_dafx_ctx, iid, dafxe->enabled, dafxe->distanceToCam);
          distanceToCamOnSpawn = dafxe->distanceToCam;
          updateAllDistanceScales(distanceToCamOnSpawn);
        }
        break;
      }
      case _MAKE4C('PFXP'):
      {
        Point4 pos = *(Point4 *)value;
        dafx::set_instance_pos(g_dafx_ctx, iid, pos);
        break;
      }
      case _MAKE4C('PFXV'): dafx::set_instance_visibility(g_dafx_ctx, iid, value ? *(uint32_t *)value : 0); break;
      case _MAKE4C('PFXI'): ((eastl::vector<dafx::InstanceId> *)value)->push_back(iid); break;
      case HUID_TM: setTm((TMatrix *)value); break;
      case HUID_EMITTER_TM: setEmitterTm((TMatrix *)value); break;
      case HUID_VELOCITY: setVelocity((Point3 *)value); break;
      case HUID_ACES_RESET: reset(); break;
      case HUID_COLOR_MULT: setColorMult((Color3 *)value); break;
      case HUID_COLOR4_MULT: setColor4Mult((Color4 *)value); break;
      case _MAKE4C('PFXG'): dafx::warmup_instance(g_dafx_ctx, iid, value ? *(float *)value : 0); break;
      case _MAKE4C('GZTM'): setGravityTm(*(Matrix3 *)value); break;
      case _MAKE4C('SPLN'): setSplineControlPoints(*(TMatrix4 *)value); break;
      default:
      {
        if (lightFx) // always last.
          lightFx->setParam(id, value);
        break;
      }
    }
  }

  void *getParam(unsigned id, void *value) override
  {
    switch (id)
    {
      case _MAKE4C('CACH'): return (void *)1; //-V566
      case HUID_ACES_IS_ACTIVE: *(bool *)value = iid && dafx::is_instance_renderable_active(g_dafx_ctx, iid); break;
      case _MAKE4C('FXLM'): *(int *)value = shared->maxInstances; break;
      case _MAKE4C('FXLP'): *(int *)value = shared->playerReserved; break;
      case _MAKE4C('FXLR'):
        if (!shared || shared->lightOnly)
          return nullptr;
        *(float *)value = shared->pdesc.emitterData.spawnRangeLimit;
        break;

      case _MAKE4C('FXLX'): *(Point2 *)value = Point2(shared->onePointNumber, shared->onePointRadius); break;
      case _MAKE4C('PFXX'):
      {
        if (!shared || shared->lightOnly)
          return nullptr;

        int parentRen = 0;
        int parentSim = 0;
        int ren = 0;
        int sim = 0;

        for (const dafx::SystemDesc &sub : shared->pdesc.subsystems)
        {
          if (sub.subsystems.empty())
            continue;

          parentRen += sub.renderElemSize;
          parentSim += sub.simulationElemSize;
          ren += sub.subsystems[0].renderElemSize;
          sim += sub.subsystems[0].simulationElemSize;
        }

        ((int *)value)[0] = parentRen;
        ((int *)value)[1] = parentSim;
        ((int *)value)[2] = ren;
        ((int *)value)[3] = sim;
        break;
      }

      case _MAKE4C('PFXT'): *(dafx::SystemId *)value = sid; break;
      case _MAKE4C('PFXC'):
        if (value)
          *(bool *)value = dafx::is_instance_renderable_visible(g_dafx_ctx, iid);
        break;

      case _MAKE4C('FXLD'):
        if (value) // @TODO: remove? this is not used anywhere
          ((eastl::vector<dafx::SystemDesc const *> *)value)->push_back(&shared->pdesc);
        break;

      case _MAKE4C('FXIC'):
        if (value)
          *(int *)value = dafx::get_instance_elem_count(g_dafx_ctx, iid);
        break;

      case _MAKE4C('FXID'):
        if (value)
          *(dafx::InstanceId *)value = iid;
        break;

      default:
        if (lightFx)
          return lightFx->getParam(id, value);
        else
          return nullptr;
    }

    return value;
  }

  void update(float dt) override
  {
    if (lightFx)
    {
      lightFx->update(dt);

      Color4 *params = nullptr;
      params = (Color4 *)lightFx->getParam(HUID_LIGHT_PARAMS, params);
      G_ASSERT_RETURN(params, );

      for (int i = 0; i < subEffects.size(); ++i)
      {
        if (subEffects[i].offsets.lightPos >= 0)
          dafx::set_subinstance_value(g_dafx_ctx, iid, i, subEffects[i].offsets.lightColor, params, sizeof(Point4));
      }
    }
  }

  void render(unsigned, const TMatrix &) override {}

  void spawnParticles(BaseParticleFxEmitter *, real) override {}

  void setColorMult(const Color3 *value) override
  {
    if (iid && value)
    {
      Color4 c(value->r, value->g, value->b, 1.f);
      setColor4Mult(&c);
    }
  }

  void setColor4Mult(const Color4 *value) override
  {
    if (!iid || !value)
      return;

    E3DCOLOR c = e3dcolor(*value);
    for (int i = 0; i < subEffects.size(); ++i)
    {
      int ofs = subEffects[i].offsets.colorMult;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, &c, 4);
    }

    if (lightFx)
      lightFx->setColor4Mult(value);
  }

  void setLocalGravity(int i, const TMatrix *stm = nullptr)
  {
    if (subEffects[i].offsets.localGravity < 0)
      return;

#if DAFX_USE_GRAVITY_ZONE
    if (stm)
      subEffects[i].rotateToLocalSpace = transpose(Matrix3(stm->array));

    Point3 invUp = subEffects[i].rotateToLocalSpace * subEffects[i].gravityUp;
#else
    G_ASSERT(stm);
    Matrix3 itm = transpose(Matrix3(stm->array));
    Point3 invUp = itm.getcol(1);
#endif

    invUp.normalizeF();
    dafx::set_subinstance_value(g_dafx_ctx, iid, i, subEffects[i].offsets.localGravity, &invUp, sizeof(invUp));
  }

  void updateDistanceScale(SubEffect &sub_effect, float new_distance)
  {
    if (!sub_effect.distanceScale.enabled)
      return;

    float prevScale = sub_effect.currentCalcdDistanceScale;
    sub_effect.currentCalcdDistanceScale = sub_effect.distanceScale.calcScale(new_distance);
    if (!is_equal_float(prevScale, sub_effect.currentCalcdDistanceScale))
    {
      float ratio = sub_effect.currentCalcdDistanceScale / prevScale;
      sub_effect.subTransform.col[0] *= ratio;
      sub_effect.subTransform.col[1] *= ratio;
      sub_effect.subTransform.col[2] *= ratio;
      sub_effect.transformScalingFactor *= ratio;
    }
  }

  void updateAllDistanceScales(float new_distance)
  {
    for (auto &subEffect : subEffects)
      updateDistanceScale(subEffect, new_distance);
  }

  void setGravityTm(const Matrix3 &tm)
  {
    G_UNUSED(tm);
#if DAFX_USE_GRAVITY_ZONE
    if (!iid)
      return;

    for (int i = 0; i < subEffects.size(); ++i)
    {
      bool gravZoneEnabled =
        subEffects[i].sflags & ((1 << MODFX_SFLAG_GRAVITY_ZONE_PER_EMITTER) | (1 << MODFX_SFLAG_GRAVITY_ZONE_PER_PARTICLE));
      if (!gravZoneEnabled)
        continue;

      subEffects[i].gravityUp = tm.getcol(1);
      setLocalGravity(i);

      int ofs = subEffects[i].offsets.gravityTm;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, &tm, sizeof(tm));
    }
#endif
  }

  void setSplineControlPoints(const TMatrix4 &data)
  {
    if (!iid)
      return;

    for (int i = 0; i < subEffects.size(); ++i)
    {
      int ofs = subEffects[i].offsets.splinePosData;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, &data, sizeof(data));
    }
  }

  void recalcFxScale()
  {
    rndSeed = generate_seed(uintptr_t(this), uint32_t(iid));
    auto prevScale = fxScale;
    fxScale = lerp(fxScaleRange.x, fxScaleRange.y, _frnd(rndSeed));
    auto scaleRatio = safediv(fxScale, prevScale);
    bool shouldScale = !is_equal_float(scaleRatio, 1.f);

    for (int i = 0; i < subEffects.size(); ++i)
    {
      TMatrix &subtm = subEffects[i].subTransform;
      if (shouldScale)
      {
        subtm.col[0] *= scaleRatio;
        subtm.col[1] *= scaleRatio;
        subtm.col[2] *= scaleRatio;
        subEffects[i].transformScalingFactor *= scaleRatio;
      }

      Point2 radius = subEffects[i].fxRadiusRange * fxScale;
      if (subEffects[i].offsets.radiusMin >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, subEffects[i].offsets.radiusMin, &radius.x, sizeof(float));
      if (subEffects[i].offsets.radiusMax >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, subEffects[i].offsets.radiusMax, &radius.y, sizeof(float));
    }
  }

  void setTransform(const TMatrix &value, dafx_ex::TransformType tr_type)
  {
    float valueScalingFactor = 0.f;
    if (hasModFxSubEffects)
      valueScalingFactor = value.getScalingFactor();

    for (int i = 0; i < subEffects.size(); ++i)
    {
      dafx_ex::TransformType type = subEffects[i].transformType;
      if (type == dafx_ex::TRANSFORM_DEFAULT)
        type = tr_type;

      // fxScale and precalculated distanceScale for the subeffect are already baked into the subTransform
      TMatrix stm = value * subEffects[i].subTransform;

      // distance lag (distance delay) allow to move spawn N meters behind the emitter
      if (i < distanceLagData.size() && distanceLagData[i].enabled)
      {
        DistanceLag &v = distanceLagData[i];
        if (v.valid)
        {
          // we need to accumulate distance, not only direct point dist cmp, in case of path curvature
          const Point3 &emPos = stm.getcol(3);

          float totalDist = 0;
          float lastSegDist = 0;
          Point3 forwardPos = emPos;
          Point3 foundDir = {0, 0, 0};

          Point3 *historyPtr = v.history.data() + v.historyIdx;

          for (int h = 0; h < v.historySize; ++h)
          {
            const Point3 &backPos = *historyPtr--;
            foundDir = backPos - forwardPos;

            lastSegDist = length(foundDir);
            totalDist += lastSegDist;

            if (totalDist >= v.distance)
              break;

            forwardPos = backPos;

            if (h == v.historyIdx)
              historyPtr = v.history.data() + v.historySize - 1;
          }

          // if distance is greater than all of history, do a projection (it will be based on last 2 points in history)
          Point3 foundDirNorm = foundDir * safeinv(lastSegDist);
          Point3 p = forwardPos + foundDirNorm * (v.distance - totalDist + lastSegDist);
          stm.setcol(3, p);

          if (lengthSq(emPos - v.history[v.historyIdx])) // add history point
          {
            ++v.historyIdx;
            if (DAGOR_UNLIKELY(v.historyIdx == v.historySize))
              v.historyIdx = 0;
            v.history[v.historyIdx] = emPos;
          }
        }
        else
        {
          v.valid = true;
          for (Point3 &p : v.history)
            p = stm.getcol(3);
        }
      }

      // uniform matrix with usage flag
      if (subEffects[i].offsets.flags >= 0) // modFx
      {
        if (type == dafx_ex::TRANSFORM_LOCAL_SPACE)
        {
          if (!(subEffects[i].rflags & (1 << MODFX_RFLAG_USE_ETM_AS_WTM)))
          {
            subEffects[i].rflags |= 1 << MODFX_RFLAG_USE_ETM_AS_WTM;
            dafx::set_subinstance_value(g_dafx_ctx, iid, i, subEffects[i].offsets.flags, &subEffects[i].rflags, sizeof(uint32_t));
          }

          setLocalGravity(i, &stm);
        }
        else if (type == dafx_ex::TRANSFORM_WORLD_SPACE)
        {
          if (subEffects[i].rflags & (1 << MODFX_RFLAG_USE_ETM_AS_WTM))
          {
            subEffects[i].rflags &= ~(1 << MODFX_RFLAG_USE_ETM_AS_WTM);
            dafx::set_subinstance_value(g_dafx_ctx, iid, i, subEffects[i].offsets.flags, &subEffects[i].rflags, sizeof(uint32_t));
          }
        }

        TMatrix4 stm4 = stm;
        stm4[3][3] = valueScalingFactor * subEffects[i].transformScalingFactor; // scale is stored in the last component

        dafx::set_subinstance_value(g_dafx_ctx, iid, i, subEffects[i].offsets.fxTm, &stm4, sizeof(TMatrix4));
      }
      else // sparks
      {
        bool isLocalSpace = type == dafx_ex::TRANSFORM_LOCAL_SPACE;
        subEffects[i].rflags &= ~(1 << DAFX_SPARK_DECL_LOCAL_SPACE);
        subEffects[i].rflags |= isLocalSpace << DAFX_SPARK_DECL_LOCAL_SPACE;

        struct
        {
          TMatrix4 stm4;
          uint32_t flags;
        } pushData;

        pushData.stm4 = stm;
        pushData.flags = subEffects[i].rflags;

        dafx::set_subinstance_value(g_dafx_ctx, iid, i, subEffects[i].offsets.fxTm, &pushData, sizeof(pushData));
      }
    }

    if (lightFx)
    {
      TMatrix tm = value * shared->lightTm;
      lightFx->setTm(&tm);
      Point3 lightPos = tm.getcol(3);
      for (int i = 0; i < subEffects.size(); ++i)
      {
        if (subEffects[i].offsets.lightPos >= 0)
          dafx::set_subinstance_value(g_dafx_ctx, iid, i, subEffects[i].offsets.lightPos, &lightPos, sizeof(Point3));
      }
    }
  }

  void setTm(const TMatrix *value) override
  {
    if (iid && value)
      setTransform(*value, dafx_ex::TRANSFORM_LOCAL_SPACE);
  }

  void setEmitterTm(const TMatrix *value) override
  {
    if (iid && value)
      setTransform(*value, dafx_ex::TRANSFORM_WORLD_SPACE);
  }

  void setFakeBrightnessBackgroundPos(const Point3 *v) override
  {
    if (!iid || !v)
      return;

    for (int i = 0; i < subEffects.size(); ++i)
    {
      int ofs = subEffects[i].offsets.fakeBrightnessBackgroundPos;
      if (ofs >= 0 && !(subEffects[i].rflags & (1 << MODFX_RFLAG_BACKGROUND_POS_INITED)))
      {
        subEffects[i].rflags |= (1 << MODFX_RFLAG_BACKGROUND_POS_INITED);
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, v, sizeof(Point3));
      }
    }
  }

  void setVelocity(const Point3 *v) override
  {
    if (!iid || !v)
      return;

    for (int i = 0; i < subEffects.size(); ++i)
    {
      int ofs = subEffects[i].offsets.parentVelocity;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, v, 12);
    }
  }

  void setVelocityScale(const float *scale) override
  {
    if (!iid || !scale)
      return;

    for (int i = 0; i < subEffects.size(); ++i)
    {
      int ofs = subEffects[i].offsets.velocityScaleMin;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, scale, sizeof(*scale));
      ofs = subEffects[i].offsets.velocityScaleMax;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, scale, sizeof(*scale));
    }
  }

  void setVelocityScaleMinMax(const Point2 *scale) override
  {
    if (!iid || !scale)
      return;

    for (int i = 0; i < subEffects.size(); ++i)
    {
      int ofs = subEffects[i].offsets.velocityScaleMin;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, &scale->x, sizeof(float));
      ofs = subEffects[i].offsets.velocityScaleMax;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, &scale->y, sizeof(float));
    }
  }

  void setWindScale(const float *scale) override
  {
    if (!iid || !scale)
      return;

    for (int i = 0; i < subEffects.size(); ++i)
    {
      int ofs = subEffects[i].offsets.windCoeff;
      if (ofs >= 0)
        dafx::set_subinstance_value_from_system(g_dafx_ctx, iid, i, sid, ofs, sizeof(float), {scale, 1});
    }
  }

  void setSpawnRate(const real *v) override
  {
    if (!iid)
      return;

    dafx::set_instance_emission_rate(g_dafx_ctx, iid, v ? *v : 0);
  }

  void reset()
  {
    if (!iid)
      return;

    dafx::reset_instance(g_dafx_ctx, iid);
    recalcFxScale();
    setTransform(TMatrix::IDENT, dafx_ex::TRANSFORM_WORLD_SPACE);
    float spawnRate = 1.f;
    setSpawnRate(&spawnRate);

    if (lightFx)
      lightFx->setParam(HUID_ACES_RESET, nullptr);
  }

  BaseParamScript *clone() override
  {
    DafxCompound *v = new DafxCompound(*this);
    if (sid)
    {
      v->iid = dafx::create_instance(g_dafx_ctx, sid);
      G_ASSERT(v->iid);
      v->recalcFxScale();
      dafx::set_instance_status(g_dafx_ctx, v->iid, false);
    }
    return v;
  }
};

class DafxCompoundFactory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new DafxCompound(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "DafxCompound"; }
};

static DafxCompoundFactory g_dafx_compound_factory;

void register_dafx_compound_factory() { register_effect_factory(&g_dafx_compound_factory); }

void dafx_compound_set_context(dafx::ContextId ctx) { g_dafx_ctx = ctx; }
