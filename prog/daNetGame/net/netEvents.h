// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/event.h>
#include "net.h"
#include "netMsg.h"
#include "userid.h"

ECS_BROADCAST_EVENT_TYPE(OnNetControlClientCreated)
ECS_BROADCAST_EVENT_TYPE(OnNetControlClientDestroyed)
ECS_BROADCAST_EVENT_TYPE(OnNetControlClientInfoMsg, const ServerInfo *)
ECS_BROADCAST_EVENT_TYPE(OnNetControlClientDisconnect)
ECS_BROADCAST_EVENT_TYPE(OnMsgSinkCreatedClient, uint16_t * /*inout clientFlags*/)
ECS_BROADCAST_EVENT_TYPE(OnNetUpdate)
ECS_BROADCAST_EVENT_TYPE(OnNetDestroy, bool /*final*/)

ECS_BROADCAST_EVENT_TYPE(OnNetDedicatedCreated, net::ServerFlags * /*inout serverFlags*/)
ECS_BROADCAST_EVENT_TYPE(OnNetDedicatedDestroyed, net::ServerFlags * /*inout serverFlags*/)

struct OnNetDedicatedPrepareServerInfo : public ecs::Event
{
  ServerInfo &srvInfoMsg;
  const net::ServerFlags &serverFlags;
  uint32_t &connFlags;
  uint16_t clientFlags;
  matching::UserId userId;
  const eastl::string &userName;
  const eastl::string &pltf;
  net::IConnection &conn;

  ECS_BROADCAST_EVENT_DECL(OnNetDedicatedPrepareServerInfo)
  OnNetDedicatedPrepareServerInfo(ServerInfo &si,
    const net::ServerFlags &sf,
    uint32_t &cnf,
    uint16_t cf,
    matching::UserId id,
    const eastl::string &nm,
    const eastl::string &pl,
    net::IConnection &cn) :
    ECS_EVENT_CONSTRUCTOR(OnNetDedicatedPrepareServerInfo),
    srvInfoMsg(si),
    serverFlags(sf),
    connFlags(cnf),
    clientFlags(cf),
    userId(id),
    userName(nm),
    pltf(pl),
    conn(cn)
  {}
};
struct OnNetDedicatedServerDisconnect : public ecs::Event
{
  net::IConnection &conn;

  ECS_BROADCAST_EVENT_DECL(OnNetDedicatedServerDisconnect)
  OnNetDedicatedServerDisconnect(net::IConnection &c) : ECS_EVENT_CONSTRUCTOR(OnNetDedicatedServerDisconnect), conn(c) {}
};

ECS_UNICAST_EVENT_TYPE(CmdSendComplaint,
  matching::UserId /*offender_userid*/,
  eastl::string /*category*/,
  eastl::string /*user_comment*/,
  eastl::string /*details_json*/,
  eastl::string /*chat_log*/,
  eastl::string /*lang*/);
