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
#include <util/dag_lookup.h>

#include <assetsGui/av_selObjDlg.h>
#include <assets/assetMgr.h>
#include <sepGui/wndPublic.h>

#include <propPanel2/c_window_base.h>

using hdpi::_pxScaled;

enum
{
  PID_SELECT_DYN_MODEL,

  PID_NODE_MASKS_GROUP,
  PID_NODE_MASKS_FILTER,
  PID_NODE_MASKS_TREE,
  PID_NODE_MASKS_ADD_MASK,
  PID_NODE_MASKS_ADD_NODE,
  PID_NODE_MASKS_REMOVE_NODE,
  PID_NODE_MASKS_SETTINGS_GROUP,
  PID_NODE_MASKS_NAME_FIELD,
  PID_NODE_MASKS_SAVE,

  PID_ANIM_BLEND_NODES_GROUP,
  PID_ANIM_BLEND_NODES_SETTINGS_GROUP,
  PID_ANIM_BLEND_NODES_FILTER,
  PID_ANIM_BLEND_NODES_TREE,
  PID_ANIM_BLEND_NODES_A2D,
  PID_ANIM_BLEND_NODES_LEAF,
  PID_ANIM_BLEND_NODES_IRQ,
  PID_ANIM_BLEND_NODES_CREATE_NODE,
  PID_ANIM_BLEND_NODES_ADD_LEAF,
  PID_ANIM_BLEND_NODES_ADD_IRQ,
  PID_ANIM_BLEND_NODES_REMOVE_NODE,

  PID_ANIM_BLEND_CTRLS_GROUP,
  PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP,
  PID_ANIM_BLEND_CTRLS_FILTER,
  PID_ANIM_BLEND_CTRLS_TREE,

  PID_ANIM_STATES_GROUP,
  PID_ANIM_STATES_FILTER,
  PID_ANIM_STATES_TREE,
  PID_ANIM_STATES_ADD_ENUM,
  PID_ANIM_STATES_ADD_ENUM_ITEM,
  PID_ANIM_STATES_REMOVE_NODE,
  PID_ANIM_STATES_SETTINGS_GROUP,
  PID_ANIM_STATES_NAME_FIELD,
  PID_ANIM_STATES_SAVE,

  PID_NODES_PARAMS_FIELD,
  PID_IRQ_INTERRUPTION_FIELD = PID_NODES_PARAMS_FIELD,
  PID_IRQ_NAME_FIELD,
  PID_NODES_PARAMS_FIELD_SIZE = PID_NODES_PARAMS_FIELD + 100,

  PID_NODES_ACTION_FIELD,
  PID_IRQ_TYPE_COMBO_SELECT = PID_NODES_ACTION_FIELD,
  PID_NODES_SETTINGS_PLAY_ANIMATION,
  PID_NODES_SETTINGS_SAVE,
  PID_NODES_ACTION_FIELD_SIZE,

  PID_CTRLS_PARAMS_FIELD,
  PID_CTRLS_PARAMS_FIELD_SIZE = PID_CTRLS_PARAMS_FIELD + 100,

  PID_CTRLS_ACTION_FIELD,
  PID_CTRLS_TYPE_COMBO_SELECT = PID_CTRLS_ACTION_FIELD,
  PID_CTRLS_SEPARATOR,

  PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT,
  PID_CTRLS_PARAM_SWITCH_ENUM_GEN_COMBO_SELECT,
  PID_CTRLS_PARAM_SWITCH_NAME_TEMPLATE,

  PID_CTRLS_PARAM_SWITCH_NODES_LIST,
  PID_CTRLS_PARAM_SWITCH_NODES_LIST_ADD,
  PID_CTRLS_PARAM_SWITCH_NODES_LIST_REMOVE,
  PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM,
  PID_CTRLS_PARAM_SWITCH_NODE_NAME,
  PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM,
  PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO,

  PID_CTRLS_SETTINGS_SAVE,
  PID_CTRLS_ACTION_FIELD_SIZE,

};

// Icon names located at D:\dagor2\tools\dagor3_cdk\commonData\gui16x16
static const char *ASSET_SKELETON_ICON = "asset_skeleton";
static const char *RES_ANIMBNL_ICON = "res_animbnl";
static const char *SHOW_PANEL_ICON = "show_panel";
static const char *FOLDER_ICON = "folder";
static const char *CREATE_SPHERE_ICON = "create_sphere";

// Icon names for nodes
static const char *ANIM_BLEND_NODE_ICON = "blend_node";
static const char *A2D_NAME_ICON = "blend_nodes_group";
static const char *NODE_MASK_ICON = "anim_node_mask1";
static const char *NODE_MASK_LEAF_ICON = ASSET_SKELETON_ICON;
static const char *ANIM_BLEND_CTRL_ICON = RES_ANIMBNL_ICON;
static const char *STATE_ICON = "anim_states";
static const char *STATE_LEAF_ICON = "anim_state_leaf";
static const char *ENUMS_ROOT_ICON = FOLDER_ICON;
static const char *ENUM_ICON = SHOW_PANEL_ICON;
static const char *ENUM_ITEM_ICON = CREATE_SPHERE_ICON;
static const char *IRQ_ICON = "irq";

static const char *CTRL_COND_HIDE_ICON = "ctrl_cond_hide";
static const char *CTRL_ATTACH_NODE_ICON = "ctrl_attach_node";
static const char *CTRL_HUB_ICON = "ctrl_hub";
static const char *CTRL_LINEAR_POLY_ICON = "ctrl_linearpoly";
static const char *CTRL_EFF_FROM_ATT_ICON = "ctrl_eff_from_att";
static const char *CTRL_RANDOM_SWITCH_ICON = "ctrl_random_switch";
static const char *CTRL_PARAM_SWITCH_ICON = "ctrl_param_switch";
static const char *CTRL_PARAMS_CTRL_ICON = "ctrl_params_ctrl";
static const char *CTRL_FIFO3_ICON = "ctrl_fifo3";
static const char *CTRL_SET_PARAM_ICON = "ctrl_set_param";
static const char *CTRL_MOVE_NODE_ICON = "move";
static const char *CTRL_ROTATE_NODE_ICON = "rotate";

static constexpr int ANIM_BLEND_NODE_A2D = PID_ANIM_BLEND_NODES_A2D;
static constexpr int ANIM_BLEND_NODE_LEAF = PID_ANIM_BLEND_NODES_LEAF;
static constexpr int ANIM_BLEND_NODE_IRQ = PID_ANIM_BLEND_NODES_IRQ;

extern String a2d_last_ref_model;

static Tab<String> irq_types{String("key"), String("relPos"), String("keyFloat")};
enum IrqType
{
  IRQ_TYPE_UNSET = -1,
  IRQ_TYPE_KEY,
  IRQ_TYPE_REL_POS,
  IRQ_TYPE_KEY_FLOAT,
  IRQ_TYPE_SIZE
};

