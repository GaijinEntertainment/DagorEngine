// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daPCG/material/material_api.h>

namespace dacsg
{
class ModelContext;
}

// Hosts pull it after CsgModule: NEED_MODULE(PcgMaterialModule)
namespace dapcg
{
// the model's, in call order; valid until its next material(), or until ctx dies
dag::ConstSpan<Material> material_table(dacsg::ModelContext &ctx);
} // namespace dapcg
