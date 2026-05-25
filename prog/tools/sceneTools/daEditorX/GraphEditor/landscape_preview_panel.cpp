// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "landscape_preview_panel.h"
#include "landscape_preview_scene.h"
#include "pluginService/graph_tex_gen_service.h"
#include "plugin.h"

#include <de3_interface.h>
#include <EditorCore/ec_interface.h>

#include <gui/dag_imguiUtil.h>
#include <imgui/imgui.h>

LandscapePreviewPanel::LandscapePreviewPanel(GraphEditorPlg &plg, IGraphTexGenService &ctx) : plugin(plg), texGenService(ctx)
{
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Landscape preview");
}

LandscapePreviewPanel::~LandscapePreviewPanel()
{
  scene.reset();
  IEditorCoreEngine::get()->deleteCustomPanel(panelWindow);
}

void LandscapePreviewPanel::updateImgui()
{
  const ImVec2 availRaw = ImGui::GetContentRegionAvail();
  if (availRaw.x <= 0.0f || availRaw.y <= 0.0f)
  {
    return;
  }

  if (!scene)
  {
    scene = eastl::make_unique<LandscapePreviewScene>();
    scene->init();
  }

  scene->updateHeightmapParams(texGenService);

  if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
  {
    controllingWithRmb = false;
  }
  else if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
  {
    controllingWithRmb = true;
  }
  const bool controlling = controllingWithRmb;

  LandscapePreviewScene::CameraInput camIn;
  if (controlling)
  {
    camIn.fwd = ImGui::IsKeyDown(ImGuiKey_W);
    camIn.back = ImGui::IsKeyDown(ImGuiKey_S);
    camIn.left = ImGui::IsKeyDown(ImGuiKey_A);
    camIn.right = ImGui::IsKeyDown(ImGuiKey_D);
    camIn.worldUp = ImGui::IsKeyDown(ImGuiKey_E);
    camIn.worldDown = ImGui::IsKeyDown(ImGuiKey_C);
    camIn.turbo = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
    const ImVec2 md = ImGui::GetIO().MouseDelta;
    camIn.mouseDx = md.x;
    camIn.mouseDy = md.y;
    ImGui::GetIO().WantCaptureMouse = true;
    ImGui::GetIO().WantCaptureKeyboard = true;
  }
  const float dt = ImGui::GetIO().DeltaTime;
  scene->actCamera(dt, camIn);

  if (!scene->isServiceAvailable())
  {
    ImGui::TextWrapped("Landscape preview unavailable: %s", scene->getUnavailableReason());
    return;
  }

  // Arm the render. The actual render happens during the plugin's renderObjects()
  // (next frame, during the host's normal render cycle when d3d state is valid).
  scene->setRenderParams(dt);

  BaseTexture *tex = scene->getOutputTexture();
  if (!tex)
  {
    ImGui::TextDisabled("Rendering first frame...");
    return;
  }

  // Render target is allocated at a fixed size; crop a centered UV sub-rect to
  // match the panel's aspect so resizing the panel does not reallocate RTs.
  const IPoint2 allocSz = scene->getOutputAllocSize();
  const float panelAspect = availRaw.x / availRaw.y;
  const float allocAspect = (allocSz.x > 0 && allocSz.y > 0) ? float(allocSz.x) / float(allocSz.y) : panelAspect;
  ImVec2 uv0(0.0f, 0.0f), uv1(1.0f, 1.0f);
  if (panelAspect < allocAspect)
  {
    const float u = panelAspect / allocAspect;
    uv0.x = 0.5f - 0.5f * u;
    uv1.x = 0.5f + 0.5f * u;
  }
  else if (panelAspect > allocAspect)
  {
    const float v = allocAspect / panelAspect;
    uv0.y = 0.5f - 0.5f * v;
    uv1.y = 0.5f + 0.5f * v;
  }
  ImGui::Image(ImGuiDagor::EncodeTexturePtr<ImTextureID>(tex), availRaw, uv0, uv1);

  if (controlling)
  {
    ImGui::SetTooltip("WASD move | E/C up/down | Shift turbo | drag RMB to look");
  }
}
