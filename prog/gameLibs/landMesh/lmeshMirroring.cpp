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
  ret.mirrorShrinkXPos = level_blk.getReal("mirrorShrinkXPos", 0.f);
  ret.mirrorShrinkXNeg = level_blk.getReal("mirrorShrinkXNeg", 0.f);
  ret.mirrorShrinkZPos = level_blk.getReal("mirrorShrinkZPos", 0.f);
  ret.mirrorShrinkZNeg = level_blk.getReal("mirrorShrinkZNeg", 0.f);
  return ret;
}
