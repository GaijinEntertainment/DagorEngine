#include "selectionNodesProcessing.h"
#include "nodesProcessing.h"
#include "propPanelPids.h"
#include <assets/asset.h>
#include <assets/assetMgr.h>

static void find_node_idx(Tab<int> &selected_idx, const Tab<String> &node_names, const char *node_name)
{
  for (int i = 0; i < node_names.size(); ++i)
  {
    if (node_names[i] == node_name)
    {
      selected_idx.push_back(i);
    }
  }
}

void SelectionNodesProcessing::init(DagorAsset *asset, CollisionResource *collision_resource)
{
  curAsset = asset;
  collisionRes = collision_resource;
}

void SelectionNodesProcessing::setPropPanel(PropertyContainerControlBase *prop_panel) { panel = prop_panel; }

static bool contains_node(const dag::Vector<String> &skip_nodes, const char *node)
{
  for (const auto &skipNode : skip_nodes)
  {
    if (skipNode == node)
    {
      return true;
    }
  }
  return false;
}

static void fill_skip_nodes_from_settings(const SelectedNodesSettings &selected_nodes, dag::Vector<String> &skip_nodes)
{
  if (selected_nodes.replaceNodes)
  {
    for (const auto &refNode : selected_nodes.refNodes)
    {
      if (!contains_node(skip_nodes, refNode))
      {
        skip_nodes.push_back(refNode);
      }
    }
  }
}

template <typename T>
static void fill_skip_nodes_from_container(const T &collision_container, dag::Vector<String> &skip_nodes)
{
  for (const auto &settings : collision_container)
  {
    fill_skip_nodes_from_settings(settings.selectedNodes, skip_nodes);
  }
}

template <>
void fill_skip_nodes_from_container(const dag::Vector<SelectedNodesSettings> &collision_container, dag::Vector<String> &skip_nodes)
{
  for (const auto &settings : collision_container)
  {
    fill_skip_nodes_from_settings(settings, skip_nodes);
  }
}

static void create_tree_leaf_from_node(const SelectedNodesSettings &selectedNodes, const char *icon_name,
  PropertyContainerControlBase *tree)
{
  String nodeName{selectedNodes.nodeName};
  if (selectedNodes.isPhysCollidable)
    nodeName.insert(0, "[phys]");
  if (selectedNodes.isTraceable)
    nodeName.insert(0, "[trace]");
  TLeafHandle leaf = tree->createTreeLeaf(0, nodeName, icon_name);

  for (const auto refNode : selectedNodes.refNodes)
  {
    tree->createTreeLeaf(leaf, refNode, "childLeaf");
  }
}

template <typename T>
void create_tree_leaf_from_container(const T &collision_container, PropertyContainerControlBase *tree, const char *icon_name)
{
  for (const auto &settings : collision_container)
  {
    create_tree_leaf_from_node(settings.selectedNodes, icon_name, tree);
  }
}

template <>
void create_tree_leaf_from_container(const dag::Vector<SelectedNodesSettings> &collision_container, PropertyContainerControlBase *tree,
  const char *icon_name)
{
  for (const auto &settings : collision_container)
  {
    switch (settings.type)
    {
      case ExportCollisionNodeType::MESH: create_tree_leaf_from_node(settings, "mesh", tree); break;
      case ExportCollisionNodeType::BOX: create_tree_leaf_from_node(settings, "box", tree); break;
      case ExportCollisionNodeType::SPHERE: create_tree_leaf_from_node(settings, "sphere", tree); break;
    }
  }
}

static void add_skip_nodes_convex_hulls(const DataBlock *node, dag::Vector<String> &skip_nodes)
{
  const int convexHullCount = node->getInt("maxConvexHulls");
  for (int i = 1; i < convexHullCount; ++i)
  {
    skip_nodes.push_back() = node->getBlockName() + String(5, "_ch%02d", i);
  }
}

