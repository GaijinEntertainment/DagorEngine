#define _WIN32_WINNT 0x0500

#include "de_aboutdlg.h"
#include <windows.h>
#include <stdio.h>

#include <oldEditor/de_interface.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_unicode.h>


#define ABOUT_CLASS_NAME "DagorAboutWindow"

enum
{
  FONT_HEIGHT = 14,

  DIALOG_WIDTH = 340,
  DIALOG_HEIGHT = 350,

  HOR_INDENT = 3,
  VERT_INDENT = 5,
  CLIENT_WIDTH = (DIALOG_WIDTH - HOR_INDENT * 2),

  BUTTON_WIDTH = 100,
  BUTTON_HEIGHT = 28,

  LABEL_HEIGHT = 20,
  EDIT_HEIGHT = 23,

  ICON_LEFT = 20,
  ICON_TOP = 20,

  TEXT_LEFT = (ICON_LEFT * 2 + 42),
  TEXT_TOP = ICON_TOP,
  TEXT_WIDTH = (CLIENT_WIDTH - TEXT_LEFT - HOR_INDENT),
  TEXT_HEIGHT = (4 * LABEL_HEIGHT),
  SCROLL_TOP = (TEXT_TOP + TEXT_HEIGHT + 4),
  SCROLL_HEIGHT = (DIALOG_HEIGHT - SCROLL_TOP - BUTTON_HEIGHT * 3),

  LINE_SIZE = 18,
  ID_TIMER = 1000,
  SCROLL_TIMEOUT = 25,
};

//==============================================================================

LRESULT CALLBACK window_about_proc(HWND hwnd, unsigned message, WPARAM wparam, LPARAM lparam)
{
  AboutDlg *lda = (AboutDlg *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if (!lda)
    return DefWindowProc(hwnd, message, wparam, lparam);

  return lda->processMessages((void *)hwnd, message, (void *)wparam, (void *)lparam);
}


static ATOM wnd_class_handle = 0;
static void register_about_class()
{
  if (::wnd_class_handle)
    return;

  WNDCLASSEX wcex = {0};

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = 0;
  wcex.lpfnWndProc = (WNDPROC)window_about_proc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = GetModuleHandle(NULL);
  wcex.hIcon = NULL;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNSHADOW);
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = ABOUT_CLASS_NAME;
  wcex.hIconSm = NULL;

  ::wnd_class_handle = RegisterClassEx(&wcex);
}

//==============================================================================

