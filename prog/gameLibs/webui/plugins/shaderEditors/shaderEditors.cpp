// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/shaderEditors.h>

#if !NBSM_COMPILE_ONLY
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include "webui/graphEditorPlugin.h"
#include <osApiWrappers/dag_direct.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_platform.h>
#include <shaders/dag_shaders.h>
#include <startup/dag_globalSettings.h>
#endif

#include <render/nodeBasedShader.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <EASTL/vector.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/unique_ptr.h>

#define NBS_PERM_CONST(VAR, VALUE) static constexpr int VAR = VALUE
#include <nbsPermutations/node_based_perm_inc.hlsli>
#undef NBS_PERM_CONST

ShaderGraphRecompiler *create_fog_shader_recompiler();
ShaderGraphRecompiler *create_envi_cover_shader_recompiler();
ShaderGraphRecompiler *create_clouds_shader_recompiler();

ShaderGraphRecompiler *ShaderGraphRecompiler::activeInstance = nullptr;

#if !NBSM_COMPILE_ONLY
static eastl::unique_ptr<ShaderGraphRecompiler> g_fog_instance;
static eastl::unique_ptr<ShaderGraphRecompiler> g_envi_cover_instance;
static eastl::unique_ptr<ShaderGraphRecompiler> g_clouds_instance;
#endif

String get_template_text_src_fog(uint32_t variant_id, NodeBasedShaderQuality nbs_quality);
String get_template_text_src_envi_cover(uint32_t variant_id, NodeBasedShaderQuality nbs_quality);
String get_template_text_src_clouds(uint32_t variant_id, NodeBasedShaderQuality nbs_quality);
String get_dshl_template_text_src_fog();
String get_dshl_template_text_src_envi_cover();
String get_dshl_template_text_src_clouds();

String find_shader_editors_path()
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

static String read_file_to_string(String &file_name)
{
  file_ptr_t h = df_open(file_name, DF_READ | DF_IGNORE_MISSING);
  if (!h)
    return String("");

  int len = df_length(h);

  String res;
  res.resize(len + 1);
  df_read(h, &res[0], len);
  res.back() = '\0';

  df_close(h);
  return res;
}


String add_nbs_quality_definition(NodeBasedShaderQuality nbs_quality)
{
  return String(32, "#define NBS_QUALITY %d", static_cast<int>(nbs_quality));
};

String collect_template_files(const String &template_dir, const Tab<String> &template_names)
{
  String res;
  for (int i = 0; i < template_names.size(); i++)
  {
    String fullPath = template_dir + "/" + template_names[i];
    if (!dd_file_exists(fullPath.str()))
      return "#error Shader source file \"" + fullPath +
             "\" not found! NBS compilation requires developer's environment (GIT source)!\n";

    res += "// include: ";
    res += fullPath;
    res += "\n";
    res += read_file_to_string(fullPath);
    res += "\n\n";
  }

  return res;
}


static String get_template_ps4_header()
{
  Tab<String> templateNames;
  templateNames.push_back(String("ps4Defines.hlsl"));
  return collect_template_files(find_shader_editors_path(), templateNames);
}

static String get_template_text_src(NodeBasedShaderType shader, uint32_t variant_id, NodeBasedShaderQuality nbs_quality)
{
  switch (shader)
  {
    case NodeBasedShaderType::Fog: return get_template_text_src_fog(variant_id, nbs_quality);
    case NodeBasedShaderType::EnviCover: return get_template_text_src_envi_cover(variant_id, nbs_quality);
    case NodeBasedShaderType::Clouds: return get_template_text_src_clouds(variant_id, nbs_quality);
    default: G_ASSERTF(false, "Implement this shader type here!"); return String("");
  }
}

static String get_dshl_template_text_src(NodeBasedShaderType shader)
{
  switch (shader)
  {
    case NodeBasedShaderType::Fog: return get_dshl_template_text_src_fog();
    case NodeBasedShaderType::EnviCover: return get_dshl_template_text_src_envi_cover();
    case NodeBasedShaderType::Clouds: return get_dshl_template_text_src_clouds();
    default: G_ASSERTF(false, "Implement this shader type here!"); return String("");
  }
}

