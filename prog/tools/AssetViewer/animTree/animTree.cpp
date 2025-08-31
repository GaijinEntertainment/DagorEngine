// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animTree.h"
#include "../av_plugin.h"
#include "../av_appwnd.h"
#include "animTreePanelPids.h"
#include "animTreeIcons.h"
#include "animTreeUtils.h"

#include "controllers/ctrlType.h"
#include "controllers/moveNode.h"
#include "controllers/rotateNode.h"
#include "controllers/paramSwitch.h"
#include "controllers/randomSwitch.h"
#include "controllers/hub.h"
#include "controllers/linearPoly.h"
#include "controllers/rotateAroundNode.h"
#include "controllers/eyeCtrl.h"
#include "controllers/fifo3.h"

#include "blendNodes/blendNodeType.h"
#include "blendNodes/single.h"
#include "blendNodes/continuous.h"
#include "blendNodes/still.h"
#include "blendNodes/parametric.h"

#include "animStates/animStatesType.h"
#include "animStates/stateDesc.h"
#include "animStates/chan.h"
#include "animStates/stateAlias.h"
#include "animStates/state.h"
#include "animStates/rootProps.h"

#include "nodeMasks/nodeMaskType.h"

#include <util/dag_lookup.h>

#include <assetsGui/av_selObjDlg.h>
#include <assets/assetMgr.h>
#include <EditorCore/ec_wndPublic.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlockCommentsDef.h>
#include <de3_interface.h>

using hdpi::_pxScaled;

static const char *NODE_MASK_DEFAULT_NAME = "node_mask";
static const char *NODE_LEAF_DEFAULT_NAME = "node_leaf";

static Tab<String> irq_types{String("key"), String("relPos"), String("keyFloat")};
enum IrqType
{
  IRQ_TYPE_UNSET = -1,
  IRQ_TYPE_KEY,
  IRQ_TYPE_REL_POS,
  IRQ_TYPE_KEY_FLOAT,
  IRQ_TYPE_SIZE
};

bool is_include_node_mask(const NodeMaskData &data) { return data.type == NodeMaskType::INCLUDE; };

struct ExeptionField
{
  const char *name;
  int len;
};

#define STR_WITH_LEN(x) {x, sizeof(x) - 1}
#define STR_LINE(x)     {x, -1}

dag::Vector<ExeptionField> exeption_fields = {STR_WITH_LEN("@clone-last:"), STR_WITH_LEN("@override:"), STR_WITH_LEN("@delete:"),
  STR_WITH_LEN("@delete-all:"), STR_LINE("@override-last"), STR_LINE("@delete-last")};

AnimTreePlugin::AnimTreePlugin() :
  animPlayer{get_app().getAssetMgr().getAssetTypeId("dynModel"), get_app().getAssetMgr()},
  ctrlTreeEventHandler(controllersData, includePaths, childsDialog),
  ctrlListSettingsEventHandler(controllersData, blendNodesData),
  childsDialog(controllersData, blendNodesData, statesData, includePaths),
  blendNodeTreeEventHandler(blendNodesData, includePaths),
  nodeMaskTreeEventHandler(nodeMasksData, includePaths),
  animStatesTreeEventHandler(statesData, controllersData, includePaths, childsDialog),
  stateListSettingsEventHandler(statesData, controllersData, blendNodesData, includePaths)
{}

bool AnimTreePlugin::begin(DagorAsset *asset)
{
  curAsset = asset;
  animPlayer.init(*curAsset);
  blendNodeTreeEventHandler.setAsset(curAsset);
  ctrlTreeEventHandler.setAsset(curAsset);
  nodeMaskTreeEventHandler.setAsset(curAsset);
  animStatesTreeEventHandler.setAsset(curAsset);

  return true;
}

bool AnimTreePlugin::end()
{
  animPlayer.clear();
  curAsset = nullptr;
  blendNodeTreeEventHandler.setAsset(nullptr);
  ctrlTreeEventHandler.setAsset(nullptr);
  nodeMaskTreeEventHandler.setAsset(nullptr);
  animStatesTreeEventHandler.setAsset(nullptr);

  clearData();
  return true;
}

bool AnimTreePlugin::getSelectionBox(BBox3 &box) const { return animPlayer.getSelectionBox(box); }

static void update_play_animation_text(PropPanel::ContainerPropertyControl *panel, bool is_anim_in_progress)
{
  if (!panel)
    return;

  if (is_anim_in_progress)
    panel->setCaption(PID_NODES_SETTINGS_PLAY_ANIMATION, "Stop playing animation");
  else
    panel->setCaption(PID_NODES_SETTINGS_PLAY_ANIMATION, "Play animation");
}

void AnimTreePlugin::actObjects(float dt)
{
  const bool oldState = animPlayer.isAnimInProgress();
  animPlayer.act(dt);
  const bool curState = animPlayer.isAnimInProgress();
  if (oldState != curState)
    update_play_animation_text(getPluginPanel(), curState);
}

bool AnimTreePlugin::supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "animTree") == 0; }

void AnimTreePlugin::fillPropPanel(PropPanel::ContainerPropertyControl &panel)
{
  const String modelName = animPlayer.getModelName();
  panel.setEventHandler(this);
  panel.createStatic(PID_SELECT_DYN_MODEL_STATIC, "Selected dynModel");
  panel.createButton(PID_SELECT_DYN_MODEL, modelName.empty() ? "Select model" : modelName.c_str());
  panel.createButton(PID_VALIDATE_DEPENDENT_NODES, "Validate dependent nodes");
  clearData();
  fillTreePanels(&panel);
}

bool AnimTreePlugin::isEnumOrEnumItem(TLeafHandle leaf, PropPanel::ContainerPropertyControl *tree)
{
  TLeafHandle parentLeaf = tree->getParentLeaf(leaf);
  TLeafHandle rootLeaf = tree->getParentLeaf(parentLeaf);
  return (rootLeaf && rootLeaf == enumsRootLeaf) || (parentLeaf && parentLeaf == enumsRootLeaf);
}

static TLeafHandle is_name_exists_in_childs(TLeafHandle parent, PropPanel::ContainerPropertyControl *tree, const char *new_name)
{
  const int childCount = tree->getChildCount(parent);
  for (int i = 0; i < childCount; ++i)
  {
    TLeafHandle child = tree->getChildLeaf(parent, i);
    if (tree->getCaption(child) == new_name)
      return child;
  }
  return nullptr;
}

static bool create_new_leaf(TLeafHandle parent, PropPanel::ContainerPropertyControl *tree, PropPanel::ContainerPropertyControl *panel,
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
    logerr("Can't create item, name <%s> already registered", sample_name);

  return !isNameExists;
}

static TLeafHandle get_anim_blend_node_leaf(PropPanel::ContainerPropertyControl *tree)
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
    else if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_INCLUDE)
      return nullptr;
    else if (leaf != tree->getRootLeaf())
      logerr("Can't find anim blend node for this selected leaf");
  }

  return nullptr;
}

static TLeafHandle get_anim_blend_node_include(PropPanel::ContainerPropertyControl *tree, const bool force_parent = false)
{
  TLeafHandle selLeaf = tree->getSelLeaf();
  if (!selLeaf)
    return tree->getRootLeaf();
  else if (tree->getUserData(selLeaf) == &ANIM_BLEND_NODE_INCLUDE && force_parent)
    return tree->getParentLeaf(selLeaf);
  else if (selLeaf == tree->getRootLeaf() || tree->getUserData(selLeaf) == &ANIM_BLEND_NODE_INCLUDE)
    return selLeaf;

  TLeafHandle nodeLeaf = get_anim_blend_node_leaf(tree);
  if (nodeLeaf)
  {
    TLeafHandle parent = tree->getParentLeaf(nodeLeaf);
    if (parent && tree->getUserData(parent) == &ANIM_BLEND_NODE_A2D)
      return tree->getParentLeaf(parent);
    else
      return tree->getParentLeaf(nodeLeaf);
  }

  return nullptr;
}

// leaf_name must be from anim blend node leaf, use get_anim_blend_node_leaf()
static DataBlock *get_selected_bnl_settings(DataBlock &props, const char *leaf_name, int &a2d_idx)
{
  for (int i = 0; i < props.blockCount(); ++i)
  {
    DataBlock *node = props.getBlock(i);
    if (!strcmp(node->getBlockName(), "AnimBlendNodeLeaf"))
    {
      for (int j = 0; j < node->blockCount(); ++j)
      {
        DataBlock *settings = node->getBlock(j);
        if (!strcmp(settings->getStr("name", ""), leaf_name))
        {
          a2d_idx = i;
          return settings;
        }
      }
    }
  }
  return nullptr;
}

void AnimTreePlugin::addController(TLeafHandle handle, CtrlType type)
{
  controllersData.emplace_back(AnimCtrlData{curCtrlAndBlendNodeUniqueIdx++, handle, type});
}
void AnimTreePlugin::addBlendNode(TLeafHandle handle, BlendNodeType type)
{
  blendNodesData.emplace_back(BlendNodeData{curCtrlAndBlendNodeUniqueIdx++, handle, type});
}

void AnimTreePlugin::clearData()
{
  enumsRootLeaf = nullptr;
  blendNodesData.clear();
  controllersData.clear();
  nodeMasksData.clear();
  statesData.clear();
  includePaths.clear();
  curCtrlAndBlendNodeUniqueIdx = 0;
  childsDialog.clear();
  childsDialog.hide();
}

void AnimTreePlugin::saveProps(DataBlock &props, const String &path)
{
  if (path.empty())
  {
    logerr("Can't save blk, path is empty");
    return;
  }
  const bool curParseCommentsAsParams = DataBlock::parseCommentsAsParams;
  // We need rise this flag, otherwise comments can't save
  DataBlock::parseCommentsAsParams = true;
  props.saveToTextFile(path);
  DataBlock::parseCommentsAsParams = curParseCommentsAsParams;
}

TLeafHandle AnimTreePlugin::getEnumsRootLeaf(PropPanel::ContainerPropertyControl *tree)
{
  if (!enumsRootLeaf)
  {
    enumsRootLeaf = tree->createTreeLeaf(tree->getRootLeaf(), "enum", ENUMS_ROOT_ICON);
    AnimStatesData *initAnimStateData = find_data_by_type(statesData, AnimStatesType::INIT_ANIM_STATE);
    if (!initAnimStateData)
    {
      TLeafHandle initAnimStateLeaf = tree->createTreeLeaf(tree->getRootLeaf(), "initAnimState", STATE_ICON);
      initAnimStateData = &statesData.emplace_back(AnimStatesData{initAnimStateLeaf, AnimStatesType::INIT_ANIM_STATE, String("Root")});
    }
    statesData.emplace_back(AnimStatesData{enumsRootLeaf, AnimStatesType::ENUM_ROOT, initAnimStateData->fileName});
  }

  return enumsRootLeaf;
}

static void create_irq_interruption_moment_type_combo(PropPanel::ContainerPropertyControl *panel, const char *type_name = "",
  IrqType selected_type = IRQ_TYPE_UNSET)
{
  if (selected_type == IRQ_TYPE_UNSET)
    selected_type = static_cast<IrqType>(lup(type_name, irq_types, IRQ_TYPE_KEY));
  G_ASSERT(irq_types.size() == IRQ_TYPE_SIZE);
  G_ASSERTF(selected_type < IRQ_TYPE_SIZE, "Wrong selected idx for interruption type");
  panel->createCombo(PID_IRQ_TYPE_COMBO_SELECT, "Interruption moment type", irq_types, selected_type);
}

static void fill_irq_settings(PropPanel::ContainerPropertyControl *panel, const char *irq_name, IrqType selected_type,
  const char *key = "", float key_float = 0.0f, bool save_enabled = true)
{
  remove_nodes_fields(panel);
  create_irq_interruption_moment_type_combo(panel, "", selected_type);
  SimpleString typeName = panel->getText(PID_IRQ_TYPE_COMBO_SELECT);
  if (selected_type == IRQ_TYPE_KEY)
    panel->createEditBox(PID_IRQ_INTERRUPTION_FIELD, typeName.c_str(), key);
  else
    panel->createEditFloat(PID_IRQ_INTERRUPTION_FIELD, typeName.c_str(), key_float, /*prec*/ 3);
  panel->createEditBox(PID_IRQ_NAME_FIELD, "name", irq_name);
  panel->createButton(PID_NODES_SETTINGS_SAVE, "Save", save_enabled);
}

static int get_irq_idx(PropPanel::ContainerPropertyControl *tree, TLeafHandle leaf)
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

template <typename Data>
static void remove_childs_data_by_leaf(PropPanel::ContainerPropertyControl *tree, dag::Vector<Data> &data_vec, TLeafHandle leaf)
{
  const int leafCount = tree->getChildCount(leaf);
  for (int i = 0; i < leafCount; ++i)
  {
    TLeafHandle child = tree->getChildLeaf(leaf, i);
    Data *removeItem = find_data_by_handle(data_vec, child);
    if (removeItem != data_vec.end())
    {
      TLeafHandle handle = removeItem->handle;
      data_vec.erase(removeItem);

      if (tree->getChildCount(handle) > 0)
        remove_childs_data_by_leaf(tree, data_vec, handle);
    }
  }
}

void AnimTreePlugin::addLeafToBlendNodesTree(PropPanel::ContainerPropertyControl *tree, DataBlock &props)
{
  int a2dIdx = -1;
  TLeafHandle leaf = get_anim_blend_node_leaf(tree);
  DataBlock *settings = get_selected_bnl_settings(props, tree->getCaption(leaf), a2dIdx);
  if (!settings)
    return;

  DataBlock *a2dBlk = props.getBlock(a2dIdx);
  const char *a2dName = a2dBlk->getStr("a2d", nullptr);
  if (!a2dName)
  {
    logerr("a2d field not exist or empty, can't create new leaf");
    return;
  }
  TLeafHandle parent = tree->getParentLeaf(leaf);
  DataBlock *newNodeSettings = a2dBlk->addNewBlock("single");
  newNodeSettings->setStr("name", "new_node");
  if (tree->getRootLeaf() == parent || tree->getUserData(parent) == &ANIM_BLEND_NODE_INCLUDE)
  {
    const String oldName = tree->getCaption(leaf);
    // Remove irq from leaf and craete on one level deep
    remove_childs_data_by_leaf(tree, blendNodesData, leaf);
    const int irqCount = tree->getChildCount(leaf);
    for (int i = 0; i < irqCount; ++i)
      tree->removeLeaf(tree->getChildLeaf(leaf, 0));

    BlendNodeType nodeType = static_cast<BlendNodeType>(lup(settings->getBlockName(), blend_node_types));
    TLeafHandle nodeLeaf = tree->createTreeLeaf(leaf, oldName.c_str(), get_blend_node_icon_name(nodeType));
    addBlendNode(nodeLeaf, nodeType);
    tree->setUserData(nodeLeaf, (void *)&ANIM_BLEND_NODE_LEAF);
    for (int i = 0; i < settings->blockCount(); ++i)
    {
      TLeafHandle irqLeaf = tree->createTreeLeaf(nodeLeaf, settings->getBlock(i)->getStr("name"), IRQ_ICON);
      addBlendNode(irqLeaf, BlendNodeType::IRQ);
      tree->setUserData(irqLeaf, (void *)&ANIM_BLEND_NODE_IRQ);
    }
    TLeafHandle newLeaf = tree->createTreeLeaf(leaf, "new_node", BLEND_NODE_SINGLE_ICON);
    addBlendNode(newLeaf, BlendNodeType::SINGLE);
    tree->setUserData(newLeaf, (void *)&ANIM_BLEND_NODE_LEAF);

    BlendNodeData *leafData = find_data_by_handle(blendNodesData, leaf);
    if (leafData != blendNodesData.end())
      leafData->type = BlendNodeType::A2D;
    tree->setCaption(leaf, a2dName);
    tree->setButtonPictures(leaf, A2D_NAME_ICON);
    tree->setUserData(leaf, (void *)&ANIM_BLEND_NODE_A2D);

    tree->setSelLeaf(newLeaf);
  }
  else
  {
    TLeafHandle newLeaf = tree->createTreeLeaf(parent, "new_node", BLEND_NODE_SINGLE_ICON);
    addBlendNode(newLeaf, BlendNodeType::SINGLE);
    tree->setUserData(newLeaf, (void *)&ANIM_BLEND_NODE_LEAF);
    tree->setSelLeaf(newLeaf);
  }
}

void AnimTreePlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case PID_SELECT_DYN_MODEL: selectDynModel(panel); break;
    case PID_VALIDATE_DEPENDENT_NODES: validateDependentNodes(panel); break;
    case PID_NODES_SETTINGS_PLAY_ANIMATION:
      animPlayer.changePlayingAnimation();
      update_play_animation_text(panel, animPlayer.isAnimInProgress());
      break;
    case PID_ANIM_STATES_ADD_ENUM: addEnumToAnimStatesTree(panel); break;
    case PID_ANIM_STATES_ADD_ITEM: addItemToAnimStatesTree(panel); break;
    case PID_ANIM_STATES_REMOVE_NODE: removeNodeFromAnimStatesTree(panel); break;
    case PID_STATES_SETTINGS_SAVE: saveSettingsAnimStatesTree(panel); break;
    case PID_STATES_NODES_LIST_ADD: addNodeStateList(panel); break;
    case PID_STATES_NODES_LIST_REMOVE: removeNodeStateList(panel); break;
    case PID_NODE_MASKS_ADD_MASK: addMaskToNodeMasksTree(panel); break;
    case PID_NODE_MASKS_ADD_NODE: addNodeToNodeMasksTree(panel); break;
    case PID_NODE_MASKS_REMOVE_NODE: removeNodeFromNodeMasksTree(panel); break;
    case PID_NODE_MASKS_SAVE: saveSettingsNodeMasksTree(panel); break;
    case PID_ANIM_BLEND_NODES_ADD_IRQ: addIrqNodeToAnimNodesTree(panel); break;
    case PID_ANIM_BLEND_NODES_CREATE_NODE: createNodeAnimNodesTree(panel); break;
    case PID_ANIM_BLEND_NODES_ADD_LEAF: createLeafAnimNodesTree(panel); break;
    case PID_ANIM_BLEND_NODES_REMOVE_NODE: removeNodeFromAnimNodesTree(panel); break;
    case PID_NODES_SETTINGS_SAVE: saveSettingsAnimNodesTree(panel); break;
    case PID_CTRLS_NODES_LIST_ADD: addNodeCtrlList(panel); break;
    case PID_CTRLS_NODES_LIST_REMOVE: removeNodeCtrlList(panel); break;
    case PID_CTRLS_SETTINGS_SAVE: saveControllerSettings(panel); break;
    case PID_CTRLS_TARGET_NODE_ADD: add_target_node(ctrlParams, panel); break;
    case PID_CTRLS_TARGET_NODE_REMOVE: remove_target_node(ctrlParams, panel); break;
    case PID_ANIM_BLEND_CTRLS_ADD_NODE: addNodeToCtrlTree(panel); break;
    case PID_ANIM_BLEND_CTRLS_ADD_INCLUDE:
    case PID_ANIM_BLEND_CTRLS_ADD_INNER_INCLUDE: addIncludeToCtrlTree(panel, pcb_id); break;
    case PID_ANIM_BLEND_CTRLS_REMOVE_NODE: removeNodeFromCtrlsTree(panel); break;
    case PID_ANIM_STATES_ADD_CHAN: addStateDescItem(panel, AnimStatesType::CHAN, "new_channel"); break;
    case PID_ANIM_STATES_ADD_STATE: addStateDescItem(panel, AnimStatesType::STATE, "new_state"); break;
    case PID_ANIM_STATES_ADD_STATE_ALIAS: addStateDescItem(panel, AnimStatesType::STATE_ALIAS, "new_state_alias"); break;
  }
}

static bool fill_comment_field(PropPanel::ContainerPropertyControl *panel, const DataBlock *node, int param_idx, int field_idx)
{
  bool isFieldCreated = true;
  if (IS_COMMENT_POST(node->getParamName(param_idx)))
    isFieldCreated = false;
  else if (IS_COMMENT_PRE(node->getParamName(param_idx)))
  {
    bool isFirstComment = false;
    int commentCount = 0;
    for (int i = 0; i < node->paramCount(); ++i)
    {
      if (CHECK_COMMENT_PREFIX(node->getParamName(i)) && IS_COMMENT_PRE(node->getParamName(i)))
      {
        ++commentCount;
        if (commentCount == 1 && i == param_idx)
          isFirstComment = true;
        else if (!isFirstComment)
          break;
      }
    }
    if (isFirstComment)
    {
      if (commentCount == 1)
        panel->createStatic(field_idx, String(0, "Comment: %s", node->getStr(param_idx)));
      else
        panel->createStatic(field_idx,
          String(0, "Comment: %s\nsee %i more comments in blk", node->getStr(param_idx), commentCount - 1));
    }
  }

  return isFieldCreated;
}

static bool fill_field_by_type(PropPanel::ContainerPropertyControl *panel, const DataBlock *node, int param_idx, int field_idx)
{
  bool isFieldCreated = true;
  if (node->getParamType(param_idx) == DataBlock::TYPE_STRING)
  {
    if (CHECK_COMMENT_PREFIX(node->getParamName(param_idx)))
      isFieldCreated = fill_comment_field(panel, node, param_idx, field_idx);
    else
      panel->createEditBox(field_idx, node->getParamName(param_idx), node->getStr(param_idx));
  }
  else if (node->getParamType(param_idx) == DataBlock::TYPE_REAL)
    panel->createEditFloat(field_idx, node->getParamName(param_idx), node->getReal(param_idx), /*prec*/ 2);
  else if (node->getParamType(param_idx) == DataBlock::TYPE_BOOL)
    panel->createCheckBox(field_idx, node->getParamName(param_idx), node->getBool(param_idx));
  else if (node->getParamType(param_idx) == DataBlock::TYPE_POINT2)
    panel->createPoint2(field_idx, node->getParamName(param_idx), node->getPoint2(param_idx));
  else if (node->getParamType(param_idx) == DataBlock::TYPE_POINT3)
    panel->createPoint3(field_idx, node->getParamName(param_idx), node->getPoint3(param_idx));
  else if (node->getParamType(param_idx) == DataBlock::TYPE_INT)
    panel->createEditInt(field_idx, node->getParamName(param_idx), node->getInt(param_idx));
  else
  {
    logerr("skip param type: %d", node->getParamType(param_idx));
    isFieldCreated = false;
  }

  const int nextIdx = param_idx + 1;
  if (isFieldCreated && nextIdx < node->paramCount())
  {
    const char *paramName = node->getParamName(nextIdx);
    if (CHECK_COMMENT_PREFIX(paramName) && IS_COMMENT_POST(paramName))
      panel->setTooltipId(field_idx, node->getStr(nextIdx));
  }

  return isFieldCreated;
}

static void fill_values_if_exists(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  const dag::Vector<AnimParamData> &params)
{
  for (int i = 0; i < settings->paramCount(); ++i)
  {
    const AnimParamData *param = find_param_by_name(params, settings->getParamName(i));
    if (param != params.end())
    {
      switch (param->type)
      {
        case DataBlock::TYPE_STRING: panel->setText(param->pid, settings->getStr(i)); break;
        case DataBlock::TYPE_REAL: panel->setFloat(param->pid, settings->getReal(i)); break;
        case DataBlock::TYPE_BOOL: panel->setBool(param->pid, settings->getBool(i)); break;
        case DataBlock::TYPE_POINT2: panel->setPoint2(param->pid, settings->getPoint2(i)); break;
        case DataBlock::TYPE_POINT3: panel->setPoint3(param->pid, settings->getPoint3(i)); break;
        case DataBlock::TYPE_INT: panel->setInt(param->pid, settings->getInt(i)); break;

        default: logerr("skip param type: %d", settings->getParamType(i)); break;
      }
    }
  }
}