#define CONTROLLERS_LIST                                                                                                       \
  DECL_CTRL(null), DECL_CTRL(setParam), DECL_CTRL(compoundRotateShift), DECL_CTRL(attachNode), DECL_CTRL(alias),               \
    DECL_CTRL(paramFromNode), DECL_CTRL(effFromAttachment), DECL_CTRL(effectorFromChildIK), DECL_CTRL(multiChainFABRIK),       \
    DECL_CTRL(nodesFromAttachement), DECL_CTRL(matFromNode), DECL_CTRL(hub), DECL_CTRL(paramSwitch), DECL_CTRL(linearPoly),    \
    DECL_CTRL(rotateNode), DECL_CTRL(rotateAroundNode), DECL_CTRL(paramsCtrl), DECL_CTRL(randomSwitch), DECL_CTRL(alignEx),    \
    DECL_CTRL(alignNode), DECL_CTRL(moveNode), DECL_CTRL(fifo3), DECL_CTRL(legsIK), DECL_CTRL(twistCtrl), DECL_CTRL(condHide), \
    DECL_CTRL(lookat), DECL_CTRL(scaleNode), DECL_CTRL(defClampCtrl)

#define DECL_CTRL(x) String(#x)
Tab<String> ctrl_type = {CONTROLLERS_LIST};
#undef DECL_CTRL

#define DECL_CTRL(x) ctrl_type_##x
enum CtrlType
{
  ctrl_type_not_found = -1,
  CONTROLLERS_LIST,
  ctrl_type_size
};
#undef DECL_CTRL

static Tab<String> param_switch_type({String("Generated from enum"), String("Enum list")});
enum ParamSwitchType
{
  PARAM_SWITCH_TYPE_ENUM_GEN,
  PARAM_SWITCH_TYPE_ENUM_LIST,
};

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
  return enumsRootLeaf ? enumsRootLeaf : enumsRootLeaf = tree->createTreeLeaf(tree->getRootLeaf(), "enum", ENUMS_ROOT_ICON);
}

static void remove_fields(PropertyContainerControlBase *panel, int field_start, int field_end, bool break_if_not_found)
{
  for (int i = field_start; i < field_end; ++i)
    if (!panel->removeById(i) && break_if_not_found)
      return;
}

static void remove_nodes_fields(PropertyContainerControlBase *panel)
{
  const bool breakIfNotFound = true;
  remove_fields(panel, PID_NODES_PARAMS_FIELD, PID_NODES_PARAMS_FIELD_SIZE, breakIfNotFound);
  remove_fields(panel, PID_NODES_ACTION_FIELD, PID_NODES_ACTION_FIELD_SIZE, !breakIfNotFound);
}

static void remove_ctrls_fields(PropertyContainerControlBase *panel)
{
  const bool breakIfNotFound = true;
  remove_fields(panel, PID_CTRLS_PARAMS_FIELD, PID_CTRLS_PARAMS_FIELD_SIZE, breakIfNotFound);
  remove_fields(panel, PID_CTRLS_ACTION_FIELD, PID_CTRLS_ACTION_FIELD_SIZE, !breakIfNotFound);
}

static void create_irq_interruption_moment_type_combo(PropertyContainerControlBase *panel, const char *type_name = "",
  IrqType selected_type = IRQ_TYPE_UNSET)
{
  if (selected_type == IRQ_TYPE_UNSET)
    selected_type = static_cast<IrqType>(lup(type_name, irq_types, IRQ_TYPE_KEY));
  G_ASSERT(irq_types.size() == IRQ_TYPE_SIZE);
  G_ASSERTF(selected_type < IRQ_TYPE_SIZE, "Wrong selected idx for interruption type");
  panel->createCombo(PID_IRQ_TYPE_COMBO_SELECT, "Interruption moment type", irq_types, selected_type);
}

static void fill_irq_settings(PropertyContainerControlBase *panel, const char *irq_name, IrqType selected_type, const char *key = "",
  float key_float = 0.0f)
{
  remove_nodes_fields(panel);
  create_irq_interruption_moment_type_combo(panel, "", selected_type);
  SimpleString typeName = panel->getText(PID_IRQ_TYPE_COMBO_SELECT);
  if (selected_type == IRQ_TYPE_KEY)
    panel->createEditBox(PID_IRQ_INTERRUPTION_FIELD, typeName.c_str(), key);
  else
    panel->createEditFloat(PID_IRQ_INTERRUPTION_FIELD, typeName.c_str(), key_float, /*prec*/ 3);
  panel->createEditBox(PID_IRQ_NAME_FIELD, "name", irq_name);
  panel->createButton(PID_NODES_SETTINGS_SAVE, "Save");
}

static int get_irq_idx(PropertyContainerControlBase *tree, TLeafHandle leaf)
{
  TLeafHandle parent = tree->getParentLeaf(leaf);
  for (int i = 0; i < tree->getChildCount(parent); ++i)
  {
    TLeafHandle child = tree->getChildLeaf(parent, i);
    if (child == leaf)
      return i;
  }
  G_ASSERT_FAIL("Can't find irq idx");
  return 0;
}

void AnimTreePlugin::onClick(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case PID_SELECT_DYN_MODEL:
      selectDynModel();
      panel->setCaption(PID_SELECT_DYN_MODEL, modelName.empty() ? "Select model" : modelName.c_str());
      break;

    case PID_NODES_SETTINGS_PLAY_ANIMATION: animProgress = 0.f; break;
    case PID_ANIM_STATES_ADD_ENUM: addEnumToAnimStatesTree(panel); break;
    case PID_ANIM_STATES_ADD_ENUM_ITEM: addEnumItemToAnimStatesTree(panel); break;
    case PID_ANIM_STATES_REMOVE_NODE: removeNodeFromAnimStatesTree(panel); break;
    case PID_ANIM_STATES_SAVE: saveSettingsAnimStatesTree(panel); break;
    case PID_NODE_MASKS_ADD_MASK: addMaskToNodeMasksTree(panel); break;
    case PID_NODE_MASKS_ADD_NODE: addNodeToNodeMasksTree(panel); break;
    case PID_NODE_MASKS_REMOVE_NODE: removeNodeFromNodeMasksTree(panel); break;
    case PID_NODE_MASKS_SAVE: saveSettingsNodeMasksTree(panel); break;
    case PID_ANIM_BLEND_NODES_ADD_IRQ: addIrqNodeToAnimNodesTree(panel); break;
    case PID_ANIM_BLEND_NODES_REMOVE_NODE: removeNodeFromAnimNodesTree(panel); break;
    case PID_NODES_SETTINGS_SAVE: saveSettingsAnimNodesTree(panel); break;
    case PID_CTRLS_PARAM_SWITCH_NODES_LIST_ADD: addNodeParamSwitchList(panel); break;
    case PID_CTRLS_PARAM_SWITCH_NODES_LIST_REMOVE: removeNodeParamSwitchList(panel); break;
    case PID_CTRLS_SETTINGS_SAVE: saveControllerSettings(panel); break;
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
  else if (node->getParamType(param_idx) == DataBlock::TYPE_POINT3)
    panel->createPoint3(field_idx, node->getParamName(param_idx), node->getPoint3(param_idx));
  else
  {
    logerr("skip param type: %d", node->getParamType(param_idx));
    isFieldCreated = false;
  }

  return isFieldCreated;
}

