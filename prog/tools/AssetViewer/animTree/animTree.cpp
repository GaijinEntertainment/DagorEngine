#include "animTree.h"
#include "../av_plugin.h"
#include "../av_appwnd.h"

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_lodController.h>
#include <de3_animCtrl.h>

#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <math/dag_mathUtils.h>

#include <assetsGui/av_selObjDlg.h>
#include <assets/assetMgr.h>
#include <sepGui/wndPublic.h>

enum
{
  PID_SELECT_DYN_MODEL,

  PID_NODE_MASKS_GROUP,
  PID_NODE_MASKS_TREE,
  PID_NODE_MASKS_ADD_MASK,
  PID_NODE_MASKS_ADD_NODE,
  PID_NODE_MASKS_REMOVE_NODE,
  PID_NODE_MASKS_SETTINGS_GROUP,
  PID_NODE_MASKS_NAME_FILED,
  PID_NODE_MASKS_SAVE,

  PID_ANIM_BLEND_NODES_GROUP,
  PID_ANIM_BLEND_NODES_SETTINGS_GROUP,
  PID_ANIM_BLEND_NODES_TREE,
  PID_ANIM_BLEND_NODES_A2D,
  PID_ANIM_BLEND_NODES_LEAF,
  PID_ANIM_BLEND_NODES_IRQ,
  PID_ANIM_BLEND_NODES_CREATE_NODE,
  PID_ANIM_BLEND_NODES_CREATE_LEAF,
  PID_ANIM_BLEND_NODES_REMOVE_NODE,

  PID_ANIM_BLEND_CTRLS_GROUP,
  PID_ANIM_BLEND_CTRLS_TREE,

  PID_ANIM_STATES_GROUP,
  PID_ANIM_STATES_TREE,
  PID_ANIM_STATES_ENUM,
  PID_ANIM_STATES_ADD_ENUM,
  PID_ANIM_STATES_ADD_ENUM_ITEM,
  PID_ANIM_STATES_REMOVE_NODE,
  PID_ANIM_STATES_SETTINGS_GROUP,
  PID_ANIM_STATES_NAME_FILED,
  PID_ANIM_STATES_SAVE,

  PID_PARAMS_FIELD,
  PID_PARAMS_FIELD_SIZE = PID_PARAMS_FIELD + 100,
  PID_PARAMS_FIELD_PLAY_ANIMATION,
  PID_PARAMS_FIELD_SAVE,
};

// Icon names located at D:\dagor2\tools\dagor3_cdk\commonData\gui16x16
static const char *ASSET_SKELETON_ICON = "asset_skeleton";
static const char *AREA_LINK_ICON = "area_link";
static const char *RES_ANIMBNL_ICON = "res_animbnl";
static const char *RES_RENDINST_ICON = "res_rendinst";
static const char *RES_RAGDOLL_ICON = "res_ragdoll";
static const char *SHOW_PANEL_ICON = "show_panel";

// Icon names for nodes
static const char *ANIM_BLEND_NODE_ICON = RES_ANIMBNL_ICON;
static const char *A2D_NAME_ICON = RES_RAGDOLL_ICON;
static const char *NODE_MASK_ICON = ASSET_SKELETON_ICON;
static const char *NODE_MASK_LEAF_ICON = RES_RENDINST_ICON;
static const char *ANIM_BLEND_CTRL_ICON = RES_ANIMBNL_ICON;
static const char *STATE_ICON = AREA_LINK_ICON;
static const char *STATE_LEAF_ICON = RES_ANIMBNL_ICON;
static const char *ENUM_ICON = SHOW_PANEL_ICON;
static const char *ENUM_ITEM_ICON = RES_RENDINST_ICON;
static const char *IRQ_ICON = AREA_LINK_ICON;

static constexpr int ANIM_BLEND_NODE_A2D = PID_ANIM_BLEND_NODES_A2D;
static constexpr int ANIM_BLEND_NODE_LEAF = PID_ANIM_BLEND_NODES_LEAF;
static constexpr int ANIM_BLEND_NODE_IRQ = PID_ANIM_BLEND_NODES_IRQ;

