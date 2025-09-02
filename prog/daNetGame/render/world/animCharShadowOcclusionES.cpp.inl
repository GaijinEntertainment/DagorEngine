// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <generic/dag_tab.h>
#include <math/dag_vecMathCompatibility.h>
#include <ecs/anim/anim.h>
#include <util/dag_delayedAction.h>
#include <render/toroidalStaticShadows.h>
#include <render/world/shadowsManager.h>
#include <shaders/dag_dynSceneRes.h>
#include <ecs/anim/animchar_visbits.h>

class AnimCharShadowOcclusionManager
{
  static constexpr int GRID_SIZE = 128;
  static constexpr int GRID_ELEMENT_MAX_BOXES = 3;

  struct CullGridElement
  {
    int boxes[GRID_ELEMENT_MAX_BOXES] = {};
    int boxesCntAtCell = 0;
  };

  IPoint2 calcCellPos(const Point3 &world_pos) const
  {
    // floor here is to handle negative values, as cast them to int is equivalent to round them up.
    return ipoint2(floor((Point2(world_pos.x, world_pos.z) - gridWorldOrigin) * invGridCellSize));
  }

  IBBox2 calcBoxCellArea(bbox3f_cref world_box) const
  {
    BBox3 bb;
    v_stu_bbox3(bb, world_box);
    IPoint2 fromCell = max(calcCellPos(bb.boxMin()), IPoint2(0, 0));
    IPoint2 toCell = min(calcCellPos(bb.boxMax()), IPoint2(GRID_SIZE - 1, GRID_SIZE - 1));
    return IBBox2(fromCell, toCell);
  }

public:
  bool isBboxVisible(bbox3f_cref bbox) const
  {
    Point3_vec4 center;
    v_st(&center.x, v_mul(v_add(bbox.bmin, bbox.bmax), V_C_HALF));
    IPoint2 cellPos = calcCellPos(center);
    if (cellPos.x >= 0 && cellPos.x < GRID_SIZE && cellPos.y >= 0 && cellPos.y < GRID_SIZE)
    {
      const CullGridElement &elem = cullGrid[cellPos.x * GRID_SIZE + cellPos.y];
      for (int i = 0; i < elem.boxesCntAtCell; ++i)
        if (bool(v_bbox3_test_box_inside(cullBboxes[elem.boxes[i]], bbox)))
          return false;
    }
    return true;
  }

  // Such invalidation isn't accurate.
  // There can be bboxes which tested before the invalidation frame
  // but their bboxes aren't read. In this case the shadow could
  // disappear for one-two frames if it intersects with the invalidated box.
  // Since the situation is rare and not significant it is unhandled.
  void invalidateBox(bbox3f_cref box)
  {
    IBBox2 area = calcBoxCellArea(box);
    for (int x = area[0].x; x <= area[1].x; ++x)
      for (int z = area[0].y; z <= area[1].y; ++z)
        cullGrid[x * GRID_SIZE + z].boxesCntAtCell = 0;
  }

  void invalidate()
  {
    for (CullGridElement &elem : cullGrid)
      elem.boxesCntAtCell = 0;
  }

  void updateCullBboxes(const dag::ConstSpan<bbox3f> &cull_bboxes, const Point3 &cam_pos, float area_half_size)
  {
    areaHalfSize = area_half_size;
    areaSize = areaHalfSize * 2.0f;
    gridCellSize = areaSize / GRID_SIZE;
    invGridCellSize = 1.0f / gridCellSize;
    cullBboxes.assign(cull_bboxes.begin(), cull_bboxes.end());
    gridWorldOrigin = Point2(cam_pos.x - areaHalfSize, cam_pos.z - areaHalfSize);
    invalidate();

    for (unsigned bboxId = 0; bboxId < cullBboxes.size(); ++bboxId)
    {
      IBBox2 area = calcBoxCellArea(cullBboxes[bboxId]);
      for (int x = area[0].x; x <= area[1].x; ++x)
      {
        for (int z = area[0].y; z <= area[1].y; ++z)
        {
          CullGridElement &elem = cullGrid[x * GRID_SIZE + z];
          if (elem.boxesCntAtCell < GRID_ELEMENT_MAX_BOXES)
            elem.boxes[elem.boxesCntAtCell++] = bboxId;
        }
      }
    }
  }

private:
  // Just a lookup table for boxes against which to test visibility.
  eastl::array<CullGridElement, GRID_SIZE * GRID_SIZE> cullGrid;
  dag::Vector<bbox3f> cullBboxes;
  Point2 gridWorldOrigin;

  float areaHalfSize = 0.0f;
  float areaSize = 0.0f;
  float gridCellSize = 1.0f;
  float invGridCellSize = 1.0f;
};

