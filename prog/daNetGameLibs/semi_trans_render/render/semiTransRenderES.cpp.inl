// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <daECS/core/entityId.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/anim/anim.h>
#include <ecs/core/entityManager.h>
#include <ecs/rendInst/riExtra.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <render/renderEvent.h>
#include <rendInst/rendInstExtraRender.h>

#include <render/world/dynModelRenderer.h>
#include <render/world/renderPrecise.h>
#include <ecs/anim/animchar_visbits.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_buffers.h>

struct SemiTransRenderManager
{
  using SbufferPtr = eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>>;

  DynamicShaderHelper dynamicObjectsRenderer;
  DynamicShaderHelper rendinstsRenderer;
  SbufferPtr rendinstMatrixBuffer;
  int placing_colorVarId;
  int diffuse_texVarId;
  ecs::EntityId curEId;
  int curResIdx;

  SemiTransRenderManager()
  {
    dynamicObjectsRenderer.init("dynamic_semi_trans", nullptr, 0, "dynamic_semi_trans", true);
    rendinstsRenderer.init("rendinst_semi_trans", nullptr, 0, "rendinst_semi_trans", true);
    rendinstMatrixBuffer.reset(d3d::create_sbuffer(sizeof(Point4), 4U, SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC,
      TEXFMT_A32B32G32R32F, "SemiTransObject"));
    placing_colorVarId = get_shader_variable_id("placing_color");
    diffuse_texVarId = get_shader_variable_id("diffuse_rendinst_tex");
    curEId = ecs::INVALID_ENTITY_ID;
    curResIdx = 0;
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(SemiTransRenderManager);
ECS_REGISTER_RELOCATABLE_TYPE(SemiTransRenderManager, nullptr);

extern int dynamicSceneTransBlockId;
extern int rendinstTransSceneBlockId;

using namespace dynmodel_renderer;

template <typename Callable>
static inline void semi_trans_manager_ecs_query(Callable);

static SemiTransRenderManager *get_manager()
{
  SemiTransRenderManager *manager = nullptr;
  semi_trans_manager_ecs_query([&](SemiTransRenderManager &semi_trans_render__manager) {
    G_ASSERT(!manager);
    manager = &semi_trans_render__manager;
  });
  G_ASSERT(manager);
  return manager;
}

ECS_TAG(render)
ECS_NO_ORDER
ECS_REQUIRE(eastl::true_type semi_transparent__visible)
static __forceinline void animchar_render_semi_trans_es_event_handler(const RenderLateTransEvent &event,
  AnimV20::AnimcharRendComponent &animchar_render,
  const Point3 &semi_transparent__placingColor,
  uint8_t &animchar_visbits,
  int semi_transparent__lod)
{
  SemiTransRenderManager *manager = get_manager();
  if (!manager || ((animchar_visbits & (VISFLG_MAIN_AND_SHADOW_VISIBLE | VISFLG_MAIN_VISIBLE)) == 0))
    return;

  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  Point4 data = Point4(semi_transparent__placingColor.x, semi_transparent__placingColor.y, semi_transparent__placingColor.z, 0);

  animchar_visbits |= VISFLG_SEMI_TRANS_RENDERED;

  animchar_render.getSceneInstance()->setLod(semi_transparent__lod);

  state.process_animchar(0, ShaderMesh::STG_imm_decal, animchar_render.getSceneInstance(), &data, 1, false,
    manager->dynamicObjectsRenderer.shader, nullptr, 0, 0, false, RenderPriority::HIGH, nullptr, event.texCtx);

  state.prepareForRender();
  const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::TRANSPARENT_MAIN);
  if (!buffer)
    return;

  TMatrix vtm = event.viewTm;
  vtm.setcol(3, 0, 0, 0);
  d3d::settm(TM_VIEW, vtm);
  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(dynamicSceneTransBlockId);
  state.render(buffer->curOffset);
  d3d::settm(TM_VIEW, event.viewTm);
}

static void set_rendist_rendering(SemiTransRenderManager *manager,
  int res_idx,
  const Point3 &placing_color,
  const TMatrix &transform,
  ShaderElement *shader_override,
  bool set_texture_to_shader = false)
{
  ShaderGlobal::set_color4(manager->placing_colorVarId, placing_color.x, placing_color.y, placing_color.z, 0);
  Point4 data[4];
  data[0] = Point4(transform.getcol(0).x, transform.getcol(1).x, transform.getcol(2).x, transform.getcol(3).x);
  data[1] = Point4(transform.getcol(0).y, transform.getcol(1).y, transform.getcol(2).y, transform.getcol(3).y);
  data[2] = Point4(transform.getcol(0).z, transform.getcol(1).z, transform.getcol(2).z, transform.getcol(3).z);
  data[3] = Point4(0, 0, 0, 0);
  manager->rendinstMatrixBuffer.get()->updateData(0, sizeof(Point4) * 4U, &data, VBLOCK_DISCARD | VBLOCK_WRITEONLY);
  IPoint2 offsAndCnt(0, 1);
  uint16_t riIdx = res_idx;
  uint32_t zeroLodOffset = 0;
  if (set_texture_to_shader)
    rendinst::render::setRIGenExtraDiffuseTexture(riIdx, manager->diffuse_texVarId);
  SCENE_LAYER_GUARD(rendinstTransSceneBlockId);
  rendinst::render::renderRIGenExtraFromBuffer(manager->rendinstMatrixBuffer.get(), dag::ConstSpan<IPoint2>(&offsAndCnt, 1),
    dag::ConstSpan<uint16_t>(&riIdx, 1), dag::ConstSpan<uint32_t>(&zeroLodOffset, 1), rendinst::RenderPass::Normal,
    rendinst::OptimizeDepthPass::Yes, rendinst::OptimizeDepthPrepass::No, rendinst::IgnoreOptimizationLimits::No,
    rendinst::LayerFlag::Transparent, shader_override);
}

ECS_TAG(render)
ECS_NO_ORDER
ECS_REQUIRE(ecs::string ri_preview__name)
ECS_REQUIRE_NOT(ecs::Tag use_texture)
ECS_REQUIRE(eastl::true_type semi_transparent__visible)
void set_shader_semi_trans_rendinst_es_event_handler(const RenderLateTransEvent &evt,
  int semi_transparent__resIdx,
  ecs::EntityId eid,
  const Point3 &semi_transparent__placingColor,
  const TMatrix &transform)
{
  SemiTransRenderManager *manager = get_manager();
  if (!manager || (semi_transparent__resIdx < 0))
    return;
  bool resetTexture = manager->curEId != eid || manager->curResIdx != semi_transparent__resIdx;
  if (resetTexture)
  {
    manager->curEId = eid;
    manager->curResIdx = semi_transparent__resIdx;
    ShaderGlobal::set_texture(manager->diffuse_texVarId, BAD_TEXTUREID);
  }
  RenderPrecise renderPrecise(evt.viewTm, evt.cameraWorldPos);
  set_rendist_rendering(manager, semi_transparent__resIdx, semi_transparent__placingColor, transform,
    manager->rendinstsRenderer.shader);
}

ECS_TAG(render)
ECS_NO_ORDER
ECS_REQUIRE(ecs::string ri_preview__name)
ECS_REQUIRE(ecs::Tag use_texture)
ECS_REQUIRE(eastl::true_type semi_transparent__visible)
void set_shader_semi_trans_rendinst_with_tex_es_event_handler(const RenderLateTransEvent &,
  int semi_transparent__resIdx,
  ecs::EntityId eid,
  const Point3 &semi_transparent__placingColor,
  const TMatrix &transform)
{
  SemiTransRenderManager *manager = get_manager();
  if (!manager || (semi_transparent__resIdx < 0))
    return;
  bool setTextureToShader = manager->curEId != eid || manager->curResIdx != semi_transparent__resIdx;
  if (setTextureToShader)
  {
    manager->curEId = eid;
    manager->curResIdx = semi_transparent__resIdx;
  }
  set_rendist_rendering(manager, semi_transparent__resIdx, semi_transparent__placingColor, transform,
    manager->rendinstsRenderer.shader, setTextureToShader);
}
