// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_globalMutex.h>
#include <webui/shaderEditors.h>
#include <osApiWrappers/dag_localConv.h>
#include <libTools/util/fileUtils.h>
#include <ioSys/dag_fileIo.h>
#include <generic/dag_enumerate.h>
#include <util/dag_finally.h>
#include <util/dag_hash.h>

#include "../DebugLevel.h"

#include <shaders/dag_shBindumps.h>
ShaderBindumpHandle load_additional_shaders_bindump(dag::ConstSpan<uint8_t>, char const *) { return INVALID_BINDUMP_HANDLE; }
ShaderBindumpHandle load_additional_shaders_bindump(char const *, d3d::shadermodel::Version) { return INVALID_BINDUMP_HANDLE; }
bool reload_shaders_bindump(ShaderBindumpHandle, dag::ConstSpan<uint8_t>, char const *) { return false; }
bool reload_shaders_bindump(ShaderBindumpHandle, char const *, d3d::shadermodel::Version) { return false; }
void unload_shaders_bindump(ShaderBindumpHandle) {}
uint32_t get_shaders_bindump_generation(ShaderBindumpHandle) { return 0; }

static constexpr struct
{
  const char *platform = nullptr;
  const char *dscSuff = nullptr;
  const char *dumpSuff = nullptr;
  const char *additionalArgs = nullptr;
  bool skip = false;
} DSC_TABLE[NodeBasedShaderManager::PLATFORM::COUNT] = {
  {"dx11", "hlsl11", "", ""},
  {"", "", "", "", true},
  {"ps4", "ps4", "PS4", ""},
  {"spirv", "spirv", "SpirV", ""},
  {"dx12", "dx12", "DX12", ""},
  {"dx12x", "dx12", "DX12x", "-platform-xbox-one"},
  {"dx12xs", "dx12", "DX12xs", "-platform-xbox-scarlett"},
  {"ps5", "ps5", "PS5", ""},
  {"metal", "metal", "MTL", ""},
  {"spirvb", "spirv", "SpirV", "-enableBindless:on"},
  {"metalb", "metal", "MTL", "-enableBindless:on"},
};

static void show_header()
{
  printf("Dagor Node-based Shader Compiler 0.2\n"
         "Target: PC DirectX 11"
         "\n\nCopyright (C) Gaijin Games KFT, 2023\n");
}

static void showUsage()
{
  show_header();
  printf("\nUsage:\n"
         "  dsc2-nodeBased-dev.exe <options (-p: is required)>\n"
         "\n"
         "Common options:\n"
         "  -v - verbose\n"
         "  -f - force rebuild all files (even if not modified)\n"
         "  -g:<game folder> - set game folder\n"
         "  -s:<subgraphs folder> - set folder with subgraphs\n"
         "  -p:<plugin name> - set plugin name, for example -p:shader_editor\n"
         "  -singleInputJson:<graph file name> - set single graph file name (.json)\n"
         "  -singleOutputBin:<prefix of compiled shaders> - file name without extension\n"
         "  -dshlShaderName:<name for runtime usage> - sets shader name for dshl build, required\n"
         "  -listTargets - show all available build targets\n"
         "  -target:<build target> - set build target\n"
         "  -optionalGraphs:<graph filename[;other graph filename]> - names for optional graphs for permutations compilation\n");
}

struct Settings
{
  bool forceRebuild;
  bool verbose;
  bool listTargets;
  bool depDumpOnly;
  String target;
  String subgraphsFolder;
  String pluginName;
  String singleInputJson;
  String singleOutputBin;
  String dshlShaderName;
  NodeBasedShaderManager::PLATFORM platformId;
  String optionalGraphs;
};

static Settings settings;

