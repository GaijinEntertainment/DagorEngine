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
#include <EASTL/vector_map.h>
#include <EASTL/string_map.h>
#include <EASTL/optional.h>
#include <EASTL/fixed_vector.h>
#include <regExp/regExp.h>
#include <util/dag_finally.h>
#include <generic/dag_enumerate.h>
#undef ERROR

BEGIN_DABUILD_PLUGIN_NAMESPACE(locShader)

enum class NBSType
{
  UNKNOWN,
  ENVI_COVER,
  FOG,
};

static eastl::string_map<NBSType> types;
static const char *TYPE = "lshader";
static String compilerExePath;
static Tab<String> compilerBinariesUsedByExe;
static String permutationsFileName;
static DataBlock permutationsFile;
static String nonOptionalFileName;
static DataBlock nonOptionalFile;
static String categorizationsFileName;
static DataBlock categorizationsFile;
static char appBlkDir[512] = {0};
static DWORD thisPid = DWORD(-1);
static DWORD thisTid = DWORD(-1);

static constexpr const char *ALL_PLATFORM_NAMES[] = {
  "dx11", "ps4", "spirv", "dx12", "dx12x", "dx12xs", "ps5", "metal", "spirvb", "metalb"};
static carray<carray<eastl::optional<Tab<String>>, 3>, c_countof(ALL_PLATFORM_NAMES)> perPlatformShaderTemplateSourceFiles;

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

static bool run_process(String &cmd, const char *logtag, auto &&error_reporter)
{
  cmd.replaceAll("/", "\\");

  HANDLE readPipe, writePipe;
  SECURITY_ATTRIBUTES saAttr{};
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = true;
  saAttr.lpSecurityDescriptor = nullptr;

  if (!::CreatePipe(&readPipe, &writePipe, &saAttr, 0) || !::SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0))
  {
    error_reporter("%s: cannot start process with <%s>", logtag, cmd);
    return false;
  }

  PROCESS_INFORMATION pi;
  STARTUPINFO si;

  ::ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = writePipe;
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  if (!::CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
  {
    error_reporter("%s: cannot start process with <%s>", logtag, cmd);
    return false;
  }
  ::CloseHandle(pi.hThread);
  ::CloseHandle(writePipe);

  String output;
  {
    char buf[1024];
    DWORD bytesRead = 0;
    while (::ReadFile(readPipe, buf, sizeof(buf), &bytesRead, nullptr) && bytesRead > 0)
    {
      output.append(buf, bytesRead);
    }
  }

  ::WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD res = -1;
  if (!GetExitCodeProcess(pi.hProcess, &res))
    res = -2;
  ::CloseHandle(pi.hProcess);

  if (res != 0)
  {
    error_reporter("%s: process <%s> returned %d", logtag, cmd.str(), res);
    if (!output.empty())
    {
      error_reporter("%s: process output:\n%s\n", logtag, output.str());
    }
    return false;
  }

  return true;
};

static bool run_process(String &cmd, const char *logtag, ILogWriter &log)
{
  return run_process(cmd, logtag,
    [&]<class... Ts>(const char *fmt, Ts &&...args) { log.addMessage(log.ERROR, fmt, eastl::forward<Ts>(args)...); });
}

static bool run_process(String &cmd)
{
  return run_process(cmd, nullptr, [&]<class... Ts>(const char *, Ts &&...) {});
}

