#pragma once

#include <sqrat.h>

class SqModules;

namespace darg
{

struct GuiSceneScriptHandlers
{
  Sqrat::Function onUpdate;
  Sqrat::Function onShutdown;
  Sqrat::Function onHotkeysNav;

  void releaseFunctions()
  {
    onUpdate.Release();
    onShutdown.Release();
    onHotkeysNav.Release();
  }
};


namespace binding
{

template <typename PictureClass>
inline SQInteger picture_script_ctor(HSQUIRRELVM vm)
{
  SQInteger nParams = sq_gettop(vm);
  if (nParams > 2)
    return sq_throwerror(vm, "Too many arguments");

  const SQChar *name = "";
  if (nParams == 2 && sq_gettype(vm, 2) == OT_STRING)
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &name)));
  PictureClass *inst = new PictureClass(vm, name);

  Sqrat::ClassType<PictureClass>::SetManagedInstance(vm, 1, inst);
  return 0;
}

void bind_script_classes(SqModules *module_mgr, Sqrat::Table &exports);

} // namespace binding

} // namespace darg
