// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <fx/dag_baseFxClasses.h>

#include <stdEmitter_decl.h>

extern int softness_distance_var_id;
extern int blending_type_c_no;

class StdParticleEmitter : public BaseParticleFxEmitter
{
public:
  StdEmitterParams par;

  TMatrix emitterTm;
  Matrix3 normTm;

  Point3 emitterVel;

  enum
  {
    GEOM_SPHERE = 0,
    GEOM_CYLINDER,
    GEOM_BOX,
  };


  StdParticleEmitter() : emitterVel(0, 0, 0)
  {
    emitterTm.identity();
    normTm.identity();
  }

  virtual BaseParamScript *clone()
  {
    StdParticleEmitter *emitter = new StdParticleEmitter(*this);
    emitter->refCounter = 0;
    return emitter;
  }

  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb) { par.load(ptr, len, load_cb); }

  virtual void setParam(unsigned id, void *value)
  {
    if (id == HUID_TM)
      setTm((TMatrix *)value);
    else if (id == HUID_VELOCITY)
      setVelocity((Point3 *)value);
  }

  // fast setters

  virtual void setTm(const TMatrix *value)
  {
    emitterTm = *value;
    normTm.setcol(0, normalize(emitterTm.getcol(0)));
    normTm.setcol(1, normalize(emitterTm.getcol(1)));
    normTm.setcol(2, normalize(emitterTm.getcol(2)));
  }
  virtual void setVelocity(const Point3 *value) { emitterVel = *value; }

  virtual void *getParam(unsigned id, void *value)
  {
    if (id == HUID_TM)
      *(TMatrix *)value = emitterTm;
    else if (id == HUID_VELOCITY)
      *(Point3 *)value = emitterVel;

    return NULL;
  }

  virtual void emitPoint(Point3 &out_pos, Point3 &out_normal, Point3 &vel, real u, real v, real w)
  {
    Point3 pos, normal;

    if (par.geometry == GEOM_SPHERE)
    {
      real a = TWOPI * u;
      real c, s;
      ::sincos(a, s, c);
      real r = sqrtf(v * (1 - v)) * 2;

      Point3 dir(c * r, s * r, 1 - 2 * v);

      if (par.isVolumetric)
        pos = dir * (par.radius * sqrtf(w));
      else
        pos = dir * par.radius;

      normal = dir;
    }
    else if (par.geometry == GEOM_CYLINDER)
    {
      real a = TWOPI * u;
      real c, s;
      ::sincos(a, s, c);

      Point3 dir(c, 0, s);

      if (par.isVolumetric)
        pos = dir * (par.radius * sqrtf(v)) + Point3(0, (w - 0.5f) * par.height, 0);
      else
        pos = dir * par.radius + Point3(0, (v - 0.5f) * par.height, 0);

      normal = dir;
    }
    else
    {
      G_ASSERT(par.geometry == GEOM_BOX);
      pos = Point3((u - 0.5f) * par.width, (v - 0.5f) * par.height, (w - 0.5f) * par.depth);
      normal = Point3(0, 1, 0);
    }

    out_pos = emitterTm * pos;
    out_normal = normTm * normal;

    vel = emitterVel;
  }

  virtual void getEmitterBoundingSphere(Point3 &center, real &radius)
  {
    center = emitterTm.getcol(3);
    radius = par.radius;

    //== handle cylinder and box
  }
};
