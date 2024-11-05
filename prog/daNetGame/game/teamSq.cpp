// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "team.h"
#include <bindQuirrelEx/autoBind.h>

SQ_DEF_AUTO_BINDING_MODULE(bind_team, "team")
{
  Sqrat::Table tbl(vm);

#define _SET_VALUE(x) tbl.SetValue(#x, x)
  _SET_VALUE(TEAM_UNASSIGNED);
  _SET_VALUE(TEAM_SPECTATOR);
  _SET_VALUE(FIRST_GAME_TEAM);
#undef _SET_VALUE

  return tbl;
}
