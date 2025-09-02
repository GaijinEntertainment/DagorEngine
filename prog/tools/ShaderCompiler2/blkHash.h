// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

using BlkHashBytes = eastl::array<uint8_t, 20>;

inline eastl::string blk_hash_string(const BlkHashBytes &bytes)
{
  eastl::string res;
  for (uint8_t byte : bytes)
    res.append_sprintf("%02x", byte);
  return res;
}