extern String a2d_last_ref_model;

AnimTreePlugin::AnimTreePlugin() :
  dynModelId{get_app().getAssetMgr().getAssetTypeId("dynModel")}, animProgress{0.0f}, animTime{1.f}, firstKey{0}, lastKey{0}
{}

bool AnimTreePlugin::begin(DagorAsset *asset)
{
  curAsset = asset;
  if (modelName.empty())
    modelName = a2d_last_ref_model;
  reloadDynModel();
  return true;
}

bool AnimTreePlugin::end()
{
  resetDynModel();
  curAsset = nullptr;
  if (selectedA2d)
  {
    ::release_game_resource((GameResource *)selectedA2d);
    selectedA2d = nullptr;
  }
  selectedA2dName.clear();

  return true;
}

bool AnimTreePlugin::getSelectionBox(BBox3 &box) const
{
  if (!entity)
    return false;
  box = entity->getBbox();
  return true;
}

static int get_key_by_name(const AnimV20::AnimData *selectedA2d, const char *key_name)
{
  for (int i = 0; i < selectedA2d->dumpData.noteTrack.size(); ++i)
    if (!strcmp(key_name, selectedA2d->dumpData.noteTrack[i].name))
      return selectedA2d->dumpData.noteTrack[i].time;

  return 0;
}

void AnimTreePlugin::actObjects(float dt)
{
  if (!entity || !ctrl || !origGeomTree || !selectedA2d)
    return;

  if (animProgress < 1.f)
  {
    animProgress += dt / animTime;
    int animKey = lerp(firstKey, lastKey, saturate(animProgress));
    animate(animKey);
  }
}

bool AnimTreePlugin::supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "animTree") == 0; }

void AnimTreePlugin::fillPropPanel(PropertyContainerControlBase &panel)
{
  panel.setEventHandler(this);
  panel.createButton(PID_SELECT_DYN_MODEL, modelName.empty() ? "Select model" : modelName.c_str());
  fillTreePanels(&panel);
}

bool AnimTreePlugin::isEnumOrEnumItem(TLeafHandle leaf, PropertyContainerControlBase *tree)
{
  TLeafHandle parentLeaf = tree->getParentLeaf(leaf);
  TLeafHandle rootLeaf = tree->getParentLeaf(parentLeaf);
  return (rootLeaf && rootLeaf == enumsRootLeaf) || (parentLeaf && parentLeaf == enumsRootLeaf);
}

static bool is_name_exists_in_childs(TLeafHandle parent, PropertyContainerControlBase *tree, const char *new_name)
{
  const int childCount = tree->getChildCount(parent);
  for (int i = 0; i < childCount; ++i)
  {
    TLeafHandle child = tree->getChildLeaf(parent, i);
    if (tree->getCaption(child) == new_name)
      return true;
  }
  return false;
}

static bool create_new_leaf(TLeafHandle parent, PropertyContainerControlBase *tree, PropertyContainerControlBase *panel,
  const char *sample_name, const char *icon_name, int field_pid)
{
  const bool isNameExists = is_name_exists_in_childs(parent, tree, sample_name);
  if (!isNameExists)
  {
    TLeafHandle newLeaf = tree->createTreeLeaf(parent, sample_name, icon_name);
    panel->setText(field_pid, sample_name);
    tree->setSelLeaf(newLeaf);
  }
  else
    logerr("Can't create enum item, name <%s> already registered", sample_name);

  return !isNameExists;
}

TLeafHandle AnimTreePlugin::getEnumsRootLeaf(PropertyContainerControlBase *tree)
{
  return enumsRootLeaf ? enumsRootLeaf : enumsRootLeaf = tree->createTreeLeaf(0, "enum", STATE_ICON);
}

