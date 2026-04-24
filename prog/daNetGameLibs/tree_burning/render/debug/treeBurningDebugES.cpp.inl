// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug3d.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtraAccess.h>
#include <util/dag_convar.h>
#include <vecmath/dag_vecMath.h>
#include <tree_burning/render/treeBurningManager.h>

static CONSOLE_BOOL_VAL("tree_burning", debug_draw, false);

ECS_NO_ORDER
ECS_TAG(dev, render)
static void tree_burning_debug_visualization_es(const ecs::UpdateStageInfoRenderDebug &,
  const TreeBurningManager &tree_burning_manager)
{
  if (!debug_draw)
    return;
  begin_draw_cached_debug_lines();
  for (auto &[riHandle, offsetCount] : tree_burning_manager.burningTreesAdditionalData)
  {
    Point4 treeBsphere;
    v_stu(&treeBsphere.x, rendinst::getRIGenExtraBSphere(riHandle));
    draw_cached_debug_sphere(Point3::xyz(treeBsphere), fabsf(treeBsphere.w), E3DCOLOR(0xffffffff));
    TMatrix treeTm = rendinst::getRIGenMatrix(rendinst::RendInstDesc(riHandle));
    int offset = offsetCount >> 16;
    int count = offsetCount & 0xffff;
    for (const Point4 &burningSource : make_span_const(tree_burning_manager.burningSources.data() + offset, count))
      draw_cached_debug_sphere(treeTm * Point3::xyz(burningSource), burningSource.w, E3DCOLOR(0xffff0000));
  }
  for (rendinst::riex_handle_t riHandle : tree_burning_manager.burntTrees)
  {
    Point4 treeBsphere;
    v_stu(&treeBsphere.x, rendinst::getRIGenExtraBSphere(riHandle));
    draw_cached_debug_sphere(Point3::xyz(treeBsphere), fabsf(treeBsphere.w), E3DCOLOR(0xffff0000));
  }
  end_draw_cached_debug_lines();
}
