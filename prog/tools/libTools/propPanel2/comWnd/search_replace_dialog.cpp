// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/comWnd/search_replace_dialog.h>
#include <winGuiWrapper/wgw_dialogs.h>


SimpleString SearchReplaceDialog::searchTextLast;
SimpleString SearchReplaceDialog::replaceTextLast;
int SearchReplaceDialog::flagsLast = 0;

enum
{
  ID_FIND_EDIT_BOX,
  ID_REPLACE_EDIT_BOX,

  ID_REPLACE_ALL_CHECK,
  ID_MATCH_WHOLE_WORDS_CHECK,
  ID_MATCH_CASE_CHECK,
};


enum
{
  DIALOG_WIDTH = 400,
  DIALOG_HEIGHT = 300,
};


SearchReplaceDialog::SearchReplaceDialog(void *handle, int &flags, SimpleString &search_text, SimpleString &replace_text,
  bool is_search_dialog) :
  CDialogWindow(handle, DIALOG_WIDTH, DIALOG_HEIGHT, is_search_dialog ? "Search" : "Replace"),
  sFlags(flags),
  isSearchDialog(is_search_dialog),
  searchText(search_text),
  replaceText(replace_text)
{
  sFlags = flagsLast;
  if (searchText.empty())
    searchText = searchTextLast;
  if (replaceText.empty())
    replaceText = replaceTextLast;

  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "SearchReplaceDialog: No panel found!");


  _panel->createEditBox(ID_FIND_EDIT_BOX, "Find what: ", searchText.str());
  if (!is_search_dialog)
  {
    _panel->createEditBox(ID_REPLACE_EDIT_BOX, "Replace with: ", replaceText.str());
    _panel->createCheckBox(ID_REPLACE_ALL_CHECK, "Replace all", sFlags & wingw::SF_REPLACE_ALL);
  }

  _panel->createCheckBox(ID_MATCH_WHOLE_WORDS_CHECK, "Match whole words", sFlags & wingw::SF_MATCH_WHOLE_WORDS);
  _panel->createCheckBox(ID_MATCH_CASE_CHECK, "Match case", sFlags & wingw::SF_MATCH_CASE);
}


bool SearchReplaceDialog::onOk()
{
  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "SearchReplaceDialog: NO PANEL FOUND!!!");

  sFlags = 0;
  searchTextLast = searchText = _panel->getText(ID_FIND_EDIT_BOX);
  if (!isSearchDialog)
  {
    replaceTextLast = replaceText = _panel->getText(ID_REPLACE_EDIT_BOX);
    if (_panel->getBool(ID_REPLACE_ALL_CHECK))
      sFlags = sFlags | wingw::SF_REPLACE_ALL;
  }

  if (_panel->getBool(ID_MATCH_WHOLE_WORDS_CHECK))
    sFlags = sFlags | wingw::SF_MATCH_WHOLE_WORDS;
  if (_panel->getBool(ID_MATCH_CASE_CHECK))
    sFlags = sFlags | wingw::SF_MATCH_CASE;

  flagsLast = sFlags;
  return true;
}