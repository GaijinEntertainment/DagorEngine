// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "outline.h"

#include <EASTL/sort.h>
#include <EASTL/vector.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/anim/anim.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_hlsl_floatx.h>
#include <rendInst/rendInstExtra.h>
#include <gameRes/dag_collisionResource.h>
#include <rendInst/rendInstExtraRender.h>
#include <rendInst/rendInstCollision.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/global_vars.h>

#include <scene/dag_occlusion.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/rendInst/riExtra.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <render/renderEvent.h>
#include <outline/shaders/outline_buffer_size.hlsli>
#include <game/gameEvents.h>

#include <render/daBfg/ecs/frameGraphNode.h>

#include <render/resolution.h>

#include <daECS/core/utility/enumRegistration.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>

static int outline_depthVarId = -1, outline_scaleVarId = -1, outline_blur_widthVarId = -1, outline_brightnessVarId = -1;
extern int dynamicDepthSceneBlockId;
extern int rendinstDepthSceneBlockId;
using namespace dynmodel_renderer;


ECS_REGISTER_RELOCATABLE_TYPE(OutlineMode, nullptr);
ECS_DECLARE_ENUM(OutlineMode, ZTEST_FAIL, ZTEST_PASS, NO_ZTEST);
ECS_REGISTER_ENUM(OutlineMode);

struct OutlineElement
{
  vec4f clipBox;
  E3DCOLOR int_color, ext_color;
  OutlineElement(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color) : clipBox(clipBox), int_color(int_color), ext_color(ext_color)
  {}
  bool operator<(const OutlineElement &rhs) const
  {
    if (int_color != rhs.int_color)
      return int_color < rhs.int_color;
    return ext_color < rhs.ext_color;
  }
};

struct OutlinedAnimchar : OutlineElement
{
  const AnimV20::AnimcharRendComponent *animchar;
  const ecs::Point4List *additionalData;
  OutlinedAnimchar(vec4f clipBox,
    E3DCOLOR int_color,
    E3DCOLOR ext_color,
    const AnimV20::AnimcharRendComponent *animchar,
    const ecs::Point4List *additionalData) :
    OutlineElement(clipBox, int_color, ext_color), animchar(animchar), additionalData(additionalData)
  {}
};

struct OutlinedRendinst : OutlineElement
{
  TMatrix transform;
  uint32_t riIdx;
  OutlinedRendinst(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color, const TMatrix &transform, uint32_t ri_idx) :
    OutlineElement(clipBox, int_color, ext_color), transform(transform), riIdx(ri_idx)
  {}
};

static bool outline_culling(mat44f_cref globtm, bbox3f_cref wbox, vec4f &clip_screen_box)
{
  vec3f threshold = v_make_vec4f(4.f / 1920, 4.f / 1080, 0, 0); // 4 FULLHD pixel minimum
  if (!v_screen_size_b(wbox.bmin, wbox.bmax, threshold, clip_screen_box, globtm))
    return false;

  clip_screen_box = v_min(v_max(clip_screen_box, v_neg(V_C_ONE)), V_C_ONE);
  return true;
}

struct OutlineContext
{

  void addOutline(E3DCOLOR int_color,
    E3DCOLOR ext_color,
    mat44f_cref globtm,
    bbox3f_cref wbox,
    const AnimV20::AnimcharRendComponent &animchar,
    const ecs::Point4List *additional_data)
  {
    vec4f clipScreenBox;
    if (outline_culling(globtm, wbox, clipScreenBox))
    {
      if (ext_color == 0)
        ext_color = int_color;
      animcharElements.emplace_back(clipScreenBox, int_color, ext_color, &animchar, additional_data);
    }
  }

  void addOutline(
    E3DCOLOR int_color, E3DCOLOR ext_color, mat44f_cref globtm, bbox3f_cref wbox, const TMatrix &transform, uint32_t ri_idx)
  {
    vec4f clipScreenBox;
    if (outline_culling(globtm, wbox, clipScreenBox))
    {
      if (ext_color == 0)
        ext_color = int_color;
      riElements.emplace_back(clipScreenBox, int_color, ext_color, transform, ri_idx);
    }
  }


  Point4 calculate_box()
  {
    vec4f box = V_C_ONE;
    for (const auto &e : animcharElements)
      box = v_min(box, v_perm_xyab(v_perm_xzxz(e.clipBox), v_neg(v_perm_ywyw(e.clipBox))));
    for (const auto &e : riElements)
      box = v_min(box, v_perm_xyab(v_perm_xzxz(e.clipBox), v_neg(v_perm_ywyw(e.clipBox))));
    box = v_perm_xyab(box, v_neg(v_rot_2(box)));
    Point4 ret;
    v_st(&ret.x, box);
    return ret;
  }

