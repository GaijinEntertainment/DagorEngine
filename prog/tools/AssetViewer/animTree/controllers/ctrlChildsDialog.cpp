// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ctrlChildsDialog.h"
#include "../animTreeUtils.h"
#include "../animTreeIcons.h"
#include "../animTreePanelPids.h"
#include "../blendNodes/blendNodeType.h"

#include "hub.h"
#include "randomSwitch.h"
#include "paramSwitch.h"
#include "linearPoly.h"

#include <imgui/imgui.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/container.h>
#include <ioSys/dag_dataBlock.h>
#include <assets/asset.h>

enum
{
  PID_CHILDS_TREE
};

namespace ContextMenu
{
enum
{
  EDIT = 0,
  EDIT_IN_PARENT,
};
}

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

bool CtrlChildsDialog::onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
  PropPanel::ITreeInterface &tree_interface)
{
  if (pcb_id == PID_CHILDS_TREE)
  {
    PropPanel::TLeafHandle selectedLeaf = tree.getSelLeaf();
    String name = tree.getCaption(selectedLeaf);
    if (strstr(name.c_str(), "not found"))
      return false;

    PropPanel::IMenu &menu = tree_interface.createContextMenu();
    menu.addItem(ROOT_MENU_ITEM, ContextMenu::EDIT, "Edit");
    if (selectedLeaf != tree.getRootLeaf())
      menu.addItem(ROOT_MENU_ITEM, ContextMenu::EDIT_IN_PARENT, "Edit in parent");
    menu.setEventHandler(this);
    return true;
  }
  return false;
}

int CtrlChildsDialog::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::EDIT: editSelectedNode(); break;
    case ContextMenu::EDIT_IN_PARENT: editInParentSelectedNode(); break;
  }
  return 0;
}

void CtrlChildsDialog::initPanel()
{
  PropPanel::ContainerPropertyControl *panel = DialogWindow::getPanel();
  panel->createTree(PID_CHILDS_TREE, "", hdpi::Px(0));
}

void CtrlChildsDialog::clear()
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_CHILDS_TREE)->getContainer();
  tree->clear();
  ctrlsTree = nullptr;
  nodesTree = nullptr;
  pluginPanel = nullptr;
  pluginEventHandler = nullptr;
  asset = nullptr;
}

void CtrlChildsDialog::setTreePanels(PropPanel::ContainerPropertyControl *panel, DagorAsset *cur_asset,
  PropPanel::ControlEventHandler *event_handler)
{
  pluginPanel = panel;
  pluginEventHandler = event_handler;
  ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  nodesTree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  asset = cur_asset;
}

void CtrlChildsDialog::fillChildsTree(PropPanel::TLeafHandle leaf)
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_CHILDS_TREE)->getContainer();
  tree->clear();
  tree->setTreeEventHandler(this);
  AnimCtrlData *leafData = find_data_by_handle(controllers, leaf);
  if (leafData != controllers.end())
    fillChilds(tree, nullptr, *leafData, String(""));
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

String CtrlChildsDialog::getImportantParamPrefixName(const DataBlock &settings, int child_idx, CtrlType type)
{
  String out;
  switch (type)
  {
    case ctrl_type_randomSwitch: return random_switch_get_child_prefix_name(settings, child_idx);
    case ctrl_type_paramSwitch: return param_switch_get_child_prefix_name(settings, child_idx);
    case ctrl_type_linearPoly: return linear_poly_get_child_prefix_name(settings, child_idx);
  }

  return out;
}

static bool is_contain_important_param(CtrlType type)
{
  return type == ctrl_type_randomSwitch || type == ctrl_type_paramSwitch || type == ctrl_type_linearPoly;
}

