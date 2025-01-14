// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/_platform.h>
#include "kbd_device_win.h"
#include <debug/dag_fatal.h>
#include <util/dag_string.h>
#include <util/dag_tabHlp.h>
#include <debug/dag_traceInpDev.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <startup/dag_inpDevClsDrv.h>

#if _TARGET_PC_MACOSX
// Hack to avoid Ptr redeclaration in CoreFoundation.h
// defined as template in /prog/dagorInclude/util/dag_safeArg.h
#define Ptr CFPtr
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#undef Ptr

#elif _TARGET_PC_LINUX
#include <dlfcn.h>
#endif

#if _TARGET_PC_MACOSX | _TARGET_PC_LINUX
#define MAX_LAYOUT_NAME_LENGTH 2
extern unsigned char unix_hid_key_to_scancode[256];
extern unsigned char unix_hid_scancode_to_key[256];
#endif


#if _TARGET_PC_WIN
#include <osApiWrappers/dag_unicode.h>
#include <workCycle/threadedWindow.h>
#include <generic/dag_smallTab.h>
#include <stdio.h>
static STICKYKEYS kbd_saved_sticky_keys = {sizeof(STICKYKEYS), 0};
static TOGGLEKEYS kbd_saved_toggle_keys = {sizeof(TOGGLEKEYS), 0};
static FILTERKEYS kbd_saved_filter_keys = {sizeof(FILTERKEYS), 0};
static bool kbd_accessibility_saved = false;
static HANDLE accessibility_semaphore = NULL;

static bool capture_single_accessibility_holder()
{
  if (accessibility_semaphore)
    return false;

  accessibility_semaphore = CreateSemaphoreA(NULL, 0, 1, "GaijinKbdAccessibilityHolder");

  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
    CloseHandle(accessibility_semaphore);
    accessibility_semaphore = NULL;
    return false;
  }

  return true;
}

static void release_single_accessibility_holder()
{
  ReleaseSemaphore(accessibility_semaphore, 1, NULL);
  CloseHandle(accessibility_semaphore);
  accessibility_semaphore = NULL;
}

static wchar_t *get_kbd_storage_filename()
{
  static wchar_t buf[512] = {0};
  DWORD len = GetTempPathW(countof(buf), buf);
  wcsncat(buf, L"gaijin_kbd_storage.0", countof(buf) - len);
  return buf;
}

static void save_accessibility_flags(DWORD sticky_flags, DWORD toggle_flags, DWORD filter_flags)
{
  FILE *f = _wfopen(get_kbd_storage_filename(), L"wt");
  if (!f)
  {
    logerr("kbd: cannot access tmp file");
    G_ASSERT(0);
    return;
  }

  fprintf(f, "%lu\n%lu\n%lu", sticky_flags, toggle_flags, filter_flags);
  fclose(f);
  debug("kbd: accessibility settings saved to file");
}

static bool try_load_accessibility_flags(DWORD &sticky_flags, DWORD &toggle_flags, DWORD &filter_flags)
{
  FILE *f = _wfopen(get_kbd_storage_filename(), L"rt");
  if (!f)
    return false;

  bool res = (fscanf(f, "%lu\n%lu\n%lu", &sticky_flags, &toggle_flags, &filter_flags) == 3);
  fclose(f);
  DeleteFileW(get_kbd_storage_filename());
  debug("kbd: accessibility settings restored from file");
  return res;
}

static void kbd_init_accessibility_shortcut_keys()
{
  if (!dgs_inpdev_disable_acessibility_shortcuts)
    return;

  BOOL res;
  res = SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &kbd_saved_sticky_keys, 0);
  if (!res)
    logwarn("kbd: failed to call SystemParametersInfo(SPI_GETSTICKYKEYS)");

  res = SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &kbd_saved_toggle_keys, 0);
  if (!res)
    logwarn("kbd: failed to call SystemParametersInfo(SPI_GETTOGGLEKEYS)");

  res = SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &kbd_saved_filter_keys, 0);
  if (!res)
    logwarn("kbd: failed to call SystemParametersInfo(SPI_GETFILTERKEYS)");
  G_UNUSED(res);

  if (!try_load_accessibility_flags(kbd_saved_sticky_keys.dwFlags, kbd_saved_toggle_keys.dwFlags, kbd_saved_filter_keys.dwFlags))
    save_accessibility_flags(kbd_saved_sticky_keys.dwFlags, kbd_saved_toggle_keys.dwFlags, kbd_saved_filter_keys.dwFlags);
}

