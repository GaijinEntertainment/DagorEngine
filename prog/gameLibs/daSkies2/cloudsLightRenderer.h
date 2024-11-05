// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>

#include "clouds2Common.h"

class CloudsLightRenderer
{
public:
  void init();
  void update(float) {}
  inline void invalidate() { resetGen = 0; }
  CloudsChangeFlags render(const Point3 &main_light_dir, const Point3 &second_light_dir);

private:
  UniqueTexHolder clouds_light_color;

  PostFxRenderer gen_clouds_light_texture_ps;
  eastl::unique_ptr<ComputeShaderElement> gen_clouds_light_texture_cs;

  uint32_t resetGen = 0;
  float lastMainLightDirY = 0, lastSecondLightDirY = 0;
};