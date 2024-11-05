// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_delayedAction.h>
#include <debug/dag_debug.h>
#import <UIKit/UIKit.h>

UIViewController *getMainController();
extern bool g_ios_pause_rendering;

static int os_message_box_impl(const char *utf8_text, const char *utf8_caption, int flags)
{
  int buttonCount = 0;
  const char *buttonNames[3] = {NULL};
  NSString *buttonKeys[3] = {@"\r", @"d", @"\e"};

  switch ( flags & 0xF )
  {
    case GUI_MB_OK:
      buttonCount     = 1;
      buttonNames [0] = "OK";
      break;

    case GUI_MB_OK_CANCEL:
      buttonCount     = 2;
      buttonNames [0] = "OK";
      buttonNames [1] = "Cancel";
      break;

    case GUI_MB_YES_NO:
      buttonCount     = 2;
      buttonNames [0] = "Yes";
      buttonNames [1] = "No";
      break;

    case GUI_MB_RETRY_CANCEL:
      buttonCount     = 2;
      buttonNames [0] = "Retry";
      buttonNames [1] = "Cancel";
      break;

    case GUI_MB_ABORT_RETRY_IGNORE:
      buttonCount     = 3;
      buttonNames [0] = "Abort";
      buttonNames [1] = "Retry";
      buttonNames [2] = "Ignore";
      break;

    case GUI_MB_YES_NO_CANCEL:
      buttonCount     = 3;
      buttonNames [0] = "Yes";
      buttonNames [1] = "No";
      buttonNames [2] = "Cancel";
      break;

    case GUI_MB_CANCEL_TRY_CONTINUE:
      buttonCount     = 3;
      buttonNames [0] = "Cancel";
      buttonNames [1] = "Try";
      buttonNames [2] = "Continue";
      break;

    default:
      buttonCount     = 1;
      buttonNames [0] = "OK";
      break;
  }

  UIAlertController* alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:utf8_caption]
                                 message:[NSString stringWithUTF8String:utf8_text]
                                 preferredStyle:UIAlertControllerStyleAlert];

  __block int pushedButton = -1;
  for (int buttonNo = 0; buttonNo < buttonCount; ++buttonNo)
  {
    UIAlertAction* defaultAction = [UIAlertAction actionWithTitle : [NSString stringWithUTF8String:buttonNames[buttonNo]]
                                                            style : UIAlertActionStyleDefault
                                                          handler : ^(UIAlertAction * action)
                                                                    {
                                                                      pushedButton = buttonNo;
                                                                      g_ios_pause_rendering = false;
                                                                    }];
    [alert addAction:defaultAction];
  }
  [getMainController() presentViewController:alert animated:YES completion:nil];
  g_ios_pause_rendering = true;

  NSRunLoop *theRL = [NSRunLoop mainRunLoop];
  G_ASSERT(theRL != nil);
  while (pushedButton == -1)
  {
    g_ios_pause_rendering = true;
    [theRL runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
  }

  return pushedButton + GUI_MB_BUTTON_1;
}

int os_message_box(const char *utf8_text, const char *utf8_caption, int flags)
{
  debug("%s(%s, %s, %d)", __FUNCTION__, utf8_text, utf8_caption, flags);

  if (utf8_text == NULL || utf8_caption == NULL)
  {
    debug("%s no text or caption", __FUNCTION__);
    return GUI_MB_FAIL;
  }
  if (is_main_thread())
    return os_message_box_impl(utf8_text, utf8_caption, flags);

  debug("trying to show messahe box on main thread");
  struct MessageBoxAction: public DelayedAction
  {
    char text[4096], caption[512];
    int flags;
    int &result;
    MessageBoxAction(const char *utf8_text, const char *utf8_caption, int flg, int &out_res): flags(flg), result(out_res)
    {
      strncpy(text, utf8_text, sizeof(text)-1); text[sizeof(text)-1] = 0;
      strncpy(caption, utf8_caption, sizeof(caption)-1); caption[sizeof(caption)-1] = 0;
    }
    virtual void performAction() { result = os_message_box_impl(text, caption, flags); }
  };
  int res = 0;
  execute_delayed_action_on_main_thread(new MessageBoxAction(utf8_text, utf8_caption, flags, res));
  return res;
}
