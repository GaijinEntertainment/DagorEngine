//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <propPanel2/comWnd/dialog_window.h>
#include <ioSys/dag_dataBlock.h>

class WColorCurveControl;
struct ColorCorrectionInfo;

class CurveColorDialog : public CDialogWindow
{
public:
  CurveColorDialog(void *phandle, const char caption[]);
  virtual ~CurveColorDialog();

  virtual void onChange(int pcb_id, PropPanel2 *panel);
  virtual void onClick(int pcb_id, PropPanel2 *panel);

  void fillPanel(PropPanel2 *panel);
  void updatePresets();

  void setColorStats(int r_channel[256], int g_channel[256], int b_channel[256]);
  void getColorTables(unsigned char r_table[256], unsigned char g_table[256], unsigned char b_table[256]);

  void setCorrectionInfo(const ColorCorrectionInfo &info);
  void getCorrectionInfo(ColorCorrectionInfo &info);

  static void loadPresets(const char *fn);
  static void savePresets(const char *fn);

private:
  WColorCurveControl *colorCurve;
  Tab<String> presets;
  static DataBlock presetsBlk;
  static bool showChartBars, showBaseLine;
};
