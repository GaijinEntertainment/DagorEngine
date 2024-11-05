// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <osApiWrappers/dag_wndProcComponent.h>


namespace darg
{

struct BhvBrowserData;

class BhvBrowser : public Behavior, public IWndProcComponent
{
public:
  BhvBrowser();
  virtual void onAttach(darg::Element *elem) override;
  virtual void onDetach(darg::Element *elem, DetachMode) override;
  virtual int update(UpdateStage, Element *elem, float dt) override;
  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent /*event*/, int /*pointer_id*/, int /*btn_idx*/,
    short /*mx*/, short /*my*/, int /*buttons*/, int /*accum_res*/) override;
  virtual IWndProcComponent::RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result) override;
};

extern BhvBrowser bhv_browser;

} // namespace darg
