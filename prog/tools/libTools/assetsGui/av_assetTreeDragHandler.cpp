// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assetsGui/av_assetTreeDragHandler.h>
#include <assetsGui/av_allAssetsTree.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/control/treeNode.h>
#include <propPanel/propPanel.h>

#include <imgui/imgui.h>

void AssetTreeDragHandler::onBeginDrag(PropPanel::TLeafHandle leaf)
{
  if (tree == nullptr)
    return;

  const PropPanel::TreeNode *tree_node = tree->getItemNode(leaf);
  const PropPanel::IconId icon = tree_node->getIconId();

  AssetDragData dragData;
  dragData.folder = get_dagor_asset_folder(tree_node->getUserData());
  dragData.asset = dragData.folder ? nullptr : static_cast<DagorAsset *>(tree_node->getUserData());

  ImGui::SetDragDropPayload(ASSET_DRAG_AND_DROP_TYPE, &dragData, sizeof(dragData));

  if (icon != PropPanel::IconId::Invalid)
  {
    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::ImguiHelper::getFontSizedIconSize();

    ImGui::Image(PropPanel::get_im_texture_id_from_icon_id(icon), fontSizedIconSize);
    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
  }
  ImGui::Text("%s", tree_node->getTitle().c_str());
}
