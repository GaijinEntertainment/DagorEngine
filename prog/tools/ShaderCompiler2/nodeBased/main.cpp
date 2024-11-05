// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <webui/shaderEditors.h>
#include <osApiWrappers/dag_localConv.h>
#include <libTools/util/fileUtils.h>

#include "../DebugLevel.h"

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
         "  dsc2-nodeBased-dev.exe <options (-g: and -p: are required)>\n"
         "\n"
         "Common options:\n"
         "  -v - verbose\n"
         "  -f - force rebuild all files (even if not modified)\n"
         "  -u - update in-graph shader code\n"
         "  -g:<game folder> - set game folder\n"
         "  -s:<subgraphs folder> - set folder with subgraphs\n"
         "  -p:<plugin name> - set plugin name, for example -p:shader_editor\n"
         "  -singleInputJson:<graph file name> - set single graph file name (.json)\n"
         "  -singleOutputBin:<prefix of compiled shaders> - file name without extension\n"
         "  -listTargets - show all available build targets\n"
         "  -target:<build target> - set build target\n"
         "  -silent - don't show names of compiled binaries\n"
         "  -optionalGraphs:<graph filename[;other graph filename]> - names for optional graphs for permutations compilation\n");
}

struct Settings
{
  bool forceRebuild;
  bool updateGraphs;
  bool verbose;
  bool listTargets;
  bool silent;
  String target;
  String gameFolder;
  String subgraphsFolder;
  String pluginName;
  String singleInputJson;
  String singleOutputBin;
  NodeBasedShaderManager::PLATFORM platformId;
  String optionalGraphs;
};

static Settings settings;

void processArguments(int argc, char **argv)
{
  settings.forceRebuild = false;
  settings.updateGraphs = false;
  settings.verbose = false;
  settings.listTargets = false;
  settings.silent = false;
  settings.platformId = NodeBasedShaderManager::PLATFORM::ALL;

  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-f") == 0)
      settings.forceRebuild = true;

    if (strcmp(argv[i], "-u") == 0)
      settings.updateGraphs = true;

    if (strcmp(argv[i], "-v") == 0)
      settings.verbose = true;

    if (strcmp(argv[i], "-silent") == 0)
      settings.silent = true;

    if (strcmp(argv[i], "-listTargets") == 0)
      settings.listTargets = true;

    if (strncmp(argv[i], "-g:", 3) == 0)
      settings.gameFolder.setStr(argv[i] + 3);

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
  }

  if (settings.verbose)
  {
    printf("V: forceRebuild=%d\n", int(settings.forceRebuild));
    printf("V: updateGraphs=%d\n", int(settings.updateGraphs));
    printf("V: verbose=%d\n", int(settings.verbose));
    printf("V: silent=%d\n", int(settings.silent));
    printf("V: listTargets=%d\n", int(settings.listTargets));
    printf("V: target=%s\n", settings.target.str());
    printf("V: gameFolder=%s\n", settings.gameFolder.str());
    printf("V: subgraphsFolder=%s\n", settings.subgraphsFolder.str());
    printf("V: pluginName=%s\n", settings.pluginName.str());
    printf("V: singleInputJson=%s\n", settings.singleInputJson.str());
    printf("V: singleOutputBin=%s\n", settings.singleOutputBin.str());
  }

  if (settings.listTargets)
  {
    Tab<String> platforms;
    NodeBasedShaderManager::getPlatformList(platforms, NodeBasedShaderManager::PLATFORM::ALL);
    for (int i = 0; i < platforms.size(); i++)
      printf("%s\n", platforms[i].str());

    exit(0);
  }

  if (!settings.singleOutputBin.empty() && settings.target.empty())
  {
    printf("ERROR: '-singleOutputBin' is set but '-target' is missed\n");
    exit(1);
  }

  if (!settings.target.empty())
  {
    Tab<String> platforms;
    NodeBasedShaderManager::getPlatformList(platforms, NodeBasedShaderManager::PLATFORM::ALL);
    for (int i = 0; i < platforms.size(); i++)
      if (!strcmp(platforms[i].str(), settings.target.str()))
        settings.platformId = NodeBasedShaderManager::PLATFORM(i);

    if (settings.platformId == NodeBasedShaderManager::PLATFORM::ALL)
    {
      printf("ERROR: unknown target '%s'. Call with '-listTarget' to list available targets\n", settings.target.str());
      exit(1);
    }
  }

  if (settings.singleInputJson.empty() != settings.singleOutputBin.empty())
  {
    printf("\nERROR: both arguments -singleInputJson: and -singleOutputBin: must be set\n");
    exit(1);
  }

  if (settings.singleInputJson.empty())
    if (settings.gameFolder.empty())
    {
      printf("\nERROR: game folder is not set (-g:)\n");
      exit(1);
    }

  if (settings.pluginName.empty())
  {
    printf("\nERROR: plugin name is not set (-p:)\n");
    exit(1);
  }

  if (settings.subgraphsFolder.empty())
    printf("WARNING: subgraphs folder is not set (-s:)\n");
}

