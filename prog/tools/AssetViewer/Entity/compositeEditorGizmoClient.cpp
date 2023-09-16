#include "compositeEditorGizmoClient.h"
#include "compositeEditorCopyDlg.h"
#include "CompositeEditorViewport.h"
#include "entityViewPluginInterface.h"
#include "../av_appwnd.h"
#include <de3_objEntity.h>
#include <math/dag_mathAng.h>
#include <winGuiWrapper/wgw_input.h>

void CompositeEditorGizmoClient::setEntity(IObjEntity *in_entity) { entity = in_entity; }

Point3 CompositeEditorGizmoClient::getPt()
{
  const CompositeEditorTreeDataNode *node = get_app().getCompositeEditor().getSelectedTreeDataNode();
  if (!node || !node->canTransform())
    return Point3::ZERO;

  if (entity)
  {
    IObjEntity *selectedSubEntity = CompositeEditorViewport::getSelectedSubEntity(entity);
    if (selectedSubEntity)
    {
      if (expectedPositionSet && get_app().isGizmoOperationStarted())
        return expectedPosition;

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

void CompositeEditorGizmoClient::changed(const Point3 &delta)
{
  const CompositeEditorTreeDataNode *node = get_app().getCompositeEditor().getSelectedTreeDataNode();
  if (!node || !node->canTransform())
    return;

  const IEditorCoreEngine::ModeType mode = IEditorCoreEngine::get()->getGizmoModeType();
  const IEditorCoreEngine::BasisType basis = IEditorCoreEngine::get()->getGizmoBasisType();
  const IEditorCoreEngine::CenterType center = IEditorCoreEngine::get()->getGizmoCenterType();
  TMatrix tm = node->getTransformationMatrix();

  if (mode == IEditorCoreEngine::MODE_Move)
    moveNode(tm, delta, basis, center);
  else if (mode == IEditorCoreEngine::MODE_Rotate)
    rotateNode(tm, delta, basis, center);
  else if (mode == IEditorCoreEngine::MODE_Scale)
    scaleNode(tm, delta, basis, center);

  if (tm != node->getTransformationMatrix())
  {
    if (mode == IEditorCoreEngine::MODE_Move)
    {
      G_ASSERT(expectedPositionSet);
      expectedPosition += delta;
    }

    get_app().getCompositeEditor().updateSelectedNodeTransform(tm);
  }
}

void CompositeEditorGizmoClient::gizmoStarted()
{
  expectedPositionSet = false;
  cloning = false;
  originalTm = TMatrix::IDENT;

  const CompositeEditorTreeDataNode *node = get_app().getCompositeEditor().getSelectedTreeDataNode();
  if (!node || !node->canTransform())
    return;

  expectedPosition = getPt();
  expectedPositionSet = true;
  originalTm = node->getTransformationMatrix();

  get_app().getCompositeEditor().beginUndo();

  G_ASSERTF_ONCE(!get_app().getCompositeEditor().getPreventUiUpdatesWhileUsingGizmo(),
    "gizmoEnded has not been called. (Non-fatal error.)");
  get_app().getCompositeEditor().setPreventUiUpdatesWhileUsingGizmo(true);

  if (wingw::is_key_pressed(wingw::V_SHIFT) && IEditorCoreEngine::get()->getGizmoModeType() == IEditorCoreEngine::MODE_Move)
  {
    cloning = true;
    cloneStartPosition = getPt();

    get_app().getCompositeEditor().cloneSelectedNode();
  }
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
        Point3 clonePosition = getPt();
        const Point3 delta = clonePosition - cloneStartPosition;

        for (int cloneIndex = 1; cloneIndex < cloneCount; ++cloneIndex)
        {
          clonePosition += delta;

          get_app().getCompositeEditor().cloneSelectedNode();

          TMatrix tm = get_app().getCompositeEditor().getSelectedTreeDataNode()->getTransformationMatrix();
          tm.setcol(3, clonePosition);
          get_app().getCompositeEditor().updateSelectedNodeTransform(tm);
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

void CompositeEditorGizmoClient::moveNode(TMatrix &tm, const Point3 &delta, IEditorCoreEngine::BasisType basis,
  IEditorCoreEngine::CenterType center)
{
  const Point3 pos = tm.getcol(3);
  tm.setcol(3, pos + delta);
}

void CompositeEditorGizmoClient::rotateNode(TMatrix &tm, const Point3 &delta, IEditorCoreEngine::BasisType basis,
  IEditorCoreEngine::CenterType center)
{
  const TMatrix rot = ::rotyTM(-delta.y) * ::rotzTM(-delta.z) * ::rotxTM(-delta.x);
  tm = rot * originalTm;
  tm.setcol(3, originalTm.getcol(3));
}

void CompositeEditorGizmoClient::scaleNode(TMatrix &tm, const Point3 &delta, IEditorCoreEngine::BasisType basis,
  IEditorCoreEngine::CenterType center)
{
  TMatrix scale = TMatrix::IDENT;
  scale.setcol(0, Point3(delta.x, 0, 0));
  scale.setcol(1, Point3(0, delta.y, 0));
  scale.setcol(2, Point3(0, 0, delta.z));
  tm = originalTm * scale;
}
