// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/shaderEditors.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_direct.h>

String get_template_text_src_envi_cover(uint32_t variant)
{
  G_UNUSED(variant);

  Tab<String> templateNames;

  clear_and_shrink(templateNames);

  templateNames.push_back(String("../../../daSDF/shaders/world_sdf.hlsli")); // Needs to be before globalHlsl
  templateNames.push_back(String("enviCoverDefines.hlsli"));
  templateNames.push_back(String("../../../publicInclude/render/grav_zones_gpu/gravity_zones_def.hlsli"));
  templateNames.push_back(String("globalHlslFunctions.hlsl"));
  templateNames.push_back(String("../../../publicInclude/render/light_consts.hlsli"));
  templateNames.push_back(String("../../../render/shaders/camera_in_camera.hlsl"));
  templateNames.push_back(String("../../../render/shaders/noise/Value3D.hlsl"));
  templateNames.push_back(String("../../../render/shaders/pixelPacking/ColorSpaceUtility.hlsl"));
  templateNames.push_back(String("../../../render/shaders/hsv_rgb_conversion.hlsl"));
  templateNames.push_back(String("../../../render/shaders/gbuffer_header_DNG.hlsl"));
  templateNames.push_back(String("../../../render/shaders/gbuffer_rw_init_DNG.hlsl"));
  templateNames.push_back(String("../../../render/shaders/gbuffer_packing_inc_DNG.hlsl"));
  templateNames.push_back(String("../../../render/shaders/gbuffer_pack_unpack_DNG.hlsl"));
  templateNames.push_back(String("../../../render/shaders/gbuffer_partial_unpacking_DNG.hlsl"));
  templateNames.push_back(String("../../../render/shaders/sparkles.hlsl"));
  templateNames.push_back(String("../../../render/shaders/depth_above.hlsl"));
  templateNames.push_back(String("../../../fftWater/shaders/water_heigtmap.hlsl"));
  templateNames.push_back(String("enviCoverCommon.hlsl"));
  templateNames.push_back(String("nbsSDF.hlsl"));
  templateNames.push_back(String("../../../render/shaders/gravity_zones_funcs.hlsl"));
  templateNames.push_back(String("../../../daSDF/shaders/world_sdf_math.hlsl"));
  templateNames.push_back(String("../../../daSDF/shaders/world_sdf_use.hlsl"));

  switch (static_cast<NodeBasedShaderEnviCoverVariant>(variant))
  {
    case NodeBasedShaderEnviCoverVariant::ENVI_COVER: templateNames.push_back(String("enviCoverShaderTemplateBody.hlsl")); break;
    case NodeBasedShaderEnviCoverVariant::ENVI_COVER_WITH_MOTION_VECS:
      templateNames.push_back(String("../../../render/shaders/reprojected_motion_vectors.hlsl"));
      templateNames.push_back(String("enviCoverCombinedShaderTemplateBody.hlsl"));
      break;
    default: G_ASSERTF(false, "error: node based shader variant #%d is invalid", variant); break;
  }


  return collect_template_files(find_shader_editors_path(), templateNames);
}

class EnviCoverShaderEditor : public ShaderGraphRecompiler
{
public:
  EnviCoverShaderEditor() : ShaderGraphRecompiler(ENVI_COVER_SHADER_EDITOR_PLUGIN_NAME, get_template_text_src_envi_cover) {}
};

#if !NBSM_COMPILE_ONLY
ShaderGraphRecompiler *create_envi_cover_shader_recompiler() { return new EnviCoverShaderEditor; }

static webui::HttpPlugin envi_cover_shader_graph_editor_http_plugin = {
  ENVI_COVER_SHADER_EDITOR_PLUGIN_NAME, "show envi cover shader graph editor", NULL, ShaderGraphRecompiler::onShaderGraphEditor};

webui::HttpPlugin get_envi_cover_shader_graph_editor_http_plugin() { return envi_cover_shader_graph_editor_http_plugin; }
#endif