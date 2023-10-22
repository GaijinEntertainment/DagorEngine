// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/c_util.h>
#include <propPanel2/comWnd/color_dialog.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <math/dag_math3d.h>
#include <ioSys/dag_dataBlock.h>

#include <windows.h>

using hdpi::_pxActual;
using hdpi::_pxS;
using hdpi::_pxScaled;

enum
{
  ID_BUTTONS_BLOCK = 100,

  ID_BUTTON_SET,
  ID_BUTTON_GET,
  ID_BUTTON_NAME,
  ID_BUTTON_LOAD,
  ID_BUTTON_SAVE,
  ID_BUTTON_PICK,

  ID_PALETTE_BLOCK,

  ID_PALETTE_CELL_FIRST,
  ID_PALETTE_CELL_LAST = ID_PALETTE_CELL_FIRST + PALETTE_COLOR_NUM,

  ID_GRADIENT_BLOCK,
  ID_MAIN_GRADIENT,
  ID_LINE_GRADIENT,
  ID_COLOR,

  ID_RADIO_GROUP,

  ID_RADIO_H,
  ID_RADIO_S,
  ID_RADIO_V,

  ID_RADIO_R,
  ID_RADIO_G,
  ID_RADIO_B,

  ID_VALUE_R,
  ID_VALUE_G,
  ID_VALUE_B,
  ID_VALUE_A,

  ID_EDIT_INT,
  ID_EDIT_FLOAT,

};

// ------------------ Palette -------------------

CPPalette::CPPalette()
{
  mCurId = -1;

  // default fill

  mPalette[0] = PaletteEntry(E3DCOLOR(0, 0, 0, 255), "black");
  mPalette[1] = PaletteEntry(E3DCOLOR(128, 128, 128, 255), "gray");
  mPalette[2] = PaletteEntry(E3DCOLOR(128, 0, 0, 255), "maroon");
  mPalette[3] = PaletteEntry(E3DCOLOR(128, 128, 0, 255), "olive");
  mPalette[4] = PaletteEntry(E3DCOLOR(0, 128, 0, 255), "green");
  mPalette[5] = PaletteEntry(E3DCOLOR(0, 128, 128, 255), "teal");
  mPalette[6] = PaletteEntry(E3DCOLOR(0, 0, 128, 255), "navy");
  mPalette[7] = PaletteEntry(E3DCOLOR(128, 0, 128, 255), "purple");

  mPalette[8] = PaletteEntry(E3DCOLOR(255, 255, 255, 255), "white");
  mPalette[9] = PaletteEntry(E3DCOLOR(192, 192, 192, 255), "silver");
  mPalette[10] = PaletteEntry(E3DCOLOR(255, 0, 0, 255), "red");
  mPalette[11] = PaletteEntry(E3DCOLOR(255, 255, 0, 255), "yellow");
  mPalette[12] = PaletteEntry(E3DCOLOR(0, 255, 0, 255), "lime");
  mPalette[13] = PaletteEntry(E3DCOLOR(0, 255, 255, 255), "aqua");
  mPalette[14] = PaletteEntry(E3DCOLOR(0, 0, 255, 255), "blue");
  mPalette[15] = PaletteEntry(E3DCOLOR(255, 0, 255, 255), "fuchsia");

  for (int i = 16; i < PALETTE_COLOR_NUM; ++i)
  {
    mPalette[i].name = "";
    mPalette[i].color = E3DCOLOR(255, 255, 255, 0);
  }
}

void CPPalette::load(void *handle)
{
  String fn = wingw::file_open_dlg(handle, "Load color palette", "Data file|*.colors.blk", "colors.blk");

  if (fn.length())
  {
    DataBlock blk(fn);
    DataBlock *palette_blk = blk.getBlockByName("Palette");
    if (palette_blk)
    {
      mCurId = palette_blk->getInt("CurId", -1);
      DataBlock *entry_blk = NULL;

      for (int i = 0; i < PALETTE_COLOR_NUM; ++i)
        if ((i < palette_blk->blockCount()) && (entry_blk = palette_blk->getBlock(i)))
        {
          mPalette[i].name = entry_blk->getStr("name", "");
          mPalette[i].color = entry_blk->getE3dcolor("color", E3DCOLOR(255, 255, 255, 0));
        }
        else
        {
          mPalette[i].name = "";
          mPalette[i].color = E3DCOLOR(255, 255, 255, 0);
        }
    }
  }
}

