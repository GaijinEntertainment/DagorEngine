// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animParamData.h"
#include "animStates/animStatesData.h"
#include "animStates/animStatesTreeEventHandler.h"
#include "animStates/stateListSettingsEventHandler.h"
#include "blendNodes/blendNodeData.h"
#include "blendNodes/blendNodeTreeEventHandler.h"
#include "blendNodes/resetRandomSwitchListEventHandler.h"
#include "controllers/animCtrlData.h"
#include "controllers/ctrlTreeEventHandler.h"
#include "controllers/ctrlListSettingsEventHandler.h"
#include "controllers/paramsCtrlDragDropHandlers.h"
#include "animTreeEditorHost.h"
#include "childsDialog.h"
#include "nodeMasks/nodeMaskData.h"
#include "nodeMasks/nodeMaskTreeEventHandler.h"
#include "animTreeDragListHandler.h"

#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_map.h>
#include <generic/dag_tab.h>
#include <propPanel/c_control_event_handler.h>
#include <util/dag_simpleString.h>

using PropPanel::TLeafHandle;

class DagorAsset;

struct ParentLeafs;

struct DependentNodesResult
{
  dag::Vector<int> dependentCtrls;
  dag::Vector<int> dependentStates;
  int dialogResult = 0;
};

// State and tree model of the animTree editor, independent of the host editor plugin.
class AnimTreeEditor : public PropPanel::ControlEventHandler
{
public:
  AnimTreeEditor(IAnimTreeEditorHost &host);
  ~AnimTreeEditor();

  void begin(DagorAsset &asset);
  void end(PropPanel::ContainerPropertyControl *panel);

  void addController(TLeafHandle handle, CtrlType type);
  void addBlendNode(TLeafHandle handle, BlendNodeType type);
  void clearData();

  void saveProps(DataBlock &props, const String &path);
  DataBlock getPropsAnimStates(PropPanel::ContainerPropertyControl *panel, const AnimStatesData &data, String &full_path,
    bool only_includes = false);
  DataBlock *findCtrlSettings(PropPanel::ContainerPropertyControl *tree, TLeafHandle leaf, CtrlType type, DataBlock &out_props,
    String &full_path, const AnimCtrlData *parent_data = nullptr, bool only_includes = false);
  void saveParamsCtrlAfterIfMathReorder(PropPanel::ContainerPropertyControl *panel, int src_idx, int insert_idx);
  void reorderResetRandomSwitchList(PropPanel::ContainerPropertyControl *panel, int from, int to);

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // Creates all animTree panel controls, the dynModel button caption comes from the host animPlayer.
  void fillMainPanel(PropPanel::ContainerPropertyControl &panel, const char *dyn_model_caption);

protected:
  IAnimTreeEditorHost &host;
  DagorAsset *curAsset = nullptr;

  dag::Vector<AnimParamData> ctrlParams;
  dag::Vector<AnimParamData> animBnlParams;
  dag::Vector<AnimParamData> stateParams;
  dag::Vector<DependentParamData> ctrlDependentParams;
  dag::Vector<DependentParamData> stateDependentParams;

  int curCtrlAndBlendNodeUniqueIdx = 0;
  TLeafHandle enumsRootLeaf = nullptr;
  dag::Vector<BlendNodeData> blendNodesData;
  dag::Vector<AnimCtrlData> controllersData;
  dag::Vector<NodeMaskData> nodeMasksData;
  dag::Vector<AnimStatesData> statesData;
  dag::Vector<String> includePaths;

  ChildsDialog childsDialog;
  CtrlTreeEventHandler ctrlTreeEventHandler;
  CtrlListSettingsEventHandler ctrlListSettingsEventHandler;
  ResetRandomSwitchListEventHandler resetRandomSwitchListEventHandler;
  BlendNodeTreeEventHandler blendNodeTreeEventHandler;
  NodeMaskTreeEventHandler nodeMaskTreeEventHandler;
  AnimStatesTreeEventHandler animStatesTreeEventHandler;
  StateListSettingsEventHandler stateListSettingsEventHandler;

  AnimTreeListDragHandler ctrlsListDragHandler;
  AnimTreeListDropHandler ctrlsListDropHandler;
  AnimTreeListDragHandler statesListDragHandler;
  AnimTreeListDropHandler statesListDropHandler;
  AnimTreeListDragHandler resetRandomSwitchListDragHandler;
  AnimTreeListDropHandler resetRandomSwitchListDropHandler;
  IfMathDragHandler ifMathDragHandler;
  IfMathDropHandler ifMathDropHandler;

  DataBlock ctrlsSettingsPanelState;
  DataBlock mainPanelState;

