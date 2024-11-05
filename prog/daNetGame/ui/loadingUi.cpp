// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "loadingUi.h"

#include <sqModules/sqModules.h>

namespace loading_ui
{

static bool fully_covering = false;

bool is_fully_covering() { return fully_covering; }
void set_fully_covering(bool o) { fully_covering = o; }

void bind(SqModules *moduleMgr)
{
  Sqrat::Table aTable(moduleMgr->getVM());
  aTable //
    .Func("is_fully_covering", is_fully_covering)
    .Func("set_fully_covering", set_fully_covering)
    /**/;
  moduleMgr->addNativeModule("loading_ui", aTable);
}

} // namespace loading_ui