void processArguments(int argc, char **argv)
{
  settings.forceRebuild = false;
  settings.verbose = false;
  settings.listTargets = false;
  settings.depDumpOnly = false;
  settings.platformId = NodeBasedShaderManager::PLATFORM::ALL;

  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-f") == 0)
      settings.forceRebuild = true;

    if (strcmp(argv[i], "-v") == 0)
      settings.verbose = true;

    if (strcmp(argv[i], "-listTargets") == 0)
      settings.listTargets = true;

    if (strcmp(argv[i], "-dependencyDumpOnly") == 0)
      settings.depDumpOnly = true;

    if (strncmp(argv[i], "-s:", 3) == 0)
      settings.subgraphsFolder.setStr(argv[i] + 3);

    if (strncmp(argv[i], "-p:", 3) == 0)
      settings.pluginName.setStr(argv[i] + 3);

    if (strncmp(argv[i], "-singleInputJson:", 17) == 0)
      settings.singleInputJson.setStr(argv[i] + 17);

    if (strncmp(argv[i], "-singleOutputBin:", 17) == 0)
      settings.singleOutputBin.setStr(argv[i] + 17);

    if (strncmp(argv[i], "-target:", 8) == 0)
      settings.target.setStr(argv[i] + 8);

    if (strncmp(argv[i], "-optionalGraphs:", 16) == 0)
      settings.optionalGraphs.setStr(argv[i] + 16);

    if (strncmp(argv[i], "-dshlShaderName:", 16) == 0)
      settings.dshlShaderName.setStr(argv[i] + 16);
  }

  if (settings.verbose)
  {
    printf("V: forceRebuild=%d\n", int(settings.forceRebuild));
    printf("V: verbose=%d\n", int(settings.verbose));
    printf("V: listTargets=%d\n", int(settings.listTargets));
    printf("V: depDumpOnly=%d\n", int(settings.depDumpOnly));
    printf("V: target=%s\n", settings.target.str());
    printf("V: subgraphsFolder=%s\n", settings.subgraphsFolder.str());
    printf("V: pluginName=%s\n", settings.pluginName.str());
    printf("V: singleInputJson=%s\n", settings.singleInputJson.str());
    printf("V: singleOutputBin=%s\n", settings.singleOutputBin.str());
  }

  if (settings.listTargets)
  {
    for (const auto &entry : DSC_TABLE)
      if (!entry.skip)
        printf("%s\n", entry.platform);

    exit(0);
  }

  if (settings.singleInputJson.empty())
  {
    printf("\nERROR: '-singleInputJson:' must be set\n");
    exit(1);
  }
  if (settings.singleOutputBin.empty())
  {
    printf("\nERROR: '-singleOutputBin:' must be set\n");
    exit(1);
  }
  if (settings.dshlShaderName.empty())
  {
    printf("\nERROR: '-dshlShaderName:' must be set\n");
    exit(1);
  }

  if (!settings.singleOutputBin.empty() && settings.target.empty())
  {
    printf("ERROR: '-singleOutputBin' is set but '-target' is missed\n");
    exit(1);
  }

  if (!settings.target.empty())
  {
    for (const auto &[i, entry] : enumerate(DSC_TABLE))
      if (!entry.skip && !strcmp(entry.platform, settings.target.str()))
        settings.platformId = NodeBasedShaderManager::PLATFORM(i);

    if (settings.platformId == NodeBasedShaderManager::PLATFORM::ALL)
    {
      printf("ERROR: unknown target '%s'. Call with '-listTarget' to list available targets\n", settings.target.str());
      exit(1);
    }
  }

  if (settings.subgraphsFolder.empty() && !settings.depDumpOnly)
    printf("WARNING: subgraphs folder is not set (-s:)\n");
}

static String readFileToString(const char *file_name)
{
  file_ptr_t h = df_open(file_name, DF_READ | DF_IGNORE_MISSING);
  if (!h)
  {
    printf("WARNING: cannot open file '%s'\n", file_name);
    return String("");
  }

  int len = df_length(h);

  String res;
  res.resize(len + 1);
  df_read(h, &res[0], len);
  res.back() = '\0';

  df_close(h);
  return res;
}

static String getSubstring(const char *str, const char *beginMarker, const char *endMarker)
{
  const char *begin = strstr(str, beginMarker);
  if (begin)
  {
    const char *end = strstr(begin, endMarker);
    if (end)
    {
      String res(tmpmem);
      res.setSubStr(begin + strlen(beginMarker), end);
      res.replaceAll("\\\\", "\x01");
      res.replaceAll("\\n", "\n");
      res.replaceAll("\\\"", "\"");
      res.replaceAll("\x01", "\\");
      return res;
    }
  }

  return String("");
}

bool is_correct_plugin(const String &shader_filename, String &plugin_name)
{
  const String shaderFile = readFileToString(shader_filename);
  if (settings.pluginName == "shader_editor")
  {
    String fogPluginSearchStr(0, "\"pluginId\": \"[[plugin:%s]]\"", "fog_shader_editor");
    if (strstr(shaderFile.str(), fogPluginSearchStr.str()) != nullptr)
    {
      plugin_name = "fog_shader_editor";
      return true;
    }
    String enviCoverPluginSearchStr(0, "\"pluginId\": \"[[plugin:%s]]\"", "envi_cover_shader_editor");
    if (strstr(shaderFile.str(), enviCoverPluginSearchStr.str()) != nullptr)
    {
      plugin_name = "envi_cover_shader_editor";
      return true;
    }
  }
  else
  {
    String pluginSearchStr(0, "\"pluginId\": \"[[plugin:%s]]\"", settings.pluginName.str());
    if (strstr(shaderFile.str(), pluginSearchStr.str()) != nullptr)
    {
      plugin_name = settings.pluginName;
      return true;
    }
  }

  return false;
}