  void clear()
  {
    animcharElements.clear();
    riElements.clear();
  }

  bool empty() const { return animcharElements.empty() && riElements.empty(); }

  dag::Vector<OutlinedAnimchar> animcharElements;
  dag::Vector<OutlinedRendinst> riElements;
};

using ColorBuffer = eastl::fixed_vector<Color4, COLOR_BUF_SIZE>;
struct OutlineRenderer
{
  alignas(16) TMatrix4 tm;
  int clipLeft = 0, clipTop = 0, clipWidth = 0, clipHeight = 0;
  eastl::array<shaders::UniqueOverrideStateId, OVERRIDES_COUNT> zTestOverrides;
  eastl::array<OutlineContext, OVERRIDES_COUNT> context;

  int width = 0, height = 0;
  PostFxRenderer finalRender, fillDepthRender;
  UniqueBuf colorsCB;
  UniqueBuf rendInstTransforms;
  ColorBuffer colors;

  bool anyVisible() { return !(context[ZTEST_PASS].empty() && context[ZTEST_FAIL].empty() && context[NO_ZTEST].empty()); }

  void resetColorBuffer()
  {
    colors.clear();
    colors.emplace_back(color4(0));
    colors.emplace_back(color4(0));
  }

  void uploadColorBufferToGPU()
  {
    colorsCB->updateData(0, colors.size() * sizeof(*colors.data()), colors.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  }

  void updateScaleVar()
  {
    float wInv = 1.f / width, hInv = 1.f / height;
    ShaderGlobal::set_color4(outline_scaleVarId, clipLeft * wInv, clipTop * hInv, clipWidth * wInv, clipHeight * hInv);
  }

  void setClippedViewport() { d3d::setview(clipLeft, clipTop, clipWidth, clipHeight, 0, 1); }

  shaders::OverrideStateId create_override(int override_type)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::STENCIL);
    state.stencil.set(CMPF_ALWAYS, STNCLOP_KEEP, STNCLOP_KEEP, STNCLOP_REPLACE, 0xFF, 0xFF);
    state.set(shaders::OverrideState::Z_WRITE_DISABLE);
    if (override_type == ZTEST_FAIL || override_type == ZTEST_PASS)
      state.set(shaders::OverrideState::Z_FUNC);
    else
      state.set(shaders::OverrideState::Z_TEST_DISABLE);

    constexpr int Z_FUNC_MODES[OVERRIDES_COUNT] = {CMPF_LESS, CMPF_GREATEREQUAL, CMPF_NEVER};
    state.zFunc = Z_FUNC_MODES[override_type];
    return shaders::overrides::create(state);
  }

  void init(float outline_brightness)
  {
    for (uint i = 0; i < OVERRIDES_COUNT; ++i)
      zTestOverrides[i] = create_override(i);

    finalRender.init("outline_final_render");
    fillDepthRender.init("outline_fill_depth");
    colorsCB = dag::buffers::create_one_frame_cb(COLOR_BUF_SIZE, "outline_colors");
    rendInstTransforms = dag::buffers::create_one_frame_sr_tbuf(4U, TEXFMT_A32B32G32R32F, "OutlineRenderer_transformsBuffer");

    outline_scaleVarId = get_shader_variable_id("outline_scale", true);
    outline_depthVarId = get_shader_variable_id("outline_depth", true);
    outline_blur_widthVarId = get_shader_variable_id("outline_blur", true);
    outline_brightnessVarId = get_shader_variable_id("outline_brighness", true);
    ShaderGlobal::set_real(outline_brightnessVarId, outline_brightness);
  }

  void changeResolution(int w, int h)
  {
    width = w;
    height = h;
  }

  void initBlurWidth(float outline_blur_width)
  {
    int displayWidth, displayHeight;
    get_display_resolution(displayWidth, displayHeight);
    ShaderGlobal::set_color4(outline_blur_widthVarId, outline_blur_width * width / displayWidth,
      outline_blur_width * height / displayHeight, 0, 0);
  }

  void calculateProjection(Point4 clipBox)
  {
    tm = matrix_ortho_off_center_lh(clipBox.x, clipBox.z, clipBox.y, clipBox.w, 0, 1);
    Point4 clipBoxTc =
      saturate(Point4(clipBox.x * 0.5f + 0.5f, clipBox.y * -0.5f + 0.5f, clipBox.z * 0.5f + 0.5f, clipBox.w * -0.5f + 0.5f));
    clipLeft = floorf(width * clipBoxTc.x);
    int clipRight = ceilf(width * clipBoxTc.z);
    int clipBottom = ceilf(height * clipBoxTc.y);
    clipTop = floorf(height * clipBoxTc.w);
    clipWidth = clipRight - clipLeft;
    clipHeight = clipBottom - clipTop;
  }
};

