// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvUiStateControl.h"

#include "ui/uiShared.h"

#include <daRg/dag_element.h>
#include <osApiWrappers/dag_atomic.h>


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvUiStateControl, bhv_ui_state_control, cstr);

volatile int BhvUiStateControl::overrideFreeCam = 0;


struct BhvUiStateControlData
{
  bool overrideFreeCam = false;
};


BhvUiStateControl::BhvUiStateControl() : Behavior(0, 0) {}


void BhvUiStateControl::onAttach(darg::Element *elem)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  BhvUiStateControlData *data = new BhvUiStateControlData();
  data->overrideFreeCam = elem->props.getBool(strings->overrideFreeCam);
  elem->props.storage.SetValue(strings->uiStateControlData, data);

  if (data->overrideFreeCam)
  {
    interlocked_increment(overrideFreeCam);
  }
}


void BhvUiStateControl::onDetach(darg::Element *elem, DetachMode)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  BhvUiStateControlData *data = elem->props.storage.RawGetSlotValue<BhvUiStateControlData *>(strings->uiStateControlData, nullptr);
  if (data)
  {
    elem->props.storage.DeleteSlot(strings->uiStateControlData);
    if (data->overrideFreeCam)
    {
      interlocked_decrement(overrideFreeCam);
    }
    delete data;
  }
}