void AnimTreePlugin::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case PID_ANIM_BLEND_NODES_TREE: selectedChangedAnimNodesTree(panel); break;
    case PID_NODE_MASKS_TREE: selectedChangedNodeMasksTree(panel); break;
    case PID_ANIM_STATES_TREE: selectedChangedAnimStatesTree(panel); break;
    case PID_IRQ_TYPE_COMBO_SELECT: irqTypeSelected(panel); break;
    case PID_ANIM_BLEND_CTRLS_TREE: fillCtrlsSettings(panel); break;
    case PID_STATES_NODES_LIST: setSelectedStateNodeListSettings(panel); break;
    case PID_CTRLS_NODES_LIST: setSelectedCtrlNodeListSettings(panel); break;
    case PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT: changeParamSwitchType(panel); break;
    case PID_NODE_MASKS_FILTER: setTreeFilter(panel, PID_NODE_MASKS_TREE, pcb_id); break;
    case PID_ANIM_BLEND_NODES_FILTER: setTreeFilter(panel, PID_ANIM_BLEND_NODES_TREE, pcb_id); break;
    case PID_ANIM_BLEND_CTRLS_FILTER: setTreeFilter(panel, PID_ANIM_BLEND_CTRLS_TREE, pcb_id); break;
    case PID_ANIM_STATES_FILTER: setTreeFilter(panel, PID_ANIM_STATES_TREE, pcb_id); break;
    case PID_NODES_SETTINGS_NODE_TYPE_COMBO_SELECT: changeAnimNodeType(panel); break;
    case PID_CTRLS_TYPE_COMBO_SELECT: changeCtrlType(panel); break;
    case PID_STATES_STATE_DESC_TYPE_COMBO_SELECT: changeStateDescType(panel); break;
    case PID_STATES_ENUM_PARAM_SWITCH_NAME: updateEnumItemValueCombo(panel); break;
  }

  if (pcb_id >= PID_NODES_PARAMS_FIELD && pcb_id <= PID_NODES_PARAMS_FIELD_SIZE)
    updateAnimNodeFields(panel, pcb_id);
}

void AnimTreePlugin::addBlendNodeToTree(PropPanel::ContainerPropertyControl *tree, const DataBlock *node, int idx, TLeafHandle parent)
{
  const DataBlock *animNode = node->getBlock(idx);
  BlendNodeType type = static_cast<BlendNodeType>(lup(animNode->getBlockName(), blend_node_types));
  const char *iconName = get_blend_node_icon_name(type);
  TLeafHandle animLeaf = tree->createTreeLeaf(parent, animNode->getStr("name"), iconName);
  addBlendNode(animLeaf, type);
  tree->setUserData(animLeaf, (void *)&ANIM_BLEND_NODE_LEAF);
  for (int i = 0; i < animNode->blockCount(); ++i)
  {
    TLeafHandle irqLeaf = tree->createTreeLeaf(animLeaf, animNode->getBlock(i)->getStr("name"), IRQ_ICON);
    addBlendNode(irqLeaf, BlendNodeType::IRQ);
    tree->setUserData(irqLeaf, (void *)&ANIM_BLEND_NODE_IRQ);
  }
}

static bool blk_has_override_fields(const DataBlock &blk)
{
  // check block name contain one of overrides
  const char *name = blk.getBlockName();
  if (name)
    for (const ExeptionField &field : exeption_fields)
      if ((field.len != -1 && strncmp(name, field.name, field.len) == 0) || strcmp(name, field.name) == 0)
        return true;

  for (int i = 0; i < blk.blockCount(); ++i)
    if (blk_has_override_fields(*blk.getBlock(i)))
      return true;

  return false;
}

static bool is_simple_one_file_blk(const String &file_path)
{
  const bool curParseIncludesAsParams = DataBlock::parseIncludesAsParams;
  const bool curParseOverridesNotApply = DataBlock::parseOverridesNotApply;
  DataBlock::parseIncludesAsParams = true;
  DataBlock::parseOverridesNotApply = true;

  DataBlock assetProps(file_path);

  DataBlock::parseIncludesAsParams = curParseIncludesAsParams;
  DataBlock::parseOverridesNotApply = curParseOverridesNotApply;

  return !blk_has_override_fields(assetProps);
}

static DataBlock get_props_from_include_leaf_blend_node(dag::ConstSpan<String> paths, const DagorAsset &asset,
  PropPanel::ContainerPropertyControl *tree, String &full_path, bool force_parent = false)
{
  TLeafHandle includeLeaf = get_anim_blend_node_include(tree, force_parent);
  return get_props_from_include_leaf(paths, asset, tree, includeLeaf, full_path);
}

static TLeafHandle get_node_mask_include(dag::ConstSpan<NodeMaskData> masks_data, PropPanel::ContainerPropertyControl *tree,
  bool force_parent = false)
{
  TLeafHandle selLeaf = tree->getSelLeaf();
  if (!selLeaf)
    return tree->getRootLeaf();
  if (selLeaf == tree->getRootLeaf())
    return selLeaf;

  String selName = tree->getCaption(selLeaf);
  const NodeMaskData *maskData =
    eastl::find_if(masks_data.begin(), masks_data.end(), [selLeaf](const NodeMaskData &data) { return selLeaf == data.handle; });

  if (maskData != masks_data.end())
  {
    if ((maskData->type == NodeMaskType::INCLUDE && force_parent) || maskData->type == NodeMaskType::MASK)
      return tree->getParentLeaf(selLeaf);
    else if (maskData->type == NodeMaskType::INCLUDE)
      return selLeaf;
    else if (maskData->type == NodeMaskType::LEAF)
    {
      TLeafHandle parent = tree->getParentLeaf(selLeaf);
      return tree->getParentLeaf(parent);
    }
  }
  return nullptr;
}

struct ParentLeafs
{
  TLeafHandle nodes;
  TLeafHandle masks;
  TLeafHandle ctrls;
};

static bool check_includes_not_looped(PropPanel::ContainerPropertyControl *tree, TLeafHandle parent, const char *name,
  const char *asset_name)
{
  if (!strcmp(name, asset_name))
    return false;

  TLeafHandle root = tree->getRootLeaf();
  while (parent && parent != root)
  {
    if (tree->getCaption(parent) == name)
      return false;

    parent = tree->getParentLeaf(parent);
  }

  return true;
}

static bool check_include_duplication_in_blk(const DataBlock &props, const String &path)
{
  const int includeNid = props.getNameId("@include");
  for (int i = 0; i < props.paramCount(); ++i)
    if (includeNid == props.getParamNameId(i) && props.getStr(i) == path)
      return true;

  if (props.blockExists("AnimBlendCtrl"))
    return check_include_duplication_in_blk(*props.getBlockByName("AnimBlendCtrl"), path);

  return false;
}

void AnimTreePlugin::fillCtrlsIncluded(PropPanel::ContainerPropertyControl *tree, const DataBlock &props, const String &source_path,
  TLeafHandle parent)
{
  const int includeNid = props.getNameId("@include");
  for (int i = 0; i < props.paramCount(); ++i)
    if (includeNid == props.getParamNameId(i))
    {
      const char *rawPath = props.getStr(i);
      const int lenWithoutDotBlk = strlen(rawPath) - 4;
      if (lenWithoutDotBlk < 0 || strncmp(rawPath + lenWithoutDotBlk, ".blk", 4) != 0)
      {
        logerr("Skip path: <%s> Path not end by \".blk\"", rawPath);
        continue;
      }
      char fileNameBuf[DAGOR_MAX_PATH];
      const char *name = dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), rawPath);
      if (!name)
      {
        logerr("File name is empty, skip path: <%s>", rawPath);
        continue;
      }
      if (!check_includes_not_looped(tree, parent, name, curAsset->getSrcFileName()))
      {
        logerr("Find loop in includes, leaf <%s> with parent <%s> already created above", name, tree->getCaption(parent));
        continue;
      }

      TLeafHandle includeLeaf = tree->createTreeLeaf(parent, name, INNER_INCLUDE_ICON);
      addController(includeLeaf, ctrl_type_inner_include);
      includePaths.emplace_back(0, rawPath);
      String path(0, rawPath);
      if (!DataBlock::resolveIncludePath(path))
      {
        char fileLocationBuf[DAGOR_MAX_PATH];
        path = String(0, "%s%s", dd_get_fname_location(fileLocationBuf, source_path.c_str()), rawPath);
      }
      dd_simplify_fname_c(path.c_str());
      if (!is_simple_one_file_blk(path))
        DAEDITOR3.conNote("Overrides found in blk with path: <%s>\n"
                          "Blocks or params with errors below are read-only and should be edited manually in a text editor.",
          path);

      DataBlock includeBlk(path);
      fillCtrlsIncluded(tree, includeBlk, path, includeLeaf);
    }

  for (int i = 0; i < props.blockCount(); ++i)
  {
    const DataBlock *settings = props.getBlock(i);
    const char *typeName = settings->getBlockName();
    const char *nameField = settings->getStr("name", nullptr);
    CtrlType type = static_cast<CtrlType>(lup(typeName, ctrl_type, ctrl_type_not_found));
    if (type == ctrl_type_not_found)
      logerr("Type: <%s> not found in controllers types", typeName);
    if (nameField)
    {
      const char *iconName = get_ctrl_icon_name(type);
      TLeafHandle leaf = tree->createTreeLeaf(parent, nameField, iconName);
      addController(leaf, type);
    }
  }
}

static TLeafHandle check_multiple_block_declarations(AnimStatesData &data, const char *include_name, const char *block_name,
  bool is_empty_block)
{
  if (data.fileName == "Root" && is_empty_block)
  {
    data.fileName = include_name;
    return data.handle;
  }
  else
    logerr("Find multiple %s blocks! Already displayed from: <%s>. "
           "Skip from include name: <%s>. For correct work resoulve it manulay",
      block_name, data.fileName, include_name);

  return nullptr;
}

void AnimTreePlugin::fillAnimStatesTreeNode(PropPanel::ContainerPropertyControl *panel, const DataBlock &node,
  const char *include_name)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle root = tree->getRootLeaf();
  const char *blockName = node.getBlockName();
  if (!strcmp(blockName, "stateDesc"))
  {
    TLeafHandle nodeLeaf;
    if (AnimStatesData *data = find_data_by_type(statesData, AnimStatesType::STATE_DESC))
    {
      nodeLeaf = check_multiple_block_declarations(*data, include_name, blockName,
        statesData.end() == eastl::find_if(statesData.begin(), statesData.end(), [](const AnimStatesData &data) {
          return data.type == AnimStatesType::STATE || data.type == AnimStatesType::CHAN || data.type == AnimStatesType::STATE_ALIAS;
        }));

      if (!nodeLeaf)
        return;
    }
    else
    {
      nodeLeaf = tree->createTreeLeaf(root, blockName, STATE_ICON);
      statesData.emplace_back(AnimStatesData{nodeLeaf, AnimStatesType::STATE_DESC, String(include_name)});
    }
    int chanNid = node.getNameId("chan");
    int stateNid = node.getNameId("state");
    int stateAliasNid = node.getNameId("state_alias");
    for (int i = 0; i < node.blockCount(); ++i)
    {
      const DataBlock *stateNode = node.getBlock(i);
      auto add_leaf_and_data = [nodeLeaf, stateNode, tree, &statesData = statesData](AnimStatesType type) {
        TLeafHandle leaf = tree->createTreeLeaf(nodeLeaf, stateNode->getStr("name"), get_state_desc_icon(type));
        statesData.emplace_back(AnimStatesData{leaf, type});
      };
      if (stateNode->getBlockNameId() == chanNid)
        add_leaf_and_data(AnimStatesType::CHAN);
      else if (stateNode->getBlockNameId() == stateNid)
        add_leaf_and_data(AnimStatesType::STATE);
      else if (stateNode->getBlockNameId() == stateAliasNid)
        add_leaf_and_data(AnimStatesType::STATE_ALIAS);
    }
  }
  else if (!strcmp(blockName, "initAnimState"))
  {
    TLeafHandle nodeLeaf;
    if (AnimStatesData *data = find_data_by_type(statesData, AnimStatesType::INIT_ANIM_STATE))
    {
      nodeLeaf = check_multiple_block_declarations(*data, include_name, blockName,
        statesData.end() == eastl::find_if(statesData.begin(), statesData.end(), [](const AnimStatesData &data) {
          return data.type == AnimStatesType::POST_BLEND_CTRL_ORDER || data.type == AnimStatesType::INIT_FIFO3;
        }));

      if (!nodeLeaf)
        return;
    }
    else
    {
      nodeLeaf = tree->createTreeLeaf(root, node.getBlockName(), STATE_ICON);
      statesData.emplace_back(AnimStatesData{nodeLeaf, AnimStatesType::INIT_ANIM_STATE, String(include_name)});
    }
    for (int j = 0; j < node.blockCount(); ++j)
    {
      const DataBlock *settings = node.getBlock(j);
      const char *settingsBlockName = settings->getBlockName();
      if (!strcmp(settingsBlockName, "enum"))
        fillEnumTree(settings, tree, root, include_name);
      else if (!strcmp(settingsBlockName, "postBlendCtrlOrder"))
      {
        TLeafHandle leaf = tree->createTreeLeaf(nodeLeaf, settingsBlockName, STATE_LEAF_ICON);
        statesData.emplace_back(AnimStatesData{leaf, AnimStatesType::POST_BLEND_CTRL_ORDER});
      }
      else
      {
        TLeafHandle leaf = tree->createTreeLeaf(nodeLeaf, settingsBlockName, STATE_LEAF_ICON);
        statesData.emplace_back(AnimStatesData{leaf, AnimStatesType::INIT_FIFO3});
      }
    }
  }
  else if (!strcmp(blockName, "preview"))
  {
    TLeafHandle leaf = tree->createTreeLeaf(root, blockName, STATE_ICON);
    statesData.emplace_back(AnimStatesData{leaf, AnimStatesType::PREVIEW});
  }
}

void AnimTreePlugin::fillTreesIncluded(PropPanel::ContainerPropertyControl *panel, const DataBlock &props, const String &source_path,
  ParentLeafs parents)
{
  PropPanel::ContainerPropertyControl *nodesTree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *masksTree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *statesTree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();

  const int includeNid = props.getNameId("@include");
  const int rootNid = props.getNameId("root");
  const int exportNid = props.getNameId("export");
  const int defaultForeignAnimNid = props.getNameId("defaultForeignAnim");
  for (int i = 0; i < props.paramCount(); ++i)
  {
    const int paramNid = props.getParamNameId(i);
    if (includeNid == paramNid)
    {
      const char *rawPath = props.getStr(i);
      const int lenWithoutDotBlk = strlen(rawPath) - 4;
      if (lenWithoutDotBlk < 0 || strncmp(rawPath + lenWithoutDotBlk, ".blk", 4) != 0)
      {
        logerr("Skip path: <%s> Path not end by \".blk\"", rawPath);
        continue;
      }
      char fileNameBuf[DAGOR_MAX_PATH];
      const char *name = dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), rawPath);
      if (!name)
      {
        logerr("File name is empty, skip path: <%s>", rawPath);
        continue;
      }
      if (!check_includes_not_looped(nodesTree, parents.nodes, name, curAsset->getSrcFileName()))
      {
        logerr("Find loop in includes, leaf <%s> with parent <%s> already created above", name, nodesTree->getCaption(parents.nodes));
        continue;
      }

      ParentLeafs includeParents;
      includeParents.nodes = nodesTree->createTreeLeaf(parents.nodes, name, FOLDER_ICON);
      nodesTree->setUserData(includeParents.nodes, (void *)&ANIM_BLEND_NODE_INCLUDE);
      includeParents.masks = masksTree->createTreeLeaf(parents.masks, name, FOLDER_ICON);
      includeParents.ctrls = ctrlsTree->createTreeLeaf(parents.ctrls, name, FOLDER_ICON);
      includePaths.emplace_back(0, rawPath);
      addBlendNode(includeParents.nodes, BlendNodeType::INCLUDE);
      nodeMasksData.emplace_back(NodeMaskData{includeParents.masks, NodeMaskType::INCLUDE});
      addController(includeParents.ctrls, ctrl_type_include);
      String path(0, rawPath);
      if (!DataBlock::resolveIncludePath(path))
      {
        char fileLocationBuf[DAGOR_MAX_PATH];
        path = String(0, "%s%s", dd_get_fname_location(fileLocationBuf, source_path.c_str()), rawPath);
      }
      dd_simplify_fname_c(path.c_str());
      if (!is_simple_one_file_blk(path))
        DAEDITOR3.conNote("Overrides found in blk with path: <%s>\n"
                          "Blocks or params with errors below are read-only and should be edited manually in a text editor.",
          path);

      DataBlock includeBlk(path);
      fillTreesIncluded(panel, includeBlk, path, includeParents);
    }
    else if (rootNid == paramNid || exportNid == paramNid || defaultForeignAnimNid == paramNid)
    {
      AnimStatesData *rootPropsData = find_data_by_type(statesData, AnimStatesType::ROOT_PROPS);
      if (!rootPropsData)
      {
        TLeafHandle root = statesTree->getRootLeaf();
        TLeafHandle rootProps = statesTree->createTreeLeaf(root, "Root props", STATE_ICON);
        statesData.emplace_back(AnimStatesData{rootProps, AnimStatesType::ROOT_PROPS, nodesTree->getCaption(parents.nodes)});
      }
      else if (rootPropsData->fileName != nodesTree->getCaption(parents.nodes))
      {
        logerr("Some root props already registred, expect include: <%s>, but get <%s>", rootPropsData->fileName,
          nodesTree->getCaption(parents.nodes));
      }
    }
  }

  for (int i = 0; i < props.blockCount(); ++i)
  {
    const DataBlock *node = props.getBlock(i);
    if (!strcmp(node->getBlockName(), "AnimBlendNodeLeaf"))
    {
      if (node->blockCount() == 1)
        addBlendNodeToTree(nodesTree, node, /*idx*/ 0, parents.nodes);
      else
      {
        TLeafHandle nodeLeaf = nodesTree->createTreeLeaf(parents.nodes, node->getStr("a2d"), A2D_NAME_ICON);
        nodesTree->setUserData(nodeLeaf, (void *)&ANIM_BLEND_NODE_A2D);
        addBlendNode(nodeLeaf, BlendNodeType::A2D);
        for (int j = 0; j < node->blockCount(); ++j)
          addBlendNodeToTree(nodesTree, node, j, nodeLeaf);
      }
    }
    else if (!strcmp(node->getBlockName(), "nodeMask"))
    {
      TLeafHandle nodeLeaf = masksTree->createTreeLeaf(parents.masks, node->getStr("name"), NODE_MASK_ICON);
      nodeMasksData.emplace_back(NodeMaskData{nodeLeaf, NodeMaskType::MASK});
      for (int j = 0; j < node->paramCount(); ++j)
        if (!strcmp(node->getParamName(j), "node"))
        {
          TLeafHandle leaf = masksTree->createTreeLeaf(nodeLeaf, node->getStr(j), NODE_MASK_LEAF_ICON);
          nodeMasksData.emplace_back(NodeMaskData{leaf, NodeMaskType::LEAF});
        }
    }
    else if (!strcmp(node->getBlockName(), "AnimBlendCtrl"))
      fillCtrlsIncluded(ctrlsTree, *node, source_path, parents.ctrls);
    else if (!strcmp(node->getBlockName(), "stateDesc") || !strcmp(node->getBlockName(), "initAnimState") ||
             !strcmp(node->getBlockName(), "preview"))
      fillAnimStatesTreeNode(panel, *node, nodesTree->getCaption(parents.nodes)); // for include name we can use any parent leaf
  }
}

void AnimTreePlugin::fillTreePanels(PropPanel::ContainerPropertyControl *panel)
{
  G_ASSERT(ctrl_type.size() == ctrl_type_size);

  auto *masksGroup = panel->createGroup(PID_NODE_MASKS_GROUP, "Node masks");
  masksGroup->createEditBox(PID_NODE_MASKS_FILTER, "Filter");
  auto *masksTree = masksGroup->createTree(PID_NODE_MASKS_TREE, "Skeleton node masks", /*height*/ _pxScaled(300));
  masksTree->setTreeEventHandler(&nodeMaskTreeEventHandler);
  masksGroup->createButton(PID_NODE_MASKS_ADD_MASK, "Add mask");
  masksGroup->createButton(PID_NODE_MASKS_ADD_NODE, "Add include", /*enabled*/ true, /*new_line*/ false);
  masksGroup->createButton(PID_NODE_MASKS_REMOVE_NODE, "Remove selected");
  auto *masksSettingsGroup = masksGroup->createGroup(PID_NODE_MASKS_SETTINGS_GROUP, "Selected settings");
  masksSettingsGroup->createEditBox(PID_NODE_MASKS_NAME_FIELD, "Field name", "");
  masksSettingsGroup->createButton(PID_NODE_MASKS_SAVE, "Save");
  auto *nodesGroup = panel->createGroup(PID_ANIM_BLEND_NODES_GROUP, "Anim blend nodes");
  nodesGroup->createEditBox(PID_ANIM_BLEND_NODES_FILTER, "Filter");
  auto *nodesTree = nodesGroup->createTree(PID_ANIM_BLEND_NODES_TREE, "Anim blend nodes", /*height*/ _pxScaled(300));
  nodesTree->setTreeEventHandler(&blendNodeTreeEventHandler);
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_CREATE_NODE, "Create blend node");
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_ADD_LEAF, "Add include");
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_ADD_IRQ, "Add irq", /*enabled*/ false, /*new_line*/ false);
  nodesGroup->createButton(PID_ANIM_BLEND_NODES_REMOVE_NODE, "Remove selected");
  nodesGroup->createGroup(PID_ANIM_BLEND_NODES_SETTINGS_GROUP, "Selected settings");
  auto *crtlsGroup = panel->createGroup(PID_ANIM_BLEND_CTRLS_GROUP, "Anim blend controllers");
  crtlsGroup->createEditBox(PID_ANIM_BLEND_CTRLS_FILTER, "Filter");
  auto *ctrlsTree = crtlsGroup->createTree(PID_ANIM_BLEND_CTRLS_TREE, "Anim blend controllers", /*height*/ _pxScaled(300));
  ctrlsTree->setTreeEventHandler(&ctrlTreeEventHandler);
  crtlsGroup->createButton(PID_ANIM_BLEND_CTRLS_ADD_NODE, "Add node");
  crtlsGroup->createButton(PID_ANIM_BLEND_CTRLS_ADD_INCLUDE, "Add include");
  crtlsGroup->createButton(PID_ANIM_BLEND_CTRLS_ADD_INNER_INCLUDE, "Add inner include", /*enabled*/ true, /*new_line*/ false);
  crtlsGroup->createButton(PID_ANIM_BLEND_CTRLS_REMOVE_NODE, "Remove selected");
  crtlsGroup->createGroup(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP, "Selected settings");
  auto *statesGroup = panel->createGroup(PID_ANIM_STATES_GROUP, "Anim states");
  statesGroup->createEditBox(PID_ANIM_STATES_FILTER, "Filter");
  auto *statesTree = statesGroup->createTree(PID_ANIM_STATES_TREE, "Anim states", /*height*/ _pxScaled(300));
  statesTree->setTreeEventHandler(&animStatesTreeEventHandler);
  statesGroup->createButton(PID_ANIM_STATES_ADD_ENUM, "Add enum");
  statesGroup->createButton(PID_ANIM_STATES_ADD_ITEM, "Add item", /*enabled*/ false, /*new_line*/ false);
  statesGroup->createButton(PID_ANIM_STATES_REMOVE_NODE, "Remove selected");
  auto *statesSettingGroup = statesGroup->createGroup(PID_ANIM_STATES_SETTINGS_GROUP, "Selected settings");
  statesSettingGroup->createEditBox(PID_ANIM_STATES_NAME_FIELD, "Field name", "");
  statesSettingGroup->createButton(PID_STATES_SETTINGS_SAVE, "Save");
  TLeafHandle masksRoot = masksTree->createTreeLeaf(0, "Root", FOLDER_ICON);
  TLeafHandle nodesRoot = nodesTree->createTreeLeaf(0, "Root", FOLDER_ICON);
  TLeafHandle ctrlsRoot = ctrlsTree->createTreeLeaf(0, "Root", FOLDER_ICON);
  TLeafHandle statesRoot = statesTree->createTreeLeaf(0, "Root", FOLDER_ICON);
  statesData.emplace_back(AnimStatesData{statesRoot, AnimStatesType::INCLUDE_ROOT});
  const String assetPath = curAsset->getSrcFilePath();
  if (!is_simple_one_file_blk(assetPath))
    DAEDITOR3.conNote("Overrides found in blk with path: <%s>\n"
                      "Blocks or params with errors below are read-only and should be edited manually in a text editor.",
      assetPath);

  const bool curParseIncludesAsParams = DataBlock::parseIncludesAsParams;
  DataBlock::parseIncludesAsParams = true;
  DataBlock assetProps(assetPath);
  fillTreesIncluded(panel, assetProps, assetPath, {nodesRoot, masksRoot, ctrlsRoot});
  auto create_by_default_if_not_created = [&states = statesData, statesTree, statesRoot](AnimStatesType type, const char *name) {
    AnimStatesData *data =
      eastl::find_if(states.begin(), states.end(), [type](const AnimStatesData &data) { return data.type == type; });
    if (data == states.end())
    {
      TLeafHandle leaf = statesTree->createTreeLeaf(statesRoot, name, STATE_ICON);
      states.emplace_back(AnimStatesData{leaf, type, String("Root")});
    }
  };
  create_by_default_if_not_created(AnimStatesType::STATE_DESC, "stateDesc");
  create_by_default_if_not_created(AnimStatesType::ROOT_PROPS, "Root props");

  fillCtrlsChilds(panel);
  fillStatesChilds(panel);

  DataBlock::parseIncludesAsParams = curParseIncludesAsParams;

  childsDialog.setTreePanels(panel, curAsset, this);
  ctrlListSettingsEventHandler.setPluginEventHandler(panel, this);
  animStatesTreeEventHandler.setPluginEventHandler(panel, this);
  stateListSettingsEventHandler.setPluginEventHandler(panel, curAsset, this);
  masksTree->setBool(masksRoot, /*open*/ true);
  nodesTree->setBool(nodesRoot, /*open*/ true);
  ctrlsTree->setBool(ctrlsRoot, /*open*/ true);
  statesTree->setBool(statesRoot, /*open*/ true);
}

