// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_stdint.h>
#include <util/dag_oaHashNameMap.h>
#include <generic/dag_staticTab.h>
#include <math/dag_frustum.h>
#include <math/dag_TMatrix.h>
#include <math/dag_hlsl_floatx.h>
#include <math/integer/dag_IPoint4.h>
#include <EASTL/vector.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_quadIndexBuffer.h>
#include <frustumCulling/frustumPlanes.h>
#include <projectiveDecals/projective_decals_const.hlsli>
#include <projectiveDecals/projectiveDecalsBuffer.h>
#include "projectiveDecals/projectiveDecals.h"

namespace var
{
static ShaderVariableInfo projective_decals_to_update_count_const_no("projective_decals_to_update_count_const_no", true);
static ShaderVariableInfo projective_decals_original_count_const_no("projective_decals_original_count_const_no", true);
static ShaderVariableInfo projective_decals_target_uav_no("projective_decals_target_uav_no", true);
static ShaderVariableInfo projective_decals_target_count_uav_no("projective_decals_target_count_uav_no", true);
static ShaderVariableInfo projective_decals_original_uav_no("projective_decals_original_uav_no", true);
static ShaderVariableInfo decal_buffer("decal_buffer", true);
} // namespace var

bool ProjectiveDecalsBase::init(const char *generator_shader_name, const char *decal_shader_name, const char *decal_cull_shader,
  const char *decal_create_indirect, size_t update_struct_size)
{
  close();
  indirectBuffer = dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, 1, "decalIndirectBuffer", RESTAG_DECALS);
  setUpIndirectBuffer();
  updateLengthPerInstance = update_struct_size / sizeof(Point4);

  // Create material.
  decalRenderer.init(decal_shader_name, NULL, 0, "projectiveDecal");

  decalUpdater.reset(new_compute_shader(generator_shader_name));
  decalCuller.reset(new_compute_shader(decal_cull_shader));
  decalCreator.reset(new_compute_shader(decal_create_indirect));
  if (!decalUpdater || !decalCuller || !decalCreator)
    return false;

  index_buffer::init_box();

  decalsToUpdateBuf = d3d::buffers::create_one_frame_cb((sizeof(Point4) * updateLengthPerInstance * MAX_DECALS_TO_UPDATE) >> 4,
    "decalsToUpdateCB", RESTAG_DECALS);

  return true;
}

void ProjectiveDecalsBase::setUpIndirectBuffer()
{
  DrawIndexedIndirectArgs *indirectData;
  if (indirectBuffer->lock(0, 0, (void **)&indirectData, VBLOCK_WRITEONLY) && indirectData)
  {
    constexpr int INDICIES_PER_PARTICLE = 6 * 6;
    indirectData->indexCountPerInstance = INDICIES_PER_PARTICLE;
    indirectData->instanceCount = 0;
    indirectData->startIndexLocation = 0;
    indirectData->baseVertexLocation = 0;
    indirectData->startInstanceLocation = 0;
    indirectBuffer->unlock();
  }
}

int ProjectiveDecalsBase::findDecal(uint32_t decal_id)
{
  int idx = -1;
  for (int findIdx = 0; findIdx < decalsToUpdateData.size(); findIdx += updateLengthPerInstance)
  {
    if (getDecalId(make_span_const(decalsToUpdateData.begin() + findIdx, updateLengthPerInstance)) == decal_id)
    {
      idx = findIdx;
      break;
    }
  }

  return idx;
}

uint32_t ProjectiveDecalsBase::getDecalId(dag::ConstSpan<Point4> decal_data)
{
  return reinterpret_cast<const DecalDataBase *>(decal_data.data())->decal_id;
}

void ProjectiveDecalsBase::setDecalId(dag::Span<Point4> decal_data, uint32_t decal_id)
{
  reinterpret_cast<DecalDataBase *>(decal_data.data())->decal_id = decal_id;
}