DataBlock read_shader_data_block(const String &shader_filename, NodeBasedShaderType &shader_type)
{
  const String shaderFile = readFileToString(shader_filename);
  String shaderBlk = getSubstring(shaderFile, "/*SHADER_BLK_START*/", "/*SHADER_BLK_END*/");
  if (shaderBlk.empty())
  {
    printf("\nERROR: Shader %s is empty\nExiting...\n", shader_filename.str());
    exit(1);
  }

  String shaderTypeName = getSubstring(shaderFile, "[[plugin:", "]]");
  if (shaderTypeName == FOG_SHADER_EDITOR_PLUGIN_NAME)
    shader_type = NodeBasedShaderType::Fog;
  else if (shaderTypeName == ENVI_COVER_SHADER_EDITOR_PLUGIN_NAME)
    shader_type = NodeBasedShaderType::EnviCover;
  else
    G_ASSERTF(false, "shader type can't be determined! %s", shader_filename.str());

  if (settings.verbose)
    printf("V: read shader from %s\n", shader_filename.str());
  DataBlock result;
  dblk::load_text(result, shaderBlk);
  return result;
}

static String getShaderNodesJS(NodeBasedShaderType type, char *exePath)
{
  String result;
  switch (type)
  {
    case NodeBasedShaderType::Fog:
      result.aprintf(0, "%s\\..\\..\\..\\prog\\gameLibs\\webui\\plugins\\shaderEditors\\shaderNodes\\shaderNodesFog.js ", exePath);
      break;
    case NodeBasedShaderType::EnviCover:
      result.aprintf(0, "%s\\..\\..\\..\\prog\\gameLibs\\webui\\plugins\\shaderEditors\\shaderNodes\\shaderNodesEnviCover.js ",
        exePath);
      break;
  }
  result.aprintf(0, "%s\\..\\..\\..\\prog\\gameLibs\\webui\\plugins\\shaderEditors\\shaderNodes\\shaderNodesCommon.js ", exePath);
  return result;
}

static NodeBasedShaderType plug_name_to_type(char const *plug_name)
{
  if (strcmp(plug_name, FOG_SHADER_EDITOR_PLUGIN_NAME) == 0)
    return NodeBasedShaderType::Fog;
  else if (strcmp(plug_name, ENVI_COVER_SHADER_EDITOR_PLUGIN_NAME) == 0)
    return NodeBasedShaderType::EnviCover;
  else
  {
    printf("ERROR: plugin in file '%s' is not recognized as a valid shader editor type!", settings.singleInputJson.str());
    exit(1);
    return NodeBasedShaderType::Fog;
  }
}

