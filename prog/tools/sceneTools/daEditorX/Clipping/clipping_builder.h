// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_mesh.h>
#include <3d/dag_materialData.h>
#include <util/dag_string.h>

class IGenSave;
struct Capsule;
class StaticGeometryMesh;
class Point3;

struct CollisionBuildSettings
{
  float leafSz[3];
  int levels;
  float gridStep, minMutualOverlap, minSmallOverlap;
  int minFaceCnt;

  CollisionBuildSettings() { defaults(); }

  void defaults()
  {
    leafSz[0] = 4;
    leafSz[1] = 8;
    leafSz[2] = 4;
    levels = 7;

    gridStep = 50.0;
    minMutualOverlap = 0.95;
    minSmallOverlap = 0.1;
    minFaceCnt = 8;
  }

  const Point3 &leafSize() const { return reinterpret_cast<const Point3 &>(leafSz); }
  Point3 &leafSize() { return reinterpret_cast<Point3 &>(leafSz); }
};


class IClippingDumpBuilder
{
public:
  virtual void destroy() = 0;

  // collision data building
  virtual void start(const CollisionBuildSettings &stg) = 0;

  // optional primitives (can be unsupported, check supportMask)
  virtual void addCapsule(Capsule &, int obj_id, int obj_flags, const char *physmat) = 0;
  virtual void addSphere(const Point3 &c, float rad, int obj_id, int obj_flags, const char *pm) = 0;
  virtual void addBox(const TMatrix &box_tm, int obj_id, int obj_flags, const char *physmat) = 0;
  virtual void addConvexHull(Mesh &m, MaterialDataList *mat, TMatrix &wtm, int obj_id, int obj_flags, int physmat_id, bool pmid_force,
    StaticGeometryMesh *sgm = NULL) = 0;

  // triangle mesh (must be supported)
  virtual void addMesh(Mesh &m, MaterialDataList *mat, const TMatrix &wtm, int obj_id, int obj_flags, int physmat_id, bool pmid_force,
    StaticGeometryMesh *sgm = NULL) = 0;

  virtual bool finishAndWrite(const char *temp_fname, IGenSave &cwr, unsigned target_code) = 0;

  // can return separate ray tracer object that it owns
  // application can use this pointer to call finishAndWrite()
  virtual IClippingDumpBuilder *getSeparateRayTracer() = 0;

public:
  enum
  {
    SUPPORT_BOX = 0x01,
    SUPPORT_SPHERE = 0x02,
    SUPPORT_CAPSULE = 0x04,
    SUPPORT_CONVEX = 0x08,
    SUPPORT_MESH = 0x10,
  };
  unsigned supportMask;

  IClippingDumpBuilder() : supportMask(0) {}
};


// standard dump builders
IClippingDumpBuilder *create_dagor_raytracer_dump_builder();
IClippingDumpBuilder *create_bullet_clipping_dump_builder(bool need_bt_data);

static bool getPhysMatNameFromMatName(MaterialData *m, String &s)
{
  if (m && m->matName)
  {
    const char *adog = ::strrchr(m->matName, '@');
    if (adog)
    {
      s = (adog + 1);
      return true;
    }
  }

  return false;
}


static bool getPhysMatNameFromMatName(const char *name, String &s)
{
  const char *adog = ::strrchr(name, '@');
  if (adog)
  {
    s = (adog + 1);
    return true;
  }

  return false;
}
