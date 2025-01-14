// Copyright (C) Gaijin Games KFT.  All rights reserved.

// #include <frustumCulling/frustumPlanes.h>
#include <util/dag_convar.h>
#include <render/viewVecs.h>
#include <perfMon/dag_statDrv.h>
#include <render/spheres_consts.hlsli>
#include "screenSpaceProbes.h"
#include "shaders/screenProbes/screenprobes_consts.hlsli"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
CONSOLE_INT_VAL("gi", sp_spatial_filter_passes, 3, 1, 5);
CONSOLE_INT_VAL("gi", sp_placement_probes_iterations, -1, -1, 8);
CONSOLE_BOOL_VAL("gi", sp_radiance_r11g11b10, true);
CONSOLE_FLOAT_VAL_MINMAX("gi", sp_additional_probes_reserve, 0.75, 0.25, 4);
CONSOLE_BOOL_VAL("gi", sp_use_selected_probes, true);
CONSOLE_BOOL_VAL("gi", sp_irradiance_use_sph3, true);

enum
{
  RADIANCE_4_IRRADIANCE_3 = 0,
  RADIANCE_56_IRRADIANCE_34 = 1,
  RADIANCE_7_IRRADIANCE_5 = 2,
  SAME_AS_RADIANCE_8 = 3,
  RADIANCE_DOWNSAMPLE = 4
};

#define GLOBAL_VARS_LIST                           \
  VAR(screenspace_probe_screen_res)                \
  VAR(prev_screenspace_probe_screen_res)           \
  VAR(sp_radiance_oct_res)                         \
  VAR(sp_irradiance_oct_quality)                   \
  VAR(sp_probes_per_warp_irradiance)               \
  VAR(sp_irradiance_res)                           \
  VAR(sp_irradiance_resf)                          \
  VAR(dagi_screenprobes_user_trace_dist)           \
  VAR(dagi_screenprobes_trace_quality)             \
  VAR(dagi_screenprobes_filter_quality)            \
  VAR(sp_radiance_area)                            \
  VAR(prev_screenspace_probe_res)                  \
  VAR(screenspace_irradiance)                      \
  VAR(screenspace_irradiance_samplerstate)         \
  VAR(prev_screenspace_probe_atlas_size)           \
  VAR(screenspace_probe_res)                       \
  VAR(sp_placement_iteration)                      \
  VAR(screenprobes_current_radiance)               \
  VAR(screenprobes_current_radiance_samplerstate)  \
  VAR(screenprobes_current_radiance_distance)      \
  VAR(prev_screenspace_radiance)                   \
  VAR(prev_screenspace_radiance_samplerstate)      \
  VAR(screenspace_probes_temporal)                 \
  VAR(screenspace_probes_draw_sequence_count)      \
  VAR(screenspace_probes_count__added__total)      \
  VAR(prev_screenspace_probes_count__added__total) \
  VAR(screenspace_probe_pos)                       \
  VAR(prev_screenspace_probe_pos)                  \
  VAR(sp_probe_pos_allocated_ofs)                  \
  VAR(debug_screenspace_probes_type)               \
  VAR(sp_projtm)                                   \
  VAR(sp_globtm)                                   \
  VAR(sp_globtm_from_campos)                       \
  VAR(sp_world_view_pos)                           \
  VAR(sp_view_z)                                   \
  VAR(sp_view_vecLT)                               \
  VAR(sp_view_vecRT)                               \
  VAR(sp_view_vecLB)                               \
  VAR(sp_view_vecRB)                               \
  VAR(sp_zn_zfar)                                  \
  VAR(sp_prev_projtm)                              \
  VAR(sp_prev_globtm)                              \
  VAR(sp_prev_globtm_from_current_campos)          \
  VAR(sp_prev_globtm_from_campos)                  \
  VAR(sp_prev_camera_pos_movement)                 \
  VAR(sp_prev_world_view_pos)                      \
  VAR(sp_prev_view_z)                              \
  VAR(sp_prev_view_vecLT)                          \
  VAR(sp_prev_view_vecRT)                          \
  VAR(sp_prev_view_vecLB)                          \
  VAR(sp_prev_view_vecRB)                          \
  VAR(sp_prev_zn_zfar)                             \
  VAR(dagi_sp_oct_to_irradiance_atlas)             \
  VAR(dagi_sp_oct_to_radiance_atlas)               \
  VAR(dagi_sp_oct_to_radiance_atlas_clamp)         \
  VAR(screenspace_probe_atlas_size)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

ScreenSpaceProbes::~ScreenSpaceProbes()
{
  ShaderGlobal::set_int4(screenspace_probe_resVarId, 0, 0, 16384, 0);
  ShaderGlobal::set_color4(screenspace_probe_atlas_sizeVarId, 0, 0, 0, 0);
  ShaderGlobal::set_int4(screenspace_probes_count__added__totalVarId, 0, 0, 0, 0);
  ShaderGlobal::set_int4(prev_screenspace_probes_count__added__totalVarId, 0, 0, 0, 0);
  ShaderGlobal::set_color4(dagi_sp_oct_to_irradiance_atlasVarId, 0, 0, 0, 0);
  ShaderGlobal::set_color4(dagi_sp_oct_to_radiance_atlasVarId, 0, 0, 0, 0);
  ShaderGlobal::set_color4(dagi_sp_oct_to_radiance_atlas_clampVarId, 0, 0, 0, 0);
}

