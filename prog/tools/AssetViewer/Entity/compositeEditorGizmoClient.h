// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface.h>
#include <dag/dag_vector.h>
#include <math/dag_TMatrix.h>

class CompositeEditorTreeDataNode;
class IObjEntity;

class CompositeEditorGizmoClient : public IGizmoClient
{
public:
  void setEntity(IObjEntity *in_entity);
  IObjEntity *getEntity() { return entity; }

  void setEditMode(int mode);
  int getAvailableTypes() override;

  void refreshEffectedNodes(const dag::Vector<CompositeEditorTreeDataNode *> &selected_nodes);
  bool hasAnyTransformableNode() const;

private:
  struct EffectedNode
  {
    CompositeEditorTreeDataNode *node = nullptr;
    bool canTransform = false;
    TMatrix originalWorldTm;
    TMatrix parentWorldTm;
  };

  // IGizmoClient
  Point3 getPt() override;
  bool getRot(Point3 &p) override;
  bool getScl(Point3 &p) override;
  bool getAxes(Point3 &ax, Point3 &ay, Point3 &az) override;
  void changed(const Point3 &delta) override;
  void gizmoStarted() override;
  void gizmoEnded(bool apply) override;
  void release() override;
  bool canStartChangeAt(IGenViewportWnd *wnd, int x, int y, int gizmo_sel) override;
  bool isMouseOver(IGenViewportWnd *wnd, int x, int y) override;

  void moveNode(TMatrix &tm, const Point3 &delta);
  void rotateNode(TMatrix &tm, const TMatrix &original_world_tm, const TMatrix &rot);
  void scaleNode(TMatrix &tm, const TMatrix &original_world_tm, const TMatrix &scale_mtx);

  IObjEntity *entity = nullptr;
  dag::Vector<EffectedNode> effectedNodes;
  // Reused across changed() calls to avoid per-frame allocations.
  dag::Vector<CompositeEditorTreeDataNode *> pendingNodes;
  dag::Vector<TMatrix> pendingLocalTms;
  Point3 expectedPosition;
  bool expectedPositionSet = false;
  bool cloning = false;
  Point3 cloneStartPosition;

  int editMode = 0;

  // Gizmo axes at drag start, used by changed() to rotate around the correct axes throughout the drag.
  TMatrix rotationAxesTm = TMatrix::IDENT;
};
