//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/customControl.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point3.h>
#include <util/dag_simpleString.h>

#include <EASTL/unique_ptr.h>

namespace PropPanel
{

class GradientPlotControl;
class PaletteCellControl;
class TwoColorIndicatorControl;

const int PALETTE_COLOR_NUM = 32;

struct PaletteEntry
{
  PaletteEntry()
  {
    color = E3DCOLOR(255, 255, 255, 0);
    name = "";
  }

  PaletteEntry(E3DCOLOR _color, const char *_name)
  {
    color = _color;
    name = _name;
  }

  E3DCOLOR color;
  SimpleString name;
};


class CPPalette
{
public:
  CPPalette();

  void load(void *handle);
  void save(void *handle);

  const char *getName(int index) const { return mPalette[index].name.str(); }
  E3DCOLOR getColor(int index) const { return mPalette[index].color; }

  int getCurrentId() { return mCurId; }
  void setCurrentId(int _id) { mCurId = clamp(_id, -1, PALETTE_COLOR_NUM - 1); }

  const char *getCurrentName() const { return (mCurId == -1) ? "" : mPalette[mCurId].name.str(); }
  void setCurrentName(const char *_name);

  E3DCOLOR getCurrentColor();
  void setCurrentColor(E3DCOLOR _color);

private:
  int mCurId;
  PaletteEntry mPalette[PALETTE_COLOR_NUM];
};


class ColorDialog : public DialogWindow, public ICustomControl
{
public:
  ColorDialog(void *phandle, const char caption[], E3DCOLOR color, hdpi::Px w = hdpi::_pxScaled(760),
    hdpi::Px h = hdpi::_pxScaled(390));
  ~ColorDialog() override;

  // DialogWindow
  void show() override;
  void onChange(int pcb_id, ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, ContainerPropertyControl *panel) override;
  void updateImguiDialog() override;

  void fillPanel(ContainerPropertyControl *panel);
  E3DCOLOR getColor() const { return mColor; }

  //! convenient static routine for color selection
  static bool select_color(void *hwnd, const char *caption, E3DCOLOR &color)
  {
    ColorDialog *dialog = new ColorDialog(hwnd, caption, color);
    bool ret = dialog->showDialog() == DIALOG_ID_OK;
    if (ret)
      color = dialog->getColor();

    del_it(dialog);
    return ret;
  }

private:
  // ICustomControl
  void customControlUpdate(int id) override;

  void setColor(E3DCOLOR color) { mOldColor = mColor = color; }
  void updatePalette();
  void pickFromScreen();

  eastl::unique_ptr<GradientPlotControl> mainGradientPlotControl;
  eastl::unique_ptr<GradientPlotControl> lineGradientPlotControl;
  eastl::unique_ptr<TwoColorIndicatorControl> twoColorIndicatorControl;
  eastl::unique_ptr<PaletteCellControl> paletteCellsControls[PALETTE_COLOR_NUM];
  E3DCOLOR mColor, mOldColor;
  Point3 mP3Color;
  bool pickingFromScreen = false;

  static CPPalette palette;
  static int rb_sel;
};

} // namespace PropPanel