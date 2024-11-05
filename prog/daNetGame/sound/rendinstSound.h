// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gamePhys/phys/rendinstSound.h>

namespace rendinstsound
{
bool rendinst_tree_sound_cb(
  TreeSoundCbType call_type, const TreeSoundDesc &desc, ri_tree_sound_cb_data_t &sound_data, const TMatrix &tm);
bool have_sound();
} // namespace rendinstsound
