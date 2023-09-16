//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <propPanel2/comWnd/dialog_window.h>


class SearchReplaceDialog : public CDialogWindow
{
public:
  SearchReplaceDialog(void *handle, int &flags, SimpleString &search_text, SimpleString &replace_text, bool is_search_dialog = true);
  ~SearchReplaceDialog() {}

  virtual bool onOk();

  static const char *getSearchText() { return searchTextLast.str(); }
  static int getLastFlags() { return flagsLast; }

private:
  bool isSearchDialog;
  SimpleString &searchText, &replaceText;
  static SimpleString searchTextLast, replaceTextLast;
  int &sFlags;
  static int flagsLast;
};
