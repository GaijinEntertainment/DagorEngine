// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <assets/asset.h>

class SaveAsNewCompositeDialog : public PropPanel::DialogWindow
{
public:
  SaveAsNewCompositeDialog(DagorAsset *edited_asset, const char *caption = nullptr, bool show_replace_option = true);

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  String getFullPath();
  String getFileName();
  bool shouldReplaceSelection();
  String getAssetName();

private:
  void handleNameError(const char *text);

  DagorAsset *editedAsset = nullptr;
  PropPanel::ContainerPropertyControl *nameInfoPanel = nullptr;
};
