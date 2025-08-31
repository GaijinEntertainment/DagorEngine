// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fstream>
#include <sstream>
#include <iomanip>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>
#include "levelProfilerUI.h"
#include "riModule.h"
#include "levelProfilerExporter.h"

namespace levelprofiler
{

// UI constants for toast notifications
static constexpr float TOAST_PADDING_X = 10.0f;
static constexpr float TOAST_PADDING_Y = 6.0f;
static constexpr float TOAST_CORNER_RADIUS = 4.0f;
static constexpr int TOAST_BG_COLOR = IM_COL32(0, 0, 0, 200);
static constexpr int TOAST_TEXT_COLOR = IM_COL32(255, 255, 255, 255);

// Buffer sizes
static constexpr int FILENAME_BUFFER_SIZE = 128;

// Tab indices
static constexpr int TEXTURE_TAB_INDEX = 0;
static constexpr int RI_TAB_INDEX = 1;

// Export formatting constants
static constexpr int DECIMAL_PRECISION = 2;
static constexpr int STYLE_VAR_COUNT = 2; // Number of style vars to pop

// --- ToastNotification implementation ---

void ToastNotification::show(const char *message, float duration_seconds)
{
  this->msg = message;
  timer = duration_seconds;
  isVisible = true;
}

void ToastNotification::update()
{
  if (isVisible)
    if ((timer -= ImGui::GetIO().DeltaTime) <= 0.0f)
      isVisible = false;
}

void ToastNotification::draw()
{
  if (!isVisible)
    return;

  // Position toast near mouse for better visibility
  ImVec2 pos = ImGui::GetMousePos();
  ImDrawList *drawList = ImGui::GetForegroundDrawList();
  // Calculate background rectangle size
  ImVec2 textSize = ImGui::CalcTextSize(msg.c_str());
  ImVec2 padding(TOAST_PADDING_X, TOAST_PADDING_Y);
  ImVec2 bgMin = pos;
  ImVec2 bgMax(pos.x + textSize.x + padding.x * 2, pos.y + textSize.y + padding.y * 2);

  // Draw toast background with semi-transparent black
  drawList->AddRectFilled(bgMin, bgMax, TOAST_BG_COLOR, TOAST_CORNER_RADIUS);

  // Draw toast message text
  drawList->AddText(ImVec2(pos.x + padding.x, pos.y + padding.y), TOAST_TEXT_COLOR, msg.c_str());
}

// --- GlobalCopyManager implementation ---

void GlobalCopyManager::setNotificationHandler(INotificationHandler *handler) { notificationHandler = handler; }

void GlobalCopyManager::registerProvider(ICopyProvider *provider, const char *tab_name)
{
  if (!provider)
    return;

  unregisterProvider(provider);
  providers.push_back({provider, ProfilerString(tab_name ? tab_name : "")});
}

void GlobalCopyManager::unregisterProvider(ICopyProvider *provider)
{
  if (eastl::erase_if(providers, [provider](const ProviderEntry &entry) { return entry.provider == provider; }) > 0)
  {
    if (activeProvider == provider)
      activeProvider = nullptr;
  }
}

void GlobalCopyManager::setActiveProvider(ICopyProvider *provider) { activeProvider = provider; }

void GlobalCopyManager::update()
{
  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C))
    handleGlobalCopy();
}

bool GlobalCopyManager::executeContextCopy(const CopyRequest &request)
{
  if (!activeProvider)
    return false;

  if (CopyResult result = activeProvider->handleContextCopy(request); result.success)
  {
    executeCopy(result, request);
    activeProvider->onCopyExecuted(request);
    return true;
  }

  return false;
}

eastl::vector<ProfilerString> GlobalCopyManager::getActiveContextMenuItems() const
{
  if (activeProvider)
    return activeProvider->getContextMenuItems();
  return {};
}

void GlobalCopyManager::handleGlobalCopy()
{
  if (activeProvider)
  {
    if (CopyResult result = activeProvider->handleGlobalCopy(); result.success)
      executeCopy(result, CopyRequest(CopyType::GLOBAL_HOTKEY));
  }
}

