
#ifndef __DAGOR_NORMALIZE_NODE_TM_H
#define __DAGOR_NORMALIZE_NODE_TM_H
#pragma once

#include <libTools/dagFileRW/dagFileNode.h>
#include <math/dag_mesh.h>


static void normalizeNodeTreeTm(Node &n, const TMatrix &parent_scale_tm, bool need_transform_mesh)
{
  const float xScale = n.tm.getcol(0).length();
  const float yScale = n.tm.getcol(1).length();
  const float zScale = n.tm.getcol(2).length();
  TMatrix scaleMatrix;
  scaleMatrix.setcol(0, Point3(xScale, 0.f, 0.f));
  scaleMatrix.setcol(1, Point3(0.f, yScale, 0.f));
  scaleMatrix.setcol(2, Point3(0.f, 0.f, zScale));
  scaleMatrix.setcol(3, ZERO<Point3>());

  const TMatrix finalScaleTm = parent_scale_tm * scaleMatrix;

  if ((n.flags & NODEFLG_RENDERABLE) && n.obj && n.obj->isSubOf(OCID_MESHHOLDER) && need_transform_mesh)
  {
    // transform mesh
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    Mesh &mesh = *mh.mesh;
    mesh.transform(finalScaleTm);
  }

  // transform tm
  const float xScaleInv = safeinv(xScale);
  const float yScaleInv = safeinv(yScale);
  const float zScaleInv = safeinv(zScale);
  const Point3 nodePos = n.tm.getcol(3);
  TMatrix scaleNodeTm;
  scaleNodeTm.setcol(0, Point3(xScaleInv, 0.f, 0.f));
  scaleNodeTm.setcol(1, Point3(0.f, yScaleInv, 0.f));
  scaleNodeTm.setcol(2, Point3(0.f, 0.f, zScaleInv));
  scaleNodeTm.setcol(3, ZERO<Point3>());

  n.tm *= scaleNodeTm;
  n.tm.setcol(3, nodePos * parent_scale_tm);
  n.invalidate_wtm();

  for (int i = 0; i < n.child.size(); ++i)
    normalizeNodeTreeTm(*n.child[i], finalScaleTm, need_transform_mesh);
}


#endif
