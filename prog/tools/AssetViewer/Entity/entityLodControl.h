// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_resId.h>
#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <util/dag_simpleString.h>
#include <util/dag_stdint.h>

struct EntityRiLodInfo
{
  enum class Kind
  {
    Mesh,
    Voxel,
    Billboard,
  };

  Kind kind;
  float range;
  int triangleCount;
  uint32_t gpuMemorySize;  // VB/IB and voxel data of this LOD only, in bytes
  Tab<TEXTUREID> textures; // used by the materials of this LOD, also when other LODs or assets share them
};

struct EntityRiLodsInfo
{
  SimpleString assetName;
  Point3 center;
  float radius;
  Tab<EntityRiLodInfo> lods;
};

// Forces the LOD of the shown asset the same way as the viewport keys, -1 selects the LOD automatically.
// out_lod is the forced LOD after clamping (-1 for automatic). Returns false when the Entity plugin is not active.
bool entity_view_set_lod(int lod, int &out_lod);

// Returns false when the Entity plugin is not active or the shown asset is not a rendInst.
bool entity_view_get_ri_lods_info(EntityRiLodsInfo &out_info);
