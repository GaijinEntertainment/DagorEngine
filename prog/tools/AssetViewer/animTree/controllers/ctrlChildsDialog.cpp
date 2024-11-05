// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ctrlChildsDialog.h"
#include "../animTreeUtils.h"
#include "../animTreeIcons.h"
#include "../blendNodes/blendNodeType.h"

#include "hub.h"
#include "randomSwitch.h"
#include "paramSwitch.h"
#include "linearPoly.h"

#include <imgui/imgui.h>
#include <propPanel/control/container.h>
#include <ioSys/dag_dataBlock.h>
#include <assets/asset.h>

enum
{
  PID_CHILDS_TREE
};

CtrlChildsDialog::CtrlChildsDialog(dag::Vector<AnimCtrlData> &controllers, dag::Vector<BlendNodeData> &nodes,
  dag::Vector<String> &paths) :
  controllers{controllers},
  nodes{nodes},
  paths{paths},
  DialogWindow(nullptr, hdpi::_pxScaled(DEFAULT_MINIMUM_WIDTH), hdpi::_pxScaled(DEFAULT_MINIMUM_HEIGHT), "Controller childs tree")
{
  setManualModalSizingEnabled();
  DialogWindow::showButtonPanel(false);
  initPanel();
}

void CtrlChildsDialog::initPanel()
{
  PropPanel::ContainerPropertyControl *panel = DialogWindow::getPanel();
  panel->createTree(PID_CHILDS_TREE, "", hdpi::Px(300));
}

void CtrlChildsDialog::clear()
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_CHILDS_TREE)->getContainer();
  tree->clear();
  ctrlsTree = nullptr;
  nodesTree = nullptr;
  asset = nullptr;
}

void CtrlChildsDialog::setTreePanels(PropPanel::ContainerPropertyControl *ctrls_tree, PropPanel::ContainerPropertyControl *nodes_tree,
  DagorAsset *cur_asset)
{
  ctrlsTree = ctrls_tree;
  nodesTree = nodes_tree;
  asset = cur_asset;
}

void CtrlChildsDialog::fillChildsTree(PropPanel::TLeafHandle leaf)
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_CHILDS_TREE)->getContainer();
  tree->clear();
  AnimCtrlData *leafData = find_data_by_handle(controllers, leaf);
  if (leafData != controllers.end())
    fillChilds(tree, nullptr, *leafData);
}

static bool check_leaf_not_looping(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle parent, const char *name)
{
  // In this case we check root leaf
  if (!parent)
    return true;

  PropPanel::TLeafHandle root = tree->getRootLeaf();
  if (tree->getCaption(root) == name)
    return false;

  while (parent && parent != root)
  {
    if (tree->getCaption(parent) == name)
      return false;

    parent = tree->getParentLeaf(parent);
  }

  return true;
}

void CtrlChildsDialog::fillChilds(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle parent, AnimCtrlData &leaf_data)
{
  String name = ctrlsTree->getCaption(leaf_data.handle);
  PropPanel::TLeafHandle leaf = tree->createTreeLeaf(parent, name, get_ctrl_icon_name(leaf_data.type));
  if (!check_leaf_not_looping(tree, parent, name))
  {
    PropPanel::TLeafHandle root = tree->getRootLeaf();
    logerr("Loop detected in controller <%s> childs. Leaf <%s> in parent <%s> was found recursively.", tree->getCaption(root), name,
      tree->getCaption(parent));
    tree->setButtonPictures(leaf, LEAF_WITH_LOOP);
    return;
  }

  tree->setBool(leaf, /*open*/ true);
  for (int i = 0; i < leaf_data.childs.size(); ++i)
  {
    const int idx = leaf_data.childs[i];
    AnimCtrlData *ctrlChild =
      eastl::find_if(controllers.begin(), controllers.end(), [idx](const AnimCtrlData &data) { return idx == data.id; });
    if (ctrlChild != controllers.end())
      fillChilds(tree, leaf, *ctrlChild);
    else
    {
      BlendNodeData *nodeChild =
        eastl::find_if(nodes.begin(), nodes.end(), [idx](const BlendNodeData &data) { return idx == data.id; });
      if (nodeChild != nodes.end())
        tree->createTreeLeaf(leaf, nodesTree->getCaption(nodeChild->handle), get_blend_node_icon_name(nodes[idx].type));
      else
      {
        PropPanel::TLeafHandle includeLeaf = ctrlsTree->getParentLeaf(leaf_data.handle);
        String fullPath;
        DataBlock props = get_props_from_include_leaf(paths, *asset, ctrlsTree, includeLeaf, fullPath, /*only_includes*/ true);
        if (DataBlock *ctrlProps = get_props_from_include_leaf_ctrl_node(controllers, paths, *asset, props, ctrlsTree, includeLeaf))
        {
          if (DataBlock *settings = find_block_by_name(ctrlProps, ctrlsTree->getCaption(leaf_data.handle)))
          {
            const char *childName = getChildNameFromSettings(*settings, leaf_data.type, i);
            CtrlChildSearchResult result =
              find_ctrl_child_idx_and_icon_by_name(ctrlsTree, nodesTree, leaf_data, controllers, nodes, childName);
            if (result.id != -1)
            {
              tree->createTreeLeaf(leaf, childName, result.iconName);
              leaf_data.childs[i] = result.id;
            }
            else
            {
              String leafName(0, "(not found) %s", childName);
              tree->createTreeLeaf(leaf, leafName.c_str(), nullptr);
            }
          }
          else
            tree->createTreeLeaf(leaf, "not found", nullptr);
        }
      }
    }
  }
}

const char *CtrlChildsDialog::getChildNameFromSettings(const DataBlock &settings, CtrlType type, int idx)
{
  switch (type)
  {
    case ctrl_type_hub: return hub_get_child_name_by_idx(settings, idx);
    case ctrl_type_randomSwitch: return random_switch_get_child_name_by_idx(settings, idx);
    case ctrl_type_paramSwitch: return param_switch_get_child_name_by_idx(settings, idx);
    case ctrl_type_linearPoly: return linear_poly_get_child_name_by_idx(settings, idx);
  }

  return nullptr;
}

void CtrlChildsDialog::updateImguiDialog()
{
  PropPanel::ContainerPropertyControl *tree = getPanel()->getById(PID_CHILDS_TREE)->getContainer();
  const float treeHeight = ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y;
  tree->setHeight(hdpi::Px(treeHeight));

  DialogWindow::updateImguiDialog();
}
