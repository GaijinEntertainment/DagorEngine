// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/shaderEditors.h>

ShaderGraphRecompiler::ShaderGraphRecompiler(const char *, ShaderGetSrcCallback) {}
ShaderGraphRecompiler::~ShaderGraphRecompiler() {}
void ShaderGraphRecompiler::initialize(NodeBasedShaderType, ShaderCompilerCallback, const char *, const char *, String, const char *)
{}
String ShaderGraphRecompiler::substitute(const DataBlock &, String) { return {}; }
String ShaderGraphRecompiler::substitute(NodeBasedShaderType, uint32_t, const DataBlock &) { return {}; }
String ShaderGraphRecompiler::substitutePs4(NodeBasedShaderType, uint32_t, const DataBlock &) { return {}; }
String ShaderGraphRecompiler::enumerateLines(const char *) { return {}; }
void ShaderGraphRecompiler::cleanUp() {}
void ShaderGraphRecompiler::recompile() {}
void ShaderGraphRecompiler::update(float) {}
void ShaderGraphRecompiler::onShaderGraphEditor(webui::RequestInfo *) {}
ShaderGraphRecompiler *ShaderGraphRecompiler::getInstance() { return nullptr; }
void ShaderGraphRecompiler::activate(NodeBasedShaderType) {}
void ShaderGraphRecompiler::getIncludeFileNames(Tab<String> &) {}
webui::HttpPlugin get_fog_shader_graph_editor_http_plugin() { return {}; }
webui::HttpPlugin get_gpu_object_shader_graph_editor_http_plugin() { return {}; }
void set_up_gpu_object_plugins(int, dag::Span<webui::HttpPlugin>) {}
void clean_up_gpu_object_plugins(int, dag::Span<webui::HttpPlugin>) {}
