#ifndef __GAIJIN_DAGORED_DE_DAGOR_PHYS_IMPL__
#define __GAIJIN_DAGORED_DE_DAGOR_PHYS_IMPL__
#pragma once


#include <oldEditor/pluginService/de_IDagorPhys.h>


class DeDagorPhys : public IDagorPhys
{
public:
  virtual StaticSceneRayTracer *loadBinaryRaytracer(IGenLoad &crd) const;

  virtual bool traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt) const;
  virtual bool traceRayStatic(const Point3 &p, const Point3 &dir, real &maxt, Point3 &norm) const;
  virtual bool rayHitStatic(const Point3 &p, const Point3 &dir, real maxt) const;

  virtual real clipCapsuleStatic(Capsule &c, Point3 &cap_pt, Point3 &world_pt) const;
  virtual real clipCapsuleStatic(Capsule &c, Point3 &cap_pt, Point3 &world_pt, Point3 &norm) const;

  virtual FastRtDump *getFastRtDump() const;

  virtual void initClippingBinary(StaticSceneRayTracer *rt) const;

  virtual void closeClipping() const;

  virtual BBox3 getBoundingBox() const;
};


#endif //__GAIJIN_DAGORED_DE_DAGOR_PHYS_IMPL__
