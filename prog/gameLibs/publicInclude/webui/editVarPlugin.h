//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "editVarNotifications.h"
#include <generic/dag_tab.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <webui/editorCurves.h>


class DataBlock;

struct GuiControlDescWebUi
{
  enum Type
  {
    GCDT_Float,
    GCDT_Int,
    GCDT_IntEnum,
    GCDT_Bool,
    GCDT_BoolSingle,
    GCDT_E3dcolor,
    GCDT_Point2,
    GCDT_Point3,
    GCDT_IPoint2,
    GCDT_IPoint3,
    GCDT_Curve,
  };
  union
  {
    void *vPtr;
    float *vFloat;
    int *vInt;
    bool *vBool;
    Point2 *vPoint2;
    Point3 *vPoint3;
    IPoint2 *vIPoint2;
    IPoint3 *vIPoint3;
    EditorCurve *vCurve;
  };
  const char *name;
  const char *layout;
  bool prevBool;
  int prevInt;
  float prevFloat;
  Point2 prevPoint2;
  Point3 prevPoint3;
  IPoint2 prevIPoint2;
  IPoint3 prevIPoint3;
  SimpleString prevCurveAsText;
  int curveType;
  bool enabled;
  Type type;
  int iVals[32];
  IEditableVariablesNotifications *onChangeCb;
  bool setFromPrev() const
  {
    switch (type)
    {
      case GCDT_Float: *vFloat = prevFloat; return true;
      case GCDT_E3dcolor:
      case GCDT_Int:
      case GCDT_IntEnum: *vInt = prevInt; return true;
      case GCDT_Bool:
      case GCDT_BoolSingle: *vBool = prevBool; return true;
      case GCDT_Point2: *vPoint2 = prevPoint2; return true;
      case GCDT_Point3: *vPoint3 = prevPoint3; return true;
      case GCDT_IPoint2: *vIPoint2 = prevIPoint2; return true;
      case GCDT_IPoint3: *vIPoint3 = prevIPoint3; return true;
      default:
      case GCDT_Curve: return false;
    }
  }

  void reset()
  {
    name = NULL;
    layout = NULL;
    prevBool = false;
    prevInt = 0;
    prevFloat = 0.f;
    prevPoint2 = Point2(0.f, 0.f);
    prevPoint3 = Point3(0.f, 0.f, 0.f);
    prevIPoint2 = IPoint2(0, 0);
    prevIPoint3 = IPoint3(0, 0, 0);
    prevCurveAsText = "";
    curveType = 0;
    enabled = false;
    type = GCDT_Int;
    memset(iVals, 0, sizeof(iVals));
    onChangeCb = NULL;
  }
};

#define DECLARE_INT_EDITBOX(P, V, MIN_V, MAX_V, DEF_V)                                                                 \
  {&((P).V), #P "." #V, "int_editbox%" #P "%" #V "%" #MIN_V "%" #MAX_V "%<def>%:", false, int(DEF_V), 0, Point2(0, 0), \
    Point3(0, 0, 0), IPoint2(0, 0), IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_Int, {0}, NULL}

#define DECLARE_INT_SLIDER(P, V, MIN_V, MAX_V, DEF_V)                                                                 \
  {&((P).V), #P "." #V, "int_slider%" #P "%" #V "%" #MIN_V "%" #MAX_V "%<def>%:", false, int(DEF_V), 0, Point2(0, 0), \
    Point3(0, 0, 0), IPoint2(0, 0), IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_Int, {0}, NULL}

#define DECLARE_FLOAT_EDITBOX(P, V, MIN_V, MAX_V, DEF_V)                                                                   \
  {&((P).V), #P "." #V, "float_editbox%" #P "%" #V "%" #MIN_V "%" #MAX_V "%<def>%:", false, 0, float(DEF_V), Point2(0, 0), \
    Point3(0, 0, 0), IPoint2(0, 0), IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_Float, {0}, NULL}

#define DECLARE_POINT2_EDITBOX(P, V, DEF_V_X, DEF_V_Y)                                                                  \
  {&((P).V), #P "." #V, "point2_editbox%" #P "%" #V "%<def>%:", false, 0, 0, Point2(DEF_V_X, DEF_V_Y), Point3(0, 0, 0), \
    IPoint2(0, 0), IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_Point2, {0}, NULL}

