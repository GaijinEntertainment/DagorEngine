// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineGenGeometryRepository.h"
#include "splineGenGeometryShaderVar.h"
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_shaderBlock.h>
#include <gameRes/dag_gameResources.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <3d/dag_lockSbuffer.h>

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
  else if (strcmp(shader_type.c_str(), "regular") == 0)
  {
    type = SplineGenType::REGULAR_SPLINE_GEN;
  }
  else
  {
    G_ASSERTF(false, "Unknown spline_gen_template__shader_type '%s' (Valid types: 'regular' or 'emissive')", shader_type.c_str());
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

  const char *cullerShader = "dynamic_spline_gen_culler";
  cullerCs = new_compute_shader(cullerShader);
  G_ASSERTF(cullerCs, "can't create computeShader %s", cullerShader);

  ibPtr = &get_spline_gen_repository().getOrMakeIb(slices, stripes);
  if (!asset_name.empty())
    assetPtr = &get_spline_gen_repository().getOrMakeAsset(asset_name);
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

void SplineGenGeometryManager::unregisterInstance(InstanceId id, bool active)
{
  freeSpots.push_back(id);
  if (active)
    usedSpots.erase_first_unsorted(id);
  else
    inactiveSpots.erase_first_unsorted(id);
}

void SplineGenGeometryManager::makeInstanceActive(InstanceId id)
{
  usedSpots.push_back(id);
  inactiveSpots.erase_first_unsorted(id);
}

void SplineGenGeometryManager::makeInstanceInactive(InstanceId id)
{
  inactiveSpots.push_back(id);
  usedSpots.erase_first_unsorted(id);
}

void SplineGenGeometryManager::updateBuffers()
{
  if (getCurrentInstanceCount() == 0)
    return;

  allocateBuffers();
  uploadIndirectionBuffer();
  attachmentMaxNo = 0;
  objBatchIdData.clear();
  currentSBIndex = (currentSBIndex + 1) % 2;
}

void SplineGenGeometryManager::updateInstancingData(
  InstanceId id, SplineGenInstance &instance, const eastl::vector<SplineGenSpline, framemem_allocator> &spline_vec)
{
  G_ASSERT(id < getAllocatedInstanceCount());

  splineBuffer[currentSBIndex].getBuf()->updateData(id * sizeof(SplineGenSpline) * (stripes + 1),
    sizeof(SplineGenSpline) * (stripes + 1), spline_vec.data(), VBLOCK_WRITEONLY);

  if (invalidatePrevSb)
    instance.flags &= ~PREV_SB_VALID;
}

void SplineGenGeometryManager::updateAttachmentBatchIds(
  InstanceId id, SplineGenInstance &instance, const eastl::vector<BatchId> &batch_ids)
{
  G_ASSERT(id < getAllocatedInstanceCount());

  instance.batchIdsStart = objBatchIdData.size();
  for (BatchId batchId : batch_ids)
  {
    objBatchIdData.push_back(batchId);
  }
  attachmentMaxNo = max(attachmentMaxNo, instance.objCount);

  instancingStagingBuffer.getBuf()->updateData(id * sizeof(SplineGenInstance), sizeof(SplineGenInstance), &instance, VBLOCK_WRITEONLY);
}

void SplineGenGeometryManager::uploadGenerateData()
{
  invalidatePrevSb = false;
  reactivationInProcess = false;
  if (instancingStagingBuffer.getBuf() && instancingBuffer.getBuf())
    instancingStagingBuffer.getBuf()->copyTo(instancingBuffer.getBuf());
  uploadObjBatchIdBufer();
}

void SplineGenGeometryManager::attachObjects()
{
  if (attachmentMaxNo == 0)
    return;

  TIME_D3D_PROFILE(spline_gen_geometry_attach_objects);

  setInstancingShaderVars();
  SCOPED_SET_TEXTURE(var::spline_gen_texture_d.get_var_id(), diffuseTex);

  getAsset().attachObjects(getActiveInstanceCount(), attachmentMaxNo);
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
  ShaderGlobal::set_int(var::spline_gen_obj_elem_count, paramsData.size() - 1);
  uploadIndirectParams(paramsData, cascade);
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

  cullerCs->dispatchThreads(getCurrentInstanceCount(), 1, 1);
}

void SplineGenGeometryManager::render(int cascade)
{
  if (getCurrentInstanceCount() == 0)
    return;

  const int block = cascade == CASCADE_COLOR ? dynamicSceneBlockId : dynamicDepthSceneBlockId;
  SCENE_LAYER_GUARD(block);

  setInstancingShaderVarsForRendering(cascade);

  {
    TIME_D3D_PROFILE(spline_gen_geometry_render);
    SCOPED_SET_TEXTURE(var::spline_gen_texture_d.get_var_id(), diffuseTex);
    SCOPED_SET_TEXTURE(var::spline_gen_texture_n.get_var_id(), normalTex);
    auto additionalTexVarId = type == SplineGenType::EMISSIVE_SPLINE_GEN ? var::spline_gen_texture_emissive_mask.get_var_id()
                                                                         : var::spline_gen_texture_skin_ao.get_var_id();
    SCOPED_SET_TEXTURE(additionalTexVarId, additionalTex);

    shElem->setStates();
    d3d::setvsrc(0, nullptr, 0);
    d3d::setind(ibPtr->indexBuffer.getBuf());
    d3d::draw_indexed_indirect(PRIM_TRILIST, paramsBuffer[cascade].getBuf());
  }

  if (attachmentMaxNo > 0 && hasObj())
    getAsset().renderObjects(paramsBuffer[cascade].getBuf(), assetLod);
}

const IPoint2 &SplineGenGeometryManager::getDisplacementTexSize() const { return displacemenTexSize; }

bool SplineGenGeometryManager::hasObj() const { return static_cast<bool>(assetPtr); }

SplineGenGeometryAsset &SplineGenGeometryManager::getAsset() const
{
  G_ASSERTF(hasObj(), "Always test for hasObj() before trying to access asset.");
  return *assetPtr;
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
  if (!needsAllocation)
    return;

  String buffName;
  instancingStagingBuffer.close();
  buffName = String(0, "%i_instancingStagingBuffer", templateName.c_str());
  instancingStagingBuffer = dag::create_sbuffer(sizeof(SplineGenInstance), getAllocatedInstanceCount(),
    SBCF_MISC_STRUCTURED | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_DYNAMIC, 0, buffName);

  instancingBuffer.close();
  buffName = String(0, "%s_instancingBuffer", templateName.c_str());
  instancingBuffer = dag::create_sbuffer(sizeof(SplineGenInstance), getAllocatedInstanceCount(),
    SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, buffName);

  for (int i = 0; i < splineBuffer.size(); i++)
  {
    splineBuffer[i].close();
    buffName = String(0, "%s_splineBuffer_%d", templateName.c_str(), i);
    splineBuffer[i] = dag::create_sbuffer(sizeof(SplineGenSpline), (stripes + 1) * getAllocatedInstanceCount(),
      SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, 0, buffName);
  }

  indirectionBuffer.close();
  buffName = String(0, "%s_indirectionBuffer", templateName.c_str());
  indirectionBuffer = dag::create_sbuffer(sizeof(InstanceId), getAllocatedInstanceCount(),
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, TEXFMT_R32UI, buffName);

  for (int i = 0; i < CASCADE_COUNT; ++i)
  {
    culledBuffer[i].close();
    buffName = String(0, "%s_culledBuffer%u", templateName.c_str(), i);
    culledBuffer[i] = dag::buffers::create_ua_sr_structured(sizeof(InstanceId), getAllocatedInstanceCount(), buffName);
  }

  needsAllocation = false;
  invalidatePrevSb = true;
  reactivateAllInstances();

  attachObjects();
}

void SplineGenGeometryManager::uploadIndirectionBuffer()
{
  if (auto indirectionData =
        lock_sbuffer<InstanceId>(indirectionBuffer.getBuf(), 0, getCurrentInstanceCount(), VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    indirectionData.updateDataRange(0, usedSpots.data(), getActiveInstanceCount());
    indirectionData.updateDataRange(getActiveInstanceCount(), inactiveSpots.data(), getInactiveInstanceCount());
  }
}

void SplineGenGeometryManager::uploadObjBatchIdBufer()
{
  G_ASSERT(hasObj() || attachmentMaxNo == 0);
  if (attachmentMaxNo == 0)
    return;
  if (objBatchIdData.size() > maxBatchesCount)
  {
    maxBatchesCount = objBatchIdData.size() + getAllocatedInstanceCount() - objBatchIdData.size() % getAllocatedInstanceCount();
    objBatchIdBuffer.close();
    String buffName = String(0, "%s_objBatchIdBuffer", templateName.c_str());
    objBatchIdBuffer = dag::create_sbuffer(sizeof(BatchId), maxBatchesCount,
      SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, TEXFMT_R32UI, buffName);
  }

  objBatchIdBuffer.getBuf()->updateData(0, sizeof(BatchId) * objBatchIdData.size(), objBatchIdData.data(),
    VBLOCK_WRITEONLY | VBLOCK_DISCARD);
}

void SplineGenGeometryManager::setInstancingShaderVarsForRendering(int cascade)
{
  ShaderGlobal::set_buffer(var::spline_gen_culled_buffer, culledBuffer[cascade]);
  setInstancingShaderVars();
}

void SplineGenGeometryManager::setInstancingShaderVars()
{
  ShaderGlobal::set_int(var::spline_gen_slices, slices);
  ShaderGlobal::set_int(var::spline_gen_stripes, stripes);
  ShaderGlobal::set_int(var::spline_gen_vertex_count, getInstanceVertexCount());
  ShaderGlobal::set_int(var::spline_gen_instance_count, getCurrentInstanceCount());
  ShaderGlobal::set_int(var::spline_gen_active_instance_count, getActiveInstanceCount());
  ShaderGlobal::set_int(var::spline_gen_attachment_max_no, attachmentMaxNo);
  ShaderGlobal::set_buffer(var::spline_gen_instancing_buffer, instancingBuffer);
  ShaderGlobal::set_buffer(var::spline_gen_spline_buffer, splineBuffer[currentSBIndex]);
  ShaderGlobal::set_buffer(var::spline_gen_prev_spline_buffer, splineBuffer[(currentSBIndex + 1) % 2]);
  ShaderGlobal::set_buffer(var::spline_gen_indirection_buffer, indirectionBuffer);
  ShaderGlobal::set_buffer(var::spline_gen_obj_batch_id_buffer, objBatchIdBuffer);

  ShaderGlobal::set_sampler(var::spline_gen_texture_d_samplerstate, get_texture_separate_sampler(diffuseTex.getTexId()));
  ShaderGlobal::set_sampler(var::spline_gen_texture_n_samplerstate, get_texture_separate_sampler(normalTex.getTexId()));
  if (type == SplineGenType::EMISSIVE_SPLINE_GEN)
    ShaderGlobal::set_sampler(var::spline_gen_texture_emissive_mask_samplerstate,
      get_texture_separate_sampler(additionalTex.getTexId()));
  else if (type == SplineGenType::SKIN_SPLINE_GEN && additionalTex.getTexId() != BAD_TEXTUREID)
    ShaderGlobal::set_sampler(var::spline_gen_texture_skin_ao_samplerstate, get_texture_separate_sampler(additionalTex.getTexId()));
}

void SplineGenGeometryManager::uploadIndirectParams(const eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> &params_data,
  int cascade)
{
  if (params_data.size() > maxParamsCount[cascade])
  {
    maxParamsCount[cascade] = params_data.size();
    paramsBuffer[cascade].close();
    String buffName = String(0, "%s_paramsBuffer%u", templateName.c_str(), cascade);
    paramsBuffer[cascade] = dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, params_data.size(), buffName);
  }

  paramsBuffer[cascade].getBuf()->updateData(0, DRAW_INDEXED_INDIRECT_BUFFER_SIZE * params_data.size(), params_data.data(),
    VBLOCK_WRITEONLY);
}
