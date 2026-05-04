// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <daNet/daNetTypes.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <debug/dag_assert.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <daNet/messageIdentifiers.h>
#include <daNet/daNetPeerInterface.h>
#include <util/dag_hashedKeyMap.h>
#include <math/random/dag_random.h>

#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>


#define __DEBUG_FILEPATH relay_get_log_prefix()


static String relay_get_log_prefix()
{
  String prefix;
  if (::dgs_get_argv("stdout"))
  {
    prefix = "*";
    return prefix;
  }
  if (::dgs_get_argv("log_path"))
  {
    prefix.printf(0, "%s/", ::dgs_get_argv("log_path"));
    return prefix;
  }
  prefix.printf(0, "%s/", ".relay_log");
  return prefix;
}


#include <startup/dag_mainCon.inc.cpp>

#include <enet/enet_dagor.h>

#define BUFFERING_BEFORE_UDP_PUNCH_WAIT_TIMEOUT 10000 // 10 sec

#if DAGOR_DBGLEVEL > 0
static bool verbose_logs_enabled = false;
#else
static const bool verbose_logs_enabled = false;
#endif
static DaNetPeerInterface *peerIface = NULL;
static Tab<SystemIndex> peers;
static ENetHost *stun_validation_host;

typedef HashedKeyMap<uint64_t, ENetHost *, uint64_t(0), oa_hashmap_util::MumStepHash<uint64_t>> ENetAdressToPortMap;
typedef HashedKeyMap<uint16_t, uint64_t, uint16_t(0), oa_hashmap_util::MumStepHash<uint16_t>> PortToENetAdressMap;
ENetAdressToPortMap adress_to_port_map;
PortToENetAdressMap port_to_address_map;

union ENetAdressKey
{
  ENetAddress adress;
  uint64_t u;
};

struct BufferedEarlyMessage
{
  dag::Vector<enet_uint8> data;
  ENetAdressKey key;
  DaNetTime time;
};

Tab<BufferedEarlyMessage> early_messages_buffer;

enet_uint16 listen_to_port = 0;
const char *self_host_str = nullptr;
uint16_t control_port = 0;
enet_uint16 stun_validation_port = 0;
uint16_t client_ports_range_start = 0;
uint16_t client_ports_range_amount = 0;

bool need_to_shutdown = false;
bool got_server_connection = false;


static void sigint_handler(int)
{
  debug("[RELAY] shutdowning...\n\n");
  peerIface->Shutdown(1000);
  delete peerIface;
  exit(0);
}

static bool assertion_handler(bool, const char *file, int line, const char *func, const char *cond, const char *, const DagorSafeArg *,
  int)
{
  fprintf(stderr, "%s:%d: %s() !\"%s\"\n", file, line, func, cond);
  abort();
  return false;
}

static void init_once()
{
  signal(SIGINT, sigint_handler);
  dgs_assertion_handler = assertion_handler;
  measure_cpu_freq();
}


ENetAddress dedicAdress;


static void send_stun_info(ENetHost *from_host, const ENetAddress &target)
{
  danet::BitStream bs;
  bs.Write((char)ID_STUN_INFO);
  bs.Write(target.host);
  bs.Write(target.port);
  ENetBuffer ebuf;
  ebuf.data = (void *)bs.GetData();
  ebuf.dataLength = bs.GetNumberOfBytesUsed();
  // SystemAddress::ToString() uses a static buffer, so store each result before debug
  char fromStr[64], toStr[64];
  strncpy(fromStr, SystemAddress(from_host->address.host, from_host->address.port).ToString(), sizeof(fromStr) - 1);
  fromStr[sizeof(fromStr) - 1] = '\0';
  strncpy(toStr, SystemAddress(target.host, target.port).ToString(), sizeof(toStr) - 1);
  toStr[sizeof(toStr) - 1] = '\0';
  debug("[RELAY] send_stun_info: from %s to %s\n", fromStr, toStr);
  enet_dagor_manual_send_immediately(from_host, &target, &ebuf);
}

