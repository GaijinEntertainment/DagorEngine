#include <util/dag_string.h>
#include <fx/dag_paramScript.h>
#include <fx/dag_paramScriptsPool.h>
#include <osApiWrappers/dag_localConv.h>
#include <debug/dag_debug.h>


ParamScriptsPool::ParamScriptsPool(IMemAlloc *mem) : factories(mem) {}


ParamScriptsPool::~ParamScriptsPool() { unloadAll(); }


ParamScriptsPool::Module::Module() : factory(NULL) {}


ParamScriptsPool::Module::~Module() { clear(); }


void ParamScriptsPool::Module::clear()
{
  if (factory)
  {
    factory->destroyFactory();
    factory = NULL;
  }
}

bool ParamScriptsPool::registerFactory(ParamScriptFactory *fac)
{
  if (!fac)
    return false;

  const char *className = fac->getClassName();
  if (!className)
  {
    debug_ctx("no class name for factory %p", fac);
    return false;
  }

  for (int i = 0; i < factories.size(); ++i)
  {
    ParamScriptFactory *f = factories[i].factory;
    if (!f)
      continue;

    if (f == fac)
    {
      debug_ctx("factory '%s' is already registered", className);
      return true;
    }

    const char *cn = f->getClassName();
    if (!cn)
      continue;

    if (dd_stricmp(className, cn) == 0)
    {
      debug_ctx("duplicate class name '%s' for factories %p and %p", className, fac, f);
      return false;
    }
  }

  int id = append_items(factories, 1);
  factories[id].factory = fac;

  return true;
}


void ParamScriptsPool::unloadAll() { clear_and_shrink(factories); }


ParamScriptFactory *ParamScriptsPool::getFactoryByName(const char *class_name)
{
  for (int i = 0; i < factories.size(); ++i)
  {
    ParamScriptFactory *f = factories[i].factory;
    if (!f)
      continue;

    const char *className = f->getClassName();
    if (!className)
      continue;

    if (dd_stricmp(className, class_name) == 0)
      return f;
  }

  return NULL;
}
