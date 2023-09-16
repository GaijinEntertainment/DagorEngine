#ifndef GAIJIN_MATCHING_AUTH_H
#define GAIJIN_MATCHING_AUTH_H

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

#endif
