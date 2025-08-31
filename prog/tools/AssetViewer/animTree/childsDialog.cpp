// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "childsDialog.h"
#include "animTreeUtils.h"
#include "animTreeIcons.h"
#include "animTreePanelPids.h"
#include "blendNodes/blendNodeType.h"
#include "animStates/animStatesType.h"

#include "animStates/state.h"
#include "controllers/hub.h"
#include "controllers/randomSwitch.h"
#include "controllers/paramSwitch.h"
#include "controllers/linearPoly.h"

#include <propPanel/control/menu.h>
#include <propPanel/control/container.h>
#include <ioSys/dag_dataBlock.h>
#include <assets/asset.h>

struct CachedInclude
{
  String includeName;
  DataBlock props;
};

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
  COLLAPSE_ALL,
  EXPAND_ALL,
  SET_AS_ROOT,
};
}

ChildsDialog::ChildsDialog(dag::Vector<AnimCtrlData> &controllers, dag::Vector<BlendNodeData> &nodes,
  dag::Vector<AnimStatesData> &states, dag::Vector<String> &paths) :
  controllers{controllers},
  nodes{nodes},
  states{states},
  paths{paths},
  DialogWindow(nullptr, hdpi::_pxScaled(DEFAULT_MINIMUM_WIDTH), hdpi::_pxScaled(DEFAULT_MINIMUM_HEIGHT), "Childs tree")
{
  setManualModalSizingEnabled();
  DialogWindow::showButtonPanel(false);
  initPanel();
}

bool ChildsDialog::onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id, PropPanel::ITreeInterface &tree_interface)
{
  using PropPanel::ROOT_MENU_ITEM;
  if (pcb_id == PID_CHILDS_TREE)
  {
    PropPanel::TLeafHandle selectedLeaf = tree.getSelLeaf();
    String name = tree.getCaption(selectedLeaf);
    if (strstr(name.c_str(), "not found"))
      return false;

    PropPanel::IMenu &menu = tree_interface.createContextMenu();
    if (!strstr(name.c_str(), "Empty child") && !strstr(name.c_str(), "Leave cur child"))
      menu.addItem(ROOT_MENU_ITEM, ContextMenu::EDIT, "Edit");
    if (selectedLeaf != tree.getRootLeaf())
    {
      menu.addItem(ROOT_MENU_ITEM, ContextMenu::EDIT_IN_PARENT, "Edit in parent");
      menu.addItem(ROOT_MENU_ITEM, ContextMenu::SET_AS_ROOT, "Set as root");
    }
    if (tree.getChildCount(selectedLeaf) > 0)
    {
      menu.addItem(ROOT_MENU_ITEM, ContextMenu::COLLAPSE_ALL, "Collapse all");
      menu.addItem(ROOT_MENU_ITEM, ContextMenu::EXPAND_ALL, "Expand all");
    }

    menu.setEventHandler(this);
    return true;
  }
  return false;
}

void copy_child_leafs_to_new_parent(PropPanel::ContainerPropertyControl &tree, PropPanel::TLeafHandle parent,
  PropPanel::TLeafHandle copy)
{
  PropPanel::TLeafHandle child = tree.createTreeLeaf(parent, tree.getCaption(copy), nullptr);
  tree.copyButtonPictures(copy, child);
  for (int i = 0; i < tree.getChildCount(copy); ++i)
    copy_child_leafs_to_new_parent(tree, child, tree.getChildLeaf(copy, i));
}

void set_leaf_as_root(PropPanel::ContainerPropertyControl &tree)
{
  PropPanel::TLeafHandle selLeaf = tree.getSelLeaf();
  PropPanel::TLeafHandle root = tree.getRootLeaf();
  PropPanel::TLeafHandle newRoot = tree.createTreeLeaf(nullptr, tree.getCaption(selLeaf), nullptr);
  tree.copyButtonPictures(selLeaf, newRoot);
  for (int i = 0; i < tree.getChildCount(selLeaf); ++i)
    copy_child_leafs_to_new_parent(tree, newRoot, tree.getChildLeaf(selLeaf, i));

  tree.removeLeaf(root);
  tree.setBool(newRoot, /*open*/ true);
}

