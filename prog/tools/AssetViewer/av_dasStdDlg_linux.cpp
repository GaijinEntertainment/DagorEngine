// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/misc/platform.h>
#include <winGuiWrapper/wgw_dialogs.h>

namespace das
{

void StdDlgInit() {}

bool GetOkCancelFromUser(const char *caption, const char *body)
{
  const int result = wingw::message_box(wingw::MBS_OKCANCEL, caption, body);
  return result == wingw::MB_ID_OK;
}

bool GetOkFromUser(const char *caption, const char *body)
{
  wingw::message_box(wingw::MBS_OK, caption, body);
  return true;
}

string GetSaveFileFromUser(const char *initialFileName, const char *initialPath, const char *filter)
{
  const String result = wingw::file_save_dlg(nullptr, "Select file", filter, "", initialPath, initialFileName);
  return string(result);
}

string GetOpenFileFromUser(const char *initialPath, const char *filter)
{
  const String result = wingw::file_open_dlg(nullptr, "Select file", filter, "", initialPath);
  return string(result);
}

string GetOpenFolderFromUser(const char *initialPath)
{
  const String result = wingw::dir_select_dlg(nullptr, "Select folder", initialPath);
  return string(result);
}

} // namespace das