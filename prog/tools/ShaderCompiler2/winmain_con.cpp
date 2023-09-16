static const char *get_debug_fname();
#define __DEBUG_FILEPATH          get_debug_fname()
#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#include <startup/dag_mainCon.inc.cpp>
#include <osApiWrappers/dag_localConv.h>

#if _TARGET_PC_MACOSX
extern int __argc;
extern char **__argv;
#endif

static const char *get_debug_fname()
{
  const char *log_dir = NULL;
  bool supressLogs = false;
  const char *log_prefix =
#if defined(_CROSS_TARGET_C1)

#elif defined(_CROSS_TARGET_C2)

#elif defined(_CROSS_TARGET_DX12)
    "ShaderLog-DX12"
#elif defined(_CROSS_TARGET_DX11)
    "ShaderLog-DX11"
#elif defined(_CROSS_TARGET_SPIRV)
    "ShaderLog-SPIRV"
#elif defined(_CROSS_TARGET_METAL)
    "ShaderLog-Metal"
#else
    "ShaderLog"
#endif
    ;
  static char log_fn[260];

  for (int i = 1; i < __argc; i++)
  {
    if (dd_stricmp(__argv[i], "-supressLogs") == 0)
      supressLogs = true;
    else if (dd_stricmp(__argv[i], "-logdir") == 0 && i + 1 < __argc)
      log_dir = __argv[i + 1];
  }
  if (supressLogs)
    return NULL;

  for (int i = 1; i < __argc; i++)
    if (dd_stricmp(__argv[i], "-c") == 0 && i + 1 < __argc)
    {
      if (log_dir)
        SNPRINTF(log_fn, sizeof(log_fn), "%s/%s.c/%s.log", log_dir, log_prefix, __argv[i + 1]);
      else
        SNPRINTF(log_fn, sizeof(log_fn), "%s.c/%s.log", log_prefix, __argv[i + 1]);
      for (char *p = log_fn + strlen(log_prefix) + (log_dir ? strlen(log_dir) + 1 : 0) + 3; *p; p++)
        if (*p == '\\' || *p == '/')
          *p = '_';
      dd_mkpath(log_fn);
      return log_fn;
    }

  if (log_dir)
  {
    SNPRINTF(log_fn, sizeof(log_fn), "%s/%s.log", log_dir, log_prefix);
    dd_mkpath(log_fn);
    return log_fn;
  }
  return log_prefix;
}
