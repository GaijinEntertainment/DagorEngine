#include "animCharRenderES.cpp.inl"
ECS_DEF_PULL_VAR(animCharRender);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc animchar_before_render_es_comps[] ={};
static void animchar_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  animchar_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        );
}
static ecs::EntitySystemDesc animchar_before_render_es_es_desc
(
  "animchar_before_render_es",
  "prog/daNetGame/render/world/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_before_render_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc animchar_render_opaque_async_es_comps[] ={};
static void animchar_render_opaque_async_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<AnimcharRenderAsyncEvent>());
  animchar_render_opaque_async_es(static_cast<const AnimcharRenderAsyncEvent&>(evt)
        );
}
static ecs::EntitySystemDesc animchar_render_opaque_async_es_es_desc
(
  "animchar_render_opaque_async_es",
  "prog/daNetGame/render/world/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_opaque_async_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AnimcharRenderAsyncEvent>::build(),
  0
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc animchar_render_opaque_es_comps[] ={};
static void animchar_render_opaque_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoRender>());
  animchar_render_opaque_es(static_cast<const UpdateStageInfoRender&>(evt)
        );
}
static ecs::EntitySystemDesc animchar_render_opaque_es_es_desc
(
  "animchar_render_opaque_es",
  "prog/daNetGame/render/world/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_opaque_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRender>::build(),
  0
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc animchar_vehicle_cockpit_render_depth_prepass_es_comps[] ={};
static void animchar_vehicle_cockpit_render_depth_prepass_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<VehicleCockpitPrepass>());
  animchar_vehicle_cockpit_render_depth_prepass_es(static_cast<const VehicleCockpitPrepass&>(evt)
        );
}
static ecs::EntitySystemDesc animchar_vehicle_cockpit_render_depth_prepass_es_es_desc
(
  "animchar_vehicle_cockpit_render_depth_prepass_es",
  "prog/daNetGame/render/world/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_vehicle_cockpit_render_depth_prepass_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<VehicleCockpitPrepass>::build(),
  0
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc animchar_render_hmap_deform_es_comps[] ={};
static void animchar_render_hmap_deform_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RenderHmapDeform>());
  animchar_render_hmap_deform_es(static_cast<const RenderHmapDeform&>(evt)
        );
}
static ecs::EntitySystemDesc animchar_render_hmap_deform_es_es_desc
(
  "animchar_render_hmap_deform_es",
  "prog/daNetGame/render/world/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_hmap_deform_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderHmapDeform>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc animchar_render_trans_init_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_contains_trans"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 4 no components at [4]
  {ECS_HASH("semi_transparent__placingColor"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("invisibleUpdatableAnimchar"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("late_transparent_render"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("requires_trans_render"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void animchar_render_trans_init_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_render_trans_init_es_event_handler(evt
        , ECS_RO_COMP(animchar_render_trans_init_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(animchar_render_trans_init_es_event_handler_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(animchar_render_trans_init_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP_PTR(animchar_render_trans_init_es_event_handler_comps, "animchar_contains_trans", ecs::Tag)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_render_trans_init_es_event_handler_es_desc
(
  "animchar_render_trans_init_es",
  "prog/daNetGame/render/world/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_trans_init_es_event_handler_all_events),
  make_span(animchar_render_trans_init_es_event_handler_comps+0, 1)/*rw*/,
  make_span(animchar_render_trans_init_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  make_span(animchar_render_trans_init_es_event_handler_comps+4, 4)/*no*/,
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
//static constexpr ecs::ComponentDesc animchar_render_trans_es_comps[] ={};
static void animchar_render_trans_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoRenderTrans>());
  animchar_render_trans_es(static_cast<const UpdateStageInfoRenderTrans&>(evt)
        );
}
static ecs::EntitySystemDesc animchar_render_trans_es_es_desc
(
  "animchar_render_trans_es",
  "prog/daNetGame/render/world/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_trans_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRenderTrans>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc animchar_render_distortion_es_comps[] ={};
static void animchar_render_distortion_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoRenderDistortion>());
  animchar_render_distortion_es(static_cast<const UpdateStageInfoRenderDistortion&>(evt)
        );
}
static ecs::EntitySystemDesc animchar_render_distortion_es_es_desc
(
  "animchar_render_distortion_es",
  "prog/daNetGame/render/world/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_distortion_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRenderDistortion>::build(),
  0
,"render",nullptr,"*");
//static constexpr ecs::ComponentDesc close_animchar_bindpose_buffer_es_comps[] ={};
static void close_animchar_bindpose_buffer_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnGameShutdown>());
  close_animchar_bindpose_buffer_es(static_cast<const EventOnGameShutdown&>(evt)
        );
}
static ecs::EntitySystemDesc close_animchar_bindpose_buffer_es_es_desc
(
  "close_animchar_bindpose_buffer_es",
  "prog/daNetGame/render/world/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, close_animchar_bindpose_buffer_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameShutdown>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc animchar_render_objects_prepare_ecs_query_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>()},
  {ECS_HASH("animchar_attaches_bbox"), ecs::ComponentTypeInfo<bbox3f>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_shadow_cull_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("animchar_render__shadow_cast_dist"), ecs::ComponentTypeInfo<float>()},
//start of 12 ro components at [8]
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>()},
  {ECS_HASH("animchar_render__dist_sq"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("animchar_attaches_bbox_precalculated"), ecs::ComponentTypeInfo<bbox3f>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_bsph_precalculated"), ecs::ComponentTypeInfo<vec4f>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_bbox_precalculated"), ecs::ComponentTypeInfo<BBox3>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_shadow_cull_bbox_precalculated"), ecs::ComponentTypeInfo<BBox3>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__usePrecalculatedData"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__actOnDemand"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__updatable"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_extra_culling_dist"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__use_precise_shadow_culling"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc animchar_render_objects_prepare_ecs_query_desc
(
  "animchar_render_objects_prepare_ecs_query",
  make_span(animchar_render_objects_prepare_ecs_query_comps+0, 8)/*rw*/,
  make_span(animchar_render_objects_prepare_ecs_query_comps+8, 12)/*ro*/,
  empty_span(),
  empty_span()
  , 4);
template<typename Callable>
inline void animchar_render_objects_prepare_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_render_objects_prepare_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_render_objects_prepare_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(animchar_render_objects_prepare_ecs_query_comps, "animchar_node_wtm", AnimcharNodesMat44)
            , ECS_RO_COMP(animchar_render_objects_prepare_ecs_query_comps, "animchar_render__dist_sq", float)
            , ECS_RW_COMP(animchar_render_objects_prepare_ecs_query_comps, "animchar_render__root_pos", vec3f)
            , ECS_RW_COMP_PTR(animchar_render_objects_prepare_ecs_query_comps, "animchar_attaches_bbox", bbox3f)
            , ECS_RO_COMP_PTR(animchar_render_objects_prepare_ecs_query_comps, "animchar_attaches_bbox_precalculated", bbox3f)
            , ECS_RW_COMP(animchar_render_objects_prepare_ecs_query_comps, "animchar_bsph", vec4f)
            , ECS_RO_COMP_PTR(animchar_render_objects_prepare_ecs_query_comps, "animchar_bsph_precalculated", vec4f)
            , ECS_RO_COMP_PTR(animchar_render_objects_prepare_ecs_query_comps, "animchar_bbox_precalculated", BBox3)
            , ECS_RO_COMP_PTR(animchar_render_objects_prepare_ecs_query_comps, "animchar_shadow_cull_bbox_precalculated", BBox3)
            , ECS_RW_COMP(animchar_render_objects_prepare_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RW_COMP(animchar_render_objects_prepare_ecs_query_comps, "animchar_shadow_cull_bbox", bbox3f)
            , ECS_RW_COMP(animchar_render_objects_prepare_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RW_COMP(animchar_render_objects_prepare_ecs_query_comps, "animchar_render__shadow_cast_dist", float)
            , ECS_RO_COMP_OR(animchar_render_objects_prepare_ecs_query_comps, "animchar__usePrecalculatedData", bool(false))
            , ECS_RO_COMP_OR(animchar_render_objects_prepare_ecs_query_comps, "animchar_render__enabled", bool(true))
            , ECS_RO_COMP_PTR(animchar_render_objects_prepare_ecs_query_comps, "animchar__actOnDemand", ecs::Tag)
            , ECS_RO_COMP_OR(animchar_render_objects_prepare_ecs_query_comps, "animchar__updatable", bool(true))
            , ECS_RO_COMP_OR(animchar_render_objects_prepare_ecs_query_comps, "animchar_extra_culling_dist", float(100))
            , ECS_RO_COMP_OR(animchar_render_objects_prepare_ecs_query_comps, "animchar__use_precise_shadow_culling", bool(false))
            );

        }while (++comp != compE);
    }
    , nullptr, animchar_render_objects_prepare_ecs_query_desc.getQuant());
}
static constexpr ecs::ComponentDesc animchar_process_objects_in_shadow_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 3 ro components at [3]
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>()},
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc animchar_process_objects_in_shadow_ecs_query_desc
(
  "animchar_process_objects_in_shadow_ecs_query",
  make_span(animchar_process_objects_in_shadow_ecs_query_comps+0, 3)/*rw*/,
  make_span(animchar_process_objects_in_shadow_ecs_query_comps+3, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void animchar_process_objects_in_shadow_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_process_objects_in_shadow_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_process_objects_in_shadow_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(animchar_process_objects_in_shadow_ecs_query_comps, "animchar_node_wtm", AnimcharNodesMat44)
            , ECS_RW_COMP(animchar_process_objects_in_shadow_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP(animchar_process_objects_in_shadow_ecs_query_comps, "animchar_bsph", vec4f)
            , ECS_RW_COMP(animchar_process_objects_in_shadow_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_OR(animchar_process_objects_in_shadow_ecs_query_comps, "animchar_render__enabled", bool(true))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc update_animchar_hmap_deform_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 5 ro components at [2]
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar__actOnDemand"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("slot_attach"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__updatable"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 2 no components at [7]
  {ECS_HASH("semi_transparent__placingColor"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("invisibleUpdatableAnimchar"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc update_animchar_hmap_deform_ecs_query_desc
(
  "update_animchar_hmap_deform_ecs_query",
  make_span(update_animchar_hmap_deform_ecs_query_comps+0, 2)/*rw*/,
  make_span(update_animchar_hmap_deform_ecs_query_comps+2, 5)/*ro*/,
  empty_span(),
  make_span(update_animchar_hmap_deform_ecs_query_comps+7, 2)/*no*/);
template<typename Callable>
inline void update_animchar_hmap_deform_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, update_animchar_hmap_deform_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(update_animchar_hmap_deform_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(update_animchar_hmap_deform_ecs_query_comps, "animchar_node_wtm", AnimcharNodesMat44)
            , ECS_RO_COMP(update_animchar_hmap_deform_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RW_COMP(update_animchar_hmap_deform_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(update_animchar_hmap_deform_ecs_query_comps, "animchar__actOnDemand", ecs::Tag)
            , ECS_RO_COMP_PTR(update_animchar_hmap_deform_ecs_query_comps, "slot_attach", ecs::Tag)
            , ECS_RO_COMP_OR(update_animchar_hmap_deform_ecs_query_comps, "animchar__updatable", bool(true))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc animchar_csm_distance_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("animchar_render__shadow_cast_dist"), ecs::ComponentTypeInfo<float>()},
//start of 5 ro components at [2]
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_shadow_cull_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [7]
  {ECS_HASH("cockpitEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc animchar_csm_distance_ecs_query_desc
(
  "animchar_csm_distance_ecs_query",
  make_span(animchar_csm_distance_ecs_query_comps+0, 2)/*rw*/,
  make_span(animchar_csm_distance_ecs_query_comps+2, 5)/*ro*/,
  empty_span(),
  make_span(animchar_csm_distance_ecs_query_comps+7, 1)/*no*/
  , 4);
template<typename Callable>
inline void animchar_csm_distance_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_csm_distance_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_csm_distance_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP(animchar_csm_distance_ecs_query_comps, "animchar_bsph", vec4f)
            , ECS_RO_COMP(animchar_csm_distance_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP(animchar_csm_distance_ecs_query_comps, "animchar_shadow_cull_bbox", bbox3f)
            , ECS_RW_COMP(animchar_csm_distance_ecs_query_comps, "animchar_render__shadow_cast_dist", float)
            , ECS_RO_COMP_PTR(animchar_csm_distance_ecs_query_comps, "attaches_list", ecs::EidList)
            , ECS_RO_COMP_OR(animchar_csm_distance_ecs_query_comps, "slot_attach__attachedTo", ecs::EntityId(ecs::INVALID_ENTITY_ID))
            );

        }while (++comp != compE);
    }
    , nullptr, animchar_csm_distance_ecs_query_desc.getQuant());
}
static constexpr ecs::ComponentDesc animchar_csm_visibility_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 5 ro components at [1]
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_attaches_bbox"), ecs::ComponentTypeInfo<bbox3f>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__shadow_cast_dist"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>(), ecs::CDF_OPTIONAL},
//start of 2 no components at [6]
  {ECS_HASH("attachedToParent"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("cockpitEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc animchar_csm_visibility_ecs_query_desc
(
  "animchar_csm_visibility_ecs_query",
  make_span(animchar_csm_visibility_ecs_query_comps+0, 1)/*rw*/,
  make_span(animchar_csm_visibility_ecs_query_comps+1, 5)/*ro*/,
  empty_span(),
  make_span(animchar_csm_visibility_ecs_query_comps+6, 2)/*no*/
  , 50);
template<typename Callable>
inline void animchar_csm_visibility_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_csm_visibility_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_csm_visibility_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP(animchar_csm_visibility_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP_PTR(animchar_csm_visibility_ecs_query_comps, "animchar_attaches_bbox", bbox3f)
            , ECS_RO_COMP(animchar_csm_visibility_ecs_query_comps, "animchar_render__shadow_cast_dist", float)
            , ECS_RO_COMP_PTR(animchar_csm_visibility_ecs_query_comps, "attaches_list", ecs::EidList)
            , ECS_RO_COMP_PTR(animchar_csm_visibility_ecs_query_comps, "slot_attach__attachedTo", ecs::EntityId)
            );

        }while (++comp != compE);
    }
    , nullptr, animchar_csm_visibility_ecs_query_desc.getQuant());
}
static constexpr ecs::ComponentDesc animchar_attaches_inherit_visibility_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()}
};
static ecs::CompileTimeQueryDesc animchar_attaches_inherit_visibility_ecs_query_desc
(
  "animchar_attaches_inherit_visibility_ecs_query",
  make_span(animchar_attaches_inherit_visibility_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void animchar_attaches_inherit_visibility_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, animchar_attaches_inherit_visibility_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(animchar_attaches_inherit_visibility_ecs_query_comps, "animchar_visbits", uint8_t)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc draw_shadow_occlusion_boxes_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_render__shadow_cast_dist"), ecs::ComponentTypeInfo<float>()},
//start of 3 ro components at [2]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("animchar_attaches_bbox"), ecs::ComponentTypeInfo<bbox3f>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("slot_attach__attachedTo"), ecs::ComponentTypeInfo<ecs::EntityId>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc draw_shadow_occlusion_boxes_ecs_query_desc
(
  "draw_shadow_occlusion_boxes_ecs_query",
  make_span(draw_shadow_occlusion_boxes_ecs_query_comps+0, 2)/*rw*/,
  make_span(draw_shadow_occlusion_boxes_ecs_query_comps+2, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void draw_shadow_occlusion_boxes_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, draw_shadow_occlusion_boxes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(draw_shadow_occlusion_boxes_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RW_COMP(draw_shadow_occlusion_boxes_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RW_COMP(draw_shadow_occlusion_boxes_ecs_query_comps, "animchar_render__shadow_cast_dist", float)
            , ECS_RO_COMP_PTR(draw_shadow_occlusion_boxes_ecs_query_comps, "animchar_attaches_bbox", bbox3f)
            , ECS_RO_COMP_PTR(draw_shadow_occlusion_boxes_ecs_query_comps, "slot_attach__attachedTo", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_animchar_async_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 10 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("invisibleUpdatableAnimchar"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_attaches_bbox"), ecs::ComponentTypeInfo<bbox3f>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__nodeVisibleStgFilters"), ecs::ComponentTypeInfo<ecs::UInt8List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__renderPriority"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("needImmediateConstPS"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [11]
  {ECS_HASH("semi_transparent__placingColor"), ecs::ComponentTypeInfo<Point3>()}
};
static ecs::CompileTimeQueryDesc gather_animchar_async_ecs_query_desc
(
  "gather_animchar_async_ecs_query",
  make_span(gather_animchar_async_ecs_query_comps+0, 1)/*rw*/,
  make_span(gather_animchar_async_ecs_query_comps+1, 10)/*ro*/,
  empty_span(),
  make_span(gather_animchar_async_ecs_query_comps+11, 1)/*no*/);
template<typename Callable>
inline void gather_animchar_async_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_animchar_async_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_animchar_async_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP_PTR(gather_animchar_async_ecs_query_comps, "invisibleUpdatableAnimchar", ecs::Tag)
            , ECS_RW_COMP(gather_animchar_async_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP(gather_animchar_async_ecs_query_comps, "animchar_bsph", vec4f)
            , ECS_RO_COMP(gather_animchar_async_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP_PTR(gather_animchar_async_ecs_query_comps, "animchar_attaches_bbox", bbox3f)
            , ECS_RO_COMP(gather_animchar_async_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP_PTR(gather_animchar_async_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_PTR(gather_animchar_async_ecs_query_comps, "animchar_render__nodeVisibleStgFilters", ecs::UInt8List)
            , ECS_RO_COMP_OR(gather_animchar_async_ecs_query_comps, "animchar__renderPriority", bool(false))
            , ECS_RO_COMP_OR(gather_animchar_async_ecs_query_comps, "needImmediateConstPS", bool(false))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_animchar_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 9 ro components at [1]
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_attaches_bbox"), ecs::ComponentTypeInfo<bbox3f>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_render__nodeVisibleStgFilters"), ecs::ComponentTypeInfo<ecs::UInt8List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("invisibleUpdatableAnimchar"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar__renderPriority"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("needImmediateConstPS"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [10]
  {ECS_HASH("semi_transparent__placingColor"), ecs::ComponentTypeInfo<Point3>()}
};
static ecs::CompileTimeQueryDesc gather_animchar_ecs_query_desc
(
  "gather_animchar_ecs_query",
  make_span(gather_animchar_ecs_query_comps+0, 1)/*rw*/,
  make_span(gather_animchar_ecs_query_comps+1, 9)/*ro*/,
  empty_span(),
  make_span(gather_animchar_ecs_query_comps+10, 1)/*no*/);
template<typename Callable>
inline void gather_animchar_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_animchar_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(gather_animchar_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP(gather_animchar_ecs_query_comps, "animchar_bsph", vec4f)
            , ECS_RO_COMP(gather_animchar_ecs_query_comps, "animchar_bbox", bbox3f)
            , ECS_RO_COMP_PTR(gather_animchar_ecs_query_comps, "animchar_attaches_bbox", bbox3f)
            , ECS_RO_COMP(gather_animchar_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP_PTR(gather_animchar_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_PTR(gather_animchar_ecs_query_comps, "animchar_render__nodeVisibleStgFilters", ecs::UInt8List)
            , ECS_RO_COMP_PTR(gather_animchar_ecs_query_comps, "invisibleUpdatableAnimchar", ecs::Tag)
            , ECS_RO_COMP_OR(gather_animchar_ecs_query_comps, "animchar__renderPriority", bool(false))
            , ECS_RO_COMP_OR(gather_animchar_ecs_query_comps, "needImmediateConstPS", bool(false))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_animchar_vehicle_cockpit_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("needImmediateConstPS"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 2 rq components at [4]
  {ECS_HASH("cockpit__vehicleEid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("cockpitEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_animchar_vehicle_cockpit_ecs_query_desc
(
  "gather_animchar_vehicle_cockpit_ecs_query",
  empty_span(),
  make_span(gather_animchar_vehicle_cockpit_ecs_query_comps+0, 4)/*ro*/,
  make_span(gather_animchar_vehicle_cockpit_ecs_query_comps+4, 2)/*rq*/,
  empty_span());
template<typename Callable>
inline void gather_animchar_vehicle_cockpit_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_animchar_vehicle_cockpit_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_animchar_vehicle_cockpit_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP_PTR(gather_animchar_vehicle_cockpit_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP(gather_animchar_vehicle_cockpit_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_OR(gather_animchar_vehicle_cockpit_ecs_query_comps, "needImmediateConstPS", bool(false))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc render_animchar_hmap_deform_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 4 ro components at [1]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("forced_lod_for_hmap_deform"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("vehicle_trails__nodesFilter"), ecs::ComponentTypeInfo<ecs::UInt8List>(), ecs::CDF_OPTIONAL},
//start of 2 no components at [5]
  {ECS_HASH("semi_transparent__placingColor"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("invisibleUpdatableAnimchar"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc render_animchar_hmap_deform_ecs_query_desc
(
  "render_animchar_hmap_deform_ecs_query",
  make_span(render_animchar_hmap_deform_ecs_query_comps+0, 1)/*rw*/,
  make_span(render_animchar_hmap_deform_ecs_query_comps+1, 4)/*ro*/,
  empty_span(),
  make_span(render_animchar_hmap_deform_ecs_query_comps+5, 2)/*no*/);
template<typename Callable>
inline void render_animchar_hmap_deform_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, render_animchar_hmap_deform_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(render_animchar_hmap_deform_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(render_animchar_hmap_deform_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(render_animchar_hmap_deform_ecs_query_comps, "additional_data", ecs::Point4List)
            , ECS_RO_COMP_PTR(render_animchar_hmap_deform_ecs_query_comps, "forced_lod_for_hmap_deform", int)
            , ECS_RO_COMP_PTR(render_animchar_hmap_deform_ecs_query_comps, "vehicle_trails__nodesFilter", ecs::UInt8List)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc animchar_render_trans_first_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("animchar_render__nodeVisibleStgFilters"), ecs::ComponentTypeInfo<ecs::UInt8List>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [3]
  {ECS_HASH("requires_trans_render"), ecs::ComponentTypeInfo<ecs::Tag>()},
//start of 1 no components at [4]
  {ECS_HASH("late_transparent_render"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc animchar_render_trans_first_ecs_query_desc
(
  "animchar_render_trans_first_ecs_query",
  make_span(animchar_render_trans_first_ecs_query_comps+0, 1)/*rw*/,
  make_span(animchar_render_trans_first_ecs_query_comps+1, 2)/*ro*/,
  make_span(animchar_render_trans_first_ecs_query_comps+3, 1)/*rq*/,
  make_span(animchar_render_trans_first_ecs_query_comps+4, 1)/*no*/);
template<typename Callable>
inline void animchar_render_trans_first_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_render_trans_first_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_render_trans_first_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(animchar_render_trans_first_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(animchar_render_trans_first_ecs_query_comps, "animchar_render__nodeVisibleStgFilters", ecs::UInt8List)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc animchar_render_distortion_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("animchar_render__nodeVisibleStgFilters"), ecs::ComponentTypeInfo<ecs::UInt8List>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [3]
  {ECS_HASH("requires_distortion_render"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc animchar_render_distortion_ecs_query_desc
(
  "animchar_render_distortion_ecs_query",
  make_span(animchar_render_distortion_ecs_query_comps+0, 1)/*rw*/,
  make_span(animchar_render_distortion_ecs_query_comps+1, 2)/*ro*/,
  make_span(animchar_render_distortion_ecs_query_comps+3, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void animchar_render_distortion_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_render_distortion_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_render_distortion_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(animchar_render_distortion_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(animchar_render_distortion_ecs_query_comps, "animchar_render__nodeVisibleStgFilters", ecs::UInt8List)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc animchar_render_find_any_visible_distortion_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 1 rq components at [1]
  {ECS_HASH("requires_distortion_render"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc animchar_render_find_any_visible_distortion_ecs_query_desc
(
  "animchar_render_find_any_visible_distortion_ecs_query",
  empty_span(),
  make_span(animchar_render_find_any_visible_distortion_ecs_query_comps+0, 1)/*ro*/,
  make_span(animchar_render_find_any_visible_distortion_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult animchar_render_find_any_visible_distortion_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, animchar_render_find_any_visible_distortion_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(animchar_render_find_any_visible_distortion_ecs_query_comps, "animchar_visbits", uint8_t)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::ComponentDesc animchar_render_trans_second_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("animchar_render__nodeVisibleStgFilters"), ecs::ComponentTypeInfo<ecs::UInt8List>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [3]
  {ECS_HASH("late_transparent_render"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc animchar_render_trans_second_ecs_query_desc
(
  "animchar_render_trans_second_ecs_query",
  make_span(animchar_render_trans_second_ecs_query_comps+0, 1)/*rw*/,
  make_span(animchar_render_trans_second_ecs_query_comps+1, 2)/*ro*/,
  make_span(animchar_render_trans_second_ecs_query_comps+3, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void animchar_render_trans_second_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_render_trans_second_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(animchar_render_trans_second_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(animchar_render_trans_second_ecs_query_comps, "animchar_visbits", uint8_t)
            , ECS_RO_COMP_PTR(animchar_render_trans_second_ecs_query_comps, "animchar_render__nodeVisibleStgFilters", ecs::UInt8List)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc count_animchar_renderer_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("animchar__res"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()}
};
static ecs::CompileTimeQueryDesc count_animchar_renderer_ecs_query_desc
(
  "count_animchar_renderer_ecs_query",
  empty_span(),
  make_span(count_animchar_renderer_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void count_animchar_renderer_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, count_animchar_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(count_animchar_renderer_ecs_query_comps, "animchar__res", ecs::string)
            , ECS_RO_COMP(count_animchar_renderer_ecs_query_comps, "animchar_visbits", uint8_t)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_animchar_renderer_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar__res"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc gather_animchar_renderer_ecs_query_desc
(
  "gather_animchar_renderer_ecs_query",
  empty_span(),
  make_span(gather_animchar_renderer_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_animchar_renderer_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_animchar_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_animchar_renderer_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(gather_animchar_renderer_ecs_query_comps, "animchar__res", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
