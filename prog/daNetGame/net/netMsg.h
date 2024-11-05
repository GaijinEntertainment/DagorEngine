// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_simpleString.h>
#include <daECS/net/msgDecl.h>
#include <matching/types.h>
#include <generic/dag_tab.h>

ECS_NET_DECL_MSG(ServerInfo,
  uint16_t,      // server flags
  uint8_t,       // tickrate
  uint8_t,       // bots tickrate
  uint32_t,      // server exe version
  eastl::string, // server's loaded scene name
  eastl::string, // server's level blk path
  Tab<uint8_t>   // Session ID
);
ECS_NET_DECL_MSG(ClientInfo,
  uint16_t,         // protov
  matching::UserId, // userid
  eastl::string,    // username,
  uint16_t,         // client flags,
  Tab<uint8_t>,     // auth_key,
  eastl::string,    // platform (win32, win64, ps5...)
  eastl::string,    // platform userid (live uid, ps4 uid, etc)
  uint32_t,         // client exe version
  danet::BitStream  // sync vroms
);

ECS_NET_DECL_MSG(ApplySyncVromsDiffs, danet::BitStream);
ECS_NET_DECL_MSG(SyncVromsDone);
