// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sound_net/soundNet.h"

bool have_sound_net() { return false; }

#include "sound_net/registerSoundNetProps.h"
#include "sound_net/shellSoundNetProps.h"

namespace soundnet
{
void register_props() {}
} // namespace soundnet

void ShellSoundNetProps::load(const DataBlock *) {}
/*static*/ const ShellSoundNetProps *ShellSoundNetProps::get_props(int) { return nullptr; }
/*static*/ void ShellSoundNetProps::register_props_class() {}
/*static*/ bool ShellSoundNetProps::can_load(const DataBlock *) { return false; }
