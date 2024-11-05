// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace net
{

bool parse_net_url(const char *url, char *hostname, int hsz, int &port);

#define PARSEURL(url, def, p)                             \
  char host[128] = def;                                   \
  int port = (p);                                         \
  if (!parse_net_url(url, host, (int)sizeof(host), port)) \
  return false

}; // namespace net
