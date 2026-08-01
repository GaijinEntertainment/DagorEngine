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
  UniqueTexWithShaderVar clouds_types_lut;
  UniqueTexWithShaderVar clouds_phase_lut;
  PostFxRenderer gen_clouds_types_lut, gen_clouds_phase_lut;
  bool frameValid = true;
};