// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daBuild.h"
#include <assets/assetMgr.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/assetPlugin.h>
#include <assets/assetHlp.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dynLib.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

#if _TARGET_STATIC_LIB
#include <assets/daBuildExpPluginChain.h>
DagorAssetExporterPlugin *DagorAssetExporterPlugin::tail = nullptr;
#endif

struct DaBuildPluginState
{
  IDaBuildPlugin *p;
#if !_TARGET_STATIC_LIB
  void *dll;
#endif
};

static Tab<DaBuildPluginState> plugins(inimem);

static IDaBuildPlugin *loadSingleExporterPlugin(get_dabuild_plugin_t get_plugin, dabuild_plugin_install_dds_helper_t install_hlp,
  const DataBlock &appblk, DagorAssetMgr &mgr, dag::ConstSpan<bool> exp_types_mask, ILogWriter &log, const char *fname)
{
  IDaBuildPlugin *p = get_plugin();
  if (!p)
    return nullptr;
  if (!p->init(appblk))
  {
    p->destroy();
    return nullptr;
  }

  if (install_hlp)
    install_hlp(&texconvcache::write_built_dds_final);

  for (int i = 0; i < p->getExpCount(); i++)
  {
    int atype = mgr.getAssetTypeId(p->getExpType(i));
    if (atype != -1 && !mgr.getAssetExporter(atype))
    {
      if (!exp_types_mask[atype])
        continue;
      IDagorAssetExporter *e = p->getExp(i);
      if (!e)
        continue;
      mgr.registerAssetExporter(e);
      debug("register <%s> for type <%s>  [%s]", e->getExporterIdStr(), e->getAssetType(), fname);
    }
  }

  for (int i = 0; i < p->getRefProvCount(); i++)
  {
    int atype = mgr.getAssetTypeId(p->getRefProvType(i));
    if (atype != -1 && !mgr.getAssetRefProvider(atype))
    {
      // if (!exp_types_mask[atype])
      //   continue;
      IDagorAssetRefProvider *rp = p->getRefProv(i);
      if (!rp)
        continue;
      mgr.registerAssetRefProvider(rp);
      debug("register <%s> for type <%s>  [%s]", rp->getRefProviderIdStr(), rp->getAssetType(), fname);
    }
  }
  return p;
}

bool loadSingleExporterPlugin(const DataBlock &appblk, DagorAssetMgr &mgr, const char *fname, dag::ConstSpan<bool> exp_types_mask,
  ILogWriter &log)
{
#if !_TARGET_STATIC_LIB
  void *dllHandle = os_dll_load_deep_bind(fname);
  IDaBuildPlugin *p = NULL;

  if (dllHandle)
  {
    get_dabuild_plugin_t get_plugin = (get_dabuild_plugin_t)os_dll_get_symbol(dllHandle, GET_DABUILD_PLUGIN_PROC);

    if (get_plugin)
    {
      p = loadSingleExporterPlugin(get_plugin,
        (dabuild_plugin_install_dds_helper_t)os_dll_get_symbol(dllHandle, DABUILD_PLUGIN_INSTALL_DDS_HLP_PROC), appblk, mgr,
        exp_types_mask, log, fname);

      if (p)
      {
        DaBuildPluginState &st = plugins.push_back();
        st.p = p;
        st.dll = dllHandle;
        ::symhlp_load(fname);
      }
    }

    if (!p)
      os_dll_close(dllHandle);
  }
  return p != NULL;
#else
  return NULL;
#endif
}

bool loadExporterPlugins(const DataBlock &appblk, DagorAssetMgr &mgr, const char *dirpath, dag::ConstSpan<bool> exp_types_mask,
  ILogWriter &log)
{
#if !_TARGET_STATIC_LIB
  alefind_t ff;
  const String mask(260, "%s/*" DAGOR_OS_DLL_SUFFIX, dirpath);
  String fname;

  if (::dd_find_first(mask, DA_FILE, &ff))
    do
    {
      fname.printf(260, "%s/%s", dirpath, ff.name);
      if (!fname.suffix(DAGOR_DLL))
        continue;
      loadSingleExporterPlugin(appblk, mgr, simplify_fname(fname), exp_types_mask, log);
    } while (::dd_find_next(&ff));

  dd_find_close(&ff);
#else
  if (plugins.size()) // only one scan!
    return true;
  if (pull_all_dabuild_plugins) //< reference var to pull code to final executable
    debug("registering %d plugins", pull_all_dabuild_plugins);
  for (DagorAssetExporterPlugin *f = DagorAssetExporterPlugin::tail; f; f = f->next)
    if (IDaBuildPlugin *p = loadSingleExporterPlugin(f->get_plugin, f->install_hlp, appblk, mgr, exp_types_mask, log, f->name))
      plugins.push_back().p = p;
#endif
  return true;
}

void unloadExporterPlugins(DagorAssetMgr &mgr)
{
  mgr.clear();

  for (int i = 0; i < plugins.size(); i++)
    plugins[i].p->destroy();

#if !_TARGET_STATIC_LIB
  for (int i = 0; i < plugins.size(); i++)
    os_dll_close(plugins[i].dll);
#endif
  clear_and_shrink(plugins);
}