static void on_packet(Packet *pkt)
{
  const char *strAddr = pkt->systemAddress.ToString();

  switch (pkt->data[0])
  {
    case ID_NEW_INCOMING_CONNECTION:
    case ID_CONNECTION_REQUEST_ACCEPTED:
      debug("[RELAY] Connected to %s\n", strAddr);
      peers.push_back(pkt->systemIndex);

      if (!got_server_connection)
      {
        debug("[RELAY] %s is registered as dedicated server\n", strAddr);
        dedicAdress.host = pkt->systemAddress.host;
        dedicAdress.port = pkt->systemAddress.port;
        got_server_connection = true;

        if (stun_validation_host)
        {
          ENetPeer *dedic_peer = &peerIface->GetHostMutable()->peers[0];
          debug("[RELAY] asking dedic %s to punch stun_validation port %d (peer state=%d, CONNECTED=%d)\n", strAddr,
            stun_validation_host->address.port, (int)dedic_peer->state, (int)ENET_PEER_STATE_CONNECTED);
          enet_dagor_peer_ping_target_request(dedic_peer, 0, stun_validation_host->address.port);
          debug("[RELAY] sending 3x stun info to dedic %s from stun_validation port %d\n", strAddr,
            stun_validation_host->address.port);
          for (int i = 0; i < 3; i++)
            send_stun_info(stun_validation_host, dedicAdress);
        }
        else
          debug("[RELAY] stun_validation_host not configured, skipping stun check for dedic\n");
      }
      break;
    case ID_DISCONNECT:
      debug("[RELAY] %s %s", strAddr, "disconnected\n");
      erase_item_by_value(peers, pkt->systemIndex);
      need_to_shutdown = true;
      break;
    default: debug("[RELAY] Unknown packet data of type %d size %d from %s\n", (int)pkt->data[0], pkt->length, strAddr);
  }
}


void DagorWinMainInit(int /*nCmdShow*/, bool /*debugmode*/)
{
  signal(SIGINT, sigint_handler);
  dgs_assertion_handler = assertion_handler;
  measure_cpu_freq();
}


static inline uint16_t try_parse_int(const char *str) // return 0 on error
{
  if (!str)
    return 0;
  uint16_t ret = 0;
  char *pend = NULL;
  long port = strtol(str, &pend, 0);
  if (!*pend && port == uint16_t(port))
    ret = (uint16_t)port;
  return ret;
}

_ENetHost *host;

int intercept_network_message_from_dedic_to_client(_ENetHost *host_created_to_handle_packets_to_specific_client, _ENetEvent *event)
{
  event->type = ENET_EVENT_TYPE_DAGOR_INTERCEPTED;
  ENetAddress who = host_created_to_handle_packets_to_specific_client->address;
  uint64_t address = port_to_address_map.findOr(who.port, uint16_t(0));
  ENetAdressKey key;
  key.u = address;
  ENetBuffer ebuf;
  ebuf.data = (void *)host_created_to_handle_packets_to_specific_client->receivedData;
  ebuf.dataLength = host_created_to_handle_packets_to_specific_client->receivedDataLength;
  int sent_length = enet_socket_send(host->socket, &key.adress, &ebuf, /*bufferCount*/ 1);

  if (verbose_logs_enabled)
  {
    debug("[RELAY] intercepted dedic_to_client event %i from %s\n", (int)ebuf.dataLength,
      SystemAddress(dedicAdress.host, dedicAdress.port).ToString());
    debug("[RELAY] to %s %i\n", SystemAddress(key.adress.host, key.adress.port).ToString(), sent_length);
  }


  // if (event->packet)
  //   enet_packet_destroy(event->packet);
  return 1;
}

