//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <fx/dag_paramScript.h>
#include <math/dag_mathBase.h>
#include <math/dag_e3dColor.h>
#include <math/dag_color.h>
#include <osApiWrappers/dag_atomic.h>


static constexpr unsigned HUID_BaseEffectObject = 0x1867A06Bu;      // BaseEffectObject
static constexpr unsigned HUID_BaseParticleEffect = 0x2869083Au;    // BaseParticleEffect
static constexpr unsigned HUID_BaseTrailEffect = 0xFBDF6642u;       // BaseTrailEffect
static constexpr unsigned HUID_BaseArcEffect = 0x078CE44Au;         // BaseArcEffect
static constexpr unsigned HUID_BaseParticleFxEmitter = 0x738B5475u; // BaseParticleEmitter


enum
{
  FX_RENDER_BEFORE = 0,
  FX_RENDER_SOLID,
  FX_RENDER_TRANS,
  FX_RENDER_PROJECTED,
  FX_RENDER_PROJECTED_TO_TRANS,
  FX_RENDER_TRANS_BATCH,

  FX_RENDER_DISTORTION,

  FX_OCCLUSION_TEST,
  FX_RENDER_TRANS_PRE_TESTED,
  FX_RENDER_TRANS_BATCH_PRE_TESTED,
};


static constexpr unsigned HUID_POS = 0xEAFE0942u;             // POS
static constexpr unsigned HUID_TM = 0xF2642399u;              // TM
static constexpr unsigned HUID_EMITTER = 0x7276B506u;         // EMITTER
static constexpr unsigned HUID_EMITTER_TM = 0x767A7D3Du;      // EMITTER_TM
static constexpr unsigned HUID_STATICVISSPHERE = 0x87DA990Bu; // STATICVISSPHERE
static constexpr unsigned HUID_RAYTRACER = 0xBAB277C9u;       // RAYTRACER
static constexpr unsigned HUID_SOUND = 0x9B7EB41Fu;           // SOUND
static constexpr unsigned HUID_BLOODY_BAKER = 0x74780947u;    // BLOODY_BAKER
static constexpr unsigned HUID_COLOR_MULT = 0x955247C9u;      // COLOR_MULT
static constexpr unsigned HUID_COLOR4_MULT = 0x32946177u;     // COLOR4_MULT
static constexpr unsigned HUID_VELOCITY = 0xD9034E15u;        // VELOCITY

static constexpr unsigned HUID_SPAWN_POS = 0x2B73EB08u; // SPAWN_POS
static constexpr unsigned HUID_SPAWN_DIR = 0xD29C2755u; // SPAWN_DIR
static constexpr unsigned HUID_SPAWN = 0x9DC28DEFu;     // SPAWN

static constexpr unsigned HUID_SEED = 0xB89D673Cu; // SEED

static constexpr unsigned HUID_RANDOMPHASE = 0x706F5334u; // RANDOMPHASE
static constexpr unsigned HUID_STARTPHASE = 0x469AE005u;  // STARTPHASE

static constexpr unsigned HUID_SPAWN_RATE = 0xE400581Eu;   // SPAWN_RATE
static constexpr unsigned HUID_LIFE = 0xAD509D80u;         // LIFE
static constexpr unsigned HUID_BURST = 0x242EE7B9u;        // BURST
static constexpr unsigned HUID_BURST_DONE = 0x68918D2Eu;   // BURST_DONE
static constexpr unsigned HUID_SET_NUMPARTS = 0x61F8DE0Au; // SET_NUMPARTS

static constexpr unsigned HUID_LIGHT = 0x6D95CA7Au; // LIGHT

static constexpr unsigned HUID_BATCH_GROUP = 0x3904D1FCu; // HUID_BATCH_GROUP

static constexpr unsigned HUID_LIGHT_PARAMS = 0x1971045du;
static constexpr unsigned HUID_LIGHT_POS = 0x1ff30453u;

#define CHECK_FX_VERSION(ptr, len, ver)                                    \
  G_ASSERT(len >= sizeof(int));                                            \
  if (*(int *)ptr != (ver))                                                \
    DAG_FATAL("unsupported version %d (expected %d)\nin %s:%d, %s()", /**/ \
      *(int *)ptr, (ver), __FILE__, __LINE__, __FUNCTION__);               \
  len -= sizeof(int);                                                      \
  ptr += sizeof(int);

// #define IF_STUB(name) virtual void name(void *value) = 0;
#define IF_STUB(name, type) virtual void name(const type *){};

class BaseEffectInterface : public BaseParamScript
{
  // fast setters
public:
  IF_STUB(setTm, TMatrix);
  IF_STUB(setEmitterTm, TMatrix);
  IF_STUB(setVelocity, Point3);
  IF_STUB(setLocalVelocity, Point3);
  IF_STUB(setSpawnRate, real);
  IF_STUB(setColorMult, Color3);
  IF_STUB(setColor4Mult, Color4);
  IF_STUB(setWind, Point3);
  IF_STUB(setVelocityScale, float);
  IF_STUB(setVelocityScaleMinMax, Point2);
  IF_STUB(setWindScale, float);
};