static eastl::optional<Tab<String>> gather_all_shader_resources(const char *target, const char *tag, NBSType type, String &out_error)
{
  out_error.clear();
  String outFn(0, "%s.depdump.tmp", tag);
  FINALLY([&] { dd_erase(outFn); });
  const char *concretePluginType = "";
  if (type == NBSType::ENVI_COVER)
    concretePluginType = "envi_cover_";
  else if (type == NBSType::FOG)
    concretePluginType = "fog_";
  String cmd(0,
    "%s -dependencyDumpOnly -dshlShaderName:%s -singleInputJson:%s -singleOutputBin:%s -target:%s -p:%sshader_editor -silent "
    "-supressLogs",
    compilerExePath, tag, tag, outFn.c_str(), target, concretePluginType);
  if (
    !run_process(cmd, tag, [&]<class... Ts>(const char *fmt, Ts &&...args) { out_error.printf(0, fmt, eastl::forward<Ts>(args)...); }))
  {
    return eastl::nullopt;
  }
  FullFileLoadCB cb(outFn);
  if (!cb.fileHandle)
  {
    out_error.printf(0, "Failed to read dependency file from '%s'", outFn.c_str());
    return eastl::nullopt;
  }
  eastl::string content;
  content.resize(df_length(cb.fileHandle));
  cb.read(content.data(), content.size());
  eastl::string_view listing{content.data(), content.size()};
  NameMap names{};
  bool atEnd = false;
  for (size_t pos = 0, end = listing.find('\n'); !atEnd; pos = end + 1, end = listing.find('\n', pos))
  {
    if (end == eastl::string::npos)
    {
      end = listing.size();
      atEnd = true;
    }
    if (pos < end && listing[end - 1] == '\r')
      --end;
    if (pos >= end)
      continue;
    char sep = eastl::exchange(content.data()[end], '\0');
    names.addNameId(listing.data() + pos);
    content.data()[end] = sep;
  }
  Tab<String> uniqueFiles{};
  uniqueFiles.reserve(names.nameCount());
  iterate_names_in_id_order(names, [&](int, const char *name) {
    String s{name};
    if (s.suffix(".tmp"))
      return;
    dd_simplify_fname_c(s.data());
    s.updateSz();
    uniqueFiles.emplace_back(eastl::move(s));
  });

  return uniqueFiles;
}

static dag::ConstSpan<String> fetch_shader_resources(const char *target, NBSType type, String &out_error)
{
  if (!target)
    return dag::ConstSpan<String>{};
  G_ASSERT(type != NBSType::UNKNOWN);
  for (auto [platform, tgt] : enumerate(ALL_PLATFORM_NAMES))
  {
    if (strcmp(tgt, target) == 0)
    {
      auto &slot = perPlatformShaderTemplateSourceFiles[platform][uint32_t(type)];
      if (!slot)
      {
        slot = gather_all_shader_resources(target,
          eastl::string{eastl::string::CtorSprintf{}, "ec_%s_srcgather_%d_%d", target, thisPid, thisTid}.c_str(), type, out_error);
      }
      if (!slot)
      {
        return dag::ConstSpan<String>{};
      }
      out_error.clear();
      return *slot;
    }
  }
  return dag::ConstSpan<String>{};
}

class NodeBasedLocShaderExporter : public IDagorAssetExporter
{
public:
  const char *__stdcall getExporterIdStr() const override { return "lshader exp"; }

  const char *__stdcall getAssetType() const override { return TYPE; }
  unsigned __stdcall getGameResClassId() const override { return LShaderGameResClassId; }
  unsigned __stdcall getGameResVersion() const override
  {
    const int baseVer = 2;
    return baseVer + 2;
  }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = compilerExePath;
    for (const String &bin : compilerBinariesUsedByExe)
      files.push_back() = bin;
    if (!permutationsFileName.empty())
      files.push_back() = permutationsFileName;
    if (!nonOptionalFileName.empty())
      files.push_back() = nonOptionalFileName;
    for (const String &nonOptGraph : gatherNonOptionalGraphFilenames(a))
      files.push_back() = nonOptGraph;
    NBSType graphType = NBSType::UNKNOWN;
    if (!categorizationsFileName.empty())
    {
      files.push_back() = categorizationsFileName;
      String mainGraphName = extractGraphName(a.getTargetFilePath());
      graphType = getCategory(mainGraphName);
    }