void GlobalCopyManager::executeCopy(const CopyResult &result, const CopyRequest & /* request */)
{
  ImGui::SetClipboardText(result.text.c_str());

  if (notificationHandler)
    notificationHandler->showCopyNotification(result.notificationMessage.c_str());
}

// --- CSVExporter implementation ---

CSVExporter::CSVExporter(TextureModule *texture_module) : textureModule(texture_module) {}

bool CSVExporter::exportData(ConstProfilerStringSpan items)
{
  if (!textureModule)
    return false;

  // Generate filename based on tab name
  ILevelProfiler *profiler = ILevelProfiler::getInstance();
  if (!profiler)
    return false;

  ProfilerTab *tab0 = profiler->getTab(TEXTURE_TAB_INDEX);
  if (!tab0)
    return false;

  char filename[FILENAME_BUFFER_SIZE];
  snprintf(filename, FILENAME_BUFFER_SIZE, "%s.csv", tab0->name.c_str());

  std::ofstream file(filename);
  if (!file.is_open())
    return false;

  file << "Name,Format,Width,Height,MipLevels,Memory size(MB),Usage\n";

  const auto &textures = textureModule->getTextures();
  const eastl::hash_map<ProfilerString, TextureUsage> *texUsage = nullptr;

  // Try getting texture usage data through proper references
  if (profiler->getTabCount() > RI_TAB_INDEX)
  {
    if (ProfilerTab *riTab = profiler->getTab(RI_TAB_INDEX); riTab && riTab->module)
    {
      // Check if module is RIProfilerUI by name (safer approach)
      if (strstr(riTab->name.c_str(), "Lods") != nullptr)
      {
        RIProfilerUI *riUI = static_cast<RIProfilerUI *>(riTab->module);
        RIModule *riModule = riUI->getRIModule();
        texUsage = &riModule->getTextureUsage();
      }
    }
  }

  // Fallback to direct cast if first method failed
  if (!texUsage && profiler->getTabCount() > RI_TAB_INDEX)
  {
    if (ProfilerTab *tab1 = profiler->getTab(RI_TAB_INDEX); tab1 && tab1->module)
    {
      // Direct cast is potentially unsafe if tab1->module is not actually an RIModule
      RIModule *riModule = static_cast<RIModule *>(tab1->module);
      texUsage = &riModule->getTextureUsage();
    }
  }

  for (size_t i = 0; i < items.size(); ++i)
  {
    const ProfilerString &name = items[i];
    auto it = textures.find(name);
    if (it == textures.end())
      continue;
    const TextureData &textureData = it->second;
    float sizeMB = textureModule->getTextureMemorySize(textureData);
    ProfilerString textureUsageStr = "Non-RI";

    if (texUsage && !texUsage->empty())
    {
      if (auto textureUsageIt = texUsage->find(name); textureUsageIt != texUsage->end())
      {
        if (const TextureUsage &textureUsage = textureUsageIt->second; textureUsage.unique == 1)
          textureUsageStr = "Unique";
        else if (textureUsage.unique >= 2)
          textureUsageStr = ProfilerString("Shared(") + eastl::to_string(textureUsage.unique) + ")";
      }
    }

    file << "\"" << name.c_str() << "\"," << TextureModule::getFormatName(textureData.info.cflg) << "," << textureData.info.w << ","
         << textureData.info.h << "," << textureData.info.mipLevels << "," << std::fixed << std::setprecision(DECIMAL_PRECISION)
         << sizeMB << "," << textureUsageStr.c_str() << "\n";
  }

  return true;
}


// --- MarkdownExporter implementation ---

MarkdownExporter::MarkdownExporter(TextureModule *texture_module) : textureModule(texture_module) {}

