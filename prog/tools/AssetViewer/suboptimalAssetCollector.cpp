// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "suboptimalAssetCollector.h"

#include "av_appwnd.h"
#include "av_assetSearchResultsWindow.h"

#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMgr.h>
#include <coolConsole/coolConsole.h>
#include <de3_interface.h>
#include <EditorCore/ec_input.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <shaders/dag_rendInstRes.h>

namespace
{

struct SearchOptions
{
  int minTotalFaces = 100;
  float minRange = 50.0f;
  float excludeRange = 10000.0f;
  bool useExcludeRange = true;
};

class SuboptimalAssetCollector
{
public:
  struct SearchResult
  {
    const DagorAsset *asset;
    int totalFaces;
    float range;
  };

  void collect(const DagorAssetFolder &asset_folder, const SearchOptions &search_options)
  {
    EcAutoBusy autoBusy;

    CoolConsole &console = get_app().getConsole();
    const bool consoleWasOpen = console.isVisible();
    console.showConsole(true);

    const dag::Vector<const DagorAsset *> assetsToProcess = getAssetsToProcess(asset_folder);

    DAEDITOR3.conNote("Processing %d assets", assetsToProcess.size());
    console.startLog();
    console.setTotal(assetsToProcess.size());
    console.startProgress();

    int processedCount = 0;
    for (const DagorAsset *asset : assetsToProcess)
    {
      processRendInst(*asset, search_options);
      console.incDone();

      ++processedCount;
      if (processedCount == 100)
      {
        processedCount = 0;
        free_unused_game_resources();
      }
    }

    free_unused_game_resources();

    const bool hasErrorsOrWarnings = console.hasErrors() || console.hasWarnings();
    console.endLogAndProgress();

    if (!consoleWasOpen && !hasErrorsOrWarnings)
      console.hideConsole();
  }

  dag::ConstSpan<SearchResult> getSearchResults() const { return searchResults; }

private:
  dag::Vector<const DagorAsset *> getAssetsToProcess(const DagorAssetFolder &asset_folder) const
  {
    dag::Vector<const DagorAsset *> assetsToProcess;

    const DagorAssetMgr &assetMgr = get_app().getAssetMgr();
    const int rendInstTypeId = assetMgr.getAssetTypeId("rendInst");
    const int folderIndex = assetMgr.getFolderIndex(asset_folder);
    if (folderIndex >= 0)
      getAssetsFromFolderRecursively(assetMgr, folderIndex, rendInstTypeId, assetsToProcess);

    return assetsToProcess;
  }

  void processRendInst(const DagorAsset &asset, const SearchOptions &search_options)
  {
    const SimpleString riResName(asset.getName());
    Ptr<RenderableInstanceLodsResource> res =
      (RenderableInstanceLodsResource *)::get_one_game_resource_ex(riResName, RendInstGameResClassId);
    if (!res)
      return;

    ::release_game_resource_ex(res.get(), RendInstGameResClassId);

    const int transitionLods = asset.props.getBlockByName("transition_lod") && asset.props.getBlockByName("impostor") ? 1 : 0;
    const int lodCount = res->lods.size() - transitionLods;
    if (lodCount != 1)
      return;

    RenderableInstanceLodsResource::Lod &lod0 = res->lods[0];
    const int totalFaces = getTotalFaces(lod0);

    if (totalFaces > search_options.minTotalFaces && lod0.range > search_options.minRange)
    {
      if (search_options.useExcludeRange && lod0.range >= search_options.excludeRange)
        return;

      SearchResult searchResult;
      searchResult.asset = &asset;
      searchResult.totalFaces = totalFaces;
      searchResult.range = lod0.range;
      searchResults.push_back(searchResult);
      DAEDITOR3.conNote("  %s, total faces: %d, range: %g", asset.getName(), totalFaces, lod0.range);
    }
  }

  static int getTotalFaces(const RenderableInstanceLodsResource::Lod &lod)
  {
    if (!lod.scene)
      return -1;

    const InstShaderMeshResource *mesh = lod.scene->getMesh();
    if (!mesh)
      return -1;

    const InstShaderMesh *shaderMesh = mesh->getMesh();
    if (!shaderMesh)
      return -1;

    return shaderMesh->calcTotalFaces();
  }

  static void getAssetsFromFolderRecursively(const DagorAssetMgr &asset_mgr, int folder_index, int rend_inst_type_id,
    dag::Vector<const DagorAsset *> &assets)
  {
    const DagorAssetFolder *folder = asset_mgr.getFolderPtr(folder_index);
    if (!folder)
      return;

    for (int subFolderIdx : folder->subFolderIdx)
      getAssetsFromFolderRecursively(asset_mgr, subFolderIdx, rend_inst_type_id, assets);

    int assetStartIndex, assetEndIndex;
    asset_mgr.getFolderAssetIdxRange(folder_index, assetStartIndex, assetEndIndex);
    for (int i = assetStartIndex; i < assetEndIndex; ++i)
    {
      const DagorAsset &asset = asset_mgr.getAsset(i);
      if (asset.getType() == rend_inst_type_id)
        assets.push_back(&asset);
    }
  }

