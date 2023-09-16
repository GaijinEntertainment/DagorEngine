#include <dasModules/dasModulesCommon.h>
#include <quirrel/sqModules/sqModules.h>


namespace das
{
extern void register_bound_funcs(HSQUIRRELVM vm, function<void(const char *module_name, HSQOBJECT tab)> cb);
}

namespace bind_dascript
{
void bind_das(HSQUIRRELVM vm, uint32_t vm_mask, SqModules *modules_mgr)
{
  das::register_bound_funcs(vm, [&](const char *module_name, HSQOBJECT tab) {
    Sqrat::Table dasBindings(tab, vm);
    if (Sqrat::Object *existingModule = modules_mgr->findNativeModule(module_name))
    {
      debug("override module %s (quirrel binding) in vm %d", module_name, vm_mask);
      Sqrat::Table existingTbl(*existingModule);
      Sqrat::Object::iterator it;
      while (dasBindings.Next(it))
        existingTbl.SetValue(Sqrat::Object(it.getKey(), vm), Sqrat::Object(it.getValue(), vm));
    }
    else
    {
      debug("register module %s (quirrel binding) in vm %d", module_name, vm_mask);
      modules_mgr->addNativeModule(module_name, dasBindings);
    }
  });
}

} // namespace bind_dascript
