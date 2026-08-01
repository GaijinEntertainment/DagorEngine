// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/shaderEditors.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_direct.h>

// TODO: Clean this up, remenants of hlsl compilation, but it is required for F4 generated graph code
// but as clouds does not have non dshl version its completely useless
String get_template_text_src_clouds(uint32_t variant, NodeBasedShaderQuality nbs_quality)
{
  G_UNUSED(variant);

  Tab<String> templateNames;
  clear_and_shrink(templateNames);

  templateNames.push_back(String("../../../publicInclude/render/nbs_spheres.hlsli"));
  templateNames.push_back(String("globalHlslFunctions.hlsl"));
  templateNames.push_back(String("../../../daSkies2/shaders/clouds2/cloud_settings.hlsli"));
  templateNames.push_back(String("../../../daSkies2/shaders/clouds2/clouds_density_height_lut.hlsli"));
  templateNames.push_back(String("cloudsNbsCommon.hlsl"));
  templateNames.push_back(String("../../../render/shaders/noise/Value3D.hlsl"));

  NodeBasedShaderCloudsVariant convertedVariant = static_cast<NodeBasedShaderCloudsVariant>(variant);
  if (!(convertedVariant == NodeBasedShaderCloudsVariant::Field || convertedVariant == NodeBasedShaderCloudsVariant::FieldCompressed))
    G_ASSERTF(false, "error: node based shader variant #%d is invalid", variant);

  return add_nbs_quality_definition(nbs_quality) + collect_template_files(find_shader_editors_path(), templateNames);
}

String get_dshl_template_text_src_clouds()
{
  return collect_template_files(find_shader_editors_path(), {String("cloudsShaderTemplate.dshl")});
}

class CloudsShaderEditor : public ShaderGraphRecompiler
{
public:
  CloudsShaderEditor() : ShaderGraphRecompiler(CLOUDS_SHADER_EDITOR_PLUGIN_NAME, get_template_text_src_clouds) {}
};

#if !NBSM_COMPILE_ONLY
ShaderGraphRecompiler *create_clouds_shader_recompiler() { return new CloudsShaderEditor; }

static webui::HttpPlugin clouds_shader_graph_editor_http_plugin = {
  CLOUDS_SHADER_EDITOR_PLUGIN_NAME, "show clouds shader graph editor", NULL, ShaderGraphRecompiler::onShaderGraphEditor};

webui::HttpPlugin get_clouds_shader_graph_editor_http_plugin() { return clouds_shader_graph_editor_http_plugin; }
#endif
