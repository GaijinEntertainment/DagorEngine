// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/world/bvh.h"
#include "splineGenGeometryRepository.h"
#include "splineGenGeometryShaderVar.h"
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_shaderBlock.h>
#include <gameRes/dag_gameResources.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <3d/dag_lockSbuffer.h>
#include <util/dag_convar.h>

CONSOLE_BOOL_VAL("spline_gen", vertex_gen_phase_exists, false);

SplineGenGeometryManager::SplineGenGeometryManager(const eastl::string &template_name,
  uint32_t slices_,
  uint32_t stripes_,
  const eastl::string &diffuse_name,
  const eastl::string &normal_name,
  const eastl::string &emissive_mask_name,
  const eastl::string &ao_tex_name,
  const eastl::string &asset_name,
  const eastl::string &shader_type,
  uint32_t asset_lod) :
  templateName(template_name),
  slices(slices_),
  stripes(stripes_),
  splineEntrySize(sizeof(SplineGenSpline) * (stripes_ + 1)),
  diffuseName(diffuse_name),
  normalName(normal_name),
  assetLod(asset_lod)
{
  const char *shader_name = "dynamic_spline_gen";
  if (strcmp(shader_type.c_str(), "emissive") == 0)
  {
    shader_name = "dynamic_emissive_spline_gen";
    type = SplineGenType::EMISSIVE_SPLINE_GEN;

    additionalTex = dag::get_tex_gameres(emissive_mask_name.c_str());
    G_ASSERT(additionalTex.getTexId() != BAD_TEXTUREID);
  }
  else if (strcmp(shader_type.c_str(), "skin") == 0)
  {
    shader_name = "dynamic_skin_spline_gen";
    type = SplineGenType::SKIN_SPLINE_GEN;

    if (!ao_tex_name.empty())
    {
      additionalTex = dag::get_tex_gameres(ao_tex_name.c_str());
      G_ASSERT(additionalTex.getTexId() != BAD_TEXTUREID);
    }
  }
  else if (strcmp(shader_type.c_str(), "refractive") == 0)
  {
    shader_name = "dynamic_refractive_spline_gen";
    type = SplineGenType::REFRACTIVE_SPLINE_GEN;
    G_ASSERTF(asset_name.empty(), "Refractive splineGen does not support attached objects!");
    get_spline_gen_repository().createTransparentSplineGenNode();
  }
  else if (strcmp(shader_type.c_str(), "regular") == 0)
  {
    type = SplineGenType::REGULAR_SPLINE_GEN;
  }
  else
  {
    G_ASSERTF(false, "Unknown spline_gen_template__shader_type '%s' (Valid types: 'regular', 'emissive', 'skin' or 'refractive')",
      shader_type.c_str());
  }
  const char *mat_script = nullptr;
  shmat = new_shader_material_by_name_optional(shader_name, mat_script);
  G_ASSERTF(shmat.get(), "can't create ShaderMaterial for '%s'", shader_name);
  shElem = shmat->make_elem();
  G_ASSERTF(shElem.get(), "can't create ShaderElement for ShaderMaterial '%s'", shader_name);

  diffuseTex = dag::get_tex_gameres(diffuseName.c_str());
  G_ASSERT(diffuseTex.getTexId() != BAD_TEXTUREID);
  normalTex = dag::get_tex_gameres(normalName.c_str());
  G_ASSERT(normalTex.getTexId() != BAD_TEXTUREID);

  TextureInfo info;
  diffuseTex->getinfo(info);
  displacemenTexSize = IPoint2(info.w, info.h);

  const char *generatorShader = "spline_gen_generator";
  generatorCs = new_compute_shader(generatorShader);
  G_ASSERTF(generatorCs, "can't create computeShader %s", generatorShader);

  const char *normalShader = "spline_gen_normal_calculator";
  normalCs = new_compute_shader(normalShader);
  G_ASSERTF(normalCs, "can't create computeShader %s", normalShader);

  const char *cullerShader = "dynamic_spline_gen_culler";
  cullerCs = new_compute_shader(cullerShader);
  G_ASSERTF(cullerCs, "can't create computeShader %s", cullerShader);

  ibPtr = get_spline_gen_repository().getOrMakeIb(slices, stripes);
  if (!asset_name.empty())
    assetPtr = get_spline_gen_repository().getOrMakeAsset(asset_name);
  shapeManagerPtr = get_spline_gen_repository().getOrMakeShapeManager();
}

