#include <math/dag_color.h>
#include <math/random/dag_random.h>
#include <shaders/dag_dynShaderBuf.h>
#include <3d/dag_render.h>
#include <generic/dag_tab.h>
#include <3d/dag_texMgr.h>
#include <gameRes/dag_gameResources.h>

#include <stdTrailPath.h>
#include <trailFlowFx_decl.h>
#include <stdEmitter.h>
#include <staticVisSphere.h>
#include "randvec.h"

// #include <debug/dag_debug.h>


class TrailFlowFxEffect : public BaseParticleEffect
{
public:
  TrailFlowFxParams par;

  Ptr<BaseParticleFxEmitter> emitter;
  StaticVisSphere staticVisibilitySphere;

  TMatrix fxTm;
  real fxScale;
  BBox3 bbox;

  class Trail : public StdTrailPath
  {
  public:
    Color3 color;
    real minV, maxV;
    real offsetU;
    uint8_t alpha;

    Point3 uvw;
    real life, maxLife;

    Point3 turb0, turb1;
    real turbK;


    Trail() :
      color(1, 1, 1),
      alpha(255),
      minV(0),
      maxV(1),
      life(-1),
      turbK(0),
      offsetU(0),
      uvw(0, 0, 0),
      maxLife(0),
      turb0(0, 0, 0),
      turb1(0, 0, 0)
    {}


    void setParams(const Color4 &c, int sub_tex_id, int num_frames, TrailFlowFxParams &par)
    {
      color.r = c.r;
      color.g = c.g;
      color.b = c.b;
      alpha = real2uchar(c.a);

      if (num_frames < 1)
        num_frames = 1;

      minV = sub_tex_id / real(num_frames);
      maxV = (sub_tex_id + 1) / real(num_frames);

      turb0 = normalize(_rnd_svec(par.seed));
      turb1 = normalize(_rnd_svec(par.seed));
      turbK = _frnd(par.seed);

      offsetU = 0;
    }


    void updateTurbulence(TrailFlowFxParams &par)
    {
      if (!isAlive())
        return;

      turb0 = turb1;
      turb1 = normalize(_rnd_svec(par.seed));
      real k = turb0 * turb1;
      if (k < 0)
        turb1 -= turb0 * (k * 2);
    }


    void addPoint(BaseParticleFxEmitter *em, TrailFlowFxParams &par, BBox3 &bb)
    {
      Point3 pos, norm, vel;
      em->emitPoint(pos, norm, vel, uvw.x, uvw.y, uvw.z);

      vel = vel * par.emitterVelK + normalize((turb1 - turb0) * ((3 - 2 * turbK) * turbK * turbK) + turb0) * par.turbVel +
            par.startVel + norm * par.normalVel;

      StdTrailPath::addPoint(pos, vel, Point3(0, 1, 0));

      bb += pos;
    }


    void render(TrailFlowFxParams &par, TrailFlowFxEffect &fx, EffectsInterface::StdParticleVertex *&buf_ptr, int &num_quads,
      int &total_quads)
    {
      real colorScale = EffectsInterface::getColorScale() * par.color.scale;

      START_STD_TRAIL_RENDER(this, buf_ptr, num_quads, total_quads)
      real lifeFactor = (curTime - p.time) / totalTimeLen;

      real radius = par.size.radius * par.size.sizeFunc.sample(lifeFactor) * fx.fxScale;

      Point3 pos = fx.fxTm * p.pos;
      Point3 displace = pos - fx.fxTm * prevPointPos;
      Point3 dir;
      if (displace.lengthSq() < 1e-6)
        dir = Point3(0, 0, 0);
      else
      {
        dir = normalize(displace % (pos - ::grs_cur_view.pos));
        if (dir.lengthSq() < 1e-6)
          dir = normalize(displace);
        dir *= radius;
      }

      Color4 fxColor(par.color.rFunc.sample(lifeFactor) * par.color.color.r, par.color.gFunc.sample(lifeFactor) * par.color.color.g,
        par.color.bFunc.sample(lifeFactor) * par.color.color.b, par.color.aFunc.sample(lifeFactor) * par.color.color.a);

      fxColor /= 255;
      fxColor.clamp01();

      if (par.shader.useColorMult)
      {
        fxColor.r *= colorScale * fx.colorMult.r;
        fxColor.g *= colorScale * fx.colorMult.g;
        fxColor.b *= colorScale * fx.colorMult.b;
        fxColor.a *= fx.colorMult.a;
      }
      else
      {
        fxColor.r *= colorScale;
        fxColor.g *= colorScale;
        fxColor.b *= colorScale;
      }

      if (isFxShaderWithFakeHdr(par.shader.shader))
        fxColor.a *= par.color.fakeHdrBrightness / 4.0f;

      fxColor.clamp01();
      E3DCOLOR vcolor = e3dcolor(fxColor);

      SET_STD_TRAIL_VERT_POS(pos, dir);
      real u = timeFactor * par.tile - offsetU;
      vert[0].tc = Point3(u, maxV, 0.f);
      vert[1].tc = Point3(u, minV, 0.f);
      vert[0].color = vcolor;
      vert[1].color = vcolor;
      END_STD_TRAIL_RENDER(this, buf_ptr, num_quads, total_quads)
    }
  };


