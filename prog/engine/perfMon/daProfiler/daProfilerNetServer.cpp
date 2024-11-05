// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilerDumpServer.h"
#include "daProfilerMessageServer.h"
#include "daProfilerNetwork.h"
#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include "daProfilerMessageTypes.h"
#include "stl/daProfilerStl.h"
#include <ioSys/dag_memIo.h>

#if _TARGET_XBOX
static constexpr int DEFAULT_PORT = 4601; // port on xbox SHOULD be 4601.
#else
static constexpr int DEFAULT_PORT = 31500;
#endif
// network / another dump saving server
// game send dumps to servers, servers process them.
// this one sends dump to socket for daProfler GUI tool
//  works on a blocking sockets, synchroniously (but called from thread)

namespace da_profiler
{
static const char *net_server_name = "$NetDumpClient@";

extern void response_plugins(IGenSave &cb, const hash_map<string, bool> &plugins);

struct MessageHeader
{
  uint32_t mark = 0;
  uint32_t length = 0;

  static constexpr uint32_t MESSAGE_MARK = 0xC50FC50F;

  bool IsValid() const { return mark == MESSAGE_MARK; }
};


class AcceptedClientServer
{
public:
  SocketHandler sock;
  AcceptedClientServer() = default;
  AcceptedClientServer(SocketHandler &&s) : sock((SocketHandler &&) s) {}
  void update()
  {
    if (!sock)
      return;
  }
};

class NetServer final : public ProfilerDumpClient
{
public:
  AcceptedClientServer client;

private:
  SocketHandler listenSocket;
  NetTCPZlibSave compressedOutStream;
  enum
  {
    UNCOMPRESSED = 0,
    ZLIB_ALGO = 1,
    ZSTD_ALGO = 2
  } algo = UNCOMPRESSED; // bits
  void reinitCompressed()
  {
    compressedOutStream.~NetTCPZlibSave();
    new (&compressedOutStream, _NEW_INPLACE) NetTCPZlibSave(client.sock.get());
  }
  IGenSave &optionalCompressed()
  {
    return algo == ZLIB_ALGO ? (IGenSave &)compressedOutStream : (IGenSave &)compressedOutStream.rawTcp;
  }

public:
  NetServer() : compressedOutStream(client.sock.get()) { os_sockets_init(); }
  ~NetServer() { os_sockets_shutdown(); }
  bool init(int start_port, uint32_t port_range)
  {
    SocketHandler sock(os_socket_create(OSAF_IPV4, OST_TCP));
    char buf[256];
    if (!sock)
    {
      logerr("Can't init profiler net server %s", os_socket_error_str(os_socket_last_error(), buf, sizeof(buf)));
      return false;
    }
    os_socket_addr addr;
    os_socket_addr_from_string(OSAF_IPV4, "0.0.0.0", &addr, sizeof(addr));
    int port = start_port, end_port = start_port + port_range;
    for (; port < end_port; ++port)
    {
      os_socket_addr_set_port(&addr, sizeof(addr), port);
      if (os_socket_bind(sock.get(), &addr, sizeof(addr)) == 0)
        break;
      else
        debug("Starting dump client on port %d is impossible %s", port, os_socket_error_str(os_socket_last_error(), buf, sizeof(buf)));
    }
    if (port == end_port)
      logerr("Can't bind profiler net client on ports %d..%d %s", start_port, end_port,
        os_socket_error_str(os_socket_last_error(), buf, sizeof(buf)));
    if (os_socket_listen(sock.get(), 4) != 0)
    {
      logerr("Can't start listen profiler net client on port %d %s", port,
        os_socket_error_str(os_socket_last_error(), buf, sizeof(buf)));
      return false;
    }
    os_socket_set_option(sock.get(), OSO_NONBLOCK, 1); // prefer non-blocking sockets for listening
    os_socket_set_reuse_addr(sock.get(), true);
    debug("Listening to profiler client on port %d", port);
    listenSocket = ((SocketHandler &&) sock);
    return true;
  }
  template <bool send>
  bool updateSocket()
  {
    if (client.sock && os_socket_error(client.sock.get()) == 0 &&
        (send ? os_socket_send(client.sock.get(), nullptr, 0) == 0 : os_socket_read_select(client.sock.get()) != -1))
      return true;
    if (client.sock)
      client.sock.close();
    const int status = os_socket_read_select(listenSocket.get());
    if (status <= 0)
      return false;
    os_socket_addr from;
    int fromLen = sizeof(from);
    SocketHandler incomingSocket(os_socket_accept(listenSocket.get(), &from, &fromLen));
    if (!incomingSocket)
      return false;
    uint32_t host = 0;
    os_socket_addr_get(&from, sizeof(from), &host);
    uint32_t port = os_socket_addr_get_port(&from, sizeof(from));
    G_UNUSED(port);
    reinitCompressed();
    debug("got profiler connection from %d.%d.%d.%d:%d", host & 0xFF, (host >> 8) & 0xFF, (host >> 16) & 0xFF, host >> 24, port);
    settingsGen = pluginsGen = 1 << 30;
    client.sock = ((SocketHandler &&) incomingSocket);
    os_socket_set_option(client.sock.get(), OSO_NONBLOCK, 0); // we prefer blocking socket. as long as connection established
    {
      struct FirstMessage
      {
        MessageHeader hdr;
        uint16_t messageType;
      } msg;
      const ReadStatus rd = read(&msg, sizeof(MessageHeader) + sizeof(uint16_t));
      if (rd != ReadStatus::Ok || !msg.hdr.IsValid())
      {
        debug("can't read correct first message %d", msg.hdr.IsValid());
        return false;
      }
      if (msg.messageType == Connected)
      {
        debug("connected profiler frontend doesn't support compression (or doesn't want to)");
        algo = UNCOMPRESSED;
      }
      else if (msg.messageType == ConnectedCompression)
      {
        uint16_t compressionAlgorithms = 0;
        if (read(&compressionAlgorithms, sizeof(compressionAlgorithms)) != ReadStatus::Ok)
        {
          debug("invalid ConnectedCompression message or disconnect");
          return false;
        }
        if (compressionAlgorithms & ZLIB_ALGO)
          algo = ZLIB_ALGO;
      }
      debug("frontend compression is %d", algo);
    }
    response_handshake(compressedOutStream.rawTcp, algo);
    debug("handshake sent");
    return true;
  }
  int64_t lastHeartBeatTick = 0;
  bool heartbeat()
  {
    if (!profile_usec_passed(lastHeartBeatTick, 500000)) // 500msec. It is rather rare, but with live frame profiling we have much more
                                                         // often update anyway
      return true;
    if (!updateSocket<true>() || !client.sock)
      return false;
    response_heartbeat(optionalCompressed());
    bool ret = updateSocket<true>() && client.sock;
    lastHeartBeatTick = profile_ref_ticks();
    return ret;
  }

