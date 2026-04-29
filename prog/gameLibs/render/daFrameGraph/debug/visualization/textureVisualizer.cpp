// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "textureVisualizer.h"

#include <imgui.h>
#include <gui/dag_imgui.h>


namespace dafg::visualization
{

TextureVisualizer::TextureVisualizer()
{
  REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_TEX_WIN_NAME, []() { ImGui::Text("Texture window WIP"); });
}

} // namespace dafg::visualization