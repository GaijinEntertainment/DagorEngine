#include "customEnviProbeES.cpp.inl"
ECS_DEF_PULL_VAR(customEnviProbe);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc custom_envi_probe_created_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("custom_envi_probe__needs_render"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [1]
  {ECS_HASH("custom_envi_probe__cubemap"), ecs::ComponentTypeInfo<SharedTexHolder>()}
};
static void custom_envi_probe_created_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    custom_envi_probe_created_es_event_handler(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(custom_envi_probe_created_es_event_handler_comps, "custom_envi_probe__cubemap", SharedTexHolder)
    , ECS_RW_COMP(custom_envi_probe_created_es_event_handler_comps, "custom_envi_probe__needs_render", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc custom_envi_probe_created_es_event_handler_es_desc
(
  "custom_envi_probe_created_es",
  "prog/daNetGameLibs/custom_envi_probe/render/customEnviProbeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, custom_envi_probe_created_es_event_handler_all_events),
  make_span(custom_envi_probe_created_es_event_handler_comps+0, 1)/*rw*/,
  make_span(custom_envi_probe_created_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc custom_cube_texture_name_changed_es_event_handler_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("custom_envi_probe__cubemap"), ecs::ComponentTypeInfo<SharedTexHolder>()},
  {ECS_HASH("custom_envi_probe__needs_render"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("custom_envi_probe__spherical_harmonics_outside"), ecs::ComponentTypeInfo<ecs::Point4List>()},
  {ECS_HASH("custom_envi_probe__spherical_harmonics_inside"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 2 ro components at [4]
  {ECS_HASH("custom_envi_probe__cubemap_res"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("custom_envi_probe__cubemap_var"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void custom_cube_texture_name_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventComponentChanged>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    custom_cube_texture_name_changed_es_event_handler(static_cast<const ecs::EventComponentChanged&>(evt)
        , ECS_RW_COMP(custom_cube_texture_name_changed_es_event_handler_comps, "custom_envi_probe__cubemap", SharedTexHolder)
    , ECS_RO_COMP(custom_cube_texture_name_changed_es_event_handler_comps, "custom_envi_probe__cubemap_res", ecs::string)
    , ECS_RO_COMP(custom_cube_texture_name_changed_es_event_handler_comps, "custom_envi_probe__cubemap_var", ecs::string)
    , ECS_RW_COMP(custom_cube_texture_name_changed_es_event_handler_comps, "custom_envi_probe__needs_render", bool)
    , ECS_RW_COMP(custom_cube_texture_name_changed_es_event_handler_comps, "custom_envi_probe__spherical_harmonics_outside", ecs::Point4List)
    , ECS_RW_COMP(custom_cube_texture_name_changed_es_event_handler_comps, "custom_envi_probe__spherical_harmonics_inside", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc custom_cube_texture_name_changed_es_event_handler_es_desc
(
  "custom_cube_texture_name_changed_es",
  "prog/daNetGameLibs/custom_envi_probe/render/customEnviProbeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, custom_cube_texture_name_changed_es_event_handler_all_events),
  make_span(custom_cube_texture_name_changed_es_event_handler_comps+0, 4)/*rw*/,
  make_span(custom_cube_texture_name_changed_es_event_handler_comps+4, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventComponentChanged>::build(),
  0
,"dev,render","custom_envi_probe__cubemap_res");
static constexpr ecs::ComponentDesc custom_cube_texture_vars_changed_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("custom_envi_probe__needs_render"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("custom_envi_probe__spherical_harmonics_outside"), ecs::ComponentTypeInfo<ecs::Point4List>()},
  {ECS_HASH("custom_envi_probe__spherical_harmonics_inside"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 4 rq components at [3]
  {ECS_HASH("custom_envi_probe__inside_mul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("custom_envi_probe__outside_mul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("custom_envi_probe__x_rotation"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("custom_envi_probe__y_rotation"), ecs::ComponentTypeInfo<float>()}
};
static void custom_cube_texture_vars_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventComponentChanged>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    custom_cube_texture_vars_changed_es_event_handler(static_cast<const ecs::EventComponentChanged&>(evt)
        , ECS_RW_COMP(custom_cube_texture_vars_changed_es_event_handler_comps, "custom_envi_probe__needs_render", bool)
    , ECS_RW_COMP(custom_cube_texture_vars_changed_es_event_handler_comps, "custom_envi_probe__spherical_harmonics_outside", ecs::Point4List)
    , ECS_RW_COMP(custom_cube_texture_vars_changed_es_event_handler_comps, "custom_envi_probe__spherical_harmonics_inside", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc custom_cube_texture_vars_changed_es_event_handler_es_desc
(
  "custom_cube_texture_vars_changed_es",
  "prog/daNetGameLibs/custom_envi_probe/render/customEnviProbeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, custom_cube_texture_vars_changed_es_event_handler_all_events),
  make_span(custom_cube_texture_vars_changed_es_event_handler_comps+0, 3)/*rw*/,
  empty_span(),
  make_span(custom_cube_texture_vars_changed_es_event_handler_comps+3, 4)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventComponentChanged>::build(),
  0
,"dev,render","custom_envi_probe__inside_mul,custom_envi_probe__outside_mul,custom_envi_probe__x_rotation,custom_envi_probe__y_rotation");
static constexpr ecs::ComponentDesc custom_cube_texture_before_render_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("custom_envi_probe__needs_render"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("custom_envi_probe__is_inside"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [2]
  {ECS_HASH("custom_envi_probe__cubemap"), ecs::ComponentTypeInfo<SharedTexHolder>()}
};
static void custom_cube_texture_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    custom_cube_texture_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(custom_cube_texture_before_render_es_comps, "custom_envi_probe__needs_render", bool)
    , ECS_RW_COMP(custom_cube_texture_before_render_es_comps, "custom_envi_probe__is_inside", bool)
    , ECS_RO_COMP(custom_cube_texture_before_render_es_comps, "custom_envi_probe__cubemap", SharedTexHolder)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc custom_cube_texture_before_render_es_es_desc
(
  "custom_cube_texture_before_render_es",
  "prog/daNetGameLibs/custom_envi_probe/render/customEnviProbeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, custom_cube_texture_before_render_es_all_events),
  make_span(custom_cube_texture_before_render_es_comps+0, 2)/*rw*/,
  make_span(custom_cube_texture_before_render_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc custom_envi_probe_after_reset_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("custom_envi_probe__needs_render"), ecs::ComponentTypeInfo<bool>()}
};
static void custom_envi_probe_after_reset_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<AfterDeviceReset>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    custom_envi_probe_after_reset_es(static_cast<const AfterDeviceReset&>(evt)
        , ECS_RW_COMP(custom_envi_probe_after_reset_es_comps, "custom_envi_probe__needs_render", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc custom_envi_probe_after_reset_es_es_desc
(
  "custom_envi_probe_after_reset_es",
  "prog/daNetGameLibs/custom_envi_probe/render/customEnviProbeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, custom_envi_probe_after_reset_es_all_events),
  make_span(custom_envi_probe_after_reset_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
);
static constexpr ecs::ComponentDesc custom_envi_probe_render_es_event_handler_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("custom_envi_probe__postfx"), ecs::ComponentTypeInfo<PostFxRenderer>()},
  {ECS_HASH("custom_envi_probe__is_inside"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("custom_envi_probe__x_rotation"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("custom_envi_probe__y_rotation"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("custom_envi_probe__outside_mul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("custom_envi_probe__inside_mul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("custom_envi_probe__cubemap"), ecs::ComponentTypeInfo<SharedTexHolder>()}
};
static void custom_envi_probe_render_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CustomEnviProbeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    custom_envi_probe_render_es_event_handler(static_cast<const CustomEnviProbeRender&>(evt)
        , ECS_RO_COMP(custom_envi_probe_render_es_event_handler_comps, "custom_envi_probe__postfx", PostFxRenderer)
    , ECS_RO_COMP(custom_envi_probe_render_es_event_handler_comps, "custom_envi_probe__is_inside", bool)
    , ECS_RO_COMP(custom_envi_probe_render_es_event_handler_comps, "custom_envi_probe__x_rotation", float)
    , ECS_RO_COMP(custom_envi_probe_render_es_event_handler_comps, "custom_envi_probe__y_rotation", float)
    , ECS_RO_COMP(custom_envi_probe_render_es_event_handler_comps, "custom_envi_probe__outside_mul", float)
    , ECS_RO_COMP(custom_envi_probe_render_es_event_handler_comps, "custom_envi_probe__inside_mul", float)
    , ECS_RO_COMP(custom_envi_probe_render_es_event_handler_comps, "custom_envi_probe__cubemap", SharedTexHolder)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc custom_envi_probe_render_es_event_handler_es_desc
(
  "custom_envi_probe_render_es",
  "prog/daNetGameLibs/custom_envi_probe/render/customEnviProbeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, custom_envi_probe_render_es_event_handler_all_events),
  empty_span(),
  make_span(custom_envi_probe_render_es_event_handler_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CustomEnviProbeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc custom_envi_probe_get_spherical_harmonics_es_event_handler_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("custom_envi_probe__spherical_harmonics_outside"), ecs::ComponentTypeInfo<ecs::Point4List>()},
  {ECS_HASH("custom_envi_probe__spherical_harmonics_inside"), ecs::ComponentTypeInfo<ecs::Point4List>()},
  {ECS_HASH("custom_envi_probe__is_inside"), ecs::ComponentTypeInfo<bool>()}
};
static void custom_envi_probe_get_spherical_harmonics_es_event_handler_all_events(ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CustomEnviProbeGetSphericalHarmonics>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    custom_envi_probe_get_spherical_harmonics_es_event_handler(static_cast<CustomEnviProbeGetSphericalHarmonics&>(evt)
        , ECS_RO_COMP(custom_envi_probe_get_spherical_harmonics_es_event_handler_comps, "custom_envi_probe__spherical_harmonics_outside", ecs::Point4List)
    , ECS_RO_COMP(custom_envi_probe_get_spherical_harmonics_es_event_handler_comps, "custom_envi_probe__spherical_harmonics_inside", ecs::Point4List)
    , ECS_RO_COMP(custom_envi_probe_get_spherical_harmonics_es_event_handler_comps, "custom_envi_probe__is_inside", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc custom_envi_probe_get_spherical_harmonics_es_event_handler_es_desc
(
  "custom_envi_probe_get_spherical_harmonics_es",
  "prog/daNetGameLibs/custom_envi_probe/render/customEnviProbeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, custom_envi_probe_get_spherical_harmonics_es_event_handler_all_events),
  empty_span(),
  make_span(custom_envi_probe_get_spherical_harmonics_es_event_handler_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CustomEnviProbeGetSphericalHarmonics>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc custom_envi_probe_log_spherical_harmonics_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("custom_envi_probe__spherical_harmonics_outside"), ecs::ComponentTypeInfo<ecs::Point4List>()},
  {ECS_HASH("custom_envi_probe__spherical_harmonics_inside"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 1 ro components at [2]
  {ECS_HASH("custom_envi_probe__is_inside"), ecs::ComponentTypeInfo<bool>()}
};
static void custom_envi_probe_log_spherical_harmonics_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CustomEnviProbeLogSphericalHarmonics>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    custom_envi_probe_log_spherical_harmonics_es_event_handler(static_cast<const CustomEnviProbeLogSphericalHarmonics&>(evt)
        , ECS_RW_COMP(custom_envi_probe_log_spherical_harmonics_es_event_handler_comps, "custom_envi_probe__spherical_harmonics_outside", ecs::Point4List)
    , ECS_RW_COMP(custom_envi_probe_log_spherical_harmonics_es_event_handler_comps, "custom_envi_probe__spherical_harmonics_inside", ecs::Point4List)
    , ECS_RO_COMP(custom_envi_probe_log_spherical_harmonics_es_event_handler_comps, "custom_envi_probe__is_inside", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc custom_envi_probe_log_spherical_harmonics_es_event_handler_es_desc
(
  "custom_envi_probe_log_spherical_harmonics_es",
  "prog/daNetGameLibs/custom_envi_probe/render/customEnviProbeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, custom_envi_probe_log_spherical_harmonics_es_event_handler_all_events),
  make_span(custom_envi_probe_log_spherical_harmonics_es_event_handler_comps+0, 2)/*rw*/,
  make_span(custom_envi_probe_log_spherical_harmonics_es_event_handler_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CustomEnviProbeLogSphericalHarmonics>::build(),
  0
,"dev,render");