void AnimTreePlugin::onClick(int pcb_id, PropertyContainerControlBase *panel)
{
  if (pcb_id == PID_SELECT_DYN_MODEL)
  {
    selectDynModel();
    panel->setCaption(PID_SELECT_DYN_MODEL, modelName.empty() ? "Select model" : modelName.c_str());
  }
  else if (pcb_id == PID_PARAMS_FIELD_PLAY_ANIMATION)
  {
    animProgress = 0.f;
  }
  else if (pcb_id == PID_ANIM_STATES_ADD_ENUM)
  {
    PropertyContainerControlBase *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
    TLeafHandle leaf = tree->getRootLeaf();
    TLeafHandle enumLeaf = getEnumsRootLeaf(tree);

    if (create_new_leaf(enumLeaf, tree, panel, "enum_name", ENUM_ICON, PID_ANIM_STATES_NAME_FILED))
      panel->setEnabledById(PID_ANIM_STATES_ADD_ENUM_ITEM, /*enabled*/ true);
  }
  else if (pcb_id == PID_ANIM_STATES_ADD_ENUM_ITEM)
  {
    PropertyContainerControlBase *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    if (!leaf || !isEnumOrEnumItem(leaf, tree))
      return;

    TLeafHandle parent = tree->getParentLeaf(leaf);
    if (parent == enumsRootLeaf)
      parent = leaf;

    create_new_leaf(parent, tree, panel, "leaf_item", ENUM_ITEM_ICON, PID_ANIM_STATES_NAME_FILED);
  }
  else if (pcb_id == PID_ANIM_STATES_REMOVE_NODE)
  {
    PropertyContainerControlBase *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    if (!leaf)
      return;

    if (isEnumOrEnumItem(leaf, tree))
    {
      tree->removeLeaf(leaf);
      panel->setText(PID_ANIM_STATES_NAME_FILED, "");
    }
  }
  else if (pcb_id == PID_ANIM_STATES_SAVE)
  {
    PropertyContainerControlBase *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    if (!leaf)
      return;

    if (isEnumOrEnumItem(leaf, tree))
    {
      TLeafHandle parent = tree->getParentLeaf(leaf);
      SimpleString name = panel->getText(PID_ANIM_STATES_NAME_FILED);
      if (!is_name_exists_in_childs(parent, tree, name))
        tree->setCaption(leaf, name);
      else
        logerr("Name \"%s\" is not unique, can't save", name);
    }
  }
  else if (pcb_id == PID_NODE_MASKS_ADD_MASK)
  {
    PropertyContainerControlBase *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
    create_new_leaf(0, tree, panel, "node_mask", NODE_MASK_ICON, PID_NODE_MASKS_NAME_FILED);
    panel->setEnabledById(PID_NODE_MASKS_ADD_NODE, /*enabled*/ true);
  }
  else if (pcb_id == PID_NODE_MASKS_ADD_NODE)
  {
    PropertyContainerControlBase *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    if (!leaf)
      return;

    TLeafHandle parent = tree->getParentLeaf(leaf);
    if (!parent)
      parent = leaf;

    create_new_leaf(parent, tree, panel, "node_leaf", NODE_MASK_LEAF_ICON, PID_NODE_MASKS_NAME_FILED);
  }
  else if (pcb_id == PID_NODE_MASKS_REMOVE_NODE)
  {
    PropertyContainerControlBase *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    if (!leaf)
      return;

    tree->removeLeaf(leaf);
    panel->setText(PID_NODE_MASKS_NAME_FILED, "");
  }
  else if (pcb_id == PID_NODE_MASKS_SAVE)
  {
    PropertyContainerControlBase *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    if (!leaf)
      return;

    TLeafHandle parent = tree->getParentLeaf(leaf);
    SimpleString name = panel->getText(PID_NODE_MASKS_NAME_FILED);
    if (!is_name_exists_in_childs(parent, tree, name))
      tree->setCaption(leaf, name);
    else
      logerr("Name \"%s\" is not unique, can't save", name);
  }
}