static uint32_t div_round_up(uint32_t n, uint32_t d) { return uint32_t(n + d - 1) / d; }

ScreenSpaceProbes::ScreenRes ScreenSpaceProbes::calc(int tile_size, int sw, int sh, float additional)
{
  ScreenRes sr;
  tile_size = clamp<int>(tile_size, 8, 32); // we can make minimum of 4, but
  sr.sw = sw;
  sr.sh = sh;
  sr.w = div_round_up(sw, tile_size);
  sr.h = div_round_up(sh, tile_size);
  sr.atlasW = sr.w;
  sr.atlasH = sr.h * (1 + additional);
  sr.screenProbes = sr.w * sr.h;
  sr.tileSize = tile_size;
  sr.totalProbes = sr.atlasW * sr.atlasH;
  sr.additionalProbes = sr.totalProbes - sr.screenProbes;
  return sr;
}

void ScreenSpaceProbes::setCurrentScreenRes(const ScreenRes &sr)
{
  const int cFrame = frame & 1;
  const bool changed = current != sr;
  const int irradianceResWithBorder = irradianceRes + 2;
  const float irrBorderMul = 0.5 * float(irradianceRes) / irradianceResWithBorder;
  const float irrBorderAdd = irrBorderMul + 1. / irradianceResWithBorder;

  current = sr;
  if (changed)
  {
    current_radiance.resize(sr.atlasW * radianceRes, sr.atlasH * radianceRes);
    if (current_radiance_distance)
    {
      current_radiance_distance.resize(sr.atlasW * radianceRes, sr.atlasH * radianceRes);
      current_radiance_distance.getTex2D()->texaddr(TEXADDR_CLAMP);
    }
    ShaderGlobal::set_texture(screenprobes_current_radiance_distanceVarId, current_radiance_distance.getTexId());

    screenspace_irradiance.resize(sr.atlasW * irradianceResWithBorder, sr.atlasH * irradianceResWithBorder);
    ShaderGlobal::set_texture(screenspace_irradianceVarId, screenspace_irradiance.getTexId());
  }

  ShaderGlobal::set_int4(screenspace_probe_resVarId, sr.w, sr.h, sr.tileSize, radianceRes);
  ShaderGlobal::set_color4(screenspace_probe_atlas_sizeVarId, sr.atlasW, sr.atlasH, 1. / sr.atlasW, 1. / sr.atlasH);
  ShaderGlobal::set_int4(screenspace_probes_count__added__totalVarId, sr.screenProbes, sr.additionalProbes, sr.totalProbes,
    (sr.screenProbes + 31) >> 5);

  ShaderGlobal::set_color4(dagi_sp_oct_to_irradiance_atlasVarId, irrBorderMul / sr.atlasW, irrBorderMul / sr.atlasH,
    irrBorderAdd / sr.atlasW, irrBorderAdd / sr.atlasH);


  ShaderGlobal::set_color4(dagi_sp_oct_to_radiance_atlasVarId, 0.5f / sr.atlasW, 0.5f / sr.atlasH, 0, 0);
  ShaderGlobal::set_color4(dagi_sp_oct_to_radiance_atlas_clampVarId, .5f / radianceRes / sr.atlasW, .5f / radianceRes / sr.atlasH,
    (1.f - .5f / radianceRes) / sr.atlasW, (1.f - .5f / radianceRes) / sr.atlasH);
  ShaderGlobal::set_color4(screenspace_probe_screen_resVarId, sr.sw, sr.sh, 1. / sr.sw, 1. / sr.sh);

  ShaderGlobal::set_buffer(screenspace_probe_posVarId, screenspaceProbePos[cFrame].getBufId());
}

void ScreenSpaceProbes::ensureRadianceRes(int cf, const ScreenRes &sr)
{
  screenspaceRadiance[cf].resize(sr.atlasW * radianceRes, sr.atlasH * radianceRes);
}

void ScreenSpaceProbes::setPrevScreenRes(const ScreenRes &sr)
{
  const int pFrame = 1 - (frame & 1);
  ShaderGlobal::set_buffer(prev_screenspace_probe_posVarId, screenspaceProbePos[pFrame].getBufId());
  ShaderGlobal::set_int4(prev_screenspace_probes_count__added__totalVarId, sr.screenProbes, sr.additionalProbes, sr.totalProbes,
    (sr.screenProbes + 31) >> 5);
  ShaderGlobal::set_color4(prev_screenspace_probe_screen_resVarId, sr.sw, sr.sh, 1. / sr.sw, 1. / sr.sh);
  ShaderGlobal::set_int4(prev_screenspace_probe_resVarId, sr.w, sr.h, sr.tileSize, radianceRes);
  ShaderGlobal::set_color4(prev_screenspace_probe_atlas_sizeVarId, sr.atlasW, sr.atlasH, 1. / sr.atlasW, 1. / sr.atlasH);
}

