#ifndef __GAIJIN_IGIZMO_OBJECT__
#define __GAIJIN_IGIZMO_OBJECT__
#pragma once


#include <EditorCore/ec_gizmofilter.h>

#include <math/dag_math3d.h>


class IGizmoObject
{
public:
  // get object's matrix
  virtual TMatrix getTm() const = 0;
  // set object's matrix
  virtual void setTm(const TMatrix &new_tm) = 0;

  // calls before changing object's matrix
  virtual void onStartChange() = 0;

  // get object's name or any identifier to use in undo name
  virtual const char *getUndoName() const = 0;

  // get axis which object can be midified
  virtual int getAvailableAxis() const { return GizmoEventFilter::AXIS_X | GizmoEventFilter::AXIS_Y | GizmoEventFilter::AXIS_Z; }
};


#endif //__GAIJIN_IGIZMO_OBJECT__
