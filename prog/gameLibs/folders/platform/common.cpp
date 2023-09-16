#include "internal.h"

#if _TARGET_PC_WIN
#include <direct.h>
#endif

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID
#include <stdlib.h>
#include <unistd.h>
#endif

#include <osApiWrappers/dag_direct.h>

namespace folders
{
namespace internal
{
void get_current_work_dir(String &dir)
{
  char buf[DAGOR_MAX_PATH * 3]; // x3 to compensate to UTF-8 large 3-byte code points in asian characters
  if (getcwd(buf, sizeof(buf)))
  {
    dir = buf;
  }
}

void truncate_exe_dir(String &dir)
{
  if (dir.empty())
    return;

  dd_simplify_fname_c(dir.str());
  char *p = strrchr(dir, PATH_DELIM);
  G_ASSERTF(p, "dir=%s (size=%d) PATH_DELIM=%c", dir, dir.length(), PATH_DELIM);

  if (p)
  {
    p[1] = '\0';
    dir.updateSz();
  }
  else
    dir = "./";
}
} // namespace internal
} // namespace folders