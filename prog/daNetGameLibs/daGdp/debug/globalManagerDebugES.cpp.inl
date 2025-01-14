// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entitySystem.h>
#include <generic/dag_enumerate.h>
#include <gui/dag_imgui.h>
#include <imgui.h>
#include "../render/globalManager.h"

namespace dagdp
{

void GlobalManager::imgui()
{
  // Just after invalidation, these will not match.
  if (debug.builders.size() != views.size())
    return;

  uint32_t maxInstances = 0;
  uint32_t dynamicInstances = 0;
  for (int viewIndex = 0; viewIndex < views.size(); ++viewIndex)
  {
    const auto &builder = debug.builders[viewIndex];
    maxInstances += builder.totalMaxInstances;
    dynamicInstances += views[viewIndex].dynamicInstanceCounter;
  }

  const float reservedTotalMiB = maxInstances * sizeof(PerInstanceData) / (1024.0f * 1024.0f);
  const float reservedDynamicMiB = rulesBuilder.maxObjects * sizeof(PerInstanceData) / (1024.0f * 1024.0f);
  const float reservedStaticMiB = reservedTotalMiB - reservedDynamicMiB;
  const float usedDynamicMiB = dynamicInstances * sizeof(PerInstanceData) / (1024.0f * 1024.0f);

  if (ImGui::TreeNodeEx("Overview", ImGuiTreeNodeFlags_Framed))
  {

    ImGui::BulletText("Estimated GPU Memory impact:");
    ImGui::Indent();
    ImGui::BulletText("Total reserved for instances: (~%.1f MiB)", reservedTotalMiB);
    ImGui::Indent();
    ImGui::BulletText("Reserved for static instances: (~%.1f MiB)", reservedStaticMiB);
    ImGui::BulletText("Reserved for dynamic instances: (~%.1f MiB)", reservedDynamicMiB);
    ImGui::Unindent();

    const float availableDynamicMiB = reservedDynamicMiB * DYNAMIC_THRESHOLD_MULTIPLIER;
    const uint32_t availableDynamicInstances = rulesBuilder.maxObjects * DYNAMIC_THRESHOLD_MULTIPLIER;
    ImGui::BulletText("Available dynamic memory budget: (~%.1f MiB). Debug threshold: (%d %%)", availableDynamicMiB,
      static_cast<int>(100 * DYNAMIC_THRESHOLD_MULTIPLIER));

    ImGui::BulletText("Used dynamic memory budget: ");
    eastl::string progressBarText({}, "%.1f%% (%.1f MiB / %.1f MiB)",
      rulesBuilder.maxObjects ? 100.0f * dynamicInstances / availableDynamicInstances : 0.0f, usedDynamicMiB, availableDynamicMiB);
    ImGui::ProgressBar(dynamicInstances / static_cast<float>(availableDynamicInstances), ImVec2(0.0f, 0.0f), progressBarText.c_str());
    if (dynamicInstances > availableDynamicInstances)
    {
      ImGui::SameLine();
      if (dynamicInstances > rulesBuilder.maxObjects)
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Hard limit is exceeded!\nRendering is disabled.");
      else
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Soft limit is exceeded!");
    }
    ImGui::TreePop();
  }

  for (int viewIndex = 0; viewIndex < views.size(); ++viewIndex)
  {
    const auto &builder = debug.builders[viewIndex];
    const auto &view = views[viewIndex];

    if (ImGui::TreeNodeEx(reinterpret_cast<void *>(viewIndex), ImGuiTreeNodeFlags_Framed, "View: %s", view.info.uniqueName.c_str()))
    {
      ImGui::BulletText("View kind: %d", static_cast<int>(eastl::to_underlying(view.info.kind)));
      ImGui::BulletText("View max draw distance: %f", view.info.maxDrawDistance);
      ImGui::BulletText("Max viewports: %" PRIu32, view.info.maxViewports);
      ImGui::BulletText("Total max instances %" PRIu32, builder.totalMaxInstances);
      ImGui::BulletText("Max static instances per viewport: %" PRIu32, builder.maxStaticInstancesPerViewport);
      ImGui::BulletText("Max dynamic instances: %" PRIu32, builder.dynamicInstanceRegion.maxCount);
      ImGui::BulletText("Renderables: %" PRIu32, builder.numRenderables);

      ImGui::TreePop();
    }
  }
}

template <typename Callable>
static inline void global_manager_debug_imgui_ecs_query(Callable);

static void imgui_callback()
{
  global_manager_debug_imgui_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.imgui(); });
}

} // namespace dagdp

static constexpr auto IMGUI_WINDOW_GROUP = "daGDP";
static constexpr auto IMGUI_WINDOW = "daGDP##dagdp";
REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP, IMGUI_WINDOW, dagdp::imgui_callback);
