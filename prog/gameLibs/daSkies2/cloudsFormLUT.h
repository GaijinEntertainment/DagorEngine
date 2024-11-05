// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>

#include "clouds2Common.h"

class CloudsFormLUT
{
public:
  void init();
  void invalidate() { frameValid = false; }
  CloudsChangeFlags render();

private:
  UniqueTexHolder clouds_types_lut;
  UniqueTex clouds_erosion_lut;
  PostFxRenderer gen_clouds_types_lut, gen_clouds_erosion_lut;
  bool frameValid = true;
};