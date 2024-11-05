// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace PropPanel
{
class DialogWindow;
}
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
  PropPanel::DialogWindow *dlg;
  DataBlock *hdrBlk;

  bool levelReloadRequired;
  bool hdrModeNone;
  bool isAcesEnvironment;
};