void SelectionNodesProcessing::fillInfoTree(PropertyContainerControlBase *tree)
{
  dag::Vector<String> skipNodes;

  if (const DataBlock *nodes = curAsset->props.getBlockByName("nodes"))
  {
    for (int i = 0; i < nodes->blockCount(); ++i)
    {
      const DataBlock *node = nodes->getBlock(i);
      const char *collision = node->getStr("collision", nullptr);
      String iconName;
      if (collision)
        iconName = collision;
      String nodeName{node->getBlockName()};
      if (node->getBool("isPhysCollidable"))
        nodeName.insert(0, "[phys]");
      if (node->getBool("isTraceable"))
        nodeName.insert(0, "[trace]");
      TLeafHandle leaf = tree->createTreeLeaf(0, nodeName, iconName);
      const DataBlock *refNodes = node->getBlockByName("refNodes");
      skipNodes.push_back() = node->getBlockName();
      if (get_export_type_by_name(collision) == ExportCollisionNodeType::CONVEX_VHACD)
      {
        add_skip_nodes_convex_hulls(node, skipNodes);
      }

      for (int j = 0; j < refNodes->blockCount(); ++j)
      {
        tree->createTreeLeaf(leaf, refNodes->getBlock(j)->getBlockName(), "childLeaf");
      }
    }
  }

  fill_skip_nodes_from_container(combinedNodesSettings, skipNodes);
  fill_skip_nodes_from_container(kdopsSettings, skipNodes);
  fill_skip_nodes_from_container(convexsComputerSettings, skipNodes);
  fill_skip_nodes_from_container(convexsVhacdSettings, skipNodes);

  create_tree_leaf_from_container(combinedNodesSettings, tree, nullptr);
  create_tree_leaf_from_container(kdopsSettings, tree, "kdop");
  create_tree_leaf_from_container(convexsComputerSettings, tree, "convexComputer");
  create_tree_leaf_from_container(convexsVhacdSettings, tree, "convexVhacd");

  dag::ConstSpan<CollisionNode> nodes = collisionRes->getAllNodes();
  for (const auto &node : nodes)
  {
    if (!contains_node(skipNodes, node.name))
    {
      String iconName;
      String nodeName{node.name};
      if (node.behaviorFlags & CollisionNode::PHYS_COLLIDABLE)
        nodeName.insert(0, "[phys]");
      if (node.behaviorFlags & CollisionNode::TRACEABLE)
        nodeName.insert(0, "[trace]");
      switch (node.type)
      {
        case COLLISION_NODE_TYPE_MESH: iconName = "mesh"; break;
        case COLLISION_NODE_TYPE_POINTS: iconName = "points"; break;
        case COLLISION_NODE_TYPE_BOX: iconName = "box"; break;
        case COLLISION_NODE_TYPE_SPHERE: iconName = "sphere"; break;
        case COLLISION_NODE_TYPE_CAPSULE: iconName = "capsule"; break;
        case COLLISION_NODE_TYPE_CONVEX: iconName = "convex"; break;
      }
      tree->createTreeLeaf(0, nodeName, iconName);
    }
  }
}

template <typename TCollision, typename TNames>
static void fill_nodes_name(const TCollision &collision_container, TNames &node_names)
{
  for (const auto &settings : collision_container)
  {
    node_names.push_back() = settings.selectedNodes.nodeName;
  }
}

template <typename TNames>
static void fill_nodes_name(const dag::Vector<SelectedNodesSettings> &collision_container, TNames &node_names)
{
  for (const auto &settings : collision_container)
  {
    node_names.push_back() = settings.nodeName;
  }
}

void SelectionNodesProcessing::fillNodeNamesTab(Tab<String> &node_names)
{
  dag::Vector<String> skipNodes;
  fill_skip_nodes_from_container(combinedNodesSettings, skipNodes);
  fill_skip_nodes_from_container(kdopsSettings, skipNodes);
  fill_skip_nodes_from_container(convexsVhacdSettings, skipNodes);
  fill_skip_nodes_from_container(convexsComputerSettings, skipNodes);

  fill_nodes_name(combinedNodesSettings, node_names);
  fill_nodes_name(kdopsSettings, node_names);
  fill_nodes_name(convexsVhacdSettings, node_names);
  fill_nodes_name(convexsComputerSettings, node_names);

  fillConvexVhacdNamesBlk(node_names, skipNodes);

  dag::ConstSpan<CollisionNode> nodes = collisionRes->getAllNodes();
  for (const auto &node : nodes)
  {
    if (!contains_node(skipNodes, node.name))
    {
      node_names.push_back() = node.name;
    }
  }
}

