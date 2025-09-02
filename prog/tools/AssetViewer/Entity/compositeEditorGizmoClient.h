// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface.h>
#include <math/dag_TMatrix.h>

class CompositeEditorTreeDataNode;
class IObjEntity;

class CompositeEditorGizmoClient : public IGizmoClient
{
public:
  void setEntity(IObjEntity *in_entity);

private:
  // IGizmoClient
  Point3 getPt() override;
  bool getRot(Point3 &p) override;
  bool getScl(Point3 &p) override;
  void changed(const Point3 &delta) override;
  void gizmoStarted() override;
  void gizmoEnded(bool apply) override;
  void release() override;
  bool canStartChangeAt(IGenViewportWnd *wnd, int x, int y, int gizmo_sel) override;
  bool isMouseOver(IGenViewportWnd *wnd, int x, int y) override;

  void moveNode(TMatrix &tm, const Point3 &delta, IEditorCoreEngine::BasisType basis, IEditorCoreEngine::CenterType center);
  void rotateNode(TMatrix &tm, const Point3 &delta, IEditorCoreEngine::BasisType basis, IEditorCoreEngine::CenterType center);
  void scaleNode(TMatrix &tm, const Point3 &delta, IEditorCoreEngine::BasisType basis, IEditorCoreEngine::CenterType center);

  IObjEntity *entity = nullptr;
  TMatrix originalTm;
  Point3 expectedPosition;
  bool expectedPositionSet = false;
  bool cloning = false;
  Point3 cloneStartPosition;
};
