#ifndef __GAIJIN_PREFABS_CLONE_DLG__
#define __GAIJIN_PREFABS_CLONE_DLG__
#pragma once

#include <propPanel2/comWnd/dialog_window.h>

class CopyDlg
{
public:
  CopyDlg(String &clone_name, int &clone_count, bool &clone_seed);
  ~CopyDlg();

  bool execute();

private:
  String &cloneName;
  int &cloneCount;
  bool &cloneSeed;

  CDialogWindow *mDialog;
};


#endif //__GAIJIN_PREFABS_CLONE_DLG__
