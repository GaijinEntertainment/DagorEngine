//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <util/dag_safeArg.h>
#include <math/dag_e3dColor.h>


namespace wingw
{
enum
{
  MBS_OK = 0x00,
  MBS_OKCANCEL = 0x01,
  MBS_ABORTRETRYIGNORE = 0x02,
  MBS_YESNOCANCEL = 0x03,
  MBS_YESNO = 0x04,

  MBS_HAND = 0x10,
  MBS_QUEST = 0x20,
  MBS_EXCL = 0x30,
  MBS_INFO = 0x40,

  MB_ID_OK = 1,
  MB_ID_CANCEL = 2,
  MB_ID_ABORT = 3,
  MB_ID_RETRY = 4,
  MB_ID_IGNORE = 5,
  MB_ID_YES = 6,
  MB_ID_NO = 7,
};

enum
{
  MAXIMUM_PATH_LEN = 2048,
};

// Native dialogs and message boxes in winGuiWrapper use this interface to make it possible
// to easily do something before and after showing a dialog or message box.
class INativeModalDialogEventHandler
{
public:
  // Called before a modal dialog or message box is shown.
  virtual void beforeModalDialogShown() = 0;

  // Called after a modal dialog or message box has been shown.
  virtual void afterModalDialogShown() = 0;
};

// Internally it uses dakernel's named pointers, so the event handler will available from DLLs too.
void set_native_modal_dialog_events(INativeModalDialogEventHandler *event_handler);

String file_open_dlg(void *hwnd, const char caption[], const char filter[], const char def_ext[], const char init_path[] = "",
  const char init_fn[] = "");

String file_save_dlg(void *hwnd, const char caption[], const char filter[], const char def_ext[], const char init_path[] = "",
  const char init_fn[] = "");

String dir_select_dlg(void *hwnd, const char caption[], const char init_path[] = "");


bool select_color(void *hwnd, E3DCOLOR &value);

#define DSA_OVERLOADS_PARAM_DECL int flags, const char *caption,
#define DSA_OVERLOADS_PARAM_PASS flags, caption,
DECLARE_DSA_OVERLOADS_FAMILY(static inline int message_box, int message_box, return message_box);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS


enum
{
  SF_MATCH_WHOLE_WORDS = 0x01,
  SF_MATCH_CASE = 0x02,
  SF_REPLACE_ALL = 0x04,
};


bool search_dialog(void *hwnd, SimpleString &search_text, int &mask);
bool search_replace_dialog(void *hwnd, SimpleString &search_text, SimpleString &replace_text, int &mask);
} // namespace wingw
