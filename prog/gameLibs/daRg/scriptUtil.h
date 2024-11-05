// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat.h>
#include <util/dag_globDef.h>
#include <quirrel/sqStackChecker.h>


namespace darg
{

class Element;


void script_print_func(HSQUIRRELVM v, const SQChar *s, ...);
void script_err_print_func(HSQUIRRELVM v, const SQChar *s, ...);
void compile_error_handler(HSQUIRRELVM v, SQMessageSeverity severity, const SQChar *desc, const SQChar *source, SQInteger line,
  SQInteger column, const SQChar *);
SQInteger runtime_error_handler(HSQUIRRELVM v);
SQInteger encode_e3dcolor(HSQUIRRELVM vm);
bool decode_e3dcolor(const Sqrat::Object &val, E3DCOLOR &color, const char **err_msg);
bool decode_e3dcolor(HSQUIRRELVM vm, SQInteger idx, E3DCOLOR &color, const char **err_msg);
bool are_sq_obj_equal(HSQUIRRELVM vm, const HSQOBJECT &a, const HSQOBJECT &b);
bool is_same_immutable_obj(const Sqrat::Object &a, const Sqrat::Object &b);
bool get_closure_info(const Sqrat::Function &f, SQInteger *nparams, SQInteger *nfreevars);
void get_closure_full_name(const Sqrat::Object &func, String &out_name);

template <typename T, typename SlotKey>
T *try_cast_instance(const Sqrat::Object &obj, const Sqrat::Object &trace_container, const SlotKey &trace_key, const char *msg)
{
  SQObjectType tp = obj.GetType();
  const HSQOBJECT hObj = obj.GetObject();
  if (tp == OT_INSTANCE && Sqrat::ClassType<T>::IsObjectOfClass(&hObj))
    return obj.Cast<T *>();
  if (tp == OT_NULL)
    return nullptr;

  darg_assert_trace_var(
    String(0, "%s: Invalid type, expected instance of %s", msg ? msg : "Reading script var", Sqrat::ClassType<T>::ClassName().c_str()),
    trace_container, trace_key);
  return nullptr;
}


Point2 script_get_point2(Sqrat::Array arr);
E3DCOLOR script_get_color(Sqrat::Array arr);

// Margins, paddings, border widths etc
// Order is HTML-like clockwise (top, right, bottom, left)
bool script_parse_offsets(const Element *elem, const Sqrat::Object &obj, float out_val[4], const char **err_msg);

SQInteger locate_element_source(HSQUIRRELVM v);
SQInteger get_element_info(HSQUIRRELVM v);

void script_set_userpointer_to_slot(const Sqrat::Object &obj, const char *key, void *up);
void *script_cast_to_userpointer(const Sqrat::Object &obj);
template <typename T>
T script_cast_from_userpointer(const Sqrat::Object &obj)
{
  return (T)(intptr_t)script_cast_to_userpointer(obj);
}

bool load_sound_info(Sqrat::Object src, Sqrat::string &out_name, Sqrat::Object &out_params, float &out_vol);


} // namespace darg
