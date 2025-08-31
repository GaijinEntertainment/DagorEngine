// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <conio.h>

#include <daNet/daNetTypes.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <debug/dag_assert.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <daNet/messageIdentifiers.h>
#include <daNet/daNetPeerInterface.h>
#include <util/dag_hashedKeyMap.h>

#include <startup/dag_mainCon.inc.cpp>

#include <enet/enet.h>

#define ID_DEDIC_REGISTER_MESSAGE ID_USER_PACKET_ENUM

static DaNetPeerInterface *peerIface = NULL;
static Tab<SystemIndex> peers;

static void sigint_handler(int)
{
  printf("shutdowning...\n\n");
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

typedef HashedKeyMap<uint64_t, ENetHost *, uint64_t(0), oa_hashmap_util::MumStepHash<uint64_t>> ENetAdressToPortMap;
typedef HashedKeyMap<uint16_t, uint64_t, uint16_t(0), oa_hashmap_util::MumStepHash<uint16_t>> PortToENetAdressMap;
ENetAdressToPortMap adress_to_port_map;
PortToENetAdressMap port_to_address_map;

bool need_to_shutdown;

static void on_packet(Packet *pkt)
{
  const char *strAddr = pkt->systemAddress.ToString();
  switch (pkt->data[0])
  {
    case ID_NEW_INCOMING_CONNECTION:
      printf("Somebody connected from %s", strAddr);
      peers.push_back(pkt->systemIndex);

      printf("%s is registered as dedicated server", strAddr);
      dedicAdress.host = pkt->systemAddress.host;
      dedicAdress.port = pkt->systemAddress.port;

      break;
    case ID_CONNECTION_REQUEST_ACCEPTED:
      printf("Connected to %s", strAddr);
      peers.push_back(pkt->systemIndex);
      break;
    case ID_DISCONNECT:
      printf("%s %s", strAddr, "disconnected\n");
      erase_item_by_value(peers, pkt->systemIndex);
      need_to_shutdown = true;
      break;
    case ID_DEDIC_REGISTER_MESSAGE:
    {
      break;
    }
    break;
    default: printf("Unknown packet data of type %d size %d from %s\n", (int)pkt->data[0], pkt->length, strAddr);
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
uint16_t relay_port = 10000;
uint16_t relay_ports_amount = 0;

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
  printf("intercepted dedic_to_client event %i from %s ", ebuf.dataLength,
    SystemAddress(dedicAdress.host, dedicAdress.port).ToString());
  printf("to %s %i\n", SystemAddress(key.adress.host, key.adress.port).ToString(), sent_length);
#endif

  // if (event->packet)
  //   enet_packet_destroy(event->packet);
  return 1;
}


enet_uint16 client_receiver_port;
const char *self_host_str;
uint16_t dedic_receiver_port;


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
    addr.port = relay_port++;
    relayHost = enet_host_create(&addr, 0, 0, 0, 0);
    adress_to_port_map.emplace(key.u, relayHost);
    port_to_address_map.emplace(addr.port, key.u);
    enet_socket_get_address(relayHost->socket, &relayHost->address);
    enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_NONBLOCK, 1);
    enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_BROADCAST, 1);
    enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_RCVBUF, ENET_HOST_RECEIVE_BUFFER_SIZE);
    enet_socket_set_option(relayHost->socket, ENET_SOCKOPT_SNDBUF, ENET_HOST_SEND_BUFFER_SIZE);
    relayHost->intercept = intercept_network_message_from_dedic_to_client;

    printf("settings %s:%d\n", self_host_str, dedic_receiver_port);
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
  printf("intercepted event %i from %s ", ebuf.dataLength, SystemAddress(from_whom.host, from_whom.port).ToString());
  printf("translated as %s ", SystemAddress(relayHost->address.host, relayHost->address.port).ToString());
  printf("to %s %i\n", SystemAddress(dedicAdress.host, dedicAdress.port).ToString(), sent_length);
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
}


