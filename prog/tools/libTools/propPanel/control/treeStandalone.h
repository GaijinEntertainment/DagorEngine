// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../contextMenuInternal.h"
#include "../imageHelper.h"
#include "../scopedImguiBeginDisabled.h"
#include <propPanel/control/treeInterface.h>
#include <propPanel/c_common.h> // TLeafHandle
#include <propPanel/c_window_event_handler.h>
#include <propPanel/constants.h>
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>
#include <dag/dag_vector.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <winGuiWrapper/wgw_input.h>

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
  {}

  void setEventHandler(WindowControlEventHandler *event_handler) { eventHandler = event_handler; }

  // The provided window_base is passed to the event handler events (set with setEventHandler). This class does not use
  // it otherwise.
  void setWindowBaseForEventHandler(WindowBase *window_base) { windowBaseForEventHandler = window_base; }

  bool isEnabled() const { return controlEnabled; }
  void setEnabled(bool enabled) { controlEnabled = enabled; }

  void setFocus() { focus_helper.requestFocus(this); }

  void setCheckboxIcons(TEXTUREID checked, TEXTUREID unchecked)
  {
    checkedIcon = ImageHelper::TextureIdToImTextureId(checked);
    uncheckedIcon = ImageHelper::TextureIdToImTextureId(unchecked);
  }

  TLeafHandle createTreeLeaf(TLeafHandle parent, const char caption[], TEXTUREID icon, void *user_data = nullptr)
  {
    TreeNode *parentNode = leafHandleAsNode(parent);
    if (!parentNode)
      parentNode = &rootNode;

    TreeNode *newNode = new TreeNode();
    newNode->title = caption;
    newNode->icon = ImageHelper::TextureIdToImTextureId(icon);
    newNode->parent = parentNode;
    newNode->userData = user_data;
    parentNode->children.push_back(newNode);

    return nodeAsLeafHandle(newNode);
  }

  TLeafHandle createTreeLeaf(TLeafHandle parent, const char caption[], const char icon_name[], void *user_data = nullptr)
  {
    TEXTUREID icon = getTexture(icon_name);
    return createTreeLeaf(parent, caption, icon, user_data);
  }

  TLeafHandle createTreeLeafAsFirst(TLeafHandle parent, const char caption[], TEXTUREID icon, void *user_data = nullptr)
  {
    TreeNode *parentNode = leafHandleAsNode(parent);
    if (!parentNode)
      parentNode = &rootNode;

    TreeNode *newNode = new TreeNode();
    newNode->title = caption;
    newNode->icon = ImageHelper::TextureIdToImTextureId(icon);
    newNode->parent = parentNode;
    newNode->userData = user_data;
    parentNode->children.insert(parentNode->children.begin(), newNode);

    return nodeAsLeafHandle(newNode);
  }

  TLeafHandle createTreeLeafAsFirst(TLeafHandle parent, const char caption[], const char icon_name[], void *user_data = nullptr)
  {
    TEXTUREID icon = getTexture(icon_name);
    return createTreeLeafAsFirst(parent, caption, icon, user_data);
  }

  void clear()
  {
    clear_all_ptr_items(rootNode.children);
    lastSelected = nullptr;
    ensureVisibleRequested = nullptr;

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

    node->icon = ImageHelper::TextureIdToImTextureId(getTexture(fname));
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
  }

  void setExpandedRecursive(TLeafHandle leaf, bool open = true)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node || node->children.empty())
      return;

    node->expanded = open;

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

  void setIcon(TLeafHandle leaf, TEXTUREID icon)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->icon = ImageHelper::TextureIdToImTextureId(icon);
  }

  void setStateIcon(TLeafHandle leaf, TEXTUREID icon)
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return;

    node->stateIcon = ImageHelper::TextureIdToImTextureId(icon);
  }

  int getChildCount(TLeafHandle leaf) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node)
      return 0;

    return node->children.size();
  }

  TEXTUREID getTexture(const char *name) { return image_helper.loadIcon(name); }

  TLeafHandle getChildLeaf(TLeafHandle leaf, unsigned idx) const
  {
    TreeNode *node = leafHandleAsNode(leaf);
    if (!node || idx >= node->children.size())
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

  virtual IMenu &createContextMenu() override
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
      ImGui::PushStyleColor(ImGuiCol_Header, Constants::TREE_SELECTION_BACKGROUND_COLOR);
      fillTree(rootNode, doubleClickedOnItem, rightClickedOnItem, clickedOnStateIcon, collapsed_item, focusRequested,
        cursorScreenPosMaxX);
      ImGui::PopStyleColor();
      ImGui::PopStyleVar();

      ImGui::EndMultiSelect();
      if (multiSelectIo)
        selectionChanged |= applySelectionRequests(*multiSelectIo);

      if (collapsed_item && multiSelectionEnabled)
        selectionChanged |= deselectAllVisibleChildren(collapsed_item);

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
      ImGui::SetCursorScreenPos(ImVec2(cursorScreenPosMaxX, ImGui::GetCursorScreenPos().y));
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
    ImTextureID icon = nullptr;
    ImTextureID stateIcon = nullptr;
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

  static void drawTreeOpenCloseIcon(ImDrawList &draw_list, const ImVec2 &center, float rectangle_radius, bool open,
    ImU32 tree_line_color, ImU32 open_close_icon_inner_color)
  {
    const float r = rectangle_radius;
    draw_list.AddRect(ImVec2(center.x - r, center.y - r), ImVec2(center.x + r + 1.0f, center.y + r + 1.0f), tree_line_color);

    const float r2 = r - (r > 4.0f ? hdpi::_pxS(2) : 1); // hdpi::_pxS(1) did not work well here
    draw_list.AddLine(ImVec2(center.x - r2 + 1.0f, center.y), ImVec2(center.x + r2, center.y), open_close_icon_inner_color);
    if (!open)
      draw_list.AddLine(ImVec2(center.x, center.y - r2 + 1.0f), ImVec2(center.x, center.y + r2), open_close_icon_inner_color);
  }

  void drawTreeLine(const TreeNode &node, ImDrawList &draw_list, const ImRect &item_rect, float icon_or_text_start_x, bool open,
    ImU32 tree_line_color, ImU32 open_close_icon_inner_color)
  {
    const bool hasPreviousSibling = &node != node.parent->children.front();
    const bool hasNextSibling = &node != node.parent->children.back();
    const float treeNodeToLabelSpacing = ImGui::GetTreeNodeToLabelSpacing();
    const float indentSpacing = ImGui::GetStyle().IndentSpacing;
    const float centerX = floorf(item_rect.Min.x + (ImGui::GetStyle().FramePadding.x * 3.0f));
    const float centerY = floorf((item_rect.Min.y + item_rect.Max.y) / 2.0f);
    const float rectangleRadius = floorf(draw_list._Data->FontSize * 0.30f);
    const float itemSpacingY = ImGui::GetStyle().ItemSpacing.y;
    const float itemTopMinusSpacing = item_rect.Min.y - itemSpacingY;
    const float itemBottomPlusSpacing = item_rect.Max.y + itemSpacingY;
    const float horizontalLineRight = icon_or_text_start_x - hdpi::_pxS(3);

    if (node.children.empty())
    {
      draw_list.AddLine(ImVec2(centerX, centerY), ImVec2(horizontalLineRight, centerY), tree_line_color);

      const float verticalLineTop = hasPreviousSibling ? item_rect.Min.y : itemTopMinusSpacing;
      const float verticalLineBottom = hasNextSibling ? itemBottomPlusSpacing : centerY;
      draw_list.AddLine(ImVec2(centerX, verticalLineTop), ImVec2(centerX, verticalLineBottom), tree_line_color);
    }
    else
    {
      if (node.parent != &rootNode)
      {
        const float verticalLineTop = hasPreviousSibling ? item_rect.Min.y : itemTopMinusSpacing;
        draw_list.AddLine(ImVec2(centerX, verticalLineTop), ImVec2(centerX, centerY - rectangleRadius), tree_line_color);
      }

      drawTreeOpenCloseIcon(draw_list, ImVec2(centerX, centerY), rectangleRadius, open, tree_line_color, open_close_icon_inner_color);

      draw_list.AddLine(ImVec2(centerX + rectangleRadius, centerY), ImVec2(horizontalLineRight, centerY), tree_line_color);

      if (hasNextSibling)
        draw_list.AddLine(ImVec2(centerX, centerY + rectangleRadius), ImVec2(centerX, itemBottomPlusSpacing), tree_line_color);
    }

    float currentTreeLineCenterX = centerX;
    for (const TreeNode *currentNode = node.parent; currentNode != &rootNode; currentNode = currentNode->parent)
    {
      currentTreeLineCenterX -= indentSpacing; // Do it before the first draw because we started at the parent.

      const bool currentHasNextSibling = currentNode != currentNode->parent->children.back();
      if (currentHasNextSibling)
      {
        const float verticalLineBottom = currentHasNextSibling ? itemBottomPlusSpacing : centerY;
        draw_list.AddLine(ImVec2(currentTreeLineCenterX, item_rect.Min.y), ImVec2(currentTreeLineCenterX, verticalLineBottom),
          tree_line_color);
      }
    }
  }

  void fillTree(TreeNode &parent_node, bool &double_clicked_on_item, bool &right_clicked_on_item, bool &clicked_on_state_icon,
    TreeNode *&collapsed_item, bool focus_selected, float &cursor_screen_pos_max_x)
  {
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    ImGuiWindow *window = ImGui::GetCurrentWindow();

    for (TreeNode *node : parent_node.children)
    {
      // SetNextItemOpen does nothing if SkipItems is set, so to avoid unintended openness change we do nothing too.
      if (window->SkipItems)
        break;

      const bool hasChild = node->children.size() != 0;
      const bool wasSelected = multiSelectionEnabled ? node->multiSelected : node == lastSelected;
      ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

      if (!hasChild)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

      if (wasSelected)
      {
        flags |= ImGuiTreeNodeFlags_Selected;

        if (focus_selected)
          ImGui::SetKeyboardFocusHere();
      }

      ImGui::SetNextItemOpen(node->expanded);

      G_STATIC_ASSERT(sizeof(ImGuiSelectionUserData) == sizeof(TreeNode *));
      ImGui::SetNextItemSelectionUserData((ImGuiSelectionUserData)node);

      bool clickedOnThisStateIcon = false;

      ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData endData;
      const bool isOpen = ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(node->title, flags, endData,
        /*allow_blocked_hover = */ false, /*show_arrow = */ false);

      if (endData.draw)
      {
        const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::ImguiHelper::getFontSizedIconSize();
        const float iconWidthWithSpacing = fontSizedIconSize.x + ImGui::GetStyle().ItemInnerSpacing.x;
        const float iconOrTextStartX = endData.textPos.x;
        ImVec2 iconPos = endData.textPos;

        ImTextureID stateIcon = node->stateIcon;
        if (hasCheckboxes)
        {
          if (node->checkboxState == CheckboxState::Checked)
            stateIcon = checkedIcon;
          else if (node->checkboxState == CheckboxState::Unchecked)
            stateIcon = uncheckedIcon;
        }

        if (stateIcon)
          endData.textPos.x += iconWidthWithSpacing;
        if (node->icon)
          endData.textPos.x += iconWidthWithSpacing;

        const bool hasColor = node->textColor != E3DCOLOR(0);
        if (hasColor)
          ImGui::PushStyleColor(ImGuiCol_Text, node->textColor);

        PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(endData);

        if (hasColor)
          ImGui::PopStyleColor();

        if (stateIcon)
        {
          const ImVec2 iconRectRightBottom(iconPos.x + fontSizedIconSize.x, iconPos.y + fontSizedIconSize.y);
          window->DrawList->AddImage(stateIcon, iconPos, iconRectRightBottom);

          clickedOnThisStateIcon = endData.hovered && ImGui::IsItemClicked(ImGuiMouseButton_Left) &&
                                   ImRect(iconPos, iconRectRightBottom).Contains(ImGui::GetMousePos());
          iconPos.x += iconWidthWithSpacing;
        }

        if (node->icon)
        {
          const ImVec2 iconRectRightBottom(iconPos.x + fontSizedIconSize.x, iconPos.y + fontSizedIconSize.y);
          window->DrawList->AddImage(node->icon, iconPos, iconRectRightBottom);
        }

        const ImRect itemRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        drawTreeLine(*node, *drawList, itemRect, iconOrTextStartX, isOpen, Constants::TREE_HIERARCHY_LINE_COLOR,
          Constants::TREE_OPEN_CLOSE_ICON_INNER_COLOR);

        const float currentMaxX = endData.textPos.x + endData.labelSize.x;
        if (currentMaxX > cursor_screen_pos_max_x)
          cursor_screen_pos_max_x = currentMaxX;
      }

      if (node->expanded != isOpen)
      {
        node->expanded = isOpen;

        if (!isOpen)
        {
          G_ASSERT(!collapsed_item);
          collapsed_item = node;
        }
      }

      if (ImGui::IsItemHovered())
      {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
          double_clicked_on_item = true;
        // Imitate Windows where the context menu comes up on mouse button release. (BeginPopupContextItem also does this.)
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
          right_clicked_on_item = true;
      }

      if (clickedOnThisStateIcon && hasCheckboxes && node->checkboxState != CheckboxState::NoCheckbox)
      {
        node->checkboxState = node->checkboxState == CheckboxState::Checked ? CheckboxState::Unchecked : CheckboxState::Checked;
        clicked_on_state_icon = true;
      }

      if (node == ensureVisibleRequested)
      {
        ensureVisibleRequested = nullptr;
        ImGui::SetScrollHereY();
      }

      if (isOpen && hasChild)
      {
        fillTree(*node, double_clicked_on_item, right_clicked_on_item, clicked_on_state_icon, collapsed_item, focus_selected,
          cursor_screen_pos_max_x);
        ImGui::TreePop();
      }
    }
  }

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

  const bool selectionChangeEventsByCode;
  const bool hasCheckboxes;
  const bool multiSelectionEnabled;
  WindowControlEventHandler *eventHandler = nullptr;
  WindowBase *windowBaseForEventHandler = nullptr;
  bool controlEnabled = true;
  TreeNode rootNode;
  TreeNode *lastSelected = nullptr;
  TreeNode *ensureVisibleRequested = nullptr;
  ImTextureID checkedIcon = nullptr;
  ImTextureID uncheckedIcon = nullptr;
  eastl::unique_ptr<ContextMenu> contextMenu;
  String message;
};

} // namespace PropPanel