void AnimTreePlugin::onChange(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case PID_ANIM_BLEND_NODES_TREE: selectedChangedAnimNodesTree(panel); break;
    case PID_NODE_MASKS_TREE: selectedChangedNodeMasksTree(panel); break;
    case PID_ANIM_STATES_TREE: selectedChangedAnimStatesTree(panel); break;
    case PID_IRQ_TYPE_COMBO_SELECT: irqTypeSelected(panel); break;
    case PID_ANIM_BLEND_CTRLS_TREE: fillCtrlsSettings(panel); break;
    case PID_CTRLS_PARAM_SWITCH_NODES_LIST: setSelectedParamSwitchSettings(panel); break;
    case PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT: changeParamSwitchType(panel); break;
    case PID_NODE_MASKS_FILTER: setTreeFilter(panel, PID_NODE_MASKS_TREE, pcb_id); break;
    case PID_ANIM_BLEND_NODES_FILTER: setTreeFilter(panel, PID_ANIM_BLEND_NODES_TREE, pcb_id); break;
    case PID_ANIM_BLEND_CTRLS_FILTER: setTreeFilter(panel, PID_ANIM_BLEND_CTRLS_TREE, pcb_id); break;
    case PID_ANIM_STATES_FILTER: setTreeFilter(panel, PID_ANIM_STATES_TREE, pcb_id); break;
  }

  if (pcb_id >= PID_NODES_PARAMS_FIELD && pcb_id <= PID_NODES_PARAMS_FIELD_SIZE)
    animNodesFieldChanged(panel, pcb_id);
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

static const char *get_ctrl_icon_name(const char *name)
{
  CtrlType type = static_cast<CtrlType>(lup(name, ctrl_type, ctrl_type_not_found));
  switch (type)
  {
    case ctrl_type_paramSwitch: return CTRL_PARAM_SWITCH_ICON;
    case ctrl_type_condHide: return CTRL_COND_HIDE_ICON;
    case ctrl_type_attachNode: return CTRL_ATTACH_NODE_ICON;
    case ctrl_type_hub: return CTRL_HUB_ICON;
    case ctrl_type_linearPoly: return CTRL_LINEAR_POLY_ICON;
    case ctrl_type_effFromAttachment: return CTRL_EFF_FROM_ATT_ICON;
    case ctrl_type_randomSwitch: return CTRL_RANDOM_SWITCH_ICON;
    case ctrl_type_paramsCtrl: return CTRL_PARAMS_CTRL_ICON;
    case ctrl_type_fifo3: return CTRL_FIFO3_ICON;
    case ctrl_type_setParam: return CTRL_SET_PARAM_ICON;
    case ctrl_type_moveNode: return CTRL_MOVE_NODE_ICON;
    case ctrl_type_rotateNode: return CTRL_ROTATE_NODE_ICON;

    default: return ANIM_BLEND_CTRL_ICON;
  }
}

void AnimTreePlugin::fillTreePanels(PropertyContainerControlBase *panel)
{
  G_ASSERT(ctrl_type.size() == ctrl_type_size);
  auto *masksGroup = panel->createGroup(PID_NODE_MASKS_GROUP, "Node masks");
  masksGroup->createEditBox(PID_NODE_MASKS_FILTER, "Filter");
  auto *masksTree = masksGroup->createTree(PID_NODE_MASKS_TREE, "Skeleton node masks", /*height*/ _pxScaled(300));
  masksGroup->createButton(PID_NODE_MASKS_ADD_MASK, "Add mask");
  masksGroup->createButton(PID_NODE_MASKS_ADD_NODE, "Add node", /*enabled*/ false, /*new_line*/ false);
  masksGroup->createButton(PID_NODE_MASKS_REMOVE_NODE, "Remove selected");
  auto *masksSettingsGroup = masksGroup->createGroup(PID_NODE_MASKS_SETTINGS_GROUP, "Selected settings");
  masksSettingsGroup->createEditBox(PID_NODE_MASKS_NAME_FIELD, "Field name", "");
  masksSettingsGroup->createButton(PID_NODE_MASKS_SAVE, "Save");
  auto *nodesGroup = panel->createGroup(PID_ANIM_BLEND_NODES_GROUP, "Anim blend nodes");
  nodesGroup->createEditBox(PID_ANIM_BLEND_NODES_FILTER, "Filter");
  auto *nodesTree = nodesGroup->createTree(PID_ANIM_BLEND_NODES_TREE, "Anim blend nodes", /*height*/ _pxScaled(300));
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_CREATE_NODE, "Create blend node");
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_ADD_LEAF, "Create leaf");
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_ADD_IRQ, "Add irq", /*enabled*/ false, /*new_line*/ false);
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_REMOVE_NODE, "Remove selected");
  nodesGroup->createGroup(PID_ANIM_BLEND_NODES_SETTINGS_GROUP, "Selected settings");
  auto *crtlsGroup = panel->createGroup(PID_ANIM_BLEND_CTRLS_GROUP, "Anim blend controllers");
  crtlsGroup->createEditBox(PID_ANIM_BLEND_CTRLS_FILTER, "Filter");
  auto *ctrlsTree = crtlsGroup->createTree(PID_ANIM_BLEND_CTRLS_TREE, "Anim blend controllers", /*height*/ _pxScaled(300));
  crtlsGroup->createGroup(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP, "Selected settings");
  auto *statesGroup = panel->createGroup(PID_ANIM_STATES_GROUP, "Anim states");
  statesGroup->createEditBox(PID_ANIM_STATES_FILTER, "Filter");
  auto *statesTree = statesGroup->createTree(PID_ANIM_STATES_TREE, "Anim states", /*height*/ _pxScaled(300));
  statesGroup->createButton(PID_ANIM_STATES_ADD_ENUM, "Add enum");
  statesGroup->createButton(PID_ANIM_STATES_ADD_ENUM_ITEM, "Add item", /*enabled*/ false, /*new_line*/ false);
  statesGroup->createButton(PID_ANIM_STATES_REMOVE_NODE, "Remove selected");
  auto *statesSettingGroup = statesGroup->createGroup(PID_ANIM_STATES_SETTINGS_GROUP, "Selected settings");
  statesSettingGroup->createEditBox(PID_ANIM_STATES_NAME_FIELD, "Field name", "");
  statesSettingGroup->createButton(PID_ANIM_STATES_SAVE, "Save");
  const DataBlock &blk = curAsset->props;
  TLeafHandle masksRoot = masksTree->createTreeLeaf(0, "Root", FOLDER_ICON);
  TLeafHandle nodesRoot = nodesTree->createTreeLeaf(0, "Root", FOLDER_ICON);
  TLeafHandle ctrlsRoot = ctrlsTree->createTreeLeaf(0, "Root", FOLDER_ICON);
  TLeafHandle statesRoot = statesTree->createTreeLeaf(0, "Root", FOLDER_ICON);
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *node = blk.getBlock(i);
    if (!strcmp(node->getBlockName(), "AnimBlendNodeLeaf"))
    {
      if (node->blockCount() == 1)
        add_anim_bnl_to_tree(/*idx*/ 0, nodesRoot, node, nodesTree);
      else
      {
        TLeafHandle nodeLeaf = nodesTree->createTreeLeaf(nodesRoot, node->getStr("a2d"), A2D_NAME_ICON);
        nodesTree->setUserData(nodeLeaf, (void *)&ANIM_BLEND_NODE_A2D);
        for (int j = 0; j < node->blockCount(); ++j)
          add_anim_bnl_to_tree(j, nodeLeaf, node, nodesTree);
      }
    }
    else if (!strcmp(node->getBlockName(), "nodeMask"))
    {
      TLeafHandle nodeLeaf = masksTree->createTreeLeaf(masksRoot, node->getStr("name"), NODE_MASK_ICON);
      for (int j = 0; j < node->paramCount(); ++j)
        if (!strcmp(node->getParamName(j), "node"))
          masksTree->createTreeLeaf(nodeLeaf, node->getStr(j), NODE_MASK_LEAF_ICON);
    }
    else if (!strcmp(node->getBlockName(), "AnimBlendCtrl"))
    {
      for (int j = 0; j < node->blockCount(); ++j)
      {
        const DataBlock *settings = node->getBlock(j);
        const char *iconName = get_ctrl_icon_name(settings->getBlockName());
        TLeafHandle leaf = ctrlsTree->createTreeLeaf(ctrlsRoot, settings->getStr("name"), iconName);
        const char *typeName = settings->getBlockName();
        CtrlType type = static_cast<CtrlType>(lup(typeName, ctrl_type, ctrl_type_not_found));
        if (type == ctrl_type_not_found)
          logerr("Type: <%s> not found in controllers types", typeName);
        controllersData.push_back({leaf, type});
      }
    }
    else if (!strcmp(node->getBlockName(), "stateDesc"))
    {
      TLeafHandle nodeLeaf = statesTree->createTreeLeaf(statesRoot, node->getBlockName(), STATE_ICON);
      for (int j = 0; j < node->blockCount(); ++j)
        statesTree->createTreeLeaf(nodeLeaf, node->getBlock(j)->getStr("name"), STATE_LEAF_ICON);
    }
    else if (!strcmp(node->getBlockName(), "initAnimState"))
    {
      TLeafHandle nodeLeaf = statesTree->createTreeLeaf(statesRoot, node->getBlockName(), STATE_ICON);
      for (int j = 0; j < node->blockCount(); ++j)
      {
        const DataBlock *settings = node->getBlock(j);
        if (!strcmp(settings->getBlockName(), "enum"))
          fillEnumTree(settings, statesTree, statesRoot);
        else
          statesTree->createTreeLeaf(nodeLeaf, settings->getBlockName(), STATE_LEAF_ICON);
      }
    }
    else if (!strcmp(node->getBlockName(), "preview"))
      statesTree->createTreeLeaf(statesRoot, node->getBlockName(), STATE_ICON);
    else
      logerr("skip node: %s with blocks: %d, params %d", node->getBlockName(), node->blockCount(), node->paramCount());
  }
  masksTree->setBool(masksRoot, /*open*/ true);
  nodesTree->setBool(nodesRoot, /*open*/ true);
  ctrlsTree->setBool(ctrlsRoot, /*open*/ true);
  statesTree->setBool(statesRoot, /*open*/ true);
}