static void kbd_allow_accessibility_shortcut_keys(bool allow_accessibility)
{
  if (!dgs_inpdev_disable_acessibility_shortcuts)
    return;

  if (allow_accessibility)
  {
    if (kbd_accessibility_saved)
    {
      STICKYKEYS sk = kbd_saved_sticky_keys;
      TOGGLEKEYS tk = kbd_saved_toggle_keys;
      FILTERKEYS fk = kbd_saved_filter_keys;
      SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &kbd_saved_sticky_keys, 0);
      SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &kbd_saved_toggle_keys, 0);
      SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &kbd_saved_filter_keys, 0);
    }
    kbd_accessibility_saved = false;
    DeleteFileW(get_kbd_storage_filename());
    release_single_accessibility_holder();
  }
  else
  {
    if (!capture_single_accessibility_holder())
    {
      logwarn("kbd: another application is already handling acessibility settings");
      return;
    }

    if (!kbd_accessibility_saved)
    {
      kbd_accessibility_saved = true;
      kbd_init_accessibility_shortcut_keys();
    }

    STICKYKEYS skOff = kbd_saved_sticky_keys;
    if ((skOff.dwFlags & SKF_STICKYKEYSON) == 0)
    {
      skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
      skOff.dwFlags &= ~SKF_CONFIRMHOTKEY;
      BOOL res = SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);
      if (!res)
        logwarn("kbd: failed to call SystemParametersInfo(SPI_SETSTICKYKEYS)");
      G_UNUSED(res);
    }

    TOGGLEKEYS tkOff = kbd_saved_toggle_keys;
    if ((tkOff.dwFlags & TKF_TOGGLEKEYSON) == 0)
    {
      tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
      tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;
      BOOL res = SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &tkOff, 0);
      if (!res)
        logwarn("kbd: failed to call SystemParametersInfo(SPI_SETTOGGLEKEYS)");
      G_UNUSED(res);
    }

    FILTERKEYS fkOff = kbd_saved_filter_keys;
    if ((fkOff.dwFlags & FKF_FILTERKEYSON) == 0)
    {
      fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
      fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;
      BOOL res = SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fkOff, 0);
      if (!res)
        logwarn("kbd: failed to call SystemParametersInfo(SPI_SETFILTERKEYS)");
      G_UNUSED(res);
    }
  }
}

static unsigned get_locks_mask()
{
  return ::GetKeyState(VK_CAPITAL) & 0x0001 | ((::GetKeyState(VK_NUMLOCK) & 0x0001) << 1) | ((::GetKeyState(VK_SCROLL) & 0x0001) << 2);
}
#endif


using namespace HumanInput;

