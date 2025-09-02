// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entitySystem.h>

// asset viewer only pull vars
#define REG_SYS \
  RS(animchar)  \
  RS(toolInput) \
  RS(input)

#define RS(x) ECS_DECL_PULL_VAR(x);
REG_SYS
#undef RS

#define RS(x) +ECS_PULL_VAR(x)
// this dummy var required to actually pull static ctors from EntitySystem's objects that otherwise have no other publicly visible
// symbols
size_t tool_pull = 0 + REG_SYS;
#undef RS