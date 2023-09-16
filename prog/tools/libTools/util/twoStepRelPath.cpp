#include <libTools/util/twoStepRelPath.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

void TwoStepRelPath::setSdkRoot(const char *root_dir, const char *subdir)
{
  sdkRoot = subdir ? String(0, "%s/%s/", root_dir, subdir) : String(0, "%s/", root_dir);
  simplify_fname(sdkRoot);

  sdkRootLen = (int)strlen(sdkRoot);

  sdkRootLen1 = sdkRootLen - 1;
  while (sdkRootLen1 > 0 && sdkRoot[sdkRootLen1 - 1] != '/')
    sdkRootLen1--;
  if (!sdkRootLen1)
    sdkRootLen1 = -1;

  debug("setSdkRoot: %s  %d %d", sdkRoot, sdkRootLen, sdkRootLen1);
}

const char *TwoStepRelPath::mkRelPath(const char *fpath)
{
  strncpy(buf, fpath, 511);
  buf[511] = 0;
  dd_simplify_fname_c(buf);

  if (!sdkRootLen)
    return buf;
  if (strnicmp(buf, sdkRoot, sdkRootLen) == 0)
    return buf + sdkRootLen;
  if (sdkRootLen1 > 0 && strnicmp(buf, sdkRoot, sdkRootLen1) == 0)
  {
    if (sdkRootLen1 >= 3)
    {
      memcpy(buf + sdkRootLen1 - 3, "../", 3);
      return buf + sdkRootLen1 - 3;
    }
    memmove(buf + 3, buf + sdkRootLen1, strlen(buf + sdkRootLen1));
    memcpy(buf, "../", 3);
  }
  return buf;
}