void CPPalette::save(void *handle)
{
  String fn = wingw::file_save_dlg(handle, "Save color palette", "Data file|*.colors.blk", "colors.blk");

  if (fn.length())
  {
    DataBlock blk;
    DataBlock *palette_blk = blk.addNewBlock("Palette");
    if (palette_blk)
    {
      palette_blk->addInt("CurId", mCurId);
      DataBlock *entry_blk = NULL;

      for (int i = 0; i < PALETTE_COLOR_NUM; ++i)
      {
        if (DataBlock *entry_blk = palette_blk->addNewBlock("Swatch"))
        {
          entry_blk->addStr("name", mPalette[i].name);
          entry_blk->addE3dcolor("color", mPalette[i].color);
        }
      }
    }

    blk.saveToTextFile(fn);
  }
}

void CPPalette::setCurrentName(const char *_name)
{
  if (mCurId == -1)
    return;

  mPalette[mCurId].name = _name;
}

E3DCOLOR CPPalette::getCurrentColor() { return (mCurId == -1) ? E3DCOLOR(255, 255, 255, 0) : mPalette[mCurId].color; }

void CPPalette::setCurrentColor(E3DCOLOR _color)
{
  if (mCurId == -1)
    return;

  mPalette[mCurId].color = _color;
}

// ----------- Select name dialog ----------------

class ColorNameDialog : public CDialogWindow
{
  enum
  {
    DIALOG_EDIT_ID = 300,
    DIALOG_COLOR_ID,
  };

public:
  ColorNameDialog(void *phandle, const char name[], E3DCOLOR color) :
    CDialogWindow(phandle, _pxScaled(300), _pxScaled(140), "Enter color name", false)
  {
    PropPanel2 *panel = CDialogWindow::getPanel();

    panel->createEditBox(DIALOG_EDIT_ID, "Color name", name);

    panel->createTwoColorIndicator(DIALOG_COLOR_ID, "", _pxScaled(40), true, false);
    panel->setPoint3(DIALOG_COLOR_ID, Point3(color.r, color.g, color.b));
    panel->setColor(DIALOG_COLOR_ID, color);

    // move and resize

    PropertyControlBase *_cur_ctrl = panel->getById(DIALOG_EDIT_ID);
    if (_cur_ctrl)
    {
      _cur_ctrl->resize(_pxScaled(230), _pxActual(_cur_ctrl->getHeight()));
      _cur_ctrl->moveTo(_cur_ctrl->getX(), _cur_ctrl->getY());
    }

    _cur_ctrl = panel->getById(DIALOG_COLOR_ID);
    if (_cur_ctrl)
    {
      _cur_ctrl->resize(_pxScaled(40), _pxScaled(40));
      _cur_ctrl->moveTo(_pxS(245), _cur_ctrl->getY());
    }
  }

  SimpleString getColorName() { return getPanel()->getText(DIALOG_EDIT_ID); }

  virtual void show()
  {
    CDialogWindow::show();

    getPanel()->setFocusById(DIALOG_EDIT_ID);
  }
};


// -------------------- Dialog -------------------

CPPalette ColorDialog::palette;
int ColorDialog::rb_sel = ID_RADIO_H;

ColorDialog::ColorDialog(void *phandle, const char caption[], E3DCOLOR color) :
  CDialogWindow(phandle, _pxScaled(710), _pxScaled(370), caption, false)
{
  setColor(color);
  fillPanel(getPanel());
}


