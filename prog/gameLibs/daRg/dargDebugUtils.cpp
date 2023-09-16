#include "guiScene.h"
#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>

#include <quirrel/sqStackChecker.h>
#include <sqext.h>


using namespace darg;


static bool sq_obj_to_string(HSQUIRRELVM vm, HSQOBJECT obj, Sqrat::string &out_string)
{
  SqStackChecker chk(vm);

  sq_pushobject(vm, obj);
  bool r = SQ_SUCCEEDED(sq_tostring(vm, -1));
  if (r)
  {
    const SQChar *s = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &s)));
    out_string = Sqrat::string(s);
    sq_pop(vm, 1);
  }
  sq_pop(vm, 1);
  return r;
}


void darg_immediate_error(HSQUIRRELVM vm, const char *error)
{
  darg::GuiScene *scene = darg::GuiScene::get_from_sqvm(vm);
  scene->errorMessage(error);
}


void darg_assert_trace_var(const char *message, const Sqrat::Object &container, const Sqrat::Object &key)
{
  G_ASSERT_RETURN(message, );

  SQChar buf[1024];
  HSQUIRRELVM vm = container.GetVM();
  const HSQOBJECT traceContainer = container.GetObject();
  const HSQOBJECT traceKey = key.GetObject();
  sq_tracevar(vm, &traceContainer, &traceKey, buf, countof(buf));
  Sqrat::string keyName;
  if (!sq_obj_to_string(vm, traceKey, keyName))
    keyName = "n/a";

  eastl::string err;
  err.sprintf("%s\nKey: \"%s\"\n\nChange stack:\n%s\n--------------\n\n", message, keyName.c_str(), buf);

  darg::GuiScene *scene = darg::GuiScene::get_from_sqvm(vm);
  scene->errorMessage(err.c_str());
}


void darg_assert_trace_var(const char *message, const Sqrat::Object &container, int key)
{
  HSQOBJECT keyObj;
  sq_resetobject(&keyObj);
  keyObj._type = OT_INTEGER;
  keyObj._unVal.nInteger = key;
  darg_assert_trace_var(message, container, Sqrat::Object(keyObj, container.GetVM()));
}


void darg_report_unrebuildable_element(const Element *elem, const char *main_message)
{
  String msg(main_message);

  int depth = 0;
  for (const Element *e = elem; e; e = e->parent, ++depth)
  {
    const Sqrat::Object &builder = e->props.scriptBuilder;
    if (!builder.IsNull())
    {
      SQFunctionInfo fi;
      G_VERIFY(SQ_SUCCEEDED(sq_ext_getfuncinfo(builder.GetObject(), &fi)));
      msg.aprintf(64, "| %s:%d, depth = %d", fi.source, fi.line, depth);
      break;
    }
  }

  Sqrat::Object::iterator it;
  Sqrat::Object traceKey;
  if (elem->props.scriptDesc.Next(it))
    traceKey = Sqrat::Object(it.getKey(), elem->props.scriptDesc.GetVM());
  else
    traceKey = elem->csk->watch;

  darg_assert_trace_var(msg, elem->props.scriptDesc, traceKey);
}
