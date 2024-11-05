//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;

namespace physmat
{
struct PhysContactProps
{
  bool removePhysContact = true;

  void load(const DataBlock *blk);
  static const PhysContactProps *get_props(int prop_id);
  static void register_props_class();
  static bool can_load(const DataBlock *);
};
}; // namespace physmat
