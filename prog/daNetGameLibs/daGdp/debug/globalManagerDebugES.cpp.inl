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

  if (ImGui::TreeNodeEx("Overview", ImGuiTreeNodeFlags_Framed))
  {
    uint32_t maxInstances = 0;
    for (int viewIndex = 0; viewIndex < views.size(); ++viewIndex)
    {
      const auto &builder = debug.builders[viewIndex];
      const auto &view = views[viewIndex];
      maxInstances += builder.maxInstancesPerViewport * view.info.maxViewports;
    }

    uint64_t reservedBytes = maxInstances * sizeof(PerInstanceData);

    ImGui::BulletText("Estimated GPU Memory impact:");
    ImGui::Indent();
    ImGui::BulletText("Reserved for instances: %" PRIu64 " bytes", reservedBytes);
    ImGui::BulletText("(~%.1f MiB)", reservedBytes / (1024.0f * 1024.0f));
    ImGui::Unindent();

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
      ImGui::BulletText("Max instances %" PRIu32, builder.maxInstancesPerViewport * view.info.maxViewports);
      ImGui::BulletText("Max instances per viewport: %" PRIu32, builder.maxInstancesPerViewport);
      ImGui::BulletText("Renderables: %" PRIu32, builder.numRenderables);

      ImGui::TreePop();
    }
  }
}

template <typename Callable>
static inline void global_manager_debug_imgui_ecs_query(Callable);

static void imgui_callback()
{
  global_manager_debug_imgui_ecs_query([](GlobalManager &dagdp__global_manager) { dagdp__global_manager.imgui(); });
}

} // namespace dagdp

static constexpr auto IMGUI_WINDOW_GROUP = "daGDP";
static constexpr auto IMGUI_WINDOW = "daGDP##dagdp";
REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP, IMGUI_WINDOW, dagdp::imgui_callback);