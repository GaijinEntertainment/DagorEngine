// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFracture/core/shatterMaterialImgui.h>

#if DAGOR_DBGLEVEL > 0 || defined(DEBUG_INTERFACE_ENABLED)

#include <math/dag_mathBase.h>
#include <imgui/imgui.h>


namespace frx
{

bool imgui_draw_shatter_material_profile(ShatterMaterialProfileImGuiState &state)
{
  ShatterMaterialProfile &material = state.profile;
  bool changed = false;
  constexpr float RAD2DEG = 57.29578f, DEG2RAD = 0.01745329f;
  constexpr ImGuiSliderFlags CLAMP = ImGuiSliderFlags_AlwaysClamp;
  const auto clamp01c = [](float c) { return c < -1.f ? -1.f : (c > 1.f ? 1.f : c); };

  const char *flavorNames[] = {"Plane (flat cuts)", "Rough (heightmap)"};
  changed |= ImGui::Combo("flavor", &state.flavorIndex, flavorNames, IM_ARRAYSIZE(flavorNames));

  if (ImGui::CollapsingHeader("Shatter driver", ImGuiTreeNodeFlags_DefaultOpen))
  {
    changed |= ImGui::DragIntRange2("min/maxPieces", &material.minPieces, &material.maxPieces, 0.1f, 1, 100, "%d", nullptr, CLAMP);
    changed |= ImGui::SliderInt("impactPointCuts", &material.impactPointCuts, 0, 30, "%d", CLAMP);
    changed |= ImGui::DragFloatRange2("relativeSizeWindow", &material.relativeSizeWindow.x, &material.relativeSizeWindow.y, 0.01f,
      0.05f, 5.f, "%.3f", nullptr, CLAMP);
    changed |= ImGui::DragFloat("minCutRatio", &material.minCutRatio, 0.01f, 0.f, 0.49f, "%.3f", CLAMP);
  }

  if (ImGui::CollapsingHeader("Size modes", ImGuiTreeNodeFlags_DefaultOpen))
  {
    for (int i = 0; i < int(material.sizeModes.size()); i++)
    {
      ImGui::PushID(i);
      ShatterMaterialProfile::SizeMode &m = material.sizeModes[i];
      changed |= ImGui::DragFloat("weight", &m.weight, 0.1f, 0.f, 100.f, "%.3f", CLAMP);
      changed |= ImGui::DragFloatRange2("powerRange", &m.powerRange.x, &m.powerRange.y, 0.01f, 0.f, 1.f, "%.3f", nullptr, CLAMP);
      changed |= ImGui::DragFloatRange2("sizeRange", &m.sizeRange.x, &m.sizeRange.y, 0.1f, 0.f, 20.f, "%.3f", nullptr, CLAMP);
      ImGui::Separator();
      ImGui::PopID();
    }
    if (ImGui::Button("+##sm"))
    {
      material.sizeModes.push_back({.weight = 1.f, .powerRange = Point2(0.f, 1.f), .sizeRange = Point2(1.5f, 2.f)});
      changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("-##sm") && !material.sizeModes.empty())
    {
      material.sizeModes.pop_back();
      changed = true;
    }
  }

  if (ImGui::CollapsingHeader("Cut plane modes", ImGuiTreeNodeFlags_DefaultOpen))
  {
    for (int i = 0; i < int(material.cutPlaneModes.size()); i++)
    {
      ImGui::PushID(1000 + i);
      ShatterMaterialProfile::CutPlaneMode &m = material.cutPlaneModes[i];
      changed |= ImGui::DragFloat("weight", &m.weight, 0.1f, 0.f, 100.f, "%.3f", CLAMP);
      float deg[2] = {acosf(clamp01c(m.cosineRange.x)) * RAD2DEG, acosf(clamp01c(m.cosineRange.y)) * RAD2DEG};
      if (ImGui::DragFloatRange2("angleRange (deg)", &deg[0], &deg[1], 0.5f, 0.f, 90.f, "%.3f", nullptr, CLAMP))
      {
        m.cosineRange = Point2(cosf(deg[0] * DEG2RAD), cosf(deg[1] * DEG2RAD));
        changed = true;
      }
      ImGui::Separator();
      ImGui::PopID();
    }
    if (ImGui::Button("+##cp"))
    {
      material.cutPlaneModes.push_back({.weight = 1.f, .cosineRange = Point2(1.f, cosf(20.f * DEG2RAD))});
      changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("-##cp") && !material.cutPlaneModes.empty())
    {
      material.cutPlaneModes.pop_back();
      changed = true;
    }
  }

  if (state.flavorIndex == 1 && ImGui::CollapsingHeader("Rough (heightmap)", ImGuiTreeNodeFlags_DefaultOpen))
  {
    RoughShatterFlavorSettings &o = state.roughSettings;
    changed |= ImGui::DragFloat("relCell", &o.relCell, 0.005f, 0.005f, 0.5f, "%.3f", CLAMP);
    changed |= ImGui::DragFloatRange2("min/maxCell", &o.minCell, &o.maxCell, 0.01f, 0.005f, 2.f, "%.3f", nullptr, CLAMP);
    changed |= ImGui::DragFloat("relAmpl", &o.relAmpl, 0.1f, 0.f, 8.f, "%.3f", CLAMP);
    changed |= ImGui::SliderInt("ridgeCells", &o.ridgeCells, 1, 16, "%d", CLAMP);
    changed |= ImGui::DragFloatRange2("ridgeRange", &o.ridgeRange.x, &o.ridgeRange.y, 0.01f, 0.f, 1.f, "%.3f", nullptr, CLAMP);
    changed |= ImGui::DragFloat("ridgeJitter", &o.ridgeJitter, 0.01f, 0.f, 1.f, "%.3f", CLAMP);
    changed |= ImGui::DragFloat("detailFrac", &o.detailFrac, 0.01f, 0.f, 2.f, "%.3f", CLAMP);
    changed |= ImGui::DragFloat("detailCells", &o.detailCells, 0.1f, 0.5f, 8.f, "%.3f", CLAMP);
    changed |= ImGui::DragFloat("fadeCells", &o.fadeCells, 0.1f, 0.f, 8.f, "%.3f", CLAMP);
    changed |= ImGui::DragFloat("creaseFollowProb", &o.creaseFollowProb, 0.01f, 0.f, 1.f, "%.3f", CLAMP);
    changed |= ImGui::DragFloat("creaseBlendCells", &o.creaseBlendCells, 0.1f, 0.f, 10.f, "%.3f", CLAMP);
    changed |= ImGui::DragFloat("creaseMaxTiltDeg", &o.creaseMaxTiltDeg, 0.5f, 0.f, 89.f, "%.3f", CLAMP);
    changed |= ImGui::DragFloat("creaseTiltRnd", &o.creaseTiltRnd, 0.01f, 0.f, 1.f, "%.3f", CLAMP);
    changed |= ImGui::DragFloat("creaseMaxOffcenter", &o.creaseMaxOffcenter, 0.01f, 0.f, 1.f, "%.3f", CLAMP);
  }

  if (changed)
  {
    // fold the selected flavor's settings into the profile (variant assignment selects that alternative)
    if (state.flavorIndex == 0)
      material.flavor = state.planeSettings;
    else
      material.flavor = state.roughSettings;
  }
  return changed;
}

} // namespace frx

#else

namespace frx
{
bool imgui_draw_shatter_material_profile(ShatterMaterialProfileImGuiState &) { return false; }
} // namespace frx

#endif
