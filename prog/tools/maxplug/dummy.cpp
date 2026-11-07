// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dummyObject.h"

namespace
{
struct LinkDummyTraits
{
  static constexpr int nameId = IDS_Dummy;
  static constexpr int rollupId = IDS_DUMMY_ROLL;
  static constexpr int paramDialogId = IDD_DUMMY_PARAM;

  static SClass_ID superClassId() { return GEOMOBJECT_CLASS_ID; }
  static Class_ID classId() { return Dummy_CID; }

  static Point3 axisColor(bool selected, int axis)
  {
    if (selected)
      return Point3(axis == 0 ? 1 : 0.5f, axis == 1 ? 1 : 0.5f, axis == 2 ? 1 : 0.5f);
    return Point3(axis == 0 ? 0.7f : 0, axis == 1 ? 0.7f : 0, axis == 2 ? 0.7f : 0);
  }
};
} // namespace

static DummyClassDesc<LinkDummyTraits> LinkDummyCD;

ClassDesc *GetDummyCD() { return &LinkDummyCD; }