static void create_outline_rendrerer_locally_es(const BeforeLoadLevel &, ecs::EntityManager &manager)
{
  manager.getOrCreateSingletonEntity(ECS_HASH("outline_renderer"));
}

ECS_ON_EVENT(on_appear)
static void outline_renderer_create_es_event_handler(
  const ecs::Event &, OutlineRenderer &outline_renderer, float outline_blur_width, float outline_brightness)
{
  outline_renderer.init(outline_brightness);
  int targetWidth, targetHeight;
  get_rendering_resolution(targetWidth, targetHeight);
  outline_renderer.changeResolution(targetWidth, targetHeight);
  outline_renderer.initBlurWidth(outline_blur_width);
}

ECS_DECLARE_RELOCATABLE_TYPE(OutlineRenderer);
ECS_REGISTER_RELOCATABLE_TYPE(OutlineRenderer, nullptr);

static void rendist_rendering(UniqueBuf &rendinstMatrixBuffer, int res_idx, const TMatrix &transform)
{
  Point4 data[4];
  data[0] = Point4(transform.getcol(0).x, transform.getcol(1).x, transform.getcol(2).x, transform.getcol(3).x),
  data[1] = Point4(transform.getcol(0).y, transform.getcol(1).y, transform.getcol(2).y, transform.getcol(3).y),
  data[2] = Point4(transform.getcol(0).z, transform.getcol(1).z, transform.getcol(2).z, transform.getcol(3).z);
  data[3] = Point4(0, 0, 0, 0); // we need zero vector to avoid executing this branch if (useCbufferParams)

  rendinstMatrixBuffer->updateData(0, sizeof(Point4) * 4U, &data, VBLOCK_WRITEONLY | VBLOCK_DISCARD);

  IPoint2 offsAndCnt(0, 1);
  uint16_t riIdx = res_idx;
  uint32_t zeroLodOffset = 0;
  SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);

  rendinst::render::renderRIGenExtraFromBuffer(rendinstMatrixBuffer.getBuf(), dag::ConstSpan<IPoint2>(&offsAndCnt, 1),
    dag::ConstSpan<uint16_t>(&riIdx, 1), dag::ConstSpan<uint32_t>(&zeroLodOffset, 1), rendinst::RenderPass::Depth,
    rendinst::OptimizeDepthPass::No, rendinst::OptimizeDepthPrepass::No, rendinst::IgnoreOptimizationLimits::No,
    rendinst::LayerFlag::Stages);
}
struct StencilColorState
{
  uint32_t prevIntColor = 0, prevExtColor = 0, stencil = 0;
};