static bool fill_field_by_type(PropertyContainerControlBase *panel, const DataBlock *node, int param_idx, int field_idx)
{
  bool isFieldCreated = true;
  if (node->getParamType(param_idx) == DataBlock::TYPE_STRING)
    panel->createEditBox(field_idx, node->getParamName(param_idx), node->getStr(param_idx));
  else if (node->getParamType(param_idx) == DataBlock::TYPE_REAL)
    panel->createEditFloat(field_idx, node->getParamName(param_idx), node->getReal(param_idx), /*prec*/ 2);
  else if (node->getParamType(param_idx) == DataBlock::TYPE_BOOL)
    panel->createCheckBox(field_idx, node->getParamName(param_idx), node->getBool(param_idx));
  else
  {
    logerr("skip param type: %d", node->getParamType(param_idx));
    isFieldCreated = false;
  }

  return isFieldCreated;
}

static void remove_fields(PropertyContainerControlBase *panel)
{
  for (int k = PID_PARAMS_FIELD; k < PID_PARAMS_FIELD_SIZE; ++k)
    if (!panel->removeById(k))
      break;

  panel->removeById(PID_PARAMS_FIELD_PLAY_ANIMATION);
  panel->removeById(PID_PARAMS_FIELD_SAVE);
}

void AnimTreePlugin::onChange(int pcb_id, PropertyContainerControlBase *panel)
{
  if (pcb_id == PID_ANIM_BLEND_NODES_TREE)
  {
    PropertyContainerControlBase *tree = panel->getById(pcb_id)->getContainer();
    PropertyContainerControlBase *group = panel->getById(PID_ANIM_BLEND_NODES_SETTINGS_GROUP)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    if (!leaf)
      return;

    fillAnimBlendSettings(tree, group);
    loadA2dResource(panel);
  }
  else if (pcb_id == PID_NODE_MASKS_TREE)
  {
    PropertyContainerControlBase *tree = panel->getById(pcb_id)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    panel->setEnabledById(PID_NODE_MASKS_ADD_NODE, static_cast<bool>(leaf));
    if (!leaf)
    {
      panel->setText(PID_NODE_MASKS_NAME_FILED, "");
      return;
    }

    panel->setText(PID_NODE_MASKS_NAME_FILED, tree->getCaption(leaf));
  }
  else if (pcb_id == PID_ANIM_STATES_TREE)
  {
    PropertyContainerControlBase *tree = panel->getById(pcb_id)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    if (!leaf)
    {
      panel->setText(PID_ANIM_STATES_NAME_FILED, "");
      panel->setEnabledById(PID_ANIM_STATES_ADD_ENUM_ITEM, /*enabled*/ false);
      return;
    }

    const bool isEnumSelected = isEnumOrEnumItem(leaf, tree);
    panel->setEnabledById(PID_ANIM_STATES_ADD_ENUM_ITEM, isEnumSelected);
    if (isEnumSelected)
      panel->setText(PID_ANIM_STATES_NAME_FILED, tree->getCaption(leaf));
    else
      panel->setText(PID_ANIM_STATES_NAME_FILED, "");
  }
  else if (pcb_id >= PID_PARAMS_FIELD && pcb_id <= PID_PARAMS_FIELD_SIZE)
  {
    const String &fieldName = fieldNames[pcb_id - PID_PARAMS_FIELD];
    if (fieldName == "time")
      animTime = panel->getFloat(pcb_id);
    else if (selectedA2d && fieldName == "key_start")
      firstKey = get_key_by_name(selectedA2d, panel->getText(pcb_id));
    else if (selectedA2d && fieldName == "key_end")
      lastKey = get_key_by_name(selectedA2d, panel->getText(pcb_id));
  }
}

