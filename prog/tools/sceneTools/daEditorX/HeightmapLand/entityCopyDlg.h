// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>

namespace PropPanel
{
class DialogWindow;
}

namespace heightmapland
{
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

  PropPanel::DialogWindow *mDialog;
};
} // namespace heightmapland