void send_message_of_client_to_dedic(ENetHost *relayHost, ENetAddress from_whom, ENetBuffer ebuf)
{
  int sent_length = enet_socket_send(relayHost->socket, &dedicAdress, &ebuf, /*bufferCount*/ 1);
  if (verbose_logs_enabled)
  {
    debug("[RELAY] intercepted event %i from %s\n", (int)ebuf.dataLength, SystemAddress(from_whom.host, from_whom.port).ToString());
    debug("[RELAY] translated as %s\n", SystemAddress(relayHost->address.host, relayHost->address.port).ToString());
    debug("[RELAY] to %s %i\n", SystemAddress(dedicAdress.host, dedicAdress.port).ToString(), sent_length);
  }
}


int intercept_udp_hole_punch(_ENetHost *host_created_to_handle_packets_to_specific_client, _ENetEvent *event)
{
  event->type = ENET_EVENT_TYPE_DAGOR_INTERCEPTED;
  ENetAddress who = host_created_to_handle_packets_to_specific_client->address;
  uint64_t address = port_to_address_map.findOr(who.port, uint16_t(0));
  ENetAdressKey key;
  key.u = address;

  debug("[RELAY] intercepted udp hole punch event via %s", SystemAddress(who.host, who.port).ToString());
  debug(" for client %s\n", SystemAddress(key.adress.host, key.adress.port).ToString());

  for (int i = 0; i < early_messages_buffer.size(); i++)
  {
    const BufferedEarlyMessage &msg = early_messages_buffer[i];
    if (msg.key.u != key.u)
      continue;
    ENetBuffer ebuf;
    ebuf.data = (void *)msg.data.data();
    ebuf.dataLength = msg.data.size();
    send_message_of_client_to_dedic(host_created_to_handle_packets_to_specific_client, who, ebuf);
    early_messages_buffer.erase(early_messages_buffer.begin() + i);
    i--;
  }
  host_created_to_handle_packets_to_specific_client->intercept = intercept_network_message_from_dedic_to_client;
  return 2;
}


DaNetPeerExecutionContext context;
void UpdatePeerInterface()
{
  peerIface->ExecuteSingleUpdate(context);
  while (Packet *pkt = peerIface->Receive())
  {
    on_packet(pkt);
    peerIface->DeallocatePacket(pkt);
  }
}

ENetHost *initialize_relay_host_for_client(ENetAdressKey key)
{
  ENetHost *relayHost = nullptr;
  ENetAddress addr;
  addr.host = ENET_HOST_ANY;
  for (int attempts = 0; attempts < 10; attempts++)
  {
    addr.port = dagor_random::rnd_int(client_ports_range_start, client_ports_range_start + client_ports_range_amount - 1);
    relayHost = enet_host_create(&addr, 0, 0, 0, 0);
    if (relayHost)
      break;
  }
  if (!relayHost)
  {
    for (int port_to_try = client_ports_range_start; port_to_try < client_ports_range_start + client_ports_range_amount; port_to_try++)
    {
      addr.port = port_to_try;
      relayHost = enet_host_create(&addr, 0, 0, 0, 0);
      if (relayHost)
        break;
    }
  }

  if (!relayHost)
  {
    debug("Can't add client - no free ports\n");
    return nullptr;
  }

  adress_to_port_map.emplace(key.u, relayHost);
  port_to_address_map.emplace(addr.port, key.u);
  enet_socket_get_address(relayHost->socket, &relayHost->address);
  enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_NONBLOCK, 1);
  enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_BROADCAST, 1);
  enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_RCVBUF, ENET_HOST_RECEIVE_BUFFER_SIZE);
  enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_SNDBUF, ENET_HOST_SEND_BUFFER_SIZE);
  relayHost->intercept = intercept_udp_hole_punch;

  debug("[RELAY] send empty udp punch message to avoid DDOS defence triggering to dedic from %s\n",
    SystemAddress(addr.host, addr.port).ToString());
  enet_dagor_manual_send_immediately(relayHost, &dedicAdress, nullptr);
  ENetPeer *dedic_peer2 = &peerIface->GetHostMutable()->peers[0];
  debug("[RELAY] send command for udp hole punch from dedic on %s (peer state=%d, CONNECTED=%d)\n",
    SystemAddress(addr.host, addr.port).ToString(), (int)dedic_peer2->state, (int)ENET_PEER_STATE_CONNECTED);
  enet_dagor_peer_ping_target_request(dedic_peer2, 0, addr.port);

  return relayHost;
}