WinKeyboardDevice::WinKeyboardDevice() : pendingKeyDownCode(0), layoutChangeTracked(false), locksChangeTracked(false)
{
  add_wnd_proc_component(this);

  DEBUG_CTX("init start");
#if _TARGET_PC_WIN
  dgs_inpdev_rawinput_kbd_inited = false;
  if (dgs_inpdev_allow_rawinput && !win32_rdp_compatible_mode)
  {
    unsigned int ridNum = 0;
    GetRegisteredRawInputDevices(NULL, &ridNum, sizeof(RAWINPUTDEVICE));

    RAWINPUTDEVICE *rid = new RAWINPUTDEVICE[ridNum + 1];
    GetRegisteredRawInputDevices(rid, &ridNum, sizeof(RAWINPUTDEVICE));

    for (int i = 0; i < ridNum; i++)
    {
      if (rid[i].usUsagePage == 0x01 && rid[i].usUsage == 0x06)
      {
        delete[] rid;
        return; // keyboard device already registered
      }
    }

    // add HID keyboard
    rid[ridNum].usUsagePage = 0x01;
    rid[ridNum].usUsage = 0x06;
    rid[ridNum].dwFlags = 0; // RIDEV_NOLEGACY
    rid[ridNum].hwndTarget = (HWND)win32_get_main_wnd();

    if (RegisterRawInputDevices(&rid[ridNum], 1, sizeof(RAWINPUTDEVICE)))
      dgs_inpdev_rawinput_kbd_inited = true;
    else
      logerr("Keyboard: RegisterRawInputDevices() failed hr=%08X hwnd=%p ridNum=%d", GetLastError(), win32_get_main_wnd(), ridNum);
    delete[] rid;
  }
  else
    debug("Keyboard: rawinput disabled");

  enableIme(false);
  kbd_allow_accessibility_shortcut_keys(false);

  locks = get_locks_mask();
#elif _TARGET_PC_LINUX
  display = ::XOpenDisplay((char *)0);
  if (display)
  {
    xkbfileLibrary = ::dlopen("libxkbfile.so", RTLD_LAZY);
    if (xkbfileLibrary)
    {
      xkbRF_GetNamesProp = (XkbRF_GetNamesPropDecl)::dlsym(xkbfileLibrary, "XkbRF_GetNamesProp");
      if (!xkbRF_GetNamesProp)
      {
        debug("Function XkbRF_GetNamesProp not found. Layout/locks handlers disabled");
        ::dlclose(xkbfileLibrary);
        xkbfileLibrary = 0;
      }
    }
    else
    {
      debug("Library libxkbfile.so not found. Layout/locks handlers disabled");
      xkbRF_GetNamesProp = 0;
    }
  }
  else
    debug("Can't open display. Layout/locks handlers disabled");
#endif
#if !_TARGET_PC_WIN
  dgs_inpdev_rawinput_kbd_inited = true;
#endif
  DEBUG_CTX("init done");
}
WinKeyboardDevice::~WinKeyboardDevice()
{
#if _TARGET_PC_LINUX
  if (display)
    ::XCloseDisplay(display);
  if (xkbfileLibrary)
    ::dlclose(xkbfileLibrary);
#endif
  del_wnd_proc_component(this);
}

#if _TARGET_PC_WIN
#pragma comment(lib, "imm32.lib")


void WinKeyboardDevice::enableIme(bool on)
{
  HWND hwnd = (HWND)win32_get_main_wnd();
  ImmAssociateContextEx(hwnd, NULL, on ? IACE_DEFAULT : 0);
}


bool WinKeyboardDevice::isImeActive() const
{
  HIMC c = ImmGetContext((HWND)win32_get_main_wnd());
  bool open = ImmGetOpenStatus(c);
  ImmReleaseContext((HWND)win32_get_main_wnd(), c);
  return open;
}
#endif

void WinKeyboardDevice::enableLayoutChangeTracking(bool enable)
{
  layoutChangeTracked = enable;
  if (enable)
    updateLayout(true);
}

void WinKeyboardDevice::enableLocksChangeTracking(bool enable)
{
  locksChangeTracked = enable;
  if (enable)
    updateLocks(true);
}

#if _TARGET_PC_WIN
struct LayoutInfo
{
  int code;
  char name[3];
  static int comporator(const void *lhs, const void *rhs) { return ((const LayoutInfo *)lhs)->code - ((const LayoutInfo *)rhs)->code; }
};

