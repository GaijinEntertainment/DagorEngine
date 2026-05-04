// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <cstring>
#include "levelProfilerUI.h"
#include "levelProfilerRiTable.h"
#include "riModule.h"

static_assert(levelprofiler::ImGuiConstants::WITH_BORDER == true, "WITH_BORDER constant must match ImGui boolean true value");
static_assert(levelprofiler::ImGuiConstants::NO_BORDER == false, "NO_BORDER constant must match ImGui boolean false value");

namespace levelprofiler
{

// --- TextureProfilerUI ---

TextureProfilerUI::TextureProfilerUI(TextureModule *texture_module, RIModule *ri_module) :
  textureModule(texture_module),
  riModule(ri_module),
  exporter(texture_module),
  horizontalSplitter(LpSplitterDirection::HORIZONTAL, 0.6f),
  verticalSplitter(LpSplitterDirection::VERTICAL, 0.7f)
{
  // Connect RIModule to filters for texture usage filtering
  filterManager.setRIModule(ri_module);

  textureTable = eastl::make_unique<LpTextureTable>(this);
  exporter.setTextureTable(textureTable.get());
}

TextureProfilerUI::~TextureProfilerUI() {}

void TextureProfilerUI::init()
{
  // Reset filters if data exists, otherwise wait for data collection
  if (textureModule->getTotalTextureCount() > 0)
  {
    filterManager.resetAllFilters();
    filterManager.applyFilters();
  }

  if (textureTable)
    textureTable->setCopyManager(getCopyManager());
}

void TextureProfilerUI::shutdown() {}

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
  {
    if (textureTable->isSortActive())
      textureTable->sortFilteredTextures();

    textureTable->draw();
  }
  ImGui::EndChild();

  ImGui::Separator();

  // Display statistics
  ImGui::Text("Textures: %u / %u", textureModule->getFilteredTextureCount(), textureModule->getTotalTextureCount());
  ImGui::Text("Memory: %.2f / %.2f MB", textureModule->getFilteredTotalMemorySize(), textureModule->getTotalMemorySize());
}

