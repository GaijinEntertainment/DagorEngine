// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFx/dafx.h>
#include <fx/dag_fxInterface.h>
#include <fx/dag_baseFxClasses.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_gameResources.h>
#include <3d/dag_texMgr.h>
#include <shaders/dag_shaders.h>
#include <math/random/dag_random.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/hash_map.h>
#include <EASTL/fixed_vector.h>
#include "dafxCompound_decl.h"
#include "dafxSystemDesc.h"
#include "dafxQuality.h"

static const int g_last_ref_slot = 16;
static dafx::ContextId g_dafx_ctx;

enum
{
  HUID_ACES_RESET = 0xA781BF24u
};
enum
{
  HUID_ACES_IS_ACTIVE = 0xD6872FCEu
};

enum
{
  RGROUP_DEFAULT = 0,
  RGROUP_LOWRES = 1,
  RGROUP_HIGHRES = 2,
  RGROUP_DISTORTION = 3,
  RGROUP_WATER_PROJ = 4,
  RGROUP_UNDERWATER = 5,
};

#define MODFX_RFLAG_USE_ETM_AS_WTM     0
#define MODFX_RFLAG_OMNI_LIGHT_ENABLED 7

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

struct DafxCompound : BaseParticleEffect
{
  struct ValueOffset
  {
    int fxTm;
    int emitterWtm;
    int emitterNtm;
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
  };

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

  dafx::ContextId ctx;
  dafx::SystemId sid;
  dafx::InstanceId iid;
  eastl::unique_ptr<dafx::SystemDesc> pdesc;
  eastl::fixed_vector<Point2, 8, true> fxRadiusRanges;
  eastl::fixed_vector<TMatrix, 8, true> subTransforms;
  eastl::fixed_vector<ValueOffset, 8, true> valueOffsets;
  eastl::fixed_vector<dafx_ex::TransformType, 8, true> transformTypes;
  eastl::fixed_vector<uint32_t, 8, true> rflags;
  eastl::fixed_vector<uint32_t, 8, true> sflags;
  eastl::vector<TEXTUREID> textures;
  eastl::vector<DistanceLag> distanceLagData; // optional for fx, so no fixed_vector

  float fxScale = 1.f;
  Point2 fxScaleRange = Point2(1.f, 1.f);
  BaseEffectObject *lightFx = nullptr;
  TMatrix lightTm = TMatrix::IDENT;
  LightfxShadowParams lightShadowParams = LightfxShadowParams();
  bool resLoaded = false;

  int maxInstances = 0;
  int playerReserved = 0;
  float spawnRangeLimit = 0;
  float onePointNumber = 0;
  float onePointRadius = 0;
  int gameResId = 0;

  DafxCompound() {}

  DafxCompound(const DafxCompound &r) :
    sid(r.sid),
    iid(r.iid),
    ctx(r.ctx),
    subTransforms(r.subTransforms),
    valueOffsets(r.valueOffsets),
    transformTypes(r.transformTypes),
    rflags(r.rflags),
    sflags(r.sflags),
    lightFx(r.lightFx ? (BaseEffectObject *)r.lightFx->clone() : nullptr),
    lightTm(r.lightTm),
    lightShadowParams(r.lightShadowParams),
    textures(r.textures),
    distanceLagData(r.distanceLagData),
    resLoaded(r.resLoaded),
    pdesc(r.pdesc ? new dafx::SystemDesc(*r.pdesc) : nullptr),
    fxScale(r.fxScale),
    fxScaleRange(r.fxScaleRange),
    fxRadiusRanges(r.fxRadiusRanges),
    maxInstances(r.maxInstances),
    playerReserved(r.playerReserved),
    spawnRangeLimit(r.spawnRangeLimit),
    onePointNumber(r.onePointNumber),
    onePointRadius(r.onePointRadius),
    gameResId(r.gameResId)
  {
    for (TEXTUREID tid : textures)
      acquire_managed_tex(tid);
  }

  ~DafxCompound()
  {
    if (iid && g_dafx_ctx)
      dafx::destroy_instance(g_dafx_ctx, iid);

    if (lightFx)
      delete lightFx;

    for (TEXTUREID tid : textures)
      release_managed_tex(tid);
  }

