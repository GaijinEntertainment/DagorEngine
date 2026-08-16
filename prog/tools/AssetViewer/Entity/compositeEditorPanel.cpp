// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorPanel.h"
#include "../av_appwnd.h"
#include "../av_cm.h"
#include "entity_cm.h"
#include "compositeEditorTreeData.h"
#include "compositeEditorTreeDataNode.h"
#include <de3_objEntity.h>
#include <de3_composit.h>
#include <de3_interface.h>
#include <de3_dataBlockIdHolder.h>
#include <EditorCore/ec_cm.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <assets/asset.h>
#include <libTools/util/blkUtil.h>
#include <util/dag_delayedAction.h>
#include <math/dag_mathAng.h>
#include <osApiWrappers/dag_clipboard.h>
#include <propPanel/commonWindow/multiListDialog.h>
#include <propPanel/imguiHelper.h>

using hdpi::_pxActual;
using hdpi::_pxScaled;

CompositeEditorPanel::CompositeEditorPanel(PropPanel::ControlEventHandler *event_handler, int x, int y, unsigned w, unsigned h) :
  PropPanel::ContainerPropertyControl(0, event_handler, nullptr, x, y, _pxActual(w), _pxActual(h))
{
  supportedNodeParameters.addInt("place_type", ICompositObj::Props::PT_none);
  supportedNodeParameters.addReal("aboveHt", 0);
  supportedNodeParameters.addPoint2("rot_x", Point2::ZERO);
  supportedNodeParameters.addPoint2("rot_y", Point2::ZERO);
  supportedNodeParameters.addPoint2("rot_z", Point2::ZERO);
  supportedNodeParameters.addPoint2("offset_x", Point2::ZERO);
  supportedNodeParameters.addPoint2("offset_y", Point2::ZERO);
  supportedNodeParameters.addPoint2("offset_z", Point2::ZERO);
  supportedNodeParameters.addPoint2("scale", Point2(1, 0));
  supportedNodeParameters.addPoint2("yScale", Point2(1, 0));
  supportedNodeParameters.addBool("ignoreParentInstSeed", true);

  editedTreeDataNodeBlockId = IDataBlockIdHolder::invalid_id;

  for (int i = 0; i < CMP_NODE_PARAM_COUNT; ++i)
    parameters[i] = false;

  G_STATIC_ASSERT(CMP_NODE_PARAM_COUNT == 11);
#define SET_SUPPORTED_NODE_PARAM_ID(NAME) \
  supportedNodeParameterIds[CMP_NODE_PARAM_IDX_##NAME] = ID_COMPOSITE_EDITOR_NODE_PARAMETERS_##NAME
  SET_SUPPORTED_NODE_PARAM_ID(PLACE_TYPE);
  SET_SUPPORTED_NODE_PARAM_ID(ABOVE_HT);
  SET_SUPPORTED_NODE_PARAM_ID(ROT_X);
  SET_SUPPORTED_NODE_PARAM_ID(ROT_Y);
  SET_SUPPORTED_NODE_PARAM_ID(ROT_Z);
  SET_SUPPORTED_NODE_PARAM_ID(OFFSET_X);
  SET_SUPPORTED_NODE_PARAM_ID(OFFSET_Y);
  SET_SUPPORTED_NODE_PARAM_ID(OFFSET_Z);
  SET_SUPPORTED_NODE_PARAM_ID(SCALE);
  SET_SUPPORTED_NODE_PARAM_ID(SCALE_Y);
  SET_SUPPORTED_NODE_PARAM_ID(IGNORE_PARENT_INST_SEED);
#undef SET_SUPPORTED_NODE_PARAM_ID

  alertId = PropPanel::load_icon("alert");
  deleteId = PropPanel::load_icon("delete");

  captionSize = ImVec2();
  for (int i = 0; i < CMP_NODE_PARAM_COUNT; ++i)
  {
    const char *paramName = supportedNodeParameters.getParamName(i);
    const ImVec2 labelSize = ImGui::CalcTextSize(paramName);
    if (captionSize.x < labelSize.x)
      captionSize = labelSize;
  }
  captionSize.x += ImGui::GetStyle().ItemSpacing.x;
}

void CompositeEditorPanel::fillEntityGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
  PropPanel::IDropTargetHandler *dropTargetHandler)
{
  PropPanel::ContainerPropertyControl *extensible = group.createExtensible(ID_COMPOSITE_EDITOR_ENTITY_ACTIONS);
  extensible->setIntValue(1 << PropPanel::EXT_BUTTON_REMOVE);

  extensible->createButton(ID_COMPOSITE_EDITOR_ENTITY_SELECTOR, treeDataNode.getName());
  extensible->setDropTargetHandler(dropTargetHandler);
  group.createEditFloat(ID_COMPOSITE_EDITOR_ENTITY_WEIGHT, "Weight", treeDataNode.getWeight());
}

void CompositeEditorPanel::fillEntitiesGroup(PropPanel::ContainerPropertyControl &group,
  const CompositeEditorTreeDataNode &treeDataNode, PropPanel::IDropTargetHandler *dropTargetHandler, bool canEditEntities)
{
  int nodesToDisplay = canEditEntities ? treeDataNode.nodes.size() : 0;
  const bool limitReached = nodesToDisplay > MAX_COMPOSITE_ENTITY_COUNT;
  if (limitReached)
  {
    nodesToDisplay = MAX_COMPOSITE_ENTITY_COUNT;

    String s;
    s.printf(48, "Only the first %d entities are shown!", nodesToDisplay);
    group.createStatic(ID_COMPOSITE_EDITOR_ENTITIES_LIMIT_REACHED, s.c_str());
  }

  for (int nodeIndex = 0; nodeIndex < nodesToDisplay; ++nodeIndex)
  {
    const CompositeEditorTreeDataNode &treeDataSubNode = *treeDataNode.nodes[nodeIndex];

    if (!treeDataSubNode.isEntBlock())
      continue;

    PropPanel::ContainerPropertyControl *extensible =
      group.createExtensible(ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_ACTIONS_FIRST + nodeIndex);
    // We allow insertion even if the display limit is reached because the new entity is inserted before the selected one.
    extensible->setIntValue((1 << PropPanel::EXT_BUTTON_INSERT) | (1 << PropPanel::EXT_BUTTON_REMOVE));

    const char *name = treeDataSubNode.getName();
    extensible->createButton(ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_FIRST + nodeIndex, name);
    extensible->setDropTargetHandler(dropTargetHandler);

    const float weight = treeDataSubNode.getWeight();
    group.createEditFloat(ID_COMPOSITE_EDITOR_ENTITIES_WEIGHT_FIRST + nodeIndex, "Weight", weight);

    group.createSeparator();
  }

  group.createButton(ID_COMPOSITE_EDITOR_ENTITIES_ADD, "+", canEditEntities && !limitReached);
  group.setDropTargetHandler(dropTargetHandler);
}