void SelectionNodesProcessing::fillConvexVhacdNamesBlk(Tab<String> &node_names, dag::Vector<String> &skip_nodes)
{
  if (const DataBlock *nodes = curAsset->props.getBlockByName("nodes"))
  {
    for (int i = 0; i < nodes->blockCount(); ++i)
    {
      const DataBlock *node = nodes->getBlock(i);
      const char *collision = node->getStr("collision", nullptr);
      if (get_export_type_by_name(collision) == ExportCollisionNodeType::CONVEX_VHACD)
      {
        node_names.push_back() = node->getBlockName();
        add_skip_nodes_convex_hulls(node, skip_nodes);
      }
    }
  }
}

static bool add_selected_nodes_from_settings(const SelectedNodesSettings &settings, const String &node_name,
  dag::Vector<String> &selected_nodes, dag::Vector<String> &delete_candidats)
{
  if (settings.nodeName == node_name)
  {
    selected_nodes.insert(selected_nodes.end(), settings.refNodes.begin(), settings.refNodes.end());
    if (settings.replaceNodes)
    {
      delete_candidats.push_back() = settings.nodeName;
    }
    return true;
  }
  return false;
}

template <typename T>
static bool add_selected_nodes_from_container(const T &container, const String &node_name, dag::Vector<String> &selected_nodes,
  dag::Vector<String> &delete_candidats)
{
  for (const auto &item : container)
  {
    if (add_selected_nodes_from_settings(item.selectedNodes, node_name, selected_nodes, delete_candidats))
      return true;
  }
  return false;
}

template <>
bool add_selected_nodes_from_container(const dag::Vector<SelectedNodesSettings> &container, const String &node_name,
  dag::Vector<String> &selected_nodes, dag::Vector<String> &delete_candidats)
{
  for (const auto &item : container)
  {
    if (add_selected_nodes_from_settings(item, node_name, selected_nodes, delete_candidats))
      return true;
  }
  return false;
}

void SelectionNodesProcessing::getSelectedNodes(dag::Vector<String> &selected_nodes)
{
  Tab<String> nameNodes;
  Tab<int> selectedIdxs;
  panel->getSelection(PID_SELECTABLE_NODES_LIST, selectedIdxs);
  panel->getStrings(PID_SELECTABLE_NODES_LIST, nameNodes);
  deleteNodesCandidats.clear();
  for (const auto idx : selectedIdxs)
  {
    if (add_selected_nodes_from_container(combinedNodesSettings, nameNodes[idx], selected_nodes, deleteNodesCandidats)) {}
    else if (add_selected_nodes_from_container(kdopsSettings, nameNodes[idx], selected_nodes, deleteNodesCandidats)) {}
    else if (add_selected_nodes_from_container(convexsComputerSettings, nameNodes[idx], selected_nodes, deleteNodesCandidats)) {}
    else if (add_selected_nodes_from_container(convexsVhacdSettings, nameNodes[idx], selected_nodes, deleteNodesCandidats)) {}
    else
      selected_nodes.push_back(nameNodes[idx]);
  }
}

static void insertRefNodes(const SelectedNodesSettings &settings, dag::Vector<String> &add_ref_nodes)
{
  add_ref_nodes.insert(add_ref_nodes.end(), settings.refNodes.begin(), settings.refNodes.end());
}

template <typename T>
static void fill_props_to_settings(T &settings, DataBlock *props, DataBlock *nodes, bool &calc_collisions_after_reject)
{
  settings.readProps(props);
  settings.removeProps(props);
  nodes->removeBlock(props->getBlockName());
  calc_collisions_after_reject = true;
}

static void replace_selected_node(const SelectedNodesSettings &settings, dag::Vector<String> &add_ref_nodes,
  dag::Vector<String> &selected_nodes, int &idx, dag::Vector<String> &delete_candidats, bool is_replace)
{
  if (is_replace)
  {
    delete_candidats.push_back(settings.nodeName);
  }
  if (!settings.replaceNodes)
  {
    insertRefNodes(settings, add_ref_nodes);
    selected_nodes.erase(selected_nodes.begin() + idx--);
  }
}

