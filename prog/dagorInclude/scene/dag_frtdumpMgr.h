//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <scene/dag_frtdump.h>

// forward declarations of external classes
class IGenLoad;


class FastRtDumpManager
{
  Tab<FastRtDump *> rt;
  Tab<unsigned> rtId;
  int generation;

public:
  FastRtDumpManager(int max_dump_reserve = 32);
  ~FastRtDumpManager();


  int loadRtDump(IGenLoad &crd, unsigned rt_id);
  int addRtDump(StaticSceneRayTracer *rt, unsigned rt_id);
  void delRtDump(unsigned rt_id);

  void delAllRtDumps();

  // NOTE: true returned when collision/rayhit detected

  // native version of StaticSceneRayTracer interface
  bool rayhit(const Point3 &p, const Point3 &dir, real t);
  bool rayhitNormalized(const Point3 &p, const Point3 &normDir, real t);
  bool traceray(const Point3 &p, const Point3 &dir, real &t);
  bool tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t);
  bool clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized);

  // pmid-oriented version of StaticSceneRayTracer interface
  bool traceray(const Point3 &p, const Point3 &dir, real &t, int &out_pmid);
  bool tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t, int &out_pmid);
  bool clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized, int &out_pmid);
  // pmid-oriented version of StaticSceneRayTracer interface (also normal is returned)
  bool traceray(const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm);
  bool tracerayNormalized(const Point3 &p, const Point3 &normDir, real &t, int &out_pmid, Point3 &out_norm);
  bool clipCapsule(const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized, int &out_pmid,
    Point3 &out_norm);

  // Custom (usage flags) pmid-oriented version of StaticSceneRayTracer interface
  bool traceray(int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid);
  bool tracerayNormalized(int custom, const Point3 &p, const Point3 &normDir, real &t, int &out_pmid);
  bool clipCapsule(int custom, const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized, int &out_pmid);
  // Custom (usage flags)  pmid-oriented version of StaticSceneRayTracer interface (also normal is returned)
  bool traceray(int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid, Point3 &out_norm);
  bool tracerayNormalized(int custom, const Point3 &p, const Point3 &normDir, real &t, int &out_pmid, Point3 &out_norm);
  bool clipCapsule(int custom, const Capsule &cap, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized, int &out_pmid,
    Point3 &out_norm);

  // custom rayhit
  bool rayhit(int custom, const Point3 &p, const Point3 &dir, real t);
  bool rayhitNormalized(int custom, const Point3 &p, const Point3 &normDir, real t);

  inline dag::ConstSpan<FastRtDump *> getRt() const { return rt; }
  inline dag::ConstSpan<unsigned> getRtID() const { return rtId; }

protected:
  int allocRtDump();
};
