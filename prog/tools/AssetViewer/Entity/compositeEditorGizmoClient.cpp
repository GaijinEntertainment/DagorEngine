// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorGizmoClient.h"
#include "compositeEditorCopyDlg.h"
#include "compositeEditorViewport.h"
#include "entityViewPluginInterface.h"
#include "../av_appwnd.h"
#include <de3_objEntity.h>
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_input.h>
#include <math/dag_mathAng.h>

void CompositeEditorGizmoClient::setEntity(IObjEntity *in_entity) { entity = in_entity; }

Point3 CompositeEditorGizmoClient::getPt()
{
  const CompositeEditorTreeDataNode *node = get_app().getCompositeEditor().getSelectedTreeDataNode();
  if (!node)
    return Point3::ZERO;

  if (expectedPositionSet && get_app().isGizmoOperationStarted())
    return expectedPosition;

  const IEditorCoreEngine::CenterType centerType = IEditorCoreEngine::get()->getGizmoCenterType();
  if (centerType == IEditorCoreEngine::CENTER_Selection || !node->canTransform())
  {
    CompositeEditor &compositeEditor = get_app().getCompositeEditor();
    Point3 selectionCenter = Point3::ZERO;
    int validCount = 0;
    for (const AffectedNode &an : affectedNodes)
    {
      if (!an.canTransform)
        continue;
      IObjEntity *subEntity = compositeEditor.getSubEntityByDataBlockId(an.node->dataBlockId);
      if (subEntity)
      {
        TMatrix tm;
        subEntity->getTm(tm);
        selectionCenter += tm.getcol(3);
        ++validCount;
      }
    }
    if (validCount > 0)
      return selectionCenter / float(validCount);
    if (!node->canTransform())
      return Point3::ZERO;
  }

  if (entity)
  {
    IObjEntity *selectedSubEntity = CompositeEditorViewport::getSelectedSubEntity(entity);
    if (selectedSubEntity)
    {
      TMatrix tm;
      selectedSubEntity->getTm(tm);
      return tm.getcol(3);
    }
  }

  const TMatrix tm = node->getTransformationMatrix();
  return tm.getcol(3);
}

bool CompositeEditorGizmoClient::getRot(Point3 &p)
{
  const CompositeEditorTreeDataNode *node = get_app().getCompositeEditor().getSelectedTreeDataNode();
  if (!node || !node->canTransform())
    return false;

  const TMatrix tm = node->getTransformationMatrix();
  ::matrix_to_euler(tm, p.y, p.z, p.x);
  return true;
}

bool CompositeEditorGizmoClient::getScl(Point3 &p)
{
  const CompositeEditorTreeDataNode *node = get_app().getCompositeEditor().getSelectedTreeDataNode();
  if (!node || !node->canTransform())
    return false;

  const TMatrix tm = node->getTransformationMatrix();
  p.x = tm.getcol(0).length();
  p.y = tm.getcol(1).length();
  p.z = tm.getcol(2).length();
  return true;
}

bool CompositeEditorGizmoClient::getAxes(Point3 &ax, Point3 &ay, Point3 &az)
{
  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  unsigned dataBlockId = compositeEditor.getSelectedTreeNodeDataBlockId();
  CompositeEditorTreeDataNode *node = compositeEditor.getTreeNodeByDataBlockId(dataBlockId);

  IEditorCoreEngine::BasisType gizmoBasis = IEditorCoreEngine::get()->getGizmoBasisType();
  const bool canTransform = (node && node->canTransform());

  if (gizmoBasis == IEditorCoreEngine::BASIS_Local && canTransform)
  {
    TMatrix tm = node->getTransformationMatrix();

    TMatrix tmParent = TMatrix::IDENT;
    int nodeIndex = -1;
    CompositeEditorTreeDataNode *parent = compositeEditor.getTreeDataNodeParent(node, nodeIndex);
    if (parent)
    {
      tmParent = compositeEditor.calcParentMatrix(parent);
      tm = tmParent * tm;
    }

    ax = normalize(tm.getcol(0));
    ay = normalize(tm.getcol(1));
    az = normalize(tm.getcol(2));
  }
  else if (gizmoBasis == IEditorCoreEngine::BASIS_Parent && canTransform)
  {
    TMatrix tmParent = TMatrix::IDENT;
    int nodeIndex = -1;
    CompositeEditorTreeDataNode *parent = compositeEditor.getTreeDataNodeParent(node, nodeIndex);
    if (parent)
      tmParent = compositeEditor.calcParentMatrix(parent);

    ax = normalize(tmParent.getcol(0));
    ay = normalize(tmParent.getcol(1));
    az = normalize(tmParent.getcol(2));
  }
  else
  {
    ax = Point3(1, 0, 0);
    ay = Point3(0, 1, 0);
    az = Point3(0, 0, 1);
  }

  return true;
}

