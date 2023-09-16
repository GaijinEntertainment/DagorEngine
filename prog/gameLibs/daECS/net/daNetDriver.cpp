#include <daECS/net/connection.h>
#include <daECS/net/message.h>
#include <EASTL/unique_ptr.h>
#include <daNet/daNetPeerInterface.h>
#include "utils.h"

#define DANET_SLEEP_TIME_MS 10
#define DEFAULT_PORT        20010
#define MAX_PORTS_TO_TRY    128

namespace net
{

extern const int ID_ENTITY_MSG_HEADER_SIZE;

class DaNetDriver final : public INetDriver
{
public:
  DaNetDriver(DaNetPeerInterface *daif)
  {
    G_ASSERT(daif == &getPeerIface());
    G_UNUSED(daif);
  }

  DaNetPeerInterface &getPeerIface()
  {
    constexpr size_t offs = (sizeof(*this) + alignof(DaNetPeerInterface) - 1) & ~(alignof(DaNetPeerInterface) - 1);
    return *(DaNetPeerInterface *)(void *)((size_t)(void *)this + offs);
  }
  const DaNetPeerInterface &getPeerIface() const { return const_cast<DaNetDriver *>(this)->getPeerIface(); }

  void destroy() override
  {
    eastl::destroy_at(this);
    eastl::destroy_at(&getPeerIface());
    memfree(this, defaultmem);
  }

  bool initListen(const char *listenurl, int max_connections, uint16_t *o_port)
  {
    max_connections = eastl::max(max_connections, 2); // for isServer() implementation
    PARSEURL(listenurl, "", DEFAULT_PORT);
    host[0] = '\0'; // always bind to INADDR_ANY as listenurl is public ip which might not be equal to available internal one
    SocketDescriptor sd(port, host);
    for (int i = 0; i < (o_port ? MAX_PORTS_TO_TRY : 1); ++i)
    {
      sd.port = port + i;
      if (getPeerIface().Startup(max_connections, DANET_SLEEP_TIME_MS, &sd))
      {
        if (o_port)
          *o_port = sd.port;
        return true;
      }
    }
    return false; // failed to bind port?
  }

  bool initListen(const SocketDescriptor &sd, int max_connections)
  {
    return getPeerIface().Startup(eastl::max(max_connections, 2), DANET_SLEEP_TIME_MS, &sd);
  }

  bool initConnect(const char *connecturl, uint16_t ext_protov)
  {
    if (!getPeerIface().Startup(/*max_conn*/ 1, DANET_SLEEP_TIME_MS))
      return false;
    return connect(connecturl, ext_protov);
  }

  virtual bool connect(const char *connecturl, uint16_t ext_protov)
  {
    PARSEURL(connecturl, "127.0.0.1", DEFAULT_PORT);
    uint32_t protov = (MessageClass::calcNumMessageClasses() << 16) | ext_protov;
    return getPeerIface().Connect(host, port, protov);
  }

  virtual void *getControlIface() const override { return (void *)&getPeerIface(); }
  virtual eastl::optional<danet::EchoResponse> receiveEchoResponse() override { return getPeerIface().ReceiveEchoResponse(); }
  virtual Packet *receive(int) override { return getPeerIface().Receive(); }
  virtual void free(Packet *pkt) override { getPeerIface().DeallocatePacket(pkt); }
  virtual void shutdown(int wait_ms) override { getPeerIface().Shutdown(wait_ms); }
  virtual void stopAll(DisconnectionCause cause) override { getPeerIface().Stop(cause); }
  virtual bool isServer() const override
  {
    return getPeerIface().GetMaximumIncomingConnections() > 1;
  } // Kind of hacky, but doesn't requires additional state (e.g. bool)
  virtual DaNetTime getLastReceivedPacketTime(ConnectionId cid) const override
  {
    return getPeerIface().GetLastReceivedPacketTime(cid);
  }
};

class DaNetConnection final : public Connection
{
public:
  DaNetPeerInterface *peerIface;

  DaNetConnection(INetDriver *drv, ConnectionId id_, scope_query_cb_t &&scope_query_) :
    Connection(id_, eastl::move(scope_query_)), peerIface(static_cast<DaNetPeerInterface *>(drv->getControlIface()))
  {
    G_ASSERT(peerIface);
  }
  virtual bool isBlackHole() const override { return false; }
  virtual void sendEcho(const char *route, uint32_t route_id) override { peerIface->SendEcho(route, route_id); }
  virtual bool send(int, const danet::BitStream &bs, PacketPriority prio, PacketReliability rel, uint8_t chn,
    int dup_delay_ms) override
  {
    return peerIface->Send(bs, prio, rel, chn, getId(), /*bcast*/ false, dup_delay_ms);
  }
  virtual int getMTU() const override { return peerIface->GetMaximumPacketSize(getId()) - ID_ENTITY_MSG_HEADER_SIZE; }
  uint32_t getIP() const override { return peerIface->GetPeerSystemAddress(getId()).host; }
  const char *getIPStr() const override { return peerIface->GetPeerSystemAddress(getId()).ToString(/*port*/ false); }
  virtual danet::PeerQoSStat getPeerQoSStat() const override { return peerIface->GetPeerQoSStat(getId()); }
  virtual bool changeSendAddress(const char *new_host) override { return getId() == 0 && peerIface->ChangeHostAddress(new_host); }
  virtual void allowReceivePlaintext(bool allow) override { peerIface->AllowReceivePlaintext(getId(), allow); }
  virtual void disconnect(DisconnectionCause cause) override
  {
    Connection::disconnect(cause);
    peerIface->CloseConnection(getId(), cause);
  }
  virtual bool isResponsive() const override { return peerIface->IsPeerResponsive(getId()); }
};

static eastl::unique_ptr<DaNetDriver, DestroyDeleter<DaNetDriver>> create_net_driver_common()
{
  void *mem = nullptr;
  DaNetPeerInterface *daif = DaNetPeerInterface::create(sizeof(DaNetDriver), &mem);
  return eastl::unique_ptr<DaNetDriver, DestroyDeleter<DaNetDriver>>(new (mem, _NEW_INPLACE) DaNetDriver(daif));
}

INetDriver *create_net_driver_listen(const char *listenurl, int max_connections, uint16_t *o_port)
{
  auto drv = create_net_driver_common();
  return drv->initListen(listenurl, max_connections, o_port) ? drv.release() : NULL;
}

INetDriver *create_net_driver_listen(const SocketDescriptor &sd, int max_connections)
{
  auto drv = create_net_driver_common();
  return drv->initListen(sd, max_connections) ? drv.release() : NULL;
}

INetDriver *create_net_driver_connect(const char *connecturl, uint16_t protov)
{
  auto drv = create_net_driver_common();
  return drv->initConnect(connecturl, protov) ? drv.release() : NULL;
}

Connection *create_net_connection(INetDriver *drv, ConnectionId id, scope_query_cb_t &&scope_query)
{
  return new DaNetConnection(drv, id, eastl::move(scope_query));
}

}; // namespace net
