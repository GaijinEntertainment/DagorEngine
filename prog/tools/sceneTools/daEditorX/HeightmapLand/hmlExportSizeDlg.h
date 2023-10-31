#ifndef __GAIJIN_HMAP_EXPORT_SIZE_DLG__
#define __GAIJIN_HMAP_EXPORT_SIZE_DLG__
#pragma once

#include <propPanel2/comWnd/dialog_window.h>

class HmapExportSizeDlg : public ControlEventHandler
{
public:
  HmapExportSizeDlg(float &min_height, float &height_range, float min_height_hm, float height_range_hm);

  ~HmapExportSizeDlg();

  virtual bool execute();
  virtual void onClick(int pcb_id, PropPanel2 *panel);

private:
  float &minHeight;
  float &heightRange;

  float minHeightHm;
  float heightRangeHm;

  CDialogWindow *mDialog;
};


#endif //__GAIJIN_HMAP_EXPORT_SIZE_DLG__
