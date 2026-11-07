// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daPCG/scene/scene_api.h>

namespace dacsg
{
class ModelContext;
}

// Hosts pull it after CsgModule: NEED_MODULE(PcgSceneModule)
namespace dapcg
{
// the model's, indexed by Node id; valid until its next scene_node or scene_child, or until ctx dies
dag::ConstSpan<SceneNode> scene_nodes(dacsg::ModelContext &ctx);
} // namespace dapcg
