//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
