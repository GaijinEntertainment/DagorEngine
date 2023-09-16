#include "compositeEditorPanel.h"
#include "../av_appwnd.h"
#include "../av_cm.h"
#include "compositeEditorTreeData.h"
#include "compositeEditorTreeDataNode.h"
#include <de3_composit.h>
#include <de3_interface.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <libTools/util/blkUtil.h>
#include <math/dag_mathAng.h>
#include <osApiWrappers/dag_clipboard.h>
#include <propPanel2/comWnd/list_dialog.h>

CompositeEditorPanel::CompositeEditorPanel(ControlEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h,
  const char caption[]) :
  CPanelWindow(event_handler, phandle, x, y, w, h, caption)
{
  supportedNodeParameters.addInt("place_type", ICompositObj::Props::PT_none);
  supportedNodeParameters.addPoint2("rot_x", Point2::ZERO);
  supportedNodeParameters.addPoint2("rot_y", Point2::ZERO);
  supportedNodeParameters.addPoint2("rot_z", Point2::ZERO);
  supportedNodeParameters.addPoint2("offset_x", Point2::ZERO);
  supportedNodeParameters.addPoint2("offset_y", Point2::ZERO);
  supportedNodeParameters.addPoint2("offset_z", Point2::ZERO);
  supportedNodeParameters.addPoint2("scale", Point2(1, 0));
  supportedNodeParameters.addPoint2("yScale", Point2(1, 0));
  supportedNodeParameters.addReal("aboveHt", 0);
}

void CompositeEditorPanel::fillEntityGroup(PropertyContainerControlBase &group, const CompositeEditorTreeDataNode &treeDataNode)
{
  PropertyContainerControlBase *extensible = group.createExtensible(ID_COMPOSITE_EDITOR_ENTITY_ACTIONS);
  extensible->setIntValue(1 << EXT_BUTTON_REMOVE);

  extensible->createButton(ID_COMPOSITE_EDITOR_ENTITY_SELECTOR, treeDataNode.getName());
  group.createEditFloat(ID_COMPOSITE_EDITOR_ENTITY_WEIGHT, "Weight", treeDataNode.getWeight());
}

void CompositeEditorPanel::fillEntitiesGroup(PropertyContainerControlBase &group, const CompositeEditorTreeDataNode &treeDataNode,
  bool canEditEntities)
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

    PropertyContainerControlBase *extensible = group.createExtensible(ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_ACTIONS_FIRST + nodeIndex);
    // We allow insertion even if the display limit is reached because the new entity is inserted before the selected one.
    extensible->setIntValue((1 << EXT_BUTTON_INSERT) | (1 << EXT_BUTTON_REMOVE));

    const char *name = treeDataSubNode.getName();
    extensible->createButton(ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_FIRST + nodeIndex, name);

    const float weight = treeDataSubNode.getWeight();
    group.createEditFloat(ID_COMPOSITE_EDITOR_ENTITIES_WEIGHT_FIRST + nodeIndex, "Weight", weight);

    group.createSeparator();
  }

  group.createButton(ID_COMPOSITE_EDITOR_ENTITIES_ADD, "+", canEditEntities && !limitReached);
}

void CompositeEditorPanel::fillChildrenGroup(PropertyContainerControlBase &group, const CompositeEditorTreeDataNode &treeDataNode,
  bool canEditChildren)
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

    PropertyContainerControlBase *extensible = group.createExtensible(ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_ACTIONS_FIRST + nodeIndex);
    // We allow insertion even if the display limit is reached because the new node is inserted before the selected one.
    extensible->setIntValue((1 << EXT_BUTTON_INSERT) | (1 << EXT_BUTTON_REMOVE));

    const char *name = treeDataSubNode.getName();
    const bool canEditAssetName = treeDataSubNode.canEditAssetName(/*isRootNode = */ false);
    extensible->createButton(ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_FIRST + nodeIndex, name, canEditAssetName);
  }

  group.createButton(ID_COMPOSITE_EDITOR_CHILDREN_ADD, "+", canEditChildren && !limitReached);
}

