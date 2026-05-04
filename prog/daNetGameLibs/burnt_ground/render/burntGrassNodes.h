// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/daFG.h>

class BurntGrassRenderer;

inline auto get_burnt_grass_namespace() { return dafg::root() / "burnt_grass"; }

dafg::NodeHandle create_burnt_grass_prepare_node(BurntGrassRenderer *renderer);