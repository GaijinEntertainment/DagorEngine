// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <propPanel/commonWindow/dialogWindow.h>

class String;

class DagListDlg : public PropPanel::DialogWindow
{
public:
  DagListDlg(Tab<String> &dag_files);
  ~DagListDlg();

private:
  PropPanel::ContainerPropertyControl *mPanel;
};
