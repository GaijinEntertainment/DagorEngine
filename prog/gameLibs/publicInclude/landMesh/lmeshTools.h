//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_bounds3.h>
#include <3d/dag_texMgr.h>

class ShaderMesh;
class GlobalVertexData;
class LandMeshManager;

class BaseTexture;
typedef BaseTexture Texture;
typedef BaseTexture ArrayTexture;

// Number of mega-detail texture stacks (albedo / reflectance / ...). Shared by the
// land-mesh manager and the land-class / virtual-texture renderer.
static constexpr int NUM_TEXTURES_STACK = 3;
