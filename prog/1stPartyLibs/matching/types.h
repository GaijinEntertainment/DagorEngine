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

  typedef int32_t   RequestId;

  static constexpr RoomId    INVALID_ROOM_ID   = 0;
  static constexpr SessionId INVALID_SESSION_ID = 0;
  static constexpr MemberId  INVALID_MEMBER_ID = MemberId(-1);
  static constexpr UserId    INVALID_USER_ID   = UserId(-1);
  static constexpr SquadId   INVALID_SQUAD_ID  = 0;

  static constexpr int MAX_PLAYER_NAME_LENGTH = 64;
}