void CompositeEditorPanel::fillParametersGroup(PropertyContainerControlBase &group, const CompositeEditorTreeDataNode &treeDataNode,
  bool canEditParameters)
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

      group.createPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_ROTATION, "Rotation, deg", rotation);
      group.createPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_LOCATION, "Location", position);
      group.createPoint3(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_TM_SCALE, "Scale", scale);
      group.createSeparator();
    }

    const int childCountBeforeFirstDynamicParameter = group.getChildCount();

    { // Randomized transform parameters.
      Point2 p2;
      if (treeDataNode.tryGetPoint2Parameter("rot_x", p2))
        group.createPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_X, "rot_x", p2, 2, !useTransformationMatrix);
      if (treeDataNode.tryGetPoint2Parameter("rot_y", p2))
        group.createPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_Y, "rot_y", p2, 2, !useTransformationMatrix);
      if (treeDataNode.tryGetPoint2Parameter("rot_z", p2))
        group.createPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ROT_Z, "rot_z", p2, 2, !useTransformationMatrix);

      if (treeDataNode.tryGetPoint2Parameter("offset_x", p2))
        group.createPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_X, "offset_x", p2, 2, !useTransformationMatrix);
      if (treeDataNode.tryGetPoint2Parameter("offset_y", p2))
        group.createPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_Y, "offset_y", p2, 2, !useTransformationMatrix);
      if (treeDataNode.tryGetPoint2Parameter("offset_z", p2))
        group.createPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_OFFSET_Z, "offset_z", p2, 2, !useTransformationMatrix);

      if (treeDataNode.tryGetPoint2Parameter("scale", p2))
        group.createPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_SCALE, "scale", p2, 2, !useTransformationMatrix);
      if (treeDataNode.tryGetPoint2Parameter("yScale", p2))
        group.createPoint2(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_SCALE_Y, "yScale", p2, 2, !useTransformationMatrix);

      if (group.getChildCount() != childCountBeforeFirstDynamicParameter)
      {
        if (useTransformationMatrix)
        {
          PropertyContainerControlBase *grpWarning = group.createGroupBox(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_WARNING_GRP, "WARNING");

          if (grpWarning)
          {
            grpWarning->createStatic(-1, "Node can't have matrix and randomized");
            grpWarning->createStatic(-1, "transforms at the same time. Choose one!");
          }
        }
      }
    }

    const int placeTypeParamIndex = treeDataNode.params.findParam("place_type");
    if (placeTypeParamIndex >= 0 && treeDataNode.params.getParamType(placeTypeParamIndex) == DataBlock::TYPE_INT)
    {
      PropertyContainerControlBase &placeGrp =
        *group.createRadioGroup(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_PLACE_TYPE, "Place on collision");

      placeGrp.createRadio(ICompositObj::Props::PT_none, "-- no --");
      placeGrp.createRadio(ICompositObj::Props::PT_coll, "Place pivot");
      placeGrp.createRadio(ICompositObj::Props::PT_collNorm, "Place pivot and use normal");
      placeGrp.createRadio(ICompositObj::Props::PT_3pod, "Place 3-point (bbox)");
      placeGrp.createRadio(ICompositObj::Props::PT_fnd, "Place foundation (bbox)");
      placeGrp.createRadio(ICompositObj::Props::PT_flt, "Place on water (floatable)");
      placeGrp.createRadio(ICompositObj::Props::PT_riColl, "Place pivot with rendinst collision");
      group.setInt(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_PLACE_TYPE, treeDataNode.params.getInt(placeTypeParamIndex));
    }

    const int aboveHtParamIndex = treeDataNode.params.findParam("aboveHt");
    if (aboveHtParamIndex >= 0 && treeDataNode.params.getParamType(aboveHtParamIndex) == DataBlock::TYPE_REAL)
    {
      const float value = treeDataNode.params.getReal(aboveHtParamIndex);
      group.createEditFloat(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ABOVE_HT, "Above height", value);
    }

    if (group.getChildCount() != childCountBeforeFirstDynamicParameter)
      group.createSeparator();
  }

  group.createButton(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ADD, "Add params", canEditParameters);
  group.createButton(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_REMOVE, "Remove params", canEditParameters, false);
  group.createSeparator();
  group.createButton(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_COPY, "Copy params", canEditParameters);
  group.createButton(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_PASTE, "Paste params", canEditParameters, false);
}

void CompositeEditorPanel::fillInternal(const CompositeEditorTreeDataNode &treeDataNode, bool isRootNode)
{
  if (treeDataNode.isEntBlock())
  {
    PropertyContainerControlBase *grpEntity = createGroup(ID_COMPOSITE_EDITOR_ENTITY_GRP, "Entity");
    if (grpEntity)
      fillEntityGroup(*grpEntity, treeDataNode);
  }
  else
  {
    PropertyContainerControlBase *grpEntities = createGroup(ID_COMPOSITE_EDITOR_ENTITIES_GRP, "Entities");
    if (grpEntities)
      fillEntitiesGroup(*grpEntities, treeDataNode, treeDataNode.canEditRandomEntities(isRootNode));
  }

  PropertyContainerControlBase *grpChildren = createGroup(ID_COMPOSITE_EDITOR_CHILDREN_GRP, "Children");
  if (grpChildren)
    fillChildrenGroup(*grpChildren, treeDataNode, treeDataNode.canEditChildren());

  PropertyContainerControlBase *grpParameters = createGroup(ID_COMPOSITE_EDITOR_NODE_PARAMETERS_GRP, "Node parameters");
  if (grpParameters)
  {
    const bool canEditParameters = !isRootNode && !treeDataNode.isEntBlock();
    fillParametersGroup(*grpParameters, treeDataNode, canEditParameters);
  }

  PropertyContainerControlBase *grpComposit = createGroup(ID_COMPOSITE_EDITOR_COMPOSIT_GRP, "Composit");
  if (grpComposit)
  {
    grpComposit->createButton(ID_COMPOSITE_EDITOR_COMPOSIT_SAVE_CHANGES, "Save changes");
    grpComposit->createButton(ID_COMPOSITE_EDITOR_COMPOSIT_RESET_TO_FILE, "Reset to file", true, false);
  }

  createButton(ID_COMPOSITE_EDITOR_DELETE_NODE, "Delete node", !isRootNode);
}

