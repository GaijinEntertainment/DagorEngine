// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <matching/types.h>
#include <generic/dag_tabFwd.h>
#include <generic/dag_span.h>

namespace net
{

matching::UserId get_user_id();
const char *get_user_name();
matching::SessionId get_session_id();
typedef dag::ConstSpan<uint8_t> auth_key_t;
auth_key_t get_platform_auth_key();
bool get_auth_key_for_platform(const char *pltf, auth_key_t *&out_key);

void clear_cached_ids();
} // namespace net