#define DECLARE_IPOINT2_EDITBOX(P, V, DEF_V_X, DEF_V_Y)                                                      \
  {&((P).V), #P "." #V, "ipoint2_editbox%" #P "%" #V "%<def>%:", false, 0, 0, Point2(0, 0), Point3(0, 0, 0), \
    IPoint2(DEF_V_X, DEF_V_Y), IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_IPoint2, {0}, NULL}

#define DECLARE_POINT3_EDITBOX(P, V, DEF_V_X, DEF_V_Y, DEF_V_Z)                                                               \
  {&((P).V), #P "." #V, "point3_editbox%" #P "%" #V "%<def>%:", false, 0, 0, Point2(0, 0), Point3(DEF_V_X, DEF_V_Y, DEF_V_Z), \
    IPoint2(0, 0), IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_Point3, {0}, NULL}

#define DECLARE_IPOINT3_EDITBOX(P, V, DEF_V_X, DEF_V_Y, DEF_V_Z)                                                            \
  {&((P).V), #P "." #V, "ipoint3_editbox%" #P "%" #V "%<def>%:", false, 0, 0, Point2(0, 0), Point3(0, 0, 0), IPoint2(0, 0), \
    IPoint3(DEF_V_X, DEF_V_Y, DEF_V_Z), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_IPoint3, {0}, NULL}

#define DECLARE_FLOAT_SLIDER(P, V, MIN_V, MAX_V, DEF_V, STEP)                                                                       \
  {&((P).V), #P "." #V, "float_slider%" #P "%" #V "%" #MIN_V "%" #MAX_V "%<def>%" #STEP "%:", false, 0, float(DEF_V), Point2(0, 0), \
    Point3(0, 0, 0), IPoint2(0, 0), IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_Float, {0}, NULL}

#define DECLARE_BOOL_CHECKBOX(P, V, DEF_V)                                                                                      \
  {&((P).V), #P "." #V, "bool_checkbox%" #P "%" #V "%<def>%:", bool(DEF_V), 0, 0, Point2(0, 0), Point3(0, 0, 0), IPoint2(0, 0), \
    IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_Bool, {0}, NULL}

#define DECLARE_BOOL_BUTTON(P, V, DEF_V)                                                                                      \
  {&((P).V), #P "." #V, "bool_button%" #P "%" #V "%<def>%:", bool(DEF_V), 0, 0, Point2(0, 0), Point3(0, 0, 0), IPoint2(0, 0), \
    IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_BoolSingle, {0}, NULL}

#define DECLARE_INT_COMBOBOX(P, V, DEF_V, ...)                                                                                      \
  {&((P).V), #P "." #V, "int_combobox%" #P "%" #V "%<def>%" #__VA_ARGS__ "%:", false, int(DEF_V), 0, Point2(0, 0), Point3(0, 0, 0), \
    IPoint2(0, 0), IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_IntEnum, {__VA_ARGS__}, NULL}

#define DECLARE_E3DCOLOR(P, V, DEF_V)                                                                                         \
  {&((P).V), #P "." #V, "e3dcolor%" #P "%" #V "%<def>%:", false, int(DEF_V), 0, Point2(0, 0), Point3(0, 0, 0), IPoint2(0, 0), \
    IPoint3(0, 0, 0), SimpleString(""), 0, true, GuiControlDescWebUi::GCDT_E3dcolor, {0}, NULL}

#define DECLARE_LINEAR_CURVE(P, V, DEF_V)                                                                                \
  {&((P).V), #P "." #V, "linear_curve%" #P "%" #V "%<def>%:", false, 0, 0, Point2(0, 0), Point3(0, 0, 0), IPoint2(0, 0), \
    IPoint3(0, 0, 0), SimpleString(DEF_V), EditorCurve::PIECEWISE_LINEAR, true, GuiControlDescWebUi::GCDT_Curve, {0}, NULL}