SplineGenGeometryManager::~SplineGenGeometryManager()
{
  // Vertex buffers passed to BVH will become dangling pointers after this manager is destroyed
  // so we need to teardown the BVH splinegen objects / instances.
  if (bvhContextId)
    bvh::remove_spline_gen_instances(bvhContextId);
}

InstanceId SplineGenGeometryManager::registerInstance()
{
  if (freeSpots.empty())
    makeMoreSpots();

  InstanceId id = freeSpots.back();
  freeSpots.pop_back();
  usedSpots.push_back(id);
  return id;
}

void SplineGenGeometryManager::eraseInstanceId(InstanceId id)
{
  inactiveSpots.erase_first_unsorted(id);
  usedSpots.erase_first_unsorted(id);
  freeSpots.erase_first_unsorted(id);
}

void SplineGenGeometryManager::unregisterInstance(InstanceId id)
{
  eraseInstanceId(id);
  freeSpots.push_back(id);
}

void SplineGenGeometryManager::makeInstanceActive(InstanceId id)
{
  eraseInstanceId(id);
  usedSpots.push_back(id);
}

void SplineGenGeometryManager::makeInstanceInactive(InstanceId id)
{
  eraseInstanceId(id);
  inactiveSpots.push_back(id);
}

void SplineGenGeometryManager::updateBuffers()
{
  if (getCurrentInstanceCount() == 0)
    return;

  allocateBuffers();
  uploadIndirectionBuffer();
  attachmentMaxNo = 0;
  objBatchIdData.clear();
  currentBufferIndex = (currentBufferIndex + 1) % 2;
}

