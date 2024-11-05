// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/autoBind.h>
#include <sqModules/sqModules.h>
#include "bhvMinimap.h"
#include "bhvMinimapInput.h"
#include "bhvMinimapCanvasPolygon.h"

static void bind_minimap_ui_behaviors(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

#define REG_BHV_DATA(name) Sqrat::Class<name, Sqrat::NoConstructor<name>> sq##name(vm, #name);
  REG_BHV_DATA(MinimapInputTouchData)
#undef REG_BHV_DATA

  Sqrat::Table tblBhv(vm);
  tblBhv //
    .SetValue("Minimap", (darg::Behavior *)&bhv_minimap)
    .SetValue("MinimapInput", (darg::Behavior *)&bhv_minimap_input)
    .SetValue("MinimapCanvasPolygon", (darg::Behavior *)&bhv_minimap_canvas_polygon)
    /**/;

  module_mgr->addNativeModule("minimap.behaviors", tblBhv);
}

SQ_DEF_AUTO_BINDING_REGFUNC_EX(bind_minimap_ui_behaviors, sq::VM_UI_ALL);

#include <daECS/core/componentType.h>
ECS_DEF_PULL_VAR(minimap_ui_bhv_sq);