bool SelectionNodesProcessing::rejectExportedCollisions(dag::Vector<String> &selected_nodes, bool is_replace)
{
  dag::Vector<String> addRefNodes;
  if (DataBlock *nodes = curAsset->props.getBlockByName("nodes"))
  {
    for (int i = 0; i < selected_nodes.size(); ++i)
    {
      if (DataBlock *props = nodes->getBlockByName(selected_nodes[i]))
      {
        const char *collision = props->getStr("collision", nullptr);
        const ExportCollisionNodeType nodeType = get_export_type_by_name(collision);
        if (nodeType == ExportCollisionNodeType::MESH || nodeType == ExportCollisionNodeType::BOX ||
            nodeType == ExportCollisionNodeType::SPHERE)
        {
          SelectedNodesSettings &settings = rejectedCombinedNodesSettings.push_back();
          fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
          replace_selected_node(settings, addRefNodes, selected_nodes, i, deleteNodesCandidats, is_replace);
          exportedCombinedNodesSettings.push_back(settings);
        }
        else if (nodeType == ExportCollisionNodeType::KDOP)
        {
          KdopSettings &settings = rejectedKdopsSettings.push_back();
          fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
          replace_selected_node(settings.selectedNodes, addRefNodes, selected_nodes, i, deleteNodesCandidats, is_replace);
          exportedKdopsSettings.push_back(settings);
        }
        else if (nodeType == ExportCollisionNodeType::CONVEX_VHACD)
        {
          ConvexVhacdSettings &settings = rejectedConvexsVhacdSettings.push_back();
          fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
          replace_selected_node(settings.selectedNodes, addRefNodes, selected_nodes, i, deleteNodesCandidats, is_replace);
          exportedConvexsVhacdSettings.push_back(settings);
        }
        else if (nodeType == ExportCollisionNodeType::CONVEX_COMPUTER)
        {
          ConvexComputerSettings &settings = rejectedConvexsComputerSettings.push_back();
          fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
          replace_selected_node(settings.selectedNodes, addRefNodes, selected_nodes, i, deleteNodesCandidats, is_replace);
          exportedConvexsComputerSettings.push_back(settings);
        }
        else
        {
          debug("unknown node type for reject %s", collision);
        }
      }
    }
    if (calcCollisionAfterReject)
    {
      selected_nodes.insert(selected_nodes.end(), addRefNodes.begin(), addRefNodes.end());
      checkReplacedNodes(addRefNodes, nodes);
    }
  }

  return calcCollisionAfterReject;
}

void SelectionNodesProcessing::checkReplacedNodes(const dag::Vector<String> &ref_nodes, DataBlock *nodes)
{
  dag::Vector<String> newRefNodes;
  for (int i = 0; i < nodes->blockCount(); ++i)
  {
    DataBlock *props = nodes->getBlock(i);
    if (props->getBool("replaceNodes"))
    {
      const DataBlock *refNodes = props->getBlockByName("refNodes");
      for (const auto refNode : ref_nodes)
      {
        for (int j = 0; j < refNodes->blockCount(); ++j)
        {
          if (refNode == refNodes->getBlock(j)->getBlockName())
          {
            const char *collision = props->getStr("collision", nullptr);
            const ExportCollisionNodeType nodeType = get_export_type_by_name(collision);
            if (nodeType == ExportCollisionNodeType::MESH || nodeType == ExportCollisionNodeType::BOX ||
                nodeType == ExportCollisionNodeType::SPHERE)
            {
              SelectedNodesSettings &settings = rejectedCombinedNodesSettings.push_back();
              fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
              insertRefNodes(settings, newRefNodes);
              exportedCombinedNodesSettings.push_back(settings);
            }
            else if (nodeType == ExportCollisionNodeType::KDOP)
            {
              KdopSettings &settings = rejectedKdopsSettings.push_back();
              fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
              insertRefNodes(settings.selectedNodes, newRefNodes);
              exportedKdopsSettings.push_back(settings);
            }
            else if (nodeType == ExportCollisionNodeType::CONVEX_VHACD)
            {
              ConvexVhacdSettings &settings = rejectedConvexsVhacdSettings.push_back();
              fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
              insertRefNodes(settings.selectedNodes, newRefNodes);
              exportedConvexsVhacdSettings.push_back(settings);
            }
            else if (nodeType == ExportCollisionNodeType::CONVEX_COMPUTER)
            {
              ConvexComputerSettings &settings = rejectedConvexsComputerSettings.push_back();
              fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
              insertRefNodes(settings.selectedNodes, newRefNodes);
              exportedConvexsComputerSettings.push_back(settings);
            }
            else
            {
              debug("unknown node type for check replaced nodes %s", collision);
            }
          }
        }
      }
    }
  }
  if (newRefNodes.size() > 0)
  {
    checkReplacedNodes(newRefNodes, nodes);
  }
}

