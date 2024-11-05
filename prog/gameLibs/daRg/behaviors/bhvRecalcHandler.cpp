// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvRecalcHandler.h"
#include "scriptUtil.h"
#include "guiScene.h"
#include "dargDebugUtils.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>


namespace darg
{


BhvRecalcHandler bhv_recalc_handler;


BhvRecalcHandler::BhvRecalcHandler() : Behavior(0, 0) {}


void BhvRecalcHandler::onRecalcLayout(Element *elem)
{
  Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  HSQUIRRELVM vm = scriptDesc.GetVM();
  Sqrat::Function f(vm, scriptDesc, scriptDesc.RawGetSlot(elem->csk->onRecalcLayout));
  if (!f.IsNull())
  {
    SQInteger nparams = 0, nfreevars = 0;
    G_VERIFY(get_closure_info(f, &nparams, &nfreevars));

    if (nparams < 1 || nparams > 3)
    {
      darg_assert_trace_var("Expected 0, 1 or 2 params for recalc handler", elem->props.scriptDesc, elem->csk->onRecalcLayout);
      return;
    }

    GuiScene *scene = GuiScene::get_from_elem(elem);

    const char *key = "BhvRecalcHandler:initial";
    bool initial = elem->props.storage.RawGetSlotValue(key, true);

    if (nparams == 1)
      scene->queueScriptHandler(new ScriptHandlerSqFunc<>(f));
    else if (nparams == 2)
      scene->queueScriptHandler(new ScriptHandlerSqFunc<bool>(f, initial));
    else
      f(initial, elem->getRef(vm));

    elem->props.storage.SetValue(key, false);
  }
}


} // namespace darg
