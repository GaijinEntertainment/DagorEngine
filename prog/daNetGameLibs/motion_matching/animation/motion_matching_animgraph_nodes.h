// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <anim/dag_animBlend.h>
#include "animation_data_base.h"

void load_motion_matching_animgraph_nodes(AnimationDataBase &data_base, AnimV20::AnimationGraph *anim_graph);
AnimationFilterTags get_mm_tags_from_animgraph(const AnimationDataBase &data_base, const AnimV20::IAnimStateHolder &st);
void reset_animgraph_tags(const AnimationDataBase &data_base, AnimV20::IAnimStateHolder &st);
