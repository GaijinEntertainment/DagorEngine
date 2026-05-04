// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bvh/bvh.h"
#include "splineGenGeometryPointers.h"
#include "splineGenGeometryAsset.h"
#include "splineGenGeometryShapeManager.h"
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_resPtr.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/spline_gen_buffer.hlsli"

#include <math/integer/dag_IPoint2.h>
#include <EASTL/vector.h>
#include <generic/dag_carray.h>
#include <EASTL/string.h>
#include <util/dag_bitArray.h>

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
// (Unless 'vertex_gen_phase_exists convar' is true, or raytracing is enabled, in which
// case we generate the vertices before the frame starts, and store them in vertexBuffers).
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
  ~SplineGenGeometryManager();
  InstanceId registerInstance();
  void eraseInstanceId(InstanceId id);
  void unregisterInstance(InstanceId id);
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
  void cullForCamera();
  void uploadCameraCullData();
  void cullForShadow();
  void uploadShadowCullData();
  void renderOpaque(int cascade);
  void renderTransparent();
  const IPoint2 &getDisplacementTexSize() const;
  bool hasObj() const;
  SplineGenGeometryAsset &getAsset() const;
  SplineGenGeometryShapeManager &getShapeManager() const;
  void reactivateAllInstances();
  bool isReactivationInProcess() const;
  void addInstancesToBVH(bvh::ContextId context_id);
  void removeBVHContext();

  enum class SplineGenType
  {
    REGULAR_SPLINE_GEN = 0,
    EMISSIVE_SPLINE_GEN = 1,
    SKIN_SPLINE_GEN = 2,
    REFRACTIVE_SPLINE_GEN = 3,
  };
  SplineGenType getType() const { return type; }


  const eastl::string templateName;
  const uint32_t slices;  // x around
  const uint32_t stripes; // y lenth of the spline
  const uint32_t splineEntrySize;
  const eastl::string diffuseName;
  const eastl::string normalName;
  const uint32_t assetLod;

  enum
  {
    CASCADE_COLOR = 0,
    CASCADE_SHADOW = 1,
    CASCADE_TRANSPARENT = 2,
    CASCADE_COUNT = 3
  };

private:
  SplineGenGeometryIbPtr ibPtr;
  SplineGenGeometryAssetPtr assetPtr;
  SplineGenGeometryShapeManagerPtr shapeManagerPtr;

  int getAllocatedInstanceCount() const;
  int getCurrentInstanceCount() const;
  int getActiveInstanceCount() const;
  int getInactiveInstanceCount() const;
  void makeMoreSpots();
  int getAdditionalTexVarId() const;
  // Also renders objects if hasObj
  void render(int cascade);
  void cull(int cascade);
  void uploadCullData(int cascade);

  eastl::vector<InstanceId> freeSpots;
  eastl::vector<InstanceId> usedSpots;
  eastl::vector<InstanceId> inactiveSpots;
  bool invalidatePrevBuffer = false;
  bool needsAllocation = false;
  bool needsVertexGeneration = false;
  bool reactivationInProcess = false;
  UniqueBuf instancingBuffer;
  Bitarray splineBufferDirtyMask;
  bool splineBufferDirty = false;
  UniqueBuf splineStagingBuffer;
  carray<UniqueBuf, 2> splineBuffer;
  carray<UniqueBuf, 2> vertexBuffer;
  int currentBufferIndex = 0;
  UniqueBuf indirectionBuffer;
  UniqueBuf objBatchIdBuffer;
  eastl::vector<BatchId> objBatchIdData;
  eastl::vector<eastl::pair<InstanceId, SplineGenInstance>> pendingInstanceWrites;
  uint32_t attachmentMaxNo = 0;
  uint32_t maxBatchesCount = 0;
  uint32_t drawIndirectArgsNr = 0;
  uint32_t bvhId = 0;
  bvh::ContextId bvhContextId = nullptr;

  uint32_t getInstanceTriangleCount() const;
  uint32_t getInstanceVertexCount() const;

  void allocateBuffers();
  void uploadIndirectionBuffer();
  void uploadObjBatchIdBuffer();
  void uploadSplineBuffer();

  SplineGenType type = SplineGenType::REGULAR_SPLINE_GEN;

  IPoint2 displacemenTexSize;

  void setInstancingShaderVars();
  void setInstancingShaderVarsForRendering(int cascade);
  void uploadIndirectParams(const eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> &params_data, int cascade);

  Ptr<ComputeShaderElement> cullerCs;
  Ptr<ComputeShaderElement> generatorCs;
  Ptr<ComputeShaderElement> normalCs;
  UniqueBuf culledBuffer[CASCADE_COUNT];
  UniqueBuf paramsBuffer[CASCADE_COUNT];
  int maxParamsCount[CASCADE_COUNT] = {};

  Ptr<ShaderMaterial> shmat;
  Ptr<ShaderElement> shElem;
  SharedTex diffuseTex;
  SharedTex normalTex;
  SharedTex additionalTex;
};
