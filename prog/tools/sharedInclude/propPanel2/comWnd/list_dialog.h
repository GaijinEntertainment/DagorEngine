//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <propPanel2/comWnd/dialog_window.h>
#include <propPanel2/c_window_event_handler.h>
#include <generic/dag_tab.h>


class WContainer;
class WListBox;

class ListDialog : public WindowControlEventHandler, public CDialogWindow
{
public:
  ListDialog(void *handle, const char *caption, const Tab<String> &vals, hdpi::Px width, hdpi::Px height);
  ~ListDialog();

  int getSelectedIndex() const;
  void setSelectedIndex(int index);
  const char *getSelectedText();

protected:
  virtual void onWcDoubleClick(WindowBase *source);

private:
  char mLastSelectText[256];
  WContainer *mContainer;
  WListBox *mList;
};


class MultiListDialog : public CDialogWindow
{
public:
  MultiListDialog(const char *caption, hdpi::Px width, hdpi::Px height, const Tab<String> &vals, Tab<String> &sels);

  virtual bool onOk();

  void setSelectionTab(Tab<int> *sels);

protected:
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onDoubleClick(int pcb_id, PropertyContainerControlBase *panel);

private:
  Tab<String> *mSels;
  Tab<int> *mSelsIndTab;
};
