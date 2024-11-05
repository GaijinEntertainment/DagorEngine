//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace rendinst
{

enum class RenderPass
{
  Normal = 0,
  ImpostorColor = 1,
  ImpostorShadow = 2,
  ToShadow = 3,
  ToGrassHeight = 4,
  Depth = 5,
  VoxelizeAlbedo = 6,
  Voxelize3d = 7,
  Grassify = 8,
  ToHeightMap = 9
};

}
