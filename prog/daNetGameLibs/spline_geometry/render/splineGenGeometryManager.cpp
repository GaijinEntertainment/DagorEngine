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
  const eastl::string &asset_name,
  uint32_t asset_lod) :
  templateName(template_name),
  slices(slices_),
  stripes(stripes_),
  diffuseName(diffuse_name),
  normalName(normal_name),
  assetLod(asset_lod)
{
  const char *shader_name = "dynamic_spline_gen";
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

void SplineGenGeometryManager::makeInstanceInctive(InstanceId id)
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
  currentVbIndex = (currentVbIndex + 1) % 2;
  attachmentMaxNo = 0;
  objBatchIdData.clear();
}

void SplineGenGeometryManager::updateInstancingData(InstanceId id,
  SplineGenInstance &instance,
  const eastl::vector<SplineGenSpline, framemem_allocator> &spline_vec,
  const eastl::vector<BatchId> &batch_ids)
{
  G_ASSERT(id < getAllocatedInstanceCount());

  if (invalidatePrevVb)
    instance.flags &= ~PREV_VB_VALID;

  splineBuffer.getBuf()->updateData(id * sizeof(SplineGenSpline) * (stripes + 1), sizeof(SplineGenSpline) * (stripes + 1),
    spline_vec.data(), VBLOCK_WRITEONLY);

  updateInactive(id, instance, batch_ids);
}

void SplineGenGeometryManager::updateInactive(InstanceId id, SplineGenInstance &instance, const eastl::vector<BatchId> &batch_ids)
{
  G_ASSERT(id < getAllocatedInstanceCount());

  instance.batchIdsStart = objBatchIdData.size();
  for (BatchId batchId : batch_ids)
  {
    objBatchIdData.push_back(batchId);
  }
  attachmentMaxNo = max(attachmentMaxNo, instance.objCount);

  instancingBuffer.getBuf()->updateData(id * sizeof(SplineGenInstance), sizeof(SplineGenInstance), &instance, VBLOCK_WRITEONLY);
}

void SplineGenGeometryManager::generate()
{
  reactivationInProcess = false;
  uploadObjBatchIdBufer();

  if (getActiveInstanceCount() == 0)
    return;

  {
    TIME_D3D_PROFILE(spline_gen_geometry_generate);
    invalidatePrevVb = false;

    setInstancingShaderVars();
    SCOPED_SET_TEXTURE(spline_gen_texture_dVarId, diffuseTex);
    static int spline_gen_vertex_buffer_uav_no = ShaderGlobal::get_slot_by_name("spline_gen_vertex_buffer_uav_no");
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, spline_gen_vertex_buffer_uav_no, VALUE), vertexBuffer[currentVbIndex].getBuf());

    int instanceVertexCnt = getInstanceVertexCount();
    int activeInstanceCnt = getActiveInstanceCount();
    generatorCs->dispatch(instanceVertexCnt, activeInstanceCnt, 1);
    normalCs->dispatch(instanceVertexCnt, activeInstanceCnt, 1);
  }

  if (attachmentMaxNo == 0)
    return;

  ShaderGlobal::set_buffer(spline_gen_vertex_bufferVarId, vertexBuffer[currentVbIndex]);

  getAsset().attachObjects(getActiveInstanceCount(), attachmentMaxNo);
}