void AnimTreePlugin::fillEnumTree(const DataBlock *settings, PropPanel::ContainerPropertyControl *panel, TLeafHandle tree_root,
  const char *include_name)
{
  if (AnimStatesData *data = find_data_by_type(statesData, AnimStatesType::ENUM_ROOT))
  {
    TLeafHandle nodeLeaf = check_multiple_block_declarations(*data, include_name, "enum",
      statesData.end() == eastl::find_if(statesData.begin(), statesData.end(), [](const AnimStatesData &data) {
        return data.type == AnimStatesType::ENUM || data.type == AnimStatesType::ENUM_ITEM;
      }));

    if (nodeLeaf)
      enumsRootLeaf = nodeLeaf;
    else
      return;
  }
  else
  {
    enumsRootLeaf = panel->createTreeLeaf(tree_root, settings->getBlockName(), ENUMS_ROOT_ICON);
    statesData.emplace_back(AnimStatesData{enumsRootLeaf, AnimStatesType::ENUM_ROOT, String(include_name)});
  }
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *enumBlk = settings->getBlock(i);
    TLeafHandle enumLeaf = panel->createTreeLeaf(enumsRootLeaf, enumBlk->getBlockName(), ENUM_ICON);
    statesData.emplace_back(AnimStatesData{enumLeaf, AnimStatesType::ENUM});
    for (int j = 0; j < enumBlk->blockCount(); ++j)
    {
      TLeafHandle leaf = panel->createTreeLeaf(enumLeaf, enumBlk->getBlock(j)->getBlockName(), ENUM_ITEM_ICON);
      statesData.emplace_back(AnimStatesData{leaf, AnimStatesType::ENUM_ITEM});
    }
  }
}

static void fill_param_data(const DataBlock *settings, dag::Vector<AnimParamData> &params, int idx, int pid)
{
  params.emplace_back(
    AnimParamData{pid, static_cast<DataBlock::ParamType>(settings->getParamType(idx)), String(settings->getParamName(idx))});
}

static void fill_params_from_settings(const DataBlock *settings, PropPanel::ContainerPropertyControl *panel,
  dag::Vector<AnimParamData> &params, int &field_idx)
{
  const int includeNid = settings->getNameId("@include");
  for (int i = 0; i < settings->paramCount(); ++i)
  {
    if (includeNid != settings->getParamNameId(i)) // ignore include params
    {
      fill_field_by_type(panel, settings, i, field_idx);
      fill_param_data(settings, params, i, field_idx);
      ++field_idx;
    }
  }
}

// Add all fields if not exists based on BlendNodeType
static void fill_anim_blend_params_settings(const BlendNodeType type, PropPanel::ContainerPropertyControl *panel,
  dag::Vector<AnimParamData> &params, int field_idx, bool default_foreign)
{
  switch (type)
  {
    case BlendNodeType::SINGLE: single_init_panel(params, panel, field_idx, default_foreign); break;
    case BlendNodeType::CONTINUOUS: continuous_init_panel(params, panel, field_idx, default_foreign); break;
    case BlendNodeType::STILL: still_init_panel(params, panel, field_idx, default_foreign); break;
    case BlendNodeType::PARAMETRIC: parametric_init_panel(params, panel, field_idx, default_foreign); break;

    default: break;
  }
}

static void set_anim_blend_dependent_defaults(const BlendNodeType type, PropPanel::ContainerPropertyControl *panel,
  dag::Vector<AnimParamData> &params, const DagorAssetMgr &mgr)
{
  switch (type)
  {
    case BlendNodeType::SINGLE: single_set_dependent_defaults(params, panel, mgr); break;
    case BlendNodeType::CONTINUOUS: continuous_set_dependent_defaults(params, panel, mgr); break;
    case BlendNodeType::STILL: still_set_dependent_defaults(params, panel); break;
    case BlendNodeType::PARAMETRIC: parametric_set_dependent_defaults(params, panel); break;

    default: break;
  }
}

static void fill_include_settings(PropPanel::ContainerPropertyControl *tree, PropPanel::ContainerPropertyControl *group,
  TLeafHandle leaf, dag::ConstSpan<String> paths, dag::Vector<AnimParamData> &params, int filed_idx)
{
  const String selName = tree->getCaption(leaf);
  const String *path = eastl::find_if(paths.begin(), paths.end(), [&selName](const String &value) {
    char fileNameBuf[DAGOR_MAX_PATH];
    return selName == dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), value.c_str());
  });
  char fileLocationBuf[DAGOR_MAX_PATH];
  String pathWithoutName = (path != paths.end() && !path->empty()) ? String(dd_get_fname_location(fileLocationBuf, path->c_str()))
                                                                   : String("#/example/path/for/include/");
  add_edit_box_if_not_exists(params, group, filed_idx, "file name", selName.c_str());
  add_edit_box_if_not_exists(params, group, filed_idx, "folder path", pathWithoutName.c_str());
}

void AnimTreePlugin::fillAnimBlendSettings(PropPanel::ContainerPropertyControl *tree, PropPanel::ContainerPropertyControl *group)
{
  TLeafHandle leaf = get_anim_blend_node_leaf(tree);
  TLeafHandle selLeaf = tree->getSelLeaf();
  remove_nodes_fields(group);
  animBnlParams.clear();
  if (!selLeaf)
    return;

  if (tree->getUserData(selLeaf) == &ANIM_BLEND_NODE_INCLUDE)
  {
    animPlayer.resetSelectedA2dName();
    fill_include_settings(tree, group, selLeaf, includePaths, animBnlParams, PID_NODES_PARAMS_FIELD);
    group->createButton(PID_NODES_SETTINGS_SAVE, "Save");
  }

  if (!leaf)
    return;

  String fullPath;
  DataBlock props = get_props_from_include_leaf_blend_node(includePaths, *curAsset, tree, fullPath);
  if (fullPath.empty())
    return;

  TLeafHandle parent = tree->getParentLeaf(selLeaf);
  const bool isA2dNode = tree->getChildCount(selLeaf) > 0 && tree->getChildLeaf(selLeaf, 0) == leaf;
  const bool isIrqNode = parent && parent == leaf;
  bool isEditButtonEnabled = true;
  int a2dIdx = -1;
  DataBlock *settings = get_selected_bnl_settings(props, tree->getCaption(leaf), a2dIdx);
  DataBlock *a2dBlk = props.getBlock(a2dIdx);
  if (!settings)
  {
    const bool curParseIncludesAsParams = DataBlock::parseIncludesAsParams;
    DataBlock::parseIncludesAsParams = true;
    props.load(fullPath);
    DataBlock::parseIncludesAsParams = curParseIncludesAsParams;
    settings = get_selected_bnl_settings(props, tree->getCaption(leaf), a2dIdx);
    a2dBlk = props.getBlock(a2dIdx);
    isEditButtonEnabled = false;
    if (settings)
      logerr("Node <%s> overrided in blk, can't edit this node. Edit it manual in %s", tree->getCaption(leaf), fullPath.c_str());
    else
      logerr("Can't find node %s in blk", tree->getCaption(leaf));
  }

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
              fill_irq_settings(group, irq->getStr("name"), type, key, keyFloat, isEditButtonEnabled);
              break;
            }
          }
        }
      }
      animPlayer.resetSelectedA2dName();
    }
    else
    {
      fill_params_from_settings(a2dBlk, group, animBnlParams, fieldIdx);
      add_edit_box_if_not_exists(animBnlParams, group, fieldIdx, "a2d");
      add_edit_box_if_not_exists(animBnlParams, group, fieldIdx, "accept_name_mask_re");
      add_edit_box_if_not_exists(animBnlParams, group, fieldIdx, "decline_name_mask_re");
      if (!isA2dNode)
      {
        group->createSeparator(fieldIdx);
        animBnlParams.emplace_back(AnimParamData{fieldIdx, DataBlock::TYPE_NONE, String("separator")});
        fieldIdx++;
        group->createCombo(PID_NODES_SETTINGS_NODE_TYPE_COMBO_SELECT, "Node type", blend_node_types, settings->getBlockName());
        fill_params_from_settings(settings, group, animBnlParams, fieldIdx);
        DataBlock fullProps(curAsset->getSrcFilePath());
        const bool defaultForeign = fullProps.getBool("defaultForeignAnim", false);
        BlendNodeType type = static_cast<BlendNodeType>(lup(settings->getBlockName(), blend_node_types));
        fill_anim_blend_params_settings(type, group, animBnlParams, fieldIdx, defaultForeign);
        set_anim_blend_dependent_defaults(type, group, animBnlParams, curAsset->getMgr());
      }
      group->createButton(PID_NODES_SETTINGS_PLAY_ANIMATION, "Play animation");
      animPlayer.setSelectedA2dName(group->getText(PID_NODES_PARAMS_FIELD));
      group->createButton(PID_NODES_SETTINGS_SAVE, "Save", isEditButtonEnabled);
    }
  }
  PropPanel::ContainerPropertyControl *panel = getPluginPanel();
  panel->setEnabledById(PID_ANIM_BLEND_NODES_CREATE_NODE, isEditButtonEnabled);
  panel->setEnabledById(PID_ANIM_BLEND_NODES_ADD_LEAF, isEditButtonEnabled);
  panel->setEnabledById(PID_ANIM_BLEND_NODES_ADD_IRQ, isEditButtonEnabled);
  panel->setEnabledById(PID_ANIM_BLEND_NODES_REMOVE_NODE, isEditButtonEnabled);
}

// Add all fields if not exists based on CtrlType
static void fill_ctrls_params_settings(PropPanel::ContainerPropertyControl *panel, CtrlType type, int field_idx,
  dag::Vector<AnimParamData> &params)
{
  switch (type)
  {
    case ctrl_type_paramSwitch: param_switch_init_panel(params, panel, field_idx); break;
    case ctrl_type_moveNode: move_node_init_panel(params, panel, field_idx); break;
    case ctrl_type_rotateNode: rotate_node_init_panel(params, panel, field_idx); break;
    case ctrl_type_randomSwitch: random_switch_init_panel(params, panel, field_idx); break;
    case ctrl_type_hub: hub_init_panel(params, panel, field_idx); break;
    case ctrl_type_linearPoly: linear_poly_init_panel(params, panel, field_idx); break;
    case ctrl_type_rotateAroundNode: rotate_around_node_init_panel(params, panel, field_idx); break;
    case ctrl_type_eyeCtrl: eye_ctrl_init_panel(params, panel, field_idx); break;
    case ctrl_type_fifo3: fifo3_init_panel(params, panel, field_idx); break;

    default: break;
  }
}

void AnimTreePlugin::findCtrlsChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings)
{
  switch (data.type)
  {
    case ctrl_type_paramSwitch: paramSwitchFindChilds(panel, data, settings); break;
    case ctrl_type_hub: hubFindChilds(panel, data, settings); break;
    case ctrl_type_linearPoly: linearPolyFindChilds(panel, data, settings); break;
    case ctrl_type_randomSwitch: randomSwitchFindChilds(panel, data, settings); break;

    default: break;
  }
}

void AnimTreePlugin::fillCtrlsChilds(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  fillCtrlsChildsBody(panel, ctrlsTree->getRootLeaf(), ctrl_type_include);
  for (AnimCtrlData &ctrl : controllersData)
    if (is_include_ctrl(ctrl))
      fillCtrlsChildsBody(panel, ctrl.handle, ctrl.type);
}

void AnimTreePlugin::fillCtrlsChildsBody(PropPanel::ContainerPropertyControl *panel, TLeafHandle handle, CtrlType type)
{
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, ctrlsTree, handle, fullPath, /*only_includes*/ true);
  if (type == ctrl_type_include && props.blockExists("AnimBlendCtrl"))
    if (const DataBlock *ctrlsProps =
          get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, ctrlsTree, handle))
    {
      for (int blkIdx = 0, leafIdx = 0; blkIdx < ctrlsProps->blockCount(); ++blkIdx, ++leafIdx)
      {
        TLeafHandle leaf = ctrlsTree->getChildLeaf(handle, leafIdx);
        AnimCtrlData *leafData = find_data_by_handle(controllersData, leaf);
        // If get include leaf we need skip it, but not skip block
        if (leafData != controllersData.end() && is_include_ctrl(*leafData))
        {
          --blkIdx;
          continue;
        }

        const DataBlock *settings = ctrlsProps->getBlock(blkIdx);
        const char *nameField = settings->getStr("name", nullptr);
        // In props we can catch blocks with comments and we skip it
        if (!nameField)
        {
          --leafIdx;
          continue;
        }
        // We should have same hierarchy with leafs and blocks exclude cases above
        if (ctrlsTree->getCaption(leaf) != nameField)
        {
          logerr("Fields order in ctrls tree doesn't match order in blk with path:<%s> \nGet:<%s>, but expect:<%s>", fullPath,
            ctrlsTree->getCaption(leaf), nameField);
          return;
        }

        findCtrlsChilds(panel, *leafData, *settings);
      }
    }
}

void AnimTreePlugin::fillCtrlsSettings(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  const int lastCtrlsPid = !ctrlParams.empty() ? ctrlParams.back().pid : PID_CTRLS_PARAMS_FIELD;
  G_ASSERTF(lastCtrlsPid < PID_CTRLS_PARAMS_FIELD_SIZE, "Previous controller have more params than reserved");
  remove_ctrls_fields(group, lastCtrlsPid);
  ctrlParams.clear();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf || leaf == tree->getRootLeaf())
    return;

  AnimCtrlData *selectedData = find_data_by_handle(controllersData, leaf);
  if (selectedData == controllersData.end())
    return;

  if (selectedData->type == ctrl_type_include || selectedData->type == ctrl_type_inner_include)
  {
    fill_include_settings(tree, group, leaf, includePaths, ctrlParams, PID_CTRLS_PARAMS_FIELD);
    group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save");
  }
  else
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    String fullPath;
    DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);
    if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
    {
      DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf), /*should_exist*/ false);
      bool isEditable = true;
      if (!settings)
      {
        props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath, /*only_includes*/ true);
        ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent);
        if (ctrlsProps)
        {
          settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf));
          isEditable = false;
          logerr("Controller <%s> overrided in blk, can't edit this node. Edit it manual in %s", tree->getCaption(leaf),
            fullPath.c_str());
        }
      }
      if (settings)
      {
        int fieldIdx = PID_CTRLS_PARAMS_FIELD;
        group->createCombo(PID_CTRLS_TYPE_COMBO_SELECT, "Controller type", ctrl_type, settings->getBlockName(), isEditable);
        fill_params_from_settings(settings, group, ctrlParams, fieldIdx);
        CtrlType type = static_cast<CtrlType>(lup(settings->getBlockName(), ctrl_type, ctrl_type_not_found));
        fill_ctrls_params_settings(group, type, fieldIdx, ctrlParams);
        fillCtrlsBlocksSettings(group, type, settings);
        panel->setListBoxEventHandler(PID_CTRLS_NODES_LIST, &ctrlListSettingsEventHandler);
        group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save", isEditable);
        group->setEnabledById(PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT, isEditable);
        group->setEnabledById(PID_CTRLS_NODES_LIST_ADD, isEditable);
        group->setEnabledById(PID_CTRLS_NODES_LIST_REMOVE, isEditable);
      }
    }
  }
}

void AnimTreePlugin::fillCtrlsBlocksSettings(PropPanel::ContainerPropertyControl *panel, CtrlType type, const DataBlock *settings)
{
  switch (type)
  {
    case ctrl_type_paramSwitch: paramSwitchFillBlockSettings(panel, settings); break;
    case ctrl_type_randomSwitch: random_switch_init_block_settings(panel, settings); break;
    case ctrl_type_hub: hub_init_block_settings(panel, settings); break;
    case ctrl_type_linearPoly: linear_poly_init_block_settings(panel, settings); break;

    default: break;
  }
}

static void save_params_blk(DataBlock *settings, dag::ConstSpan<AnimParamData> params, PropPanel::ContainerPropertyControl *panel)
{
  const int includeNid = settings->getNameId("@include");
  for (int i = settings->paramCount() - 1; i >= 0; --i)
  {
    if ((!CHECK_COMMENT_PREFIX(settings->getParamName(i)) && settings->getParamNameId(i) != includeNid) ||
        IS_COMMENT_POST(settings->getParamName(i)))
      settings->removeParam(i);
  }

  int idx = 0;
  for (const AnimParamData &param : params)
  {
    switch (param.type)
    {
      case DataBlock::TYPE_STRING:
      {
        if (!CHECK_COMMENT_PREFIX(param.name))
        {
          const SimpleString value = panel->getText(param.pid);
          if (value == "##") // For ImGui combobox "##" mean empty value
            settings->addStr(param.name, "");
          else
            settings->addStr(param.name, value.c_str());
        }
        else if (IS_COMMENT_PRE(param.name))
        {
          // In this case we save location pre located comment with not removed value above
          settings->addStr(param.name, settings->getStr(0));
          settings->removeParam((uint32_t)0);
        }
        break;
      }
      case DataBlock::TYPE_REAL: settings->addReal(param.name, panel->getFloat(param.pid)); break;
      case DataBlock::TYPE_BOOL: settings->addBool(param.name, panel->getBool(param.pid)); break;
      case DataBlock::TYPE_POINT2: settings->addPoint2(param.name, panel->getPoint2(param.pid)); break;
      case DataBlock::TYPE_POINT3: settings->addPoint3(param.name, panel->getPoint3(param.pid)); break;
      case DataBlock::TYPE_INT: settings->addInt(param.name, panel->getInt(param.pid)); break;

      default: logerr("can't save param <%s> type: %d", param.name, param.type); break;
    }
    if (!CHECK_COMMENT_PREFIX(param.name))
    {
      const int nextIdx = idx + 1;
      if (nextIdx < params.size() && CHECK_COMMENT_PREFIX(params[nextIdx].name) && IS_COMMENT_POST(params[nextIdx].name))
        settings->addStr(params[nextIdx].name, panel->getTooltipById(param.pid));
    }
    ++idx;
  }
}

void AnimTreePlugin::setSelectedCtrlNodeListSettings(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  String fullPath;
  TLeafHandle parent = tree->getParentLeaf(leaf);
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);
  if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
  {
    DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf), /*should_exist*/ false);
    if (!settings)
    {
      props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath, /*only_includes*/ true);
      ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent);
      if (ctrlsProps)
        settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf));
    }
    if (settings)
    {
      CtrlType type = static_cast<CtrlType>(panel->getInt(PID_CTRLS_TYPE_COMBO_SELECT));
      switch (type)
      {
        case ctrl_type_randomSwitch: random_switch_set_selected_node_list_settings(panel, settings); break;
        case ctrl_type_paramSwitch: param_switch_set_selected_node_list_settings(panel, settings); break;
        case ctrl_type_hub: hub_set_selected_node_list_settings(panel, settings); break;
        case ctrl_type_linearPoly: linear_poly_set_selected_node_list_settings(panel, settings); break;

        default: break;
      }
    }
  }
}

static const char *NEW_NODE_CHILD_NAME = "new_node";

void AnimTreePlugin::addNodeCtrlList(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;
  AnimCtrlData *data = find_data_by_handle(controllersData, leaf);
  if (data != controllersData.end())
  {
    int idx = panel->addString(PID_CTRLS_NODES_LIST, NEW_NODE_CHILD_NAME);
    panel->setInt(PID_CTRLS_NODES_LIST, idx);
    setSelectedCtrlNodeListSettings(panel);
    panel->setEnabledById(PID_CTRLS_NODES_LIST_ADD, /*enabled*/ false);
    data->childs.emplace_back(ctrl_type_not_found);
  }
}

void update_add_button(PropPanel::ContainerPropertyControl *panel)
{
  SimpleString selectedName = panel->getText(PID_CTRLS_NODES_LIST);
  if (selectedName == NEW_NODE_CHILD_NAME)
    panel->setEnabledById(PID_CTRLS_NODES_LIST_ADD, /*enabled*/ true);
}

static void change_include_leaf(PropPanel::ContainerPropertyControl *tree, const char *name, TLeafHandle leaf)
{
  tree->setCaption(leaf, name);
  const int leafCount = tree->getChildCount(leaf);
  for (int i = 0; i < leafCount; ++i)
    tree->removeLeaf(tree->getChildLeaf(leaf, 0));
}

template <typename Data>
static void change_include_leaf(PropPanel::ContainerPropertyControl *tree, dag::Vector<Data> &data_vec, const char *name,
  TLeafHandle leaf)
{
  remove_childs_data_by_leaf(tree, data_vec, leaf);
  change_include_leaf(tree, name, leaf);
}