void TextureProfilerUI::drawRightPanel()
{
  // Calculate available area and divide vertically
  ImVec2 availablePanelArea = ImGui::GetContentRegionAvail();
  float previewPaneHeight = availablePanelArea.y * verticalSplitter.getRatio();

  // Preview pane (top)
  ImGui::BeginChild("PreviewPane", ImVec2(0, previewPaneHeight), ImGuiConstants::NO_BORDER);
  {
    const auto &filteredTextureNames = textureModule->getFilteredTextures();
    ProfilerString selectedTextureName = textureTable->getSelectedTexture();

    if (selectedTextureName.empty() && !filteredTextureNames.empty())
    {
      selectedTextureName = filteredTextureNames[0];
      textureTable->setSelectedTexture(selectedTextureName);
    }

    if (!selectedTextureName.empty())
      textureModule->drawTextureView(selectedTextureName.c_str());
  }
  ImGui::EndChild();

  verticalSplitter.draw(availablePanelArea);

  // Texture Usage pane (bottom)
  ImGui::BeginChild("TextureUsagePane", ImVec2(0, 0), ImGuiConstants::WITH_BORDER);
  {
    ImGui::TextUnformatted("Texture Usage Objects:");

    const ProfilerString currentSelectedTextureName = textureTable->getSelectedTexture();

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

        const auto &riCounts = riModule->getRiInstanceCounts();
        for (const auto &assetName : iterator->second)
        {
          int count = 0;
          if (auto itCnt = riCounts.find(assetName); itCnt != riCounts.end())
            count = itCnt->second;
          char displayBuf[256];
          if (count > 0)
            snprintf(displayBuf, sizeof(displayBuf), "%s (%d)", assetName.c_str(), count);
          else
            snprintf(displayBuf, sizeof(displayBuf), "%s", assetName.c_str());

          bool isSelected = (selectedAssetName == assetName);
          if (ImGui::Selectable(displayBuf, isSelected))
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
    const ProfilerString &selectedTexture = textureTable->getSelectedTexture();
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

eastl::vector<ProfilerString> TextureProfilerUI::getContextMenuItems() const { return {}; }

GlobalCopyManager *TextureProfilerUI::getCopyManager() const
{
  auto levelProfiler = static_cast<LevelProfilerUI *>(ILevelProfiler::getInstance());
  return levelProfiler->getCopyManager();
}

ProfilerString TextureProfilerUI::generateFullTextureInfo(const ProfilerString &texture_name) const
{
  const auto &textures = textureModule->getTextures();
  auto it = textures.find(texture_name);
  if (it == textures.end())
    return ProfilerString{};

  const TextureData &textureData = it->second;

  ProfilerString info;
  info += "Texture: " + texture_name + "\n";
  info += "Format: " + ProfilerString(TextureModule::getFormatName(textureData.info.cflg)) + "\n";
  info += "Dimensions: " + eastl::to_string(textureData.info.w) + "x" + eastl::to_string(textureData.info.h) + "\n";
  info += "Mip levels: " + eastl::to_string(textureData.info.mipLevels) + "\n";

  float sizeMB = textureModule->getTextureMemorySize(textureData);
  info += "Memory size: " + eastl::to_string(sizeMB) + " MB\n";

  const auto &textureUsageMap = riModule->getTextureUsage();
  if (auto usageIt = textureUsageMap.find(texture_name); usageIt != textureUsageMap.end())
  {
    const TextureUsage &usage = usageIt->second;
    if (usage.unique == 0)
      info += "Usage: Non-RI";
    else if (usage.unique == 1)
      info += "Usage: Unique";
    else
      info += "Usage: Shared(" + eastl::to_string(usage.unique) + ")";
  }
  else
    info += "Usage: Non-RI";

  return info;
}


// --- RIProfilerUI ---

RIProfilerUI::RIProfilerUI(RIModule *ri_module) : riModule(ri_module) { riTable = eastl::make_unique<LpRiTable>(ri_module); }

RIProfilerUI::~RIProfilerUI() {}

void RIProfilerUI::init()
{
  if (riTable)
    riTable->setCopyManager(getCopyManager());
}

void RIProfilerUI::shutdown() {}

void RIProfilerUI::drawUI()
{
  float tableRegionHeight = ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 2.5f;

  ImGui::BeginChild("RiTableRegion", ImVec2(0, tableRegionHeight), ImGuiConstants::NO_BORDER);

  if (riTable && riModule)
  {
    if (const auto &riData = riModule->getRiData(); !riData.empty())
      riTable->draw();
    else
      ImGui::Text("No RI assets found. Please collect data first.");
  }
  else
    ImGui::Text("No RI module available");

  ImGui::EndChild();

  ImGui::Separator();

  if (riTable)
    ImGui::Text("RI Assets: %u / %u", (unsigned)riTable->getFilteredRiCount(), (unsigned)riTable->getTotalRiCount());
}

void RIProfilerUI::collectAndDisplayData()
{
  if (!riModule)
    return;

  riModule->collect();
}

void RIProfilerUI::updateFilter() {}

CopyResult RIProfilerUI::handleGlobalCopy() const
{
  if (riTable)
  {
    const ProfilerString &selectedRi = riTable->getSelectedRi();
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

eastl::vector<ProfilerString> RIProfilerUI::getContextMenuItems() const { return {}; }

GlobalCopyManager *RIProfilerUI::getCopyManager() const
{
  auto levelProfiler = static_cast<LevelProfilerUI *>(ILevelProfiler::getInstance());
  return levelProfiler->getCopyManager();
}

ProfilerString RIProfilerUI::generateFullRiInfo(const ProfilerString &ri_name) const
{
  const RiData *riDataItem = riModule->getRiDataByName(ri_name);
  if (!riDataItem)
    return ProfilerString{};

  ProfilerString info;
  info += "RI Asset: " + ri_name + "\n";
  info += "Count on map: " + eastl::to_string(riDataItem->countOnMap) + "\n";
  info += "BSphere radius: " + eastl::to_string(riDataItem->bSphereRadius) + "\n";
  info += "BBox radius: " + eastl::to_string(riDataItem->bBoxRadius) + "\n";

  for (size_t i = 0; i < riDataItem->lods.size(); ++i)
  {
    const auto &lod = riDataItem->lods[i];
    info += "LOD" + eastl::to_string(i) + " - Dips: " + eastl::to_string(lod.drawCalls);
    info += ", Tris: " + eastl::to_string(lod.totalFaces);
    info += ", Dist: " + eastl::to_string(lod.lodDistance);
    info += ", Screen%: " + eastl::to_string(lod.screenPercent) + "\n";
  }

  if (riDataItem->collision.physTriangles > 0 || riDataItem->collision.traceTriangles > 0)
  {
    info += "Collision - Phys: " + eastl::to_string(riDataItem->collision.physTriangles);
    info += ", Trace: " + eastl::to_string(riDataItem->collision.traceTriangles) + "\n";
  }

  return info;
}

// --- LevelProfilerUI ---

LevelProfilerUI::LevelProfilerUI()
{
  textureModule = eastl::make_unique<TextureModule>();
  riModule = eastl::make_unique<RIModule>();

  textureProfilerUI = eastl::make_unique<TextureProfilerUI>(textureModule.get(), riModule.get());
  riProfilerUI = eastl::make_unique<RIProfilerUI>(riModule.get());
}


void LevelProfilerUI::initialize()
{
  toastAdapter = eastl::make_unique<ToastNotificationAdapter>(&textureProfilerUI->getExporter().getToast());
  textureProfilerUI->getExporter().setNotificationHandler(toastAdapter.get());
  globalCopyManager.setNotificationHandler(toastAdapter.get());

  globalCopyManager.registerProvider(textureProfilerUI.get(), "Texture pool");
  globalCopyManager.registerProvider(riProfilerUI.get(), "Lods statistic");

  addTab("Texture pool", textureProfilerUI.get(), [](LevelProfilerUI *self) { self->collectTextureData(); });
  addTab("Lods statistic", riProfilerUI.get(), [](LevelProfilerUI *self) { self->collectRiData(); });
}

LevelProfilerUI::~LevelProfilerUI()
{
  globalCopyManager.unregisterProvider(textureProfilerUI.get());
  globalCopyManager.unregisterProvider(riProfilerUI.get());
  tabs.clear();
}

void LevelProfilerUI::init()
{
  textureModule->init();
  riModule->init();

  for (auto &tab : tabs)
    tab.module->init();
}

void LevelProfilerUI::shutdown()
{
  // Shutdown UI modules (in reverse order)
  for (auto &tab : tabs)
    tab.module->shutdown();

  textureModule->shutdown();
  riModule->shutdown();
}

void LevelProfilerUI::drawUI()
{
  ImGui::Begin("Level Profiler");

  globalCopyManager.update();

  if (toastAdapter)
    toastAdapter->update();

  if (ImGui::Button("Collect Data"))
    collectData();

  ImGui::SameLine();

  // Export button (shared exporter currently owned by texture tab UI)
  if (!tabs.empty())
  {
    TextureProfilerUI *texUI = static_cast<TextureProfilerUI *>(tabs[0].module);
    if (currentTabIndex == 0)
    {
      texUI->getExporter().setFilenameBase(tabs[0].name);
      texUI->getExporter().setTextureTable(texUI->getTextureTable());
      texUI->getExporter().setRiTable(nullptr);
      texUI->getExporter().drawExportButton();
      texUI->getExporter().drawExportMenu();
    }
    else if (currentTabIndex == 1 && riProfilerUI)
    {
      texUI->getExporter().setFilenameBase(tabs[1].name);
      texUI->getExporter().setTextureTable(nullptr);
      if (LpRiTable *riTablePtr = riProfilerUI->getRiTable())
        texUI->getExporter().setRiTable(riTablePtr);
      texUI->getExporter().drawExportButton();
      texUI->getExporter().drawExportMenu();
    }
  }

  ImGui::Separator();

  drawTabBar();
  drawRenamePopup();

  ImGui::End();

  if (toastAdapter)
    toastAdapter->draw();
}

void LevelProfilerUI::collectData()
{
  if (currentTabIndex < 0 || currentTabIndex >= (int)tabs.size())
    return;

  ProfilerTab &tab = tabs[currentTabIndex];
  if (tab.collectFn)
    tab.collectFn(this);
}

void LevelProfilerUI::collectTextureData()
{
  if (!textureModule)
    return;

  if (riModule)
  {
    riModule->clear();
    riModule->collect();
    if (riProfilerUI)
      riProfilerUI->init();
  }

  textureModule->clear();
  textureModule->collect();
  textureModule->initializeFilteredTextures();

  for (auto &tab : tabs)
    if (tab.module == textureProfilerUI.get())
      tab.module->init();
}

void LevelProfilerUI::collectRiData()
{
  if (!riModule)
    return;

  riModule->clear();
  riModule->collect();

  if (riProfilerUI)
    riProfilerUI->init();
}

void LevelProfilerUI::clearData()
{
  textureModule->clear();
  riModule->clear();
}

void LevelProfilerUI::addTab(const char *name, IProfilerModule *module_ptr, void (*collectFn)(LevelProfilerUI *))
{
  tabs.push_back(ProfilerTab(name, module_ptr, collectFn));
}

ProfilerTab *LevelProfilerUI::getTab(int index)
{
  if (index >= 0 && index < static_cast<int>(tabs.size()))
    return &tabs[index];

  return nullptr;
}

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

} // namespace levelprofiler