int ChildsDialog::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::EDIT: editSelectedNode(); break;
    case ContextMenu::EDIT_IN_PARENT: editInParentSelectedNode(); break;
    case ContextMenu::COLLAPSE_ALL: setAllExpanded(/*expanded*/ false); break;
    case ContextMenu::EXPAND_ALL: setAllExpanded(/*expanded*/ true); break;
    case ContextMenu::SET_AS_ROOT: set_leaf_as_root(*getPanel()->getById(PID_CHILDS_TREE)->getContainer()); break;
  }
  return 0;
}

void ChildsDialog::initPanel()
{
  PropPanel::ContainerPropertyControl *panel = DialogWindow::getPanel();
  panel->createTree(PID_CHILDS_TREE, "", hdpi::Px(0));
}

void ChildsDialog::clear()
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

void ChildsDialog::setTreePanels(PropPanel::ContainerPropertyControl *panel, DagorAsset *cur_asset,
  PropPanel::ControlEventHandler *event_handler)
{
  pluginPanel = panel;
  pluginEventHandler = event_handler;
  ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  nodesTree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  statesTree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  asset = cur_asset;
}

void ChildsDialog::fillChildsTree(PropPanel::TLeafHandle leaf)
{
  dag::Vector<CachedInclude> includes;
  PropPanel::ContainerPropertyControl *panel = getPanel();
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_CHILDS_TREE)->getContainer();
  tree->clear();
  tree->setTreeEventHandler(this);
  AnimCtrlData *leafData = find_data_by_handle(controllers, leaf);
  AnimStatesData *initAnimStateData = eastl::find_if(states.begin(), states.end(),
    [](const AnimStatesData &data) { return data.type == AnimStatesType::INIT_ANIM_STATE; });
  DataBlock props;
  const DataBlock *enumRootProps = &DataBlock::emptyBlock;
  if (initAnimStateData != states.end() && !initAnimStateData->fileName.empty())
  {
    String fullPath;
    props = get_props_from_include_state_data(paths, controllers, *asset, ctrlsTree, *initAnimStateData, fullPath);
    enumRootProps = props.getBlockByNameEx("initAnimState")->getBlockByNameEx("enum");
  }

  if (leafData != controllers.end())
    fillCtrlChilds(tree, nullptr, *leafData, String(""), includes, *enumRootProps);
  else
    fillStateChilds(tree, leaf, *enumRootProps);
}