void CompositeEditorPanel::fillChildrenGroup(PropPanel::ContainerPropertyControl &group,
  const CompositeEditorTreeDataNode &treeDataNode, PropPanel::IDropTargetHandler *dropTargetHandler, bool canEditChildren)
{
  int nodesToDisplay = canEditChildren ? treeDataNode.nodes.size() : 0;
  const bool limitReached = nodesToDisplay > MAX_COMPOSITE_ENTITY_COUNT;
  if (limitReached)
  {
    nodesToDisplay = MAX_COMPOSITE_ENTITY_COUNT;

    String s;
    s.printf(48, "Only the first %d children are shown!", nodesToDisplay);
    group.createStatic(ID_COMPOSITE_EDITOR_CHILDREN_LIMIT_REACHED, s.c_str());
  }

  for (int nodeIndex = 0; nodeIndex < nodesToDisplay; ++nodeIndex)
  {
    const CompositeEditorTreeDataNode &treeDataSubNode = *treeDataNode.nodes[nodeIndex];

    if (treeDataSubNode.isEntBlock())
      continue;

    PropPanel::ContainerPropertyControl *extensible =
      group.createExtensible(ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_ACTIONS_FIRST + nodeIndex);
    // We allow insertion even if the display limit is reached because the new node is inserted before the selected one.
    extensible->setIntValue((1 << PropPanel::EXT_BUTTON_INSERT) | (1 << PropPanel::EXT_BUTTON_REMOVE));

    const char *name = treeDataSubNode.getName();
    const bool canEditAssetName = treeDataSubNode.canEditAssetName(/*isRootNode = */ false);
    extensible->createButton(ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_FIRST + nodeIndex, name, canEditAssetName);
    extensible->setDropTargetHandler(dropTargetHandler);
  }

  group.createButton(ID_COMPOSITE_EDITOR_CHILDREN_ADD, "+", canEditChildren && !limitReached);
  group.setDropTargetHandler(dropTargetHandler);
}

static PropPanel::ContainerPropertyControl *createRemovableParameterContainer(int index, PropPanel::ContainerPropertyControl &group)
{
  const int removeId = ID_COMPOSITE_EDITOR_NODE_PARAMETERS_REMOVE + index;
  PropPanel::ContainerPropertyControl *extensible = group.createExtensible(removeId, true, "delete", "Remove param");
  extensible->setIntValue(1 << PropPanel::EXT_BUTTON_SINGLE_ACTION);
  return extensible;
}

static void createAddGlobalParam(int index, const char *paramName, PropPanel::ContainerPropertyControl &group)
{
  const int addId = ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ADD + index;
  PropPanel::ContainerPropertyControl *extensible = group.createExtensible(addId, true, nullptr, "Add param", "+");
  extensible->setIntValue(1 << PropPanel::EXT_BUTTON_SINGLE_ACTION);
  extensible->createStatic(0, paramName);
}

static void fillPoint2Parameter(const ImVec2 captionSize, int index, const char *paramName, int id,
  PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode, const bool useTransformationMatrix,
  real tmValue, real def)
{
  Point2 p2;
  bool fill = treeDataNode.tryGetPoint2Parameter(paramName, p2);
  if (!fill)
  {
    if (useTransformationMatrix)
    {
      if (!is_relative_equal_float(tmValue, def))
      {
        fill = true;
        p2 = Point2(tmValue, 0);
      }
    }
    else
      createAddGlobalParam(index, paramName, group);
  }

  if (fill)
  {
    PropPanel::ContainerPropertyControl *extensible = createRemovableParameterContainer(index, group);
    if (useTransformationMatrix)
      extensible->setIntValue(0); // Disables the delete button
    extensible->setUseFixedWidthColumns();
    extensible->createStatic(0, paramName);
    extensible->setWidthById(0, hdpi::_pxActual(captionSize.x));
    extensible->setEnabledById(0, !useTransformationMatrix);

    extensible->createStaticWithIcon(1, nullptr, false);
    if (useTransformationMatrix)
    {
      static const char *warningTooltip = "This field can not be edited directly.\n"
                                          "To make changes, adjust the values in the Matrix section.";
      PropPanel::PropertyControlBase *icon = extensible->getById(1);
      icon->setButtonPictureValues("alert");
      icon->setTooltip(warningTooltip);
    }
    const int height = ImGui::GetTextLineHeight();
    const ImVec2 iconSize = PropPanel::ImguiHelper::getImageButtonSize(ImVec2(height, height));
    extensible->setWidthById(1, hdpi::_pxActual(iconSize.x + ImGui::GetStyle().ItemSpacing.x));
    extensible->setEnabledById(1, !useTransformationMatrix);

    extensible->createPoint2(id, nullptr, p2, 2, !useTransformationMatrix, false);
    extensible->setWidthById(id, hdpi::_pxActual(ImGui::GetIO().DisplaySize.x));
  }
}

static void getTransformationComponents(const CompositeEditorTreeDataNode &treeDataNode, Point3 &position, Point3 &rotation,
  Point3 &scale)
{
  const TMatrix tm = treeDataNode.getTransformationMatrix();
  position = tm.getcol(3);

  ::matrix_to_euler(tm, rotation.y, rotation.z, rotation.x);
  rotation.x = RadToDeg(rotation.x);
  rotation.y = RadToDeg(rotation.y);
  rotation.z = RadToDeg(rotation.z);

  scale = Point3(::length(tm.getcol(0)), ::length(tm.getcol(1)), ::length(tm.getcol(2)));
}

void CompositeEditorPanel::fillGlobalParametersGroup(PropPanel::ContainerPropertyControl &group,
  const CompositeEditorTreeDataNode &treeDataNode)
{
  Point3 position(0, 0, 0);
  Point3 rotation(0, 0, 0);
  Point3 scale(1, 1, 1);
  const bool useTransformationMatrix = treeDataNode.getUseTransformationMatrix();
  if (useTransformationMatrix)
    getTransformationComponents(treeDataNode, position, rotation, scale);

  fillGlobalParametersGroup(group, treeDataNode, useTransformationMatrix, position, rotation, scale);
}

