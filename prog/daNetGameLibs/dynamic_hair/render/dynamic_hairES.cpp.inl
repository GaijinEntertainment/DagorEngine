// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/componentType.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <shaders/dag_dynSceneRes.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/render/postfx_renderer.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <render/renderSettings.h>
#include <render/renderEvent.h>
#include <render/daBfg/bfg.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/world/dynModelRenderer.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/dynmodelRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_matricesAndPerspective.h>

template <typename Callable>
static void render_hair_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
static void gather_hair_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
static void add_entity_with_hair_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
static void remove_entity_with_hair_ecs_query(ecs::EntityId, Callable c);

extern int dynamicSceneTransBlockId, dynamicSceneBlockId, dynamicDepthSceneBlockId;

static ShaderVariableInfo hair_transparent_passVarId("hair_transparent_pass");
static ShaderVariableInfo dynamic_hair_atest_valueVarId("dynamic_hair_atest_value");

#if DAGOR_DBGLEVEL > 0
CONSOLE_BOOL_VAL("render", transparentHairsEnable, true);
CONSOLE_BOOL_VAL("render", opaqueHairsEnable, true);
#else
static const bool transparentHairsEnable = true;
static const bool opaqueHairsEnable = true;
#endif

static const char *HAIR_SHADER_NAME = "dynamic_hair";
static float hairsAtestValue = 1.0f / 255.0f;

ECS_ON_EVENT(on_appear)
static void detect_hair_es(const ecs::Event &, AnimV20::AnimcharRendComponent &animchar_render, ecs::EntityId eid)
{
  DynamicRenderableSceneInstance *sceneInstance = animchar_render.getSceneInstance();
  G_ASSERT_RETURN(sceneInstance && sceneInstance->getLodsResource(), );
  dag::VectorSet<int, eastl::less<int>, framemem_allocator> dynamicHairNodes;

  for (const DynamicRenderableSceneLodsResource::Lod &dynResLod : sceneInstance->getLodsResource()->lods)
  {
    const DynamicRenderableSceneResource *dynRes = dynResLod.scene;
    G_ASSERT_CONTINUE(dynRes);

    const auto cb = [&dynamicHairNodes](int nodeId) { dynamicHairNodes.emplace(nodeId); };
    dynRes->findSkinNodesWithMaterials(make_span_const(&HAIR_SHADER_NAME, 1), cb);
    dynRes->findRigidNodesWithMaterials(make_span_const(&HAIR_SHADER_NAME, 1), cb);
  }
  if (dynamicHairNodes.empty())
    return;

  add_sub_template_async(eid, "dynamic_hair_render_tag");

  add_entity_with_hair_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("dynamic_hair_renderer")),
    [&](int &dynamic_hair__max_nodes_count, ecs::EidList &dynamic_hair__entities, ecs::IntList &dynamic_hair__nodeMaskOffsets,
      ecs::UInt8List &dynamic_hair__nodeMasks) {
      dynamic_hair__entities.push_back(eid);
      int nodeMaskOffset = dynamic_hair__nodeMasks.size();
      dynamic_hair__nodeMaskOffsets.push_back(nodeMaskOffset);
      int nodeCount = sceneInstance->getNodeCount();
      dynamic_hair__max_nodes_count = max(nodeCount, dynamic_hair__max_nodes_count);
      dynamic_hair__nodeMasks.resize(dynamic_hair__nodeMasks.size() + nodeCount, 0);
      for (int nodeId : dynamicHairNodes)
        dynamic_hair__nodeMasks[nodeMaskOffset + nodeId] = 0xff;
    });
}

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag dynamic_hair_render)
static void remove_hair_on_destroy_es(const ecs::Event &, ecs::EntityId eid)
{
  remove_entity_with_hair_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("dynamic_hair_renderer")),
    [eid](ecs::EidList &dynamic_hair__entities, ecs::IntList &dynamic_hair__nodeMaskOffsets, ecs::UInt8List &dynamic_hair__nodeMasks) {
      int idx = find_value_idx(dynamic_hair__entities, eid);
      if (idx < 0)
        return;
      dynamic_hair__entities.erase(dynamic_hair__entities.begin() + idx);
      int nodeOffset = dynamic_hair__nodeMaskOffsets[idx];
      int nodeOffsetNext =
        idx + 1 < dynamic_hair__nodeMaskOffsets.size() ? dynamic_hair__nodeMaskOffsets[idx + 1] : dynamic_hair__nodeMasks.size();
      dynamic_hair__nodeMaskOffsets.erase(dynamic_hair__nodeMaskOffsets.begin() + idx);
      dynamic_hair__nodeMasks.erase(dynamic_hair__nodeMasks.begin() + nodeOffset, dynamic_hair__nodeMasks.begin() + nodeOffsetNext);
      int erasedMaskLength = nodeOffsetNext - nodeOffset;
      for (int i = idx, ie = dynamic_hair__nodeMaskOffsets.size(); i < ie; ++i)
        dynamic_hair__nodeMaskOffsets[i] -= erasedMaskLength;
    });
}