void AnimTreePlugin::fillEnumTree(const DataBlock *settings, PropertyContainerControlBase *panel, TLeafHandle tree_root)
{
  enumsRootLeaf = panel->createTreeLeaf(tree_root, settings->getBlockName(), ENUMS_ROOT_ICON);
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
  remove_nodes_fields(group);
  fieldNames.clear();

  if (settings)
  {
    int fieldIdx = PID_NODES_PARAMS_FIELD;
    if (isIrqNode)
    {
      for (int i = 0; i < settings->blockCount(); ++i)
      {
        const DataBlock *irq = settings->getBlock(i);
        if (tree->getChildLeaf(parent, i) == selLeaf) // search by leaf becuse irq name may not be unique
        {
          for (int i = 0; i < irq_types.size(); ++i)
          {
            const char *typeName = irq_types[i].c_str();
            if (irq->paramExists(typeName))
            {
              const char *key = "";
              float keyFloat = 0.f;
              IrqType type = static_cast<IrqType>(i);
              if (type == IRQ_TYPE_KEY)
                key = irq->getStr(typeName);
              else
                keyFloat = irq->getReal(typeName);
              fill_irq_settings(group, irq->getStr("name"), type, key, keyFloat);
              break;
            }
          }
        }
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
      group->createButton(PID_NODES_SETTINGS_PLAY_ANIMATION, "Play animation");
      selectedA2dName = group->getText(PID_NODES_PARAMS_FIELD);
      group->createButton(PID_NODES_SETTINGS_SAVE, "Save");
    }
  }
}

static void fill_param_data(const DataBlock *settings, dag::Vector<AnimParamData> &params, int idx, int pid)
{
  params.emplace_back(
    AnimParamData{pid, static_cast<DataBlock::ParamType>(settings->getParamType(idx)), String(settings->getParamName(idx))});
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

static DataBlock *find_block_by_name(DataBlock *props, const String &name)
{
  for (int i = 0; i < props->blockCount(); ++i)
  {
    DataBlock *settings = props->getBlock(i);
    if (name == settings->getStr("name"))
      return settings;
  }

  G_ASSERT_FAIL("Can't find block with name <%s>", name);
  return nullptr;
}

void AnimTreePlugin::fillCtrlsSettings(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropertyContainerControlBase *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf || leaf == tree->getRootLeaf())
    return;

  DataBlock *ctrlsBlk = curAsset->props.getBlockByName("AnimBlendCtrl");
  if (!ctrlsBlk)
  {
    logerr("Can't find AnimBlendCtrl block in props");
    return;
  }
  remove_ctrls_fields(group);
  ctrlParams.clear();
  if (DataBlock *settings = find_block_by_name(ctrlsBlk, tree->getCaption(leaf)))
  {
    int fieldIdx = PID_CTRLS_PARAMS_FIELD;
    group->createCombo(PID_CTRLS_TYPE_COMBO_SELECT, "Controller type", ctrl_type, settings->getBlockName());
    for (int j = 0; j < settings->paramCount(); ++j)
    {
      fill_field_by_type(group, settings, j, fieldIdx);
      fill_param_data(settings, ctrlParams, j, fieldIdx);
      ++fieldIdx;
    }
    fillCtrlsParamsSettings(group, settings, fieldIdx);
    fillCtrlsBlocksSettings(group, settings);
    group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save");
  }
}

static bool is_contains_param(dag::Vector<AnimParamData> &params, const char *name)
{
  return eastl::find_if(params.begin(), params.end(), [name](AnimParamData value) { return value.name == name; }) != params.end();
}

static void add_edit_box_if_not_exists(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, int &pid,
  const char *name, const char *default_value = "")
{
  if (!is_contains_param(params, name))
  {
    panel->createEditBox(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_STRING, String(name)});
    ++pid;
  }
}

static void add_edit_float_if_not_exists(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, int &pid,
  const char *name, float default_value = 0.f)
{
  if (!is_contains_param(params, name))
  {
    panel->createEditFloat(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_REAL, String(name)});
    ++pid;
  }
}

static void add_edit_point3_if_not_exists(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, int &pid,
  const char *name, Point3 default_value = Point3(0.f, 0.f, 0.f))
{
  if (!is_contains_param(params, name))
  {
    panel->createPoint3(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_POINT3, String(name)});
    ++pid;
  }
}

static void add_edit_bool_if_not_exists(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, int &pid,
  const char *name, bool default_value = false)
{
  if (!is_contains_param(params, name))
  {
    panel->createCheckBox(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_BOOL, String(name)});
    ++pid;
  }
}