bool updateGraph(const char *graph_name, const char *output_name)
{
  if (settings.verbose)
    printf("V: update graph %s\n", graph_name);

  String cmd;
  cmd.aprintf(0, "set \"ROOTJSON=%s\" && ", graph_name);
  cmd.aprintf(0, "set \"OUTPUTJSON=%s\" && ", output_name);
  cmd.aprintf(0, "set \"PLUGIN=%s\" && ", settings.pluginName.str());
  cmd.aprintf(0, "set \"SUBGRAPHSDIR=%s\" && ", settings.subgraphsFolder.str());
  cmd.aprintf(0, "pushd ..\\..\\graphEditor\\offlineGraphUpdater && call rebuildGraph.bat && popd");

  if (settings.verbose)
    printf("V: executing: %s\n", cmd.str());

  int res = system(cmd);

  if (settings.verbose)
    printf("V: result = %d\n", res);

  return res == 0;
}

eastl::vector<String> findLevelDirectories(const String &game_folder)
{
  eastl::vector<String> levelDirectories;
  alefind_t file;
  const String fileMask = game_folder + "/content/*";
  for (int lastErrCode = ::dd_find_first(fileMask, DA_SUBDIR, &file); lastErrCode != 0; lastErrCode = dd_find_next(&file))
  {
    levelDirectories.push_back(String(file.name));
    if (settings.verbose)
      printf("V: level directory: %s\n", file.name);
  }
  return levelDirectories;
}

String extractFilenameWithoutExt(String filename)
{
  char filenameBuffer[270] = {0};
  dd_get_fname_without_path_and_ext(filenameBuffer, 260, filename.str());
  do
  {
    filename = String(filenameBuffer);
    dd_get_fname_without_path_and_ext(filenameBuffer, 260, filename.str());
  } while (filename != String(filenameBuffer));
  return filename;
}

struct StringLessComparator
{
  bool operator()(const String &s1, const String &s2) const
  {
    bool less = true;
    const int minLength = static_cast<int>(min(s1.size(), s2.size()));
    for (int i = 0; i < minLength && less; ++i)
      less &= s1[i] <= s2[i];
    return less && s1 != s2 && s1.size() <= s2.size();
  }
};

eastl::vector<String> findShaderSources(const String &root)
{
  eastl::vector<String> shaderFiles;
  for (const String &levelDir : findLevelDirectories(root))
  {
    alefind_t file;
    const String fileDir = root + "/content/" + levelDir + "/shaders/";

    eastl::map<String, int64_t, StringLessComparator> compiledShadersModificationTime;
    if (!settings.forceRebuild)
    {
      const String binFileMask = fileDir + "*.bin";
      for (int lastErrCode = ::dd_find_first(binFileMask, 0, &file); lastErrCode != 0; lastErrCode = dd_find_next(&file))
        compiledShadersModificationTime[extractFilenameWithoutExt(String(file.name))] = file.mtime;
    }

    const String fileMask = fileDir + "*.json";
    for (int lastErrCode = ::dd_find_first(fileMask, 0, &file); lastErrCode != 0; lastErrCode = dd_find_next(&file))
    {
      String s = fileDir + file.name;
      if (settings.forceRebuild || compiledShadersModificationTime[extractFilenameWithoutExt(String(file.name))] <= file.mtime)
      {
        shaderFiles.push_back(s);
        if (settings.verbose)
          printf("V: will be processed: %s\n", s.str());
      }
      else
      {
        if (settings.verbose)
          printf("V: will be ignored: %s\n", s.str());
      }
    }
  }
  return shaderFiles;
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
  else
    G_ASSERTF(false, "shader type can't be determined! %s", shader_filename.str());

  if (settings.verbose)
    printf("V: read shader from %s\n", shader_filename.str());
  DataBlock result;
  dblk::load_text(result, shaderBlk);
  return result;
}


String get_parent_dir_prefix(String filename)
{
  String res("");
  for (int i = 0; i < 16; i++)
  {
    if (dd_file_exists(res + filename))
      return res;
    res += "..\\";
  }

  return res;
}