  dag::Vector<SearchResult> searchResults;
};

class StartDialog : public PropPanel::DialogWindow
{
public:
  // Due to the long messages the dialog's size is based on the font size. Otherwise it would be either too large at
  // 100% scale or not large enough at 175% scale.
  explicit StartDialog(const SearchOptions &search_options) :
    DialogWindow(nullptr, hdpi::_pxActual(ImGui::GetFontSize() * 22), hdpi::_pxActual(ImGui::GetFontSize() * 22),
      "Collect suboptimal assets"),
    searchOptions(search_options)
  {
    getPanel()->createEditInt(PID_MIN_TOTAL_FACES, "Total faces", searchOptions.minTotalFaces);
    getPanel()->createEditFloat(PID_MIN_RANGE, "Minimum draw distance (m)", searchOptions.minRange);
    getPanel()->createCheckBox(PID_USE_EXCLUDE_RANGE, "Exclude assets with large draw distance", searchOptions.useExcludeRange);
    getPanel()->createEditFloat(PID_EXCLUDE_RANGE, "Exclude draw distance (m)", searchOptions.excludeRange);
    getPanel()->createSeparator();
    getPanel()->createStatic(PID_HELP, "");

    updateExcludeRangeState();
    updateHelp();
  }

  const SearchOptions &getSearchOptions() const { return searchOptions; }

private:
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == PID_MIN_TOTAL_FACES)
    {
      searchOptions.minTotalFaces = panel->getInt(pcb_id);
      updateHelp();
    }
    else if (pcb_id == PID_MIN_RANGE)
    {
      searchOptions.minRange = panel->getFloat(pcb_id);
      updateHelp();
    }
    else if (pcb_id == PID_EXCLUDE_RANGE)
    {
      searchOptions.excludeRange = panel->getFloat(pcb_id);
      updateHelp();
    }
    else if (pcb_id == PID_USE_EXCLUDE_RANGE)
    {
      searchOptions.useExcludeRange = panel->getBool(pcb_id);
      updateExcludeRangeState();
      updateHelp();
    }
  }

  void updateExcludeRangeState() { getPanel()->setEnabledById(PID_EXCLUDE_RANGE, searchOptions.useExcludeRange); }

  void updateHelp()
  {
    String help(0, "List assets that have:\n- a single LOD\n- and more than %d total faces\n- and more than %g meters draw distance",
      searchOptions.minTotalFaces, searchOptions.minRange);
    if (searchOptions.useExcludeRange)
      help.aprintf(0, "\n\nbut exclude asset if\n- its draw distance is at least %g meters", searchOptions.excludeRange);

    getPanel()->setText(PID_HELP, help);
  }

  enum
  {
    PID_MIN_TOTAL_FACES = 1,
    PID_MIN_RANGE,
    PID_USE_EXCLUDE_RANGE,
    PID_EXCLUDE_RANGE,
    PID_HELP,
  };

  SearchOptions searchOptions;
};

SearchOptions last_used_search_options;

} // namespace

void load_suboptimal_asset_default_search_options(const DataBlock &app_blk)
{
  const DataBlock *assetViewerBlk = app_blk.getBlockByNameEx("asset_viewer");
  const DataBlock *searchOptionsBlk = assetViewerBlk->getBlockByNameEx("suboptimal_asset_search");
  last_used_search_options.minTotalFaces = searchOptionsBlk->getInt("minTotalFaces", last_used_search_options.minTotalFaces);
  last_used_search_options.minRange = searchOptionsBlk->getReal("minRange", last_used_search_options.minRange);
  last_used_search_options.useExcludeRange = searchOptionsBlk->getBool("useExcludeRange", last_used_search_options.useExcludeRange);
  last_used_search_options.excludeRange = searchOptionsBlk->getReal("excludeRange", last_used_search_options.excludeRange);
}

void show_suboptimal_asset_collector_dialog(const DagorAssetFolder &asset_folder)
{
  StartDialog dialog(last_used_search_options);
  if (dialog.showDialog() != PropPanel::DIALOG_ID_OK)
    return;

  last_used_search_options = dialog.getSearchOptions();

  SuboptimalAssetCollector suboptimalAssetCollector;
  suboptimalAssetCollector.collect(asset_folder, last_used_search_options);

  asset_search_results_window.reset(new AssetSearchResultsWindow("Suboptimal assets"));
  asset_search_results_window->addColumnTitle("Asset name");
  asset_search_results_window->addColumnTitle("Faces");
  asset_search_results_window->addColumnTitle("Range");

  String tempBuffer;
  for (const SuboptimalAssetCollector::SearchResult &searchResult : suboptimalAssetCollector.getSearchResults())
  {
    AssetSearchResultsListControl::SearchResult outputSearchResult;
    outputSearchResult.asset = searchResult.asset;

    tempBuffer.printf(0, "%d", searchResult.totalFaces);
    outputSearchResult.additionalColumns.emplace_back(tempBuffer.c_str());

    tempBuffer.printf(0, "%g", searchResult.range);
    outputSearchResult.additionalColumns.emplace_back(tempBuffer.c_str());

    asset_search_results_window->addResult(eastl::move(outputSearchResult));
  }

  asset_search_results_window->fillResultsList();
  asset_search_results_window->show();
}
