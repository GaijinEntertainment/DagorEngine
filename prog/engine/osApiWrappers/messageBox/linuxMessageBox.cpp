#include <osApiWrappers/dag_messageBox.h>
#include <debug/dag_debug.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

int os_message_box(const char *utf8_text, const char *utf8_caption, int flags)
{
  debug("%s(%s, %s, %d)", __FUNCTION__, utf8_text, utf8_caption, flags);
  if (utf8_text == NULL || utf8_caption == NULL)
  {
    debug("%s no text or caption", __FUNCTION__);
    return GUI_MB_FAIL;
  }

  fl_message_title(utf8_caption);
  int buttonCount = 0;
  const char *buttonNames[3] = {NULL};

  switch (flags & 0xF)
  {
    case GUI_MB_OK:
      buttonCount = 1;
      buttonNames[0] = "OK";
      break;

    case GUI_MB_OK_CANCEL:
      buttonCount = 2;
      buttonNames[0] = "OK";
      buttonNames[1] = "Cancel";
      break;

    case GUI_MB_YES_NO:
      buttonCount = 2;
      buttonNames[0] = "Yes";
      buttonNames[1] = "No";
      break;

    case GUI_MB_RETRY_CANCEL:
      buttonCount = 2;
      buttonNames[0] = "Retry";
      buttonNames[1] = "Cancel";
      break;

    case GUI_MB_ABORT_RETRY_IGNORE:
      buttonCount = 3;
      buttonNames[0] = "Abort";
      buttonNames[1] = "Retry";
      buttonNames[2] = "Ignore";
      break;

    case GUI_MB_YES_NO_CANCEL:
      buttonCount = 3;
      buttonNames[0] = "Yes";
      buttonNames[1] = "No";
      buttonNames[2] = "Cancel";
      break;

    case GUI_MB_CANCEL_TRY_CONTINUE:
      buttonCount = 3;
      buttonNames[0] = "Cancel";
      buttonNames[1] = "Try";
      buttonNames[2] = "Continue";
      break;

    default:
      buttonCount = 1;
      buttonNames[0] = "OK";
      break;
  }

  int ret = GUI_MB_FAIL;
  if (buttonCount == 3)
  {
    ret = fl_choice("%s", buttonNames[1], buttonNames[0], buttonNames[2], utf8_text);
    ret = ret == 1 ? GUI_MB_BUTTON_1 : (ret == 0 ? GUI_MB_BUTTON_2 : GUI_MB_BUTTON_3);
  }
  else if (buttonCount == 2)
  {
    ret = fl_choice("%s", buttonNames[1], buttonNames[0], NULL, utf8_text);
    ret = ret == 1 ? GUI_MB_BUTTON_1 : GUI_MB_BUTTON_2;
  }
  else if (buttonCount == 1)
  {
    fl_choice("%s", buttonNames[0], NULL, NULL, utf8_text);
    ret = GUI_MB_BUTTON_1;
  }
  Fl::flush();
  return ret;
}
