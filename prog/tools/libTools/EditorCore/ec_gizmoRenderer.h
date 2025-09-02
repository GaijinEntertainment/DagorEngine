// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_gizmofilter.h>

class IGizmoRenderer
{
public:
  IGizmoRenderer(const GizmoEventFilter *gizmo, int arrowSideSize) : gizmo(gizmo), arrowSideSize(arrowSideSize) {}
  virtual ~IGizmoRenderer() = default;
  virtual void renderMove(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel) const = 0;
  virtual void renderSurfMove(const Point2 &center, const Point2 axes[3], int sel) const = 0;
  virtual void renderScale(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel, const Point3 &scale) const = 0;
  void renderRotate(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], const Point2 &startPos2d, real angle,
    real startAngle, int sel) const;
  virtual int intersection(const Point2 &mouse, const Point2 &center, const Point2 axes[3], int selected, Point2 &rotateDir) const = 0;
  virtual void flip(IGenViewportWnd *w, const Matrix3 &basis, Point2 &ax, Point2 &ay, Point2 &az) const {}

  virtual GizmoEventFilter::Style getStyle() const = 0;

protected:
  void drawGizmoArrow(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, real offset = 0.1) const;
  void drawGizmoArrowScale(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, real offset = 0.1) const;
  const GizmoEventFilter *gizmo;
  int arrowSideSize;
  float arrowSideThicknessRatio = 0.25f;
};

class GizmoRendererNew : public IGizmoRenderer
{
public:
  GizmoRendererNew(const GizmoEventFilter *gizmo);
  void renderMove(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel) const override;
  void renderSurfMove(const Point2 &center, const Point2 axes[3], int sel) const override;
  void renderScale(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel, const Point3 &scale) const override;
  int intersection(const Point2 &mouse, const Point2 &center, const Point2 axes[3], int selected, Point2 &rotateDir) const override;

  GizmoEventFilter::Style getStyle() const override { return GizmoEventFilter::Style::New; }

private:
  void drawGizmoQuad(const Point2 &from, const Point2 &delta1, const Point2 &delta2, E3DCOLOR col, E3DCOLOR col_sel, bool sel,
    real div) const;
  void drawCentralHandle(const IGenViewportWnd *wnd, const Point2 &center, const Point2 axes[3], const E3DCOLOR col[3],
    bool isSelected) const;
};

class GizmoRendererClassic : public IGizmoRenderer
{
public:
  GizmoRendererClassic(const GizmoEventFilter *gizmo);
  void renderMove(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel) const override;
  void renderSurfMove(const Point2 &center, const Point2 axes[3], int sel) const override;
  void renderScale(IGenViewportWnd *w, const Point2 &center, const Point2 axes[3], int sel, const Point3 &scale) const override;
  int intersection(const Point2 &mouse, const Point2 &center, const Point2 axes[3], int selected, Point2 &rotateDir) const override;
  void flip(IGenViewportWnd *w, const Matrix3 &basis, Point2 &ax, Point2 &ay, Point2 &az) const override;

  GizmoEventFilter::Style getStyle() const override { return GizmoEventFilter::Style::Classic; }

private:
  void drawGizmoQuad(const Point2 &from, const Point2 &delta1, const Point2 &delta2, E3DCOLOR col1, E3DCOLOR col2, E3DCOLOR col_sel,
    bool sel, real div = 3.0) const;
};