    for (const eastl::pair<NBSType, Tab<Tab<String>>> &permutations : gatherOptionalGraphFilenames(a))
      if (graphType == NBSType::UNKNOWN || graphType == permutations.first)
        for (const Tab<String> &permGroups : permutations.second)
          for (const String optGraph : permGroups)
            files.push_back() = optGraph;

    for (const auto &sourceFile : gatherAllShaderResources(a, graphType))
      files.push_back() = sourceFile;
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

  eastl::vector_map<NBSType, Tab<Tab<String>>> gatherOptionalGraphFilenames(const DagorAsset &a, ILogWriter *log = nullptr)
  {
    eastl::vector_map<NBSType, Tab<Tab<String>>> optionalGraphs;
    const DataBlock *currentPermutations = permutationsFile.getBlockByNameEx(a.props.getStr("permutations", ""));
    int subgraphNameId = currentPermutations->getNameId("subgraph");
    if (currentPermutations->blockCount() == 0) // legacy
    {
      optionalGraphs[NBSType::UNKNOWN] = {};
      if (log && currentPermutations->paramCount() >= 16)
        log->addMessage(log->ERROR, "Only 15 optional graphs allowed. All other graphs are ignored");
      if (subgraphNameId != -1)
        for (uint32_t i = 0, ie = min<uint32_t>(currentPermutations->paramCount(), 16); i < ie; ++i)
          if (currentPermutations->getParamNameId(i) == subgraphNameId)
            optionalGraphs[NBSType::UNKNOWN].push_back({String(0, "%sdevelop/%s", appBlkDir, currentPermutations->getStr(i))});
    }
    else
    {
      for (uint32_t i = 0; i < currentPermutations->blockCount(); i++)
      {
        const DataBlock *currentBlock = currentPermutations->getBlock(i);
        if (auto it = types.find(currentBlock->getBlockName()); it == types.end() && log)
        {
          log->addMessage(log->ERROR, "Read a block with name: '%s' which is not a valid NBS type in permutation file: %s",
            currentBlock->getBlockName(), permutationsFileName.c_str());
        }
        NBSType type = types[currentBlock->getBlockName()];
        optionalGraphs[type] = {};

        const uint32_t groupNameId = currentBlock->getNameId("group");
        const uint32_t subgraphNameId = currentBlock->getNameId("subgraph");

        for (uint32_t permGroupIdx = 0; permGroupIdx < currentBlock->blockCount(); permGroupIdx++)
        {
          const DataBlock *permGroup = currentBlock->getBlock(permGroupIdx);
          if (log && permGroup->getBlockNameId() != groupNameId)
          {
            log->addMessage(log->ERROR, "Encountered block withing permutation definitation with name '%s' instead of 'group'",
              permGroup->getBlockName());
            continue;
          }
          Tab<String> groupGraphNames;
          for (uint32_t j = 0; j < permGroup->paramCount(); j++)
          {
            if (permGroup->getParamNameId(j) == subgraphNameId)
              groupGraphNames.push_back(String(0, "%sdevelop/%s", appBlkDir, permGroup->getStr(j)));
          }
          bool isExclusive = permGroup->getBool("isExclusive", true);
          if (!isExclusive)
          {
            for (const String &graphName : groupGraphNames)
              optionalGraphs[type].push_back({graphName});
          }
          else
          {
            optionalGraphs[type].push_back({groupGraphNames});
          }
        }
      }
    }
    return optionalGraphs;
  }

