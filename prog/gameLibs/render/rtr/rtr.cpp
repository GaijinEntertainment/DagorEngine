// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtr.h>
#include <render/denoiser.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_bindless.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <bvh/bvh.h>
#include <perfMon/dag_statDrv.h>
#include <image/dag_texPixel.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/algorithm.h>
#include <EASTL/optional.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

namespace rtr
{

static ComputeShaderElement *trace = nullptr;
static ComputeShaderElement *decode = nullptr;
static ComputeShaderElement *classify = nullptr;
static ComputeShaderElement *sample = nullptr;

static UniqueTex validation_texture;

static UniqueTex reflection_value;
static UniqueTex denoised_reflection;
static UniqueTex decoded_denoised_reflection;
static UniqueTex tiles;

static bool is_half_res = false;

static int inv_proj_tmVarId = -1;
static int rtr_targetVarId = -1;
static int rtr_denoisedVarId = -1;
static int rtr_tilesVarId = -1;
static int rtr_sample_countVarId = -1;
static int rtr_frame_indexVarId = -1;
static int rtr_hit_dist_paramsVarId = -1;
static int rtr_rough_ray_lengthVarId = -1;
static int rtr_water_hit_dist_paramsVarId = -1;
static int rtr_resolutionIVarId = -1;
static int rtr_bindless_slotVarId = -1;
static int rtr_output_typeVarId = -1;
static int rtr_classify_tresholdsVarId = -1;
static int rtr_validation_textureVarId = -1;
static int rtr_shadowVarId = -1;
static int rtr_use_csmVarId = -1;
static int rtr_fom_shadowVarId = -1;
static int rtr_res_mulVarId = -1;
static int rtr_checkerboardVarId = -1;
static int rt_nrVarId = -1;
static int rt_cockpit_relfection_powerVarId = -1;
static int downsampled_close_depth_texVarId = -1;
static int denoiser_glass_history_confidence_tweakVarId = -1;
static int denoiser_view_zVarId = -1;

static float water_ray_length = 5000;
static float gloss_ray_length = 1000;
static float rough_ray_length = 1000;
static float distance_factor = 0;
static float scatter_factor = 1;
static float roughness_factor = 0;

static float al_luminance_sigma_scale = 2.0f;
static float al_hit_distance_sigma_scale = 2.0f;
static float al_luminance_antilag_power = 1.0f;
static float al_hit_distance_antilag_power = 1.0f;

static float classify_treshold1 = 0.4;
static float classify_treshold2 = 0.15;
static float classify_treshold3 = 0.01;

static float glass_tweak_full = 0.3;
static float glass_tweak_half = 0.6;

static denoiser::ReflectionMethod output_type = denoiser::ReflectionMethod::Relax;

static bool performance_mode = true;
static bool checkerboard = true;
static bool show_validation = false;
static bool use_anti_firefly = false;
static bool use_nrd_lib = false;

static d3d::SamplerHandle linear_sampler = d3d::INVALID_SAMPLER_HANDLE;

static PostFxRenderer *validation_renderer;

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

void initialize(bool half_res, bool checkerboard_, float cockpit_reflection_power)
{
  if (!bvh::is_available())
    return;

  rtr::checkerboard = checkerboard_;
  rtr_frame_indexVarId = get_shader_variable_id("rtr_frame_index");
  rtr_hit_dist_paramsVarId = get_shader_variable_id("rtr_hit_dist_params");
  rtr_rough_ray_lengthVarId = get_shader_variable_id("rtr_rough_ray_length");
  rtr_water_hit_dist_paramsVarId = get_shader_variable_id("rtr_water_hit_dist_params", true);
  rtr_resolutionIVarId = get_shader_variable_id("rtr_resolutionI");
  rtr_targetVarId = get_shader_variable_id("rtr_target");
  rtr_denoisedVarId = get_shader_variable_id("rtr_denoised");
  rtr_tilesVarId = get_shader_variable_id("rtr_tiles");
  rtr_sample_countVarId = get_shader_variable_id("rtr_sample_count");
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  rtr_bindless_slotVarId = get_shader_variable_id("rtr_bindless_slot");
  rtr_output_typeVarId = get_shader_variable_id("rtr_output_type");
  rtr_classify_tresholdsVarId = get_shader_variable_id("rtr_classify_tresholds");
  rtr_validation_textureVarId = get_shader_variable_id("rtr_validation_texture");
  rtr_res_mulVarId = get_shader_variable_id("rtr_res_mul");
  rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true);
  rtr_use_csmVarId = get_shader_variable_id("rtr_use_csm", true);
  rtr_fom_shadowVarId = get_shader_variable_id("rtr_fom_shadow", true);
  rtr_checkerboardVarId = get_shader_variable_id("rtr_checkerboard", true);
  rt_nrVarId = get_shader_variable_id("rt_nr");
  rt_cockpit_relfection_powerVarId = get_shader_variable_id("rt_cockpit_relfection_power", true);
  downsampled_close_depth_texVarId = get_shader_variable_id("downsampled_close_depth_tex");
  denoiser_glass_history_confidence_tweakVarId = get_shader_variable_id("denoiser_glass_history_confidence_tweak", true);
  denoiser_view_zVarId = get_shader_variable_id("denoiser_view_z", true);
  ShaderGlobal::set_int(rtr_res_mulVarId, half_res ? 2 : 1);
  ShaderGlobal::set_int(rtr_checkerboardVarId, checkerboard_ ? 1 : 0);
  ShaderGlobal::set_real(rt_cockpit_relfection_powerVarId, cockpit_reflection_power);

