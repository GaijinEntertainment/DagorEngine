// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/_platform.h>
#include "kbd_classdrv_win.h"
#include "kbd_device_win.h"
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_unicode.h>
#include "kbd_private.h"
#include <stdio.h>

#if _TARGET_PC_MACOSX | _TARGET_PC_LINUX

#if _TARGET_PC_MACOSX
#include "kbd_macosx_names.h"
#else
#include "kbd_linux_names.h"
#endif

unsigned char unix_hid_key_to_scancode[256] = {0};
unsigned char unix_hid_scancode_to_key[256] = {0};

#elif _TARGET_XBOX
#include "kbd_xboxone_names.h"
#endif

using namespace HumanInput;

static Tab<char> keyNames(inimem);
char *HumanInput::key_name[256];

IGenKeyboardClassDrv *HumanInput::createWinKeyboardClassDriver()
{
  init_key_to_shift_bit();
  memset(&raw_state_kbd, 0, sizeof(raw_state_kbd));

  WinKeyboardClassDriver *cd = new (inimem) WinKeyboardClassDriver;
  if (!cd->init())
  {
    delete cd;
    cd = NULL;
  }
  return cd;
}

#if _TARGET_PC_WIN
static bool set_english_layout()
{
  static const int sublanguages[] = {
    SUBLANG_ENGLISH_US,
    SUBLANG_ENGLISH_UK,
    SUBLANG_ENGLISH_AUS,
    SUBLANG_ENGLISH_CAN,
    SUBLANG_ENGLISH_NZ,
    SUBLANG_ENGLISH_INDIA,
    SUBLANG_ENGLISH_MALAYSIA,
    SUBLANG_ENGLISH_SINGAPORE,
    SUBLANG_ENGLISH_PHILIPPINES,
    SUBLANG_ENGLISH_CARIBBEAN,
    SUBLANG_ENGLISH_EIRE,
    SUBLANG_ENGLISH_SOUTH_AFRICA,
    SUBLANG_ENGLISH_JAMAICA,
    SUBLANG_ENGLISH_BELIZE,
    SUBLANG_ENGLISH_TRINIDAD,
    SUBLANG_ENGLISH_ZIMBABWE,
  };

  for (int sublanguage : sublanguages)
  {
    intptr_t langId = MAKELANGID(LANG_ENGLISH, sublanguage);
    if (IsValidLocale(MAKELCID(langId, SORT_DEFAULT), LCID_INSTALLED) && ActivateKeyboardLayout((HKL)langId, KLF_SETFORPROCESS))
      return true;
  }

  return false;
}
#endif

bool WinKeyboardClassDriver::init()
{
  stg_kbd.enabled = false;

  wchar_t kname[64];
  char kname_ut8[256];

#if _TARGET_PC_MACOSX | _TARGET_PC_LINUX
  memset(unix_hid_key_to_scancode, 0, sizeof(unix_hid_key_to_scancode));
  memset(unix_hid_scancode_to_key, 0, sizeof(unix_hid_scancode_to_key));

  for (int j = 0; j < sizeof(kbdMappedNames) / sizeof(kbdMappedNames[0]); j++)
  {
    G_ASSERTF(unix_hid_key_to_scancode[kbdMappedNames[j].id] == 0 && unix_hid_scancode_to_key[kbdMappedNames[j].hid_id] == 0,
      "%d: already inited (%d,%d)", j, unix_hid_key_to_scancode[kbdMappedNames[j].id],
      unix_hid_scancode_to_key[kbdMappedNames[j].hid_id]);

    unix_hid_key_to_scancode[kbdMappedNames[j].id] = kbdMappedNames[j].hid_id;
    unix_hid_scancode_to_key[kbdMappedNames[j].hid_id] = kbdMappedNames[j].id;
  }

  for (int j = 1; j < 256; j++)
    if (unix_hid_key_to_scancode[j] == 0 && unix_hid_scancode_to_key[j] == 0)
      unix_hid_key_to_scancode[j] = unix_hid_scancode_to_key[j] = j;

  for (int j = 1; j < 256; j++)
    if (unix_hid_key_to_scancode[j] == 0)
      for (int k = 0; k < 256; k++)
        if (unix_hid_scancode_to_key[k] == 0)
        {
          unix_hid_key_to_scancode[j] = k;
          unix_hid_scancode_to_key[k] = j;
          break;
        }
#endif

#if _TARGET_PC_WIN && !IS_CONSOLE_EXE
  HKL prevLayout = GetKeyboardLayout(0);

  // On Windows calling ActivateKeyboardLayout() causes IME input to stop working.
  // Disable key names localization in this case - on these systems the problem solved by
  // switching to English layout does not exist.
  bool useKeyNamesFromEnglishLayout =
    !HumanInput::keyboard_has_ime_layout() &&
    (!::dgs_get_settings() || ::dgs_get_settings()->getBlockByNameEx("input")->getBool("useKeyNamesFromEnglishLayout", true));

  if (useKeyNamesFromEnglishLayout && !set_english_layout())
  {
    logwarn("WinKeyboardClassDriver.init(): Failed to set English keyboard layout");
    useKeyNamesFromEnglishLayout = false;
  }
#endif

  memset(HumanInput::key_name, 0xFF, sizeof(HumanInput::key_name));
  for (int i = 0; i < 256; i++)
  {
    if ((intptr_t)HumanInput::key_name[i] != -1)
      continue;
#if _TARGET_PC_WIN
    int len = ::GetKeyNameTextW(((i & 0x7f) << 16) | ((i & 0x80) << 17), kname, 64);

    //== some drivers report NumLock/Pause incorrectly, so we hardcoded these keys
    if (i == 0x45)
    {
      len = 9 * 2;
      memcpy(kname, L"Num Lock", len);
    }
    else if (i == 0xC5)
    {
      len = 6 * 2;
      memcpy(kname, L"Pause", len);
    }

    HumanInput::key_name[i] = (char *)(intptr_t)(len ? (int)keyNames.size() : -1);
    if (len)
    {
      wcs_to_utf8(kname, kname_ut8, sizeof(kname_ut8));
      len = i_strlen(kname_ut8);
      append_items(keyNames, len + 1, kname_ut8);
    }

#else
    _snprintf(kname_ut8, 64, "K%03d", i);
#if _TARGET_PC_MACOSX | _TARGET_PC_LINUX
    _snprintf(kname_ut8, 64, "K%03d", unix_hid_scancode_to_key[i]);
#endif
#if _TARGET_PC_MACOSX | _TARGET_PC_LINUX | _TARGET_XBOX
    for (int j = 0; j < sizeof(kbdMappedNames) / sizeof(kbdMappedNames[0]); j++)
      if (kbdMappedNames[j].hid_id == i)
      {
        strcpy(kname_ut8, kbdMappedNames[j].name);
        break;
      }
#endif
    int len = i_strlen(kname_ut8);
    HumanInput::key_name[i] = (char *)(intptr_t)(len ? (int)keyNames.size() : -1);
    append_items(keyNames, len + 1, kname_ut8);
#endif
  }

#if _TARGET_PC_WIN
  if (useKeyNamesFromEnglishLayout)
    ActivateKeyboardLayout(prevLayout, KLF_SETFORPROCESS);
#endif

  keyNames.shrink_to_fit();
  for (int i = 0; i < 256; i++)
  {
    HumanInput::key_name[i] = (intptr_t)HumanInput::key_name[i] >= 0 ? &keyNames[(intptr_t)HumanInput::key_name[i]] : NULL;
    // debug("%d: <%s>", i, HumanInput::key_name[i]);
  }

  refreshDeviceList();
  if (device)
    enable(true);
  return true;
}

