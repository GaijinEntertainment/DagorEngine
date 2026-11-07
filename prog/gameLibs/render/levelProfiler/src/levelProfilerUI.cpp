// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <cstring>
#include <EASTL/algorithm.h>
#include <debug/dag_log.h>
#include "levelProfilerUI.h"
#include "levelProfilerRiTable.h"
#include "riModule.h"

static_assert(levelprofiler::ImGuiConstants::WITH_BORDER == true, "WITH_BORDER constant must match ImGui boolean true value");
static_assert(levelprofiler::ImGuiConstants::NO_BORDER == false, "NO_BORDER constant must match ImGui boolean false value");

namespace levelprofiler
{

// --- TextureProfilerUI ---

TextureProfilerUI::TextureProfilerUI(TextureModule *texture_module, RIModule *ri_module, GlobalCopyManager &copy_manager) :
  textureModule(texture_module),
  riModule(ri_module),
  horizontalSplitter(LpSplitterDirection::HORIZONTAL, 0.6f),
  verticalSplitter(LpSplitterDirection::VERTICAL, 0.7f)
{
  textureTable = eastl::make_unique<LpTextureTable>(this);
  textureTable->setCopyManager(&copy_manager);
}

TextureProfilerUI::~TextureProfilerUI() {}

void TextureProfilerUI::selectTexture(const ProfilerString &texture_name)
{
  textureModule->resetMipLevel();
  textureTable->setSelectedName(texture_name);
}

void TextureProfilerUI::onDataCollected()
{
  // The selection is by name, so it survives a recollect as long as the texture is still there.
  if (textureTable)
  {
    const ProfilerString &selected = textureTable->getSelectedName();
    if (!selected.empty() && textureModule->getTextures().find(selected) == textureModule->getTextures().end())
      selectTexture("");
  }
}

void TextureProfilerUI::drawUI()
{
  ImVec2 availableArea = ImGui::GetContentRegionAvail();
  float leftPanelWidth = availableArea.x * horizontalSplitter.getRatio();

  // Left panel: table and filters
  ImGui::BeginChild("PoolLeft", ImVec2(leftPanelWidth, 0), ImGuiConstants::WITH_BORDER);
  drawLeftPanel();
  ImGui::EndChild();

  ImGui::SameLine();

  horizontalSplitter.draw(availableArea);

  // Right panel: preview and Usage list
  ImGui::SameLine();
  ImGui::BeginChild("PoolRight", ImVec2(0, 0), ImGuiConstants::WITH_BORDER);
  drawRightPanel();
  ImGui::EndChild();
}

void TextureProfilerUI::drawLeftPanel()
{
  float tableRegionHeight = ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 2.5f;

  ImGui::BeginChild("TableRegion", ImVec2(0, tableRegionHeight), ImGuiConstants::NO_BORDER);
  if (textureTable)
    textureTable->draw();
  ImGui::EndChild();

  ImGui::Separator();

  const LpTextureMemorySums memory = textureTable->sumMemory();
  lp_draw_row_count("Textures", textureTable->getVisibleRowCount(), textureTable->getRowCount());
  lp_draw_column_sum(memory.label.c_str(), memory.memory, lp_format_memory_sum_mb);
  if (!memory.differenceLabel.empty())
  {
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    lp_draw_column_sum(memory.differenceLabel.c_str(), memory.difference, lp_format_memory_sum_mb);
  }
}

void TextureProfilerUI::drawRightPanel()
{
  // Calculate available area and divide vertically
  ImVec2 availablePanelArea = ImGui::GetContentRegionAvail();
  float previewPaneHeight = availablePanelArea.y * verticalSplitter.getRatio();

  // Preview pane (top)
  ImGui::BeginChild("PreviewPane", ImVec2(0, previewPaneHeight), ImGuiConstants::NO_BORDER);
  {
    if (textureTable->getSelectedName().empty() && textureTable->getVisibleRowCount() > 0)
      selectTexture(textureTable->getVisibleRow(0).name);

    if (const ProfilerString &selectedTextureName = textureTable->getSelectedName(); !selectedTextureName.empty())
      textureModule->drawTextureView(selectedTextureName);
  }
  ImGui::EndChild();

  verticalSplitter.draw(availablePanelArea);

  // Texture Usage pane (bottom)
  ImGui::BeginChild("TextureUsagePane", ImVec2(0, 0), ImGuiConstants::WITH_BORDER);
  {
    ImGui::TextUnformatted("Texture Usage Objects:");

    const ProfilerString &currentSelectedTextureName = textureTable->getSelectedName();

    // Clear selected asset if texture changed
    if (static ProfilerString lastSelectedTexture; lastSelectedTexture != currentSelectedTextureName)
    {
      selectedAssetName.clear();
      lastSelectedTexture = currentSelectedTextureName;
    }

    if (!currentSelectedTextureName.empty())
    {
      // Look up assets using this texture
      const auto &textureToAssetsMap = riModule->getTextureToAssetsMap();
      if (auto iterator = textureToAssetsMap.find(currentSelectedTextureName); iterator != textureToAssetsMap.end())
      {
        ImGui::BeginChild("TextureUsageList", ImVec2(0, 0), ImGuiConstants::NO_BORDER);

        ProfilerString label;
        for (const auto &assetName : iterator->second)
        {
          label.clear();
          lp_append_name_count(label, assetName.c_str(), riModule->getRiInstanceCount(assetName));

          bool isSelected = (selectedAssetName == assetName);
          if (ImGui::Selectable(label.c_str(), isSelected))
            selectedAssetName = assetName;
        }

        ImGui::EndChild();
      }
      else
        ImGui::TextDisabled("No texture usage data available");
    }
  }
  ImGui::EndChild();
}

CopyResult TextureProfilerUI::handleGlobalCopy() const
{
  // Priority 1: Selected asset name
  if (!selectedAssetName.empty())
    return CopyResult(selectedAssetName, "Asset name copied");

  // Priority 2: Selected texture name
  if (textureTable)
  {
    const ProfilerString &selectedTexture = textureTable->getSelectedName();
    if (!selectedTexture.empty())
      return CopyResult(selectedTexture, "Texture name copied");
  }

  return CopyResult(); // Nothing to copy
}

CopyResult TextureProfilerUI::handleContextCopy(const CopyRequest &request) const
{
  if (request.customData.empty())
    return CopyResult();

  ProfilerString notificationMsg;
  switch (request.type)
  {
    case CopyType::CONTEXT_CELL: notificationMsg = "Cell copied"; break;
    case CopyType::CONTEXT_ROW: notificationMsg = "Row copied"; break;
    case CopyType::CONTEXT_CUSTOM: notificationMsg = "Info copied"; break;
    default: notificationMsg = "Copied to clipboard";
  }

  return CopyResult(request.customData, notificationMsg);
}

// --- RIProfilerUI ---

RIProfilerUI::RIProfilerUI(RIModule *ri_module, GlobalCopyManager &copy_manager) : riModule(ri_module)
{
  riTable = eastl::make_unique<LpRiTable>(ri_module);
  riTable->setCopyManager(&copy_manager);
}

RIProfilerUI::~RIProfilerUI() {}

void RIProfilerUI::drawUI()
{
  float tableRegionHeight = ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 2.5f;

  ImGui::BeginChild("RiTableRegion", ImVec2(0.0f, tableRegionHeight), ImGuiConstants::NO_BORDER);

  bool hasModule = riTable && riModule;

  if (hasModule)
  {
    const auto &riData = riModule->getRiData();
    if (!riData.empty())
    {
      if (riModule->hasProvisionalRiData())
      {
        if (riModule->isCollecting())
          ImGui::TextDisabled("Collecting RI data... values update live.");
        else if (riModule->isPaused())
          ImGui::TextDisabled("Collection paused. Rendinst counts are incomplete.");
        else if (riModule->wasCollectCancelled())
          ImGui::TextDisabled("Collection stopped: the riGen layers were unloaded or reloaded. Counts are incomplete.");
        else
          ImGui::TextDisabled("RI data not finalized. Values remain provisional.");
        ImGui::Spacing();
      }
      riTable->draw();
    }
    else
    {
      if (riModule->isCollecting())
        ImGui::Text("Collecting RI data... preparing table.");
      else
        ImGui::Text("No RI assets found. Please collect data first.");
    }
  }
  else
  {
    ImGui::Text("No RI module available");
  }

  ImGui::EndChild();

  ImGui::Separator();

  if (hasModule && !riModule->getRiData().empty())
  {
    const LpColumnSum instances = riTable->sumColumn(*RiColumn::COUNT_ON_MAP);
    lp_draw_row_count("RI Assets", riTable->getVisibleRowCount(), riTable->getRowCount());
    lp_draw_column_sum("Instances", instances, lp_format_count_sum);
  }
}

CopyResult RIProfilerUI::handleGlobalCopy() const
{
  if (riTable)
  {
    const ProfilerString &selectedRi = riTable->getSelectedName();
    if (!selectedRi.empty())
      return CopyResult(selectedRi, "RI asset name copied");
  }

  return CopyResult();
}

CopyResult RIProfilerUI::handleContextCopy(const CopyRequest &request) const
{
  if (request.customData.empty())
    return CopyResult();

  ProfilerString notificationMsg;
  switch (request.type)
  {
    case CopyType::CONTEXT_CELL: notificationMsg = "Cell copied"; break;
    case CopyType::CONTEXT_ROW: notificationMsg = "Row copied"; break;
    case CopyType::CONTEXT_CUSTOM: notificationMsg = "Info copied"; break;
    default: notificationMsg = "Copied to clipboard";
  }

  return CopyResult(request.customData, notificationMsg);
}

// --- LevelProfilerUI ---

LevelProfilerUI::LevelProfilerUI()
{
  textureModule = eastl::make_unique<TextureModule>();
  riModule = eastl::make_unique<RIModule>();

  textureProfilerUI = eastl::make_unique<TextureProfilerUI>(textureModule.get(), riModule.get(), globalCopyManager);
  riProfilerUI = eastl::make_unique<RIProfilerUI>(riModule.get(), globalCopyManager);

  // RI first: the texture usage view is built from RI instance counts.
  registerDataModule(riModule.get());
  registerDataModule(textureModule.get());

  globalCopyManager.setActiveProvider(textureProfilerUI.get());

  addTab("Texture pool", textureProfilerUI.get());
  addTab("Lods statistic", riProfilerUI.get());
}

void LevelProfilerUI::init()
{
  for (DataModuleEntry &entry : dataModules)
    entry.module->init();

  for (auto &tab : tabs)
    tab.module->init();
}

void LevelProfilerUI::shutdown()
{
  for (auto &tab : tabs)
    tab.module->shutdown();

  for (DataModuleEntry &entry : dataModules)
    entry.module->shutdown();
}

void LevelProfilerUI::drawUI()
{
  ImGui::Begin("Level Profiler");

  globalCopyManager.update();

  for (DataModuleEntry &entry : dataModules)
    entry.collector->continueCollect();

  const bool collectBusy = isAnyBusy();
  if (collectWasBusy && !collectBusy)
    notifyTabsDataCollected();
  collectWasBusy = collectBusy;

  toast.update();

  const bool collecting = isAnyCollecting();
  const char *collectButtonLabel = "Collect Data";
  if (collecting)
    collectButtonLabel = "Pause Collecting";
  else if (isAnyPaused())
    collectButtonLabel = "Resume Collecting";

  bool applyPauseStyle = collecting;
  if (applyPauseStyle)
  {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.14f, 0.11f, 0.02f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.96f, 0.82f, 0.24f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.88f, 0.34f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.90f, 0.75f, 0.22f, 1.0f));
  }

  if (ImGui::Button(collectButtonLabel))
  {
    if (collecting)
    {
      for (DataModuleEntry &entry : dataModules)
        entry.collector->pauseCollection();
    }
    else if (isAnyPaused())
    {
      for (DataModuleEntry &entry : dataModules)
        entry.collector->resumeCollection();
    }
    else
      collectData();
  }

  if (applyPauseStyle)
    ImGui::PopStyleColor(4);

  ImGui::SameLine();

  exporter.drawExportButton(tabs[currentTabIndex]);
  exporter.drawExportMenu();

  ImGui::Separator();

  float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() * 2.5f;
  ImVec2 contentAvail = ImGui::GetContentRegionAvail();
  float tabAreaHeight = eastl::max(0.0f, contentAvail.y - statusBarHeight);

  ImGui::BeginChild("TabArea", ImVec2(0.0f, tabAreaHeight), ImGuiConstants::NO_BORDER);
  drawTabBar();
  ImGui::EndChild();

  drawRenamePopup();

  ImGui::Separator();

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.12f, 0.9f));
  ImGui::BeginChild("StatusBar", ImVec2(0.0f, statusBarHeight), ImGuiConstants::WITH_BORDER,
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  drawStatusBar();
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::End();

  toast.draw();
}

