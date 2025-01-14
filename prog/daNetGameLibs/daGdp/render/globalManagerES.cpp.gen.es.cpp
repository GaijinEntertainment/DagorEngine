#include "globalManagerES.cpp.inl"
ECS_DEF_PULL_VAR(globalManager);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc dagdp_update_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static void dagdp_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_update_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(dagdp_update_es_comps, "dagdp__global_manager", dagdp::GlobalManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_update_es_es_desc
(
  "dagdp_update_es",
  "prog/daNetGameLibs/daGdp/render/globalManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_update_es_all_events),
  make_span(dagdp_update_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dagdp_after_device_reset_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static void dagdp_after_device_reset_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<AfterDeviceReset>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_after_device_reset_es(static_cast<const AfterDeviceReset&>(evt)
        , ECS_RW_COMP(dagdp_after_device_reset_es_comps, "dagdp__global_manager", dagdp::GlobalManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_after_device_reset_es_es_desc
(
  "dagdp_after_device_reset_es",
  "prog/daNetGameLibs/daGdp/render/globalManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_after_device_reset_es_all_events),
  make_span(dagdp_after_device_reset_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dagdp_on_render_settings_change_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static void dagdp_on_render_settings_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnRenderSettingsReady>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_on_render_settings_change_es(static_cast<const OnRenderSettingsReady&>(evt)
        , ECS_RW_COMP(dagdp_on_render_settings_change_es_comps, "dagdp__global_manager", dagdp::GlobalManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_on_render_settings_change_es_es_desc
(
  "dagdp_on_render_settings_change_es",
  "prog/daNetGameLibs/daGdp/render/globalManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_on_render_settings_change_es_all_events),
  make_span(dagdp_on_render_settings_change_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dagdp_on_render_features_change_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static void dagdp_on_render_features_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ChangeRenderFeatures>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_on_render_features_change_es(static_cast<const ChangeRenderFeatures&>(evt)
        , ECS_RW_COMP(dagdp_on_render_features_change_es_comps, "dagdp__global_manager", dagdp::GlobalManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_on_render_features_change_es_es_desc
(
  "dagdp_on_render_features_change_es",
  "prog/daNetGameLibs/daGdp/render/globalManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_on_render_features_change_es_all_events),
  make_span(dagdp_on_render_features_change_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dagdp_on_shader_reload_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static void dagdp_on_shader_reload_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<AfterShaderReload>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_on_shader_reload_es(static_cast<const AfterShaderReload&>(evt)
        , ECS_RW_COMP(dagdp_on_shader_reload_es_comps, "dagdp__global_manager", dagdp::GlobalManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_on_shader_reload_es_es_desc
(
  "dagdp_on_shader_reload_es",
  "prog/daNetGameLibs/daGdp/render/globalManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_on_shader_reload_es_all_events),
  make_span(dagdp_on_shader_reload_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterShaderReload>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dagdp_on_level_unload_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static void dagdp_on_level_unload_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UnloadLevel>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_on_level_unload_es(static_cast<const UnloadLevel&>(evt)
        , ECS_RW_COMP(dagdp_on_level_unload_es_comps, "dagdp__global_manager", dagdp::GlobalManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_on_level_unload_es_es_desc
(
  "dagdp_on_level_unload_es",
  "prog/daNetGameLibs/daGdp/render/globalManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_on_level_unload_es_all_events),
  make_span(dagdp_on_level_unload_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UnloadLevel>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dagdp_placer_changed_es_comps[] =
{
//start of 3 rq components at [0]
  {ECS_HASH("dagdp__object_groups"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("dagdp_placer"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dagdp_placer_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dagdp::dagdp_placer_changed_es(evt
        );
}
static ecs::EntitySystemDesc dagdp_placer_changed_es_es_desc
(
  "dagdp_placer_changed_es",
  "prog/daNetGameLibs/daGdp/render/globalManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_placer_changed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(dagdp_placer_changed_es_comps+0, 3)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","dagdp__name,dagdp__object_groups");
static constexpr ecs::ComponentDesc dagdp_level_settings_changed_es_comps[] =
{
//start of 3 rq components at [0]
  {ECS_HASH("dagdp__use_heightmap_dynamic_objects"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("dagdp_level_settings"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("dagdp__max_objects"), ecs::ComponentTypeInfo<int>()}
};
static void dagdp_level_settings_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dagdp::dagdp_level_settings_changed_es(evt
        );
}
static ecs::EntitySystemDesc dagdp_level_settings_changed_es_es_desc
(
  "dagdp_level_settings_changed_es",
  "prog/daNetGameLibs/daGdp/render/globalManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_level_settings_changed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(dagdp_level_settings_changed_es_comps+0, 3)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","dagdp__max_objects,dagdp__use_heightmap_dynamic_objects");
static constexpr ecs::ComponentDesc level_settings_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("dagdp__max_objects"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dagdp__use_heightmap_dynamic_objects"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("dagdp_level_settings"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc level_settings_ecs_query_desc
(
  "dagdp::level_settings_ecs_query",
  empty_span(),
  make_span(level_settings_ecs_query_comps+0, 2)/*ro*/,
  make_span(level_settings_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::level_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, level_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(level_settings_ecs_query_comps, "dagdp__max_objects", int)
            , ECS_RO_COMP(level_settings_ecs_query_comps, "dagdp__use_heightmap_dynamic_objects", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc object_groups_named_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 rq components at [2]
  {ECS_HASH("dagdp_object_group"), ecs::ComponentTypeInfo<ecs::Tag>()},
//start of 1 no components at [3]
  {ECS_HASH("dagdp_placer"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc object_groups_named_ecs_query_desc
(
  "dagdp::object_groups_named_ecs_query",
  empty_span(),
  make_span(object_groups_named_ecs_query_comps+0, 2)/*ro*/,
  make_span(object_groups_named_ecs_query_comps+2, 1)/*rq*/,
  make_span(object_groups_named_ecs_query_comps+3, 1)/*no*/);
template<typename Callable>
inline void dagdp::object_groups_named_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, object_groups_named_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(object_groups_named_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(object_groups_named_ecs_query_comps, "dagdp__name", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc object_groups_nameless_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 2 rq components at [1]
  {ECS_HASH("dagdp_object_group"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("dagdp_placer"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc object_groups_nameless_ecs_query_desc
(
  "dagdp::object_groups_nameless_ecs_query",
  empty_span(),
  make_span(object_groups_nameless_ecs_query_comps+0, 1)/*ro*/,
  make_span(object_groups_nameless_ecs_query_comps+1, 2)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::object_groups_nameless_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, object_groups_nameless_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(object_groups_nameless_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc placers_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dagdp__object_groups"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 rq components at [3]
  {ECS_HASH("dagdp_placer"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc placers_ecs_query_desc
(
  "dagdp::placers_ecs_query",
  empty_span(),
  make_span(placers_ecs_query_comps+0, 3)/*ro*/,
  make_span(placers_ecs_query_comps+3, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::placers_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, placers_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(placers_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(placers_ecs_query_comps, "dagdp__object_groups", ecs::StringList)
            , ECS_RO_COMP(placers_ecs_query_comps, "dagdp__name", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static ecs::CompileTimeQueryDesc manager_ecs_query_desc
(
  "dagdp::manager_ecs_query",
  make_span(manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::manager_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(manager_ecs_query_comps, "dagdp__global_manager", dagdp::GlobalManager)
            );

        }while (++comp != compE);
    }
  );
}