// Table from https://msdn.microsoft.com/en-us/library/ee825488(v=cs.20).aspx
// Title Table of Language Culture Names, Codes, and ISO Values Method [C++]
static const LayoutInfo languageInfo[] = {{0x0004, "zh"}, {0x0401, "ar"}, {0x0402, "bg"}, {0x0403, "ca"}, {0x0404, "zh"},
  {0x0405, "cs"}, {0x0406, "da"}, {0x0407, "de"}, {0x0408, "el"}, {0x0409, "en"}, {0x040B, "fi"}, {0x040C, "fr"}, {0x040D, "he"},
  {0x040E, "hu"}, {0x040F, "is"}, {0x0410, "it"}, {0x0411, "ja"}, {0x0412, "ko"}, {0x0413, "nl"}, {0x0414, "nb"}, {0x0415, "pl"},
  {0x0416, "pt"}, {0x0418, "ro"}, {0x0419, "ru"}, {0x041A, "hr"}, {0x041B, "sk"}, {0x041C, "sq"}, {0x041D, "sv"}, {0x041E, "th"},
  {0x041F, "tr"}, {0x0420, "ur"}, {0x0421, "id"}, {0x0422, "uk"}, {0x0423, "be"}, {0x0424, "sl"}, {0x0425, "et"}, {0x0426, "lv"},
  {0x0427, "lt"}, {0x0429, "fa"}, {0x042A, "vi"}, {0x042B, "hy"}, {0x042C, "az"}, {0x042D, "eu"}, {0x042F, "mk"}, {0x0436, "af"},
  {0x0437, "ka"}, {0x0438, "fo"}, {0x0439, "hi"}, {0x043E, "ms"}, {0x043F, "kk"}, {0x0440, "ky"}, {0x0441, "sw"}, {0x0443, "uz"},
  {0x0444, "tt"}, {0x0446, "pa"}, {0x0447, "gu"}, {0x0449, "ta"}, {0x044A, "te"}, {0x044B, "kn"}, {0x044E, "mr"}, {0x044F, "sa"},
  {0x0450, "mn"}, {0x0456, "gl"}, {0x0457, "ko"}, {0x045A, "sy"}, {0x0465, "di"}, {0x0801, "ar"}, {0x0804, "zh"}, {0x0807, "de"},
  {0x0809, "en"}, {0x080A, "es"}, {0x080C, "fr"}, {0x0810, "it"}, {0x0813, "nl"}, {0x0814, "nn"}, {0x0816, "pt"}, {0x081A, "sr"},
  {0x081D, "sv"}, {0x082C, "az"}, {0x083E, "ms"}, {0x0843, "uz"}, {0x0C01, "ar"}, {0x0C04, "zh"}, {0x0C07, "de"}, {0x0C09, "en"},
  {0x0C0A, "es"}, {0x0C0C, "fr"}, {0x0C1A, "sr"}, {0x1001, "ar"}, {0x1004, "zh"}, {0x1007, "de"}, {0x1009, "en"}, {0x100A, "es"},
  {0x100C, "fr"}, {0x1401, "ar"}, {0x1404, "zh"}, {0x1407, "de"}, {0x1409, "en"}, {0x140A, "es"}, {0x140C, "fr"}, {0x1801, "ar"},
  {0x1809, "en"}, {0x180A, "es"}, {0x180C, "fr"}, {0x1C01, "ar"}, {0x1C09, "en"}, {0x1C0A, "es"}, {0x2001, "ar"}, {0x2009, "en"},
  {0x200A, "es"}, {0x2401, "ar"}, {0x2409, "en"}, {0x240A, "es"}, {0x2801, "ar"}, {0x2809, "en"}, {0x280A, "es"}, {0x2C01, "ar"},
  {0x2C09, "en"}, {0x2C0A, "es"}, {0x3001, "ar"}, {0x3009, "en"}, {0x300A, "es"}, {0x3401, "ar"}, {0x3409, "en"}, {0x340A, "es"},
  {0x3801, "ar"}, {0x380A, "es"}, {0x3C01, "ar"}, {0x3C0A, "es"}, {0x4001, "ar"}, {0x400A, "es"}, {0x440A, "es"}, {0x480A, "es"},
  {0x4C0A, "es"}, {0x500A, "es"}, {0x7C04, "zh"}};
static const int languageInfoSize = sizeof(languageInfo) / sizeof(*languageInfo);

void WinKeyboardDevice::updateLayout(bool notify_when_not_changed)
{
  int layoutId = LOWORD(::GetKeyboardLayout(windows::get_thread_id()));

  const LayoutInfo *layoutInfo =
    (const LayoutInfo *)::dag_bin_search(&layoutId, languageInfo, languageInfoSize, sizeof(LayoutInfo), LayoutInfo::comporator);
  if (!layoutInfo)
    return;

  const char *newLayout = layoutInfo->name;

  bool changed = layout != newLayout;
  if (changed)
    layout = newLayout;
  if (changed || notify_when_not_changed)
    onLayoutChanged();
}

void WinKeyboardDevice::updateLocks(bool notify_when_not_changed)
{
  unsigned newLocks = get_locks_mask();
  bool changed = locks != newLocks;
  if (changed)
    locks = newLocks;
  if (changed || notify_when_not_changed)
    onLocksChanged();
}

