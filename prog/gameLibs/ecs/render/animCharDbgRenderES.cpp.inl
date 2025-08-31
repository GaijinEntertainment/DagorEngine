// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entitySystem.h>
#include <shaders/dag_dynSceneRes.h>
#include <daECS/core/updateStage.h>
#include <ecs/anim/anim.h>
#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <ecs/anim/animchar_visbits.h>

CONSOLE_BOOL_VAL("anim", render_debug_bounds, false);
CONSOLE_BOOL_VAL("anim", render_skeleton, false);

template <typename Callable>
inline void animchar_bounds_debug_render_ecs_query(Callable c);

static void drawSkeletonLinks(const GeomNodeTree &tree, dag::Index16 n, const DynamicRenderableSceneInstance *scn,
  const AnimcharNodesMat44 &animchar_node_wtm)
{
  const mat44f &nwtm = animchar_node_wtm.nwtm[n.index()];
  Point3 wofs = as_point3(&animchar_node_wtm.wofs);
  for (unsigned i = 0, ie = tree.getChildCount(n); i < ie; i++)
  {
    auto cn = tree.getChildNodeIdx(n, i);
    if (cn.index() >= tree.importantNodeCount())
      continue;
    const mat44f &cnwtm = animchar_node_wtm.nwtm[cn.index()];
    draw_cached_debug_line(as_point3(&nwtm.col3) + wofs, as_point3(&cnwtm.col3) + wofs, E3DCOLOR(128, 128, 255, 255));
    if (scn && scn->getNodeId(tree.getNodeName(n)) >= 0)
      draw_cached_debug_sphere(as_point3(&nwtm.col3) + wofs, 0.03, E3DCOLOR(0, 0, 255, 255));
    drawSkeletonLinks(tree, cn, scn, animchar_node_wtm);
  }
}

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void animchar_render_debug_es(const ecs::UpdateStageInfoRenderDebug &)
{
  if (!render_debug_bounds.get() && !render_skeleton.get())
    return;

  begin_draw_cached_debug_lines();
  animchar_bounds_debug_render_ecs_query(
    [&](const AnimV20::AnimcharBaseComponent &animchar, const AnimV20::AnimcharRendComponent &animchar_render,
      const AnimcharNodesMat44 &animchar_node_wtm, const vec4f &animchar_bsph, const bbox3f &animchar_bbox,
      const animchar_visbits_t animchar_visbits ECS_REQUIRE(eastl::true_type animchar_render__enabled)) {
      if (!animchar_visbits)
        return;

      if (render_debug_bounds.get())
      {
        if (as_point4(&animchar_bsph).w > 0)
          draw_cached_debug_sphere(as_point3(&animchar_bsph), as_point4(&animchar_bsph).w, E3DCOLOR_MAKE(128, 128, 128, 255));

        BBox3 box;
        v_stu_bbox3(box, animchar_bbox);
        draw_cached_debug_box(box, E3DCOLOR_MAKE(0, 255, 0, 255));
      }

      if (render_skeleton.get())
      {
        const DynamicRenderableSceneInstance *scn = animchar_render.getSceneInstance();
        const GeomNodeTree &tree = animchar.getNodeTree();
        if (!tree.empty())
        {
          draw_cached_debug_sphere(as_point3(&animchar_node_wtm.nwtm[0].col3) + as_point3(&animchar_node_wtm.wofs), 0.06,
            E3DCOLOR(255, 0, 255, 255));
          for (unsigned i = 0, ie = tree.getChildCount(dag::Index16(0)); i < ie; i++)
            drawSkeletonLinks(tree, tree.getChildNodeIdx(dag::Index16(0), i), scn, animchar_node_wtm);
        }
      }
    });
  end_draw_cached_debug_lines();
}
