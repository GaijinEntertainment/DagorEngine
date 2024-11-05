//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>

namespace PropPanel
{

class ListDialog : public DialogWindow
{
public:
  ListDialog(const char *caption, const Tab<String> &vals, hdpi::Px width, hdpi::Px height);

  int getSelectedIndex() const;
  const char *getSelectedText();
  void setSelectedIndex(int index);

protected:
  virtual void onDoubleClick(int pcb_id, ContainerPropertyControl *panel) override;

private:
  SimpleString lastSelectText;
};

} // namespace PropPanel