String ShaderGraphRecompiler::enumerateLines(const char *s)
{
  String res;
  int num = 1;
  int len = int(strlen(s));
  res.reserve(int(len * 1.1f));
  res.setStr("   1 ");
  for (int i = 0; i < len; i++)
  {
    if (s[i] == '\n')
    {
      num++;
      res.aprintf(16, "\n%4d ", num);
    }
    else
      res += s[i];
  }

  return res;
}

String ShaderGraphRecompiler::substitute(NodeBasedShaderType shader, uint32_t variant_id, NodeBasedShaderQuality nbs_quality,
  const DataBlock &shader_blk)
{
  return substitute(shader_blk, get_template_text_src(shader, variant_id, nbs_quality));
}

String ShaderGraphRecompiler::substitutePs4(NodeBasedShaderType shader, uint32_t variant_id, NodeBasedShaderQuality nbs_quality,
  const DataBlock &shader_blk)
{
  return substitute(shader_blk, String(get_template_ps4_header()) + get_template_text_src(shader, variant_id, nbs_quality));
}

String ShaderGraphRecompiler::substitute(const DataBlock &shader_blk, String shader_template)
{
  for (int i = 0; i < shader_blk.paramCount(); i++)
    if (shader_blk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      String from(64, "[[%s]]", shader_blk.getParamName(i));
      shader_template.replaceAll(from, shader_blk.getStr(i));
    }

  return shader_template;
}

String ShaderGraphRecompiler::substituteDshl(NodeBasedShaderType shader, const DataBlock &shader_blk)
{
  return substitute(shader_blk, get_dshl_template_text_src(shader));
}

#if !NBSM_COMPILE_ONLY

void ShaderGraphRecompiler::activate(NodeBasedShaderType shader)
{
  switch (shader)
  {
    case NodeBasedShaderType::Fog: activeInstance = g_fog_instance.get(); break;
    case NodeBasedShaderType::Clouds: activeInstance = g_clouds_instance.get(); break;

    default: G_ASSERTF(false, "Implement activating this shader here"); return;
  }

  G_ASSERT(activeInstance);
}

static String join_strings(const eastl::vector<String> &strings)
{
  String result("[");
  for (const String &str : strings)
    result += "'" + str + "',";
  return result + "]";
}

static String get_default_user_script()
{
  String userScript;
  userScript +=
    "GE_setExternalNames('sample spheres density', " + join_strings(nodebasedshaderutils::getAvailableVolumeChannels()) + ");\n";
  userScript += "GE_setExternalNames('external texture', " + join_strings(nodebasedshaderutils::getAvailableTextures()) + ");\n";
  userScript += "GE_setExternalNames('external int', " + join_strings(nodebasedshaderutils::getAvailableInt()) + ");\n";
  userScript += "GE_setExternalNames('external float', " + join_strings(nodebasedshaderutils::getAvailableFloat()) + ");\n";
  userScript += "GE_setExternalNames('external float4', " + join_strings(nodebasedshaderutils::getAvailableFloat4()) + ");\n";
  return userScript;
}

