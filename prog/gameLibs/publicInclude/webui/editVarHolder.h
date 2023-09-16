//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "editVarNotifications.h"
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <webui/editorCurves.h>

template <typename T>
struct EditableVariableHolder
{
  EditableVariableHolder(const T &default_value) : value{default_value} {}
  EditableVariableHolder(const EditableVariableHolder &) = delete;
  EditableVariableHolder(EditableVariableHolder &&) { G_ASSERT(false); };

  EditableVariableHolder &operator=(const EditableVariableHolder &) = delete;
  EditableVariableHolder &operator=(EditableVariableHolder &&) = delete;

  virtual ~EditableVariableHolder()
  {
    if (varNotification)
      varNotification->removeVariable(&value);
  }

  T value;
  IEditableVariablesNotifications *varNotification = nullptr;
};

struct EditableIntHolderEditbox : public EditableVariableHolder<int>
{
  EditableIntHolderEditbox(int default_value = 0) : EditableVariableHolder(default_value) {}
  void registerValue(IEditableVariablesNotifications *notifications, int min_v, int max_v, const char *panel_name,
    const char *var_name)
  {
    notifications->registerIntEditbox(&value, min_v, max_v, panel_name, var_name);
    varNotification = notifications;
  }
};

struct EditableIntHolderSlider : public EditableVariableHolder<int>
{
  EditableIntHolderSlider(int default_value = 0) : EditableVariableHolder(default_value) {}
  void registerValue(IEditableVariablesNotifications *notifications, int min_v, int max_v, const char *panel_name,
    const char *var_name)
  {
    notifications->registerIntSlider(&value, min_v, max_v, panel_name, var_name);
    varNotification = notifications;
  }
};

struct EditableFloatHolderEditbox : EditableVariableHolder<float>
{
  EditableFloatHolderEditbox(float default_value = 0.0f) : EditableVariableHolder{default_value} {}
  void registerValue(IEditableVariablesNotifications *notifications, float min_v, float max_v, float step, const char *panel_name,
    const char *var_name)
  {
    notifications->registerFloatEditbox(&value, min_v, max_v, step, panel_name, var_name);
    varNotification = notifications;
  }
};

struct EditableFloatHolderSlider : EditableVariableHolder<float>
{
  EditableFloatHolderSlider(float default_value = 0.0f) : EditableVariableHolder{default_value} {}
  void registerValue(IEditableVariablesNotifications *notifications, float min_v, float max_v, float step, const char *panel_name,
    const char *var_name)
  {
    notifications->registerFloatSlider(&value, min_v, max_v, step, panel_name, var_name);
    varNotification = notifications;
  }
};

struct EditablePoint2HolderEditbox : EditableVariableHolder<Point2>
{
  EditablePoint2HolderEditbox(const Point2 &default_value = Point2{0.0f, 0.0f}) : EditableVariableHolder{default_value} {}

  void registerValue(IEditableVariablesNotifications *notifications, const char *panel_name, const char *var_name)
  {
    notifications->registerPoint2Editbox(&value, panel_name, var_name);
    varNotification = notifications;
  }
};

struct EditableIPoint2HolderEditbox : EditableVariableHolder<IPoint2>
{
  EditableIPoint2HolderEditbox(const IPoint2 &default_value = IPoint2{0, 0}) : EditableVariableHolder{default_value} {}

  void registerValue(IEditableVariablesNotifications *notifications, const char *panel_name, const char *var_name)
  {
    notifications->registerIPoint2Editbox(&value, panel_name, var_name);
    varNotification = notifications;
  }
};

struct EditablePoint3HolderEditbox : EditableVariableHolder<Point3>
{
  EditablePoint3HolderEditbox(const Point3 &default_value = Point3{0.0f, 0.0f, 0.0f}) : EditableVariableHolder{default_value} {}

  void registerValue(IEditableVariablesNotifications *notifications, const char *panel_name, const char *var_name)
  {
    notifications->registerPoint3Editbox(&value, panel_name, var_name);
    varNotification = notifications;
  }
};

struct EditableIPoint3HolderEditbox : EditableVariableHolder<IPoint3>
{
  EditableIPoint3HolderEditbox(const IPoint3 &default_value = IPoint3{0, 0, 0}) : EditableVariableHolder{default_value} {}

  void registerValue(IEditableVariablesNotifications *notifications, const char *panel_name, const char *var_name)
  {
    notifications->registerIPoint3Editbox(&value, panel_name, var_name);
    varNotification = notifications;
  }
};

struct EditableE3dcolorHolderEditbox : EditableVariableHolder<E3DCOLOR>
{
  EditableE3dcolorHolderEditbox(const E3DCOLOR &default_value = E3DCOLOR{0U}) : EditableVariableHolder{default_value} {}

  void registerValue(IEditableVariablesNotifications *notifications, const char *panel_name, const char *var_name)
  {
    notifications->registerE3dcolor(&value, panel_name, var_name);
    varNotification = notifications;
  }
};

struct EditableCurveHolderEditbox : EditableVariableHolder<EditorCurve>
{
  EditableCurveHolderEditbox(const EditorCurve &default_value = EditorCurve{}) : EditableVariableHolder{default_value} {}

  void registerValue(IEditableVariablesNotifications *notifications, const char *panel_name, const char *var_name)
  {
    notifications->registerCurve(&value, panel_name, var_name);
    varNotification = notifications;
  }
};
