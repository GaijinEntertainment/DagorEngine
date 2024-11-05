// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entitySystem.h>
#include <daECS/core/updateStage.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animchar_visbits.h>
#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <rendInst/rendInstExtra.h>
#include <camera/sceneCam.h>

class AnimCharShadowOcclusionManager;
AnimCharShadowOcclusionManager *get_animchar_shadow_occlusion();
bool is_bbox_visible_in_shadows(const AnimCharShadowOcclusionManager *manager, bbox3f_cref bbox);

CONSOLE_BOOL_VAL("app", debug_shadow_culling, false);

template <typename Callable>
inline void animchar_shadow_cull_bounds_debug_render_ecs_query(Callable c);

ECS_TAG(render, dev)
ECS_NO_ORDER
static __forceinline void shadow_occlusion_render_debug_es(const ecs::UpdateStageInfoRenderDebug &)
{
  if (!debug_shadow_culling.get())
    return;

  const AnimCharShadowOcclusionManager *animCharShadowOcclMgr = get_animchar_shadow_occlusion();

  begin_draw_cached_debug_lines();
  animchar_shadow_cull_bounds_debug_render_ecs_query(
    [&](const bbox3f &animchar_shadow_cull_bbox,
      const uint8_t animchar_visbits ECS_REQUIRE(eastl::true_type animchar_render__enabled)) {
      if (!(animchar_visbits & VISFLG_MAIN_VISIBLE) || is_bbox_visible_in_shadows(animCharShadowOcclMgr, animchar_shadow_cull_bbox))
        return;
      BBox3 box;
      v_stu_bbox3(box, animchar_shadow_cull_bbox);
      draw_cached_debug_box(box, E3DCOLOR_MAKE(255, 0, 0, 255));
    });

  TMatrix camTm = ECS_GET_OR(get_cur_cam_entity(), transform, TMatrix::IDENT);
  vec4f viewPos = v_ldu(&camTm.getcol(3).x);
  vec4f gatherBoxSize = v_splats(100.0f);
  bbox3f gatherBbox;
  gatherBbox.bmin = v_sub(viewPos, gatherBoxSize);
  gatherBbox.bmax = v_add(viewPos, gatherBoxSize);
  Tab<bbox3f> riBboxes(framemem_ptr());
  rendinst::gatherRIGenExtraShadowInvisibleBboxes(riBboxes, gatherBbox);
  for (bbox3f &bbox : riBboxes)
  {
    BBox3 box;
    v_stu_bbox3(box, bbox);
    draw_cached_debug_box(box, E3DCOLOR_MAKE(255, 0, 0, 255));
  }

  end_draw_cached_debug_lines();
}
