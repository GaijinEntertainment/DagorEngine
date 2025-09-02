// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class String;
class DataBlock;

struct GuiControlDesc
{
  enum Type
  {
    GCDT_Float,
    GCDT_Int,
    GCDT_IntEnum,
    GCDT_Bool,
    GCDT_Button,
    GCDT_Point2,
    GCDT_Point3,
    GCDT_IPoint2,
    GCDT_IPoint3,
    GCDT_SliderFloat,
    GCDT_SliderInt,
  };
  union
  {
    void *vPtr;
    int *vI32;
    float *vF32;
    bool *vBool;
  };
  union
  {
    int vMinI32;
    float vMinF32;
  };
  union
  {
    int vMaxI32;
    float vMaxF32;
  };
  union
  {
    int vDefI32[4];
    float vDefF32[4];
  };
  const char *name;
  const char *panelName;
  int type, iSel;
  int iVals[16];
  const char *sVal;

  static inline int f2i(float v)
  {
    union
    {
      float f;
      int i;
    } vv = {v};
    return vv.i;
  }
};

#define DECLARE_INT_EDITBOX(P, V, MIN_V, MAX_V, DEF_V) \
  {{&((P).V)}, {MIN_V}, {MAX_V}, {{DEF_V, 0, 0, 0}}, #V, #P, GuiControlDesc::GCDT_Int, 0, {0}, nullptr}

#define DECLARE_IPOINT2_EDITBOX(P, V, DEF_V_X, DEF_V_Y) \
  {{&((P).V)}, {0}, {0}, {{DEF_V_X, DEF_V_Y, 0, 0}}, #V, #P, GuiControlDesc::GCDT_IPoint2, 0, {0}, nullptr}

#define DECLARE_IPOINT3_EDITBOX(P, V, DEF_V_X, DEF_V_Y, DEF_V_Z) \
  {{&((P).V)}, {0}, {0}, {{DEF_V_X, DEF_V_Y, DEF_V_Z, 0}}, #V, #P, GuiControlDesc::GCDT_IPoint3, 0, {0}, nullptr}

#define DECLARE_FLOAT_EDITBOX(P, V, MIN_V, MAX_V, DEF_V)                                                                    \
  {{&((P).V)}, {GuiControlDesc::f2i(MIN_V)}, {GuiControlDesc::f2i(MAX_V)}, {{GuiControlDesc::f2i(DEF_V), 0, 0, 0}}, #V, #P, \
    GuiControlDesc::GCDT_Int, 0, {0}, nullptr}

#define DECLARE_POINT2_EDITBOX(P, V, DEF_V_X, DEF_V_Y)                                                                              \
  {{&((P).V)}, {0}, {0}, {{GuiControlDesc::f2i(DEF_V_X), GuiControlDesc::f2i(DEF_V_Y), 0, 0}}, #V, #P, GuiControlDesc::GCDT_Point2, \
    0, {0}, nullptr}

#define DECLARE_POINT3_EDITBOX(P, V, DEF_V_X, DEF_V_Y, DEF_V_Z)                                                                   \
  {{&((P).V)}, {0}, {0}, {{GuiControlDesc::f2i(DEF_V_X), GuiControlDesc::f2i(DEF_V_Y), GuiControlDesc::f2i(DEF_V_Z), 0}}, #V, #P, \
    GuiControlDesc::GCDT_Point3, 0, {0}, nullptr}

#define DECLARE_INT_SLIDER(P, V, MIN_V, MAX_V, DEF_V) \
  {{&((P).V)}, {MIN_V}, {MAX_V}, {{DEF_V, 0, 0, 0}}, #V, #P, GuiControlDesc::GCDT_SliderInt, 0, {0}, nullptr}

#define DECLARE_FLOAT_SLIDER(P, V, MIN_V, MAX_V, DEF_V, STEP)                                                               \
  {{&((P).V)}, {GuiControlDesc::f2i(MIN_V)}, {GuiControlDesc::f2i(MAX_V)}, {{GuiControlDesc::f2i(DEF_V), 0, 0, 0}}, #V, #P, \
    GuiControlDesc::GCDT_SliderFloat, 0, {0}, nullptr}

#define DECLARE_BOOL_CHECKBOX(P, V, DEF_V) \
  {{&((P).V)}, {0}, {0}, {{DEF_V, 0, 0, 0}}, #V, #P, GuiControlDesc::GCDT_Bool, 0, {0}, nullptr}

#define DECLARE_BOOL_BUTTON(P, V, DEF_V) \
  {{&((P).V)}, {0}, {0}, {{DEF_V, 0, 0, 0}}, #V, #P, GuiControlDesc::GCDT_Button, 0, {0}, nullptr}

#define DECLARE_INT_COMBOBOX(P, V, DEF_V, ...) \
  {{&((P).V)}, {0}, {0}, {{DEF_V, 0, 0, 0}}, #V, #P, GuiControlDesc::GCDT_IntEnum, 0, {__VA_ARGS__}, #__VA_ARGS__}


#define GUI_ENABLE_OBJ(P, V)     de3_imgui_enable_obj(&((P).V), true)
#define GUI_DISABLE_OBJ(P, V)    de3_imgui_enable_obj(&((P).V), false)
#define GUI_IS_ENABLED_OBJ(P, V) de3_imgui_is_obj_enabled(&((P).V))

#if 1 // GUI linking enabled
void de3_imgui_init(const char *submenu_name, const char *window_name);
void de3_imgui_term();
void de3_imgui_build(GuiControlDesc *desc, int desc_num);
bool de3_imgui_act(float dt);
void de3_imgui_before_render();
void de3_imgui_render();
void de3_imgui_enable_obj(const void *v_ptr, bool enable);
bool de3_imgui_is_obj_enabled(const void *v_ptr);
void de3_imgui_set_gui_active(bool active, bool overlay = false);

#else
// gui stabbed out
static void de3_imgui_init(const char *, const char *) {}
static void de3_imgui_term() {}
static void de3_imgui_build(GuiControlDesc *desc, int desc_num) {}
static bool de3_imgui_act(float dt) { return false; }
static void de3_imgui_before_render() {}
static void de3_imgui_render() {}
static void de3_imgui_enable_obj(const void *, bool) {}
static bool de3_imgui_is_obj_enabled(const void *) { return false; }
static void de3_imgui_set_gui_active(bool) {}
#endif

void de3_imgui_save_values(DataBlock &dest_blk);
void de3_imgui_load_values(const DataBlock &src_blk);
