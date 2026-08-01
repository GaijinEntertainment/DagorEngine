// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_viewportWindow.h"
#include "assetStats.h"
#include "av_cm.h"
#include "av_appwnd.h"
#include "de3_dynRenderService.h"

#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_ViewportWindowStatSettingsDialog.h>
#include <gameRes/dag_collisionResource.h>
#include <gui/dag_stdGuiRender.h>
#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>
#include <util/dag_string.h>

struct AssetStatType
{
  enum
  {
    TrianglesRenderable = 0,
    PhysGeometry,
    TraceGeometry,
    MaterialCount,
    TextureCount,
    CurrentLod,

    Count,
  };
};

G_STATIC_ASSERT(AssetStatType::Count == 6);
static const char *asset_stat_names[AssetStatType::Count] = {
  "triangles renderable", "phys geometry", "trace geometry", "material count", "texture count", "current LOD"};
static bool displayed_asset_stats[AssetStatType::Count] = {true, true, true, true, true, true};

struct FxStatType
{
  enum
  {
    Instances = 0,
    SimulatedElems,
    DrawCalls,
    Tris,
    ElemSizes,
    Count,
  };
};

G_STATIC_ASSERT(FxStatType::Count == 5);
static const char *fx_stat_names[FxStatType::Count] = {"instances", "simulated elems", "draw calls", "tris", "element sizes"};
static bool displayed_fx_stats[FxStatType::Count] = {true, true, true, true, true};

static int get_fx_stat_index_by_name(const char *name)
{
  for (int i = 0; i < FxStatType::Count; ++i)
    if (strcmp(fx_stat_names[i], name) == 0)
      return i;
  return -1;
}

int AssetViewerViewportWindow::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case CM_CAMERAS_FREE:
    case CM_CAMERAS_FPS:
    case CM_CAMERAS_TPS: static_cast<PropPanel::IMenuEventHandler &>(get_app()).onMenuItemClick(id); break;
  }

  return ViewportWindow::onMenuItemClick(id);
}

void AssetViewerViewportWindow::load(const DataBlock &blk)
{
  ViewportWindow::load(blk);

  showAssetStats = blk.getBool("show_asset_stats", showAssetStats);
  showFxStats = blk.getBool("show_fx_stats", showFxStats);

  if (const auto *assetStatBlock = blk.getBlockByName("displayed_asset_stat_list"))
  {
    for (int i = 0; i < AssetStatType::Count; ++i)
      displayed_asset_stats[i] = false;

    const int nid = assetStatBlock->getNameId("stat");
    for (int i = 0; i < assetStatBlock->paramCount(); ++i)
      if (assetStatBlock->getParamType(i) == DataBlock::TYPE_STRING && assetStatBlock->getParamNameId(i) == nid)
      {
        const char *statName = assetStatBlock->getStr(i);
        const int statIndex = getAssetStatIndexByName(statName);
        if (statIndex >= 0)
          displayed_asset_stats[statIndex] = true;
      }
  }

  if (const DataBlock *fxStatBlock = blk.getBlockByName("displayed_fx_stat_list"))
  {
    for (int i = 0; i < FxStatType::Count; ++i)
      displayed_fx_stats[i] = false;

    const int nid = fxStatBlock->getNameId("stat");
    for (int i = 0; i < fxStatBlock->paramCount(); ++i)
      if (fxStatBlock->getParamType(i) == DataBlock::TYPE_STRING && fxStatBlock->getParamNameId(i) == nid)
      {
        const char *statName = fxStatBlock->getStr(i);
        const int statIndex = get_fx_stat_index_by_name(statName);
        if (statIndex >= 0)
          displayed_fx_stats[statIndex] = true;
      }
  }
}

void AssetViewerViewportWindow::save(DataBlock &blk) const
{
  ViewportWindow::save(blk);

  blk.setBool("show_asset_stats", showAssetStats);
  blk.setBool("show_fx_stats", showFxStats);

  DataBlock *assetStatBlock = blk.addBlock("displayed_asset_stat_list");
  assetStatBlock->clearData();

  for (int i = 0; i < AssetStatType::Count; ++i)
    if (displayed_asset_stats[i])
      assetStatBlock->addStr("stat", asset_stat_names[i]);

  DataBlock *fxStatBlock = blk.addBlock("displayed_fx_stat_list");
  fxStatBlock->clearData();

  for (int i = 0; i < FxStatType::Count; ++i)
    if (displayed_fx_stats[i])
      fxStatBlock->addStr("stat", fx_stat_names[i]);
}