  if (!trace)
    trace = new_compute_shader("rt_reflection");

  if (!sample)
    sample = new_compute_shader("rt_reflection_sample");

  if (!classify)
    classify = new_compute_shader("rt_reflection_classify");

  validation_texture.close();
  reflection_value.close();
  denoised_reflection.close();
  decoded_denoised_reflection.close();
  tiles.close();

  denoiser::make_reflection_maps(reflection_value, denoised_reflection, output_type, half_res);

  TextureInfo ti;
  reflection_value->getinfo(ti);

  int checkerboardWidth = divide_up(ti.w, 2);
  tiles = dag::create_tex(nullptr, divide_up(checkerboard_ ? checkerboardWidth : ti.w, 8), divide_up(ti.h, 8),
    TEXCF_UNORDERED | TEXFMT_R8UI, 1, "rtr_tiles");

  validation_texture = dag::create_tex(nullptr, ti.w, ti.h, TEXCF_UNORDERED, 1, "rtr_validation_texture");

  is_half_res = half_res;

  d3d::SamplerInfo si;
  si.mip_map_mode = d3d::MipMapMode::Point;
  si.filter_mode = d3d::FilterMode::Linear;
  si.address_mode_u = d3d::AddressMode::Clamp;
  si.address_mode_v = d3d::AddressMode::Clamp;
  linear_sampler = d3d::request_sampler(si);
}

template <typename T>
static void safe_delete(T *&ptr)
{
  if (ptr)
  {
    delete ptr;
    ptr = nullptr;
  }
}

void set_performance_mode(bool performance_mode_) { rtr::performance_mode = performance_mode_; }

void teardown()
{
  safe_delete(trace);
  safe_delete(sample);
  safe_delete(decode);
  safe_delete(classify);
  safe_delete(validation_renderer);

  validation_texture.close();
  reflection_value.close();
  denoised_reflection.close();
  decoded_denoised_reflection.close();
  tiles.close();

  ShaderGlobal::set_int(rtr_bindless_slotVarId, -1);
}

void set_water_params()
{
  if (rtr_water_hit_dist_paramsVarId < 0)
    rtr_water_hit_dist_paramsVarId = get_shader_variable_id("rtr_water_hit_dist_params", true);
  Point4 waterHitDistParams = Point4(water_ray_length, distance_factor, scatter_factor, roughness_factor);
  ShaderGlobal::set_color4(rtr_water_hit_dist_paramsVarId, waterHitDistParams);
}