void SplineGenGeometryManager::updateInstancingData(
  InstanceId id, SplineGenInstance &instance, const eastl::vector<SplineGenSpline, framemem_allocator> &spline_vec)
{
  G_ASSERT(id < getAllocatedInstanceCount());

  if (!splineBufferDirty)
  {
    auto lockedStagingBuffer = lock_sbuffer<SplineGenSpline>(splineStagingBuffer.getBuf(), 0,
      (stripes + 1) * getAllocatedInstanceCount(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    if (lockedStagingBuffer)
      lockedStagingBuffer.updateDataRange(id * (stripes + 1), spline_vec.data(), stripes + 1);
    else
      logerr("spline_gen: failed to lock staging buffer for instance %u", (unsigned)id);
  }
  else
  {
    splineStagingBuffer.getBuf()->updateData(id * splineEntrySize, splineEntrySize, spline_vec.data(),
      VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE);
  }
  splineBufferDirtyMask.set(id);
  splineBufferDirty = true;

  if (invalidatePrevBuffer)
    instance.flags &= ~PREV_SB_VALID;
}

void SplineGenGeometryManager::updateAttachmentBatchIds(
  InstanceId id, SplineGenInstance &instance, const eastl::vector<BatchId> &batch_ids)
{
  G_ASSERT(id < getAllocatedInstanceCount());

  uint32_t expectedBatches = hasObj() ? (instance.objCount + getAsset().batchSize - 1) / getAsset().batchSize : 0;
  G_UNUSED(expectedBatches);
  G_ASSERTF(batch_ids.size() == expectedBatches, "spline_gen: batch_ids.size()=%d != expected %d (objCount=%d, batchSize=%d)",
    (int)batch_ids.size(), (int)expectedBatches, (int)instance.objCount, (int)getAsset().batchSize);

  instance.batchIdsStart = objBatchIdData.size();
  for (BatchId batchId : batch_ids)
  {
    objBatchIdData.push_back(batchId);
  }
  attachmentMaxNo = max(attachmentMaxNo, instance.objCount);

  pendingInstanceWrites.emplace_back(id, instance);
}

void SplineGenGeometryManager::uploadGenerateData()
{
  invalidatePrevBuffer = false;
  reactivationInProcess = false;
  if (instancingBuffer.getBuf())
  {
    if (!pendingInstanceWrites.empty())
    {
      const int lockCount = getAllocatedInstanceCount();
      for (const auto &piw : pendingInstanceWrites)
        if (piw.first < lockCount)
          instancingBuffer.getBuf()->updateData(piw.first * sizeof(SplineGenInstance), sizeof(SplineGenInstance), &piw.second,
            VBLOCK_WRITEONLY);
    }
  }
  pendingInstanceWrites.clear();
  uploadSplineBuffer();
  uploadObjBatchIdBuffer();
}

void SplineGenGeometryManager::generate()
{
  if (getActiveInstanceCount() == 0)
    return;

  if (needsVertexGeneration)
  {
    TIME_D3D_PROFILE(spline_gen_geometry_generate);
    invalidatePrevBuffer = false;

    setInstancingShaderVars();
    SCOPED_SET_TEXTURE(var::spline_gen_texture_d.get_var_id(), diffuseTex);
    ShaderGlobal::set_int(var::spline_gen_geometry_refractive_variant, type == SplineGenType::REFRACTIVE_SPLINE_GEN ? 1 : 0);
    static int spline_gen_vertex_buffer_uav_no = ShaderGlobal::get_slot_by_name("spline_gen_vertex_buffer_uav_no");
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, spline_gen_vertex_buffer_uav_no, VALUE),
      vertexBuffer[currentBufferIndex].getBuf());

    int instanceVertexCnt = getInstanceVertexCount();
    int activeInstanceCnt = getActiveInstanceCount();
    generatorCs->dispatch(instanceVertexCnt, activeInstanceCnt, 1);
    normalCs->dispatch(instanceVertexCnt, activeInstanceCnt, 1);
  }

  if (attachmentMaxNo == 0)
    return;

  TIME_D3D_PROFILE(spline_gen_geometry_attach_objects);
  setInstancingShaderVars();
  SCOPED_SET_TEXTURE(var::spline_gen_texture_d.get_var_id(), diffuseTex);
  if (needsVertexGeneration)
    ShaderGlobal::set_buffer(var::spline_gen_vertex_buffer, vertexBuffer[currentBufferIndex]);
  getAsset().attachObjects(getActiveInstanceCount(), attachmentMaxNo);
}

void SplineGenGeometryManager::addInstancesToBVH(bvh::ContextId context_id)
{
  if (getCurrentInstanceCount() == 0)
    return;

  bvhContextId = context_id;

  // Can't add instances if there is no vertex buffer
  if (!needsVertexGeneration)
    return;

  TIME_D3D_PROFILE(spline_gen_geometry_add_to_bvh);

  eastl::vector<uint32_t> totalInstanceIds;
  totalInstanceIds.reserve(usedSpots.size() + inactiveSpots.size());
  totalInstanceIds.insert(totalInstanceIds.end(), usedSpots.begin(), usedSpots.end());
  totalInstanceIds.insert(totalInstanceIds.end(), inactiveSpots.begin(), inactiveSpots.end());

  eastl::vector<eastl::pair<uint32_t, bvh::MeshInfo>> bvhMeshes;
  bvhMeshes.reserve(totalInstanceIds.size());
  int instanceVertexCount = getInstanceVertexCount();
  int nextBufferIndex = (currentBufferIndex + 1) % 2;

  for (InstanceId instanceId : totalInstanceIds)
  {
    bvh::MeshInfo meshInfo;
    meshInfo.indices = ibPtr.lock()->indexBuffer.getBuf();
    meshInfo.indexCount = getInstanceTriangleCount() * 3;
    meshInfo.startIndex = 0;
    meshInfo.vertices = vertexBuffer[nextBufferIndex].getBuf();
    meshInfo.vertexCount = instanceVertexCount;
    meshInfo.startVertex = 0;
    meshInfo.vertexSize = sizeof(SplineGenVertex);
    meshInfo.baseVertex = instanceId * instanceVertexCount;
    meshInfo.positionOffset = offsetof(SplineGenVertex, worldPos);
    meshInfo.positionFormat = VSDT_FLOAT3;
    meshInfo.texcoordOffset = offsetof(SplineGenVertex, texcoord);
    meshInfo.texcoordFormat = VSDT_FLOAT2;
    meshInfo.normalOffset = offsetof(SplineGenVertex, normal);
    meshInfo.posMul = Point4::ONE;
    meshInfo.posAdd = Point4::ZERO;
    meshInfo.albedoTextureId = diffuseTex.getTexId();

    bvhMeshes.push_back(eastl::make_pair(instanceId, meshInfo));
  }

  bvh::gather_splinegen_instances(context_id, vertexBuffer[nextBufferIndex].getBuf(), bvhMeshes, instanceVertexCount, bvhId);
}

