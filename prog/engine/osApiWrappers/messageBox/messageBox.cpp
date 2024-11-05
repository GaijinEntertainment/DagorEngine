// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include <EASTL/unique_ptr.h>
#include <stdlib.h>
#include <windows.h>

#define TOOLS_DAKERNEL (_TARGET_PC_WIN && !_TARGET_STATIC_LIB)

static void minimizeMainWindow()
{
  if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
  {
    HWND hwnd = (HWND)win32_get_main_wnd();
    if (hwnd)
      ShowWindow(hwnd, (is_main_thread() ? SW_MINIMIZE : SW_FORCEMINIMIZE)); // Exit fullscreen.
  }
}


#if !TOOLS_DAKERNEL
#include <commctrl.h>
#include <osApiWrappers/dag_shellExecute.h>
#pragma comment(lib, "comctl32.lib")

static bool mb_init = false;  // already initialized
static int mb_button_pressed; // which button was pressed

// Process messages.
static LRESULT CALLBACK windowProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
  switch (msg)
  {
    case WM_COMMAND:
      if (HIWORD(w_param) == BN_CLICKED)
      {
        // button pressed
        mb_button_pressed = LOWORD(w_param);
      }
      break;

    case WM_NOTIFY:
      PNMLINK link = PNMLINK(l_param);
      if (link == NULL)
        break;

      switch (link->hdr.code)
      {
        case NM_CLICK:
        case NM_RETURN:
        {
          // link pressed
          if (link->item.iLink == 0)
          {
            os_shell_execute_w(L"open", link->item.szUrl, NULL, NULL);
          }
        }
        break;
      }
      break;
  }

  return ::DefWindowProcW(wnd, msg, w_param, l_param);
}


template <typename T, class F, F release>
struct MBDeleter
{
  typedef T pointer;
  void operator()(T p) const
  {
    if (p)
      release(p);
  }
};

