// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include <math/dag_frustum.h>
#include <projectiveDecals/projectiveDecals.h>
#include <util/dag_oaHashNameMap.h>

#include <shaders/dag_computeShaders.h>
#include <math/integer/dag_IPoint4.h>
#include <3d/dag_quadIndexBuffer.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_resPtr.h>

#include <projectiveDecals/projective_decals_const.hlsli>
#include <frustumCulling/frustumPlanes.h>


static const int INDICIES_PER_PARTICLE = 6 * 6;


static int decal_bufferVarId = -1;

#define PROJECTIVE_DECALS_CONST_LIST     \
  VAR(projective_decals_to_update_count) \
  VAR(projective_decals_to_update)       \
  VAR(projective_decals_original_count)
#define VAR(a) static ShaderVariableInfo a##_const_no = -1;
PROJECTIVE_DECALS_CONST_LIST
#undef VAR

#define PROJECTIVE_DECALS_UAV_LIST    \
  VAR(projective_decals_target)       \
  VAR(projective_decals_target_count) \
  VAR(projective_decals_original)
#define VAR(a) static ShaderVariableInfo a##_uav_no = -1;
PROJECTIVE_DECALS_UAV_LIST
#undef VAR

bool ProjectiveDecals::init(const char *generator_shader_name, const char *decal_shader_name, const char *decal_cull_shader,
  const char *decal_create_indirect)
{
  close();
  indirectBuffer = dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, 1, "decalIndirectBuffer");
  setUpIndirectBuffer();

  // Create material.
  decalRenderer.init(decal_shader_name, NULL, 0, "projectiveDecal");

  decalUpdater.reset(new_compute_shader(generator_shader_name));
  decalCuller.reset(new_compute_shader(decal_cull_shader));
  decalCreator.reset(new_compute_shader(decal_create_indirect));
  if (!decalUpdater || !decalCuller || !decalCreator)
    return false;

  decal_bufferVarId = ::get_shader_variable_id("decal_buffer");

  index_buffer::init_box();

#define VAR(a) a##_const_no = get_shader_variable_id(#a "_const_no");
  PROJECTIVE_DECALS_CONST_LIST
#undef VAR

#define VAR(a) a##_uav_no = get_shader_variable_id(#a "_uav_no");
  PROJECTIVE_DECALS_UAV_LIST
#undef VAR

  return true;
}

void ProjectiveDecals::setUpIndirectBuffer()
{
  DrawIndexedIndirectArgs *indirectData;
  if (indirectBuffer->lock(0, 0, (void **)&indirectData, VBLOCK_WRITEONLY) && indirectData)
  {
    indirectData->indexCountPerInstance = INDICIES_PER_PARTICLE;
    indirectData->instanceCount = 0;
    indirectData->startIndexLocation = 0;
    indirectData->baseVertexLocation = 0;
    indirectData->startInstanceLocation = 0;
    indirectBuffer->unlock();
  }
}


void ProjectiveDecals::updateDecal(uint32_t decal_id, const TMatrix &transform, float rad, uint16_t texture_id, uint16_t matrix_id,
  const Point4 &params, uint16_t flags)
{
  const Point3 &pos = transform.getcol(3);
  const Point3 &norm = normalize(transform.getcol(1));
  const Point3 &up = normalize(transform.getcol(2));

  // updates must be merged in single index, otherwise we hit data race in updater CS
  //(writing to same index by different threads in group)
  int idx = -1;
  for (int findIdx = 0; findIdx < decalsToUpdate.size(); findIdx++)
    if (decalsToUpdate[findIdx].decal_id == decal_id)
    {
      idx = findIdx;
      break;
    }
  if (idx == -1)
  {
    idx = append_items(decalsToUpdate, 1);
    decalsToUpdate[idx].decal_id = decal_id;
    decalsToUpdate[idx].tid_mid_flags = 0;
  }

  if (flags & UPDATE_ALL)
  {
    decalsToUpdate[idx].pos_size = Point4::xyzV(pos, rad);
    decalsToUpdate[idx].normal = norm;
    decalsToUpdate[idx].decal_id = decal_id;
    decalsToUpdate[idx].up = up;
    uint32_t tex_id_matrix_id = (texture_id & 0x7f) | (matrix_id << 7);
    decalsToUpdate[idx].tid_mid_flags = (tex_id_matrix_id << 16);
    decalsToUpdate[idx].params = params;
  }
  else
  {
    // partial updates
    if (flags & UPDATE_PARAM_Z)
      decalsToUpdate[idx].params.z = params.z;
  }

  decalsToUpdate[idx].tid_mid_flags |= flags;
}

