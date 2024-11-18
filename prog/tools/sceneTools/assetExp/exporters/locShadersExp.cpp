// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/fileUtils.h>
#include <gameRes/dag_stdGameRes.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_globalMutex.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <windows.h>
#include <util/dag_stlqsort.h>
#undef ERROR

BEGIN_DABUILD_PLUGIN_NAMESPACE(locShader)

static const char *TYPE = "lshader";
static String compilerExePath;
static String permutationsFileName;
static DataBlock permutationsFile;
static String nonOptionalFileName;
static DataBlock nonOptionalFile;
static char appBlkDir[512] = {0};

static String find_shader_editors_path()
{
  static String foundPath;
  if (!foundPath.empty())
    return foundPath;

  String path("prog/gameLibs/webui/plugins/shaderEditors");

  for (int i = 0; i < 16; i++)
    if (::dd_dir_exists(path.str()))
    {
      foundPath = path;
      return foundPath;
    }
    else
      path = String("../") + path;

  // error, but it gets propagated to collect_template_files with a hlsl #error
  return String("");
}

static String join_str(Tab<String> &arr, const char *delimiter)
{
  String res;
  for (int i = 0; i < arr.size(); i++)
  {
    if (!res.empty())
      res += delimiter;
    res += arr[i];
  }

  return res;
}

static String extractGraphName(const String &path)
{
  const char *begin = max(path.find('/', 0, false) + 1, path.find('\\', 0, false) + 1);
  return String(begin, path.end() - begin);
}

class NodeBasedLocShaderExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "lshader exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return LShaderGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const { return 1; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = compilerExePath;
    if (!permutationsFileName.empty())
      files.push_back() = permutationsFileName;
    for (const String &optGraph : gatherOptionalGraphFilenames(a))
      files.push_back() = optGraph;
    if (!nonOptionalFileName.empty())
      files.push_back() = nonOptionalFileName;
    for (const String &nonOptGraph : gatherNonOptionalGraphFilenames(a))
      files.push_back() = nonOptGraph;

    Tab<String> shaderIncludes = gatherAllShaderIncludes();
    for (int i = 0; i < shaderIncludes.size(); ++i)
      files.push_back() = shaderIncludes[i];
  }

  Tab<String> gatherNonOptionalGraphFilenames(const DagorAsset &a)
  {
    Tab<String> graphs;
    const DataBlock *current = nonOptionalFile.getBlockByNameEx(a.props.getStr("non_optional", ""));
    int subgraphNameId = current->getNameId("subgraph");
    if (subgraphNameId != -1)
      for (uint32_t i = 0; i < current->paramCount(); ++i)
        if (current->getParamNameId(i) == subgraphNameId)
          graphs.push_back(String(current->getStr(i)));
    return graphs;
  }

  Tab<String> gatherOptionalGraphFilenames(const DagorAsset &a, ILogWriter *log = nullptr)
  {
    Tab<String> optionalGraphs;
    const DataBlock *currentPermutations = permutationsFile.getBlockByNameEx(a.props.getStr("permutations", ""));
    int subgraphNameId = currentPermutations->getNameId("subgraph");
    if (log && currentPermutations->paramCount() >= 16)
      log->addMessage(log->WARNING, "Only 15 optional graphs allowed. All other graphs are ignored");
    if (subgraphNameId != -1)
      for (uint32_t i = 0, ie = min<uint32_t>(currentPermutations->paramCount(), 16); i < ie; ++i)
        if (currentPermutations->getParamNameId(i) == subgraphNameId)
          optionalGraphs.push_back(String(0, "%sdevelop/%s", appBlkDir, currentPermutations->getStr(i)));
    return optionalGraphs;
  }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    String inputJson(a.getTargetFilePath());
    String out_tmp_fn(0, "%s.tmp.bin", a.getName());

    Tab<String> optionalGraphs = gatherOptionalGraphFilenames(a, &log);
    stlsort::sort(optionalGraphs.begin(), optionalGraphs.end());
    DataBlock permutationNames;
    for (int i = 0; i < optionalGraphs.size(); ++i)
      permutationNames.addStr("subgraph", extractGraphName(optionalGraphs[i]));

    cwr.beginTaggedBlock(_MAKE4C('LSh'));
    if (optionalGraphs.size() > 0)
      permutationNames.saveToStream(cwr.getRawWriter());

    Tab<String> nonOptionalGraphs = gatherNonOptionalGraphFilenames(a);
    Tab<String> nonOptionalSubgraphs;
    for (int i = 0; i < nonOptionalGraphs.size(); ++i)
      nonOptionalSubgraphs.push_back(nonOptionalGraphs[i]);
    String nonOptional = join_str(nonOptionalSubgraphs, ";");

    for (uint32_t permIdx = 0; permIdx < (1 << optionalGraphs.size()); ++permIdx)
    {
      Tab<String> enabledSubgraphs;
      for (size_t subgraphIdx = 0; subgraphIdx < optionalGraphs.size(); ++subgraphIdx)
        if (permIdx & (1 << subgraphIdx))
          enabledSubgraphs.push_back(optionalGraphs[subgraphIdx]);
      String permutation = join_str(enabledSubgraphs, ";");
      if (!nonOptional.empty())
      {
        if (!permutation.empty())
          permutation += ';';
        permutation += nonOptional;
      }
      String cmd(0, "%s -s:%s -singleInputJson:%s -singleOutputBin:%s -target:%s -p:shader_editor -silent -optionalGraphs:%s",
        compilerExePath, subgraphsFolder, inputJson, out_tmp_fn, a.props.getStr("target", ""), permutation);

      cmd.replaceAll("/", "\\");

      void *buf = nullptr;
      int fsize = 0;

      {
        struct MutexLocker
        {
          void *mutex;
          MutexLocker() { mutex = global_mutex_create_enter("node_based_exe"); }
          ~MutexLocker() { global_mutex_leave_destroy(mutex, "node_based_exe"); }
        } mutexLocker;

        dd_erase(out_tmp_fn);

        PROCESS_INFORMATION pi;
        STARTUPINFO si;

        ::ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        if (!::CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
          log.addMessage(log.ERROR, "%s: cannot start process with <%s>", a.getName(), cmd);
          return false;
        }
        ::CloseHandle(pi.hThread);

        ::WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD res = -1;
        if (!GetExitCodeProcess(pi.hProcess, &res))
          res = -2;
        ::CloseHandle(pi.hProcess);

        if (res != 0)
        {
          log.addMessage(log.ERROR, "%s: process <%s> returned %d", a.getName(), cmd.str(), res);
          return false;
        }

        FullFileLoadCB cb(out_tmp_fn);
        if (!cb.fileHandle)
        {
          log.addMessage(log.ERROR, "%s: '%s' not found", a.getName(), out_tmp_fn);
          return false;
        }

        fsize = df_length(cb.fileHandle);
        buf = memalloc(fsize, tmpmem);
        cb.read(buf, fsize);
      }

      cwr.writeRaw(buf, fsize);

      memfree(buf, tmpmem);
      dd_erase(out_tmp_fn);
    }

    cwr.endBlock();

    return true;
  }

  String subgraphsFolder;

