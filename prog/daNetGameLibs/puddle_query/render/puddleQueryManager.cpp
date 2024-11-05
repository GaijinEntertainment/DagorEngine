// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "math/dag_mathUtils.h"
#include "math/integer/dag_IPoint4.h"
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_resetDevice.h>
#include "3d/dag_ringCPUQueryLock.h"
#include "shaders/dag_shaders.h"
#include "shaders/dag_computeShaders.h"
#include "perfMon/dag_statDrv.h"

#include "puddleQueryManager.h"
#include <puddle_query/shaders/puddle_query.hlsli>

static eastl::unique_ptr<PuddleQueryManager> puddle_query_mgr;

PuddleQueryManager *get_puddle_query_mgr() { return puddle_query_mgr.get(); }
void init_puddle_query_mgr() { puddle_query_mgr = eastl::make_unique<PuddleQueryManager>(); }
void close_puddle_query_mgr() { puddle_query_mgr.reset(); }

PuddleQueryManager::PuddleQueryManager()
{
  int resultBufferSlot = -1;
  if (!ShaderGlobal::get_int_by_name("puddle_buf_const_no", resultBufferSlot))
  {
    logerr("PuddleQuery: Could not find shader variable \"puddle_buf_const_no\"");
    return;
  }
  int resultsOffsetSlot = -1;
  if (!ShaderGlobal::get_int_by_name("puddle_results_offset_no", resultsOffsetSlot))
  {
    logerr("BiomeQuery: Could not find shader variable \"biome_results_offset_slot\"");
    return;
  }
  int numInputsSlot = -1;
  if (!ShaderGlobal::get_int_by_name("num_puddle_points_const_no", numInputsSlot))
  {
    logerr("PuddleQuery: Could not find shader variable \"num_puddle_points_const_no\"");
    return;
  }

  GpuReadbackQuerySystemDescription grqsDesc;
  grqsDesc.maxQueriesPerFrame = MAX_PUDDLE_QUERIES_PER_FRAME;
  grqsDesc.maxQueriesPerDispatch = MAX_PUDDLE_QUERIES_PER_DISPATCH;
  grqsDesc.computeShaderName = "puddle_query_cs";
  grqsDesc.resultBufferName = "puddle_query_results_ring_buf";
  grqsDesc.resultBufferSlot = resultBufferSlot;
  grqsDesc.resultsOffsetSlot = resultsOffsetSlot;
  grqsDesc.inputBufferName = "puddle_query_input_buf";
  grqsDesc.numInputsPerThreadGroup = PUDDLE_QUERY_WARP_SIZE;
  grqsDesc.setNumInputsToShader = true;
  grqsDesc.numInputsSlot = numInputsSlot;

  grqSystem = eastl::make_unique<GpuReadbackQuerySystem<Point4, float>>(grqsDesc);
  if (!grqSystem->isValid())
  {
    logerr("PuddleQuery: Failed to initialize GpuReadbackQuerySystem.");
    grqSystem.reset();
    return;
  }
}

PuddleQueryManager::~PuddleQueryManager() {}

void PuddleQueryManager::update()
{
  if (grqSystem == nullptr)
    return;

  grqSystem->update();
}

void PuddleQueryManager::beforeDeviceReset()
{
  if (grqSystem)
    grqSystem->beforeDeviceReset();
}

void PuddleQueryManager::afterDeviceReset()
{
  if (grqSystem)
    grqSystem->afterDeviceReset();
}

int PuddleQueryManager::queryPoint(const Point3 &point)
{
  if (grqSystem == nullptr)
    return -1;

  Point4 queryInput = Point4::xyz0(point);
  return grqSystem->query(queryInput);
}

GpuReadbackResultState PuddleQueryManager::getQueryValue(int query_id, float &value)
{
  if (grqSystem == nullptr)
    return GpuReadbackResultState::SYSTEM_NOT_INITIALIZED;

  return grqSystem->getQueryResult(query_id, value);
}

void puddle_query_before_device_reset(bool)
{
  if (get_puddle_query_mgr())
    get_puddle_query_mgr()->beforeDeviceReset();
}

void puddle_query_after_device_reset(bool)
{
  if (get_puddle_query_mgr())
    get_puddle_query_mgr()->afterDeviceReset();
}

REGISTER_D3D_BEFORE_RESET_FUNC(puddle_query_before_device_reset);
REGISTER_D3D_AFTER_RESET_FUNC(puddle_query_after_device_reset);