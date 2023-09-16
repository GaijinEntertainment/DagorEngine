//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_e3dColor.h>

class EditorCurve;

class IEditableVariablesNotifications
{
public:
  virtual ~IEditableVariablesNotifications() {}

  virtual void registerVariable(void *value_ptr) = 0;
  virtual void unregisterVariable(void *value_ptr) = 0;
  virtual void registerIntEditbox(int *ptr, int min_v, int max_v, const char *panel_name, const char *var_name) = 0;
  virtual void registerIntSlider(int *ptr, int min_v, int max_v, const char *panel_name, const char *var_name) = 0;
  virtual void registerFloatEditbox(float *ptr, float min_v, float max_v, float step, const char *panel_name,
    const char *var_name) = 0;
  virtual void registerFloatSlider(float *ptr, float min_v, float max_v, float step, const char *panel_name, const char *var_name) = 0;
  virtual void registerPoint2Editbox(Point2 *ptr, const char *panel_name, const char *var_name) = 0;
  virtual void registerIPoint2Editbox(IPoint2 *ptr, const char *panel_name, const char *var_name) = 0;
  virtual void registerPoint3Editbox(Point3 *ptr, const char *panel_name, const char *var_name) = 0;
  virtual void registerIPoint3Editbox(IPoint3 *ptr, const char *panel_name, const char *var_name) = 0;
  virtual void registerE3dcolor(E3DCOLOR *ptr, const char *panel_name, const char *var_name) = 0;
  virtual void registerCurve(EditorCurve *curve, const char *panel_name, const char *var_name) = 0;
  virtual void removeVariable(void *ptr) = 0;

  bool varChanged;
};

class EditableVariablesNotificationsStub : public IEditableVariablesNotifications
{
public:
  virtual ~EditableVariablesNotificationsStub() {}
  virtual void registerVariable(void *) {}
  virtual void unregisterVariable(void *) {}
  virtual void registerIntEditbox(int *, int, int, const char *, const char *) {}
  virtual void registerIntSlider(int *, int, int, const char *, const char *) {}
  virtual void registerFloatEditbox(float *, float, float, float, const char *, const char *) {}
  virtual void registerFloatSlider(float *, float, float, float, const char *, const char *) {}
  virtual void registerPoint2Editbox(Point2 *, const char *, const char *) {}
  virtual void registerIPoint2Editbox(IPoint2 *, const char *, const char *) {}
  virtual void registerPoint3Editbox(Point3 *, const char *, const char *) {}
  virtual void registerIPoint3Editbox(IPoint3 *, const char *, const char *) {}
  virtual void registerE3dcolor(E3DCOLOR *, const char *, const char *) {}
  virtual void registerCurve(EditorCurve *, const char *, const char *){};
  virtual void removeVariable(void *) {}
};