int DagorWinMain(int /*nCmdShow*/, bool /*debugmode*/)
{
  if (::dgs_argc <= 1) // no args or only exe name
  {
    printf("--ports_range_start:NUM - sets port to begin allocation ports from\n");
    printf("--client_ports_amount:NUM - max amount of ports to sequentially allocate for client (total would be +2 of that)\n");
    printf("--host_url:HOST_ADDR_STR - host to bind to (usually just 0.0.0.0)\n");
  }

  cpujobs::init(dgs_get_settings()->getBlockByNameEx("debug")->getInt("coreCount", -1), /*reserve_jobmgr_cores*/ false);

  peerIface = new DaNetPeerInterface(nullptr, /* is_manual */ true);

  const char *ports_range_start_str = dgs_get_argv("ports_range_start");
  const char *client_ports_amount_str = dgs_get_argv("client_ports_amount");
  if (!ports_range_start_str)
  {
    printf("ERROR: missing arg --ports_range_start:NUM\n");
    return 1;
  }

  if (!client_ports_amount_str)
  {
    printf("ERROR: missing arg --client_ports_amount:NUM\n");
    return 1;
  }
  uint16_t ports_range_start = ports_range_start_str ? try_parse_int(ports_range_start_str) : 0;
  uint16_t client_ports_amount = ports_range_start_str ? try_parse_int(client_ports_amount_str) : 0;

  self_host_str = dgs_get_argv("host_url");

  if (!self_host_str)
  {
    printf("ERROR: missing arg --host_url:HOST_ADDR_STR\n");
    return 1;
  }

  printf("create listen net driver\n");

  client_receiver_port = ports_range_start;
  if (!client_receiver_port)
  {
    printf("ERROR: failed to parse bind port from port arg '%s:%d'", self_host_str, client_receiver_port);
    return 1;
  }

  dedic_receiver_port = ports_range_start + 1;
  if (!dedic_receiver_port)
  {
    printf("ERROR: failed to parse bind port from port arg '%s:%d'", self_host_str, dedic_receiver_port);
    return 1;
  }

  relay_port = ports_range_start + 2;
  relay_ports_amount = client_ports_amount;

  G_VERIFY(enet_initialize() == 0);
  enet_time_set(1); // not 0, because 0 has special meaning ("invalid time")
  ENetAddress addr;
  addr.host = ENET_HOST_ANY;
  addr.port = client_receiver_port;
  host = enet_host_create(&addr, 0, 0, 0, 0); // from client to dedic

  enet_socket_get_address(host->socket, &host->address);
  enet_socket_set_option(host->socket, ENET_SOCKOPT_NONBLOCK, 1);
  enet_socket_set_option(host->socket, ENET_SOCKOPT_BROADCAST, 1);
  enet_socket_set_option(host->socket, ENET_SOCKOPT_RCVBUF, ENET_HOST_RECEIVE_BUFFER_SIZE);
  enet_socket_set_option(host->socket, ENET_SOCKOPT_SNDBUF, ENET_HOST_SEND_BUFFER_SIZE);
  host->intercept = intercept_network_message;

  printf("settings %s:%d\n", self_host_str, dedic_receiver_port);

  SocketDescriptor sd = SocketDescriptor(dedic_receiver_port, self_host_str);
  if (!peerIface->Startup(4, 50, &sd))
  {
    fprintf(stderr, "Can't start server on %s:%d\n", self_host_str, dedic_receiver_port);
    return 1;
  }
  else
    printf("Started server on %s:%d\n", self_host_str, dedic_receiver_port);

  context = DaNetPeerExecutionContext();

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
      printf("ERROR: fromClienttoDedic enet_host_service() failed, last_error = %d", last_error);
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
        printf("ERROR: fromDedicToClient enet_host_service() failed, last_error = %d", last_error);
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
  }

  return 0;
}

void DagorWinMainInit(bool debugmode) { DagorWinMainInit(0, debugmode); }
int DagorWinMain(bool debugmode) { return DagorWinMain(0, debugmode); }
