// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include "sound/rendinstSound.h"

namespace rendinstsound
{
bool rendinst_tree_sound_cb(TreeSoundCbType, const TreeSoundDesc &, ri_tree_sound_cb_data_t &, const TMatrix &) { return false; }

bool have_sound() { return false; }
} // namespace rendinstsound
