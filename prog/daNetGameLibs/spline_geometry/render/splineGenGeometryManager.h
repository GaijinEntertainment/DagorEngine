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
// Its parameters are taken from ECS of Entities with spline_gen_template template
// To make a new type of it, add a new spline_gen_template entity to scene
// Then you can instanstiate spline_gen_geometry entities with that template name
// It's stored in SplineGenGeometryRepository
// Buffers don't shrink to reduce number of allocations
// No individual vertex / normal data is stored, as that is generated in the vertex shader.
// Reactivates inactive instances if the buffers are reallocated.
class SplineGenGeometryManager
{
public:
  SplineGenGeometryManager(const eastl::string &template_name,
    uint32_t slices_,
    uint32_t stripes_,
    const eastl::string &diffuse_name,
    const eastl::string &normal_name,
    const eastl::string &emissive_mask_name,
    const eastl::string &ao_tex_name,
    const eastl::string &asset_name,
    const eastl::string &shader_type,
    uint32_t asset_lod);
  InstanceId registerInstance();
  void unregisterInstance(InstanceId id, bool active);
  void makeInstanceActive(InstanceId id);
  void makeInstanceInactive(InstanceId id);
  void updateBuffers();
  void updateInstancingData(
    InstanceId id, SplineGenInstance &instance, const eastl::vector<SplineGenSpline, framemem_allocator> &spline_vec);
  void updateAttachmentBatchIds(InstanceId id, SplineGenInstance &instance, const eastl::vector<BatchId> &batch_ids);
  void uploadGenerateData();
  // Also attaches objects if hasObj
  void generate();
  void attachObjects();
  void uploadCullData(int cascade);
  void cull(int cascade);
  // Also renders objects if hasObj
  void render(int cascade);
  const IPoint2 &getDisplacementTexSize() const;
  bool hasObj() const;
  SplineGenGeometryAsset &getAsset() const;
  void reactivateAllInstances();
  bool isReactivationInProcess() const;

  enum class SplineGenType
  {
    REGULAR_SPLINE_GEN = 0,
    EMISSIVE_SPLINE_GEN = 1,
    SKIN_SPLINE_GEN = 2
  };
  SplineGenType getType() const { return type; }


  const eastl::string templateName;
  const uint32_t slices;  // x around
  const uint32_t stripes; // y lenth of the spline
  const eastl::string diffuseName;
  const eastl::string normalName;
  const uint32_t assetLod;

  enum
  {
    CASCADE_COLOR = 0,
    CASCADE_SHADOW = 1,
    CASCADE_COUNT = 2
  };

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
  bool invalidatePrevSb = false;
  bool needsAllocation = false;
  bool reactivationInProcess = false;
  UniqueBuf instancingStagingBuffer;
  UniqueBuf instancingBuffer;
  carray<UniqueBuf, 2> splineBuffer;
  int currentSBIndex = 0;
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

  SplineGenType type = SplineGenType::REGULAR_SPLINE_GEN;

  IPoint2 displacemenTexSize;

  void setInstancingShaderVars();
  void setInstancingShaderVarsForRendering(int cascade);
  void uploadIndirectParams(const eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> &params_data, int cascade);

  Ptr<ComputeShaderElement> cullerCs;
  UniqueBuf culledBuffer[CASCADE_COUNT];
  UniqueBuf paramsBuffer[CASCADE_COUNT];
  int maxParamsCount[CASCADE_COUNT] = {};

  Ptr<ShaderMaterial> shmat;
  Ptr<ShaderElement> shElem;
  SharedTex diffuseTex;
  SharedTex normalTex;
  SharedTex additionalTex;
};