bool MarkdownExporter::exportData(ConstProfilerStringSpan items)
{
  if (!textureModule)
    return false;

  std::ostringstream ss;
  ss << "| Name | Format | Width | Height | Mips | Memory size(MB) | Usage |\n"
     << "|------|--------|-------|--------|------|------------------|-------|\n";

  const auto &textures = textureModule->getTextures();
  const eastl::hash_map<ProfilerString, TextureUsage> *texUsage = nullptr;

  // Try to get RIModule through proper reference
  ILevelProfiler *profiler = ILevelProfiler::getInstance();
  if (profiler->getTabCount() > RI_TAB_INDEX)
  {
    if (ProfilerTab *riTab = profiler->getTab(RI_TAB_INDEX); riTab && riTab->module)
    {
      if (strstr(riTab->name.c_str(), "Lods") != nullptr)
      {
        RIProfilerUI *riUI = static_cast<RIProfilerUI *>(riTab->module);
        RIModule *riModule = riUI->getRIModule();
        texUsage = &riModule->getTextureUsage();
      }
    }
  }

  if (!texUsage && profiler->getTabCount() > RI_TAB_INDEX)
  {
    if (ProfilerTab *tab1 = profiler->getTab(RI_TAB_INDEX); tab1 && tab1->module)
    {
      RIModule *riModule = static_cast<RIModule *>(tab1->module);
      texUsage = &riModule->getTextureUsage();
    }
  }

  for (size_t i = 0; i < items.size(); ++i)
  {
    const ProfilerString &name = items[i];
    auto it = textures.find(name);
    if (it == textures.end())
      continue;
    const TextureData &textureData = it->second;
    float sizeMB = textureModule->getTextureMemorySize(textureData);
    ProfilerString textureUsageStr = "Non-RI";

    if (texUsage && !texUsage->empty())
    {
      if (auto textureUsageIt = texUsage->find(name); textureUsageIt != texUsage->end())
      {
        if (const TextureUsage &textureUsage = textureUsageIt->second; textureUsage.unique == 1)
          textureUsageStr = "Unique";
        else if (textureUsage.unique >= 2)
          textureUsageStr = ProfilerString("Shared(") + eastl::to_string(textureUsage.unique) + ")";
      }
    }
    ss << "| " << name.c_str() << " | " << TextureModule::getFormatName(textureData.info.cflg) << " | " << textureData.info.w << " | "
       << textureData.info.h << " | " << textureData.info.mipLevels << " | " << std::fixed << std::setprecision(2) << sizeMB << " | "
       << textureUsageStr.c_str() << " |\n";
  }

  ImGui::SetClipboardText(ss.str().c_str());
  return true;
}


// --- ProfilerExporter implementation ---

ProfilerExporter::ProfilerExporter(TextureModule *texture_module) : textureModule(texture_module)
{
  exporters.push_back(eastl::make_unique<CSVExporter>(texture_module));
  exporters.push_back(eastl::make_unique<MarkdownExporter>(texture_module));
}

ProfilerExporter::~ProfilerExporter() {}

void ProfilerExporter::drawExportButton()
{
  if (ImGui::Button("Export Data"))
  {
    if (textureModule)
      exportData(textureModule->getFilteredTextures());
    else
      toast.show("Export error: Module unavailable", DEFAULT_TOAST_DURATION);
  }

  // Format dropdown button
  ImGui::SameLine(0, 0);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

  if (ImGui::ArrowButton("##ExportOptions", ImGuiDir_Down))
    ImGui::OpenPopup("ExportOptionsPopup");

  ImGui::PopStyleVar(STYLE_VAR_COUNT);

  toast.update();
}

void ProfilerExporter::drawExportMenu()
{
  // Format selection dropdown
  if (ImGui::BeginPopup("ExportOptionsPopup"))
  {
    for (size_t i = 0; i < exporters.size(); i++)
    {
      if (ExportFormat format = static_cast<ExportFormat>(i);
          ImGui::Selectable(exporters[i]->getFormatName(), currentFormat == format))
        setFormat(format);
    }
    ImGui::EndPopup();
  }

  toast.draw();
}

void ProfilerExporter::setFormat(ExportFormat format) { currentFormat = format; }

int ProfilerExporter::getExporterIndex() const { return static_cast<int>(currentFormat); }

} // namespace levelprofiler