static void add_anim_bnl_to_tree(int idx, TLeafHandle parent, const DataBlock *node, PropertyContainerControlBase *tree)
{
  const DataBlock *animNode = node->getBlock(idx);
  TLeafHandle animLeaf = tree->createTreeLeaf(parent, animNode->getStr("name"), ANIM_BLEND_NODE_ICON);
  tree->setUserData(animLeaf, (void *)&ANIM_BLEND_NODE_LEAF);
  for (int i = 0; i < animNode->blockCount(); ++i)
  {
    TLeafHandle irqLeaf = tree->createTreeLeaf(animLeaf, animNode->getBlock(i)->getStr("name"), IRQ_ICON);
    tree->setUserData(irqLeaf, (void *)&ANIM_BLEND_NODE_IRQ);
  }
}

void AnimTreePlugin::fillTreePanels(PropertyContainerControlBase *panel)
{
  auto *masksGroup = panel->createGroup(PID_NODE_MASKS_GROUP, "Node masks");
  auto *masksTree = masksGroup->createTree(PID_NODE_MASKS_TREE, "Skeleton node masks", /*height*/ 300);
  masksGroup->createButton(PID_NODE_MASKS_ADD_MASK, "Add mask");
  masksGroup->createButton(PID_NODE_MASKS_ADD_NODE, "Add node", /*enabled*/ false, /*new_line*/ false);
  masksGroup->createButton(PID_NODE_MASKS_REMOVE_NODE, "Remove selected");
  auto *masksSettingsGroup = masksGroup->createGroup(PID_NODE_MASKS_SETTINGS_GROUP, "Selected settings");
  masksSettingsGroup->createEditBox(PID_NODE_MASKS_NAME_FILED, "Field name", "");
  masksSettingsGroup->createButton(PID_NODE_MASKS_SAVE, "Save");
  auto *nodesGroup = panel->createGroup(PID_ANIM_BLEND_NODES_GROUP, "Anim blend nodes");
  auto *nodesTree = nodesGroup->createTree(PID_ANIM_BLEND_NODES_TREE, "Anim blend nodes", /*height*/ 300);
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_CREATE_NODE, "Create blend node");
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_CREATE_LEAF, "Create leaf");
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_REMOVE_NODE, "Remove selected node");
  nodesGroup->createGroup(PID_ANIM_BLEND_NODES_SETTINGS_GROUP, "Selected settings");
  auto *crtlsGroup = panel->createGroup(PID_ANIM_BLEND_CTRLS_GROUP, "Anim blend controllers");
  auto *ctrlsTree = crtlsGroup->createTree(PID_ANIM_BLEND_CTRLS_TREE, "Anim blend controllers", /*height*/ 300);
  auto *statesGroup = panel->createGroup(PID_ANIM_STATES_GROUP, "Anim states");
  auto *statesTree = statesGroup->createTree(PID_ANIM_STATES_TREE, "Anim states", /*height*/ 300);
  statesGroup->createButton(PID_ANIM_STATES_ADD_ENUM, "Add enum");
  statesGroup->createButton(PID_ANIM_STATES_ADD_ENUM_ITEM, "Add item", /*enabled*/ false, /*new_line*/ false);
  statesGroup->createButton(PID_ANIM_STATES_REMOVE_NODE, "Remove selected");
  auto *statesSettingGroup = statesGroup->createGroup(PID_ANIM_STATES_SETTINGS_GROUP, "Selected settings");
  statesSettingGroup->createEditBox(PID_ANIM_STATES_NAME_FILED, "Field name", "");
  statesSettingGroup->createButton(PID_ANIM_STATES_SAVE, "Save");
  const DataBlock &blk = curAsset->props;
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *node = blk.getBlock(i);
    if (!strcmp(node->getBlockName(), "AnimBlendNodeLeaf"))
    {
      if (node->blockCount() == 1)
        add_anim_bnl_to_tree(/*idx*/ 0, /*parent*/ 0, node, nodesTree);
      else
      {
        TLeafHandle nodeLeaf = nodesTree->createTreeLeaf(0, node->getStr("a2d"), A2D_NAME_ICON);
        nodesTree->setUserData(nodeLeaf, (void *)&ANIM_BLEND_NODE_A2D);
        for (int j = 0; j < node->blockCount(); ++j)
          add_anim_bnl_to_tree(j, nodeLeaf, node, nodesTree);
      }
    }
    else if (!strcmp(node->getBlockName(), "nodeMask"))
    {
      TLeafHandle nodeLeaf = masksTree->createTreeLeaf(0, node->getStr("name"), NODE_MASK_ICON);
      for (int j = 0; j < node->paramCount(); ++j)
        if (!strcmp(node->getParamName(j), "node"))
          masksTree->createTreeLeaf(nodeLeaf, node->getStr(j), NODE_MASK_LEAF_ICON);
    }
    else if (!strcmp(node->getBlockName(), "AnimBlendCtrl"))
    {
      for (int j = 0; j < node->blockCount(); ++j)
      {
        const DataBlock *settings = node->getBlock(j);
        ctrlsTree->createTreeLeaf(0, settings->getStr("name"), ANIM_BLEND_CTRL_ICON);
      }
    }
    else if (!strcmp(node->getBlockName(), "stateDesc"))
    {
      TLeafHandle nodeLeaf = statesTree->createTreeLeaf(0, node->getBlockName(), STATE_ICON);
      for (int j = 0; j < node->blockCount(); ++j)
        statesTree->createTreeLeaf(nodeLeaf, node->getBlock(j)->getStr("name"), STATE_LEAF_ICON);
    }
    else if (!strcmp(node->getBlockName(), "initAnimState"))
    {
      TLeafHandle nodeLeaf = statesTree->createTreeLeaf(0, node->getBlockName(), STATE_ICON);
      for (int j = 0; j < node->blockCount(); ++j)
      {
        const DataBlock *settings = node->getBlock(j);
        if (!strcmp(settings->getBlockName(), "enum"))
          fillEnumTree(settings, statesTree);
        else
          statesTree->createTreeLeaf(nodeLeaf, settings->getBlockName(), STATE_LEAF_ICON);
      }
    }
    else if (!strcmp(node->getBlockName(), "preview"))
      statesTree->createTreeLeaf(0, node->getBlockName(), STATE_ICON);
    else
      logerr("skip node: %s with blocks: %d, params %d", node->getBlockName(), node->blockCount(), node->paramCount());
  }
}