void ScreenSpaceProbes::init(int tile_size, int sw, int sh, int max_sw, int max_sh, int radiance_res, bool need_distance,
  float temporality)
{
  initMaxResolution(tile_size, max_sw, max_sh, radiance_res, need_distance, temporality);
  next = calc(tile_size, sw, sh, additionalPlanesAllocation);
}

void ScreenSpaceProbes::initProbesList(uint32_t total, bool history)
{
  screenspace_probes_list.close();
  screenspace_probes_list = dag::create_sbuffer(sizeof(uint32_t), total * (history ? 2 : 1) + 2,
    SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "screenspace_probes_list");
}

void ScreenSpaceProbes::initMaxResolution(int tile_size, int max_sw, int max_sh, int radiance_res, bool need_distance,
  float temporality)
{
  // probably we can make minimum 4, but requires permutation in additional_probes_placement_cs
  // we can also make tile size 64 and more, but requires more bits in coord (is in todo)
  const float additional = sp_additional_probes_reserve * (tile_size / 16.);
  radiance_res = clamp<int>(radiance_res, 4, 16);
  ScreenRes a = calc(tile_size, max_sw, max_sh, additional);
  temporality = clamp(temporality, 0.01f, 1.f);
  if (a == allocated && bool(temporality < 1.0f) != (currentTemporality < 1.0f))
    initProbesList(allocated.totalProbes, bool(temporality < 1.0f));

  currentTemporality = temporality;
  // atlas
  int irradiance_res = clamp<int>(int(radiance_res * (6.f / 8.f) + 0.2499f), 3, 6);
  if (a == allocated && radiance_res == radianceRes && irradianceRes == irradiance_res &&
      need_distance == bool(current_radiance_distance))
    return;
  additionalPlanesAllocation = additional;
  allocated = a;
  debug("DaGI2: screen probes inited with tileSize = %d, resolution %dx%d (%dx%d), additional %f,"
        " radiance_res %d, irradiance_res = %d, need_dist = %d, temporality = %f",
    tile_size, a.w, a.h, a.atlasW, a.atlasH, additional, radiance_res, irradiance_res, need_distance, currentTemporality);
  validHistory = false;
  radianceRes = radiance_res;
  irradianceRes = irradiance_res;
  if (!initial_probes_placement_cs)
  {
#define CS(a) a.reset(new_compute_shader(#a))
    CS(calc_screenspace_radiance_cs);
    CS(calc_screenspace_selected_radiance_cs);
    CS(filter_screenspace_radiance_cs);
    CS(temporal_filter_screenspace_radiance_cs);
    CS(temporal_only_filter_screenspace_radiance_cs);
    CS(calc_screenspace_irradiance_cs);
    CS(calc_screenspace_irradiance_sph3_cs);
    CS(initial_probes_placement_cs);
    CS(clear_additional_screenspace_probes_count_cs);
    CS(additional_probes_placement_cs);
    CS(rearrange_additional_probes_placement_cs);
    CS(screenspace_probes_create_dispatch_indirect_cs);
#undef CS
  }
  supportRGBE_UAV = d3d::check_texformat(TEXFMT_R9G9B9E5 | TEXCF_UNORDERED);
  useRGBEFormat = supportRGBE_UAV;
  debug("screen probes supportRGBE_UAV: %d", supportRGBE_UAV);
  screenprobes_indirect_buffer.close();
  screenprobes_indirect_buffer = dag::create_sbuffer(sizeof(uint32_t), SP_INDIRECT_BUFFER_SIZE,
    SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_UA_INDIRECT, 0, "screenprobes_indirect_buffer");
  const uint32_t currentRadianceFmt = supportRGBE_UAV ? TEXFMT_R9G9B9E5 : TEXFMT_R11G11B10F;

  current_radiance.close();
  current_radiance = dag::create_tex(NULL, allocated.atlasW * radianceRes, allocated.atlasH * radianceRes,
    TEXCF_UNORDERED | currentRadianceFmt, 1, // TEXFMT_A16B16G16R16F
    "screenprobes_trace_radiance");
  current_radiance->disableSampler();
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    screenprobes_current_radiance_samplerstateVarId.set_sampler(sampler);
    prev_screenspace_radiance_samplerstateVarId.set_sampler(sampler);
    screenspace_irradiance_samplerstateVarId.set_sampler(sampler);
  }

  current_radiance_distance.close();
  if (need_distance)
  {
    const uint32_t hitDistFmt = TEXFMT_L8; // TEXFMT_R16F;
    current_radiance_distance = dag::create_tex(NULL, allocated.atlasW * radianceRes, allocated.atlasH * radianceRes,
      TEXCF_UNORDERED | hitDistFmt, 1, "screenprobes_trace_radiance_distance");
    current_radiance_distance.getTex2D()->texaddr(TEXADDR_CLAMP);
    ShaderGlobal::set_texture(screenprobes_current_radiance_distanceVarId, current_radiance_distance.getTexId());
  }

  screenspaceRadiance[0].close();
  screenspaceRadiance[1].close();

  const uint32_t radianceFMT = supportRGBE_UAV ? TEXFMT_R9G9B9E5 : (sp_radiance_r11g11b10 ? TEXFMT_R11G11B10F : TEXFMT_A16B16G16R16F);
  screenspaceRadiance[0] = dag::create_tex(NULL, allocated.atlasW * radianceRes, allocated.atlasH * radianceRes,
    TEXCF_UNORDERED | radianceFMT, 1, "screenspace_radiance0");
  screenspaceRadiance[1] = dag::create_tex(NULL, allocated.atlasW * radianceRes, allocated.atlasH * radianceRes,
    TEXCF_UNORDERED | radianceFMT, 1, "screenspace_radiance1");
  screenspaceRadiance[0]->disableSampler();
  screenspaceRadiance[1]->disableSampler();

  const uint32_t irradianceFmt = supportRGBE_UAV ? TEXFMT_R9G9B9E5 : TEXFMT_R11G11B10F;
  const int irradianceResWithBorder = irradianceRes + 2;
  screenspace_irradiance.close();
  screenspace_irradiance = dag::create_tex(NULL, allocated.atlasW * irradianceResWithBorder,
    allocated.atlasH * irradianceResWithBorder, TEXCF_UNORDERED | irradianceFmt, 1, "screenspace_irradiance_a");
  screenspace_irradiance->disableSampler();
  ShaderGlobal::set_texture(screenspace_irradianceVarId, screenspace_irradiance.getTexId());

  screenspaceProbePos[0].close();
  screenspaceProbePos[1].close();
  initProbesList(allocated.totalProbes, bool(temporality < 1.0f));

  // combine with additional_screenspace_probes_count
  ShaderGlobal::set_int4(sp_probe_pos_allocated_ofsVarId, allocated.totalProbes * 2, allocated.totalProbes * 2 + 1,
    allocated.totalProbes * 8, allocated.totalProbes * 8 + 4);

  const uint32_t totalProbePosCount = allocated.totalProbes * 2 + allocated.w * allocated.h + 1;
  screenspaceProbePos[0] = dag::create_sbuffer(sizeof(uint32_t), totalProbePosCount,
    SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "screenspace_probe_pos0");
  screenspaceProbePos[1] = dag::create_sbuffer(sizeof(uint32_t), totalProbePosCount,
    SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "screenspace_probe_pos1");

  tileClassificator.close();
  tileClassificator =
    dag::create_sbuffer(sizeof(uint32_t), (allocated.w * allocated.h + 31) / 32 + ((allocated.additionalProbes + 3) & ~3),
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "screenspace_tile_classificator");

  linked_list_additional_screenspace_probes.close();
  linked_list_additional_screenspace_probes =
    dag::create_sbuffer(sizeof(uint32_t), allocated.screenProbes + 1 + allocated.additionalProbes * 3,
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "linked_list_additional_screenspace_probes");
  const int halfRes = (radianceRes + 1) / 2, halfResArea = halfRes * halfRes;
  ShaderGlobal::set_int4(sp_radiance_areaVarId, radianceRes * radianceRes, halfResArea, halfRes,
    bitwise_cast<uint32_t>(1.f / halfResArea));


  ShaderGlobal::set_int4(sp_irradiance_resVarId, irradianceRes, irradianceResWithBorder, irradianceRes * irradianceRes,
    irradianceResWithBorder * irradianceResWithBorder);
  ShaderGlobal::set_real(sp_irradiance_resfVarId, irradianceRes);
  ShaderGlobal::set_int(sp_radiance_oct_resVarId, radianceRes);
  uint32_t probesPerWarp = 0;
  if (radianceRes > 8)
  {
    G_ASSERT(irradianceResWithBorder == 8);
    probesPerWarp = 1;
    ShaderGlobal::set_int(sp_irradiance_oct_qualityVarId, RADIANCE_DOWNSAMPLE);
  }
  else if (radianceRes == 8)
  {
    G_ASSERT(irradianceResWithBorder == 8);
    probesPerWarp = 1;
    ShaderGlobal::set_int(sp_irradiance_oct_qualityVarId, SAME_AS_RADIANCE_8);
  }
  else if (radianceRes == 7)
  {
    G_ASSERT(irradianceRes == 5);
    ShaderGlobal::set_int(sp_irradiance_oct_qualityVarId, RADIANCE_7_IRRADIANCE_5);
  }
  else if (radianceRes == 4)
  {
    G_ASSERT(irradianceRes == 3);
    ShaderGlobal::set_int(sp_irradiance_oct_qualityVarId, RADIANCE_4_IRRADIANCE_3);
  }
  else
  {
    G_ASSERT((radianceRes == 6 && irradianceRes == 4) || (radianceRes == 5 && irradianceRes == 3));
    ShaderGlobal::set_int(sp_irradiance_oct_qualityVarId, RADIANCE_56_IRRADIANCE_34);
  }
  if (probesPerWarp == 0)
  {
    auto updateTsz = calc_screenspace_irradiance_cs->getThreadGroupSizes();
    const uint32_t warpSize = updateTsz[0] * updateTsz[1];
    probesPerWarp = warpSize / (irradianceRes * irradianceRes);
  }
  ShaderGlobal::set_int(sp_probes_per_warp_irradianceVarId, probesPerWarp);
  setCurrentScreenRes(allocated);
  setPrevScreenRes(prev = allocated);
  afterReset();
}

