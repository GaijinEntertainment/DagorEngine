//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <webui/helpers.h>
#include <webui/graphEditorPlugin.h>
#include <webui/nodeBasedShaderType.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <EASTL/functional.h>

#define HAS_SHADER_GRAPH_COMPILE_SUPPORT (DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN)

typedef bool (*ShaderCompilerCallback)(uint32_t variant_id, const String &name, const String &code, const DataBlock &shader_blk,
  String &out_errors);

typedef String (*ShaderGetSrcCallback)(uint32_t variant_id);

class ShaderGraphRecompiler
{
public:
  void recompile();
  void update(float /* dt */);

  void getIncludeFileNames(Tab<String> &includeGraphNames);

  static void activate(NodeBasedShaderType shader);

  static void initialize(NodeBasedShaderType shader, ShaderCompilerCallback compiler_callback, const char *root_graph_filename,
    const char *subgraphs_dir = nullptr, String user_script = String(""), const char *optional_graphs_dir = nullptr);

  static ShaderGraphRecompiler *getInstance();

  static String substitute(NodeBasedShaderType shader, uint32_t variant_id, const DataBlock &shader_blk);
  static String substitutePs4(NodeBasedShaderType shader, uint32_t variant_id, const DataBlock &shader_blk);
  static String substitutePs5(NodeBasedShaderType shader, uint32_t variant_id, const DataBlock &shader_blk)
  {
    return substitutePs4(shader, variant_id, shader_blk);
  }
  static String substitute(const DataBlock &shader_blk, String shader_template);
  static String enumerateLines(const char *s);

  static void onShaderGraphEditor(webui::RequestInfo *params);

  static void cleanUp();

  ~ShaderGraphRecompiler();

protected:
  ShaderGraphRecompiler(const char *editor_name, ShaderGetSrcCallback shader_get_src_callback);

  void init(NodeBasedShaderType shader, const char *subgraphs_dir, const char *user_script, const char *root_graph_filename,
    const char *optional_graphs_dir, ShaderCompilerCallback compiler_callback);

  static ShaderGraphRecompiler *activeInstance;

  static ShaderGraphRecompiler *fogInstance;

private:
  webui::GraphEditor *shader_editor = nullptr;

  ShaderCompilerCallback shaderCompilerCallback = nullptr;

  NodeBasedShaderType shaderType = static_cast<NodeBasedShaderType>(0);
  String currentShaderName;
  String shaderBlkText;
  String currentGraphFileName;
  String lastCompileError;
  String subgraphsDir; // for graph in .json
  String translateDir; // for translated to .blk
  String rootGraphFileName;
  String optionalGraphsDir;

  String editorName;

  bool shouldRecompile = false;

  ShaderGetSrcCallback shaderGetSrcCallback = nullptr;

  static void refreshShaders();
};

String find_shader_editors_path();
String collect_template_files(const String &template_dir, const Tab<String> &template_names);


extern webui::HttpPlugin get_fog_shader_graph_editor_http_plugin();

#define FOG_SHADER_EDITOR_PLUGIN_NAME "fog_shader_editor"
