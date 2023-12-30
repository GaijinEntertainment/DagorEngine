#pragma once

#include <math/integer/dag_IPoint2.h>
#include <3d/dag_renderStateId.h>
#include <shaders/dag_shaders.h>

namespace rendinst::render
{

struct RiGenRenderRecord
{
  enum : uint32_t
  {
    DISABLE_OPTIMIZATION_BIT_STATE = 0x80000000
  };
  enum Visibility : uint8_t
  {
    INVALID,
    PER_INSTANCE,
    PER_CELL,
  };
  const ShaderElement *curShader;
  uint32_t prog;
  shaders::RenderStateId rstate;
  uint32_t state;
  uint32_t tstate;
  uint32_t cstate;
  uint16_t variant;
  uint16_t poolOrder;
  uint16_t vstride;
  uint32_t offset;
  uint32_t count;
  int poolIdx;
  int startIndex;
  int numFaces;
  int baseVertex;
  uint8_t vbIdx;
  int8_t drawOrder;
  uint8_t meshDebugValue;
  uint8_t stage;
  Visibility visibility;
  uint8_t instanceLod;
  RiGenRenderRecord(const ShaderElement *cur_shader, int variant, uint32_t prog, uint32_t state, shaders::RenderStateId rstate,
    uint32_t tstate, uint32_t cstate, uint16_t pool_order, int8_t draw_order, uint8_t stage, uint16_t vstride, uint8_t vb_idx,
    uint32_t offset, uint32_t count, int pool_idx, int start_index, int num_faces, int base_vertex, Visibility visibility,
    uint8_t instance_lod, uint8_t mesh_debug_value) :
    curShader(cur_shader),
    variant(variant < 0 ? ~0 : variant),
    prog(prog),
    state(state),
    rstate(rstate),
    tstate(tstate),
    cstate(cstate),
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
    visibility(visibility),
    instanceLod(instance_lod),
    meshDebugValue(mesh_debug_value)
  {}
};

} // namespace rendinst::render