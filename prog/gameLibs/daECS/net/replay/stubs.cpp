#include <daECS/net/netbase.h>
#include <daECS/net/connection.h>

class StubNetDriver final : public net::INetDriver
{
public:
  virtual void *getControlIface() const override { return NULL; }
  virtual Packet *receive(int) override { return NULL; }
  virtual void free(Packet *) override {}
  virtual void shutdown(int) override {}
  virtual void stopAll(DisconnectionCause) override {}
  virtual bool isServer() const override { return true; }
};

class StubConnection final : public net::Connection
{
public:
  StubConnection() : Connection(0) {}
  virtual bool isBlackHole() const override { return true; }
  virtual void sendEcho(const char *, uint32_t) override{};
  virtual bool send(int, const danet::BitStream &, PacketPriority, PacketReliability, uint8_t, int) override { return true; }
  virtual int getMTU() const override { return 1 << 30; } // unlimited
  uint32_t getIP() const override { return 0; }
  const char *getIPStr() const override { return nullptr; }
  virtual danet::PeerQoSStat getPeerQoSStat() const override { return danet::PeerQoSStat{}; }
  virtual void disconnect(DisconnectionCause) override { G_ASSERTF(0, "Not supported!"); }
  virtual bool isResponsive() const override { return true; }
};

net::INetDriver *net::create_stub_net_driver() { return new StubNetDriver; }

net::Connection *net::create_stub_connection() { return new StubConnection; }
