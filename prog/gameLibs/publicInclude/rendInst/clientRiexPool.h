//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/constants.h>


namespace rendinst
{
class ClientRiexPoolId
{
  int idx = -1;
  explicit ClientRiexPoolId(int id) : idx(id) {}

public:
  ClientRiexPoolId() = default;

  int id() const { return idx; }
  bool valid() const { return idx >= 0; }
  bool operator==(const ClientRiexPoolId &rhs) const { return idx == rhs.idx; }

  static ClientRiexPoolId get(const char *ri_res_name);
  static ClientRiexPoolId add(const char *ri_res_name, AddRIFlags ri_flags);
};
} // namespace rendinst
