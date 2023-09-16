#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <lightFx_decl.h>
#include <math/dag_color.h>
#include <staticVisSphere.h>

enum
{
  HUID_ACES_RESET = 0xA781BF24u
};

class LightEffect : public BaseEffectObject
{
public:
  LightEffect() :
    time(0.f),
    globalTime(0.f),
    output(0.f, 0.f, 0.f, 0.f),
    overrideColor(1.f, 1.f, 1.f, 1.f),
    position(0.f, 0.f, 0.f),
    fadeoutCur(0.f),
    fadeoutMax(0.f),
    colorScale(1.f),
    radiusScale(1.f),
    lifetime(0.f)
  {}

  virtual void update(float dt)
  {
    time += dt;
    globalTime += dt;

    if (lifetime > 0 && globalTime >= lifetime)
    {
      output = Color4(0.f, 0.f, 0.f, 0.f);
      return;
    }

    if (time > par.phaseTime)
    {
      if (par.burstMode)
      {
        time = par.phaseTime + 1.f;
        output = Color4(0.f, 0.f, 0.f, 0.f);
        return;
      }
      else
      {
        time = 0;
      }
    }

    float v = time / par.phaseTime;

    Color4 src = color4(par.color.color);
    Color3 col = Color3::rgb(src) * src.a;

    col.r *= par.color.rFunc.sample(v);
    col.g *= par.color.gFunc.sample(v);
    col.b *= par.color.bFunc.sample(v);

    col *= par.color.aFunc.sample(v);
    col *= par.color.scale;

    float rad = par.size.radius;
    rad *= par.size.sizeFunc.sample(v);

    output = Color4(col.r, col.g, col.b, rad);
    output.clamp0();

    if (lifetime == 0 || globalTime > (lifetime - fadeoutMax))
    {
      if (fadeoutMax > 0)
      {
        fadeoutCur += dt;
        float f = 1.f - min(fadeoutCur / fadeoutMax, 1.f);
        output.r *= f;
        output.g *= f;
        output.b *= f;
      }
    }

    output.r *= colorScale;
    output.g *= colorScale;
    output.b *= colorScale;
    output.a *= radiusScale;
    output *= overrideColor;
  }

  virtual void drawEmitter(const Point3 &) {}

  const Color4 &getResultLight() const { return output; }

  virtual void render(unsigned /*rtype*/) {}

  virtual void setTm(const TMatrix *tm) { position = tm->getcol(3); }

  virtual void setEmitterTm(const TMatrix *tm) { setTm(tm); }

  virtual BaseParamScript *clone()
  {
    LightEffect *o = new LightEffect(*this);
    return o;
  }

  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb)
  {
    CHECK_FX_VERSION(ptr, len, 1);
    par.load(ptr, len, load_cb);

    StaticVisSphere staticVisibilitySphere;
    staticVisibilitySphere.load(ptr, len, load_cb);
  }

  void setColor4Mult(const Color4 *value) override
  {
    G_ASSERT_RETURN(value, );

    if (par.color.allow_game_override)
      overrideColor = *value;
  }

  virtual void setParam(unsigned id, void *value)
  {
    if (id == HUID_EMITTER_TM || id == HUID_TM)
    {
      setTm(static_cast<TMatrix *>(value));
    }
    else if (id == HUID_LIGHT_PARAMS)
    {
      time = 0.f;
      globalTime = 0.f;
      output = *static_cast<Color4 *>(value);
    }
    else if (id == _MAKE4C('LFXL'))
    {
      lifetime = *static_cast<float *>(value);
    }
    else if (id == _MAKE4C('LFXF'))
    {
      if (fadeoutMax == 0)
        fadeoutMax = *static_cast<float *>(value);
    }
    else if (id == _MAKE4C('LFXS'))
    {
      colorScale = value ? *(float *)value : 0;
    }
    else if (id == _MAKE4C('LFXR'))
    {
      radiusScale = value ? *(float *)value : 0;
    }
    else if (id == _MAKE4C('LFXO'))
    {
      par.color.allow_game_override = value ? *(bool *)value : false;
    }
    else if (id == HUID_COLOR4_MULT)
    {
      setColor4Mult((Color4 *)value);
    }
    else if (id == HUID_ACES_RESET)
    {
      time = 0;
      globalTime = 0;
      fadeoutCur = 0;
      fadeoutMax = 0;
    }
  }

  virtual void *getParam(unsigned id, void *value)
  {
    if (id == HUID_LIGHT_PARAMS)
      return &output;
    else if (id == HUID_LIGHT_POS)
      return &position;
    else if (id == HUID_EMITTER_TM || id == HUID_TM)
    {
      TMatrix *tm = (TMatrix *)value;
      *tm = TMatrix::IDENT;
      tm->setcol(3, position);
      return value;
    }
    else if (id == _MAKE4C('CACH')) // Does no rendering so is shader cache friendly.
    {
      return (void *)1; //-V566
    }
    else
      return NULL;
  }

private:
  float time;
  float globalTime;
  float lifetime;
  float fadeoutCur;
  float fadeoutMax;
  float colorScale;
  float radiusScale;
  Color4 output;
  Color4 overrideColor;
  Point3 position;
  LightFxParams par;
};

class LighFxFactory : public BaseEffectFactory
{
public:
  virtual BaseParamScript *createObject() { return new LightEffect(); }

  virtual void destroyFactory() {}

  virtual const char *getClassName() { return "LightFx"; }
};

static LighFxFactory factory;


void register_light_fx_factory() { register_effect_factory(&factory); }