ECS_DECLARE_RELOCATABLE_TYPE(AnimCharShadowOcclusionManager);
ECS_REGISTER_RELOCATABLE_TYPE(AnimCharShadowOcclusionManager, nullptr);
ECS_AUTO_REGISTER_COMPONENT(AnimCharShadowOcclusionManager, "animchar_shadow_occlusion_manager", nullptr, 0);

template <typename Callable>
inline void animchar_shadow_occlusion_manager_ecs_query(Callable c);

template <typename Callable>
inline void test_box_half_size_ecs_query(Callable c);

template <typename Callable>
inline void gather_soldier_bboxes_to_cull_ecs_query(Callable c);

template <typename Callable>
inline void expand_bbox_by_attach_ecs_query(ecs::EntityId eid, Callable c);

AnimCharShadowOcclusionManager *get_animchar_shadow_occlusion()
{
  AnimCharShadowOcclusionManager *result = nullptr;
  animchar_shadow_occlusion_manager_ecs_query(
    [&](AnimCharShadowOcclusionManager &animchar_shadow_occlusion_manager) { result = &animchar_shadow_occlusion_manager; });
  return result;
}

bool is_bbox_visible_in_shadows(const AnimCharShadowOcclusionManager *manager, bbox3f_cref bbox)
{
  if (manager)
    return manager->isBboxVisible(bbox);
  return true;
}

static void expand_bbox_by_attaches(bbox3f &bbox, ecs::EntityId owner_eid, const ecs::EidList *attaches_list)
{
  if (!attaches_list)
    return;

  for (ecs::EntityId attachedEid : *attaches_list)
  {
    expand_bbox_by_attach_ecs_query(attachedEid,
      [&](const bbox3f &animchar_shadow_cull_bbox, const animchar_visbits_t &animchar_visbits,
        ecs::EntityId animchar_attach__attachedTo ECS_REQUIRE(eastl::true_type animchar_render__enabled = true)) {
        // we process shadow visibility in same time in other thread. it adds _other_ bit
        if ((interlocked_relaxed_load(animchar_visbits) & VISFLG_WITHIN_RANGE) && animchar_attach__attachedTo == owner_eid)
          v_bbox3_add_box(bbox, animchar_shadow_cull_bbox);
      });
  }
}

bbox3f ShadowsManager::gatherBboxesForAnimcharShadowCull(const Point3 &cam_pos,
  dag::Vector<GpuVisibilityTestManager::TestObjectData, framemem_allocator> &out_bboxes,
  uint32_t usr_data) const
{
  constexpr float CULL_TEST_FRAMES_DELAY = 2.0f;
  constexpr int MAX_BBOX_SIZE_SQ_TEXELS = 16384;

  bbox3f overallBbox;
  v_bbox3_init_empty(overallBbox);
  if (!staticShadows || staticShadows->cascadesCount() <= 0)
    return overallBbox;

  constexpr size_t TOROIDAL_TEXTURE_MAX_VIEWS_PER_BOX = 4;
  eastl::fixed_vector<ViewTransformData, TOROIDAL_TEXTURE_MAX_VIEWS_PER_BOX> views(TOROIDAL_TEXTURE_MAX_VIEWS_PER_BOX);
  {
    int viewsCnt;
    staticShadows->getCascadeWorldBoxViews(staticShadows->cascadesCount() - 1, views.data(), viewsCnt);
    views.resize(viewsCnt);
  }

  if (views.size() <= 0)
    return overallBbox;

  static Point3 prevCamPos = cam_pos;
  Point3 camPosDiff = cam_pos - prevCamPos;

  float maxTestingDist = csmShadowsMaxDist;
  vec4f testBboxExpandSize = v_zero();
  vec4f testBboxMaxHalfSize = v_zero();
  test_box_half_size_ecs_query(
    [&](const Point3 &soldier_bbox_expand_size,
      const Point3 &soldier_bbox_max_half_size ECS_REQUIRE(const AnimCharShadowOcclusionManager &animchar_shadow_occlusion_manager)) {
      testBboxExpandSize = v_make_vec4f(soldier_bbox_expand_size.x, soldier_bbox_expand_size.y, soldier_bbox_expand_size.z, 0.0f);
      testBboxMaxHalfSize =
        v_make_vec4f(soldier_bbox_max_half_size.x, soldier_bbox_max_half_size.y, soldier_bbox_max_half_size.z, 0.0f);
    });
  float expandSizeShadows = getStaticShadowsBiggestTexelSize() * 0.5f;
  vec4f bboxExpandSize = v_add(v_make_vec4f(expandSizeShadows, expandSizeShadows, expandSizeShadows, 0.0f), testBboxExpandSize);

  gather_soldier_bboxes_to_cull_ecs_query(
    [&](ecs::EntityId eid, const AnimV20::AnimcharRendComponent &animchar_render, const vec4f &animchar_bsph,
      const bbox3f &animchar_shadow_cull_bbox, const animchar_visbits_t &animchar_visbits,
      const ecs::EidList *attaches_list ECS_REQUIRE(eastl::true_type animchar_render__enabled = true) ECS_REQUIRE(ecs::Tag human)) {
      vec4f r = v_sub(animchar_bsph, v_ldu(&cam_pos.x));
      if (!(interlocked_relaxed_load(animchar_visbits) & VISFLG_WITHIN_RANGE) ||
          v_extract_x(v_dot3_x(r, r)) > maxTestingDist * maxTestingDist)
        return;
      bbox3f testBbox = animchar_shadow_cull_bbox;
      expand_bbox_by_attaches(testBbox, eid, attaches_list);

      if (!v_bbox3_is_empty(testBbox))
      {
        const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
        Point3_vec4 movement = scene->getNodeWtm(0).getcol(3) - scene->getPrevNodeWtm(0).getcol(3) + camPosDiff;
        vec4f predictedMovement =
          v_mul(v_ld(&movement.x), v_make_vec4f(CULL_TEST_FRAMES_DELAY, CULL_TEST_FRAMES_DELAY, CULL_TEST_FRAMES_DELAY, 0.0f));

        vec4f movementExpandMin = v_min(predictedMovement, v_zero());
        vec4f movementExpandMax = v_max(predictedMovement, v_zero());
        testBbox.bmin = v_add(v_sub(testBbox.bmin, bboxExpandSize), movementExpandMin);
        testBbox.bmax = v_add(v_add(testBbox.bmax, bboxExpandSize), movementExpandMax);

        testBbox.bmin = v_max(testBbox.bmin, v_sub(animchar_bsph, testBboxMaxHalfSize));
        testBbox.bmax = v_min(testBbox.bmax, v_add(animchar_bsph, testBboxMaxHalfSize));

        // Any view is suitable since we only need distance between points.
        vec4f bminScr = v_mat44_mul_vec3v(views[0].globtm, testBbox.bmin);
        vec4f bmaxScr = v_mat44_mul_vec3v(views[0].globtm, testBbox.bmax);
        vec4f bsizeScr = v_sub(v_max(bminScr, bmaxScr), v_min(bminScr, bmaxScr));
        vec4f bsizeVp = v_mul(bsizeScr, v_make_vec4f(views[0].w, views[0].h, 0, 0));
        vec4f bsizeVpExtended = v_ceil(v_add(bsizeVp, v_splats(2.f)));
        float area = v_extract_x(v_mul_x(bsizeVpExtended, v_splat_y(bsizeVpExtended)));
        if (area > MAX_BBOX_SIZE_SQ_TEXELS * 4.f)
          return;

        testBbox.bmax = v_perm_xyzd(testBbox.bmax, v_cast_vec4f(v_splatsi(usr_data)));
        out_bboxes.push_back(testBbox);
        v_bbox3_add_box(overallBbox, testBbox);
      }
    });

  prevCamPos = cam_pos;

  return overallBbox;
}

