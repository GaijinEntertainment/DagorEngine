// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <winGuiWrapper/wgw_dialogs.h>
#include <propPanel2/comWnd/curve_color_dialog.h>
#include <propPanel2/comWnd/color_correction_info.h>
#include "comControls/color_curve.h"

#include <windows.h>

enum
{
  ID_CURVE_CONTAINER = 100,
  ID_PRESET,
  ID_SAVE_PRESET,
  ID_REMOVE_PRESET,
  ID_INDENT,
  ID_COLOR_MODE,
  ID_SHOW_BAR_CHART,
  ID_SHOW_BASE_LINE,

  ID_PRESET_NAME = 200,
};


// -------------------- Dialog -------------------


DataBlock CurveColorDialog::presetsBlk;
bool CurveColorDialog::showChartBars = true;
bool CurveColorDialog::showBaseLine = true;


CurveColorDialog::CurveColorDialog(void *phandle, const char caption[]) :
  CDialogWindow(phandle, _pxScaled(310), _pxScaled(560), caption, false), colorCurve(NULL), presets(midmem)
{
  updatePresets();
  fillPanel(getPanel());
}


CurveColorDialog::~CurveColorDialog() { del_it(colorCurve); }


void CurveColorDialog::onChange(int pcb_id, PropPanel2 *panel)
{
  switch (pcb_id)
  {
    case ID_COLOR_MODE:
      if (colorCurve)
        colorCurve->setMode(panel->getInt(ID_COLOR_MODE));
      break;

    case ID_CURVE_CONTAINER:
    {
      int cur_prst = panel->getInt(ID_PRESET);
      if (cur_prst != presets.size() - 1)
        panel->setInt(ID_PRESET, presets.size() - 1);
    }
    break;

    case ID_PRESET:
    {
      int preset_ind = panel->getInt(ID_PRESET);
      ColorCorrectionInfo cc_info;

      if (preset_ind > 0 && preset_ind + 1 < presets.size())
      {
        DataBlock *p_blk = presetsBlk.getBlockByName("preset", preset_ind - 2);
        if (p_blk)
          cc_info.load(*p_blk);
      }
      else if (preset_ind != 0)
        return;

      setCorrectionInfo(cc_info);
    }
    break;
  }
}


void CurveColorDialog::onClick(int pcb_id, PropPanel2 *panel)
{
  switch (pcb_id)
  {
    case ID_SHOW_BAR_CHART:
      if (colorCurve)
      {
        showChartBars = panel->getBool(ID_SHOW_BAR_CHART);
        colorCurve->setChartBarsVisible(showChartBars);
      }
      break;

    case ID_SHOW_BASE_LINE:
      if (colorCurve)
      {
        showBaseLine = panel->getBool(ID_SHOW_BASE_LINE);
        colorCurve->setBaseLineVisible(showBaseLine);
      }
      break;

    case ID_SAVE_PRESET:
      if (colorCurve)
      {
        ColorCorrectionInfo cc_info;
        getCorrectionInfo(cc_info);

        CDialogWindow dialog(mpHandle, _pxScaled(300), _pxScaled(130), "Preset name");
        PropPanel2 *d_panel = dialog.getPanel();
        String new_name("new_preset");
        d_panel->createEditBox(ID_PRESET_NAME, "Enter new preset name", new_name.str());
        bool name_ok = false;
        while (!name_ok)
          if (dialog.showDialog() == DIALOG_ID_OK)
          {
            name_ok = true;
            new_name = d_panel->getText(ID_PRESET_NAME);
            for (int i = 0; i < presets.size(); ++i)
              if (new_name == presets[i])
              {
                name_ok = false;
                d_panel->setCaption(ID_PRESET_NAME, "Enter new preset name (name is busy):");
                break;
              }
          }
          else
            return;

        DataBlock *p_blk = presetsBlk.addNewBlock("preset");
        p_blk->addStr("preset_name", new_name.str());
        cc_info.save(*p_blk);
        updatePresets();
        panel->setStrings(ID_PRESET, presets);
        panel->setInt(ID_PRESET, presets.size() - 2);
      }
      break;

    case ID_REMOVE_PRESET:
    {
      int preset_ind = panel->getInt(ID_PRESET);
      if (preset_ind > 0 && (preset_ind + 1 < presets.size()) &&
          (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Remove preset \"%s\"?",
             presets[preset_ind].str()) == wingw::MB_ID_YES))
      {
        presetsBlk.removeBlock(preset_ind - 1);
        updatePresets();
        panel->setStrings(ID_PRESET, presets);
        panel->setInt(ID_PRESET, presets.size() - 1);
      }
    }
    break;
  }
}


