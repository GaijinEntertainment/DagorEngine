#include <assets/daBuildInterface.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dynLib.h>
#include <debug/dag_log.h>

static IDaBuildInterface *dabuild = NULL;
static void *dllHandle = NULL;

IDaBuildInterface *get_dabuild(const char *dll_fname)
{
  if (dabuild)
    return dabuild;

  dllHandle = os_dll_load(dll_fname);

  if (!dllHandle)
  {
    logerr("can't load DLL: %s, error message: %s", dll_fname, os_dll_get_last_error_str());
    return NULL;
  }

  get_dabuild_interface_t p = (get_dabuild_interface_t)os_dll_get_symbol(dllHandle, GET_DABUILD_INTERFACE_PROC);

  if (!p)
  {
    logerr("can't find entry point in '%s'", dll_fname);
    os_dll_close(dllHandle);
    return NULL;
  }

  ::symhlp_load(dll_fname);
  dabuild = p();
  if (!dabuild)
  {
    logerr("can't create dabuild interface");
    os_dll_close(dllHandle);
    ::symhlp_unload(dll_fname);
  }
  if (!dabuild->checkVersion())
  {
    logerr("wrong dabuild ver: %d, req: %d", dabuild->getDllVer(), dabuild->getReqVer());
    dabuild->release();
    os_dll_close(dllHandle);
    ::symhlp_unload(dll_fname);
    return NULL;
  }

  return dabuild;
}

void release_dabuild(IDaBuildInterface *dabuild)
{
  if (!dabuild)
    return;

  dabuild->release();
  os_dll_close(dllHandle);

  dabuild = NULL;
  dllHandle = NULL;
}
