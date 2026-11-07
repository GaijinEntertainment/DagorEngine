// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "render/drawOrder.h"

#include <math/integer/dag_IPoint2.h>
#include <drv/3d/dag_renderStateId.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderState.h>
#include <shaders/dag_shaderVarsUtils.h>

namespace rendinst::render
{

struct RiGenRenderRecord
{
  enum Visibility : uint8_t
  {
    INVALID,
    PER_INSTANCE,
    PER_CELL,
  };
  const ShaderElement *curShader;
  shaders::CombinedDynVariantState dvState;
  uint16_t poolOrder;
  uint16_t vstride;
  uint32_t offset;
  uint32_t count;
  int poolIdx;
  int startIndex;
  int numFaces;
  int baseVertex;
  int numVertex;
  int startVertex;
  uint32_t voxelSurfaceId;
  uint32_t voxelDataOffset;
  uint8_t primitive;
  uint8_t vbIdx;
  PackedDrawOrder drawOrder;
  uint8_t meshDebugValue;
  uint8_t stage;
  Visibility visibility;
  uint8_t instanceLod;
  uint8_t isSWVertexFetch : 1;
  RiGenRenderRecord(const ShaderElement *cur_shader, const shaders::CombinedDynVariantState &dv_state, uint16_t pool_order,
    PackedDrawOrder draw_order, uint8_t stage, uint16_t vstride, uint8_t vb_idx, uint32_t offset, uint32_t count, int pool_idx,
    int start_index, int num_faces, int base_vertex, int num_vertex, int start_vertex, uint32_t voxel_surface, uint32_t voxel_offset,
    uint8_t primitive, Visibility visibility, uint8_t instance_lod, uint8_t mesh_debug_value, bool sw_vertex_fetch) :
    curShader(cur_shader),
    dvState(dv_state),
    poolOrder(pool_order),
    vstride(vstride),
    vbIdx(vb_idx),
    offset(offset),
    count(count),
    poolIdx(pool_idx),
    drawOrder(draw_order),
    stage(stage),
    startIndex(start_index),
    numFaces(num_faces),
    baseVertex(base_vertex),
    numVertex(num_vertex),
    startVertex(start_vertex),
    voxelSurfaceId(voxel_surface),
    voxelDataOffset(voxel_offset),
    primitive(primitive),
    visibility(visibility),
    instanceLod(instance_lod),
    meshDebugValue(mesh_debug_value),
    isSWVertexFetch(sw_vertex_fetch)
  {}
};

} // namespace rendinst::render