void CtrlChildsDialog::fillChilds(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle parent, AnimCtrlData &leaf_data,
  const String &prefix_name)
{
  String name = String(0, "%s%s", prefix_name, ctrlsTree->getCaption(leaf_data.handle));
  PropPanel::TLeafHandle leaf = tree->createTreeLeaf(parent, name, get_ctrl_icon_name(leaf_data.type));
  if (!check_leaf_not_looping(tree, parent, name))
  {
    PropPanel::TLeafHandle root = tree->getRootLeaf();
    logerr("Loop detected in controller <%s> childs. Leaf <%s> in parent <%s> was found recursively.", tree->getCaption(root), name,
      tree->getCaption(parent));
    tree->setButtonPictures(leaf, LEAF_WITH_LOOP);
    return;
  }

  PropPanel::TLeafHandle includeLeaf = ctrlsTree->getParentLeaf(leaf_data.handle);
  DataBlock props;
  DataBlock *settings = nullptr;
  if (is_contain_important_param(leaf_data.type))
  {
    String fullPath;
    props = get_props_from_include_leaf(paths, *asset, ctrlsTree, includeLeaf, fullPath, /*only_includes*/ true);
    DataBlock *ctrlProps = get_props_from_include_leaf_ctrl_node(controllers, paths, *asset, props, ctrlsTree, includeLeaf);
    settings = find_block_by_name(ctrlProps, ctrlsTree->getCaption(leaf_data.handle));
  }
  tree->setBool(leaf, /*open*/ true);
  for (int i = 0; i < leaf_data.childs.size(); ++i)
  {
    String prefixChildName;
    if (settings)
      prefixChildName = getImportantParamPrefixName(*settings, i, leaf_data.type);
    const int idx = leaf_data.childs[i];
    AnimCtrlData *ctrlChild =
      eastl::find_if(controllers.begin(), controllers.end(), [idx](const AnimCtrlData &data) { return idx == data.id; });
    if (ctrlChild != controllers.end())
      fillChilds(tree, leaf, *ctrlChild, prefixChildName);
    else
    {
      BlendNodeData *nodeChild =
        eastl::find_if(nodes.begin(), nodes.end(), [idx](const BlendNodeData &data) { return idx == data.id; });
      if (nodeChild != nodes.end())
        tree->createTreeLeaf(leaf, String(0, "%s%s", prefixChildName, nodesTree->getCaption(nodeChild->handle)),
          get_blend_node_icon_name(nodeChild->type));
      else
      {
        if (!settings)
        {
          String fullPath;
          props = get_props_from_include_leaf(paths, *asset, ctrlsTree, includeLeaf, fullPath, /*only_includes*/ true);
          DataBlock *ctrlProps = get_props_from_include_leaf_ctrl_node(controllers, paths, *asset, props, ctrlsTree, includeLeaf);
          settings = find_block_by_name(ctrlProps, ctrlsTree->getCaption(leaf_data.handle));
        }
        const char *childName = getChildNameFromSettings(*settings, leaf_data.type, i);
        CtrlChildSearchResult result =
          find_ctrl_child_idx_and_icon_by_name(ctrlsTree, nodesTree, leaf_data, controllers, nodes, childName);
        if (result.id != -1)
        {
          tree->createTreeLeaf(leaf, String(0, "%s%s", prefixChildName, childName), result.iconName);
          leaf_data.childs[i] = result.id;
        }
        else
          tree->createTreeLeaf(leaf, String(0, "(not found) %s%s", prefixChildName, childName), nullptr);
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

static const char *get_name_for_search(const char *leaf_name)
{
  const char *name = strstr(leaf_name, "] ");
  if (name)
    name += 2; // Skip "[optional_hint] " part
  else
    name = leaf_name;

  return name;
}

void CtrlChildsDialog::editSelectedNode()
{
  PropPanel::ContainerPropertyControl *tree = DialogWindow::getPanel()->getById(PID_CHILDS_TREE)->getContainer();
  String leafName = tree->getCaption(tree->getSelLeaf());
  const char *name = get_name_for_search(leafName.c_str());

  AnimCtrlData *ctrlData = eastl::find_if(controllers.begin(), controllers.end(),
    [ctrlsTree = ctrlsTree, name](const AnimCtrlData &data) { return ctrlsTree->getCaption(data.handle) == name; });
  if (ctrlData != controllers.end())
  {
    // Check if group minimized and open it
    if (pluginPanel->getBool(PID_ANIM_BLEND_CTRLS_GROUP))
      pluginPanel->setBool(PID_ANIM_BLEND_CTRLS_GROUP, false);
    ctrlsTree->setSelLeaf(ctrlData->handle);
    PropPanel::focus_helper.requestFocus(pluginPanel->getById(PID_ANIM_BLEND_CTRLS_GROUP)->getContainer());
    pluginEventHandler->onChange(PID_ANIM_BLEND_CTRLS_TREE, pluginPanel);
  }
  else
  {
    BlendNodeData *nodeData = eastl::find_if(nodes.begin(), nodes.end(),
      [nodesTree = nodesTree, name](const BlendNodeData &data) { return nodesTree->getCaption(data.handle) == name; });
    if (nodeData != nodes.end())
    {
      // Check if group minimized and open it
      if (pluginPanel->getBool(PID_ANIM_BLEND_NODES_GROUP))
        pluginPanel->setBool(PID_ANIM_BLEND_NODES_GROUP, false);
      nodesTree->setSelLeaf(nodeData->handle);
      PropPanel::focus_helper.requestFocus(pluginPanel->getById(PID_ANIM_BLEND_NODES_GROUP)->getContainer());
      pluginEventHandler->onChange(PID_ANIM_BLEND_NODES_TREE, pluginPanel);
    }
  }
}

void CtrlChildsDialog::editInParentSelectedNode()
{
  PropPanel::ContainerPropertyControl *tree = DialogWindow::getPanel()->getById(PID_CHILDS_TREE)->getContainer();
  PropPanel::TLeafHandle selectedLeaf = tree->getSelLeaf();
  PropPanel::TLeafHandle parent = tree->getParentLeaf(selectedLeaf);
  String leafName = tree->getCaption(parent);
  const char *parentName = get_name_for_search(leafName.c_str());
  AnimCtrlData *ctrlData = eastl::find_if(controllers.begin(), controllers.end(),
    [ctrlsTree = ctrlsTree, parentName](const AnimCtrlData &data) { return ctrlsTree->getCaption(data.handle) == parentName; });
  if (ctrlData != controllers.end())
  {
    // Check if group minimized and open it
    if (pluginPanel->getBool(PID_ANIM_BLEND_CTRLS_GROUP))
      pluginPanel->setBool(PID_ANIM_BLEND_CTRLS_GROUP, false);
    ctrlsTree->setSelLeaf(ctrlData->handle);
    pluginEventHandler->onChange(PID_ANIM_BLEND_CTRLS_TREE, pluginPanel);
    PropPanel::focus_helper.requestFocus(pluginPanel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer());
    for (int i = 0; i < ctrlsTree->getChildCount(parent); ++i)
    {
      if (ctrlsTree->getChildLeaf(parent, i) == selectedLeaf)
      {
        pluginPanel->setInt(PID_CTRLS_NODES_LIST, i);
        break;
      }
    }
    pluginEventHandler->onChange(PID_CTRLS_NODES_LIST, pluginPanel);
  }
}
