// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvBrowser.h"

#include <daRg/dag_element.h>

#include <startup/dag_inpDevClsDrv.h>
#include <osApiWrappers/dag_wndProcComponent.h>

namespace darg
{

BhvBrowser bhv_browser;

BhvBrowser::BhvBrowser() : Behavior(STAGE_BEFORE_RENDER, F_HANDLE_KEYBOARD | F_HANDLE_MOUSE | F_FOCUS_ON_CLICK | F_CAN_HANDLE_CLICKS)
{}

void BhvBrowser::onAttach(darg::Element *) {}
void BhvBrowser::onDetach(darg::Element *, DetachMode) {}
int BhvBrowser::update(UpdateStage, Element *, float) { return 0; }
int BhvBrowser::mouseEvent(ElementTree *, Element *, InputDevice, InputEvent, int /*pointer_id*/, int /*btn_idx*/, short /*mx*/,
  short /*my*/, int /*buttons*/, int /*accum_res*/)
{
  return 0;
}

IWndProcComponent::RetCode BhvBrowser::process(void *, unsigned, uintptr_t, intptr_t, intptr_t &)
{
  return IWndProcComponent::RetCode::PROCEED_OTHER_COMPONENTS;
}

} // namespace darg
