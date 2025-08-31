// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gui/dag_imgui.h>
#include <EASTL/unique_ptr.h>
#include "levelProfilerInterface.h"
#include "levelProfilerExporter.h"
#include "levelProfilerTextureTable.h"
#include "levelProfilerFilter.h"

namespace levelprofiler
{

// ImGui constants to improve code readability
namespace ImGuiConstants
{
static constexpr bool WITH_BORDER = true;
static constexpr bool NO_BORDER = false;
} // namespace ImGuiConstants

class TextureProfilerUI : public IProfilerModule, public ICopyProvider
{
public:
  TextureProfilerUI(TextureModule *texture_module, RIModule *ri_module);
  virtual ~TextureProfilerUI();

  TextureModule *getTextureModule() const { return textureModule; }
  RIModule *getRIModule() const { return riModule; }
  LpTextureTable *getTextureTable() const { return textureTable.get(); }

  void init() override;
  void shutdown() override;
  void drawUI() override;
  ICopyProvider *getCopyProvider() override { return this; }

  CopyResult handleGlobalCopy() const override;
  CopyResult handleContextCopy(const CopyRequest &request) const override;
  eastl::vector<ProfilerString> getContextMenuItems() const override;

  ProfilerExporter &getExporter() { return exporter; }
  TextureFilterManager &getFilterManager() { return filterManager; }

private:
  TextureModule *textureModule = nullptr;
  RIModule *riModule = nullptr;
  TextureFilterManager filterManager;
  ProfilerExporter exporter;
  eastl::unique_ptr<LpTextureTable> textureTable;
  ProfilerString selectedAssetName;

  // Tab rename popup
  bool isShowRenamePopup = false;
  int renameTabIndex = -1;
  char renameBuffer[64] = "";

  void drawLeftPanel();  // Table and statistics
  void drawRightPanel(); // Preview and texture usage list

  GlobalCopyManager *getCopyManager() const;
  ProfilerString generateFullTextureInfo(const ProfilerString &texture_name) const;
};

class RIProfilerUI : public IProfilerModule, public ICopyProvider
{
public:
  RIProfilerUI(RIModule *ri_module);
  virtual ~RIProfilerUI();

  RIModule *getRIModule() const { return riModule; }

  void init() override;
  void shutdown() override;
  void drawUI() override;
  ICopyProvider *getCopyProvider() override { return this; }

  CopyResult handleGlobalCopy() const override;
  CopyResult handleContextCopy(const CopyRequest &request) const override;
  eastl::vector<ProfilerString> getContextMenuItems() const override;

private:
  RIModule *riModule = nullptr;
};

class LevelProfilerUI : public ILevelProfiler
{
public:
  LevelProfilerUI();
  virtual ~LevelProfilerUI();

  void initialize();

  void init() override;
  void shutdown() override;
  void drawUI() override;
  void collectData() override;
  void clearData() override;
  void addTab(const char *name, IProfilerModule *module_ptr) override;
  int getTabCount() const override { return static_cast<int>(tabs.size()); }
  ProfilerTab *getTab(int index) override;
  void renameTab(int index, const char *new_name) override;

  GlobalCopyManager *getCopyManager() { return &globalCopyManager; }

private:
  GlobalCopyManager globalCopyManager;
  eastl::unique_ptr<ToastNotificationAdapter> toastAdapter;

  eastl::unique_ptr<TextureModule> textureModule;
  eastl::unique_ptr<RIModule> riModule;

  eastl::unique_ptr<TextureProfilerUI> textureProfilerUI;
  eastl::unique_ptr<RIProfilerUI> riProfilerUI;

  eastl::vector<ProfilerTab> tabs;
  int currentTabIndex = 0;

  bool isOpenRenamePopup = false;
  int renameTabIndex = -1;
  ProfilerString renameBuffer;

  void drawTabBar();
  void drawRenamePopup();
};

} // namespace levelprofiler