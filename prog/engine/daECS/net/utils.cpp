// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/urlUtils.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

namespace net
{

bool parse_net_url(const char *url, char *hostname, int hsz, int &port)
{
  if (!url || !*url)
    return true;
  const char *pColon = strrchr(url, ':');
  if (pColon)
  {
    int hlen = pColon - url;
    if (hlen >= hsz)
      return false;
    char *eptr = NULL;
    port = (int)strtol(pColon + 1, &eptr, 10);
    if (*eptr != '\0' || port < 0 || port > USHRT_MAX)
      return false;
    memcpy(hostname, url, hlen);
    hostname[hlen] = '\0';
  }
  else
  {
    strncpy(hostname, url, hsz - 1);
    hostname[hsz - 1] = '\0';
  }
  return true;
}

} // namespace net
