// clang-format off  // generated text, do not modify!
#pragma once
#include "readType.h"

#include <math/dag_curveParams.h>
#include <fx/dag_paramScript.h>


#include <fx/dag_baseFxClasses.h>


namespace ScriptHelpers
{
class TunedElement;
};

#include <dafxEmitter_decl.h>

class DafxLinearDistribution
{
public:
  real start;
  real end;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    start = readType<real>(ptr, len);
    end = readType<real>(ptr, len);
  }
};

class DafxRectangleDistribution
{
public:
  Point2 leftTop;
  Point2 rightBottom;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    leftTop = readType<Point2>(ptr, len);
    rightBottom = readType<Point2>(ptr, len);
  }
};

class DafxCubeDistribution
{
public:
  Point3 center;
  Point3 size;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    center = readType<Point3>(ptr, len);
    size = readType<Point3>(ptr, len);
  }
};

class DafxSphereDistribution
{
public:
  Point3 center;
  real radius;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    center = readType<Point3>(ptr, len);
    radius = readType<real>(ptr, len);
  }
};

class DafxSectorDistribution
{
public:
  Point3 center;
  Point3 scale;
  real radiusMin;
  real radiusMax;
  real azimutMax;
  real azimutNoise;
  real inclinationMin;
  real inclinationMax;
  real inclinationNoise;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    center = readType<Point3>(ptr, len);
    scale = readType<Point3>(ptr, len);
    radiusMin = readType<real>(ptr, len);
    radiusMax = readType<real>(ptr, len);
    azimutMax = readType<real>(ptr, len);
    azimutNoise = readType<real>(ptr, len);
    inclinationMin = readType<real>(ptr, len);
    inclinationMax = readType<real>(ptr, len);
    inclinationNoise = readType<real>(ptr, len);
  }
};

class DafxSparksSimParams
{
public:
  DafxCubeDistribution pos;
  DafxLinearDistribution width;
  DafxLinearDistribution life;
  int seed;
  DafxSectorDistribution velocity;
  real restitution;
  real friction;
  E3DCOLOR color0;
  E3DCOLOR color1;
  real color1Portion;
  E3DCOLOR colorEnd;
  real velocityBias;
  real dragCoefficient;
  DafxLinearDistribution turbulenceForce;
  DafxLinearDistribution turbulenceFreq;
  Point3 liftForce;
  real liftTime;
  real spawnNoise;
  DafxCubeDistribution spawnNoisePos;
  real hdrScale1;
  real windForce;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 6);

    pos.load(ptr, len, load_cb);
    width.load(ptr, len, load_cb);
    life.load(ptr, len, load_cb);
    seed = readType<int>(ptr, len);
    velocity.load(ptr, len, load_cb);
    restitution = readType<real>(ptr, len);
    friction = readType<real>(ptr, len);
    color0 = readType<E3DCOLOR>(ptr, len);
    color1 = readType<E3DCOLOR>(ptr, len);
    color1Portion = readType<real>(ptr, len);
    colorEnd = readType<E3DCOLOR>(ptr, len);
    velocityBias = readType<real>(ptr, len);
    dragCoefficient = readType<real>(ptr, len);
    turbulenceForce.load(ptr, len, load_cb);
    turbulenceFreq.load(ptr, len, load_cb);
    liftForce = readType<Point3>(ptr, len);
    liftTime = readType<real>(ptr, len);
    spawnNoise = readType<real>(ptr, len);
    spawnNoisePos.load(ptr, len, load_cb);
    hdrScale1 = readType<real>(ptr, len);
    windForce = readType<real>(ptr, len);
  }
};

class DafxSparksRenParams
{
public:
  int blending;
  real motionScale;
  real hdrScale;
  real arrowShape;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 3);

    blending = readType<int>(ptr, len);
    motionScale = readType<real>(ptr, len);
    hdrScale = readType<real>(ptr, len);
    arrowShape = readType<real>(ptr, len);
  }
};

class DafxSparksGlobalParams
{
public:
  real spawn_range_limit;
  int max_instances;
  int player_reserved;
  real emission_min;
  real one_point_number;
  real one_point_radius;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    spawn_range_limit = readType<real>(ptr, len);
    max_instances = readType<int>(ptr, len);
    player_reserved = readType<int>(ptr, len);
    emission_min = readType<real>(ptr, len);
    one_point_number = readType<real>(ptr, len);
    one_point_radius = readType<real>(ptr, len);
  }
};

class DafxSparksQuality
{
public:
  bool low_quality;
  bool medium_quality;
  bool high_quality;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    low_quality = readType<int>(ptr, len);
    medium_quality = readType<int>(ptr, len);
    high_quality = readType<int>(ptr, len);
  }
};

class DafxRenderGroup
{
public:
  int type;


  static ScriptHelpers::TunedElement *createTunedElement(const char *name);

  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)
  {
    G_UNREFERENCED(load_cb);
    CHECK_FX_VERSION(ptr, len, 1);

    type = readType<int>(ptr, len);
  }
};