// Show message box window with hyperlinks.
static int messageBoxHyperlinks(const wchar_t *text, const wchar_t *caption, int flags)
{
  const char *const FUNC_NAME = __FUNCTION__;
  auto fail = [FUNC_NAME](const char *msg) {
    G_UNUSED(msg);
    logwarn("%s return GUI_MB_FAIL: %s failed, err=%d", FUNC_NAME, msg, ::GetLastError());
    return GUI_MB_FAIL;
  };

  HINSTANCE instance = (HINSTANCE)win32_get_instance();
  if (!instance)
  {
    instance = ::GetModuleHandle(NULL);

    if (!instance)
    {
      return fail("GetModuleHandle");
    }
  }

  if (!mb_init)
  {
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_LINK_CLASS;
    if (!::InitCommonControlsEx(&iccex))
    {
      return fail("InitCommonControlsEx");
    }

    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WNDPROC(windowProc);
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = NULL;
    wcex.hIconSm = NULL;
    wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = HBRUSH(COLOR_WINDOW);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"gui_message_box";
    if (!::RegisterClassExW(&wcex))
    {
      return fail("RegisterClassExW");
    }

    mb_init = true;
  }

  minimizeMainWindow();

  // create font
  eastl::unique_ptr<void, MBDeleter<HFONT, decltype(::DeleteObject), ::DeleteObject>> font(::CreateFont(20, 0, 0, 0, FW_NORMAL, 0, 0,
    0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "MS Sans Serif"));
  if (!font)
  {
    return fail("CreateFont");
  }

  // create window
  const int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
  const int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
  const int windowWidth = 600;
  const int windowHeight = 400;
  const int windowX = (screenWidth - windowWidth) / 2;
  const int windowY = (screenHeight - windowHeight) / 2;
  const bool topMost = ((flags & GUI_MB_TOPMOST) == GUI_MB_TOPMOST);

  eastl::unique_ptr<void, MBDeleter<HWND, decltype(::DestroyWindow), ::DestroyWindow>> window(
    ::CreateWindowExW(WS_EX_DLGMODALFRAME | (topMost ? WS_EX_TOPMOST : 0), L"gui_message_box", caption,
      WS_CAPTION | WS_BORDER | WS_SYSMENU, windowX, windowY, windowWidth, windowHeight, NULL, NULL, instance, NULL));
  if (!window)
  {
    return fail("CreateWindowExW");
  }

  const int iconFlags = flags & 0xF00;
  if (iconFlags)
  {
    LPCSTR iconName = NULL;
    switch (iconFlags)
    {
      case GUI_MB_ICON_ERROR: iconName = IDI_ERROR; break;
      case GUI_MB_ICON_WARNING: iconName = IDI_WARNING; break;
      case GUI_MB_ICON_QUESTION: iconName = IDI_QUESTION; break;
      case GUI_MB_ICON_INFORMATION: iconName = IDI_INFORMATION; break;
    }

    ::SendMessageW(window.get(), WM_SETICON, ICON_BIG, (LPARAM)::LoadIconA(NULL, iconName));
    ::SendMessageW(window.get(), WM_SETICON, ICON_SMALL, (LPARAM)::LoadIconA(NULL, iconName));
  }

  // create text
  const int textX = 16;
  const int textY = 16;
  const int textWidth = windowWidth - textX - 2 * textX;
  const int textHeight = windowHeight - 80;
  HWND link = ::CreateWindowExW(0, WC_LINK, text, WS_VISIBLE | WS_CHILD | WS_TABSTOP, textX, textY, textWidth, textHeight,
    window.get(), NULL, instance, NULL);
  if (link == NULL)
  {
    return fail("create link");
  }

  ::SendMessageW(link, WM_SETFONT, WPARAM(font.get()), false);

  // create buttons
  int buttonCount;
  wchar_t const *buttonNames[4];

  switch (flags & 0xF)
  {
    case GUI_MB_OK:
      buttonCount = 1;
      buttonNames[0] = L"OK";
      break;

    case GUI_MB_OK_CANCEL:
      buttonCount = 2;
      buttonNames[0] = L"OK";
      buttonNames[1] = L"Cancel";
      break;

    case GUI_MB_YES_NO:
      buttonCount = 2;
      buttonNames[0] = L"Yes";
      buttonNames[1] = L"No";
      break;

    case GUI_MB_RETRY_CANCEL:
      buttonCount = 2;
      buttonNames[0] = L"Retry";
      buttonNames[1] = L"Cancel";
      break;

    case GUI_MB_ABORT_RETRY_IGNORE:
      buttonCount = 3;
      buttonNames[0] = L"Abort";
      buttonNames[1] = L"Retry";
      buttonNames[2] = L"Ignore";
      break;

    case GUI_MB_YES_NO_CANCEL:
      buttonCount = 3;
      buttonNames[0] = L"Yes";
      buttonNames[1] = L"No";
      buttonNames[2] = L"Cancel";
      break;

    case GUI_MB_CANCEL_TRY_CONTINUE:
      buttonCount = 3;
      buttonNames[0] = L"Cancel";
      buttonNames[1] = L"Try";
      buttonNames[2] = L"Continue";
      break;

    default: buttonCount = 0; break;
  }

  int buttonDef;
  switch (flags & 0xF0)
  {
    case GUI_MB_DEF_BUTTON_2: buttonDef = 1; break;
    case GUI_MB_DEF_BUTTON_3: buttonDef = 2; break;
    default: buttonDef = 0; break;
  }

  for (int i = 0; i < buttonCount; i++)
  {
    const int buttonWidth = 128;
    const int buttonDiv = 64;
    const int buttonHeight = 32;

    const int buttonX = (windowWidth - buttonCount * (buttonWidth + buttonDiv) + buttonDiv) / 2 + i * (buttonWidth + buttonDiv);
    const int buttonY = windowHeight - 80;

    const HWND button = ::CreateWindowExW(0, L"BUTTON", buttonNames[i],
      BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP | ((buttonDef == i) ? BS_DEFPUSHBUTTON : 0), buttonX, buttonY, buttonWidth,
      buttonHeight, window.get(), HMENU((intptr_t)i), instance, NULL);
    if (button == NULL)
    {
      return fail("create button");
    }

    ::SendMessageW(button, WM_SETFONT, WPARAM(HFONT(font.get())), false);
  }

  ::ShowWindow(window.get(), SW_SHOW);
  ::UpdateWindow(window.get());

  if ((flags & GUI_MB_FOREGROUND) == GUI_MB_FOREGROUND)
    SetForegroundWindow(window.get());

  // use GetMessage() to avoid eating CPU time
  mb_button_pressed = -1;
  MSG msg;
  for (;;)
  {
    const BOOL ret = ::GetMessageW(&msg, window.get(), 0, 0);
    if (ret == 0 || ret == -1 || mb_button_pressed != -1)
      break;

    ::TranslateMessage(&msg);
    ::DispatchMessageW(&msg);
  }

  int result = (mb_button_pressed != -1) ? (GUI_MB_BUTTON_1 + mb_button_pressed) : GUI_MB_CLOSE;
  debug("%s return %d", __FUNCTION__, result);
  return result;
}
#endif // TOOLS_DAKERNEL

