// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entitySystem.h>
#include <generic/dag_enumerate.h>
#include <gui/dag_imgui.h>
#include <imgui.h>
#include "../../render/globalManager.h"
#include "../../render/placers/heightmap.h"

namespace dagdp
{

ECS_NO_ORDER static inline void heightmap_debug_invalidate_views_es(const dagdp::EventInvalidateViews &,
  dagdp::HeightmapManager &dagdp__heightmap_manager)
{
  dagdp__heightmap_manager.debug.builders.clear();
}

static void imgui(GlobalManager &globalManager, HeightmapManager &self)
{
  const auto &debug = eastl::as_const(self.debug);

  if (ImGui::TreeNodeEx("Overview", ImGuiTreeNodeFlags_Framed))
  {
    uint32_t totalTiles = 0;
    if (debug.builders.size() > 0)
    {
      const auto &mainCameraView = debug.builders[0]; // For now, assume it's always present at index 0.
      for (const auto &grid : mainCameraView.grids)
        totalTiles += grid.tiles.size();
    }

    ImGui::BulletText("Estimated GPU impact:");
    ImGui::Indent();
    ImGui::BulletText("Total tiles for main camera (larger is slower): %" PRIu32, totalTiles);
    ImGui::Unindent();

    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Config", ImGuiTreeNodeFlags_Framed))
  {
    ImGui::InputFloat("drawRangeLogBase", &self.config.drawRangeLogBase);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
      if (self.config.drawRangeLogBase <= 1.0f)
      {
        logerr("drawRangeLogBase must be > 1. Reset to 2.");
        self.config.drawRangeLogBase = 2.0f;
      }
      globalManager.invalidateRules();
    }
    ImGui::TreePop();
  }

  for (const auto [viewIndex, builder] : enumerate(debug.builders))
  {
    const auto &viewInfo = globalManager.getViewInfo(viewIndex);
    if (ImGui::TreeNodeEx(reinterpret_cast<void *>(viewIndex), ImGuiTreeNodeFlags_Framed, "View: %s", viewInfo.uniqueName.c_str()))
    {
      for (const auto [gridIndex, grid] : enumerate(builder.grids))
        if (ImGui::TreeNode(reinterpret_cast<void *>(gridIndex), "Grid #%zu: D=%f, TC=%" PRIu32, gridIndex, grid.density,
              grid.tiles.size()))
        {
          ImGui::BulletText("Density: %f", grid.density);
          ImGui::BulletText("Jitter: %f", grid.gridJitter);
          ImGui::BulletText("Seed: %d", grid.prngSeed);
          ImGui::BulletText("Tiles count: %" PRIu32, grid.tiles.size());
          int32_t xMax = 0;
          for (const auto &tile : grid.tiles)
            xMax = max(xMax, tile.x);
          ImGui::BulletText("Tiles bounds: [%" PRIi32 " .. %" PRIi32 "]", -xMax, xMax);
          ImGui::BulletText("Tile world size: %f", grid.tileWorldSize);
          ImGui::BulletText("Grid world range: %f", grid.tileWorldSize * xMax);
          ImGui::BulletText("Lower level: %d", grid.lowerLevel);
          ImGui::BulletText("Use dynamic allocation: %s", grid.useDynamicAllocation ? "yes" : "no");
          for (const auto [variantIndex, variant] : enumerate(grid.variants))
            if (ImGui::TreeNode(reinterpret_cast<void *>(variantIndex), "Variant #%zu", variantIndex))
            {
              ImGui::BulletText("Biomes:");
              for (const auto biome : variant.biomes)
              {
                ImGui::SameLine();
                ImGui::Text("%d", biome);
              }
              ImGui::BulletText("Effective density: %f", variant.effectiveDensity);
              for (const auto [objectGroupIndex, objectGroup] : enumerate(variant.objectGroups))
                if (ImGui::TreeNode(reinterpret_cast<void *>(objectGroupIndex), "Object group #%zu", objectGroupIndex))
                {
                  ImGui::BulletText("Effective density: %f", objectGroup.effectiveDensity);
                  // TODO: object group info probably should be separated as it is common.
                  // Maybe a button which opens the corresponding window is best.
                  ImGui::BulletText("Max draw distance: %f", objectGroup.info->maxDrawDistance);
                  ImGui::BulletText("Max placeable bounding radius: %f", objectGroup.info->maxPlaceableBoundingRadius);
                  for (const auto [placeableIndex, placeable] : enumerate(objectGroup.info->placeables))
                    if (ImGui::TreeNode(reinterpret_cast<void *>(placeableIndex), "Placeable #%zu", placeableIndex))
                    {
                      ImGui::BulletText("Flags: %" PRIx32, placeable.params.flags);
                      ImGui::BulletText("Weight: %f", placeable.params.weight);
                      auto showMD = [](const char *label, const Point2 &md) { ImGui::BulletText("%s: %f +/- %f", label, md.x, md.y); };
                      showMD("Scale", placeable.params.scaleMidDev);
                      showMD("Yaw", placeable.params.yawRadiansMidDev);
                      showMD("Pitch", placeable.params.pitchRadiansMidDev);
                      showMD("Roll", placeable.params.rollRadiansMidDev);
                      ImGui::BulletText("Slope factor: %f", placeable.params.slopeFactor);
                      for (const auto [rangeIndex, range] : enumerate(placeable.ranges))
                        if (ImGui::TreeNode(reinterpret_cast<void *>(rangeIndex), "Range #%zu: D=%f rID=%" PRIu32, rangeIndex,
                              range.baseDrawDistance, range.rId))
                        {
                          ImGui::BulletText("Base draw distance: %f", range.baseDrawDistance);
                          // TODO: show renderable info.
                          // Maybe a button which opens the corresponding window is best.
                          ImGui::BulletText("Renderable ID: %" PRIu32, range.rId);

                          ImGui::TreePop();
                        }
                      ImGui::TreePop();
                    }
                  ImGui::TreePop();
                }
              ImGui::TreePop();
            }
          ImGui::TreePop();
        }
      ImGui::TreePop();
    }
  }
}

template <typename Callable>
static inline void heightmap_debug_imgui_ecs_query(Callable);

static void imgui_callback()
{
  heightmap_debug_imgui_ecs_query([](dagdp::GlobalManager &dagdp__global_manager, dagdp::HeightmapManager &dagdp__heightmap_manager) {
    imgui(dagdp__global_manager, dagdp__heightmap_manager);
  });
}

} // namespace dagdp

static constexpr auto IMGUI_WINDOW_GROUP = "daGDP";
static constexpr auto IMGUI_WINDOW = "Heightmap##dagdp-heightmap";
REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP, IMGUI_WINDOW, dagdp::imgui_callback);
