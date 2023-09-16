#include <math/dag_math3d.h>
#include <math/random/dag_random.h>
#include <math/dag_color.h>
#include <generic/dag_DObject.h>
#include <generic/dag_span.h>
#include <math/dag_curveParams.h>
#include <fx/dag_fxInterface.h>
#include <fx/dag_baseFxClasses.h>
#include <gameRes/dag_gameResources.h>
#include <3d/dag_texMgr.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>

#include <animPlanes2Fx_decl.h>
#include <stdEmitter.h>
#include <staticVisSphere.h>

class AnimPlanes2FxEffect : public BaseEffectObject
{
public:
  enum
  {
    FACING_FACING,
    FACING_AXIS,
    FACING_FREE,
  };

  enum
  {
    TIME_REPEAT,
    TIME_CONSTANT,
    TIME_OFFSET,
    TIME_EXTRAPOLATE,
  };

  AnimPlanes2FxParams par;

  Ptr<BaseParticleFxEmitter> emitter;
  StaticVisSphere staticVisibilitySphere;

  TMatrix fxTm;
  real fxScale;

  Color3 colorMult;


  struct AnimPlane2
  {
    real curTime, endTime;
    TMatrix tm;

    real rollOfs, rollSign;

    real radiusTimeOfs, alphaTimeOfs, brightnessTimeOfs, rgbTimeOfs, rotTimeOfs, rollTimeOfs, scaleXTimeOfs, scaleYTimeOfs, posTimeOfs;


    AnimPlane2() : curTime(0), endTime(-2) {}

    bool isAlive() const { return endTime > 0; }
  };

  SmallTab<AnimPlane2> parts;
  SmallTab<StandardFxParticle> renderParts;
  SmallTab<int> partInd;

  virtual void drawEmitter(const Point3 &) {}

  AnimPlanes2FxEffect() : colorMult(1, 1, 1)
  {
    fxTm.identity();
    fxScale = 1;
    par.seed = grnd();
  }


  ~AnimPlanes2FxEffect() { release_gameres_or_texres((GameResource *)par.shader.texture); }


  static real evalCurve(CubicCurveSampler &func, AnimPlane2CurveTime &time_params, real time, real ofs)
  {
    time *= time_params.timeScale;
    time -= (time_params.maxTimeOffset - time_params.minTimeOffset) * ofs + time_params.minTimeOffset;

    if (time_params.outType == TIME_REPEAT)
    {
      time -= real2int(time - 0.5f);
      return func.sample(time);
    }
    else if (time_params.outType == TIME_CONSTANT)
    {
      if (time <= 0)
        return func.sample(0);
      else if (time >= 1)
        return func.sample(1);
      else
        return func.sample(time);
    }
    else if (time_params.outType == TIME_OFFSET)
    {
      int step = real2int(time - 0.5f);
      time -= step;
      return func.sample(time) + (func.sample(1) - func.sample(0)) * step;
    }
    else
      return func.sample(time);
  }


  static real evalScalar(AnimPlane2Scalar &scalar, real time, real ofs)
  {
    return evalCurve(scalar.func, scalar.time, time, ofs) * scalar.scale;
  }


  E3DCOLOR evalColor(AnimPlane2Color &color, real time, real a_ofs, real b_ofs, real rgb_ofs)
  {
    real br = evalCurve(color.brightness.func, color.brightness.time, time, b_ofs);

    Color4 c(evalCurve(color.rgb.rFunc, color.rgb.time, time, rgb_ofs) * color.color.r,
      evalCurve(color.rgb.gFunc, color.rgb.time, time, rgb_ofs) * color.color.g,
      evalCurve(color.rgb.bFunc, color.rgb.time, time, rgb_ofs) * color.color.b,
      evalCurve(color.alpha.func, color.alpha.time, time, a_ofs) * color.color.a);

    c /= 255;
    c.clamp01();

    real colorScale = EffectsInterface::getColorScale() * br;

    if (par.shader.useColorMult)
    {
      c.r *= colorScale * colorMult.r;
      c.g *= colorScale * colorMult.g;
      c.b *= colorScale * colorMult.b;
    }
    else
    {
      c.r *= colorScale;
      c.g *= colorScale;
      c.b *= colorScale;
    }

    if (isFxShaderWithFakeHdr(par.shader.shader))
      c.a *= color.fakeHdrBrightness / 4.0f;

    c.clamp01();

    return e3dcolor(c);
  }


  virtual BaseParamScript *clone()
  {
    AnimPlanes2FxEffect *o = new AnimPlanes2FxEffect(*this);
    o->par.seed = grnd();
    if (emitter)
      o->emitter = (BaseParticleFxEmitter *)emitter->clone();
    acquire_managed_tex((TEXTUREID)(uintptr_t)par.shader.texture);
    return o;
  }


