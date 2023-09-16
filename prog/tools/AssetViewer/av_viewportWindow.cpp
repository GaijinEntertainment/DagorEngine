#include "av_viewportWindow.h"
#include "assetStats.h"
#include "av_cm.h"
#include <EditorCore/ec_cm.h>
#include <gameRes/dag_collisionResource.h>
#include <gui/dag_stdGuiRender.h>
#include <ioSys/dag_dataBlock.h>
#include <propPanel2/c_panel_base.h>
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

AssetViewerViewportWindow::AssetViewerViewportWindow(TEcHandle parent, int left, int top, int w, int h) :
  ViewportWindow(parent, left, top, w, h)
{}

void AssetViewerViewportWindow::load(const DataBlock &blk)
{
  __super::load(blk);

  showAssetStats = blk.getBool("show_asset_stats", false);

  const DataBlock *assetStatBlock = blk.getBlockByName("displayed_asset_stat_list");
  if (!assetStatBlock)
    return;

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

void AssetViewerViewportWindow::save(DataBlock &blk) const
{
  __super::save(blk);

  blk.setBool("show_asset_stats", showAssetStats);

  DataBlock *assetStatBlock = blk.addBlock("displayed_asset_stat_list");
  assetStatBlock->clearData();

  for (int i = 0; i < AssetStatType::Count; ++i)
    if (displayed_asset_stats[i])
      assetStatBlock->addStr("stat", asset_stat_names[i]);
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
  __super::paint(w, h);

  if (!needShowAssetStats())
    return;

  StdGuiRender::start_render();

  String statText;

  if (assetStats.assetType == AssetStats::AssetType::None)
  {
    statText = "asset stats: -";
    drawText(8, nextStat3dLineY, statText);
    nextStat3dLineY += 20;
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

      drawText(8, nextStat3dLineY, statText);
      nextStat3dLineY += 20;
    }
  }

  StdGuiRender::end_render();
}

void AssetViewerViewportWindow::fillStatSettingsDialog(PropertyContainerControlBase &tab_panel)
{
  __super::fillStatSettingsDialog(tab_panel);

  PropertyContainerControlBase *mainTabPage = tab_panel.getContainerById(CM_STATS_SETTINGS_MAIN_PAGE);
  G_ASSERT(mainTabPage);
  mainTabPage->createCheckBox(CM_STATS_SETTINGS_MAIN_SHOW_ASSETS_STATS, "Show asset stats", showAssetStats);

  PropertyContainerControlBase *tabPage = tab_panel.createTabPage(CM_STATS_SETTINGS_ASSET_STATS_PAGE, "Asset stats");

  G_STATIC_ASSERT((CM_STATS_SETTINGS_ASSET_STAT_LAST - CM_STATS_SETTINGS_ASSET_STAT_FIRST + 1) == AssetStatType::Count);
  for (int i = 0; i < AssetStatType::Count; ++i)
    tabPage->createCheckBox(CM_STATS_SETTINGS_ASSET_STAT_FIRST + i, asset_stat_names[i], displayed_asset_stats[i]);
}

void AssetViewerViewportWindow::handleStatSettingsDialogChange(int pcb_id)
{
  G_STATIC_ASSERT((CM_STATS_SETTINGS_ASSET_STAT_LAST - CM_STATS_SETTINGS_ASSET_STAT_FIRST + 1) == AssetStatType::Count);
  if (pcb_id >= CM_STATS_SETTINGS_ASSET_STAT_FIRST && pcb_id <= CM_STATS_SETTINGS_ASSET_STAT_LAST)
  {
    const int statIndex = pcb_id - CM_STATS_SETTINGS_ASSET_STAT_FIRST;
    displayed_asset_stats[statIndex] = !displayed_asset_stats[statIndex];
  }
  else if (pcb_id == CM_STATS_SETTINGS_MAIN_SHOW_ASSETS_STATS)
    showAssetStats = !showAssetStats;
  else
    __super::handleStatSettingsDialogChange(pcb_id);
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
