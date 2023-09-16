#ifndef _DAGOR_SQ_PLUS_EXT_
#define _DAGOR_SQ_PLUS_EXT_
#pragma once

#include <util/dag_string.h>


namespace SqPlus
{
  inline bool Match(SqPlus::TypeWrapper<const String&>, HSQUIRRELVM v, int idx) {
    return sq_gettype(v,idx) == OT_STRING;
  }

  inline String Get(SqPlus::TypeWrapper<const String&>, HSQUIRRELVM v, int idx) {
    const SQChar * s; SQPLUS_CHECK_GET(sq_getstring(v,idx,&s)); return String(s);
  }

  template<> struct functor<String>
    { static inline void push(HSQUIRRELVM v,const String &value) { sq_pushstring(v, value, value.length()); } };
  template<> struct functor<SimpleString>
    { static inline void push(HSQUIRRELVM v,const char *value) { sq_pushstring(v, value, -1); } };
}

#endif //_DAGOR_SQ_PLUS_EXT_

