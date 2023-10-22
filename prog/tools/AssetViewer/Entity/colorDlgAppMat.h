#pragma once

#include <propPanel2/comWnd/color_dialog.h>
#include <propPanel2/c_panel_base.h>
#include <shaders/dag_shMaterialUtils.h>


class ColorDialogAppMat : public ColorDialog
{
public:
  ColorDialogAppMat(void *phandle, const char caption[]);
  void onChange(int pcb_id, PropPanel2 *panel) override;

private:
  enum
  {
    ID_CHECKBOX_MODE = 200,
  };
};