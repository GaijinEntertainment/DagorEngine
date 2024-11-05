// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetHlp.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>
#include <assets/assetPlugin.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dynLib.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

struct DaBuildPluginState
{
  IDaBuildPlugin *p;
  void *dll;
};

static Tab<DaBuildPluginState> plugins(inimem);
static Tab<int> expTypes(inimem);

static bool loadSingleExporterPlugin(const DataBlock &appblk, DagorAssetMgr &mgr, const char *fname)
{
  void *dllHandle = os_dll_load_deep_bind(fname);
  IDaBuildPlugin *p = NULL;

  if (dllHandle)
  {
    get_dabuild_plugin_t get_plugin = (get_dabuild_plugin_t)os_dll_get_symbol(dllHandle, GET_DABUILD_PLUGIN_PROC);

    if (get_plugin)
    {
      p = get_plugin();
      if (p && p->init(appblk))
      {
        DaBuildPluginState &st = plugins.push_back();
        st.p = p;
        st.dll = dllHandle;
        ::symhlp_load(fname);
        int refs_used = 0;

        if (dabuild_plugin_install_dds_helper_t install_hlp =
              (dabuild_plugin_install_dds_helper_t)os_dll_get_symbol(dllHandle, DABUILD_PLUGIN_INSTALL_DDS_HLP_PROC))
          install_hlp(&texconvcache::write_built_dds_final);

        for (int i = 0; i < p->getRefProvCount(); i++)
        {
          int atype = mgr.getAssetTypeId(p->getRefProvType(i));
          if (atype != -1 && !mgr.getAssetRefProvider(atype))
          {
            IDagorAssetRefProvider *rp = p->getRefProv(i);
            if (!rp)
              continue;
            refs_used++;
            mgr.registerAssetRefProvider(rp);
            debug("register <%s> for type <%s>  [%s]", rp->getRefProviderIdStr(), rp->getAssetType(), fname);
          }
        }

        if (!refs_used)
        {
          p->destroy();
          p = NULL;
          ::symhlp_unload(fname);
          plugins.pop_back();
          debug("unused: %s", fname);
        }
      }
      else
      {
        debug("%s init failed", fname);
        p = NULL;
      }
    }

    if (!p)
      os_dll_close(dllHandle);
  }
  else
    logwarn("failed to load %s\nerror=%s", fname, os_dll_get_last_error_str());
  return p != NULL;
}

static bool load_plugins_from_dir(DataBlock &appblk, DagorAssetMgr &mgr, const char *dirpath)
{
  alefind_t ff;
  const String mask(260, "%s/*" DAGOR_OS_DLL_SUFFIX, dirpath);
  String fname;

  if (::dd_find_first(mask, DA_FILE, &ff))
    do
    {
      fname.printf(260, "%s/%s", dirpath, ff.name);
      dd_simplify_fname_c(fname);
      loadSingleExporterPlugin(appblk, mgr, fname);
    } while (::dd_find_next(&ff));

  dd_find_close(&ff);
  return true;
}

bool assetrefs::load_plugins(DagorAssetMgr &mgr, DataBlock &appblk, const char *start_dir, bool dabuild_exist)
{
  const char *appdir = appblk.getStr("appDir", "./");
  const DataBlock &exp_blk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
  int nid_folder = appblk.getNameId("folder"), nid_plugin = appblk.getNameId("plugin"), nid_type = appblk.getNameId("type");

  if (appblk.getStr("shaders", NULL))
    appblk.setStr("shadersAbs", String(260, "%s/%s", appdir, appblk.getStr("shaders", NULL)));
  else
    appblk.setStr("shadersAbs", String(260, "%s/../common/compiledShaders/tools", start_dir));

  // build exported types list
  {
    const DataBlock &blk = *exp_blk.getBlockByNameEx("types");

    expTypes.clear();
    expTypes.push_back(mgr.getTexAssetTypeId());
    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid_type)
      {
        int atype = mgr.getAssetTypeId(blk.getStr(i));
        if (atype != -1)
        {
          bool found = false;
          for (int j = 0; j < expTypes.size(); j++)
            if (expTypes[j] == atype)
            {
              found = true;
              break;
            }
          if (!found)
            expTypes.push_back(atype);
        }
      }
  }

  if (dabuild_exist)
    return true;

  // load ref provider plugins
  const DataBlock &blk = *exp_blk.getBlockByNameEx("plugins");

  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid_folder)
    {
      const char *pfolder = blk.getStr(i);
      const char *common_dir = "plugins/dabuild";
      bool ret;
      if (stricmp(pfolder, "*common") == 0)
        ret = load_plugins_from_dir(appblk, mgr, String(260, "%s/%s", start_dir, common_dir));
      else
        ret = load_plugins_from_dir(appblk, mgr, String(260, "%s/%s", appdir, pfolder));

      if (!ret)
        return false;
    }
    else if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid_plugin)
    {
      if (!loadSingleExporterPlugin(appblk, mgr, String(260, "%s/%s", appdir, blk.getStr(i))))
        return false;
    }
  return true;
}

void assetrefs::unload_plugins(DagorAssetMgr &mgr)
{
  mgr.clear();

  for (int i = 0; i < plugins.size(); i++)
    plugins[i].p->destroy();

  for (int i = 0; i < plugins.size(); i++)
    os_dll_close(plugins[i].dll);
  clear_and_shrink(plugins);
  clear_and_shrink(expTypes);
}

dag::ConstSpan<int> assetrefs::get_exported_types() { return make_span_const(expTypes).subspan(1); }
dag::ConstSpan<int> assetrefs::get_all_exported_types() { return expTypes; }
int assetrefs::get_tex_type() { return expTypes[0]; }