  void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb) override
  {
    CHECK_FX_VERSION(ptr, len, 3);

    if (!g_dafx_ctx)
    {
      logwarn("fx: compound: failed to load params data, context was not initialized");
      return;
    }

    resLoaded = false;
    gameResId = load_cb->getSelfGameResId();

    pdesc.reset(new dafx::SystemDesc());
    pdesc->gameResId = gameResId;
    subTransforms.clear();
    valueOffsets.clear();
    transformTypes.clear();
    fxRadiusRanges.clear();
    rflags.clear();
    sflags.clear();
    distanceLagData.clear();

    for (TEXTUREID tid : textures)
      release_managed_tex(tid);
    textures.clear();

    CompModfx modfxList;
    modfxList.load(ptr, len, load_cb);

    LightfxParams parLight;
    parLight.load(ptr, len, load_cb);

    CFxGlobalParams parGlobals;
    parGlobals.load(ptr, len, load_cb);

    maxInstances = parGlobals.max_instances;
    playerReserved = parGlobals.player_reserved;
    spawnRangeLimit = parGlobals.spawn_range_limit;
    onePointNumber = parGlobals.one_point_number;
    onePointRadius = parGlobals.one_point_radius;
    lightShadowParams.enabled = false;

    if (modfxList.instance_life_time_min > 0 || modfxList.instance_life_time_max > 0)
    {
      if (modfxList.instance_life_time_min > 0 && modfxList.instance_life_time_max > 0)
      {
        pdesc->emitterData.globalLifeLimitMin = max(modfxList.instance_life_time_min, 0.f);
        pdesc->emitterData.globalLifeLimitMax = max(modfxList.instance_life_time_max, modfxList.instance_life_time_min);
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
        lightShadowParams = parLight.shadow;
        lightTm = TMatrix::IDENT;
        lightTm.setcol(3, parLight.offset);
      }
    }

    int subIdx = 0;

    fxScaleRange.x = modfxList.fx_scale_min;
    fxScaleRange.y = modfxList.fx_scale_max;

    for (int i = 0; i < modfxList.array.size(); ++i)
    {
      const ModfxParams &par = modfxList.array[i];
      if (par.ref_slot < 1 || par.ref_slot > g_last_ref_slot)
        continue;

      void *ref = refFirst[par.ref_slot - 1];
      if (!ref)
      {
        debug("dafx compound: invalid sub fx reference: %d, skipping", i);
        continue;
      }

      BaseEffectObject *fx = static_cast<BaseEffectObject *>(ref);

      dafx::SystemDesc *sdesc = nullptr;
      if (!fx->getParam(_MAKE4C('PFXQ'), &sdesc) || !sdesc)
        continue;

      ValueOffset &offsets = valueOffsets.push_back();

      pdesc->subsystems.push_back(*sdesc);
      sdesc = &pdesc->subsystems.back();
      sdesc->qualityFlags = fx_apply_quality_bits(par.quality, sdesc->qualityFlags);

      dafx::SystemDesc *ddesc = sdesc->subsystems.empty() ? nullptr : &sdesc->subsystems[0];
      G_ASSERT_CONTINUE(ddesc);

      ddesc->renderSortDepth = subIdx++;
      ddesc->emitterData.delay += par.delay;

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

        const dafx::Config &cfg = dafx::get_config(g_dafx_ctx);
        unsigned int lim = dafx::get_emitter_limit(ddesc->emitterData, true);
        if (lim == 0 || lim >= cfg.emission_limit)
        {
          logerr("dafx::emitter, over emitter limit");
          continue;
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

      TMatrix scl = TMatrix::IDENT;
      scl.m[0][0] = par.scale.x;
      scl.m[1][1] = par.scale.y;
      scl.m[2][2] = par.scale.z;

      TMatrix wtm;
      wtm = rotxTM(par.rotation.x * DEG_TO_RAD) * rotyTM(par.rotation.y * DEG_TO_RAD) * rotzTM(par.rotation.z * DEG_TO_RAD) * scl;

      wtm.setcol(3, par.offset);
      subTransforms.push_back(wtm);

      dafx_ex::TransformType trtype = (dafx_ex::TransformType)par.transform_type;
      G_ASSERT(trtype <= dafx_ex::TRANSFORM_LOCAL_SPACE);

      offsets.lightPos = -1;
      offsets.lightRadius = -1;
      offsets.lightColor = -1;
      offsets.parentVelocity = -1;
      offsets.localGravity = -1;
      offsets.gravityTm = -1;
      offsets.emitterNtm = -1;
      offsets.colorMult = -1;
      offsets.flags = -1;
      offsets.velocityScaleMin = -1;
      offsets.velocityScaleMax = -1;
      offsets.radiusMin = -1;
      offsets.radiusMax = -1;
      offsets.windCoeff = -1;

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

      int isModfx = 0;
      if (fx->getParam(_MAKE4C('PFXM'), &isModfx) && isModfx)
      {
        G_ASSERT_CONTINUE(sinfo);

        offsets.flags = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_FLAGS];
        offsets.fxTm = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_TM];
        offsets.emitterWtm = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_TM];

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

        G_ASSERT(sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_RADIUS_MIN) != sinfo->valueOffsets.end());
        G_ASSERT(sinfo->valueOffsets.find(dafx_ex::SystemInfo::VAL_RADIUS_MAX) != sinfo->valueOffsets.end());
        offsets.radiusMin = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_RADIUS_MIN];
        offsets.radiusMax = sinfo->valueOffsets[dafx_ex::SystemInfo::VAL_RADIUS_MAX];

        Point2 fxRadiusRange = Point2(1, 1);
        fxRadiusRange.x = *(float *)(simulationRefData + offsets.radiusMin - renderRefDataSize);
        fxRadiusRange.y = *(float *)(simulationRefData + offsets.radiusMax - renderRefDataSize);

        fxRadiusRanges.push_back(fxRadiusRange);

        uint32_t flg = sinfo->rflags;
        if (lightFx)
          flg |= 1 << MODFX_RFLAG_OMNI_LIGHT_ENABLED;

        *(uint32_t *)(sdesc->emissionData.renderRefData.data() + offsets.flags) = flg;
        rflags.push_back(flg);
        sflags.push_back(sinfo->sflags);
      }
      else // sparks
      {
        offsets.fxTm = 0;
        offsets.emitterWtm = 0;
        offsets.emitterNtm = sdesc->emissionData.renderRefData.size();
        offsets.parentVelocity = sdesc->emissionData.renderRefData.size() + 64;
        rflags.push_back(0);
        sflags.push_back(0);
        fxRadiusRanges.push_back(Point2(1.f, 1.f));
      }

      if (sinfo && trtype == dafx_ex::TRANSFORM_DEFAULT)
        trtype = sinfo->transformType;
      transformTypes.push_back(trtype);
    }

    if (pdesc->subsystems.empty())
    {
      if (!lightFx)
        logerr("dafx compound fx is empty");
      pdesc.reset(nullptr);
    }
    else
    {
      gather_sub_textures(*pdesc, textures);
      for (TEXTUREID tid : textures)
        acquire_managed_tex(tid);

      pdesc->emitterData.type = dafx::EmitterType::FIXED;
      pdesc->emitterData.fixedData.count = 0;

      pdesc->emissionData.type = dafx::EmissionType::NONE;
      pdesc->simulationData.type = dafx::SimulationType::NONE;
    }

    resLoaded = true;

    release_game_resource((GameResource *)modfxList.fx1);
    release_game_resource((GameResource *)modfxList.fx2);
    release_game_resource((GameResource *)modfxList.fx3);
    release_game_resource((GameResource *)modfxList.fx4);
    release_game_resource((GameResource *)modfxList.fx5);
    release_game_resource((GameResource *)modfxList.fx6);
    release_game_resource((GameResource *)modfxList.fx7);
    release_game_resource((GameResource *)modfxList.fx8);
    release_game_resource((GameResource *)modfxList.fx9);
    release_game_resource((GameResource *)modfxList.fx10);
    release_game_resource((GameResource *)modfxList.fx11);
    release_game_resource((GameResource *)modfxList.fx12);
    release_game_resource((GameResource *)modfxList.fx13);
    release_game_resource((GameResource *)modfxList.fx14);
    release_game_resource((GameResource *)modfxList.fx15);
    release_game_resource((GameResource *)modfxList.fx16);
  }

  void setParam(unsigned id, void *value) override
  {
    if (id == _MAKE4C('PFXR'))
    {
      // context was recreated and all systems was released
      if (ctx != g_dafx_ctx)
      {
        G_ASSERT(!sid && !iid);
        sid = dafx::SystemId();
        iid = dafx::InstanceId();
        ctx = g_dafx_ctx;
      }

      if (!pdesc)
        return;

      if (iid)
        dafx::destroy_instance(g_dafx_ctx, iid);

      String resName;
      get_game_resource_name(gameResId, resName);

      eastl::string name;
      name.append_sprintf("%s__%d", resName.c_str(), gameResId);

      sid = dafx::get_system_by_name(g_dafx_ctx, name);
      bool forceRecreate = value && *(bool *)value;
      if (sid && forceRecreate)
      {
        dafx::release_system(g_dafx_ctx, sid);
        sid = dafx::SystemId();
      }

      if (!sid)
      {
        sid = dafx::register_system(g_dafx_ctx, *pdesc, name);
        if (!sid)
          logerr("fx: modfx: failed to register system");
      }

      if (sid && forceRecreate)
      {
        iid = dafx::create_instance(g_dafx_ctx, sid);
        G_ASSERT_RETURN(iid, );
        dafx::set_instance_status(g_dafx_ctx, iid, false);
        recalcFxScale();
      }
      return;
    }

    if (sid && !iid && id == _MAKE4C('PFXE'))
    {
      iid = dafx::create_instance(g_dafx_ctx, sid);
      G_ASSERT_RETURN(iid, );
      recalcFxScale();
    }

    if (!iid)
      return;

    if (id == _MAKE4C('PFXE') || id == _MAKE4C('PFXF'))
      dafx::set_instance_status(g_dafx_ctx, iid, value ? *(bool *)value : false);
    else if (id == _MAKE4C('PFXP'))
    {
      Point4 pos = *(Point4 *)value;
      dafx::set_instance_pos(g_dafx_ctx, iid, pos);
    }
    else if (id == _MAKE4C('PFXV'))
      dafx::set_instance_visibility(g_dafx_ctx, iid, value ? *(uint32_t *)value : 0);
    else if (id == _MAKE4C('PFXI'))
      ((eastl::vector<dafx::InstanceId> *)value)->push_back(iid);
    else if (id == HUID_TM)
      setTm((TMatrix *)value);
    else if (id == HUID_EMITTER_TM)
      setEmitterTm((TMatrix *)value);
    else if (id == HUID_VELOCITY)
      setVelocity((Point3 *)value);
    else if (id == HUID_ACES_RESET)
      reset();
    else if (id == HUID_COLOR_MULT)
      setColorMult((Color3 *)value);
    else if (id == HUID_COLOR4_MULT)
      setColor4Mult((Color4 *)value);
    else if (id == _MAKE4C('PFXG'))
      dafx::warmup_instance(g_dafx_ctx, iid, value ? *(float *)value : 0);
    else if (id == _MAKE4C('GZTM'))
      setGravityTm(*(Matrix3 *)value);
    else if (lightFx) // always last.
      lightFx->setParam(id, value);
  }

  void *getParam(unsigned id, void *value) override
  {
    if (id == _MAKE4C('CACH'))
    {
      return (void *)1; //-V566
    }
    else if (id == HUID_ACES_IS_ACTIVE)
    {
      *(bool *)value = iid && dafx::is_instance_renderable_active(g_dafx_ctx, iid);
      return value;
    }
    else if (id == _MAKE4C('FXLM'))
    {
      *(int *)value = maxInstances;
    }
    else if (id == _MAKE4C('FXLP'))
    {
      *(int *)value = playerReserved;
    }
    else if (id == _MAKE4C('FXLR'))
    {
      *(float *)value = spawnRangeLimit;
    }
    else if (id == _MAKE4C('FXLX'))
    {
      *(Point2 *)value = Point2(onePointNumber, onePointRadius);
      return value;
    }
    else if (id == _MAKE4C('FXSH'))
    {
      *(LightfxShadowParams *)value = lightShadowParams;
      return value;
    }
    else if (id == _MAKE4C('PFXX') && pdesc)
    {
      int parentRen = 0;
      int parentSim = 0;
      int ren = 0;
      int sim = 0;

      for (const dafx::SystemDesc &sub : pdesc->subsystems)
      {
        parentRen += sub.renderElemSize;
        parentSim += sub.simulationElemSize;
        ren += sub.subsystems[0].renderElemSize;
        sim += sub.subsystems[0].simulationElemSize;
      }

      ((int *)value)[0] = parentRen;
      ((int *)value)[1] = parentSim;
      ((int *)value)[2] = ren;
      ((int *)value)[3] = sim;
      return value;
    }
    else if (id == _MAKE4C('PFXT'))
    {
      *(dafx::SystemId *)value = sid;
      return value;
    }
    else if (id == _MAKE4C('PFXC') && value)
    {
      *(bool *)value = dafx::is_instance_renderable_visible(g_dafx_ctx, iid);
      return value;
    }
    else if (id == _MAKE4C('FXLD'))
    {
      if (value)
        ((eastl::vector<dafx::SystemDesc *> *)value)->push_back(pdesc.get());
      return value;
    }
    else if (id == _MAKE4C('FXIC') && value)
    {
      *(int *)value = dafx::get_instance_elem_count(g_dafx_ctx, iid);
      return value;
    }
    else if (id == _MAKE4C('FXID') && value)
    {
      *(dafx::InstanceId *)value = iid;
      return value;
    }
    else if (lightFx)
      return lightFx->getParam(id, value);

    return nullptr;
  }

  void update(float dt) override
  {
    if (lightFx)
    {
      lightFx->update(dt);

      Color4 *params = nullptr;
      params = (Color4 *)lightFx->getParam(HUID_LIGHT_PARAMS, params);
      G_ASSERT_RETURN(params, );

      for (int i = 0; i < subTransforms.size(); ++i)
      {
        if (valueOffsets[i].lightPos >= 0)
          dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].lightColor, params, sizeof(Point4));
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
    for (int i = 0; i < valueOffsets.size(); ++i)
    {
      int ofs = valueOffsets[i].colorMult;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, &c, 4);
    }

    if (lightFx)
      lightFx->setColor4Mult(value);
  }

  void setGravityTm(const Matrix3 &tm)
  {
    if (!iid)
      return;

    for (int i = 0; i < valueOffsets.size(); ++i)
    {
      int ofs = valueOffsets[i].gravityTm;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, &tm, sizeof(tm));
    }
  }

  void recalcFxScale()
  {
    int seed = uintptr_t(this) | uint32_t(iid);
    fxScale = lerp(fxScaleRange.x, fxScaleRange.y, _frnd(seed));

    G_ASSERT(subTransforms.size() == fxRadiusRanges.size());
    for (int i = 0; i < subTransforms.size(); ++i)
    {
      Point2 radius = fxRadiusRanges[i] * fxScale;
      if (valueOffsets[i].radiusMin >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].radiusMin, &radius.x, sizeof(float));
      if (valueOffsets[i].radiusMax >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].radiusMax, &radius.y, sizeof(float));
    }
  }

  void setTransform(const TMatrix &value, dafx_ex::TransformType tr_type)
  {
    for (int i = 0; i < subTransforms.size(); ++i)
    {
      dafx_ex::TransformType type = transformTypes[i];
      if (type == dafx_ex::TRANSFORM_DEFAULT)
        type = tr_type;

      TMatrix stm = value * subTransforms[i];
      stm.col[0] *= fxScale;
      stm.col[1] *= fxScale;
      stm.col[2] *= fxScale;

      if (i < distanceLagData.size() && distanceLagData[i].enabled)
      {
        DistanceLag &v = distanceLagData[i];
        if (v.valid)
        {
          const Point3 &em = stm.getcol(3);
          const Point3 &pl = v.history[v.historyIdx];
          if (lengthSq(em - pl) > v.stepSq)
          {
            v.historyIdx = (v.historyIdx + 1) % v.historySize;
            v.history[v.historyIdx] = em;
          }

          // we need to accumulate distance, not only direct point dist cmp, in case of path curvature
          bool found = false;
          float distNext = 0.f;
          float distPrev = 0.f;
          for (int h = 0; h < v.historySize - 1; ++h)
          {
            int idx = v.historyIdx + v.historySize - h;
            const Point3 &p0 = v.history[idx % v.historySize];
            const Point3 &p1 = h > 0 ? v.history[(idx - 1) % v.historySize] : p0;
            distPrev = distNext;
            distNext += length(p0 - p1);
            if (distNext * distNext > v.distanceSq)
            {
              found = true;
              float overshoot = distNext - v.distance;
              float ratio = overshoot / (distNext - distPrev);
              stm.setcol(3, lerp(p0, p1, ratio));
              break;
            }
          }

          if (!found) // outside history, attach to the older emmiter pos
            stm.setcol(3, v.history[(v.historyIdx + 1) % v.historySize]);
        }
        else
        {
          v.valid = true;
          for (Point3 &p : v.history)
            p = stm.getcol(3);
        }
      }

      // uniform matrix with usage flag
      if (valueOffsets[i].flags >= 0) // modFx
      {
        if (type == dafx_ex::TRANSFORM_LOCAL_SPACE)
        {
          if (!(rflags[i] & (1 << MODFX_RFLAG_USE_ETM_AS_WTM)))
          {
            rflags[i] |= 1 << MODFX_RFLAG_USE_ETM_AS_WTM;
            dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].flags, &rflags[i], sizeof(uint32_t));
          }

          if (valueOffsets[i].localGravity >= 0)
          {
            TMatrix itm = orthonormalized_inverse(stm);
            Point3 vec = itm.getcol(1); // inv-up
            dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].localGravity, &vec, sizeof(Point3));
          }
        }
        else if (type == dafx_ex::TRANSFORM_WORLD_SPACE)
        {
          if (rflags[i] & (1 << MODFX_RFLAG_USE_ETM_AS_WTM))
          {
            rflags[i] &= ~(1 << MODFX_RFLAG_USE_ETM_AS_WTM);
            dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].flags, &rflags[i], sizeof(uint32_t));
          }
        }

        TMatrix4 stm4 = stm;
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].fxTm, &stm4, sizeof(TMatrix4));
      }
      else // sparks
      {
        struct
        {
          TMatrix4 stm4;
          uint32_t isLocalSpace;
        } pushData;

        pushData.stm4 = stm;
        pushData.isLocalSpace = type == dafx_ex::TRANSFORM_LOCAL_SPACE;
        if (pushData.isLocalSpace) // using it like an inverse matrix to get forces in local space.
          pushData.stm4 = pushData.stm4.transpose();

        dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].fxTm, &pushData, sizeof(pushData));

        if (type == dafx_ex::TRANSFORM_WORLD_SPACE)
        {
          TMatrix emitterNormalTm;
          emitterNormalTm.setcol(0, normalize(stm.getcol(0)));
          emitterNormalTm.setcol(1, normalize(stm.getcol(1)));
          emitterNormalTm.setcol(2, normalize(stm.getcol(2)));
          emitterNormalTm.setcol(3, Point3(0, 0, 0));

          TMatrix4 ntm4 = emitterNormalTm;
          dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].emitterNtm, &ntm4, sizeof(TMatrix4));
        }
      }
    }

    if (lightFx)
    {
      TMatrix tm = value * lightTm;
      Point3 lightPos = tm.getcol(3);
      for (int i = 0; i < subTransforms.size(); ++i)
      {
        if (valueOffsets[i].lightPos >= 0)
          dafx::set_subinstance_value(g_dafx_ctx, iid, i, valueOffsets[i].lightPos, &lightPos, sizeof(Point3));
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

  void setVelocity(const Point3 *v) override
  {
    if (!iid || !v)
      return;

    for (int i = 0; i < valueOffsets.size(); ++i)
    {
      int ofs = valueOffsets[i].parentVelocity;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, v, 12);
    }
  }

  void setVelocityScale(const float *scale) override
  {
    if (!iid || !scale)
      return;

    for (int i = 0; i < valueOffsets.size(); ++i)
    {
      int ofs = valueOffsets[i].velocityScaleMin;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, scale, sizeof(*scale));
      ofs = valueOffsets[i].velocityScaleMax;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, scale, sizeof(*scale));
    }
  }

  void setVelocityScaleMinMax(const Point2 *scale) override
  {
    if (!iid || !scale)
      return;

    for (int i = 0; i < valueOffsets.size(); ++i)
    {
      int ofs = valueOffsets[i].velocityScaleMin;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, &scale->x, sizeof(float));
      ofs = valueOffsets[i].velocityScaleMax;
      if (ofs >= 0)
        dafx::set_subinstance_value(g_dafx_ctx, iid, i, ofs, &scale->y, sizeof(float));
    }
  }

  void setWindScale(const float *scale) override
  {
    if (!iid || !scale)
      return;

    for (int i = 0; i < valueOffsets.size(); ++i)
    {
      int ofs = valueOffsets[i].windCoeff;
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
    if (!resLoaded)
      return nullptr;

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