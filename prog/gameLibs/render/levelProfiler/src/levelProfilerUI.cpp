// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <cstring>
#include "levelProfilerUI.h"
#include "riModule.h"

static_assert(levelprofiler::ImGuiConstants::WITH_BORDER == true, "WITH_BORDER constant must match ImGui boolean true value");
static_assert(levelprofiler::ImGuiConstants::NO_BORDER == false, "NO_BORDER constant must match ImGui boolean false value");

namespace levelprofiler
{

// --- TextureProfilerUI ---

TextureProfilerUI::TextureProfilerUI(TextureModule *texture_module, RIModule *ri_module) :
  textureModule(texture_module), riModule(ri_module), exporter(texture_module)
{
  // Connect RIModule to filters for texture usage filtering
  filterManager.setRIModule(ri_module);

  textureTable = eastl::make_unique<LpTextureTable>(this);
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
  static float leftPanelRatio = 0.6f; // 60% for left panel by default
  float leftPanelWidth = availableArea.x * leftPanelRatio;

  // Left panel: table and filters
  ImGui::BeginChild("PoolLeft", ImVec2(leftPanelWidth, 0), ImGuiConstants::WITH_BORDER);
  drawLeftPanel();
  ImGui::EndChild();

  ImGui::SameLine();

  // InvisibleButton as a splitter handle
  ImGui::InvisibleButton("##HSplitter", ImVec2(4.0f, eastl::max(availableArea.y, 1.0f)));

  // Handle splitter dragging
  if (ImGui::IsItemActive())
  {
    leftPanelRatio += ImGui::GetIO().MouseDelta.x / availableArea.x;
    leftPanelRatio = eastl::clamp(leftPanelRatio, 0.2f, 0.8f); // Constrain to reasonable range
  }

  // Draw visual splitter line
  {
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImU32 col = ImGui::GetColorU32(ImGuiCol_Separator);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p0.x + 2.0f, p0.y), ImVec2(p0.x + 2.0f, p1.y), col, 1.0f);
  }

  // Right panel: preview and Usage list
  ImGui::SameLine();
  ImGui::BeginChild("PoolRight", ImVec2(0, 0), ImGuiConstants::WITH_BORDER);
  drawRightPanel();
  ImGui::EndChild();
}

void TextureProfilerUI::drawLeftPanel()
{
  // If the data is collected, we only show progress for the entire panel
  if (textureModule->isCollecting())
  {
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    float centerY = availableSize.y / 2.0f - ImGui::GetTextLineHeightWithSpacing() * 2;

    ImGui::Dummy(ImVec2(0, centerY));

    // Title
    ImGui::SetCursorPosX((availableSize.x - ImGui::CalcTextSize("Collecting texture data...").x) / 2.0f);
    ImGui::Text("Collecting texture data...");

    // progress have
    ImGui::Spacing();
    float progress = textureModule->getCollectionProgress();
    ImGui::ProgressBar(progress, ImVec2(-1, 0));

    // Information about the current texture
    if (const ProfilerString &currentTexture = textureModule->getCurrentCollectionTexture(); !currentTexture.empty())
    {
      ImGui::Spacing();
      char statusText[256];
      snprintf(statusText, sizeof(statusText), "Processing: %s", currentTexture.c_str());
      ImGui::SetCursorPosX((availableSize.x - ImGui::CalcTextSize(statusText).x) / 2.0f);
      ImGui::Text("%s", statusText);
    }
  }
  else
  {
    // Default mode -table and statistics
    // Reserve space at bottom for statistics
    float tableRegionHeight = ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 2.5f;

    ImGui::BeginChild("TableRegion", ImVec2(0, tableRegionHeight), ImGuiConstants::NO_BORDER);
    if (textureTable)
    {
      // Sort table if needed
      if (textureTable->isSorted())
        textureTable->sortFilteredTextures();

      textureTable->draw();
    }
    ImGui::EndChild();

    ImGui::Separator();

    // Display statistics
    ImGui::Text("Textures: %u / %u", textureModule->getFilteredTextureCount(), textureModule->getTotalTextureCount());
    ImGui::Text("Memory: %.2f / %.2f MB", textureModule->getFilteredTotalMemorySize(), textureModule->getTotalMemorySize());
  }
}