void ProjectiveDecals::updateParams(uint32_t decal_id, const Point4 &params, uint16_t flags)
{
  updateDecal(decal_id, TMatrix::ZERO, 0, 0, 0, params, flags & UPDATE_PARAM_Z);
}


void ProjectiveDecals::prepareRender(const Frustum &frustum)
{
  if (decalsToRender == 0 && decalsToUpdate.size() == 0) // there are no decals
    return;

  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, projective_decals_original_uav_no.get_int(), VALUE), buffer.decalInstances.get());
  while (!decalsToUpdate.empty())
  {
    uint numToUpdate = min<uint>(decalsToUpdate.size(), MAX_DECALS_TO_UPDATE);
    uint startIndex = decalsToUpdate.size() - numToUpdate;
    IPoint4 params(numToUpdate, 0, 0, 0);
    d3d::set_cs_const(projective_decals_to_update_count_const_no.get_int(), (float *)&params.x, 1);
    d3d::set_cs_const(projective_decals_to_update_const_no.get_int(), (float *)&decalsToUpdate[startIndex],
      numToUpdate * ((elem_size(decalsToUpdate) + 15) / 16));
    decalUpdater->dispatch(1, 1, 1);
    d3d::resource_barrier({buffer.decalInstances.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

    decalsToUpdate.resize(startIndex);
  }

  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, projective_decals_target_count_uav_no.get_int(), VALUE), indirectBuffer.get());
  decalCreator->dispatch(1, 1, 1); // clear

  set_frustum_planes(frustum);

  d3d::resource_barrier({indirectBuffer.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

  int decalsCount = decalsToRender;
  IPoint4 params{decalsCount, 0, 0, 0};
  d3d::set_cs_const(projective_decals_original_count_const_no.get_int(), (float *)&params.x, 1);
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, projective_decals_target_uav_no.get_int(), VALUE), buffer.culledInstances.getBuf());
  decalCuller->dispatch((decalsCount + DECALS_MAX_CULL_X - 1) / DECALS_MAX_CULL_X, 1, 1);
}

void ProjectiveDecals::render()
{
  if (decalsToRender == 0)
    return;
  index_buffer::use_box();
  d3d::resource_barrier({indirectBuffer.get(), RB_RO_INDIRECT_BUFFER});
  d3d::resource_barrier({buffer.culledInstances.getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});
  ShaderGlobal::set_buffer(decal_bufferVarId, buffer.culledInstances.getBufId());
  decalRenderer.shader->setStates();
  d3d::setvsrc(0, 0, 0);
  d3d::draw_indexed_indirect(PRIM_TRILIST, indirectBuffer.get(), 0);
}


void ProjectiveDecals::afterReset() { setUpIndirectBuffer(); }

void ProjectiveDecals::clear()
{
  clear_and_shrink(decalsToUpdate);
  decalsToRender = 0;
}

void ProjectiveDecals::close()
{
  buffer.decalInstances.close();
  buffer.culledInstances.close();
  index_buffer::release_box();
  decalCreator.reset();
  decalCuller.reset();
  decalUpdater.reset();
  indirectBuffer.close();
}


ProjectiveDecals::~ProjectiveDecals() { close(); }
