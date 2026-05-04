// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/container.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/control/menu.h>
#include <util/dag_string.h>
#include "ecsSceneOutlinerPanel.h"

class ECSSceneOutlinerPanel::EventHandler final : public PropPanel::ITreeControlEventHandler, public PropPanel::IMenuEventHandler
{
public:
  explicit EventHandler(ECSSceneOutlinerPanel &owner) noexcept;

  bool onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id, PropPanel::ITreeInterface &tree_interface) override;
  int onMenuItemClick(unsigned id) override;

private:
  void addImport(const String &path);

  ECSSceneOutlinerPanel &owner;
};