bool SelectionNodesProcessing::checkNodeName(const char *node_name)
{
  if (!node_name)
    return false;

  dag::Vector<String> nodeNames;
  dag::ConstSpan<CollisionNode> nodes = collisionRes->getAllNodes();
  for (const auto &node : nodes)
  {
    nodeNames.push_back() = node.name;
  }
  fill_nodes_name(combinedNodesSettings, nodeNames);
  fill_nodes_name(kdopsSettings, nodeNames);
  fill_nodes_name(convexsVhacdSettings, nodeNames);
  fill_nodes_name(convexsComputerSettings, nodeNames);

  return !contains_node(nodeNames, node_name);
}

void SelectionNodesProcessing::addCombinedSettings(const SelectedNodesSettings &settings)
{
  combinedNodesSettings.push_back(settings);
}

void SelectionNodesProcessing::addKdopSettings(const KdopSettings &settings) { kdopsSettings.push_back(settings); }

void SelectionNodesProcessing::addConvexComputerSettings(const ConvexComputerSettings &settings)
{
  convexsComputerSettings.push_back(settings);
}

void SelectionNodesProcessing::addConvexVhacdSettings(const ConvexVhacdSettings &settings)
{
  convexsVhacdSettings.push_back(settings);
}

template <typename T>
static int delete_selected_node_from_container(T &container, const String &name, ExportCollisionNodeType &deleted_type)
{
  for (int i = 0; i < container.size(); ++i)
  {
    if (container[i].selectedNodes.nodeName == name)
    {
      deleted_type = container[i].selectedNodes.type;
      container.erase(container.begin() + i);
      return i;
    }
  }
  return -1;
}

int SelectionNodesProcessing::deleteSelectedNode(const String &name, ExportCollisionNodeType &deleted_type)
{
  for (int i = 0; i < combinedNodesSettings.size(); ++i)
  {
    if (combinedNodesSettings[i].nodeName == name)
    {
      deleted_type = combinedNodesSettings[i].type;
      combinedNodesSettings.erase(combinedNodesSettings.begin() + i);
      return i;
    }
  }

  if (int deletedIdx = delete_selected_node_from_container(kdopsSettings, name, deleted_type); deletedIdx != -1)
    return deletedIdx;
  if (int deletedIdx = delete_selected_node_from_container(convexsComputerSettings, name, deleted_type); deletedIdx != -1)
    return deletedIdx;
  if (int deletedIdx = delete_selected_node_from_container(convexsVhacdSettings, name, deleted_type); deletedIdx != -1)
    return deletedIdx;

  return -1;
}

bool SelectionNodesProcessing::deleteNodeFromBlk(const String &name)
{
  if (DataBlock *nodes = curAsset->props.getBlockByName("nodes"))
  {
    if (DataBlock *props = nodes->getBlockByName(name))
    {
      const char *collision = props->getStr("collision", nullptr);
      const ExportCollisionNodeType nodeType = get_export_type_by_name(collision);
      if (nodeType == ExportCollisionNodeType::MESH || nodeType == ExportCollisionNodeType::BOX ||
          nodeType == ExportCollisionNodeType::SPHERE)
      {
        SelectedNodesSettings &settings = exportedCombinedNodesSettings.push_back();
        fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
      }
      else if (nodeType == ExportCollisionNodeType::KDOP)
      {
        KdopSettings &settings = exportedKdopsSettings.push_back();
        fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
      }
      else if (nodeType == ExportCollisionNodeType::CONVEX_VHACD)
      {
        ConvexVhacdSettings &settings = exportedConvexsVhacdSettings.push_back();
        fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
      }
      else if (nodeType == ExportCollisionNodeType::CONVEX_COMPUTER)
      {
        ConvexComputerSettings &settings = exportedConvexsComputerSettings.push_back();
        fill_props_to_settings(settings, props, nodes, calcCollisionAfterReject);
      }
      else
      {
        debug("unknown node type for check replaced nodes %s", collision);
      }
      return true;
    }
  }
  return false;
}