#elif _TARGET_PC_LINUX
void WinKeyboardDevice::updateLayout(bool notify_when_not_changed)
{
  if (!xkbRF_GetNamesProp)
    return;

  static char lastLayoutName[MAX_LAYOUT_NAME_LENGTH + 1] = {0};
  XkbRF_VarDefsRec vdr;
  char *tmp = NULL;
  xkbRF_GetNamesProp(display, &tmp, &vdr);

  // layoutName format: "en,uk,ru"
  const char *layoutName = vdr.layout;
  if (!layoutName)
    return;

  XkbStateRec xkbState;
  ::XkbGetState(display, XkbUseCoreKbd, &xkbState);
  int layoutId = static_cast<int>(xkbState.group);

  while (layoutId-- > 0)
  {
    const char *commaChar = strchr(layoutName, ',');
    if (!commaChar)
      return;
    layoutName = commaChar + 1;
  }

  const char *lastCommaChar = strchr(layoutName, ',');
  int layoutNameLength;
  if (lastCommaChar)
    layoutNameLength = lastCommaChar - layoutName;
  else
    layoutNameLength = strlen(layoutName);

  if (layoutNameLength > MAX_LAYOUT_NAME_LENGTH)
    layoutNameLength = MAX_LAYOUT_NAME_LENGTH;

  bool changed = strcmp(lastLayoutName, layoutName) != 0;
  if (changed)
  {
    strncpy(lastLayoutName, layoutName, layoutNameLength);
    layout = lastLayoutName;
  }
  if (changed || notify_when_not_changed)
    onLayoutChanged();
}

void WinKeyboardDevice::updateLocks(bool notify_when_not_changed)
{
  if (!display)
    return;

  unsigned newLocks;
  ::XkbGetIndicatorState(display, XkbUseCoreKbd, &newLocks);

  bool changed = locks != newLocks;
  if (changed)
    locks = newLocks;
  if (changed || notify_when_not_changed)
    onLocksChanged();
}

#elif _TARGET_PC_MACOSX
void WinKeyboardDevice::updateLayout(bool notify_when_not_changed)
{
  static const char macosxLayoutPrefix[] = "keylayout.";
  static int macosxLayoutPrefixLength = sizeof(macosxLayoutPrefix) - 1;
  static char lastLayout[MAX_LAYOUT_NAME_LENGTH + 1];

  TISInputSourceRef inputSource = ::TISCopyCurrentKeyboardInputSource();
  if (inputSource)
  {
    CFStringRef layoutStringRef = (CFStringRef)::TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceID);
    if (!layoutStringRef)
    {
      ::CFRelease(inputSource);
      return;
    }
    const char *newLayout = ::CFStringGetCStringPtr(layoutStringRef, kCFStringEncodingUTF8);
    newLayout = strstr(newLayout, macosxLayoutPrefix);

    if (!newLayout)
    {
      ::CFRelease(inputSource);
      return;
    }

    newLayout += macosxLayoutPrefixLength;
    bool changed = strcmp(lastLayout, newLayout) != 0;
    if (changed)
    {
      strncpy(lastLayout, newLayout, MAX_LAYOUT_NAME_LENGTH);
      layout = lastLayout;
    }

    ::CFRelease(inputSource);

    if (changed || notify_when_not_changed)
      onLayoutChanged();
  }
}

void WinKeyboardDevice::updateLocks(bool notify_when_not_changed) {}

#else
void WinKeyboardDevice::updateLayout(bool notify_when_not_changed) {}
void WinKeyboardDevice::updateLocks(bool notify_when_not_changed) {}
#endif

#if _TARGET_PC_WIN | _TARGET_XBOX
void WinKeyboardDevice::OnChar(uintptr_t wParam)
{
  DEBUG_TRACE_INPDEV("WM:char: %X %X", pendingKeyDownCode, wParam);
  onKeyDown(pendingKeyDownCode, wParam >= 0x20 ? wParam : 0);
  pendingKeyDownCode = 0;
}

