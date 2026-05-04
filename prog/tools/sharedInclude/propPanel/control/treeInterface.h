//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <imgui/imgui.h>
#include "propPanel/c_common.h"
#include "propPanel/imguiHelper.h"

namespace PropPanel
{

class ContainerPropertyControl;
class IMenu;
class ITreeInterface;
class TreeNode;

class ITreeControlEventHandler
{
public:
  virtual bool onTreeContextMenu(ContainerPropertyControl &tree, int pcb_id, ITreeInterface &tree_interface) = 0;
};

// NOTE: ImGui porting: this and its usage is totally new compared to the original.
class ITreeInterface
{
public:
  virtual ~ITreeInterface() {}

  virtual IMenu &createContextMenu() = 0;
};

class ITreeRenderEx
{
public:
  struct NodeStartData
  {
    ImGuiID id;
    ImGuiTreeNodeFlags flags;
    const char *label;
    const char *labelEnd;
    ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &endData;
    bool allowBlockedHover;
  };

  struct ItemRenderData
  {
    TLeafHandle handle;
    ImVec2 textPos;
    bool isHovered;
  };

  virtual bool treeNodeStart(const NodeStartData &data) = 0;
  virtual void treeNodeRender(ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool show_arrow) = 0;
  virtual void treeNodeEnd(ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data) = 0;
  virtual void treeItemRenderEx(const ItemRenderData &data) = 0;
};

class ITreeFilter
{
public:
  virtual ~ITreeFilter() = default;
  virtual bool filterNode(const TreeNode &node) = 0;
  virtual bool hasAnyFilter() const = 0;
  virtual const char *getNoResultsMsg() const { return nullptr; }
};

} // namespace PropPanel