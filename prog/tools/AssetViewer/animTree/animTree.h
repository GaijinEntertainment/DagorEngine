// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
#include "animParamData.h"
#include "animStates/animStatesData.h"
#include "animStates/animStatesTreeEventHandler.h"
#include "blendNodes/blendNodeData.h"
#include "blendNodes/blendNodeTreeEventHandler.h"
#include "controllers/animCtrlData.h"
#include "controllers/ctrlTreeEventHandler.h"
#include "controllers/ctrlChildsDialog.h"
#include "nodeMasks/nodeMaskData.h"
#include "nodeMasks/nodeMaskTreeEventHandler.h"
#include "animTreeAnimationPlayer.h"

#include <EditorCore/ec_interface.h>

#include <propPanel/c_control_event_handler.h>
#include <assets/asset.h>
#include <math/dag_geomTree.h>
#include <anim/dag_simpleNodeAnim.h>

using PropPanel::TLeafHandle;

class ILodController;
class IObjEntity;
enum CtrlType;
enum class AnimStatesType;
struct ParentLeafs;

class AnimTreePlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  AnimTreePlugin();
  ~AnimTreePlugin() { AnimTreePlugin::end(); }

  virtual const char *getInternalName() const { return "animTree"; }

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset);
  virtual bool end();

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const;

  virtual void actObjects(float dt);
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects() {}

  virtual bool supportAssetType(const DagorAsset &asset) const;

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &panel);
  virtual void postFillPropPanel() {}

  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);