void CurveColorDialog::fillPanel(PropPanel2 *panel)
{
  panel->createCombo(ID_PRESET, "Preset:", presets, presets.size() - 1);
  panel->createButton(ID_SAVE_PRESET, "Save custom preset");
  panel->createButton(ID_REMOVE_PRESET, "Remove preset", true, false);

  Tab<String> lines(tmpmem);
  lines.push_back() = "RGB";
  lines.push_back() = "Red";
  lines.push_back() = "Green";
  lines.push_back() = "Blue";
  panel->createCombo(ID_COLOR_MODE, "Channel:", lines, 0);

  PropertyContainerControlBase *container = panel->createContainer(ID_CURVE_CONTAINER, true, _pxScaled(135));
  container->createStatic(ID_INDENT, "");
  del_it(colorCurve);
  colorCurve = new WColorCurveControl(container, container->getWindow(), 10, 10, _pxS(270), _pxS(270));

  panel->createCheckBox(ID_SHOW_BAR_CHART, "Show bar chart", showChartBars);
  colorCurve->setChartBarsVisible(showChartBars);
  panel->createCheckBox(ID_SHOW_BASE_LINE, "Show base line", showBaseLine);
  colorCurve->setBaseLineVisible(showBaseLine);
}


void CurveColorDialog::setColorStats(int r_channel[256], int g_channel[256], int b_channel[256])
{
  if (!colorCurve)
    return;

  unsigned char bar_chart[3][256];
  memset(bar_chart, 0, 256 * 3);

  int maxR = 0, maxG = 0, maxB = 0;

  for (int i = 0; i < 256; ++i)
  {
    if (maxR < r_channel[i])
      maxR = r_channel[i];
    if (maxG < g_channel[i])
      maxG = g_channel[i];
    if (maxB < b_channel[i])
      maxB = b_channel[i];
  }

  for (int i = 0; i < 256; ++i)
  {
    bar_chart[0][i] = (unsigned char)maxR ? (r_channel[i] * 255 / maxR) : 0;
    bar_chart[1][i] = (unsigned char)maxG ? (g_channel[i] * 255 / maxG) : 0;
    bar_chart[2][i] = (unsigned char)maxB ? (b_channel[i] * 255 / maxB) : 0;
  }

  colorCurve->setBarChart(bar_chart);
}


int fixColorVal(float val)
{
  if (val > 255)
    return 255;
  if (val < 0)
    return 0;

  return val;
}


void CurveColorDialog::getColorTables(unsigned char r_table[256], unsigned char g_table[256], unsigned char b_table[256])
{
  if (!colorCurve)
    return;

  unsigned char rgb_r_g_b[4][256];
  colorCurve->getColorTables(rgb_r_g_b);

  for (int i = 0; i < 256; ++i)
  {
    float dt = 1.0 * (rgb_r_g_b[0][i] + 1) / (i + 1);

    for (int j = 1; j < 4; ++j)
      rgb_r_g_b[j][i] = fixColorVal(dt * rgb_r_g_b[j][i]);
  }

  memcpy(r_table, rgb_r_g_b[1], 256);
  memcpy(g_table, rgb_r_g_b[2], 256);
  memcpy(b_table, rgb_r_g_b[3], 256);
}

void CurveColorDialog::setCorrectionInfo(const ColorCorrectionInfo &info)
{
  if (!colorCurve)
    return;
  colorCurve->setCorrectionInfo(info);
}


void CurveColorDialog::getCorrectionInfo(ColorCorrectionInfo &settings)
{
  if (!colorCurve)
    return;
  colorCurve->getCorrectionInfo(settings);
}


void CurveColorDialog::updatePresets()
{
  clear_and_shrink(presets);
  presets.push_back() = "Default";
  for (int i = 0; i < presetsBlk.blockCount(); ++i)
  {
    DataBlock *_blk = presetsBlk.getBlock(i);
    if (_blk)
      presets.push_back() = _blk->getStr("preset_name");
  }
  presets.push_back() = "Custom";
}


void CurveColorDialog::loadPresets(const char *fn)
{
  debug("Load color correction presets \"%s\"", fn);
  DataBlock blk(fn);
  DataBlock *p_blk = blk.getBlockByName("color_correction_presets");
  if (p_blk)
    presetsBlk.setFrom(p_blk);
}


void CurveColorDialog::savePresets(const char *fn)
{
  debug("Save color correction presets \"%s\"", fn);
  DataBlock blk;
  DataBlock *p_blk = blk.addBlock("color_correction_presets");
  if (p_blk)
    p_blk->setFrom(&presetsBlk);
  blk.saveToTextFile(fn);
}
