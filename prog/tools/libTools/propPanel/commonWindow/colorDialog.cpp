// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include <propPanel/commonWindow/colorDialog.h>
#include "gradientPlotControl.h"
#include "paletteCellControl.h"
#include "twoColorIndicatorControl.h"
#include "../c_constants.h" // TRACK_GRADIENT_BUTTON_HEIGHT

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_math3d.h>
#include <propPanel/c_util.h>
#include <propPanel/control/container.h>
#include <winGuiWrapper/wgw_busy.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>
#include <workCycle/dag_workCycle.h>

#include <imgui/imgui.h>
#include <windows.h>

using hdpi::_pxActual;
using hdpi::_pxS;
using hdpi::_pxScaled;
using namespace PropPanel;

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

class ColorNameDialogImgui : public DialogWindow, public ICustomControl
{
  enum
  {
    DIALOG_EDIT_ID = 300,
    DIALOG_COLOR_ID,
  };

public:
  ColorNameDialogImgui(void *phandle, const char name[], E3DCOLOR in_color) :
    DialogWindow(phandle, _pxScaled(300), _pxScaled(140), "Enter color name", false), color(in_color)
  {
    PropPanel::ContainerPropertyControl *panel = DialogWindow::getPanel();
    panel->createEditBox(DIALOG_EDIT_ID, "Color name", name);
    panel->createCustomControlHolder(DIALOG_COLOR_ID, this, false);
  }

  SimpleString getColorName() { return getPanel()->getText(DIALOG_EDIT_ID); }

  virtual void show()
  {
    DialogWindow::show();

    getPanel()->setFocusById(DIALOG_EDIT_ID);
  }

private:
  virtual void customControlUpdate(int id) override
  {
    if (id == DIALOG_COLOR_ID)
    {
      const ImVec2 pos = ImGui::GetWindowPos() + ImGui::GetCursorPos();
      const int size = ImGui::GetItemRectMax().y - pos.y; // Use the edit box's bottom as bottom.

      G_STATIC_ASSERT(sizeof(Color4) == (sizeof(ImVec4)));
      const Color4 asColor4(color);
      ImGui::ColorButton("", reinterpret_cast<const ImVec4 &>(asColor4), ImGuiColorEditFlags_None, ImVec2(size, size));
    }
  }

  const E3DCOLOR color;
};


// -------------------- Dialog -------------------

CPPalette ColorDialog::palette;
int ColorDialog::rb_sel = ID_RADIO_H;

ColorDialog::ColorDialog(void *phandle, const char caption[], E3DCOLOR color) :
  DialogWindow(phandle, _pxScaled(760), _pxScaled(370), caption, false)
{
  setColor(color);
  fillPanel(ColorDialog::getPanel());
}

ColorDialog::~ColorDialog() {}

void ColorDialog::show()
{
  autoSize();

  DialogWindow::show();
}

