// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <outline/outlineCommon.h>

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

#include <scene/dag_occlusion.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/rendInst/riExtra.h>

#include <daECS/core/utility/enumRegistration.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>

static int outline_scaleVarId = -1, outline_blur_widthVarId = -1, outline_brightnessVarId = -1;

ECS_REGISTER_RELOCATABLE_TYPE(OutlineMode, nullptr);
ECS_DECLARE_ENUM(OutlineMode, ZTEST_FAIL, ZTEST_PASS, NO_ZTEST);
ECS_REGISTER_ENUM(OutlineMode);

ECS_REGISTER_RELOCATABLE_TYPE(OutlineRenderer, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(OutlineContexts, nullptr);

static bbox3f make_world_bbox(Point3 min_bound, Point3 max_bound, const TMatrix &transform)
{
  bbox3f localBox{v_ldu(&min_bound.x), v_ldu(&max_bound.x)};
  mat44f wtm;
  v_mat44_make_from_43cu_unsafe(wtm, transform.array);
  bbox3f worldBox;
  v_bbox3_init(worldBox, wtm, localBox);
  return worldBox;
}

static bool outline_culling(mat44f_cref globtm, bbox3f_cref wbox, vec4f &clip_screen_box)
{
  vec3f threshold = v_make_vec4f(4.f / 1920, 4.f / 1080, 0, 0); // 4 FULLHD pixel minimum
  if (!v_screen_size_b(wbox.bmin, wbox.bmax, threshold, clip_screen_box, globtm))
    return false;

  clip_screen_box = v_min(v_max(clip_screen_box, v_neg(V_C_ONE)), V_C_ONE);
  return true;
}

OutlineElement::OutlineElement(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color) :
  clipBox(clipBox), int_color(int_color), ext_color(ext_color)
{}
bool OutlineElement::operator<(const OutlineElement &rhs) const
{
  if (int_color != rhs.int_color)
    return int_color < rhs.int_color;
  return ext_color < rhs.ext_color;
}

OutlinedAnimchar::OutlinedAnimchar(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color,
  const AnimV20::AnimcharRendComponent *animchar, const ecs::Point4List *additionalData) :
  OutlineElement(clipBox, int_color, ext_color), animchar(animchar), additionalData(additionalData)
{}

OutlinedRendinst::OutlinedRendinst(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color, const TMatrix &transform, uint32_t ri_idx) :
  OutlineElement(clipBox, int_color, ext_color), transform(transform), riIdx(ri_idx)
{}

void OutlineContext::addOutline(E3DCOLOR int_color, E3DCOLOR ext_color, mat44f_cref globtm, bbox3f_cref wbox,
  const AnimV20::AnimcharRendComponent &animchar, const ecs::Point4List *additional_data)
{
  vec4f clipScreenBox;
  if (outline_culling(globtm, wbox, clipScreenBox))
  {
    if (ext_color == 0)
      ext_color = int_color;
    animcharElements.emplace_back(clipScreenBox, int_color, ext_color, &animchar, additional_data);
  }
}

void OutlineContext::addOutline(E3DCOLOR int_color, E3DCOLOR ext_color, mat44f_cref globtm, bbox3f_cref wbox, const TMatrix &transform,
  uint32_t ri_idx)
{
  vec4f clipScreenBox;
  if (outline_culling(globtm, wbox, clipScreenBox))
  {
    if (ext_color == 0)
      ext_color = int_color;
    riElements.emplace_back(clipScreenBox, int_color, ext_color, transform, ri_idx);
  }
}

Point4 OutlineContext::calculate_box() const
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

void OutlineContext::clear()
{
  animcharElements.clear();
  riElements.clear();
}

bool OutlineContext::empty() const { return animcharElements.empty() && riElements.empty(); }

bool OutlineContexts::anyVisible() const
{
  return !(context[ZTEST_PASS].empty() && context[ZTEST_FAIL].empty() && context[NO_ZTEST].empty());
}

Point4 OutlineContexts::calculateBox() const
{
  Point4 screenBoxes[OVERRIDES_COUNT] = {
    context[ZTEST_FAIL].calculate_box(), context[ZTEST_PASS].calculate_box(), context[NO_ZTEST].calculate_box()};

  Point4 finalBox = Point4(1, 1, 0, 0);
  for (int i = 0; i < OVERRIDES_COUNT; i++)
  {
    finalBox.x = min(finalBox.x, screenBoxes[i].x);
    finalBox.y = min(finalBox.y, screenBoxes[i].y);
    finalBox.z = max(finalBox.z, screenBoxes[i].z);
    finalBox.w = max(finalBox.w, screenBoxes[i].w);
  }
  return finalBox;
}

void OutlineRenderer::resetColorBuffer()
{
  colors.clear();
  colors.emplace_back(color4(0));
  colors.emplace_back(color4(0));
}

void OutlineRenderer::uploadColorBufferToGPU()
{
  colorsCB->updateData(0, colors.size() * sizeof(*colors.data()), colors.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
}

void OutlineRenderer::updateScaleVar()
{
  float wInv = 1.f / width, hInv = 1.f / height;
  ShaderGlobal::set_color4(outline_scaleVarId, clipLeft * wInv, clipTop * hInv, clipWidth * wInv, clipHeight * hInv);
}

void OutlineRenderer::setClippedViewport() { d3d::setview(clipLeft, clipTop, clipWidth, clipHeight, 0, 1); }

shaders::OverrideStateId OutlineRenderer::create_override(int override_type)
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

void OutlineRenderer::init(float outline_brightness)
{
  for (uint i = 0; i < OVERRIDES_COUNT; ++i)
    zTestOverrides[i] = create_override(i);

  finalRender.init("outline_final_render");
  fillDepthRender.init("outline_fill_depth");
  colorsCB = dag::buffers::create_one_frame_cb(COLOR_BUF_SIZE, "outline_colors");
  rendInstTransforms = dag::buffers::create_one_frame_sr_tbuf(4U, TEXFMT_A32B32G32R32F, "OutlineRenderer_transformsBuffer");

  outline_scaleVarId = get_shader_variable_id("outline_scale", true);
  outline_blur_widthVarId = get_shader_variable_id("outline_blur", true);
  outline_brightnessVarId = get_shader_variable_id("outline_brighness", true);
  ShaderGlobal::set_real(outline_brightnessVarId, outline_brightness);
}

void OutlineRenderer::changeResolution(int w, int h)
{
  width = w;
  height = h;
}

void OutlineRenderer::initBlurWidth(float outline_blur_width, int display_width, int display_height)
{
  ShaderGlobal::set_color4(outline_blur_widthVarId, outline_blur_width * width / display_width,
    outline_blur_width * height / display_height, 0, 0);
}

void OutlineRenderer::calculateProjection(Point4 clipBox)
{
  Point4 clipBoxTc =
    saturate(Point4(clipBox.x * 0.5f + 0.5f, clipBox.y * -0.5f + 0.5f, clipBox.z * 0.5f + 0.5f, clipBox.w * -0.5f + 0.5f));
  clipLeft = floorf(width * clipBoxTc.x);
  int clipRight = ceilf(width * clipBoxTc.z);
  int clipBottom = ceilf(height * clipBoxTc.y);
  clipTop = floorf(height * clipBoxTc.w);
  clipWidth = clipRight - clipLeft;
  clipHeight = clipBottom - clipTop;

  float invWidth = 1.f / width;
  float invHeight = 1.f / height;
  clipBox = Point4((clipLeft * invWidth) * 2.f - 1.f, -((clipBottom * invHeight) * 2.f - 1.f), (clipRight * invWidth) * 2.f - 1.f,
    -((clipTop * invHeight) * 2.f - 1.f));
  tm = matrix_ortho_off_center_lh(clipBox.x, clipBox.z, clipBox.y, clipBox.w, 0, 1);
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

template <typename F>
static void outline_add_attaches(ecs::EntityId eid, const ecs::EidList &attaches, const F &add)
{
  for (auto attach_eid : attaches)
  {
    attach_ecs_query(attach_eid,
      [&](ecs::EntityId animchar_attach__attachedTo, const AnimV20::AnimcharRendComponent &animchar_render,
        const bbox3f &animchar_bbox,
        const animchar_visbits_t &animchar_visbits ECS_REQUIRE(eastl::true_type animchar_render__enabled = true),
        const ecs::Point4List *additional_data = nullptr, const ecs::EidList *attaches_list = nullptr) {
        if (animchar_attach__attachedTo != eid) // sanity check
          return;
        if (attaches_list)
          outline_add_attaches(attach_eid, *attaches_list, add);
        add(animchar_render, animchar_bbox, animchar_visbits, additional_data);
      });
  }
};

static void add_ri_ex_to_outline_ctx(OutlineContext &outlineContext, mat44f_cref glob_tm,
  const rendinst::riex_handle_t ri_extra__handle, const TMatrix &transform, Point3 ri_extra__bboxMin, Point3 ri_extra__bboxMax,
  E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0)
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

void outline_update_contexts(OutlineContexts &outline_ctxs, const TMatrix4 &globtm, const Occlusion *occlusion)
{
  mat44f glob_tm;
  v_mat44_make_from_44cu(glob_tm, &globtm.row[0].x);
  OutlineContext &zTestFailCtx = outline_ctxs.context[ZTEST_FAIL];
  OutlineContext &zTestCtx = outline_ctxs.context[ZTEST_PASS];
  OutlineContext &noZTestCtx = outline_ctxs.context[NO_ZTEST];
  noZTestCtx.clear();
  zTestCtx.clear();
  zTestFailCtx.clear();

  outline_render_always_visible_ecs_query(
    [&](ecs::EntityId eid, AnimV20::AnimcharRendComponent &animchar_render,
      animchar_visbits_t &animchar_visbits ECS_REQUIRE(eastl::true_type outline__enabled, eastl::true_type outline__always_visible,
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
      animchar_visbits |= VISFLG_OUTLINE_RENDER;
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
      const bbox3f &animchar_bbox, animchar_visbits_t &animchar_visbits, const ecs::EidList *attaches_list,
      const ecs::Point4List *additional_data, E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0) {
      if (!outline__color.a)
        return;
      bool haveVisibleAttaches = false;
      if (attaches_list)
        outline_add_attaches(eid, *attaches_list,
          [&outline__color, &outline__extcolor, &glob_tm, &zTestCtx, &haveVisibleAttaches](
            const AnimV20::AnimcharRendComponent &render, const bbox3f &bbox, const animchar_visbits_t &visbits,
            const ecs::Point4List *additional_data) {
            if (!(visbits & (VISFLG_MAIN_VISIBLE | VISFLG_MAIN_AND_SHADOW_VISIBLE)))
              return;
            haveVisibleAttaches = true;
            zTestCtx.addOutline(outline__color, outline__extcolor, glob_tm, bbox, render, additional_data);
          });
      if ((animchar_visbits & (VISFLG_MAIN_VISIBLE | VISFLG_MAIN_AND_SHADOW_VISIBLE)) != 0 || haveVisibleAttaches)
      {
        animchar_visbits |= VISFLG_OUTLINE_RENDER;
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
      animchar_visbits_t &animchar_visbits, const ecs::EidList *attaches_list, const ecs::Point4List *additional_data,
      int &outline__frames_visible ECS_REQUIRE(eastl::true_type outline__enabled, eastl::true_type outline__z_fail,
        eastl::false_type outline__always_visible = false, eastl::true_type animchar_render__enabled = true),
      E3DCOLOR outline__color = 0xFFFFFFFF, E3DCOLOR outline__extcolor = 0, int outline__frames_history = 20) {
      if (!outline__color.a)
        return;
      auto add = [occlusion, &outline__color, &outline__extcolor, &glob_tm, &zTestFailCtx, &outline__frames_visible,
                   &outline__frames_history](const AnimV20::AnimcharRendComponent &animchar_render, const bbox3f &animchar_bbox,
                   const animchar_visbits_t &visbits, const ecs::Point4List *additional_data) {
        if ((visbits & (VISFLG_MAIN_VISIBLE | VISFLG_MAIN_AND_SHADOW_VISIBLE)))
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
        if (!occlusion || occlusion->isOccludedBox(check_box))
          outline__frames_visible = outline__frames_history;

        zTestFailCtx.addOutline(outline__color, outline__extcolor, glob_tm, animchar_bbox, animchar_render, additional_data);
      };
      if (attaches_list)
        outline_add_attaches(eid, *attaches_list, add);
      animchar_visbits |= VISFLG_OUTLINE_RENDER;
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

  static constexpr bool always_false = false;
  if (always_false) // codegen can't generate otherwise
    attach_ecs_query(ecs::INVALID_ENTITY_ID,
      [&](ecs::EntityId animchar_attach__attachedTo, const AnimV20::AnimcharRendComponent &animchar_render,
        const bbox3f &animchar_bbox,
        const animchar_visbits_t &animchar_visbits ECS_REQUIRE(eastl::true_type animchar_render__enabled = true),
        const ecs::Point4List *additional_data = nullptr, const ecs::EidList *attaches_list = nullptr) {
        G_UNUSED(animchar_visbits);
        G_UNUSED(animchar_bbox);
        G_UNUSED(animchar_attach__attachedTo);
        G_UNUSED(animchar_render);
        G_UNUSED(additional_data);
        G_UNUSED(attaches_list);
      });
}

static void outline_prepare_depth(OutlineRenderer &outline_renderer, OutlineContexts &outline_ctxs)
{
  TIME_D3D_PROFILE(render_outline);

  outline_renderer.calculateProjection(outline_ctxs.calculateBox());
  outline_renderer.updateScaleVar();
  outline_renderer.setClippedViewport();

  const bool shouldFillDepth = !(outline_ctxs.context[ZTEST_PASS].empty() && outline_ctxs.context[ZTEST_FAIL].empty());
  if (shouldFillDepth) // copy depth from depth_gbuf
  {
    TIME_D3D_PROFILE(fill_depth);
    outline_renderer.fillDepthRender.render();
    ShaderElement::invalidate_cached_state_block();
  }
}

void outline_prepare(OutlineRenderer &outline_renderer, OutlineContexts &outline_ctxs, Texture *outline_depth, const TMatrix &view_tm,
  const TMatrix4_vec4 &proj_tm)
{
  TIME_D3D_PROFILE(prepare_stencil_outlines);

  outline_prepare_depth(outline_renderer, outline_ctxs);
  outline_render_elements(outline_renderer, outline_ctxs, view_tm, proj_tm);
  outline_renderer.uploadColorBufferToGPU();

  d3d::resummarize_htile(outline_depth);
  d3d::resource_barrier({outline_depth, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}

void outline_apply(OutlineRenderer &outline_renderer, Texture *outline_depth)
{
  TIME_D3D_PROFILE(apply_stencil_outlines);

  outline_renderer.setClippedViewport();

  outline_depth->setReadStencil(true);
  static int outline_final_render_AllColors_const_no = ShaderGlobal::get_slot_by_name("outline_final_render_AllColors_const_no");
  d3d::set_const_buffer(STAGE_PS, outline_final_render_AllColors_const_no, outline_renderer.colorsCB.getBuf());

  outline_renderer.finalRender.render();

  d3d::set_const_buffer(STAGE_PS, outline_final_render_AllColors_const_no, NULL);
  outline_depth->setReadStencil(false);
}
