//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace net
{

bool parse_net_url(const char *url, char *hostname, int hsz, int &port);

#define PARSEURL(url, def, p)                                  \
  char host[128] = def;                                        \
  int port = (p);                                              \
  if (!net::parse_net_url(url, host, (int)sizeof(host), port)) \
  return false

#define TRYPARSEURL(url, def, p) \
  bool parsed = false;           \
  char host[128] = def;          \
  int port = (p);                \
  parsed = net::parse_net_url(url, host, (int)sizeof(host), port);

}; // namespace net
