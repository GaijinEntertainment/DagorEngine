// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../imageHelper.h"
#include "../scopedImguiBeginDisabled.h"
#include <libTools/util/strUtil.h>
#include <propPanel/control/contextMenu.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/control/treeHierarchyLineDrawer.h>
#include <propPanel/control/dragAndDropHandler.h>
#include <propPanel/control/treeNode.h>
#include <propPanel/c_common.h> // TLeafHandle
#include <propPanel/c_window_event_handler.h>
#include <propPanel/colors.h>
#include <propPanel/constants.h>
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/immediateFocusLossHandler.h>
#include <propPanel/propPanel.h>
#include <dag/dag_vector.h>
#include <winGuiWrapper/wgw_input.h>

#include <EASTL/stack.h>
#include <EASTL/unique_ptr.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

class TreeControlStandalone : public ITreeInterface
{
  inline static constexpr const char *default_no_results_msg = "No results found";

public:
  // The original Windows-based control sent events even when the selection was changed by code, not just when it was
  // set by the user through the UI. This behaviour can be controlled with selection_change_events_by_code.
  explicit TreeControlStandalone(bool selection_change_events_by_code, bool has_checkboxes = false, bool multi_select = false);

  void setEventHandler(WindowControlEventHandler *event_handler) { eventHandler = event_handler; }

  // The provided window_base is passed to the event handler events (set with setEventHandler). This class does not use
  // it otherwise.
  void setWindowBaseForEventHandler(WindowBase *window_base) { windowBaseForEventHandler = window_base; }

  bool isEnabled() const { return controlEnabled; }
  void setEnabled(bool enabled) { controlEnabled = enabled; }

  void setFocus() { focus_helper.requestFocus(this); }

  void setCheckboxIcons(IconId checked, IconId unchecked)
  {
    checkedIcon = checked;
    uncheckedIcon = unchecked;
  }

  TLeafHandle createTreeLeaf(TLeafHandle parent, const char caption[], IconId icon, void *user_data = nullptr)
  {
    TreeNode *parentNode = leafHandleAsNode(parent);
    if (!parentNode)
      parentNode = &rootNode;

    TreeNode *newNode = new TreeNode();
    newNode->title = caption;
    newNode->icon = icon;
    newNode->parent = parentNode;
    newNode->userData = user_data;
    parentNode->children.push_back(newNode);

    attemptRefilterOnLeafCreation(newNode);
    markLinearizedTreeNodesDirty();

    return nodeAsLeafHandle(newNode);
  }

  TLeafHandle createTreeLeaf(TLeafHandle parent, const char caption[], const char icon_name[], void *user_data = nullptr)
  {
    const IconId icon = getTexture(icon_name);
    return createTreeLeaf(parent, caption, icon, user_data);
  }

  TLeafHandle createTreeLeafAsFirst(TLeafHandle parent, const char caption[], IconId icon, void *user_data = nullptr)
  {
    TreeNode *parentNode = leafHandleAsNode(parent);
    if (!parentNode)
      parentNode = &rootNode;

    TreeNode *newNode = new TreeNode();
    newNode->title = caption;
    newNode->icon = icon;
    newNode->parent = parentNode;
    newNode->userData = user_data;
    parentNode->children.insert(parentNode->children.begin(), newNode);

    attemptRefilterOnLeafCreation(newNode);
    markLinearizedTreeNodesDirty();

    return nodeAsLeafHandle(newNode);
  }

  TLeafHandle createTreeLeafAsFirst(TLeafHandle parent, const char caption[], const char icon_name[], void *user_data = nullptr)
  {
    const IconId icon = getTexture(icon_name);
    return createTreeLeafAsFirst(parent, caption, icon, user_data);
  }

  void clear()
  {
    clear_all_ptr_items(rootNode.children);
    lastSelected = nullptr;
    ensureVisibleRequested = nullptr;
    markLinearizedTreeNodesDirty();
    resetMaximumSeenWidth();

    if (selectionChangeEventsByCode && eventHandler)
      eventHandler->onWcChange(windowBaseForEventHandler);
  }

  bool removeLeaf(TLeafHandle id)
  {
    TreeNode *node = leafHandleAsNode(id);
    if (!node)
      return false;

    TreeNode *parent = node->parent;
    G_ASSERT(parent);
    const int index = parent->getChildIndex(node);
    G_ASSERT(index >= 0);
    parent->children.erase(parent->children.begin() + index);

    if (node == ensureVisibleRequested)
      ensureVisibleRequested = nullptr;

    if (node == lastSelected)
    {
      if (parent->children.size() > 0)
        lastSelected = parent->children[index >= parent->children.size() ? (parent->children.size() - 1) : index];
      else
        lastSelected = parent == &rootNode ? nullptr : parent;

      if (selectionChangeEventsByCode && eventHandler)
        eventHandler->onWcChange(windowBaseForEventHandler);
    }

    delete node;

    attemptRefilterOnLeafDeletion(parent);
    markLinearizedTreeNodesDirty();
    resetMaximumSeenWidth();
    return true;
  }

