#include "colorDlgAppMat.h"
#include <shaders/dag_shaders.h>

ColorDialogAppMat::ColorDialogAppMat(void *phandle, const char caption[], E3DCOLOR color) :
  ColorDialog(phandle, caption, color), applyMode(false)
{
  resizeWindow(710, 410);
  use_custom_color_paintVarId = ::get_shader_variable_id("use_custom_color_paint", true);
  custom_color_paintVarId = ::get_shader_variable_id("custom_color_paint", true);
  applyMode = ::ShaderGlobal::get_int(use_custom_color_paintVarId);
  getPanel()->createCheckBox(ID_CHECKBOX_MODE, "Apply color to entity mode", applyMode);
}

void ColorDialogAppMat::onChange(int pcb_id, PropPanel2 *panel)
{
  if (pcb_id == ID_CHECKBOX_MODE)
  {
    applyMode = panel->getBool(ID_CHECKBOX_MODE);
    ShaderGlobal::set_int(use_custom_color_paintVarId, (int)applyMode);
  }
  else
  {
    ColorDialog::onChange(pcb_id, panel);
  }
  ShaderGlobal::set_color4(custom_color_paintVarId, Color4(ColorDialog::getColor()));
}