void CompositeEditorPanel::fillGlobalParametersGroup(PropPanel::ContainerPropertyControl &group,
  const CompositeEditorTreeDataNode &treeDataNode, const bool useTransformationMatrix, const Point3 &position, const Point3 &rotation,
  const Point3 &scale)
{
  if (group.getChildCount() > 0)
    group.clear();

  ImVec2 captionSize = ImVec2();
  for (int i = 0; i < (supportedNodeParameters.paramCount() - 1); ++i)
  {
    const char *paramName = supportedNodeParameters.getParamName(i);
    const ImVec2 labelSize = ImGui::CalcTextSize(paramName);
    if (captionSize.x < labelSize.x)
      captionSize = labelSize;
  }
  captionSize.x += ImGui::GetStyle().ItemSpacing.x;

  for (int index = 0; index < supportedNodeParameters.paramCount(); ++index)
  {
    const char *paramName = supportedNodeParameters.getParamName(index);
    if (CMP_NODE_PARAM_IDX_ROT_X <= index && index <= CMP_NODE_PARAM_IDX_SCALE_Y)
    { // Randomized transform parameters.
      if (index == CMP_NODE_PARAM_IDX_ROT_X)
        fillPoint2Parameter(captionSize, index, paramName, ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_X, group, treeDataNode,
          useTransformationMatrix, rotation[0], 0);
      if (index == CMP_NODE_PARAM_IDX_ROT_Y)
        fillPoint2Parameter(captionSize, index, paramName, ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_Y, group, treeDataNode,
          useTransformationMatrix, rotation[1], 0);
      if (index == CMP_NODE_PARAM_IDX_ROT_Z)
        fillPoint2Parameter(captionSize, index, paramName, ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_Z, group, treeDataNode,
          useTransformationMatrix, rotation[2], 0);

      if (index == CMP_NODE_PARAM_IDX_OFFSET_X)
        fillPoint2Parameter(captionSize, index, paramName, ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_X, group, treeDataNode,
          useTransformationMatrix, position[0], 0);
      if (index == CMP_NODE_PARAM_IDX_OFFSET_Y)
        fillPoint2Parameter(captionSize, index, paramName, ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_Y, group, treeDataNode,
          useTransformationMatrix, position[1], 0);
      if (index == CMP_NODE_PARAM_IDX_OFFSET_Z)
        fillPoint2Parameter(captionSize, index, paramName, ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_Z, group, treeDataNode,
          useTransformationMatrix, position[2], 0);

      if (index == CMP_NODE_PARAM_IDX_SCALE)
        fillPoint2Parameter(captionSize, index, paramName, ID_COMPOSITE_EDITOR_NODE_PARAMETERS_SCALE, group, treeDataNode,
          useTransformationMatrix, scale[0], 1);
      if (index == CMP_NODE_PARAM_IDX_SCALE_Y)
        fillPoint2Parameter(captionSize, index, paramName, ID_COMPOSITE_EDITOR_NODE_PARAMETERS_SCALE_Y, group, treeDataNode,
          useTransformationMatrix, scale[1], 1);
    }
    else
    {
      if (index == CMP_NODE_PARAM_IDX_PLACE_TYPE)
      {
        const int placeTypeParamIndex = treeDataNode.params.findParam(paramName);
        if (placeTypeParamIndex >= 0 && treeDataNode.params.getParamType(placeTypeParamIndex) == DataBlock::TYPE_INT)
        {
          PropPanel::ContainerPropertyControl *extensible = createRemovableParameterContainer(index, group);

          PropPanel::ContainerPropertyControl &placeGrp =
            *extensible->createRadioGroup(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_PLACE_TYPE, "Place on collision");

          placeGrp.createRadio(ICompositObj::Props::PT_none, "-- no --");
          placeGrp.createRadio(ICompositObj::Props::PT_coll, "Place pivot");
          placeGrp.createRadio(ICompositObj::Props::PT_collNorm, "Place pivot and use normal");
          placeGrp.createRadio(ICompositObj::Props::PT_3pod, "Place 3-point (bbox)");
          placeGrp.createRadio(ICompositObj::Props::PT_fnd, "Place foundation (bbox)");
          placeGrp.createRadio(ICompositObj::Props::PT_flt, "Place on water (floatable)");
          placeGrp.createRadio(ICompositObj::Props::PT_riColl, "Place pivot with rendinst collision");
          extensible->setInt(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_PLACE_TYPE, treeDataNode.params.getInt(placeTypeParamIndex));
        }
        else
          createAddGlobalParam(index, paramName, group);
      }

      if (index == CMP_NODE_PARAM_IDX_ABOVE_HT)
      {
        const int aboveHtParamIndex = treeDataNode.params.findParam(paramName);
        if (aboveHtParamIndex >= 0 && treeDataNode.params.getParamType(aboveHtParamIndex) == DataBlock::TYPE_REAL)
        {
          PropPanel::ContainerPropertyControl *extensible = createRemovableParameterContainer(index, group);
          const float value = treeDataNode.params.getReal(aboveHtParamIndex);
          extensible->createEditFloat(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ABOVE_HT, "Above height", value);
        }
        else
          createAddGlobalParam(index, paramName, group);
      }

      if (index == CMP_NODE_PARAM_IDX_IGNORE_PARENT_INST_SEED)
      {
        const int ignoreParentInstSeedParamIndex = treeDataNode.params.findParam(paramName);
        if (ignoreParentInstSeedParamIndex >= 0 &&
            treeDataNode.params.getParamType(ignoreParentInstSeedParamIndex) == DataBlock::TYPE_BOOL)
        {
          PropPanel::ContainerPropertyControl *extensible = createRemovableParameterContainer(index, group);
          extensible->createCheckBox(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_IGNORE_PARENT_INST_SEED, "Ignore parent instance seed",
            treeDataNode.params.getBool(ignoreParentInstSeedParamIndex));
        }
        else
          createAddGlobalParam(index, paramName, group);
      }
    }
  }
}

static bool hasNestedComposites(unsigned dataBlockId)
{
  IObjEntity *entity = get_app().getCompositeEditor().getSubEntityByDataBlockId(dataBlockId);
  if (!entity)
    return false;

  ICompositObj *compositObj = entity->queryInterface<ICompositObj>();
  if (!compositObj)
    return false;

  const int subEntityCount = compositObj->getCompositSubEntityCount();
  for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
  {
    IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
    if (!subEntity)
      continue;

    ICompositObj *subCompositObj = subEntity->queryInterface<ICompositObj>();
    if (subCompositObj)
      return true;
  }
  return false;
}

void CompositeEditorPanel::fillParametersGroup(PropPanel::ContainerPropertyControl &group,
  const CompositeEditorTreeDataNode &treeDataNode, bool canEditParameters)
{
  if (canEditParameters)
  {
    const bool useTransformationMatrix = treeDataNode.getUseTransformationMatrix();

    group.createCheckBox(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_USE_TRANSFORMATION_MATRIX, "Use transformation matrix",
      useTransformationMatrix);

    group.createSeparator();

    if (useTransformationMatrix)
    {
      const TMatrix tm = treeDataNode.getTransformationMatrix();
      const Point3 position = tm.getcol(3);
      const Point3 scale(::length(tm.getcol(0)), ::length(tm.getcol(1)), ::length(tm.getcol(2)));

      Point3 rotation;
      ::matrix_to_euler(tm, rotation.y, rotation.z, rotation.x);
      rotation.x = RadToDeg(rotation.x);
      rotation.y = RadToDeg(rotation.y);
      rotation.z = RadToDeg(rotation.z);

      group.createPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_LOCATION, "Location", position);
      group.createPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_ROTATION, "Rotation, deg", rotation);
      group.createPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_SCALE, "Scale", scale);
      group.createSeparator();
    }

    PropPanel::ContainerPropertyControl *grpGlobal =
      group.createGroup(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_GLOBAL_GRP, "Global parameters");
    if (grpGlobal)
    {
      fillGlobalParametersGroup(*grpGlobal, treeDataNode);
    }
  }

  group.createSeparator();
  group.createButton(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_COPY, "Copy params", canEditParameters);
  group.createButton(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_PASTE, "Paste params", canEditParameters, false);
}

