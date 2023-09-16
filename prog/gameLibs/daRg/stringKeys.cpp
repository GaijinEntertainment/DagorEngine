#include <daRg/dag_stringKeys.h>
#include "scriptUtil.h"


namespace darg
{

void StringKeys::init(HSQUIRRELVM vm)
{
  SqStackChecker stackCheck(vm);

#define KEY(x) getStr(vm, #x, x);

  DARG_STRING_KEYS_LIST

#undef KEY
}


void StringKeys::getStr(HSQUIRRELVM vm, const char *str, Sqrat::Object &obj)
{
  sq_pushstring(vm, str, -1);
  HSQOBJECT hobj;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &hobj)));
  obj = Sqrat::Object(hobj, vm);
  sq_pop(vm, 1);
}

} // namespace darg
