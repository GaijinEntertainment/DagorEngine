#include <bindQuirrelEx/autoBind.h>
#include <sqModules/sqModules.h>
#include <debug/dag_assert.h>

namespace sq
{
static BindingRegRec *auto_binding_tail = NULL;

BindingRegRec::BindingRegRec(binding_module_cb_t cb, uint32_t vmt_, const char *module_name) :
  bindModuleCb(cb), vmt(vmt_), moduleName(module_name), next(auto_binding_tail)
{
  G_ASSERT(bindModuleCb);
  G_ASSERT(module_name);
  auto_binding_tail = this;
}
BindingRegRec::BindingRegRec(binding_regfunc_cb_t cb, uint32_t vmt_) :
  bindRegfuncCb(cb), vmt(vmt_ | VM_FLG_REGFUNC), moduleName(nullptr), next(auto_binding_tail)
{
  G_ASSERT(bindRegfuncCb);
  auto_binding_tail = this;
}

void auto_bind_native_api(SqModules *modules_mgr, uint32_t vm_mask)
{
  G_ASSERT(modules_mgr);
  G_ASSERT(vm_mask);

  HSQUIRRELVM vm = modules_mgr->getVM();

  for (BindingRegRec *brr = auto_binding_tail; brr; brr = brr->next)
  {
    if (vm_mask & brr->vmt)
    {
      if ((brr->vmt & VM_FLG_REGFUNC) && !brr->moduleName)
      {
        brr->bindRegfuncCb(modules_mgr);
        continue;
      }
      G_ASSERT(brr->moduleName);
      G_ASSERT(brr->bindModuleCb);
      Sqrat::Table moduleTbl = brr->bindModuleCb(vm);
      if (moduleTbl.IsNull())
      {
        logwarn("SQ: no exports from module '%s'", brr->moduleName);
        continue;
      }
      G_ASSERT(moduleTbl.GetType() == OT_TABLE);
      const char *mn = brr->moduleName;
      if (Sqrat::Object *existingModule = modules_mgr->findNativeModule(mn)) // if exist -> merge with overwrite
      {
        logwarn("SQ: duplicate registered module %s", brr->moduleName);
        G_ASSERTF_CONTINUE(existingModule->GetType() == OT_TABLE,
          "Unexpected module '%s' squirrel type %d, only table merge supported", brr->moduleName, existingModule->GetType());
        // This should not happen anymore, but keep the code for now just in case
        Sqrat::Table existingTbl(*existingModule);
        Sqrat::Object::iterator it;
        while (moduleTbl.Next(it))
          existingTbl.SetValue(Sqrat::Object(it.getKey(), vm), Sqrat::Object(it.getValue(), vm)); // To consider: warning when
                                                                                                  // overwriting already existing
                                                                                                  // symbol?
      }
      else // not exist -> add new
        modules_mgr->addNativeModule(mn, moduleTbl);
    }
  }
}

} // namespace sq
