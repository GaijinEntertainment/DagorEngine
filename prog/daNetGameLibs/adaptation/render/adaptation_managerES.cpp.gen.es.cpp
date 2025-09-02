// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "adaptation_managerES.cpp.inl"
ECS_DEF_PULL_VAR(adaptation_manager);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc adaptation_settings_tracking_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("render_settings__gpuResidentAdaptation"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__adaptation"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__fullDeferred"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__forwardRendering"), ecs::ComponentTypeInfo<bool>()}
};
static void adaptation_settings_tracking_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    adaptation_settings_tracking_es(evt
        , ECS_RO_COMP(adaptation_settings_tracking_es_comps, "render_settings__gpuResidentAdaptation", bool)
    , ECS_RO_COMP(adaptation_settings_tracking_es_comps, "render_settings__adaptation", bool)
    , ECS_RO_COMP(adaptation_settings_tracking_es_comps, "render_settings__fullDeferred", bool)
    , ECS_RO_COMP(adaptation_settings_tracking_es_comps, "render_settings__forwardRendering", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc adaptation_settings_tracking_es_es_desc
(
  "adaptation_settings_tracking_es",
  "prog/daNetGameLibs/adaptation/render/adaptation_managerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, adaptation_settings_tracking_es_all_events),
  empty_span(),
  make_span(adaptation_settings_tracking_es_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__adaptation,render_settings__forwardRendering,render_settings__fullDeferred,render_settings__gpuResidentAdaptation");
static constexpr ecs::ComponentDesc adaptation_level_settings_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("adaptation_level_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void adaptation_level_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  adaptation_level_settings_es(evt
        );
}
static ecs::EntitySystemDesc adaptation_level_settings_es_es_desc
(
  "adaptation_level_settings_es",
  "prog/daNetGameLibs/adaptation/render/adaptation_managerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, adaptation_level_settings_es_all_events),
  empty_span(),
  empty_span(),
  make_span(adaptation_level_settings_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","adaptation_level_settings");
static constexpr ecs::ComponentDesc adaptation_default_settings_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("adaptation_default_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void adaptation_default_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  adaptation_default_settings_es(evt
        );
}
static ecs::EntitySystemDesc adaptation_default_settings_es_es_desc
(
  "adaptation_default_settings_es",
  "prog/daNetGameLibs/adaptation/render/adaptation_managerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, adaptation_default_settings_es_all_events),
  empty_span(),
  empty_span(),
  make_span(adaptation_default_settings_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","adaptation_default_settings");
static constexpr ecs::ComponentDesc adaptation_override_settings_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("adaptation_override_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void adaptation_override_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  adaptation_override_settings_es(evt
        );
}
static ecs::EntitySystemDesc adaptation_override_settings_es_es_desc
(
  "adaptation_override_settings_es",
  "prog/daNetGameLibs/adaptation/render/adaptation_managerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, adaptation_override_settings_es_all_events),
  empty_span(),
  empty_span(),
  make_span(adaptation_override_settings_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","adaptation_override_settings");
static constexpr ecs::ComponentDesc adaptation_after_device_reset_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation__manager"), ecs::ComponentTypeInfo<AdaptationManager>()}
};
static void adaptation_after_device_reset_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    adaptation_after_device_reset_es(evt
        , ECS_RW_COMP(adaptation_after_device_reset_es_comps, "adaptation__manager", AdaptationManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc adaptation_after_device_reset_es_es_desc
(
  "adaptation_after_device_reset_es",
  "prog/daNetGameLibs/adaptation/render/adaptation_managerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, adaptation_after_device_reset_es_all_events),
  make_span(adaptation_after_device_reset_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc adaptation_update_time_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation__manager"), ecs::ComponentTypeInfo<AdaptationManager>()},
//start of 1 ro components at [1]
  {ECS_HASH("adaptation__track_changes"), ecs::ComponentTypeInfo<bool>()}
};
static void adaptation_update_time_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    adaptation_update_time_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(adaptation_update_time_es_comps, "adaptation__manager", AdaptationManager)
    , ECS_RO_COMP(adaptation_update_time_es_comps, "adaptation__track_changes", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc adaptation_update_time_es_es_desc
(
  "adaptation_update_time_es",
  "prog/daNetGameLibs/adaptation/render/adaptation_managerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, adaptation_update_time_es_all_events),
  make_span(adaptation_update_time_es_comps+0, 1)/*rw*/,
  make_span(adaptation_update_time_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc on_reset_exposure_evt_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation__manager"), ecs::ComponentTypeInfo<AdaptationManager>()}
};
static void on_reset_exposure_evt_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderReinitCube>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    on_reset_exposure_evt_es(static_cast<const RenderReinitCube&>(evt)
        , ECS_RW_COMP(on_reset_exposure_evt_es_comps, "adaptation__manager", AdaptationManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_reset_exposure_evt_es_es_desc
(
  "on_reset_exposure_evt_es",
  "prog/daNetGameLibs/adaptation/render/adaptation_managerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, on_reset_exposure_evt_es_all_events),
  make_span(on_reset_exposure_evt_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderReinitCube>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc on_set_no_exposure_evt_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation__manager"), ecs::ComponentTypeInfo<AdaptationManager>()}
};
static void on_set_no_exposure_evt_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderSetExposure>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    on_set_no_exposure_evt_es(static_cast<const RenderSetExposure&>(evt)
        , ECS_RW_COMP(on_set_no_exposure_evt_es_comps, "adaptation__manager", AdaptationManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_set_no_exposure_evt_es_es_desc
(
  "on_set_no_exposure_evt_es",
  "prog/daNetGameLibs/adaptation/render/adaptation_managerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, on_set_no_exposure_evt_es_all_events),
  make_span(on_set_no_exposure_evt_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderSetExposure>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_set_exposure_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation__exposure"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_set_exposure_ecs_query_desc
(
  "get_set_exposure_ecs_query",
  make_span(get_set_exposure_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_set_exposure_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_set_exposure_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_set_exposure_ecs_query_comps, "adaptation__exposure", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_adaptation_center_weight_override_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("adaptation__centerWeightOverride"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_adaptation_center_weight_override_ecs_query_desc
(
  "get_adaptation_center_weight_override_ecs_query",
  empty_span(),
  make_span(get_adaptation_center_weight_override_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_adaptation_center_weight_override_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_adaptation_center_weight_override_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_adaptation_center_weight_override_ecs_query_comps, "adaptation__centerWeightOverride", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc adaptation_node_init_ecs_query_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("adaptation__manager"), ecs::ComponentTypeInfo<AdaptationManager>()},
  {ECS_HASH("adaptation__update_readback_exposure_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("adaptation__create_histogram_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("adaptation__gen_histogram_forward_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("adaptation__gen_histogram_node"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()},
  {ECS_HASH("adaptation__accumulate_histogram"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("adaptation__adapt_exposure_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("adaptation__set_exposure_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static ecs::CompileTimeQueryDesc adaptation_node_init_ecs_query_desc
(
  "adaptation_node_init_ecs_query",
  make_span(adaptation_node_init_ecs_query_comps+0, 8)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void adaptation_node_init_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, adaptation_node_init_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(adaptation_node_init_ecs_query_comps, "adaptation__manager", AdaptationManager)
            , ECS_RW_COMP(adaptation_node_init_ecs_query_comps, "adaptation__update_readback_exposure_node", dafg::NodeHandle)
            , ECS_RW_COMP(adaptation_node_init_ecs_query_comps, "adaptation__create_histogram_node", dafg::NodeHandle)
            , ECS_RW_COMP(adaptation_node_init_ecs_query_comps, "adaptation__gen_histogram_forward_node", dafg::NodeHandle)
            , ECS_RW_COMP(adaptation_node_init_ecs_query_comps, "adaptation__gen_histogram_node", resource_slot::NodeHandleWithSlotsAccess)
            , ECS_RW_COMP(adaptation_node_init_ecs_query_comps, "adaptation__accumulate_histogram", dafg::NodeHandle)
            , ECS_RW_COMP(adaptation_node_init_ecs_query_comps, "adaptation__adapt_exposure_node", dafg::NodeHandle)
            , ECS_RW_COMP(adaptation_node_init_ecs_query_comps, "adaptation__set_exposure_node", dafg::NodeHandle)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_adaptation_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation__manager"), ecs::ComponentTypeInfo<AdaptationManager>()}
};
static ecs::CompileTimeQueryDesc get_adaptation_manager_ecs_query_desc
(
  "get_adaptation_manager_ecs_query",
  make_span(get_adaptation_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_adaptation_manager_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_adaptation_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(get_adaptation_manager_ecs_query_comps, "adaptation__manager", AdaptationManager)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_default_settings_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("adaptation_default_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc get_default_settings_ecs_query_desc
(
  "get_default_settings_ecs_query",
  empty_span(),
  make_span(get_default_settings_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_default_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_default_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_default_settings_ecs_query_comps, "adaptation_default_settings", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_level_settings_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("adaptation_level_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc get_level_settings_ecs_query_desc
(
  "get_level_settings_ecs_query",
  empty_span(),
  make_span(get_level_settings_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_level_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_level_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_level_settings_ecs_query_comps, "adaptation_level_settings", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_override_settings_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("adaptation_override_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc get_override_settings_ecs_query_desc
(
  "get_override_settings_ecs_query",
  empty_span(),
  make_span(get_override_settings_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_override_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_override_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_override_settings_ecs_query_comps, "adaptation_override_settings", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
