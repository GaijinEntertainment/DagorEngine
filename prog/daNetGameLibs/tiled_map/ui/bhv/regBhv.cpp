// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTiledMap.h"
#include "bhvTiledMapInput.h"
#include <bindQuirrelEx/autoBind.h>
#include <sqmodules/sqmodules.h>

static void bind_tiled_map_ui_behaviors(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

#define REG_BHV_DATA(name) Sqrat::Class<name, Sqrat::NoConstructor<name>> sq##name(vm, #name);
  REG_BHV_DATA(TiledMapInputTouchData)
#undef REG_BHV_DATA

  Sqrat::Table tblBhv(vm);
  tblBhv.SetValue("TiledMap", (darg::Behavior *)&bhv_tiled_map);
  tblBhv.SetValue("TiledMapInput", (darg::Behavior *)&bhv_tiled_map_input);

  module_mgr->addNativeModule("tiledMap.behaviors", tblBhv);
}


SQ_DEF_AUTO_BINDING_REGFUNC_EX(bind_tiled_map_ui_behaviors, sq::VM_UI_ALL);

#include <daECS/core/componentType.h>
ECS_DEF_PULL_VAR(tiled_map_ui_bhv_sq);