  bool swapLeaf(TLeafHandle id, bool after)
  {
    TreeNode *node = leafHandleAsNode(id);
    if (!node)
      return false;

    TreeNode *parent = node->parent;
    G_ASSERT(parent);
    const int index = parent->getChildIndex(node);
    G_ASSERT(index >= 0);

    if (after)
    {
      if ((index + 1) >= parent->children.size())
        return false;

      eastl::swap(parent->children[index], parent->children[index + 1]);
    }
    else
    {
      if (index == 0)
        return false;

      eastl::swap(parent->children[index], parent->children[index - 1]);
    }

    markLinearizedTreeNodesDirty();
    return true;
  }

  void setTitle(TLeafHandle leaf, const char value[])
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->title = value;
  }

  void setButtonPictures(TLeafHandle leaf, const char *fname = NULL)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->icon = getTexture(fname);
  }

  void setButtonPictures(TLeafHandle leaf, IconId icon_id)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->icon = icon_id;
  }

  void copyButtonPictures(TLeafHandle from, TLeafHandle to)
  {
    TreeNode *nodeFrom = leafHandleAsNode(from);
    TreeNode *nodeTo = leafHandleAsNode(to);
    if (!nodeFrom || !nodeTo)
      return;

    nodeTo->icon = nodeFrom->icon;
  }

  void setColor(TLeafHandle leaf, E3DCOLOR value)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->textColor = value;
  }

  bool isExpanded(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return false;

    return node->getFlagValue(TreeNode::EXPANDED);
  }

  bool isFilteredIn(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return false;

    return node->getFlagValue(TreeNode::FILTERED_IN);
  }

  void setExpanded(TLeafHandle leaf, bool open = true)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->setFlagValue(TreeNode::EXPANDED, open);
    markLinearizedTreeNodesDirty();
    resetMaximumSeenWidth();
  }

  void setExpandedRecursive(TLeafHandle leaf, bool open = true)
  {
    TreeNode *node = leaf ? leafHandleAsNode(leaf) : &rootNode;
    if (!node || node->children.empty() || !node->getFlagValue(TreeNode::FILTERED_IN))
      return;

    node->setFlagValue(TreeNode::EXPANDED, open);
    markLinearizedTreeNodesDirty();
    resetMaximumSeenWidth();

    for (TreeNode *child : node->children)
      setExpandedRecursive(nodeAsLeafHandle(child), open);
  }

  void setExpandedTillRoot(TLeafHandle leaf, bool open = true)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    while (node && node != &rootNode)
    {
      node->setFlagValue(TreeNode::EXPANDED, open);
      node = node->parent;
      G_ASSERT(node);
    }

    markLinearizedTreeNodesDirty();
    resetMaximumSeenWidth();
  }

  void setUserData(TLeafHandle leaf, void *value)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->userData = value;
  }

  void setCheckboxEnabled(TLeafHandle leaf, bool enabled)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->setFlagValue(TreeNode::CHECKBOX_ENABLED, enabled);
  }
  void setCheckboxValue(TLeafHandle leaf, bool val)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->setFlagValue(TreeNode::CHECKBOX_ENABLED, true);
    node->setFlagValue(TreeNode::CHECKBOX_VALUE, val);
  }
  bool getCheckboxEnabled(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return false;

    return node->getFlagValue(TreeNode::CHECKBOX_ENABLED);
  }
  bool getCheckboxValue(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return false;

    return node->getFlagValue(TreeNode::CHECKBOX_VALUE);
  }

  void setIcon(TLeafHandle leaf, IconId icon)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->icon = icon;
  }

  void setStateIcon(TLeafHandle leaf, IconId icon)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->stateIcon = icon;
  }

  void setNewParent(TLeafHandle leaf, TLeafHandle new_parent_leaf)
  {
    TreeNode *source = leafHandleAsNode(leaf);
    TreeNode *sourceParent = source->parent;
    TreeNode *newParent = leafHandleAsNode(new_parent_leaf);

    auto it = eastl::remove_if(sourceParent->children.begin(), sourceParent->children.end(),
      [&source](TreeNode *entry) { return entry == source; });
    sourceParent->children.erase(it, sourceParent->children.end());
    newParent->children.push_back(source);
    source->parent = newParent;

    attemptRefilterOnLeafDeletion(sourceParent);
    attemptRefilterOnLeafCreation(source);
    markLinearizedTreeNodesDirty();
  }

  IconId getTexture(const char *name) { return image_helper.loadIcon(name); }

  // This function works specially for null leafs. If leaf == 0 then it returns the number of items at the topmost level (the internal
  // root's children count).
  // getChildCount(0) is different than calling getChildCount(getRootLeaf()), the latter equals to getChildCount(getChild(0, 0)).
  int getChildCount(TLeafHandle leaf) const
  {
    const TreeNode *node = leaf == nullptr ? &rootNode : leafHandleAsNode(leaf);
    return node->children.size();
  }

  int getChildCountFiltered(TLeafHandle leaf) const
  {
    const TreeNode *node = leaf == nullptr ? &rootNode : leafHandleAsNode(leaf);
    return node->getChildCountFiltered();
  }


  // This function works specially for null leafs. If leaf == 0 then it returns the items at the topmost level (the root's children).
  // getChildLeaf(0, 0) is different than calling getChildLeaf(getRootLeaf(), 0), the latter equals to getChildLeaf(getChildLeaf(0, 0),
  // 0).
  TLeafHandle getChildLeaf(TLeafHandle leaf, unsigned idx) const
  {
    const TreeNode *node = leaf == nullptr ? &rootNode : leafHandleAsNode(leaf);
    if (idx >= node->children.size())
      return nullptr;

    return nodeAsLeafHandle(node->children[idx]);
  }

  int getChildIndex(TLeafHandle leaf) const
  {
    TreeNode *leafNode = leafHandleAsNode(leaf);
    if (!leafNode)
    {
      return -1;
    }

    const TreeNode *parentNode = leafNode->parent ? leafNode->parent : &rootNode;
    G_ASSERT(parentNode);

    return parentNode->getChildIndex(leafNode);
  }

  void setChildIndex(TLeafHandle leaf, int idx)
  {
    TreeNode *leafNode = leafHandleAsNode(leaf);
    if (!leafNode)
    {
      return;
    }

    TreeNode *parentNode = leafNode->parent ? leafNode->parent : &rootNode;
    G_ASSERT(parentNode);

    auto it = eastl::find(parentNode->children.begin(), parentNode->children.end(), leafNode);
    if (it == parentNode->children.cend())
    {
      return;
    }

    TreeNode *copy = *it;
    *it = nullptr;

    parentNode->children.insert(parentNode->children.begin() + idx, copy);
    parentNode->children.erase(eastl::find(parentNode->children.begin(), parentNode->children.end(), nullptr));

    markLinearizedTreeNodesDirty();
  }

  TLeafHandle getParentLeaf(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return nullptr;

    TreeNode *parent = node->parent;
    G_ASSERT(parent);
    return parent == &rootNode ? 0 : nodeAsLeafHandle(parent);
  }

  TLeafHandle getNextLeaf(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return 0;

    if (TreeNode *nextChild = node->getFirstChildFiltered())
      return nodeAsLeafHandle(nextChild);

    TreeNode *sibling = node->getNextSiblingFiltered();
    if (sibling)
      return nodeAsLeafHandle(sibling);

    for (;;)
    {
      TreeNode *parent = node->parent;
      G_ASSERT(parent);
      if (parent == &rootNode)
        return getRootLeaf();

      TreeNode *siblingParent = parent->getNextSiblingFiltered();
      if (siblingParent)
        return nodeAsLeafHandle(siblingParent);

      node = parent;
    }
  }

  TLeafHandle getPrevLeaf(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return 0;

    TreeNode *sibling = node->getPrevSiblingFiltered();
    if (sibling)
      return nodeAsLeafHandle(getLastSubNode(sibling));

    TreeNode *parent = node->parent;
    G_ASSERT(parent);
    if (parent == &rootNode)
    {
      TreeNode *result = getLastSubNode(parent);
      return result == parent ? 0 : nodeAsLeafHandle(result);
    }
    else
    {
      return nodeAsLeafHandle(parent);
    }
  }

  TreeNode *getItemNode(TLeafHandle p) { return leafHandleAsNode(p); }

  const TreeNode *getItemNode(TLeafHandle p) const { return leafHandleAsNode(p); }

  // Returns with the first topmost level item.
  // If you want to get the number of topmost level items use getChildCount(nullptr), if you want to iterate over the
  // topmost level items use getChildLeaf(nullptr, ...) instead.
  TLeafHandle getRootLeaf() const { return nodeAsLeafHandle(rootNode.getFirstChild()); }

  const TreeNode &getRootNode() const { return rootNode; }

  // Visible means that the item's parents are expanded, and the item might be visible.
  // The functinon returns with 0 if there are no items. (Unlike getNextLeaf that returns with 0 or the root.)
  TLeafHandle getNextVisibleLeaf(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return 0;

    {
      TreeNode *nextChild = node->getFirstChildFiltered();
      if (nextChild && node->getFlagValue(TreeNode::EXPANDED))
        return nodeAsLeafHandle(nextChild);
    }

    TreeNode *sibling = node->getNextSiblingFiltered();
    if (sibling)
      return nodeAsLeafHandle(sibling);

    for (;;)
    {
      TreeNode *parent = node->parent;
      G_ASSERT(parent);
      if (parent == &rootNode)
        return 0;

      TreeNode *siblingParent = parent->getNextSiblingFiltered();
      if (siblingParent)
        return nodeAsLeafHandle(siblingParent);

      node = parent;
    }
  }

  const String *getItemTitle(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return nullptr;

    return &node->title;
  }

  void *getUserData(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return nullptr;

    return node->userData;
  }

  bool isLeafSelected(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return false;

    return node->getFlagValue(TreeNode::SELECTED);
  }

  void setSelectedLeaf(TLeafHandle leaf, bool keep_selected = false)
  {
    TreeNode *node = leafHandleAsNode(leaf);

    if (multiSelectionEnabled)
    {
      setExpandedTillRoot(leaf);
      if (!keep_selected)
        resetFlags(rootNode, TreeNode::SELECTED, false);

      if (node)
        node->setFlagValue(TreeNode::SELECTED, true);
    }
    else
    {
      if (node == lastSelected)
        return;

      setExpandedTillRoot(leaf);

      if (lastSelected)
      {
        lastSelected->setFlagValue(TreeNode::SELECTED, false);
      }
    }

    lastSelected = node;
    ensureVisibleRequested = node;

    if (!multiSelectionEnabled && lastSelected)
    {
      lastSelected->setFlagValue(TreeNode::SELECTED, true);
    }

    if (selectionChangeEventsByCode && eventHandler)
      eventHandler->onWcChange(windowBaseForEventHandler);
  }

  TLeafHandle getSelectedLeaf() const { return nodeAsLeafHandle(lastSelected); }

  // Visible means that the item's parents are expanded, and the item might be visible.
  // Returns true if the selection has been changed.
  bool selectAllVisibleItems(bool select = true)
  {
    G_ASSERT(multiSelectionEnabled);

    bool selectionChanged = false;

    for (TLeafHandle item = getRootLeaf(); item; item = getNextVisibleLeaf(item))
    {
      TreeNode *node = leafHandleAsNode(item);
      if (node->getFlagValue(TreeNode::SELECTED) != select)
      {
        node->setFlagValue(TreeNode::SELECTED, select);
        selectionChanged = true;

        if (select)
          lastSelected = node;
        else if (node == lastSelected)
          lastSelected = nullptr;
      }
    }

    return selectionChanged;
  }

  // Both from and to are inclusive.
  // Visible means that the item's parents are expanded, and the item might be visible.
  // Returns true if the selection has been changed.
  bool selectVisibleItemRange(TLeafHandle from, TLeafHandle to, bool select = true)
  {
    G_ASSERT(multiSelectionEnabled);

    bool selectionChanged = false;

    if (!from || !to)
      return selectionChanged;

    bool goForward = false;

    for (TLeafHandle item = from; item; item = getNextVisibleLeaf(item))
    {
      if (item == to)
      {
        goForward = true;
        break;
      }
    }

    if (!goForward)
      eastl::swap(from, to);

    for (TLeafHandle item = from; item; item = getNextVisibleLeaf(item))
    {
      TreeNode *node = leafHandleAsNode(item);
      if (node->getFlagValue(TreeNode::SELECTED) != select)
      {
        node->setFlagValue(TreeNode::SELECTED, select);
        selectionChanged = true;

        if (select)
          lastSelected = node;
        else if (node == lastSelected)
          lastSelected = nullptr;
      }

      if (item == to)
        break;
    }

    return selectionChanged;
  }

  void getSelectedLeafs(dag::Vector<TLeafHandle> &leafs, bool search_in_collapsed, bool include_filtered_out) const
  {
    if (multiSelectionEnabled)
    {
      eastl::stack<TreeNode *> items;
      const auto pushChildren = [&](const TreeNode *node) {
        for (auto it = node->children.rbegin(); it != node->children.rend(); ++it)
        {
          items.push(*it);
        }
      };
      pushChildren(&rootNode);

      while (!items.empty())
      {
        TreeNode *item = items.top();
        items.pop();

        if (!include_filtered_out && !item->getFlagValue(TreeNode::FILTERED_IN))
        {
          continue;
        }

        if (item->getFlagValue(TreeNode::SELECTED))
        {
          leafs.push_back(nodeAsLeafHandle(item));
        }

        if (item->getFlagValue(TreeNode::EXPANDED) || search_in_collapsed)
        {
          pushChildren(item);
        }
      }
    }
    else
    {
      TLeafHandle leaf = getSelectedLeaf();
      if (leaf)
        leafs.push_back(leaf);
    }
  }

  void ensureVisible(TLeafHandle leaf)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    ensureVisibleRequested = node;
  }

  IMenu &createContextMenu() override
  {
    contextMenu.reset(new ContextMenu());
    return *contextMenu;
  }

  ImGuiDragDropFlags getDragDropFlags() const { return dragDropFlags; }
  void setDragDropFlags(ImGuiDragDropFlags drag_drop_flags) { dragDropFlags = drag_drop_flags; }

  void setDragHandler(ITreeDragHandler *drag_handler) { dragHandler = drag_handler; }
  ITreeDragHandler *getDragHandler() const { return dragHandler; }

  void setDropHandler(ITreeDropHandler *drop_handler) { dropHandler = drop_handler; }
  ITreeDropHandler *getDropHandler() const { return dropHandler; }

  // A message to display in the center of the tree.
  // If in_message is empty then nothing will be displayed.
  void setMessage(const char *in_message)
  {
    userMessage = in_message;
    setMessageInternal();
  }

  // If control_height is 0 then it will use the entire available height.
  void updateImgui(float control_height = 0.0f)
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    // "c" stands for child. It could be anything.
    const ImGuiID childWindowId = ImGui::GetCurrentWindow()->GetID("c");
    const ImVec2 regionAvailable = ImGui::GetContentRegionAvail();
    const ImVec2 childSize(regionAvailable.x, control_height > 0.0f ? control_height : regionAvailable.y);
    if (ImGui::BeginChild(childWindowId, childSize, ImGuiChildFlags_FrameStyle,
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_HorizontalScrollbar))
    {
      bool selectionChanged = false;
      bool doubleClickedOnItem = false;
      bool rightClickedOnItem = false;
      bool clickedOnStateIcon = false;
      bool toggledCheckboxWithSpaceKey = false;
      TreeNode *collapsed_item = nullptr;
      float cursorScreenPosMaxX = ImGui::GetCursorScreenPos().x;

      const bool focusRequested = focus_helper.getRequestedControlToFocus() == this;
      if (focusRequested)
        focus_helper.clearRequest();

      ImGuiMultiSelectFlags multiSelectFlags = ImGuiMultiSelectFlags_None;
      if (!multiSelectionEnabled)
        multiSelectFlags = ImGuiMultiSelectFlags_SingleSelect | ImGuiMultiSelectFlags_SelectOnClickRelease;

      ImGuiMultiSelectIO *multiSelectIo = ImGui::BeginMultiSelect(multiSelectFlags, -1, -1);
      if (multiSelectIo)
      {
        selectionChanged |= applySelectionRequests(*multiSelectIo);

        // BeginMultiSelect starts a focus scope. If this is the active (focused) focus scope then forbid EndMultiSelect
        // to handle the space key (to toggle selection) because we use it to toggle the check box.
        if (hasCheckboxes && ImGui::GetCurrentFocusScope() == ImGui::GetCurrentContext()->NavFocusScopeId)
        {
          ImGui::SetKeyOwner(ImGuiKey_Space, childWindowId);
          toggledCheckboxWithSpaceKey = ImGui::IsKeyPressed(ImGuiKey_Space, ImGuiInputFlags_None, childWindowId);
        }
      }

      // SetNextItemOpen does nothing if SkipItems is set, so to avoid unintended openness change we do nothing too.
      if (!ImGui::GetCurrentWindow()->SkipItems)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 3.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, getOverriddenColor(ColorOverride::TREE_SELECTION_BACKGROUND));

        if (linearizedTreeNodesDirty)
        {
          performLinearization();
        }

        ImGuiListClipper clipper;
        clipper.Begin(linearizedTreeNodes.size());

        // The visible requested item cannot be clipped otherwise jumping to it will not work.
        if (ensureVisibleRequested)
        {
          auto it = eastl::find(linearizedTreeNodes.begin(), linearizedTreeNodes.end(), ensureVisibleRequested);
          if (it != linearizedTreeNodes.end())
            clipper.IncludeItemByIndex(it - linearizedTreeNodes.begin());
        }

        // Include a screenful of items around the selection's head item when the user is moving the cursor with keys,
        // otherwise up/down/page up/page down do not work as expected if the user has scrolled away and the cursor is off screen.
        // This should not be needed, ImGuiListClipper_StepInternal() does something similar, but it does not work correctly.
        // See https://github.com/ocornut/imgui/issues/9079.
        if (multiSelectIo && multiSelectIo->RangeSrcItem != 0)
        {
          // ImGuiListClipper_StepInternal() uses this condition to check whether the user has requested a cursor move.
          ImGuiContext &g = *ImGui::GetCurrentContext();
          ImGuiWindow *window = ImGui::GetCurrentWindowRead();
          const bool isNavRequest = g.NavMoveScoringItems && g.NavWindow && g.NavWindow->RootWindowForNav == window->RootWindowForNav;
          if (isNavRequest)
          {
            const TreeNode *rangeSrcItemNode = reinterpret_cast<TreeNode *>(multiSelectIo->RangeSrcItem);
            auto it = eastl::find(linearizedTreeNodes.begin(), linearizedTreeNodes.end(), rangeSrcItemNode);
            if (it != linearizedTreeNodes.end())
            {
              const int rangeSrcItemIndex = it - linearizedTreeNodes.begin();
              const float windowInnerHeight = ImGui::GetCurrentWindowRead()->InnerRect.GetHeight();
              const float totalItemHeight = ImGui::GetTextLineHeightWithSpacing();
              const int itemsPerPage = (int)ceilf(windowInnerHeight / totalItemHeight);
              const int startIndex = max(rangeSrcItemIndex - itemsPerPage, 0);
              const int endIndex = min(rangeSrcItemIndex + itemsPerPage, (int)linearizedTreeNodes.size());
              clipper.IncludeItemsByIndex(startIndex, endIndex);
            }
          }
        }

        while (clipper.Step())
          for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index)
          {
            drawNode(linearizedTreeNodes[index], doubleClickedOnItem, rightClickedOnItem, clickedOnStateIcon, collapsed_item,
              focusRequested, cursorScreenPosMaxX);
          }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
      }

      ImGui::EndMultiSelect();
      if (multiSelectIo)
        selectionChanged |= applySelectionRequests(*multiSelectIo);

      if (collapsed_item && multiSelectionEnabled)
      {
        PropPanel::send_immediate_focus_loss_notification();
        selectionChanged |= deselectAllVisibleChildren(collapsed_item);
      }

      if (toggledCheckboxWithSpaceKey && !selectionChanged)
      {
        toggleSelectionCheckState();
        clickedOnStateIcon = true;
      }

      if ((selectionChanged || clickedOnStateIcon) && eventHandler)
        eventHandler->onWcChange(windowBaseForEventHandler);

      if (doubleClickedOnItem && eventHandler)
        eventHandler->onWcDoubleClick(windowBaseForEventHandler);
      else if (rightClickedOnItem && eventHandler)
        eventHandler->onWcRightClick(windowBaseForEventHandler);
      else if (ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_LeftArrow))
        setExpandedRecursive(getSelectedLeaf(), false);
      else if (ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_RightArrow))
        setExpandedRecursive(getSelectedLeaf(), true);
      // TODO: ImGui porting: this is needed for Composite Editor. Maybe add an overrideable function instead and make
      // ImGui::Shortcut accessible behind a method, and get rid of onWcKeyDown?
      else if (eventHandler && ImGui::Shortcut(ImGuiKey_Delete))
        eventHandler->onWcKeyDown(windowBaseForEventHandler, wingw::V_DELETE);

      if (contextMenu && !contextMenu->updateImgui())
        contextMenu.reset();

      // Set the maximum needed horizontal scrolling.
      // We keep track of the maximum seen row width to prevent the horizontal scrollbar from getting smaller when
      // scrolling a longer row out of screen. This also prevents the scrollbar from continuously appearing and hiding
      // in edge cases (for example when the longest row gets pushed out of screen by the appearance of the scrollbar).
      const float scrollX = ImGui::GetScrollX();
      maximumSeenWidth = max(maximumSeenWidth, cursorScreenPosMaxX + scrollX);
      ImGui::SetCursorScreenPos(ImVec2(maximumSeenWidth - scrollX, ImGui::GetCursorScreenPos().y));
      ImGui::Dummy(ImVec2(0.0f, 0.0f));

      if (!message.empty())
        drawMessage();
    }
    ImGui::EndChild();
  }

  void setTreeRenderEx(ITreeRenderEx *interface);

  // Note: due to continious filtering (on leaf creation/deletion), it is not advisable
  // to have filter interface installed with filters enabled (hasAnyFilter() returns true)
  // when constructing a tree with a large number of items, since on every add/remove
  // it will try to refilter hierarchy related to that item
  void setTreeFilter(ITreeFilter *filter)
  {
    treeFilter = filter;
    refilter();
  }
  void filter() { refilter(); }

  TLeafHandle search(const char *text, TLeafHandle first, bool forward, bool use_wildcard_search = false)
  {
    const String searchText = String(text).toLower();
    TLeafHandle next = first;

    while (true)
    {
      next = forward ? getNextLeaf(next) : getPrevLeaf(next);
      if (!next || next == first)
        return 0;

      TreeNode *node = getItemNode(next);
      G_ASSERT(node);
      if (!node)
        continue;

      if (searchText.empty())
        return next;

      if (use_wildcard_search)
      {
        if (str_matches_wildcard_pattern(String(node->getTitle()).toLower().c_str(), searchText.c_str()))
          return next;
      }
      else
      {
        if (strstr(String(node->getTitle()).toLower().c_str(), searchText.c_str()))
          return next;
      }
    }
  }

