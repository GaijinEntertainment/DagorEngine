// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <UIKit/UIKit.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_simpleString.h>
#include <util/dag_delayedAction.h>
#include <debug/dag_debug.h>

static bool canBeVisible = false, canBeHidden = false;

static void (*ime_finish_cb)(void *userdata, const char *str, int cursor, int status) = NULL;
static void *ime_finish_userdata = NULL;
static bool ime_started = false;


static void onFinishEdit_IME(const char *text, int cursorPos)
{
  if (ime_finish_cb)
  {
    struct FinishAction : public DelayedAction
    {
      void (*finish_cb)(void *userdata, const char *str, int _cursor, int status);
      void *finish_userdata;
      SimpleString utf8;
      bool confirm;
      int cursor;

      FinishAction(const char *text_utf8, int _cursor, bool _confirm)
      {
        utf8 = text_utf8;
        finish_cb = ime_finish_cb;
        finish_userdata = ime_finish_userdata;
        confirm = _confirm;
        cursor = _cursor;
      }

      virtual void performAction()
      {
        finish_cb(finish_userdata, utf8, cursor, confirm);
      }
    };

    FinishAction *a = new FinishAction(text, cursorPos, 1);

    ime_started = false;
    ime_finish_cb = NULL;
    ime_finish_userdata = NULL;
    add_delayed_action(a);
  }
}

bool HumanInput::isImeAvailable() { return true; }
bool HumanInput::showScreenKeyboard_IME(const DataBlock &init_params, void(on_finish_cb)(void *userdata, const char *str, int cursor, int status),
  void *userdata)
{
  if (!init_params.paramCount() && !on_finish_cb && userdata)
  {
    if (ime_finish_cb && userdata == ime_finish_userdata)
    {
      ime_finish_cb = NULL;
      ime_finish_userdata = NULL;
      showScreenKeyboard(false);
      ime_started = false;
      return true;
    }
    return false;
  }
  if (ime_finish_cb && userdata != ime_finish_userdata)
  {
    ime_finish_cb = NULL;
    ime_finish_userdata = NULL;
    showScreenKeyboard(false);
    ime_started = false;
  }
  if (ime_started)
  {
    logwarn("IME double start prevented");
    return true;
  }

  ime_finish_cb = on_finish_cb;
  ime_finish_userdata = userdata;
  ime_started = true;

  unsigned int edit_flag = UIKeyboardTypeDefault;
  unsigned int ime_flag = UIReturnKeyDefault;
  bool secure_flag = false;

  if (const char *type = init_params.getStr("type", NULL))
  {
    if (strcmp(type, "lat") == 0)
      edit_flag = UIKeyboardTypeASCIICapable;
    else if (strcmp(type, "url") == 0)
      edit_flag = UIKeyboardTypeURL;
    else if (strcmp(type, "mail") == 0)
      edit_flag = UIKeyboardTypeEmailAddress;
    else if (strcmp(type, "num") == 0)
      edit_flag = UIKeyboardTypeASCIICapableNumberPad;
    else
      logerr("unrecognized IME type: <%s>", type);
  }
  if (const char *type = init_params.getStr("label", NULL))
  {
    if (strcmp(type, "send") == 0)
      ime_flag = UIReturnKeySend;
    else if (strcmp(type, "search") == 0)
      ime_flag = UIReturnKeySearch;
    else if (strcmp(type, "go") == 0)
      ime_flag = UIReturnKeyGo;
    else
      logerr("unrecognized IME label: <%s>", type);
 }
  if (init_params.getBool("optMultiLine", false))
    ;//todo, currently one line
  if (init_params.getBool("optPassw", false))
    secure_flag = true;
  if (init_params.getBool("optNoCopy", false))
    ; // n/a
  if (init_params.getBool("optFixedPos", false))
    ; // n/a
  if (!init_params.getBool("optNoAutoCap", false))
    ; //todo, currently disable
  if (init_params.getBool("optNoLearning", false))
    ; //todo, currently disable
  int max_chars = init_params.getInt("maxChars", 128);

  int cursor_pos = init_params.getInt("optCursorPos", -1);

  //debug("show: str=<%s> hint=<%s> flg=%08X ime=%08X (%s,%s)", init_params.getStr("str"), init_params.getStr("hint", ""), edit_flags,
    //ime_flags, init_params.getStr("type", NULL), init_params.getStr("label", NULL));
  showScreenKeyboardIME_iOS(init_params.getStr("str"), init_params.getStr("hint", ""), ime_flag, edit_flag, cursor_pos, secure_flag, max_chars, onFinishEdit_IME );
  return true;
}
