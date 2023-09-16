#include "internal.h"

#pragma warning(push)
#pragma warning(disable : 4917)
#include <shlobj.h>
#pragma warning(pop)
#include <direct.h>

#include <util/dag_string.h>
#include <generic/dag_smallTab.h>

namespace folders
{
namespace internal
{
String game_name;

static void get_sys_path(DWORD flags, String &dir)
{
  wchar_t buf[MAX_PATH + 1];
  buf[0] = 0;
  HRESULT hr = SHGetFolderPathW(NULL, flags | CSIDL_FLAG_CREATE, NULL, 0, buf);
  if (!FAILED(hr))
  {
    convert_to_utf8(dir, buf);
    dir += PATH_DELIM;
  }
}

static void get_appdata_dir(DWORD flags, String &dir)
{
  get_sys_path(flags, dir);
  dir += game_name;
}

void platform_initialize(const char *app_name) { game_name = app_name; }

String get_exe_dir()
{
  String dir;
  WCHAR pathBuf[MAX_PATH] = {0};
  if (GetModuleFileNameW(NULL, pathBuf, sizeof(pathBuf) / sizeof(WCHAR)))
  {
    convert_to_utf8(dir, pathBuf);
    dir.updateSz();
    truncate_exe_dir(dir);
  }
  return dir;
}

String get_game_dir()
{
  G_ASSERT_RETURN(!game_name.empty(), {});
  String dir;
  get_current_work_dir(dir);
  return dir;
}

String get_temp_dir()
{
  String dir;
  DWORD len = GetTempPathW(0, NULL);
  if (len)
  {
    SmallTab<wchar_t, TmpmemAlloc> wdir;
    clear_and_resize(wdir, len + 1);
    if (GetTempPathW(len + 1, wdir.data()))
    {
      convert_to_utf8(dir, wdir.data(), len + 1);
      dir.updateSz();
    }
  }
  return dir;
}

String get_gamedata_dir()
{
  G_ASSERT_RETURN(!game_name.empty(), {});
  String dir;
  get_sys_path(CSIDL_PERSONAL, dir);
  dir += "My Games";
  dir += PATH_DELIM;
  dir += game_name;
  return dir;
}

String get_local_appdata_dir()
{
  G_ASSERT_RETURN(!game_name.empty(), {});
  String dir;
  get_appdata_dir(CSIDL_LOCAL_APPDATA, dir);
  return dir;
}

String get_common_appdata_dir()
{
  G_ASSERT_RETURN(!game_name.empty(), {});
  String dir;
  get_appdata_dir(CSIDL_COMMON_APPDATA, dir);
  return dir;
}

String get_downloads_dir()
{
  String local = get_local_appdata_dir();
  String dir;
  dir.printf(0, "%s/%s", local, "downloads");
  return dir;
}
} // namespace internal
} // namespace folders