  class TreeFilter;
  eastl::unordered_map<int, eastl::unique_ptr<TreeFilter>> treeFilters;

  bool checkIncludeBeforeSave(PropPanel::ContainerPropertyControl *tree, TLeafHandle leaf, const SimpleString &file_name,
    const SimpleString &new_path, const DataBlock &props);
  bool createBlkIfNotExistWithDialog(String &path, PropPanel::ContainerPropertyControl *tree, TLeafHandle include_leaf);
  void saveIncludeChange(PropPanel::ContainerPropertyControl *panel, PropPanel::ContainerPropertyControl *tree,
    TLeafHandle include_leaf, DataBlock &props, const String &old_name, String &new_full_path, ParentLeafs parents);
  void selectIncludeFileDialog(PropPanel::ContainerPropertyControl *panel, int pid);

  void saveSettingsAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void saveStatesParamsSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimStatesType type,
    const DataBlock &props);

  void addNodeToCtrlTree(PropPanel::ContainerPropertyControl *panel);
  void addIncludeToCtrlTree(PropPanel::ContainerPropertyControl *panel, int type_pid);
  void removeNodeFromCtrlsTree(PropPanel::ContainerPropertyControl *panel);

  void addLeafToBlendNodesTree(PropPanel::ContainerPropertyControl *tree, DataBlock &props);
  void removeBlendNodeOrA2d(PropPanel::ContainerPropertyControl *tree, TLeafHandle leaf, DataBlock *props);

  TLeafHandle getEnumsRootLeaf(PropPanel::ContainerPropertyControl *tree);
  bool isEnumOrEnumItem(TLeafHandle leaf, PropPanel::ContainerPropertyControl *tree);
  void fillCtrlsIncluded(PropPanel::ContainerPropertyControl *tree, const DataBlock &props, const String &source_path,
    TLeafHandle parent);
  void fillTreesIncluded(PropPanel::ContainerPropertyControl *panel, const DataBlock &props, const String &source_path,
    ParentLeafs parents);
  void fillAnimStatesTreeNode(PropPanel::ContainerPropertyControl *panel, const DataBlock &node, const char *include_name);
  void fillEnumTree(const DataBlock *settings, PropPanel::ContainerPropertyControl *panel, TLeafHandle tree_root,
    const char *include_name);
  void fillStatesChilds(PropPanel::ContainerPropertyControl *panel);
  void fillInitAnimStateChilds(PropPanel::ContainerPropertyControl *panel);
  void fillTreePanels(PropPanel::ContainerPropertyControl *panel);

  void addBlendNodeToTree(PropPanel::ContainerPropertyControl *tree, const DataBlock *node, int idx, TLeafHandle parent);
  void addIrqLeafsToBlendNodesTree(PropPanel::ContainerPropertyControl *tree, const DataBlock &settings, TLeafHandle parent);
  void addIncludeLeaf(PropPanel::ContainerPropertyControl *panel, int main_pid, TLeafHandle main_parent, const String &parent_name);
  void setStatesDragAndDropHandlers(PropPanel::ContainerPropertyControl *panel, AnimStatesType type);
  void setCtrlsDragAndDropHandlers(PropPanel::ContainerPropertyControl *panel, CtrlType type, AnimCtrlData *ctrl_data);
  void setTreeFilter(PropPanel::ContainerPropertyControl *panel, int tree_pid, int filter_pid);

  void setSelectedCtrlNodeListSettings(PropPanel::ContainerPropertyControl *panel);
  void addNodeCtrlList(PropPanel::ContainerPropertyControl *panel);
  void removeNodeCtrlList(PropPanel::ContainerPropertyControl *panel);

  void addEnumToAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void addItemToAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void removeNodeFromAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void updateStateFields(PropPanel::ContainerPropertyControl *panel, int pid);
  void updateStateDependentStatus(PropPanel::ContainerPropertyControl *panel, int pid);

  void selectedChangedAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void changeStateDescType(PropPanel::ContainerPropertyControl *panel);
  void addStateDescItem(PropPanel::ContainerPropertyControl *panel, AnimStatesType type, const char *name);
  void addStateDescItem(PropPanel::ContainerPropertyControl *panel, DataBlock &props, String &full_path, TLeafHandle state_desc_leaf,
    AnimStatesType type, const char *name);
  void setSelectedStateNodeListSettings(PropPanel::ContainerPropertyControl *panel);
  void addNodeStateList(PropPanel::ContainerPropertyControl *panel);
  void removeNodeStateList(PropPanel::ContainerPropertyControl *panel);
  void updateEnumItemValueCombo(PropPanel::ContainerPropertyControl *panel);

  void updateCtrlFields(PropPanel::ContainerPropertyControl *panel, int pid);
  void updateCtrlDependentStatus(PropPanel::ContainerPropertyControl *panel, int pid);

  void addMaskToNodeMasksTree(PropPanel::ContainerPropertyControl *panel);
  void addNodeToNodeMasksTree(PropPanel::ContainerPropertyControl *panel);
  void selectedChangedNodeMasksTree(PropPanel::ContainerPropertyControl *panel);
  void removeNodeFromNodeMasksTree(PropPanel::ContainerPropertyControl *panel);
  void saveSettingsNodeMasksTree(PropPanel::ContainerPropertyControl *panel);

  void addIrqNodeToAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void addResetRandomSwitchNodeList(PropPanel::ContainerPropertyControl *panel);
  void removeResetRandomSwitchNodeList(PropPanel::ContainerPropertyControl *panel);
  void selectedChangedResetRandomSwitchList(PropPanel::ContainerPropertyControl *panel);
  void irqTypeSelected(PropPanel::ContainerPropertyControl *panel);
  void changeAnimNodeType(PropPanel::ContainerPropertyControl *panel);

  void createNodeAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void createLeafAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void removeNodeFromAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void saveSettingsAnimNodesTree(PropPanel::ContainerPropertyControl *panel);

  void changeCtrlType(PropPanel::ContainerPropertyControl *panel);

  void fillAnimBlendSettings(PropPanel::ContainerPropertyControl *tree, PropPanel::ContainerPropertyControl *group);
  void selectedChangedAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void updateAnimNodeFields(PropPanel::ContainerPropertyControl *panel, int pid);

  void paramSwitchFillBlockSettings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
  void fillParamSwitchEnumGen(PropPanel::ContainerPropertyControl *panel, const DataBlock *nodes);
  void changeParamSwitchType(PropPanel::ContainerPropertyControl *panel);
  Tab<String> paramSwitchGetNodeNames(const DataBlock &settings);
  void paramSwitchFillMorphTimes(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings);
  void paramSwitchSetSelectedMorphTimeSettings(PropPanel::ContainerPropertyControl *panel);
  void paramSwitchAddMorphTime(PropPanel::ContainerPropertyControl *panel);
  void paramSwitchRemoveMorphTime(PropPanel::ContainerPropertyControl *panel);
  void paramSwitchRefreshMorphTimesCombo(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings = nullptr);
  void paramSwitchFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings,
    bool find_enum_gen_parent = true);

  void changeMultiChainFABRIKBlockType(PropPanel::ContainerPropertyControl *panel);
  void changeParamsCtrlBlockType(PropPanel::ContainerPropertyControl *panel);
  void paramsCtrlRemoveIfMath(PropPanel::ContainerPropertyControl *panel);
  void paramsCtrlAddIfMath(PropPanel::ContainerPropertyControl *panel);
  void paramsCtrlInitBlockSettings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
  void initIfMathFields(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings);

  void hubFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
  void linearPolyFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
  void randomSwitchFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
  void aliasFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
  void animateNodeFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);

  void fillCtrlsChilds(PropPanel::ContainerPropertyControl *panel);
  void findCtrlsChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
  void fillCtrlsChildsBody(PropPanel::ContainerPropertyControl *panel, TLeafHandle handle, CtrlType type);
  void fillCtrlsSettings(PropPanel::ContainerPropertyControl *panel);
  void fillCtrlsBlocksSettings(PropPanel::ContainerPropertyControl *panel, CtrlType type, const DataBlock *settings,
    AnimCtrlData *ctrl_data);

  void saveControllerSettings(PropPanel::ContainerPropertyControl *panel);
  void saveControllerParamsSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
  void saveControllerBlocksSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);

  void paramSwitchSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);
  void hubSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);
  void linearPolySaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);
  void randomSwitchSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);
  void stateSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock &settings, AnimStatesData &data,
    const DataBlock &state_desc);
  void postBlendCtrlOrderSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock &settings, AnimStatesData &data);
  void initFifo3SaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock &settings, AnimStatesData &data);

  void validateDependentNodes(PropPanel::ContainerPropertyControl *panel);
  DependentNodesResult checkDependentNodes(PropPanel::ContainerPropertyControl *panel, int idx, const char *new_name,
    const String &old_name);
  void proccesDependentNodes(PropPanel::ContainerPropertyControl *panel, const char *name, const String &old_name,
    DependentNodesResult &result);
  void updateSelectedDependentCtrl(PropPanel::ContainerPropertyControl *panel, dag::ConstSpan<int> dependent_ctrls);
  void updateSelectedDependentState(PropPanel::ContainerPropertyControl *panel, dag::ConstSpan<int> dependent_states);
};
