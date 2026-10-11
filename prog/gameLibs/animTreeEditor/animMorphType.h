// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <anim/dag_animDecl.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>

static_assert(AnimV20::MT_COUNT == 4, "FifoMorphType count mismatch, update morph_type_names");

inline const Tab<String> morph_type_names({String("linear"), String("quad_in"), String("quad_out"), String("quad_in_out")});
inline const char *DEFAULT_MORPH_TYPE = "linear";
