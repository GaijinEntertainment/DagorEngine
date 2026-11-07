// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dsPadSteam.h"
#include <osApiWrappers/dag_files.h>
#include <generic/dag_tab.h>
#include <string.h>
#include <stdlib.h>

namespace HumanInput
{
namespace dspad
{

#if _TARGET_PC_WIN
// Steam Input writes the file named by the SteamVirtualGamepadInfo environment variable: one [slot N]
// section per virtual Xbox pad with the VID and PID of the physical pad behind it
static bool steam_virtual_gamepad_info_lists_vendor(const char *path, uint16_t vid)
{
  file_ptr_t fp = df_open(path, DF_READ | DF_IGNORE_MISSING | DF_REALFILE_ONLY);
  if (!fp)
    return false;
  Tab<char> text;
  text.resize(df_length(fp) + 1);
  const int len = df_read(fp, text.data(), text.size() - 1);
  df_close(fp);
  if (len <= 0)
    return false;
  text[len] = 0;

  for (char *line = text.data(); *line;)
  {
    char *next = strpbrk(line, "\r\n");
    if (next)
      *next++ = 0;
    else
      next = line + strlen(line);
    if (strncmp(line, "VID=", 4) == 0 && strtoul(line + 4, nullptr, 16) == vid)
      return true;
    line = next;
  }
  return false;
}

bool steam_input_remaps_vendor(uint16_t vid)
{
  const char *path = getenv("SteamVirtualGamepadInfo");
  return path && steam_virtual_gamepad_info_lists_vendor(path, vid);
}
#else
static bool vid_pid_list_has_vendor(const char *list, uint16_t vid)
{
  for (const char *entry = list;; entry++)
  {
    if (strtoul(entry, nullptr, 16) == vid)
      return true;
    if (!(entry = strchr(entry, ',')))
      return false;
  }
}

// Set per game, unlike the global file
bool steam_input_remaps_vendor(uint16_t vid)
{
  const char *except = getenv("SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT");
  if (except && *except)
    return !vid_pid_list_has_vendor(except, vid);
  const char *ignored = getenv("SDL_GAMECONTROLLER_IGNORE_DEVICES");
  return ignored && vid_pid_list_has_vendor(ignored, vid);
}
#endif

} // namespace dspad
} // namespace HumanInput