void CompositeEditorPanel::fillInternal(const CompositeEditorTreeDataNode &treeDataNode,
  PropPanel::IDropTargetHandler *dropTargetHandler, bool isRootNode)
{
  if (treeDataNode.isEntBlock())
  {
    PropPanel::ContainerPropertyControl *grpEntity = createGroup(ID_COMPOSITE_EDITOR_ENTITY_GRP, "Entity");
    if (grpEntity)
      fillEntityGroup(*grpEntity, treeDataNode, dropTargetHandler);
  }
  else
  {
    PropPanel::ContainerPropertyControl *grpEntities = createGroup(ID_COMPOSITE_EDITOR_ENTITIES_GRP, "Entities");
    if (grpEntities)
      fillEntitiesGroup(*grpEntities, treeDataNode, dropTargetHandler, treeDataNode.canEditRandomEntities(isRootNode));
  }

  PropPanel::ContainerPropertyControl *grpChildren = createGroup(ID_COMPOSITE_EDITOR_CHILDREN_GRP, "Children");
  if (grpChildren)
    fillChildrenGroup(*grpChildren, treeDataNode, dropTargetHandler, treeDataNode.canEditChildren());

  PropPanel::ContainerPropertyControl *grpParameters = createGroup(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_GRP, "Node transforms");
  if (grpParameters)
  {
    const bool canEditParameters = !isRootNode && !treeDataNode.isEntBlock();
    fillParametersGroup(*grpParameters, treeDataNode, canEditParameters);
  }

  PropPanel::ContainerPropertyControl *grpComposit = createGroup(ID_COMPOSITE_EDITOR_COMPOSIT_GRP, "Composit");
  if (grpComposit)
  {
    // Asset wide parameters live in the root block, and onChange() only gets the selected node, so they can
    // only be edited while the root is selected. This one is the default for a placed composit in daEditorX.
    if (isRootNode)
    {
      grpComposit->createCheckBox(ID_COMPOSITE_EDITOR_COMPOSIT_AUTO_INST_SEED, "Auto-reseed enabled",
        treeDataNode.params.getBool("autoInstSeed", true));
      grpComposit->setTooltipId(ID_COMPOSITE_EDITOR_COMPOSIT_AUTO_INST_SEED,
        "The per-instance seed of the sub-entities is a hash of the composit\nposition, so moving a composit in daEditorX "
        "reseeds all of them.\nTurn it off and daEditorX gives each placed object a per-inst seed\nof its own instead: a huge "
        "composit becomes movable, but its\nper-instance variation stops following the position.\nA placed object can override "
        "this.");
    }

    grpComposit->createButton(ID_COMPOSITE_EDITOR_COMPOSIT_SAVE_CHANGES, "Save changes");
    grpComposit->createButton(ID_COMPOSITE_EDITOR_COMPOSIT_RESET_TO_FILE, "Reset to file", true, false);
  }

  if (!get_app().getCompositeEditor().areMultipleNodesSelected())
  {
    if (treeDataNode.isCompositeAsset())
    {
      PropPanel::ContainerPropertyControl *grpSplit = createGroup(ID_COMPOSITE_EDITOR_SPLIT_GRP, "Split composit");
      if (grpSplit)
      {
        grpSplit->setBoolValue(splitMinimized);
        const bool recursiveEnabled = hasNestedComposites(treeDataNode.dataBlockId);
        grpSplit->createCheckBox(ID_COMPOSITE_EDITOR_SPLIT_RECURSIVE, "Recursive split", splitRecursive, recursiveEnabled);
        grpSplit->createStaticWithIcon(ID_COMPOSITE_EDITOR_SPLIT_WARNING,
          "If enabled, all nested sub-composites inside this composite will also be split into separate assets.", true, true);
        PropPanel::PropertyControlBase *icon = grpSplit->getById(ID_COMPOSITE_EDITOR_SPLIT_WARNING);
        icon->setButtonPictureValues("alert");
        icon->setEnabled(recursiveEnabled);
        grpSplit->createButton(ID_COMPOSITE_EDITOR_SPLIT_COMPOSIT, "Split composit");
      }
    }
  }

  if (get_app().getCompositeEditor().canSaveSelectedAsComposite())
    createButton(ID_COMPOSITE_EDITOR_SAVE_AS_NEW_CMP, "Save as a new composite");

  const CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  if (compositeEditor.isEditingSubComposite())
  {
    PropPanel::ContainerPropertyControl *grpSubComposite = createGroup(ID_COMPOSITE_EDITOR_SUB_COMPOSITE_GRP, "Sub-composite");
    if (grpSubComposite)
    {
      grpSubComposite->createButton(CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE, "Save");
      grpSubComposite->createButton(CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE_UNIQUE, "Save unique", true, false);
      grpSubComposite->createButton(CM_COMPOSITE_EDITOR_SUB_COMPOSITE_REVERT, "Revert", compositeEditor.isModified(), false);
      PropPanel::PropertyControlBase *revertBtn = grpSubComposite->getById(CM_COMPOSITE_EDITOR_SUB_COMPOSITE_REVERT);
      if (revertBtn)
        revertBtn->setTooltip("Revert all modifications to the original version");
    }
  }
  if (!compositeEditor.areMultipleNodesSelected() && treeDataNode.isCompositeAsset())
    createButton(ID_COMPOSITE_EDITOR_EDIT_SUB_COMPOSITE, "Edit sub-composite in place");

  createButton(ID_COMPOSITE_EDITOR_DELETE_NODE, "Delete node", !isRootNode);
}

int CompositeEditorPanel::saveState(DataBlock &datablk, bool by_name)
{
  PropPanel::ContainerPropertyControl *grpSplit = getContainerById(ID_COMPOSITE_EDITOR_SPLIT_GRP);
  if (grpSplit)
    splitMinimized = grpSplit->getBoolValue();

  return ContainerPropertyControl::saveState(datablk, by_name);
}

void CompositeEditorPanel::fill(const CompositeEditorTreeData &treeData, const CompositeEditorTreeDataNode *selectedTreeDataNode,
  PropPanel::IDropTargetHandler *dropTargetHandler)
{
  panelState.reset();
  saveState(panelState);

  clear();

  if (treeData.isComposite && selectedTreeDataNode)
  {
    const bool isRootNode = selectedTreeDataNode == &treeData.rootNode;
    editedTreeDataNode = !isRootNode ? selectedTreeDataNode : nullptr;
    if (editedTreeDataNode)
    {
      if (editedTreeDataNode->dataBlockId != editedTreeDataNodeBlockId)
      {
        splitRecursive = false;
        editedTreeDataNodeBlockId = editedTreeDataNode->dataBlockId;
      }
    }
    else
      editedTreeDataNodeBlockId = IDataBlockIdHolder::invalid_id;
    fillInternal(*selectedTreeDataNode, dropTargetHandler, isRootNode);
  }

  loadState(panelState);
}