void CompositeEditorGizmoClient::refreshAffectedNodes(const dag::Vector<CompositeEditorTreeDataNode *> &selected_nodes)
{
  affectedNodes.clear();
  for (CompositeEditorTreeDataNode *node : selected_nodes)
  {
    AffectedNode an;
    an.node = node;
    an.canTransform = node && node->canTransform();
    affectedNodes.push_back(an);
  }

  // Disable nodes that are descendants of another transformable node in the selection
  // (the ancestor's transform will propagate to the descendant automatically).
  for (int i = 0; i < (int)affectedNodes.size(); ++i)
  {
    if (!affectedNodes[i].canTransform)
      continue;
    for (int j = 0; j < (int)affectedNodes.size(); ++j)
    {
      if (i == j || !affectedNodes[j].canTransform)
        continue;
      if (affectedNodes[j].node->isAncestorOfNode(affectedNodes[i].node->dataBlockId))
      {
        affectedNodes[i].canTransform = false;
        break;
      }
    }
  }
}

bool CompositeEditorGizmoClient::hasAnyTransformableNode() const
{
  for (const AffectedNode &an : affectedNodes)
    if (an.canTransform)
      return true;
  return false;
}

void CompositeEditorGizmoClient::changed(const Point3 &delta)
{
  if (!hasAnyTransformableNode())
    return;

  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  const IEditorCoreEngine::ModeType mode = IEditorCoreEngine::get()->getGizmoModeType();
  const IEditorCoreEngine::CenterType center = IEditorCoreEngine::get()->getGizmoCenterType();

  TMatrix rot, scaleMtx;
  if (mode == IEditorCoreEngine::MODE_Rotate)
  {
    // delta components are angles around the gizmo's ax/ay/az axes, not world axes.
    // Transform to world space using the axes cached at drag start.
    const TMatrix localRot = ::rotyTM(-delta.y) * ::rotzTM(-delta.z) * ::rotxTM(-delta.x);
    rot = rotationAxesTm * localRot * inverse(rotationAxesTm);
  }
  else if (mode == IEditorCoreEngine::MODE_Scale)
  {
    scaleMtx = TMatrix::IDENT;
    scaleMtx.setcol(0, Point3(delta.x, 0, 0));
    scaleMtx.setcol(1, Point3(0, delta.y, 0));
    scaleMtx.setcol(2, Point3(0, 0, delta.z));
  }

  Point3 selectionCenter = Point3::ZERO;
  if (
    center == IEditorCoreEngine::CENTER_Selection && (mode == IEditorCoreEngine::MODE_Rotate || mode == IEditorCoreEngine::MODE_Scale))
  {
    int count = 0;
    for (const AffectedNode &an : affectedNodes)
      if (an.canTransform)
      {
        selectionCenter += an.originalWorldTm.getcol(3);
        ++count;
      }
    if (count > 0)
      selectionCenter /= float(count);
  }

  pendingNodes.clear();
  pendingLocalTms.clear();

  bool primaryChanged = false;

  for (const AffectedNode &an : affectedNodes)
  {
    if (!an.canTransform)
      continue;

    TMatrix worldTm;
    if (mode == IEditorCoreEngine::MODE_Move)
    {
      // Move delta is incremental (per-frame), so start from the current position.
      worldTm = an.parentWorldTm * an.node->getTransformationMatrix();
      moveNode(worldTm, delta);
    }
    else if (mode == IEditorCoreEngine::MODE_Rotate)
    {
      rotateNode(worldTm, an.originalWorldTm, rot);
      if (center == IEditorCoreEngine::CENTER_Selection)
        worldTm.setcol(3, selectionCenter + rot * (an.originalWorldTm.getcol(3) - selectionCenter));
      else
        worldTm.setcol(3, an.originalWorldTm.getcol(3));
    }
    else if (mode == IEditorCoreEngine::MODE_Scale)
    {
      scaleNode(worldTm, an.originalWorldTm, scaleMtx);
      if (center == IEditorCoreEngine::CENTER_Selection)
      {
        const Point3 offset = an.originalWorldTm.getcol(3) - selectionCenter;
        worldTm.setcol(3, selectionCenter + Point3(offset.x * delta.x, offset.y * delta.y, offset.z * delta.z));
      }
    }
    else
      continue;

    const TMatrix newLocalTm = inverse(an.parentWorldTm) * worldTm;
    if (newLocalTm != an.node->getTransformationMatrix())
    {
      pendingNodes.push_back(an.node);
      pendingLocalTms.push_back(newLocalTm);
      if (an.movesExpectedPosition)
        primaryChanged = true;
    }
  }

  if (!pendingNodes.empty())
  {
    if (mode == IEditorCoreEngine::MODE_Move && primaryChanged)
    {
      G_ASSERT(expectedPositionSet);
      expectedPosition += delta;
    }
    compositeEditor.updateMultipleNodesTransforms(pendingNodes, pendingLocalTms);
  }
}

