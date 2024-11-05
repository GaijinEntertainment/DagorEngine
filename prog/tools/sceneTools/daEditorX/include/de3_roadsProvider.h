//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_bezier.h>

namespace splineclass
{
class AssetData;
}

class IRoadsProvider
{
public:
  static constexpr unsigned HUID = 0xA7CF0257u; // IRoadsProvider

  struct RoadPt
  {
    Point3 pt, relIn, relOut;
    const splineclass::AssetData *asset;
  };

  struct RoadSpline
  {
    dag::ConstSpan<RoadPt> pt;
    BezierSpline3d curve;
    bool closed;
  };

  struct RoadCross
  {
    struct CrossRec
    {
      const RoadSpline *road;
      int ptIdx;
    };

    dag::ConstSpan<CrossRec> roads;
  };

  class Roads
  {
  public:
    virtual void release() = 0;
    virtual void debugDump() = 0;
    inline unsigned getGenTimeStamp() { return generationTimestamp; }

  protected:
    unsigned generationTimestamp;

  public:
    dag::ConstSpan<RoadSpline> roads;
    dag::ConstSpan<RoadCross> cross;
  };

public:
  //! returns reference counted object with current roads data snapshot
  virtual Roads *getRoadsSnapshot() = 0;
};