void CompositeEditorPanel::updateTransformParams(const CompositeEditorTreeData &treeData,
  CompositeEditorTreeDataNode *selectedTreeDataNode)
{
  if (!selectedTreeDataNode)
    return;

  const bool isRootNode = selectedTreeDataNode == &treeData.rootNode;
  const bool canEditParameters = !isRootNode && !selectedTreeDataNode->isEntBlock();
  if (!canEditParameters)
    return;

  const bool useTransformationMatrix = selectedTreeDataNode->getUseTransformationMatrix();
  if (useTransformationMatrix)
  {
    const TMatrix tm = selectedTreeDataNode->getTransformationMatrix();
    const Point3 position = tm.getcol(3);
    const Point3 scale(::length(tm.getcol(0)), ::length(tm.getcol(1)), ::length(tm.getcol(2)));

    Point3 rotation;
    ::matrix_to_euler(tm, rotation.y, rotation.z, rotation.x);
    rotation.x = RadToDeg(rotation.x);
    rotation.y = RadToDeg(rotation.y);
    rotation.z = RadToDeg(rotation.z);

    setPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_ROTATION, rotation);
    setPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_LOCATION, position);
    setPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_SCALE, scale);

    updateGlobalTransformParameters(*selectedTreeDataNode, true, position, rotation, scale);
  }
}

CompositeEditorRefreshType CompositeEditorPanel::onAddNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode)
{
  Tab<String> availableVars;
  for (int i = 0; i < supportedNodeParameters.paramCount(); ++i)
    if (treeDataNode.params.findParam(supportedNodeParameters.getParamName(i)) < 0)
      availableVars.push_back(String(supportedNodeParameters.getParamName(i)));

  Tab<String> selectedVars;
  PropPanel::MultiListDialog selectVars("Parameters to add", _pxScaled(300), _pxScaled(400), availableVars, selectedVars);
  if (selectVars.showDialog() != PropPanel::DIALOG_ID_OK || selectedVars.size() == 0)
    return CompositeEditorRefreshType::Nothing;

  makeUndoForPropertyEditing();

  for (const String &variableName : selectedVars)
  {
    const int paramIndex = supportedNodeParameters.findParam(variableName);
    G_ASSERT(paramIndex >= 0);
    G_ASSERT(treeDataNode.params.findParam(variableName) < 0);
    addOverrideParam(treeDataNode.params, supportedNodeParameters, paramIndex, false);
  }

  return CompositeEditorRefreshType::EntityAndCompositeEditor;
}

CompositeEditorRefreshType CompositeEditorPanel::onRemoveNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode)
{
  Tab<String> availableVars;
  for (int i = 0; i < supportedNodeParameters.paramCount(); ++i)
  {
    const int paramIndex = treeDataNode.params.findParam(supportedNodeParameters.getParamName(i));
    if (paramIndex >= 0 && treeDataNode.params.getParamType(paramIndex) == supportedNodeParameters.getParamType(i))
      availableVars.push_back(String(supportedNodeParameters.getParamName(i)));
  }

  Tab<String> selectedVars;
  PropPanel::MultiListDialog selectVars("Parameters to remove", _pxScaled(300), _pxScaled(400), availableVars, selectedVars);
  if (selectVars.showDialog() != PropPanel::DIALOG_ID_OK || selectedVars.size() == 0)
    return CompositeEditorRefreshType::Nothing;

  makeUndoForPropertyEditing();

  for (const String &variableName : selectedVars)
  {
    G_VERIFY(treeDataNode.params.removeParam(variableName));
  }

  return CompositeEditorRefreshType::EntityAndCompositeEditor;
}

CompositeEditorRefreshType CompositeEditorPanel::onPasteNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode)
{
  DataBlock block;
  if (!CompositeEditor::getNodeParamsFromClipboard(block))
    return CompositeEditorRefreshType::Nothing;

  makeUndoForPropertyEditing();

  treeDataNode.params.setParamsFrom(&block);

  return CompositeEditorRefreshType::EntityAndCompositeEditor;
}

class ScaleConflictDlg : public PropPanel::DialogWindow
{
  enum
  {
    DIALOG_MESSAGE = PropPanel::DIALOG_ID_FIRST_FREE
  };

public:
  ScaleConflictDlg(void *phandle) : DialogWindow(phandle, _pxScaled(370), _pxScaled(140), "Scale inputs conflict", false)
  {
    static const char *message = "The Z scale value will be discarded because it conflicts with the X scale value.\n\n"
                                 "Do you want to proceed with keeping only the X scale value?";

    PropPanel::ContainerPropertyControl *panel = DialogWindow::getPanel();
    panel->createStatic(DIALOG_MESSAGE, message, false, true, true);

    buttonsPanel->setText(PropPanel::DIALOG_ID_OK, "Proceed");
    setInitialFocus(PropPanel::DIALOG_ID_OK);
  }
};

void CompositeEditorPanel::updateParameter(int index, real x, CompositeEditorTreeDataNode &treeDataNode)
{
  const int id = supportedNodeParameterIds[index];
  PropPanel::PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    Point2 value = ptr->getPoint2Value();
    value.x = x;
    ptr->setPoint2Value(value);
    const char *paramName = supportedNodeParameters.getParamName(index);
    if (treeDataNode.params.paramExists(paramName))
      treeDataNode.params.setPoint2(paramName, value);
  }
}

void CompositeEditorPanel::addNonDefaultParam(int index, CompositeEditorTreeDataNode &treeDataNode, real tmValue, real def)
{
  const char *paramName = supportedNodeParameters.getParamName(index);
  bool exists = treeDataNode.params.paramExists(paramName);
  if (!exists)
  {
    if (!is_relative_equal_float(tmValue, def))
      treeDataNode.params.addPoint2(paramName, Point2(tmValue, 0));
  }
}

void CompositeEditorPanel::shouldRefillParam(bool &refill, int index, const CompositeEditorTreeDataNode &treeDataNode, real tmValue,
  real def)
{
  if (!refill)
  {
    const char *paramName = supportedNodeParameters.getParamName(index);
    bool exists = treeDataNode.params.paramExists(paramName);
    if (!exists)
    {
      if (!is_relative_equal_float(tmValue, def))
        refill = true;
    }
  }
}

