// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "colorDlgAppMat.h"
#include "../av_environment.h"

ColorDialogAppMat::ColorDialogAppMat(void *phandle, const char caption[]) :
  ColorDialog(phandle, caption, environment::getSinglePaintColor())
{
  getPanel()->createCheckBox(ID_CHECKBOX_MODE, "Apply color to entity mode", environment::isUsingSinglePaintColor());
}

void ColorDialogAppMat::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == ID_CHECKBOX_MODE)
  {
    environment::setUseSinglePaintColor(panel->getBool(ID_CHECKBOX_MODE));
  }
  else
  {
    ColorDialog::onChange(pcb_id, panel);
  }

  environment::setSinglePaintColor(ColorDialog::getColor());
  environment::updatePaintColorTexture();
}
