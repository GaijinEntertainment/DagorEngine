// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../contextMenuInternal.h"
#include "../imageHelper.h"
#include "../scopedImguiBeginDisabled.h"
#include <propPanel/control/treeInterface.h>
#include <propPanel/control/treeHierarchyLineDrawer.h>
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

#include <EASTL/unique_ptr.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

class TreeControlStandalone : public ITreeInterface
{
public:
  enum class CheckboxState
  {
    NoCheckbox,
    Checked,
    Unchecked,
  };

  // The original Windows-based control sent events even when the selection was changed by code, not just when it was
  // set by the user through the UI. This behaviour can be controlled with selection_change_events_by_code.
  explicit TreeControlStandalone(bool selection_change_events_by_code, bool has_checkboxes = false, bool multi_select = false) :
    selectionChangeEventsByCode(selection_change_events_by_code), hasCheckboxes(has_checkboxes), multiSelectionEnabled(multi_select)
  {
    resetMaximumSeenWidth();
  }

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

    return node->expanded;
  }

  void setExpanded(TLeafHandle leaf, bool open = true)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->expanded = open;
    resetMaximumSeenWidth();
  }

  void setExpandedRecursive(TLeafHandle leaf, bool open = true)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node || node->children.empty())
      return;

    node->expanded = open;
    resetMaximumSeenWidth();

    for (TreeNode *child : node->children)
      setExpandedRecursive(nodeAsLeafHandle(child), open);
  }

  void setExpandedTillRoot(TLeafHandle leaf, bool open = true)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    while (node && node != &rootNode)
    {
      node->expanded = open;
      node = node->parent;
      G_ASSERT(node);
    }

    resetMaximumSeenWidth();
  }

  void setUserData(TLeafHandle leaf, void *value)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->userData = value;
  }

  void setCheckboxState(TLeafHandle leaf, CheckboxState state)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->checkboxState = state;
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

  IconId getTexture(const char *name) { return image_helper.loadIcon(name); }

  // This function works specially for null leafs. If leaf == 0 then it returns the number of items at the topmost level (the internal
  // root's children count).
  // getChildCount(0) is different than calling getChildCount(getRootLeaf()), the latter equals to getChildCount(getChild(0, 0)).
  int getChildCount(TLeafHandle leaf) const
  {
    const TreeNode *node = leaf == nullptr ? &rootNode : leafHandleAsNode(leaf);
    return node->children.size();
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

    if (!node->children.empty())
      return nodeAsLeafHandle(node->children[0]);

    TreeNode *sibling = node->getNextSibling();
    if (sibling)
      return nodeAsLeafHandle(sibling);

    for (;;)
    {
      TreeNode *parent = node->parent;
      G_ASSERT(parent);
      if (parent == &rootNode)
        return getRootLeaf();

      TreeNode *siblingParent = parent->getNextSibling();
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

    TreeNode *sibling = node->getPrevSibling();
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

  // Returns with the first topmost level item.
  // If you want to get the number of topmost level items use getChildCount(nullptr), if you want to iterate over the
  // topmost level items use getChildLeaf(nullptr, ...) instead.
  TLeafHandle getRootLeaf() const { return nodeAsLeafHandle(rootNode.getFirstChild()); }

  // Visible means that the item's parents are expanded, and the item might be visible.
  // The functinon returns with 0 if there are no items. (Unlike getNextLeaf that returns with 0 or the root.)
  TLeafHandle getNextVisibleLeaf(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return 0;

    if (!node->children.empty() && node->expanded)
      return nodeAsLeafHandle(node->children[0]);

    TreeNode *sibling = node->getNextSibling();
    if (sibling)
      return nodeAsLeafHandle(sibling);

    for (;;)
    {
      TreeNode *parent = node->parent;
      G_ASSERT(parent);
      if (parent == &rootNode)
        return 0;

      TreeNode *siblingParent = parent->getNextSibling();
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

  CheckboxState getCheckboxState(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return CheckboxState::NoCheckbox;

    return node->checkboxState;
  }

  bool isLeafSelected(TLeafHandle leaf) const { return leafHandleAsNode(leaf) == lastSelected; }

  void setSelectedLeaf(TLeafHandle leaf, bool keep_selected = false)
  {
    TreeNode *node = leafHandleAsNode(leaf);

    if (multiSelectionEnabled)
    {
      setExpandedTillRoot(leaf);
      if (!keep_selected)
        selectAllVisibleItems(false);

      if (node)
        node->multiSelected = true;
    }
    else
    {
      if (node == lastSelected)
        return;

      setExpandedTillRoot(leaf);
    }

    lastSelected = node;
    ensureVisibleRequested = node;

    if (selectionChangeEventsByCode && eventHandler)
      eventHandler->onWcChange(windowBaseForEventHandler);
  }

  TLeafHandle getSelectedLeaf() const { return nodeAsLeafHandle(lastSelected); }

  bool isItemMultiSelected(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    return node && node->multiSelected;
  }

  // Visible means that the item's parents are expanded, and the item might be visible.
  // Returns true if the selection has been changed.
  bool selectAllVisibleItems(bool select = true)
  {
    G_ASSERT(multiSelectionEnabled);

    bool selectionChanged = false;

    for (TLeafHandle item = getRootLeaf(); item; item = getNextVisibleLeaf(item))
    {
      TreeNode *node = leafHandleAsNode(item);
      if (node->multiSelected != select)
      {
        node->multiSelected = select;
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
      if (node->multiSelected != select)
      {
        node->multiSelected = select;
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

  void getSelectedLeafs(dag::Vector<TLeafHandle> &leafs) const
  {
    if (multiSelectionEnabled)
    {
      for (TLeafHandle item = getRootLeaf(); item; item = getNextVisibleLeaf(item))
        if (isItemMultiSelected(item))
          leafs.push_back(item);
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

  // A message to display in the center of the tree.
  // For example "No results found" when searching in the tree.
  // If in_message is empty then nothing will be displayed.
  void setMessage(const char *in_message) { message = in_message; }

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

      ImGuiMultiSelectIO *multiSelectIo =
        ImGui::BeginMultiSelect(multiSelectionEnabled ? ImGuiMultiSelectFlags_None : ImGuiMultiSelectFlags_SingleSelect, -1, -1);
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

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 3.0f));
      ImGui::PushStyleColor(ImGuiCol_Header, getOverriddenColor(ColorOverride::TREE_SELECTION_BACKGROUND));
      fillTree(rootNode, doubleClickedOnItem, rightClickedOnItem, clickedOnStateIcon, collapsed_item, focusRequested,
        cursorScreenPosMaxX);
      ImGui::PopStyleColor();
      ImGui::PopStyleVar();

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

      if (contextMenu)
      {
        const bool open = contextMenu->updateImgui();
        if (!open)
          contextMenu.reset();
      }

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

private:
  struct TreeNode
  {
    ~TreeNode() { clear_all_ptr_items(children); }

    int getChildIndex(const TreeNode *child) const
    {
      auto it = eastl::find(children.begin(), children.end(), child);
      if (it == children.end())
        return -1;
      else
        return it - children.begin();
    }

    TreeNode *getFirstChild() const { return children.empty() ? nullptr : children[0]; }

    TreeNode *getPrevSibling() const
    {
      const int i = parent->getChildIndex(this);
      G_ASSERT(i >= 0);
      if (i >= 1)
        return parent->children[i - 1];
      return nullptr;
    }

    TreeNode *getNextSibling() const
    {
      const int i = parent->getChildIndex(this);
      G_ASSERT(i >= 0);
      if ((i + 1) < parent->children.size())
        return parent->children[i + 1];
      return nullptr;
    }

    TreeNode *parent = nullptr;
    dag::Vector<TreeNode *> children;

    String title;
    void *userData = nullptr;
    IconId icon = IconId::Invalid;
    IconId stateIcon = IconId::Invalid;
    E3DCOLOR textColor = E3DCOLOR(0);
    CheckboxState checkboxState = CheckboxState::NoCheckbox;
    bool expanded = false;
    bool multiSelected = false; // Only valid in multi selection mode.
  };

  static TreeNode *leafHandleAsNode(TLeafHandle handle) { return reinterpret_cast<TreeNode *>(handle); }

  static TLeafHandle nodeAsLeafHandle(TreeNode *node) { return reinterpret_cast<TLeafHandle>(node); }

  static TreeNode *getLastSubNode(TreeNode *item)
  {
    if (!item)
      return 0;

    TreeNode *child = item->getFirstChild();
    if (!child)
      return item;

    while (TreeNode *siblingChild = child->getNextSibling())
      child = siblingChild;

    return getLastSubNode(child);
  }

  void toggleSelectionCheckState()
  {
    const TLeafHandle caret = getSelectedLeaf();
    if (!caret)
      return;

    const TreeNode *caretNode = leafHandleAsNode(caret);
    if (caretNode->checkboxState == CheckboxState::NoCheckbox)
      return;

    const CheckboxState newState =
      caretNode->checkboxState == CheckboxState::Checked ? CheckboxState::Unchecked : CheckboxState::Checked;

    dag::Vector<TLeafHandle> leafs;
    getSelectedLeafs(leafs);
    for (TLeafHandle leaf : leafs)
    {
      TreeNode *node = leafHandleAsNode(leaf);
      if (node->checkboxState == CheckboxState::NoCheckbox)
        continue;
      node->checkboxState = newState;
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
            lastSelected = newSelection;
            selectionChanged = true;
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

    for (TreeNode *child = node->getFirstChild(); child; child = child->getNextSibling())
    {
      if (child->multiSelected)
      {
        child->multiSelected = false;
        selectionChanged = true;

        if (child == lastSelected)
          lastSelected = nullptr;
      }

      if (child->expanded)
        selectionChanged |= deselectAllVisibleChildren(child);
    }

    return selectionChanged;
  }

  void fillTree(TreeNode &parent_node, bool &double_clicked_on_item, bool &right_clicked_on_item, bool &clicked_on_state_icon,
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
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), posMin, posMax, posMax.x, posMax.x, message, nullptr, &textSize);
  }

  void resetMaximumSeenWidth() { maximumSeenWidth = 0.0f; }

  friend class TreeHierarchyLineDrawer<TreeNode>;

  const bool selectionChangeEventsByCode;
  const bool hasCheckboxes;
  const bool multiSelectionEnabled;
  WindowControlEventHandler *eventHandler = nullptr;
  WindowBase *windowBaseForEventHandler = nullptr;
  bool controlEnabled = true;
  TreeNode rootNode;
  TreeNode *lastSelected = nullptr;
  TreeNode *ensureVisibleRequested = nullptr;
  IconId checkedIcon = IconId::Invalid;
  IconId uncheckedIcon = IconId::Invalid;
  eastl::unique_ptr<ContextMenu> contextMenu;
  String message;
  float maximumSeenWidth;
};

} // namespace PropPanel