// Show message box window without hyperlinks.
static int messageBoxNative(const wchar_t *text, const wchar_t *caption, int flags)
{
  minimizeMainWindow();

  int winFlags = 0;
  switch (flags & 0xF)
  {
    case GUI_MB_OK: winFlags = MB_OK; break;
    case GUI_MB_OK_CANCEL: winFlags = MB_OKCANCEL; break;
    case GUI_MB_YES_NO: winFlags = MB_YESNO; break;
    case GUI_MB_RETRY_CANCEL: winFlags = MB_RETRYCANCEL; break;
    case GUI_MB_ABORT_RETRY_IGNORE: winFlags = MB_ABORTRETRYIGNORE; break;
    case GUI_MB_YES_NO_CANCEL: winFlags = MB_YESNOCANCEL; break;
    case GUI_MB_CANCEL_TRY_CONTINUE: winFlags = MB_CANCELTRYCONTINUE; break;
  }
  switch (flags & 0xF0)
  {
    case GUI_MB_DEF_BUTTON_2: winFlags |= MB_DEFBUTTON2; break;
    case GUI_MB_DEF_BUTTON_3: winFlags |= MB_DEFBUTTON3; break;
    default: winFlags |= MB_DEFBUTTON1; break;
  }
  switch (flags & 0xF00)
  {
    case GUI_MB_ICON_ERROR: winFlags |= MB_ICONERROR; break;
    case GUI_MB_ICON_WARNING: winFlags |= MB_ICONWARNING; break;
    case GUI_MB_ICON_QUESTION: winFlags |= MB_ICONQUESTION; break;
    case GUI_MB_ICON_INFORMATION: winFlags |= MB_ICONINFORMATION; break;
  }
  if ((flags & GUI_MB_TOPMOST) == GUI_MB_TOPMOST)
    winFlags |= MB_TOPMOST;
  if ((flags & GUI_MB_FOREGROUND) == GUI_MB_FOREGROUND)
    winFlags |= MB_SETFOREGROUND;

  HWND mainWnd = ::is_main_thread() && dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE ? (HWND)win32_get_main_wnd() : 0;
  const int winResult = ::MessageBoxW(mainWnd, text, caption, winFlags);

  switch (winResult)
  {
    case IDOK:
    case IDYES:
    case IDABORT: return GUI_MB_BUTTON_1;

    case IDNO:
    case IDTRYAGAIN: return GUI_MB_BUTTON_2;

    case IDCANCEL:
      switch (flags & 0xF)
      {
        case GUI_MB_OK_CANCEL:
        case GUI_MB_RETRY_CANCEL: return GUI_MB_BUTTON_2;
        case GUI_MB_YES_NO_CANCEL: return GUI_MB_BUTTON_3;
      }

    case IDIGNORE:
    case IDCONTINUE: return GUI_MB_BUTTON_3;

    case IDRETRY:
    {
      int btnType = winFlags & MB_TYPEMASK;
      if (btnType == MB_ABORTRETRYIGNORE)
        return GUI_MB_BUTTON_2;
      else if (btnType == MB_RETRYCANCEL)
        return GUI_MB_BUTTON_1;
    }
      [[fallthrough]];

    case 0:
    default: return GUI_MB_FAIL;
  }
}


// Show message box window with hyperlinks in text.
int os_message_box(const char *utf8_text, const char *utf8_caption, int flags)
{
  debug("%s(%s, %s, %d)", __FUNCTION__, utf8_text, utf8_caption, flags);
  if (utf8_text == NULL || utf8_caption == NULL)
  {
    debug("%s no text or caption", __FUNCTION__);
    return GUI_MB_FAIL;
  }

  wchar_t caption[256];
  wchar_t text[2048];

  utf8_to_wcs(utf8_caption, caption, countof(caption));
  utf8_to_wcs(utf8_text, text, countof(text));

  ::ClipCursor(NULL);

  int res = GUI_MB_FAIL;
#if !TOOLS_DAKERNEL
  if ((flags & GUI_MB_NATIVE_DLG) == 0)
    res = messageBoxHyperlinks(text, caption, flags);
#endif
  if (res == GUI_MB_FAIL)
    res = messageBoxNative(text, caption, flags);

  return res;
}

#define EXPORT_PULL dll_pull_osapiwrappers_messageBox
#include <supp/exportPull.h>