private:
  static TreeNode *leafHandleAsNode(TLeafHandle handle) { return reinterpret_cast<TreeNode *>(handle); }

  static TLeafHandle nodeAsLeafHandle(TreeNode *node) { return reinterpret_cast<TLeafHandle>(node); }

  static TreeNode *getLastSubNode(TreeNode *item)
  {
    if (!item)
      return 0;

    TreeNode *child = item->getFirstChildFiltered();
    if (!child)
      return item;

    while (TreeNode *siblingChild = child->getNextSiblingFiltered())
      child = siblingChild;

    return getLastSubNode(child);
  }

  void toggleSelectionCheckState()
  {
    const TLeafHandle caret = getSelectedLeaf();
    if (!caret)
      return;

    const TreeNode *caretNode = leafHandleAsNode(caret);
    if (!caretNode->getFlagValue(TreeNode::CHECKBOX_ENABLED))
      return;

    const bool newState = !caretNode->getFlagValue(TreeNode::CHECKBOX_VALUE);

    dag::Vector<TLeafHandle> leafs;
    getSelectedLeafs(leafs, false, false);
    for (TLeafHandle leaf : leafs)
    {
      TreeNode *node = leafHandleAsNode(leaf);
      if (!node->getFlagValue(TreeNode::CHECKBOX_ENABLED))
        continue;
      node->setFlagValue(TreeNode::CHECKBOX_VALUE, newState);
    }
  }

  bool applySelectionRequests(const ImGuiMultiSelectIO &multi_select_io)
  {
    bool selectionChanged = false;

    // Send the notification regardless of the actual change if there was some input.
    if (!multi_select_io.Requests.empty())
      PropPanel::send_immediate_focus_loss_notification();

    if (multiSelectionEnabled)
    {
      for (const ImGuiSelectionRequest &request : multi_select_io.Requests)
      {
        if (request.Type == ImGuiSelectionRequestType_SetAll)
        {
          selectionChanged |= selectAllVisibleItems(request.Selected);
        }
        else if (request.Type == ImGuiSelectionRequestType_SetRange)
        {
          TreeNode *firstNode = reinterpret_cast<TreeNode *>(request.RangeFirstItem);
          TreeNode *lastNode = reinterpret_cast<TreeNode *>(request.RangeLastItem);
          selectionChanged |= selectVisibleItemRange(nodeAsLeafHandle(firstNode), nodeAsLeafHandle(lastNode), request.Selected);
        }
      }
    }
    else
    {
      for (const ImGuiSelectionRequest &request : multi_select_io.Requests)
      {
        if (request.Type == ImGuiSelectionRequestType_SetAll)
        {
          G_ASSERT(!request.Selected);
          if (lastSelected != nullptr)
          {
            lastSelected->setFlagValue(TreeNode::SELECTED, false);
            lastSelected = nullptr;
            selectionChanged = true;
          }
        }
        else if (request.Type == ImGuiSelectionRequestType_SetRange)
        {
          G_ASSERT(request.RangeFirstItem == request.RangeLastItem);
          TreeNode *newSelection = request.Selected ? reinterpret_cast<TreeNode *>(request.RangeFirstItem) : nullptr;
          if (lastSelected != newSelection)
          {
            if (lastSelected)
            {
              lastSelected->setFlagValue(TreeNode::SELECTED, false);
            }
            lastSelected = newSelection;
            selectionChanged = true;

            if (lastSelected)
            {
              lastSelected->setFlagValue(TreeNode::SELECTED, true);
            }
          }
        }
      }
    }

    return selectionChanged;
  }

  bool deselectAllVisibleChildren(TreeNode *node)
  {
    G_ASSERT(multiSelectionEnabled);

    bool selectionChanged = false;

    for (TreeNode *child = node->getFirstChildFiltered(); child; child = child->getNextSiblingFiltered())
    {
      if (child->getFlagValue(TreeNode::SELECTED))
      {
        child->setFlagValue(TreeNode::SELECTED, false);
        selectionChanged = true;

        if (child == lastSelected)
          lastSelected = nullptr;
      }

      if (child->getFlagValue(TreeNode::EXPANDED))
        selectionChanged |= deselectAllVisibleChildren(child);
    }

    return selectionChanged;
  }

  void drawNode(TreeNode *node, bool &double_clicked_on_item, bool &right_clicked_on_item, bool &clicked_on_state_icon,
    TreeNode *&collapsed_item, bool focus_selected, float &cursor_screen_pos_max_x);

  void drawMessage()
  {
    const ImVec2 textSize = ImGui::CalcTextSize(message, nullptr);
    const ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
    ImVec2 posMin = ImGui::GetCursorScreenPos();
    ImVec2 posMax(posMin.x + contentRegionAvail.x, posMin.y + contentRegionAvail.y);
    posMin.x = max(posMin.x, posMin.x + (((posMax.x - posMin.x) - textSize.x) * 0.5f));
    posMin.y = max(posMin.y, posMin.y + (((posMax.y - posMin.y) - textSize.y) * 0.5f));
    posMax.x = min(posMax.x, posMin.x + textSize.x);
    posMax.y = min(posMax.y, posMin.y + textSize.y);
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), posMin, posMax, posMax.x, message, nullptr, &textSize);
  }

  void resetMaximumSeenWidth() { maximumSeenWidth = 0.0f; }

  void markLinearizedTreeNodesDirty() { linearizedTreeNodesDirty = true; }

  void linearizeNode(TreeNode &node)
  {
    for (TreeNode *child : node.children)
    {
      if (!child->getFlagValue(TreeNode::FILTERED_IN))
      {
        continue;
      }

      linearizedTreeNodes.push_back(child);
      if (!child->children.empty() && child->getFlagValue(TreeNode::EXPANDED))
        linearizeNode(*child);
    }
  }

  int getNodeLevel(const TreeNode &node) const
  {
    int level = 0;
    for (const TreeNode *n = node.parent; n != &rootNode; n = n->parent)
      ++level;
    return level;
  }

  void handleDragAndDrop(TreeNode *node, ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &endData);

  void refilter()
  {
    filterRoutine(rootNode);

    TLeafHandle selection = getSelectedLeaf();
    if (selection)
      ensureVisible(selection);

    performLinearization();
    setMessageInternal();
  }

  bool filterRoutine(TreeNode &node)
  {
    bool hasItems = false;
    for (int i = 0; i < node.children.size(); ++i)
    {
      TreeNode &nd = *node.children[i];

      nd.setFlagValue(TreeNode::FILTERED_IN, false);
      const bool isVisible = treeFilter && treeFilter->hasAnyFilter() ? treeFilter->filterNode(nd) : true;

      if (!isVisible && !nd.children.size())
        continue;

      if (nd.children.size())
      {
        bool res = filterRoutine(nd);

        if (!res && !isVisible)
        {
          continue;
        }
      }

      nd.setFlagValue(TreeNode::FILTERED_IN, true);
      hasItems = true;
    }

    return hasItems;
  }

  void performLinearization()
  {
    linearizedTreeNodesDirty = false;
    linearizedTreeNodes.clear();
    linearizeNode(rootNode);
  }

  void resetFlags(TreeNode &node, TreeNode::Flags flag, bool val)
  {
    node.setFlagValue(flag, val);
    for (TreeNode *child : node.children)
    {
      resetFlags(*child, flag, val);
    }
  }

  void attemptRefilterOnLeafCreation(TreeNode *new_node)
  {
    if (treeFilter && treeFilter->hasAnyFilter())
    {
      const bool isVisible = treeFilter->filterNode(*new_node);
      new_node->setFlagValue(TreeNode::FILTERED_IN, isVisible);

      if (isVisible)
      {
        TreeNode *currParent = new_node->parent;
        while (currParent)
        {
          if (currParent->getFlagValue(TreeNode::FILTERED_IN))
          {
            break;
          }

          currParent = currParent->parent;
        }

        if (currParent)
        {
          for (TreeNode *childNode : currParent->children)
          {
            resetFlags(*childNode, TreeNode::FILTERED_IN, false);
          }
          filterRoutine(*currParent);
          setMessageInternal();
        }
        else
        {
          refilter();
        }
      }
    }
  }

  void attemptRefilterOnLeafDeletion(TreeNode *parent)
  {
    if (treeFilter && treeFilter->hasAnyFilter() && !parent->getFirstChildFiltered())
    {
      if (parent == &rootNode)
      {
        resetFlags(rootNode, TreeNode::FILTERED_IN, false);
      }
      else
      {
        TreeNode *currParent = parent->parent;
        while (currParent)
        {
          if (getChildCountFiltered(nodeAsLeafHandle(currParent)) > 1)
          {
            break;
          }

          currParent = currParent->parent;
        }

        if (!currParent)
        {
          resetFlags(rootNode, TreeNode::FILTERED_IN, false);
        }
        else
        {
          for (TreeNode *childNode : currParent->children)
          {
            resetFlags(*childNode, TreeNode::FILTERED_IN, false);
          }
          filterRoutine(*currParent);
        }
      }

      setMessageInternal();
    }
  }

  void setMessageInternal()
  {
    if (treeFilter && treeFilter->hasAnyFilter() && rootNode.getChildCount() != 0 && !rootNode.getFirstChildFiltered())
    {
      const char *msg = treeFilter->getNoResultsMsg();
      message = msg ? msg : default_no_results_msg;
    }
    else
    {
      message = userMessage;
    }
  }

  friend class TreeHierarchyLineDrawer<TreeNode>;

  const bool selectionChangeEventsByCode;
  const bool hasCheckboxes;
  const bool multiSelectionEnabled;
  WindowControlEventHandler *eventHandler = nullptr;
  WindowBase *windowBaseForEventHandler = nullptr;
  ITreeDragHandler *dragHandler = nullptr;
  ITreeDropHandler *dropHandler = nullptr;
  ImGuiDragDropFlags dragDropFlags = ImGuiDragDropFlags_PayloadNoCrossProcess;
  bool controlEnabled = true;
  TreeNode rootNode;
  TreeNode *lastSelected = nullptr;
  TreeNode *ensureVisibleRequested = nullptr;
  IconId checkedIcon = IconId::Invalid;
  IconId uncheckedIcon = IconId::Invalid;
  eastl::unique_ptr<ContextMenu> contextMenu;
  String message;
  String userMessage;
  float maximumSeenWidth;
  dag::Vector<TreeNode *> linearizedTreeNodes;
  bool linearizedTreeNodesDirty = false;
  ITreeRenderEx *treeRenderEx = nullptr;
  ITreeFilter *treeFilter = nullptr;
};

} // namespace PropPanel