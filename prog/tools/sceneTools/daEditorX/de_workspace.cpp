#include <oldEditor/de_workspace.h>

#include <assetsGui/av_globalState.h>
#include <ioSys/dag_dataBlock.h>

#include <generic/dag_sort.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/de_TextureName.h>
#include <libTools/util/fileUtils.h>

#include <libTools/containers/dag_TabOps.h>
#include <libTools/containers/tab_sort.h>

#include <3d/dag_drv3d.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>

#include <debug/dag_debug.h>

#include <sepGui/wndGlobal.h>


//==============================================================================
DeWorkspace::DeWorkspace() : f11Tags(midmem), plugTags(midmem) {}


//==============================================================================
DeWorkspace::Workspace::Workspace() : recents(midmem), exportIgnore(midmem), enabledColliders(midmem) { reset(); }


//==============================================================================
void DeWorkspace::Workspace::reset()
{
  exportIgnore.clear();

  collision = "DagorNovodex";
  additionalPlugins.clear();

  recents.clear();
}


bool DeWorkspace::loadAppSpecific(const DataBlock &blk)
{
  clear();

  const DataBlock *sdkBlk = blk.getBlockByName("SDK");

  if (sdkBlk)
  {
    params.additionalPlugins = sdkBlk->getStr("daeditor3x_additional_plugins", NULL);

    if (params.additionalPlugins.length())
      params.additionalPlugins = ::make_full_path(getAppDir(), params.additionalPlugins);
  }

  f11Tags.clear();
  plugTags.clear();

  shGlobVarsScheme.setFrom(blk.getBlockByNameEx("shader_glob_vars_scheme"));

  const DataBlock *tagsBlk = blk.getBlockByName("dagored_visibility_tags");

  if (tagsBlk)
  {
    for (int i = 0; i < tagsBlk->blockCount(); ++i)
    {
      const DataBlock *plugBlk = tagsBlk->getBlock(i);

      if (plugBlk)
      {
        const char *plugName = plugBlk->getStr("name", NULL);
        String plugTagsStr(plugBlk->getStr("tags", NULL));

        if (plugName && *plugName && plugTagsStr.length())
        {
          const char *tag = strtok(plugTagsStr, ",; ");
          TabInt keys;

          while (tag)
          {
            const int idx = ::check_and_add_stri(f11Tags, tag);
            keys.push_back(idx);

            tag = strtok(NULL, ",; ");
          }

          plugTags.add(String(plugName), keys);
        }
      }
    }

    // sorting
    Tab<String> old = f11Tags;
    sort(f11Tags, &tab_sort_stringsi);

    TabInt keys;
    String plugName;

    for (bool ok = plugTags.getFirst(plugName, keys); ok; ok = plugTags.getNext(plugName, keys))
    {
      for (int i = 0; i < keys.size(); ++i)
      {
        const char *keyStr = old[keys[i]];

        for (int j = 0; j < f11Tags.size(); ++j)
        {
          if (!stricmp(keyStr, f11Tags[j]))
          {
            keys[i] = j;
            break;
          }
        }
      }

      plugTags.set(plugName, keys);
    }
  }

  const DataBlock *hmapBlk = blk.getBlockByNameEx("heightMap");

  hmapRequireTileTex = hmapBlk->getBool("requireTileTex", false);
  hmapHasColorTex = hmapBlk->getBool("hasColorTex", true);
  hmapHasLightmapTex = hmapBlk->getBool("hasLightmapTex", true);
  hmapUseMeshSurface = hmapBlk->getBool("useMeshSurface", false);
  hmapUseNormalMap = hmapBlk->getBool("hasNormalMap", false);

  hmapLandClsLayersDesc.setFrom(hmapBlk->getBlockByNameEx("landClassLayers"));

  genFwdRadius = hmapBlk->getReal("genFwdRadius", 1500);
  genBackRadius = hmapBlk->getReal("genBackRadius", genFwdRadius);
  genDiscardRadius = hmapBlk->getReal("genDiscardRadius", 2000);

  if (d3d::is_stub_driver())
    genFwdRadius = genBackRadius = 10;

  const DataBlock *enCollBlk = blk.getBlockByName("enabled_colliders");
  if (enCollBlk)
  {
    int nid = enCollBlk->getNameId("name");

    for (int i = 0; i < enCollBlk->paramCount(); i++)
    {
      if (enCollBlk->getParamNameId(i) == nid && enCollBlk->getParamType(i) == DataBlock::TYPE_STRING)
      {
        const char *name = enCollBlk->getStr(i);
        params.enabledColliders.push_back() = name;
      }
    }
  }

  return true;
}


