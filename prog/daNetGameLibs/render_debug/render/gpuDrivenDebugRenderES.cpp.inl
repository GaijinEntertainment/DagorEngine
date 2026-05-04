// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>
#include <render/daFrameGraph/daFG.h>

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_draw.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_computeShaders.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>

#include <render/world/frameGraphHelpers.h>

typedef Point4 float4;
typedef Point3 float3;
#include "../shaders/gpu_debug_render_structs.hlsli"

template <typename Callable>
static void query_gpu_debug_render_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static void query_gpu_debug_render_for_prepare_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static void query_gpu_debug_render_for_render_ecs_query(ecs::EntityManager &manager, Callable);

namespace
{
ShaderVariableInfo gpu_debug_render_buffering_active("gpu_debug_render_buffering_active", true);
ShaderVariableInfo gpu_debug_render_flat_spheres_buf("gpu_debug_render_flat_spheres_buf", true);
ShaderVariableInfo gpu_debug_render_flat_spheres_count_buf("gpu_debug_render_flat_spheres_count_buf", true);

ShaderVariableInfo gpu_debug_render_lines_buf("gpu_debug_render_lines_buf", true);
ShaderVariableInfo gpu_debug_render_lines_count_buf("gpu_debug_render_lines_count_buf", true);
} // namespace

class GpuDrivenDebugRender
{
public:
  GpuDrivenDebugRender()
  {
    initNodes();

    gpu_debug_renderer_clear_cs.reset(new_compute_shader("gpu_debug_renderer_clear_cs"));
    gpu_debug_renderer_create_draw_indirect_cs.reset(new_compute_shader("gpu_debug_renderer_create_draw_indirect_cs"));
    gpu_debug_renderer_draw_indirect_flat_spheres_ps.init("gpu_debug_renderer_draw_indirect_flat_spheres_ps", NULL, 0,
      "gpu_debug_renderer_draw_indirect_flat_spheres_ps");
    gpu_debug_renderer_draw_indirect_lines.init("gpu_debug_renderer_draw_indirect_lines", NULL, 0,
      "gpu_debug_renderer_draw_indirect_lines");

    flatSpheres = dag::buffers::create_ua_sr_structured(sizeof(GpuDrivenDebugRenderFlatSphere), GPU_DEBUG_RENDER_MAX_ELEMS_PER_TYPE,
      "gpu_debug_render_flat_spheres_buf");
    flatSpheresCount = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 1, "gpu_debug_render_flat_spheres_count_buf");

