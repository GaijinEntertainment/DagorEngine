#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <conio.h>

#include <debug/dag_assert.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <daNet/daNetPeerInterface.h>
#include <daNet/messageIdentifiers.h>

#define DEF_HOST     "localhost"
#define DEF_PORT     36666
#define ID_CHAT_DATA ID_USER_PACKET_ENUM

static bool server = false;
static DaNetPeerInterface *peerIface = NULL;
static Tab<SystemIndex> peers;

static void sigint_handler(int)
{
  printf("shutdowning...\n");
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

static void on_packet(Packet *pkt)
{
  const char *strAddr = pkt->systemAddress.ToString();
  switch (pkt->data[0])
  {
    case ID_NEW_INCOMING_CONNECTION:
      printf("Somebody connected from %s\n", strAddr);
      peers.push_back(pkt->systemIndex);
      break;
    case ID_CONNECTION_REQUEST_ACCEPTED:
      printf("Connected to %s\n", strAddr);
      peers.push_back(pkt->systemIndex);
      break;
    case ID_DISCONNECT:
      printf("%s %s\n", strAddr, "disconnected");
      erase_item_by_value(peers, pkt->systemIndex);
      break;
    case ID_CHAT_DATA:
    {
      danet::BitStream bs(pkt->data, pkt->length, false);
      bs.IgnoreBytes(1);
      SystemAddress addr;
      bs.Read(addr);
      if (addr == SystemAddress())
        addr = pkt->systemAddress;
      printf("%s says \"%s\"\n", addr.ToString(), (const char *)&pkt->data[BITS_TO_BYTES(bs.GetReadOffset())]); // warn: no null
                                                                                                                // termination
                                                                                                                // validation
      if (server)
      {
        bs.WriteAt(pkt->systemAddress, BYTES_TO_BITS(1));
        send(bs);
      }
    }
    break;
    default: printf("Unknown packet data of type %d size %d from %s\n", (int)pkt->data[0], pkt->length, strAddr);
  }
}

int main(int argc, const char *argv[])
{
  init_once();

  peerIface = new DaNetPeerInterface();
  if (argc < 2 || strcmp(argv[1], "client") != 0)
  {
    SocketDescriptor sd(DEF_PORT, DEF_HOST);
    if (!peerIface->Startup(4, 50, &sd))
    {
      fprintf(stderr, "Can't start server on %s:%d\n", DEF_HOST, DEF_PORT);
      return 1;
    }
    else
      printf("Started server on %s:%d\n", DEF_HOST, DEF_PORT);
    server = true;
  }
  else if (!peerIface->Startup(1, 50) || !peerIface->Connect(DEF_HOST, DEF_PORT))
  {
    fprintf(stderr, "Can't connect to %s:%d\n", DEF_HOST, DEF_PORT);
    return 1;
  }
  else
    printf("Client trying to connect to %s:%d\n", DEF_HOST, DEF_PORT);
  char curMsg[256];
  char *p = curMsg;
  while (1)
  {
    while (Packet *pkt = peerIface->Receive())
    {
      on_packet(pkt);
      peerIface->DeallocatePacket(pkt);
    }
    if (kbhit())
    {
      char c = getch();
      if (c == '\r' || p == curMsg + sizeof(curMsg) - 1)
      {
        printf("\n");
        *p++ = '\0';

        danet::BitStream bs;
        bs.Write((char)ID_CHAT_DATA);
        bs.Write(SystemAddress());
        bs.Write(curMsg, p - curMsg);
        send(bs);

        p = curMsg;
      }
      else if (c == '\b')
      {
        if (p != curMsg)
        {
          *(--p) = ' ';
          printf("\r%s", curMsg);
          *p = '\0';
          printf("\r%s", curMsg);
        }
      }
      else
      {
        *p++ = c;
        *p = '\0';
        printf("\r%s", curMsg);
      }
    }
    sleep_msec(50);
  }
  // not reachable
  return 0;
}
