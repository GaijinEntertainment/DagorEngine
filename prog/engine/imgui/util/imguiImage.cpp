// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imguiUtil.h>
#include <drv/3d/dag_tex3d.h>

void ImGuiDagor::Image(const TEXTUREID &id, float aspect, const ImVec2 &uv0, const ImVec2 &uv1)
{
  int width = ImGui::GetContentRegionAvail().x;
  int height = width / aspect;

  ImGui::Image(reinterpret_cast<ImTextureID>(unsigned(id)), ImVec2(width, height), uv0, uv1);
}

void ImGuiDagor::Image(const TEXTUREID &id, Texture *texture, const ImVec2 &uv0, const ImVec2 &uv1)
{
  TextureInfo ti;
  texture->getinfo(ti);

  ImGuiDagor::Image(id, float(ti.w) / ti.h, uv0, uv1);
}