void ShaderGraphRecompiler::initialize(NodeBasedShaderType shader, ShaderCompilerCallback compiler_callback,
  const char *root_graph_filename, const char *subgraphs_dir, String user_script, const char *permutations_blk_filename)
{
  if (subgraphs_dir == nullptr)
    subgraphs_dir = "../develop/assets/loc_shaders/__subgraphs";
  if (user_script.empty())
    user_script = get_default_user_script();
  if (permutations_blk_filename == nullptr)
    permutations_blk_filename = "../develop/assets/loc_shaders/permutations.blk";

  ShaderGraphRecompiler *instance = nullptr;

  switch (shader)
  {
    case NodeBasedShaderType::Fog:
      g_fog_instance.reset(create_fog_shader_recompiler());
      instance = g_fog_instance.get();
      break;

    case NodeBasedShaderType::EnviCover:
      g_envi_cover_instance.reset(create_envi_cover_shader_recompiler());
      instance = g_envi_cover_instance.get();
      break;

    case NodeBasedShaderType::Clouds:
      g_clouds_instance.reset(create_clouds_shader_recompiler());
      instance = g_clouds_instance.get();
      break;

    default: G_ASSERTF(false, "This shader recompiler is not yet implemented!"); break;
  }

  if (instance)
    instance->init(shader, subgraphs_dir, user_script, root_graph_filename, permutations_blk_filename, compiler_callback);
}

ShaderGraphRecompiler *ShaderGraphRecompiler::getInstance() { return activeInstance; }

void init_fog_shader_graph_plugin();
void init_envi_cover_graph_plugin();
void init_clouds_shader_graph_plugin();

void ShaderGraphRecompiler::onShaderGraphEditor(webui::RequestInfo *params)
{
  if (strcmp(params->plugin->name, FOG_SHADER_EDITOR_PLUGIN_NAME) == 0)
  {
    if (!g_fog_instance)
      init_fog_shader_graph_plugin();

    if (g_fog_instance && g_fog_instance->shader_editor)
      g_fog_instance->shader_editor->processRequest(params);
    else
      webui::html_response_raw(params->conn, "Error: ShaderGraphRecompiler::shader_editor == null<br>"
                                             "Wait for game loading to finish! If issue persists after that<br>"
                                             "maybe 'volfog_nbs' template/entity is missing from scene");
  }
  else if (strcmp(params->plugin->name, ENVI_COVER_SHADER_EDITOR_PLUGIN_NAME) == 0)
  {
    if (!g_envi_cover_instance)
      init_envi_cover_graph_plugin();

    if (g_envi_cover_instance && g_envi_cover_instance->shader_editor)
      g_envi_cover_instance->shader_editor->processRequest(params);
    else
      webui::html_response_raw(params->conn, "Error: ShaderGraphRecompiler::shader_editor == null<br>"
                                             "Wait for game loading to finish! If issue persists after that<br>"
                                             "maybe 'envi_cover_nbs' template/entity is missing from scene");
  }
  else if (strcmp(params->plugin->name, CLOUDS_SHADER_EDITOR_PLUGIN_NAME) == 0)
  {
    if (!g_clouds_instance)
      init_clouds_shader_graph_plugin();

    if (g_clouds_instance && g_clouds_instance->shader_editor)
      g_clouds_instance->shader_editor->processRequest(params);
    else
      webui::html_response_raw(params->conn, "Error: ShaderGraphRecompiler::shader_editor == null<br>"
                                             "Wait for game loading to finish! If issue persists after that<br>"
                                             "maybe 'clouds_nbs' template/entity is missing from scene");
  }
  else
  {
    G_ASSERT(false); // Implement handling this shader type
    webui::html_response_raw(params->conn, "Handling this shader type is not implemented!");
  }
}

ShaderGraphRecompiler::ShaderGraphRecompiler(const char *editor_name, ShaderGetSrcCallback shader_get_src_callback) :
  editorName(editor_name), shaderGetSrcCallback(shader_get_src_callback)
{}

ShaderGraphRecompiler::~ShaderGraphRecompiler() { del_it(shader_editor); }

static const char *getPermutationBlockName(NodeBasedShaderType shader)
{
  switch (shader)
  {
    case NodeBasedShaderType::Fog: return "volfog";
    case NodeBasedShaderType::EnviCover: return "envi_cover";
    default: return nullptr;
  }
}