template <typename T>
static SelectedNodesSettings *find_selected_node_settings_in_container(T &container, const String &name)
{
  for (auto &item : container)
  {
    if (name == item.selectedNodes.nodeName)
    {
      return &item.selectedNodes;
    }
  }
  return nullptr;
}

template <>
SelectedNodesSettings *find_selected_node_settings_in_container(dag::Vector<SelectedNodesSettings> &container, const String &name)
{
  for (auto &item : container)
  {
    if (item.nodeName == name)
    {
      return &item;
    }
  }
  return nullptr;
}

void SelectionNodesProcessing::findSelectedNodeSettings(const String &name)
{
  if ((editSettings = find_selected_node_settings_in_container(combinedNodesSettings, name))) {}
  else if ((editSettings = find_selected_node_settings_in_container(kdopsSettings, name))) {}
  else if ((editSettings = find_selected_node_settings_in_container(convexsComputerSettings, name))) {}
  else if ((editSettings = find_selected_node_settings_in_container(convexsVhacdSettings, name))) {}
  G_ASSERT_LOG(editSettings, "Selected settings for edit not found, node: %s", name);
}

static void remove_ch_postfix(String &node_name)
{
  const int lenWitoutPostfix = strlen(node_name) - 5;
  if (lenWitoutPostfix > 0 && !strncmp(node_name + lenWitoutPostfix, "_ch", 3))
  {
    node_name.setStr(node_name, lenWitoutPostfix);
  }
}

void SelectionNodesProcessing::selectNode(const char *node_name, bool ctrl_pressed)
{
  if (!node_name)
  {
    if (!ctrl_pressed)
      panel->setSelection(PID_SELECTABLE_NODES_LIST, {});
    panel->getById(PID_COLLISION_NODES_TREE)->getContainer()->setSelLeaf(nullptr);
    return;
  }
  Tab<String> nodes;
  Tab<int> sels;
  String nodeName(node_name);
  remove_ch_postfix(nodeName);
  panel->getStrings(PID_SELECTABLE_NODES_LIST, nodes);
  if (ctrl_pressed)
  {
    panel->getSelection(PID_SELECTABLE_NODES_LIST, sels);
  }
  find_node_idx(sels, nodes, nodeName);
  panel->setSelection(PID_SELECTABLE_NODES_LIST, sels);

  PropertyControlBase *treeControl = panel->getById(PID_COLLISION_NODES_TREE);
  if (!treeControl)
    return;

  PropertyContainerControlBase *tree = treeControl->getContainer();
  iterate_all_leafs_linear(*tree, tree->getRootLeaf(), [tree, &nodeName](TLeafHandle child) {
    String leafName(tree->getCaption(child));
    NodesProcessing::delete_flags_prefix(leafName);
    if (leafName != nodeName)
      return true;
    tree->setSelLeaf(child);
    return false;
  });
}

static void update_hidden_nodes(const SelectedNodesSettings &settings, const dag::ConstSpan<CollisionNode> &nodes,
  dag::Vector<bool> &hidden_nodes)
{
  if (settings.replaceNodes)
  {
    for (const auto &refNode : settings.refNodes)
    {
      for (int i = 0; i < nodes.size(); ++i)
      {
        if (refNode == nodes[i].name.c_str())
        {
          hidden_nodes[i] = true;
          break;
        }
      }
    }
  }
}

template <typename T>
void update_hidden_nodes_from_contianer(const T &container, const dag::ConstSpan<CollisionNode> &nodes,
  dag::Vector<bool> &hidden_nodes)
{
  for (const auto &settings : container)
  {
    update_hidden_nodes(settings.selectedNodes, nodes, hidden_nodes);
  }
}

template <>
void update_hidden_nodes_from_contianer(const dag::Vector<SelectedNodesSettings> &container,
  const dag::ConstSpan<CollisionNode> &nodes, dag::Vector<bool> &hidden_nodes)
{
  for (const auto &settings : container)
  {
    update_hidden_nodes(settings, nodes, hidden_nodes);
  }
}