void AssetViewerViewportWindow::formatGeometryStat(String &statText, const char *stat_name, const AssetStats::GeometryStat &geometry)
{
  G_STATIC_ASSERT(NUM_COLLISION_NODE_TYPES == 6);

  bool firstStat = true;

  statText.printf(64, "%s: ", stat_name);

  if (geometry.meshNodeCount > 0)
  {
    statText.aprintf(64, "%d meshes (%d faces)", geometry.meshNodeCount, geometry.meshTriangleCount);
    firstStat = false;
  }

  if (geometry.boxNodeCount > 0)
  {
    if (!firstStat)
      statText += ", ";
    statText.aprintf(64, "%d boxes", geometry.boxNodeCount);
    firstStat = false;
  }

  if (geometry.sphereNodeCount > 0)
  {
    if (!firstStat)
      statText += ", ";
    statText.aprintf(64, "%d spheres", geometry.sphereNodeCount);
    firstStat = false;
  }

  if (geometry.capsuleNodeCount > 0)
  {
    if (!firstStat)
      statText += ", ";
    statText.aprintf(64, "%d capsules", geometry.capsuleNodeCount);
    firstStat = false;
  }

  if (geometry.convexNodeCount > 0)
  {
    if (!firstStat)
      statText += ", ";
    statText.aprintf(64, "%d convex (%d points)", geometry.convexNodeCount, geometry.convexVertexCount);
    firstStat = false;
  }

  if (firstStat)
    statText += "none";
}

void AssetViewerViewportWindow::paint(int w, int h)
{
  using hdpi::_pxScaled;

  ViewportWindow::paint(w, h);

  const bool showAsset = needShowAssetStats();
  const bool showFx = needShowFxStats() && fxStats.valid;
  if (!showAsset && !showFx)
    return;

  StdGuiRender::start_render();

  String statText;
  auto drawLine = [&](const String &text) {
    drawText(_pxScaled(8), nextStat3dLineY, text);
    nextStat3dLineY += _pxScaled(20);
  };

  if (showAsset)
  {
    if (assetStats.assetType == AssetStats::AssetType::None)
    {
      if (!showFx)
      {
        statText = "asset stats: -";
        drawLine(statText);
      }
    }
    else
    {
      for (int i = 0; i < AssetStatType::Count; ++i)
      {
        if (!displayed_asset_stats[i])
          continue;

        if (assetStats.assetType == AssetStats::AssetType::Collision && i != AssetStatType::PhysGeometry &&
            i != AssetStatType::TraceGeometry)
          continue;

        if (i == AssetStatType::CurrentLod)
        {
          if (assetStats.mixedLod)
            statText.printf(64, "%s: mixed", asset_stat_names[AssetStatType::CurrentLod]);
          else if (assetStats.currentLod >= 0)
            statText.printf(64, "%s: %d", asset_stat_names[AssetStatType::CurrentLod], assetStats.currentLod);
          else
            statText.printf(64, "%s: none", asset_stat_names[AssetStatType::CurrentLod]);
        }
        else if (i == AssetStatType::PhysGeometry)
          formatGeometryStat(statText, asset_stat_names[i], assetStats.physGeometry);
        else if (i == AssetStatType::TraceGeometry)
          formatGeometryStat(statText, asset_stat_names[i], assetStats.traceGeometry);
        else
          statText.printf(64, "%s: %d", asset_stat_names[i], getAssetStatByIndex(i));

        drawLine(statText);
      }
    }
  }

  if (showFx)
  {
    for (int i = 0; i < FxStatType::Count; ++i)
    {
      if (!displayed_fx_stats[i])
        continue;

      switch (i)
      {
        case FxStatType::Instances: statText.printf(64, "fx instances: %d", fxStats.instances); break;
        case FxStatType::SimulatedElems:
          statText.printf(64, "fx cpu elems: %d", fxStats.cpuElemProcessed);
          drawLine(statText);
          statText.printf(64, "fx gpu elems: %d", fxStats.gpuElemProcessed);
          break;
        case FxStatType::DrawCalls: statText.printf(64, "fx draw calls: %d", fxStats.drawCalls); break;
        case FxStatType::Tris: statText.printf(64, "fx tris %d/%d", fxStats.visibleTriangles, fxStats.renderedTriangles); break;
        case FxStatType::ElemSizes:
          statText.printf(96, "param ren: %d, sim: %d", fxStats.paramRenSize, fxStats.paramSimSize);
          drawLine(statText);
          statText.printf(96, "particle ren: %d, sim: %d", fxStats.partRenSize, fxStats.partSimSize);
          break;
      }

      drawLine(statText);
    }
  }

  StdGuiRender::end_render();
}

