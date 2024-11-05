// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entitySystem.h>

#define REG_SYS RS(rumble)

#define REG_SQM RS(bind_dainput2_events)

#define RS(x) ECS_DECL_PULL_VAR(x);
REG_SYS
#undef RS
#define RS(x) extern const size_t sq_autobind_pull_##x;
REG_SQM
#undef RS

#define RS(x) +ECS_PULL_VAR(x)
// this var is required to actually pull static ctors from EntitySystem's objects that otherwise have no other publicly visible symbols
size_t framework_input_pulls = 0 REG_SYS
#undef RS
#define RS(x) +sq_autobind_pull_##x
  REG_SQM
#undef RS
  ;