bool LevelProfilerUI::isAnyCollecting() const
{
  for (const DataModuleEntry &entry : dataModules)
    if (entry.collector->isCollecting())
      return true;
  return false;
}

float LevelProfilerUI::collectProgress() const
{
  // Only the collectors that are still working: a synchronous one reports 1.0 and would otherwise
  // drag the bar to the middle of its range for the whole walk.
  float sum = 0.0f;
  int count = 0;
  for (const DataModuleEntry &entry : dataModules)
    if (entry.collector->isCollecting() || entry.collector->isPaused())
    {
      sum += entry.collector->getCollectProgress();
      count++;
    }
  return count ? sum / count : 1.0f;
}

bool LevelProfilerUI::isAnyPaused() const
{
  for (const DataModuleEntry &entry : dataModules)
    if (entry.collector->isPaused())
      return true;
  return false;
}

void LevelProfilerUI::collectData()
{
  // The tabs read one level snapshot and depend on each other's data, so collection is one action
  // over every collector rather than a per-tab one. An incremental collector keeps working after
  // its collect() returns and is pumped from drawUI. If one collector cannot read its source now,
  // every tab keeps the previous snapshot.
  for (const DataModuleEntry &entry : dataModules)
    if (!entry.collector->canCollect())
    {
      logwarn("levelProfiler: a data source is not ready (riGen load), the collect is skipped");
      return;
    }

  for (DataModuleEntry &entry : dataModules)
    entry.collector->collect();

  collectWasBusy = isAnyBusy();
  notifyTabsDataCollected();
}