private:
  // TODO: do NOT copy-paste these, supply them from the node based compiler
  // TODO: also, maybe store this list to avoid generating it for each asset over and over
  Tab<String> gatherAllShaderIncludes()
  {
    Tab<String> templateNames;

    templateNames.push_back(String("ps4Defines.hlsl"));

    templateNames.push_back(String("globalHlslFunctions.hlsl"));
    templateNames.push_back(String("../../../render/shaders/wang_hash.hlsl"));
    templateNames.push_back(String("../../../render/shaders/pcg_hash.hlsl"));
    templateNames.push_back(String("../../../publicInclude/render/volumetricLights/heightFogNode.hlsli"));
    templateNames.push_back(String("../../../render/shaders/phase_functions.hlsl"));
    templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_common_def.hlsli"));
    templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_common.hlsl"));
    templateNames.push_back(String("../../../render/shaders/wind/sample_wind_common.hlsl"));

    templateNames.push_back(String("../../../render/shaders/static_shadow_int.hlsl"));
    templateNames.push_back(String("../../../render/shaders/static_shadow.hlsl"));
    templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_df_common.hlsl"));
    templateNames.push_back(String("raymarchFogShaderTemplateHeader.hlsl"));
    templateNames.push_back(String("fogCommon.hlsl"));
    templateNames.push_back(String("raymarchFogShaderTemplateFunctions.hlsl"));
    templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_df_raymarch.hlsl"));
    templateNames.push_back(String("raymarchFogShaderTemplateBody.hlsl"));

    templateNames.push_back(String("froxelFogShaderTemplateHeader.hlsl"));
    templateNames.push_back(String("froxelFogShaderTemplateBody.hlsl"));

    templateNames.push_back(String("fogShadowShaderTemplateHeader.hlsl"));
    templateNames.push_back(String("fogShadowShaderTemplateFunctions.hlsl"));
    templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_shadow.hlsl"));
    templateNames.push_back(String("fogShadowShaderTemplateBody.hlsl"));

    String editorPath = find_shader_editors_path();
    for (int i = 0; i < templateNames.size(); ++i)
      templateNames[i] = editorPath + "/" + templateNames[i];
    return templateNames;
  }
};

class NodeBasedLevelShaderRefs : public IDagorAssetRefProvider
{
public:
  virtual const char *__stdcall getRefProviderIdStr() const { return "lshader refs"; }
  virtual const char *__stdcall getAssetType() const { return TYPE; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  dag::ConstSpan<Ref> __stdcall getAssetRefs(DagorAsset &a) override { return {}; }
};

class NodeBasedLocShaderExporterPlugin : public IDaBuildPlugin
{
public:
  virtual bool __stdcall init(const DataBlock &appblk)
  {
    dd_get_fname_location(appBlkDir, appblk.resolveFilename());

    const DataBlock *lshaderBlk = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("lshader");

    const char *subgraphsFolderPtr = lshaderBlk->getStr("subgraphsFolder", nullptr);
    expNodeBased.subgraphsFolder = subgraphsFolderPtr ? String(0, "%sdevelop/%s", appBlkDir, subgraphsFolderPtr) : String();

    const char *permutationsFileNamePtr = lshaderBlk->getStr("permutationsFile", nullptr);
    permutationsFileName = permutationsFileNamePtr ? String(0, "%sdevelop/%s", appBlkDir, permutationsFileNamePtr) : String();

    const char *nonOptionalFileNamePtr = lshaderBlk->getStr("nonOptionalFile", nullptr);
    nonOptionalFileName = nonOptionalFileNamePtr ? String(0, "%sdevelop/%s", appBlkDir, nonOptionalFileNamePtr) : String();

    char exePath[1024];
    dag_get_appmodule_dir(exePath, sizeof(exePath));
    compilerExePath.printf(0, "%s/dsc2-nodeBased-dev.exe", exePath);
    if (!permutationsFileName.empty())
      permutationsFile.load(permutationsFileName);
    if (!nonOptionalFileName.empty())
      nonOptionalFile.load(nonOptionalFileName);
    return true;
  }

  virtual void __stdcall destroy() { delete this; }

  virtual int __stdcall getExpCount() { return 1; }
  virtual const char *__stdcall getExpType(int) { return TYPE; }
  virtual IDagorAssetExporter *__stdcall getExp(int) { return &expNodeBased; }
  virtual int __stdcall getRefProvCount() { return 1; }
  virtual const char *__stdcall getRefProvType(int) { return TYPE; }
  virtual IDagorAssetRefProvider *__stdcall getRefProv(int) { return &refsNodeBased; }

protected:
  NodeBasedLocShaderExporter expNodeBased;
  NodeBasedLevelShaderRefs refsNodeBased;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) NodeBasedLocShaderExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(locShader)
REGISTER_DABUILD_PLUGIN(locShader, nullptr)
#else
int pull_locShader = 1;
#endif
