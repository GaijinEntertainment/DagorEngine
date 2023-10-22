//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <landMesh/lmeshManager.h>

class Mesh;
class BaseTexture;
typedef BaseTexture Texture;


namespace landmesh
{
struct Cell;
}

struct landmesh::Cell
{
  Mesh *land_mesh;
  Mesh *combined_mesh;
  Mesh *decal_mesh;
  Mesh *patches_mesh;
  BBox3 box;
};