protected:
  DagorAsset *curAsset = nullptr;

  AnimTreeAnimationPlayer animPlayer;
  dag::Vector<AnimParamData> ctrlParams;
  dag::Vector<AnimParamData> animBnlParams;
  dag::Vector<AnimParamData> stateParams;

  int curCtrlAndBlendNodeUniqueIdx = 0;
  TLeafHandle enumsRootLeaf = nullptr;
  dag::Vector<BlendNodeData> blendNodesData;
  dag::Vector<AnimCtrlData> controllersData;
  dag::Vector<NodeMaskData> nodeMasksData;
  dag::Vector<AnimStatesData> statesData;
  dag::Vector<String> includePaths;

  CtrlChildsDialog ctrlChildsDialog;
  CtrlTreeEventHandler ctrlTreeEventHandler;
  BlendNodeTreeEventHandler blendNodeTreeEventHandler;
  NodeMaskTreeEventHandler nodeMaskTreeEventHandler;
  AnimStatesTreeEventHandler animStatesTreeEventHandler;

  void addController(TLeafHandle handle, CtrlType type);
  void addBlendNode(TLeafHandle handle, BlendNodeType type);

  void savePropsWithoutReload(DataBlock &props, const String &path);
  TLeafHandle getEnumsRootLeaf(PropPanel::ContainerPropertyControl *tree);
  bool isEnumOrEnumItem(TLeafHandle leaf, PropPanel::ContainerPropertyControl *tree);
  void fillTreePanels(PropPanel::ContainerPropertyControl *panel);
  void fillCtrlsIncluded(PropPanel::ContainerPropertyControl *tree, const DataBlock &props, const String &source_path,
    TLeafHandle parent);
  void fillTreesIncluded(PropPanel::ContainerPropertyControl *panel, const DataBlock &props, const String &source_path,
    ParentLeafs parents);
  void fillAnimStatesTreeNode(PropPanel::ContainerPropertyControl *panel, const DataBlock &node, const char *include_name);
  void fillEnumTree(const DataBlock *settings, PropPanel::ContainerPropertyControl *panel, TLeafHandle tree_root);
  void fillAnimBlendSettings(PropPanel::ContainerPropertyControl *tree, PropPanel::ContainerPropertyControl *group);
  void fillCtrlsChilds(PropPanel::ContainerPropertyControl *panel);
  void findCtrlsChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
  void fillCtrlsChildsBody(PropPanel::ContainerPropertyControl *panel, TLeafHandle handle, CtrlType type);
  void fillCtrlsSettings(PropPanel::ContainerPropertyControl *panel);
  void fillCtrlsBlocksSettings(PropPanel::ContainerPropertyControl *panel, CtrlType type, const DataBlock *settings);
  void paramSwitchFillBlockSettings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
  void fillParamSwitchEnumGen(PropPanel::ContainerPropertyControl *panel, const DataBlock *nodes);
  void changeParamSwitchType(PropPanel::ContainerPropertyControl *panel);
  void setSelectedCtrlNodeListSettings(PropPanel::ContainerPropertyControl *panel);
  void addNodeCtrlList(PropPanel::ContainerPropertyControl *panel);
  void removeNodeCtrlList(PropPanel::ContainerPropertyControl *panel);

  bool checkIncludeBeforeSave(PropPanel::ContainerPropertyControl *tree, TLeafHandle leaf, const SimpleString &file_name,
    const SimpleString &new_path, const DataBlock &props);
  bool createBlkIfNotExistWithDialog(String &path, PropPanel::ContainerPropertyControl *tree, TLeafHandle include_leaf);
  void saveIncludeChange(PropPanel::ContainerPropertyControl *panel, PropPanel::ContainerPropertyControl *tree,
    TLeafHandle include_leaf, DataBlock &props, const String &old_name, String &new_full_path, ParentLeafs parents);
  void saveControllerSettings(PropPanel::ContainerPropertyControl *panel);
  void saveControllerParamsSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
  void saveControllerBlocksSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);

  void setTreeFilter(PropPanel::ContainerPropertyControl *panel, int tree_pid, int filter_pid);
  void addIncludeLeaf(PropPanel::ContainerPropertyControl *panel, int main_pid, TLeafHandle main_parent, const String &parent_name);

  void addEnumToAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void addItemToAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void removeNodeFromAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void saveSettingsAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void saveStatesParamsSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimStatesType type,
    const DataBlock &props);
  void selectedChangedAnimStatesTree(PropPanel::ContainerPropertyControl *panel);
  void changeStateDescType(PropPanel::ContainerPropertyControl *panel);
  void addStateDescItem(PropPanel::ContainerPropertyControl *panel, AnimStatesType type, const char *name);
  void addStateDescItem(PropPanel::ContainerPropertyControl *panel, DataBlock &props, String &full_path, TLeafHandle state_desc_leaf,
    AnimStatesType type, const char *name);

  void addNodeToCtrlTree(PropPanel::ContainerPropertyControl *panel);
  void addIncludeToCtrlTree(PropPanel::ContainerPropertyControl *panel, int type_pid);
  void removeNodeFromCtrlsTree(PropPanel::ContainerPropertyControl *panel);
  void changeCtrlType(PropPanel::ContainerPropertyControl *panel);

  void addMaskToNodeMasksTree(PropPanel::ContainerPropertyControl *panel);
  void addNodeToNodeMasksTree(PropPanel::ContainerPropertyControl *panel);
  void removeNodeFromNodeMasksTree(PropPanel::ContainerPropertyControl *panel);
  void saveSettingsNodeMasksTree(PropPanel::ContainerPropertyControl *panel);
  void selectedChangedNodeMasksTree(PropPanel::ContainerPropertyControl *panel);

  void createNodeAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void createLeafAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void addIrqNodeToAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void removeNodeFromAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void saveSettingsAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void selectedChangedAnimNodesTree(PropPanel::ContainerPropertyControl *panel);
  void irqTypeSelected(PropPanel::ContainerPropertyControl *panel);
  void changeAnimNodeType(PropPanel::ContainerPropertyControl *panel);
  void updateAnimNodeFields(PropPanel::ContainerPropertyControl *panel, int pid);
  void addLeafToBlendNodesTree(PropPanel::ContainerPropertyControl *tree, DataBlock &props);
  void addBlendNodeToTree(PropPanel::ContainerPropertyControl *tree, const DataBlock *node, int idx, TLeafHandle parent);
  void removeBlendNodeOrA2d(PropPanel::ContainerPropertyControl *tree, TLeafHandle leaf, DataBlock *props);

  void selectDynModel(PropPanel::ContainerPropertyControl *panel);

  // Save block settings for controllers
  void paramSwitchSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);
  void hubSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);
  void linearPolySaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);
  void randomSwitchSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data);

  // Find childs for controllers
  void paramSwitchFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
  void hubFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
  void linearPolyFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
  void randomSwitchFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings);
};