int DagorWinMain(bool)
{
  // get options
  if (__argc < 2)
  {
    showUsage();
    return 1;
  }

  if (dd_stricmp(__argv[1], "-h") == 0 || dd_stricmp(__argv[1], "-H") == 0 || dd_stricmp(__argv[1], "/?") == 0)
  {
    showUsage();
    return 0;
  }

  processArguments(__argc, __argv);
  int res = 0;

  char exePath[1024];
  dag_get_appmodule_dir(exePath, sizeof(exePath));

  String pluginName;
  if (settings.depDumpOnly)
  {
    pluginName = settings.pluginName; // Save file read
  }
  else if (!is_correct_plugin(settings.singleInputJson, pluginName))
  {
    printf("ERROR: plugin in file '%s' is incorrect, expected '%s'", settings.singleInputJson.str(), settings.pluginName.str());
    return 1;
  }

  String outputDshl = settings.singleInputJson + "." + settings.target + ".tmp";
  String outputDscBlk = settings.singleInputJson + "." + settings.target + ".blk.tmp";

  // We need to use pluginName to identify shaderType as outputJson does not exist yet.
  NodeBasedShaderType shaderType = plug_name_to_type(pluginName);

  String dshl;
  DataBlock shaderBlk;

  if (!settings.depDumpOnly)
  {
    String outputJson = settings.singleInputJson + "." + settings.target + ".compiled.tmp";
    FINALLY([&] { dd_erase(outputJson.str()); });

    String cmd(0,
      "call %s\\duktape.exe "
      "%s\\..\\..\\..\\prog\\gameLibs\\webui\\plugins\\grapheditor\\editorScripts\\offlineEditorApiStub.js "
      "%s" // Shader type based shaderNode variant
      "%s\\..\\..\\..\\prog\\gameLibs\\webui\\plugins\\grapheditor\\editorScripts\\nodeUtils.js "
      "%s\\..\\..\\..\\prog\\gameLibs\\webui\\plugins\\grapheditor\\editorScripts\\graphEditor.js "
      "%s\\..\\..\\..\\prog\\gameLibs\\webui\\plugins\\grapheditor\\editorScripts\\rebuildShaderCode.js "
      "pluginName=%s globalSubgraphsDir=%s rootFileName=%s outputFileName=%s %s",
      exePath, exePath, getShaderNodesJS(shaderType, exePath).str(), exePath, exePath, exePath, pluginName.str(),
      settings.subgraphsFolder.str(), settings.singleInputJson.str(), outputJson.str(),
      (String("includes=") + settings.optionalGraphs).str());

    if (settings.verbose)
      printf("\nexecuting: %s\n\n", cmd.str());

    res += system(cmd.str());
    if (res)
    {
      printf("ERROR: failed to run rebuildShaderCode.js");
      return 1;
    }

    String errors;
    shaderBlk = read_shader_data_block(outputJson, shaderType);
  }

  shaderBlk.setStr("shader_name", "\n" + settings.dshlShaderName);
  // @TODO: remove this lib-level dependency -- header should be enough, the substitution code is trivial
  dshl = ShaderGraphRecompiler::substituteDshl(shaderType, shaderBlk);

  String dscBlk = String(0,
    R"(
          shader_root_dir:t="."
          outDumpName:t="%s"
          engineRootDir:t="%s/../../.."
          compileCppStcode:b=no
          source {
            file:t="%s"
            includePath:t="%s/../../../prog/gameLibs/webui/plugins/shaderEditors"
            includePath:t="%s/../../../prog/gameLibs/render/shaders"
            includePath:t="%s/../../../prog/gameLibs/daSDF/shaders"
          }
          Compile {
            fsh:t = 5.0
            additional_dump:b = yes
          }
        )",
    settings.singleOutputBin, exePath, outputDshl, exePath, exePath, exePath);

  FINALLY([&] {
    dd_erase(outputDshl.str());
    dd_erase(outputDscBlk.str());
  });

  auto writeFile = [](const String &name, const String &content) {
    FullFileSaveCB file(name.str());
    if (!file.fileHandle)
    {
      G_ASSERTF(0, "Cannot write to file '%s'", name.str());
      return false;
    }

    file.write(content.str(), content.length());
    return true;
  };

  if (!writeFile(outputDshl, dshl))
    return 1;
  if (!writeFile(outputDscBlk, dscBlk))
    return 1;

  auto makeHashedName = [](const auto &s) { return String{0, "lsh.%x", str_hash_fnv1<64>(s.c_str())}; };

  char const *forceArgs = settings.forceRebuild ? "-r -no_sha1_cache" : "";

  // @TODO: fork join for speed (separate tmp files will be needed)
  for (int platformId =
         (settings.platformId == NodeBasedShaderManager::PLATFORM::ALL ? NodeBasedShaderManager::PLATFORM::DX11 : settings.platformId);
       platformId < (settings.platformId == NodeBasedShaderManager::PLATFORM::ALL ? NodeBasedShaderManager::PLATFORM::COUNT
                                                                                  : settings.platformId + 1);
       ++platformId)
  {
    if (DSC_TABLE[platformId].skip)
      continue;

    struct Sha1CacheMutex
    {
      void *mutex = nullptr;
      String name{};
      explicit Sha1CacheMutex(int platform_id) : name{String("dsc_sha1_") + String(DSC_TABLE[platform_id].platform)}
      {
        if (!settings.depDumpOnly)
          mutex = global_mutex_create_enter(name.str());
      }
      ~Sha1CacheMutex()
      {
        if (!settings.depDumpOnly)
          global_mutex_leave_destroy(mutex, name.str());
      }
    } lock{platformId};

    String cmd(0,
      "call %s\\dsc2-%s-dev.exe %s -q -shaderOn -nodisassembly -commentPP -codeDumpErr -r -cj0 %s -o "
      "%s\\..\\..\\..\\_output\\lshader\\shaders~%s~%s -quiet -supressLogs %s",
      exePath, DSC_TABLE[platformId].dscSuff, outputDscBlk, DSC_TABLE[platformId].additionalArgs, exePath,
      DSC_TABLE[platformId].platform, makeHashedName(outputDshl).str(), forceArgs);

    if (settings.depDumpOnly)
    {
      cmd += " -dependencyDumpMode -dependencyDumpFile ";
      cmd += settings.singleOutputBin;
    }

    if (settings.verbose)
      printf("\nexecuting: %s\n\n", cmd.str());

    res += system(cmd.str());
    if (res)
    {
      printf("ERROR: failed to run dsc");
      return 1;
    }
  }

  if (settings.verbose)
    printf("V: done\n");

  return res;
}
