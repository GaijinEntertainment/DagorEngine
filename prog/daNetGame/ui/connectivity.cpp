// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "connectivity.h"
#include <bindQuirrelEx/autoBind.h>

/*
ATTENTION!
this is bad code as it couples everything needed in deferrent games in one

*/
SQ_DEF_AUTO_BINDING_MODULE_EX(connectivity, "connectivity", sq::VM_UI_ALL)
{
  using namespace ui;
  Sqrat::Table tbl(vm);

#define _SET_VALUE(x) tbl.SetValue(#x, x)
  _SET_VALUE(CONNECTIVITY_OK);
  _SET_VALUE(CONNECTIVITY_NO_PACKETS);
  _SET_VALUE(CONNECTIVITY_OUTDATED_AAS);

#undef _SET_VALUE

  return tbl;
}
