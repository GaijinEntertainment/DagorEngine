#include "scriptUtil.h"
#include "guiScene.h"
#include "elementRef.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>
#include <sqModules/sqModules.h>
#include <gui/dag_visualLog.h>

#include <sqext.h>

#include "dargDebugUtils.h"

namespace darg
{


void script_print_func(HSQUIRRELVM /*v*/, const SQChar *s, ...)
{
  va_list vl;
  va_start(vl, s);
  String msg(framemem_ptr());
  msg.cvprintf(0, s, vl);
  logmessage(_MAKE4C('SQRL'), msg.c_str());
  va_end(vl);
}


void script_err_print_func(HSQUIRRELVM /*v*/, const SQChar *s, ...)
{
  va_list vl;
  va_start(vl, s);
  String msg(framemem_ptr());
  msg.cvprintf(0, s, vl);
  logmessage(LOGLEVEL_ERR, msg.c_str());
  va_end(vl);
}


void compile_error_handler(HSQUIRRELVM v, const SQChar *desc, const SQChar *source, SQInteger line, SQInteger column,
  const SQChar *extra)
{
  String str;
  str.printf(128, "Squirrel compile error %s (%d:%d): %s", source, line, column, desc);
  if (extra)
  {
    str.append("\n", 1);
    str.append(extra);
  }

  GuiScene *scene = GuiScene::get_from_sqvm(v);
  scene->errorMessageWithCb(str.str());
}


SQInteger runtime_error_handler(HSQUIRRELVM v)
{
  G_ASSERT(sq_gettop(v) == 2);
  SqStackChecker stackCheck(v);

  const char *errMsg = nullptr;
  if (SQ_FAILED(sq_getstring(v, 2, &errMsg)))
    errMsg = "Unknown error";

  stackCheck.check();

  const char *callstack = nullptr;
  if (SQ_SUCCEEDED(sqstd_formatcallstackstring(v)))
  {
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, -1, &callstack)));
    G_ASSERT(callstack);
  }

  String fullMsg(0, "%s\n%s", errMsg, callstack ? callstack : "No callstack");
  if (callstack)
    sq_pop(v, 1);

  GuiScene *scene = GuiScene::get_from_sqvm(v);
  scene->errorMessageWithCb(fullMsg);

  return 0;
}


SQInteger encode_e3dcolor(HSQUIRRELVM vm)
{
  SQInteger r = 255, g = 255, b = 255, a = 255;
  SQInteger top = sq_gettop(vm);

  if (top != 4 && top != 5)
    return sq_throwerror(vm, "Color: expected 3 or 4 paramenters");

  sq_getinteger(vm, 2, &r);
  sq_getinteger(vm, 3, &g);
  sq_getinteger(vm, 4, &b);

  if (top > 4)
    SQ_SUCCEEDED(sq_getinteger(vm, 5, &a));

  E3DCOLOR c = E3DCOLOR_MAKE(r, g, b, a);
  int i = (int &)c.u;
  sq_pushinteger(vm, SQInteger(i));

  return 1;
}


bool decode_e3dcolor(const Sqrat::Object &val, E3DCOLOR &color, const char **err_msg)
{
  SQObjectType type = val.GetType();

  if (type == OT_INTEGER)
  {
    int intVal = val.Cast<int>();
    color = E3DCOLOR((unsigned int &)intVal);
    return true;
  }

  if (type == OT_INSTANCE)
  {
    if (Sqrat::ClassType<E3DCOLOR>::IsClassInstance(val.GetObject(), false))
      color = val.Cast<E3DCOLOR>();
    else if (Sqrat::ClassType<Color3>::IsClassInstance(val.GetObject(), false))
      color = e3dcolor(*val.Cast<const Color3 *>());
    else if (Sqrat::ClassType<Color4>::IsClassInstance(val.GetObject(), false))
      color = e3dcolor(*val.Cast<const Color4 *>());
    else
    {
      *err_msg = "Expected class instance type (only E3DCOLOR, Color3 and Color4 are supported";
      return false;
    }
    return true;
  }

  *err_msg = "Expected numeric type (including Color() call result) or color class instance";
  return false;
}