void WinKeyboardClassDriver::destroyDevices()
{
  unacquireDevices();
  if (device)
    delete device;
  device = NULL;
}

// generic hid class driver interface
void WinKeyboardClassDriver::enable(bool en)
{
  enabled = en;
  stg_kbd.enabled = en;
}
void WinKeyboardClassDriver::destroy()
{
  destroyDevices();
  stg_kbd.present = false;

  delete this;
}

// generic keyboard class driver interface
IGenKeyboard *WinKeyboardClassDriver::getDevice(int idx) const { return (idx == 0) ? device : NULL; }
void WinKeyboardClassDriver::useDefClient(IGenKeyboardClient *cli)
{
  defClient = cli;
  if (device)
    device->setClient(defClient);
}

void WinKeyboardClassDriver::refreshDeviceList()
{
  destroyDevices();

  bool was_enabled = enabled;
  enable(false);

  device = new WinKeyboardDevice;

  // update global settings
  stg_kbd.present = true;
  enable(was_enabled);

  useDefClient(defClient);
  acquireDevices();
}

#if _TARGET_PC_WIN
static struct
{
  int winKey;
  int dKey;
} shiftsToReinit[] = {{VK_LMENU, HumanInput::DKEY_LALT}, {VK_RMENU, HumanInput::DKEY_RALT}, {VK_LSHIFT, HumanInput::DKEY_LSHIFT},
  {VK_RSHIFT, HumanInput::DKEY_RSHIFT}, {VK_LCONTROL, HumanInput::DKEY_LCONTROL}, {VK_RCONTROL, HumanInput::DKEY_RCONTROL}};
#endif

void WinKeyboardClassDriver::acquireDevices()
{
#if _TARGET_PC_WIN
  for (int i = getDeviceCount() - 1; i >= 0; i--)
  {
    HumanInput::IGenKeyboard *dev = getDevice(i);
    HumanInput::IGenKeyboardClient *cli = dev->getClient();
    HumanInput::KeyboardRawState &s = (HumanInput::KeyboardRawState &)dev->getRawState();
    for (int i = 0; i < countof(shiftsToReinit); i++)
    {
      int dKey = shiftsToReinit[i].dKey;
      if (GetAsyncKeyState(shiftsToReinit[i].winKey) & 0x8000)
      {
        if (!s.isKeyDown(dKey))
        {
          s.setKeyDown(dKey);
          if (cli)
            cli->gkcButtonDown(dev, dKey, false, 0);
        }
      }
      else
      {
        if (s.isKeyDown(dKey))
        {
          s.clrKeyDown(dKey);
          if (cli)
            cli->gkcButtonUp(dev, dKey);
        }
      }
    }
    dev->syncRawState();
  }
#endif
}
