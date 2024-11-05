// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_dynSceneRes.h>

class DynamicPhysObjectData;

class PhysCarCreationData
{
public:
  PhysCarCreationData() : carDataPtr(NULL), bodyPhObjData(NULL) {}

  PhysCarSettings2 *carDataPtr;
  DynamicPhysObjectData *bodyPhObjData;
  Ptr<DynamicRenderableSceneLodsResource> frontWheelModel;
  Ptr<DynamicRenderableSceneLodsResource> rearWheelModel;
};


namespace physcarprivate
{
extern bool delayed_cb_update;
extern float globalFrictionMul;
extern float globalFrictionVDecay;
extern DataBlock *globalCarParamsBlk;

const DataBlock &load_car_params_data_block(const char *car_name, const DataBlock *&car_blk);
} // namespace physcarprivate
