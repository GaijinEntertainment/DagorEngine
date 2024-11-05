// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_simpleString.h>

class DataBlock;

struct ShellSoundNetProps
{
  SimpleString throwPhrase;

  void load(const DataBlock *blk);

  static const ShellSoundNetProps *get_props(int prop_id);
  static const ShellSoundNetProps *try_get_props(int prop_id);
  static void register_props_class();
  static bool can_load(const DataBlock *);
};