int intercept_network_message(_ENetHost *host, _ENetEvent *event)
{
  event->type = ENET_EVENT_TYPE_DAGOR_INTERCEPTED;
  ENetAddress from_whom = host->receivedAddress;
  ENetAdressKey key;
  key.adress = from_whom;
  ENetHost *relayHost = adress_to_port_map.findOr(key.u, nullptr);
  if (DAGOR_UNLIKELY(relayHost == nullptr))
  {
    relayHost = initialize_relay_host_for_client(key);
    if (!relayHost)
      return 1;
  }

  if (DAGOR_UNLIKELY(relayHost->intercept == intercept_udp_hole_punch))
  {
    BufferedEarlyMessage &msg = early_messages_buffer.push_back();
    msg.data = dag::Vector<enet_uint8>(host->receivedData, host->receivedData + host->receivedDataLength);
    msg.key = key;
    msg.time = enet_time_get();
    return 1;
  }
  else
  {
    ENetBuffer ebuf;
    ebuf.data = (void *)host->receivedData;
    ebuf.dataLength = host->receivedDataLength;
    send_message_of_client_to_dedic(relayHost, from_whom, ebuf);
    return 1;
  }
}

void shutdown_host(ENetHost *host_to_shutdown)
{
  enet_socket_destroy(host_to_shutdown->socket);
  for (_ENetPeer *cp = host_to_shutdown->peers; cp < &host_to_shutdown->peers[host_to_shutdown->peerCount]; ++cp)
    enet_peer_reset(cp);
  enet_free(host_to_shutdown->peers);
  enet_free(host_to_shutdown);
}

void shutdown()
{
  if (host)
    shutdown_host(host);
  if (stun_validation_host)
    shutdown_host(stun_validation_host);
  for (auto host : adress_to_port_map)
  {
    shutdown_host(host.val());
  }
  adress_to_port_map.clear();
  port_to_address_map.clear();
  peerIface->Shutdown();
  enet_deinitialize();
}

