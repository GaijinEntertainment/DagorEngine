//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

//
// Show message box window with hyperlinks in text.
// Mark hyperlinks like HTML <A> tag.
//
// utf8_text = "<A HREF="url">title</A>"
// utf8_text = "Visit <A HREF=\"http://gaijin.ru\">Gaijin</A> for more information"
//
KRNLIMP int os_message_box(const char *utf8_text, const char *utf8_caption, int flags = 0);

#include <supp/dag_undef_KRNLIMP.h>


// Flags for message_box.
// This parameter can be combination of flags from following groups.
enum
{
  GUI_MB_OK = 0,                    // one   push button:  OK
  GUI_MB_OK_CANCEL = 0x1,           // two   push buttons: OK and Cancel
  GUI_MB_YES_NO = 0x2,              // two   push buttons: Yes and No
  GUI_MB_RETRY_CANCEL = 0x3,        // two   push buttons: Retry and Cancel
  GUI_MB_ABORT_RETRY_IGNORE = 0x4,  // three push buttons: Abort, Retry, and Ignore
  GUI_MB_YES_NO_CANCEL = 0x5,       // three push buttons: Yes, No, and Cancel
  GUI_MB_CANCEL_TRY_CONTINUE = 0x6, // three push buttons: Cancel, Try Again, Continue.

  GUI_MB_DEF_BUTTON_1 = 0,    // first button is default
  GUI_MB_DEF_BUTTON_2 = 0x10, // second button is the default
  GUI_MB_DEF_BUTTON_3 = 0x20, // third button is the default

  GUI_MB_ICON_ERROR = 0x100,       // error icon appears
  GUI_MB_ICON_WARNING = 0x200,     // warning icon appears
  GUI_MB_ICON_QUESTION = 0x300,    // question icon appears
  GUI_MB_ICON_INFORMATION = 0x400, // information icon appears

  GUI_MB_TOPMOST = 0x1000,    // top most window
  GUI_MB_FOREGROUND = 0x2000, // foreground window
  GUI_MB_NATIVE_DLG = 0x4000, // use native OS dialog if possible (e.g., MessageBox for Win API)
};


// Return values for message_box.
enum
{
  GUI_MB_FAIL = -1,    // fail when creating window
  GUI_MB_CLOSE = 0,    // window just closed
  GUI_MB_BUTTON_1 = 1, // first  button pressed
  GUI_MB_BUTTON_2 = 2, // second button pressed
  GUI_MB_BUTTON_3 = 3  // third  button pressed
};