bool ShaderGraphRecompiler::gatherPermutationGraphs(Tab<String> &out_graph_filenames, String &out_permutation_table_json)
{
  out_graph_filenames.clear();

  const char *typeBlockName = getPermutationBlockName(shaderType);

  if (!typeBlockName || permutationsBlkFileName.empty() || !dd_file_exists(permutationsBlkFileName))
    return false;

  DataBlock permutationsBlk;
  if (!permutationsBlk.load(permutationsBlkFileName))
    return false;

  const DataBlock *typeBlock = nullptr;
  for (int i = 0; i < permutationsBlk.blockCount() && !typeBlock; i++)
    typeBlock = permutationsBlk.getBlock(i)->getBlockByName(typeBlockName);
  if (!typeBlock)
    return false;

  const int groupNameId = typeBlock->getNameId("group");
  const int subgraphNameId = typeBlock->getNameId("subgraph");

  Tab<Tab<String>> permGroups;
  for (int i = 0; i < typeBlock->blockCount(); i++)
  {
    const DataBlock *groupBlk = typeBlock->getBlock(i);
    if (groupBlk->getBlockNameId() != groupNameId)
      continue;

    Tab<String> groupGraphs;
    for (int j = 0; j < groupBlk->paramCount(); j++)
      if (groupBlk->getParamNameId(j) == subgraphNameId && groupBlk->getParamType(j) == DataBlock::TYPE_STRING)
        groupGraphs.push_back(String(0, "../develop/%s", groupBlk->getStr(j)));

    if (groupGraphs.empty())
      continue;

    if (groupBlk->getBool("isExclusive", true))
      permGroups.push_back(groupGraphs);
    else
      for (const String &g : groupGraphs)
        permGroups.push_back().push_back(g);
  }

  out_permutation_table_json = "[";
  int emittedGroups = 0;
  for (int g = 0; g < permGroups.size(); g++)
  {
    if (emittedGroups >= MAX_GROUP_COUNT)
    {
      logerr("NBS editor: '%s' declares more than %d permutation groups; extra groups ignored", typeBlockName, MAX_GROUP_COUNT);
      break;
    }
    for (int p = 0; p < permGroups[g].size(); p++)
    {
      if (p >= MAX_PERMS_PER_GROUP)
      {
        logerr("NBS editor: permutation group %d of '%s' has more than %d graphs; extras ignored", g, typeBlockName,
          MAX_PERMS_PER_GROUP);
        break;
      }
      out_graph_filenames.push_back(permGroups[g][p]);
      out_permutation_table_json.aprintf(32, "%s{\"g\":%d,\"p\":%d}", out_graph_filenames.size() > 1 ? "," : "", emittedGroups, p);
    }
    emittedGroups++;
  }
  out_permutation_table_json += "]";

  return !out_graph_filenames.empty();
}

void ShaderGraphRecompiler::refreshShaders() // find all subgraphs on attach
{
  G_ASSERT(activeInstance);

  Tab<String> graphNames;
  String descriptions(tmpmem);
  const char *category = "Shaders";
  activeInstance->shader_editor->collectGraphs(activeInstance->subgraphsDir, "*.json", activeInstance->editorName, category,
    graphNames, descriptions, false);

  activeInstance->shader_editor->setGraphDescriptions(category, descriptions.str()); // call this before setGraphJson()
  activeInstance->shader_editor->setFilenames(graphNames); // << set file names which will be available to select in frontend on Ctrl+O
}

