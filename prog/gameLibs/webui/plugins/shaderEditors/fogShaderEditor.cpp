// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/shaderEditors.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_direct.h>


String get_template_text_src_fog(uint32_t variant_id)
{
  Tab<String> templateNames;

  clear_and_shrink(templateNames);

  templateNames.push_back(String("fogDefines.hlsli"));
  templateNames.push_back(String("../../../daSDF/shaders/world_sdf.hlsli")); // Needs to be before globalHlsl
  templateNames.push_back(String("../../../publicInclude/render/grav_zones_gpu/gravity_zones_def.hlsli"));
  templateNames.push_back(String("globalHlslFunctions.hlsl"));
  templateNames.push_back(String("../../../publicInclude/render/light_consts.hlsli"));
  templateNames.push_back(String("../../../render/shaders/camera_in_camera.hlsl"));
  templateNames.push_back(String("../../../render/shaders/pcg_hash.hlsl"));
  templateNames.push_back(String("../../../publicInclude/render/volumetricLights/heightFogNode.hlsli"));
  templateNames.push_back(String("../../../render/shaders/phase_functions.hlsl"));
  templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_common_def.hlsli"));
  templateNames.push_back(String("../../../render/shaders/depth_above.hlsl"));
  templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_common.hlsl"));
  templateNames.push_back(String("../../../render/shaders/wind/sample_wind_common.hlsl"));
  templateNames.push_back(String("../../../fftWater/shaders/water_heigtmap.hlsl"));
  templateNames.push_back(String("nbsSDF.hlsl"));
  templateNames.push_back(String("../../../render/shaders/gravity_zones_funcs.hlsl"));
  templateNames.push_back(String("../../../daSDF/shaders/world_sdf_math.hlsl"));
  templateNames.push_back(String("../../../daSDF/shaders/world_sdf_use.hlsl"));


  switch (static_cast<NodeBasedShaderFogVariant>(variant_id))
  {
    case NodeBasedShaderFogVariant::Raymarch:
      templateNames.push_back(String("../../../render/shaders/static_shadow_int.hlsl"));
      templateNames.push_back(String("../../../render/shaders/static_shadow.hlsl"));
      templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_df_common.hlsl"));
      templateNames.push_back(String("raymarchFogShaderTemplateHeader.hlsl"));
      templateNames.push_back(String("fogCommon.hlsl"));
      templateNames.push_back(String("raymarchFogShaderTemplateFunctions.hlsl"));
      templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_df_raymarch.hlsl"));
      templateNames.push_back(String("raymarchFogShaderTemplateBody.hlsl"));
      break;
    case NodeBasedShaderFogVariant::Froxel:
      templateNames.push_back(String("froxelFogShaderTemplateHeader.hlsl"));
      templateNames.push_back(String("fogCommon.hlsl"));
      templateNames.push_back(String("froxelFogShaderTemplateBody.hlsl"));
      break;
    case NodeBasedShaderFogVariant::Shadow:
      templateNames.push_back(String("fogShadowShaderTemplateHeader.hlsl"));
      templateNames.push_back(String("fogCommon.hlsl"));
      templateNames.push_back(String("fogShadowShaderTemplateFunctions.hlsl"));
      templateNames.push_back(String("../../../render/volumetricLights/shaders/volfog_shadow.hlsl"));
      templateNames.push_back(String("fogShadowShaderTemplateBody.hlsl"));
      break;
    default: G_ASSERTF(false, "error: node based shader variant #%d is invalid", variant_id); break;
  }

  return collect_template_files(find_shader_editors_path(), templateNames);
}

class FogShaderEditor : public ShaderGraphRecompiler
{
public:
  FogShaderEditor() : ShaderGraphRecompiler(FOG_SHADER_EDITOR_PLUGIN_NAME, get_template_text_src_fog) {}
};

#if !NBSM_COMPILE_ONLY

ShaderGraphRecompiler *create_fog_shader_recompiler() { return new FogShaderEditor; }

static webui::HttpPlugin fog_shader_graph_editor_http_plugin = {
  FOG_SHADER_EDITOR_PLUGIN_NAME, "show fog shader graph editor", NULL, ShaderGraphRecompiler::onShaderGraphEditor};

webui::HttpPlugin get_fog_shader_graph_editor_http_plugin() { return fog_shader_graph_editor_http_plugin; }

#endif // !NBSM_COMPILE_ONLY