void CompositeEditorPanel::updateGlobalTransformParameters(CompositeEditorTreeDataNode &treeDataNode,
  const bool useTransformationMatrix, const Point3 &position, Point3 const &rotation, const Point3 &scale)
{
  bool refill = false;
  shouldRefillParam(refill, CMP_NODE_PARAM_IDX_ROT_X, treeDataNode, rotation.x, 0);
  shouldRefillParam(refill, CMP_NODE_PARAM_IDX_ROT_Y, treeDataNode, rotation.y, 0);
  shouldRefillParam(refill, CMP_NODE_PARAM_IDX_ROT_Z, treeDataNode, rotation.z, 0);

  shouldRefillParam(refill, CMP_NODE_PARAM_IDX_OFFSET_X, treeDataNode, position.x, 0);
  shouldRefillParam(refill, CMP_NODE_PARAM_IDX_OFFSET_Y, treeDataNode, position.y, 0);
  shouldRefillParam(refill, CMP_NODE_PARAM_IDX_OFFSET_Z, treeDataNode, position.z, 0);

  shouldRefillParam(refill, CMP_NODE_PARAM_IDX_SCALE, treeDataNode, scale.x, 1);
  shouldRefillParam(refill, CMP_NODE_PARAM_IDX_SCALE_Y, treeDataNode, scale.y, 1);

  if (refill)
  {
    PropPanel::ContainerPropertyControl *grpParameters = getContainerById(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_GRP);
    if (grpParameters)
    {
      PropPanel::ContainerPropertyControl *grpGlobal = grpParameters->getContainerById(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_GLOBAL_GRP);
      if (grpGlobal)
      {
        fillGlobalParametersGroup(*grpGlobal, treeDataNode, useTransformationMatrix, position, rotation, scale);
      }
    }
  }
  else
  {
    updateParameter(CMP_NODE_PARAM_IDX_ROT_X, rotation.x, treeDataNode);
    updateParameter(CMP_NODE_PARAM_IDX_ROT_Y, rotation.y, treeDataNode);
    updateParameter(CMP_NODE_PARAM_IDX_ROT_Z, rotation.z, treeDataNode);

    updateParameter(CMP_NODE_PARAM_IDX_OFFSET_X, position.x, treeDataNode);
    updateParameter(CMP_NODE_PARAM_IDX_OFFSET_Y, position.y, treeDataNode);
    updateParameter(CMP_NODE_PARAM_IDX_OFFSET_Z, position.z, treeDataNode);

    updateParameter(CMP_NODE_PARAM_IDX_SCALE, scale.x, treeDataNode);
    updateParameter(CMP_NODE_PARAM_IDX_SCALE_Y, scale.y, treeDataNode);
  }
}

// TODO:
void CompositeEditorPanel::onChangeTransformationMatrixDelayed()
{
  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  CompositeEditorPanel *compositeEditorPanel = compositeEditor.compositePropPanel.get();
  if (!compositeEditorPanel)
    return;

  // TODO:
  ScaleConflictDlg dlg(nullptr);
  if (dlg.showDialog() == PropPanel::DIALOG_ID_CANCEL)
  {
    PropPanel::PropertyControlBase *control =
      compositeEditorPanel->getById(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_USE_TRANSFORMATION_MATRIX);
    G_ASSERT(control);
    control->setBoolValue(true);
    return;
  }

  // TODO:
  unsigned int dataBlockId = compositeEditor.getSelectedTreeNodeDataBlockId();
  CompositeEditorTreeDataNode *treeDataNode = compositeEditor.getTreeNodeByDataBlockId(dataBlockId);
  if (!treeDataNode)
    return;

  const Point3 position = compositeEditorPanel->getPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_LOCATION);
  const Point3 rotation = compositeEditorPanel->getPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_ROTATION);
  const Point3 scale = compositeEditorPanel->getPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_SCALE);

  compositeEditorPanel->addNonDefaultParam(CMP_NODE_PARAM_IDX_ROT_X, *treeDataNode, rotation.x, 0);
  compositeEditorPanel->addNonDefaultParam(CMP_NODE_PARAM_IDX_ROT_Y, *treeDataNode, rotation.y, 0);
  compositeEditorPanel->addNonDefaultParam(CMP_NODE_PARAM_IDX_ROT_Z, *treeDataNode, rotation.z, 0);

  compositeEditorPanel->addNonDefaultParam(CMP_NODE_PARAM_IDX_OFFSET_X, *treeDataNode, position.x, 0);
  compositeEditorPanel->addNonDefaultParam(CMP_NODE_PARAM_IDX_OFFSET_Y, *treeDataNode, position.y, 0);
  compositeEditorPanel->addNonDefaultParam(CMP_NODE_PARAM_IDX_OFFSET_Z, *treeDataNode, position.z, 0);

  compositeEditorPanel->addNonDefaultParam(CMP_NODE_PARAM_IDX_SCALE, *treeDataNode, scale.x, 1);
  compositeEditorPanel->addNonDefaultParam(CMP_NODE_PARAM_IDX_SCALE_Y, *treeDataNode, scale.y, 1);

  PropPanel::ContainerPropertyControl *grpParameters = compositeEditorPanel->getContainerById(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_GRP);
  if (grpParameters)
  {
    PropPanel::ContainerPropertyControl *grpGlobal = grpParameters->getContainerById(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_GLOBAL_GRP);
    if (grpGlobal)
    {
      compositeEditorPanel->fillGlobalParametersGroup(*grpGlobal, *treeDataNode, false, position, rotation, scale);
    }
  }

  G_VERIFY(treeDataNode->params.removeParam("tm"));

  // TODO:
  compositeEditor.beginUndo();
  compositeEditor.updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);
  compositeEditor.endUndo("Composit Editor: Property editing");
}

