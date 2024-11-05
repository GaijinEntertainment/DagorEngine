// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_simpleString.h>

class DataBlock;

namespace sound
{
struct ActionProps
{
  SimpleString humanHitSoundName;
  SimpleString humanHitSoundPath;

  void load(const DataBlock *blk);
  static const ActionProps *get_props(int prop_id);
  static const ActionProps *try_get_props(int prop_id);
  static void register_props_class();
  static bool can_load(const DataBlock *blk);
};
}; // namespace sound
