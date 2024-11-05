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
  virtual Point3 getPt() override;
  virtual bool getRot(Point3 &p) override;
  virtual bool getScl(Point3 &p) override;
  virtual void changed(const Point3 &delta) override;
  virtual void gizmoStarted() override;
  virtual void gizmoEnded(bool apply) override;
  virtual void release() override;
  virtual bool canStartChangeAt(IGenViewportWnd *wnd, int x, int y, int gizmo_sel) override;

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
