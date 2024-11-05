//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

namespace sndsys
{
typedef uint64_t var_id_t;
enum : var_id_t
{
  INVALID_VAR_ID = 0
};

struct VarId
{
  VarId() = default;
  explicit VarId(var_id_t id) : id(id) {}
  explicit operator bool() const { return id != INVALID_VAR_ID; }
  void reset() { id = 0; }
  bool operator==(const VarId &rhs) const { return id == rhs.id; }
  bool operator!=(const VarId &rhs) const { return id != rhs.id; }
  bool operator<(const VarId &rhs) const { return id < rhs.id; }
  explicit operator var_id_t() const { return id; }

private:
  var_id_t id = INVALID_VAR_ID;
};
} // namespace sndsys