void SelectionNodesProcessing::updateHiddenNodes()
{
  dag::ConstSpan<CollisionNode> collisionNodes = collisionRes->getAllNodes();
  hiddenNodes.clear();
  hiddenNodes.resize(collisionNodes.size(), false);
  update_hidden_nodes_from_contianer(combinedNodesSettings, collisionNodes, hiddenNodes);
  update_hidden_nodes_from_contianer(kdopsSettings, collisionNodes, hiddenNodes);
  update_hidden_nodes_from_contianer(convexsComputerSettings, collisionNodes, hiddenNodes);
  update_hidden_nodes_from_contianer(convexsVhacdSettings, collisionNodes, hiddenNodes);
}

template <typename T>
static void save_nodes_from_container(const T &container, DataBlock *nodes)
{
  for (const auto &settings : container)
  {
    settings.saveProps(nodes);
  }
}

void SelectionNodesProcessing::saveCollisionNodes()
{
  if (!curAsset)
    return;

  DataBlock *nodes = curAsset->props.addBlock("nodes");
  save_nodes_from_container(combinedNodesSettings, nodes);
  save_nodes_from_container(kdopsSettings, nodes);
  save_nodes_from_container(convexsVhacdSettings, nodes);
  save_nodes_from_container(convexsComputerSettings, nodes);
  clearSettings();
  if (curAsset->isVirtual())
  {
    curAsset->props.setStr("name", curAsset->getSrcFileName());
    curAsset->props.saveToTextFile(String(0, "%s/%s.collision.blk", curAsset->getFolderPath(), curAsset->getName()));
    curAsset->getMgr().callAssetChangeNotifications(*curAsset, curAsset->getNameId(), curAsset->getType());
  }
}

void SelectionNodesProcessing::saveExportedCollisionNodes()
{
  if (!curAsset || calcCollisionAfterReject ||
      (!exportedCombinedNodesSettings.size() && !exportedKdopsSettings.size() && !exportedConvexsComputerSettings.size() &&
        !exportedConvexsVhacdSettings.size()))
    return;

  DataBlock *nodes = curAsset->props.addBlock("nodes");
  save_nodes_from_container(exportedCombinedNodesSettings, nodes);
  save_nodes_from_container(exportedKdopsSettings, nodes);
  save_nodes_from_container(exportedConvexsVhacdSettings, nodes);
  save_nodes_from_container(exportedConvexsComputerSettings, nodes);
  curAsset->getMgr().callAssetChangeNotifications(*curAsset, curAsset->getNameId(), curAsset->getType());

  clearSettings();
}

template <typename T>
static bool find_settings_from_container(const String &name, const dag::Vector<T> &container, T &settings)
{
  for (const auto &containerSettings : container)
  {
    if (name == containerSettings.selectedNodes.nodeName)
    {
      settings = containerSettings;
      return true;
    }
  }
  return false;
}

bool SelectionNodesProcessing::findSettingsByName(const String &name, KdopSettings &settings)
{
  return find_settings_from_container(name, kdopsSettings, settings);
}

bool SelectionNodesProcessing::findSettingsByName(const String &name, ConvexComputerSettings &settings)
{
  return find_settings_from_container(name, convexsComputerSettings, settings);
}

bool SelectionNodesProcessing::findSettingsByName(const String &name, ConvexVhacdSettings &settings)
{
  return find_settings_from_container(name, convexsVhacdSettings, settings);
}

void SelectionNodesProcessing::clearRejectedSettings()
{
  rejectedCombinedNodesSettings.clear();
  rejectedKdopsSettings.clear();
  rejectedConvexsComputerSettings.clear();
  rejectedConvexsVhacdSettings.clear();
}

void SelectionNodesProcessing::clearSettings()
{
  combinedNodesSettings.clear();
  kdopsSettings.clear();
  convexsComputerSettings.clear();
  convexsVhacdSettings.clear();
  exportedCombinedNodesSettings.clear();
  exportedKdopsSettings.clear();
  exportedConvexsVhacdSettings.clear();
  exportedConvexsComputerSettings.clear();
  clearRejectedSettings();
}
