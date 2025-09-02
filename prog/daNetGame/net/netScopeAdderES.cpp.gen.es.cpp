#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t replication_get_type();
static ecs::LTComponentList replication_component(ECS_HASH("replication"), replication_get_type(), "prog/daNetGame/net/netScopeAdderES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "netScopeAdderES.cpp.inl"
ECS_DEF_PULL_VAR(netScopeAdder);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::component_t replication_get_type(){return ecs::ComponentTypeInfo<net::Object>::type; }
