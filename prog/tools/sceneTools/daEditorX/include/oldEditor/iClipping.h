//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>

class Point3;
class String;
class FastRtDump;


class IClipping
{
public:
  static constexpr unsigned HUID = 0x87281C9Fu; // IClipping

  virtual bool compileClippingWithDialog(bool for_game) = 0;
  virtual bool initClipping(bool for_game) = 0;

  // Need for DagorEditor to call it before call IGenEditorPlugin::beforeMainLoop()
  virtual void manageCustomColliders() = 0;

  virtual void getFastRtDump(FastRtDump **) = 0;
};
