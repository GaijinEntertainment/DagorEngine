#include "motionBlurES.cpp.inl"
ECS_DEF_PULL_VAR(motionBlur);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc motion_blur_update_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("motion_blur"), ecs::ComponentTypeInfo<MotionBlur>()}
};
static void motion_blur_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    motion_blur_update_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(motion_blur_update_es_comps, "motion_blur", MotionBlur)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc motion_blur_update_es_es_desc
(
  "motion_blur_update_es",
  "prog/daNetGame/render/motionBlurES.cpp.inl",
  ecs::EntitySystemOps(nullptr, motion_blur_update_es_all_events),
  make_span(motion_blur_update_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc motion_blur_settings_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("motion_blur"), ecs::ComponentTypeInfo<MotionBlur>()},
//start of 6 ro components at [1]
  {ECS_HASH("motion_blur__scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_blur__velocityMultiplier"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_blur__maxVelocity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_blur__alphaMulOnApply"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_blur__overscanMul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("motion_blur__forwardBlur"), ecs::ComponentTypeInfo<float>()}
};
static void motion_blur_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    motion_blur_settings_es(evt
        , ECS_RW_COMP(motion_blur_settings_es_comps, "motion_blur", MotionBlur)
    , ECS_RO_COMP(motion_blur_settings_es_comps, "motion_blur__scale", float)
    , ECS_RO_COMP(motion_blur_settings_es_comps, "motion_blur__velocityMultiplier", float)
    , ECS_RO_COMP(motion_blur_settings_es_comps, "motion_blur__maxVelocity", float)
    , ECS_RO_COMP(motion_blur_settings_es_comps, "motion_blur__alphaMulOnApply", float)
    , ECS_RO_COMP(motion_blur_settings_es_comps, "motion_blur__overscanMul", float)
    , ECS_RO_COMP(motion_blur_settings_es_comps, "motion_blur__forwardBlur", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc motion_blur_settings_es_es_desc
(
  "motion_blur_settings_es",
  "prog/daNetGame/render/motionBlurES.cpp.inl",
  ecs::EntitySystemOps(nullptr, motion_blur_settings_es_all_events),
  make_span(motion_blur_settings_es_comps+0, 1)/*rw*/,
  make_span(motion_blur_settings_es_comps+1, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"motion_blur__alphaMulOnApply,motion_blur__forwardBlur,motion_blur__maxVelocity,motion_blur__overscanMul,motion_blur__scale,motion_blur__velocityMultiplier");
static constexpr ecs::ComponentDesc motion_blur_destroyed_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("motion_blur"), ecs::ComponentTypeInfo<MotionBlur>()}
};
static void motion_blur_destroyed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  motion_blur_destroyed_es(evt
        );
}
static ecs::EntitySystemDesc motion_blur_destroyed_es_es_desc
(
  "motion_blur_destroyed_es",
  "prog/daNetGame/render/motionBlurES.cpp.inl",
  ecs::EntitySystemOps(nullptr, motion_blur_destroyed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(motion_blur_destroyed_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc query_motion_blur_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("motion_blur"), ecs::ComponentTypeInfo<MotionBlur>()},
//start of 1 ro components at [1]
  {ECS_HASH("motion_blur__mode"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc query_motion_blur_ecs_query_desc
(
  "query_motion_blur_ecs_query",
  make_span(query_motion_blur_ecs_query_comps+0, 1)/*rw*/,
  make_span(query_motion_blur_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_motion_blur_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_motion_blur_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(query_motion_blur_ecs_query_comps, "motion_blur", MotionBlur)
            , ECS_RO_COMP(query_motion_blur_ecs_query_comps, "motion_blur__mode", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc query_motion_blur_scale_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("motion_blur__scale"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [1]
  {ECS_HASH("motion_blur"), ecs::ComponentTypeInfo<MotionBlur>()}
};
static ecs::CompileTimeQueryDesc query_motion_blur_scale_ecs_query_desc
(
  "query_motion_blur_scale_ecs_query",
  empty_span(),
  make_span(query_motion_blur_scale_ecs_query_comps+0, 1)/*ro*/,
  make_span(query_motion_blur_scale_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void query_motion_blur_scale_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_motion_blur_scale_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(query_motion_blur_scale_ecs_query_comps, "motion_blur__scale", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc set_motion_blur_mode_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("motion_blur__mode"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc set_motion_blur_mode_ecs_query_desc
(
  "set_motion_blur_mode_ecs_query",
  make_span(set_motion_blur_mode_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_motion_blur_mode_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, set_motion_blur_mode_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_motion_blur_mode_ecs_query_comps, "motion_blur__mode", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc delete_motion_blur_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("motion_blur"), ecs::ComponentTypeInfo<MotionBlur>()}
};
static ecs::CompileTimeQueryDesc delete_motion_blur_ecs_query_desc
(
  "delete_motion_blur_ecs_query",
  empty_span(),
  make_span(delete_motion_blur_ecs_query_comps+0, 1)/*ro*/,
  make_span(delete_motion_blur_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void delete_motion_blur_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, delete_motion_blur_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(delete_motion_blur_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