void AnimTreePlugin::removeNodeCtrlList(PropPanel::ContainerPropertyControl *panel)
{
  const int removeIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (removeIdx < 0)
    return;

  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  String fullPath;
  TLeafHandle parent = tree->getParentLeaf(leaf);
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);
  if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
    if (DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf)))
    {
      update_add_button(panel);
      CtrlType type = static_cast<CtrlType>(lup(settings->getBlockName(), ctrl_type, ctrl_type_not_found));
      switch (type)
      {
        case ctrl_type_randomSwitch: random_switch_remove_node_from_list(panel, settings); break;
        case ctrl_type_paramSwitch: param_switch_remove_node_from_list(panel, settings); break;
        case ctrl_type_hub: hub_remove_node_from_list(panel, settings); break;
        case ctrl_type_linearPoly: linear_poly_remove_node_from_list(panel, settings); break;

        default: break;
      }
      saveProps(props, fullPath);
    }

  AnimCtrlData *data = find_data_by_handle(controllersData, leaf);
  const SimpleString childName = panel->getText(PID_CTRLS_NODES_LIST);
  if (data != controllersData.end() && data->childs.size())
    data->childs.erase(data->childs.begin() + removeIdx);
  else
    logerr("Can't find selected leaf with name <%s> in controllers data for remove child idx", childName.c_str());

  panel->removeString(PID_CTRLS_NODES_LIST, removeIdx);
  setSelectedCtrlNodeListSettings(panel);
}

void AnimTreePlugin::fillStatesChilds(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  AnimStatesData *stateDescData = eastl::find_if(statesData.begin(), statesData.end(),
    [](const AnimStatesData &data) { return data.type == AnimStatesType::STATE_DESC; });
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *stateDescData, fullPath, /*only_includes*/ true);
  if (fullPath.empty())
    return;

  DataBlock *stateDesc = props.addBlock("stateDesc");
  for (int blkIdx = 0, leafIdx = 0; blkIdx < stateDesc->blockCount(); ++blkIdx, ++leafIdx)
  {
    TLeafHandle leaf = tree->getChildLeaf(stateDescData->handle, leafIdx);
    AnimStatesData *leafData = find_data_by_handle(statesData, leaf);

    const DataBlock *settings = stateDesc->getBlock(blkIdx);
    const char *nameField = settings->getStr("name", nullptr);

    // We should have same hierarchy with leafs and blocks exclude cases above
    if (tree->getCaption(leaf) != nameField)
    {
      logerr("Fields order in states tree doesn't match order in blk with path:<%s> \nGet:<%s>, but expect:<%s>", fullPath,
        tree->getCaption(leaf), nameField);
      return;
    }

    for (int i = 0; i < settings->blockCount(); ++i)
    {
      const char *childName = settings->getBlock(i)->getStr("name", nullptr);
      if (!childName)
        continue;

      int idx = leafData->childs.emplace_back(
        find_state_child_idx_by_name(panel, leafData->handle, controllersData, blendNodesData, childName));
      check_state_child_idx(idx, nameField, childName);
    }
  }
}

static void save_include_change(PropPanel::ContainerPropertyControl *tree, TLeafHandle include_leaf, dag::Vector<String> &paths,
  DataBlock &props, const DagorAsset &asset, const String &old_name, String &new_full_path)
{
  String *path = eastl::find_if(paths.begin(), paths.end(), [&old_name](const String &value) {
    char fileNameBuf[DAGOR_MAX_PATH];
    return old_name == dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), value.c_str());
  });

  if (path != paths.end())
  {
    const int includeNid = props.getNameId("@include");
    for (int i = 0; i < props.paramCount(); ++i)
      if (includeNid == props.getParamNameId(i) && *path == props.getStr(i))
        props.setStr(i, new_full_path.c_str());

    *path = new_full_path;
  }
  else
  {
    props.addStr("@include", new_full_path.c_str());
    paths.emplace_back(new_full_path);
  }

  if (!DataBlock::resolveIncludePath(new_full_path))
  {
    String folderPath = get_folder_path_based_on_parent(paths, asset, tree, include_leaf);
    new_full_path = String(0, "%s%s", folderPath.c_str(), new_full_path.c_str());
    dd_simplify_fname_c(new_full_path.c_str());
  }
}

bool AnimTreePlugin::checkIncludeBeforeSave(PropPanel::ContainerPropertyControl *tree, TLeafHandle leaf, const SimpleString &file_name,
  const SimpleString &new_path, const DataBlock &props)
{
  if (file_name.empty())
  {
    logerr("File name can't be empty");
    return false;
  }
  TLeafHandle parent = tree->getParentLeaf(leaf);
  if (!check_includes_not_looped(tree, parent, file_name.c_str(), curAsset->getSrcFileName()))
  {
    logerr("Find loop in includes, can't save leaf <%s> with parent <%s> already created above", file_name.c_str(),
      tree->getCaption(parent));
    return false;
  }
  String newFullPath = String(0, "%s%s.blk", new_path.c_str(), file_name.c_str());
  if (check_include_duplication_in_blk(props, newFullPath))
  {
    logerr("Can't save include <%s>, already exist in blk. Path: %s", file_name.str(), newFullPath.c_str());
    return false;
  }
  if (!createBlkIfNotExistWithDialog(newFullPath, tree, leaf))
    return false;

  return true;
}

bool AnimTreePlugin::createBlkIfNotExistWithDialog(String &path, PropPanel::ContainerPropertyControl *tree, TLeafHandle include_leaf)
{
  if (!DataBlock::resolveIncludePath(path))
  {
    String folderPath = get_folder_path_based_on_parent(includePaths, *curAsset, tree, include_leaf);
    path = String(0, "%s%s", folderPath.c_str(), path.c_str());
  }

  if (dd_file_exist(path.c_str()))
    return true;

  dd_simplify_fname_c(path.c_str());
  const int dialogResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Include change",
    "File with path: %s doesn't exist.\nDo you want to create new blk?", path.c_str());

  return dialogResult == wingw::MB_ID_YES;
}

void AnimTreePlugin::saveIncludeChange(PropPanel::ContainerPropertyControl *panel, PropPanel::ContainerPropertyControl *tree,
  TLeafHandle include_leaf, DataBlock &props, const String &old_name, String &new_full_path, ParentLeafs parents)
{
  save_include_change(tree, include_leaf, includePaths, props, *curAsset, old_name, new_full_path);

  const bool curParseIncludesAsParams = DataBlock::parseIncludesAsParams;
  DataBlock::parseIncludesAsParams = true;
  DataBlock blk;
  if (!dd_file_exist(new_full_path.c_str()))
    blk.saveToTextFile(new_full_path.c_str());
  else
    blk.load(new_full_path.c_str());

  fillTreesIncluded(panel, blk, new_full_path, parents);
  DataBlock::parseIncludesAsParams = curParseIncludesAsParams;
}

static TLeafHandle find_include_leaf_by_name_anim_blend(PropPanel::ContainerPropertyControl *tree, TLeafHandle parent,
  const String &sel_name)
{
  const int childCount = tree->getChildCount(parent);
  for (int i = 0; i < childCount; ++i)
  {
    TLeafHandle leaf = tree->getChildLeaf(parent, i);
    if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_INCLUDE && tree->getCaption(leaf) == sel_name)
      return leaf;
  }
  for (int i = 0; i < childCount; ++i)
    if (TLeafHandle leaf = find_include_leaf_by_name_anim_blend(tree, tree->getChildLeaf(parent, i), sel_name))
      return leaf;

  return nullptr;
}

struct ChangeIncludeResult
{
  TLeafHandle leaf;
  bool needUpdateSelected;
};

static ChangeIncludeResult change_include_leaf_nodes(PropPanel::ContainerPropertyControl *panel, dag::Vector<BlendNodeData> &nodes,
  const String &sel_name, const char *new_name)
{
  PropPanel::ContainerPropertyControl *nodesTree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  TLeafHandle selLeaf = nodesTree->getSelLeaf();
  TLeafHandle root = nodesTree->getRootLeaf();
  TLeafHandle nodeLeaf = find_include_leaf_by_name_anim_blend(nodesTree, root, sel_name);
  change_include_leaf(nodesTree, nodes, new_name, nodeLeaf);
  return {nodeLeaf, selLeaf == nodeLeaf || selLeaf != nodesTree->getSelLeaf()};
}

template <typename Data, typename Predicate>
static TLeafHandle find_include_leaf_by_name(PropPanel::ContainerPropertyControl *tree, dag::ConstSpan<Data> data_span,
  const String &sel_name, const Predicate &is_include_leaf)
{
  TLeafHandle root = tree->getRootLeaf();
  if (sel_name == tree->getCaption(root))
    return root;

  const Data *data = eastl::find_if(data_span.begin(), data_span.end(), [&sel_name, tree, &is_include_leaf](const Data &item) {
    return is_include_leaf(item) && sel_name == tree->getCaption(item.handle);
  });

  return data != data_span.end() ? data->handle : nullptr;
}

template <typename Data, typename Predicate>
static ChangeIncludeResult change_include_leaf(PropPanel::ContainerPropertyControl *panel, dag::Vector<Data> &data_vec,
  const String &sel_name, const char *new_name, int pid, const Predicate &is_include_leaf)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(pid)->getContainer();
  TLeafHandle selLeaf = tree->getSelLeaf();
  TLeafHandle leaf = find_include_leaf_by_name(tree, make_span_const(data_vec), sel_name, is_include_leaf);
  change_include_leaf(tree, data_vec, new_name, leaf);
  return {leaf, selLeaf == leaf || selLeaf != tree->getSelLeaf()};
}

// Returns true if need update selected settings
static bool remove_include_states_tree(PropPanel::ContainerPropertyControl *panel, dag::Vector<AnimStatesData> &states,
  const String &name)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle selLeaf = tree->getSelLeaf();
  bool needUpdateSelectedLeaf = false;
  auto updateFileNameAndCheckHandle = [selLeaf, &needUpdateSelectedLeaf](AnimStatesData *data) {
    data->fileName = "Root";
    if (data->handle == selLeaf)
      needUpdateSelectedLeaf = true;
  };
  if (AnimStatesData *data = find_data_by_type(states, AnimStatesType::STATE_DESC))
    if (data->fileName == name)
    {
      for (int i = states.size() - 1; i >= 0; --i)
      {
        if (states[i].type == AnimStatesType::CHAN || states[i].type == AnimStatesType::STATE ||
            states[i].type == AnimStatesType::STATE_ALIAS)
        {
          tree->removeLeaf(states[i].handle);
          states.erase(states.begin() + i);
        }
      }
      updateFileNameAndCheckHandle(data);
    }
  if (AnimStatesData *data = find_data_by_type(states, AnimStatesType::ROOT_PROPS))
    if (data->fileName == name)
    {
      updateFileNameAndCheckHandle(data);
    }
  if (AnimStatesData *data = find_data_by_type(states, AnimStatesType::INIT_ANIM_STATE))
    if (data->fileName == name)
    {
      for (int i = states.size() - 1; i >= 0; --i)
      {
        if (states[i].type == AnimStatesType::ENUM || states[i].type == AnimStatesType::ENUM_ITEM ||
            states[i].type == AnimStatesType::INIT_FIFO3 || states[i].type == AnimStatesType::POST_BLEND_CTRL_ORDER)
        {
          tree->removeLeaf(states[i].handle);
          states.erase(states.begin() + i);
        }
      }
      updateFileNameAndCheckHandle(data);
      if (AnimStatesData *enumData = find_data_by_type(states, AnimStatesType::ENUM_ROOT))
        updateFileNameAndCheckHandle(enumData);
    }
  return needUpdateSelectedLeaf || selLeaf != tree->getSelLeaf();
}

struct DependentNodesResult
{
  dag::Vector<int> dependentCtrls;
  dag::Vector<int> dependentStates;
  int dialogResult = 0;
};

void AnimTreePlugin::saveControllerSettings(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  AnimCtrlData *selectedData = find_data_by_handle(controllersData, leaf);
  if (selectedData == controllersData.end())
    return;

  TLeafHandle parent = tree->getParentLeaf(leaf);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);

  if (selectedData->type == ctrl_type_include || selectedData->type == ctrl_type_inner_include)
  {
    const SimpleString fileName = get_str_param_by_name_optional(ctrlParams, panel, "file name");
    const SimpleString newPath = get_str_param_by_name_optional(ctrlParams, panel, "folder path");
    if (!checkIncludeBeforeSave(tree, leaf, fileName, newPath, props))
      return;

    String newFullPath = String(0, "%s%s.blk", newPath.c_str(), fileName.c_str());
    const String selName = tree->getCaption(leaf);
    change_include_leaf(tree, controllersData, fileName.c_str(), leaf);
    if (selectedData->type == ctrl_type_include)
    {
      auto [nodeLeaf, updateSelectedNodes] = change_include_leaf_nodes(panel, blendNodesData, selName, fileName.c_str());
      auto [maskLeaf, updateSelectedMasks] =
        change_include_leaf(panel, nodeMasksData, selName, fileName.c_str(), PID_NODE_MASKS_TREE, is_include_node_mask);
      bool updateSelectedStates = remove_include_states_tree(panel, statesData, selName);
      saveIncludeChange(panel, tree, leaf, props, selName, newFullPath, {nodeLeaf, maskLeaf, leaf});
      if (updateSelectedNodes)
        selectedChangedAnimNodesTree(panel);
      if (updateSelectedMasks)
        selectedChangedNodeMasksTree(panel);
      if (updateSelectedStates)
        selectedChangedAnimStatesTree(panel);
    }
    else
    {
      // In this case we save inner include and it can have parent as include or inner include
      AnimCtrlData *parentData = find_data_by_handle(controllersData, parent);
      if ((parentData != controllersData.end() && parentData->type == ctrl_type_include) || parent == tree->getRootLeaf())
      {
        DataBlock *ctrlsBlk = props.addBlock("AnimBlendCtrl");
        save_include_change(tree, leaf, includePaths, *ctrlsBlk, *curAsset, selName, newFullPath);
      }
      else
        save_include_change(tree, leaf, includePaths, props, *curAsset, selName, newFullPath);

      const bool curParseIncludesAsParams = DataBlock::parseIncludesAsParams;
      DataBlock::parseIncludesAsParams = true;
      DataBlock blk;
      if (!dd_file_exist(newFullPath.c_str()))
        blk.saveToTextFile(newFullPath.c_str());
      else
        blk.load(newFullPath.c_str());

      fillCtrlsIncluded(tree, blk, newFullPath, leaf);
      DataBlock::parseIncludesAsParams = curParseIncludesAsParams;
    }
  }
  else
  {
    if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
      if (DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf)))
      {
        SimpleString newTypeName = panel->getText(PID_CTRLS_TYPE_COMBO_SELECT);
        // If ctrl type changed we need change block name and remove all blocks from old type if they exist
        if (settings->getBlockName() != newTypeName)
        {
          settings->changeBlockName(newTypeName.c_str());
          dag::ConstSpan<String> names = panel->getStrings(PID_CTRLS_NODES_LIST);
          // After change type user can't create more then 1 new node
          if (names.size() <= 1)
            for (int i = settings->blockCount() - 1; i >= 0; --i)
              settings->removeBlock(i);
        }
        CtrlType type = static_cast<CtrlType>(lup(newTypeName.c_str(), ctrl_type, ctrl_type_not_found));
        selectedData->type = type;
        tree->setButtonPictures(leaf, get_ctrl_icon_name(type));
        saveControllerParamsSettings(panel, settings);
        saveControllerBlocksSettings(panel, settings, *selectedData);
        const String oldName = tree->getCaption(leaf);
        const char *newName = settings->getStr("name");
        DependentNodesResult result;
        if (oldName != newName)
        {
          result = checkDependentNodes(panel, selectedData->id, newName, oldName);
          switch (result.dialogResult)
          {
            case PropPanel::DIALOG_ID_YES:
              // Save base props now, becuse we can rewrite blk file after update dependent childs
              saveProps(props, fullPath);
              proccesDependentNodes(panel, newName, oldName, result);
              // Change caption before update selected, becuse we can use caption in combobox for nodes
              tree->setCaption(leaf, newName);
              // If update child name in selected dependent state we need refill prop panel with updated child name
              updateSelectedDependentState(panel, result.dependentStates);
              break;
            case PropPanel::DIALOG_ID_NO: selectedData->id = curCtrlAndBlendNodeUniqueIdx++; break;
            case PropPanel::DIALOG_ID_CANCEL: return;
            default: break;
          }
          if (result.dialogResult != PropPanel::DIALOG_ID_YES)
            tree->setCaption(leaf, newName);
        }
        // For enum_gen paramSwitch need find childs after rename operation in enums
        if (selectedData->type == ctrl_type_paramSwitch)
        {
          ParamSwitchType type = static_cast<ParamSwitchType>(panel->getInt(PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT));
          if (type == PARAM_SWITCH_TYPE_ENUM_GEN)
          {
            selectedData->childs.clear();
            paramSwitchFindChilds(panel, *selectedData, *settings, /*find_enum_gen_parent*/ false);
          }
        }
        // Early return for skip rewrite props with old values in saveProps if name changed
        if (result.dialogResult == PropPanel::DIALOG_ID_YES)
          return;
      }
  }

  saveProps(props, fullPath);
}

void AnimTreePlugin::saveControllerParamsSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  // Make copy, becuse we can don't save optinal fields
  dag::Vector<AnimParamData> params = ctrlParams;

  // Prepare params, before save we need remove unchanged default params based on CtrlType
  CtrlType type = static_cast<CtrlType>(lup(settings->getBlockName(), ctrl_type, ctrl_type_not_found));
  switch (type)
  {
    case ctrl_type_paramSwitch: param_switch_prepare_params(params, panel); break;
    case ctrl_type_moveNode: move_node_prepare_params(params, panel); break;
    case ctrl_type_rotateNode: rotate_node_prepare_params(params, panel); break;
    case ctrl_type_randomSwitch: random_switch_prepare_params(params, panel); break;
    case ctrl_type_hub: hub_prepare_params(params, panel); break;
    case ctrl_type_linearPoly: linear_poly_prepare_params(params, panel); break;
    case ctrl_type_rotateAroundNode: rotate_around_node_prepare_params(params, panel); break;
    case ctrl_type_eyeCtrl: eye_ctrl_prepare_params(params, panel); break;
    case ctrl_type_fifo3: fifo3_prepare_params(params, panel); break;

    default: break;
  }

  save_params_blk(settings, make_span_const(params), panel);
}

void AnimTreePlugin::saveControllerBlocksSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data)
{
  update_add_button(panel);
  CtrlType type = static_cast<CtrlType>(lup(settings->getBlockName(), ctrl_type, ctrl_type_not_found));
  switch (type)
  {
    case ctrl_type_paramSwitch: paramSwitchSaveBlockSettings(panel, settings, data); break;
    case ctrl_type_hub: hubSaveBlockSettings(panel, settings, data); break;
    case ctrl_type_linearPoly: linearPolySaveBlockSettings(panel, settings, data); break;
    case ctrl_type_randomSwitch: randomSwitchSaveBlockSettings(panel, settings, data); break;

    default: break;
  }
}

void AnimTreePlugin::setTreeFilter(PropPanel::ContainerPropertyControl *panel, int tree_pid, int filter_pid)
{
  SimpleString filterText = panel->getText(filter_pid);
  panel->setText(tree_pid, String(filterText).toLower().c_str());
}

void AnimTreePlugin::addIncludeLeaf(PropPanel::ContainerPropertyControl *panel, int main_pid, TLeafHandle main_parent,
  const String &parent_name)
{
  PropPanel::ContainerPropertyControl *nodesTree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *masksTree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle nodesInclude, masksInclude, ctrlsInclude;

  TLeafHandle rootNodes = nodesTree->getRootLeaf();
  if (main_pid == PID_ANIM_BLEND_NODES_TREE)
    nodesInclude = main_parent;
  else if (parent_name == nodesTree->getCaption(rootNodes))
    nodesInclude = rootNodes;
  else
    nodesInclude = find_include_leaf_by_name_anim_blend(nodesTree, rootNodes, parent_name);

  if (main_pid == PID_NODE_MASKS_TREE)
    masksInclude = main_parent;
  else
    masksInclude = find_include_leaf_by_name(masksTree, make_span_const(nodeMasksData), parent_name, is_include_node_mask);

  if (main_pid == PID_ANIM_BLEND_CTRLS_TREE)
    ctrlsInclude = main_parent;
  else
    ctrlsInclude = find_include_leaf_by_name(ctrlsTree, make_span_const(controllersData), parent_name, is_include_ctrl);

  TLeafHandle nodesLeaf = nodesTree->createTreeLeaf(nodesInclude, "new_include", FOLDER_ICON);
  addBlendNode(nodesLeaf, BlendNodeType::INCLUDE);
  nodesTree->setUserData(nodesLeaf, (void *)&ANIM_BLEND_NODE_INCLUDE);
  TLeafHandle masksLeaf = masksTree->createTreeLeaf(masksInclude, "new_include", FOLDER_ICON);
  nodeMasksData.emplace_back(NodeMaskData{masksLeaf, NodeMaskType::INCLUDE});
  TLeafHandle ctrlsLeaf = ctrlsTree->createTreeLeaf(ctrlsInclude, "new_include", FOLDER_ICON);
  addController(ctrlsLeaf, ctrl_type_include);

  if (main_pid == PID_ANIM_BLEND_NODES_TREE)
    nodesTree->setSelLeaf(nodesLeaf);
  else if (main_pid == PID_NODE_MASKS_TREE)
    masksTree->setSelLeaf(masksLeaf);
  else if (main_pid == PID_ANIM_BLEND_CTRLS_TREE)
    ctrlsTree->setSelLeaf(ctrlsLeaf);
}

void AnimTreePlugin::addEnumToAnimStatesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle enumLeaf = getEnumsRootLeaf(tree);
  AnimStatesData *data = find_data_by_handle(statesData, enumLeaf);
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;

  if (create_new_leaf(enumLeaf, tree, panel, "enum_name", ENUM_ICON, PID_ANIM_STATES_NAME_FIELD))
  {
    panel->setEnabledById(PID_ANIM_STATES_ADD_ITEM, /*enabled*/ true);
    props.addBlock("initAnimState")->addBlock("enum")->addBlock("enum_name");
    saveProps(props, fullPath);
    statesData.emplace_back(AnimStatesData{tree->getSelLeaf(), AnimStatesType::ENUM});
    selectedChangedAnimStatesTree(panel);
  }
}

void AnimTreePlugin::addItemToAnimStatesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  AnimStatesData *data = find_data_by_handle(statesData, leaf);
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;

  if (isEnumOrEnumItem(leaf, tree))
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    if (parent == enumsRootLeaf)
      parent = leaf;

    if (create_new_leaf(parent, tree, panel, "leaf_item", ENUM_ITEM_ICON, PID_ANIM_STATES_NAME_FIELD))
    {
      const String parentName = tree->getCaption(parent);
      props.addBlock("initAnimState")->addBlock("enum")->addBlock(parentName.c_str())->addBlock("leaf_item");
      saveProps(props, fullPath);
      statesData.emplace_back(AnimStatesData{tree->getSelLeaf(), AnimStatesType::ENUM_ITEM});
      selectedChangedAnimStatesTree(panel);
    }
  }
  else if (data->type == AnimStatesType::CHAN || data->type == AnimStatesType::STATE_ALIAS || data->type == AnimStatesType::STATE ||
           data->type == AnimStatesType::STATE_DESC)
  {
    TLeafHandle stateDescLeaf = leaf;
    if (data->type != AnimStatesType::STATE_DESC)
      stateDescLeaf = tree->getParentLeaf(leaf);
    addStateDescItem(panel, props, fullPath, stateDescLeaf, AnimStatesType::STATE, "new_node");
  }
}

static void remove_include_node(PropPanel::ContainerPropertyControl *tree, dag::Vector<String> &paths, DataBlock &props,
  TLeafHandle leaf)
{
  String name = tree->getCaption(leaf);
  String *path = eastl::find_if(paths.begin(), paths.end(), [&name](const String &value) {
    char fileNameBuf[DAGOR_MAX_PATH];
    return name == dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), value.c_str());
  });

  if (path != paths.end())
  {
    const int includeNid = props.getNameId("@include");
    for (int i = 0; i < props.paramCount(); ++i)
      if (includeNid == props.getParamNameId(i) && *path == props.getStr(i))
        props.removeParam(i);

    paths.erase(path);
  }
  tree->removeLeaf(leaf);
}

