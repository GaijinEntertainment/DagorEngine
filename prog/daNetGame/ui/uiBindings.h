// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class SqModules;

namespace ui
{

enum ExtraStateFlags
{
  S_CLIPPED = 0x1000
};

void bind_ui_behaviors(SqModules *module_mgr);

} // namespace ui
