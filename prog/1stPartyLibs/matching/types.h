// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

namespace matching
{
  typedef uint64_t  UserId;
  typedef uint64_t  RoomId;
  typedef uint64_t  SessionId;
  typedef uint16_t  MemberId;
  typedef uint32_t  SquadId;
  typedef uint64_t  GroupId;

  typedef int32_t   RequestId;

  const RoomId    INVALID_ROOM_ID   = 0;
  const SessionId INVALID_SESSION_ID = 0;
  const MemberId  INVALID_MEMBER_ID = MemberId(-1);
  const UserId    INVALID_USER_ID   = UserId(-1);
  const SquadId   INVALID_SQUAD_ID  = 0;
  const GroupId   INVALID_GROUP_ID  = 0;
}
