#ifndef __GAIJIN_DAGORED_EXPORT_TO_DAG_DLG__
#define __GAIJIN_DAGORED_EXPORT_TO_DAG_DLG__
#pragma once

#include <propPanel2/comWnd/dialog_window.h>
#include <generic/dag_tab.h>


enum
{
  FIRST_CHECK_ID = 1000,
};


class ExportToDagDlg : public CDialogWindow
{
public:
  ExportToDagDlg(Tab<String> &sel_names, bool visual, const char *name);

  void getSelPlugins(Tab<int> &sel);

private:
  PropertyContainerControlBase *mDPanel;
  Tab<unsigned> plugs;
};


#endif
