#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstate.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"
#include "sqarray.h"


SQRESULT sq_ext_getfuncinfo(HSQOBJECT obj, SQFunctionInfo *fi)
{
  if (!fi)
    return SQ_ERROR;

  if (!sq_isclosure(obj) && !sq_isnativeclosure(obj))
    return SQ_ERROR;

  if (sq_isclosure(obj))
  {
    SQFunctionProto *proto = _closure(obj)->_function;
    if (!proto)
      return SQ_ERROR;

    fi->funcid = proto;
    fi->name = sq_type(proto->_name) == OT_STRING?_stringval(proto->_name):_SC("unknown");
    fi->source = sq_type(proto->_sourcename) == OT_STRING?_stringval(proto->_sourcename):_SC("unknown");
    fi->line = proto->_lineinfos[0]._line;
  }
  else
  {
    SQNativeClosure *c = _nativeclosure(obj);

    fi->funcid = (SQUserPointer)c->_function;
    fi->name = sq_type(c->_name) == OT_STRING?_stringval(c->_name):_SC("unknown");
    fi->source = _SC("native");
    fi->line = -1;
  }

  return SQ_OK;
}


SQRESULT sq_ext_get_array_floats(HSQOBJECT obj, int start, int count, float * dest)
{
  if (sq_type(obj) != OT_ARRAY || !dest)
    return SQ_ERROR;

  SQArray * array = _array(obj);
  SQObjectPtrVec & values = array->_values;
  int arraySize = values.size();

  if (count + start > arraySize)
    return SQ_ERROR;

  for (int i = 0; i < count; i++)
  {
    SQObjectPtr & v = values[i + start];
    if (sq_type(v) == OT_FLOAT)
      dest[i] = _float(v);
    else if (sq_type(v) == OT_INTEGER || sq_type(v) == OT_BOOL)
      dest[i] = _integer(v);
    else
      dest[i] = 0;
  }

  return SQ_OK;
}


int sq_ext_get_array_int(HSQOBJECT obj, int index, int def)
{
  if (sq_type(obj) != OT_ARRAY)
    return def;

  SQObjectPtrVec & values = _array(obj)->_values;
  if (index < 0 || index >= values.size())
    return def;

  SQObjectPtr & v = values[index];
  if (sq_type(v) == OT_INTEGER || sq_type(v) == OT_BOOL)
    return _integer(v);
  else if (sq_type(v) == OT_FLOAT)
    return int(_float(v));
  else
    return def;
}


float sq_ext_get_array_float(HSQOBJECT obj, int index, float def)
{
  if (sq_type(obj) != OT_ARRAY)
    return def;

  SQObjectPtrVec & values = _array(obj)->_values;
  if (index < 0 || index >= values.size())
    return def;

  SQObjectPtr & v = values[index];
  if (sq_type(v) == OT_FLOAT)
    return _float(v);
  if (sq_type(v) == OT_INTEGER || sq_type(v) == OT_BOOL)
    return _integer(v);
  else
    return def;
}

