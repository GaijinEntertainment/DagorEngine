//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>

class Sbuffer;

struct BVHConnection
{
  virtual bool isReady() const = 0;
  virtual bool prepare() = 0;
  virtual void done() = 0;
  virtual bool translateObjectId(int id, D3DRESID &buffer_id)
  {
    G_UNUSED(id);
    G_UNUSED(buffer_id);
    return false;
  }
  virtual void textureUsed(TEXTUREID texture_id) { G_UNUSED(texture_id); }
  virtual const UniqueBuf &getInstanceCounter() = 0;
  virtual const UniqueBuf &getInstancesBuffer() = 0;
  virtual const UniqueBuf &getMappingsBuffer() = 0;
};
