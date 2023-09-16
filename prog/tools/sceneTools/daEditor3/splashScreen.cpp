#include "splashScreen.h"
#include <oldEditor/de_interface.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_unicode.h>
#include <util/dag_globDef.h>

#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <stdio.h>

#include <libTools/util/strUtil.h>


#define SPLASH_CLASS_NAME  "DagorSplashScreen"
#define COPYRIGHT_STR      "© 2023 Gaijin Games KFT"
#define COPYRIGHT_FONT_COL RGB(255, 255, 255)


void *SplashScreen::hwnd = NULL;
void *SplashScreen::bitmap = NULL;

int SplashScreen::bmpW = 640;
int SplashScreen::bmpH = 175;


//==============================================================================
unsigned SplashScreen::registerSplashClass()
{
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = 0;
  wcex.lpfnWndProc = (WNDPROC)wndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = GetModuleHandle(NULL);
  wcex.hIcon = NULL;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = SPLASH_CLASS_NAME;
  wcex.hIconSm = NULL;

  return RegisterClassEx(&wcex);
}


//==============================================================================
SplashScreen::SplashScreen()
{
  G_VERIFY(registerSplashClass());

  char bmpPath[512];
  ::GetModuleFileName(NULL, bmpPath, 512);

  char *slash = strrchr(bmpPath, '\\');

  if (slash)
    strcpy(slash, "\\..\\commonData\\tex\\splash.bmp");

  bitmap = ::LoadImage(::GetModuleHandle(NULL), bmpPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

  if (bitmap)
  {
    BITMAP bm;
    ::GetObject((HBITMAP)bitmap, sizeof(bm), (LPVOID)&bm);

    bmpW = bm.bmWidth;
    bmpH = bm.bmHeight;
  }

  const int sw = ::GetSystemMetrics(SM_CXSCREEN);
  const int sh = ::GetSystemMetrics(SM_CYSCREEN);

  hwnd = ::CreateWindowEx(WS_EX_TOPMOST, SPLASH_CLASS_NAME, NULL, WS_POPUP, (sw - bmpW) / 2, (sh - bmpH) / 2, bmpW, bmpH, NULL, NULL,
    ::GetModuleHandle(NULL), NULL);

  ::ShowWindow((HWND)hwnd, SW_SHOW);
  ::UpdateWindow((HWND)hwnd);

  ::dgs_pre_shutdown_handler = &SplashScreen::kill;
}


//==============================================================================
SplashScreen::~SplashScreen() { kill(); }


//==============================================================================
void SplashScreen::kill()
{
  ::DestroyWindow((HWND)hwnd);
  ::UnregisterClass("DagorSplashScreen", GetModuleHandle(NULL));
  hwnd = NULL;

  if (bitmap)
  {
    ::DeleteObject(bitmap);
    bitmap = NULL;
  }
  ::dgs_pre_shutdown_handler = NULL;
}


//==============================================================================
int __stdcall SplashScreen::wndProc(void *h_wnd, unsigned msg, void *w_param, void *l_param)
{
  if (msg == WM_PAINT)
  {
    PAINTSTRUCT ps;
    HDC dc = ::BeginPaint((HWND)hwnd, &ps);

    HDC bmpDc = ::CreateCompatibleDC(dc);
    HBITMAP bmpOldDc = (HBITMAP)::SelectObject(bmpDc, bitmap);

    ::BitBlt(dc, 0, 0, bmpW, bmpH, bmpDc, 0, 0, SRCCOPY);

    ::SelectObject(dc, bmpOldDc);
    ::DeleteDC(bmpDc);
    ::DeleteObject(bmpOldDc);

    LOGFONTA fontRec;
    memset(&fontRec, 0, sizeof(LOGFONTA));

    fontRec.lfWeight = FW_NORMAL;
    fontRec.lfCharSet = DEFAULT_CHARSET;
    fontRec.lfOutPrecision = OUT_DEFAULT_PRECIS;
    fontRec.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    fontRec.lfQuality = DRAFT_QUALITY;
    fontRec.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    fontRec.lfHeight = -11;
    sprintf(fontRec.lfFaceName, "Tahoma");

    HFONT font = ::CreateFontIndirect(&fontRec);
    HFONT fontOld = (HFONT)::SelectObject(dc, font);

    ::SetMapMode(dc, MM_TEXT);
    ::SetTextColor(dc, COPYRIGHT_FONT_COL);
    ::SetBkMode(dc, TRANSPARENT);
    ::TextOut(dc, 20, 145, IDagorEd2Engine::getBuildVersion(), (int)strlen(IDagorEd2Engine::getBuildVersion()));

    Tab<wchar_t> copyright_str;
    convert_utf8_to_u16_buf(copyright_str, COPYRIGHT_STR, -1);
    ::TextOutW(dc, 450, 145, copyright_str.data(), (int)wcslen(copyright_str.data()));

    ::SelectObject(dc, fontOld);
    ::DeleteObject(fontOld);
    ::DeleteObject(font);

    ::EndPaint((HWND)hwnd, &ps);

    return 0;
  }

  return ::DefWindowProcW((HWND)h_wnd, msg, (WPARAM)w_param, (LPARAM)l_param);
}
