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


static void onFinishEdit_IME(const char *text, int cursorPos, int status)
{
  if (ime_finish_cb)
  {
    struct FinishAction : public DelayedAction
    {
      void (*finish_cb)(void *userdata, const char *str, int _cursor, int status);
      void *finish_userdata;
      SimpleString utf8;
      int status;
      int cursor;

      FinishAction(const char *text_utf8, int _cursor, int _status)
      {
        utf8 = text_utf8;
        finish_cb = ime_finish_cb;
        finish_userdata = ime_finish_userdata;
        status = _status;
        cursor = _cursor;
      }

      virtual void performAction() { finish_cb(finish_userdata, utf8, cursor, status); }
    };

    FinishAction *a = new FinishAction(text ? text : "", cursorPos, status);

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

  ScreenKeyboardParamsIOS params;
  bool isTextClass = true;     // same rules as on Android: auto capitalization only for text, not numbers
  bool noAutoCapByType = false; // and not for url, e-mail and password

  if (const char *type = init_params.getStr("type", NULL))
  {
    if (strcmp(type, "lat") == 0)
      params.keyboardType = UIKeyboardTypeASCIICapable;
    else if (strcmp(type, "url") == 0)
    {
      params.keyboardType = UIKeyboardTypeURL;
      noAutoCapByType = true;
    }
    else if (strcmp(type, "mail") == 0)
    {
      params.keyboardType = UIKeyboardTypeEmailAddress;
      params.contentType = ScreenKeyboardParamsIOS::CONTENT_EMAIL;
      noAutoCapByType = true;
    }
    else if (strcmp(type, "num") == 0)
    {
      params.keyboardType = UIKeyboardTypeASCIICapableNumberPad;
      isTextClass = false;
    }
    else if (strcmp(type, "integer") == 0 || strcmp(type, "float") == 0)
    {
      params.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
      isTextClass = false;
    }
    else
      logerr("unrecognized IME type: <%s>", type);
  }
  if (const char *type = init_params.getStr("label", NULL))
  {
    if (strcmp(type, "send") == 0)
      params.returnKeyType = UIReturnKeySend;
    else if (strcmp(type, "search") == 0)
      params.returnKeyType = UIReturnKeySearch;
    else if (strcmp(type, "go") == 0)
      params.returnKeyType = UIReturnKeyGo;
    else
      logerr("unrecognized IME label: <%s>", type);
  }
  if (init_params.getBool("optMultiLine", false))
    ; // todo, currently one line
  if (init_params.getBool("optPassw", false))
  {
    params.secure = true;
    params.contentType = ScreenKeyboardParamsIOS::CONTENT_PASSWORD;
    noAutoCapByType = true;
  }
  if (init_params.getBool("optNoCopy", false))
    ; // n/a
  if (init_params.getBool("optFixedPos", false))
    ; // n/a
  params.autocapitalizationType = (isTextClass && !init_params.getBool("optNoAutoCap", noAutoCapByType))
                                    ? UITextAutocapitalizationTypeSentences
                                    : UITextAutocapitalizationTypeNone;
  if (init_params.getBool("optNoLearning", false))
    ; // n/a: autocorrection is always off, iOS has no "suggestions without autocorrection" mode like Android
  params.maxChars = init_params.getInt("maxChars", 128);
  params.cursorPos = init_params.getInt("optCursorPos", -1);
  params.text = init_params.getStr("str", "");
  params.hint = init_params.getStr("hint", "");

  ime_finish_cb = on_finish_cb;
  ime_finish_userdata = userdata;
  ime_started = true;
  if (!showScreenKeyboardIME_iOS(params, onFinishEdit_IME))
  {
    ime_finish_cb = NULL;
    ime_finish_userdata = NULL;
    ime_started = false;
    return false;
  }
  return true;
}
