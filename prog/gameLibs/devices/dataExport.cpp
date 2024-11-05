// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <devices/dataExport.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_sockets.h>

namespace dataexport
{
#define UDP_PACKET_MAX_SIZE (1024 - 28) // MTU
class UDPDataExport : public DataExportInterface
{
public:
  UDPDataExport() : inited(false), sock(OSAF_IPV4, OST_UDP, false) {}
  virtual void init(const DataBlock &cfg)
  {
    if (!init_socket())
      return;
    if (!connect(cfg.getStr("udpAddr", ""), cfg.getInt("udpPort", 0)))
    {
      shutdown_socket();
      return;
    }
    inited = true;
  }
  virtual void destroy()
  {
    shutdown_socket();
    delete this;
  }
  virtual void send(void *buf, short len)
  {
    if (!inited || !sock)
      return;
    G_ASSERT(len <= UDP_PACKET_MAX_SIZE);
    if (len > UDP_PACKET_MAX_SIZE)
      return;
    int nsent = sock.send((const char *)buf, len);
    if (nsent < 0 && sock.getLastError() != OS_SOCKET_ERR_WOULDBLOCK)
      logerr("%s: failed to send data to dataexport: %d", __FUNCTION__, sock.getLastError());
  }

protected:
  bool init_socket()
  {
    char errStr[512];
    sock.reset();
    if (!sock)
    {
      logerr("%s: failed to reinitialize dataexport socket: %s (%d)", __FUNCTION__, sock.getLastErrorStr(errStr, sizeof(errStr)),
        sock.getLastError());
      return false;
    }
    if (sock.setopt(OSO_NONBLOCK, true) != 0)
    {
      logerr("%s: failed to set non-blocking mode for dataexport socket: %s (%d)", __FUNCTION__,
        sock.getLastErrorStr(errStr, sizeof(errStr)), sock.getLastError());
      sock.close();
      return false;
    }

    return true;
  }
  void shutdown_socket() { sock.close(); }
  bool connect(const char *addr, int port)
  {
    G_ASSERT(addr && *addr);

    if (!sock)
    {
      logerr("%s: uninitialized socket", __FUNCTION__);
      return false;
    }

    debug("%s: dataexport address: %s:%d", __FUNCTION__, addr, port);

    sockets::SocketAddr<OSAF_IPV4> ipv4Addr(addr, (uint16_t)port);
    if (!ipv4Addr.isValid())
    {
      logerr("%s: bad dataexport address: %s:%d", __FUNCTION__, addr, port);
      return false;
    }

    if (sock.connect(ipv4Addr) != 0)
    {
      char errStr[512];
      logerr("%s: connect() call failed: %s", __FUNCTION__, sock.getLastErrorStr(errStr, sizeof(errStr)));
      return false;
    }

    return true;
  }

  bool inited;
  sockets::Socket sock;
};

DataExportInterface *create(const DataBlock &cfg)
{
  if (cfg.paramExists("udpAddr") && cfg.paramExists("udpPort"))
  {
    DataExportInterface *ret = new UDPDataExport();
    ret->init(cfg);
    return ret;
  }

  return NULL;
};
}; // namespace dataexport