  void sendPlugins(const hash_map<string, bool> &plugins, uint32_t gen)
  {
    if (pluginsGen != gen)
    {
      if (!updateSocket<true>() || !client.sock)
        return;
      response_plugins(optionalCompressed(), plugins);
      pluginsGen = gen;
    }
  }
  void liveFrame(const uint64_t *frame_times, uint32_t count, uint32_t available)
  {
    if (!updateSocket<true>() || !client.sock)
      return;
    response_live_frames(optionalCompressed(), available, frame_times, count);
  }
  bool reportsLiveFrame() const { return bool(client.sock); }

  // ProfilerDumpClient
  const char *getDumpClientName() const override { return net_server_name; }
  DumpProcessing process(const Dump &dump) override
  {
    if (!updateSocket<true>() || !client.sock)
      return DumpProcessing::Continue;
    the_profiler.saveDump(optionalCompressed(), dump);
    // ProfilerData::finish(cb);
    return DumpProcessing::Continue;
  }

  bool sendSettings(const UserSettings &s)
  {
    if (!updateSocket<true>() || !client.sock)
      return false;
    response_settings(optionalCompressed(), s);
    return (updateSocket<true>() && client.sock);
  }
  enum class ReadStatus
  {
    Ok,
    NoData,
    Error
  };
  ReadStatus read(void *ptr_, size_t size) // either reads full data, or return error
  {
    const int status = os_socket_read_select(client.sock.get());
    if (status == 0)
      return ReadStatus::NoData;
    if (status == -1)
    {
      client.sock.close();
      return ReadStatus::Error;
    }
    int sz = blocked_socket_recvfrom(client.sock.get(), (char *)ptr_, size);
    if (sz == size)
      return ReadStatus::Ok;
    client.sock.close();
    return ReadStatus::Error;
  }
  uint32_t settingsGen = 1 << 30, pluginsGen = 1 << 31;
  DumpProcessing updateDumpClient(const UserSettings &s) override
  {
    if (!heartbeat())
      return DumpProcessing::Continue;

    if (settingsGen != s.generation)
    {
      if (!sendSettings(s))
        return DumpProcessing::Continue;
      settingsGen = s.generation;
    }
    if (!updateSocket<false>() || !client.sock)
      return DumpProcessing::Continue;
    vector<char> messageMem;
    while (true)
    {
      MessageHeader hdr;
      const ReadStatus rd = read(&hdr, sizeof(hdr));
      if (rd != ReadStatus::Ok)
        break;
      if (!hdr.IsValid())
        continue;
      NetTCPLoad loadCb(client.sock.get());
      uint16_t messageType = uint16_t(~uint16_t(0));
      loadCb.read(&messageType, sizeof(messageType));

      if (hdr.length >= sizeof(messageType))
      {
        messageMem.resize(hdr.length - sizeof(messageType));
        if (messageMem.size())
          loadCb.read(messageMem.data(), messageMem.size());
        if (!client.sock)
          break;
        InPlaceMemLoadCB memCb(messageMem.data(), messageMem.size());
        read_and_perform_message(messageType, messageMem.size(), memCb, optionalCompressed());
      }
      if (messageType == Disconnect)
      {
        debug("net client disconnected");
        client.sock.close();
        break;
      }
    }
    return DumpProcessing::Continue;
  }
  int priority() const override { return 0; }
  void removeDumpClient() override
  {
    client.sock.close();
    listenSocket.close();
    delete this;
  }
};

bool stop_network_dump_server() { return remove_dump_client_by_name(net_server_name); }

bool start_network_dump_server(int port)
{
  stop_network_dump_server();
  unique_ptr<NetServer> n = make_unique<NetServer>();
  if (!n->init(port == 0 ? DEFAULT_PORT : port, 4))
    return false;
  add_dump_client(n.release());
  return true;
}

} // namespace da_profiler