void TextureProfilerUI::drawRightPanel()
{
  // Calculate available area and divide vertically
  ImVec2 availablePanelArea = ImGui::GetContentRegionAvail();
  static float previewPaneRatio = 0.7f; // 70% for preview by default
  float previewPaneHeight = availablePanelArea.y * previewPaneRatio;

  // Preview pane (top)
  ImGui::BeginChild("PreviewPane", ImVec2(0, previewPaneHeight), ImGuiConstants::NO_BORDER);
  {
    const auto &filteredTextureNames = textureModule->getFilteredTextures();
    ProfilerString selectedTextureName = textureTable->getSelectedTexture();

    // If the data collection is ongoing, SelectededTexturename will already be installed through Selecttexture
    if (selectedTextureName.empty() && !filteredTextureNames.empty() && !textureModule->isCollecting())
    {
      // Auto-select first texture if none selected (only when the data is not collected)
      selectedTextureName = filteredTextureNames[0];
      textureTable->setSelectedTexture(selectedTextureName);
    }

    if (!selectedTextureName.empty())
    {
      if (textureModule->isCollecting())
        ImGui::Text("Collecting data - Processing texture:");

      textureModule->drawTextureView(selectedTextureName.c_str());
    }
  }
  ImGui::EndChild();

  // Horizontal splitter
  ImGui::InvisibleButton("##Splitter", ImVec2(eastl::max(availablePanelArea.x, 1.0f), 4.0f));
  ImGui::GetWindowDrawList()->AddLine(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);

  // Handle splitter dragging
  if (ImGui::IsItemActive() && ImGui::GetIO().MouseDelta.y != 0.0f)
  {
    previewPaneRatio += ImGui::GetIO().MouseDelta.y / availablePanelArea.y;
    previewPaneRatio = eastl::clamp(previewPaneRatio, 0.2f, 0.8f); // Constrain to reasonable range
  }

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

        for (const auto &assetName : iterator->second)
          if (bool isSelected = (selectedAssetName == assetName); ImGui::Selectable(assetName.c_str(), isSelected))
            selectedAssetName = assetName;

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

RIProfilerUI::RIProfilerUI(RIModule *ri_module) : riModule(ri_module) {}

RIProfilerUI::~RIProfilerUI() {}

void RIProfilerUI::init() {}

void RIProfilerUI::shutdown() {}

void RIProfilerUI::drawUI()
{
  // Just a placeholder for future implementation
  ImGui::Text("LOD statistics not yet implemented.");
}

CopyResult RIProfilerUI::handleGlobalCopy() const
{
  // No copy functionality implemented yet for RI tab
  return CopyResult();
}

CopyResult RIProfilerUI::handleContextCopy(const CopyRequest & /* request */) const
{
  // No context copy functionality implemented yet for RI tab
  return CopyResult();
}

eastl::vector<ProfilerString> RIProfilerUI::getContextMenuItems() const
{
  // No context menu items for RI tab yet
  return {};
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
  globalCopyManager.setNotificationHandler(toastAdapter.get());

  globalCopyManager.registerProvider(textureProfilerUI.get(), "Texture pool");
  globalCopyManager.registerProvider(riProfilerUI.get(), "Lods statistic");

  addTab("Texture pool", textureProfilerUI.get());
  addTab("Lods statistic", riProfilerUI.get());
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

  // Update the data collection process if it is active
  if (textureModule->isCollecting())
    textureModule->updateDataCollection();

  if (ImGui::Button("Collect Data"))
    collectData();

  ImGui::SameLine();

  // Export button - handled by TextureProfilerUI
  if (currentTabIndex == 0 && !tabs.empty())
  {
    TextureProfilerUI *currentTextureUI = static_cast<TextureProfilerUI *>(tabs[0].module);
    currentTextureUI->getExporter().drawExportButton();
    currentTextureUI->getExporter().drawExportMenu();
  }

  ImGui::Separator();

  drawTabBar();
  drawRenamePopup();

  ImGui::End();
}

void LevelProfilerUI::collectData()
{
  textureModule->clear();
  riModule->clear();

  textureModule->collect();
  riModule->collect();

  textureModule->initializeFilteredTextures();

  if (!tabs.empty() && currentTabIndex == 0)
  {
    if (TextureProfilerUI *currentTextureUI = static_cast<TextureProfilerUI *>(tabs[0].module))
    {
      currentTextureUI->getFilterManager().resetAllFilters();
      currentTextureUI->getFilterManager().applyFilters();
    }
  }

  for (auto &tab : tabs)
    tab.module->init();

  textureModule->startDataCollection();
}

void LevelProfilerUI::clearData()
{
  textureModule->clear();
  riModule->clear();
}

void LevelProfilerUI::addTab(const char *name, IProfilerModule *module_ptr) { tabs.push_back(ProfilerTab(name, module_ptr)); }

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

      // Set selected tab flag
      ImGuiTabItemFlags tabDisplayFlags = (currentTabIndex == i ? ImGuiTabItemFlags_SetSelected : 0);

      bool isTabVisible = ImGui::BeginTabItem(tabs[i].name.c_str(), nullptr, tabDisplayFlags);

      // Handle right-click for renaming
      bool isTabHovered = ImGui::IsItemHovered();
      bool isRightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

      if (isTabVisible)
      {
        currentTabIndex = i;

        if (ICopyProvider *provider = tabs[i].module->getCopyProvider())
          globalCopyManager.setActiveProvider(provider);

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