// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <daNet/daNetPeerInterface.h>
#include <daNet/getTime.h>
#include <EASTL/unique_ptr.h>
#include <enet/enet.h>
#include <stdio.h>

TEST_CASE("Reliable timeout detects upload loss while server packets arrive", "[danet][responsiveness]")
{
  DaNetPeerInterface client(nullptr, true);
  REQUIRE(client.Startup(1, int(OS_WAIT_IGNORE)));

  ENetAddress address{};
  REQUIRE(enet_address_set_host(&address, "127.0.0.1") == 0);
  eastl::unique_ptr<ENetHost, decltype(&enet_host_destroy)> server(enet_host_create(&address, 2, 1, 0, 0), enet_host_destroy);
  REQUIRE(server.get() != nullptr);
  address.port = server->address.port;
  REQUIRE(client.Connect("127.0.0.1", address.port));

  ENetPeer *serverPeer = nullptr;
  unsigned receivedReliable = 0;
  unsigned receivedFromServer = 0;
  DaNetPeerExecutionContext context;
  const uint8_t payload[] = {0xab};
  auto step = [&] {
    enet_time_set(enet_time_get() + 100);
    if (serverPeer && serverPeer->state == ENET_PEER_STATE_CONNECTED)
      REQUIRE(enet_peer_send(serverPeer, 0, enet_packet_create(payload, sizeof(payload), ENET_PACKET_FLAG_UNSEQUENCED)) == 0);
    ENetEvent event{};
    while (enet_host_service(server.get(), &event, 0) > 0)
    {
      if (event.type == ENET_EVENT_TYPE_CONNECT)
      {
        serverPeer = event.peer;
        enet_peer_timeout(serverPeer, 0, 20000, 30000);
      }
      else if (event.type == ENET_EVENT_TYPE_RECEIVE)
      {
        if (event.packet->flags & ENET_PACKET_FLAG_RELIABLE)
          ++receivedReliable;
        enet_packet_destroy(event.packet);
      }
      else if (event.type == ENET_EVENT_TYPE_DAGOR_RECEIVE_UNCONNECTED)
        enet_packet_destroy(event.packet);
    }
    context.curTime = danet::GetTime();
    client.ExecuteSingleUpdate(context);
    while (Packet *packet = client.Receive())
    {
      if (packet->length == sizeof(payload) && packet->data[0] == payload[0])
        ++receivedFromServer;
      client.DeallocatePacket(packet);
    }
  };

  for (int i = 0; i < 30 && (!serverPeer || !client.GetConnectionStartTime(0)); ++i)
    step();
  REQUIRE(serverPeer);
  REQUIRE(client.GetConnectionStartTime(0) != 0);
  client.SetupPeerTimeouts(0, 30000, 20000);
  for (int i = 0; i < 20; ++i)
    step();
  CHECK(client.GetReliableTimeoutTime(0) == 0);
  CHECK(client.GetReliableTimeoutTime(UNASSIGNED_SYSTEM_INDEX) == 0);

  server->intercept = [](ENetHost *, ENetEvent *) { return 1; };
  for (int i = 0; i < 2; ++i) // the route change must resend every command in flight, not only the first
    REQUIRE(client.Send(dag::ConstSpan<uint8_t>(payload, sizeof(payload)), HIGH_PRIORITY, RELIABLE_ORDERED, 0, 0, false));
  const unsigned receivedBeforeLoss = receivedFromServer;
  for (int i = 0; i < 60; ++i)
    step();

  REQUIRE(client.GetENetPeer(0)->state == ENET_PEER_STATE_CONNECTED);
  CHECK(receivedReliable == 0);
  CHECK(receivedFromServer > receivedBeforeLoss);
  REQUIRE(client.GetReliableTimeoutTime(0) != 0);
  CHECK(danet::GetTime() - client.GetReliableTimeoutTime(0) > 4000);
  CHECK(danet::GetTime() - client.GetLastReceivedPacketTime(0) < 200);

  // A route change must resend at once, so wait until the backed-off retransmit is seconds away.
  auto resendDueIn = [&] {
    const auto *cmd = (const ENetOutgoingCommand *)enet_list_front(&client.GetENetPeer(0)->sentReliableCommands);
    return int(cmd->sentTime + cmd->roundTripTimeout - enet_time_get());
  };
  for (int i = 0; i < 50 && resendDueIn() <= 2000; ++i)
    step();
  REQUIRE(resendDueIn() > 2000);
  server->intercept = nullptr;
  char serverUrl[32];
  snprintf(serverUrl, sizeof(serverUrl), "127.0.0.1:%u", unsigned(address.port));
  REQUIRE(client.ChangeHostAddress(serverUrl));
  for (int i = 0; i < 10 && receivedReliable < 2; ++i)
    step();
  CHECK(receivedReliable == 2);
  CHECK(client.GetReliableTimeoutTime(0) == 0); // cleared in the update that got the ack, not after the 600 ms gate

  server->intercept = [](ENetHost *, ENetEvent *) { return 1; };
  REQUIRE(client.Send(dag::ConstSpan<uint8_t>(payload, sizeof(payload)), HIGH_PRIORITY, RELIABLE_ORDERED, 0, 0, false));
  for (int i = 0; i < 60; ++i)
    step();
  REQUIRE(client.GetReliableTimeoutTime(0) != 0);
  ENetPeer *clientPeer = &client.GetHostMutable()->peers[0];
  enet_peer_disconnect(clientPeer, 0);
  for (int i = 0; i < 10; ++i)
    step();
  REQUIRE(clientPeer->state == ENET_PEER_STATE_DISCONNECTING);
  REQUIRE(clientPeer->earliestTimeout != 0);
  CHECK(client.GetReliableTimeoutTime(0) == 0);
}