void CompositeEditorGizmoClient::gizmoStarted()
{
  expectedPositionSet = false;
  cloning = false;

  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  const CompositeEditorTreeDataNode *primaryNode = compositeEditor.getSelectedTreeDataNode();
  if (!hasAnyTransformableNode())
    return;

  expectedPosition = getPt();
  expectedPositionSet = true;
  cloning = ec_is_shift_key_down() && IEditorCoreEngine::get()->getGizmoModeType() == IEditorCoreEngine::MODE_Move;

  compositeEditor.beginUndo();

  G_ASSERTF_ONCE(!compositeEditor.getPreventUiUpdatesWhileUsingGizmo(), "gizmoEnded has not been called. (Non-fatal error.)");
  compositeEditor.setPreventUiUpdatesWhileUsingGizmo(true);

  // Cache world transforms now so changed() can work in world space.
  for (AffectedNode &an : affectedNodes)
  {
    if (!an.canTransform)
      continue;
    int nodeIndex = -1;
    CompositeEditorTreeDataNode *parent = compositeEditor.getTreeDataNodeParent(an.node, nodeIndex);
    an.parentWorldTm = parent ? compositeEditor.calcParentMatrix(parent) : TMatrix::IDENT;
    an.originalWorldTm = an.parentWorldTm * an.node->getTransformationMatrix();
  }

  // Cache gizmo axes for rotation so changed() rotates around the correct axes throughout the drag,
  // rather than the updating TM that causes spinning.
  rotationAxesTm = TMatrix::IDENT;
  const IEditorCoreEngine::BasisType basis = IEditorCoreEngine::get()->getGizmoBasisType();
  if (basis == IEditorCoreEngine::BASIS_Local || basis == IEditorCoreEngine::BASIS_Parent)
  {
    for (const AffectedNode &an : affectedNodes)
    {
      if (!an.canTransform || an.node != primaryNode)
        continue;
      const TMatrix &src = (basis == IEditorCoreEngine::BASIS_Local) ? an.originalWorldTm : an.parentWorldTm;
      rotationAxesTm.setcol(0, normalize(src.getcol(0)));
      rotationAxesTm.setcol(1, normalize(src.getcol(1)));
      rotationAxesTm.setcol(2, normalize(src.getcol(2)));
      rotationAxesTm.setcol(3, Point3(0, 0, 0));
      break;
    }
  }

  if (cloning)
  {
    // clones[i] matches affectedNodes[i] by selection index (see cloneSelectedNodes contract).
    dag::Vector<CompositeEditorTreeDataNode *> clones;
    compositeEditor.cloneSelectedNodes(clones);
    for (int i = 0; i < (int)clones.size() && i < (int)affectedNodes.size(); ++i)
    {
      if (!clones[i])
        continue;
      affectedNodes[i].node = clones[i];
      if (!affectedNodes[i].canTransform && clones[i]->canTransform())
      {
        // The clone of a selected node's descendant stays in the old hierarchy when its direct parent wasn't selected.
        int nodeIndex = -1;
        CompositeEditorTreeDataNode *parent = compositeEditor.getTreeDataNodeParent(clones[i], nodeIndex);
        if (eastl::find(clones.begin(), clones.end(), parent) == clones.end())
        {
          affectedNodes[i].canTransform = true;
          affectedNodes[i].parentWorldTm = parent ? compositeEditor.calcParentMatrix(parent) : TMatrix::IDENT;
          affectedNodes[i].originalWorldTm = affectedNodes[i].parentWorldTm * clones[i]->getTransformationMatrix();
        }
      }
    }
    cloneStartPosition = expectedPosition;
  }

  primaryNode = compositeEditor.getSelectedTreeDataNode();
  const bool primaryCanTransform = primaryNode && primaryNode->canTransform();
  for (AffectedNode &an : affectedNodes)
    an.movesExpectedPosition = !primaryCanTransform || an.node == primaryNode || an.node->isAncestorOfNode(primaryNode->dataBlockId);
}

static bool has_transformable_clone_ancestor(CompositeEditorTreeDataNode *start_node,
  const dag::Vector<CompositeEditorTreeDataNode *> &clones, CompositeEditor &composite_editor)
{
  CompositeEditorTreeDataNode *ancestor = start_node;
  while (ancestor)
  {
    if (ancestor->canTransform() && eastl::find(clones.begin(), clones.end(), ancestor) != clones.end())
      return true;
    int ancestorIdx = -1;
    ancestor = composite_editor.getTreeDataNodeParent(ancestor, ancestorIdx);
  }
  return false;
}