static void fill_param_switch_params(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, int field_idx)
{
  const char *nameParam = "name";
  const char *varnameParam = "varname";
  const char *morphTimeParam = "morphTime";
  const char *acceptNameMaskReParam = "accept_name_mask_re";
  const float defaultMorphTime = 0.15f;

  add_edit_box_if_not_exists(params, panel, field_idx, nameParam);
  add_edit_box_if_not_exists(params, panel, field_idx, varnameParam);
  add_edit_float_if_not_exists(params, panel, field_idx, morphTimeParam, defaultMorphTime);
  add_edit_box_if_not_exists(params, panel, field_idx, acceptNameMaskReParam);
}

static auto find_param_by_name(const dag::Vector<AnimParamData> &params, const char *name)
{
  auto it = eastl::find_if(params.begin(), params.end(), [name](AnimParamData value) { return value.name == name; });
  if (it == params.end())
    logerr("can't find <%s> param data", name);

  return it;
}

static bool get_bool_param_by_name(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, const char *name)
{
  auto data = find_param_by_name(params, name);
  if (data != params.end())
    return panel->getBool(data->pid);
  else
    G_ASSERT_FAIL("Can't find param <%s>", name);

  return false;
}

static void fill_move_node_params(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, int field_idx)
{
  const char *localAdditiveParam = "localAdditive";
  const char *localAdditive2Param = "localAdditive2";
  const float defaultKMul = 1.0f;
  const Point3 defaultRotAxis = Point3(0.f, 1.f, 0.f);
  const Point3 defaultDirAxis = Point3(0.f, 0.f, 1.f);

  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "param");
  add_edit_box_if_not_exists(params, panel, field_idx, "axis_course");
  add_edit_float_if_not_exists(params, panel, field_idx, "kCourseAdd");
  add_edit_float_if_not_exists(params, panel, field_idx, "kMul", defaultKMul);
  add_edit_float_if_not_exists(params, panel, field_idx, "kAdd");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rotAxis", defaultRotAxis);
  add_edit_point3_if_not_exists(params, panel, field_idx, "dirAxis", defaultDirAxis);
  add_edit_bool_if_not_exists(params, panel, field_idx, localAdditiveParam);
  { // additive param default value depends on value in localAdditiveParam
    const bool localAdditiveValue = get_bool_param_by_name(params, panel, localAdditiveParam);
    add_edit_bool_if_not_exists(params, panel, field_idx, "additive", localAdditiveValue);
  }
  add_edit_bool_if_not_exists(params, panel, field_idx, localAdditive2Param);
  { // saveOtherAxisMove param default value depends on value in localAdditive2Param
    const bool localAdditive2Value = get_bool_param_by_name(params, panel, localAdditive2Param);
    add_edit_bool_if_not_exists(params, panel, field_idx, "saveOtherAxisMove", localAdditive2Value);
  }
}

void AnimTreePlugin::fillCtrlsParamsSettings(PropertyContainerControlBase *panel, const DataBlock *settings, int field_idx)
{
  CtrlType type = static_cast<CtrlType>(lup(settings->getBlockName(), ctrl_type, ctrl_type_not_found));
  switch (type)
  {
    case ctrl_type_paramSwitch: fill_param_switch_params(ctrlParams, panel, field_idx); break;
    case ctrl_type_moveNode: fill_move_node_params(ctrlParams, panel, field_idx); break;

    default: break;
  }
}

void AnimTreePlugin::fillCtrlsBlocksSettings(PropertyContainerControlBase *panel, const DataBlock *settings)
{
  CtrlType type = static_cast<CtrlType>(lup(settings->getBlockName(), ctrl_type, ctrl_type_not_found));
  switch (type)
  {
    case ctrl_type_paramSwitch: fillParamSwitchBlockSettings(panel, settings); break;

    default: break;
  }
}

static void fill_param_switch_enum_list(PropertyContainerControlBase *panel, const DataBlock *nodes)
{
  Tab<String> nodeNames;
  const int defBlockIdx = 0;
  const DataBlock *defBlock = nodes->getBlock(defBlockIdx);
  const char *nameParam = "name";
  const char *rangeFromParam = "rangeFrom";
  const char *rangeToParam = "rangeTo";
  for (int i = 0; i < nodes->blockCount(); ++i)
    nodeNames.emplace_back(nodes->getBlock(i)->getBlockName());
  panel->createList(PID_CTRLS_PARAM_SWITCH_NODES_LIST, "Nodes", nodeNames, defBlockIdx);
  panel->createButton(PID_CTRLS_PARAM_SWITCH_NODES_LIST_ADD, "Add");
  panel->createButton(PID_CTRLS_PARAM_SWITCH_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM, "Enum item", defBlock ? defBlock->getBlockName() : "");
  panel->createEditBox(PID_CTRLS_PARAM_SWITCH_NODE_NAME, nameParam, defBlock ? defBlock->getStr(nameParam, "") : "");
  panel->createEditFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM, rangeFromParam,
    defBlock ? defBlock->getReal(rangeFromParam, 0.f) : 0.f);
  panel->createEditFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO, rangeToParam, defBlock ? defBlock->getReal(rangeToParam, 0.f) : 0.f);
}

void AnimTreePlugin::fillParamSwitchBlockSettings(PropertyContainerControlBase *panel, const DataBlock *settings)
{
  const DataBlock *nodes = settings->getBlockByName("nodes");
  if (!nodes)
  {
    logerr("Param switch controller should have <nodes> block with child nodes");
    return;
  }

  const char *enumGenField = "enum_gen";
  const bool isEnumGenType = nodes->paramExists(enumGenField);
  panel->createSeparator(PID_CTRLS_SEPARATOR);
  panel->createCombo(PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT, "Param swtich type", param_switch_type,
    isEnumGenType ? PARAM_SWITCH_TYPE_ENUM_GEN : PARAM_SWITCH_TYPE_ENUM_LIST);
  if (isEnumGenType)
    fillParamSwitchEnumGen(panel, nodes);
  else
    fill_param_switch_enum_list(panel, nodes);
}

void AnimTreePlugin::fillParamSwitchEnumGen(PropertyContainerControlBase *panel, const DataBlock *nodes)
{
  const char *enumGenField = "enum_gen";
  const char *nameTemplateField = "name_template";
  if (!enumsRootLeaf)
  {
    logerr("Enums root leaf not found. Please add enums to the anim states tree first");
    return;
  }
  DataBlock *initAnimState = curAsset->props.getBlockByName("initAnimState");
  if (!initAnimState)
  {
    logerr("Block <initAnimState> not found in props");
    return;
  }
  DataBlock *enumBlock = initAnimState->getBlockByName("enum");
  if (!enumBlock)
  {
    logerr("Block <enum> not found in props");
    return;
  }

  Tab<String> enumNames;
  for (int i = 0; i < enumBlock->blockCount(); ++i)
    enumNames.emplace_back(enumBlock->getBlock(i)->getBlockName());
  panel->createCombo(PID_CTRLS_PARAM_SWITCH_ENUM_GEN_COMBO_SELECT, enumGenField, enumNames, nodes->getStr(enumGenField, ""));
  panel->createEditBox(PID_CTRLS_PARAM_SWITCH_NAME_TEMPLATE, nameTemplateField, nodes->getStr(nameTemplateField, ""));
}

