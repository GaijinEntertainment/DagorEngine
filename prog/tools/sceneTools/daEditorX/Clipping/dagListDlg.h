#ifndef __GAIJIN_CLIP_DAGLIST_DLG__
#define __GAIJIN_CLIP_DAGLIST_DLG__
#pragma once

#include <generic/dag_tab.h>
#include <propPanel2/comWnd/dialog_window.h>

class String;
class PropertyContainerControlBase;

class DagListDlg : public CDialogWindow
{
public:
  DagListDlg(Tab<String> &dag_files);
  ~DagListDlg();

private:
  PropertyContainerControlBase *mPanel;
};

#endif //__GAIJIN_CLIP_DAGLIST_DLG__
