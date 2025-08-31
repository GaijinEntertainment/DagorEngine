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

#include <startup/dag_mainCon.inc.cpp>

#include <enet/enet.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define ID_DEDIC_REGISTER_MESSAGE ID_USER_PACKET_ENUM

static DaNetPeerInterface *peerIface = NULL;
static Tab<SystemIndex> peers;


typedef HashedKeyMap<uint64_t, ENetHost *, uint64_t(0), oa_hashmap_util::MumStepHash<uint64_t>> ENetAdressToPortMap;
typedef HashedKeyMap<uint16_t, uint64_t, uint16_t(0), oa_hashmap_util::MumStepHash<uint16_t>> PortToENetAdressMap;
ENetAdressToPortMap adress_to_port_map;
PortToENetAdressMap port_to_address_map;


enet_uint16 listen_to_port;
const char *self_host_str;
uint16_t control_port;
uint16_t client_ports_range_start;
uint16_t client_ports_range_amount;

bool need_to_shutdown;
bool got_server_connection;


static void sigint_handler(int)
{
  printf("[RELAY] shutdowning...\n\n");
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

static void send(const danet::BitStream &bs)
{
  for (int i = 0; i < peers.size(); ++i)
    peerIface->Send(bs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, peers[i], false);
}

ENetAddress dedicAdress;

static void send_to_dedic() {}

union ENetAdressKey
{
  ENetAddress adress;
  uint64_t u;
};

static void on_packet(Packet *pkt)
{
  const char *strAddr = pkt->systemAddress.ToString();
  switch (pkt->data[0])
  {
    case ID_NEW_INCOMING_CONNECTION:
    case ID_CONNECTION_REQUEST_ACCEPTED:
      printf("[RELAY] Connected to %s\n", strAddr);
      peers.push_back(pkt->systemIndex);

      if (!got_server_connection)
      {
        printf("[RELAY] %s is registered as dedicated server\n", strAddr);
        dedicAdress.host = pkt->systemAddress.host;
        dedicAdress.port = pkt->systemAddress.port;
        got_server_connection = true;
      }
      break;
    case ID_DISCONNECT:
      printf("[RELAY] %s %s", strAddr, "disconnected\n");
      erase_item_by_value(peers, pkt->systemIndex);
      need_to_shutdown = true;
      break;
    case ID_DEDIC_REGISTER_MESSAGE:
    {
      break;
    }
    break;
    default: printf("[RELAY] Unknown packet data of type %d size %d from %s\n", (int)pkt->data[0], pkt->length, strAddr);
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
  event->type = ENET_EVENT_TYPE_INTERCEPTED;
  ENetAddress who = host_created_to_handle_packets_to_specific_client->address;
  uint64_t address = port_to_address_map.findOr(who.port, uint16_t(0));
  ENetAdressKey key;
  key.u = address;
  ENetBuffer ebuf;
  ebuf.data = (void *)host_created_to_handle_packets_to_specific_client->receivedData;
  ebuf.dataLength = host_created_to_handle_packets_to_specific_client->receivedDataLength;
  int sent_length = enet_socket_send(host->socket, &key.adress, &ebuf, /*bufferCount*/ 1);

#if DEBUG_INTERCEPT
  printf("[RELAY] intercepted dedic_to_client event %i from %s ", ebuf.dataLength,
    SystemAddress(dedicAdress.host, dedicAdress.port).ToString());
  printf("[RELAY] to %s %i\n", SystemAddress(key.adress.host, key.adress.port).ToString(), sent_length);
#endif

  // if (event->packet)
  //   enet_packet_destroy(event->packet);
  return 1;
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

int intercept_network_message(_ENetHost *host, _ENetEvent *event)
{
  event->type = ENET_EVENT_TYPE_INTERCEPTED;
  ENetAddress from_whom = host->receivedAddress;
  ENetAdressKey key;
  key.adress = from_whom;
  ENetHost *relayHost = adress_to_port_map.findOr(key.u, nullptr);
  ENetAddress addr;
  if (DAGOR_UNLIKELY(relayHost == nullptr))
  {
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

      for (int port_to_try = client_ports_range_start; port_to_try < client_ports_range_start + client_ports_range_amount;
           port_to_try++)
      {
        addr.port = port_to_try;
        relayHost = enet_host_create(&addr, 0, 0, 0, 0);
        if (relayHost)
          break;
      }
    }

    if (!relayHost)
    {
      fprintf(stderr, "Can't add client - no free ports\n");
      return 1;
    }

    adress_to_port_map.emplace(key.u, relayHost);
    port_to_address_map.emplace(addr.port, key.u);
    enet_socket_get_address(relayHost->socket, &relayHost->address);
    enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_NONBLOCK, 1);
    enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_BROADCAST, 1);
    enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_RCVBUF, ENET_HOST_RECEIVE_BUFFER_SIZE);
    enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_SNDBUF, ENET_HOST_SEND_BUFFER_SIZE);
    relayHost->intercept = intercept_network_message_from_dedic_to_client;

    printf("[RELAY] settings %s:%d\n", self_host_str, control_port);
    enet_peer_ping_target_request(&peerIface->GetHostRaw()->peers[0], addr.port);
    for (int i = 0; i < 10; i++)
    {
      UpdatePeerInterface();
      sleep_msec(100);
    }
  }

  ENetBuffer ebuf;
  ebuf.data = (void *)host->receivedData;
  ebuf.dataLength = host->receivedDataLength;
  int sent_length = enet_socket_send(relayHost->socket, &dedicAdress, &ebuf, /*bufferCount*/ 1);
