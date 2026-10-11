// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "treeDestr.h"

#if RI_DESTR_DEBUG_ENABLED

#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <util/dag_console.h>

#include <rendInst/rendInstDebris.h>
#include <rendInst/treeDestr.h>

namespace rendinstdestr
{
static void imguiWindow()
{
  static bool isPlaying = false;

  rendinstdestr::BranchDestr &branchDestr =
    debugTreeInstData.last_object_was_from_damage ? debugTreeInstData.branchDestrFromDamage : debugTreeInstData.branchDestrOther;

  if (ImGui::Checkbox("Enable Branch Destruction", &get_tree_destr_mutable().branchDestrFromDamage.enableBranchDestruction))
    get_tree_destr_mutable().branchDestrOther.enableBranchDestruction =
      get_tree_destr_mutable().branchDestrFromDamage.enableBranchDestruction;

  ImGui::Checkbox("Use Debug Values", &useDebugTreeInstData);
  ImGui::Checkbox("Use New Physics", &branchDestr.newPhysics);
  if (ImGui::Button("Reset Debug Values"))
  {
    debugTreeInstData.timer_offset = 0.0f;
    branchDestr = get_tree_destr_mutable().branchDestrFromDamage;
  }

  static float maxTime = 3.0f;
  if (ImGui::Button(isPlaying ? "Stop" : "Play"))
    isPlaying = !isPlaying;

  if (isPlaying)
  {
    debugTreeInstData.timer_offset += ImGui::GetIO().DeltaTime;
    if (debugTreeInstData.timer_offset > maxTime)
      debugTreeInstData.timer_offset = 0.0f;
  }

  ImGui::SliderFloat("Timer Offset", &debugTreeInstData.timer_offset, -2.0f, maxTime, "%0.2f");
  ImGui::SliderFloat("Timer Max (For Play)", &maxTime, 0.0, 15.0f, "%0.2f");
  ImGui::Spacing();
  ImGui::SliderFloat("Impulse Mul", &branchDestr.impulseMul, 0, 15.0f, "%0.2f");

  ImGui::SliderFloat("Impulse Min", &branchDestr.impulseMin, 0, 100.0f, "%0.1f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Impulse Max", &branchDestr.impulseMax, 0, 100.0f, "%0.1f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Start keeping bigger branches than", &branchDestr.branchSizeMin, 0, 100.0f, "%0.2f",
    ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Keep all branches bigger than", &branchDestr.branchSizeMax, 0, 100.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Rotate Speed X", &branchDestr.rotateRandomSpeedMulX, 0, 10.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Rotate Speed Y", &branchDestr.rotateRandomSpeedMulY, 0, 10.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Rotate Speed Z", &branchDestr.rotateRandomSpeedMulZ, 0, 10.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Angle Spread", &branchDestr.rotateRandomAngleSpread, 0, 10.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Branch Size Slow Down", &branchDestr.branchSizeSlowDown, 0, 1.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Falling Speed", &branchDestr.fallingSpeedMul, 0, 2.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Falling Speed Rnd", &branchDestr.fallingSpeedRnd, 0, 1.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Horizontal Speed", &branchDestr.horizontalSpeedMul, 0, 20.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Fall Through Ground If Bigger", &branchDestr.fallThroughGroundIfBigger, 0, 20.0f, "%0.3f",
    ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Bend Strength", &branchDestr.bendStrength, 0, 0.2f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::Checkbox("Force Pyramid Canopy", &branchDestr.forcePyramidCanopy);
  ImGui::Checkbox("Disable cutting", &debugTreeInstData.disableCut);
  ImGui::Checkbox("Use Cut Height Override", &debugTreeInstData.useCutHeightOverride);
  ImGui::SliderFloat("Cut Height", &debugTreeInstData.cutHeightOverride, 0, 20.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Cut Damage Threshold", &branchDestr.cutInHalfDamageThreshold, -1, 2000.0f, "%0.1f",
    ImGuiSliderFlags_Logarithmic);

  float lastImpulse = -1.0f;
  float chance = 0.0f;
  if (riPhys.size())
  {
    lastImpulse = riPhys[riPhys.size() - 1].treeInstData.impactXZ.length();
    chance =
      clamp((lastImpulse - branchDestr.impulseMin) / max(branchDestr.impulseMax - branchDestr.impulseMin, 0.01f), 0.0f, 1.0f) * 100.0f;
  }

  ImGui::Text("Last impulse: %.2f. Chance: %.2f%% (%s)", lastImpulse, chance,
    debugTreeInstData.last_object_was_from_damage ? "From damage" : "Non-damage");
  ImGui::Text("Cut tree count: %d / %d", currentCutTreeCount, get_tree_destr().maxCutTreeCount);

  auto fnActivateTreeDestroyTool = [](bool activate, float damage, float radius, bool weaponDamage) {
    char cmd[100]; // Create a buffer to store the formatted string
    snprintf(cmd, sizeof(cmd), "dm.ri_damage %.2f %.2f %s", activate ? damage : 0.0f, radius, weaponDamage ? "true" : "false");
    console::command(cmd);
  };

  static bool showPhysicsShapes = false;
  if (ImGui::Checkbox("Show Physics Shapes", &showPhysicsShapes))
  {
    char cmd[100]; // Create a buffer to store the formatted string
    snprintf(cmd, sizeof(cmd), "physWorld.debug_draw %s", showPhysicsShapes ? "true" : "false");
    console::command(cmd);
  }

  static bool treeDestroyToolActive = false;
  static bool treeDestroyToolWeaponDamage = true;
  static float treeDestroyDamage = 500.0f;
  static float treeDestroyRadius = 0.0f;
  ImGui::SliderFloat("Destroy Damage", &treeDestroyDamage, 0, 5000, "%0.0f", ImGuiSliderFlags_Logarithmic);
  if (ImGui::Checkbox("Tree destroy with weapon damage", &treeDestroyToolWeaponDamage))
  {
    if (treeDestroyToolActive)
      fnActivateTreeDestroyTool(true, treeDestroyDamage, treeDestroyRadius, treeDestroyToolWeaponDamage);
  }

  static bool treeDestroyInArea = false;
  if (ImGui::Checkbox("Tree destroy in area", &treeDestroyInArea))
  {
    treeDestroyRadius = treeDestroyInArea ? 50.0f : 0.0f;
    fnActivateTreeDestroyTool(true, treeDestroyDamage, treeDestroyRadius, treeDestroyToolWeaponDamage);
  }

  if (ImGui::Checkbox("Tree destroy tool", &treeDestroyToolActive))
  {
    fnActivateTreeDestroyTool(treeDestroyToolActive, treeDestroyDamage, treeDestroyRadius, treeDestroyToolWeaponDamage);
  }

  if (ImGui::Button("Restore trees"))
  {
    for (int i = 0; i < riPhys.size(); ++i)
    {
      RendInstPhys &phys = riPhys[i];
      if (restore_destroyed_tree(phys))
      {
        erase_items(riPhys, i, 1);
        i--;
      }
    }
  }
}

REGISTER_IMGUI_WINDOW("Render", "Tree Destruction", imguiWindow);

int pull_tree_destr_imgui = 0;
} // namespace rendinstdestr
#endif