void AnimTreePlugin::setSelectedParamSwitchSettings(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  DataBlock *ctrlsBlk = curAsset->props.getBlockByName("AnimBlendCtrl");
  if (!ctrlsBlk)
  {
    logerr("Can't find AnimBlendCtrl block in props");
    return;
  }
  if (DataBlock *settings = find_block_by_name(ctrlsBlk, tree->getCaption(leaf)))
  {
    const SimpleString nodeName = panel->getText(PID_CTRLS_PARAM_SWITCH_NODES_LIST);
    const DataBlock *nodes = settings->getBlockByNameEx("nodes");
    const DataBlock *selectedNode = nodes->getBlockByNameEx(nodeName.c_str());
    const char *nameParam = "name";
    const char *rangeFromParam = "rangeFrom";
    const char *rangeToParam = "rangeTo";
    panel->setText(PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM, nodeName.c_str());
    panel->setText(PID_CTRLS_PARAM_SWITCH_NODE_NAME, selectedNode->getStr(nameParam, ""));
    panel->setFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM, selectedNode->getReal(rangeFromParam, 0.f));
    panel->setFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO, selectedNode->getReal(rangeToParam, 0.f));
  }
}

void AnimTreePlugin::changeParamSwitchType(PropertyContainerControlBase *panel)
{
  remove_fields(panel, PID_CTRLS_PARAM_SWITCH_ENUM_GEN_COMBO_SELECT, PID_CTRLS_ACTION_FIELD_SIZE, false);
  ParamSwitchType type = static_cast<ParamSwitchType>(panel->getInt(PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT));
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropertyContainerControlBase *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  DataBlock *ctrlsBlk = curAsset->props.getBlockByName("AnimBlendCtrl");
  if (!ctrlsBlk)
  {
    logerr("Can't find AnimBlendCtrl block in props");
    return;
  }
  if (DataBlock *settings = find_block_by_name(ctrlsBlk, tree->getCaption(leaf)))
  {
    const DataBlock *nodes = settings->getBlockByNameEx("nodes");
    if (type == PARAM_SWITCH_TYPE_ENUM_GEN)
      fillParamSwitchEnumGen(group, nodes);
    else
      fill_param_switch_enum_list(group, nodes);
  }
  group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save");
}

void AnimTreePlugin::addNodeParamSwitchList(PropertyContainerControlBase *panel)
{
  int idx = panel->addString(PID_CTRLS_PARAM_SWITCH_NODES_LIST, "new_node");
  panel->setInt(PID_CTRLS_PARAM_SWITCH_NODES_LIST, idx);
  setSelectedParamSwitchSettings(panel);
}

void AnimTreePlugin::removeNodeParamSwitchList(PropertyContainerControlBase *panel)
{
  const int removeIdx = panel->getInt(PID_CTRLS_PARAM_SWITCH_NODES_LIST);
  const SimpleString removeName = panel->getText(PID_CTRLS_PARAM_SWITCH_NODES_LIST);
  if (removeIdx < 0)
    return;

  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;
  DataBlock *ctrlsBlk = curAsset->props.getBlockByName("AnimBlendCtrl");
  if (!ctrlsBlk)
  {
    logerr("Can't find AnimBlendCtrl block in props");
    return;
  }
  if (DataBlock *settings = find_block_by_name(ctrlsBlk, tree->getCaption(leaf)))
  {
    DataBlock *nodesBlk = settings->getBlockByName("nodes");
    if (!nodesBlk)
      return;
    nodesBlk->removeBlock(removeName.c_str());
  }

  panel->removeString(PID_CTRLS_PARAM_SWITCH_NODES_LIST, removeIdx);
  setSelectedParamSwitchSettings(panel);
}

static void save_params_blk(DataBlock *settings, const dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel)
{
  for (int i = settings->paramCount() - 1; i >= 0; --i)
    settings->removeParam(i);

  for (int i = 0; i < params.size(); ++i)
    switch (params[i].type)
    {
      case DataBlock::TYPE_STRING:
      {
        const SimpleString value = panel->getText(params[i].pid);
        settings->setStr(params[i].name, value.c_str());
        break;
      }
      case DataBlock::TYPE_REAL: settings->setReal(params[i].name, panel->getFloat(params[i].pid)); break;
      case DataBlock::TYPE_BOOL: settings->setBool(params[i].name, panel->getBool(params[i].pid)); break;
      case DataBlock::TYPE_POINT3: settings->setPoint3(params[i].name, panel->getPoint3(params[i].pid)); break;

      default: logerr("can't save param <%s> type: %d", params[i].name, params[i].type); break;
    }
}

void AnimTreePlugin::saveControllerSettings(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  DataBlock *ctrlsBlk = curAsset->props.getBlockByName("AnimBlendCtrl");
  if (!ctrlsBlk)
  {
    logerr("Can't find AnimBlendCtrl block in props");
    return;
  }
  if (DataBlock *settings = find_block_by_name(ctrlsBlk, tree->getCaption(leaf)))
  {
    saveControllerParamsSettings(panel, settings);
    saveControllerBlocksSettings(panel, settings);
    tree->setCaption(leaf, settings->getStr("name"));
  }
}

static void remove_param_if_default_float(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, const char *name,
  float default_value = 0.f)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const float value = panel->getFloat(param->pid);
    if (is_equal_float(value, default_value))
      params.erase(param);
  }
}