class BaseParticleFxEmitter : public BaseEffectInterface
{
public:
  BaseParticleFxEmitter() : refCounter(0) {}
  BaseParticleFxEmitter(const BaseParticleFxEmitter &) = default;
  BaseParticleFxEmitter(BaseParticleFxEmitter &&) = default;
  BaseParticleFxEmitter &operator=(const BaseParticleFxEmitter &) = default;
  BaseParticleFxEmitter &operator=(BaseParticleFxEmitter &&) = default;


  void addRef() { interlocked_increment(refCounter); }
  void delRef()
  {
    if (refCounter <= 0)
    {
      G_ASSERT(refCounter > 0);
      return;
    }
    if (interlocked_decrement(refCounter) == 0)
      delete this;
  }


  virtual void emitPoint(Point3 &pos, Point3 &normal, Point3 &vel, real u, real v, real w) = 0;

  virtual void getEmitterBoundingSphere(Point3 &center, real &radius) = 0;

  virtual bool isSubOf(unsigned id) { return id == HUID_BaseParticleFxEmitter || BaseParamScript::isSubOf(id); }

protected:
  volatile int refCounter;

  virtual ~BaseParticleFxEmitter()
  {
    if (refCounter != 0)
    {
      G_ASSERT(refCounter == 0);
    }
  }
};


class IEffectRayTracer
{
public:
  virtual bool traceRay(const Point3 &pos, const Point3 &dir, float &dist, Point3 *out_normal = NULL) = 0;
  virtual bool traceRay(const Point3 &pos, const Point3 &dir, float &dist, Point3 *out_normal, int *out_pmid)
  {
    if (out_pmid)
      *out_pmid = -1;
    return traceRay(pos, dir, dist, out_normal);
  }
};


class IBloodyBaker
{
public:
  virtual void invoke(const Point3 &pos, const Point3 &uvec, const Point3 &vvec, const Point2 &texCoord0, const Point2 &texCoord1,
    E3DCOLOR color, int texture) = 0;
};

class ISoundPlayer
{
public:
  virtual void playSound3d(const Point3 &pos) = 0;
  virtual void playSound3dByMaterial(const Point3 &pos, int pmid) = 0;
};

class BaseEffectObject : public BaseEffectInterface
{
public:
  virtual void update(float dt) = 0;

  virtual void render(unsigned render_type_id, const TMatrix &view_itm) = 0;
  virtual bool getResult(unsigned /*render_type_id*/) { return true; }

  virtual void onDeviceReset() {}

  virtual bool isSubOf(unsigned id) { return id == HUID_BaseEffectObject || BaseParamScript::isSubOf(id); }

  virtual int getShaderId() const { return -1; }

  virtual void drawEmitter(const Point3 &) = 0;

  int res_id;
};


class BaseParticleEffect : public BaseEffectObject
{
public:
  virtual void spawnParticles(BaseParticleFxEmitter *emitter, real amount) = 0;
  virtual void drawEmitter(const Point3 &) {}
  virtual bool isSubOf(unsigned id) { return id == HUID_BaseParticleEffect || BaseEffectObject::isSubOf(id); }
};


class BaseTrailEffect : public BaseEffectObject
{
public:
  virtual int getNewTrailId(const Color4 &color, int sub_tex_id) = 0;

  virtual void addTrailPoint(int trail_id, const Point3 &pos, const Point3 &vel, const Point3 &dir) = 0;
  virtual void addTrailPoint(const Point3 & /*pos*/, const Point3 & /*vel*/, const Point3 & /*dir*/) {}

  virtual void fadeTrail(int /*trail_id*/, real /*time*/) {}
  virtual void fitTrailTexture() {}

  virtual void drawEmitter(const Point3 &) {}

  virtual bool isSubOf(unsigned id) { return id == HUID_BaseTrailEffect || BaseEffectObject::isSubOf(id); }
};


class BaseArcEffect : public BaseEffectObject
{
public:
  virtual void spawnArcLine(const Point3 &from_pos, const Point3 &from_vel, const Point3 &to_pos, float radius, const Color4 &color,
    int sub_tex_id) = 0;

  virtual void drawEmitter(const Point3 &) {}
  virtual bool isSubOf(unsigned id) { return id == HUID_BaseArcEffect || BaseEffectObject::isSubOf(id); }
};


class BaseEffectFactory : public ParamScriptFactory
{
public:
  virtual bool isSubOf(unsigned id) { return id == HUID_BaseEffectObject || ParamScriptFactory::isSubOf(id); }
};


void register_effect_factory(BaseEffectFactory *);