static void get_include_file_names(const String &root_file, Tab<String> &includeGraphNames)
{
  // test include
  String rootGraphDir = root_file;
  char *p = max(strrchr(rootGraphDir.str(), '\\'), strrchr(rootGraphDir.str(), '/'));
  if (p)
    *p = 0;
  else
    clear_and_shrink(rootGraphDir);

  includeGraphNames.push_back(String(0, "%s/%s", rootGraphDir.str(), "empty_include_graph.json"));
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


int DagorWinMain(bool debugmode)
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

  NodeBasedShaderManager::initCompilation();

  if (!settings.singleInputJson.empty()) // soon there will be only this codepass
  {
    int res = 0;

    String pluginName;
    if (!is_correct_plugin(settings.singleInputJson, pluginName))
    {
      printf("ERROR: plugin in file '%s' is incorrect, expected '%s'", settings.singleInputJson.str(), settings.pluginName.str());
      return 1;
    }

    String outputJson = settings.singleInputJson + ".compiled.tmp";

    char exePath[1024];
    dag_get_appmodule_dir(exePath, sizeof(exePath));

    String cmd(0,
      "call %s\\duktape.exe "
      "%s\\..\\commonData\\graphEditor\\builder\\offlineEditorApiStub.js "
      "%s\\..\\commonData\\graphEditor\\builder\\shaderNodes.js "
      "%s\\..\\commonData\\graphEditor\\builder\\nodeUtils.js "
      "%s\\..\\commonData\\graphEditor\\builder\\graphEditor.js "
      "%s\\..\\commonData\\graphEditor\\builder\\rebuildShaderCode.js "
      "pluginName=%s globalSubgraphsDir=%s rootFileName=%s outputFileName=%s %s",
      exePath, exePath, exePath, exePath, exePath, exePath, pluginName.str(), settings.subgraphsFolder.str(),
      settings.singleInputJson.str(), outputJson.str(), (String("includes=") + settings.optionalGraphs).str());

    if (settings.verbose)
      printf("\nexecuting: %s\n\n", cmd.str());

    res += system(cmd.str());
    if (res)
    {
      dd_erase(outputJson.str());
      printf("ERROR: failed to run rebuildShaderCode.js");
      return 1;
    }

    String errors;
    NodeBasedShaderType shaderType;
    const DataBlock shaderBlk = read_shader_data_block(outputJson, shaderType);
    NodeBasedShaderManager shaderManager(shaderType, get_shader_name(shaderType), get_shader_suffix(shaderType));

    const bool compilationSuccess = shaderManager.update(shaderBlk, errors, settings.platformId);

    dd_erase(outputJson.str());

    if (!compilationSuccess)
    {
      printf("\nERROR: Compilation Error: %s\nExiting...\n", errors.str());
      exit(1);
    }
    else
    {
      shaderManager.saveToFile(settings.singleOutputBin, settings.platformId);
      if (settings.verbose)
        printf("V: saveShaders %s\n", settings.singleOutputBin.str());

      Tab<String> fileNames;
      shaderManager.getShadersBinariesFileNames(settings.singleOutputBin, fileNames, settings.platformId);
      if (!settings.silent)
        for (int i = 0; i < fileNames.size(); i++)
          printf("%s\n", fileNames[i].str());
    }

    return res;
  }


  const eastl::vector<String> shaderFiles = findShaderSources(settings.gameFolder);

  if (settings.verbose)
    printf("V: total files to build: %d\n", int(shaderFiles.size()));


  for (String shaderFile : shaderFiles)
  {
    bool eraseTmp = false;
    char shaderName[270] = {0};
    dd_get_fname_without_path_and_ext(shaderName, 260, shaderFile.str());
    printf("Shader name: %s\n", dd_get_fname(shaderName));
    char shaderLocation[270] = {0};
    dd_get_fname_location(shaderLocation, shaderFile.str());

    String pluginName;
    if (!is_correct_plugin(shaderFile, pluginName))
    {
      if (settings.verbose)
        printf("V: Plugin mismatch. Skipping...\n");

      continue;
    }

    if (settings.updateGraphs)
    {
      eraseTmp = true;
      String outFile;
      outFile.printf(0, "%s.compiled.tmp", shaderFile.str());
      updateGraph(shaderFile, outFile);
      shaderFile = outFile.str();
    }

    String errors;
    NodeBasedShaderType shaderType;
    const DataBlock shaderBlk = read_shader_data_block(shaderFile, shaderType);
    NodeBasedShaderManager shaderManager(shaderType, get_shader_name(shaderType), get_shader_suffix(shaderType));
    const bool compilationSuccess = shaderManager.update(shaderBlk, errors, settings.platformId);

    if (eraseTmp)
    {
      if (settings.verbose)
        printf("V: erasing %s\n", shaderFile.str());

      dd_erase(shaderFile);
    }

    if (!compilationSuccess)
    {
      printf("\nERROR: Compilation Error: %s\nExiting...\n", errors.str());
      exit(1);
    }
    else
    {
      shaderManager.saveToFile(String(shaderName), settings.platformId);
      if (settings.verbose)
        printf("V: saveShaders %s\n", shaderName);
    }
  }

  if (settings.verbose)
    printf("V: done\n");

  return 0;
}

// stub for func used in NodeBasedShaderManager::resetCachedResources()
void release_managed_res(D3DRESID id) {}

#if !HAS_PS4_PS5_TRANSCODE
bool compile_compute_shader_ps5(const char *, unsigned, const char *, const char *, Tab<uint32_t> &, String &) { return false; }
bool compile_compute_shader_ps4(const char *, unsigned, const char *, const char *, Tab<uint32_t> &, String &) { return false; }
#endif
#if !HAS_XBOX_TRANSCODE
bool compile_compute_shader_xboxone(const char *, unsigned, const char *, const char *, Tab<uint32_t> &, String &) { return false; }
#endif