CompositeEditorRefreshType CompositeEditorPanel::onChange(CompositeEditorTreeDataNode &treeDataNode, int pcb_id)
{
  if (pcb_id == ID_COMPOSITE_EDITOR_ENTITY_WEIGHT)
  {
    if (treeDataNode.isEntBlock())
      treeDataNode.params.setReal("weight", getFloat(pcb_id));
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_ENTITIES_WEIGHT_FIRST && pcb_id <= ID_COMPOSITE_EDITOR_ENTITIES_WEIGHT_LAST)
  {
    const int entityIndex = pcb_id - ID_COMPOSITE_EDITOR_ENTITIES_WEIGHT_FIRST;
    CompositeEditorTreeDataNode &treeDataSubNode = *treeDataNode.nodes[entityIndex];
    if (treeDataSubNode.isEntBlock())
    {
      const float newValue = getFloat(pcb_id);
      treeDataSubNode.params.setReal("weight", newValue);
    }
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_USE_TRANSFORMATION_MATRIX)
  {
    const bool useTransformationMatrix = getBool(pcb_id);
    if (useTransformationMatrix != treeDataNode.getUseTransformationMatrix())
    {
      if (useTransformationMatrix)
      {
        treeDataNode.setIdentityTransformationMatrix();

        bool updateMatrix = false;
        for (int i = CMP_NODE_PARAM_IDX_ROT_X; i <= CMP_NODE_PARAM_IDX_SCALE_Y; ++i)
        {
          const int id = supportedNodeParameterIds[i];
          if (getById(id) != nullptr)
          {
            updateMatrix = true;
            break;
          }
        }

        if (updateMatrix)
        {
          // TODO:
          Point3 rotation;
          rotation[0] = getPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_X).x;
          rotation[1] = getPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_Y).x;
          rotation[2] = getPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_Z).x;

          Quat q;
          euler_to_quat(DegToRad(rotation.y), DegToRad(rotation.z), DegToRad(rotation.x), q);
          const TMatrix rotTm = makeTM(q);

          Point3 position;
          position[0] = getPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_X).x;
          position[1] = getPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_Y).x;
          position[2] = getPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_Z).x;

          TMatrix posTm = TMatrix::IDENT;
          posTm.setcol(3, position);

          Point3 scale = Point3(1, 1, 1);
          PropPanel::PropertyControlBase *ptr = getById(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_SCALE);
          if (ptr)
            scale[0] = scale[2] = ptr->getPoint2Value().x;
          ptr = getById(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_SCALE_Y);
          if (ptr)
            scale[1] = ptr->getPoint2Value().x;

          TMatrix scaleTm = TMatrix::IDENT;
          scaleTm[0][0] = scale.x;
          scaleTm[1][1] = scale.y;
          scaleTm[2][2] = scale.z;

          const TMatrix tm = posTm * rotTm * scaleTm;

          const int paramIndex = treeDataNode.params.findParam("tm");
          if (paramIndex >= 0 && treeDataNode.params.getParamType(paramIndex) == DataBlock::TYPE_MATRIX)
            treeDataNode.params.setTm(paramIndex, tm);
        }
      }
      else
      {
        // TODO:
        const Point3 scale = getPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_SCALE);
        if (!is_relative_equal_float(scale.x, scale.z))
        {
          delayed_call(&onChangeTransformationMatrixDelayed);
          return CompositeEditorRefreshType::Nothing;
        }

        const Point3 position = getPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_LOCATION);
        const Point3 rotation = getPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_ROTATION);

        // TODO:
        addNonDefaultParam(CMP_NODE_PARAM_IDX_ROT_X, treeDataNode, rotation.x, 0);
        addNonDefaultParam(CMP_NODE_PARAM_IDX_ROT_Y, treeDataNode, rotation.y, 0);
        addNonDefaultParam(CMP_NODE_PARAM_IDX_ROT_Z, treeDataNode, rotation.z, 0);

        addNonDefaultParam(CMP_NODE_PARAM_IDX_OFFSET_X, treeDataNode, position.x, 0);
        addNonDefaultParam(CMP_NODE_PARAM_IDX_OFFSET_Y, treeDataNode, position.y, 0);
        addNonDefaultParam(CMP_NODE_PARAM_IDX_OFFSET_Z, treeDataNode, position.z, 0);

        addNonDefaultParam(CMP_NODE_PARAM_IDX_SCALE, treeDataNode, scale.x, 1);
        addNonDefaultParam(CMP_NODE_PARAM_IDX_SCALE_Y, treeDataNode, scale.y, 1);

        PropPanel::ContainerPropertyControl *grpParameters = getContainerById(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_GRP);
        if (grpParameters)
        {
          PropPanel::ContainerPropertyControl *grpGlobal =
            grpParameters->getContainerById(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_GLOBAL_GRP);
          if (grpGlobal)
          {
            fillGlobalParametersGroup(*grpGlobal, treeDataNode, useTransformationMatrix, position, rotation, scale);
          }
        }

        G_VERIFY(treeDataNode.params.removeParam("tm"));
      }

      return CompositeEditorRefreshType::EntityAndCompositeEditor;
    }
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_ROTATION || pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_LOCATION ||
           pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_SCALE)
  {
    const Point3 rotation = getPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_ROTATION);
    const Point3 position = getPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_LOCATION);
    const Point3 scale = getPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_SCALE);

    Quat q;
    euler_to_quat(DegToRad(rotation.y), DegToRad(rotation.z), DegToRad(rotation.x), q);
    const TMatrix rotTm = makeTM(q);

    TMatrix posTm = TMatrix::IDENT;
    posTm.setcol(3, position);

    TMatrix scaleTm = TMatrix::IDENT;
    scaleTm[0][0] = scale.x;
    scaleTm[1][1] = scale.y;
    scaleTm[2][2] = scale.z;

    const TMatrix tm = posTm * rotTm * scaleTm;

    const int paramIndex = treeDataNode.params.findParam("tm");
    if (paramIndex >= 0 && treeDataNode.params.getParamType(paramIndex) == DataBlock::TYPE_MATRIX)
      treeDataNode.params.setTm(paramIndex, tm);

    updateGlobalTransformParameters(treeDataNode, true, position, rotation, scale);
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_X)
  {
    treeDataNode.params.setPoint2("rot_x", getPoint2(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_Y)
  {
    treeDataNode.params.setPoint2("rot_y", getPoint2(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_Z)
  {
    treeDataNode.params.setPoint2("rot_z", getPoint2(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_X)
  {
    treeDataNode.params.setPoint2("offset_x", getPoint2(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_Y)
  {
    treeDataNode.params.setPoint2("offset_y", getPoint2(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_Z)
  {
    treeDataNode.params.setPoint2("offset_z", getPoint2(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_SCALE)
  {
    treeDataNode.params.setPoint2("scale", getPoint2(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_SCALE_Y)
  {
    treeDataNode.params.setPoint2("yScale", getPoint2(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_PLACE_TYPE)
  {
    treeDataNode.params.setInt("place_type", getInt(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ABOVE_HT)
  {
    treeDataNode.params.setReal("aboveHt", getFloat(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_IGNORE_PARENT_INST_SEED)
  {
    treeDataNode.params.setBool("ignoreParentInstSeed", getBool(pcb_id));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_COMPOSIT_AUTO_INST_SEED)
  {
    if (getBool(pcb_id))
      treeDataNode.params.removeParam("autoInstSeed"); // the default, so keep it out of the blk
    else
      treeDataNode.params.setBool("autoInstSeed", false);
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_SPLIT_RECURSIVE)
  {
    splitRecursive = !splitRecursive;
  }
  else
  {
    return CompositeEditorRefreshType::Nothing;
  }

  return CompositeEditorRefreshType::Entity;
}

CompositeEditorRefreshType CompositeEditorPanel::onClick(CompositeEditorTreeDataNode &treeDataNode, int pcb_id)
{
  if (pcb_id == ID_COMPOSITE_EDITOR_ENTITY_SELECTOR)
  {
    const char *oldAssetName = treeDataNode.getAssetName();
    const char *assetName = DAEDITOR3.selectAsset(oldAssetName, "Select asset", DAEDITOR3.getGenObjAssetTypes());
    if (!assetName)
      return CompositeEditorRefreshType::Nothing;

    makeUndoForPropertyEditing();

    if (*assetName)
      treeDataNode.params.setStr("name", assetName);
    else
      treeDataNode.params.removeParam("name");

    // Remove the unused type parameter from old .blk files.
    treeDataNode.params.removeParam("type");
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_ENTITIES_ADD)
  {
    const char *assetName = DAEDITOR3.selectAsset("", "Select asset", DAEDITOR3.getGenObjAssetTypes());
    const bool emptyAsset = !assetName || *assetName == 0;

    makeUndoForPropertyEditing();

    treeDataNode.insertEntBlock(-1);

    if (!emptyAsset)
      treeDataNode.nodes.back()->params.setStr("name", assetName);
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_ACTIONS_FIRST && pcb_id <= ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_ACTIONS_LAST)
  {
    const int entityIndex = pcb_id - ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_ACTIONS_FIRST;

    const int action = getInt(pcb_id);
    if (action == PropPanel::EXT_BUTTON_INSERT)
    {
      makeUndoForPropertyEditing();
      treeDataNode.insertEntBlock(entityIndex);
    }
    else if (action == PropPanel::EXT_BUTTON_REMOVE)
    {
      makeUndoForPropertyEditing();
      treeDataNode.nodes.erase(treeDataNode.nodes.begin() + entityIndex);
    }
    else
    {
      return CompositeEditorRefreshType::Nothing;
    }
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_FIRST && pcb_id <= ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_LAST)
  {
    const int entityIndex = pcb_id - ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_FIRST;

    const char *oldAssetName = treeDataNode.nodes[entityIndex]->getAssetName();
    const char *assetName = DAEDITOR3.selectAsset(oldAssetName, "Select asset", DAEDITOR3.getGenObjAssetTypes());
    if (!assetName)
      return CompositeEditorRefreshType::Nothing;

    makeUndoForPropertyEditing();

    if (*assetName)
      treeDataNode.nodes[entityIndex]->params.setStr("name", assetName);
    else
      treeDataNode.nodes[entityIndex]->params.removeParam("name");

    // Remove the unused type parameter from old .blk files.
    treeDataNode.nodes[entityIndex]->params.removeParam("type");
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_CHILDREN_ADD)
  {
    makeUndoForPropertyEditing();
    treeDataNode.insertNodeBlock(-1);
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_ACTIONS_FIRST && pcb_id <= ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_ACTIONS_LAST)
  {
    const int entityIndex = pcb_id - ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_ACTIONS_FIRST;

    const int action = getInt(pcb_id);
    if (action == PropPanel::EXT_BUTTON_INSERT)
    {
      makeUndoForPropertyEditing();
      treeDataNode.insertNodeBlock(entityIndex);
    }
    else if (action == PropPanel::EXT_BUTTON_REMOVE)
    {
      makeUndoForPropertyEditing();
      treeDataNode.nodes.erase(treeDataNode.nodes.begin() + entityIndex);
    }
    else
    {
      return CompositeEditorRefreshType::Nothing;
    }
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_FIRST && pcb_id <= ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_LAST)
  {
    const int childIndex = pcb_id - ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_FIRST;

    const char *oldAssetName = treeDataNode.nodes[childIndex]->getAssetName();
    const char *assetName = DAEDITOR3.selectAsset(oldAssetName, "Select asset", DAEDITOR3.getGenObjAssetTypes());
    if (!assetName)
      return CompositeEditorRefreshType::Nothing;

    makeUndoForPropertyEditing();

    if (*assetName)
      treeDataNode.nodes[childIndex]->params.setStr("name", assetName);
    else
      treeDataNode.nodes[childIndex]->params.removeParam("name");

    // Remove the unused type parameter from old .blk files.
    treeDataNode.nodes[childIndex]->params.removeParam("type");
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ADD && pcb_id <= ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ADD_END)
  {
    makeUndoForPropertyEditing();

    const int paramIndex = pcb_id - ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ADD;
    const char *variableName = supportedNodeParameters.getParamName(paramIndex);
    G_ASSERT(paramIndex >= 0);
    G_ASSERT(treeDataNode.params.findParam(variableName) < 0);
    addOverrideParam(treeDataNode.params, supportedNodeParameters, paramIndex, false);
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_NODE_PARAMETERS_REMOVE && pcb_id <= ID_COMPOSITE_EDITOR_NODE_PARAMETERS_REMOVE_END)
  {
    makeUndoForPropertyEditing();

    const int paramIndex = pcb_id - ID_COMPOSITE_EDITOR_NODE_PARAMETERS_REMOVE;
    const char *variableName = supportedNodeParameters.getParamName(paramIndex);
    G_ASSERT(paramIndex >= 0);
    G_VERIFY(treeDataNode.params.removeParam(variableName));
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_COPY)
  {
    get_app().getCompositeEditor().copySelectedNodeParams();
    return CompositeEditorRefreshType::Nothing;
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_PASTE)
  {
    return onPasteNodeParametersClicked(treeDataNode);
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_SPLIT_COMPOSIT)
  {
    get_app().getCompositeEditor().splitSelectedCompositeNode(splitRecursive);
  }
  else
  {
    return CompositeEditorRefreshType::Nothing;
  }

  return CompositeEditorRefreshType::EntityAndCompositeEditor;
}

CompositeEditorRefreshType CompositeEditorPanel::onDragAndDropAsset(CompositeEditorTreeDataNode &treeDataNode, int pcb_id,
  DagorAsset *asset)
{
  G_ASSERT(asset);

  if (pcb_id == ID_COMPOSITE_EDITOR_ENTITY_SELECTOR)
  {
    const char *assetName = asset->getName();
    if (!assetName)
      return CompositeEditorRefreshType::Nothing;

    makeUndoForPropertyEditing();

    if (*assetName)
      treeDataNode.params.setStr("name", assetName);
    else
      treeDataNode.params.removeParam("name");

    // Remove the unused type parameter from old .blk files.
    treeDataNode.params.removeParam("type");
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_ENTITIES_ADD)
  {
    const char *assetName = asset->getName();
    const bool emptyAsset = !assetName || *assetName == 0;

    makeUndoForPropertyEditing();

    treeDataNode.insertEntBlock(-1);

    if (!emptyAsset)
      treeDataNode.nodes.back()->params.setStr("name", assetName);
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_FIRST && pcb_id <= ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_LAST)
  {
    const int entityIndex = pcb_id - ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_FIRST;

    const char *assetName = asset->getName();
    if (!assetName)
      return CompositeEditorRefreshType::Nothing;

    makeUndoForPropertyEditing();

    if (*assetName)
      treeDataNode.nodes[entityIndex]->params.setStr("name", assetName);
    else
      treeDataNode.nodes[entityIndex]->params.removeParam("name");

    // Remove the unused type parameter from old .blk files.
    treeDataNode.nodes[entityIndex]->params.removeParam("type");
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_CHILDREN_ADD)
  {
    const int entityIndex = treeDataNode.nodeCount();

    makeUndoForPropertyEditing();
    treeDataNode.insertNodeBlock(-1);

    const char *assetName = asset->getName();
    if (*assetName)
      treeDataNode.nodes[entityIndex]->params.setStr("name", assetName);
    else
      treeDataNode.nodes[entityIndex]->params.removeParam("name");
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_FIRST && pcb_id <= ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_LAST)
  {
    const int childIndex = pcb_id - ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_FIRST;

    makeUndoForPropertyEditing();

    const char *assetName = asset->getName();
    if (*assetName)
      treeDataNode.nodes[childIndex]->params.setStr("name", assetName);
    else
      treeDataNode.nodes[childIndex]->params.removeParam("name");

    // Remove the unused type parameter from old .blk files.
    treeDataNode.nodes[childIndex]->params.removeParam("type");
  }
  else
  {
    return CompositeEditorRefreshType::Nothing;
  }

  return CompositeEditorRefreshType::EntityAndCompositeEditor;
}

void CompositeEditorPanel::makeUndoForPropertyEditing()
{
  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  compositeEditor.beginUndo();
  compositeEditor.endUndo("Composit Editor: Property editing");
}