  Tab<Trail> trails;

  Color4 colorMult;

  real spawnCounter, spawnRate, nominalRate;
  bool wasBurstSpawned;


  TrailFlowFxEffect() : spawnCounter(0), spawnRate(0), wasBurstSpawned(false), colorMult(1, 1, 1, 1), nominalRate(0), trails(midmem)
  {
    par.numTrails = 0;
    fxTm.identity();
    fxScale = 1;
    par.seed = grnd();
  }


  ~TrailFlowFxEffect() { release_gameres_or_texres((GameResource *)par.shader.texture); }


  virtual BaseParamScript *clone()
  {
    TrailFlowFxEffect *o = new TrailFlowFxEffect(*this);
    o->par.seed = grnd();
    if (emitter)
      o->emitter = (BaseParticleFxEmitter *)emitter->clone();
    acquire_managed_tex((TEXTUREID)(uintptr_t)par.shader.texture);
    return o;
  }


  void setNumTrails(int num)
  {
    trails.resize(num);
    for (int i = 0; i < trails.size(); ++i)
      trails[i].resize(par.numSegs, par.timeLength);

    if (par.life != 0)
      spawnRate = num / (par.life + par.timeLength);
    else
      spawnRate = 0;

    nominalRate = spawnRate;

    spawnCounter = 0;
  }


  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb)
  {
    CHECK_FX_VERSION(ptr, len, 2);

    int oldNum = par.numTrails;
    int oldSegs = par.numSegs;
    real oldLife = par.life;
    real oldTimeLen = par.timeLength;

    par.load(ptr, len, load_cb);

    if (par.numFrames < 1)
      par.numFrames = 1;

    staticVisibilitySphere.load(ptr, len, load_cb);

    emitter = new StdParticleEmitter;
    emitter->loadParamsData(ptr, len, load_cb);

    if (par.numTrails != oldNum || par.numSegs != oldSegs || par.life != oldLife || par.timeLength != oldTimeLen)
      setNumTrails(par.numTrails);

    wasBurstSpawned = false;
  }


  virtual void setParam(unsigned id, void *value)
  {
    if (id == HUID_TM)
    {
      fxTm = *(TMatrix *)value;
      fxScale = length(fxTm.getcol(0));
    }
    else if (id == HUID_EMITTER_TM && emitter)
      emitter->setParam(HUID_TM, value);
    else if (id == HUID_VELOCITY && emitter)
      emitter->setParam(id, value);
    else if (id == HUID_EMITTER)
      emitter = (BaseParticleFxEmitter *)value;
    else if (id == HUID_COLOR_MULT)
    {
      Color3 temp = *(Color3 *)value;
      colorMult.r = temp.r;
      colorMult.g = temp.g;
      colorMult.b = temp.b;
      colorMult.a = 1;
    }
    else if (id == HUID_COLOR4_MULT)
    {
      colorMult = *(Color4 *)value;
    }
    else if (id == HUID_SEED)
      par.seed = *(int *)value;
    else if (id == HUID_BURST)
    {
      par.burstMode = *(bool *)value;
      wasBurstSpawned = false;
    }
    else if (id == HUID_LIFE)
    {
      par.life = *(real *)value;
      if (par.life != 0)
        spawnRate = trails.size() / (par.life + par.timeLength);
      else
        spawnRate = 0;
      if (spawnRate > nominalRate)
        spawnRate = nominalRate;
    }
    else if (id == HUID_SPAWN_RATE)
      spawnRate = nominalRate * (*(real *)value);
  }


  virtual void *getParam(unsigned id, void *value)
  {
    if (id == HUID_TM)
      *(TMatrix *)value = fxTm;
    else if (id == HUID_EMITTER_TM && emitter)
      return emitter->getParam(HUID_TM, value);
    else if (id == HUID_VELOCITY && emitter)
      return emitter->getParam(id, value);
    else if (id == HUID_EMITTER)
      return emitter;
    else if (id == HUID_STATICVISSPHERE)
      *(BSphere3 *)value = BSphere3(staticVisibilitySphere.par.center, staticVisibilitySphere.par.radius);
    else if (id == HUID_BURST)
      return &par.burstMode;
    else if (id == HUID_BURST_DONE)
    {
      for (int i = 0; i < trails.size(); i++)
        if (trails[i].isAlive())
        {
          if (value)
            *((bool *)value) = false;
          return value;
        }

      if (value)
        *((bool *)value) = true;
      return value;
    }
    else if (id == HUID_COLOR_MULT)
    {
      Color3 temp;
      temp.r = colorMult.r;
      temp.g = colorMult.g;
      temp.b = colorMult.b;
      if (value)
        *(Color3 *)value = temp;
      return value;
    }
    else if (id == _MAKE4C('BBOX'))
    {
      G_ASSERT(value);
      *(BBox3 *)value = bbox;
      return value;
    }

    return NULL;
  }


  void spawnTrail(int i, BaseParticleFxEmitter *em, const Point3 &em_uvw)
  {
    Trail &p = trails[i];

    p.setParams(colorMult, _rnd(par.seed) % par.numFrames, par.numFrames, par);

    p.uvw = em_uvw;
    p.maxLife = p.life = par.life + _srnd(par.seed) * par.randomLife;

    p.addPoint(em, par, bbox);
  }


  bool spawnTrail(BaseParticleFxEmitter *em, const Point3 &em_uvw)
  {
    for (int i = 0; i < trails.size(); ++i)
    {
      Trail &p = trails[i];
      if (p.isAlive())
        continue;

      spawnTrail(i, em, em_uvw);
      return true;
    }

    return false;
  }


  virtual void spawnParts(BaseParticleFxEmitter *em, real amount)
  {
    if (!em || amount <= 0)
      return;

    spawnCounter += amount;

    while (spawnCounter >= 1)
    {
      if (!spawnTrail(em, _rnd_fvec(par.seed)))
        break;

      spawnCounter -= 1;
    }

    if (spawnCounter > 1)
      spawnCounter = 1;
  }


  void spawnParticles(BaseParticleFxEmitter *em, real amount) { spawnParts(em, amount * par.amountScale); }


  virtual void update(real dt)
  {
    bbox.setempty();

    if (!par.burstMode)
    {
      spawnParts(emitter, spawnRate * dt);
    }
    else if (emitter && !wasBurstSpawned)
    {
      wasBurstSpawned = true;

      for (int i = 0; i < trails.size(); ++i)
      {
        spawnTrail(i, emitter, _rnd_fvec(par.seed));
      }
    }

    real vk = 1 - par.viscosity * dt;
    if (vk < 0)
      vk = 0;

    real deltaU = par.scrollSpeed * dt;

    for (int ti = 0; ti < trails.size(); ++ti)
    {
      Trail &tr = trails[ti];

      FOR_EACH_TRAIL_POINT(&tr, p)
      p.pos += p.vel * dt;
      bbox += p.pos;
      p.vel *= vk;
      p.vel.y -= par.gravity * dt;
      END_FOR_EACH_TRAIL_POINT(&tr)

      if (tr.life > 0)
      {
        tr.life -= dt;

        tr.turbK += par.turbFreq * dt;
        if (tr.turbK >= 1)
        {
          tr.turbK -= floorf(tr.turbK);
          tr.updateTurbulence(par);
        }

        if (emitter)
          tr.addPoint(emitter, par, bbox);
      }

      tr.updateTrail(dt);
      tr.offsetU += deltaU;
    }
  }


  virtual void render(unsigned rtype)
  {
    if (rtype == FX_RENDER_TRANS)
    {
      // TMatrix vtm;
      // EffectsInterface::getViewToWorldTm(vtm);

      // DynShaderMeshBuf *renderBuffer=EffectsInterface::getStdRenderBuffer();

      int totalQuads = 0;
      for (int i = 0; i < trails.size(); ++i)
        totalQuads += trails[i].getQuadCount();

      if (totalQuads)
      {
        EffectsInterface::startStdRender();

        EffectsInterface::setStdParticleShader(par.shader.shader);
        EffectsInterface::setStdParticleTexture((TEXTUREID)(uintptr_t)par.shader.texture);

        EffectsInterface::setLightingParams(par.ltPower, par.sunLtPower, par.ambLtPower);

        EffectsInterface::StdParticleVertex *ptr;
        int numQuads = EffectsInterface::getMaxQuadCount();
        if (numQuads > totalQuads)
          numQuads = totalQuads;

        EffectsInterface::beginQuads(&ptr, numQuads);

        for (int i = 0; i < trails.size(); ++i)
          trails[i].render(par, *this, ptr, numQuads, totalQuads);

        EffectsInterface::endStdRender();

        G_ASSERT(numQuads == 0 && totalQuads == 0);
      }
    }
  }
};


class TrailFlowFxFactory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new TrailFlowFxEffect(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "TrailFlowFx"; }
};

static TrailFlowFxFactory factory;


void register_trail_flow_fx_factory() { register_effect_factory(&factory); }