void WinKeyboardDevice::OnKeyUpKeyDown(unsigned msg, uintptr_t wParam, intptr_t lParam)
{
  int makecode = wParam == VK_PROCESSKEY ? 0 : (((lParam >> 16) & 0xFF) | ((lParam >> 17) & 0x80));
  bool up = msg == WM_KEYUP || msg == WM_SYSKEYUP;

  DEBUG_TRACE_INPDEV("WM:%s: %X %X %X", up ? "keyUp" : "keyDown", makecode, wParam, lParam);

  // Ignore fake pause/break key, and transform it into proper one.
  if (((lParam >> 16) & 0xFF) == 0x45)
    makecode ^= 0x80;

#if !_TARGET_XBOX
  if (pendingKeyDownCode)
  {
    if (!state.isKeyDown(pendingKeyDownCode))
    {
      DEBUG_TRACE_INPDEV("onKeyDown: %X", pendingKeyDownCode);
      onKeyDown(pendingKeyDownCode, 0);
    }
    pendingKeyDownCode = 0;
  }
#endif

  if (up)
  {
#if !_TARGET_XBOX
    // Handle printscreen specially (since WM_KEYDOWN for it is absent)
    if (makecode == 0xB7 && !state.isKeyDown(makecode))
    {
      DEBUG_TRACE_INPDEV("onKeyDown: %X", makecode);
      onKeyDown(makecode, 0);
    }
    // On either shift up we shall make other also up (since WM_KEYUP for other shift is absent)
    if (makecode == 0x2A && state.isKeyDown(0x36))
      onKeyUp(0x36);
    if (makecode == 0x36 && state.isKeyDown(0x2A))
      onKeyUp(0x2A);
#endif

    DEBUG_TRACE_INPDEV("onKeyUp: %X", makecode);
    onKeyUp(makecode);
  }
  else
  {
#if _TARGET_XBOX
    DEBUG_TRACE_INPDEV("onKeyDown: %X", makecode);
    onKeyDown(makecode, 0);
#else
    pendingKeyDownCode = makecode;
#endif
  }
}
#endif

IWndProcComponent::RetCode WinKeyboardDevice::process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result)
{
  G_UNREFERENCED(result);
#if !_TARGET_PC_WIN
  G_UNREFERENCED(hwnd);
#endif

#if _TARGET_PC_WIN

  if (msg == WM_ACTIVATE)
  {
    int fActive = LOWORD(wParam);
    if (IsIconic((HWND)hwnd))
      fActive = WA_INACTIVE;

    bool allowAccessability = (fActive == WA_INACTIVE);
    kbd_allow_accessibility_shortcut_keys(allowAccessability);
  }

  if (msg == WM_DESTROY)
  {
    kbd_allow_accessibility_shortcut_keys(true);
  }

  if (msg == WM_ACTIVATEAPP && wParam == TRUE)
  {
    if (locksChangeTracked)
      updateLocks();
  }
  else if (msg == WM_IME_NOTIFY && wParam == IMN_SETOPENSTATUS)
  {
    // updateImeContext();
    return PROCEED_DEF_WND_PROC;
  }

  if (msg == WM_INPUT || msg == WM_DAGOR_INPUT)
  {
    if (!dgs_inpdev_rawinput_kbd_inited)
      return PROCEED_OTHER_COMPONENTS;

    RAWINPUT ri{};
    if (msg == WM_INPUT)
    {
      UINT dwSize = sizeof(ri);
      if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &ri, &dwSize, sizeof(ri.header)) == (UINT)-1)
        return PROCEED_OTHER_COMPONENTS;
    }
    else if (auto pri = windows::get_rid(); pri)
      ri = *pri;
    else
      return PROCEED_OTHER_COMPONENTS;

    if (ri.header.dwType != RIM_TYPEKEYBOARD)
      return PROCEED_OTHER_COMPONENTS;

    if (ri.data.keyboard.MakeCode > 0xFF)
      return PROCEED_OTHER_COMPONENTS; // we don't know how to handle those

    bool ime_active = isImeActive();
    DEBUG_TRACE_INPDEV("WM:key: %X %X %X ime=%d", ri.data.keyboard.MakeCode, ri.data.keyboard.VKey, ri.data.keyboard.Flags,
      (int)ime_active);
    if (ime_active)
      return PROCEED_DEF_WND_PROC;
    if (ri.data.keyboard.Flags & RI_KEY_E0)
      if (ri.data.keyboard.MakeCode == 0x2A || ri.data.keyboard.MakeCode == 0x36)
        return PROCEED_DEF_WND_PROC; //== ignore fake shifts


    // Ignore fake pause/break key, and transform it into proper one.
    if (ri.data.keyboard.Flags & RI_KEY_E1 && ri.data.keyboard.VKey == 0x13 && ri.data.keyboard.MakeCode == 0x1D)
      return PROCEED_DEF_WND_PROC;

    if (ri.data.keyboard.VKey == 0xFF && ri.data.keyboard.MakeCode == 0x45)
      ri.data.keyboard.MakeCode |= 0x80;

