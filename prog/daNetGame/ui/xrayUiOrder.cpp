// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "xrayUiOrder.h"
#include <sqModules/sqModules.h>

namespace ui
{
namespace xray_ui_order
{
static bool xray_before_ui = false;

bool should_render_xray_before_ui() { return xray_before_ui; }
static SQInteger set_xray_before_ui(HSQUIRRELVM vm)
{
  SQBool xrayBeforeUi;
  sq_getbool(vm, SQInteger(2), &xrayBeforeUi);
  xray_before_ui = xrayBeforeUi;
  return 1;
}
void bind_script(SqModules *moduleMgr)
{
  Sqrat::Table aTable(moduleMgr->getVM());
  aTable.SquirrelFunc("set_xray_before_ui", set_xray_before_ui, 2);
  moduleMgr->addNativeModule("xray_ui_order", aTable);
}

} // namespace xray_ui_order
} // namespace ui