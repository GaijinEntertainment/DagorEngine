// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_shaders.h>
#include <render/spheres_consts.hlsli>

#include "render/debugLightProbeSpheres.h"

DebugLightProbeSpheres::DebugLightProbeSpheres(const char *shaderName)
{
  sphereMat.reset(new_shader_material_by_name(shaderName));
  if (sphereMat)
  {
    sphereMat->addRef();
    sphereElem = sphereMat->make_elem();
  }
}

void DebugLightProbeSpheres::render() const
{
  if (!spheresCount || !sphereMat)
    return;

  sphereElem->setStates(0, true);

  d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW, spheresCount);
}
