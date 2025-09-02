// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "landMesh/vtex.hlsli"
#include "clipmapDebug.h"
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>

ClipmapDebugStats clipmapDebugStats;

static void clipmapImguiTableRowTexel(const char *name, const eastl::array<float, TEX_MIPS> &perMipData)
{
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(name);
  for (int i = 0; i < perMipData.size(); ++i)
  {
    ImGui::TableNextColumn();
    ImGui::Text("%.5f", perMipData[i]);
  }
}

static void clipmapImguiTableRowMipArea(const char *name, const eastl::array<float, TEX_MIPS> &perMipAreas,
  const eastl::array<int, TEX_MIPS> &perMipTiles)
{
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(name);
  float total = 0.f;
  for (int i = 0; i < perMipAreas.size(); ++i)
  {
    ImGui::TableNextColumn();
    float perMipKM2 = clipmapDebugStats.tilesUsed[i] * perMipAreas[i];
    total += perMipKM2;
    ImGui::Text("%.5f", perMipKM2);
  }
  ImGui::TableNextColumn();
  ImGui::Text("%.5f", total);
}

static void clipmapImguiTableRowTiles(const char *name, const eastl::array<int, TEX_MIPS> &mips)
{
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(name);
  int total = 0;
  for (int i = 0; i < TEX_MIPS; ++i)
  {
    total += mips[i];
    ImGui::TableNextColumn();
    ImGui::Text("%d / %.1f%%", mips[i], 100.f * float(mips[i]) / clipmapDebugStats.tilesCount);
  }
  ImGui::TableNextColumn();
  ImGui::Text("%d / %.1f%%", total, 100.f * float(total) / clipmapDebugStats.tilesCount);
}

static void clipmapImguiMipHeaderRow()
{
  String columnNameStr(framemem_ptr());
  columnNameStr.reserve(16);
  for (int i = 0; i < TEX_MIPS; ++i)
  {
    columnNameStr.printf(8, "Mip %d", i);
    ImGui::TableSetupColumn(columnNameStr.c_str());
  }
}

static void clipmapImguiWindow()
{
  eastl::array<float, TEX_MIPS> texelRatios;
  eastl::array<float, TEX_MIPS> tileSizesMeters;
  eastl::array<float, TEX_MIPS> tilesAreasKM2;
  for (int i = 0; i < TEX_MIPS; ++i)
  {
    texelRatios[i] = clipmapDebugStats.texelSize * (clipmapDebugStats.tileZoom << i);
    tileSizesMeters[i] = texelRatios[i] * clipmapDebugStats.texTileInnerSize;
    tilesAreasKM2[i] = tileSizesMeters[i] * tileSizesMeters[i] * 10e-6;
  }

  ImGui::Text("Cache dim: %d, %d", clipmapDebugStats.cacheDim.x, clipmapDebugStats.cacheDim.y);
  ImGui::Text("Cache tiles: %d", clipmapDebugStats.tilesCount);
  ImGui::Text("Tile size / inner: %d / %d", clipmapDebugStats.texTileSize, clipmapDebugStats.texTileInnerSize);
  ImGui::Text("Tile zoom: %d", clipmapDebugStats.tileZoom);
  ImGui::SeparatorText("Texel");
  ImGui::Text("Base texel size: %f", clipmapDebugStats.texelSize);
  if (ImGui::BeginTable("TexelSizes", TEX_MIPS + 1))
  {
    ImGui::TableSetupColumn("Measure");
    clipmapImguiMipHeaderRow();
    ImGui::TableHeadersRow();
    clipmapImguiTableRowTexel("Texel ratio", texelRatios);
    clipmapImguiTableRowTexel("Tile size (meters)", tileSizesMeters);
    clipmapImguiTableRowTexel("Tile area (km2)", tilesAreasKM2);
    ImGui::EndTable();
  }
  ImGui::SeparatorText("Tiles");
  ImGui::Text("Unassigned tiles: %d / %.1f%%", clipmapDebugStats.tilesUnassigned,
    100.f * float(clipmapDebugStats.tilesUnassigned) / clipmapDebugStats.tilesCount);
  if (ImGui::BeginTable("Stats", TEX_MIPS + 2))
  {
    ImGui::TableSetupColumn("State");
    clipmapImguiMipHeaderRow();
    ImGui::TableSetupColumn("Total");
    ImGui::TableHeadersRow();
    clipmapImguiTableRowTiles("Invalid", clipmapDebugStats.tilesInvalid);
    clipmapImguiTableRowTiles("NeedUpdate", clipmapDebugStats.tilesNeedUpdate);
    clipmapImguiTableRowTiles("Used", clipmapDebugStats.tilesUsed);
    clipmapImguiTableRowTiles("Feedback", clipmapDebugStats.tilesFromFeedback);
    clipmapImguiTableRowTiles("Unused", clipmapDebugStats.tilesUnused);
    ImGui::EndTable();
  }
  ImGui::SeparatorText("Covered area (km2)");
  if (ImGui::BeginTable("CoveredArea", TEX_MIPS + 2))
  {
    ImGui::TableSetupColumn("State");
    clipmapImguiMipHeaderRow();
    ImGui::TableSetupColumn("Total");
    ImGui::TableHeadersRow();
    clipmapImguiTableRowMipArea("Used", tilesAreasKM2, clipmapDebugStats.tilesUsed);
    clipmapImguiTableRowMipArea("Feedback", tilesAreasKM2, clipmapDebugStats.tilesFromFeedback);
    ImGui::EndTable();
  }
}

REGISTER_IMGUI_WINDOW("Render", "Clipmap", clipmapImguiWindow);

bool clipmap_should_collect_debug_stats() { return imgui_window_is_visible("Render", "Clipmap"); }