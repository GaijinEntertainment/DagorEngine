// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "texture_preview_panel.h"
#include "pluginService/graph_tex_gen_service.h"

#include <de3_interface.h>
#include <EditorCore/ec_interface.h>
#include <gui/dag_imguiUtil.h>
#include <imgui/imgui.h>

#include <textureGen/textureGenerator.h>
#include <textureGen/textureRegManager.h>


TexturePreviewPanel::TexturePreviewPanel(GraphEditorPlg &plg, IGraphTexGenService &ctx) :
  plugin(plg), texGenService(ctx), panelWindow(nullptr)
{
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Texture Preview");
}

TexturePreviewPanel::~TexturePreviewPanel() { IEditorCoreEngine::get()->deleteCustomPanel(panelWindow); }

void TexturePreviewPanel::updateImgui()
{
  const SelectedTextureState &state = texGenService.getSelectedTextureState();

  if (!state.texture || state.width <= 0 || state.height <= 0)
  {
    ImGui::TextDisabled("No texture selected");
    return;
  }

  const ImVec2 availRaw = ImGui::GetContentRegionAvail();
  if (availRaw.x <= 0 || availRaw.y <= 0)
  {
    return;
  }

  // Clamp content area to a minimum of 64x64 so very small panels still get a usable layout.
  const ImVec2 avail(ImMax(availRaw.x, 64.0f), ImMax(availRaw.y, 64.0f));

  // Wrap body in a child with no scrollbars so contents always fit (or clip) instead of scrolling.
  if (!ImGui::BeginChild("tex_preview_content", avail, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
  {
    ImGui::EndChild();
    return;
  }

  int texW = state.width;
  int texH = state.height;

  // Reserve one text line at the bottom for the info string.
  const float infoH = ImGui::GetTextLineHeightWithSpacing();
  const ImVec2 imageAreaSize(avail.x, ImMax(0.0f, avail.y - infoH));
  const ImVec2 imageAreaTopLeft = ImGui::GetCursorScreenPos();

  if (imageAreaSize.x <= 0.0f || imageAreaSize.y <= 0.0f || texW <= 0 || texH <= 0) // -V560
  {
    ImGui::EndChild();
    return;
  }

  // baseSize = largest rectangle preserving the texture aspect ratio that fits in imageAreaSize.
  // This is the "zoom = 1" rectangle.
  const float texAspect = (float)texW / (float)texH;
  const float areaAspect = imageAreaSize.x / imageAreaSize.y;
  ImVec2 baseSize;
  if (areaAspect > texAspect)
  {
    baseSize.y = imageAreaSize.y;
    baseSize.x = imageAreaSize.y * texAspect;
  }
  else
  {
    baseSize.x = imageAreaSize.x;
    baseSize.y = imageAreaSize.x / texAspect;
  }

  // Compute display size and UVs depending on zoom direction.
  ImVec2 displaySize;
  ImVec2 uv0, uv1;
  if (viewScale >= 1.0f)
  {
    displaySize = baseSize;
    const float halfW = 0.5f / viewScale;
    const float halfH = 0.5f / viewScale;
    uv0 = ImVec2(viewCenter.x - halfW, viewCenter.y - halfH);
    uv1 = ImVec2(viewCenter.x + halfW, viewCenter.y + halfH);
  }
  else
  {
    // Zoom out: shrink the image inside the zoom=1 rectangle, no tiling.
    displaySize = ImVec2(baseSize.x * viewScale, baseSize.y * viewScale);
    uv0 = ImVec2(0.0f, 0.0f);
    uv1 = ImVec2(1.0f, 1.0f);
  }

  // Center the image inside the image area.
  const ImVec2 imagePos(imageAreaTopLeft.x + (imageAreaSize.x - displaySize.x) * 0.5f,
    imageAreaTopLeft.y + (imageAreaSize.y - displaySize.y) * 0.5f);
  ImGui::SetCursorScreenPos(imagePos);

  ImGui::Image(ImGuiDagor::EncodeTexturePtr<ImTextureID>(state.texture), displaySize, uv0, uv1);

  // Input handling
  if (ImGui::IsItemHovered())
  {
    ImGuiIO &io = ImGui::GetIO();

    // Zoom with mouse wheel
    if (io.MouseWheel != 0.0f)
    {
      float zoomFactor = 1.0f + io.MouseWheel * 0.1f;
      viewScale = clamp(viewScale * zoomFactor, 0.1f, 64.0f);
    }

    // Pan with middle mouse drag (only when zoomed in; when zoomed out the texture fits entirely).
    if (viewScale >= 1.0f && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
      ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
      ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
      viewCenter.x -= delta.x / (displaySize.x * viewScale);
      viewCenter.y -= delta.y / (displaySize.y * viewScale);
    }

    // Show pixel info tooltip
    ImVec2 mousePos = ImGui::GetMousePos();
    float relX = (mousePos.x - imagePos.x) / displaySize.x;
    float relY = (mousePos.y - imagePos.y) / displaySize.y;
    float texU = uv0.x + relX * (uv1.x - uv0.x);
    float texV = uv0.y + relY * (uv1.y - uv0.y);
    int px = static_cast<int>(texU * texW);
    int py = static_cast<int>(texV * texH);

    Color4 c;
    if (px >= 0 && px < texW && py >= 0 && py < texH && texGenService.readSelectedTexturePixel(px, py, c))
    {
      ImGui::SetTooltip("[%d, %d]\nR: %.4f  G: %.4f\nB: %.4f  A: %.4f\n(%d, %d, %d, %d)", px, py, c.r, c.g, c.b, c.a,
        clamp(static_cast<int>(c.r * 255.f + 0.5f), 0, 255), clamp(static_cast<int>(c.g * 255.f + 0.5f), 0, 255),
        clamp(static_cast<int>(c.b * 255.f + 0.5f), 0, 255), clamp(static_cast<int>(c.a * 255.f + 0.5f), 0, 255));
    }

    // Reset zoom with double click
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
      viewScale = 1.0f;
      viewCenter = {0.5f, 0.5f};
    }
  }

  // Show texture info at bottom of the reserved strip, regardless of where the image ended up.
  ImGui::SetCursorScreenPos(ImVec2(imageAreaTopLeft.x, imageAreaTopLeft.y + imageAreaSize.y));
  ImGui::Text("%s  %dx%d  %dch  zoom: %.1fx", state.name.c_str(), texW, texH, state.channels, viewScale);

  ImGui::EndChild();
}
