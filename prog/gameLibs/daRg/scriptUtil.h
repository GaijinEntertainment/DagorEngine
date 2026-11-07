// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat.h>
#include <util/dag_globDef.h>
#include <util/dag_stdint.h>
#include <util/dag_bitwise_cast.h>
#include <quirrel/sqStackChecker.h>
#include <math/dag_Point2.h>
#include <daRg/dag_layout.h>


namespace darg
{

class Element;


void script_print_func(HSQUIRRELVM v, const char *s, ...);
void script_err_print_func(HSQUIRRELVM v, const char *s, ...);
void compile_error_handler(HSQUIRRELVM v, SQMessageSeverity severity, const char *desc, const char *source, SQInteger line,
  SQInteger column, const char *);
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


Point2 script_get_point2(Sqrat::Object obj, Point2 def);
E3DCOLOR script_get_color(Sqrat::Array arr);

// Margins, paddings, border widths etc
// Order is HTML-like clockwise (top, right, bottom, left)
bool script_parse_offsets(const Element *elem, const Sqrat::Object &obj, float out_val[4], const char **err_msg);

SQInteger locate_element_source(HSQUIRRELVM v);
SQInteger get_element_info(HSQUIRRELVM v);

void script_set_userpointer_to_slot(const Sqrat::Object &obj, const char *key, void *up);
void *script_cast_to_userpointer(const Sqrat::Object &obj);

// Mode in the 3 low mantissa bits to fit 32-bit pointers; mode 0 (PIXELS) is never encoded, so 8-aligned pointers fail
constexpr uint32_t SIZE_SPEC_MODE_MASK = 7;
static_assert(SizeSpec::ELEM_SELF_H <= SIZE_SPEC_MODE_MASK, "SizeSpec::Mode must fit SIZE_SPEC_MODE_MASK");

inline SQUserPointer encode_size_spec(SizeSpec::Mode mode, float value)
{
  G_ASSERT(mode != SizeSpec::PIXELS);
  uint32_t bits = (bitwise_cast<uint32_t>(value) & ~SIZE_SPEC_MODE_MASK) | uint32_t(mode);
  return (SQUserPointer)(uintptr_t)bits;
}

inline bool decode_size_spec(SQUserPointer up, SizeSpec &res)
{
  uintptr_t raw = (uintptr_t)up;
  uint32_t bits = uint32_t(raw);
  if ((bits & SIZE_SPEC_MODE_MASK) == SizeSpec::PIXELS || (uint64_t(raw) >> 32) != 0)
    return false;
  res.mode = SizeSpec::Mode(bits & SIZE_SPEC_MODE_MASK);
  res.value = bitwise_cast<float>(bits & ~SIZE_SPEC_MODE_MASK);
  return true;
}

bool load_sound_info(Sqrat::Object src, Sqrat::string &out_name, Sqrat::Object &out_params, float &out_vol);


} // namespace darg