template <typename Data>
static void remove_include_tree_node(PropPanel::ContainerPropertyControl *tree, dag::Vector<Data> &data_vec,
  dag::Vector<String> &paths, DataBlock &props, TLeafHandle leaf)
{
  remove_childs_data_by_leaf(tree, data_vec, leaf);
  remove_include_node(tree, paths, props, leaf);
}

// Returns true if need update selected settings
template <typename Data, typename Predicate>
static bool remove_include_leaf(PropPanel::ContainerPropertyControl *panel, int pid, dag::Vector<Data> &data_vec, const String &name,
  const Predicate &is_include_leaf)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(pid)->getContainer();
  TLeafHandle selLeaf = tree->getSelLeaf();
  Data *data = eastl::find_if(data_vec.begin(), data_vec.end(),
    [&name, tree, &is_include_leaf](const Data &item) { return is_include_leaf(item) && name == tree->getCaption(item.handle); });
  if (data != data_vec.end())
  {
    TLeafHandle includeLeaf = data->handle;
    data_vec.erase(data);
    remove_childs_data_by_leaf(tree, data_vec, includeLeaf);
    tree->removeLeaf(includeLeaf);
  }
  return selLeaf != tree->getSelLeaf();
}

// Returns true if need update selected settings
static bool remove_include_leaf_nodes(PropPanel::ContainerPropertyControl *panel, dag::Vector<BlendNodeData> &nodes,
  const String &name)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  TLeafHandle selLeaf = tree->getSelLeaf();
  TLeafHandle root = tree->getRootLeaf();
  TLeafHandle leaf = find_include_leaf_by_name_anim_blend(tree, root, name);
  remove_childs_data_by_leaf(tree, nodes, leaf);
  BlendNodeData *removeData = find_data_by_handle(nodes, leaf);
  if (removeData != nodes.end())
    nodes.erase(removeData);
  tree->removeLeaf(leaf);
  return selLeaf != tree->getSelLeaf();
}

void AnimTreePlugin::removeNodeFromAnimStatesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  AnimStatesData *data = find_data_by_handle(statesData, leaf);
  if (data == statesData.end())
    return;

  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;


  if (data->type == AnimStatesType::ENUM || data->type == AnimStatesType::ENUM_ITEM)
  {
    const int initAnimStateId = props.getNameId("initAnimState");
    const int enumId = props.getNameId("enum");
    if (initAnimStateId != -1 && enumId != -1)
    {
      DataBlock *initAnimStateProps = props.getBlockByNameId(initAnimStateId);
      DataBlock *enumBlock = initAnimStateProps->getBlockByNameId(enumId);
      const String name = tree->getCaption(leaf);
      if (data->type == AnimStatesType::ENUM_ITEM)
      {
        const String parentName = tree->getCaption(tree->getParentLeaf(leaf));
        DataBlock *parentBlock = enumBlock->getBlockByName(parentName.c_str());
        if (parentBlock)
          parentBlock->removeBlock(name.c_str());
      }
      else
        enumBlock->removeBlock(name.c_str());

      if (enumBlock->isEmpty())
        initAnimStateProps->removeBlock("enum");
      if (initAnimStateProps->isEmpty())
        props.removeBlock("initAnimState");
    }

    statesData.erase(data);
    tree->removeLeaf(leaf);
  }
  else if (data->type == AnimStatesType::CHAN || data->type == AnimStatesType::STATE_ALIAS || data->type == AnimStatesType::STATE)
  {
    const String name = tree->getCaption(leaf);
    DataBlock *stateDescProps = props.getBlockByName("stateDesc");
    for (int i = 0; i < stateDescProps->blockCount(); ++i)
    {
      DataBlock *settings = stateDescProps->getBlock(i);
      if (name == settings->getStr("name", ""))
      {
        stateDescProps->removeBlock(i);
        break;
      }
    }
    if (stateDescProps->isEmpty())
      props.removeBlock("stateDesc");
    statesData.erase(data);
    tree->removeLeaf(leaf);
  }

  saveProps(props, fullPath);
  selectedChangedAnimStatesTree(panel);
}

#define replaceParam(name, type)                  \
  if (props.paramExists(#name))                   \
  {                                               \
    out.set##type(#name, props.get##type(#name)); \
    props.removeParam(#name);                     \
  }

void AnimTreePlugin::saveSettingsAnimStatesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  AnimStatesData *data = find_data_by_handle(statesData, leaf);
  if (data == statesData.end())
    return;
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;

  // Change blk location
  if (data->type == AnimStatesType::INIT_ANIM_STATE || data->type == AnimStatesType::STATE_DESC ||
      data->type == AnimStatesType::ROOT_PROPS)
  {
    const SimpleString newFileName = panel->getText(PID_ANIM_STATES_NAME_FIELD);
    if (!newFileName.empty() && newFileName != data->fileName.c_str())
    {
      data->fileName = newFileName.c_str();
      String outFullPath;
      DataBlock out = getPropsAnimStates(panel, *data, outFullPath);
      DataBlock *copy = nullptr;
      if (data->type == AnimStatesType::INIT_ANIM_STATE)
      {
        copy = props.getBlockByName("initAnimState");
        // Enum block located in initAnimState block and need update after change include
        AnimStatesData *enumRoot = eastl::find_if(statesData.begin(), statesData.end(),
          [](const AnimStatesData &data) { return data.type == AnimStatesType::ENUM_ROOT; });
        enumRoot->fileName = newFileName.c_str();
      }
      else if (data->type == AnimStatesType::ROOT_PROPS)
      {
        replaceParam(root, Str);
        replaceParam(export, Bool);
        replaceParam(defaultForeignAnim, Bool);
      }
      else // AnimStatesType::STATE_DESC
        copy = props.getBlockByName("stateDesc");

      if (copy || data->type == AnimStatesType::ROOT_PROPS)
      {
        if (copy)
        {
          out.setBlock(copy, copy->getBlockName());
          props.removeBlock(copy->getBlockName());
        }
        saveProps(props, fullPath);
        props = out;
        fullPath = outFullPath;
      }
    }
  }

  // Save props based on type
  if (data->type == AnimStatesType::ENUM || data->type == AnimStatesType::ENUM_ITEM)
  {
    dag::Vector<AnimCtrlData *> ctrlsForUpdate;
    TLeafHandle parent = tree->getParentLeaf(leaf);
    const String oldName = tree->getCaption(leaf);
    SimpleString name = panel->getText(PID_ANIM_STATES_NAME_FIELD);
    if (oldName != name.c_str() && is_name_exists_in_childs(parent, tree, name))
    {
      logerr("Name \"%s\" is not unique, can't save", name);
      return;
    }

    DataBlock *enumRootProps = props.addBlock("initAnimState")->addBlock("enum");
    if (data->type == AnimStatesType::ENUM)
    {
      DataBlock *enumBlk = enumRootProps->addBlock(oldName.c_str());
      enumBlk->changeBlockName(name.c_str());
    }
    else // AnimStatesType::ENUM_ITEM
    {
      const String parentName = tree->getCaption(parent);
      DataBlock *enumProps = enumRootProps->addBlock(parentName.c_str());
      DataBlock *enumItemProps = enumProps->addBlock(oldName.c_str());
      enumItemProps->changeBlockName(name.c_str());
      const int selectedParamSwitchValue = panel->getInt(PID_STATES_NODES_LIST);
      if (selectedParamSwitchValue >= 0)
      {
        SimpleString paramSwitchName = panel->getText(PID_STATES_ENUM_PARAM_SWITCH_NAME);
        if (paramSwitchName == "##") // For ImGui combobox "##" mean empty value
          paramSwitchName = "";
        panel->setText(PID_STATES_NODES_LIST, paramSwitchName.c_str());
        const char *oldParamSwitchName = enumItemProps->getParamName(selectedParamSwitchValue);
        const char *oldParamSwitchValue = enumItemProps->getStr(selectedParamSwitchValue);
        if (paramSwitchName != oldParamSwitchName)
          enumItemProps->changeParamName(selectedParamSwitchValue, paramSwitchName.c_str());

        auto addDependentCtrl = [panel, &ctrlsForUpdate, &controllers = controllersData](const char *name) {
          PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
          AnimCtrlData *ctrlChild =
            eastl::find_if(controllers.begin(), controllers.end(), [ctrlsTree, name](const AnimCtrlData &data) {
              return data.type == ctrl_type_paramSwitch && ctrlsTree->getCaption(data.handle) == name;
            });
          if (ctrlChild != controllers.end())
            ctrlsForUpdate.emplace_back(ctrlChild);
          else
            logerr("Can't find paramSwitch controller <%s> for update childs when update enum item value", name);
        };
        // Need update childs for all dependent controllers when update _use value or change from _use to paramSwitch
        if (paramSwitchName == "_use" || !strcmp(oldParamSwitchName, "_use"))
        {
          PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
          AnimStatesData *enumData = find_data_by_handle(statesData, parent);
          for (int idx : enumData->childs)
          {
            AnimCtrlData *ctrlChild = eastl::find_if(controllersData.begin(), controllersData.end(),
              [idx](const AnimCtrlData &data) { return idx == data.id; });
            // We could remove the controller if we didn't find it, just skip it
            if (ctrlChild != controllersData.end())
              ctrlsForUpdate.emplace_back(ctrlChild);
          }
        }
        else if ((paramSwitchName == oldParamSwitchName || !strcmp(oldParamSwitchName, "")) &&
                 panel->getText(PID_STATES_ENUM_PARAM_SWITCH_VALUE) != oldParamSwitchValue)
          addDependentCtrl(paramSwitchName.c_str());
        else if (paramSwitchName != oldParamSwitchName)
        {
          addDependentCtrl(oldParamSwitchName);
          addDependentCtrl(paramSwitchName.c_str());
        }

        auto saveValue = [panel, enumItemProps, selectedParamSwitchValue](int pid) {
          SimpleString value = panel->getText(pid);
          if (value == "##") // For ImGui combobox "##" mean empty value
            value = "";
          enumItemProps->setStr(selectedParamSwitchValue, value);
        };
        if (paramSwitchName == "_use")
          saveValue(PID_STATES_ENUM_ENUM_ITEM_VALUE);
        else
          saveValue(PID_STATES_ENUM_PARAM_SWITCH_VALUE);
      }
    }
    saveProps(props, fullPath);
    tree->setCaption(leaf, name);
    PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
    for (AnimCtrlData *data : ctrlsForUpdate)
    {
      TLeafHandle includeLeaf = ctrlsTree->getParentLeaf(data->handle);
      String fullPath;
      DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, ctrlsTree, includeLeaf, fullPath, /*only_includes*/ true);
      DataBlock *ctrlProps =
        get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, ctrlsTree, includeLeaf);
      DataBlock *settings = find_block_by_name(ctrlProps, ctrlsTree->getCaption(data->handle));
      data->childs.clear();
      paramSwitchFindChilds(panel, *data, *settings, /*find_enum_gen_parent*/ false);
    }
  }
  else if (data->type == AnimStatesType::STATE_DESC)
  {
    DataBlock *settings = props.addBlock("stateDesc");
    saveStatesParamsSettings(panel, settings, data->type, *settings);
    saveProps(props, fullPath);
  }
  else if (data->type == AnimStatesType::ROOT_PROPS)
  {
    saveStatesParamsSettings(panel, &props, data->type, props);
    saveProps(props, fullPath);
  }
  else if (data->type == AnimStatesType::INIT_ANIM_STATE)
  {
    DataBlock *settings = props.addBlock("initAnimState");
    saveStatesParamsSettings(panel, settings, data->type, *settings);
    saveProps(props, fullPath);
  }
  else if (data->type == AnimStatesType::CHAN || data->type == AnimStatesType::STATE_ALIAS || data->type == AnimStatesType::STATE)
  {
    DataBlock *stateDesc = props.addBlock("stateDesc");
    const String oldName = tree->getCaption(leaf);
    if (DataBlock *settings = find_block_by_name(stateDesc, oldName))
    {
      const char *newTypeName = get_state_desc_cbox_block_name(panel->getInt(PID_STATES_STATE_DESC_TYPE_COMBO_SELECT));
      // If state desc type changed we need change block name
      if (settings->getBlockName() != newTypeName)
      {
        settings->changeBlockName(newTypeName);
        AnimStatesType newType = get_state_desc_cbox_enum_value(panel->getInt(PID_STATES_STATE_DESC_TYPE_COMBO_SELECT));
        tree->setButtonPictures(leaf, get_state_desc_icon(newType));
        data->type = newType;
      }
      saveStatesParamsSettings(panel, settings, data->type, *stateDesc);
      switch (data->type)
      {
        case AnimStatesType::STATE: stateSaveBlockSettings(panel, *settings, *data, *stateDesc); break;
        default: break;
      }
      const char *newName = settings->getStr("name");
      if (oldName != newName)
        tree->setCaption(leaf, newName);
    }
    saveProps(props, fullPath);
  }
}

void AnimTreePlugin::saveStatesParamsSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimStatesType type,
  const DataBlock &props)
{
  // Make copy, becuse we can don't save optinal fields
  dag::Vector<AnimParamData> params = stateParams;

  // Prepare params, before save we need remove unchanged default params based on AnimStatesType
  switch (type)
  {
    case AnimStatesType::STATE_DESC: state_desc_prepare_params(params, panel); break;
    case AnimStatesType::CHAN: chan_prepare_params(params, panel); break;
    case AnimStatesType::STATE_ALIAS: state_alias_prepare_params(params, panel); break;
    case AnimStatesType::STATE: state_prepare_params(params, panel, props); break;
    case AnimStatesType::ROOT_PROPS: root_props_prepare_params(params, panel); break;
    default: break;
  }

  save_params_blk(settings, make_span_const(params), panel);
}

DataBlock AnimTreePlugin::getPropsAnimStates(PropPanel::ContainerPropertyControl *panel, const AnimStatesData &data, String &full_path,
  bool only_includes)
{
  PropPanel::ContainerPropertyControl *statesTree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  const AnimStatesData *includeSource = &data;
  if (includeSource->type == AnimStatesType::ENUM || includeSource->type == AnimStatesType::ENUM_ITEM ||
      includeSource->type == AnimStatesType::CHAN || includeSource->type == AnimStatesType::STATE ||
      includeSource->type == AnimStatesType::STATE_ALIAS || includeSource->type == AnimStatesType::POST_BLEND_CTRL_ORDER ||
      includeSource->type == AnimStatesType::INIT_FIFO3)
  {
    TLeafHandle parent = statesTree->getParentLeaf(data.handle);
    if (includeSource->type == AnimStatesType::ENUM_ITEM)
      parent = statesTree->getParentLeaf(parent);

    includeSource = find_data_by_handle(statesData, parent);
  }
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  return get_props_from_include_state_data(includePaths, controllersData, *curAsset, ctrlsTree, *includeSource, full_path,
    only_includes);
}

static void fill_states_params_settings(PropPanel::ContainerPropertyControl *panel, AnimStatesType type, int field_idx,
  dag::Vector<AnimParamData> &params, const DataBlock &props)
{
  switch (type)
  {
    case AnimStatesType::STATE_DESC: state_desc_init_panel(params, panel, field_idx); break;
    case AnimStatesType::CHAN: chan_init_panel(params, panel, field_idx); break;
    case AnimStatesType::STATE_ALIAS: state_alias_init_panel(params, panel, field_idx); break;
    case AnimStatesType::STATE: state_init_panel(params, panel, field_idx, props); break;
    case AnimStatesType::ROOT_PROPS: root_props_init_panel(params, panel, field_idx); break;
    default: break;
  }
}

static void fill_states_block_settings(PropPanel::ContainerPropertyControl *prop_panel, AnimStatesType type, const DataBlock &settings,
  const DataBlock &state_desc, dag::ConstSpan<AnimStatesData> states_data)
{
  switch (type)
  {
    case AnimStatesType::STATE: state_init_block_settings(prop_panel, settings, state_desc, states_data); break;
    default: break;
  }
}

void fill_include_file_combo(PropPanel::ContainerPropertyControl *panel, PropPanel::ContainerPropertyControl *group,
  dag::ConstSpan<AnimCtrlData> ctrls, const AnimStatesData &data, const char *combo_name)
{
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  Tab<String> names;
  names.emplace_back("Root");
  // We use ctrls data becuse include paths content includes and inner includes, we want use only includes
  for (const AnimCtrlData &ctrl : ctrls)
    if (ctrl.type == ctrl_type_include)
      names.emplace_back(ctrlsTree->getCaption(ctrl.handle));

  group->createCombo(PID_ANIM_STATES_NAME_FIELD, combo_name, names, data.fileName);
}

void AnimTreePlugin::selectedChangedAnimStatesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_STATES_SETTINGS_GROUP)->getContainer();
  group->removeById(PID_ANIM_STATES_NAME_FIELD);
  remove_fields(group, PID_STATES_PARAMS_FIELD, PID_STATES_PARAMS_FIELD_SIZE);
  remove_fields(group, PID_STATES_ACTION_FIELD, PID_STATES_SETTINGS_SAVE);
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
  {
    panel->setEnabledById(PID_ANIM_STATES_ADD_ITEM, /*enabled*/ false);
    return;
  }
  AnimStatesData *data = find_data_by_handle(statesData, leaf);
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;
  stateParams.clear();

  const bool isEnumSelected = isEnumOrEnumItem(leaf, tree);
  const bool isStateDescItemSelected = data->type == AnimStatesType::CHAN || data->type == AnimStatesType::STATE_ALIAS ||
                                       data->type == AnimStatesType::STATE || data->type == AnimStatesType::STATE_DESC;
  panel->setEnabledById(PID_ANIM_STATES_ADD_ITEM, isEnumSelected || isStateDescItemSelected);
  if (isEnumSelected)
  {
    group->createEditBox(PID_ANIM_STATES_NAME_FIELD, "Field name", tree->getCaption(leaf));
    if (data->type == AnimStatesType::ENUM_ITEM)
    {
      DataBlock *initAnimStateProps = props.getBlockByName("initAnimState");
      DataBlock *enumRootProps = initAnimStateProps->getBlockByName("enum");
      DataBlock *enumProps = enumRootProps->addBlock(tree->getCaption(tree->getParentLeaf(leaf)));
      DataBlock *enumItemProps = enumProps->addBlock(tree->getCaption(leaf));
      Tab<String> names;
      for (int i = 0; i < enumItemProps->paramCount(); ++i)
        names.emplace_back(enumItemProps->getParamName(i));
      const char *defaultParamSwitch = enumItemProps->paramCount() ? enumItemProps->getParamName(0) : "";

      PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
      Tab<String> paramSwitchNames;
      paramSwitchNames.emplace_back("##");
      paramSwitchNames.emplace_back("_use");
      for (const AnimCtrlData &data : controllersData)
        if (data.type == ctrl_type_paramSwitch)
          paramSwitchNames.emplace_back(ctrlsTree->getCaption(data.handle));

      group->createList(PID_STATES_NODES_LIST, "List", names, 0);
      group->createButton(PID_STATES_NODES_LIST_ADD, "Add");
      group->createButton(PID_STATES_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
      group->createCombo(PID_STATES_ENUM_PARAM_SWITCH_NAME, "Param switch", paramSwitchNames, defaultParamSwitch,
        group->getInt(PID_STATES_NODES_LIST) >= 0);
      updateEnumItemValueCombo(panel);
    }
  }
  else if (leaf == tree->getRootLeaf())
  {
    PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
    Tab<String> names;
    names.emplace_back("Root");
    // We use ctrls data becuse include paths content includes and inner includes, we want use only includes
    for (const AnimCtrlData &data : controllersData)
      if (data.type == ctrl_type_include)
        names.emplace_back(ctrlsTree->getCaption(data.handle));

    group->createCombo(PID_ANIM_STATES_NAME_FIELD, "Root include", names, tree->getCaption(leaf), statesData.empty());
  }
  else if (data->type == AnimStatesType::ENUM_ROOT)
    group->createCombo(PID_ANIM_STATES_NAME_FIELD, "Enum include", {data->fileName}, data->fileName, /*enabled*/ false);
  else if (data->type == AnimStatesType::INIT_ANIM_STATE)
  {
    fill_include_file_combo(panel, group, controllersData, *data, "Init anim state include");
    DataBlock *settings = props.getBlockByName("initAnimState");
    int fieldIdx = PID_STATES_PARAMS_FIELD;
    fill_params_from_settings(settings, group, stateParams, fieldIdx);
  }
  else if (data->type == AnimStatesType::STATE_DESC)
  {
    int fieldIdx = PID_STATES_PARAMS_FIELD;
    DataBlock *settings = props.addBlock("stateDesc");
    fill_include_file_combo(panel, group, controllersData, *data, "State description include");
    fill_params_from_settings(settings, group, stateParams, fieldIdx);
    fill_states_params_settings(group, data->type, fieldIdx, stateParams, *settings);
  }
  else if (data->type == AnimStatesType::ROOT_PROPS)
  {
    int fieldIdx = PID_STATES_PARAMS_FIELD;
    fill_include_file_combo(panel, group, controllersData, *data, "Root props include");
    fill_params_from_settings(&props, group, stateParams, fieldIdx);
    fill_states_params_settings(group, data->type, fieldIdx, stateParams, props);

    // Replace root filed from editbox to combobox
    Tab<String> childNames;
    childNames.emplace_back("##");
    fill_child_names(childNames, panel, blendNodesData, controllersData);
    AnimParamData *rootParam = find_param_by_name(stateParams, "root");
    const SimpleString rootValue = panel->getText(rootParam->pid);
    panel->removeById(rootParam->pid);
    int index = get_selected_name_idx_combo(childNames, rootValue);
    group->createCombo(rootParam->pid, rootParam->name, childNames, index);
    group->moveById(rootParam->pid, rootParam->pid + 1);
  }
  else if (data->type == AnimStatesType::CHAN || data->type == AnimStatesType::STATE_ALIAS || data->type == AnimStatesType::STATE)
  {
    int fieldIdx = PID_STATES_PARAMS_FIELD;
    DataBlock *stateProps = props.getBlockByName("stateDesc");
    DataBlock *settings = find_block_by_name(stateProps, tree->getCaption(leaf), /*should_exist*/ false);
    bool isEditable = true;
    if (!settings)
    {
      props = getPropsAnimStates(panel, *data, fullPath, /*only_includes*/ true);
      stateProps = props.getBlockByName("stateDesc");
      settings = find_block_by_name(stateProps, tree->getCaption(leaf));
      isEditable = false;
      logerr("State description node <%s> overrided in blk, can't edit this node. Edit it manual in %s", tree->getCaption(leaf),
        fullPath.c_str());
    }
    if (settings)
    {
      group->createCombo(PID_STATES_STATE_DESC_TYPE_COMBO_SELECT, "State description type", state_desc_combo_box_types,
        get_state_desc_cbox_index(data->type), isEditable);
      fill_params_from_settings(settings, group, stateParams, fieldIdx);
      fill_states_params_settings(group, data->type, fieldIdx, stateParams, *stateProps);
      fill_states_block_settings(panel, data->type, *settings, *stateProps, statesData);
      panel->setListBoxEventHandler(PID_STATES_NODES_LIST, &stateListSettingsEventHandler);
      group->setEnabledById(PID_STATES_SETTINGS_SAVE, isEditable);
      group->setEnabledById(PID_STATES_NODES_LIST_ADD, isEditable);
      group->setEnabledById(PID_STATES_NODES_LIST_REMOVE, isEditable);
    }
  }
  group->moveById(PID_STATES_SETTINGS_SAVE);
}