void AssetViewerViewportWindow::fillStatSettingsDialog(ViewportWindowStatSettingsDialog &dialog, bool include_camera_distance)
{
  ViewportWindow::fillStatSettingsDialog(dialog, include_camera_distance);

  PropPanel::TLeafHandle assetStatsGroup =
    dialog.addGroup(CM_STATS_SETTINGS_ASSET_STATS_GROUP, "Asset stats", showAssetStats, DEFAULT_SHOW_ASSET_STATS);
  G_STATIC_ASSERT((CM_STATS_SETTINGS_ASSET_STAT_LAST - CM_STATS_SETTINGS_ASSET_STAT_FIRST + 1) == AssetStatType::Count);
  for (int i = 0; i < AssetStatType::Count; ++i)
    dialog.addOption(assetStatsGroup, CM_STATS_SETTINGS_ASSET_STAT_FIRST + i, asset_stat_names[i], displayed_asset_stats[i]);

  PropPanel::TLeafHandle fxStatsGroup =
    dialog.addGroup(CM_STATS_SETTINGS_FX_STATS_GROUP, "FX stats", showFxStats, DEFAULT_SHOW_FX_STATS);
  G_STATIC_ASSERT((CM_STATS_SETTINGS_FX_STAT_LAST - CM_STATS_SETTINGS_FX_STAT_FIRST + 1) == FxStatType::Count);
  for (int i = 0; i < FxStatType::Count; ++i)
    dialog.addOption(fxStatsGroup, CM_STATS_SETTINGS_FX_STAT_FIRST + i, fx_stat_names[i], displayed_fx_stats[i]);
}

void AssetViewerViewportWindow::handleStatSettingsDialogChange(int pcb_id, bool value)
{
  G_STATIC_ASSERT((CM_STATS_SETTINGS_ASSET_STAT_LAST - CM_STATS_SETTINGS_ASSET_STAT_FIRST + 1) == AssetStatType::Count);
  if (pcb_id >= CM_STATS_SETTINGS_ASSET_STAT_FIRST && pcb_id <= CM_STATS_SETTINGS_ASSET_STAT_LAST)
  {
    const int statIndex = pcb_id - CM_STATS_SETTINGS_ASSET_STAT_FIRST;
    displayed_asset_stats[statIndex] = value;
  }
  else if (pcb_id == CM_STATS_SETTINGS_ASSET_STATS_GROUP)
    showAssetStats = value;
  else if (pcb_id >= CM_STATS_SETTINGS_FX_STAT_FIRST && pcb_id <= CM_STATS_SETTINGS_FX_STAT_LAST)
    displayed_fx_stats[pcb_id - CM_STATS_SETTINGS_FX_STAT_FIRST] = value;
  else if (pcb_id == CM_STATS_SETTINGS_FX_STATS_GROUP)
    showFxStats = value;
  else
    ViewportWindow::handleStatSettingsDialogChange(pcb_id, value);
}

bool AssetViewerViewportWindow::canStartInteractionWithViewport()
{
  return ViewportWindow::canStartInteractionWithViewport() && !get_app().isGizmoOperationStarted();
}

int AssetViewerViewportWindow::getAssetStatByIndex(int index)
{
  G_STATIC_ASSERT(AssetStatType::Count == 6);
  // PhysGeometry and TraceGeometry are intentionally not here.
  switch (index)
  {
    case AssetStatType::TrianglesRenderable: return assetStats.trianglesRenderable;
    case AssetStatType::MaterialCount: return assetStats.materialCount;
    case AssetStatType::TextureCount: return assetStats.textureCount;
    case AssetStatType::CurrentLod: return assetStats.currentLod;
  }

  G_ASSERT(false);
  return -1;
}

int AssetViewerViewportWindow::getAssetStatIndexByName(const char *name)
{
  for (int i = 0; i < AssetStatType::Count; ++i)
    if (strcmp(asset_stat_names[i], name) == 0)
      return i;

  return -1;
}

BaseTexture *AssetViewerViewportWindow::getDepthBuffer()
{
  if (auto *srv = EDITORCORE->queryEditorInterface<IDynRenderService>())
  {
    return srv->getDepthBuffer();
  }

  return nullptr;
}