void ColorDialog::onChange(int pcb_id, PropPanel2 *panel)
{
  if (pcb_id >= ID_PALETTE_CELL_FIRST && pcb_id < ID_PALETTE_CELL_LAST)
  {
    for (int id = ID_PALETTE_CELL_FIRST; id < ID_PALETTE_CELL_LAST; ++id)
      if (id != pcb_id)
        panel->setBool(id, false);

    palette.setCurrentId(pcb_id - ID_PALETTE_CELL_FIRST);

    panel->setEnabledById(ID_BUTTON_SET, true);
    panel->setEnabledById(ID_BUTTON_GET, true);
    panel->setEnabledById(ID_BUTTON_NAME, true);

    onClick(ID_BUTTON_GET, panel);
    return;
  }

  unsigned char _alpha = mColor.a;

  switch (pcb_id)
  {
    case ID_MAIN_GRADIENT:
    case ID_LINE_GRADIENT:
      if (pcb_id == ID_MAIN_GRADIENT)
      {
        mColor = panel->getColor(ID_MAIN_GRADIENT);
        mP3Color = panel->getPoint3(ID_MAIN_GRADIENT);
        panel->setPoint3(ID_LINE_GRADIENT, mP3Color);
      }
      else
      {
        mColor = panel->getColor(ID_LINE_GRADIENT);
        mP3Color = panel->getPoint3(ID_LINE_GRADIENT);
        panel->setPoint3(ID_MAIN_GRADIENT, mP3Color);
      }

      panel->setColor(ID_COLOR, mColor);

      panel->setInt(ID_VALUE_R, mColor.r);
      panel->setInt(ID_VALUE_G, mColor.g);
      panel->setInt(ID_VALUE_B, mColor.b);
      mColor.a = _alpha;
      break;

    case ID_VALUE_R:
    case ID_VALUE_G:
    case ID_VALUE_B:
      mColor.r = panel->getInt(ID_VALUE_R);
      mColor.g = panel->getInt(ID_VALUE_G);
      mColor.b = panel->getInt(ID_VALUE_B);

      panel->setColor(ID_MAIN_GRADIENT, mColor);
      panel->setColor(ID_LINE_GRADIENT, mColor);
      panel->setColor(ID_COLOR, mColor);

      mP3Color = panel->getPoint3(ID_MAIN_GRADIENT);
      break;

    case ID_VALUE_A:
      mColor.a = panel->getInt(ID_VALUE_A);
      break;

      // change mode

    case ID_RADIO_GROUP:
      rb_sel = panel->getInt(ID_RADIO_GROUP);
      switch (rb_sel)
      {
        case ID_RADIO_H:
          panel->setInt(ID_MAIN_GRADIENT, GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_XY);
          panel->setInt(ID_LINE_GRADIENT, GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_S:
          panel->setInt(ID_MAIN_GRADIENT, GRADIENT_COLOR_MODE_S + GRADIENT_AXIS_MODE_XY);
          panel->setInt(ID_LINE_GRADIENT, GRADIENT_COLOR_MODE_S + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_V:
          panel->setInt(ID_MAIN_GRADIENT, GRADIENT_COLOR_MODE_V + GRADIENT_AXIS_MODE_XY);
          panel->setInt(ID_LINE_GRADIENT, GRADIENT_COLOR_MODE_V + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_R:
          panel->setInt(ID_MAIN_GRADIENT, GRADIENT_COLOR_MODE_R + GRADIENT_AXIS_MODE_XY);
          panel->setInt(ID_LINE_GRADIENT, GRADIENT_COLOR_MODE_R + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_G:
          panel->setInt(ID_MAIN_GRADIENT, GRADIENT_COLOR_MODE_G + GRADIENT_AXIS_MODE_XY);
          panel->setInt(ID_LINE_GRADIENT, GRADIENT_COLOR_MODE_G + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_B:
          panel->setInt(ID_MAIN_GRADIENT, GRADIENT_COLOR_MODE_B + GRADIENT_AXIS_MODE_XY);
          panel->setInt(ID_LINE_GRADIENT, GRADIENT_COLOR_MODE_B + GRADIENT_AXIS_MODE_Y);
          break;
      }
      break;
  }
  if (pcb_id != ID_EDIT_INT && pcb_id != ID_EDIT_FLOAT)
  {
    panel->setText(ID_EDIT_INT, String(0, "%d, %d, %d, %d", mColor.r, mColor.g, mColor.b, mColor.a).c_str());
    panel->setText(ID_EDIT_FLOAT,
      String(0, "%.3f, %.3f, %.3f, %.3f", mColor.r / 255.f, mColor.g / 255.f, mColor.b / 255.f, mColor.a / 255.f).c_str());
  }
}


void ColorDialog::onClick(int pcb_id, PropPanel2 *panel)
{
  switch (pcb_id)
  {
    case ID_BUTTON_SET:
    {
      int cell_id = ID_PALETTE_CELL_FIRST + palette.getCurrentId();
      panel->setColor(cell_id, mColor);
      panel->setText(cell_id, palette.getCurrentName());
      palette.setCurrentColor(mColor);
    }
    break;

    case ID_BUTTON_GET:
    {
      mColor = palette.getCurrentColor();
      panel->setColor(ID_MAIN_GRADIENT, mColor);
      panel->setInt(ID_VALUE_A, mColor.a);
      onChange(ID_MAIN_GRADIENT, panel);
    }
    break;

    case ID_BUTTON_NAME:
    {
      ColorNameDialog *dialog = new ColorNameDialog(mpHandle, palette.getCurrentName(), palette.getCurrentColor());

      if (dialog->showDialog() == DIALOG_ID_OK)
      {
        palette.setCurrentName(dialog->getColorName());
        panel->setText(ID_PALETTE_CELL_FIRST + palette.getCurrentId(), palette.getCurrentName());
      }

      del_it(dialog);
    }
    break;

    case ID_BUTTON_LOAD:
      palette.load(getHandle());
      updatePalette();
      break;

    case ID_BUTTON_SAVE: palette.save(getHandle()); break;

    case ID_BUTTON_PICK: pickFromScreen(); break;

    case ID_COLOR:
      panel->setColor(ID_MAIN_GRADIENT, mOldColor);
      onChange(ID_MAIN_GRADIENT, panel);
      break;
  }
}


void ColorDialog::fillPanel(PropPanel2 *panel)
{
  PropertyControlBase *_cur_ctrl = NULL;
  PropPanel2 *buttons_block = panel->createContainer(ID_BUTTONS_BLOCK, true, _pxScaled(5));
  PropPanel2 *gradient_block = panel->createContainer(ID_GRADIENT_BLOCK, false, _pxScaled(0));

  //----------------------
  // buttons block

  buttons_block->setWidth(_pxScaled(290));
  gradient_block->moveTo(0, gradient_block->getY());

  buttons_block->createButton(ID_BUTTON_SET, "Add/change");
  buttons_block->createButton(ID_BUTTON_GET, "Pick from swatch", true, false);
  buttons_block->createButton(ID_BUTTON_NAME, "Swatch name");
  buttons_block->createButton(ID_BUTTON_PICK, "Pick from screen", true, false);
  buttons_block->createButton(ID_BUTTON_SAVE, "Save swatches");
  buttons_block->createButton(ID_BUTTON_LOAD, "Load swatches", true, false);

  buttons_block->createStatic(0, "");

  //----------------------
  // palette block

  for (int id = ID_PALETTE_CELL_FIRST; id < ID_PALETTE_CELL_LAST; ++id)
    buttons_block->createPaletteCell(id, "", true, !((id - ID_PALETTE_CELL_FIRST) % 8));

  updatePalette();

  //----------------------
  // gradient block

  gradient_block->setWidth(_pxScaled(400));
  gradient_block->moveTo(_pxS(300), gradient_block->getY());

  gradient_block->createGradientPlot(ID_MAIN_GRADIENT, "", _pxScaled(200));
  panel->setInt(ID_MAIN_GRADIENT, GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_XY);
  panel->setColor(ID_MAIN_GRADIENT, mColor);

  gradient_block->createGradientPlot(ID_LINE_GRADIENT, "", _pxScaled(200 + 11), true, false);
  panel->setInt(ID_LINE_GRADIENT, GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_Y);
  panel->setColor(ID_LINE_GRADIENT, mColor);

  PropPanel2 *_rg = gradient_block->createRadioGroup(ID_RADIO_GROUP, "Select axis:", false);

  // resize and move

  panel->setWidthById(ID_MAIN_GRADIENT, _pxScaled(200));
  panel->setWidthById(ID_LINE_GRADIENT, _pxScaled(30));
  panel->setWidthById(ID_RADIO_GROUP, _pxScaled(145));

  _cur_ctrl = panel->getById(ID_MAIN_GRADIENT);
  if (_cur_ctrl)
    _cur_ctrl->moveTo(_cur_ctrl->getX(), _cur_ctrl->getY() + _pxS(5));

  _cur_ctrl = panel->getById(ID_LINE_GRADIENT);
  if (_cur_ctrl)
    _cur_ctrl->moveTo(_pxS(215), _cur_ctrl->getY());

  _rg->moveTo(_pxS(255), _rg->getY());

  // radio buttons

  _rg->createRadio(ID_RADIO_H, "H");
  _rg->createRadio(ID_RADIO_S, "S");
  _rg->createRadio(ID_RADIO_V, "V");

  _rg->createRadio(ID_RADIO_R, "R");
  _rg->createEditInt(ID_VALUE_R, "", mColor.r, true, false);
  _rg->setMinMaxStep(ID_VALUE_R, 0, 255, 1);

  _cur_ctrl = panel->getById(ID_VALUE_R);
  if (_cur_ctrl)
    _cur_ctrl->moveTo(_cur_ctrl->getX() - _pxS(15), _cur_ctrl->getY());

  _rg->createRadio(ID_RADIO_G, "G");
  _rg->createEditInt(ID_VALUE_G, "", mColor.g, true, false);
  _rg->setMinMaxStep(ID_VALUE_G, 0, 255, 1);

  _cur_ctrl = panel->getById(ID_VALUE_G);
  if (_cur_ctrl)
    _cur_ctrl->moveTo(_cur_ctrl->getX() - _pxS(15), _cur_ctrl->getY());

  _rg->createRadio(ID_RADIO_B, "B");
  _rg->createEditInt(ID_VALUE_B, "B", mColor.b, true, false);
  _rg->setMinMaxStep(ID_VALUE_B, 0, 255, 1);

  _cur_ctrl = panel->getById(ID_VALUE_B);
  if (_cur_ctrl)
    _cur_ctrl->moveTo(_cur_ctrl->getX() - _pxS(15), _cur_ctrl->getY());

  panel->setInt(ID_RADIO_GROUP, rb_sel);

  // current color block

  gradient_block->createTwoColorIndicator(ID_COLOR, "", _pxScaled(35));
  panel->setPoint3(ID_COLOR, Point3(mColor.r, mColor.g, mColor.b));
  panel->setColor(ID_COLOR, mColor);

  gradient_block->createTrackInt(ID_VALUE_A, "Alpha:", mColor.a, 0, 255, 1, true, false);

  // resizing

  _cur_ctrl = panel->getById(ID_COLOR);
  if (_cur_ctrl)
  {
    _cur_ctrl->resize(_pxScaled(100), _pxScaled(20));
    _cur_ctrl->moveTo(_cur_ctrl->getX(), _cur_ctrl->getY() + _pxS(5));
  }

  _cur_ctrl = panel->getById(ID_VALUE_A);
  if (_cur_ctrl)
  {
    _cur_ctrl->moveTo(_pxS(115), _cur_ctrl->getY());
    _cur_ctrl->resize(_pxScaled(285), _pxActual(_cur_ctrl->getHeight()));
  }

  panel->createEditBox(ID_EDIT_INT, "", String(0, "%d, %d, %d, %d", mColor.r, mColor.g, mColor.b, mColor.a).c_str(), true, true);
  panel->createEditBox(ID_EDIT_FLOAT, "",
    String(0, "%.3f, %.3f, %.3f, %.3f", mColor.r / 255.f, mColor.g / 255.f, mColor.b / 255.f, mColor.a / 255.f).c_str(), true, false);
}


void ColorDialog::updatePalette()
{
  PropPanel2 *buttons_block = (PropPanel2 *)getPanel()->getById(ID_BUTTONS_BLOCK);
  int cur_id = palette.getCurrentId();

  for (int id = ID_PALETTE_CELL_FIRST; id < ID_PALETTE_CELL_LAST; ++id)
  {
    buttons_block->setBool(id, false);
    palette.setCurrentId(id - ID_PALETTE_CELL_FIRST);
    buttons_block->setColor(id, palette.getCurrentColor());
    buttons_block->setText(id, palette.getCurrentName());
  }

  palette.setCurrentId(cur_id);
  if (cur_id != -1)
    buttons_block->setBool(ID_PALETTE_CELL_FIRST + cur_id, true);

  buttons_block->setEnabledById(ID_BUTTON_SET, (cur_id != -1));
  buttons_block->setEnabledById(ID_BUTTON_GET, (cur_id != -1));
  buttons_block->setEnabledById(ID_BUTTON_NAME, (cur_id != -1));
}


void ColorDialog::pickFromScreen()
{
  HDC sDC = GetWindowDC(NULL);
  POINT pos;
  COLORREF color;
  MSG msg;
  const int CAPTURE_TIMER_ID = 1001;
  SetTimer((HWND)getHandle(), CAPTURE_TIMER_ID, 100, NULL);


  PropPanel2 *panel = getPanel();
  panel->setEnabledById(ID_BUTTON_PICK, false);
  panel->setCaption(ID_BUTTON_PICK, "Middle mouse wait ...");

  bool colorSelected = false;

  while ((mDResult == DIALOG_ID_NONE) && GetMessageW(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);

    if (msg.message == WM_PAINT)
      continue;

    GetCursorPos(&pos);
    color = GetPixel(sDC, pos.x, pos.y);
    mColor.r = GetRValue(color);
    mColor.g = GetGValue(color);
    mColor.b = GetBValue(color);

    panel->setColor(ID_COLOR, mColor);

    colorSelected = wingw::is_key_pressed(VK_MBUTTON);
    if (colorSelected)
      break;
  }

  KillTimer((HWND)getHandle(), CAPTURE_TIMER_ID);
  ReleaseDC(NULL, sDC);
  panel->setCaption(ID_BUTTON_PICK, "Pick from screen");
  panel->setEnabledById(ID_BUTTON_PICK, true);

  if (colorSelected)
  {
    panel->setColor(ID_MAIN_GRADIENT, mColor);
    onChange(ID_MAIN_GRADIENT, panel);
    SetFocus((HWND)getHandle());
  }
}
