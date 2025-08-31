// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFx/dafx.h>
#include <fx/dag_baseFxClasses.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_texMgr.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix4.h>
#include "dafxSystemDesc.h"
#include <debug/dag_debug3d.h>
#include <dafxEmitterDebug.h>
#include <math/dag_hlsl_floatx.h>
#include <daFx/dafx_gravity_zone.hlsli>
#include <gameRes/dag_gameResSystem.h>

int dafx_gravity_zone_count = 0;
GravityZoneDescriptor_buffer dafx_gravity_zone_buffer = nullptr;

enum
{
  HUID_ACES_RESET = 0xA781BF24u
};

static dafx::ContextId g_dafx_ctx;

// TODO: These are copy-pasted from modfx_decl.hlsli. It should be refactored later.
#define MODFX_RFLAG_USE_ETM_AS_WTM        0
#define MODFX_RFLAG_BACKGROUND_POS_INITED 15

extern bool dafx_modfx_system_load(const char *ptr, int len, BaseParamScriptLoadCB *load_cb, dafx::ContextId ctx,
  dafx::SystemDesc &sdesc, dafx_ex::SystemInfo &sinfo, dafx_ex::EmitterDebug *&emitter_debug);

void dafx_report_broken_res(int game_res_id, const char *fx_type)
{
  String resName;
  ::get_game_resource_name(game_res_id, resName);
  logerr("failed to load resource: %s | id:%d | type: %s", resName.c_str(), game_res_id, fx_type);
}


struct DafxModFx : BaseParticleEffect
{
  dafx::ContextId ctx;
  dafx::SystemId sid;
  dafx::InstanceId iid;
  eastl::unique_ptr<dafx::SystemDesc> pdesc;
  dafx_ex::SystemInfo sinfo;
  eastl::unique_ptr<dafx_ex::EmitterDebug> emitterDebug;

  eastl::vector<TEXTUREID> textures;
  float distanceToCamOnSpawn = 0;
  int gameResId = 0;

  DafxModFx() : emitterDebug(nullptr) {}

  DafxModFx(const DafxModFx &r) :
    sid(r.sid),
    iid(r.iid),
    ctx(r.ctx),
    pdesc(r.pdesc ? new dafx::SystemDesc(*r.pdesc) : nullptr),
    sinfo(r.sinfo),
    textures(r.textures),
    gameResId(r.gameResId)
  {
    for (TEXTUREID t : textures)
      if (t != BAD_TEXTUREID)
        acquire_managed_tex(t);
    emitterDebug = eastl::make_unique<dafx_ex::EmitterDebug>(*r.emitterDebug);
  }

  ~DafxModFx()
  {
    if (iid && g_dafx_ctx)
      dafx::destroy_instance(g_dafx_ctx, iid);

    for (TEXTUREID t : textures)
      if (t != BAD_TEXTUREID)
        release_managed_tex(t);
  }

