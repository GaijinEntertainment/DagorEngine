//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daSDF/update_sdf_request.h>

class IGenLoad;
class BBox3;
struct mat43f;

struct ObjectsSDF
{
  virtual ~ObjectsSDF() {}
  virtual void debugRender() = 0;
  virtual UpdateSDFQualityStatus checkObjects(UpdateSDFQualityRequest request, const uint8_t *new_mips, const uint32_t cnt) = 0;
  // virtual uint32_t getInstances() const = 0;
  // virtual bool getInstance(int i, int &obj_i, mat43f &m) const = 0;
  virtual int getMipsCount() const = 0;
  virtual float getMipSize(int mip) const = 0;
  virtual int uploadInstances(const uint16_t *obj_ods, const mat43f *instance, int cnt) = 0;
  virtual void setAtlasSize(int atlas_bw, int atlas_bh, int atlas_bd) = 0; // in 8x8x8 blocks
};

ObjectsSDF *new_objects_sdf(IGenLoad &cb);