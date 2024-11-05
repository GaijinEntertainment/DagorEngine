// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stats.h"

#include "configuration.h"
#include "log.h"

#include <osApiWrappers/dag_sockets.h>

#include <cstring>
#include <iostream>
#include <string>
#include <stdint.h>


namespace breakpad
{
namespace stats
{

static sockets::Socket sock(OSAF_IPV4, OST_UDP, false);
static std::string tags;

void init(const Configuration &cfg)
{
  if (cfg.stats.host.empty() || os_sockets_init() != 0)
  {
    log << "Stats init failed (host '" << cfg.stats.host << "')" << std::endl;
    return;
  }

  sock.reset();
  if (sock)
  {
    tags = ",env=" + cfg.stats.env + ",host=end-client-host,project=gaijin,application=bpreport";
    tags += ",on_behalf_of=" + cfg.product + ",crash_type=" + cfg.crashType;
    sockets::SocketAddr<OSAF_IPV4> addr(cfg.stats.host.c_str(), cfg.stats.port);
    sock.connect(addr);
  }
  log << "Stats init " << (sock ? "ok" : "failed") << ": tags string '" << tags << "'" << std::endl;
}


void send(const char *m, const char *s, const char *t, long v)
{
  if (!sock)
    return;

  std::string data = m + tags + ",status=" + s + ":" + std::to_string(v) + "|" + t;
  log << "Stats send: '" << data << "'" << std::endl;
  sock.send(data.c_str(), data.length());
}


void shutdown() { os_sockets_shutdown(); }

} // namespace stats
} // namespace breakpad