#define DECLARE_MONOTONIC_CURVE(P, V, DEF_V)                                                                                \
  {&((P).V), #P "." #V, "monotonic_curve%" #P "%" #V "%<def>%:", false, 0, 0, Point2(0, 0), Point3(0, 0, 0), IPoint2(0, 0), \
    IPoint3(0, 0, 0), SimpleString(DEF_V), EditorCurve::PIECEWISE_MONOTONIC, true, GuiControlDescWebUi::GCDT_Curve, {0}, NULL}

#define DECLARE_POLYNOM_CURVE(P, V, DEF_V)                                                                                \
  {&((P).V), #P "." #V, "polynom_curve%" #P "%" #V "%<def>%:", false, 0, 0, Point2(0, 0), Point3(0, 0, 0), IPoint2(0, 0), \
    IPoint3(0, 0, 0), SimpleString(DEF_V), EditorCurve::POLYNOM, true, GuiControlDescWebUi::GCDT_Curve, {0}, NULL}

#define DECLARE_STEPS_CURVE(P, V, DEF_V)                                                                                \
  {&((P).V), #P "." #V, "steps_curve%" #P "%" #V "%<def>%:", false, 0, 0, Point2(0, 0), Point3(0, 0, 0), IPoint2(0, 0), \
    IPoint3(0, 0, 0), SimpleString(DEF_V), EditorCurve::STEPS, true, GuiControlDescWebUi::GCDT_Curve, {0}, NULL}

#define GUI_ENABLE_OBJ(P, V)     de3_webui_enable_obj(#P "." #V, true)
#define GUI_DISABLE_OBJ(P, V)    de3_webui_enable_obj(#P "." #V, false)
#define GUI_IS_ENABLED_OBJ(P, V) de3_webui_is_obj_enabled(#P "." #V)

void de3_webui_init();
bool de3_webui_check_inited();
void de3_webui_term();
void de3_webui_build_c(const GuiControlDescWebUi *desc, int desc_num);
void de3_webui_clear_c(const GuiControlDescWebUi *desc, int desc_num);
template <int N>
inline void de3_webui_build(const GuiControlDescWebUi (&desc)[N])
{
  de3_webui_build_c(&desc[0], N);
}
template <int N>
inline void de3_webui_clear(const GuiControlDescWebUi (&desc)[N])
{
  de3_webui_clear_c(&desc[0], N);
}
void de3_webui_enable_obj(const char *obj_name, bool enable);
bool de3_webui_is_obj_enabled(const char *obj_name);
bool de3_webui_remove_object(const void *value_ptr);

void de3_webui_save_values(DataBlock &dest_blk);
void de3_webui_load_values(const DataBlock &src_blk);


class EditableVariablesNotifications : public IEditableVariablesNotifications
{
  Tab<SimpleString> strings;
  Tab<void *> ptrs;
  const char *makePermanent(const char *str_ptr);

public:
  virtual ~EditableVariablesNotifications();
  virtual void registerVariable(void *value_ptr);
  virtual void unregisterVariable(void *value_ptr);
  virtual void registerIntEditbox(int *ptr, int min_v, int max_v, const char *panel_name, const char *var_name);
  virtual void registerIntSlider(int *ptr, int min_v, int max_v, const char *panel_name, const char *var_name);
  virtual void registerFloatEditbox(float *ptr, float min_v, float max_v, float step, const char *panel_name, const char *var_name);
  virtual void registerFloatSlider(float *ptr, float min_v, float max_v, float step, const char *panel_name, const char *var_name);
  virtual void registerPoint2Editbox(Point2 *ptr, const char *panel_name, const char *var_name);
  virtual void registerIPoint2Editbox(IPoint2 *ptr, const char *panel_name, const char *var_name);
  virtual void registerPoint3Editbox(Point3 *ptr, const char *panel_name, const char *var_name);
  virtual void registerIPoint3Editbox(IPoint3 *ptr, const char *panel_name, const char *var_name);
  virtual void registerE3dcolor(E3DCOLOR *ptr, const char *panel_name, const char *var_name);
  virtual void registerCurve(EditorCurve *curve, const char *panel_name, const char *var_name);
  virtual void removeVariable(void *value_ptr);
};
