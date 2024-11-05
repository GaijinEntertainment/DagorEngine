// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetObject

#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>

#endif


namespace HumanInput
{

bool keyboard_has_ime_layout()
{
#if _TARGET_PC_WIN
  int size = GetKeyboardLayoutList(0, NULL);
  if (size < 0)
  {
    G_ASSERTF(0, "GetKeyboardLayoutList has returned 0x%X", size);
    return false;
  }
  Tab<HKL> list(framemem_ptr());
  list.resize(size);
  GetKeyboardLayoutList(size, list.data());
  for (HKL lang : list)
  {
    WORD primary = PRIMARYLANGID((intptr_t)lang);
    if (primary == LANG_CHINESE || primary == LANG_JAPANESE || primary == LANG_KOREAN)
      return true;
  }
  return false;
#else
  return false;
#endif
}


} // namespace HumanInput