void AnimTreePlugin::fillEnumTree(const DataBlock *settings, PropertyContainerControlBase *panel)
{
  enumsRootLeaf = panel->createTreeLeaf(0, settings->getBlockName(), STATE_ICON);
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *enumBlk = settings->getBlock(i);
    TLeafHandle enumLeaf = panel->createTreeLeaf(enumsRootLeaf, enumBlk->getBlockName(), ENUM_ICON);
    for (int j = 0; j < enumBlk->blockCount(); ++j)
      panel->createTreeLeaf(enumLeaf, enumBlk->getBlock(j)->getBlockName(), ENUM_ITEM_ICON);
  }
}

static TLeafHandle get_anim_blend_node_leaf(PropertyContainerControlBase *tree)
{
  TLeafHandle leaf = tree->getSelLeaf();
  if (leaf)
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    if (parent && tree->getUserData(leaf) == &ANIM_BLEND_NODE_IRQ)
      return parent;
    else if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_LEAF)
      return leaf;
    else if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_A2D && tree->getChildCount(leaf) > 0)
      return tree->getChildLeaf(leaf, 0);
    else
      logerr("Can't find anim blend node for this selected leaf");
  }

  return nullptr;
}

static const DataBlock *get_selected_bnl_settings(const DataBlock &props, const char *leaf_name, int &a2d_idx)
{
  for (int i = 0; i < props.blockCount(); ++i)
  {
    const DataBlock *node = props.getBlock(i);
    if (!strcmp(node->getBlockName(), "AnimBlendNodeLeaf"))
    {
      for (int j = 0; j < node->blockCount(); ++j)
      {
        const DataBlock *settings = node->getBlock(j);
        if (!strcmp(settings->getStr("name"), leaf_name))
        {
          a2d_idx = i;
          return settings;
        }
      }
    }
  }
  return nullptr;
}