#if DEBUG_INTERCEPT
  printf("[RELAY] intercepted event %i from %s ", ebuf.dataLength, SystemAddress(from_whom.host, from_whom.port).ToString());
  printf("[RELAY] translated as %s ", SystemAddress(relayHost->address.host, relayHost->address.port).ToString());
  printf("[RELAY] to %s %i\n", SystemAddress(dedicAdress.host, dedicAdress.port).ToString(), sent_length);
#endif
  // if (event->packet)
  //   enet_packet_destroy(event->packet);
  return 1;
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
  dagor_random::set_rnd_seed(0);
  if (::dgs_argc <= 1) // no args or only exe name
  {
    printf("[RELAY] --listen:INT - sets public port for clients to relay to clientside-dedic\n");
    printf("[RELAY] --control_port:INT - \n");
    printf("[RELAY] --client_ports_range_start:INT - range from where to begin trying to allocate client ports\n");
    printf("[RELAY] --client_ports_range_amount:INT - amount of possible client ports\n");
    printf("[RELAY] --host_url:HOST_ADDR_STR - host to bind to (usually just 0.0.0.0)\n");
    printf("[RELAY] --listen_socket:SOCKET_FILE_DESCRIPTOR - (OPTIONAL) for agent instantiating relay and having shared socket\n");
    printf("[RELAY] --control_socket:SOCKET_FILE_DESCRIPTOR - (OPTIONAL) for agent instantiating relay and having shared socket\n");
  }

  cpujobs::init(dgs_get_settings()->getBlockByNameEx("debug")->getInt("coreCount", -1), /*reserve_jobmgr_cores*/ false);


  const char *listen_to_port_str = dgs_get_argv("listen");
  const char *control_port_str = dgs_get_argv("control_port");
  const char *client_ports_range_start_str = dgs_get_argv("client_ports_range_start");
  const char *client_ports_range_amount_str = dgs_get_argv("client_ports_range_amount");
  const char *listen_socket_str = dgs_get_argv("listen_socket");
  const char *control_socket_str = dgs_get_argv("control_socket");
  self_host_str = dgs_get_argv("host_url");

  if (!listen_to_port_str)
  {
    printf("[RELAY] ERROR: missing arg --listen:INT\n");
    return 1;
  }

  if (!control_port_str)
  {
    printf("[RELAY] ERROR: missing arg --control_port:INT\n");
    return 1;
  }

  if (!client_ports_range_start_str)
  {
    printf("[RELAY] ERROR: missing arg --client_ports_range_start:INT\n");
    return 1;
  }

  if (!client_ports_range_amount_str)
  {
    printf("[RELAY] ERROR: missing arg --client_ports_range_amount:INT\n");
    return 1;
  }


  listen_to_port = try_parse_int(listen_to_port_str);
  control_port = try_parse_int(control_port_str);
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
    printf("[RELAY] ERROR: missing arg --host_url:HOST_ADDR_STR\n");
    return 1;
  }

  printf("[RELAY] create listen net driver\n");


  G_VERIFY(enet_initialize() == 0);
  enet_time_set(1); // not 0, because 0 has special meaning ("invalid time")
  ENetAddress addr;
  addr.host = ENET_HOST_ANY;
  addr.port = listen_to_port;
  host = enet_host_create(listenSD.port > 0 ? NULL : &addr, 0, 0, 0, 0); // from client to dedic
  if (!host)
  {
    fprintf(stderr, "Can't start relay with listen port on %s:%d\n", self_host_str, listen_to_port);
    fprintf(stderr, "bind failed: %s\n", strerror(errno));
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

  printf("[RELAY] settings %s:%d\n", self_host_str, control_port);


  SocketDescriptor sd = controlSD.port != 0 ? controlSD : SocketDescriptor(control_port, self_host_str);
  peerIface = new DaNetPeerInterface(nullptr, /* is_manual */ true);
  if (!peerIface->Startup(1, 50, &sd))
  {
    fprintf(stderr, "Can't start relay with control port on %s:%d\n", self_host_str, control_port);
    return 1;
  }
  else
    printf("[RELAY] Started relay with control port on %s:%d\n", self_host_str, control_port);

  context = DaNetPeerExecutionContext();

  int timeout = 30000; // msec
  while (1)
  {
    DaNetTime cur_time = enet_time_get();

    ENetEvent toDedicEvent;
    int r = enet_host_service_only_incoming_commands(host, &toDedicEvent, /*timeout*/ 0); // send/receive
    if (r < 0)
    {
#if _TARGET_PC_WIN | _TARGET_XBOX
      int last_error = WSAGetLastError();
#else
      int last_error = errno;
#endif
      G_UNUSED(last_error);
      printf("[RELAY] ERROR: fromClienttoDedic enet_host_service() failed, last_error = %d", last_error);
      return 1;
    }

    UpdatePeerInterface();

    for (auto host : adress_to_port_map)
    {
      ENetHost *dedic_to_client_listener_host = host.val();

      ENetEvent toClientEvent;
      int r = enet_host_service_only_incoming_commands(dedic_to_client_listener_host, &toClientEvent, /*timeout*/ 0); // send/receive
      if (r < 0)
      {
#if _TARGET_PC_WIN | _TARGET_XBOX
        int last_error = WSAGetLastError();
#else
        int last_error = errno;
#endif
        G_UNUSED(last_error);
        printf("[RELAY] ERROR: fromDedicToClient enet_host_service() failed, last_error = %d", last_error);
        return 1;
      }
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
      printf("[RELAY] TIMEOUT: no server control connection established - exiting\n");
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