void ChildsDialog::fillStateChilds(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle leaf,
  const DataBlock &enum_root_props)
{
  dag::Vector<CachedInclude> includes;
  AnimStatesData *leafData = find_data_by_handle(states, leaf);
  if (leafData != states.end())
  {
    String fullPath;
    AnimStatesData *stateDescData =
      eastl::find_if(states.begin(), states.end(), [](const AnimStatesData &data) { return data.type == AnimStatesType::STATE_DESC; });
    CachedInclude &include = includes.emplace_back(CachedInclude{stateDescData->fileName,
      get_props_from_include_state_data(paths, controllers, *asset, ctrlsTree, *stateDescData, fullPath, /*only_includes*/ true)});
    DataBlock *stateDesc = include.props.addBlock("stateDesc");
    DataBlock *settings = find_block_by_name(stateDesc, ctrlsTree->getCaption(leaf));
    PropPanel::TLeafHandle stateLeaf = tree->createTreeLeaf(nullptr, statesTree->getCaption(leaf), STATE_LEAF_ICON);
    tree->setBool(stateLeaf, /*open*/ true);
    for (int i = 0; i < leafData->childs.size(); ++i)
    {
      const int idx = leafData->childs[i];
      const char *chanName = settings->getBlock(i)->getBlockName();
      if (idx == AnimStatesData::EMPTY_CHILD_TYPE)
        tree->createTreeLeaf(stateLeaf, String(0, "[%s] (Empty child)", chanName), nullptr);
      else if (idx == AnimStatesData::LEAVE_CUR_CHILD_TYPE)
        tree->createTreeLeaf(stateLeaf, String(0, "[%s] (Leave cur child)", chanName), nullptr);
      else
      {
        AnimCtrlData *ctrlChild =
          eastl::find_if(controllers.begin(), controllers.end(), [idx](const AnimCtrlData &data) { return idx == data.id; });
        if (ctrlChild != controllers.end())
          fillCtrlChilds(tree, stateLeaf, *ctrlChild, String(0, "[%s] ", chanName), includes, enum_root_props);
        else
        {
          BlendNodeData *nodeChild =
            eastl::find_if(nodes.begin(), nodes.end(), [idx](const BlendNodeData &data) { return idx == data.id; });
          if (nodeChild != nodes.end())
            tree->createTreeLeaf(stateLeaf, String(0, "[%s] %s", chanName, nodesTree->getCaption(nodeChild->handle)),
              get_blend_node_icon_name(nodeChild->type));
          else
          {
            const char *childName = state_get_child_name_by_idx(*settings, i);
            CtrlChildSearchResult result =
              find_child_idx_and_icon_by_name(ctrlsTree, nodesTree, stateLeaf, controllers, nodes, childName);
            if (result.id != AnimCtrlData::NOT_FOUND_CHILD)
            {
              tree->createTreeLeaf(stateLeaf, String(0, "[%s] %s", chanName, childName), result.iconName);
              leafData->childs[i] = result.id;
            }
            else
              tree->createTreeLeaf(stateLeaf, String(0, "[%s] (not found) %s", chanName, childName), nullptr);
          }
        }
      }
    }
  }
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

String ChildsDialog::getImportantParamPrefixName(const DataBlock &settings, int child_idx, CtrlType type,
  const DataBlock &enum_root_props)
{
  String out;
  switch (type)
  {
    case ctrl_type_randomSwitch: return random_switch_get_child_prefix_name(settings, child_idx);
    case ctrl_type_paramSwitch: return param_switch_get_child_prefix_name(settings, child_idx, enum_root_props);
    case ctrl_type_linearPoly: return linear_poly_get_child_prefix_name(settings, child_idx);
    default: break;
  }

  return out;
}

static bool is_contain_important_param(CtrlType type)
{
  return type == ctrl_type_randomSwitch || type == ctrl_type_paramSwitch || type == ctrl_type_linearPoly;
}

static bool get_child_is_optional_from_settings(const DataBlock &settings, CtrlType type, int idx)
{
  switch (type)
  {
    case ctrl_type_hub: return hub_get_child_is_optional_by_idx(settings, idx);
    case ctrl_type_paramSwitch: return param_switch_get_child_is_optional_by_idx(settings, idx);
    default: break;
  }

  return false;
}

void ChildsDialog::fillCtrlChilds(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle parent, AnimCtrlData &leaf_data,
  const String &prefix_name, dag::Vector<CachedInclude> &includes, const DataBlock &enum_root_props)
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
  DataBlock *settings = nullptr;
  if (is_contain_important_param(leaf_data.type))
  {
    CachedInclude *include = findIncludeProps(includes, includeLeaf);
    DataBlock *ctrlProps = get_props_from_include_leaf_ctrl_node(controllers, paths, *asset, include->props, ctrlsTree, includeLeaf);
    settings = find_block_by_name(ctrlProps, ctrlsTree->getCaption(leaf_data.handle));
  }
  if (!parent)
    tree->setBool(leaf, /*open*/ true);
  for (int i = 0; i < leaf_data.childs.size(); ++i)
  {
    String prefixChildName;
    if (settings)
      prefixChildName = getImportantParamPrefixName(*settings, i, leaf_data.type, enum_root_props);
    const int idx = leaf_data.childs[i];
    if (idx == AnimCtrlData::CHILD_AS_SELF)
    {
      String ctrlName = tree->getCaption(leaf);
      tree->createTreeLeaf(leaf, String(0, "%s%s", prefixChildName, ctrlName), LEAF_WITH_LOOP);
      logerr("Controller <%s> can't use self as child", ctrlName);
      continue;
    }
    AnimCtrlData *ctrlChild =
      eastl::find_if(controllers.begin(), controllers.end(), [idx](const AnimCtrlData &data) { return idx == data.id; });
    if (ctrlChild != controllers.end())
      fillCtrlChilds(tree, leaf, *ctrlChild, prefixChildName, includes, enum_root_props);
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
          CachedInclude *include = findIncludeProps(includes, includeLeaf);
          DataBlock *ctrlProps =
            get_props_from_include_leaf_ctrl_node(controllers, paths, *asset, include->props, ctrlsTree, includeLeaf);
          settings = find_block_by_name(ctrlProps, ctrlsTree->getCaption(leaf_data.handle));
        }
        String childName = getChildNameFromSettings(*settings, leaf_data.type, i);
        CtrlChildSearchResult result =
          find_child_idx_and_icon_by_name(ctrlsTree, nodesTree, leaf_data.handle, controllers, nodes, childName);
        if (result.id != AnimCtrlData::NOT_FOUND_CHILD)
        {
          tree->createTreeLeaf(leaf, String(0, "%s%s", prefixChildName, childName), result.iconName);
          leaf_data.childs[i] = result.id;
        }
        else
        {
          bool isOptional = get_child_is_optional_from_settings(*settings, leaf_data.type, i);
          check_ctrl_child_idx(result.id, tree->getCaption(leaf), String(0, "%s%s", prefixChildName, childName), isOptional);
          if (isOptional)
            tree->createTreeLeaf(leaf, String(0, "(optional not found) %s%s", prefixChildName, childName), nullptr);
          else
            tree->createTreeLeaf(leaf, String(0, "(not found) %s%s", prefixChildName, childName), nullptr);
        }
      }
    }
  }
}

