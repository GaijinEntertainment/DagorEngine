// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/propPanelService.h>
#include <propPanel/propPanel.h>

#include <gui/dag_imgui.h>
#include <imgui/imgui.h>

namespace PropPanel
{

namespace
{

class PropPanelServiceImpl final : public IPropPanelService
{
public:
  void *getImguiContext() override { return ImGui::GetCurrentContext(); }

  ImTextureID getIconTextureId(const char *icon_name, int size) override
  {
    return get_im_texture_id_from_icon_id(load_icon(icon_name, size));
  }

  ImFont *getBoldFont() override { return imgui_get_bold_font(); }
};

PropPanelServiceImpl the_service;

} // namespace

IPropPanelService &get_service() { return the_service; }

} // namespace PropPanel