static dabfg::NodeHandle makeDynamicHairRenderNode()
{
  auto nodeNs = dabfg::root() / "transparent" / "far";

  return nodeNs.registerNode("dynamic_hair_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestRenderPass().color({"color_target"}).depthRw("depth"); // TODO: ZWrite off, but RW
                                                                           // allows to avoid depth
                                                                           // decompression on barrier
                                                                           // while switch to RO
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_HAIRS);

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();
    auto texCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    registry.requestState().setFrameBlock("global_frame");

    return [texCtxHndl]() {
      render_hair_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("dynamic_hair_renderer")),
        [texCtxHndl](int &dynamic_hair__max_nodes_count, const ecs::EidList &dynamic_hair__entities,
          const ecs::IntList &dynamic_hair__nodeMaskOffsets, const ecs::UInt8List &dynamic_hair__nodeMasks) {
          if (!transparentHairsEnable)
            return;

          ShaderGlobal::set_int(hair_transparent_passVarId, 1);
          ShaderGlobal::set_real(dynamic_hair_atest_valueVarId, hairsAtestValue);

          dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
          SmallTab<uint8_t, framemem_allocator> nodeMask;
          nodeMask.resize(dynamic_hair__max_nodes_count);

          for (int i = 0, ie = dynamic_hair__entities.size(); i < ie; ++i)
          {
            gather_hair_ecs_query(dynamic_hair__entities[i],
              [&](const AnimV20::AnimcharRendComponent &animchar_render, uint8_t animchar_visbits) {
                if (!(animchar_visbits & VISFLG_MAIN_VISIBLE))
                  return;
                const DynamicRenderableSceneInstance *sceneInstance = animchar_render.getSceneInstance();
                const DynamicRenderableSceneResource *dynRes = sceneInstance->getCurSceneResource();
                if (!dynRes)
                  return;

                const int nodeCount = sceneInstance->getNodeCount();
                {
                  // Nodes could be hidden in runtime. So we need check visibility of node
                  const uint8_t *nodeMaskBegin = dynamic_hair__nodeMasks.begin() + dynamic_hair__nodeMaskOffsets[i];

                  for (int nodeId = 0; nodeId < nodeCount; nodeId++)
                    nodeMask[nodeId] = (nodeMaskBegin[nodeId] != 0) && !sceneInstance->isNodeHidden(nodeId) ? 0xff : 0;
                }

                state.process_animchar(ShaderMesh::STG_atest, ShaderMesh::STG_atest, sceneInstance, dynRes, nullptr, 0, false, nullptr,
                  nodeMask.begin(), nodeCount, UpdateStageInfoRender::RENDER_MAIN, false, dynmodel_renderer::RenderPriority::HIGH,
                  nullptr, texCtxHndl.ref());
              });
          }
          state.prepareForRender();
          const dynmodel_renderer::DynamicBufferHolder *buffer = state.requestBuffer(dynmodel_renderer::BufferType::OTHER);
          if (!buffer)
            return;
          state.setVars(buffer->buffer.getBufId());
          SCENE_LAYER_GUARD(dynamicSceneBlockId);

          state.render(buffer->curOffset);
        });

      ShaderGlobal::set_int(hair_transparent_passVarId, 0);
      ShaderGlobal::set_real(dynamic_hair_atest_valueVarId, opaqueHairsEnable ? hairsAtestValue : 1.0f);
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
static void init_dynamic_hair_es_event_handler(const ecs::Event &, dabfg::NodeHandle &dynamic_hair__render_node)
{
  dynamic_hair__render_node = makeDynamicHairRenderNode();
}