// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "userid.h"
#include <matching/auth.h>
#include <openssl/rand.h>               // RAND_bytes
#include <zlib.h>                       // crc32
#include <osApiWrappers/dag_sockets.h>  // os_gethostname
#include <startup/dag_globalSettings.h> // dgs_get_argv
#include <osApiWrappers/dag_miscApi.h>  // get_platform_string_id
#include <stdlib.h>                     // strtoull, getenv
#include <limits.h>                     // ULLONG_MAX
#include <string.h>
#include "dedicated.h"
#include <generic/dag_tab.h>
#include <crypto/base64.h>
#if _TARGET_PC_WIN
#include <windows.h> // GetUserNameA
#endif
#include "main/appProfile.h"

namespace net
{
static matching::UserId cachedUserId = 0;
static matching::SessionId cachedSessionId = matching::INVALID_SESSION_ID - 1;

matching::UserId get_user_id()
{
  if (cachedUserId) // fast path
    return cachedUserId;
  if (auto &userid = app_profile::get().userId; !userid.empty())
  {
    char *eptr = NULL;
    long long uid = strtoull(userid.c_str(), &eptr, 0);
    if (!*eptr)
      return cachedUserId = uid;
  }
  // No valid command line was found -> auto generate user id
  char hostname[128];
  os_gethostname(hostname, sizeof(hostname), get_platform_string_id());
  uint16_t bindedPort = dedicated::get_binded_port();
  cachedUserId = crc32(0, (const unsigned char *)hostname, strlen(hostname)); // bits 0..31 - hash of hostname

  if (bindedPort) // dedicated's code path
  {
    union
    {
      uint32_t u32;
      uint8_t data[sizeof(uint32_t)];
    } rnd32 = {0};
    RAND_bytes(rnd32.data, sizeof(rnd32.data));
    cachedUserId |= uint64_t(bindedPort) << 32; // bits 32..47 - port
    cachedUserId |= uint64_t(rnd32.u32) << 48;  // bits 48..62 - random
  }
  else // client's code path
  {
    const char *uname = get_user_name();
    cachedUserId |= uint64_t(crc32(0, (const unsigned char *)uname, strlen(uname))) << 32; // bits 32..62 - hash of username
  }
  cachedUserId |= 1ULL << 63; // set high (63) bit to avoid collision with real user ids
  return cachedUserId;
}

const char *get_user_name()
{
  const char *username = dgs_get_argv("user_name");
  return username ? username : "{Local Player}";
}

matching::SessionId get_session_id()
{
  if (cachedSessionId != (matching::INVALID_SESSION_ID - 1)) // fast path
    return cachedSessionId;
  if (auto &session_id = app_profile::get().serverSessionId; !session_id.empty())
  {
    char *eptr = NULL;
    long long sid = strtoull(session_id.c_str(), &eptr, 0);
    if (!*eptr)
      return cachedSessionId = sid;
  }
  return cachedSessionId = matching::INVALID_SESSION_ID;
}

void clear_cached_ids()
{
  cachedUserId = 0;
  cachedSessionId = matching::INVALID_SESSION_ID - 1;
}

} // namespace net
