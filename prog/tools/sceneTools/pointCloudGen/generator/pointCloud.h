// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_materialData.h>
#include <EASTL/fixed_vector.h>
#include <math/dag_e3dColor.h>
#include <math/dag_hlsl_floatx.h>
#include <generic/dag_tab.h>

namespace plod
{

struct PointCloud
{
  Tab<float3> positions;
  Tab<float3> normals;
  Tab<eastl::fixed_vector<E3DCOLOR, 8>> material;
  MaterialData materialData;
};

} // namespace plod
