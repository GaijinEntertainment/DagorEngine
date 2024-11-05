// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <de3_roadsProvider.h>
#include <generic/dag_DObject.h>
#include <generic/dag_tab.h>


class HmapLandObjectEditor;


class RoadsSnapshot : public IRoadsProvider::Roads, public DObject
{
public:
  RoadsSnapshot(HmapLandObjectEditor &objEd);
  virtual void release() { delRef(); }
  virtual void debugDump();

protected:
  Tab<IRoadsProvider::RoadSpline> rspl;
  Tab<IRoadsProvider::RoadCross> rc;
  Tab<IRoadsProvider::RoadCross::CrossRec> rcr;
  Tab<IRoadsProvider::RoadPt> rpt;
};
