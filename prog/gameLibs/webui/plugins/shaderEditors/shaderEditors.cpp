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


ShaderGraphRecompiler *create_fog_shader_recompiler();

ShaderGraphRecompiler *ShaderGraphRecompiler::activeInstance = nullptr;
ShaderGraphRecompiler *ShaderGraphRecompiler::fogInstance = nullptr;

String get_template_text_src_fog(uint32_t variant_id);

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

static String get_template_text_src(NodeBasedShaderType shader, uint32_t variant_id)
{
  switch (shader)
  {
    case NodeBasedShaderType::Fog: return get_template_text_src_fog(variant_id);

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

String ShaderGraphRecompiler::substitute(NodeBasedShaderType shader, uint32_t variant_id, const DataBlock &shader_blk)
{
  return substitute(shader_blk, get_template_text_src(shader, variant_id));
}

String ShaderGraphRecompiler::substitutePs4(NodeBasedShaderType shader, uint32_t variant_id, const DataBlock &shader_blk)
{
  return substitute(shader_blk, String(get_template_ps4_header()) + get_template_text_src(shader, variant_id));
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

#if !NBSM_COMPILE_ONLY

void ShaderGraphRecompiler::activate(NodeBasedShaderType shader)
{
  switch (shader)
  {
    case NodeBasedShaderType::Fog: activeInstance = fogInstance; break;

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
  userScript += "GE_setExternalNames('sample spheres density', " + join_strings(nodebasedshaderutils::getAvailableBuffers()) + ");\n";
  userScript += "GE_setExternalNames('external texture', " + join_strings(nodebasedshaderutils::getAvailableTextures()) + ");\n";
  userScript += "GE_setExternalNames('external int', " + join_strings(nodebasedshaderutils::getAvailableInt()) + ");\n";
  userScript += "GE_setExternalNames('external float', " + join_strings(nodebasedshaderutils::getAvailableFloat()) + ");\n";
  userScript += "GE_setExternalNames('external float4', " + join_strings(nodebasedshaderutils::getAvailableFloat4()) + ");\n";
  return userScript;
}

void ShaderGraphRecompiler::initialize(NodeBasedShaderType shader, ShaderCompilerCallback compiler_callback,
  const char *root_graph_filename, const char *subgraphs_dir, String user_script, const char *optional_graphs_dir)
{
  if (subgraphs_dir == nullptr)
    subgraphs_dir = "../develop/assets/loc_shaders/__subgraphs";
  if (user_script.empty())
    user_script = get_default_user_script();
  if (optional_graphs_dir == nullptr)
    optional_graphs_dir = "../develop/assets/loc_shaders";

  ShaderGraphRecompiler *instance = nullptr;

  switch (shader)
  {
    case NodeBasedShaderType::Fog:
      del_it(fogInstance);
      instance = fogInstance = create_fog_shader_recompiler();
      break;

    default: G_ASSERTF(false, "This shader recompiler is not yet implemented!"); break;
  }

  if (instance)
    instance->init(shader, subgraphs_dir, user_script, root_graph_filename, optional_graphs_dir, compiler_callback);
}

ShaderGraphRecompiler *ShaderGraphRecompiler::getInstance() { return activeInstance; }

void init_fog_shader_graph_plugin();

void ShaderGraphRecompiler::onShaderGraphEditor(webui::RequestInfo *params)
{
  if (strcmp(params->plugin->name, FOG_SHADER_EDITOR_PLUGIN_NAME) == 0)
  {
    if (!ShaderGraphRecompiler::fogInstance)
      init_fog_shader_graph_plugin();

    if (ShaderGraphRecompiler::fogInstance && ShaderGraphRecompiler::fogInstance->shader_editor)
      ShaderGraphRecompiler::fogInstance->shader_editor->processRequest(params);
    else
      webui::html_response_raw(params->conn, "Error: ShaderGraphRecompiler::shader_editor == null<br>"
                                             "Maybe 'rootFogGraph:t' was not found in block 'volFog' in level blk");
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

void ShaderGraphRecompiler::getIncludeFileNames(Tab<String> &includeGraphNames)
{
  String descriptions(tmpmem);
  shader_editor->collectGraphs(optionalGraphsDir, "*.json", editorName, "Shaders", includeGraphNames, descriptions, false, false);
  for (String &incGraph : includeGraphNames)
    incGraph = optionalGraphsDir + "/" + incGraph + ".json";
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
  const char *root_graph_filename, const char *optional_graphs_dir, ShaderCompilerCallback compiler_callback)
{
  if (shader_editor == nullptr)
    shader_editor = new webui::GraphEditor(editorName);

  shaderType = shader;
  subgraphsDir = subgraphs_dir;
  rootGraphFileName = root_graph_filename;
  shaderCompilerCallback = compiler_callback;
  optionalGraphsDir = optional_graphs_dir;

  Tab<String> includeNames;
  getIncludeFileNames(includeNames);
  shader_editor->setIncludeFilenames(includeNames);

  shader_editor->setRootGraphFileName(root_graph_filename); // will be created if does not exist
  shader_editor->setSaveDir(subgraphs_dir);
  shader_editor->onAttachCallback = [this]() {
    ShaderGraphRecompiler::activeInstance = this;
    ShaderGraphRecompiler::refreshShaders();
  };
  shader_editor->onNeedReloadGraphsCallback = refreshShaders;

  String fname = webui::GraphEditor::findFileInParentDir("tools/dagor_cdk/commonData/graphEditor/builder/shaderNodes.js");
  String inlinedShaderNodes = webui::GraphEditor::readFileToString(fname);

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
  {
    String shaderTemplateText = (String)shaderGetSrcCallback(variantId);
    String code = substitute(shaderBlk, shaderTemplateText);
    String errors;
    bool ok = shaderCompilerCallback(variantCnt, currentShaderName, code, shaderBlk, errors);
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

    {
      shaderBlkText = webui::GraphEditor::getSubstring(s.str(), "/*SHADER_BLK_START*/", "/*SHADER_BLK_END*/");
      String shaderCode = webui::GraphEditor::getSubstring(s.str(), "/*HLSL_GRAPH_START*/", "/*HLSL_GRAPH_END*/");
      shaderBlkText.replaceAll("[[shader_name]]", currentShaderName.str());

      if (inessentional == false)
        shouldRecompile = true;
    }

    // TODO: update graph
    // createCompilerProcess();
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
        String shaderTemplateText = (String)shaderGetSrcCallback(variantId);
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
        }

        if (!dd_file_exists(path.str()))
        {
          String error(128, "error: cannot open graph '%s'", filename.str());
          shader_editor->sendCommand(error.str());
        }
        else
        {
          Tab<String> includeNames;
          if (isRoot)
            getIncludeFileNames(includeNames);
          shader_editor->gatherAdditionalIncludes(includeNames);

          currentShaderName = filename;
          String graphStr = webui::GraphEditor::readFileToString(path.str());
          shader_editor->setGraphJson(graphStr.str());
          shader_editor->setCurrentFileName(path.str());
          currentGraphFileName = path;
        }
      }
    }
  }
}

void ShaderGraphRecompiler::cleanUp() { del_it(fogInstance); }
#else
void ShaderGraphRecompiler::cleanUp() {}
#endif
