//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <imgui/imgui.h>
#include "propPanel/c_common.h"
#include "propPanel/imguiHelper.h"
#include <EASTL/optional.h>

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

// The parts of one tree row. The parts can be drawn in any order, skipped or replaced.
// ITreeRenderEx::drawTreeNode() can also draw ImGui widgets. Draw them after drawLabel(), because the tooltip of a
// clipped label tests the hover on the last item. Return true from ITreeRenderEx::hasRowWidgets(), so that the widgets
// do not change the hover and click handling of the row.
class TreeNodeDrawContext
{
public:
  // The row state.

  TLeafHandle getHandle() const { return reinterpret_cast<TLeafHandle>(node); }

  // When ITreeRenderEx::hasRowWidgets() returns true, also true when the mouse is over a widget of the row.
  // False while a drag and drop target is between this row and its neighbour.
  bool isHovered() const { return endData.hovered; }

  bool isSelected() const { return endData.selected; }

  // The row itself, without the vertical ItemSpacing that the highlight, hover and drop area add.
  const ImRect &getRowRect() const { return endData.rowBB; }

  // The draw values. They start with the node's values. Set a value before the draw function that uses it.

  // The state icon is used by drawIcons(). It also moves the icon and the label, so set it before drawIcons() and drawLabel().
  // If set to IconId::Invalid then the icon is not drawn.
  // On a row with an enabled check box it starts as the checked or unchecked icon, not the node's state icon.
  // The tree reads it back after drawTreeNode(). A click toggles the check box only while it is not IconId::Invalid.
  IconId getStateIcon() const { return stateIcon; }
  void setStateIcon(IconId icon_id) { stateIcon = icon_id; }

  // The icon is used by drawIcons(). It also moves the label, so set it before drawIcons() and drawLabel().
  // If set to IconId::Invalid then the icon is not drawn.
  IconId getIcon() const { return icon; }
  void setIcon(IconId icon_id) { icon = icon_id; }

  // The label is used by drawLabel(). text/text_end must stay valid until drawLabel() returns.
  // If text_end is nullptr then label ends at '\0'.
  void setLabel(const char *text, const char *text_end = nullptr);

  // The label starts as the row's label, and a setLabel() call replaces it.
  // The pointers are valid only while that text is valid: the row's label during drawTreeNode(), or the text given to setLabel().
  const char *getLabel() const { return endData.label; }
  const char *getLabelEnd() const { return endData.labelEnd; }

  // The text color is used by drawLabel(). E3DCOLOR(0) uses the style color.
  E3DCOLOR getTextColor() const { return textColor; }
  void setTextColor(E3DCOLOR color) { textColor = color; }

  // Used by drawBackground() when the row is not hovered/selected.
  eastl::optional<E3DCOLOR> getNormalBackgroundColor() const { return normalBackgroundColor; }
  void setNormalBackgroundColor(eastl::optional<E3DCOLOR> color) { normalBackgroundColor = color; }

  // The draw functions. drawDefault() calls the others in this order.

  // A hovered or selected row uses ImGuiCol_HeaderHovered, ImGuiCol_HeaderActive (held) or ImGuiCol_Header (selected).
  // To change these colors, push them with ImGui::PushStyleColor() around this call.
  // drawBackground() also draws the keyboard navigation cursor of the row.
  void drawBackground();

  // Draws the hierarchy lines and the open/close icon of the row.
  void drawHierarchyLines();

  void drawIcons();

  // clip_max_x: screen space X coordinate where the label is clipped. It is not a width.
  //   nullptr means the right edge of the content region.
  void drawLabel(const float *clip_max_x = nullptr);

  void drawDefault();

  // The queries. They use the current draw values.

  // The screen position of the label.
  ImVec2 getLabelPos() const;

  // Uses the clip of the last drawLabel() call, or the default clip.
  bool isLabelClipped() const;

private:
  TreeNodeDrawContext(ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data, TreeNode &tree_node, IconId state_icon_id);

  ImVec2 getIconPos() const;

  // Returns false if the state icon is IconId::Invalid.
  bool getStateIconRect(ImRect &rect) const;

  ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &endData;
  TreeNode *const node;
  const ImVec2 iconOrTextStartPos;
  float labelClipMaxX;
  IconId stateIcon;
  IconId icon;
  E3DCOLOR textColor;
  eastl::optional<E3DCOLOR> normalBackgroundColor;

  friend class TreeControlStandalone;
};

class ITreeRenderEx
{
public:
  // The returned color is used as the frame border of the whole tree.
  virtual eastl::optional<E3DCOLOR> getBorderColor() const { return eastl::nullopt; }

  // Return true if drawTreeNode() draws ImGui widgets (for example image buttons) over the row.
  virtual bool hasRowWidgets() const { return false; }

  // The tree restores the ImGui cursor after this call.
  virtual void drawTreeNode(TreeNodeDrawContext &ctx) { ctx.drawDefault(); }
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