void AnimTreePlugin::changeStateDescType(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_STATES_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  AnimStatesData *data = find_data_by_handle(statesData, leaf);
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;
  const AnimStatesType type = get_state_desc_cbox_enum_value(panel->getInt(PID_STATES_STATE_DESC_TYPE_COMBO_SELECT));
  remove_fields(panel, PID_STATES_PARAMS_FIELD, PID_STATES_PARAMS_FIELD_SIZE);
  remove_fields(panel, PID_STATES_ACTION_FIELD + 1, PID_STATES_ACTION_FIELD_SIZE);
  stateParams.clear();

  int fieldIdx = PID_STATES_PARAMS_FIELD;
  DataBlock *stateDescProps = props.getBlockByName("stateDesc");
  fill_states_params_settings(group, type, fieldIdx, stateParams, *stateDescProps);

  // Before fill block settings we need check new and old types, if they different use emptyBlock for fill
  const DataBlock *blocksSettings = &DataBlock::emptyBlock;
  if (DataBlock *settings = find_block_by_name(stateDescProps, tree->getCaption(leaf)))
  {
    fill_states_block_settings(panel, type, *settings, *stateDescProps, statesData);
    AnimStatesType oldType = get_state_desc_cbox_enum_value(settings->getBlockName());
    if (oldType == type)
      blocksSettings = settings;
    fill_values_if_exists(group, settings, stateParams);
  }
  group->createButton(PID_STATES_SETTINGS_SAVE, "Save");
}

void AnimTreePlugin::addStateDescItem(PropPanel::ContainerPropertyControl *panel, AnimStatesType type, const char *name)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  AnimStatesData *data = find_data_by_handle(statesData, leaf);
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;

  TLeafHandle stateDescLeaf = leaf;
  if (data->type != AnimStatesType::STATE_DESC)
    stateDescLeaf = tree->getParentLeaf(leaf);

  addStateDescItem(panel, props, fullPath, stateDescLeaf, type, name);
}

void AnimTreePlugin::addStateDescItem(PropPanel::ContainerPropertyControl *panel, DataBlock &props, String &full_path,
  TLeafHandle state_desc_leaf, AnimStatesType type, const char *name)
{
  if (const char *blockName = get_state_desc_cbox_block_name(type))
  {
    PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
    DataBlock *stateDescProps = props.addBlock("stateDesc");
    TLeafHandle newLeaf = tree->createTreeLeaf(state_desc_leaf, name, get_state_desc_icon(type));
    statesData.emplace_back(AnimStatesData{newLeaf, type});
    DataBlock *settings = stateDescProps->addNewBlock(blockName);
    settings->setStr("name", name);
    saveProps(props, full_path);
    tree->setSelLeaf(newLeaf);
    selectedChangedAnimStatesTree(panel);
  }
}

void AnimTreePlugin::setSelectedStateNodeListSettings(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  AnimStatesData *data = find_data_by_handle(statesData, leaf);
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;

  if (data->type == AnimStatesType::STATE)
  {
    DataBlock *stateProps = props.getBlockByName("stateDesc");
    DataBlock *settings = find_block_by_name(stateProps, tree->getCaption(leaf), /*should_exist*/ false);
    bool isEditable = true;
    if (!settings)
    {
      props = getPropsAnimStates(panel, *data, fullPath, /*only_includes*/ true);
      stateProps = props.getBlockByName("stateDesc");
      settings = find_block_by_name(stateProps, tree->getCaption(leaf));
    }
    if (settings)
    {
      AnimStatesType type = get_state_desc_cbox_enum_value(panel->getInt(PID_STATES_STATE_DESC_TYPE_COMBO_SELECT));
      state_set_selected_node_list_settings(panel, *settings, *stateProps, statesData);
    }
  }
  else if (data->type == AnimStatesType::ENUM_ITEM)
  {
    DataBlock *enumRootProps = props.getBlockByName("initAnimState")->getBlockByName("enum");
    DataBlock *enumProps = enumRootProps->addBlock(tree->getCaption(tree->getParentLeaf(leaf)));
    DataBlock *enumItemProps = enumProps->addBlock(tree->getCaption(leaf));

    int selectedIdx = panel->getInt(PID_STATES_NODES_LIST);
    const bool isControlEnabled = selectedIdx >= 0;
    panel->setText(PID_STATES_ENUM_PARAM_SWITCH_NAME, isControlEnabled ? enumItemProps->getParamName(selectedIdx) : "");
    panel->setEnabledById(PID_STATES_ENUM_PARAM_SWITCH_NAME, isControlEnabled);
    updateEnumItemValueCombo(panel);
    if (panel->getById(PID_STATES_ENUM_PARAM_SWITCH_VALUE))
    {
      panel->setText(PID_STATES_ENUM_PARAM_SWITCH_VALUE, isControlEnabled ? enumItemProps->getStr(selectedIdx) : "");
      panel->setEnabledById(PID_STATES_ENUM_PARAM_SWITCH_VALUE, isControlEnabled);
    }
    else if (panel->getById(PID_STATES_ENUM_ENUM_ITEM_VALUE))
    {
      panel->setText(PID_STATES_ENUM_ENUM_ITEM_VALUE, isControlEnabled ? enumItemProps->getStr(selectedIdx) : "");
      panel->setEnabledById(PID_STATES_ENUM_ENUM_ITEM_VALUE, isControlEnabled);
    }
  }
}

static bool is_chan_exist_in_list(dag::ConstSpan<String> names, const String &channel_name)
{
  for (const String &name : names)
    if (name == channel_name)
      return true;

  return false;
}

void AnimTreePlugin::addNodeStateList(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  AnimStatesData *data = find_data_by_handle(statesData, leaf);
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;

  if (data->type == AnimStatesType::ENUM_ITEM)
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    DataBlock *enumRootProps = props.addBlock("initAnimState")->addBlock("enum");
    DataBlock *enumItemProps = enumRootProps->addBlock(tree->getCaption(parent))->addBlock(tree->getCaption(leaf));
    enumItemProps->addStr("", "");
    int idx = panel->addString(PID_STATES_NODES_LIST, "");
    panel->setInt(PID_STATES_NODES_LIST, idx);
    saveProps(props, fullPath);
  }
  else // AnimStatesType::STATE
  {
    dag::ConstSpan<String> names = panel->getStrings(PID_STATES_NODES_LIST);
    String newChannelName;
    for (const AnimStatesData &data : statesData)
    {
      if (data.type == AnimStatesType::CHAN)
      {
        String channelName = tree->getCaption(data.handle);
        if (!is_chan_exist_in_list(names, channelName))
        {
          newChannelName = channelName;
          break;
        }
      }
    }

    if (newChannelName.empty())
    {
      logerr("Can't add new channel, all channels exist in list");
      return;
    }

    int idx = panel->addString(PID_STATES_NODES_LIST, newChannelName);
    panel->setInt(PID_STATES_NODES_LIST, idx);
    if (names.empty())
      for (int i = PID_STATES_STATE_NODE_CHANNEL; i <= PID_STATES_STATE_NODE_MAX_TIME_SCALE; ++i)
        panel->setEnabledById(i, /*enabled*/ true);

    DataBlock *stateDesc = props.addBlock("stateDesc");
    if (DataBlock *settings = find_block_by_name(stateDesc, tree->getCaption(leaf)))
    {
      DataBlock *channelSettings = settings->addNewBlock(newChannelName);
      channelSettings->addStr("name", "");
      data->childs.emplace_back(AnimStatesData::EMPTY_CHILD_TYPE);
      saveProps(props, fullPath);
    }
  }
  setSelectedStateNodeListSettings(panel);
}

void AnimTreePlugin::removeNodeStateList(PropPanel::ContainerPropertyControl *panel)
{
  const int removeIdx = panel->getInt(PID_STATES_NODES_LIST);
  if (removeIdx < 0)
    return;

  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  AnimStatesData *data = find_data_by_handle(statesData, leaf);
  String fullPath;
  DataBlock props = getPropsAnimStates(panel, *data, fullPath);
  if (fullPath.empty())
    return;

  if (data->type == AnimStatesType::ENUM_ITEM)
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    DataBlock *enumRootProps = props.addBlock("initAnimState")->addBlock("enum");
    const String parentName = tree->getCaption(parent);
    DataBlock *enumProps = enumRootProps->addBlock(parentName.c_str());
    DataBlock *enumItemProps = enumProps->addBlock(tree->getCaption(leaf));
    const SimpleString removedItem = panel->getText(PID_STATES_NODES_LIST);
    if (removedItem == enumItemProps->getParamName(removeIdx))
    {
      enumItemProps->removeParam(removeIdx);
      saveProps(props, fullPath);
      panel->removeString(PID_STATES_NODES_LIST, removeIdx);
      setSelectedStateNodeListSettings(panel);
    }
    else
      logerr("Can't correct remove enum item <%s> child for paramSwitch %s", tree->getCaption(leaf), removedItem);
  }
  else // AnimStatesType::STATE
  {
    DataBlock *stateDesc = props.addBlock("stateDesc");
    if (DataBlock *settings = find_block_by_name(stateDesc, tree->getCaption(leaf)))
    {
      AnimStatesType type = get_state_desc_cbox_enum_value(settings->getBlockName());
      switch (type)
      {
        case AnimStatesType::STATE: state_remove_node_from_list(panel, *settings); break;
        default: break;
      }
      saveProps(props, fullPath);
    }

    const SimpleString childName = panel->getText(PID_STATES_STATE_NODE_NAME);
    if (data != statesData.end() && data->childs.size())
      data->childs.erase(data->childs.begin() + removeIdx);
    else
      logerr("Can't find selected leaf with name <%s> in states data for remove child idx", childName.c_str());
    panel->removeString(PID_STATES_NODES_LIST, removeIdx);
    dag::ConstSpan<String> names = panel->getStrings(PID_STATES_NODES_LIST);
    if (names.empty())
      for (int i = PID_STATES_STATE_NODE_CHANNEL; i <= PID_STATES_STATE_NODE_MAX_TIME_SCALE; ++i)
        panel->setEnabledById(i, /*enabled*/ false);
    setSelectedStateNodeListSettings(panel);
  }
}

void AnimTreePlugin::updateEnumItemValueCombo(PropPanel::ContainerPropertyControl *panel)
{
  const SimpleString selectedParamSwitchName = panel->getText(PID_STATES_ENUM_PARAM_SWITCH_NAME);
  PropPanel::PropertyControlBase *paramSwitchValueControl = panel->getById(PID_STATES_ENUM_PARAM_SWITCH_VALUE);
  PropPanel::PropertyControlBase *enumItemValueControl = panel->getById(PID_STATES_ENUM_ENUM_ITEM_VALUE);
  const bool controlNotCreated = !paramSwitchValueControl && !enumItemValueControl;
  const bool changeToParamSwitchValue = (enumItemValueControl || controlNotCreated) && selectedParamSwitchName != "_use";
  const bool changeToEnumItemValue = (paramSwitchValueControl || controlNotCreated) && selectedParamSwitchName == "_use";
  if (changeToParamSwitchValue || changeToEnumItemValue)
  {
    PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
    PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_STATES_SETTINGS_GROUP)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    AnimStatesData *data = find_data_by_handle(statesData, leaf);
    String fullPath;
    DataBlock props = getPropsAnimStates(panel, *data, fullPath);
    if (fullPath.empty())
      return;

    DataBlock *enumRootProps = props.addBlock("initAnimState")->addBlock("enum");
    DataBlock *enumProps = enumRootProps->addBlock(tree->getCaption(tree->getParentLeaf(leaf)));
    DataBlock *enumItemProps = enumProps->addBlock(tree->getCaption(leaf));
    const int selectedIdx = panel->getInt(PID_STATES_NODES_LIST);
    const bool isActive = selectedIdx >= 0;
    const char *selectionValue = "";
    Tab<String> childValuesNames;
    childValuesNames.emplace_back("##");
    if (changeToEnumItemValue)
    {
      if (!controlNotCreated)
        panel->removeById(PID_STATES_ENUM_PARAM_SWITCH_VALUE);

      // When change control need check can we set old default value
      if (isActive && strcmp(enumItemProps->getParamName(selectedIdx), "_use") == 0)
        selectionValue = enumItemProps->getStr(selectedIdx);
      for (int i = 0; i < enumProps->blockCount(); ++i)
        childValuesNames.emplace_back(enumProps->getBlock(i)->getBlockName());
      group->createCombo(PID_STATES_ENUM_ENUM_ITEM_VALUE, "Enum item value", childValuesNames, selectionValue, isActive);
      group->moveById(PID_STATES_ENUM_ENUM_ITEM_VALUE, PID_STATES_ENUM_PARAM_SWITCH_NAME, /*after*/ true);
    }
    else if (changeToParamSwitchValue)
    {
      if (!controlNotCreated)
        panel->removeById(PID_STATES_ENUM_ENUM_ITEM_VALUE);

      // When change control need check can we set old default value
      if (isActive && strcmp(enumItemProps->getParamName(selectedIdx), "_use") != 0)
        selectionValue = enumItemProps->getStr(selectedIdx);
      fill_child_names(childValuesNames, panel, blendNodesData, controllersData);
      int index = get_selected_name_idx_combo(childValuesNames, selectionValue);
      group->createCombo(PID_STATES_ENUM_PARAM_SWITCH_VALUE, "Child node", childValuesNames, index, isActive);
      group->moveById(PID_STATES_ENUM_PARAM_SWITCH_VALUE, PID_STATES_ENUM_PARAM_SWITCH_NAME, /*after*/ true);
    }
  }
}

void AnimTreePlugin::addNodeToCtrlTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  TLeafHandle root = tree->getRootLeaf();
  TLeafHandle includeLeaf = leaf;
  if (!leaf)
    includeLeaf = root;
  else if (leaf != root)
  {
    AnimCtrlData *selectedData = find_data_by_handle(controllersData, leaf);
    if (selectedData == controllersData.end())
      return;

    if (selectedData->type != ctrl_type_include && selectedData->type != ctrl_type_inner_include)
      includeLeaf = tree->getParentLeaf(leaf);
  }

  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, includeLeaf, fullPath);
  if (fullPath.empty())
    return;

  if (
    DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, includeLeaf))
  {
    TLeafHandle newLeaf = tree->createTreeLeaf(includeLeaf, "new_node", ANIM_BLEND_CTRL_ICON);
    addController(newLeaf, ctrl_type_null);
    DataBlock *settings = ctrlsProps->addNewBlock("null");
    settings->setStr("name", "new_node");
    saveProps(props, fullPath);
    tree->setSelLeaf(newLeaf);
    fillCtrlsSettings(panel);
  }
}

void AnimTreePlugin::addIncludeToCtrlTree(PropPanel::ContainerPropertyControl *panel, int type_pid)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  TLeafHandle root = tree->getRootLeaf();
  if (!leaf)
    leaf = root;
  TLeafHandle includeLeaf = leaf;

  if (leaf != root)
  {
    AnimCtrlData *selectedData = find_data_by_handle(controllersData, leaf);
    if (selectedData == controllersData.end())
      return;

    if (selectedData->type != ctrl_type_include && selectedData->type != ctrl_type_inner_include)
      includeLeaf = tree->getParentLeaf(leaf);
  }

  AnimCtrlData *includeData = find_data_by_handle(controllersData, includeLeaf);

  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, includeLeaf, fullPath);
  if (fullPath.empty())
    return;

  // We wan't create include leaf in inner include
  if (type_pid == PID_ANIM_BLEND_CTRLS_ADD_INCLUDE &&
      (includeLeaf == tree->getRootLeaf() || (includeData != controllersData.end() && includeData->type != ctrl_type_inner_include)))
    addIncludeLeaf(panel, PID_ANIM_BLEND_CTRLS_TREE, includeLeaf, tree->getCaption(includeLeaf));
  else if (type_pid == PID_ANIM_BLEND_CTRLS_ADD_INNER_INCLUDE)
  {
    TLeafHandle ctrlsLeaf = tree->createTreeLeaf(includeLeaf, "new_include", INNER_INCLUDE_ICON);
    addController(ctrlsLeaf, ctrl_type_inner_include);
    tree->setSelLeaf(ctrlsLeaf);
  }
  fillCtrlsSettings(panel);
}

void AnimTreePlugin::removeNodeFromCtrlsTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;

  AnimCtrlData *selectedData = find_data_by_handle(controllersData, leaf);
  if (selectedData == controllersData.end())
    return;

  TLeafHandle parent = tree->getParentLeaf(leaf);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);

  if (selectedData->type == ctrl_type_include)
  {
    const String selName = tree->getCaption(leaf);
    remove_include_tree_node(tree, controllersData, includePaths, props, leaf);
    if (remove_include_leaf_nodes(panel, blendNodesData, selName))
      selectedChangedAnimNodesTree(panel);
    if (remove_include_leaf(panel, PID_NODE_MASKS_TREE, nodeMasksData, selName, is_include_node_mask))
      selectedChangedNodeMasksTree(panel);
    if (remove_include_states_tree(panel, statesData, selName))
      selectedChangedAnimStatesTree(panel);
  }
  else if (selectedData->type == ctrl_type_inner_include)
  {
    AnimCtrlData *parentData = find_data_by_handle(controllersData, parent);
    if ((parentData != controllersData.end() && parentData->type == ctrl_type_include) || parent == tree->getRootLeaf())
    {
      DataBlock *ctrlsBlk = props.addBlock("AnimBlendCtrl");
      remove_include_tree_node(tree, controllersData, includePaths, *ctrlsBlk, leaf);
    }
    else
      remove_include_tree_node(tree, controllersData, includePaths, props, leaf);
  }
  else
  {
    if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
    {
      const String name = tree->getCaption(leaf);
      for (int i = 0; i < ctrlsProps->blockCount(); ++i)
      {
        DataBlock *settings = ctrlsProps->getBlock(i);
        if (name == settings->getStr("name", ""))
        {
          ctrlsProps->removeBlock(i);
          break;
        }
      }
      tree->removeLeaf(leaf);
      if (ctrlsProps->isEmpty())
        props.removeBlock("AnimBlendCtrl");
    }
  }

  controllersData.erase(selectedData);
  saveProps(props, fullPath);
  fillCtrlsSettings(panel);
}

void AnimTreePlugin::changeCtrlType(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  TLeafHandle parent = tree->getParentLeaf(leaf);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);
  if (fullPath.empty())
    return;
  const CtrlType type = static_cast<CtrlType>(panel->getInt(PID_CTRLS_TYPE_COMBO_SELECT));
  const int lastCtrlsPid = !ctrlParams.empty() ? ctrlParams.back().pid : PID_CTRLS_PARAMS_FIELD;
  G_ASSERTF(lastCtrlsPid < PID_CTRLS_PARAMS_FIELD_SIZE, "Previous controller have more params than reserved");
  const bool breakIfNotFound = true;
  remove_fields(panel, PID_CTRLS_PARAMS_FIELD, lastCtrlsPid + 1, !breakIfNotFound);
  remove_fields(panel, PID_CTRLS_ACTION_FIELD + 1, PID_CTRLS_ACTION_FIELD_SIZE, !breakIfNotFound);
  ctrlParams.clear();

  int fieldIdx = PID_CTRLS_PARAMS_FIELD;
  fill_ctrls_params_settings(group, type, fieldIdx, ctrlParams);

  // Before fill block settings we need check new and old types, if they different use emptyBlock for fill
  const DataBlock *blocksSettings = &DataBlock::emptyBlock;
  if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
    if (DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf)))
    {
      CtrlType oldType = static_cast<CtrlType>(lup(settings->getBlockName(), ctrl_type, ctrl_type_not_found));
      if (oldType == type)
        blocksSettings = settings;
      fill_values_if_exists(group, settings, ctrlParams);
    }

  fillCtrlsBlocksSettings(group, type, blocksSettings);
  group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save");
}

void AnimTreePlugin::addMaskToNodeMasksTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  TLeafHandle includeLeaf = get_node_mask_include(nodeMasksData, tree);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, includeLeaf, fullPath);
  if (fullPath.empty())
    return;
  if (create_new_leaf(includeLeaf, tree, panel, NODE_MASK_DEFAULT_NAME, NODE_MASK_ICON, PID_NODE_MASKS_NAME_FIELD))
  {
    nodeMasksData.emplace_back(NodeMaskData{tree->getSelLeaf(), NodeMaskType::MASK});
    DataBlock *node = props.addNewBlock("nodeMask");
    node->setStr("name", NODE_MASK_DEFAULT_NAME);
    saveProps(props, fullPath);
    selectedChangedNodeMasksTree(panel);
  }
}

void AnimTreePlugin::addNodeToNodeMasksTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  TLeafHandle root = tree->getRootLeaf();
  if (!leaf)
    leaf = root;
  TLeafHandle includeLeaf = get_node_mask_include(nodeMasksData, tree);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, includeLeaf, fullPath);
  if (fullPath.empty())
    return;

  const NodeMaskData *maskData = find_data_by_handle(nodeMasksData, leaf);
  if (maskData == nodeMasksData.end() && includeLeaf != root)
  {
    logerr("Selected leaf <%s> not found in node masks data", panel->getCaption(leaf));
    return;
  }

  if (maskData->type == NodeMaskType::INCLUDE || (maskData == nodeMasksData.end() && includeLeaf == root))
    addIncludeLeaf(panel, PID_NODE_MASKS_TREE, includeLeaf, tree->getCaption(includeLeaf));
  else if (maskData->type == NodeMaskType::MASK || maskData->type == NodeMaskType::LEAF)
  {
    TLeafHandle parent = maskData->type == NodeMaskType::MASK ? leaf : tree->getParentLeaf(leaf);
    if (create_new_leaf(parent, tree, panel, NODE_LEAF_DEFAULT_NAME, NODE_MASK_LEAF_ICON, PID_NODE_MASKS_NAME_FIELD))
    {
      nodeMasksData.emplace_back(NodeMaskData{tree->getSelLeaf(), NodeMaskType::LEAF});
      const int nodeMaskNid = props.getNameId("nodeMask");
      const String nodeMaskName = tree->getCaption(parent);
      for (int i = 0; i < props.blockCount(); ++i)
      {
        if (props.getBlock(i)->getNameId() == nodeMaskNid)
        {
          DataBlock *node = props.getBlock(i);
          if (nodeMaskName == node->getStr("name"))
            node->addStr("node", NODE_LEAF_DEFAULT_NAME);
        }
      }
      saveProps(props, fullPath);
    }
  }
  selectedChangedNodeMasksTree(panel);
}

void AnimTreePlugin::removeNodeFromNodeMasksTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf || leaf == tree->getRootLeaf())
    return;

  // if selected leaf is include leaf, then we need parent include for remove selected include from it
  const bool forceParent = true;
  TLeafHandle includeLeaf = get_node_mask_include(nodeMasksData, tree, forceParent);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, includeLeaf, fullPath);
  if (fullPath.empty())
    return;

  const NodeMaskData *maskData = find_data_by_handle(nodeMasksData, leaf);
  if (maskData == nodeMasksData.end())
  {
    logerr("Selected leaf <%s> not found in node masks data", panel->getCaption(leaf));
    return;
  }

  if (maskData->type == NodeMaskType::INCLUDE)
  {
    const String selName = tree->getCaption(leaf);
    remove_include_tree_node(tree, nodeMasksData, includePaths, props, leaf);
    if (remove_include_leaf_nodes(panel, blendNodesData, selName))
      selectedChangedAnimNodesTree(panel);
    if (remove_include_leaf(panel, PID_ANIM_BLEND_CTRLS_TREE, controllersData, selName, is_include_ctrl))
      fillCtrlsSettings(panel);
    if (remove_include_states_tree(panel, statesData, selName))
      selectedChangedAnimStatesTree(panel);
    panel->setText(PID_NODE_MASKS_NAME_FIELD, "");
    panel->setCaption(PID_NODE_MASKS_NAME_FIELD, "Field name");
    panel->removeById(PID_NODE_MASKS_PATH_FIELD);
  }
  else if (maskData->type == NodeMaskType::MASK || maskData->type == NodeMaskType::LEAF)
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    TLeafHandle nodeMaskLeaf = maskData->type == NodeMaskType::LEAF ? parent : leaf;
    const int nodeMaskNid = props.getNameId("nodeMask");
    const String nodeMaskName = tree->getCaption(nodeMaskLeaf);
    for (int i = 0; i < props.blockCount(); ++i)
    {
      if (props.getBlock(i)->getNameId() == nodeMaskNid)
      {
        DataBlock *node = props.getBlock(i);
        if (nodeMaskName == node->getStr("name"))
        {
          if (leaf != nodeMaskLeaf)
          {
            const String leafName = tree->getCaption(leaf);
            const int nodeNid = node->getNameId("node");
            for (int j = 0; j < node->paramCount(); ++j)
              if (nodeNid == node->getParamNameId(j) && leafName == node->getStr(j))
                node->removeParam(j);
          }
          else
            props.removeBlock(i);
        }
      }
    }

    remove_childs_data_by_leaf(tree, nodeMasksData, leaf);
    tree->removeLeaf(leaf);
    panel->setText(PID_NODE_MASKS_NAME_FIELD, "");
  }

  nodeMasksData.erase(maskData);
  saveProps(props, fullPath);
  selectedChangedNodeMasksTree(panel);
}

