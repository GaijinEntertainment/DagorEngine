// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/daFG.h>

class BurntGrassRenderer;

inline auto get_burnt_ground_namespace() { return dafg::root() / "burnt_ground"; }

dafg::NodeHandle create_burnt_ground_prepare_decals_node(
  uint32_t max_decals, Sbuffer *decals_staging_buffer, Sbuffer *decals_indices_staging_buf);