void ShadowsManager::setAnimcharShadowCullBboxes(dag::Span<bbox3f> tested_bboxes, const Point3 &cam_pos)
{
  vec4f vExpandSize = v_splats(getStaticShadowsBiggestTexelSize() * 0.5f);
  animchar_shadow_occlusion_manager_ecs_query([&](AnimCharShadowOcclusionManager &animchar_shadow_occlusion_manager) {
    // We need to transform back the tested bbox to the original bbox.
    for (bbox3f &bbox : tested_bboxes)
    {
      bbox.bmin = v_add(bbox.bmin, vExpandSize);
      bbox.bmax = v_sub(bbox.bmax, vExpandSize);
    }
    animchar_shadow_occlusion_manager.updateCullBboxes(tested_bboxes, cam_pos, csmShadowsMaxDist);
  });
}

void ShadowsManager::invalidateAnimCharShadowOcclusionBox(const BBox3 &box)
{
  animchar_shadow_occlusion_manager_ecs_query([&](AnimCharShadowOcclusionManager &animchar_shadow_occlusion_manager) {
    bbox3f vBox;
    vBox.bmin = v_make_vec4f(box[0].x, box[0].y, box[0].z, 0.0f);
    vBox.bmax = v_make_vec4f(box[1].x, box[1].y, box[1].z, 0.0f);
    animchar_shadow_occlusion_manager.invalidateBox(vBox);
  });
}

static void invalidate_animchar_shadow_occlusion()
{
  animchar_shadow_occlusion_manager_ecs_query(
    [&](AnimCharShadowOcclusionManager &animchar_shadow_occlusion_manager) { animchar_shadow_occlusion_manager.invalidate(); });
}

struct InvalidateAnimcharShadowOcclusionAction final : public DelayedAction
{
  void performAction() override { invalidate_animchar_shadow_occlusion(); }
};

void ShadowsManager::invalidateAnimCharShadowOcclusion()
{
  if (is_main_thread())
    invalidate_animchar_shadow_occlusion();
  else
    add_delayed_action(new InvalidateAnimcharShadowOcclusionAction());
}