void ShaderGraphRecompiler::init(NodeBasedShaderType shader, const char *subgraphs_dir, const char *user_script,
  const char *root_graph_filename, const char *permutations_blk_filename, ShaderCompilerCallback compiler_callback)
{
  if (shader_editor == nullptr)
    shader_editor = new webui::GraphEditor(editorName);

  shaderType = shader;
  subgraphsDir = subgraphs_dir;
  rootGraphFileName = root_graph_filename;
  shaderCompilerCallback = compiler_callback;
  permutationsBlkFileName = permutations_blk_filename;

  char nameBuf[260];
  rootShaderName = dd_get_fname_without_path_and_ext(nameBuf, sizeof(nameBuf), root_graph_filename);

  String permTable;
  Tab<String> permGraphs;
  gatherPermutationGraphs(permGraphs, permTable);
  shader_editor->setIncludeFilenames(permGraphs);
  shader_editor->setPermutationTable(permTable.str());

  shader_editor->setRootGraphFileName(root_graph_filename); // will be created if does not exist
  shader_editor->setSaveDir(subgraphs_dir);
  shader_editor->onAttachCallback = [this]() {
    ShaderGraphRecompiler::activeInstance = this;
    ShaderGraphRecompiler::refreshShaders();
  };
  shader_editor->onNeedReloadGraphsCallback = refreshShaders;

  String addonFName;
  switch (shader)
  {
    case NodeBasedShaderType::Fog:
      addonFName = webui::GraphEditor::findFileInParentDir("prog/gameLibs/webui/plugins/shaderEditors/shaderNodes/shaderNodesFog.js");
      break;
    case NodeBasedShaderType::EnviCover:
      addonFName =
        webui::GraphEditor::findFileInParentDir("prog/gameLibs/webui/plugins/shaderEditors/shaderNodes/shaderNodesEnviCover.js");
      break;
    case NodeBasedShaderType::Clouds:
      addonFName =
        webui::GraphEditor::findFileInParentDir("prog/gameLibs/webui/plugins/shaderEditors/shaderNodes/shaderNodesClouds.js");
      break;
  }
  String inlinedShaderNodes = webui::GraphEditor::readFileToString(addonFName);
  String commonFName =
    webui::GraphEditor::findFileInParentDir("prog/gameLibs/webui/plugins/shaderEditors/shaderNodes/shaderNodesCommon.js");
  inlinedShaderNodes += webui::GraphEditor::readFileToString(commonFName);

  shader_editor->setNodesSettingsText(inlinedShaderNodes);

  shader_editor->setUserScript(user_script);

  G_ASSERTF(shaderCompilerCallback, "error: shaderCompilerCallback was not set for shader #%d", (int)shader);
}

void ShaderGraphRecompiler::recompile()
{
  clear_and_shrink(lastCompileError);

  if (!shaderCompilerCallback)
    return;

  if (shaderBlkText.empty())
    return;

  if (currentShaderName.empty())
    return;

  DataBlock shaderBlk;
  shaderBlk.loadText(shaderBlkText.str(), shaderBlkText.length());

#if _TARGET_PC_WIN
  uint32_t variantCnt = get_shader_variant_count(shaderType);
  for (uint32_t variantId = 0; variantId < variantCnt; ++variantId)
    for (uint32_t nbsQuality = 0; nbsQuality < static_cast<uint32_t>(NodeBasedShaderQuality::COUNT); nbsQuality++)
    {
      String errors;
      bool ok = shaderCompilerCallback(currentShaderName, shaderBlk, errors);
      if (!ok)
      {
        shader_editor->sendCommand(String(1024, "error: in shader #%d, variant #%d: %s", (int)shaderType, variantId, errors.str()));
        lastCompileError = errors;
        return;
      }
    }

  shader_editor->sendCommand("compiled");
#else
  shader_editor->sendCommand("error: shader can be compiled only on Windows");
#endif
}


