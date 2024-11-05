// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>

class HmapExportSizeDlg : public PropPanel::ControlEventHandler
{
public:
  HmapExportSizeDlg(float &min_height, float &height_range, float min_height_hm, float height_range_hm);

  ~HmapExportSizeDlg();

  virtual bool execute();
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);

private:
  float &minHeight;
  float &heightRange;

  float minHeightHm;
  float heightRangeHm;

  PropPanel::DialogWindow *mDialog;
};