static void render_outline_stencil(const TMatrix &view_tm,
  dag::Vector<OutlinedAnimchar> &animcharElements,
  DynModelRenderingState &state,
  StencilColorState &stencil,
  ColorBuffer &colors)
{
  if (animcharElements.empty())
    return;
  TIME_D3D_PROFILE(outlined_animchars);
  TMatrix vtm = view_tm;
  vtm.col[3] = Point3(0, 0, 0);
  d3d::settm(TM_VIEW, vtm);

  eastl::sort(animcharElements.begin(), animcharElements.end());
  for (const auto &e : animcharElements)
  {
    if (stencil.prevIntColor != e.int_color || stencil.prevExtColor != e.ext_color)
    {
      colors.emplace_back(color4(e.int_color));
      colors.emplace_back(color4(e.ext_color));
      stencil.prevIntColor = e.int_color;
      stencil.prevExtColor = e.ext_color;
      shaders::set_stencil_ref(stencil.stencil);
      state.prepareForRender();
      const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::OTHER);
      if (buffer)
      {
        state.setVars(buffer->buffer.getBufId());
        SCENE_LAYER_GUARD(dynamicDepthSceneBlockId);
        state.render(buffer->curOffset);
      }
      state.clear();
      stencil.stencil++;
    }
    Point4 zero(0, 0, 0, 0);
    bool hasAdditionalData = e.additionalData && !e.additionalData->empty();
    const Point4 *additionalData = hasAdditionalData ? e.additionalData->data() : &zero;
    uint32_t additionalDataSize = hasAdditionalData ? e.additionalData->size() : 1;
    state.process_animchar(0, ShaderMesh::STG_imm_decal, e.animchar->getSceneInstance(), additionalData, additionalDataSize, false);
  }
  colors.emplace_back(color4(stencil.prevIntColor));
  colors.emplace_back(color4(stencil.prevExtColor));
  shaders::set_stencil_ref(stencil.stencil);
  state.prepareForRender();
  const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::OTHER);
  if (buffer)
  {
    state.setVars(buffer->buffer.getBufId());
    SCENE_LAYER_GUARD(dynamicDepthSceneBlockId);
    state.render(buffer->curOffset);
  }
  stencil.stencil++;
}
static void render_outline_stencil(const TMatrix &view_tm,
  dag::Vector<OutlinedRendinst> &riElements,
  UniqueBuf &rend_inst_transforms,
  StencilColorState &stencil_state,
  ColorBuffer &colors)
{
  if (riElements.empty())
    return;

  // We can optimize RI rendering with instancing
  // Need to gather all RI with same color in buffer and slices and pass to renderRIGenExtraFromBuffer
  TIME_D3D_PROFILE(outlined_rendinst);
  d3d::settm(TM_VIEW, view_tm);
  eastl::sort(riElements.begin(), riElements.end());
  for (const OutlinedRendinst &ri : riElements)
  {
    if (stencil_state.prevIntColor != ri.int_color || stencil_state.prevExtColor != ri.ext_color)
    {
      colors.emplace_back(color4(ri.int_color));
      colors.emplace_back(color4(ri.ext_color));
      stencil_state.prevIntColor = ri.int_color;
      stencil_state.prevExtColor = ri.ext_color;
      shaders::set_stencil_ref(++stencil_state.stencil);
    }
    rendist_rendering(rend_inst_transforms, ri.riIdx, ri.transform);
  }
}

static void outline_render_prepare(OutlineRenderer &outline_renderer, const TMatrix &view_tm, mat44f_cref proj_tm)
{
  TIME_D3D_PROFILE(render_outline);

  outline_renderer.updateScaleVar();
  outline_renderer.setClippedViewport();

  const bool shouldFillDepth = !(outline_renderer.context[ZTEST_PASS].empty() && outline_renderer.context[ZTEST_FAIL].empty());
  if (shouldFillDepth) // copy depth from depth_gbuf
  {
    TIME_D3D_PROFILE(fill_depth);
    outline_renderer.fillDepthRender.render();
    ShaderElement::invalidate_cached_state_block();
  }

  StencilColorState stencilState;
  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  auto renderOutlineType = [&](int override_type) {
    auto &context = outline_renderer.context[override_type];
    if (context.empty())
      return;
    shaders::overrides::set(outline_renderer.zTestOverrides[override_type]);
    render_outline_stencil(view_tm, context.animcharElements, state, stencilState, outline_renderer.colors);
    render_outline_stencil(view_tm, context.riElements, outline_renderer.rendInstTransforms, stencilState, outline_renderer.colors);
    shaders::overrides::reset();
  };

  {
    SCOPE_VIEW_PROJ_MATRIX;
    mat44f nproj;
    v_mat44_mul(nproj, (mat44f &)outline_renderer.tm, proj_tm);
    d3d::settm(TM_PROJ, nproj);

    outline_renderer.resetColorBuffer();
    ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));

    renderOutlineType(NO_ZTEST);
    renderOutlineType(ZTEST_PASS);
    renderOutlineType(ZTEST_FAIL);

    ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
    outline_renderer.uploadColorBufferToGPU();
  }
}

template <typename Callable>
static inline void outline_render_z_pass_ecs_query(Callable fn);
template <typename Callable>
static inline void outline_render_z_fail_ecs_query(Callable fn);
template <typename Callable>
static inline void outline_render_always_visible_ecs_query(Callable fn);