int DagorWinMain(int /*nCmdShow*/, bool /*debugmode*/)
{
  if (::dgs_argc <= 1) // no args or only exe name
  {
    debug("[RELAY] --listen:INT - sets public port for clients to relay to clientside-dedic\n");
    debug("[RELAY] --control_port:INT - \n");
    debug("[RELAY] --stun_validation_port:INT - \n");
    debug("[RELAY] --client_ports_range_start:INT - range from where to begin trying to allocate client ports\n");
    debug("[RELAY] --client_ports_range_amount:INT - amount of possible client ports\n");
    debug("[RELAY] --host_url:HOST_ADDR_STR - host to bind to (usually just 0.0.0.0)\n");
    debug("[RELAY] --listen_socket:SOCKET_FILE_DESCRIPTOR - (OPTIONAL) for shared socket\n");
    debug("[RELAY] --control_socket:SOCKET_FILE_DESCRIPTOR - (OPTIONAL) for shared socket\n");
    debug("[RELAY] --stun_validation_socket:SOCKET_FILE_DESCRIPTOR - (OPTIONAL) for shared socket\n");
#if DAGOR_DBGLEVEL > 0
    debug("[RELAY] --enable_verbose_logs - (OPTIONAL)\n");
#endif
  }
  debug("[RELAY] START\n");

  dagor_random::set_rnd_seed(0);
  debug("[RELAY] set rnd seed\n");
  cpujobs::init(dgs_get_settings()->getBlockByNameEx("debug")->getInt("coreCount", -1), /*reserve_jobmgr_cores*/ false);
  debug("[RELAY] set cpuJobs\n");
  const char *listen_to_port_str = dgs_get_argv("listen");
  const char *control_port_str = dgs_get_argv("control_port");
  const char *stun_validation_port_str = dgs_get_argv("stun_validation_port");
  const char *client_ports_range_start_str = dgs_get_argv("client_ports_range_start");
  const char *client_ports_range_amount_str = dgs_get_argv("client_ports_range_amount");
  const char *listen_socket_str = dgs_get_argv("listen_socket");
  const char *control_socket_str = dgs_get_argv("control_socket");
  const char *stun_validation_socket_str = dgs_get_argv("stun_validation_socket");
#if DAGOR_DBGLEVEL > 0
  verbose_logs_enabled = dgs_get_argv("enable_verbose_logs") != nullptr;
#endif
  self_host_str = dgs_get_argv("host_url");

  if (!listen_to_port_str)
  {
    debug("[RELAY] ERROR: missing arg --listen:INT\n");
    return 1;
  }

  if (!control_port_str)
  {
    debug("[RELAY] ERROR: missing arg --control_port:INT\n");
    return 1;
  }

  if (!client_ports_range_start_str)
  {
    debug("[RELAY] ERROR: missing arg --client_ports_range_start:INT\n");
    return 1;
  }

  if (!client_ports_range_amount_str)
  {
    debug("[RELAY] ERROR: missing arg --client_ports_range_amount:INT\n");
    return 1;
  }


  listen_to_port = try_parse_int(listen_to_port_str);
  control_port = try_parse_int(control_port_str);
  if (stun_validation_port_str)
    stun_validation_port = try_parse_int(stun_validation_port_str);
  client_ports_range_start = try_parse_int(client_ports_range_start_str);
  client_ports_range_amount = try_parse_int(client_ports_range_amount_str);


  SocketDescriptor listenSD;
  if (listen_socket_str)
  {
    char *eptr = NULL;
    long sockfp = strtol(listen_socket_str, &eptr, 0);
    if (*eptr == '\0' && sockfp > 0)
    {
      listenSD.type = SocketDescriptor::SOCKET;
      listenSD.socket = sockfp;
      listenSD.port = listen_to_port;
    }
  }

  SocketDescriptor controlSD;
  if (control_socket_str)
  {
    char *eptr = NULL;
    long sockfp = strtol(control_socket_str, &eptr, 0);
    if (*eptr == '\0' && sockfp > 0)
    {
      controlSD.type = SocketDescriptor::SOCKET;
      controlSD.socket = sockfp;
      controlSD.port = control_port;
    }
  }


  if (!self_host_str)
  {
    debug("[RELAY] ERROR: missing arg --host_url:HOST_ADDR_STR\n");
    return 1;
  }

  debug("[RELAY] create listen net driver\n");


  G_VERIFY(enet_initialize() == 0);
  enet_time_set(1); // not 0, because 0 has special meaning ("invalid time")
  ENetAddress addr;
  addr.host = ENET_HOST_ANY;
  addr.port = listen_to_port;
  host = enet_host_create(listenSD.port > 0 ? NULL : &addr, 0, 0, 0, 0); // from client to dedic
  if (!host)
  {
    debug("Can't start relay with listen port on %s:%d\n", self_host_str, listen_to_port);
    debug("bind failed: %s\n", strerror(errno));
    return 1;
  }

  if (listenSD.port > 0)
  {
    enet_socket_destroy(host->socket);
    // binded udp socket expected here
    G_ASSERT(listenSD.socket != ENET_SOCKET_NULL);
    host->socket = listenSD.socket;
  }

  enet_socket_get_address(host->socket, &host->address);
  enet_socket_set_option(host->socket, ENET_SOCKOPT_NONBLOCK, 1);
  enet_socket_set_option(host->socket, ENET_SOCKOPT_BROADCAST, 1);
  enet_socket_set_option(host->socket, ENET_SOCKOPT_RCVBUF, ENET_HOST_RECEIVE_BUFFER_SIZE);
  enet_socket_set_option(host->socket, ENET_SOCKOPT_SNDBUF, ENET_HOST_SEND_BUFFER_SIZE);
  host->intercept = intercept_network_message;

  debug("[RELAY] settings %s:%d\n", self_host_str, control_port);


  SocketDescriptor sd = controlSD.port != 0 ? controlSD : SocketDescriptor(control_port, self_host_str);
  peerIface = new DaNetPeerInterface(nullptr, /* is_manual */ true);
  if (!peerIface->Startup(1, 50, &sd))
  {
    debug("Can't start relay with control port on %s:%d\n", self_host_str, control_port);
    return 1;
  }
  else
    debug("[RELAY] Started relay with control port on %s:%d\n", self_host_str, control_port);

  if (stun_validation_port)
  {
    SocketDescriptor stunValidationSD;
    long sockfp = 0;
    if (stun_validation_socket_str)
    {
      char *eptr = NULL;
      sockfp = strtol(stun_validation_socket_str, &eptr, 0);
      if (*eptr == '\0' && sockfp > 0)
      {
        stunValidationSD.type = SocketDescriptor::SOCKET;
        stunValidationSD.socket = sockfp;
        stunValidationSD.port = stun_validation_port;
      }
    }

    ENetAddress addr;
    addr.host = ENET_HOST_ANY;
    addr.port = stun_validation_port;
    stun_validation_host = enet_host_create(stunValidationSD.port > 0 ? NULL : &addr, 0, 0, 0, 0);
    if (!stun_validation_host)
    {
      debug("Can't start relay with stun validation port on %s:%d\n", self_host_str, stun_validation_port);
      debug("bind failed: %s\n", strerror(errno));
      return 1;
    }
    else
    {
      debug("[RELAY] stun validation on %s:%d (sockfp: %ld)\n", self_host_str, stun_validation_port, sockfp);
    }

    if (stunValidationSD.port > 0)
    {
      enet_socket_destroy(stun_validation_host->socket);
      // binded udp socket expected here
      G_ASSERT(stunValidationSD.socket != ENET_SOCKET_NULL);
      stun_validation_host->socket = stunValidationSD.socket;
      debug("[RELAY] stun validation set bound socket\n");
    }
    enet_socket_get_address(stun_validation_host->socket, &stun_validation_host->address);
    enet_socket_set_option(stun_validation_host->socket, ENET_SOCKOPT_NONBLOCK, 1);
    enet_socket_set_option(stun_validation_host->socket, ENET_SOCKOPT_BROADCAST, 1);
    enet_socket_set_option(stun_validation_host->socket, ENET_SOCKOPT_RCVBUF, ENET_HOST_RECEIVE_BUFFER_SIZE);
    enet_socket_set_option(stun_validation_host->socket, ENET_SOCKOPT_SNDBUF, ENET_HOST_SEND_BUFFER_SIZE);

    debug("[RELAY] stun validation setup finished\n");
  }

  context = DaNetPeerExecutionContext();

  int timeout = 30000; // msec
  while (1)
  {
    DaNetTime cur_time = enet_time_get();

    ENetEvent toDedicEvent;
    int r = enet_dagor_host_service_only_incoming_commands(host, &toDedicEvent); // send/receive
    if (r < 0)
    {
#if _TARGET_PC_WIN
      int last_error = WSAGetLastError();
#else
      int last_error = errno;
#endif
      G_UNUSED(last_error);
      debug("[RELAY] ERROR: fromClienttoDedic enet_host_service() failed, last_error = %d\n", last_error);
      return 1;
    }

    // debug("[RELAY] update peer interface\n");
    UpdatePeerInterface();

    // debug("[RELAY] update peer interface end\n");
    for (auto host : adress_to_port_map)
    {
      ENetHost *dedic_to_client_listener_host = host.val();

      ENetEvent toClientEvent;
      int r = enet_dagor_host_service_only_incoming_commands(dedic_to_client_listener_host, &toClientEvent); // send/receive
      if (r < 0)
      {
#if _TARGET_PC_WIN
        int last_error = WSAGetLastError();
#else
        int last_error = errno;
#endif
        G_UNUSED(last_error);
        debug("[RELAY] ERROR: fromDedicToClient enet_host_service() failed, last_error = %d\n", last_error);
        return 1;
      }

      for (int i = early_messages_buffer.size() - 1; i >= 0; i--)
      {
        const BufferedEarlyMessage &msg = early_messages_buffer[i];
        // 10 sec, if there were no punch during this time connection reasonably be considered invalid
        if (cur_time - msg.time > BUFFERING_BEFORE_UDP_PUNCH_WAIT_TIMEOUT)
          early_messages_buffer.erase(early_messages_buffer.begin() + i);
      }
    }

    if (stun_validation_host)
    {
      int r = enet_host_service(stun_validation_host, /*event*/ NULL, /*timeout*/ 0); // send/receive
      if (r < 0)
      {
#if _TARGET_PC_WIN
        int last_error = WSAGetLastError();
#else
        int last_error = errno;
#endif
        G_UNUSED(last_error);
        debug("[RELAY] ERROR: stun_validation enet_host_service() failed, last_error = %d\n", last_error);
      }
      bool no_new_events = false;
      do
      {
        ENetEvent stunEvent;
        enet_host_check_events(stun_validation_host, &stunEvent);
        switch (stunEvent.type)
        {
          case ENET_EVENT_TYPE_DAGOR_RECEIVE_UNCONNECTED:
            G_ASSERT(stunEvent.packet);
            debug("[RELAY] stun_validation: unconnected datagram from %s size=%u\n",
              SystemAddress(stunEvent.address.host, stunEvent.address.port).ToString(), stunEvent.packet->dataLength);
            if (stunEvent.address.host == dedicAdress.host && stunEvent.address.port == dedicAdress.port)
            {
              // Dedicated punched stun_validation_port - send stun info back from this port
              debug("[RELAY] stun_validation: punch from dedic %s -> sending stun info back\n",
                SystemAddress(stunEvent.address.host, stunEvent.address.port).ToString());
              send_stun_info(stun_validation_host, stunEvent.address);
            }
            else
            {
              // Client punch request: tell dedicated to ping the client's external address
              ENetPeer *dedic_peer3 = &peerIface->GetHostMutable()->peers[0];
              debug("[RELAY] stun_validation: punch from client %s -> asking dedic %s to ping client (peer state=%d, CONNECTED=%d)\n",
                SystemAddress(stunEvent.address.host, stunEvent.address.port).ToString(),
                SystemAddress(dedicAdress.host, dedicAdress.port).ToString(), (int)dedic_peer3->state, (int)ENET_PEER_STATE_CONNECTED);
              enet_dagor_peer_ping_target_request(dedic_peer3, stunEvent.address.host, stunEvent.address.port);
            }
            enet_packet_destroy(stunEvent.packet);
            break;
          case ENET_EVENT_TYPE_NONE: no_new_events = true; break;
          default: debug("[RELAY] stun_validation: unexpected event type %d\n", (int)stunEvent.type); no_new_events = true;
        };
      } while (!no_new_events);
    }

    if (need_to_shutdown)
    {
      shutdown();
      sleep_msec(500);
      break;
    }
    sleep_msec(1);
    timeout -= 1;
    if (timeout == 0 && !need_to_shutdown && !got_server_connection)
    {
      debug("[RELAY] TIMEOUT: no server control connection established - exiting\n");
      need_to_shutdown = true;
    }
  }

  return 0;
}

void DagorWinMainInit(bool debugmode) { DagorWinMainInit(0, debugmode); }
int DagorWinMain(bool debugmode) { return DagorWinMain(0, debugmode); }

#if _TARGET_PC_LINUX
int os_message_box(const char *, const char *, int) { return 0; }
#endif