void CompositeEditorGizmoClient::gizmoEnded(bool apply)
{
  if (cloning)
  {
    if (apply)
    {
      CompositeEditorCopyDlg copyDlg;
      const int cloneCount = copyDlg.execute();
      if (cloneCount > 1)
      {
        const Point3 delta = expectedPosition - cloneStartPosition;

        for (int cloneIndex = 1; cloneIndex < cloneCount; ++cloneIndex)
        {
          CompositeEditor &compositeEditor = get_app().getCompositeEditor();

          dag::Vector<CompositeEditorTreeDataNode *> newClones;
          compositeEditor.cloneSelectedNodes(newClones);

          dag::Vector<CompositeEditorTreeDataNode *> nodesToUpdate;
          dag::Vector<TMatrix> newTms;
          for (int ci = 0; ci < (int)newClones.size(); ++ci)
          {
            CompositeEditorTreeDataNode *clone = newClones[ci];
            if (!clone || !clone->canTransform())
              continue;
            // Skip clones that have a transformable ancestor clone; that ancestor carries them via the hierarchy.
            int nodeIndex = -1;
            CompositeEditorTreeDataNode *parent = compositeEditor.getTreeDataNodeParent(clone, nodeIndex);
            if (has_transformable_clone_ancestor(parent, newClones, compositeEditor))
              continue;
            const TMatrix parentWorldTm = parent ? compositeEditor.calcParentMatrix(parent) : TMatrix::IDENT;
            TMatrix worldTm = parentWorldTm * clone->getTransformationMatrix();
            worldTm.setcol(3, worldTm.getcol(3) + delta);
            nodesToUpdate.push_back(clone);
            newTms.push_back(inverse(parentWorldTm) * worldTm);
          }
          compositeEditor.updateMultipleNodesTransforms(nodesToUpdate, newTms);
        }
      }
      else if (cloneCount < 1)
        apply = false;
    }

    get_app().getCompositeEditor().endUndo("Composit Editor: Gizmo cloning", apply);
  }
  else
    get_app().getCompositeEditor().endUndo("Composit Editor: Gizmo transform", apply);

  get_app().getCompositeEditor().setPreventUiUpdatesWhileUsingGizmo(false, cloning);

  expectedPositionSet = false;
  cloning = false;
}

void CompositeEditorGizmoClient::release() {}

bool CompositeEditorGizmoClient::canStartChangeAt(IGenViewportWnd *wnd, int x, int y, int gizmo_sel)
{
  return get_app().getCompositeEditor().getEntityViewPluginInterface().isMouseOverSelectedCompositeSubEntity(wnd, x, y, entity);
}

bool CompositeEditorGizmoClient::isMouseOver(IGenViewportWnd *wnd, int x, int y)
{
  return get_app().getCompositeEditor().getEntityViewPluginInterface().isMouseOverSelectedCompositeSubEntity(wnd, x, y, entity);
}

void CompositeEditorGizmoClient::moveNode(TMatrix &tm, const Point3 &delta) { tm.setcol(3, tm.getcol(3) + delta); }

void CompositeEditorGizmoClient::rotateNode(TMatrix &tm, const TMatrix &original_world_tm, const TMatrix &rot)
{
  tm = rot * original_world_tm;
}

void CompositeEditorGizmoClient::scaleNode(TMatrix &tm, const TMatrix &original_world_tm, const TMatrix &scale_mtx)
{
  tm = original_world_tm * scale_mtx;
}

void CompositeEditorGizmoClient::setEditMode(int mode) { editMode = mode; }

int CompositeEditorGizmoClient::getAvailableTypes()
{
  int types = 0;

  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  unsigned dataBlockId = compositeEditor.getSelectedTreeNodeDataBlockId();
  CompositeEditorTreeDataNode *node = compositeEditor.getTreeNodeByDataBlockId(dataBlockId);
  switch (editMode)
  {
    case CM_OBJED_MODE_ROTATE:
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_SURF_MOVE: types |= IEditorCoreEngine::BASIS_World;
    case CM_OBJED_MODE_SCALE:
    {
      types |= IEditorCoreEngine::BASIS_Local;
      if (node)
      {
        int nodeIndex;
        CompositeEditorTreeDataNode *parent = compositeEditor.getTreeDataNodeParent(node, nodeIndex);
        if (parent && !compositeEditor.isTreeDataNodeRootNode(parent))
          types |= IEditorCoreEngine::BASIS_Parent;
      }
    }
    break;
  }

  if (types)
    types |= IEditorCoreEngine::CENTER_Pivot | IEditorCoreEngine::CENTER_Selection;

  return types;
}
