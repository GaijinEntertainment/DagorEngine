// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <generic/dag_tab.h>


enum
{
  FIRST_CHECK_ID = 1000,
};


class ExportToDagDlg : public PropPanel::DialogWindow
{
public:
  ExportToDagDlg(Tab<String> &sel_names, bool visual, const char *name);

  void getSelPlugins(Tab<int> &sel);

private:
  PropPanel::ContainerPropertyControl *mDPanel;
  Tab<unsigned> plugs;
};
