// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>

namespace PropPanel
{

class ScopedImguiBeginDisabled
{
public:
  explicit ScopedImguiBeginDisabled(bool in_disabled) : disabled(in_disabled)
  {
    if (disabled)
      ImGui::BeginDisabled();
  }

  ~ScopedImguiBeginDisabled()
  {
    if (disabled)
      ImGui::EndDisabled();
  }

private:
  const bool disabled;
};

} // namespace PropPanel