void LevelProfilerUI::notifyTabsDataCollected()
{
  for (auto &tab : tabs)
    tab.module->onDataCollected();
}

void LevelProfilerUI::addTab(const char *name, IProfilerModule *module_ptr) { tabs.push_back(ProfilerTab(name, module_ptr)); }

void LevelProfilerUI::selectTexture(const char *texture_name) { textureProfilerUI->selectTexture(texture_name); }

void LevelProfilerUI::renameTab(int index, const char *new_name)
{
  if (index >= 0 && index < static_cast<int>(tabs.size()))
    tabs[index].name = new_name;
}

void LevelProfilerUI::drawTabBar()
{
  if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_Reorderable))
  {
    for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
    {
      ImGui::PushID(i);

      bool isTabVisible = ImGui::BeginTabItem(tabs[i].name.c_str());

      // Handle right-click for renaming
      bool isTabHovered = ImGui::IsItemHovered();
      bool isRightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

      if (isTabVisible)
      {
        if (currentTabIndex != i)
        {
          currentTabIndex = i;
          if (ICopyProvider *provider = tabs[i].module->getCopyProvider())
            globalCopyManager.setActiveProvider(provider);
        }

        tabs[i].module->drawUI();
        ImGui::EndTabItem();
      }

      // Apply right-click logic after tab content is processed
      if (isTabHovered && isRightClicked)
      {
        renameTabIndex = i;
        renameBuffer = tabs[i].name;
        isOpenRenamePopup = true; // Signal to open the rename popup.
      }

      ImGui::PopID();
    }
    ImGui::EndTabBar();
  }
}