  void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb) override
  {
    gameResId = load_cb->getSelfGameResId();
    if (!loadParamsDataInternal(ptr, len, load_cb)) // pdesc will be null on fail
      dafx_report_broken_res(gameResId, "dafx_modfx");
  }

  bool loadParamsDataInternal(const char *ptr, int len, BaseParamScriptLoadCB *load_cb)
  {
    if (!g_dafx_ctx)
    {
      logwarn("fx: modfx: failed to load params data, context was not initialized");
      return false;
    }

    pdesc.reset(new dafx::SystemDesc());
    sinfo = dafx_ex::SystemInfo();
    pdesc->gameResId = gameResId;
    emitterDebug = eastl::make_unique<dafx_ex::EmitterDebug>(dafx_ex::EmitterDebug());
    dafx_ex::EmitterDebug *emitterDebugObject = emitterDebug.get();
    if (!dafx_modfx_system_load(ptr, len, load_cb, g_dafx_ctx, *pdesc, sinfo, emitterDebugObject))
    {
      pdesc.reset(nullptr);
      return false;
    }

    if (pdesc && !pdesc->subsystems.empty())
    {
      pdesc->subsystems[0].gameResId = gameResId;
      for (auto [tid, a] : pdesc->subsystems[0].texturesPs)
        if (tid != BAD_TEXTUREID)
          textures.push_back(tid);
    }

    return true;
  }

  void setGravityTm(const Matrix3 &tm)
  {
    auto it = sinfo.valueOffsets.find(dafx_ex::SystemInfo::VAL_GRAVITY_ZONE_TM);
    if (it != sinfo.valueOffsets.end())
      dafx::set_instance_value(g_dafx_ctx, iid, it->second, &tm, sizeof(Matrix3));
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

      if (iid)
        dafx::destroy_instance(g_dafx_ctx, iid);

      eastl::string name;
      name.append_sprintf("modfx_%d", gameResId);
      sid = dafx::get_system_by_name(g_dafx_ctx, name);
      bool forceRecreate = value && *(bool *)value;
      if (sid && forceRecreate)
      {
        dafx::release_system(g_dafx_ctx, sid);
        sid = dafx::SystemId();
      }

      if (!sid)
      {
        sid = pdesc ? dafx::register_system(g_dafx_ctx, *pdesc, name) : dafx::get_dummy_system_id(g_dafx_ctx);
        if (!sid)
          logerr("fx: modfx: failed to register system");
      }

      if (sid && forceRecreate)
      {
        iid = dafx::create_instance(g_dafx_ctx, sid);
        G_ASSERT_RETURN(iid, );
        dafx::set_instance_status(g_dafx_ctx, iid, false);
      }
      return;
    }

    if (sid && !iid && id == _MAKE4C('PFXE')) // for tools
      iid = dafx::create_instance(g_dafx_ctx, sid);

    if (!iid)
      return;

    if (id == _MAKE4C('PFXE'))
    {
      G_ASSERT(value);
      BaseEffectEnabled *dafxe = static_cast<BaseEffectEnabled *>(value);
      distanceToCamOnSpawn = dafxe->distanceToCam;
      dafx::set_instance_status(g_dafx_ctx, iid, dafxe->enabled, dafxe->distanceToCam);
    }
    else if (id == _MAKE4C('PFXP'))
    {
      Point4 pos = *(Point4 *)value;
      dafx::set_instance_pos(g_dafx_ctx, iid, pos);
    }
    else if (id == _MAKE4C('PFXV'))
      dafx::set_instance_visibility(g_dafx_ctx, iid, value ? *(uint32_t *)value : 0);
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
    else if (id == _MAKE4C('PFXI'))
      ((eastl::vector<dafx::InstanceId> *)value)->push_back(iid);
    else if (id == _MAKE4C('GZTM'))
      setGravityTm(*(Matrix3 *)value);
  }

  void *getParam(unsigned id, void *value) override
  {
    if (id == _MAKE4C('PFXM'))
    {
      *(int *)value = 1;
      return value;
    }
    else if (id == _MAKE4C('PFXQ'))
    {
      *(dafx::SystemDesc **)value = pdesc.get();
      return value;
    }
    else if (id == _MAKE4C('PFVR'))
    {
      *(dafx_ex::SystemInfo **)value = &sinfo;
      return value;
    }
    else if (id == _MAKE4C('CACH'))
    {
      return (void *)1; //-V566
    }
    else if (id == HUID_ACES_IS_ACTIVE)
    {
      *(bool *)value = dafx::is_instance_renderable_active(g_dafx_ctx, iid);
      return value;
    }
    else if (id == _MAKE4C('FXLM'))
    {
      *(int *)value = sinfo.maxInstances;
    }
    else if (id == _MAKE4C('FXLP'))
    {
      *(int *)value = sinfo.playerReserved;
    }
    else if (id == _MAKE4C('FXLR') && pdesc)
    {
      *(float *)value = pdesc->emitterData.spawnRangeLimit;
    }
    else if (id == _MAKE4C('FXLX'))
    {
      *(Point2 *)value = Point2(sinfo.onePointNumber, sinfo.onePointRadius);
      return value;
    }
    else if (id == _MAKE4C('PFXX') && pdesc)
    {
      ((int *)value)[0] = pdesc->renderElemSize;
      ((int *)value)[1] = pdesc->simulationElemSize;
      ((int *)value)[2] = pdesc->subsystems[0].renderElemSize;
      ((int *)value)[3] = pdesc->subsystems[0].simulationElemSize;
      return value;
    }
    else if (id == _MAKE4C('PFXC') && value)
      *(bool *)value = dafx::is_instance_renderable_visible(g_dafx_ctx, iid);
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
    else if (id == _MAKE4C('FXIX') && value)
    {
      *(int *)value = sinfo.transformType;
      return value;
    }
    else if (id == _MAKE4C('FXID') && value)
    {
      *(dafx::InstanceId *)value = iid;
      return value;
    }

    return nullptr;
  }

  void update(float) override {}

  virtual void drawEmitter(const Point3 &pos)
  {
    E3DCOLOR blue = E3DCOLOR_MAKE(0, 0, 255, 255);
    switch (emitterDebug->type)
    {
      case dafx_ex::NONE: break;
      case dafx_ex::SPHERE:
      {
        draw_debug_sph(pos + emitterDebug->sphere.offset, emitterDebug->sphere.radius, blue);
      }
      break;
      case dafx_ex::BOX:
      {
        BBox3 debugBox;
        debugBox.lim[0] = pos + emitterDebug->box.offset - (emitterDebug->box.dims);
        debugBox.lim[1] = pos + emitterDebug->box.offset + (emitterDebug->box.dims);
        draw_debug_box(debugBox, blue);
      }
      break;
      case dafx_ex::CYLINDER:
      {
        Point3 position = pos + emitterDebug->cylinder.offset + emitterDebug->cylinder.vec * emitterDebug->cylinder.height * 0.5f;
        Point3 norm;
        if (abs(emitterDebug->cylinder.vec.x) > 0.1)
          norm = emitterDebug->cylinder.vec % Point3(0, 0, 1);
        else
          norm = emitterDebug->cylinder.vec % Point3(1, 0, 0);

        Point3 cross = norm % emitterDebug->cylinder.vec;

        Point3 p1 = position + emitterDebug->cylinder.vec * (emitterDebug->cylinder.height) * 0.5;
        Point3 p2 = position - emitterDebug->cylinder.vec * (emitterDebug->cylinder.height) * 0.5;
        draw_debug_circle(p1, cross, norm, emitterDebug->cylinder.radius, blue);
        draw_debug_circle(p2, cross, norm, emitterDebug->cylinder.radius, blue);

        const float angleStep = 2 * PI / 4;
        for (int i = 0; i < 4; ++i)
        {
          Quat quaternion = Quat(emitterDebug->cylinder.vec, i * angleStep);
          Point3 newDir = quaternion * norm;
          draw_debug_line(p1 + newDir * emitterDebug->cylinder.radius, p2 + newDir * emitterDebug->cylinder.radius, blue);
        }
      }
      break;
      case dafx_ex::CONE:
      {
        Point3 position = pos + emitterDebug->cone.offset + emitterDebug->cone.vec * emitterDebug->cone.h1 * 0.5f;
        Point3 norm;
        if (abs(emitterDebug->cone.vec.x) > 0.1)
          norm = normalize(emitterDebug->cone.vec % Point3(0, 0, 1));
        else
          norm = normalize(emitterDebug->cone.vec % Point3(1, 0, 0));

        Point3 cross = normalize(norm % emitterDebug->cone.vec);

        float ro = safediv(emitterDebug->cone.rad, emitterDebug->cone.h2);
        float r3 = emitterDebug->cone.h1 * ro;
        float r2 = r3 + emitterDebug->cone.rad;

        Point3 p1 = position + emitterDebug->cone.vec * (emitterDebug->cone.h1) * 0.5;
        Point3 p2 = position - emitterDebug->cone.vec * (emitterDebug->cone.h1) * 0.5;
        draw_debug_circle(p1, cross, norm, r2, blue);
        draw_debug_circle(p2, cross, norm, emitterDebug->cone.rad, blue);

        const float angleStep = 2 * PI / 4;
        for (int i = 0; i < 4; ++i)
        {
          Quat quaternion = Quat(emitterDebug->cone.vec, i * angleStep);
          Point3 newDir = quaternion * norm;
          draw_debug_line(p1 + newDir * r2, p2 + newDir * emitterDebug->cone.rad, blue);
        }
      }
      break;
      case dafx_ex::SPHERESECTOR:
      {
        Point3 vec = normalize(emitterDebug->sphereSector.vec);

        float yAngle = (emitterDebug->sphereSector.sector - 0.5) * PI;
        float s1, c1;
        sincos(yAngle, s1, c1);

        Point3 tv = Point3(c1, s1, c1);

        Point3 bottom = pos - vec * emitterDebug->sphereSector.radius;
        Point3 top = pos + vec * emitterDebug->sphereSector.radius;

        Point3 norm;
        if (abs(vec.x) > 0.1)
          norm = normalize(vec % Point3(0, 0, 1));
        else
          norm = normalize(vec % Point3(1, 0, 0));

        float lenthFirstCircle = (tv.y + 1.0) * emitterDebug->sphereSector.radius;
        Point3 firstCirclePos = bottom + vec * lenthFirstCircle;

        Point3 cross = normalize(norm % vec);

        float y = length(firstCirclePos - bottom) / (emitterDebug->sphereSector.radius * 2.0);

        float h = 2.0f * emitterDebug->sphereSector.radius * y;
        float radius = sqrtf(2.0f * emitterDebug->sphereSector.radius * h - sqr(h));
        draw_debug_circle(firstCirclePos, cross, norm, radius, blue);

        Point3 p1 = firstCirclePos + cross * radius;
        Point3 p2 = firstCirclePos - cross * radius;
        Point3 p3 = firstCirclePos + norm * radius;
        Point3 p4 = firstCirclePos - norm * radius;

        draw_debug_line(pos, p1, blue);
        draw_debug_line(pos, p2, blue);
        draw_debug_line(pos, p3, blue);
        draw_debug_line(pos, p4, blue);

        const int totalSegments = 48;
        const int steps = totalSegments * (1.0f - y) + 1;
        const float angleStep = 2 * PI / 4;
        Point3 posLine2 = Point3(0, 0, 0);
        float radiusLine2 = 0.0f;
        for (int i = 0; i < steps - 1; ++i)
        {
          float h1 = 2.0f * emitterDebug->sphereSector.radius * (i / (float)totalSegments);
          float h2 = 2.0f * emitterDebug->sphereSector.radius * ((i + 1) / (float)totalSegments);
          float radiusLine1 = sqrtf(2.0f * emitterDebug->sphereSector.radius * h1 - sqr(h1));
          radiusLine2 = sqrtf(2.0f * emitterDebug->sphereSector.radius * h2 - sqr(h2));
          Point3 posLine1 = top - vec * (i / (float)totalSegments) * 2.0;
          posLine2 = top - vec * ((i + 1) / (float)totalSegments) * 2.0;
          for (int j = 0; j < 4; ++j)
          {
            Quat quaternion = Quat(vec, j * angleStep);
            Point3 newDir = quaternion * norm;
            draw_debug_line(posLine1 + newDir * radiusLine1, posLine2 + newDir * radiusLine2, blue);
          }
        }

        for (int j = 0; j < 4; ++j)
        {
          Quat quaternion = Quat(vec, j * angleStep);
          Point3 newDir = quaternion * norm;
          draw_debug_line(posLine2 + newDir * radiusLine2, firstCirclePos + newDir * radius, blue);
        }

        if (emitterDebug->sphereSector.sector < 0.5)
          draw_debug_circle(pos, cross, norm, emitterDebug->sphereSector.radius, blue);
      }
      break;
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
    auto ofsIt = sinfo.valueOffsets.find(dafx_ex::SystemInfo::VAL_COLOR_MUL);
    if (iid && value && ofsIt != sinfo.valueOffsets.end())
    {
      E3DCOLOR c = e3dcolor(*value);
      dafx::set_instance_value(g_dafx_ctx, iid, ofsIt->second, &c, 4);
    }
  }

  void setTransform(TMatrix value, dafx_ex::TransformType tr_type)
  {
    int type = sinfo.transformType;
    if (type == dafx_ex::TRANSFORM_DEFAULT)
      type = tr_type;

    sinfo.distanceScale.apply(value, distanceToCamOnSpawn);

    if (type == dafx_ex::TRANSFORM_LOCAL_SPACE)
    {
      if (!(sinfo.rflags & (1 << MODFX_RFLAG_USE_ETM_AS_WTM)))
      {
        sinfo.rflags |= 1 << MODFX_RFLAG_USE_ETM_AS_WTM;
        dafx::set_instance_value(g_dafx_ctx, iid, sinfo.valueOffsets[dafx_ex::SystemInfo::VAL_FLAGS], &sinfo.rflags, sizeof(uint32_t));
      }

      auto it = sinfo.valueOffsets.find(dafx_ex::SystemInfo::VAL_LOCAL_GRAVITY_VEC);
      if (it != sinfo.valueOffsets.end())
      {
        TMatrix itm = orthonormalized_inverse(value);
        Point3 vec = itm.getcol(1); // inv-up
        dafx::set_instance_value(g_dafx_ctx, iid, it->second, &vec, sizeof(Point3));
      }
    }
    else if (type == dafx_ex::TRANSFORM_WORLD_SPACE && (sinfo.rflags & (1 << MODFX_RFLAG_USE_ETM_AS_WTM)))
    {
      sinfo.rflags &= ~(1 << MODFX_RFLAG_USE_ETM_AS_WTM);
      dafx::set_instance_value(g_dafx_ctx, iid, sinfo.valueOffsets[dafx_ex::SystemInfo::VAL_FLAGS], &sinfo.rflags, sizeof(uint32_t));
    }

    TMatrix4 fxTm4 = value;
    fxTm4[3][3] = value.getScalingFactor(); // scale is stored in the last component
    dafx::set_instance_value(g_dafx_ctx, iid, sinfo.valueOffsets[dafx_ex::SystemInfo::VAL_TM], &fxTm4, 64);
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
    auto ofsIt = sinfo.valueOffsets.find(dafx_ex::SystemInfo::VAL_VELOCITY_START_ADD);
    if (!iid || !v || ofsIt == sinfo.valueOffsets.end())
      return;

    dafx::set_instance_value(g_dafx_ctx, iid, ofsIt->second, v, 12);
  }

  void setFakeBrightnessBackgroundPos(const Point3 *v) override
  {
    auto ofsIt = sinfo.valueOffsets.find(dafx_ex::SystemInfo::VAL_FAKE_BRIGHTNESS_BACKGROUND_POS);
    if (!iid || !v || ofsIt == sinfo.valueOffsets.end() || sinfo.rflags & (1 << MODFX_RFLAG_BACKGROUND_POS_INITED))
      return;

    dafx::set_instance_value(g_dafx_ctx, iid, ofsIt->second, v, 12);
    sinfo.rflags |= 1 << MODFX_RFLAG_BACKGROUND_POS_INITED;
  }

  void reset()
  {
    if (!iid)
      return;

    dafx::reset_instance(g_dafx_ctx, iid);

    setTm(&TMatrix::IDENT);
    setEmitterTm(&TMatrix::IDENT);

    float spawnRate = 1.f;
    setSpawnRate(&spawnRate);
  }

  void setSpawnRate(const real *v) override
  {
    if (!iid)
      return;

    dafx::set_instance_emission_rate(g_dafx_ctx, iid, v ? *v : 0);
  }

  BaseParamScript *clone() override
  {
    if (!sid) // compoundPs trying to grab a copy
    {
      DafxModFx *v = new DafxModFx(*this);
      return v;
    }
    else
    {
      DafxModFx *v = new DafxModFx();
      v->sid = sid;
      if (sid)
        v->iid = dafx::create_instance(g_dafx_ctx, sid);
      G_ASSERT(v->iid);

      v->sinfo = sinfo;
      v->textures = textures;

      for (TEXTUREID t : textures)
        if (t != BAD_TEXTUREID)
          acquire_managed_tex(t);

      dafx::set_instance_status(g_dafx_ctx, v->iid, false);
      return v;
    }
    return nullptr;
  }
};

class DafxModfxFactory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new DafxModFx(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "DafxModFx"; }
};

static DafxModfxFactory g_dafx_modfx_factory;

void register_dafx_modfx_factory() { register_effect_factory(&g_dafx_modfx_factory); }

void dafx_modfx_set_context(dafx::ContextId ctx) { g_dafx_ctx = ctx; }