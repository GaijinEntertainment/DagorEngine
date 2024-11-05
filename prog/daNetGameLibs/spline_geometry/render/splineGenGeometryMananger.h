// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "splineGenGeometryAsset.h"
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_resPtr.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/spline_gen_buffer.hlsli"

#include <math/integer/dag_IPoint2.h>
#include <EASTL/vector.h>
#include <generic/dag_carray.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

class SplineGenGeometryIb;
class framemem_allocator;

// Responsible for instanced rendering of SplineGenGeometry objects
// SplineGenGeometry objects can register and unregister from it
// Allocates space for instances before generation
// Provides context for attached object rendering (if hasObj)
// Uses SplineGenGeometryIb for index buffer
// It's parameters are taken from ECS of Entities with spline_gen_template template
// To make a new type of it, add a new spline_gen_template entity to scene
// Then you can instanstiate spline_gen_geometry entities with that template name
// It's stored in SplineGenGeometryRepository
// Buffers don't shrink to reduce number of allocations
// Active and inactive instances are stored in different buffers.
// Active instances are updated and generated every frame.
// Reactivates inactive instances if the buffers are reallocated.
class SplineGenGeometryManager
{
public:
  SplineGenGeometryManager(const eastl::string &template_name,
    uint32_t slices_,
    uint32_t stripes_,
    const eastl::string &diffuse_name,
    const eastl::string &normal_name,
    const eastl::string &asset_name,
    uint32_t asset_lod);
  InstanceId registerInstance();
  void unregisterInstance(InstanceId id, bool active);
  void makeInstanceActive(InstanceId id);
  void makeInstanceInctive(InstanceId id);
  void updateBuffers();
  void updateInstancingData(InstanceId id,
    SplineGenInstance &instance,
    const eastl::vector<SplineGenSpline, framemem_allocator> &spline_vec,
    const eastl::vector<BatchId> &batch_ids);
  void updateInactive(InstanceId id, SplineGenInstance &instance, const eastl::vector<BatchId> &batch_ids);
  // Also attaches objects if hasObj
  void generate();
  // Also renders objects if hasObj
  void render(bool render_color);
  const IPoint2 &getDisplacementTexSize() const;
  bool hasObj() const;
  SplineGenGeometryAsset &getAsset() const;
  void reactivateAllInstances();
  bool isReactivationInProcess() const;


  const eastl::string templateName;
  const uint32_t slices;  // x around
  const uint32_t stripes; // y lenth of the spline
  const eastl::string diffuseName;
  const eastl::string normalName;
  const uint32_t assetLod;

private:
  SplineGenGeometryIb *ibPtr = nullptr;
  SplineGenGeometryAsset *assetPtr = nullptr;

  int getAllocatedInstanceCount() const;
  int getCurrentInstanceCount() const;
  int getActiveInstanceCount() const;
  int getInactiveInstanceCount() const;
  void makeMoreSpots();

  eastl::vector<InstanceId> freeSpots;
  eastl::vector<InstanceId> usedSpots;
  eastl::vector<InstanceId> inactiveSpots;
  bool needsAllocation = false;
  bool invalidatePrevVb = false;
  bool reactivationInProcess = false;
  UniqueBuf instancingBuffer;
  UniqueBuf splineBuffer;
  UniqueBuf indirectionBuffer;
  UniqueBuf objBatchIdBuffer;
  eastl::vector<BatchId> objBatchIdData;
  uint32_t attachmentMaxNo = 0;
  uint32_t maxBatchesCount = 0;

  uint32_t getInstanceTriangleCount() const;
  uint32_t getInstanceVertexCount() const;

  void allocateBuffers();
  void uploadIndirectionBuffer();
  void uploadObjBatchIdBufer();

  Ptr<ComputeShaderElement> generatorCs;
  Ptr<ComputeShaderElement> normalCs;
  carray<UniqueBuf, 2> vertexBuffer;
  int currentVbIndex = 0;
  IPoint2 displacemenTexSize;

  void setInstancingShaderVars();
  void uploadIndirectParams(const eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> &params_data);

  Ptr<ComputeShaderElement> cullerCs;
  UniqueBuf culledBuffer;
  UniqueBuf paramsBuffer;
  int maxParamsCount = 0;

  Ptr<ShaderMaterial> shmat;
  Ptr<ShaderElement> shElem;
  SharedTex diffuseTex;
  SharedTex normalTex;
};