bool decode_e3dcolor(HSQUIRRELVM vm, SQInteger idx, E3DCOLOR &color, const char **err_msg)
{
  return decode_e3dcolor(Sqrat::Var<Sqrat::Object>(vm, idx).value, color, err_msg);
}


bool are_sq_obj_equal(HSQUIRRELVM vm, const HSQOBJECT &a, const HSQOBJECT &b)
{
  SQInteger res = 0;
  return sq_direct_cmp(vm, &a, &b, &res) && (res == 0);
}


bool is_same_immutable_obj(const Sqrat::Object &a, const Sqrat::Object &b)
{
  return (sq_objflags(a.GetObject()) & SQOBJ_FLAG_IMMUTABLE) && (sq_objflags(b.GetObject()) & SQOBJ_FLAG_IMMUTABLE) && a.IsEqual(b);
}


bool get_closure_info(const Sqrat::Function &f, SQInteger *nparams, SQInteger *nfreevars)
{
  HSQUIRRELVM vm = f.GetVM();
  SqStackChecker chk(vm);

  sq_pushobject(vm, f.GetFunc());
  *nparams = *nfreevars = 0;
  bool ok = SQ_SUCCEEDED(sq_getclosureinfo(vm, -1, nparams, nfreevars));
  sq_pop(vm, 1);

  return ok;
}

void get_closure_full_name(const Sqrat::Object &func, String &out_name)
{
  if (func.IsNull())
    out_name = "<null>";
  else if (func.GetType() != OT_CLOSURE)
    out_name.printf(0, "<not a closure (%s)>", sq_objtypestr(func.GetType()));
  else
  {
    SQFunctionInfo fi;
    if (SQ_SUCCEEDED(sq_ext_getfuncinfo(func.GetObject(), &fi)))
      out_name.printf(0, "%s (%s:%d)", fi.name, fi.source, int(fi.line));
    else
      out_name = "<invalid>";
  }
}


Point2 script_get_point2(Sqrat::Array arr)
{
  if (arr.GetType() != OT_ARRAY)
  {
    G_ASSERTF(0, "Point2 must be specified via an array");
    return Point2(0, 0);
  }

  Point2 val(0, 0);
  SQInteger len = arr.Length();
  G_ASSERT(len == 2);
  for (SQInteger i = 0; i < len && i < 2; ++i)
    val[int(i)] = arr.RawGetSlotValue(i, 0.0f);
  return val;
}


E3DCOLOR script_get_color(Sqrat::Array arr)
{
  int val[4] = {255, 255, 255, 255};
  arr.GetArray(val, ::min(4, int(arr.Length())));
  return E3DCOLOR(val[0], val[1], val[2], val[3]);
}


E3DCOLOR script_decode_e3dcolor(SQInteger c)
{
  int i = (int)c;
  E3DCOLOR res((unsigned int &)i);
  return res;
}


float resolve_offset_val(const Element *elem, const Sqrat::Object &o)
{
  SQObjectType tp = o.GetType();

  if (tp & SQOBJECT_NUMERIC)
    return o.Cast<float>();

  if (tp == OT_USERDATA)
  {
    SizeSpec *ss = NULL;
    HSQOBJECT hObj = o.GetObject();
    if (SQ_SUCCEEDED(sq_direct_getuserdata(&hObj, (SQUserPointer *)&ss)))
    {
      G_ASSERT(ss);
      if (ss)
      {
        if (elem && ss->mode == SizeSpec::FONT_H)
          return elem->calcFontHeightPercent(ss->value);
        else
          G_ASSERTF(0, "Unsupported size mode %d", ss->mode);
      }
    }
    else
      G_ASSERT(0);
  }
  else
    G_ASSERTF(0, "Unsupported field type %X", tp);

  return 0;
}