void SplineGenGeometryManager::removeBVHContext() { bvhContextId = nullptr; }

void SplineGenGeometryManager::uploadCameraCullData()
{
  int cascade = type == SplineGenType::REFRACTIVE_SPLINE_GEN ? CASCADE_TRANSPARENT : CASCADE_COLOR;
  uploadCullData(cascade);
}

void SplineGenGeometryManager::uploadShadowCullData()
{
  if (type == SplineGenType::REFRACTIVE_SPLINE_GEN)
    return;
  uploadCullData(CASCADE_SHADOW);
}

void SplineGenGeometryManager::uploadCullData(int cascade)
{
  if (getCurrentInstanceCount() == 0)
    return;

  eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> paramsData(1);
  paramsData[0].indexCountPerInstance = getInstanceTriangleCount() * 3;
  paramsData[0].instanceCount = 0;
  paramsData[0].startIndexLocation = 0;
  paramsData[0].baseVertexLocation = 0;
  paramsData[0].startInstanceLocation = 0;

  if (attachmentMaxNo > 0)
    getAsset().addIndirectParams(paramsData, assetLod);
  drawIndirectArgsNr = paramsData.size();
  uploadIndirectParams(paramsData, cascade);
}

void SplineGenGeometryManager::cullForCamera()
{
  int cascade = type == SplineGenType::REFRACTIVE_SPLINE_GEN ? CASCADE_TRANSPARENT : CASCADE_COLOR;
  cull(cascade);
}

void SplineGenGeometryManager::cullForShadow()
{
  if (type == SplineGenType::REFRACTIVE_SPLINE_GEN)
    return;
  cull(CASCADE_SHADOW);
}

void SplineGenGeometryManager::cull(int cascade)
{
  if (getCurrentInstanceCount() == 0)
    return;

  TIME_D3D_PROFILE(spline_gen_geometry_cull);
  setInstancingShaderVars();

  static int spline_gen_params_buffer_uav_no = ShaderGlobal::get_slot_by_name("spline_gen_params_buffer_uav_no");
  static int spline_gen_culled_buffer_uav_no = ShaderGlobal::get_slot_by_name("spline_gen_culled_buffer_uav_no");

  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, spline_gen_params_buffer_uav_no, VALUE), paramsBuffer[cascade].getBuf());
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, spline_gen_culled_buffer_uav_no, VALUE), culledBuffer[cascade].getBuf());

  ShaderGlobal::set_int(var::spline_gen_obj_elem_count, drawIndirectArgsNr);
  cullerCs->dispatchThreads(getCurrentInstanceCount(), 1, 1);
}

void SplineGenGeometryManager::renderTransparent()
{
  if (type != SplineGenType::REFRACTIVE_SPLINE_GEN)
    return;

  render(CASCADE_TRANSPARENT);
}

void SplineGenGeometryManager::renderOpaque(int cascade)
{
  if (type == SplineGenType::REFRACTIVE_SPLINE_GEN)
    return;

  render(cascade);
}