void ColorDialog::onChange(int pcb_id, ContainerPropertyControl *panel)
{
  if (pcb_id >= ID_PALETTE_CELL_FIRST && pcb_id < ID_PALETTE_CELL_LAST)
  {
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
        mColor = mainGradientPlotControl->getColorValue();
        mP3Color = mainGradientPlotControl->getColorFValue();
        lineGradientPlotControl->setColorFValue(mP3Color);
      }
      else
      {
        mColor = lineGradientPlotControl->getColorValue();
        mP3Color = lineGradientPlotControl->getColorFValue();
        mainGradientPlotControl->setColorFValue(mP3Color);
      }

      twoColorIndicatorControl->setNewColor(mColor);

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

      mainGradientPlotControl->setColorValue(mColor);
      lineGradientPlotControl->setColorValue(mColor);
      twoColorIndicatorControl->setNewColor(mColor);

      mP3Color = mainGradientPlotControl->getColorFValue();
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
          mainGradientPlotControl->setMode(GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_XY);
          lineGradientPlotControl->setMode(GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_S:
          mainGradientPlotControl->setMode(GRADIENT_COLOR_MODE_S + GRADIENT_AXIS_MODE_XY);
          lineGradientPlotControl->setMode(GRADIENT_COLOR_MODE_S + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_V:
          mainGradientPlotControl->setMode(GRADIENT_COLOR_MODE_V + GRADIENT_AXIS_MODE_XY);
          lineGradientPlotControl->setMode(GRADIENT_COLOR_MODE_V + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_R:
          mainGradientPlotControl->setMode(GRADIENT_COLOR_MODE_R + GRADIENT_AXIS_MODE_XY);
          lineGradientPlotControl->setMode(GRADIENT_COLOR_MODE_R + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_G:
          mainGradientPlotControl->setMode(GRADIENT_COLOR_MODE_G + GRADIENT_AXIS_MODE_XY);
          lineGradientPlotControl->setMode(GRADIENT_COLOR_MODE_G + GRADIENT_AXIS_MODE_Y);
          break;

        case ID_RADIO_B:
          mainGradientPlotControl->setMode(GRADIENT_COLOR_MODE_B + GRADIENT_AXIS_MODE_XY);
          lineGradientPlotControl->setMode(GRADIENT_COLOR_MODE_B + GRADIENT_AXIS_MODE_Y);
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

void ColorDialog::onClick(int pcb_id, ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case ID_BUTTON_SET: palette.setCurrentColor(mColor); break;

    case ID_BUTTON_GET:
    {
      mColor = palette.getCurrentColor();
      mainGradientPlotControl->setColorValue(mColor);
      panel->setInt(ID_VALUE_A, mColor.a);
      onChange(ID_MAIN_GRADIENT, panel);
    }
    break;

    case ID_BUTTON_NAME:
    {
      ColorNameDialogImgui *dialog = new ColorNameDialogImgui(nullptr, palette.getCurrentName(), palette.getCurrentColor());

      if (dialog->showDialog() == DIALOG_ID_OK)
        palette.setCurrentName(dialog->getColorName());

      del_it(dialog);
    }
    break;

    case ID_BUTTON_LOAD:
      palette.load(nullptr);
      updatePalette();
      break;

    case ID_BUTTON_SAVE: palette.save(nullptr); break;

    case ID_BUTTON_PICK: pickFromScreen(); break;

    case ID_COLOR:
      mainGradientPlotControl->setColorValue(mOldColor);
      onChange(ID_MAIN_GRADIENT, panel);
      break;
  }
}

void ColorDialog::fillPanel(ContainerPropertyControl *panel)
{
  ContainerPropertyControl *buttons_block = panel->createContainer(ID_BUTTONS_BLOCK, true, _pxScaled(5));
  ContainerPropertyControl *gradient_block = panel->createContainer(ID_GRADIENT_BLOCK, false, _pxScaled(0));

  //----------------------
  // buttons block

  buttons_block->createButton(ID_BUTTON_SET, "Add/change");
  buttons_block->createButton(ID_BUTTON_GET, "Pick from swatch", true, false);
  buttons_block->createButton(ID_BUTTON_NAME, "Swatch name");
  buttons_block->createButton(ID_BUTTON_PICK, "Pick from screen", true, false);
  buttons_block->createButton(ID_BUTTON_SAVE, "Save swatches");
  buttons_block->createButton(ID_BUTTON_LOAD, "Load swatches", true, false);

  buttons_block->createStatic(0, "");

  //----------------------
  // palette block

  for (int i = 0; i < PALETTE_COLOR_NUM; ++i)
  {
    const int controlId = ID_PALETTE_CELL_FIRST + i;
    const bool newLine = (i % 8) == 0;
    buttons_block->createCustomControlHolder(controlId, this, newLine);

    paletteCellsControls[i].reset(new PaletteCellControl(*buttons_block->getById(controlId), _pxS(36), _pxS(36)));
  }

  updatePalette();

  //----------------------
  // gradient block

  G_ASSERT(!mainGradientPlotControl);
  gradient_block->createCustomControlHolder(ID_MAIN_GRADIENT, this);
  mainGradientPlotControl.reset(new GradientPlotControl(*gradient_block->getById(ID_MAIN_GRADIENT), 0, 0, _pxS(200), _pxS(200)));

  G_ASSERT(!lineGradientPlotControl);
  gradient_block->createCustomControlHolder(ID_LINE_GRADIENT, this, false);
  lineGradientPlotControl.reset(
    new GradientPlotControl(*gradient_block->getById(ID_LINE_GRADIENT), 0, 0, _pxS(200), _pxS(200 + TRACK_GRADIENT_BUTTON_HEIGHT)));

  mainGradientPlotControl->setMode(GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_XY);
  mainGradientPlotControl->setColorValue(mColor);

  lineGradientPlotControl->setMode(GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_Y);
  lineGradientPlotControl->setColorValue(mColor);

  // radio buttons

  ContainerPropertyControl *_rg = gradient_block->createRadioGroup(ID_RADIO_GROUP, "Select axis:", false);
  _rg->createRadio(ID_RADIO_H, "H");
  _rg->createRadio(ID_RADIO_S, "S");
  _rg->createRadio(ID_RADIO_V, "V");

  _rg->createRadio(ID_RADIO_R, "R");
  _rg->createEditInt(ID_VALUE_R, "", mColor.r, true, false);
  _rg->setMinMaxStep(ID_VALUE_R, 0, 255, 1);

  _rg->createRadio(ID_RADIO_G, "G");
  _rg->createEditInt(ID_VALUE_G, "", mColor.g, true, false);
  _rg->setMinMaxStep(ID_VALUE_G, 0, 255, 1);

  _rg->createRadio(ID_RADIO_B, "B");
  _rg->createEditInt(ID_VALUE_B, "", mColor.b, true, false);
  _rg->setMinMaxStep(ID_VALUE_B, 0, 255, 1);

  panel->setInt(ID_RADIO_GROUP, rb_sel);

  // current color block

  gradient_block->createCustomControlHolder(ID_COLOR, this);
  G_ASSERT(!twoColorIndicatorControl);
  twoColorIndicatorControl.reset(new TwoColorIndicatorControl(*gradient_block->getById(ID_COLOR)));
  twoColorIndicatorControl->setOldColor(mColor);
  twoColorIndicatorControl->setNewColor(mColor);

  gradient_block->createTrackInt(ID_VALUE_A, "Alpha:", mColor.a, 0, 255, 1, true, false);

  panel->createEditBox(ID_EDIT_INT, "", String(0, "%d, %d, %d, %d", mColor.r, mColor.g, mColor.b, mColor.a).c_str(), true, true);
  panel->createEditBox(ID_EDIT_FLOAT, "",
    String(0, "%.3f, %.3f, %.3f, %.3f", mColor.r / 255.f, mColor.g / 255.f, mColor.b / 255.f, mColor.a / 255.f).c_str(), true, false);
}

void ColorDialog::updatePalette()
{
  ContainerPropertyControl *buttons_block = (ContainerPropertyControl *)getPanel()->getById(ID_BUTTONS_BLOCK);
  const bool hasSelection = palette.getCurrentId() >= 0;
  buttons_block->setEnabledById(ID_BUTTON_SET, hasSelection);
  buttons_block->setEnabledById(ID_BUTTON_GET, hasSelection);
  buttons_block->setEnabledById(ID_BUTTON_NAME, hasSelection);
}

void ColorDialog::pickFromScreen()
{
  G_ASSERT_RETURN(!pickingFromScreen, );
  pickingFromScreen = true;

  ContainerPropertyControl *panel = getPanel();
  panel->setEnabledById(ID_BUTTON_PICK, false);

  HDC sDC = GetWindowDC(NULL);

  while (dialogResult == DIALOG_ID_NONE)
  {
    d3d::GpuAutoLock gpuLock;

    dagor_work_cycle_flush_pending_frame();
    dagor_draw_scene_and_gui(true, true);
    d3d::update_screen();
    dagor_work_cycle();

    POINT pos;
    GetCursorPos(&pos);

    COLORREF color = GetPixel(sDC, pos.x, pos.y);
    mColor.r = GetRValue(color);
    mColor.g = GetGValue(color);
    mColor.b = GetBValue(color);
    twoColorIndicatorControl->setNewColor(mColor);

    if (wingw::is_key_pressed(VK_MBUTTON))
    {
      mainGradientPlotControl->setColorValue(mColor);
      onChange(ID_MAIN_GRADIENT, panel);
      break;
    }
  }

  ReleaseDC(NULL, sDC);

  panel->setEnabledById(ID_BUTTON_PICK, true);
  pickingFromScreen = false;
}

void ColorDialog::customControlUpdate(int id)
{
  if (id == ID_MAIN_GRADIENT && mainGradientPlotControl)
  {
    // Line up with the other gradient control.
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (hdpi::_pxS(TRACK_GRADIENT_BUTTON_HEIGHT) / 2));
    mainGradientPlotControl->updateImgui();
  }
  else if (id == ID_LINE_GRADIENT && lineGradientPlotControl)
  {
    lineGradientPlotControl->updateImgui();
  }
  else if (id == ID_COLOR && twoColorIndicatorControl)
  {
    const int height = ImGui::GetContentRegionAvail().y;
    twoColorIndicatorControl->updateImgui(height * 2, height);
  }
  else if (id >= ID_PALETTE_CELL_FIRST && id < ID_PALETTE_CELL_LAST)
  {
    const int index = id - ID_PALETTE_CELL_FIRST;
    if (paletteCellsControls[index])
    {
      const char *name = palette.getName(index);
      const E3DCOLOR color = palette.getColor(index);
      const bool selected = index == palette.getCurrentId();
      paletteCellsControls[index]->updateImgui(color, name, selected);
    }
  }
}

void ColorDialog::updateImguiDialog()
{
  DialogWindow::updateImguiDialog();

  if (!pickingFromScreen)
    return;

  const ImVec2 position = ImGui::GetWindowPos();
  ImGui::SetNextWindowPos(position);
  ImGui::Begin("ScreenPickingTooltip", nullptr,
    ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
  ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
  ImGui::Text("Use the middle mouse button to pick a color from the screen.");
  ImGui::PopStyleColor();
  ImGui::End();
}