bool script_parse_offsets(const Element *elem, const Sqrat::Object &obj, float out_val[4], const char **err_msg)
{
  SQObjectType tp = obj.GetType();
  if (tp & SQOBJECT_NUMERIC)
  {
    out_val[0] = out_val[1] = out_val[2] = out_val[3] = obj.Cast<float>();
  }
  else if (tp == OT_USERDATA)
  {
    out_val[0] = out_val[1] = out_val[2] = out_val[3] = resolve_offset_val(elem, obj);
  }
  else if (tp == OT_ARRAY)
  {
    Sqrat::Array arr(obj);
    SQInteger len = min(4, (int)arr.Length());

    for (SQInteger i = 0; i < len; ++i)
      out_val[i] = resolve_offset_val(elem, arr.RawGetSlot(i));

    if (len == 1)
      out_val[1] = out_val[2] = out_val[3] = out_val[0];
    else if (len == 2)
    {
      out_val[3] = out_val[1];
      out_val[2] = out_val[0];
    }
    else if (len == 3)
      out_val[3] = out_val[1];
  }
  else if (tp == OT_NULL)
  {
    memset(out_val, 0, sizeof(float) * 4);
  }
  else
  {
    memset(out_val, 0, sizeof(float) * 4);
    if (err_msg)
      *err_msg = "Unexpected offset type. Expected numeric, array or user data";
    return false;
  }
  return true;
}


SQInteger locate_element_source(HSQUIRRELVM vm)
{
  ElementRef *eref = ElementRef::get_from_stack(vm, 2);
  if (eref && eref->elem)
  {
    SQChar buf[1024];
    const HSQOBJECT traceContainer = eref->elem->props.scriptDesc.GetObject();
    HSQOBJECT traceKey = eref->elem->csk->rendObj.GetObject();
    Sqrat::Object::iterator it;
    if (eref->elem->props.scriptDesc.Next(it))
      traceKey = it.getKey();
    sq_tracevar(vm, &traceContainer, &traceKey, buf, countof(buf));

    Sqrat::Table res(vm);
    res.SetValue("stack", (const SQChar *)buf);
    Sqrat::PushVar(vm, res);
    return 1;
  }
  else
  {
    sq_pushnull(vm);
    return 1;
  }
}

SQInteger get_element_info(HSQUIRRELVM vm)
{
  ElementRef *eref = ElementRef::get_from_stack(vm, 2);
  if (eref && eref->elem)
  {
    Sqrat::Table info = eref->elem->getElementInfo(vm);
    Sqrat::PushVar(vm, info);
    return 1;
  }
  else
  {
    sq_pushnull(vm);
    return 1;
  }
}


void script_set_userpointer_to_slot(const Sqrat::Object &obj, const char *key, void *up)
{
  HSQUIRRELVM vm = obj.GetVM();

  SqStackChecker stackCheck(vm);

  sq_pushobject(vm, obj.GetObject());
  sq_pushstring(vm, key, -1);
  sq_pushuserpointer(vm, up);
  G_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)));
  sq_pop(vm, 1);
}


void *script_cast_to_userpointer(const Sqrat::Object &obj)
{
  G_ASSERT(obj.GetType() == OT_USERPOINTER);
  return obj.GetObject()._unVal.pUserPointer;
}


bool load_sound_info(Sqrat::Object src, Sqrat::string &out_name, Sqrat::Object &out_params, float &out_vol)
{
  out_name.clear();
  out_params.Release();
  out_vol = 1;

  SQObjectType srcType = src.GetType();
  if (srcType == OT_STRING)
  {
    out_name = src.Cast<Sqrat::string>();
    return true;
  }
  else if (srcType == OT_ARRAY)
  {
    bool foundString = false;
    Sqrat::Array arr(src);
    for (SQInteger i = 0, n = arr.Length(); i < n; ++i)
    {
      Sqrat::Object field = arr.RawGetSlot(i);
      switch (field.GetType())
      {
        case OT_STRING:
          out_name = field.Cast<Sqrat::string>();
          foundString = true;
          break;
        case OT_INTEGER:
        case OT_FLOAT: out_vol = field.Cast<float>(); break;
        case OT_TABLE:
        case OT_CLASS:
        case OT_INSTANCE: out_params = field; break;
        default: break;
      }
    }
    return foundString;
  }
  else if (srcType == OT_TABLE || srcType == OT_CLASS || srcType == OT_INSTANCE)
  {
    out_name = src.RawGetSlotValue<Sqrat::string>("name", "<n/a>");
    out_params = src.RawGetSlot("params");
    out_vol = src.RawGetSlotValue<float>("vol", 1.0f);
    return true;
  }

  return false;
}


} // namespace darg