static void remove_param_if_default_str(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, const char *name,
  const char *default_value = "")
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const SimpleString value = panel->getText(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

static void remove_param_if_default_point3(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, const char *name,
  Point3 default_value = Point3(0.f, 0.f, 0.f))
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const Point3 value = panel->getPoint3(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

static void remove_param_if_default_bool(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel, const char *name,
  bool default_value = false)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const bool value = panel->getBool(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

static void remove_unchanged_default_params_param_switch(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel)
{
  const char *morphTimeParam = "morphTime";
  const char *acceptNameMaskReParam = "accept_name_mask_re";
  const float defaultMorphTime = 0.15f;

  remove_param_if_default_float(params, panel, morphTimeParam, defaultMorphTime);
  remove_param_if_default_str(params, panel, acceptNameMaskReParam);
}

static void remove_unchanged_default_params_move_node(dag::Vector<AnimParamData> &params, PropertyContainerControlBase *panel)
{
  const char *localAdditiveParam = "localAdditive";
  const char *localAdditive2Param = "localAdditive2";
  const float defaultKMul = 1.0f;
  const Point3 defaultRotAxis = Point3(0.f, 1.f, 0.f);
  const Point3 defaultDirAxis = Point3(0.f, 0.f, 1.f);
  const bool localAdditiveValue = get_bool_param_by_name(params, panel, localAdditiveParam);
  const bool localAdditive2Value = get_bool_param_by_name(params, panel, localAdditive2Param);

  remove_param_if_default_str(params, panel, "axis_course");
  remove_param_if_default_float(params, panel, "kCourseAdd");
  remove_param_if_default_float(params, panel, "kMul", defaultKMul);
  remove_param_if_default_float(params, panel, "kAdd");
  remove_param_if_default_point3(params, panel, "rotAxis", defaultRotAxis);
  remove_param_if_default_point3(params, panel, "dirAxis", defaultDirAxis);
  remove_param_if_default_bool(params, panel, localAdditiveParam);
  remove_param_if_default_bool(params, panel, "additive", localAdditiveValue);
  remove_param_if_default_bool(params, panel, localAdditive2Param);
  remove_param_if_default_bool(params, panel, "saveOtherAxisMove", localAdditive2Value);
}

void AnimTreePlugin::saveControllerParamsSettings(PropertyContainerControlBase *panel, DataBlock *settings)
{
  // Make copy, becuse we can don't save optinal fields
  dag::Vector<AnimParamData> params = ctrlParams;

  CtrlType type = static_cast<CtrlType>(lup(settings->getBlockName(), ctrl_type, ctrl_type_not_found));
  switch (type)
  {
    case ctrl_type_paramSwitch: remove_unchanged_default_params_param_switch(params, panel); break;
    case ctrl_type_moveNode: remove_unchanged_default_params_move_node(params, panel); break;

    default: break;
  }

  save_params_blk(settings, params, panel);
}

void AnimTreePlugin::saveControllerBlocksSettings(PropertyContainerControlBase *panel, DataBlock *settings)
{
  CtrlType type = static_cast<CtrlType>(lup(settings->getBlockName(), ctrl_type, ctrl_type_not_found));
  switch (type)
  {
    case ctrl_type_paramSwitch: saveParamSwitchBlockSettings(panel, settings); break;

    default: break;
  }
}

bool is_enum_item_name_exist(const DataBlock *props, const SimpleString &name)
{
  const DataBlock *enums = props->getBlockByNameEx("enum");
  for (int i = 0; i < enums->blockCount(); ++i)
  {
    const DataBlock *enumBlk = enums->getBlock(i);
    for (int j = 0; j < enumBlk->blockCount(); ++j)
      if (name == enumBlk->getBlock(j)->getBlockName())
        return true;
  }
  return false;
}

void AnimTreePlugin::saveParamSwitchBlockSettings(PropertyContainerControlBase *panel, DataBlock *settings)
{
  ParamSwitchType type = static_cast<ParamSwitchType>(panel->getInt(PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT));
  DataBlock *nodes = settings->getBlockByName("nodes");
  if (!nodes)
    nodes = settings->addBlock("nodes");

  const char *enumGenField = "enum_gen";
  const char *nameTemplateField = "name_template";
  if (type == PARAM_SWITCH_TYPE_ENUM_GEN)
  {
    // if enumGenField not found then it can be enum list and we want remove all this blocks
    if (!nodes->paramExists(enumGenField))
      for (int i = nodes->blockCount() - 1; i >= 0; --i)
        nodes->removeBlock(i);
    const SimpleString eunmGenSelect = panel->getText(PID_CTRLS_PARAM_SWITCH_ENUM_GEN_COMBO_SELECT);
    const SimpleString nameTemplate = panel->getText(PID_CTRLS_PARAM_SWITCH_NAME_TEMPLATE);
    nodes->setStr(enumGenField, eunmGenSelect.c_str());
    nodes->setStr(nameTemplateField, nameTemplate.c_str());
  }
  else
  {
    const SimpleString enumNameList = panel->getText(PID_CTRLS_PARAM_SWITCH_NODES_LIST);
    const SimpleString enumItemName = panel->getText(PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM);
    if (!is_enum_item_name_exist(curAsset->props.getBlockByNameEx("initAnimState"), enumItemName))
    {
      logerr("Can't save settings, <%s> enum item not found in props", enumItemName.c_str());
      return;
    }

    if (nodes->paramExists(enumGenField))
      nodes->removeParam(enumGenField);
    if (nodes->paramExists(nameTemplateField))
      nodes->removeParam(nameTemplateField);

    DataBlock *enumItem = nodes->getBlockByName(enumNameList.c_str());
    if (!enumItem)
      enumItem = nodes->addBlock(enumItemName.c_str());

    if (enumNameList != enumItemName)
    {
      enumItem->changeBlockName(enumItemName.c_str());
      panel->setText(PID_CTRLS_PARAM_SWITCH_NODES_LIST, enumItemName.c_str());
    }

    const SimpleString nodeName = panel->getText(PID_CTRLS_PARAM_SWITCH_NODE_NAME);
    const float rangeFrom = panel->getFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM);
    const float rangeTo = panel->getFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO);
    const char *rangeFromField = "rangeFrom";
    const char *rangeToField = "rangeTo";
    enumItem->setStr("name", nodeName.c_str());
    if (rangeFrom > 0.0f || rangeTo > 0.0f)
    {
      enumItem->setReal(rangeFromField, rangeFrom);
      enumItem->setReal(rangeToField, rangeTo);
    }
    else
    {
      enumItem->removeParam(rangeFromField);
      enumItem->removeParam(rangeToField);
    }
  }
}

void AnimTreePlugin::setTreeFilter(PropertyContainerControlBase *panel, int tree_pid, int filter_pid)
{
  SimpleString filterText = panel->getText(filter_pid);
  panel->setText(tree_pid, String(filterText).toLower().c_str());
}

void AnimTreePlugin::addEnumToAnimStatesTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getRootLeaf();
  TLeafHandle enumLeaf = getEnumsRootLeaf(tree);

  if (create_new_leaf(enumLeaf, tree, panel, "enum_name", ENUM_ICON, PID_ANIM_STATES_NAME_FIELD))
    panel->setEnabledById(PID_ANIM_STATES_ADD_ENUM_ITEM, /*enabled*/ true);
}

void AnimTreePlugin::addEnumItemToAnimStatesTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf || !isEnumOrEnumItem(leaf, tree))
    return;

  TLeafHandle parent = tree->getParentLeaf(leaf);
  if (parent == enumsRootLeaf)
    parent = leaf;

  create_new_leaf(parent, tree, panel, "leaf_item", ENUM_ITEM_ICON, PID_ANIM_STATES_NAME_FIELD);
}

void AnimTreePlugin::removeNodeFromAnimStatesTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  if (isEnumOrEnumItem(leaf, tree))
  {
    tree->removeLeaf(leaf);
    panel->setText(PID_ANIM_STATES_NAME_FIELD, "");
  }
}

void AnimTreePlugin::saveSettingsAnimStatesTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  if (isEnumOrEnumItem(leaf, tree))
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    SimpleString name = panel->getText(PID_ANIM_STATES_NAME_FIELD);
    if (!is_name_exists_in_childs(parent, tree, name))
      tree->setCaption(leaf, name);
    else
      logerr("Name \"%s\" is not unique, can't save", name);
  }
}

void AnimTreePlugin::selectedChangedAnimStatesTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf || leaf == tree->getRootLeaf())
  {
    panel->setText(PID_ANIM_STATES_NAME_FIELD, "");
    panel->setEnabledById(PID_ANIM_STATES_ADD_ENUM_ITEM, /*enabled*/ false);
    return;
  }

  const bool isEnumSelected = isEnumOrEnumItem(leaf, tree);
  panel->setEnabledById(PID_ANIM_STATES_ADD_ENUM_ITEM, isEnumSelected);
  if (isEnumSelected)
    panel->setText(PID_ANIM_STATES_NAME_FIELD, tree->getCaption(leaf));
  else
    panel->setText(PID_ANIM_STATES_NAME_FIELD, "");
}

void AnimTreePlugin::addMaskToNodeMasksTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  create_new_leaf(0, tree, panel, "node_mask", NODE_MASK_ICON, PID_NODE_MASKS_NAME_FIELD);
  panel->setEnabledById(PID_NODE_MASKS_ADD_NODE, /*enabled*/ true);
}