int SplineGenGeometryManager::getAdditionalTexVarId() const
{
  switch (type)
  {
    case SplineGenType::EMISSIVE_SPLINE_GEN: return var::spline_gen_texture_emissive_mask.get_var_id();
    case SplineGenType::SKIN_SPLINE_GEN: return var::spline_gen_texture_skin_ao.get_var_id();
    default: return -1;
  }
}

void SplineGenGeometryManager::render(int cascade)
{
  if (getCurrentInstanceCount() == 0)
    return;

  const int blocks[CASCADE_COUNT] = {dynamicSceneBlockId, dynamicDepthSceneBlockId, dynamicSceneTransBlockId};
  const int block = blocks[cascade];
  SCENE_LAYER_GUARD(block);

  setInstancingShaderVarsForRendering(cascade);

  if (auto lockedIb = ibPtr.lock())
  {
    TIME_D3D_PROFILE(spline_gen_geometry_render);
    SCOPED_SET_TEXTURE(var::spline_gen_texture_d.get_var_id(), diffuseTex);
    SCOPED_SET_TEXTURE(var::spline_gen_texture_n.get_var_id(), normalTex);
    auto additionalTexVarId = getAdditionalTexVarId();
    SCOPED_SET_TEXTURE(additionalTexVarId, additionalTex);

    shElem->setStates();
    d3d::setvsrc(0, nullptr, 0);
    d3d::setind(lockedIb->indexBuffer.getBuf());
    d3d::draw_indexed_indirect(PRIM_TRILIST, paramsBuffer[cascade].getBuf());
  }

  if (attachmentMaxNo > 0 && hasObj() && (cascade != CASCADE_TRANSPARENT))
    getAsset().renderObjects(paramsBuffer[cascade].getBuf(), assetLod);
}

const IPoint2 &SplineGenGeometryManager::getDisplacementTexSize() const { return displacemenTexSize; }

bool SplineGenGeometryManager::hasObj() const { return !assetPtr.expired(); }

SplineGenGeometryAsset &SplineGenGeometryManager::getAsset() const
{
  G_ASSERTF(hasObj(), "Always test for hasObj() before trying to access asset.");
  return *assetPtr.lock();
}

void SplineGenGeometryManager::reactivateAllInstances()
{
  reactivationInProcess = true;
  for (InstanceId inactiveId : inactiveSpots)
  {
    usedSpots.push_back(inactiveId);
  }
  inactiveSpots.clear();
}

bool SplineGenGeometryManager::isReactivationInProcess() const { return reactivationInProcess; }

int SplineGenGeometryManager::getAllocatedInstanceCount() const { return freeSpots.size() + usedSpots.size() + inactiveSpots.size(); }

int SplineGenGeometryManager::getCurrentInstanceCount() const { return usedSpots.size() + inactiveSpots.size(); }

int SplineGenGeometryManager::getActiveInstanceCount() const { return usedSpots.size(); }

int SplineGenGeometryManager::getInactiveInstanceCount() const { return inactiveSpots.size(); }

void SplineGenGeometryManager::makeMoreSpots()
{
  int currentCount = getAllocatedInstanceCount();
  int newCount = max(currentCount * 2, 16);
  for (int i = newCount - 1; i >= currentCount; i--)
    freeSpots.push_back(static_cast<InstanceId>(i));
  needsAllocation = true;
}

uint32_t SplineGenGeometryManager::getInstanceTriangleCount() const { return stripes * slices * 2; }

uint32_t SplineGenGeometryManager::getInstanceVertexCount() const { return (slices + 1) * (stripes + 1); }