#if DAGOR_DBGLEVEL > 0
    if (!ri.data.keyboard.MakeCode)
    {
      ri.data.keyboard.MakeCode = MapVirtualKey(ri.data.keyboard.VKey, 0) | 0x80;
      if (pendingKeyDownCode)
        ri.data.keyboard.Flags |= RI_KEY_BREAK;
    }
#endif

    if (pendingKeyDownCode)
    {
      DEBUG_TRACE_INPDEV("onKeyDown: %X", pendingKeyDownCode);
      onKeyDown(pendingKeyDownCode, 0);
      pendingKeyDownCode = 0;
    }

    if (ri.data.keyboard.Flags & RI_KEY_E0)
      ri.data.keyboard.MakeCode |= 0x80;

    if (ri.data.keyboard.Flags & RI_KEY_BREAK)
    {
      DEBUG_TRACE_INPDEV("onKeyUp: %X", ri.data.keyboard.MakeCode);
      onKeyUp(ri.data.keyboard.MakeCode);
    }
    else
      pendingKeyDownCode = ri.data.keyboard.MakeCode;

    return PROCEED_DEF_WND_PROC;
  }
  else if (msg == WM_CHAR)
    OnChar(wParam);
  else if (msg == 0)
  {
    if (locksChangeTracked)
      updateLocks();
    if (pendingKeyDownCode)
    {
      DEBUG_TRACE_INPDEV("onKeyDown: %X", pendingKeyDownCode);
      onKeyDown(pendingKeyDownCode, 0);
      pendingKeyDownCode = 0;
    }
  }
  else if (msg == WM_INPUTLANGCHANGE)
  {
    if (layoutChangeTracked)
      updateLayout();
  }
  else if (!dgs_inpdev_rawinput_kbd_inited || isImeActive())
  {
    if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP)
      OnKeyUpKeyDown(msg, wParam, lParam);
  }
#elif _TARGET_PC_MACOSX | _TARGET_PC_LINUX
  if (msg == GPCM_Activate && wParam != GPCMP1_Inactivate)
  {
    if (layoutChangeTracked)
      updateLayout();
    if (locksChangeTracked)
      updateLocks();
  }
  else if (msg == GPCM_KeyPress)
  {
    wParam &= 0xFF;
    onKeyDown(unix_hid_key_to_scancode[wParam], (lParam >= 0x20 && lParam != 0x7F) ? lParam : 0);
#if _TARGET_PC_MACOSX
    if (layoutChangeTracked)
      updateLayout();
#endif
  }
  else if (msg == GPCM_KeyRelease)
  {
    wParam &= 0xFF;
    onKeyUp(unix_hid_key_to_scancode[wParam]);
#if _TARGET_PC_LINUX
    if (layoutChangeTracked)
      updateLayout();
    if (locksChangeTracked)
      updateLocks();
#endif
  }
  else if (msg == GPCM_Char)
  {
    onKeyDown(0, (wParam >= 0x20 && lParam != 0x7F) ? wParam : 0);
  }
#if _TARGET_PC_LINUX
  else if (msg == GPCM_Activate)
  {
    if (state.isKeyDown(DKEY_TAB))
      onKeyUp(DKEY_TAB);
    if (state.isKeyDown(DKEY_LALT))
      onKeyUp(DKEY_LALT);
    if (state.isKeyDown(DKEY_RALT))
      onKeyUp(DKEY_RALT);
  }
#endif
#elif _TARGET_XBOX
  if (msg == GPCM_KeyPress)
  {
    onKeyDown(wParam, 0);
    return PROCEED_DEF_WND_PROC;
  }
  else if (msg == GPCM_KeyRelease)
  {
    onKeyUp(wParam);
    return PROCEED_DEF_WND_PROC;
  }
  else if (msg == GPCM_Char)
  {
    if (client && wParam >= 0x20)
      client->gkcButtonDown(this, 0, false, wParam);
  }
#endif
#if _TARGET_XBOX
  if (msg == WM_CHAR)
    OnChar(wParam);
#endif
  return PROCEED_OTHER_COMPONENTS;
}
