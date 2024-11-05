// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


struct BhvUiStateControlData;

class BhvUiStateControl : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings, cstr, overrideFreeCam, uiStateControlData);

  BhvUiStateControl();
  virtual void onAttach(darg::Element *) override;
  virtual void onDetach(darg::Element *, DetachMode) override;

  static volatile int overrideFreeCam;
};


extern BhvUiStateControl bhv_ui_state_control;
