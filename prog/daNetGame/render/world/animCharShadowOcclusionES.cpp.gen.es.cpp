#include "animCharShadowOcclusionES.cpp.inl"
ECS_DEF_PULL_VAR(animCharShadowOcclusion);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_shadow_occlusion_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_shadow_occlusion_manager"), ecs::ComponentTypeInfo<AnimCharShadowOcclusionManager>()}
};
static ecs::CompileTimeQueryDesc animchar_shadow_occlusion_manager_ecs_query_desc
(
  "animchar_shadow_occlusion_manager_ecs_query",
  make_span(animchar_shadow_occlusion_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void animchar_shadow_occlusion_manager_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_shadow_occlusion_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_shadow_occlusion_manager_ecs_query_comps, "animchar_shadow_occlusion_manager", AnimCharShadowOcclusionManager)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc expand_bbox_by_attach_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("animchar_shadow_cull_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc expand_bbox_by_attach_ecs_query_desc
(
  "expand_bbox_by_attach_ecs_query",
  empty_span(),
  make_span(expand_bbox_by_attach_ecs_query_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void expand_bbox_by_attach_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, expand_bbox_by_attach_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          if ( !(ECS_RO_COMP_OR(expand_bbox_by_attach_ecs_query_comps, "animchar_render__enabled", bool( true))) )
            return;
          function(
              ECS_RO_COMP(expand_bbox_by_attach_ecs_query_comps, "animchar_shadow_cull_bbox", bbox3f)
            , ECS_RO_COMP(expand_bbox_by_attach_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP(expand_bbox_by_attach_ecs_query_comps, "slot_attach__attachedTo", ecs::EntityId)
            );

        }
      }
  );
}
static constexpr ecs::ComponentDesc test_box_half_size_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("soldier_bbox_expand_size"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("soldier_bbox_max_half_size"), ecs::ComponentTypeInfo<Point3>()},
//start of 1 rq components at [2]
  {ECS_HASH("animchar_shadow_occlusion_manager"), ecs::ComponentTypeInfo<AnimCharShadowOcclusionManager>()}
};
static ecs::CompileTimeQueryDesc test_box_half_size_ecs_query_desc
(
  "test_box_half_size_ecs_query",
  empty_span(),
  make_span(test_box_half_size_ecs_query_comps+0, 2)/*ro*/,
  make_span(test_box_half_size_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void test_box_half_size_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, test_box_half_size_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(test_box_half_size_ecs_query_comps, "soldier_bbox_expand_size", Point3)
            , ECS_RO_COMP(test_box_half_size_ecs_query_comps, "soldier_bbox_max_half_size", Point3)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_soldier_bboxes_to_cull_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_shadow_cull_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [7]
  {ECS_HASH("human"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_soldier_bboxes_to_cull_ecs_query_desc
(
  "gather_soldier_bboxes_to_cull_ecs_query",
  empty_span(),
  make_span(gather_soldier_bboxes_to_cull_ecs_query_comps+0, 7)/*ro*/,
  make_span(gather_soldier_bboxes_to_cull_ecs_query_comps+7, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gather_soldier_bboxes_to_cull_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_soldier_bboxes_to_cull_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP_OR(gather_soldier_bboxes_to_cull_ecs_query_comps, "animchar_render__enabled", bool( true))) )
            continue;
          function(
              ECS_RO_COMP(gather_soldier_bboxes_to_cull_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(gather_soldier_bboxes_to_cull_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(gather_soldier_bboxes_to_cull_ecs_query_comps, "animchar_bsph", vec4f)
            , ECS_RO_COMP(gather_soldier_bboxes_to_cull_ecs_query_comps, "animchar_shadow_cull_bbox", bbox3f)
            , ECS_RO_COMP(gather_soldier_bboxes_to_cull_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(gather_soldier_bboxes_to_cull_ecs_query_comps, "attaches_list", ecs::EidList)
            );

        }while (++comp != compE);
      }
  );
}
