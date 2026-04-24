//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>

class RenderableInstanceLodsResource;
class IPoint2;

struct BVHInstanceMapper
{
  struct InstanceBuffers
  {
    Sbuffer *instanceCount = nullptr;
    Sbuffer *instances = nullptr;
  };

  virtual void mapRendinst(const RenderableInstanceLodsResource *ri, int lod, int elem, IPoint2 &out_blas,
    uint32_t &out_meta) const = 0;
  virtual void submitBuffers(Sbuffer *instance_counter, Sbuffer *instances) = 0;
  static size_t getHWInstanceSize();
  virtual void requestStaticBLAS(const RenderableInstanceLodsResource *ri) = 0;
  virtual bool isInStaticBLASQueue(const RenderableInstanceLodsResource *ri) = 0;
};