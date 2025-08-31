//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#ifndef ImTextureID
typedef void *ImTextureID;
#endif

namespace PropPanel
{

enum class IconId : int
{
  Invalid = 0,
  // The rest of the ids are dynamically allocated.
};

// Release the internal structures used by PropPanel.
void release();

// This must be called after ImGui::NewFrame before calling any updateImgui functions.
void after_new_frame();

// This must be called before ImGui::EndFrame.
void before_end_frame();

// Loads an icon from p2util::get_icon_path(). If the icon cannot be found there then it tries to fall back to
// p2util::get_icon_fallback_path(). In case of tools they'll be dagor/tools/dagor_cdk/commonData/icons/{THEME NAME}/
// and/or dagor/tools/dagor_cdk/commonData/icons/ depending on the theme.
// filename: name of the icon without extension. E.g.: "close_editor".
IconId load_icon(const char *filename);

ImTextureID get_im_texture_id_from_icon_id(IconId icon_id);

void reload_all_icons();

// Display tooltip for the previous ImGui control if the mouse is over it.
// Similar to ImGui::SetItemTooltip but the tooltip behaves more like Windows tooltips.
// control: any unique identifier that identifies the control. It can be (const void *)ImGui::GetItemID() too.
void set_previous_imgui_control_tooltip(const void *control, const char *text, const char *text_end = nullptr);

} // namespace PropPanel