//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_e3dColor.h>
#include <propPanel2/comWnd/dialog_window.h>

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


class ColorDialog : public CDialogWindow
{
public:
  ColorDialog(void *phandle, const char caption[], E3DCOLOR color);

  virtual void show() override;
  virtual void onChange(int pcb_id, PropPanel2 *panel);
  virtual void onClick(int pcb_id, PropPanel2 *panel);

  void fillPanel(PropPanel2 *panel);
  E3DCOLOR getColor() { return mColor; }

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
  void setColor(E3DCOLOR color) { mOldColor = mColor = color; }
  void updatePalette();
  void pickFromScreen();

  E3DCOLOR mColor, mOldColor;
  Point3 mP3Color;

  static CPPalette palette;
  static int rb_sel;
};