void ScreenSpaceProbes::initial_probes_placement()
{
  DA_PROFILE_GPU;
  initial_probes_placement_cs->dispatchThreads(current.w, current.h, 1);
  // d3d::resource_barrier({additional_screenspace_probes_count[frame&1].getBuf(), RB_FLUSH_UAV});
}
void ScreenSpaceProbes::additional_probes_placement()
{
  // after initial
  d3d::resource_barrier({screenspaceProbePos[frame & 1].getBuf(), RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE | RB_FLUSH_UAV});
  d3d::resource_barrier({tileClassificator.getBuf(), RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE | RB_FLUSH_UAV});
  DA_PROFILE_GPU;
  const int iterations =
    min<int>(sp_placement_probes_iterations < 0 ? max<int>(1, get_bigger_log2(current.tileSize) - 1) : sp_placement_probes_iterations,
      SP_MAX_ADDITIONAL_PROBES_COUNT);
  for (int i = 0; i < iterations; ++i)
  {
    ShaderGlobal::set_int4(sp_placement_iterationVarId, i, i != iterations - 1 ? 1 : 0, iterations, 0);
    TIME_D3D_PROFILE(iteration);
    additional_probes_placement_cs->dispatch(current.w, current.h, 1);
    d3d::resource_barrier({screenspaceProbePos[frame & 1].getBuf(), RB_NONE}); // since we don't write to overlapped
    d3d::resource_barrier(
      {linked_list_additional_screenspace_probes.getBuf(), RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE | RB_FLUSH_UAV});
    d3d::resource_barrier({screenspace_probes_list.getBuf(), RB_NONE});
  }
  d3d::resource_barrier({screenspaceProbePos[frame & 1].getBuf(), RB_NONE}); // since we don't write to it
}

