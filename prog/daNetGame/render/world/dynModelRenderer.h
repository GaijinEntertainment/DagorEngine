// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_smallTab.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/string.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <shaders/dag_shaderState.h>
#include <3d/dag_resPtr.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_Point4.h>
#include <3d/dag_texStreamingContext.h>
#include <ecs/render/renderPasses.h>
#include <shaders/dag_shSkinMesh.h>
#include <drv/3d/dag_renderStateId.h>
#include <3d/dag_ringDynBuf.h>
#include <shaders/dag_shaderVarsUtils.h>


struct UpdateStageInfoRender;
class GlobalVertexData;
class ShaderElement;
class DynamicRenderableSceneInstance;
class DynamicRenderableSceneResource;
class GlobalVariableStates;
namespace dynmodel_renderer
{
struct RenderElement;
}
DAG_DECLARE_RELOCATABLE(dynmodel_renderer::RenderElement);


namespace dynmodel_renderer
{

enum class RenderPriority : uint8_t
{
  HIGH = 0,
  DEFAULT = 1,
};

struct DynamicBufferHolder
{
  RingDynamicSB buffer;
  int curOffset = -1;
};

struct RenderElement
{
  ShaderElement *curShader;
  GlobalVertexData *vData;
  int curVar; // probably can be just int16_t
  uint32_t prog;
  ShaderStateBlockId state;
  shaders::RenderStateId rstate;
  shaders::TexStateIdx tstate;
  shaders::ConstStateIdx cstate;
  uint8_t reqTexLevel;
  RenderPriority priority;
#if DAGOR_DBGLEVEL > 0
  uint8_t lodNo;
#else
  static constexpr uint8_t lodNo = 0;
#endif
  uint32_t instanceId : 31; // uint16_t cnt;
  uint32_t needImmediateConstPS : 1;
  int si, numf, bv;              // can be 24 bit each, and so 12 bytes alltogether with instance Id, saving 4 bytes
  uint16_t bindposeBufferOffset; // maybe merge with something else, ~12 bits should be enough

  RenderElement(ShaderElement *curShader,
    int curVar,
    uint32_t prog,
    ShaderStateBlockId state,
    shaders::RenderStateId rstate,
    shaders::TexStateIdx tstate,
    shaders::ConstStateIdx cstate,
    GlobalVertexData *vData,
    uint8_t req_tex_level,
    RenderPriority priority,
#if DAGOR_DBGLEVEL > 0
    uint8_t lodNo,
#endif
    uint32_t instanceId,
    int si,
    int numf,
    int bv,
    bool needImmediateConstPS,
    int bindposeBufferOffset) :
    curShader(curShader),
    curVar(curVar),
    prog(prog),
    state(state),
    rstate(rstate),
    tstate(tstate),
    cstate(cstate),
    vData(vData),
    reqTexLevel(req_tex_level),
    priority(priority),
#if DAGOR_DBGLEVEL > 0
    lodNo(lodNo),
#endif
    instanceId(instanceId),
    si(si),
    numf(numf),
    bv(bv),
    needImmediateConstPS(needImmediateConstPS),
    bindposeBufferOffset(bindposeBufferOffset)
  {}
};

// todo: make it project dependent. Render feature should ask for exclusive buffer if it is going
// to use large amounts of dynmodels
enum BufferType
{
  OTHER = 0,
  TRANSPARENT_MAIN = 1,
  MAIN = 2,
  DYNAMIC_SHADOW = 3,
  SHADOWS_CSM = 4, // and further
};

inline BufferType get_buffer_type_from_render_pass(int render_pass)
{
  switch (render_pass)
  {
    case RENDER_MAIN: return BufferType::MAIN;
    case RENDER_DYNAMIC_SHADOW: return BufferType::DYNAMIC_SHADOW;
    default:
      return render_pass >= RENDER_SHADOWS_CSM ? static_cast<BufferType>(render_pass + BufferType::SHADOWS_CSM) : BufferType::OTHER;
  }
}

struct DynModelRenderingState
{
  void swap(DynModelRenderingState &a);
  bool empty() const { return list.empty() && multidrawList.empty(); }

  void process_animchar(uint32_t start_stage,
    uint32_t end_stage, // inclusive range of stages
    const DynamicRenderableSceneInstance *scene,
    const DynamicRenderableSceneResource *lodResource,
    const Point4 *bones_additional_data,
    uint32_t additional_data_size,
    bool need_previous_matrices,
    ShaderElement *shader_override = nullptr,
    const uint8_t *path_filter = nullptr,
    uint32_t path_filter_size = 0,
    uint8_t render_mask = 0,
    bool need_immediate_const_ps = false,
    RenderPriority priority = RenderPriority::DEFAULT,
    const GlobalVariableStates *gvars_state = nullptr,
    TexStreamingContext texCtx = TexStreamingContext(0));
  // legacy (get DynamicRenderableSceneResource* as scene->getCurSceneResource())
  void process_animchar(uint32_t start_stage,
    uint32_t end_stage, // inclusive range of stages
    const DynamicRenderableSceneInstance *scene,
    const Point4 *bones_additional_data,
    uint32_t additional_data_size,
    bool need_previous_matrices,
    ShaderElement *shader_override = nullptr,
    const uint8_t *path_filter = nullptr,
    uint32_t path_filter_size = 0,
    uint8_t render_mask = 0,
    bool need_immediate_const_ps = false,
    RenderPriority priority = RenderPriority::DEFAULT,
    const GlobalVariableStates *gvars_state = nullptr,
    TexStreamingContext texCtx = TexStreamingContext(0));
  void prepareForRender();
  void clear();
  void render(int offset) const;
  void render_no_packed(int offset) const;
  void addStateFrom(DynModelRenderingState &&s); // we can add only states with same stride
  void setVars(D3DRESID buf_id) const;
  const DynamicBufferHolder *requestBuffer(BufferType type) const;
  void coalesceDrawcalls();
  void updateDataForPackedRender() const;
  void render_multidraw(int offset) const;

  SmallTab<RenderElement> list, multidrawList;
  SmallTab<Point4> instanceData;
  static const uint32_t MATRIX_ROWS = 3;
  int matrixStride = 0;

  struct PackedDrawCallsRange
  {
    uint32_t start;
    uint32_t count;
  };
  dag::Vector<PackedDrawCallsRange> drawcallRanges;
  ska::flat_hash_map<shaders::ConstStateIdx, uint8_t> bindlessStatesToUpdateTexLevels;
};

// Should always return valid pointer except maybe OOM cases. And OOM has its own handling for exception.
DynModelRenderingState *create_state(const char *name = nullptr);
const DynModelRenderingState *get_state(const char *state);
inline DynModelRenderingState &get_immediate_state() { return *create_state(); }

inline void DynModelRenderingState::clear()
{
  list.clear();
  multidrawList.clear();
  instanceData.clear();
  drawcallRanges.clear();
  bindlessStatesToUpdateTexLevels.clear();
}

uint32_t prepare_bones_to(
  vec4f *__restrict bones, const ShaderSkinnedMesh &skin, const DynamicRenderableSceneInstance &scene, bool previous_matrices);

void init_dynmodel_rendering(int csm_num_cascades);
void close_dynmodel_rendering();

}; // namespace dynmodel_renderer
