// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/colorDialog.h>
#include <propPanel/control/container.h>
#include <shaders/dag_shMaterialUtils.h>


class ColorDialogAppMat : public PropPanel::ColorDialog
{
public:
  ColorDialogAppMat(void *phandle, const char caption[]);
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  enum
  {
    ID_CHECKBOX_MODE = 200,
  };
};