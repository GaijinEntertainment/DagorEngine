//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>

class Point3;
class String;
class FastRtDump;


class ICollision
{
public:
  static constexpr unsigned HUID = 0x87281C9Fu; // ICollision

  virtual bool compileCollisionWithDialog(bool for_game) = 0;
  virtual bool initCollision(bool for_game) = 0;

  // Need for DagorEditor to call it before call IGenEditorPlugin::beforeMainLoop()
  virtual void manageCustomColliders() = 0;

  virtual void getFastRtDump(FastRtDump **) = 0;
};