void AnimTreePlugin::fillAnimBlendSettings(PropertyContainerControlBase *tree, PropertyContainerControlBase *group)
{
  TLeafHandle leaf = get_anim_blend_node_leaf(tree);
  if (!leaf)
    return;

  TLeafHandle selLeaf = tree->getSelLeaf();
  TLeafHandle parent = tree->getParentLeaf(selLeaf);
  const bool isA2dNode = tree->getChildCount(selLeaf) > 0 && tree->getChildLeaf(selLeaf, 0) == leaf;
  const bool isIrqNode = parent && parent == leaf;
  int a2dIdx = -1;
  const DataBlock *settings = get_selected_bnl_settings(curAsset->props, tree->getCaption(leaf), a2dIdx);
  const DataBlock *a2dBlk = curAsset->props.getBlock(a2dIdx);
  remove_fields(group);
  fieldNames.clear();

  if (settings)
  {
    int fieldIdx = PID_PARAMS_FIELD;
    if (isIrqNode)
    {
      for (int i = 0; i < settings->blockCount(); ++i)
      {
        const DataBlock *irq = settings->getBlock(i);
        if (tree->getChildLeaf(parent, i) == selLeaf) // search by leaf becuse irq name may not be unique
          fillAnimBlendFields(group, irq, fieldIdx);
      }
      selectedA2dName.clear();
    }
    else
    {
      fillAnimBlendFields(group, a2dBlk, fieldIdx);
      if (!isA2dNode)
      {
        group->createSeparator(fieldIdx++);
        fieldNames.emplace_back("separator");
        fillAnimBlendFields(group, settings, fieldIdx);
      }
      group->createButton(PID_PARAMS_FIELD_PLAY_ANIMATION, "Play animation");
      selectedA2dName = group->getText(PID_PARAMS_FIELD);
    }
    group->createButton(PID_PARAMS_FIELD_SAVE, "Save");
  }
}

void AnimTreePlugin::fillAnimBlendFields(PropertyContainerControlBase *panel, const DataBlock *node, int &field_idx)
{
  for (int i = 0; i < node->paramCount(); ++i)
  {
    if (fill_field_by_type(panel, node, i, field_idx))
    {
      fieldNames.emplace_back(node->getParamName(i));
      ++field_idx;
    }
  }
}

void AnimTreePlugin::selectDynModel()
{
  String selectMsg(64, "Select dynModel");
  String resetMsg(64, "Reset dynModel");
  int x, y;
  unsigned w, h;

  G_ASSERT(getPropPanel() && "Plugin panel closed!");
  if (!getWndManager().getWindowPosSize(getPropPanel()->getParentWindowHandle(), x, y, w, h))
    return;

  getWndManager().clientToScreen(x, y);
  SelectAssetDlg dlg(0, &curAsset->getMgr(), selectMsg, selectMsg, resetMsg, make_span_const(&dynModelId, 1), x - w - 5, y, w, h);

  int result = dlg.showDialog();
  if (result == DIALOG_ID_CLOSE)
    return;

  modelName = result == DIALOG_ID_OK ? dlg.getSelObjName() : "";
  a2d_last_ref_model = modelName;

  reloadDynModel();
}

void AnimTreePlugin::resetDynModel()
{
  if (ctrl)
    ctrl->setSkeletonForRender(nullptr);
  destroy_it(entity);
  geomTree = GeomNodeTree();
  ctrl = nullptr;
  origGeomTree = nullptr;
  animNodes.clear();
}

