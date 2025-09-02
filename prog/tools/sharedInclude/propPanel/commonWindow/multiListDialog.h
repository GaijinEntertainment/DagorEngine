//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <generic/dag_tab.h>

namespace PropPanel
{

class MultiListDialog : public DialogWindow
{
public:
  MultiListDialog(const char *caption, hdpi::Px width, hdpi::Px height, const Tab<String> &vals, Tab<String> &sels);

  bool onOk() override;

  void setSelectionTab(Tab<int> *sels);

protected:
  void onClick(int pcb_id, ContainerPropertyControl *panel) override;
  void onDoubleClick(int pcb_id, ContainerPropertyControl *panel) override;

private:
  Tab<String> *mSels;
  Tab<int> *mSelsIndTab;
};

} // namespace PropPanel