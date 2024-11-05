// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_computeShaders.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_carray.h>
#include <shaders/dag_dynSceneRes.h>
#include <gameRes/dag_gameResources.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/spline_gen_buffer.hlsli"

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

class SplineGenGeometryManager;
class framemem_allocator;

// Responsible for instanced rendering of attached objects
// It's created by SplineGenGeometryManager based on that object's assetName variable
// Empty string in spline_gen_template means no Asset
// Negotiates objCount with SplineGenGeometry
// It allocates the GPU buffers before generation
// For generation and rendering it requires the instancing context of SplineGenGeometryManager
// The assets are loaded in by the engine
// It's stored in SplineGenGeometryRepository
// Buffers don't shrink to reduce number of allocations
// If the buffers are reallocated, it notifies all managers using it, to reactivate all instances.
class SplineGenGeometryAsset
{
public:
  SplineGenGeometryAsset(const eastl::string &asset_name, uint32_t batch_size = 32);
  void updateBuffers();
  void updateInstancingData(SplineGenInstance &instance, eastl::vector<BatchId> &batch_ids, uint32_t requested_obj_count);
  void closeInstancingData(eastl::vector<BatchId> &batch_ids);
  float getObjectDiameter() const;

  const eastl::string assetName;
  const uint32_t batchSize;

private:
  // These methods are hidden, because they should only be called from SplineGenGeometryManager
  friend class SplineGenGeometryManager;
  void attachObjects(int instance_count, int attachment_max_no);
  void addIndirectParams(eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> &params_data, uint32_t lod) const;
  void renderObjects(Sbuffer *params_buffer, uint32_t lod);

  DynamicRenderableSceneLodsResourcePtr res;
  GameResHandle objHandle;
  float objectDiameter = 0.0f;

  BatchId registerBatch();
  void unregisterBatch(BatchId id);
  int getAllocatedBatchCount() const;
  int getCurrentBatchCount() const;

  eastl::vector<BatchId> freeSpots;
  eastl::vector<BatchId> usedSpots;
  bool invalidatePrevData = false;
  int unservicedBatchRequests = 0;

  void allocateBuffers(int goal);

  Ptr<ComputeShaderElement> attacherCs;
  carray<UniqueBuf, 2> dataBuffer;
  int currentDataIndex = 0;

  DrawIndexedIndirectArgs extractRElemParams(const ShaderMesh::RElem &elem) const;
};
