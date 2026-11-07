// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dummyObject.h"

namespace
{
struct RBHelperTraits
{
  static constexpr int nameId = IDS_RBHelper;
  static constexpr int rollupId = IDS_RBDUMMY_ROLL;
  static constexpr int paramDialogId = IDD_RBDUMMY_PARAM;

  static SClass_ID superClassId() { return HELPER_CLASS_ID; }
  static Class_ID classId() { return RBDummy_CID; }

  static Point3 axisColor(bool selected, int) { return selected ? Point3(1, 1, 1) : Point3(0.5f, 1.0f, 0.5f); }
};
} // namespace

static DummyClassDesc<RBHelperTraits> RBHelperCD;

ClassDesc *GetRBDummyCD() { return &RBHelperCD; }