void ProjectiveDecalsBase::updateDecal(dag::ConstSpan<Point4> decal_data)
{
  G_ASSERT(decal_data.size() == updateLengthPerInstance);

  uint decal_id = reinterpret_cast<const DecalDataBase *>(decal_data.data())->decal_id;
  int idx = findDecal(decal_id);

  if (idx == -1)
    idx = append_items(decalsToUpdateData, updateLengthPerInstance);

  memcpy(decalsToUpdateData.begin() + idx, decal_data.data(), decal_data.size() * sizeof(Point4));
}

dag::Span<Point4> ProjectiveDecalsBase::getSrcData(uint32_t decal_id)
{
  int idx = findDecal(decal_id);

  if (idx == -1)
    return {};

  return make_span(decalsToUpdateData.begin() + idx, updateLengthPerInstance);
}

void ProjectiveDecalsBase::prepareRender(const Frustum &frustum)
{
  if (decalsToRender == 0 && decalsToUpdateData.size() == 0) // there are no decals
    return;

  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, var::projective_decals_original_uav_no.get_int(), VALUE),
    buffer.decalInstances.get());
  while (!decalsToUpdateData.empty())
  {
    uint updateSize = min<uint>(decalsToUpdateData.size(), MAX_DECALS_TO_UPDATE * updateLengthPerInstance);
    uint numDecalsToUpdate = updateSize / updateLengthPerInstance;
    uint startIndex = decalsToUpdateData.size() - updateSize;

    G_ASSERT(startIndex % updateLengthPerInstance == 0);
    decalsToUpdateBuf->updateData(0, updateSize * sizeof(Point4), &decalsToUpdateData[startIndex], VBLOCK_WRITEONLY | VBLOCK_DISCARD);

    IPoint4 params(numDecalsToUpdate, 0, 0, 0);
    d3d::set_cs_const(var::projective_decals_to_update_count_const_no.get_int(), (float *)&params.x, 1);
    d3d::set_const_buffer(STAGE_CS, 1, decalsToUpdateBuf);
    decalUpdater->dispatch(1, 1, 1);
    d3d::resource_barrier({buffer.decalInstances.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

    decalsToUpdateData.resize(startIndex);
  }

  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, var::projective_decals_target_count_uav_no.get_int(), VALUE), indirectBuffer.get());
  decalCreator->dispatch(1, 1, 1); // clear

  set_frustum_planes(frustum);

  d3d::resource_barrier({indirectBuffer.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

  int decalsCount = decalsToRender;
  IPoint4 params{decalsCount, 0, 0, 0};
  d3d::set_cs_const(var::projective_decals_original_count_const_no.get_int(), (float *)&params.x, 1);
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, var::projective_decals_target_uav_no.get_int(), VALUE),
    buffer.culledInstances.getBuf());
  decalCuller->dispatch((decalsCount + DECALS_MAX_CULL_X - 1) / DECALS_MAX_CULL_X, 1, 1);
}

void ProjectiveDecalsBase::render()
{
  if (decalsToRender == 0)
    return;
  index_buffer::use_box();
  d3d::resource_barrier({indirectBuffer.get(), RB_RO_INDIRECT_BUFFER});
  d3d::resource_barrier({buffer.culledInstances.getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});
  ShaderGlobal::set_buffer(var::decal_buffer, buffer.culledInstances.getBufId());
  if (!decalRenderer.shader->setStates())
    return;
  d3d::setvsrc(0, 0, 0);
  d3d::draw_indexed_indirect(PRIM_TRILIST, indirectBuffer.get(), 0);
}


void ProjectiveDecalsBase::afterReset() { setUpIndirectBuffer(); }

void ProjectiveDecalsBase::clear()
{
  clear_and_shrink(decalsToUpdateData);
  decalsToRender = 0;
}

void ProjectiveDecalsBase::close()
{
  buffer.decalInstances.close();
  buffer.culledInstances.close();
  index_buffer::release_box();
  decalCreator.reset();
  decalCuller.reset();
  decalUpdater.reset();
  indirectBuffer.close();
  del_d3dres(decalsToUpdateBuf);
}


ProjectiveDecalsBase::~ProjectiveDecalsBase() { close(); }
