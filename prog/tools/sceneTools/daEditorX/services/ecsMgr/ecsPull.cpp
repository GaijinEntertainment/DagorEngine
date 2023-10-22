#include <ecs/core/entitySystem.h>

extern size_t tool_pull;

#define REG_SYS  \
  RS(tonemap)    \
  RS(tonemapUpd) \
  RS(instantiateDependencies)

#define RS(x) ECS_DECL_PULL_VAR(x);
REG_SYS
#undef RS

#define RS(x) +ECS_PULL_VAR(x)
// this dummy var required to actually pull static ctors from EntitySystem's objects that otherwise have no other publicly visible
// symbols
size_t ecs_manager_dummy_ecs_sum = tool_pull + REG_SYS;
#undef RS