template <typename Callable>
static inline void outline_render_z_pass_ri_ecs_query(Callable fn);
template <typename Callable>
static inline void outline_render_z_pass_ri_handle_ecs_query(Callable fn);
template <typename Callable>
static inline void outline_render_z_fail_ri_ecs_query(Callable fn);
template <typename Callable>
static inline void outline_render_z_fail_ri_handle_ecs_query(Callable fn);
template <typename Callable>
static inline void outline_render_always_visible_ri_ecs_query(Callable fn);
template <typename Callable>
static inline void outline_render_always_visible_ri_handle_ecs_query(Callable fn);

template <typename Callable>
static inline void attach_ecs_query(ecs::EntityId eid, Callable fn);

template <typename Callable>
static inline void render_outline_ecs_query(Callable fn);

ECS_TAG(render)
static void outline_render_resolution_es_event_handler(const SetResolutionEvent &event, OutlineRenderer &outline_renderer)
{
  int width = event.renderingResolution.x, height = event.renderingResolution.y;
  outline_renderer.changeResolution(width, height);
}

static bbox3f make_world_bbox(Point3 min_bound, Point3 max_bound, const TMatrix &transform)
{
  bbox3f localBox{v_ldu(&min_bound.x), v_ldu(&max_bound.x)};
  mat44f wtm;
  v_mat44_make_from_43cu_unsafe(wtm, transform.array);
  bbox3f worldBox;
  v_bbox3_init(worldBox, wtm, localBox);
  return worldBox;
}

template <typename F>
static void outline_add_attaches(ecs::EntityId eid, const ecs::EidList &attaches, const F &add)
{
  for (auto attach_eid : attaches)
  {
    attach_ecs_query(attach_eid,
      [&](ecs::EntityId slot_attach__attachedTo, const AnimV20::AnimcharRendComponent &animchar_render, const bbox3f &animchar_bbox,
        const uint8_t &animchar_visbits ECS_REQUIRE(eastl::true_type animchar_render__enabled = true),
        const ecs::Point4List *additional_data = nullptr, const ecs::EidList *attaches_list = nullptr) {
        if (slot_attach__attachedTo != eid) // sanity check
          return;
        if (attaches_list)
          outline_add_attaches(attach_eid, *attaches_list, add);
        add(animchar_render, animchar_bbox, animchar_visbits, additional_data);
      });
  }
};