void AnimTreePlugin::saveSettingsNodeMasksTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  TLeafHandle root = tree->getRootLeaf();
  if (!leaf || leaf == root)
    return;

  // if selected leaf is include leaf, then we need parent include for save selected include in it
  const bool forceParent = true;
  TLeafHandle includeLeaf = get_node_mask_include(nodeMasksData, tree, forceParent);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, includeLeaf, fullPath);
  if (fullPath.empty())
    return;
  const NodeMaskData *maskData = find_data_by_handle(nodeMasksData, leaf);
  if (maskData == nodeMasksData.end())
  {
    logerr("Selected leaf <%s> not found in node masks data", panel->getCaption(leaf));
    return;
  }

  if (maskData->type == NodeMaskType::INCLUDE)
  {
    const SimpleString fileName = panel->getText(PID_NODE_MASKS_NAME_FIELD);
    const SimpleString newPath = panel->getText(PID_NODE_MASKS_PATH_FIELD);
    if (!checkIncludeBeforeSave(tree, leaf, fileName, newPath, props))
      return;

    String newFullPath = String(0, "%s%s.blk", newPath.c_str(), fileName.c_str());
    const String selName = tree->getCaption(leaf);
    auto [nodeLeaf, updateSelectedNodes] = change_include_leaf_nodes(panel, blendNodesData, selName, fileName.c_str());
    change_include_leaf(tree, nodeMasksData, fileName.c_str(), leaf);
    auto [ctrlLeaf, updateSelectedCtrls] =
      change_include_leaf(panel, controllersData, selName, fileName.c_str(), PID_ANIM_BLEND_CTRLS_TREE, is_include_ctrl);
    bool updateSelectedStates = remove_include_states_tree(panel, statesData, selName);
    saveIncludeChange(panel, tree, leaf, props, selName, newFullPath, {nodeLeaf, leaf, ctrlLeaf});
    if (updateSelectedNodes)
      selectedChangedAnimNodesTree(panel);
    if (updateSelectedCtrls)
      fillCtrlsSettings(panel);
    if (updateSelectedStates)
      selectedChangedAnimStatesTree(panel);
  }
  else if (maskData->type == NodeMaskType::MASK || maskData->type == NodeMaskType::LEAF)
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    SimpleString name = panel->getText(PID_NODE_MASKS_NAME_FIELD);
    if (TLeafHandle handle = is_name_exists_in_childs(parent, tree, name))
    {
      if (handle != leaf)
        logerr("Name \"%s\" is not unique, can't save", name);
      return;
    }

    TLeafHandle nodeMaskLeaf = maskData->type == NodeMaskType::LEAF ? parent : leaf;
    const int nodeMaskNid = props.getNameId("nodeMask");
    const String nodeMaskName = tree->getCaption(nodeMaskLeaf);
    for (int i = 0; i < props.blockCount(); ++i)
    {
      if (props.getBlock(i)->getNameId() == nodeMaskNid)
      {
        DataBlock *node = props.getBlock(i);
        if (nodeMaskName == node->getStr("name"))
        {
          if (leaf != nodeMaskLeaf)
          {
            const String leafName = tree->getCaption(leaf);
            const int nodeNid = node->getNameId("node");
            for (int j = 0; j < node->paramCount(); ++j)
              if (nodeNid == node->getParamNameId(j) && leafName == node->getStr(j))
                node->setStr(j, name.c_str());
          }
          else
            node->setStr("name", name.c_str());
        }
      }
    }
    tree->setCaption(leaf, name.c_str());
  }

  saveProps(props, fullPath);
}

static bool try_find_mask_node_with_override(const String &path, const String &name)
{
  const bool curParseIncludesAsParams = DataBlock::parseIncludesAsParams;
  DataBlock::parseIncludesAsParams = true;
  DataBlock props(path);
  DataBlock::parseIncludesAsParams = curParseIncludesAsParams;
  const int nodeMaskNid = props.getNameId("nodeMask");
  for (int i = 0; i < props.blockCount(); ++i)
  {
    const DataBlock *node = props.getBlock(i);
    if (node->getNameId() == nodeMaskNid && node->getStr("name") == name)
      return true;
  }
  return false;
}

void AnimTreePlugin::selectedChangedNodeMasksTree(PropPanel::ContainerPropertyControl *panel)
{
  // Clear fields by default
  panel->setText(PID_NODE_MASKS_NAME_FIELD, "");
  panel->setCaption(PID_NODE_MASKS_NAME_FIELD, "Field name");
  panel->removeById(PID_NODE_MASKS_PATH_FIELD);

  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_NODE_MASKS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_NODE_MASKS_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (leaf == tree->getRootLeaf())
    panel->setCaption(PID_NODE_MASKS_ADD_NODE, "Add include");
  if (!leaf || leaf == tree->getRootLeaf())
    return;

  NodeMaskData *selectedData = find_data_by_handle(nodeMasksData, leaf);
  if (selectedData == nodeMasksData.end())
    return;

  if (selectedData->type == NodeMaskType::INCLUDE)
  {
    String selName = tree->getCaption(leaf);
    group->setCaption(PID_NODE_MASKS_NAME_FIELD, "file name");
    group->setText(PID_NODE_MASKS_NAME_FIELD, selName.c_str());

    const String *path = eastl::find_if(includePaths.begin(), includePaths.end(), [&selName](const String &value) {
      char fileNameBuf[DAGOR_MAX_PATH];
      return selName == dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), value.c_str());
    });
    char fileLocationBuf[DAGOR_MAX_PATH];
    String pathWithoutName = (path != includePaths.end() && !path->empty())
                               ? String(dd_get_fname_location(fileLocationBuf, path->c_str()))
                               : String("#/example/path/for/include/");
    group->createEditBox(PID_NODE_MASKS_PATH_FIELD, "folder path", pathWithoutName);
    group->moveById(PID_NODE_MASKS_PATH_FIELD, PID_NODE_MASKS_SAVE);
    panel->setCaption(PID_NODE_MASKS_ADD_NODE, "Add include");
  }
  else if (selectedData->type == NodeMaskType::MASK || selectedData->type == NodeMaskType::LEAF)
  {
    bool isEditEnabled = false;
    TLeafHandle nodeMaskLeaf = selectedData->type == NodeMaskType::MASK ? leaf : tree->getParentLeaf(leaf);
    TLeafHandle includeLeaf = get_node_mask_include(nodeMasksData, tree);
    String fullPath;
    DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, includeLeaf, fullPath);
    const String nodeMaskName = tree->getCaption(nodeMaskLeaf);
    const int nodeMaskNid = props.getNameId("nodeMask");
    for (int i = 0; i < props.blockCount(); ++i)
    {
      const DataBlock *node = props.getBlock(i);
      if (node->getNameId() == nodeMaskNid && node->getStr("name") == nodeMaskName)
        isEditEnabled = true;
    }

    if (!isEditEnabled)
    {
      if (try_find_mask_node_with_override(fullPath, nodeMaskName))
        logerr("Node <%s> overrided in blk, can't edit this node. Edit it manual in %s", tree->getCaption(leaf), fullPath.c_str());
      else
        logerr("Can't find node %s in blk", tree->getCaption(leaf));
    }

    group->setText(PID_NODE_MASKS_NAME_FIELD, tree->getCaption(leaf));
    panel->setCaption(PID_NODE_MASKS_ADD_NODE, "Add node");
    panel->setEnabledById(PID_NODE_MASKS_ADD_NODE, isEditEnabled);
    panel->setEnabledById(PID_NODE_MASKS_SAVE, isEditEnabled);
  }
}

void AnimTreePlugin::createNodeAnimNodesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  String fullPath;
  DataBlock props = get_props_from_include_leaf_blend_node(includePaths, *curAsset, tree, fullPath);
  if (fullPath.empty())
    return;

  TLeafHandle includeLeaf = get_anim_blend_node_include(tree);
  if (!includeLeaf)
    return;

  TLeafHandle leaf = tree->createTreeLeaf(includeLeaf, "new_node", BLEND_NODE_SINGLE_ICON);
  tree->setUserData(leaf, (void *)&ANIM_BLEND_NODE_LEAF);
  addBlendNode(leaf, BlendNodeType::SINGLE);
  DataBlock *settings = props.addNewBlock("AnimBlendNodeLeaf")->addBlock("single");
  settings->setStr("name", "new_node");
  tree->setSelLeaf(leaf);
  saveProps(props, fullPath);
  selectedChangedAnimNodesTree(panel);
}

void AnimTreePlugin::createLeafAnimNodesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  TLeafHandle selLeaf = tree->getSelLeaf();
  if (!selLeaf)
    selLeaf = tree->getRootLeaf();

  String fullPath;
  DataBlock props = get_props_from_include_leaf_blend_node(includePaths, *curAsset, tree, fullPath);
  if (fullPath.empty())
    return;

  if (tree->getUserData(selLeaf) == &ANIM_BLEND_NODE_LEAF || tree->getUserData(selLeaf) == &ANIM_BLEND_NODE_A2D)
    addLeafToBlendNodesTree(tree, props);
  else if (tree->getUserData(selLeaf) == &ANIM_BLEND_NODE_INCLUDE || tree->getRootLeaf() == selLeaf)
    addIncludeLeaf(panel, PID_ANIM_BLEND_NODES_TREE, selLeaf, tree->getCaption(selLeaf));

  saveProps(props, fullPath);
  selectedChangedAnimNodesTree(panel);
}

void AnimTreePlugin::addIrqNodeToAnimNodesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_NODES_SETTINGS_GROUP)->getContainer();
  String fullPath;
  DataBlock props = get_props_from_include_leaf_blend_node(includePaths, *curAsset, tree, fullPath);
  if (fullPath.empty())
    return;
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf || tree->getUserData(leaf) != &ANIM_BLEND_NODE_LEAF)
    return;

  const char *leafName = "irq_name";
  TLeafHandle irqLeaf = tree->createTreeLeaf(leaf, leafName, IRQ_ICON);
  addBlendNode(irqLeaf, BlendNodeType::IRQ);
  tree->setUserData(irqLeaf, (void *)&ANIM_BLEND_NODE_IRQ);
  tree->setSelLeaf(irqLeaf);
  fill_irq_settings(group, leafName, IRQ_TYPE_KEY);
  panel->setEnabledById(PID_ANIM_BLEND_NODES_ADD_IRQ, /*enabled*/ false);
  String bnlName = tree->getCaption(leaf);
  DataBlock *settings = find_blend_node_settings(props, bnlName.c_str());
  if (!settings)
  {
    logerr("Can't find animBlendNodeLeaf props with name %s", bnlName.c_str());
    return;
  }

  DataBlock *irqProps = settings->addNewBlock("irq");
  irqProps->addStr(irq_types[IRQ_TYPE_KEY], "");
  irqProps->addStr("name", leafName);
  saveProps(props, fullPath);
}

void AnimTreePlugin::removeBlendNodeOrA2d(PropPanel::ContainerPropertyControl *tree, TLeafHandle leaf, DataBlock *props)
{
  int a2dIdx = -1;
  TLeafHandle nodeLeaf = get_anim_blend_node_leaf(tree);
  if (!nodeLeaf)
    return;

  DataBlock *settings = get_selected_bnl_settings(*props, tree->getCaption(nodeLeaf), a2dIdx);
  DataBlock *a2dBlk = props->getBlock(a2dIdx);

  TLeafHandle parent = tree->getParentLeaf(leaf);
  // First case is selected a2d node or it's collapsed a2d node with only 1 child
  if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_A2D ||
      (tree->getUserData(leaf) == &ANIM_BLEND_NODE_LEAF &&
        (parent == tree->getRootLeaf() || tree->getUserData(parent) == &ANIM_BLEND_NODE_INCLUDE)))
  {
    props->removeBlock(a2dIdx);
    remove_childs_data_by_leaf(tree, blendNodesData, leaf);
    tree->removeLeaf(leaf);
  }
  else if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_LEAF)
  {
    TLeafHandle a2dLeaf = tree->getParentLeaf(leaf);
    // In case when a2d node have only 1 child we display only anim blend node in tree
    // And if childs 2 or more display a2d node with anim blend node leafs
    if (tree->getChildCount(a2dLeaf) > 2)
    {
      for (int i = 0; i < tree->getChildCount(a2dLeaf); ++i)
        if (tree->getChildLeaf(a2dLeaf, i) == leaf)
          a2dBlk->removeBlock(i);

      remove_childs_data_by_leaf(tree, blendNodesData, leaf);
      tree->removeLeaf(leaf);
    }
    else if (tree->getChildCount(a2dLeaf) == 2)
    {
      // In this case we need remove selected node and collapse a2d node to just anim blend node
      const int selectedIdx = tree->getChildLeaf(a2dLeaf, 0) == leaf ? 0 : 1;
      TLeafHandle anotherLeaf = tree->getChildLeaf(a2dLeaf, selectedIdx == 0 ? 1 : 0);
      const String anotherLeafName = tree->getCaption(anotherLeaf);
      const DataBlock *anotherLeafSettings = get_selected_bnl_settings(*props, anotherLeafName.c_str(), a2dIdx);
      tree->setCaption(a2dLeaf, anotherLeafName.c_str());
      tree->setUserData(a2dLeaf, (void *)&ANIM_BLEND_NODE_LEAF);
      BlendNodeData *a2dData = find_data_by_handle(blendNodesData, a2dLeaf);
      BlendNodeType a2dLeafType = BlendNodeType::UNSET;
      if (anotherLeafSettings)
        a2dLeafType = static_cast<BlendNodeType>(lup(anotherLeafSettings->getBlockName(), blend_node_types));
      if (a2dData != blendNodesData.end())
        a2dData->type = a2dLeafType;
      else
        addBlendNode(a2dLeaf, a2dLeafType);
      tree->setButtonPictures(a2dLeaf, get_blend_node_icon_name(a2dLeafType));

      // this remove data for leaf and anotherLeaf and they childs
      remove_childs_data_by_leaf(tree, blendNodesData, a2dLeaf);
      tree->removeLeaf(leaf);
      tree->removeLeaf(anotherLeaf);
      a2dBlk->removeBlock(selectedIdx);
      DataBlock *anotherSettings = a2dBlk->getBlock(0);

      for (int i = 0; i < anotherSettings->blockCount(); ++i)
      {
        TLeafHandle irqLeaf = tree->createTreeLeaf(a2dLeaf, anotherSettings->getBlock(i)->getStr("name"), IRQ_ICON);
        addBlendNode(irqLeaf, BlendNodeType::IRQ);
        tree->setUserData(irqLeaf, (void *)&ANIM_BLEND_NODE_IRQ);
      }
    }
  }
}

void AnimTreePlugin::removeNodeFromAnimNodesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  String fullPath;
  // if selected leaf is include leaf, then we need parent include for remove selected include from it
  const bool forceParent = true;
  DataBlock props = get_props_from_include_leaf_blend_node(includePaths, *curAsset, tree, fullPath, forceParent);
  if (fullPath.empty())
    return;

  TLeafHandle leaf = tree->getSelLeaf();
  BlendNodeData *removeData = find_data_by_handle(blendNodesData, leaf);
  if (removeData != blendNodesData.end())
    blendNodesData.erase(removeData);

  if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_IRQ)
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    String bnlName = tree->getCaption(parent);
    DataBlock *settings = find_blend_node_settings(props, bnlName.c_str());
    if (!settings)
    {
      logerr("Can't find animBlendNodeLeaf props with name %s", bnlName.c_str());
      return;
    }
    settings->removeBlock(get_irq_idx(tree, leaf));
    remove_nodes_fields(panel);
    tree->removeLeaf(leaf);
  }
  else if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_A2D || tree->getUserData(leaf) == &ANIM_BLEND_NODE_LEAF)
  {
    removeBlendNodeOrA2d(tree, leaf, &props);
    remove_nodes_fields(panel);
    animBnlParams.clear();
  }
  else if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_INCLUDE)
  {
    const String selName = tree->getCaption(leaf);
    remove_childs_data_by_leaf(tree, blendNodesData, leaf);
    remove_include_node(tree, includePaths, props, leaf);
    if (remove_include_leaf(panel, PID_NODE_MASKS_TREE, nodeMasksData, selName, is_include_node_mask))
      selectedChangedNodeMasksTree(panel);
    if (remove_include_leaf(panel, PID_ANIM_BLEND_CTRLS_TREE, controllersData, selName, is_include_ctrl))
      fillCtrlsSettings(panel);
    if (remove_include_states_tree(panel, statesData, selName))
      selectedChangedAnimStatesTree(panel);
    remove_nodes_fields(panel);
    animBnlParams.clear();
  }

  saveProps(props, fullPath);
  selectedChangedAnimNodesTree(panel);
}

static void save_irq(DataBlock *settings, PropPanel::ContainerPropertyControl *panel)
{
  const IrqType selectedType = static_cast<IrqType>(panel->getInt(PID_IRQ_TYPE_COMBO_SELECT));
  if (selectedType == IRQ_TYPE_KEY)
  {
    SimpleString key = panel->getText(PID_IRQ_INTERRUPTION_FIELD);
    if (!key.empty())
    {
      for (const String &typeParam : irq_types)
        settings->removeParam(typeParam.c_str());
      settings->addStr(irq_types[selectedType], key.c_str());
    }
    else
      logerr("Irq key can't be empty");
  }
  else if (selectedType == IRQ_TYPE_REL_POS || selectedType == IRQ_TYPE_KEY_FLOAT)
  {
    for (const String &typeParam : irq_types)
      settings->removeParam(typeParam.c_str());
    const float floatParam = panel->getFloat(PID_IRQ_INTERRUPTION_FIELD);
    settings->addReal(irq_types[selectedType], floatParam);
  }

  SimpleString irqName = panel->getText(PID_IRQ_NAME_FIELD);
  if (!irqName.empty())
  {
    PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    tree->setCaption(leaf, irqName.c_str());
    settings->setStr("name", irqName.c_str());
  }
  else
    logerr("New irq name can't be empty");
}

void AnimTreePlugin::saveSettingsAnimNodesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  String fullPath;
  // if selected leaf is include leaf, then we need parent include for save selected include in it
  const bool forceParent = true;
  DataBlock props = get_props_from_include_leaf_blend_node(includePaths, *curAsset, tree, fullPath, forceParent);
  if (fullPath.empty())
    return;

  TLeafHandle leaf = tree->getSelLeaf();
  if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_IRQ)
  {
    TLeafHandle parent = tree->getParentLeaf(leaf);
    String bnlName = tree->getCaption(parent);
    DataBlock *settings = find_blend_node_settings(props, bnlName.c_str());
    if (!settings)
    {
      logerr("Can't find animBlendNodeLeaf props with name %s", bnlName.c_str());
      return;
    }
    DataBlock *irqSettings = settings->getBlock(get_irq_idx(tree, leaf));
    save_irq(irqSettings, panel);
  }
  else if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_LEAF || tree->getUserData(leaf) == &ANIM_BLEND_NODE_A2D)
  {
    int a2dIdx = -1;
    TLeafHandle nodeLeaf = get_anim_blend_node_leaf(tree);
    if (!nodeLeaf)
      return;

    DataBlock *settings = get_selected_bnl_settings(props, tree->getCaption(nodeLeaf), a2dIdx);
    DataBlock *a2dBlk = props.getBlock(a2dIdx);
    if (!settings)
    {
      logerr("Not found node with name %s", tree->getCaption(nodeLeaf));
      return;
    }

    // Make copy, becuse we can don't save optinal fields if it's node leaf
    dag::Vector<AnimParamData> params = animBnlParams;
    remove_param_if_default_str(params, panel, "accept_name_mask_re");
    remove_param_if_default_str(params, panel, "decline_name_mask_re");
    if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_LEAF)
    {
      BlendNodeType type = static_cast<BlendNodeType>(panel->getInt(PID_NODES_SETTINGS_NODE_TYPE_COMBO_SELECT));
      DataBlock fullProps(curAsset->getSrcFilePath());
      const bool defaultForeign = fullProps.getBool("defaultForeignAnim", false);
      switch (type)
      {
        case BlendNodeType::SINGLE: single_prepare_params(params, panel, defaultForeign, curAsset->getMgr()); break;
        case BlendNodeType::CONTINUOUS: continuous_prepare_params(params, panel, defaultForeign, curAsset->getMgr()); break;
        case BlendNodeType::STILL: still_prepare_params(params, panel, defaultForeign); break;
        case BlendNodeType::PARAMETRIC: parametric_prepare_params(params, panel, defaultForeign); break;

        default: break;
      }
    }

    auto separator =
      eastl::find_if(params.begin(), params.end(), [](const AnimParamData &value) { return value.type == DataBlock::TYPE_NONE; });
    if (separator != params.end())
    {
      // Split params based on separator in params, separator not included
      dag::ConstSpan<AnimParamData> a2dParams = make_span_const(params.begin(), eastl::distance(params.begin(), separator));
      dag::ConstSpan<AnimParamData> settingsParams = make_span_const(separator + 1, eastl::distance(separator + 1, params.end()));
      save_params_blk(a2dBlk, a2dParams, panel);
      save_params_blk(settings, settingsParams, panel);
      SimpleString newTypeName = panel->getText(PID_NODES_SETTINGS_NODE_TYPE_COMBO_SELECT);
      settings->changeBlockName(newTypeName.c_str());
      BlendNodeType type = static_cast<BlendNodeType>(lup(newTypeName.c_str(), blend_node_types));
      tree->setButtonPictures(leaf, get_blend_node_icon_name(type));
      BlendNodeData *data = find_data_by_handle(blendNodesData, leaf);
      const String oldName = tree->getCaption(leaf);
      if (data != blendNodesData.end())
        data->type = type;
      else
        logerr("Can't find and update data for blend node with name: %s", oldName);

      const char *newName = settings->getStr("name");
      if (oldName != newName)
      {
        DependentNodesResult result = checkDependentNodes(panel, data->id, newName, oldName);
        switch (result.dialogResult)
        {
          case PropPanel::DIALOG_ID_YES:
            // Save base props now, becuse we can rewrite blk file after update dependent childs
            saveProps(props, fullPath);
            proccesDependentNodes(panel, newName, oldName, result);
            // Change caption before update selected, becuse we can use caption in combobox for nodes
            tree->setCaption(leaf, newName);
            // If update child name in selected dependent controller or state we need refill prop panel with updated child name
            updateSelectedDependentCtrl(panel, result.dependentCtrls);
            updateSelectedDependentState(panel, result.dependentStates);
            break;
          case PropPanel::DIALOG_ID_NO: data->id = curCtrlAndBlendNodeUniqueIdx++; break;
          case PropPanel::DIALOG_ID_CANCEL: return;
          default: break;
        }

        // Early return for skip rewrite props with old values in saveProps
        if (result.dialogResult == PropPanel::DIALOG_ID_YES)
          return;
        else
          tree->setCaption(leaf, newName);
      }
    }
    else
    {
      save_params_blk(a2dBlk, make_span_const(params), panel);
      const auto a2dParam = find_param_by_name(params, "a2d");
      if (a2dParam != params.end())
        tree->setCaption(leaf, panel->getText(a2dParam->pid));
    }
    animPlayer.setSelectedA2dName(panel->getText(PID_NODES_PARAMS_FIELD));
    animPlayer.loadA2dResource(panel, animBnlParams, leaf);
    update_play_animation_text(panel, animPlayer.isAnimInProgress());
  }
  else if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_INCLUDE)
  {
    const SimpleString fileName = get_str_param_by_name_optional(animBnlParams, panel, "file name");
    const SimpleString newPath = get_str_param_by_name_optional(animBnlParams, panel, "folder path");
    if (!checkIncludeBeforeSave(tree, leaf, fileName, newPath, props))
      return;

    String newFullPath = String(0, "%s%s.blk", newPath.c_str(), fileName.c_str());
    const String selName = tree->getCaption(leaf);
    change_include_leaf(tree, blendNodesData, fileName.c_str(), leaf);
    auto [maskLeaf, updateSelectedMasks] =
      change_include_leaf(panel, nodeMasksData, selName, fileName.c_str(), PID_NODE_MASKS_TREE, is_include_node_mask);
    auto [ctrlLeaf, updateSelectedCtrls] =
      change_include_leaf(panel, controllersData, selName, fileName.c_str(), PID_ANIM_BLEND_CTRLS_TREE, is_include_ctrl);
    bool updateSelectedStates = remove_include_states_tree(panel, statesData, selName);
    saveIncludeChange(panel, tree, leaf, props, selName, newFullPath, {leaf, maskLeaf, ctrlLeaf});
    if (updateSelectedMasks)
      selectedChangedNodeMasksTree(panel);
    if (updateSelectedCtrls)
      fillCtrlsSettings(panel);
    if (updateSelectedStates)
      selectedChangedAnimStatesTree(panel);
  }

  saveProps(props, fullPath);
}

