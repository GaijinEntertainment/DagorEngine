// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvActivateActionSet.h"

#include "ui/uiShared.h"

#include <daRg/dag_element.h>


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvActivateActionSet, bhv_activate_action_set, cstr);


BhvActivateActionSet::BhvActivateActionSet() : Behavior(0, 0) {}


void BhvActivateActionSet::onAttach(darg::Element *elem)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  Sqrat::Object strObj = elem->props.getObject(strings->actionSet);
  if (strObj.GetType() == OT_STRING)
  {
    Sqrat::Var<const char *> strVar = strObj.GetVar<const char *>();
    dainput::action_set_handle_t ash = dainput::get_action_set_handle(strVar.value);
    if (ash != dainput::BAD_ACTION_SET_HANDLE)
    {
      elem->props.storage.SetValue(strings->actionSet, ash);
      uishared::activate_ui_elem_action_set(ash, true);
    }
  }
}


void BhvActivateActionSet::onDetach(darg::Element *elem, DetachMode)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  dainput::action_set_handle_t ash = elem->props.storage.RawGetSlotValue(strings->actionSet, dainput::BAD_ACTION_SET_HANDLE);
  if (ash != dainput::BAD_ACTION_SET_HANDLE)
  {
    elem->props.storage.RawDeleteSlot(strings->actionSet);
    uishared::activate_ui_elem_action_set(ash, false);
  }
}
