#ifndef __GAIJIN_ENVIRONMENT_HDR_SETTINGS_DLG__
#define __GAIJIN_ENVIRONMENT_HDR_SETTINGS_DLG__
#pragma once


#include <propPanel2/comWnd/dialog_window.h>

class DataBlock;


class HdrViewSettingsDialog
{
public:
  HdrViewSettingsDialog(DataBlock *hdr_blk, bool if_aces_plugin = false);
  ~HdrViewSettingsDialog();

  int showDialog();

  bool isLevelReloadRequired() { return levelReloadRequired; }
  bool isHdrModeNone() { return hdrModeNone; }

private:
  CDialogWindow *dlg;
  DataBlock *hdrBlk;

  bool levelReloadRequired;
  bool hdrModeNone;
  bool isAcesEnvironment;
};

#endif //__GAIJIN_ENVIRONMENT_HDR_SETTINGS_DLG__
