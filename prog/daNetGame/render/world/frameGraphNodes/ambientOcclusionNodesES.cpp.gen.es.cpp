// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "ambientOcclusionNodesES.cpp.inl"
ECS_DEF_PULL_VAR(ambientOcclusionNodes);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc reset_ao_camera_nodes_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("dng_ao_camera_nodes__w"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__h"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__creation_flags"), ecs::ComponentTypeInfo<uint32_t>()},
  {ECS_HASH("dng_ao_camera_nodes__useGTAO"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [4]
  {ECS_HASH("dafg_camera_registrator__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void reset_ao_camera_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ResetAoNodes>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    reset_ao_camera_nodes_es(static_cast<const ResetAoNodes&>(evt)
        , ECS_RW_COMP(reset_ao_camera_nodes_es_comps, "dng_ao_camera_nodes__w", int)
    , ECS_RW_COMP(reset_ao_camera_nodes_es_comps, "dng_ao_camera_nodes__h", int)
    , ECS_RW_COMP(reset_ao_camera_nodes_es_comps, "dng_ao_camera_nodes__creation_flags", uint32_t)
    , ECS_RW_COMP(reset_ao_camera_nodes_es_comps, "dng_ao_camera_nodes__useGTAO", bool)
    , ECS_RO_COMP(reset_ao_camera_nodes_es_comps, "dafg_camera_registrator__name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc reset_ao_camera_nodes_es_es_desc
(
  "reset_ao_camera_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/ambientOcclusionNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_ao_camera_nodes_es_all_events),
  make_span(reset_ao_camera_nodes_es_comps+0, 4)/*rw*/,
  make_span(reset_ao_camera_nodes_es_comps+4, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ResetAoNodes>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc create_gtao_camera_main_view_nodes_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("dng_ao_camera_nodes__w"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__h"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__creation_flags"), ecs::ComponentTypeInfo<uint32_t>()},
  {ECS_HASH("dng_ao_camera_nodes__useGTAO"), ecs::ComponentTypeInfo<bool>()}
};
static void create_gtao_camera_main_view_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnCameraMainViewNodeConstruction>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_gtao_camera_main_view_nodes_es(static_cast<const OnCameraMainViewNodeConstruction&>(evt)
        , ECS_RO_COMP(create_gtao_camera_main_view_nodes_es_comps, "dng_ao_camera_nodes__w", int)
    , ECS_RO_COMP(create_gtao_camera_main_view_nodes_es_comps, "dng_ao_camera_nodes__h", int)
    , ECS_RO_COMP(create_gtao_camera_main_view_nodes_es_comps, "dng_ao_camera_nodes__creation_flags", uint32_t)
    , ECS_RO_COMP(create_gtao_camera_main_view_nodes_es_comps, "dng_ao_camera_nodes__useGTAO", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_gtao_camera_main_view_nodes_es_es_desc
(
  "create_gtao_camera_main_view_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/ambientOcclusionNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_gtao_camera_main_view_nodes_es_all_events),
  empty_span(),
  make_span(create_gtao_camera_main_view_nodes_es_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraMainViewNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc create_gtao_camera_view_nodes_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("dng_ao_camera_nodes__w"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__h"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__creation_flags"), ecs::ComponentTypeInfo<uint32_t>()},
  {ECS_HASH("dng_ao_camera_nodes__useGTAO"), ecs::ComponentTypeInfo<bool>()}
};
static void create_gtao_camera_view_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnCameraPerViewNodeConstruction>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_gtao_camera_view_nodes_es(static_cast<const OnCameraPerViewNodeConstruction&>(evt)
        , ECS_RO_COMP(create_gtao_camera_view_nodes_es_comps, "dng_ao_camera_nodes__w", int)
    , ECS_RO_COMP(create_gtao_camera_view_nodes_es_comps, "dng_ao_camera_nodes__h", int)
    , ECS_RO_COMP(create_gtao_camera_view_nodes_es_comps, "dng_ao_camera_nodes__creation_flags", uint32_t)
    , ECS_RO_COMP(create_gtao_camera_view_nodes_es_comps, "dng_ao_camera_nodes__useGTAO", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_gtao_camera_view_nodes_es_es_desc
(
  "create_gtao_camera_view_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/ambientOcclusionNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_gtao_camera_view_nodes_es_all_events),
  empty_span(),
  make_span(create_gtao_camera_view_nodes_es_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraPerViewNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc create_ssao_camera_main_view_nodes_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("dng_ao_camera_nodes__w"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__h"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__creation_flags"), ecs::ComponentTypeInfo<uint32_t>()},
  {ECS_HASH("dng_ao_camera_nodes__useGTAO"), ecs::ComponentTypeInfo<bool>()}
};
static void create_ssao_camera_main_view_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnCameraMainViewNodeConstruction>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_ssao_camera_main_view_nodes_es(static_cast<const OnCameraMainViewNodeConstruction&>(evt)
        , ECS_RO_COMP(create_ssao_camera_main_view_nodes_es_comps, "dng_ao_camera_nodes__w", int)
    , ECS_RO_COMP(create_ssao_camera_main_view_nodes_es_comps, "dng_ao_camera_nodes__h", int)
    , ECS_RO_COMP(create_ssao_camera_main_view_nodes_es_comps, "dng_ao_camera_nodes__creation_flags", uint32_t)
    , ECS_RO_COMP(create_ssao_camera_main_view_nodes_es_comps, "dng_ao_camera_nodes__useGTAO", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_ssao_camera_main_view_nodes_es_es_desc
(
  "create_ssao_camera_main_view_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/ambientOcclusionNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_ssao_camera_main_view_nodes_es_all_events),
  empty_span(),
  make_span(create_ssao_camera_main_view_nodes_es_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraMainViewNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc create_ssao_camera_view_nodes_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("dng_ao_camera_nodes__w"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__h"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("dng_ao_camera_nodes__useGTAO"), ecs::ComponentTypeInfo<bool>()}
};
static void create_ssao_camera_view_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnCameraPerViewNodeConstruction>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_ssao_camera_view_nodes_es(static_cast<const OnCameraPerViewNodeConstruction&>(evt)
        , ECS_RO_COMP(create_ssao_camera_view_nodes_es_comps, "dng_ao_camera_nodes__w", int)
    , ECS_RO_COMP(create_ssao_camera_view_nodes_es_comps, "dng_ao_camera_nodes__h", int)
    , ECS_RO_COMP(create_ssao_camera_view_nodes_es_comps, "dng_ao_camera_nodes__useGTAO", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_ssao_camera_view_nodes_es_es_desc
(
  "create_ssao_camera_view_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/ambientOcclusionNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_ssao_camera_view_nodes_es_all_events),
  empty_span(),
  make_span(create_ssao_camera_view_nodes_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraPerViewNodeConstruction>::build(),
  0
,"render");