void AnimTreePlugin::addNodeToNodeMasksTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  TLeafHandle parent = tree->getParentLeaf(leaf);
  if (!parent)
    parent = leaf;

  create_new_leaf(parent, tree, panel, "node_leaf", NODE_MASK_LEAF_ICON, PID_NODE_MASKS_NAME_FIELD);
}

void AnimTreePlugin::removeNodeFromNodeMasksTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  tree->removeLeaf(leaf);
  panel->setText(PID_NODE_MASKS_NAME_FIELD, "");
}

void AnimTreePlugin::saveSettingsNodeMasksTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  TLeafHandle parent = tree->getParentLeaf(leaf);
  SimpleString name = panel->getText(PID_NODE_MASKS_NAME_FIELD);
  if (!is_name_exists_in_childs(parent, tree, name))
    tree->setCaption(leaf, name);
  else
    logerr("Name \"%s\" is not unique, can't save", name);
}

void AnimTreePlugin::selectedChangedNodeMasksTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  const bool isSelectedLeafValid = leaf && leaf != tree->getRootLeaf();
  panel->setEnabledById(PID_NODE_MASKS_ADD_NODE, static_cast<bool>(isSelectedLeafValid));
  if (!isSelectedLeafValid)
  {
    panel->setText(PID_NODE_MASKS_NAME_FIELD, "");
    return;
  }

  panel->setText(PID_NODE_MASKS_NAME_FIELD, tree->getCaption(leaf));
}

void AnimTreePlugin::addIrqNodeToAnimNodesTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  PropertyContainerControlBase *group = panel->getById(PID_ANIM_BLEND_NODES_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf || tree->getUserData(leaf) != &ANIM_BLEND_NODE_LEAF)
    return;

  const char *leafName = "irq_name";
  TLeafHandle irqLeaf = tree->createTreeLeaf(leaf, leafName, IRQ_ICON);
  tree->setUserData(irqLeaf, (void *)&ANIM_BLEND_NODE_IRQ);
  tree->setSelLeaf(irqLeaf);
  fill_irq_settings(group, leafName, IRQ_TYPE_KEY);
  panel->setEnabledById(PID_ANIM_BLEND_NODES_ADD_IRQ, /*enabled*/ false);
  String bnlName = tree->getCaption(leaf);
  DataBlock *settings = findBnlProps(bnlName.c_str());
  if (!settings)
  {
    logerr("Can't find animBlendNodeLeaf props with name %s", bnlName.c_str());
    return;
  }

  DataBlock *irqProps = settings->addNewBlock("irq");
  irqProps->addStr(irq_types[IRQ_TYPE_KEY], "");
  irqProps->addStr("name", leafName);
}
void AnimTreePlugin::removeNodeFromAnimNodesTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_IRQ)
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    String bnlName = tree->getCaption(parent);
    DataBlock *settings = findBnlProps(bnlName.c_str());
    if (!settings)
    {
      logerr("Can't find animBlendNodeLeaf props with name %s", bnlName.c_str());
      return;
    }
    settings->removeBlock(get_irq_idx(tree, leaf));
    remove_nodes_fields(panel);
    tree->removeLeaf(leaf);
  }
}

void AnimTreePlugin::saveSettingsAnimNodesTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_IRQ)
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    String bnlName = tree->getCaption(parent);
    DataBlock *settings = findBnlProps(bnlName.c_str());
    if (!settings)
    {
      logerr("Can't find animBlendNodeLeaf props with name %s", bnlName.c_str());
      return;
    }
    DataBlock *irqSettings = settings->getBlock(get_irq_idx(tree, leaf));

    const IrqType selectedType = static_cast<IrqType>(panel->getInt(PID_IRQ_TYPE_COMBO_SELECT));
    if (selectedType == IRQ_TYPE_KEY)
    {
      SimpleString key = panel->getText(PID_IRQ_INTERRUPTION_FIELD);
      if (!key.empty())
      {
        for (const String &typeParam : irq_types)
          irqSettings->removeParam(typeParam.c_str());
        irqSettings->addStr(irq_types[selectedType], key.c_str());
      }
      else
        logerr("Irq key can't be empty");
    }
    else if (selectedType == IRQ_TYPE_REL_POS || selectedType == IRQ_TYPE_KEY_FLOAT)
    {
      for (const String &typeParam : irq_types)
        irqSettings->removeParam(typeParam.c_str());
      const float floatParam = panel->getFloat(PID_IRQ_INTERRUPTION_FIELD);
      irqSettings->addReal(irq_types[selectedType], floatParam);
    }

    SimpleString irqName = panel->getText(PID_IRQ_NAME_FIELD);
    if (!irqName.empty())
    {
      tree->setCaption(leaf, irqName.c_str());
      irqSettings->setStr("name", irqName.c_str());
    }
    else
      logerr("New irq name can't be empty");
  }
}

void AnimTreePlugin::selectedChangedAnimNodesTree(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  PropertyContainerControlBase *group = panel->getById(PID_ANIM_BLEND_NODES_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  const bool isSelectedLeafValid = leaf && leaf != tree->getRootLeaf();
  const bool isAnimLeaf = isSelectedLeafValid && (tree->getUserData(leaf) == &ANIM_BLEND_NODE_LEAF);
  panel->setEnabledById(PID_ANIM_BLEND_NODES_ADD_IRQ, isAnimLeaf);
  if (!isSelectedLeafValid)
    return;

  fillAnimBlendSettings(tree, group);
  loadA2dResource(panel);
}

void AnimTreePlugin::irqTypeSelected(PropertyContainerControlBase *panel)
{
  PropertyContainerControlBase *group = panel->getById(PID_ANIM_BLEND_NODES_SETTINGS_GROUP)->getContainer();
  const IrqType selectedType = static_cast<IrqType>(group->getInt(PID_IRQ_TYPE_COMBO_SELECT));
  const SimpleString irqName = group->getText(PID_IRQ_NAME_FIELD);
  fill_irq_settings(group, irqName.c_str(), selectedType);
}

void AnimTreePlugin::animNodesFieldChanged(PropertyContainerControlBase *panel, int pid)
{
  const String &fieldName = fieldNames[pid - PID_NODES_PARAMS_FIELD];
  if (fieldName == "time")
    animTime = panel->getFloat(pid);
  else if (selectedA2d && fieldName == "key_start")
    firstKey = get_key_by_name(selectedA2d, panel->getText(pid));
  else if (selectedA2d && fieldName == "key_end")
    lastKey = get_key_by_name(selectedA2d, panel->getText(pid));
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
    return PID_NODES_PARAMS_FIELD + eastl::distance(fieldNames.begin(), it);
  else
    return -1;
}

DataBlock *AnimTreePlugin::findBnlProps(const char *name)
{
  for (int i = 0; i < curAsset->props.blockCount(); ++i)
  {
    DataBlock *node = curAsset->props.getBlock(i);
    if (!strcmp(node->getBlockName(), "AnimBlendNodeLeaf"))
    {
      for (int j = 0; j < node->blockCount(); ++j)
      {
        DataBlock *settings = node->getBlock(j);
        if (!strcmp(settings->getStr("name"), name))
          return settings;
      }
    }
  }
  return nullptr;
}
