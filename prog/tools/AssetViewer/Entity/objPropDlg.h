// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_window_event_handler.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <generic/dag_tab.h>
#include <string>

class ObjPropDialog : public PropPanel::DialogWindow
{
public:
  ObjPropDialog(const char *caption, hdpi::Px width, hdpi::Px height, const Tab<String> &nodes, const Tab<String> &scripts);

  bool onOk() override;

  const Tab<String> &getScripts() const { return scripts; }

protected:
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void applyChanges(const Tab<int> &sels);

private:
  Tab<String> scripts;
  Tab<int> prevSels;
  bool changed = false, shouldSave = false;
};
