// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineGenGeometryAsset.h"
#include "splineGenGeometryRepository.h"
#include "splineGenGeometryShaderVar.h"
#include <gameRes/dag_stdGameResId.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <memory/dag_framemem.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/spline_gen_buffer.hlsli"
#include <render/debugMesh.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_buffers.h>

SplineGenGeometryAsset::SplineGenGeometryAsset(const eastl::string &asset_name, uint32_t batch_size) :
  assetName(asset_name), batchSize(batch_size)
{
  objHandle = GAMERES_HANDLE_FROM_STRING(assetName.c_str());
  res = (DynamicRenderableSceneLodsResource *)get_one_game_resource_ex(objHandle, DynModelGameResClassId);
  release_game_resource(objHandle); // remove the reference added by get_one_game_resource_ex, res will keep one too

  const char *attacherShader = "spline_gen_obj_attacher";
  attacherCs = new_compute_shader(attacherShader);
  G_ASSERTF(attacherCs, "can't create computeShader %s", attacherShader);

  allocateBuffers(16);
}

BatchId SplineGenGeometryAsset::registerBatch()
{
  if (freeSpots.empty())
  {
    unservicedBatchRequests++;
    return INVALID_INSTANCE_ID;
  }

  BatchId id = freeSpots.back();
  freeSpots.pop_back();
  usedSpots.push_back(id);
  return id;
}

void SplineGenGeometryAsset::unregisterBatch(BatchId id)
{
  freeSpots.push_back(id);
  usedSpots.erase_first(id);
}

void SplineGenGeometryAsset::updateBuffers()
{
  if (unservicedBatchRequests > 0)
  {
    allocateBuffers(getAllocatedBatchCount() + unservicedBatchRequests);
  }
  unservicedBatchRequests = 0;

  if (getCurrentBatchCount() == 0)
    return;

  currentDataIndex = (currentDataIndex + 1) % 2;
}

void SplineGenGeometryAsset::updateInstancingData(
  SplineGenInstance &instance, eastl::vector<BatchId> &batch_ids, uint32_t requested_obj_count)
{
  uint32_t prevObjCount = instance.objCount;
  uint32_t prevBatches = (prevObjCount + batchSize - 1) / batchSize;
  uint32_t currBatches = (requested_obj_count + batchSize - 1) / batchSize;
  G_ASSERT(prevBatches == batch_ids.size());
  for (uint32_t i = prevBatches; i < currBatches; i++)
  {
    BatchId receivedId = registerBatch();
    if (receivedId == INVALID_INSTANCE_ID)
    {
      currBatches = i;
      break;
    }
    batch_ids.push_back(receivedId);
  }
  for (int i = (int)prevBatches - 1; i >= (int)currBatches; i--)
  {
    unregisterBatch(batch_ids[i]);
  }
  batch_ids.resize(currBatches);
  uint32_t currObjCount = min(requested_obj_count, currBatches * batchSize);
  instance.objCount = currObjCount;
  if (invalidatePrevData || currObjCount > prevObjCount)
    instance.flags &= ~PREV_ATTACHMENT_DATA_VALID;
}

void SplineGenGeometryAsset::closeInstancingData(eastl::vector<BatchId> &batch_ids)
{
  eastl::for_each(batch_ids.rbegin(), batch_ids.rend(), [this](BatchId id) { unregisterBatch(id); });
  batch_ids.clear();
}

void SplineGenGeometryAsset::attachObjects(int instance_count, int attachment_max_no)
{
  TIME_D3D_PROFILE(spline_gen_geometry_attach_objects);
  invalidatePrevData = false;

  ShaderGlobal::set_int(spline_gen_attachment_batch_sizeVarId, batchSize);
  static int spline_gen_attachment_data_uav_no = ShaderGlobal::get_slot_by_name("spline_gen_attachment_data_uav_no");
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, spline_gen_attachment_data_uav_no, VALUE), dataBuffer[currentDataIndex].getBuf());

  attacherCs->dispatchThreads(attachment_max_no, instance_count, 1);
}