void ScreenSpaceProbes::rearrange_probes_placement()
{
  DA_PROFILE_GPU;
  d3d::resource_barrier(
    {linked_list_additional_screenspace_probes.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE}); // flush after
                                                                                                                      // clear
  rearrange_additional_probes_placement_cs->dispatchThreads(current.w * current.h, 1, 1);
}

void ScreenSpaceProbes::probes_placement()
{
  DA_PROFILE_GPU;
  d3d::set_rwbuffer(STAGE_CS, 0, screenspaceProbePos[frame & 1].getBuf());
  d3d::set_rwbuffer(STAGE_CS, 1, tileClassificator.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 2, linked_list_additional_screenspace_probes.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 3, screenspace_probes_list.getBuf());
  initial_probes_placement();
  additional_probes_placement();
  rearrange_probes_placement();
  d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 2, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 3, nullptr);
  d3d::resource_barrier({screenspace_probes_list.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
}
void ScreenSpaceProbes::clear_probes()
{
  DA_PROFILE_GPU;
  d3d::set_rwbuffer(STAGE_CS, 0, tileClassificator.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 1, linked_list_additional_screenspace_probes.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 2, screenspaceProbePos[frame & 1].getBuf());
  d3d::set_rwbuffer(STAGE_CS, 3, screenspace_probes_list.getBuf());
  clear_additional_screenspace_probes_count_cs->dispatchThreads(
    (max<int>((current.w * current.h + 31) / 32, current.additionalProbes) + 3) / 4, 1, 1);
  d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 2, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 3, nullptr);
  d3d::resource_barrier(
    {linked_list_additional_screenspace_probes.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({tileClassificator.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({screenspace_probes_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
}

void ScreenSpaceProbes::create_indirect()
{
  d3d::set_rwbuffer(STAGE_CS, 0, screenprobes_indirect_buffer.getBuf());
  screenspace_probes_create_dispatch_indirect_cs->dispatch(1, 1, 1);
  d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  d3d::resource_barrier({screenprobes_indirect_buffer.getBuf(), RB_RO_INDIRECT_BUFFER});
}

void ScreenSpaceProbes::calc_probe_pos()
{
  DA_PROFILE_GPU;
  clear_probes();
  probes_placement();
  d3d::resource_barrier(
    {screenspaceProbePos[frame & 1].getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | (RB_STAGE_COMPUTE | RB_STAGE_PIXEL)});
  create_indirect();
}

void ScreenSpaceProbes::filter_probe_radiance(const ComputeShaderElement &e, BaseTexture *dest)
{
  DA_PROFILE_GPU;
  d3d::set_rwtex(STAGE_CS, 0, dest, 0, 0);
  e.dispatch_indirect(screenprobes_indirect_buffer.getBuf(), SP_FILTER_INDIRECT_OFFSET * 4);
  d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  d3d::resource_barrier({dest, RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
}

void ScreenSpaceProbes::trace_probe_radiance(float quality, bool angle_filtering)
{
  // exposure will be applied in additional step.
  // it is a bit faster even, than directly in trace, though probably a bit less accurate precision on trace
  // but we mostly need exposure to fight temporal issues with insufficient precision, so it doesn't matter
  auto &dest = validHistory ? current_radiance : screenspaceRadiance[1 - radianceFrame];
  DA_PROFILE_GPU;
  ShaderGlobal::set_int(dagi_screenprobes_trace_qualityVarId, quality <= 0 ? 0 : angle_filtering ? 2 : 1);
  ShaderGlobal::set_real(dagi_screenprobes_user_trace_distVarId, max(0.f, quality));
  d3d::set_rwtex(STAGE_CS, 0, dest.getTex2D(), 0, 0);
  d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);
  d3d::set_rwtex(STAGE_CS, 2, current_radiance_distance.getTex2D(), 0, 0);
  d3d::set_cs_constbuffer_size(256);
  const bool useSelected = sp_use_selected_probes && validHistory && currentTemporality < 1;
  if (useSelected)
    calc_screenspace_selected_radiance_cs->dispatch_indirect(screenprobes_indirect_buffer.getBuf(), SP_TRACE_SELECTED_OFS * 4);
  else
    calc_screenspace_radiance_cs->dispatch_indirect(screenprobes_indirect_buffer.getBuf(), SP_TRACE_INDIRECT_OFFSET * 4);
  d3d::set_cs_constbuffer_size(0);
  d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);
  d3d::set_rwtex(STAGE_CS, 2, nullptr, 0, 0);

  d3d::resource_barrier({dest.getTex2D(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  ShaderGlobal::set_texture(screenprobes_current_radianceVarId, dest.getTexId());
  if (current_radiance_distance)
    d3d::resource_barrier({current_radiance_distance.getTex2D(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
}

void ScreenSpaceProbes::calc_probe_radiance(float quality, bool angle_filtering)
{
  DA_PROFILE_GPU;
  angle_filtering &= bool(current_radiance_distance) && quality > 0 && currentTemporality >= 1;
  ShaderGlobal::set_int(dagi_screenprobes_filter_qualityVarId, angle_filtering ? 1 : 0);
  const bool useSelected = sp_use_selected_probes && validHistory && currentTemporality < 1;
  const bool changed = current != prev;
  if (changed)
    ensureRadianceRes(1 - radianceFrame, current);

  ShaderGlobal::set_texture(prev_screenspace_radianceVarId, screenspaceRadiance[radianceFrame].getTexId());

  auto &temporalDest = screenspaceRadiance[1 - radianceFrame];
  if (useSelected && validHistory)
  {
    TIME_D3D_PROFILE(sp_only_history);
    d3d::set_rwtex(STAGE_CS, 0, temporalDest.getTex2D(), 0, 0);
    d3d::set_rwbuffer(STAGE_CS, 3, screenspace_probes_list.getBuf());
    temporal_only_filter_screenspace_radiance_cs->dispatch_indirect(screenprobes_indirect_buffer.getBuf(),
      SP_TEMPORAL_HISTORY_OFS * 4);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::set_rwbuffer(STAGE_CS, 3, nullptr);
    d3d::resource_barrier({screenspace_probes_list.getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::resource_barrier({temporalDest.getTex2D(), RB_NONE | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    // todo: rewrite only SP_TEMPORAL_SELECTED_OFS!
    create_indirect();
  }

  trace_probe_radiance(quality, angle_filtering);

  if (validHistory)
  {
    TIME_D3D_PROFILE(sp_temporal_filter);
    ShaderGlobal::set_texture(screenprobes_current_radianceVarId, current_radiance.getTexId());

    d3d::set_rwtex(STAGE_CS, 0, temporalDest.getTex2D(), 0, 0);
    temporal_filter_screenspace_radiance_cs->dispatch_indirect(screenprobes_indirect_buffer.getBuf(), SP_TEMPORAL_SELECTED_OFS * 4);

    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::resource_barrier({temporalDest.getTex2D(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  }
  if (changed)
    ensureRadianceRes(radianceFrame, current);
  radianceFrame = 1 - radianceFrame;

  {
    TIME_D3D_PROFILE(spatial);
    for (int i = 0, ie = sp_spatial_filter_passes - 1; i < ie; ++i)
    {
      ShaderGlobal::set_int4(sp_placement_iterationVarId, i, ie, 1 << (ie - 1 - i), i == ie - 1);
      ShaderGlobal::set_texture(screenprobes_current_radianceVarId, screenspaceRadiance[radianceFrame].getTexId());
      radianceFrame = 1 - radianceFrame;
      filter_probe_radiance(*filter_screenspace_radiance_cs, screenspaceRadiance[radianceFrame].getTex2D());
    }
  }
  ShaderGlobal::set_texture(screenprobes_current_radianceVarId, screenspaceRadiance[radianceFrame].getTexId());
}

void ScreenSpaceProbes::calc_probe_irradiance()
{
  DA_PROFILE_GPU;
  d3d::set_rwtex(STAGE_CS, 0, screenspace_irradiance.getTex2D(), 0, 0);
  // on radianceRes < 8 amount of operations needed to calculate spherical harmonics is bigger,
  //  than needed to calculate direct irradiance
  // with interlocked averaging we issue in each thread:
  //  averaging: 28 interlocked operations: (27 adds and 1 max)
  //  averaging: 14 clamps + 14 madds + 14 floors for
  //  1 sh3_dot3 (6 dots + 6 madds) + 1 sh3_basis for irradiance
  //  1 add_sh3 + 2 mul_sh3, 2 sh3_basis for radiance (and 3 on radianceRes 5 and 6)
  //  1 decode spherical distance
  // compared to 16 dots, 16 groupshared read, 32 groupshared write
  if (radianceRes >= 8 && irradianceRes == 6 && sp_irradiance_use_sph3)
    calc_screenspace_irradiance_sph3_cs->dispatch_indirect(screenprobes_indirect_buffer.getBuf(), SP_IRRADIANCE_INDIRECT_OFFSET * 4);
  else
    calc_screenspace_irradiance_cs->dispatch_indirect(screenprobes_indirect_buffer.getBuf(), SP_IRRADIANCE_INDIRECT_OFFSET * 4);
  d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  d3d::resource_barrier({screenspace_irradiance.getTex2D(),
    RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | (RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX), 0, 0});
}

void ScreenSpaceProbes::afterReset()
{
  const int cFrame = frame & 1;
  ShaderGlobal::set_buffer(screenspace_probe_posVarId, screenspaceProbePos[cFrame].getBufId());
  uint32_t v[4] = {0, 0, 0, 0};
  d3d::clear_rwbufi(screenspaceProbePos[cFrame].getBuf(), v);
  d3d::clear_rwbufi(screenspaceProbePos[1 - cFrame].getBuf(), v);
  float vf[4] = {1, 1, 1, 1};
  d3d::clear_rwtexf(screenspace_irradiance.getTex2D(), vf, 0, 0);
  d3d::clear_rwtexf(screenspaceRadiance[0].getTex2D(), vf, 0, 0);
  d3d::clear_rwtexf(screenspaceRadiance[1].getTex2D(), vf, 0, 0);
  frame = radianceFrame = 0;
  validHistory = false;
}

static void set_frame_info(const DaGIFrameInfo &fi)
{
  const TMatrix4 globTm = fi.globTm.transpose();
  ShaderGlobal::set_float4x4(sp_globtmVarId, globTm);
  const TMatrix4 projTm = fi.projTm.transpose();
  ShaderGlobal::set_float4x4(sp_projtmVarId, projTm);
  const TMatrix4 globTmNoOfs = fi.globTmNoOfs.transpose();
  ShaderGlobal::set_float4x4(sp_globtm_from_camposVarId, globTmNoOfs);
  ShaderGlobal::set_color4(sp_world_view_posVarId, P3D(fi.world_view_pos), 0);
  ShaderGlobal::set_color4(sp_view_zVarId, P3D(fi.viewZ), 0);
  ShaderGlobal::set_color4(sp_view_vecLTVarId, Color4(&fi.viewVecLT.x));
  ShaderGlobal::set_color4(sp_view_vecRTVarId, Color4(&fi.viewVecRT.x));
  ShaderGlobal::set_color4(sp_view_vecLBVarId, Color4(&fi.viewVecLB.x));
  ShaderGlobal::set_color4(sp_view_vecRBVarId, Color4(&fi.viewVecRB.x));
  ShaderGlobal::set_color4(sp_zn_zfarVarId, fi.znear, fi.zfar);
}

static void set_prev_frame_info(const DaGIFrameInfo &curFi, const DaGIFrameInfo &fi)
{
  const TMatrix4 globTm = fi.globTm.transpose();
  ShaderGlobal::set_float4x4(sp_prev_globtmVarId, globTm);
  const TMatrix4 globTmNoOfs = fi.globTmNoOfs.transpose();
  ShaderGlobal::set_float4x4(sp_prev_globtm_from_camposVarId, globTmNoOfs);

  const DPoint3 move = curFi.world_view_pos - fi.world_view_pos;

  double reprojected_world_pos_d[4] = {(double)globTmNoOfs[0][0] * move.x + (double)globTmNoOfs[0][1] * move.y +
                                         (double)globTmNoOfs[0][2] * move.z + (double)globTmNoOfs[0][3],
    globTmNoOfs[1][0] * move.x + (double)globTmNoOfs[1][1] * move.y + (double)globTmNoOfs[1][2] * move.z + (double)globTmNoOfs[1][3],
    globTmNoOfs[2][0] * move.x + (double)globTmNoOfs[2][1] * move.y + (double)globTmNoOfs[2][2] * move.z + (double)globTmNoOfs[2][3],
    globTmNoOfs[3][0] * move.x + (double)globTmNoOfs[3][1] * move.y + (double)globTmNoOfs[3][2] * move.z + (double)globTmNoOfs[3][3]};

  Point4 reprojected_world_pos = {(float)reprojected_world_pos_d[0], (float)reprojected_world_pos_d[1],
    (float)reprojected_world_pos_d[2], (float)reprojected_world_pos_d[3]};
  TMatrix4 globTmNoOfsFromCurCamPos = fi.globTmNoOfs;
  globTmNoOfsFromCurCamPos.setrow(3, globTmNoOfsFromCurCamPos.getrow(3) + reprojected_world_pos);
  globTmNoOfsFromCurCamPos = globTmNoOfsFromCurCamPos.transpose();

  ShaderGlobal::set_float4x4(sp_prev_globtm_from_current_camposVarId, globTmNoOfsFromCurCamPos);
  ShaderGlobal::set_color4(sp_prev_camera_pos_movementVarId, P3D(move), length(move));
  ShaderGlobal::set_color4(sp_prev_world_view_posVarId, P3D(fi.world_view_pos), 0);
  ShaderGlobal::set_color4(sp_prev_view_zVarId, P3D(fi.viewZ), 0);
  ShaderGlobal::set_color4(sp_prev_view_vecLTVarId, Color4(&fi.viewVecLT.x));
  ShaderGlobal::set_color4(sp_prev_view_vecRTVarId, Color4(&fi.viewVecRT.x));
  ShaderGlobal::set_color4(sp_prev_view_vecLBVarId, Color4(&fi.viewVecLB.x));
  ShaderGlobal::set_color4(sp_prev_view_vecRBVarId, Color4(&fi.viewVecRB.x));
  ShaderGlobal::set_color4(sp_prev_zn_zfarVarId, fi.znear, fi.zfar);
}

void ScreenSpaceProbes::calcProbesPlacement(const DPoint3 &world_pos, const TMatrix &viewItm, const TMatrix4 &proj, float zn, float zf)
{
  G_ASSERT_RETURN(allocated.sw != 0 && allocated.sh != 0, );
  DA_PROFILE_GPU;
  setCurrentScreenRes(next);
  setPrevScreenRes(prev);
  uint32_t prob64k = clamp(65536.f * currentTemporality, 0.f, 65536.f);
  ShaderGlobal::set_int4(screenspace_probes_temporalVarId, validHistory ? 1 : 0, frame, validHistory ? prob64k : 65536, frame % 8);
  auto cFrameInfo = get_frame_info(world_pos, viewItm, proj, zn, zf);
  set_prev_frame_info(cFrameInfo, prevFrameInfo);
  set_frame_info(cFrameInfo);
  calc_probe_pos();
  prevFrameInfo = cFrameInfo;
  validHistory = true;
}

void ScreenSpaceProbes::relightProbes(float quality, bool angle_filtering)
{
  DA_PROFILE_GPU;
  calc_probe_radiance(quality, angle_filtering);
  calc_probe_irradiance();
  prev = current;
  ++frame;
}

void ScreenSpaceProbes::initDebug()
{
  drawDebugAllProbes.init("screenspace_probes_draw_debug", NULL, 0, "screenspace_probes_draw_debug");
}
void ScreenSpaceProbes::drawDebugRadianceProbes(int debug_type)
{
  if (!drawDebugAllProbes.shader)
    initDebug();
  if (!drawDebugAllProbes.shader)
    return;
  DA_PROFILE_GPU;

  ShaderGlobal::set_int(debug_screenspace_probes_typeVarId, (int)debug_type);
  drawDebugAllProbes.shader->setStates(0, true);

  d3d::setvsrc(0, 0, 0);

  d3d::draw_instanced(PRIM_TRILIST, 0, LOW_SPHERES_INDICES_TO_DRAW, current.totalProbes);
}

void ScreenSpaceProbes::drawDebugTileClassificator()
{
  if (!tileClassificatorDebug.getElem())
    tileClassificatorDebug.init("screenspace_probes_draw_debug_classificator");
  DA_PROFILE_GPU;
  tileClassificatorDebug.render();
}

void ScreenSpaceProbes::drawSequence(uint32_t count)
{
  if (!drawJitteredPoints.shader)
    drawJitteredPoints.init("screenspace_probes_draw_sequence", NULL, 0, "screenspace_probes_draw_sequence");
  if (!drawJitteredPoints.shader)
    return;
  DA_PROFILE_GPU;
  ShaderGlobal::set_int(screenspace_probes_draw_sequence_countVarId, count);
  drawJitteredPoints.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);

  d3d::draw(PRIM_POINTLIST, 0, count);
  // d3d::draw_instanced(PRIM_POINTLIST, 0, 1, count);
}