void LevelProfilerUI::drawRenamePopup()
{
  // Open popup if requested
  if (isOpenRenamePopup)
  {
    ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
    ImGui::OpenPopup("Rename Tab");
    isOpenRenamePopup = false; // Reset flag after opening.
  }

  if (ImGui::BeginPopupModal("Rename Tab", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
  {
    renameBuffer.resize(64);

    if (ImGui::InputText("New name", renameBuffer.data(), renameBuffer.capacity()))
      renameBuffer.resize(strlen(renameBuffer.c_str()));

    bool enterPressed = ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
    bool escapePressed = ImGui::IsKeyPressed(ImGuiKey_Escape);

    if (ImGui::Button("OK") || enterPressed)
    {
      renameTab(renameTabIndex, renameBuffer.c_str());
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel") || escapePressed)
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
}

void LevelProfilerUI::drawStatusBar()
{
  if (!riModule)
  {
    ImGui::TextUnformatted("RI module unavailable.");
    return;
  }

  if (isAnyCollecting())
  {
    // The status bar child has no scrollbar, so the message keeps its own line: chaining the
    // indicators after it with SameLine pushes them past the clip rect in a narrow window.
    ImGui::TextUnformatted("Counting rendinst objects on the level (table updates live). Closing the window pauses collection.");

    float frameHeight = ImGui::GetFrameHeight();

    ImGui::ProgressBar(collectProgress(), ImVec2(160.0f, frameHeight));

    // The detail below is riGen specific. A second incremental collector would need this line
    // split per collector, or reduced to the combined progress bar above.
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Layers %d/%d", riModule->getCollectCompletedLayers(), riModule->getCollectLayerCount());

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Cells %u/%u", static_cast<unsigned>(riModule->getCollectProcessedCells()),
      static_cast<unsigned>(riModule->getCollectTotalCells()));

    return;
  }

  if (!riModule->canCollect())
  {
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.82f, 0.24f, 1.0f));
    ImGui::TextUnformatted("riGen is loading: Collect Data is skipped until the load ends.");
    ImGui::PopStyleColor();

    return;
  }

  if (riModule->wasCollectCancelled())
  {
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.82f, 0.24f, 1.0f));
    ImGui::TextUnformatted("RI collection stopped: the riGen layers were unloaded or reloaded. Press Collect Data to start over.");
    ImGui::PopStyleColor();

    return;
  }

  if (riModule->hasProvisionalRiData())
  {
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.82f, 0.24f, 1.0f));
    ImGui::TextUnformatted("RI data incomplete. Rendinst counts may differ. Resume or recollect for accurate totals.");
    ImGui::PopStyleColor();

    return;
  }

  if (riModule->getRiData().empty())
  {
    ImGui::TextUnformatted("Press Collect Data to begin.");
    return;
  }

  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("Data collected.");
}

} // namespace levelprofiler