void add_ri_ex_to_outline_ctx(OutlineContext &outlineContext,
  mat44f_cref glob_tm,
  const rendinst::riex_handle_t ri_extra__handle,
  const TMatrix &transform,
  Point3 ri_extra__bboxMin,
  Point3 ri_extra__bboxMax,
  E3DCOLOR outline__color = 0xFFFFFFFF,
  E3DCOLOR outline__extcolor = 0)
{
  if (!outline__color.a || ri_extra__handle == rendinst::RIEX_HANDLE_NULL)
    return;

  bbox3f worldBox = make_world_bbox(ri_extra__bboxMin, ri_extra__bboxMax, transform);

  uint32_t resIdx = rendinst::handle_to_ri_type(ri_extra__handle);
  CollisionResource *collRes = rendinst::getRIGenExtraCollRes(resIdx);
  if (collRes)
  {
    bbox3f collBox = make_world_bbox(collRes->boundingBox.boxMin(), collRes->boundingBox.boxMax(), transform);
    worldBox.bmin = v_min(worldBox.bmin, collBox.bmin);
    worldBox.bmax = v_max(worldBox.bmax, collBox.bmax);
  }

  outlineContext.addOutline(outline__color, outline__extcolor, glob_tm, worldBox, transform, resIdx);
}

ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void outline_prepare_es(const UpdateStageInfoBeforeRender &stg, OutlineRenderer &outline_renderer)
{
  mat44f glob_tm;
  v_mat44_make_from_44cu(glob_tm, &stg.globTm.row[0].x);
  OutlineContext &zTestFailCtx = outline_renderer.context[ZTEST_FAIL];
  OutlineContext &zTestCtx = outline_renderer.context[ZTEST_PASS];
  OutlineContext &noZTestCtx = outline_renderer.context[NO_ZTEST];
  noZTestCtx.clear();
  zTestCtx.clear();
  zTestFailCtx.clear();

  outline_render_always_visible_ecs_query(
    [&](ecs::EntityId eid, AnimV20::AnimcharRendComponent &animchar_render,
      uint8_t &animchar_visbits ECS_REQUIRE(eastl::true_type outline__enabled, eastl::true_type outline__always_visible,
        eastl::true_type animchar_render__enabled = true),
      const bbox3f &animchar_bbox, const ecs::EidList *attaches_list, const ecs::Point4List *additional_data,
      E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0) {
      // ignores animchar_visbits
      if (!outline__color.a)
        return;
      if (attaches_list)
        outline_add_attaches(eid, *attaches_list,
          [&outline__color, &outline__extcolor, &glob_tm, &noZTestCtx](const AnimV20::AnimcharRendComponent &render,
            const bbox3f &bbox, const uint8_t &, const ecs::Point4List *additional_data) {
            noZTestCtx.addOutline(outline__color, outline__extcolor, glob_tm, bbox, render, additional_data);
          });
      noZTestCtx.addOutline(outline__color, outline__extcolor, glob_tm, animchar_bbox, animchar_render, additional_data);
      animchar_visbits |= VISFLG_RENDER_CUSTOM;
    });
  outline_render_always_visible_ri_ecs_query(
    [&](ECS_REQUIRE(eastl::true_type outline__enabled, eastl::true_type outline__always_visible) const TMatrix &transform,
      RiExtraComponent ri_extra, Point3 ri_extra__bboxMin, Point3 ri_extra__bboxMax, E3DCOLOR outline__color = 0xFFFFFFFF,
      E3DCOLOR outline__extcolor = 0) {
      add_ri_ex_to_outline_ctx(noZTestCtx, glob_tm, ri_extra.handle, transform, ri_extra__bboxMin, ri_extra__bboxMax, outline__color,
        outline__extcolor);
    });
  outline_render_always_visible_ri_handle_ecs_query(
    [&](ECS_REQUIRE(eastl::true_type outline__enabled, eastl::true_type outline__always_visible)
          ECS_REQUIRE_NOT(RiExtraComponent & ri_extra) const TMatrix &transform,
      const rendinst::riex_handle_t ri_extra__handle, Point3 ri_extra__bboxMin, Point3 ri_extra__bboxMax,
      E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0) {
      add_ri_ex_to_outline_ctx(noZTestCtx, glob_tm, ri_extra__handle, transform, ri_extra__bboxMin, ri_extra__bboxMax, outline__color,
        outline__extcolor);
    });

  outline_render_z_pass_ecs_query(
    [&](ecs::EntityId eid,
      const AnimV20::AnimcharRendComponent &animchar_render ECS_REQUIRE(eastl::true_type outline__enabled,
        eastl::false_type outline__z_fail = false, eastl::false_type outline__always_visible = false,
        eastl::true_type animchar_render__enabled = true, eastl::true_type outline__isOccluded),
      const bbox3f &animchar_bbox, uint8_t &animchar_visbits, const ecs::EidList *attaches_list,
      const ecs::Point4List *additional_data, E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0) {
      if (!outline__color.a)
        return;
      bool haveVisibleAttaches = false;
      if (attaches_list)
        outline_add_attaches(eid, *attaches_list,
          [&outline__color, &outline__extcolor, &glob_tm, &zTestCtx, &haveVisibleAttaches](
            const AnimV20::AnimcharRendComponent &render, const bbox3f &bbox, const uint8_t &visbits,
            const ecs::Point4List *additional_data) {
            if (!(visbits & VISFLG_MAIN_VISIBLE))
              return;
            haveVisibleAttaches = true;
            zTestCtx.addOutline(outline__color, outline__extcolor, glob_tm, bbox, render, additional_data);
          });
      if ((animchar_visbits & VISFLG_MAIN_VISIBLE) != 0 || haveVisibleAttaches)
      {
        animchar_visbits |= VISFLG_RENDER_CUSTOM;
        zTestCtx.addOutline(outline__color, outline__extcolor, glob_tm, animchar_bbox, animchar_render, additional_data);
      }
    });

  outline_render_z_pass_ri_ecs_query(
    [&](ECS_REQUIRE(eastl::true_type outline__enabled, eastl::false_type outline__z_fail = false,
          eastl::false_type outline__always_visible = false, eastl::true_type outline__isOccluded) const TMatrix &transform,
      RiExtraComponent ri_extra, Point3 ri_extra__bboxMin, Point3 ri_extra__bboxMax, E3DCOLOR outline__color = 0xFFFFFFFF,
      E3DCOLOR outline__extcolor = 0) {
      add_ri_ex_to_outline_ctx(zTestCtx, glob_tm, ri_extra.handle, transform, ri_extra__bboxMin, ri_extra__bboxMax, outline__color,
        outline__extcolor);
    });

  outline_render_z_pass_ri_handle_ecs_query(
    [&](ECS_REQUIRE(eastl::true_type outline__enabled, eastl::false_type outline__z_fail = false,
          eastl::false_type outline__always_visible = false, eastl::true_type outline__isOccluded)
          ECS_REQUIRE_NOT(RiExtraComponent & ri_extra) const TMatrix &transform,
      const rendinst::riex_handle_t ri_extra__handle, Point3 ri_extra__bboxMin, Point3 ri_extra__bboxMax,
      E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0) {
      add_ri_ex_to_outline_ctx(zTestCtx, glob_tm, ri_extra__handle, transform, ri_extra__bboxMin, ri_extra__bboxMax, outline__color,
        outline__extcolor);
    });

  outline_render_z_fail_ecs_query(
    [&](ecs::EntityId eid, const AnimV20::AnimcharRendComponent &animchar_render, const bbox3f &animchar_bbox,
      uint8_t &animchar_visbits, const ecs::EidList *attaches_list, const ecs::Point4List *additional_data,
      int &outline__frames_visible ECS_REQUIRE(eastl::true_type outline__enabled, eastl::true_type outline__z_fail,
        eastl::false_type outline__always_visible = false, eastl::true_type animchar_render__enabled = true),
      E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0, int outline__frames_history = 20) {
      if (!outline__color.a)
        return;
      auto add = [&outline__color, &outline__extcolor, &glob_tm, &zTestFailCtx, &outline__frames_visible, &outline__frames_history](
                   const AnimV20::AnimcharRendComponent &animchar_render, const bbox3f &animchar_bbox, const uint8_t &visbits,
                   const ecs::Point4List *additional_data) {
        if ((visbits & VISFLG_MAIN_VISIBLE))
        {
          if (outline__frames_visible <= 0)
            return;
          outline__frames_visible--;
        }
        vec3f c = v_bbox3_center(animchar_bbox);
        vec3f smallExt = v_mul(v_sub(animchar_bbox.bmax, c), v_splats(0.5));
        bbox3f check_box = animchar_bbox;
        check_box.bmin = v_sub(c, smallExt);
        check_box.bmax = v_add(c, smallExt);
        if (current_occlusion->isOccludedBox(check_box))
          outline__frames_visible = outline__frames_history;

        zTestFailCtx.addOutline(outline__color, outline__extcolor, glob_tm, animchar_bbox, animchar_render, additional_data);
      };
      if (attaches_list)
        outline_add_attaches(eid, *attaches_list, add);
      animchar_visbits |= VISFLG_RENDER_CUSTOM;
      add(animchar_render, animchar_bbox, animchar_visbits, additional_data);
    });

  outline_render_z_fail_ri_ecs_query([&](ECS_REQUIRE(eastl::true_type outline__enabled, eastl::true_type outline__z_fail,
                                           eastl::false_type outline__always_visible = false) const TMatrix &transform,
                                       RiExtraComponent ri_extra, Point3 ri_extra__bboxMin, Point3 ri_extra__bboxMax,
                                       E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0) {
    add_ri_ex_to_outline_ctx(zTestFailCtx, glob_tm, ri_extra.handle, transform, ri_extra__bboxMin, ri_extra__bboxMax, outline__color,
      outline__extcolor);
  });

  outline_render_z_fail_ri_handle_ecs_query(
    [&](ECS_REQUIRE(eastl::true_type outline__enabled, eastl::true_type outline__z_fail,
          eastl::false_type outline__always_visible = false) ECS_REQUIRE_NOT(RiExtraComponent & ri_extra) const TMatrix &transform,
      const rendinst::riex_handle_t ri_extra__handle, Point3 ri_extra__bboxMin, Point3 ri_extra__bboxMax,
      E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0) {
      add_ri_ex_to_outline_ctx(zTestFailCtx, glob_tm, ri_extra__handle, transform, ri_extra__bboxMin, ri_extra__bboxMax,
        outline__color, outline__extcolor);
    });

  Point4 screenBoxes[OVERRIDES_COUNT] = {zTestFailCtx.calculate_box(), zTestCtx.calculate_box(), noZTestCtx.calculate_box()};

  Point4 finalBox = Point4(1, 1, 0, 0);
  for (int i = 0; i < OVERRIDES_COUNT; i++)
  {
    finalBox.x = min(finalBox.x, screenBoxes[i].x);
    finalBox.y = min(finalBox.y, screenBoxes[i].y);
    finalBox.z = max(finalBox.z, screenBoxes[i].z);
    finalBox.w = max(finalBox.w, screenBoxes[i].w);
  }

  outline_renderer.calculateProjection(finalBox);

  static constexpr bool always_false = false;
  if (always_false) // codegen can't generate otherwise
    attach_ecs_query(ecs::INVALID_ENTITY_ID,
      [&](ecs::EntityId slot_attach__attachedTo, const AnimV20::AnimcharRendComponent &animchar_render, const bbox3f &animchar_bbox,
        const uint8_t &animchar_visbits ECS_REQUIRE(eastl::true_type animchar_render__enabled = true),
        const ecs::Point4List *additional_data = nullptr, const ecs::EidList *attaches_list = nullptr) {
        G_UNUSED(animchar_visbits);
        G_UNUSED(animchar_bbox);
        G_UNUSED(slot_attach__attachedTo);
        G_UNUSED(animchar_render);
        G_UNUSED(additional_data);
        G_UNUSED(attaches_list);
      });
}

