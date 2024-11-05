// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include  "types.h"

namespace matching
{

  const size_t  ROOM_MEMBER_AUTH_KEY_SIZE = 20;

  struct  AuthKey
  {
    uint8_t bytes[ROOM_MEMBER_AUTH_KEY_SIZE];
  };

  bool  generate_peer_auth(UserId user_id,
                           void const* room_secret, size_t secret_size,
                           AuthKey& output);
}
