// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/lmeshMirroring.h>
#include <ioSys/dag_dataBlock.h>

LmeshMirroringParams load_lmesh_mirroring(const DataBlock &level_blk)
{
  LmeshMirroringParams ret;
  ret.numBorderCellsXPos = level_blk.getInt("numBorderCellsXPos", 2);
  ret.numBorderCellsXNeg = level_blk.getInt("numBorderCellsXNeg", 2);
  ret.numBorderCellsZPos = level_blk.getInt("numBorderCellsZPos", 2);
  ret.numBorderCellsZNeg = level_blk.getInt("numBorderCellsZNeg", 2);
  return ret;
}