#define TRANSPARENCY_NODE_PRIORITY_OUTLINE_APPLY 5
ECS_ON_EVENT(on_appear)
static void create_outline_node_es(const ecs::Event &, dabfg::NodeHandle &outline_prepare_node, dabfg::NodeHandle &outline_apply_node)
{
  auto nodeNs = dabfg::root() / "transparent" / "close";
  outline_prepare_node = nodeNs.registerNode("outline_prepare_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    bool isForward = renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING);
    registry.read(isForward ? "srv_depth_for_transparency" : "opaque_depth_with_water")
      .texture()
      .atStage(dabfg::Stage::PS)
      .bindToShaderVar("depth_gbuf");

    auto outlineDepth = registry.create("outline_depth", dabfg::History::No)
                          .texture({TEXFMT_DEPTH24 | TEXCF_RTARGET, registry.getResolution<2>("main_view")});

    registry.requestRenderPass().depthRw(outlineDepth).clear(outlineDepth, make_clear_value(0.f, 0));

    auto outlineDepthHndl =
      eastl::move(outlineDepth).atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::DEPTH_ATTACHMENT).handle();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.requestState().setFrameBlock("global_frame");


    return [cameraHndl, outlineDepthHndl] {
      render_outline_ecs_query([&](OutlineRenderer &outline_renderer) {
        if (!outline_renderer.anyVisible())
          return;

        const auto &camera = cameraHndl.ref();
        shaders::overrides::reset();

        outline_render_prepare(outline_renderer, camera.viewTm, (mat44f_cref)camera.jitterProjTm);

        d3d::resummarize_htile(outlineDepthHndl.view().getTex2D());
        d3d::resource_barrier({outlineDepthHndl.view().getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      });
    };
  });

  outline_apply_node = nodeNs.registerNode("outline_apply_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    render_transparency(registry);
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_OUTLINE_APPLY);

    auto outlineDepthHndl =
      registry.read("outline_depth").texture().atStage(dabfg::Stage::PS).bindToShaderVar("outline_depth").handle();
    registry.create("outline_depth_sampler", dabfg::History::No)
      .blob(d3d::request_sampler({}))
      .bindToShaderVar("outline_depth_samplerstate");

    read_gbuffer_material_only(registry);

    return [outlineDepthHndl] {
      render_outline_ecs_query([&](OutlineRenderer &outline_renderer) {
        auto outline_depth = outlineDepthHndl.view();

        if (!outline_renderer.anyVisible())
          return;

        TIME_D3D_PROFILE(apply_outlines);
        SCOPE_VIEWPORT; // Workaround for FG not tracking viewport
        outline_renderer.setClippedViewport();

        outline_depth->setReadStencil(true);
        static int outline_final_render_AllColors_const_no = ShaderGlobal::get_slot_by_name("outline_final_render_AllColors_const_no");
        d3d::set_const_buffer(STAGE_PS, outline_final_render_AllColors_const_no, outline_renderer.colorsCB.getBuf());

        outline_renderer.finalRender.render();

        d3d::set_const_buffer(STAGE_PS, outline_final_render_AllColors_const_no, NULL);
        outline_depth->setReadStencil(false);
      });
    };
  });
}

// ri destruction is synced earlier than entity destruction
// as a result the game tries to render outline of a destroyed ri and crashes
// So prevent outline rendering as soon as the client learns that ri is destroyed.
ECS_TAG(render)
static inline void disable_outline_on_ri_destroyed_es_event_handler(const EventRiExtraDestroyed &, bool &outline__enabled)
{
  outline__enabled = false;
}