void SplineGenGeometryManager::allocateBuffers()
{
  bool neededInBVH = is_bvh_enabled();
  if (!needsAllocation && needsVertexGeneration == (vertex_gen_phase_exists || neededInBVH))
    return;

  needsVertexGeneration = vertex_gen_phase_exists || neededInBVH;

  String buffName;
  buffName = String(0, "%s_splineGen_instancingBuffer", templateName.c_str());
  instancingBuffer = dag::create_sbuffer(sizeof(SplineGenInstance), getAllocatedInstanceCount(),
    SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, buffName);

  buffName = String(0, "%s_splineGen_splineStagingBuffer", templateName.c_str());
  splineStagingBuffer = dag::create_sbuffer(sizeof(SplineGenSpline), (stripes + 1) * getAllocatedInstanceCount(),
    SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_DYNAMIC, 0, buffName);

  if (needsVertexGeneration)
  {
    for (int i = 0; i < vertexBuffer.size(); i++)
    {
      buffName = String(0, "%s_vertexBuffer_%d", templateName.c_str(), i);
      vertexBuffer[i] = dag::buffers::create_ua_sr_structured(sizeof(SplineGenVertex),
        getInstanceVertexCount() * getAllocatedInstanceCount(), buffName);
    }
  }
  else
  {
    for (int i = 0; i < vertexBuffer.size(); i++)
      vertexBuffer[i].close();
  }

  for (int i = 0; i < splineBuffer.size(); i++)
  {
    buffName = String(0, "%s_splineGen_splineBuffer_%d", templateName.c_str(), i);
    splineBuffer[i] = dag::create_sbuffer(sizeof(SplineGenSpline), (stripes + 1) * getAllocatedInstanceCount(),
      SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, buffName);
  }

  // add 1 bit in the end of array to simplify range loop
  splineBufferDirtyMask.resize(getAllocatedInstanceCount() + 1);
  splineBufferDirtyMask.reset();
  splineBufferDirty = false;

  buffName = String(0, "%s_splineGen_indirectionBuffer", templateName.c_str());
  indirectionBuffer = dag::create_sbuffer(sizeof(InstanceId), getAllocatedInstanceCount(),
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, TEXFMT_R32UI, buffName);

  for (int i = 0; i < CASCADE_COUNT; ++i)
  {
    buffName = String(0, "%s_splineGen_culledBuffer%u", templateName.c_str(), i);
    culledBuffer[i] = dag::buffers::create_ua_sr_structured(sizeof(InstanceId), getAllocatedInstanceCount(), buffName);
  }

  needsAllocation = false;
  invalidatePrevBuffer = true;
  reactivateAllInstances();
}