String ChildsDialog::getChildNameFromSettings(const DataBlock &settings, CtrlType type, int idx)
{
  switch (type)
  {
    case ctrl_type_hub: return String(hub_get_child_name_by_idx(settings, idx));
    case ctrl_type_randomSwitch: return String(random_switch_get_child_name_by_idx(settings, idx));
    case ctrl_type_paramSwitch:
      if (settings.getBlockByNameEx("nodes")->paramExists("enum_gen"))
      {
        AnimStatesData *initAnimStateData = find_data_by_type(states, AnimStatesType::INIT_ANIM_STATE);
        DataBlock props;
        const DataBlock *enumRootProps = &DataBlock::emptyBlock;
        if (initAnimStateData && !initAnimStateData->fileName.empty())
        {
          String fullPath;
          props = get_props_from_include_state_data(paths, controllers, *asset, ctrlsTree, *initAnimStateData, fullPath);
          enumRootProps = props.getBlockByNameEx("initAnimState")->getBlockByNameEx("enum");
        }
        const DataBlock *enumProps = enumRootProps->getBlockByName(settings.getBlockByName("nodes")->getStr("enum_gen"));
        return param_switch_get_enum_gen_child_name_by_idx(*enumProps, settings.getStr("name", ""), idx);
      }
      else
        return String(param_switch_get_child_name_by_idx(settings, idx));
    case ctrl_type_linearPoly: return String(linear_poly_get_child_name_by_idx(settings, idx));
    default: break;
  }

  return String("");
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

void ChildsDialog::editSelectedNode()
{
  PropPanel::ContainerPropertyControl *tree = DialogWindow::getPanel()->getById(PID_CHILDS_TREE)->getContainer();
  PropPanel::TLeafHandle selLeaf = tree->getSelLeaf();
  String leafName = tree->getCaption(selLeaf);
  const char *name = get_name_for_search(leafName.c_str());

  // State leaf can be only root in tree
  if (selLeaf == tree->getRootLeaf())
  {
    AnimStatesData *stateData = eastl::find_if(states.begin(), states.end(),
      [statesTree = statesTree, name](const AnimStatesData &data) { return statesTree->getCaption(data.handle) == name; });
    if (stateData != states.end())
    {
      focus_selected_node(pluginEventHandler, pluginPanel, stateData->handle, PID_ANIM_STATES_GROUP, PID_ANIM_STATES_GROUP,
        PID_ANIM_STATES_TREE);
      return;
    }
  }

  AnimCtrlData *ctrlData = eastl::find_if(controllers.begin(), controllers.end(),
    [ctrlsTree = ctrlsTree, name](const AnimCtrlData &data) { return ctrlsTree->getCaption(data.handle) == name; });
  if (ctrlData != controllers.end())
    focus_selected_node(pluginEventHandler, pluginPanel, ctrlData->handle, PID_ANIM_BLEND_CTRLS_GROUP, PID_ANIM_BLEND_CTRLS_GROUP,
      PID_ANIM_BLEND_CTRLS_TREE);
  else
  {
    BlendNodeData *nodeData = eastl::find_if(nodes.begin(), nodes.end(),
      [nodesTree = nodesTree, name](const BlendNodeData &data) { return nodesTree->getCaption(data.handle) == name; });
    if (nodeData != nodes.end())
      focus_selected_node(pluginEventHandler, pluginPanel, nodeData->handle, PID_ANIM_BLEND_NODES_GROUP, PID_ANIM_BLEND_NODES_GROUP,
        PID_ANIM_BLEND_NODES_TREE);
  }
}

void ChildsDialog::editInParentSelectedNode()
{
  PropPanel::ContainerPropertyControl *tree = DialogWindow::getPanel()->getById(PID_CHILDS_TREE)->getContainer();
  PropPanel::TLeafHandle selectedLeaf = tree->getSelLeaf();
  PropPanel::TLeafHandle parent = tree->getParentLeaf(selectedLeaf);
  String leafName = tree->getCaption(parent);
  const char *parentName = get_name_for_search(leafName.c_str());

  if (parent == tree->getRootLeaf())
  {
    AnimStatesData *stateData = eastl::find_if(states.begin(), states.end(),
      [statesTree = statesTree, parentName](const AnimStatesData &data) { return statesTree->getCaption(data.handle) == parentName; });
    if (stateData != states.end())
    {
      focus_selected_node(pluginEventHandler, pluginPanel, stateData->handle, PID_ANIM_STATES_GROUP, PID_ANIM_STATES_SETTINGS_GROUP,
        PID_ANIM_STATES_TREE);
      for (int i = 0; i < ctrlsTree->getChildCount(parent); ++i)
      {
        if (ctrlsTree->getChildLeaf(parent, i) == selectedLeaf)
        {
          pluginPanel->setInt(PID_STATES_NODES_LIST, i);
          break;
        }
      }
      pluginEventHandler->onChange(PID_STATES_NODES_LIST, pluginPanel);
      return;
    }
  }

  AnimCtrlData *ctrlData = eastl::find_if(controllers.begin(), controllers.end(),
    [ctrlsTree = ctrlsTree, parentName](const AnimCtrlData &data) { return ctrlsTree->getCaption(data.handle) == parentName; });
  if (ctrlData != controllers.end())
  {
    focus_selected_node(pluginEventHandler, pluginPanel, ctrlData->handle, PID_ANIM_BLEND_CTRLS_GROUP,
      PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP, PID_ANIM_BLEND_CTRLS_TREE);
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

void set_all_expanded(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle leaf, bool expanded)
{
  for (int i = 0; i < tree->getChildCount(leaf); ++i)
  {
    set_all_expanded(tree, tree->getChildLeaf(leaf, i), expanded);
    tree->setBool(tree->getChildLeaf(leaf, i), expanded);
  }
}

void ChildsDialog::setAllExpanded(bool expanded)
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_CHILDS_TREE)->getContainer();
  PropPanel::TLeafHandle selLeaf = tree->getSelLeaf();
  set_all_expanded(tree, selLeaf, expanded);
  tree->setBool(selLeaf, expanded);
}

CachedInclude *ChildsDialog::findIncludeProps(dag::Vector<CachedInclude> &includes, PropPanel::TLeafHandle include_leaf)
{
  String includeName = ctrlsTree->getCaption(include_leaf);
  CachedInclude *include = eastl::find_if(includes.begin(), includes.end(),
    [&includeName](const CachedInclude &include) { return include.includeName == includeName; });
  if (include == includes.end())
  {
    String fullPath;
    include = &includes.emplace_back(CachedInclude{
      includeName, get_props_from_include_leaf(paths, *asset, ctrlsTree, include_leaf, fullPath, /*only_includes*/ true)});
  }

  return include;
}
