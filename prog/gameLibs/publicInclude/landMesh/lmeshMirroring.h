//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct LmeshMirroringParams
{
  int numBorderCellsXPos = 2;
  int numBorderCellsXNeg = 2;
  int numBorderCellsZPos = 2;
  int numBorderCellsZNeg = 2;
};

class DataBlock;

LmeshMirroringParams load_lmesh_mirroring(const DataBlock &level_blk);