void SplineGenGeometryManager::uploadIndirectionBuffer()
{
  if (auto indirectionData =
        lock_sbuffer<InstanceId>(indirectionBuffer.getBuf(), 0, getAllocatedInstanceCount(), VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    indirectionData.updateDataRange(0, usedSpots.data(), getActiveInstanceCount());
    indirectionData.updateDataRange(getActiveInstanceCount(), inactiveSpots.data(), getInactiveInstanceCount());
  }
}

void SplineGenGeometryManager::uploadObjBatchIdBuffer()
{
  G_ASSERT(hasObj() || attachmentMaxNo == 0);
  if (attachmentMaxNo == 0)
    return;
  if (objBatchIdData.size() > maxBatchesCount)
  {
    maxBatchesCount = objBatchIdData.size() + getAllocatedInstanceCount() - objBatchIdData.size() % getAllocatedInstanceCount();
    String buffName = String(0, "%s_objBatchIdBuffer", templateName.c_str());
    objBatchIdBuffer = dag::create_sbuffer(sizeof(BatchId), maxBatchesCount,
      SBCF_ZEROMEM | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, TEXFMT_R32UI, buffName);
  }

  objBatchIdBuffer.getBuf()->updateData(0, sizeof(BatchId) * objBatchIdData.size(), objBatchIdData.data(),
    VBLOCK_WRITEONLY | VBLOCK_DISCARD);
}

void SplineGenGeometryManager::uploadSplineBuffer()
{
  if (!splineBufferDirty)
    return;

  if (splineStagingBuffer.getBuf() == nullptr || splineBuffer[currentBufferIndex].getBuf() == nullptr)
    return;

  uint32_t offset = 0, size_bytes = 0;
  for (int i = 0, count = splineBufferDirtyMask.size(); i < count; ++i)
    if (splineBufferDirtyMask[i])
    {
      if (size_bytes == 0)
        offset = i * splineEntrySize;

      size_bytes += splineEntrySize;
    }
    // last bit is always 0, so no leftover processing is needed
    else if (size_bytes > 0)
    {
      splineStagingBuffer->copyTo(splineBuffer[currentBufferIndex].getBuf(), offset, offset, size_bytes);
      size_bytes = 0;
    }

  splineBufferDirtyMask.reset();
  splineBufferDirty = false;
}

void SplineGenGeometryManager::setInstancingShaderVarsForRendering(int cascade)
{
  ShaderGlobal::set_buffer(var::spline_gen_culled_buffer, culledBuffer[cascade]);
  if (needsVertexGeneration)
  {
    ShaderGlobal::set_buffer(var::spline_gen_vertex_buffer, vertexBuffer[currentBufferIndex]);
    ShaderGlobal::set_buffer(var::spline_gen_prev_vertex_buffer, vertexBuffer[(currentBufferIndex + 1) % 2]);
  }
  setInstancingShaderVars();
}

void SplineGenGeometryManager::setInstancingShaderVars()
{
  ShaderGlobal::set_int(var::spline_gen_has_generation_phase, vertex_gen_phase_exists ? 1 : 0);
  ShaderGlobal::set_int(var::spline_gen_slices, slices);
  ShaderGlobal::set_int(var::spline_gen_stripes, stripes);
  ShaderGlobal::set_int(var::spline_gen_vertex_count, getInstanceVertexCount());
  ShaderGlobal::set_int(var::spline_gen_instance_count, getCurrentInstanceCount());
  ShaderGlobal::set_int(var::spline_gen_active_instance_count, getActiveInstanceCount());
  ShaderGlobal::set_int(var::spline_gen_attachment_max_no, attachmentMaxNo);
  ShaderGlobal::set_buffer(var::spline_gen_instancing_buffer, instancingBuffer);
  ShaderGlobal::set_buffer(var::spline_gen_spline_buffer, splineBuffer[currentBufferIndex]);
  ShaderGlobal::set_buffer(var::spline_gen_prev_spline_buffer, splineBuffer[(currentBufferIndex + 1) % 2]);
  ShaderGlobal::set_buffer(var::spline_gen_indirection_buffer, indirectionBuffer);
  ShaderGlobal::set_buffer(var::spline_gen_obj_batch_id_buffer, objBatchIdBuffer);

  ShaderGlobal::set_sampler(var::spline_gen_texture_d_samplerstate, get_texture_separate_sampler(diffuseTex.getTexId()));
  ShaderGlobal::set_sampler(var::spline_gen_texture_n_samplerstate, get_texture_separate_sampler(normalTex.getTexId()));
  if (type == SplineGenType::EMISSIVE_SPLINE_GEN)
    ShaderGlobal::set_sampler(var::spline_gen_texture_emissive_mask_samplerstate,
      get_texture_separate_sampler(additionalTex.getTexId()));
  else if (type == SplineGenType::SKIN_SPLINE_GEN && additionalTex.getTexId() != BAD_TEXTUREID)
    ShaderGlobal::set_sampler(var::spline_gen_texture_skin_ao_samplerstate, get_texture_separate_sampler(additionalTex.getTexId()));
  if (auto lockedShapeManager = shapeManagerPtr.lock())
    lockedShapeManager->bindShapes();
}

void SplineGenGeometryManager::uploadIndirectParams(const eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> &params_data,
  int cascade)
{
  if (params_data.size() > maxParamsCount[cascade])
  {
    maxParamsCount[cascade] = params_data.size();
    paramsBuffer[cascade].close();
    String buffName = String(0, "%s_splineGen_paramsBuffer%u", templateName.c_str(), cascade);
    paramsBuffer[cascade] = dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, params_data.size(), buffName);
  }

  paramsBuffer[cascade].getBuf()->updateData(0, DRAW_INDEXED_INDIRECT_BUFFER_SIZE * params_data.size(), params_data.data(),
    VBLOCK_WRITEONLY);
}

SplineGenGeometryShapeManager &SplineGenGeometryManager::getShapeManager() const
{
  G_ASSERTF(!shapeManagerPtr.expired(), "ShapeManager pointer is null. This should never happen.");
  return *shapeManagerPtr.lock();
}