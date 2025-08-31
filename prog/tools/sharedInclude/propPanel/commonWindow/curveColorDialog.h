//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/customControl.h>
#include <ioSys/dag_dataBlock.h>

namespace PropPanel
{

class ColorCurveControl;
struct ColorCorrectionInfo;

class CurveColorDialog : public DialogWindow, public ICustomControl
{
public:
  CurveColorDialog(void *phandle, const char caption[]);
  ~CurveColorDialog() override;

  void fillPanel(ContainerPropertyControl *panel);
  void updatePresets();

  void setColorStats(int r_channel[256], int g_channel[256], int b_channel[256]);
  void getColorTables(unsigned char r_table[256], unsigned char g_table[256], unsigned char b_table[256]);

  void setCorrectionInfo(const ColorCorrectionInfo &info);
  void getCorrectionInfo(ColorCorrectionInfo &info);

  static void loadPresets(const char *fn);
  static void savePresets(const char *fn);

private:
  // DialogWindow
  void onChange(int pcb_id, ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, ContainerPropertyControl *panel) override;
  void onDoubleClick(int pcb_id, ContainerPropertyControl *panel) override;
  void onPostEvent(int pcb_id, ContainerPropertyControl *panel) override;

  // ICustomControl
  void customControlUpdate(int id) override;

  ColorCurveControl *colorCurve;
  Tab<String> presets;
  static DataBlock presetsBlk;
  static bool showChartBars, showBaseLine;
};

} // namespace PropPanel