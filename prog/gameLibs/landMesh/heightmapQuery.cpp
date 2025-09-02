// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include "math/dag_mathUtils.h"
#include "math/dag_hlsl_floatx.h"
#include "math/integer/dag_IPoint4.h"
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_resPtr.h>
#include "shaders/dag_shaders.h"
#include "util/dag_console.h"
#include "util/dag_convar.h"
#include "util/dag_oaHashNameMap.h"
#include "gpuReadbackQuery/gpuReadbackQuerySystem.h"
#include <math/dag_hlsl_floatx.h>
#include "landMesh/heightmap_query.hlsli"
#include <osApiWrappers/dag_critSec.h>

#include "landMesh/heightmapQuery.h"


static WinCritSec heightmap_query_mutex;
#define HEIGHTMAP_QUERY_BLOCK WinAutoLock heightmapQueryLock(heightmap_query_mutex);


class HeightmapQueryCtx
{
  LandclassQueryData data;

public:
  bool init();
  void update();
  void beforeDeviceReset();
  void afterDeviceReset();

  void set_landclass_data(LandclassQueryData &data) { this->data = data; }

  int query(const Point3 &world_pos, const Point3 &grav_dir);
  GpuReadbackResultState getQueryResult(int query_id, HeightmapQueryResult &result);

private:
  eastl::unique_ptr<GpuReadbackQuerySystem<HeightmapQueryInput, HeightmapQueryResult>> grqSystem;
};

static eastl::unique_ptr<HeightmapQueryCtx> heightmap_query_ctx;
static int num_heightmap_groups_slot = -1;
static int details_cb_slot = -1;

bool HeightmapQueryCtx::init()
{
  int resultBufferSlot = -1;
  if (!ShaderGlobal::get_int_by_name("hmap_query_dispatch_buf_const_no", resultBufferSlot))
  {
    logerr("HeightmapQuery: Could not find shader variable \"hmap_query_dispatch_buf_const_no\"");
    return false;
  }
  int resultsOffsetSlot = -1;
  if (!ShaderGlobal::get_int_by_name("hmap_query_dispatch_results_offset_no", resultsOffsetSlot))
  {
    logerr("HeightmapQuery: Could not find shader variable \"hmap_query_dispatch_results_offset_no\"");
    return false;
  }
  int numInputsSlot = -1;
  if (!ShaderGlobal::get_int_by_name("hmap_query_dispatch_cnt_const_no", numInputsSlot))
  {
    logerr("HeightmapQuery: Could not find shader variable \"hmap_query_dispatch_cnt_const_no\"");
    return false;
  }

  GpuReadbackQuerySystemDescription grqsDesc;
  grqsDesc.maxQueriesPerFrame = MAX_HEIGHTMAP_QUERIES_PER_FRAME;
  grqsDesc.maxQueriesPerDispatch = MAX_HEIGHTMAP_QUERIES_PER_DISPATCH;
  grqsDesc.computeShaderName = "heightmap_query_cs";
  grqsDesc.resultBufferName = "heightmap_query_results_ring_buf";
  grqsDesc.resultBufferSlot = resultBufferSlot;
  grqsDesc.resultsOffsetSlot = resultsOffsetSlot;
  grqsDesc.inputBufferName = "heightmap_query_input_buf";
  grqsDesc.numInputsPerThreadGroup = HEIGHTMAP_QUERY_WARP_SIZE;
  grqsDesc.setNumInputsToShader = true;
  grqsDesc.numInputsSlot = numInputsSlot;

  grqSystem = eastl::make_unique<GpuReadbackQuerySystem<HeightmapQueryInput, HeightmapQueryResult>>(grqsDesc);
  if (!grqSystem->isValid())
  {
    logerr("HeightmapQuery: Failed to initialize GpuReadbackQuerySystem.");
    grqSystem.reset();
    return false;
  }

  return true;
}

void HeightmapQueryCtx::update() { grqSystem->update(); }

void HeightmapQueryCtx::beforeDeviceReset() { grqSystem->beforeDeviceReset(); }

void HeightmapQueryCtx::afterDeviceReset() { grqSystem->afterDeviceReset(); }