    lines = dag::buffers::create_ua_sr_structured(sizeof(GpuDrivenDebugRenderLine), GPU_DEBUG_RENDER_MAX_ELEMS_PER_TYPE,
      "gpu_debug_render_lines_buf");
    linesCount = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 1, "gpu_debug_render_lines_count_buf");
  }

  ~GpuDrivenDebugRender() { ShaderGlobal::set_float(gpu_debug_render_buffering_active, 0.0f); }

  void beforeRender()
  {
    ShaderGlobal::set_float(gpu_debug_render_buffering_active, 0.0f);
    ShaderGlobal::set_buffer(gpu_debug_render_flat_spheres_buf, nullptr);
    ShaderGlobal::set_buffer(gpu_debug_render_flat_spheres_count_buf, nullptr);
    ShaderGlobal::set_buffer(gpu_debug_render_lines_buf, nullptr);
    ShaderGlobal::set_buffer(gpu_debug_render_lines_count_buf, nullptr);

    if (freezeFrame)
      return;

    d3d::resource_barrier(ResourceBarrierDesc{flatSpheresCount.getBuf(), RB_STAGE_COMPUTE | RB_RW_UAV});
    d3d::resource_barrier(ResourceBarrierDesc{linesCount.getBuf(), RB_STAGE_COMPUTE | RB_RW_UAV});
    ShaderGlobal::set_buffer(gpu_debug_render_flat_spheres_count_buf, flatSpheresCount);
    ShaderGlobal::set_buffer(gpu_debug_render_lines_count_buf, linesCount);
    gpu_debug_renderer_clear_cs->dispatch(1, 1, 1);

    // for global access in shaders during the FG execution
    ShaderGlobal::set_float(gpu_debug_render_buffering_active, 1.0f);
    ShaderGlobal::set_buffer(gpu_debug_render_flat_spheres_buf, flatSpheres);
    ShaderGlobal::set_buffer(gpu_debug_render_lines_buf, lines);
    d3d::resource_barrier(ResourceBarrierDesc{flatSpheres.getBuf(), RB_STAGE_ALL_SHADERS | RB_RW_UAV});
    d3d::resource_barrier(ResourceBarrierDesc{flatSpheresCount.getBuf(), RB_STAGE_ALL_SHADERS | RB_RW_UAV});
    d3d::resource_barrier(ResourceBarrierDesc{lines.getBuf(), RB_STAGE_ALL_SHADERS | RB_RW_UAV});
    d3d::resource_barrier(ResourceBarrierDesc{linesCount.getBuf(), RB_STAGE_ALL_SHADERS | RB_RW_UAV});
  }

  void prepareShapes()
  {
    ShaderGlobal::set_float(gpu_debug_render_buffering_active, 0.0f);
    d3d::resource_barrier(ResourceBarrierDesc{flatSpheresCount.getBuf(), RB_STAGE_COMPUTE | RB_RO_SRV});
    d3d::resource_barrier(ResourceBarrierDesc{linesCount.getBuf(), RB_STAGE_COMPUTE | RB_RO_SRV});

    ShaderGlobal::set_buffer(gpu_debug_render_flat_spheres_count_buf, flatSpheresCount);
    ShaderGlobal::set_buffer(gpu_debug_render_lines_count_buf, linesCount);

    gpu_debug_renderer_create_draw_indirect_cs->dispatch(1, 1, 1);
  }

  void renderFlatSpheres(Sbuffer *indirect_buf)
  {
    d3d::resource_barrier(ResourceBarrierDesc{flatSpheres.getBuf(), RB_STAGE_VERTEX | RB_RO_SRV});

    ShaderGlobal::set_buffer(gpu_debug_render_flat_spheres_buf, flatSpheres);
    gpu_debug_renderer_draw_indirect_flat_spheres_ps.shader->setStates(0, true);
    d3d::setvsrc(0, 0, 0);
    d3d::draw_indirect(PRIM_TRILIST, indirect_buf, 0);
  }

  void renderLines(Sbuffer *indirect_buf)
  {
    d3d::resource_barrier(ResourceBarrierDesc{lines.getBuf(), RB_STAGE_VERTEX | RB_RO_SRV});

    ShaderGlobal::set_buffer(gpu_debug_render_lines_buf, lines);
    gpu_debug_renderer_draw_indirect_lines.shader->setStates(0, true);
    d3d::setvsrc(0, 0, 0);
    d3d::draw_indirect(PRIM_TRILIST, indirect_buf, 0);
  }

  void initNodes()
  {
    auto ns = dafg::root() / "debug";

    beforeRenderNode = ns.registerNode("gpu_debug_renderer_before_render", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.readBlob<OrderingToken>("before_world_render_setup_token");

      return []() {
        query_gpu_debug_render_for_prepare_ecs_query(*g_entity_mgr,
          [&](GpuDrivenDebugRender &gpu_driven_debug_render) { gpu_driven_debug_render.beforeRender(); });
      };
    });

    prepareNode = ns.registerNode("gpu_debug_renderer_prepare", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.orderMeAfter("begin");

      registry.create("gpu_debug_render_spheres_indirect_cmd_buf")
        .indirectBuffer(d3d::buffers::Indirect::Draw, 1)
        .atStage(dafg::Stage::CS)
        .bindToShaderVar("gpu_debug_render_flat_spheres_indirect_buf");

      registry.create("gpu_debug_render_lines_indirect_cmd_buf")
        .indirectBuffer(d3d::buffers::Indirect::Draw, 1)
        .atStage(dafg::Stage::CS)
        .bindToShaderVar("gpu_debug_render_lines_indirect_buf");

      return []() {
        query_gpu_debug_render_for_prepare_ecs_query(*g_entity_mgr,
          [&](GpuDrivenDebugRender &gpu_driven_debug_render) { gpu_driven_debug_render.prepareShapes(); });
      };
    });

    renderNode = ns.registerNode("gpu_debug_renderer_render", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      use_camera_in_camera(registry);
      registry.requestState().setFrameBlock("global_frame");

      auto debugNs = registry.root() / "debug";
      auto colorTarget = debugNs.modifyTexture("target_for_debug");
      registry.requestRenderPass().color({colorTarget}).depthRo("depth_for_postfx");

      auto indirectSpheresBufHndl = registry.read("gpu_debug_render_spheres_indirect_cmd_buf")
                                      .buffer()
                                      .useAs(dafg::Usage::INDIRECTION_BUFFER)
                                      .atStage(dafg::Stage::ALL_INDIRECTION)
                                      .handle();

      auto indirectLinesBufHndl = registry.read("gpu_debug_render_lines_indirect_cmd_buf")
                                    .buffer()
                                    .useAs(dafg::Usage::INDIRECTION_BUFFER)
                                    .atStage(dafg::Stage::ALL_INDIRECTION)
                                    .handle();

      return [indirectSpheresBufHndl, indirectLinesBufHndl](const dafg::multiplexing::Index &multiplexing_index) {
        camera_in_camera::ApplyMasterState camcam{multiplexing_index};

        query_gpu_debug_render_for_render_ecs_query(*g_entity_mgr, [&](GpuDrivenDebugRender &gpu_driven_debug_render) {
          gpu_driven_debug_render.renderFlatSpheres(const_cast<Sbuffer *>(indirectSpheresBufHndl.get()));
          gpu_driven_debug_render.renderLines(const_cast<Sbuffer *>(indirectLinesBufHndl.get()));
        });
      };
    });
  }

  void toggleFreezeFrame() { freezeFrame = !freezeFrame; }

