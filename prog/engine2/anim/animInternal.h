#pragma once
#include <startup/dag_globalSettings.h>
#include <debug/dag_log.h>

#define SIZE_OF_FATAL_CTX_TMPBUF (2 << 10)

#if _TARGET_STATIC_LIB
template <typename... Args>
__forceinline void animerr_helper(const char *fmt, const Args &...args)
{
  char tmpbuf[SIZE_OF_FATAL_CTX_TMPBUF];
  const DagorSafeArg a[sizeof...(args) + 1] = {args..., dgs_get_fatal_context(tmpbuf, sizeof(tmpbuf))};
  logmessage_fmt(LOGLEVEL_ERR, fmt, a, sizeof...(args) + 1);
}
#define ANIM_ERR(fmt, ...) animerr_helper(fmt "%s", ##__VA_ARGS__)
#else
#define ANIM_ERR fatal_x
#endif