  NBSType getCategory(const String &fn, ILogWriter *log = nullptr)
  {
    if (categorizationsFile.isEmpty())
      return NBSType::UNKNOWN;

    for (auto &type : types)
    {
      const DataBlock *currentCategoryBlock = categorizationsFile.getBlockByName(type.first);
      if (currentCategoryBlock && !currentCategoryBlock->isEmpty())
      {
        int graphNameId = currentCategoryBlock->getNameId("graph");
        int regexNameId = currentCategoryBlock->getNameId("regex");
        for (uint32_t i = 0; i < currentCategoryBlock->paramCount(); i++)
        {
          if (currentCategoryBlock->getParamNameId(i) == graphNameId && strcmp(currentCategoryBlock->getStr(i), fn.c_str()) == 0)
            return type.second;

          if (currentCategoryBlock->getParamNameId(i) == regexNameId)
          {
            RegExp re;
            if (!re.compile(currentCategoryBlock->getStr(i)))
            {
              if (log)
              {
                log->addMessage(log->ERROR, "In permutation.blk tried to parse regex: '%s', but its invalid!",
                  currentCategoryBlock->getStr(i));
              }
              continue;
            }

            if (re.test(fn.c_str()))
              return type.second;
          }
        }
      }
    }
    return NBSType::UNKNOWN;
  }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  Tab<Tab<String>> mergeMap(const eastl::vector_map<NBSType, Tab<Tab<String>>> &permMap)
  {
    Tab<Tab<String>> output;
    for (const eastl::pair<NBSType, Tab<Tab<String>>> &permGroups : permMap)
      for (const Tab<String> &permGroup : permGroups.second)
        output.push_back(permGroup);

    return output;
  }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    if (!dependencyCollectionError.empty())
    {
      log.addMessage(log.ERROR, "Dependency collection failed, won't export asset: %s", dependencyCollectionError.c_str());
      return false;
    }

    String inputJson(a.getTargetFilePath());
    String out_tmp_fn(0, "%s.tmp.bin", a.getName());

    NBSType nbsType = getCategory(extractGraphName(a.getTargetFilePath()), &log);
    if (nbsType == NBSType::UNKNOWN)
      nbsType = snoopGraphTypeFromJson(a, inputJson, log);
    if (nbsType == NBSType::UNKNOWN)
      return false;

    const eastl::vector_map<NBSType, Tab<Tab<String>>> &permutationMap = gatherOptionalGraphFilenames(a, &log);

    DataBlock permutationNames;
    Tab<Tab<String>> permGroups{};

    if (auto it = permutationMap.find(nbsType); it != permutationMap.end())
      permGroups = it->second;

    uint32_t permAmount = 1;
    for (const Tab<String> &permGroup : permGroups)
      permAmount *= permGroup.size() + 1;

    permutationNames.addInt("permutationCnt", permAmount);
    permutationNames.addInt("permutationGroupCnt", permGroups.size());
    DataBlock *currentPermutationSizes = permutationNames.addBlock("permutationGroupSizes");
    for (int permGroupIdx = 0; permGroupIdx < permGroups.size(); permGroupIdx++)
    {
      currentPermutationSizes->addInt("groupSize", permGroups[permGroupIdx].size() + 1);
      for (int elemIdx = 0; elemIdx < permGroups[permGroupIdx].size(); elemIdx++)
      {
        DataBlock *currentPermutation = permutationNames.addNewBlock("permutation");
        currentPermutation->addStr("graph", extractGraphName(permGroups[permGroupIdx][elemIdx]));
        currentPermutation->addInt("groupIdx", permGroupIdx);
        currentPermutation->addInt("elemIdx", elemIdx + 1); // 0 is for none
      }
    }

    cwr.beginTaggedBlock(_MAKE4C('LSh'));
    cwr.writeInt32e(_MAKE4C('DShl'));
    permutationNames.saveToStream(cwr.getRawWriter());

    Tab<String> nonOptionalGraphs = gatherNonOptionalGraphFilenames(a);
    Tab<String> nonOptionalSubgraphs;
    for (int i = 0; i < nonOptionalGraphs.size(); ++i)
      nonOptionalSubgraphs.push_back(nonOptionalGraphs[i]);
    String nonOptional = join_str(nonOptionalSubgraphs, ";");


