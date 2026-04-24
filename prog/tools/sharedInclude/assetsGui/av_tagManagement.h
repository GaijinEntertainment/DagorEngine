// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assetsGui/av_client.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/customControl.h>
#include <propPanel/messageQueue.h>
#include <generic/dag_tab.h>
#include <EASTL/string.h>
#include <imgui/imgui.h>

constexpr const char *AssetTagInputLabel = "##tagText";

class AssetTagManager : public IAssetBaseViewClient, public PropPanel::IDelayedCallbackHandler
{
public:
  explicit AssetTagManager(const DagorAsset *_a);

  // Returns with false if the user requested closing the window with a hotkey.
  bool updateImgui();

  // IAssetBaseViewClient
  virtual void onAvClose() {};
  virtual void onAvAssetDblClick(DagorAsset *, const char *) {};
  virtual void onAvSelectAsset(DagorAsset *asset, const char *asset_name);
  virtual void onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name);

  static constexpr ImGuiKeyChord WINDOW_TOGGLE_HOTKEY = ImGuiMod_Ctrl | ImGuiKey_T;

private:
  const DagorAsset *lastSelectedAsset = nullptr;

  void onImguiDelayedCallback(void *data) override;

  void resetInput();

  eastl::string inputText;
  bool forceReloadInputBuf = false;
  const char *inputHint = "";
  float lastScrollX = 0.0f;
  Tab<eastl::string> inputTagNames;
  Tab<ImVec2> inputTagNameSizes;
  Tab<ImVec2> inputTagNamePositions;
  Tab<bool> inputTagNameValidity;
  bool inputDisabled = false;
  bool inputLastActive = false;
  bool allTagsValid = true;
  bool allTagsReallyNew = true;
};


class AssetTagsDlg : public PropPanel::DialogWindow, public PropPanel::ICustomControl
{
public:
  AssetTagsDlg(void *phandle, const DagorAsset *selected_asset);

  AssetTagManager *getTagManager() { return &mTagManager; }

private:
  // PropPanel::ICustomControl
  void customControlUpdate(int id) override;

  static const int DEFAULT_MINIMUM_WIDTH = 250;
  static const int DEFAULT_MINIMUM_HEIGHT = 115;
  static const int DEFAULT_WIDTH = 440;
  static const int DEFAULT_HEIGHT = 135;

  AssetTagManager mTagManager;
};