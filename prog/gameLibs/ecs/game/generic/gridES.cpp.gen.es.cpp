#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t grid_holder_get_type();
static ecs::LTComponentList grid_holder_component(ECS_HASH("grid_holder"), grid_holder_get_type(), "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl", "", 0);
static constexpr ecs::component_t net__scopeDistanceSq_get_type();
static ecs::LTComponentList net__scopeDistanceSq_component(ECS_HASH("net__scopeDistanceSq"), net__scopeDistanceSq_get_type(), "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "gridES.cpp.inl"
ECS_DEF_PULL_VAR(grid);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc grid_obj_update_main_with_animchar_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
//start of 4 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("grid_obj__fixedTmScale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
//start of 2 rq components at [5]
  {ECS_HASH("grid_obj__updateAlways"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("grid_obj__updateInMainThread"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void grid_obj_update_main_with_animchar_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    grid_obj_update_main_with_animchar_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(grid_obj_update_main_with_animchar_es_comps, "grid_obj", GridObjComponent)
    , ECS_RO_COMP(grid_obj_update_main_with_animchar_es_comps, "transform", TMatrix)
    , ECS_RO_COMP_PTR(grid_obj_update_main_with_animchar_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(grid_obj_update_main_with_animchar_es_comps, "collres", CollisionResource)
    , ECS_RO_COMP_OR(grid_obj_update_main_with_animchar_es_comps, "grid_obj__fixedTmScale", float(-1.f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_obj_update_main_with_animchar_es_es_desc
(
  "grid_obj_update_main_with_animchar_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(grid_obj_update_main_with_animchar_es_all),
  make_span(grid_obj_update_main_with_animchar_es_comps+0, 1)/*rw*/,
  make_span(grid_obj_update_main_with_animchar_es_comps+1, 4)/*ro*/,
  make_span(grid_obj_update_main_with_animchar_es_comps+5, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,nullptr,"after_animchar_update_sync");
static constexpr ecs::ComponentDesc grid_debug_draw_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static void grid_debug_draw_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RO_COMP(grid_debug_draw_es_comps, "camera__active", bool)) )
      continue;
    grid_debug_draw_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(grid_debug_draw_es_comps, "transform", TMatrix)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_debug_draw_es_es_desc
(
  "grid_debug_draw_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(grid_debug_draw_es_all),
  empty_span(),
  make_span(grid_debug_draw_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc grid_holder_destroyed_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("grid_holder"), ecs::ComponentTypeInfo<GridHolder>()},
  {ECS_HASH("grid_holder__typeHash"), ecs::ComponentTypeInfo<int>()}
};
static void grid_holder_destroyed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_holder_destroyed_es_event_handler(evt
        , ECS_RO_COMP(grid_holder_destroyed_es_event_handler_comps, "grid_holder", GridHolder)
    , ECS_RO_COMP(grid_holder_destroyed_es_event_handler_comps, "grid_holder__typeHash", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_holder_destroyed_es_event_handler_es_desc
(
  "grid_holder_destroyed_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_holder_destroyed_es_event_handler_all_events),
  empty_span(),
  make_span(grid_holder_destroyed_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc grid_obj_verify_server_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("grid_obj__gridType"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL}
};
static void grid_obj_verify_server_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_obj_verify_server_es(evt
        , ECS_RO_COMP(grid_obj_verify_server_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(grid_obj_verify_server_es_comps, "grid_obj", GridObjComponent)
    , ECS_RO_COMP_OR(grid_obj_verify_server_es_comps, "grid_obj__gridType", ecs::string("default"))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_obj_verify_server_es_es_desc
(
  "grid_obj_verify_server_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_obj_verify_server_es_all_events),
  make_span(grid_obj_verify_server_es_comps+0, 1)/*rw*/,
  make_span(grid_obj_verify_server_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"server");
static constexpr ecs::ComponentDesc grid_obj_init_tm_scale_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
  {ECS_HASH("grid_obj__fixedTmScale"), ecs::ComponentTypeInfo<float>()},
//start of 1 ro components at [2]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void grid_obj_init_tm_scale_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_obj_init_tm_scale_es_event_handler(evt
        , ECS_RW_COMP(grid_obj_init_tm_scale_es_event_handler_comps, "grid_obj", GridObjComponent)
    , ECS_RO_COMP(grid_obj_init_tm_scale_es_event_handler_comps, "transform", TMatrix)
    , ECS_RW_COMP(grid_obj_init_tm_scale_es_event_handler_comps, "grid_obj__fixedTmScale", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_obj_init_tm_scale_es_event_handler_es_desc
(
  "grid_obj_init_tm_scale_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_obj_init_tm_scale_es_event_handler_all_events),
  make_span(grid_obj_init_tm_scale_es_event_handler_comps+0, 2)/*rw*/,
  make_span(grid_obj_init_tm_scale_es_event_handler_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdUpdateGridScale,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc grid_obj_update_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
//start of 5 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("grid_obj__fixedTmScale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("grid_obj__scaledBoxBounding"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [6]
  {ECS_HASH("grid_obj__updateAlways"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void grid_obj_update_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_obj_update_es_event_handler(evt
        , ECS_RW_COMP(grid_obj_update_es_event_handler_comps, "grid_obj", GridObjComponent)
    , ECS_RO_COMP(grid_obj_update_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP_PTR(grid_obj_update_es_event_handler_comps, "collres", CollisionResource)
    , ECS_RO_COMP_PTR(grid_obj_update_es_event_handler_comps, "ri_extra", RiExtraComponent)
    , ECS_RO_COMP_OR(grid_obj_update_es_event_handler_comps, "grid_obj__fixedTmScale", float(-1.f))
    , ECS_RO_COMP_PTR(grid_obj_update_es_event_handler_comps, "grid_obj__scaledBoxBounding", ecs::Tag)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_obj_update_es_event_handler_es_desc
(
  "grid_obj_update_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_obj_update_es_event_handler_all_events),
  make_span(grid_obj_update_es_event_handler_comps+0, 1)/*rw*/,
  make_span(grid_obj_update_es_event_handler_comps+1, 5)/*ro*/,
  empty_span(),
  make_span(grid_obj_update_es_event_handler_comps+6, 1)/*no*/,
  ecs::EventSetBuilder<CmdUpdateGrid,
                       EventOnEntityTeleported,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"transform",nullptr,"animchar_act_on_phys_teleport_es");
static constexpr ecs::ComponentDesc grid_obj_update_with_animchar_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
//start of 4 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("grid_obj__fixedTmScale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [5]
  {ECS_HASH("grid_obj__updateAlways"), ecs::ComponentTypeInfo<ecs::Tag>()},
//start of 1 no components at [6]
  {ECS_HASH("grid_obj__updateInMainThread"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void grid_obj_update_with_animchar_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_obj_update_with_animchar_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RW_COMP(grid_obj_update_with_animchar_es_comps, "grid_obj", GridObjComponent)
    , ECS_RO_COMP(grid_obj_update_with_animchar_es_comps, "transform", TMatrix)
    , ECS_RO_COMP_PTR(grid_obj_update_with_animchar_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(grid_obj_update_with_animchar_es_comps, "collres", CollisionResource)
    , ECS_RO_COMP_OR(grid_obj_update_with_animchar_es_comps, "grid_obj__fixedTmScale", float(-1.f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_obj_update_with_animchar_es_es_desc
(
  "grid_obj_update_with_animchar_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_obj_update_with_animchar_es_all_events),
  make_span(grid_obj_update_with_animchar_es_comps+0, 1)/*rw*/,
  make_span(grid_obj_update_with_animchar_es_comps+1, 4)/*ro*/,
  make_span(grid_obj_update_with_animchar_es_comps+5, 1)/*rq*/,
  make_span(grid_obj_update_with_animchar_es_comps+6, 1)/*no*/,
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,nullptr,nullptr,nullptr,"after_animchar_update_sync");
static constexpr ecs::ComponentDesc grid_obj_update_with_animchar_evt_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
//start of 4 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("grid_obj__fixedTmScale"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [5]
  {ECS_HASH("grid_obj__updateAlways"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void grid_obj_update_with_animchar_evt_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_obj_update_with_animchar_evt_es(evt
        , ECS_RW_COMP(grid_obj_update_with_animchar_evt_es_comps, "grid_obj", GridObjComponent)
    , ECS_RO_COMP(grid_obj_update_with_animchar_evt_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(grid_obj_update_with_animchar_evt_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(grid_obj_update_with_animchar_evt_es_comps, "collres", CollisionResource)
    , ECS_RO_COMP_OR(grid_obj_update_with_animchar_evt_es_comps, "grid_obj__fixedTmScale", float(-1.f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_obj_update_with_animchar_evt_es_es_desc
(
  "grid_obj_update_with_animchar_evt_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_obj_update_with_animchar_evt_es_all_events),
  make_span(grid_obj_update_with_animchar_evt_es_comps+0, 1)/*rw*/,
  make_span(grid_obj_update_with_animchar_evt_es_comps+1, 4)/*ro*/,
  make_span(grid_obj_update_with_animchar_evt_es_comps+5, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventOnEntityTeleported>::build(),
  0
,nullptr,nullptr,nullptr,"animchar_act_on_phys_teleport_es");
static constexpr ecs::ComponentDesc grid_obj_disappear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
//start of 1 rq components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void grid_obj_disappear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_obj_disappear_es(evt
        , ECS_RW_COMP(grid_obj_disappear_es_comps, "grid_obj", GridObjComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_obj_disappear_es_es_desc
(
  "grid_obj_disappear_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_obj_disappear_es_all_events),
  make_span(grid_obj_disappear_es_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(grid_obj_disappear_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc grid_obj_disappear_animchar_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
//start of 3 rq components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void grid_obj_disappear_animchar_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_obj_disappear_animchar_es(evt
        , ECS_RW_COMP(grid_obj_disappear_animchar_es_comps, "grid_obj", GridObjComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_obj_disappear_animchar_es_es_desc
(
  "grid_obj_disappear_animchar_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_obj_disappear_animchar_es_all_events),
  make_span(grid_obj_disappear_animchar_es_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(grid_obj_disappear_animchar_es_comps+1, 3)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc grid_obj_hide_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("grid_obj__hidden"), ecs::ComponentTypeInfo<int>()},
//start of 1 rq components at [2]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void grid_obj_hide_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_obj_hide_es_event_handler(evt
        , ECS_RW_COMP(grid_obj_hide_es_event_handler_comps, "grid_obj", GridObjComponent)
    , ECS_RO_COMP(grid_obj_hide_es_event_handler_comps, "grid_obj__hidden", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_obj_hide_es_event_handler_es_desc
(
  "grid_obj_hide_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_obj_hide_es_event_handler_all_events),
  make_span(grid_obj_hide_es_event_handler_comps+0, 1)/*rw*/,
  make_span(grid_obj_hide_es_event_handler_comps+1, 1)/*ro*/,
  make_span(grid_obj_hide_es_event_handler_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"grid_obj__hidden");
static constexpr ecs::ComponentDesc grid_holder_created_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("grid_holder"), ecs::ComponentTypeInfo<GridHolder>()},
  {ECS_HASH("grid_holder__typeHash"), ecs::ComponentTypeInfo<int>()},
//start of 2 ro components at [2]
  {ECS_HASH("grid_holder__type"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("grid_holder__cellSize"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL}
};
static void grid_holder_created_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_holder_created_es(evt
        , ECS_RW_COMP(grid_holder_created_es_comps, "grid_holder", GridHolder)
    , ECS_RO_COMP(grid_holder_created_es_comps, "grid_holder__type", ecs::string)
    , ECS_RW_COMP(grid_holder_created_es_comps, "grid_holder__typeHash", int)
    , ECS_RO_COMP_OR(grid_holder_created_es_comps, "grid_holder__cellSize", int(SPATIAL_HASH_DEFAULT_CELL_SIZE))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_holder_created_es_es_desc
(
  "grid_holder_created_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_holder_created_es_all_events),
  make_span(grid_holder_created_es_comps+0, 2)/*rw*/,
  make_span(grid_holder_created_es_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc grid_holder_created_client_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_holder"), ecs::ComponentTypeInfo<GridHolder>()},
//start of 1 ro components at [1]
  {ECS_HASH("grid_holder__type"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void grid_holder_created_client_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grid_holder_created_client_es(evt
        , ECS_RW_COMP(grid_holder_created_client_es_comps, "grid_holder", GridHolder)
    , ECS_RO_COMP(grid_holder_created_client_es_comps, "grid_holder__type", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grid_holder_created_client_es_es_desc
(
  "grid_holder_created_client_es",
  "prog/gameLibs/ecs/game/generic/./gridES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grid_holder_created_client_es_all_events),
  make_span(grid_holder_created_client_es_comps+0, 1)/*rw*/,
  make_span(grid_holder_created_client_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"netClient");
static constexpr ecs::ComponentDesc all_grid_holders_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("grid_holder"), ecs::ComponentTypeInfo<GridHolder>()}
};
static ecs::CompileTimeQueryDesc all_grid_holders_ecs_query_desc
(
  "all_grid_holders_ecs_query",
  empty_span(),
  make_span(all_grid_holders_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void all_grid_holders_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, all_grid_holders_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(all_grid_holders_ecs_query_comps, "grid_holder", GridHolder)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc all_grid_objects_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()}
};
static ecs::CompileTimeQueryDesc all_grid_objects_ecs_query_desc
(
  "all_grid_objects_ecs_query",
  make_span(all_grid_objects_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void all_grid_objects_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, all_grid_objects_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(all_grid_objects_ecs_query_comps, "grid_obj", GridObjComponent)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc grid_object_assigned_grid_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("grid_obj__gridType"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc grid_object_assigned_grid_ecs_query_desc
(
  "grid_object_assigned_grid_ecs_query",
  make_span(grid_object_assigned_grid_ecs_query_comps+0, 1)/*rw*/,
  make_span(grid_object_assigned_grid_ecs_query_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void grid_object_assigned_grid_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, grid_object_assigned_grid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(grid_object_assigned_grid_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(grid_object_assigned_grid_ecs_query_comps, "grid_obj", GridObjComponent)
            , ECS_RO_COMP_OR(grid_object_assigned_grid_ecs_query_comps, "grid_obj__gridType", ecs::string("default"))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc all_grid_holders_with_type_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grid_holder"), ecs::ComponentTypeInfo<GridHolder>()},
//start of 1 ro components at [1]
  {ECS_HASH("grid_holder__type"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc all_grid_holders_with_type_ecs_query_desc
(
  "all_grid_holders_with_type_ecs_query",
  make_span(all_grid_holders_with_type_ecs_query_comps+0, 1)/*rw*/,
  make_span(all_grid_holders_with_type_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void all_grid_holders_with_type_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, all_grid_holders_with_type_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(all_grid_holders_with_type_ecs_query_comps, "grid_holder", GridHolder)
            , ECS_RO_COMP(all_grid_holders_with_type_ecs_query_comps, "grid_holder__type", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc all_doors_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("net__scopeDistanceSq"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [2]
  {ECS_HASH("isDoor"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc all_doors_ecs_query_desc
(
  "all_doors_ecs_query",
  empty_span(),
  make_span(all_doors_ecs_query_comps+0, 2)/*ro*/,
  make_span(all_doors_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void all_doors_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, all_doors_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(all_doors_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(all_doors_ecs_query_comps, "net__scopeDistanceSq", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t grid_holder_get_type(){return ecs::ComponentTypeInfo<GridHolder>::type; }
static constexpr ecs::component_t net__scopeDistanceSq_get_type(){return ecs::ComponentTypeInfo<float>::type; }