    String allPermutationSubgraphs;
    for (const auto &group : permGroups)
    {
      if (!allPermutationSubgraphs.empty())
        allPermutationSubgraphs += ';';
      allPermutationSubgraphs += '(';
      for (const auto &perm : group)
      {
        if (allPermutationSubgraphs[allPermutationSubgraphs.length() - 1] != '(')
          allPermutationSubgraphs += ':';
        allPermutationSubgraphs.append(perm);
      }
      allPermutationSubgraphs += ')';
    }
    if (!nonOptional.empty())
    {
      if (!allPermutationSubgraphs.empty())
        allPermutationSubgraphs += ';';
      allPermutationSubgraphs += nonOptional;
    }

    int fsize;
    void *buf;
    {
      const char *target = a.props.getStr("target", "");
      String outFn(0, "%s.tmp", a.getName());
      String outFnBindump(0, "%s%s.ps50.shdump.bin", outFn,
        strcmp(target, "spirvb") == 0 || strcmp(target, "metalb") == 0 ? ".bindless" : "");
      String cmd(0,
        "%s -dshlShaderName:%s -s:%s -singleInputJson:%s -singleOutputBin:%s -target:%s -p:shader_editor -silent "
        "-optionalGraphs:%s -supressLogs",
        compilerExePath, a.getName(), subgraphsFolder, inputJson, outFn.c_str(), target, allPermutationSubgraphs.str());

      dd_erase(outFnBindump);

      if (!run_process(cmd, a.getName(), log))
        return false;

      FullFileLoadCB cb(outFnBindump);
      if (!cb.fileHandle)
      {
        log.addMessage(log.ERROR, "%s: '%s' not found", a.getName(), outFnBindump);
        return false;
      }

      fsize = df_length(cb.fileHandle);
      buf = memalloc(fsize, tmpmem);
      cb.read(buf, fsize);
      cb.close();

      dd_erase(outFnBindump);
    }

    cwr.writeRaw(buf, fsize);
    memfree(buf, tmpmem);

    cwr.endBlock();

    return true;
  }

  String subgraphsFolder;
  String dependencyCollectionError;

private:
  Tab<String> gatherAllShaderResources(const DagorAsset &a, NBSType type)
  {
    Tab<String> templateNames;
    templateNames.push_back(String("shaderNodes/shaderNodesCommon.js"));
    if (type == NBSType::FOG || type == NBSType::UNKNOWN)
    {
      templateNames.push_back(String("shaderNodes/shaderNodesFog.js"));
      templateNames.push_back(String("fogShaderTemplate.dshl"));
    }

    if (type == NBSType::ENVI_COVER || type == NBSType::UNKNOWN)
    {
      templateNames.push_back(String("shaderNodes/shaderNodesEnviCover.js"));
      templateNames.push_back(String("enviCoverShaderTemplate.dshl"));
    }

    String editorPath = find_shader_editors_path();
    for (int i = 0; i < templateNames.size(); ++i)
      templateNames[i] = editorPath + "/" + templateNames[i];

    eastl::fixed_vector<NBSType, 2> eligibleTypes;
    if (type == NBSType::UNKNOWN)
    {
      eligibleTypes.push_back(NBSType::FOG);
      eligibleTypes.push_back(NBSType::ENVI_COVER);
    }
    else
    {
      eligibleTypes.push_back(type);
    }

    for (const auto t : eligibleTypes)
    {
      for (const auto &sourceDepFile : fetch_shader_resources(a.props.getStr("target", nullptr), t, dependencyCollectionError))
      {
        if (!dependencyCollectionError.empty())
          goto end;
        templateNames.push_back() = sourceDepFile;
      }
    }

  end:
    return templateNames;
  }

  NBSType snoopGraphTypeFromJson(const DagorAsset &a, const char *input_json, ILogWriter &log)
  {
    file_ptr_t json = df_open(input_json, DF_READ);
    if (!json)
    {
      log.addMessage(log.ERROR, "%s: cannot open <%s> to determine graph type", a.getName(), input_json);
      return NBSType::UNKNOWN;
    }
    String content{};
    content.resize(df_length(json));
    df_read(json, content.data(), content.length());
    df_close(json);

    bool isFog = content.find("[[plugin:fog_shader_editor]]") != nullptr;
    bool isEnviCover = content.find("[[plugin:envi_cover_shader_editor]]") != nullptr;

    if (isFog && isEnviCover)
    {
      log.addMessage(log.ERROR, "%s: multiple plugins in <%s>", a.getName(), input_json);
      return NBSType::UNKNOWN;
    }
    else if (!isFog && !isEnviCover)
    {
      log.addMessage(log.ERROR, "%s: no plugins in <%s>", a.getName(), input_json);
      return NBSType::UNKNOWN;
    }

    return isFog ? NBSType::FOG : NBSType::ENVI_COVER;
  }
};

class NodeBasedLocShaderRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "lshader refs"; }
  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override { return refs.clear(); }
};

class NodeBasedLocShaderExporterPlugin : public IDaBuildPlugin
{
public:
  bool __stdcall init(const DataBlock &appblk) override
  {
    dd_get_fname_location(appBlkDir, appblk.resolveFilename());

    types["envi_cover"] = NBSType::ENVI_COVER;
    types["volfog"] = NBSType::FOG;

    const DataBlock *lshaderBlk = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("lshader");

    const char *subgraphsFolderPtr = lshaderBlk->getStr("subgraphsFolder", nullptr);
    expNodeBased.subgraphsFolder = subgraphsFolderPtr ? String(0, "%sdevelop/%s", appBlkDir, subgraphsFolderPtr) : String();

    const char *permutationsFileNamePtr = lshaderBlk->getStr("permutationsFile", nullptr);
    permutationsFileName = permutationsFileNamePtr ? String(0, "%sdevelop/%s", appBlkDir, permutationsFileNamePtr) : String();

    const char *categorizationsFileNamePtr = lshaderBlk->getStr("categorizationsFile", nullptr);
    categorizationsFileName = categorizationsFileNamePtr ? String(0, "%sdevelop/%s", appBlkDir, categorizationsFileNamePtr) : String();

    const char *nonOptionalFileNamePtr = lshaderBlk->getStr("nonOptionalFile", nullptr);
    nonOptionalFileName = nonOptionalFileNamePtr ? String(0, "%sdevelop/%s", appBlkDir, nonOptionalFileNamePtr) : String();

    char exePath[1024];
    dag_get_appmodule_dir(exePath, sizeof(exePath));
    compilerExePath.printf(0, "%s/dsc2-nodeBased-dev.exe", exePath);
    for (const char *platform : {"hlsl11", "dx12", "spirv", "metal", "ps4", "ps5"})
      compilerBinariesUsedByExe.emplace_back(0, "%s/dsc2-%s-dev.exe", exePath, platform);
    if (!permutationsFileName.empty())
      permutationsFile.load(permutationsFileName);
    if (!nonOptionalFileName.empty())
      nonOptionalFile.load(nonOptionalFileName);
    if (!categorizationsFileName.empty())
      categorizationsFile.load(categorizationsFileName);

    thisPid = GetCurrentProcessId();
    thisTid = GetCurrentThreadId();

    return true;
  }

  void __stdcall destroy() override
  {
    types.clear();
    delete this;
  }

  int __stdcall getExpCount() override { return 1; }
  const char *__stdcall getExpType(int) override { return TYPE; }
  IDagorAssetExporter *__stdcall getExp(int) override { return &expNodeBased; }
  int __stdcall getRefProvCount() override { return 1; }
  const char *__stdcall getRefProvType(int) override { return TYPE; }
  IDagorAssetRefProvider *__stdcall getRefProv(int) override { return &refsNodeBased; }

protected:
  NodeBasedLocShaderExporter expNodeBased;
  NodeBasedLocShaderRefs refsNodeBased;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) NodeBasedLocShaderExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(locShader)
REGISTER_DABUILD_PLUGIN(locShader, nullptr)
#else
int pull_locShader = 1;
#endif
