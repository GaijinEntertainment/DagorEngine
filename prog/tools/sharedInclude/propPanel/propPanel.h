//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>

namespace PropPanel
{

// Release the internal structures used by PropPanel.
void release();

// This must be called after ImGui::NewFrame before calling any updateImgui functions.
void after_new_frame();

// This must be called before ImGui::EndFrame.
void before_end_frame();

// Loads an icon from p2util::get_icon_path(). In case of tools that will be dagor/tools/dagor_cdk/commonData/gui16x16/.
// filename: name of the icon without extension. E.g.: "close_editor".
TEXTUREID load_icon(const char *filename);

// Display tooltip for the previous ImGui control if the mouse is over it.
// Similar to ImGui::SetItemTooltip but the tooltip behaves more like Windows tooltips.
// control: any unique identifier that identifies the control. It can be (const void *)ImGui::GetItemID() too.
void set_previous_imgui_control_tooltip(const void *control, const char *text, const char *text_end = nullptr);

} // namespace PropPanel