void SplineGenGeometryManager::render(bool render_color)
{
  if (getCurrentInstanceCount() == 0)
    return;

  const int block = render_color ? dynamicSceneBlockId : dynamicDepthSceneBlockId;
  SCENE_LAYER_GUARD(block);

  setInstancingShaderVars();

  {
    TIME_D3D_PROFILE(spline_gen_geometry_render);

    {
      eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> paramsData(1);
      paramsData[0].indexCountPerInstance = getInstanceTriangleCount() * 3;
      paramsData[0].instanceCount = 0;
      paramsData[0].startIndexLocation = 0;
      paramsData[0].baseVertexLocation = 0;
      paramsData[0].startInstanceLocation = 0;


      if (attachmentMaxNo > 0)
        getAsset().addIndirectParams(paramsData, assetLod);
      ShaderGlobal::set_int(spline_gen_obj_elem_countVarId, paramsData.size() - 1);
      uploadIndirectParams(paramsData);

      static int spline_gen_params_buffer_uav_no = ShaderGlobal::get_slot_by_name("spline_gen_params_buffer_uav_no");
      static int spline_gen_culled_buffer_uav_no = ShaderGlobal::get_slot_by_name("spline_gen_culled_buffer_uav_no");

      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, spline_gen_params_buffer_uav_no, VALUE), paramsBuffer.getBuf());
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, spline_gen_culled_buffer_uav_no, VALUE), culledBuffer.getBuf());

      cullerCs->dispatchThreads(getCurrentInstanceCount(), 1, 1);
    }

    ShaderGlobal::set_buffer(spline_gen_vertex_bufferVarId, vertexBuffer[currentVbIndex]);
    ShaderGlobal::set_buffer(spline_gen_prev_vertex_bufferVarId, vertexBuffer[(currentVbIndex + 1) % 2]);
    SCOPED_SET_TEXTURE(spline_gen_texture_dVarId, diffuseTex);
    SCOPED_SET_TEXTURE(spline_gen_texture_nVarId, normalTex);

    shElem->setStates();
    d3d::setvsrc(0, nullptr, 0);
    d3d::setind(ibPtr->indexBuffer.getBuf());
    d3d::draw_indexed_indirect(PRIM_TRILIST, paramsBuffer.getBuf());
  }

  if (attachmentMaxNo > 0 && hasObj())
    getAsset().renderObjects(paramsBuffer.getBuf(), assetLod);
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

  for (int i = 0; i < vertexBuffer.size(); i++)
  {
    vertexBuffer[i].close();
    buffName = String(0, "%s_vertexBuffer_%d", templateName.c_str(), i);
    vertexBuffer[i] =
      dag::buffers::create_ua_sr_structured(sizeof(SplineGenVertex), getInstanceVertexCount() * getAllocatedInstanceCount(), buffName);
  }

  instancingBuffer.close();
  buffName = String(0, "%s_instancingBuffer", templateName.c_str());
  instancingBuffer = dag::create_sbuffer(sizeof(SplineGenInstance), getAllocatedInstanceCount(),
    SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, 0, buffName);

  splineBuffer.close();
  buffName = String(0, "%s_splineBuffer", templateName.c_str());
  splineBuffer = dag::create_sbuffer(sizeof(SplineGenSpline), (stripes + 1) * getAllocatedInstanceCount(),
    SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, 0, buffName);

  indirectionBuffer.close();
  buffName = String(0, "%s_indirectionBuffer", templateName.c_str());
  indirectionBuffer = dag::create_sbuffer(sizeof(InstanceId), getAllocatedInstanceCount(),
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC, TEXFMT_R32UI, buffName);

  culledBuffer.close();
  buffName = String(0, "%s_culledBuffer", templateName.c_str());
  culledBuffer = dag::buffers::create_ua_sr_structured(sizeof(InstanceId), getAllocatedInstanceCount(), buffName);

  needsAllocation = false;
  invalidatePrevVb = true;
  reactivateAllInstances();
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

void SplineGenGeometryManager::setInstancingShaderVars()
{
  ShaderGlobal::set_int(spline_gen_slicesVarId, slices);
  ShaderGlobal::set_int(spline_gen_stripesVarId, stripes);
  ShaderGlobal::set_int(spline_gen_vertex_countVarId, getInstanceVertexCount());
  ShaderGlobal::set_int(spline_gen_instance_countVarId, getCurrentInstanceCount());
  ShaderGlobal::set_int(spline_gen_active_instance_countVarId, getActiveInstanceCount());
  ShaderGlobal::set_int(spline_gen_attachment_max_noVarId, attachmentMaxNo);
  ShaderGlobal::set_buffer(spline_gen_instancing_bufferVarId, instancingBuffer);
  ShaderGlobal::set_buffer(spline_gen_spline_bufferVarId, splineBuffer);
  ShaderGlobal::set_buffer(spline_gen_indirection_bufferVarId, indirectionBuffer);
  ShaderGlobal::set_buffer(spline_gen_culled_bufferVarId, culledBuffer);
  ShaderGlobal::set_buffer(spline_gen_obj_batch_id_bufferVarId, objBatchIdBuffer);
}

void SplineGenGeometryManager::uploadIndirectParams(const eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> &params_data)
{
  if (params_data.size() > maxParamsCount)
  {
    maxParamsCount = params_data.size();
    paramsBuffer.close();
    String buffName = String(0, "%s_paramsBuffer", templateName.c_str());
    paramsBuffer = dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, params_data.size(), buffName);
  }

  paramsBuffer.getBuf()->updateData(0, DRAW_INDEXED_INDIRECT_BUFFER_SIZE * params_data.size(), params_data.data(), VBLOCK_WRITEONLY);
}