void SplineGenGeometryAsset::renderObjects(Sbuffer *params_buffer, uint32_t lod)
{
  if (is_game_resource_loaded(objHandle, DynModelGameResClassId))
  {
    G_ASSERT(res->isSubOf(DynamicRenderableSceneLodsResourceCID));
    objectDiameter = length(res->bbox.width());

    TIME_D3D_PROFILE(spline_gen_geometry_render_attached_objects);

    ShaderGlobal::set_int(spline_gen_attachment_batch_sizeVarId, batchSize);
    ShaderGlobal::set_buffer(spline_gen_attachment_dataVarId, dataBuffer[currentDataIndex]);
    ShaderGlobal::set_buffer(spline_gen_prev_attachment_dataVarId, dataBuffer[(currentDataIndex + 1) % 2]);

    int renderCount = 0;
    dag::Span<DynamicRenderableSceneLodsResource::Lod> availableLods = res->getUsedLods();
    lod = min<int>(lod, availableLods.size() - 1);
    int debugValue = lod;
#if DAGOR_DBGLEVEL > 0
    if (debug_mesh::is_enabled(debug_mesh::Type::drawElements))
    {
      debugValue = 0;
      for (DynamicRenderableSceneResource::RigidObject &rigid : availableLods[lod].scene->getRigids())
        debugValue += rigid.mesh->getMesh()->getAllElems().size();
    }
#endif
    debug_mesh::set_debug_value(debugValue);
    for (DynamicRenderableSceneResource::RigidObject &rigid : availableLods[lod].scene->getRigids())
    {
      for (const ShaderMesh::RElem &elem : rigid.mesh->getMesh()->getAllElems())
      {
        elem.e->setStates();
        elem.vertexData->setToDriver();
        d3d::draw_indexed_indirect(PRIM_TRILIST, params_buffer, DRAW_INDEXED_INDIRECT_BUFFER_SIZE * (renderCount + 1));
        renderCount++;
      }
    }
    debug_mesh::reset_debug_value();
  }
}

float SplineGenGeometryAsset::getObjectDiameter() const { return objectDiameter; }

void SplineGenGeometryAsset::addIndirectParams(eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> &params_data,
  uint32_t lod) const
{
  if (!is_game_resource_loaded(objHandle, DynModelGameResClassId))
    return;
  dag::Span<DynamicRenderableSceneLodsResource::Lod> availableLods = res->getUsedLods();
  lod = min<int>(lod, availableLods.size() - 1);
  for (DynamicRenderableSceneResource::RigidObject &rigid : availableLods[lod].scene->getRigids())
  {
    for (const ShaderMesh::RElem &elem : rigid.mesh->getMesh()->getAllElems())
    {
      params_data.push_back(extractRElemParams(elem));
    }
  }
}

int SplineGenGeometryAsset::getAllocatedBatchCount() const { return usedSpots.size() + freeSpots.size(); }

int SplineGenGeometryAsset::getCurrentBatchCount() const { return usedSpots.size(); }

void SplineGenGeometryAsset::allocateBuffers(int goal)
{
  int currentCount = getAllocatedBatchCount();
  if (goal <= currentCount)
    return;
  int newCount = max(currentCount, 16);
  while (newCount < goal)
    newCount *= 2;
  for (int i = newCount - 1; i >= currentCount; i--)
    freeSpots.push_back(static_cast<BatchId>(i));


  String buffName;

  for (int i = 0; i < dataBuffer.size(); i++)
  {
    dataBuffer[i].close();
    buffName = String(0, "%s_dataBuffer_%d", assetName.c_str(), i);
    dataBuffer[i] = dag::buffers::create_ua_sr_structured(ATTACHMENT_DATA_SIZE, getAllocatedBatchCount() * batchSize, buffName);
  }

  invalidatePrevData = true;

  for (auto &managerPair : get_spline_gen_repository().getManagers())
  {
    if (managerPair.second->hasObj() && &managerPair.second->getAsset() == this)
      managerPair.second->reactivateAllInstances();
  }
}

DrawIndexedIndirectArgs SplineGenGeometryAsset::extractRElemParams(const ShaderMesh::RElem &elem) const
{
  DrawIndexedIndirectArgs paramsData;
  paramsData.instanceCount = 0;
  paramsData.startInstanceLocation = 0;
  paramsData.indexCountPerInstance = elem.numf * 3;
  paramsData.startIndexLocation = elem.si;
  paramsData.baseVertexLocation = elem.baseVertex;
  return paramsData;
}
