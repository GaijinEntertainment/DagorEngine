#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t animchar_attach__attachedTo_get_type();
static ecs::LTComponentList animchar_attach__attachedTo_component(ECS_HASH("animchar_attach__attachedTo"), animchar_attach__attachedTo_get_type(), "prog/daNetGame/net/netComponentFiltersES.cpp.inl", "", 0);
static constexpr ecs::component_t gun__owner_get_type();
static ecs::LTComponentList gun__owner_component(ECS_HASH("gun__owner"), gun__owner_get_type(), "prog/daNetGame/net/netComponentFiltersES.cpp.inl", "", 0);
static constexpr ecs::component_t human_anim__vehicleSelected_get_type();
static ecs::LTComponentList human_anim__vehicleSelected_component(ECS_HASH("human_anim__vehicleSelected"), human_anim__vehicleSelected_get_type(), "prog/daNetGame/net/netComponentFiltersES.cpp.inl", "", 0);
static constexpr ecs::component_t possessed_get_type();
static ecs::LTComponentList possessed_component(ECS_HASH("possessed"), possessed_get_type(), "prog/daNetGame/net/netComponentFiltersES.cpp.inl", "", 0);
static constexpr ecs::component_t possessedByPlr_get_type();
static ecs::LTComponentList possessedByPlr_component(ECS_HASH("possessedByPlr"), possessedByPlr_get_type(), "prog/daNetGame/net/netComponentFiltersES.cpp.inl", "", 0);
static constexpr ecs::component_t specTarget_get_type();
static ecs::LTComponentList specTarget_component(ECS_HASH("specTarget"), specTarget_get_type(), "prog/daNetGame/net/netComponentFiltersES.cpp.inl", "", 0);
static constexpr ecs::component_t squad_member__squad_get_type();
static ecs::LTComponentList squad_member__squad_component(ECS_HASH("squad_member__squad"), squad_member__squad_get_type(), "prog/daNetGame/net/netComponentFiltersES.cpp.inl", "", 0);
static constexpr ecs::component_t team_get_type();
static ecs::LTComponentList team_component(ECS_HASH("team"), team_get_type(), "prog/daNetGame/net/netComponentFiltersES.cpp.inl", "", 0);
static constexpr ecs::component_t turret__owner_get_type();
static ecs::LTComponentList turret__owner_component(ECS_HASH("turret__owner"), turret__owner_get_type(), "prog/daNetGame/net/netComponentFiltersES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "netComponentFiltersES.cpp.inl"
ECS_DEF_PULL_VAR(netComponentFilters);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::component_t animchar_attach__attachedTo_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t gun__owner_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t human_anim__vehicleSelected_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t possessed_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t possessedByPlr_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t specTarget_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t squad_member__squad_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t team_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t turret__owner_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
