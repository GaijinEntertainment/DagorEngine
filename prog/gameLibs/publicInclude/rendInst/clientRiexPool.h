//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/constants.h>


namespace rendinst
{
class ClientRiexPool
{
  int id_ = -1;
  explicit ClientRiexPool(int id) : id_(id) {}

public:
  ClientRiexPool() = default;

  int id() const { return id_; }
  bool valid() const { return id_ >= 0; }
  bool operator==(const ClientRiexPool &rhs) const { return id_ == rhs.id_; }

  static ClientRiexPool get(const char *ri_res_name);
  static ClientRiexPool add(const char *ri_res_name, AddRIFlags ri_flags);
};
} // namespace rendinst
