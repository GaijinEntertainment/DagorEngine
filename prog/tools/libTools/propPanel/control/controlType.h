// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace PropPanel
{

// Only used to facilitate safe casting or for special handling, so only the necessary controls are here.
enum class ControlType
{
  Unknown,
  GroupBox,
  TabPage,
  ToolbarToggleButtonGroup,
};

} // namespace PropPanel
