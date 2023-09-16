//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <sqrat.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>


namespace Sqrat
{


template <>
struct Var<SimpleString>
{
  SimpleString value;

  Var(HSQUIRRELVM vm, SQInteger idx)
  {
    const SQChar *ret = _SC("n/a");
    SQInteger size;
    G_VERIFY(SQ_SUCCEEDED(sq_tostring(vm, idx)));
    G_VERIFY(SQ_SUCCEEDED(sq_getstringandsize(vm, -1, &ret, &size)));
    value = SimpleString(ret, size);
    sq_pop(vm, 1);
  }

  static void push(HSQUIRRELVM vm, const SimpleString &value) { sq_pushstring(vm, value.c_str(), value.length()); }

  static const SQChar *getVarTypeName() { return _SC("SimpleString"); }
  static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) == OT_STRING; }
};

template <>
struct Var<const SimpleString &>
{
  SimpleString value;

  Var(HSQUIRRELVM vm, SQInteger idx)
  {
    const SQChar *ret = _SC("n/a");
    SQInteger size;
    G_VERIFY(SQ_SUCCEEDED(sq_tostring(vm, idx)));
    G_VERIFY(SQ_SUCCEEDED(sq_getstringandsize(vm, -1, &ret, &size)));
    value = SimpleString(ret, size);
    sq_pop(vm, 1);
  }

  static void push(HSQUIRRELVM vm, const SimpleString &value) { sq_pushstring(vm, value.c_str(), value.length()); }

  static const SQChar *getVarTypeName() { return _SC("SimpleString ref"); }
  static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) == OT_STRING; }
};

template <>
struct Var<String>
{
  String value;

  Var(HSQUIRRELVM vm, SQInteger idx)
  {
    const SQChar *ret = _SC("n/a");
    SQInteger size;
    G_VERIFY(SQ_SUCCEEDED(sq_tostring(vm, idx)));
    G_VERIFY(SQ_SUCCEEDED(sq_getstringandsize(vm, -1, &ret, &size)));
    value = String(ret, size);
    sq_pop(vm, 1);
  }

  static void push(HSQUIRRELVM vm, const String &value) { sq_pushstring(vm, value.c_str(), value.length()); }

  static const SQChar *getVarTypeName() { return _SC("DagorString"); }
  static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) == OT_STRING; }
};

template <>
struct Var<const String &>
{
  String value;

  Var(HSQUIRRELVM vm, SQInteger idx)
  {
    const SQChar *ret = _SC("n/a");
    SQInteger size;
    G_VERIFY(SQ_SUCCEEDED(sq_tostring(vm, idx)));
    G_VERIFY(SQ_SUCCEEDED(sq_getstringandsize(vm, -1, &ret, &size)));
    value = String(ret, size);
    sq_pop(vm, 1);
  }

  static void push(HSQUIRRELVM vm, const String &value) { sq_pushstring(vm, value.c_str(), value.length()); }

  static const SQChar *getVarTypeName() { return _SC("DagorString ref"); }
  static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) == OT_STRING; }
};


SQRAT_MAKE_NONREFERENCABLE(SimpleString)
SQRAT_MAKE_NONREFERENCABLE(String)

} // namespace Sqrat