void AnimTreePlugin::reloadDynModel()
{
  resetDynModel();
  if (modelName.empty())
    return;

  DagorAsset *modelAsset = curAsset->getMgr().findAsset(modelName, dynModelId);
  if (!modelAsset)
  {
    logerr("Can't find asset for load dynModel: %s", modelName);
    return;
  }

  entity = DAEDITOR3.createEntity(*modelAsset);
  entity->setSubtype(DAEDITOR3.registerEntitySubTypeId("single_ent"));
  ctrl = entity->queryInterface<ILodController>();
  origGeomTree = ctrl->getSkeleton();
  if (!origGeomTree)
    return;
  geomTree = *origGeomTree;
  ctrl->setSkeletonForRender(&geomTree);
}

void AnimTreePlugin::loadA2dResource(PropertyContainerControlBase *panel)
{
  if (selectedA2dName.empty() || !entity || !ctrl || !origGeomTree)
    return;

  if (selectedA2d)
  {
    ::release_game_resource((GameResource *)selectedA2d);
    selectedA2d = nullptr;
  }
  selectedA2d = (AnimV20::AnimData *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(selectedA2dName), Anim2DataGameResClassId);
  animNodes.clear();
  animProgress = 0.f;
  if (!selectedA2d)
    return;

  for (dag::Index16 ni(0), idxEnd(geomTree.nodeCount()); ni != idxEnd; ++ni)
    if (const char *nodeName = geomTree.getNodeName(ni))
      if (SimpleNodeAnim anim; anim.init(selectedA2d, nodeName))
        animNodes.emplace_back(AnimNodeData{nodeName, geomTree.findNodeIndex(nodeName), anim});

  if (int pid = getPidByName("time"); pid != -1)
    animTime = panel->getFloat(pid);
  else
    animTime = 1.f;
  const int firstKeyPid = getPidByName("key_start");
  const int lastKeyPid = getPidByName("key_end");
  if (firstKeyPid != -1 && lastKeyPid != -1)
  {
    firstKey = get_key_by_name(selectedA2d, panel->getText(firstKeyPid));
    lastKey = get_key_by_name(selectedA2d, panel->getText(lastKeyPid));
  }
  else
  {
    const int lastTrackIdx = selectedA2d->dumpData.noteTrack.size() - 1;
    firstKey = 0;
    lastKey = selectedA2d->dumpData.noteTrack[lastTrackIdx].time;
  }

  animate(firstKey);
}

static void blend_add_anim(mat44f &dest_tm, const mat44f &base_tm, const TMatrix &add_tm_s)
{
  vec4f p0, r0, s0;
  vec4f p1, r1, s1;
  mat44f add_tm;
  v_mat44_make_from_43cu(add_tm, add_tm_s.m[0]);
  v_mat4_decompose(base_tm, p0, r0, s0);
  v_mat4_decompose(add_tm, p1, r1, s1);
  v_mat44_compose(dest_tm, v_add(p0, p1), v_quat_mul_quat(r0, r1), v_mul(s0, s1));
}

void AnimTreePlugin::animate(int key)
{
  if (!entity || !ctrl || !origGeomTree || !selectedA2d)
    return;

  for (AnimNodeData &animNode : animNodes)
  {
    TMatrix tm;
    mat44f &nodeTm = geomTree.getNodeTm(animNode.skelIdx);
    animNode.anim.calcAnimTm(tm, key);
    if (selectedA2d->isAdditive())
      blend_add_anim(nodeTm, origGeomTree->getNodeTm(animNode.skelIdx), tm);
    else
    {
      v_stu_p3(&tm.col[3].x, nodeTm.col3);
      geomTree.setNodeTmScalar(animNode.skelIdx, tm);
    }
  }

  geomTree.invalidateWtm();
  geomTree.calcWtm();
  entity->setTm(TMatrix::IDENT);
}

int AnimTreePlugin::getPidByName(const char *name)
{
  G_STATIC_ASSERT((eastl::is_same<decltype(fieldNames)::value_type, String>::value));
  auto it = eastl::find(fieldNames.begin(), fieldNames.end(), name);
  if (it != fieldNames.end())
    return PID_PARAMS_FIELD + eastl::distance(fieldNames.begin(), it);
  else
    return -1;
}