void ShaderGraphRecompiler::update(float dt)
{
  G_UNUSED(dt);
  if (!shader_editor)
    return;

  String s(tmpmem);
  bool inessentional = false;
  bool graphUpdated = shader_editor->getCurrentGraphJson(s, true, &inessentional); // << get actual graph after each change
  if (graphUpdated)
  {
    // autosave
    if (!currentGraphFileName.empty())
    {
      String tmp(128, "%s~~~", currentGraphFileName.str());
      if (webui::GraphEditor::writeStringToFile(tmp.str(), s.str()))
      {
        if (dd_rename(tmp.str(), currentGraphFileName.str()) == 0)
          shader_editor->sendCommand(String(128, "error: cannot rename file '%s'", tmp.str()));
      }
      else
      {
        shader_editor->sendCommand(String(128, "error: cannot save graph to '%s'", tmp.str()));
      }
    }
  }

  String compileJson(tmpmem);
  if (shader_editor->getCompileGraphJson(compileJson, true))
  {
    shaderBlkText = webui::GraphEditor::getSubstring(compileJson.str(), "/*SHADER_BLK_START*/", "/*SHADER_BLK_END*/");
    shaderBlkText.replaceAll("[[shader_name]]", currentShaderName.str());

    if (inessentional == false)
      shouldRecompile = true;
  }


  if (shouldRecompile && abs(lastRecompileTimeMsec - get_time_msec()) > 300)
  {
    recompile();
    shouldRecompile = false;
    lastRecompileTimeMsec = get_time_msec();
  }


  Tab<String> cmd;
  if (shader_editor->fetchCommands(cmd)) // << get commands from frontend
  {
    for (int i = 0; i < cmd.size(); i++)
    {
      const char *cmdStr = cmd[i].str();

      if (!strcmp(cmdStr, "get_compiler_log"))
        shader_editor->sendCommand(String(0, "%%compiler_log:%s", lastCompileError.str()));

      if (!strcmp(cmdStr, "get_generated_code"))
      {
        DataBlock shaderBlk;
        shaderBlk.loadText(shaderBlkText.str(), shaderBlkText.length());
        uint32_t variantId = 0; // TODO: get it as param (and replace all this low-level hell)
        const NodeBasedShaderQuality lowQuality = NodeBasedShaderQuality::Low; // TODO: get it as param (and replace all this low-level
                                                                               // hell)
        String shaderTemplateText = (String)shaderGetSrcCallback(variantId, lowQuality);
        String code = substitute(shaderBlk, shaderTemplateText);
        shader_editor->sendCommand(String(0, "%%generated_code:%s", code.str()));
      }

      if (strstr(cmdStr, "open:") == cmdStr)
      {
        String filename(tmpmem);
        filename.setStr(cmdStr + sizeof("open:") - 1);
        debug("open graph: %s", filename.str());
        bool isRoot = false;

        String path(128, "%s/%s.json", subgraphsDir, filename.str());
        bool isPermutation = false;

        if (strstr(filename.str(), "ROOT: ") == filename.str())
        {
          path = rootGraphFileName;
          filename.replaceAll("ROOT: ", "");
          isRoot = true;
        }

        if (strstr(filename.str(), "INCL: ") == filename.str())
        {
          path = shader_editor->getFullIncludeFileName(filename);
          filename.replaceAll("INCL: ", "");
          isPermutation = true;
        }

        if (!dd_file_exists(path.str()))
        {
          String error(128, "error: cannot open graph '%s'", filename.str());
          shader_editor->sendCommand(error.str());
        }
        else
        {
          Tab<String> includeNames;
          String _unusedPermTable;
          gatherPermutationGraphs(includeNames, _unusedPermTable);
          shader_editor->gatherAdditionalIncludes(includeNames);

          editedPermutationIdx = -1;
          if (isPermutation)
            for (int idx = 0; idx < includeNames.size(); idx++)
              if (!strcmp(dd_get_fname(includeNames[idx]), dd_get_fname(path.str())))
              {
                editedPermutationIdx = idx;
                break;
              }

          shader_editor->setRootGraphJson(webui::GraphEditor::readFileToString(rootGraphFileName).str());
          currentShaderName = rootShaderName;

          shader_editor->setEditedPermutation(editedPermutationIdx);

          String graphStr = webui::GraphEditor::readFileToString(path.str());
          shader_editor->setGraphJson(graphStr.str());
          shader_editor->setCurrentFileName(path.str());
          currentGraphFileName = path;
        }
      }
    }
  }
}

void ShaderGraphRecompiler::cleanUp()
{
  g_fog_instance.reset();
  g_envi_cover_instance.reset();
  g_clouds_instance.reset();
}
#else
void ShaderGraphRecompiler::cleanUp() {}
#endif