void CompositeEditorPanel::fill(const CompositeEditorTreeData &treeData, const CompositeEditorTreeDataNode *selectedTreeDataNode)
{
  panelState.reset();
  saveState(panelState);

  showPanel(false);
  clear();

  if (treeData.isComposite && selectedTreeDataNode)
  {
    const bool isRootNode = selectedTreeDataNode == &treeData.rootNode;
    fillInternal(*selectedTreeDataNode, isRootNode);
  }

  showPanel(true);
  loadState(panelState);
}

void CompositeEditorPanel::updateTransformParams(const CompositeEditorTreeData &treeData,
  const CompositeEditorTreeDataNode *selectedTreeDataNode)
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
  }
}

CompositeEditorRefreshType CompositeEditorPanel::onAddNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode)
{
  Tab<String> availableVars;
  for (int i = 0; i < supportedNodeParameters.paramCount(); ++i)
    if (treeDataNode.params.findParam(supportedNodeParameters.getParamName(i)) < 0)
      availableVars.push_back(String(supportedNodeParameters.getParamName(i)));

  Tab<String> selectedVars;
  MultiListDialog selectVars("Parameters to add", 300, 400, availableVars, selectedVars);
  if (selectVars.showDialog() != DIALOG_ID_OK || selectedVars.size() == 0)
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
  MultiListDialog selectVars("Parameters to remove", 300, 400, availableVars, selectedVars);
  if (selectVars.showDialog() != DIALOG_ID_OK || selectedVars.size() == 0)
    return CompositeEditorRefreshType::Nothing;

  makeUndoForPropertyEditing();

  for (const String &variableName : selectedVars)
  {
    G_VERIFY(treeDataNode.params.removeParam(variableName));
  }

  return CompositeEditorRefreshType::EntityAndCompositeEditor;
}

CompositeEditorRefreshType CompositeEditorPanel::onCopyNodeParametersClicked(const CompositeEditorTreeDataNode &treeDataNode)
{
  SimpleString text = blk_util::blkTextData(treeDataNode.params);
  clipboard::set_clipboard_utf8_text(text.c_str());

  return CompositeEditorRefreshType::Nothing;
}

CompositeEditorRefreshType CompositeEditorPanel::onPasteNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode)
{
  const int maxLength = 128 * 1024; // It's unlikely that a composit blk file would be larger than 128 KB.
  Tab<char> buffer;
  buffer.resize(maxLength);
  if (!clipboard::get_clipboard_utf8_text(&buffer[0], buffer.size()))
    return CompositeEditorRefreshType::Nothing;

  DataBlock block;
  if (!block.loadText(&buffer[0], strlen(&buffer[0])))
    return CompositeEditorRefreshType::Nothing;

  makeUndoForPropertyEditing();

  treeDataNode.params.setParamsFrom(&block);

  return CompositeEditorRefreshType::EntityAndCompositeEditor;
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
        treeDataNode.params.setTm("tm", TMatrix::IDENT);
      }
      else
      {
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
    if (action == EXT_BUTTON_INSERT)
    {
      makeUndoForPropertyEditing();
      treeDataNode.insertEntBlock(entityIndex);
    }
    else if (action == EXT_BUTTON_REMOVE)
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
    if (action == EXT_BUTTON_INSERT)
    {
      makeUndoForPropertyEditing();
      treeDataNode.insertNodeBlock(entityIndex);
    }
    else if (action == EXT_BUTTON_REMOVE)
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
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_ADD)
  {
    return onAddNodeParametersClicked(treeDataNode);
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_REMOVE)
  {
    return onRemoveNodeParametersClicked(treeDataNode);
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_COPY)
  {
    return onCopyNodeParametersClicked(treeDataNode);
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_NODE_PARAMETERS_PASTE)
  {
    return onPasteNodeParametersClicked(treeDataNode);
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