static int landclass_check(const Point3 &world_pos, const Point3 &grav_dir, const LandclassQueryData &data)
{
  float max_dot = -1.0f;
  int max_index = -1;

  Point3 grav_dir_normalized = normalize(grav_dir);

  for (int i = 0; i < data.landclass_gravs.size(); i++)
  {
    Point3 landclass_grav = data.landclass_gravs[i];
    Point3 diff = world_pos - Point3(data.pos_radiusSqr[i].x, data.pos_radiusSqr[i].y, data.pos_radiusSqr[i].z);
    float distSqr = diff * diff;

    float dot = grav_dir_normalized * landclass_grav;
    if (dot > max_dot && dot > 0.9f && distSqr < data.pos_radiusSqr[i].w)
    {
      max_dot = dot;
      max_index = i;
    }
  }

  Point3 default_grav_dir = Point3(0, -1, 0);
  float dot = grav_dir_normalized * default_grav_dir;
  if (dot > max_dot && dot > 0.9f)
  {
    max_dot = dot;
    max_index = -1;
  }

  return max_index < 0 ? -1 : data.indices[max_index];
}

int HeightmapQueryCtx::query(const Point3 &world_pos, const Point3 &grav_dir)
{
  int landclass_index = landclass_check(world_pos, grav_dir, data);

  HeightmapQueryInput input;
  input.worldPos = world_pos;
  input.riLandclassIndex = landclass_index;
  return grqSystem->query(input);
}

GpuReadbackResultState HeightmapQueryCtx::getQueryResult(int query_id, HeightmapQueryResult &result)
{
  return grqSystem->getQueryResult(query_id, result);
}


void heightmap_query::init()
{
  HEIGHTMAP_QUERY_BLOCK;
  heightmap_query_ctx = eastl::make_unique<HeightmapQueryCtx>();
  if (!heightmap_query_ctx->init())
  {
    logerr("Heightmap query system failed to initialize.");
    heightmap_query_ctx.reset();
  }
}

void heightmap_query::close()
{
  HEIGHTMAP_QUERY_BLOCK;
  heightmap_query_ctx.reset();
}

void heightmap_query::update()
{
  HEIGHTMAP_QUERY_BLOCK;
  if (!heightmap_query_ctx)
    return;
  heightmap_query_ctx->update();
}

void heightmap_query::update_landclass_data(LandclassQueryData &data)
{
  HEIGHTMAP_QUERY_BLOCK;
  if (!heightmap_query_ctx)
    return;

  heightmap_query_ctx->set_landclass_data(data);
}

int heightmap_query::query(const Point3 &world_pos, const Point3 &grav_dir = Point3(0, -1, 0))
{
  HEIGHTMAP_QUERY_BLOCK;
  return heightmap_query_ctx ? heightmap_query_ctx->query(world_pos, grav_dir) : -1;
}

GpuReadbackResultState heightmap_query::get_query_result(int query_id, HeightmapQueryResultWrapper &wrapped_result)
{
  HEIGHTMAP_QUERY_BLOCK;
  HeightmapQueryResult result = {
    .normal = Point3(0.0f, 0.0f, 0.0f), .hitDistNoOffset = 0.0f, .hitDistWithOffset = 0.0f, .hitDistWithOffsetDeform = 0.0f};
  auto ret_val =
    heightmap_query_ctx ? heightmap_query_ctx->getQueryResult(query_id, result) : GpuReadbackResultState::SYSTEM_NOT_INITIALIZED;

  wrapped_result.hitDistNoOffset = result.hitDistNoOffset;
  wrapped_result.hitDistWithOffset = result.hitDistWithOffset;
  wrapped_result.hitDistWithOffsetDeform = result.hitDistWithOffsetDeform;
  wrapped_result.normal = result.normal;
  return ret_val;
}

GpuReadbackResultState heightmap_query::get_query_result(int query_id, HeightmapQueryResult &result)
{
  HEIGHTMAP_QUERY_BLOCK;
  return heightmap_query_ctx ? heightmap_query_ctx->getQueryResult(query_id, result) : GpuReadbackResultState::SYSTEM_NOT_INITIALIZED;
}


static void heightmap_query_before_device_reset(bool)
{
  HEIGHTMAP_QUERY_BLOCK;
  if (heightmap_query_ctx)
    heightmap_query_ctx->beforeDeviceReset();
}

static void heightmap_query_after_device_reset(bool)
{
  HEIGHTMAP_QUERY_BLOCK;
  if (heightmap_query_ctx)
    heightmap_query_ctx->afterDeviceReset();
}

REGISTER_D3D_BEFORE_RESET_FUNC(heightmap_query_before_device_reset);
REGISTER_D3D_AFTER_RESET_FUNC(heightmap_query_after_device_reset);