  void setNumParts(int num)
  {
    parts.resize(num);
    renderParts.resize(num);

    partInd.clear();
  }


  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb)
  {
    CHECK_FX_VERSION(ptr, len, 2);

    par.load(ptr, len, load_cb);

    staticVisibilitySphere.load(ptr, len, load_cb);

    emitter = new StdParticleEmitter;
    emitter->loadParamsData(ptr, len, load_cb);

    setNumParts(par.array.size());

    for (unsigned int partNo = 0; partNo < par.array.size(); partNo++)
    {
      if (par.array[partNo].framesX < 1)
        par.array[partNo].framesX = 1;
      if (par.array[partNo].framesY < 1)
        par.array[partNo].framesY = 1;
    }
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
    else if (id == HUID_EMITTER)
      emitter = (BaseParticleFxEmitter *)value;
    else if (id == HUID_STATICVISSPHERE)
      *(BSphere3 *)value = BSphere3(staticVisibilitySphere.par.center, staticVisibilitySphere.par.radius);
    else if (id == HUID_COLOR_MULT)
      colorMult = *(Color3 *)value;
    else if (id == HUID_SEED)
      par.seed = *(int *)value;
  }

  virtual void *getParam(unsigned id, void *value)
  {
    if (id == HUID_TM)
      *(TMatrix *)value = fxTm;
    else if (id == HUID_EMITTER_TM && emitter)
      return emitter->getParam(HUID_TM, value);
    else if (id == HUID_EMITTER)
      return emitter;
    else if (id == HUID_COLOR_MULT)
    {
      if (value)
        *(Color3 *)value = colorMult;
      return value;
    }

    return NULL;
  }


  void spawnPart(AnimPlane2 &p, AnimPlane2Params &pp, real delay)
  {
    if (emitter)
    {
      Point3 pos, norm, vel;
      emitter->emitPoint(pos, norm, vel, _frnd(par.seed), _frnd(par.seed), _frnd(par.seed));

      if (pp.dirFromEmitter)
      {
        real a = _frnd(par.seed) * PI;
        p.tm = makeTM(quat_rotation_arc(Point3(0, 1, 0), norm) * Quat(0, sinf(a), 0, cosf(a)));
      }
      else
        p.tm.identity();

      if (pp.posFromEmitter)
        p.tm.setcol(3, pos);

      p.curTime = -delay;
      p.endTime = pp.life.life + pp.life.lifeVar * _srnd(par.seed);

      p.radiusTimeOfs = _frnd(par.seed);
      p.alphaTimeOfs = _frnd(par.seed);
      p.brightnessTimeOfs = _frnd(par.seed);
      p.rgbTimeOfs = _frnd(par.seed);
      p.rotTimeOfs = _frnd(par.seed);
      p.rollTimeOfs = _frnd(par.seed);
      p.scaleXTimeOfs = _frnd(par.seed);
      p.scaleYTimeOfs = _frnd(par.seed);
      p.posTimeOfs = _frnd(par.seed);

      p.rollOfs = pp.rollOffset + pp.rollOffsetVar * _srnd(par.seed);

      if (pp.randomRollSign)
        p.rollSign = (_rnd(par.seed) & 2) - 1;
      else
        p.rollSign = 1;
    }
  }


  virtual void update(real dt)
  {
    for (int i = 0; i < parts.size(); ++i)
    {
      AnimPlane2 &p = parts[i];
      AnimPlane2Params &pp = par.array[i];

      if (p.isAlive())
      {
        p.curTime += dt;
        if (p.curTime >= p.endTime)
        {
          if (pp.life.repeated)
          {
            spawnPart(p, pp, pp.life.repeatDelay + pp.life.repeatDelayVar * _srnd(par.seed) + p.endTime - p.curTime);
          }
          else
            p.endTime = -1;
        }
      }
      else
      {
        if (p.endTime == -2)
          spawnPart(p, pp, pp.life.delayBefore + pp.life.delayBeforeVar * _srnd(par.seed));
      }
    }
  }


  virtual void render(unsigned rtype)
  {
    if (rtype == FX_RENDER_TRANS)
    {
      TMatrix vtm;
      EffectsInterface::getViewToWorldTm(vtm);

      TMatrix4 globTm;
      if (par.shader.sorted)
        EffectsInterface::getGlobTm(globTm);

      TMatrix emTm;
      if (emitter)
      {
        emitter->getParam(HUID_TM, &emTm);
        emTm = fxTm * emTm;
      }
      else
        emTm = fxTm;

      if (par.shader.sorted)
      {
        if (partInd.size() != parts.size())
        {
          partInd.resize(parts.size());
          for (int i = 0; i < partInd.size(); ++i)
            partInd[i] = i;
        }
      }

      for (int i = 0; i < parts.size(); ++i)
      {
        AnimPlane2 &p = parts[i];
        AnimPlane2Params &pp = par.array[i];
        StandardFxParticle &rp = renderParts[i];

        int totalFrames = pp.framesX * pp.framesY;
        real stepU = 1.f / pp.framesX;
        real stepV = 1.f / pp.framesY;

        if (!p.isAlive() || p.curTime < 0)
        {
          rp.hide();
          continue;
        }

        if (pp.moveWithEmitter && !emitter)
        {
          rp.hide();
          continue;
        }

        real time = p.curTime / p.endTime;

        real radius = evalScalar(pp.size, time, p.radiusTimeOfs) * 0.5f * fxScale;
        if (radius <= 0)
        {
          rp.hide();
          continue;
        }

        TMatrix tm;
        if (pp.moveWithEmitter)
          tm = emTm * p.tm;
        else
          tm = fxTm * p.tm;

        rp.pos = tm * (pp.posOffset + Point3(evalScalar(pp.posX, time, p.posTimeOfs), evalScalar(pp.posY, time, p.posTimeOfs),
                                        evalScalar(pp.posZ, time, p.posTimeOfs)));

        if (float_nonzero(pp.viewZOffset))
          rp.pos += normalize(vtm.getcol(3) - rp.pos) * pp.viewZOffset;

        if (par.shader.sorted)
          rp.calcSortOrder(rp.pos, globTm);
        else
          rp.show();

        rp.color = evalColor(pp.color, time, p.alphaTimeOfs, p.brightnessTimeOfs, p.rgbTimeOfs);

        real roll = (evalScalar(pp.roll, time, p.rollTimeOfs) * p.rollSign + p.rollOfs) * TWOPI;

        if (pp.facingType == FACING_FACING)
        {
          rp.setFacingRotated(vtm.getcol(0), vtm.getcol(1), radius, roll);
        }
        else
        {
          real rx = evalScalar(pp.rotationX, time, p.rotTimeOfs) * TWOPI;
          real ry = evalScalar(pp.rotationY, time, p.rotTimeOfs) * TWOPI;

          Point3 xvec, yvec;

          if (pp.facingType == FACING_AXIS)
          {
            Matrix3 rtm = rotxM3(rx) * rotzM3(ry);

            yvec = (rtm * tm.getcol(1)) * radius;
            xvec = normalize((rp.pos - vtm.getcol(3)) % yvec) * radius;
          }
          else
          {
            Matrix3 rtm = rotxM3(rx) * rotzM3(ry);

            xvec = (rtm * tm.getcol(0)) * radius;
            yvec = (rtm * tm.getcol(2)) * radius;
          }

          real ca = cosf(roll), sa = sinf(roll);

          rp.uvec = xvec * ca + yvec * sa;
          rp.vvec = yvec * ca - xvec * sa;
        }

        rp.uvec *= evalScalar(pp.scaleX, time, p.scaleXTimeOfs);
        rp.vvec *= evalScalar(pp.scaleY, time, p.scaleYTimeOfs);

        rp.tcu = Point2(pp.tc.uSize / 1024, 0);
        rp.tcv = Point2(0, pp.tc.vSize / 1024);
        rp.tco = Point2(pp.tc.uOffset / 1024, pp.tc.vOffset / 1024);

        int frame = real2int(totalFrames * time * pp.animSpeed - 0.5f);
        frame %= totalFrames;
        if (frame < 0)
          frame += totalFrames;

        rp.setFrame(frame % pp.framesX, frame / pp.framesX, stepU, stepV);
      }

      EffectsInterface::setStdParticleShader(par.shader.shader);
      EffectsInterface::setStdParticleTexture((TEXTUREID)(uintptr_t)par.shader.texture);

      EffectsInterface::setLightingParams(par.ltPower, par.sunLtPower, par.ambLtPower);
      ShaderGlobal::set_real(::get_shader_variable_id("color_scale", true), 1);

      EffectsInterface::renderStdFlatParticles(&renderParts[0], renderParts.size(), par.shader.sorted ? &partInd[0] : NULL);
    }
  }
};


class AnimPlanes2FxFactory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new AnimPlanes2FxEffect(); }

  virtual void destroyFactory() {}
  virtual const char *getClassName() { return "AnimPlanes2Fx"; }
};

static AnimPlanes2FxFactory factory;


void register_anim_planes_fx_2_factory() { register_effect_factory(&factory); }
