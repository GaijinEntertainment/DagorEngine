// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <matching/types.h>
#include <matching/auth.h>
#include <json/json.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <daECS/core/entityId.h>
#include <daECS/core/event.h>

namespace dedicated_matching
{
namespace state_data
{
extern bool (*generate_peer_auth)(matching::UserId user_id, void const *room_secret, size_t secret_size, matching::AuthKey &output);
extern void (*try_start_relay)();
} // namespace state_data
} // namespace dedicated_matching

#define NET_MATCHING_ECS_EVENTS                                                                            \
  NET_MATCHING_ECS_EVENT(NetMatchingEventOnInit)                                                           \
  NET_MATCHING_ECS_EVENT(NetMatchingEventOnTerm)                                                           \
  NET_MATCHING_ECS_EVENT(NetMatchingEventOnUpdate)                                                         \
  NET_MATCHING_ECS_EVENT(NetMatchingEventOnLevelLoaded)                                                    \
  NET_MATCHING_ECS_EVENT(NetMatchingLeaveRoomEvent)                                                        \
  NET_MATCHING_ECS_EVENT(NetMatchingKickPlayerEvent, matching::UserId /*user_id*/)                         \
  NET_MATCHING_ECS_EVENT(NetMatchingRemovePlayerEvent, matching::UserId /*user_id*/)                       \
  NET_MATCHING_ECS_EVENT(NetMatchingBanPlayerEvent, matching::UserId /*user_id*/)                          \
  NET_MATCHING_ECS_EVENT(NetMatchingChangeTeamEvent, matching::UserId /*user_id*/, int /*team*/)           \
  NET_MATCHING_ECS_EVENT(NetMatchingEventOnJoinRoom, matching::RoomId /*room_id*/)                         \
  NET_MATCHING_ECS_EVENT(NetMatchingEventOnRegisterRoomMember, matching::UserId /*userId*/)                \
  NET_MATCHING_ECS_EVENT(NetMatchingEventOnPlayerAppIdChanged, matching::UserId /*userId*/, int /*appId*/) \
  NET_MATCHING_ECS_EVENT(NetMatchingRelayGoalEvent, bool /*up*/)                                           \
  NET_MATCHING_ECS_EVENT(NetMatchingRelayLinkEvent, bool /*up*/)                                           \
  NET_MATCHING_ECS_EVENT(NetMatchingRelayAllocDoneEvent, bool /*ok*/)                                      \
  NET_MATCHING_ECS_EVENT(NetMatchingRelayPublishDoneEvent, bool /*ok*/, int /*pub_gen*/)                   \
  NET_MATCHING_ECS_EVENT(NetMatchingRelayReadyEvent, bool /*up*/)

#define NET_MATCHING_ECS_EVENT ECS_BROADCAST_EVENT_TYPE
NET_MATCHING_ECS_EVENTS
#undef NET_MATCHING_ECS_EVENT
