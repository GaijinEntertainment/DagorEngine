// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/nodeHandle.h>
#include <EASTL/array.h>

eastl::array<dafg::NodeHandle, 2> make_blood_accumulation_prepass_node();
dafg::NodeHandle make_blood_resolve_node();
dafg::NodeHandle make_blood_forward_node();
