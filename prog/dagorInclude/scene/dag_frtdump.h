//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_smallTab.h>
#include <math/dag_mathBase.h>
#include <sceneRay/dag_sceneRayDecl.h>


// forward declarations of external classes
class IGenLoad;
struct Capsule;


class FastRtDump
{
public:
  enum
  {
    CUSTOM_Default,
    CUSTOM_Bullets,
    CUSTOM_Camera,
    CUSTOM_Character,
    CUSTOM_Character2,
    CUSTOM_Character3,

    CUSTOM__NUM
  };

  FastRtDump();
  FastRtDump(IGenLoad &crd);
  FastRtDump(StaticSceneRayTracer *_rt);
  ~FastRtDump() { unloadData(); }

  // pmid-oriented version of StaticSceneRayTracer interface
  inline int traceray(const Point3 &p, const Point3 &dir, real &t, int &out_pmid);
  inline int tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t, int &out_pmid);
  inline int clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized, int &out_pmid);
  // pmid-oriented version of StaticSceneRayTracer interface (also normal is returned)
  inline int traceray(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm);
  inline int tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t, int &out_pmid, Point3 &out_norm);
  inline int clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized, int &out_pmid,
    Point3 &out_norm);

  // Custom (usage flags) pmid-oriented version of StaticSceneRayTracer interface
  inline int traceray(int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid);
  inline int tracerayNormalized(int custom, const Point3 &p, const Point3 &normDir, real &t, int &out_pmid);
  inline int clipCapsule(int custom, const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
    int &out_pmid);

  // Custom (usage flags)  pmid-oriented version of StaticSceneRayTracer interface (also normal is returned)
  inline int traceray(int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm);
  inline int tracerayNormalized(int custom, const Point3 &p, const Point3 &normDir, real &t, int &out_pmid, Point3 &out_norm);
  inline int clipCapsule(int custom, const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
    int &out_pmid, Point3 &out_norm);

  // rayhit
  inline bool rayhitNormalized(const Point3 &p, const Point3 &norm_dir, real t);

  // custom rayhit
  inline bool rayhitNormalized(int custom, const Point3 &p, const Point3 &normDir, real t);

  // StaticSceneRayTracer for direct usage (when pmid not needed)
  inline StaticSceneRayTracer &getRt() const { return *rt; }

  inline int getFacePmid(int face_idx) const { return pmid[face_idx]; }

  // internal data managment
  void unloadData();
  void loadData(IGenLoad &crd);
  void setData(StaticSceneRayTracer *rt);

  inline bool isDataValid() const { return rt != NULL; }

  inline void setCustomFlags(int custom);
  inline void setDefaultFlags();

  // returns true if face_flags meet requirements for custom usage flags
  inline bool checkFaceFlags(int custom, int face_flags);

  void activate(bool active_);
  bool isActive() const { return active; }

protected:
  static int customUsageMask[CUSTOM__NUM];
  static int customSkipMask[CUSTOM__NUM];

  StaticSceneRayTracer *rt;
  SmallTab<unsigned char, MidmemAlloc> pmid;
  bool active;
};