bool DeWorkspace::loadSpecific(const DataBlock &blk)
{
  const String graphiteDir = ::make_full_path(sgg::get_exe_path_full(), "graphite/");

  params.graphiteDir = blk.getStr("graphite_dir", graphiteDir);
  if (!params.graphiteDir.length() || !::dd_dir_exist(params.graphiteDir))
    params.graphiteDir = graphiteDir;

  const DataBlock *exportIgnoreBlk = blk.getBlockByName("export_ignore_def");

  if (exportIgnoreBlk)
    for (int i = 0; i < exportIgnoreBlk->paramCount(); ++i)
      params.exportIgnore.push_back() = exportIgnoreBlk->getStr(i);

  const DataBlock *recentBlk = blk.getBlockByName("recents");
  if (recentBlk)
  {
    const int fileNameId = blk.getNameId("file");
    for (int recentId = recentBlk->paramCount(); recentId >= 0; --recentId)
    {
      if ((fileNameId != recentBlk->getParamNameId(recentId)) || (DataBlock::TYPE_STRING != recentBlk->getParamType(recentId)))
        continue;

      const char *fn = recentBlk->getStr(recentId);
      if (!::dd_file_exist(::make_full_path(getSdkDir(), fn)))
        continue;

      const int id = params.recents.size();
      params.recents.push_back() = fn;
      simplify_fname(params.recents[id]);
    }
  }

  AssetSelectorGlobalState::load(blk);

  return true;
}


bool DeWorkspace::saveSpecific(DataBlock &blk)
{
  simplify_fname(params.graphiteDir);
  blk.setStr("graphite_dir", params.graphiteDir);

  DataBlock *exportIgnoreBlk = blk.addBlock("export_ignore_def");

  if (exportIgnoreBlk)
  {
    exportIgnoreBlk->clearData();

    for (int i = 0; i < params.exportIgnore.size(); ++i)
      exportIgnoreBlk->addStr("name", params.exportIgnore[i]);
  }

  DataBlock *recentBlk = blk.addBlock("recents");
  if (recentBlk)
  {
    recentBlk->clearData();
    for (int recentId = params.recents.size() - 1; recentId >= 0; --recentId)
    {
      simplify_fname(params.recents[recentId]);
      recentBlk->addStr("file", params.recents[recentId]);
    }
  }

  AssetSelectorGlobalState::save(blk);

  return true;
}


void DeWorkspace::clear() { params.reset(); }


bool DeWorkspace::getMetricsBlk(DataBlock &blk) const
{
  DataBlock appBlk;
  appBlk.load(getAppPath());

  DataBlock *metrBlk = appBlk.getBlockByName("level_metrics");

  if (metrBlk)
  {
    blk.setFrom(metrBlk);
    return true;
  }

  return false;
}


const DataBlock *DeWorkspace::findWspBlk(const char *app_blk_path)
{
  if (wspData)
  {
    const int wspNid = wspData->blk.getNameId("workspace");

    for (int i = 0; i < wspData->blk.blockCount(); ++i)
    {
      DataBlock *wspBlk = wspData->blk.getBlock(i);

      if (wspBlk && wspBlk->getBlockNameId() == wspNid)
      {
        const char *appBlkPath = wspBlk->getStr("application_path", NULL);
        if (appBlkPath && !::dag_path_compare(appBlkPath, app_blk_path))
          return wspBlk;
      }
    }
  }

  return NULL;
}