void AnimTreePlugin::selectedChangedAnimNodesTree(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_NODES_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (leaf == tree->getRootLeaf() || tree->getUserData(leaf) == &ANIM_BLEND_NODE_INCLUDE)
    panel->setCaption(PID_ANIM_BLEND_NODES_ADD_LEAF, "Add include");
  else
    panel->setCaption(PID_ANIM_BLEND_NODES_ADD_LEAF, "Add leaf");
  const bool isSelectedLeafValid = leaf && leaf != tree->getRootLeaf();
  const bool isAnimLeaf = isSelectedLeafValid && (tree->getUserData(leaf) == &ANIM_BLEND_NODE_LEAF);
  panel->setEnabledById(PID_ANIM_BLEND_NODES_ADD_IRQ, isAnimLeaf);
  fillAnimBlendSettings(tree, group);
  if (!isSelectedLeafValid)
    return;

  animPlayer.loadA2dResource(panel, animBnlParams, leaf);
  update_play_animation_text(panel, animPlayer.isAnimInProgress());
}

void AnimTreePlugin::irqTypeSelected(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_NODES_SETTINGS_GROUP)->getContainer();
  const IrqType selectedType = static_cast<IrqType>(group->getInt(PID_IRQ_TYPE_COMBO_SELECT));
  const SimpleString irqName = group->getText(PID_IRQ_NAME_FIELD);
  fill_irq_settings(group, irqName.c_str(), selectedType);
}

void AnimTreePlugin::changeAnimNodeType(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_NODES_SETTINGS_GROUP)->getContainer();
  String fullPath;
  DataBlock props = get_props_from_include_leaf_blend_node(includePaths, *curAsset, tree, fullPath);
  if (fullPath.empty())
    return;
  const BlendNodeType type = static_cast<BlendNodeType>(panel->getInt(PID_NODES_SETTINGS_NODE_TYPE_COMBO_SELECT));
  auto separator = eastl::find_if(animBnlParams.begin(), animBnlParams.end(),
    [](const AnimParamData &value) { return value.type == DataBlock::TYPE_NONE; });

  // We remove all params after separator
  animBnlParams.erase(separator + 1, animBnlParams.end());
  const bool breakIfNotFound = true;
  remove_fields(panel, separator->pid + 1, PID_NODES_PARAMS_FIELD_SIZE, breakIfNotFound);
  DataBlock fullProps(curAsset->getSrcFilePath());
  const bool defaultForeign = fullProps.getBool("defaultForeignAnim", false);
  fill_anim_blend_params_settings(type, group, animBnlParams, separator->pid + 1, defaultForeign);
  TLeafHandle leaf = tree->getSelLeaf();
  if (const DataBlock *settings = find_blend_node_settings(props, tree->getCaption(leaf)))
    fill_values_if_exists(group, settings, animBnlParams);
  set_anim_blend_dependent_defaults(type, group, animBnlParams, curAsset->getMgr());
  group->moveById(PID_NODES_SETTINGS_PLAY_ANIMATION);
  group->moveById(PID_NODES_SETTINGS_SAVE);
}

static void update_anim_node_dependent_field(PropPanel::ContainerPropertyControl *panel, AnimTreeAnimationPlayer &player,
  dag::Vector<AnimParamData> &params, AnimParamData &changed_param, const DagorAssetMgr &mgr);

static void update_dependent_text_field(PropPanel::ContainerPropertyControl *panel, AnimTreeAnimationPlayer &player,
  dag::Vector<AnimParamData> &params, const char *changed_field, const char *text, const DagorAssetMgr &mgr)
{
  AnimParamData *param = find_param_by_name(params, changed_field);
  if (param != params.end() && param->dependent)
  {
    panel->setText(param->pid, text);
    update_anim_node_dependent_field(panel, player, params, *param, mgr);
    player.animNodesFieldChanged(panel, params, *param);
  }
}

static void update_dependent_float_field(PropPanel::ContainerPropertyControl *panel, AnimTreeAnimationPlayer &player,
  dag::Vector<AnimParamData> &params, const char *changed_field, float value, const DagorAssetMgr &mgr)
{
  AnimParamData *param = find_param_by_name(params, changed_field);
  if (param != params.end() && param->dependent)
  {
    panel->setFloat(param->pid, value);
    update_anim_node_dependent_field(panel, player, params, *param, mgr);
    player.animNodesFieldChanged(panel, params, *param);
  }
}

static void update_anim_node_dependent_field(PropPanel::ContainerPropertyControl *panel, AnimTreeAnimationPlayer &player,
  dag::Vector<AnimParamData> &params, AnimParamData &changed_param, const DagorAssetMgr &mgr)
{
  if (changed_param.name == "name")
    update_dependent_text_field(panel, player, params, "key", panel->getText(changed_param.pid), mgr);
  else if (changed_param.name == "key")
  {
    const SimpleString keyValue = panel->getText(changed_param.pid);
    update_dependent_text_field(panel, player, params, "key_start", String(0, "%s_start", keyValue.c_str()), mgr);
    update_dependent_text_field(panel, player, params, "key_end", String(0, "%s_end", keyValue.c_str()), mgr);

    const AnimParamData *nameParam = find_param_by_name(params, "name");
    if (nameParam != params.end())
      changed_param.dependent = keyValue == panel->getText(nameParam->pid);
  }
  else if (changed_param.name == "key_start")
  {
    update_dependent_text_field(panel, player, params, "sync_time", panel->getText(changed_param.pid), mgr);
    const AnimParamData *keyParam = find_param_by_name(params, "key");
    if (keyParam != params.end())
    {
      const SimpleString keyValue = panel->getText(keyParam->pid);
      const String keyStartDefault = String(0, "%s_start", keyValue.c_str());
      changed_param.dependent = panel->getText(changed_param.pid) == keyStartDefault.c_str();
    }
  }
  else if (changed_param.name == "key_end")
  {
    const AnimParamData *keyParam = find_param_by_name(params, "key");
    if (keyParam != params.end())
    {
      const SimpleString keyValue = panel->getText(keyParam->pid);
      const String keyEndDefault = String(0, "%s_end", keyValue.c_str());
      changed_param.dependent = panel->getText(changed_param.pid) == keyEndDefault.c_str();
    }
  }
  else if (changed_param.name == "sync_time")
  {
    const AnimParamData *keyStartParam = find_param_by_name(params, "key_start");
    if (keyStartParam != params.end())
      changed_param.dependent = panel->getText(changed_param.pid) == panel->getText(keyStartParam->pid);
  }
  else if (changed_param.name == "a2d")
    update_dependent_float_field(panel, player, params, "time", get_default_anim_time(panel->getText(changed_param.pid), mgr), mgr);
  else if (changed_param.name == "time")
  {
    const AnimParamData *a2dParam = find_param_by_name(params, "a2d");
    if (a2dParam != params.end())
      changed_param.dependent =
        is_equal_float(panel->getFloat(changed_param.pid), get_default_anim_time(panel->getText(a2dParam->pid), mgr));
  }
}

void AnimTreePlugin::updateAnimNodeFields(PropPanel::ContainerPropertyControl *panel, int pid)
{
  AnimParamData *param =
    eastl::find_if(animBnlParams.begin(), animBnlParams.end(), [pid](const AnimParamData &value) { return value.pid == pid; });
  if (param == animBnlParams.end())
  {
    logerr("Param with pid: %d not found", pid);
    return;
  }
  update_anim_node_dependent_field(panel, animPlayer, animBnlParams, *param, curAsset->getMgr());
  animPlayer.animNodesFieldChanged(panel, animBnlParams, *param);
}

void AnimTreePlugin::selectDynModel(PropPanel::ContainerPropertyControl *panel)
{
  String selectMsg(64, "Select dynModel");
  String resetMsg(64, "Reset dynModel");

  const int dynModelId = animPlayer.getDynModelId();
  SelectAssetDlg dlg(0, &curAsset->getMgr(), selectMsg, selectMsg, resetMsg, make_span_const(&dynModelId, 1));
  dlg.setManualModalSizingEnabled();
  if (!dlg.hasEverBeenShown())
    dlg.positionLeftToWindow("Properties", true);

  int result = dlg.showDialog();
  if (result == PropPanel::DIALOG_ID_CLOSE)
    return;

  String modelName(result == PropPanel::DIALOG_ID_OK ? dlg.getSelObjName() : "");
  animPlayer.setDynModel(modelName.c_str(), *curAsset);
  panel->setCaption(PID_SELECT_DYN_MODEL, modelName.empty() ? "Select model" : modelName.c_str());
}

void AnimTreePlugin::validateDependentNodes(PropPanel::ContainerPropertyControl *panel)
{
  // Clear filters becuse fillCtrlsChilds and fillStatesChilds use getChildLeaf based on actual tree view
  SimpleString ctrlFitler = panel->getText(PID_ANIM_BLEND_CTRLS_FILTER);
  SimpleString stateFilter = panel->getText(PID_ANIM_STATES_FILTER);
  if (!ctrlFitler.empty())
  {
    panel->setText(PID_ANIM_BLEND_CTRLS_FILTER, "");
    panel->setText(PID_ANIM_BLEND_CTRLS_TREE, "");
  }
  if (!stateFilter.empty())
  {
    panel->setText(PID_ANIM_STATES_FILTER, "");
    panel->setText(PID_ANIM_STATES_TREE, "");
  }

  for (AnimStatesData &data : statesData)
    data.childs.clear();
  for (AnimCtrlData &data : controllersData)
    data.childs.clear();

  fillCtrlsChilds(panel);
  fillStatesChilds(panel);

  bool hasNotFoundNodes = false;
  for (const AnimStatesData &data : statesData)
    for (int idx : data.childs)
      hasNotFoundNodes |= (idx == AnimStatesData::NOT_FOUND_CHILD);

  for (const AnimCtrlData &data : controllersData)
    for (int idx : data.childs)
      hasNotFoundNodes |= (idx == AnimCtrlData::NOT_FOUND_CHILD || idx == AnimCtrlData::CHILD_AS_SELF);

  if (!hasNotFoundNodes)
    wingw::message_box(wingw::MBS_INFO, "All nodes checked", "All child nodes have correct indices");
}

void AnimTreePlugin::updateSelectedDependentCtrl(PropPanel::ContainerPropertyControl *panel, dag::ConstSpan<int> dependent_ctrls)
{
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::TLeafHandle leaf = ctrlsTree->getSelLeaf();
  if (leaf)
  {
    AnimCtrlData *data = find_data_by_handle(controllersData, leaf);
    const int dataIdx = eastl::distance(controllersData.begin(), data);
    for (int idx : dependent_ctrls)
      if (idx == dataIdx)
      {
        const int selectedListIdx = panel->getInt(PID_CTRLS_NODES_LIST);
        fillCtrlsSettings(panel);
        if (selectedListIdx > 0)
        {
          panel->setInt(PID_CTRLS_NODES_LIST, selectedListIdx);
          setSelectedCtrlNodeListSettings(panel);
        }
        break;
      }
  }
}

void AnimTreePlugin::updateSelectedDependentState(PropPanel::ContainerPropertyControl *panel, dag::ConstSpan<int> dependent_states)
{
  PropPanel::ContainerPropertyControl *statesTree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  PropPanel::TLeafHandle leaf = statesTree->getSelLeaf();
  if (leaf)
  {
    AnimStatesData *data = find_data_by_handle(statesData, leaf);
    const int dataIdx = eastl::distance(statesData.begin(), data);
    for (int idx : dependent_states)
      if (idx == dataIdx)
      {
        const int selectedListIdx = panel->getInt(PID_STATES_NODES_LIST);
        selectedChangedAnimStatesTree(panel);
        if (selectedListIdx > 0)
        {
          panel->setInt(PID_STATES_NODES_LIST, selectedListIdx);
          setSelectedStateNodeListSettings(panel);
        }
        break;
      }
  }
}

template <typename T>
static dag::Vector<int> find_dependent_nodes(dag::ConstSpan<T> data_container, int idx)
{
  dag::Vector<int> dependentNodes;
  for (int i = 0; i < data_container.size(); ++i)
    if (data_container[i].childs.size() > 0)
      for (int childIdx : data_container[i].childs)
        if (childIdx == idx)
        {
          dependentNodes.emplace_back(i);
          break;
        }

  return dependentNodes;
}

DependentNodesResult AnimTreePlugin::checkDependentNodes(PropPanel::ContainerPropertyControl *panel, int idx, const char *new_name,
  const String &old_name)
{
  dag::Vector<int> dependentCtrls = find_dependent_nodes(make_span_const(controllersData), idx);
  dag::Vector<int> dependentStates = find_dependent_nodes(make_span_const(statesData), idx);

  int dialogResult = PropPanel::DIALOG_ID_NONE; // default value in case if we don't have any dependency
  if (dependentCtrls.size() + dependentStates.size() > 0)
  {
    // We can't update dependent nodes properly if suffix has changed
    if (get_node_mask_suffix_from_name(new_name) != get_node_mask_suffix_from_name(old_name.c_str()))
    {
      PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
      PropPanel::ContainerPropertyControl *statesTree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
      wingw::message_box(wingw::MBS_INFO, "Node suffix changed",
        "You have changed node name with %d dependent nodes. Need update child name manualy, see dependent nodes name in logs",
        dependentCtrls.size() + dependentStates.size());

      dialogResult = PropPanel::DIALOG_ID_NONE;
      DAEDITOR3.conNote("Make manual update child node in this nodes:");
      if (dependentCtrls.size() > 0)
        DAEDITOR3.conNote("Controllers:");
      for (int idx : dependentCtrls)
        DAEDITOR3.conNote("%s", ctrlsTree->getCaption(controllersData[idx].handle));
      if (dependentStates.size() > 0)
        DAEDITOR3.conNote("States:");
      for (int idx : dependentStates)
        DAEDITOR3.conNote("%s", statesTree->getCaption(statesData[idx].handle));
    }
    else
    {
      const int mbResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, "Node name changed",
        "You have changed node name with %d dependent nodes. Do you want to update child name in other nodes?\n"
        "Warning: selected dependent controller or state can be updated in prop panel.",
        dependentCtrls.size() + dependentStates.size());
      if (mbResult == wingw::MB_ID_YES)
        dialogResult = PropPanel::DIALOG_ID_YES;
      else if (mbResult == wingw::MB_ID_NO)
        dialogResult = PropPanel::DIALOG_ID_NO;
      else
        dialogResult = PropPanel::DIALOG_ID_CANCEL;
    }
  }

  return {eastl::move(dependentCtrls), eastl::move(dependentStates), dialogResult};
}

static void enum_update_child_name(DataBlock &settings, const char *name, const String &old_name, dag::Vector<int> &dependent_items)
{
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    DataBlock *enumItem = settings.getBlock(i);
    int oldNamePid = enumItem->getNameId(old_name);
    for (int j = 0; j < enumItem->paramCount(); ++j)
    {
      if (enumItem->getParamNameId(j) == oldNamePid)
      {
        enumItem->changeParamName(j, name);
        dependent_items.emplace_back(i);
        break;
      }
    }
  }
}

static TLeafHandle find_enum_handle_by_name(PropPanel::ContainerPropertyControl *tree, TLeafHandle root_leaf, const char *name)
{
  for (int i = 0; i < tree->getChildCount(root_leaf); ++i)
  {
    String leafName = tree->getCaption(tree->getChildLeaf(root_leaf, i));
    if (leafName == name)
      return tree->getChildLeaf(root_leaf, i);
  }
  return nullptr;
}

static void update_depedndent_states_tree_child_names(PropPanel::ContainerPropertyControl *tree, dag::ConstSpan<AnimStatesData> states,
  dag::ConstSpan<int> dependent_states, DataBlock *state_desc, DataBlock *enum_root, const char *name, const String &old_name,
  dag::Vector<int> &dependent_enum_items)
{
  for (int idx : dependent_states)
  {
    switch (states[idx].type)
    {
      case AnimStatesType::STATE:
      {
        G_ASSERTF(state_desc, "Try update dependent state name but state_desc block is nullptr");
        DataBlock *settings = find_block_by_name(state_desc, tree->getCaption(states[idx].handle));
        state_update_child_name(*settings, name, old_name);
        break;
      }
      case AnimStatesType::ENUM:
      {
        G_ASSERTF(enum_root, "Try update dependent enum name but enum block is nullptr");
        dag::Vector<int> dependentItemsIdxs;
        DataBlock *settings = find_block_by_block_name(enum_root, tree->getCaption(states[idx].handle));
        enum_update_child_name(*settings, name, old_name, dependentItemsIdxs);
        for (int itemIdx : dependentItemsIdxs)
        {
          TLeafHandle leaf = tree->getChildLeaf(states[idx].handle, itemIdx);
          dependent_enum_items.emplace_back(eastl::distance(states.begin(), find_data_by_handle(states, leaf)));
        }
        break;
      }
      default: break;
    }
  }
}

void AnimTreePlugin::proccesDependentNodes(PropPanel::ContainerPropertyControl *panel, const char *name, const String &old_name,
  DependentNodesResult &result)
{
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *statesTree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  dag::Vector<int> dependentEnumItems;

  for (int idx : result.dependentCtrls)
  {
    PropPanel::TLeafHandle parent = ctrlsTree->getParentLeaf(controllersData[idx].handle);
    String fullPath;
    DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, ctrlsTree, parent, fullPath);
    if (DataBlock *ctrlsProps =
          get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, ctrlsTree, parent))
      if (DataBlock *settings = find_block_by_name(ctrlsProps, ctrlsTree->getCaption(controllersData[idx].handle)))
      {
        switch (controllersData[idx].type)
        {
          case ctrl_type_paramSwitch:
            if (settings->getBlockByNameEx("nodes")->paramExists("enum_gen"))
            {
              AnimStatesData *initAnimStateData = find_data_by_type(statesData, AnimStatesType::INIT_ANIM_STATE);
              DataBlock propsState;
              DataBlock *enumRootProps = nullptr;
              String fullPathEnum;
              if (initAnimStateData && !initAnimStateData->fileName.empty())
              {
                propsState = get_props_from_include_state_data(includePaths, controllersData, *curAsset, ctrlsTree, *initAnimStateData,
                  fullPathEnum);
                enumRootProps = propsState.addBlock("initAnimState")->addBlock("enum");
              }
              const char *enumName = settings->getBlockByName("nodes")->getStr("enum_gen");
              DataBlock *enumProps = enumRootProps->getBlockByName(enumName);
              TLeafHandle enumLeaf = find_enum_handle_by_name(statesTree, getEnumsRootLeaf(statesTree), enumName);
              dag::Vector<int> dependentItemsIdxs;
              param_switch_update_enum_gen_child_name(*enumProps, settings->getStr("name", ""), name, old_name, dependentItemsIdxs);
              for (int itemIdx : dependentItemsIdxs)
              {
                TLeafHandle leaf = statesTree->getChildLeaf(enumLeaf, itemIdx);
                dependentEnumItems.emplace_back(eastl::distance(statesData.begin(), find_data_by_handle(statesData, leaf)));
              }
              saveProps(propsState, fullPathEnum);
              // Skip saveProps becuse we didn't change controller
              continue;
            }
            else
              param_switch_update_child_name(*settings, name, old_name);
            break;
          case ctrl_type_hub: hub_update_child_name(*settings, name, old_name); break;
          case ctrl_type_linearPoly: linear_poly_update_child_name(*settings, name, old_name); break;
          case ctrl_type_randomSwitch: random_switch_update_child_name(*settings, name, old_name); break;

          default: break;
        }
      }
    saveProps(props, fullPath);
  }

  AnimStatesData *stateDescData = find_data_by_type(statesData, AnimStatesType::STATE_DESC);
  AnimStatesData *initAnimStateData = find_data_by_type(statesData, AnimStatesType::INIT_ANIM_STATE);
  if (stateDescData && initAnimStateData && stateDescData->fileName == initAnimStateData->fileName)
  {
    String fullPath;
    DataBlock props = getPropsAnimStates(panel, *stateDescData, fullPath);
    DataBlock *stateDesc = props.getBlockByName("stateDesc");
    DataBlock *enumRoot = props.addBlock("initAnimState")->addBlock("enum");
    update_depedndent_states_tree_child_names(statesTree, statesData, result.dependentStates, stateDesc, enumRoot, name, old_name,
      dependentEnumItems);
    saveProps(props, fullPath);
  }
  else
  {
    DataBlock *stateDesc = nullptr;
    DataBlock *enumRoot = nullptr;
    String stateDescFullPath;
    DataBlock stateDescProps;
    String fullPathInitAnimState;
    DataBlock initAnimStateProps;
    if (stateDescData)
    {
      stateDescProps = getPropsAnimStates(panel, *stateDescData, stateDescFullPath);
      stateDesc = stateDescProps.addBlock("stateDesc");
    }
    if (initAnimStateData)
    {
      initAnimStateProps = getPropsAnimStates(panel, *initAnimStateData, fullPathInitAnimState);
      enumRoot = initAnimStateProps.addBlock("initAnimState")->addBlock("enum");
    }
    update_depedndent_states_tree_child_names(statesTree, statesData, result.dependentStates, stateDesc, enumRoot, name, old_name,
      dependentEnumItems);
    if (stateDescData)
      saveProps(stateDescProps, stateDescFullPath);
    if (initAnimStateData)
      saveProps(initAnimStateProps, fullPathInitAnimState);
  }
  result.dependentStates.insert(result.dependentStates.end(), dependentEnumItems.begin(), dependentEnumItems.end());
}