void set_rtr_hit_distance_params()
{
  if (rtr_hit_dist_paramsVarId < 0)
    rtr_hit_dist_paramsVarId = get_shader_variable_id("rtr_hit_dist_params", true);
  Point4 hitDistParams = Point4(gloss_ray_length, distance_factor, scatter_factor, roughness_factor);
  ShaderGlobal::set_color4(rtr_hit_dist_paramsVarId, hitDistParams);
}

void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, bool rt_shadow, bool csm_shadow, bool high_speed_mode,
  TEXTUREID half_depth)
{
  TIME_D3D_PROFILE(rtr::render);

  TextureInfo ti;
  denoised_reflection->getinfo(ti);

  TextureInfo tiTiles;
  tiles->getinfo(tiTiles);

  bvh::bind_resources(context_id, ti.w);

  Point4 hitDistParams = Point4(gloss_ray_length, distance_factor, scatter_factor, roughness_factor);
  Point4 waterHitDistParams = Point4(water_ray_length, distance_factor, scatter_factor, roughness_factor);

  ShaderGlobal::set_texture(denoiser_view_zVarId, denoiser::get_viewz_texId(ti.w));
  ShaderGlobal::set_texture(rtr_targetVarId, reflection_value.getTexId());
  ShaderGlobal::set_texture(rtr_denoisedVarId, denoised_reflection.getTexId());
  ShaderGlobal::set_texture(rtr_tilesVarId, tiles.getTexId());
  ShaderGlobal::set_int(rtr_frame_indexVarId, denoiser::get_frame_number());
  ShaderGlobal::set_real(rtr_rough_ray_lengthVarId, rough_ray_length);
  ShaderGlobal::set_color4(rtr_hit_dist_paramsVarId, hitDistParams);
  ShaderGlobal::set_color4(rtr_water_hit_dist_paramsVarId, waterHitDistParams);
  ShaderGlobal::set_color4(rtr_classify_tresholdsVarId, classify_treshold1, classify_treshold2, classify_treshold3);
  ShaderGlobal::set_int4(rtr_resolutionIVarId, ti.w, ti.h, tiTiles.w, tiTiles.h);
  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(proj_tm));
  ShaderGlobal::set_int(rtr_output_typeVarId, (int)output_type);
  ShaderGlobal::set_texture(rt_nrVarId, denoiser::get_nr_texId(ti.w));
  ShaderGlobal::set_texture(downsampled_close_depth_texVarId, half_depth);
  ShaderGlobal::set_int(rtr_shadowVarId, rt_shadow ? 1 : 0);
  ShaderGlobal::set_int(rtr_use_csmVarId, csm_shadow ? 1 : 0);
  ShaderGlobal::set_real(denoiser_glass_history_confidence_tweakVarId, is_half_res ? glass_tweak_half : glass_tweak_full);

  {
    // const float clear[] = { 0, 1, 0, 1 };
    // d3d::clear_rwtexf(reflection_value.getTex2D(), clear, 0, 0);
    // d3d::clear_rwtexf(denoised_reflection.getTex2D(), clear, 0, 0);

    TIME_D3D_PROFILE(rtr::noisy_image);
    {
      TIME_D3D_PROFILE(rtr::classify);
      const uint32_t clear[] = {0, 0, 0, 0};
      d3d::clear_rwtexi(reflection_value.getTex2D(), clear, 0, 0);
      d3d::set_sampler(STAGE_CS, 5, linear_sampler);
      classify->dispatch(tiTiles.w, tiTiles.h, 1);
    }
    {
      TIME_D3D_PROFILE(rtr::sampling_pass);
      sample->dispatch(tiTiles.w, tiTiles.h, 1);
    }
    {
      TIME_D3D_PROFILE(rtr::tracing_64_samples);
      ShaderGlobal::set_int(rtr_sample_countVarId, 64);
      trace->dispatch(tiTiles.w, tiTiles.h, 1);
      d3d::resource_barrier(ResourceBarrierDesc(reflection_value.getTex2D(), RB_NONE, 0, 0));
    }
    {
      TIME_D3D_PROFILE(rtr::tracing_16_samples);
      ShaderGlobal::set_int(rtr_sample_countVarId, 16);
      trace->dispatch(divide_up(tiTiles.w, 2), divide_up(tiTiles.h, 2), 1);
      d3d::resource_barrier(ResourceBarrierDesc(reflection_value.getTex2D(), RB_NONE, 0, 0));
    }
    {
      TIME_D3D_PROFILE(rtr::tracing_4_samples);
      ShaderGlobal::set_int(rtr_sample_countVarId, 4);
      trace->dispatch(divide_up(tiTiles.w, 4), divide_up(tiTiles.h, 4), 1);
    }
  }

  denoiser::ReflectionDenoiser params;
  params.hitDistParams = hitDistParams;
  params.antilagSettings =
    Point4(al_luminance_sigma_scale, al_hit_distance_sigma_scale, al_luminance_antilag_power, al_hit_distance_antilag_power);
  params.denoisedReflection = denoised_reflection.getTex2D();
  params.reflectionValue = reflection_value.getTex2D();
  params.performanceMode = performance_mode;
  params.validationTexture = show_validation ? validation_texture.getTex2D() : nullptr;
  params.antiFirefly = use_anti_firefly;
  params.checkerboard = rtr::checkerboard;
  params.useNRDLib = use_nrd_lib;
  params.highSpeedMode = high_speed_mode;
  denoiser::denoise_reflection(params);

  d3d::resource_barrier(ResourceBarrierDesc(denoised_reflection.getTex2D(), RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
}

void render_validation_layer()
{
  if (!show_validation)
    return;

  if (!validation_renderer)
    validation_renderer = new PostFxRenderer("rtr_validation_renderer");

  ShaderGlobal::set_texture(rtr_validation_textureVarId, validation_texture.getTexId());
  validation_renderer->render();
}

#if DAGOR_DBGLEVEL > 0

static int imgui_draw_functionVarId = -1;
static bool show_tiles = false;

inline const char *operator!(denoiser::ReflectionMethod mode)
{
  switch (mode)
  {
    case denoiser::ReflectionMethod::Reblur: return "Reblur";
    case denoiser::ReflectionMethod::Relax: return "Relax";
    default: return "Unknown";
  }
}

static void imguiWindow()
{
  static float glass_cockpit_reflection_power = 0.4;
  ImGui::SliderFloat("Water ray length", &water_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Gloss ray length", &gloss_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Rough ray length", &rough_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Distance factor", &distance_factor, 0, 1);
  ImGui::SliderFloat("Scatter factor", &scatter_factor, 0, 2);
  ImGui::SliderFloat("Roughness factor", &roughness_factor, 0, 10);

  ImGui::SliderFloat("luminanceSigmaScale", &al_luminance_sigma_scale, 1, 3);
  ImGui::SliderFloat("hitDistanceSigmaScale", &al_hit_distance_sigma_scale, 1, 3);
  ImGui::SliderFloat("luminanceAntilagPower", &al_luminance_antilag_power, 0, 1);
  ImGui::SliderFloat("hitDistanceAntilagPower", &al_hit_distance_antilag_power, 0, 1);

  ImGui::SliderFloat("Glass tweak", is_half_res ? &glass_tweak_half : &glass_tweak_full, 0, 1);
  ImGui::SliderFloat("Cockpit reflection power", &glass_cockpit_reflection_power, 0, 1);
  ShaderGlobal::set_real(denoiser_glass_history_confidence_tweakVarId, is_half_res ? glass_tweak_half : glass_tweak_full);

  ImGui::SliderFloat("Classify 64 treshold", &classify_treshold1, 0, 0.5);
  ImGui::SliderFloat("Classify 16 treshold", &classify_treshold2, 0, 0.2);
  ImGui::SliderFloat("Classify 4 treshold", &classify_treshold3, 0, 0.1);

  auto oldType = output_type;

  using RDM = denoiser::ReflectionMethod;
  ImGuiDagor::EnumCombo("Denoiser", RDM::Reblur, RDM::Relax, output_type, &operator!);

  if (output_type == RDM::Reblur)
    ImGui::Checkbox("Performance mode", &performance_mode);

  ImGui::Checkbox("Show validation layer", &show_validation);
  ImGui::Checkbox("Use NRD library", &use_nrd_lib);
  ImGui::Checkbox("Use anti firefly", &use_anti_firefly);

  if (oldType != output_type)
  {
    teardown();
    initialize(is_half_res, checkerboard, glass_cockpit_reflection_power);
  }

  ImGui::Separator();
  ImGui::Checkbox("Show tiles", &show_tiles);

  if (denoised_reflection)
  {
    TextureInfo ti;
    denoised_reflection->getinfo(ti);

    if (show_tiles)
    {
      TextureInfo tiTiles;
      tiles->getinfo(tiTiles);

      imgui_draw_functionVarId = get_shader_variable_id("imgui_draw_function");

      auto set_image_mode = [](const ImDrawList *, const ImDrawCmd *) { ShaderGlobal::set_int(imgui_draw_functionVarId, 1); };

      auto reset_image_mode = [](const ImDrawList *, const ImDrawCmd *) { ShaderGlobal::set_int(imgui_draw_functionVarId, 0); };

      ImGui::GetWindowDrawList()->AddCallback(set_image_mode, nullptr);
      ImGuiDagor::Image(tiles.getTexId(), float(tiTiles.w * 2) / tiTiles.h, ImVec2(0, 0), ImVec2(1, 1));
      ImGui::GetWindowDrawList()->AddCallback(reset_image_mode, nullptr);

      ImGui::Text("One tile is 8x8 pixels");
      ImGui::Text("Red - 64 rays");
      ImGui::Text("Blue - 16 rays");
      ImGui::Text("Green - 4 rays");
      ImGui::Text("White - 0 rays, use specular cube");
      ImGui::Text("Black - 0 rays, ignore (sky, underwater)");
    }
    else
    {
      if (!decoded_denoised_reflection)
        decoded_denoised_reflection =
          dag::create_tex(nullptr, ti.w / 2, ti.h / 2, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "decoded_denoised_reflection");

      if (!decode)
        decode = new_compute_shader("rt_reflection_decode");

      static int rtr_reflection_decode_srcVarId = get_shader_variable_id("rtr_reflection_decode_src");
      static int rtr_reflection_decode_dstVarId = get_shader_variable_id("rtr_reflection_decode_dst");

      ShaderGlobal::set_texture(rtr_reflection_decode_srcVarId, denoised_reflection.getTexId());
      ShaderGlobal::set_texture(rtr_reflection_decode_dstVarId, decoded_denoised_reflection.getTexId());
      decode->dispatchThreads(ti.w / 2, ti.h / 2, 1);

      ImGuiDagor::Image(decoded_denoised_reflection.getTexId(), decoded_denoised_reflection.getTex2D());
    }
  }
}

REGISTER_IMGUI_WINDOW("Render", "RTR", imguiWindow);
#endif

void turn_off() { ShaderGlobal::set_int(rtr_bindless_slotVarId, -1); }

void set_classify_threshold(float threshold_64, float threshold_16, float threshold_4)
{
  classify_treshold1 = threshold_64;
  classify_treshold2 = threshold_16;
  classify_treshold3 = threshold_4;
}

void set_reflection_method(denoiser::ReflectionMethod method)
{
  G_ASSERTF(!trace, "set_reflection_method should be called before initalization!");
  output_type = method;
}

} // namespace rtr