AboutDlg::AboutDlg(void *hwnd) : mpHandle(hwnd)
{
  ::register_about_class();

  // fonts

  mFNH = CreateFont(-FONT_HEIGHT, 0, 0, 0, FW_NORMAL, false, false, false, RUSSIAN_CHARSET, 0, 0, PROOF_QUALITY, 0, "Arial");

  mFBH = CreateFont(-FONT_HEIGHT, 0, 0, 0, FW_SEMIBOLD, false, false, false, RUSSIAN_CHARSET, 0, 0, PROOF_QUALITY, 0, "Arial");

  // bitmap

  char bmpPath[512];
  ::GetModuleFileName(NULL, bmpPath, 512);

  char *slash = strrchr(bmpPath, '\\');

  if (slash)
    strcpy(slash, "\\data\\tex\\de_icon.bmp");

  mBitmapHandle = ::LoadImage(::GetModuleHandle(NULL), bmpPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

  if (mBitmapHandle)
  {
    BITMAP bm;
    ::GetObject((HBITMAP)mBitmapHandle, sizeof(bm), (LPVOID)&bm);

    bmpW = bm.bmWidth;
    bmpH = bm.bmHeight;
  }
}

//==============================================================================

AboutDlg::~AboutDlg() { DestroyWindow((HWND)mHandle); }

//==============================================================================

long AboutDlg::processMessages(void *hwnd, unsigned msg, void *w_param, void *l_param)
{
  switch (msg)
  {
    case WM_KEYDOWN:
      if ((intptr_t)w_param == VK_ESCAPE || (intptr_t)w_param == VK_RETURN)
      {
        DestroyWindow((HWND)hwnd);
      }
      return 0;

    case WM_COMMAND:
    {
      if ((l_param == mOkButtonHandle) && (HIWORD(w_param) == BN_CLICKED))
        DestroyWindow((HWND)hwnd);
    }
      return 0;

    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC dc = ::BeginPaint((HWND)hwnd, &ps);

      HDC bmpDc = ::CreateCompatibleDC(dc);
      HBITMAP bmpOldDc = (HBITMAP)::SelectObject(bmpDc, (HBITMAP)mBitmapHandle);

      ::BitBlt(dc, ICON_LEFT, ICON_TOP, bmpW, bmpH, bmpDc, 0, 0, SRCCOPY);

      ::SelectObject(dc, bmpOldDc);
      ::DeleteDC(bmpDc);
      ::DeleteObject(bmpOldDc);

      ::EndPaint((HWND)hwnd, &ps);

      return 0;
    }

    case WM_TIMER:
    {
      mScrollPos -= 1;

      if (-mScrollPos > scrollH)
        mScrollPos = SCROLL_HEIGHT;

      SetWindowPos((HWND)mTextHandle, 0, 0, mScrollPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

      return 0;
    }

    case WM_CLOSE:
      KillTimer((HWND)hwnd, ID_TIMER);
      DestroyWindow((HWND)hwnd);
      return 0;
  }

  return DefWindowProc((HWND)hwnd, msg, (WPARAM)w_param, (LPARAM)l_param);
}

//==============================================================================

void AboutDlg::show()
{
  mHandle =
    CreateWindowEx(0, ABOUT_CLASS_NAME, "About DagorEditor 3.5", WS_SYSMENU, (GetSystemMetrics(SM_CXSCREEN) - DIALOG_WIDTH) / 2,
      (GetSystemMetrics(SM_CYSCREEN) - DIALOG_HEIGHT) / 2, DIALOG_WIDTH, DIALOG_HEIGHT, (HWND)mpHandle, 0, GetModuleHandle(NULL), 0);

  SetWindowLongPtr((HWND)mHandle, GWLP_USERDATA, (LONG_PTR)this);

  // create controls

  // title

  char caption[512] = "\0";
  sprintf(caption, "DaEditor3 \r\n%s\n\r\n\rCopyright (c) Gaijin Games KFT", IDagorEd2Engine::getBuildVersion());

  void *_handle = CreateWindowEx(0, "STATIC", caption, WS_CHILD | WS_VISIBLE | SS_LEFT, TEXT_LEFT, TEXT_TOP, TEXT_WIDTH, TEXT_HEIGHT,
    (HWND)mHandle, 0, GetModuleHandle(NULL), 0);

  SendMessage((HWND)_handle, WM_SETFONT, (WPARAM)mFNH, MAKELPARAM(true, 0));

  // scroll

  _handle = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 0, SCROLL_TOP, DIALOG_WIDTH, SCROLL_HEIGHT, (HWND)mHandle, 0,
    GetModuleHandle(NULL), 0);

  scrollH = 0;
  mScrollPos = SCROLL_HEIGHT;

  mTextHandle = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 0, mScrollPos, DIALOG_WIDTH, scrollH, (HWND)_handle, 0,
    GetModuleHandle(NULL), 0);

  // button

  mOkButtonHandle =
    CreateWindowEx(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER | WS_TABSTOP, (CLIENT_WIDTH - BUTTON_WIDTH) / 2,
      DIALOG_HEIGHT - 2.5 * BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT, (HWND)mHandle, 0, GetModuleHandle(NULL), 0);

  SendMessage((HWND)mOkButtonHandle, WM_SETFONT, (WPARAM)mFNH, MAKELPARAM(true, 0));

  // show

  fill();
  ShowWindow((HWND)mHandle, SW_SHOWDEFAULT);

  SetTimer((HWND)mHandle, ID_TIMER, SCROLL_TIMEOUT, NULL);
}

//==============================================================================

void AboutDlg::addLine(const char text[], bool bolt)
{
  wchar_t wcs_buf[512];
  void *_handle = CreateWindowExW(0, L"STATIC", utf8_to_wcs(text, wcs_buf, sizeof(wcs_buf) / sizeof(wcs_buf[0])),
    WS_CHILD | WS_VISIBLE | SS_CENTER, 0, scrollH, DIALOG_WIDTH, LINE_SIZE, (HWND)mTextHandle, 0, GetModuleHandle(NULL), 0);

  SendMessage((HWND)_handle, WM_SETFONT, (WPARAM)((bolt) ? mFBH : mFNH), MAKELPARAM(true, 0));

  scrollH += LINE_SIZE;

  SetWindowPos((HWND)mTextHandle, 0, 0, 0, DIALOG_WIDTH, scrollH, SWP_NOZORDER | SWP_NOMOVE);
}

//==============================================================================

void AboutDlg::fill()
{
  addLine("Менеджеры", true);
  addLine("Антон Юдинцев");
  addLine("Кирилл Юдинцев");

  addLine("");

  addLine("Ведущие программисты", true);
  addLine("Николай Савичев");
  addLine("Олег Смирнов");

  addLine("");

  addLine("Программисты", true);
  addLine("Алексей Волынсков");
  addLine("Сергей Галкин");
  addLine("Александр Демидов");
  addLine("Андрей Мироненко");
  addLine("Андрей Резник");
  addLine("Дмитрий Шипилов");
  addLine("");
  addLine("Александ Харьков");
  addLine("Олег Олейниченко");
  addLine("Игорь Турега");

  addLine("");

  addLine("Документация", true);
  addLine("Олег Демешко");

  addLine("");

  addLine("Тестирование, рекомендации", true);
  addLine("Олег Курченко");
  addLine("Кирилл Плюснин");
  addLine("Роман Шелагуров");
}
