// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

struct NormalFrameBuffer
{
  static float *start_ht;
  float *ht;
  uint16_t *normal; // if normal is 0xFFFF - it is landMesh
  inline void operator+=(int ofs)
  {
    ht += ofs;
    // normal += ofs;
  }
};

extern bool create_lmesh_normal_map(LandMeshMap &land, NormalFrameBuffer map, float ox, float oy, float scale, int wd,
  int ht); // returns true if rendered any pixel