private:
  eastl::unique_ptr<ComputeShaderElement> gpu_debug_renderer_create_draw_indirect_cs, gpu_debug_renderer_clear_cs;
  DynamicShaderHelper gpu_debug_renderer_draw_indirect_flat_spheres_ps, gpu_debug_renderer_draw_indirect_lines;

  dafg::NodeHandle beforeRenderNode, prepareNode, renderNode;

  UniqueBufHolder flatSpheres, flatSpheresCount, lines, linesCount;
  bool freezeFrame = false;
};

ECS_DECLARE_RELOCATABLE_TYPE(GpuDrivenDebugRender);
ECS_REGISTER_RELOCATABLE_TYPE(GpuDrivenDebugRender, nullptr);

namespace
{
bool gpu_driven_debug_renderer_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("gpu_dbg_render", "enabled", 1, 1)
  {
    ecs::EntityId dbgEntity = g_entity_mgr->getSingletonEntity(ECS_HASH("gpu_driven_debug_render"));
    if (dbgEntity)
      g_entity_mgr->destroyEntity(dbgEntity);
    else
      g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("gpu_driven_debug_render"));
  }
  CONSOLE_CHECK_NAME("gpu_dbg_render", "freeze_frame", 1, 1)
  {
    query_gpu_debug_render_ecs_query(*g_entity_mgr,
      [](GpuDrivenDebugRender &gpu_driven_debug_render) { gpu_driven_debug_render.toggleFreezeFrame(); });
  }
  return found;
}
} // namespace

REGISTER_CONSOLE_HANDLER(gpu_driven_debug_renderer_console_handler);

size_t gpu_driven_debug_renderer_pull = 0;