// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_frustum.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IBBox2.h>
#include <math/integer/dag_IBBox3.h>
#include <math/integer/dag_IPoint4.h>

#include <3d/dag_quadIndexBuffer.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_tex3d.h>
#include <shaders/dag_computeShaders.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_smallTab.h>
#include <textureUtil/textureUtil.h>

#include <daGI/daGI.h>
#include "global_vars.h"

#include "shaders/poisson_samples.hlsli"
#include "shaders/dagi_common_types.hlsli"
#include "shaders/dagi_scene_common_types.hlsli"
#include "shaders/dagi_voxels_consts.hlsli"
#include "shaders/octahedral_tile_side_length.hlsli"
#include "shaders/dagi_temporal_consts.hlsli"

#include "util/dag_convar.h"
#include <render/spheres_consts.hlsli>
#include <render/voxelization_target.h>
#include <frustumCulling/frustumPlanes.h>

CONSOLE_BOOL_VAL("render", debug_inline_rt, false);

#if D3D_HAS_RAY_TRACING
static int TOP_ACC_STRUCTURE_REGISTER = -1;
static const int OCTAHEDRAL_DISTANCES_REGISTER = 7;
static const int DEAD_PROBES_REGISTER = 8;
#endif

static constexpr int GI_MOVEMENT_THRESHOLD = 2;
#define GI_VIGNETEE_OFFSET 0.5f

extern Point3_vec4 POISSON_SAMPLES[SAMPLE_NUM];

static void setCSIndirect(Sbuffer *buf, int cnt = 1)
{
  if (buf)
  {
    uint *data;
    if (buf->lock(0, 0, (void **)&data, VBLOCK_WRITEONLY))
    {
      for (int i = 0; i < cnt; ++i, data += 3)
        data[0] = data[1] = data[2] = 1;
      buf->unlock();
    }
  }
}

static void setDrawIndirect(Sbuffer *buf)
{
  if (buf)
  {
    uint *data;
    if (buf->lock(0, 0, (void **)&data, VBLOCK_WRITEONLY))
    {
      data[0] = 36; // nverts
      data[1] = 1;  // instances
      data[2] = 0;  // start index
      data[3] = 0;  // base vertex
      data[4] = 0;  // base instance
      buf->unlock();
    }
  }
}


// #define GI3D_VERBOSE_DEBUG 1

GI3D::GI3D() : debugLastGTM(), volmap()
{
  init_global_vars();
  cascadeResolution[0] = Point2(0.45, 0.45);
  cascadeResolution[1] = cascadeResolution[0] * 3.0;
  setBouncingMode(BouncingMode::CONTINUOUS);
  init_voxelization();
  v_bbox3_init_empty(lastRestrictedUpdateGiBox);
}

bool GI3D::setSubsampledTargetAndOverride(int w, int h) { return set_voxelization_target_and_override(w, h); }

void GI3D::resetOverride() { reset_voxelization_override(); }

GI3D::~GI3D() { close_voxelization(); }

void GI3D::initDebug()
{
  drawDebugVolmap.init("debug_render_volmap", NULL, 0, "debug_render_volmap");
  drawDebugAllVolmap.init("debug_render_all_volmap", NULL, 0, "debug_render_all_volmap");
  create_debug_render_cs.reset(new_compute_shader("create_debug_render_cs"));

  debugVolmapDrawIndirect = dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, 1, "visibleVoxelsDrawIndirect");
  setDrawIndirect(debugVolmapDrawIndirect.get());
}


void GI3D::drawDebugAllProbes(int cascade, const Frustum &camera_frustum, DebugAllVolmapType debug_type)
{
  if (!drawDebugAllVolmap.shader)
    initDebug();
  if (!drawDebugAllVolmap.shader)
    return;
  TIME_D3D_PROFILE(debugVolmapAll);

  d3d::settm(TM_WORLD, TMatrix::IDENT);
  set_frustum_planes(camera_frustum);

  ShaderGlobal::set_int(debug_volmap_typeVarId, (int)debug_type);
  drawDebugAllVolmap.shader->setStates(0, true);

  d3d::setvsrc(0, 0, 0);
  index_buffer::use_box();
  cascade = clamp<int>(cascade, 0, volmap.size() - 1);
  ShaderGlobal::set_int(debug_cascadeVarId, cascade);

  if ((int)debug_type == 4)
    d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW,
      volmap[cascade].getDimXZ() * volmap[cascade].getDimXZ() * volmap[cascade].getDimY());
  else
    d3d::drawind_instanced(PRIM_TRILIST, 0, 12, 0,
      volmap[cascade].getDimXZ() * volmap[cascade].getDimXZ() * volmap[cascade].getDimY());

  d3d::setind(0);
}

void GI3D::drawDebug(int cascade, DebugVolmapType debug_volmap)
{
  if (!common.cull_ambient_voxels_cs.get())
    return;
  if (!drawDebugVolmap.shader)
    initDebug();
  if (!drawDebugVolmap.shader)
    return;
  bool debugVolmapNonIntersected = debug_volmap <= NON_INTERSECTED;
  if (debug_volmap > NON_INTERSECTED) // not yet
    return;
  TIME_D3D_PROFILE(debugVolmap);

  d3d::settm(TM_WORLD, TMatrix::IDENT);
  cascade = clamp<int>(cascade, 0, volmap.size() - 1);

  volmap[cascade].cull(common, debugLastGTM);
  TIME_D3D_PROFILE(draw_volmap_voxels);

  ShaderGlobal::set_int(debug_volmap_typeVarId, debug_volmap);
  ShaderGlobal::set_int(debug_cascadeVarId, cascade);

  if (debugVolmapNonIntersected)
  {
    static int frustum_visible_ambient_voxels_count_const_no =
      ShaderGlobal::get_slot_by_name("create_debug_render_cs_frustum_visible_ambient_voxels_count_const_no");
    static int drawIndirectBuffer_uav_no = ShaderGlobal::get_slot_by_name("create_debug_render_cs_drawIndirectBuffer_uav_no");

    d3d::set_rwbuffer(STAGE_CS, drawIndirectBuffer_uav_no, debugVolmapDrawIndirect.get());
    d3d::set_buffer(STAGE_CS, frustum_visible_ambient_voxels_count_const_no, common.frustumVisibleAmbientVoxelsCount.get());
    create_debug_render_cs->dispatch(1, 1, 1);
    d3d::set_buffer(STAGE_CS, frustum_visible_ambient_voxels_count_const_no, 0);
    d3d::set_rwbuffer(STAGE_CS, drawIndirectBuffer_uav_no, 0);
    // uint bins2[5] = {0};
    // load_data(bins2, debugVolmapDrawIndirect.get(), sizeof(bins2));
    // debug("debug visible probes %d %d %d %d %d", bins2[0], bins2[1], bins2[2], bins2[3], bins2[4]);
  }

  d3d::setvsrc(0, 0, 0);
  index_buffer::use_box();
  drawDebugVolmap.shader->setStates(0, true);
  d3d::draw_indexed_indirect(PRIM_TRILIST, debugVolmapDrawIndirect.get(), 0);
  d3d::set_buffer(STAGE_VS, 8, 0);
  d3d::setind(0);
}

void GI3D::VolmapCascade::init(const char *name_, const Point2 &sz, int xz, int y, int y_from, int y_total, int cascade_id)
{
  if (xz == getDimXZ() && y == getDimY() && sz == params.voxelSize)
    return;
  name = name_;
  cascadeId = cascade_id;
  params.xz_dim = xz;
  params.y_dim = y;
  params.tcclamp = float4(0.5 / params.xz_dim, 1 - 0.5 / params.xz_dim, 0.5 / params.y_dim, 1 - 0.5 / params.y_dim);
  // add tolerance for clipping while torodial origin is not updated
  params.tcClampDouble = float2(1. + 1. / params.xz_dim * sz.x, 1. + 1. / params.y_dim * sz.y);
  const float texelVerticalDisappearenceWidth = 4.0f;
  const float texelHorizontalDisappearenceWidth = 10.0f;
  const float2 fullDist = float2(params.xz_dim, params.y_dim) * float2(sz.x, sz.y);
  params.vignetteConsts = float4((fullDist.x - GI_VIGNETEE_OFFSET) / texelHorizontalDisappearenceWidth,
    (fullDist.y - GI_VIGNETEE_OFFSET) / texelVerticalDisappearenceWidth,
    -((fullDist.x - GI_MOVEMENT_THRESHOLD) / texelHorizontalDisappearenceWidth - 1.f),
    -((fullDist.y - GI_MOVEMENT_THRESHOLD) / texelVerticalDisappearenceWidth - 1.f));
  params.cascade_z_ofs = y_from;
  params.cascade_z_tc_mul_add.x = float(y) / y_total;
  params.cascade_z_tc_mul_add.y = float(y_from) / y_total;
  params.voxelSize = sz;

  voxelsWeightHistogram = dag::buffers::create_ua_sr_structured(sizeof(uint), HISTOGRAM_SIZE, "voxelsWeightHistogram");
  voxelChoiceProbability[0] = dag::buffers::create_ua_sr_structured(sizeof(float), MAX_BINS, "voxelsWeightHistogram0");
  voxelChoiceProbability[1] = dag::buffers::create_ua_sr_structured(sizeof(float), MAX_BINS, "voxelsWeightHistogram1");
  currentProbability = 0;
  toroidalOrigin = IPoint3(0, -10000, 0);
  resetted = true;
  fillBoxParams();
  uint v[4] = {0, 0, 0, 0};
  d3d::clear_rwbufi(voxelsWeightHistogram.get(), v); // todo:to be cleared in render if invalid
}

BBox3 GI3D::VolmapCascade::getBox() const
{
  const Point3 resolution = getResolution();
  Point3 alignedOrigin = mul(point3(toroidalOrigin), resolution);
  const Point3 wd = Point3((getDimXZ() * getVoxelSizeXZ()), (getDimY() * getVoxelSizeY()), (getDimXZ() * getVoxelSizeXZ()));
  const Point3 lt = alignedOrigin - Point3((getDimXZ() / 2 * getVoxelSizeXZ()), (getDimY() / 2 * getVoxelSizeY()),
                                      (getDimXZ() / 2 * getVoxelSizeXZ()));
  return BBox3(lt, lt + wd);
}

void GI3D::VolmapCommonData::initCommon()
{
  if (cull_ambient_voxels_cs.get())
    return;

  initEnviCube();
  cull_ambient_voxels_cs.reset(new_compute_shader("cull_ambient_voxels_cs"));
  cull_ambient_voxels_cs_warp_64.reset(new_compute_shader("cull_ambient_voxels_cs_warp_64"));
  warpSize = d3d::get_driver_desc().minWarpSize;
  debug("culling warp size %d", warpSize);
  light_ss_ambient_voxels_cs.reset(new_compute_shader("light_ss_ambient_voxels_cs"));
  light_ss_combine_ambient_voxels_cs.reset(new_compute_shader("light_ss_combine_ambient_voxels_cs"));
  light_initial_ambient_voxels_cs.reset(new_compute_shader("light_initial_ambient_voxels_cs"));
  calc_initial_ambient_walls_dist_cs.reset(new_compute_shader("calc_initial_ambient_walls_dist_cs"));
  light_initialize_ambient_voxels_cs.reset(new_compute_shader("light_initialize_ambient_voxels_cs"));

  light_partial_initialize_ambient_voxels_cs.reset(new_compute_shader("light_partial_initialize_ambient_voxels_cs"));
  light_initialize_culling_cs.reset(new_compute_shader("light_initialize_culling_cs"));
  light_initialize_culling_check_age_cs.reset(new_compute_shader("light_initialize_culling_check_age_cs"));
  light_initialize_clear_indirect_cs.reset(new_compute_shader("light_initialize_clear_indirect_cs"));

  create_point_occlusion_dispatch_cs.reset(new_compute_shader("create_point_occlusion_dispatch_cs"));
  ssgi_change_probability_cs.reset(new_compute_shader("ssgi_change_probability_cs"));
  random_point_occlusion_ambient_voxels_cs.reset(new_compute_shader("random_point_occlusion_ambient_voxels_cs"));
  temporal_ambient_voxels_cs.reset(new_compute_shader("temporal_ambient_voxels_cs"));

  frustumVisibleAmbientVoxelsCount = dag::buffers::create_ua_sr_structured(4, 1, "visibleAmbientVoxelsCount");

  visibleAmbientVoxelsIndirect = dag::buffers::create_ua_indirect(dag::buffers::Indirect::Dispatch, 5, "visibleVoxelsLitIndirect");
  setCSIndirect(visibleAmbientVoxelsIndirect.get(), 5);
  static constexpr int MAX_VISIBLE_FULL_VOXELS = MAX_SELECTED_INTERSECTED_VOXELS + MAX_SELECTED_NON_INTERSECTED_VOXELS;
  static constexpr int MAX_VISIBLE_VOXELS = MAX_VISIBLE_FULL_VOXELS + MAX_SELECTED_INITIAL_VOXELS;

  frustumVisiblePointVoxels =
    dag::buffers::create_ua_sr_structured(sizeof(VisibleAmbientVoxelPoint), MAX_VISIBLE_VOXELS, "frustum_visible_point_voxels");

  selectedAmbientVoxels =
    dag::buffers::create_ua_sr_structured(sizeof(VisibleAmbientVoxel), MAX_VISIBLE_FULL_VOXELS, "visibleAmbientVoxels");

  selectedAmbientVoxelsPlanes =
    dag::buffers::create_ua_sr_structured(sizeof(AmbientVoxelsPlanes), MAX_VISIBLE_VOXELS, "selectedAmbientVoxelsPlanes");

  traceRayResults = dag::buffers::create_ua_sr_structured(sizeof(TraceResultAmbientVoxel), MAX_VISIBLE_FULL_VOXELS * NUM_RAY_GROUPS,
    "traceRayResults");

  volmapCB = dag::buffers::create_persistent_cb(dag::buffers::cb_array_reg_count<VolmapCB>(MAX_VOLMAP_CASCADES), "VolmapCBuffer");

  {
    typedef Point3_vec4 float3;
    poissonBuf = dag::create_sbuffer(16, (sizeof(POISSON_SAMPLES) + 15) / 16, SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES,
      TEXFMT_A32B32G32R32F, "poissonSamples");
    poissonBuf->updateDataWithLock(0, sizeof(POISSON_SAMPLES), POISSON_SAMPLES, 0);
  }
  ssgi_clear_volmap_cs.reset(new_compute_shader("ssgi_clear_volmap_cs"));
  ssgi_copy_from_volmap_cs.reset(new_compute_shader("ssgi_copy_from_volmap_cs"));
  ssgi_copy_to_volmap_cs.reset(new_compute_shader("ssgi_copy_to_volmap_cs"));
  move_y_from_cs.reset(new_compute_shader("move_y_from_cs"));
  move_y_to_cs.reset(new_compute_shader("move_y_to_cs"));

  rt.initShaders(quality);
}

void GI3D::VolmapCommonData::invalidateTextureContent()
{
  if (ssgiTemporalWeight.getVolTex())
  {
    d3d::clear_rwtexf(ssgiTemporalWeight.getVolTex(), ResourceClearValue{}.asFloat, 0, 0);
    d3d::resource_barrier({ssgiTemporalWeight.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }
  if (cube.getVolTex())
  {
    d3d::clear_rwtexf(cube.getVolTex(), ResourceClearValue{}.asFloat, 0, 0);
    d3d::resource_barrier({cube.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }
}

void GI3D::VolmapCommonData::afterReset()
{
  invalidateTextureContent();
  setCSIndirect(visibleAmbientVoxelsIndirect.get(), 5);
  if (poissonBuf)
    poissonBuf->updateDataWithLock(0, sizeof(POISSON_SAMPLES), POISSON_SAMPLES, 0);
}

void GI3D::VolmapCommonData::init(int xz_dimensions, int y_dimensions, int max_voxels, bool scalar_ao)
{
  initCommon();
  maxVoxels = max_voxels;
  int format = scalar_ao ? TEXFMT_R8 : TEXFMT_R11G11B10F;
  cube = dag::create_voltex(xz_dimensions, xz_dimensions, y_dimensions * 6, format | TEXCF_UNORDERED, 1, "gi_ambient_volmap");
  cube->texaddr(TEXADDR_WRAP);
  cube->texaddrw(TEXADDR_CLAMP);

  d3d::resource_barrier({cube.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  ssgiTemporalWeight =
    dag::create_voltex(xz_dimensions, xz_dimensions, y_dimensions, TEXFMT_L8 | TEXCF_UNORDERED, 1, "ssgi_ambient_volmap_temporal");
  ssgiTemporalWeight->texaddr(TEXADDR_WRAP);
  ssgiTemporalWeight->texaddrw(TEXADDR_CLAMP);
  invalidateTextureContent();

  G_ASSERT(maxVoxels > 0);
  frustumVisibleAmbientVoxels = dag::buffers::create_ua_sr_structured(sizeof(uint), maxVoxels, "frustum_visible_ambient_voxels");
}

void GI3D::VolmapCommonData::RT::initShaders(QualitySettings quality)
{
  if (quality != RAYTRACING)
    return;
  octahedral_distances_cs.reset(new_compute_shader("octahedral_distances_cs"));
  move_y_octahderal_distances_cs.reset(new_compute_shader("move_y_octahderal_distances_cs"));
  move_y_dead_probes_cs.reset(new_compute_shader("move_y_dead_probes_cs"));
#if D3D_HAS_RAY_TRACING
  TOP_ACC_STRUCTURE_REGISTER = ShaderGlobal::get_slot_by_name("dagi_accelerationStructure_const_no");
#endif
}

void GI3D::VolmapCommonData::RT::createTextures(QualitySettings quality, int xz_dimensions, int y_dim)
{
  if (quality != RAYTRACING)
    return;
  int xzDim = xz_dimensions * OCTAHEDRAL_TILE_SIDE_LENGTH;
  octahedralDistances = dag::create_array_tex(xzDim, xzDim, y_dim, TEXFMT_R8 | TEXCF_UNORDERED, 1, "octahedral_distances");
  deadProbes = dag::create_array_tex(xz_dimensions, xz_dimensions, y_dim, TEXFMT_R8 | TEXCF_UNORDERED, 1, "dead_probes");
  deadProbes.getArrayTex()->texfilter(TEXFILTER_POINT);
}

void GI3D::VolmapCommonData::RT::createOctahedralDistances(int xz_dim, int y_dim)
{
#if D3D_HAS_RAY_TRACING
  if (!octahedralDistances.getArrayTex())
    return;
  STATE_GUARD_NULLPTR(d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, VALUE), sceneTop);
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, OCTAHEDRAL_DISTANCES_REGISTER, VALUE, 0, 0), octahedralDistances.getArrayTex());
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, DEAD_PROBES_REGISTER, VALUE, 0, 0), deadProbes.getArrayTex());
  octahedral_distances_cs->dispatch(xz_dim, xz_dim, y_dim);
#else
  G_UNUSED(xz_dim);
  G_UNUSED(y_dim);
#endif
}

void GI3D::setQuality(QualitySettings qs, float max_height_above_floor, float xz_25d_distance, float xz_3d_distance,
  float cascade0_scale, int xz_3d_res, int y_3d_res)
{
  // invalidate
  common.quality = qs;
  ShaderGlobal::set_int(gi_qualityVarId, common.quality);
  initSceneVoxels();

  xz_3d_res = clamp(xz_3d_res, 32, 128);
  y_3d_res = clamp(y_3d_res, 16, (xz_3d_res * sceneYResolution()) / sceneXZResolution());
  const float worstXZProbeSize = xz_3d_distance / xz_3d_res;
  const float best25DProbeSize = worstXZProbeSize * 1.45f; // we should at least have that probe size. probably even 2.f is OK
  xz_25d_distance = max(max(xz_25d_distance, xz_3d_distance * 2.f), irradiance25D.getXZResolution() * best25DProbeSize);
  const float probeSize25DXZ = xz_25d_distance / irradiance25D.getXZResolution();
  const float probeSize25DY = max_height_above_floor / (irradiance25D.getYResolution() - 0.5f);
  const int moveDist25D = scene25D.moveDistThreshold();            // we ray cast in meters
  const int traceDist25D = irradiance25D.getSceneCoordTraceDist(); // we ray cast in meters
  // *2 as it is from both sides, -1 is because of rounding error
  const int resolutionLeft25DXZ = scene25D.getXZResolution() - moveDist25D * 2 - traceDist25D * 2 - 1;
  G_ASSERT(resolutionLeft25DXZ > 1);
  const float scene25DVoxelSizeXZ = xz_25d_distance / resolutionLeft25DXZ;
  const float scene25DVoxelSizeY = max_height_above_floor / (scene25D.getYResolution() - 0.5f); // so center is on top
  // validate that it is correct
  const float traceDist = scene25D.getVoxelSizeXZ() * irradiance25D.getSceneCoordTraceDist();
  if (irradiance25D.getXZWidth() * 0.5f + traceDist > scene25D.getXZWidth() * 0.5f)
    logerr("25d scene size %f should be bigger than 25d light probe field size (%f) + traceDist = %f", scene25D.getXZWidth() * 0.5f,
      irradiance25D.getXZWidth() * 0.5f, traceDist);

  const float maxSceneVoxelSize = (xz_3d_distance + 2.f * MAX_TRACE_DIST) / (sceneXZResolution() - getCoordMoveThreshold() * 2);
  const float minSceneVoxelSize = maxSceneVoxelSize / (1 << (getSceneCascadesCount() - 1));
  const float probeSize1 = xz_3d_distance / xz_3d_res;
  const float probeSize0 = probeSize1 / clamp(cascade0_scale, 2.f, 4.f);
  cascade0Dist = probeSize0 * xz_3d_distance * 0.5;
  const int y_dim0 = y_3d_res;
  const int y_dim1 = max(y_dim0 * 3 / 4, 16);

  debug("GI inited with quality = %d\n"
        "25d: height %f gi width/probeSize = %f/(%fx%f), scene width/voxel: %f/(%fx%f)\n"
        "3d: res: %dx%d(cascade1=%d), width/probeSize0/probeSize1 : %f/%f/%f, scene width/voxel %f/%f",
    (int)qs, max_height_above_floor, xz_25d_distance, probeSize25DXZ, probeSize25DY, scene25DVoxelSizeXZ * scene25D.getXZResolution(),
    scene25DVoxelSizeXZ, scene25DVoxelSizeY, xz_3d_res, y_dim0, y_dim1, xz_3d_distance, probeSize0, probeSize1,
    sceneXZResolution() * maxSceneVoxelSize, minSceneVoxelSize);

  scene25dXZSize = scene25DVoxelSizeXZ;
  scene25dYSize = scene25DVoxelSizeY;
  xz_dimensions = xz_3d_res;
  y_dimensions = y_3d_res;
  irradiance25D.init(getQuality() == ONLY_AO, probeSize25DXZ, probeSize25DY);
  scene25D.init(getQuality() == ONLY_AO, scene25DVoxelSizeXZ, scene25DVoxelSizeY);
  updateVoxelMinSize(minSceneVoxelSize); // set scene voxelsize

  common.init(xz_dimensions, y_dim0 + y_dim1, xz_dimensions * xz_dimensions * max(y_dim1, y_dim0), getQuality() == ONLY_AO);
  Point4 fullVolmapResolution = Point4(xz_dimensions, xz_dimensions, (y_dim0 + y_dim1) * 6, -0.5);
  volmap[0].params.fullVolmapResolution = volmap[1].params.fullVolmapResolution = fullVolmapResolution;
  Point3 invFullVolmapResolution =
    Point3(safeinv(fullVolmapResolution.x), safeinv(fullVolmapResolution.y), safeinv(fullVolmapResolution.z));
  volmap[0].params.invFullVolmapResolution = volmap[1].params.invFullVolmapResolution = invFullVolmapResolution;

  cascadeResolution[0] = Point2(probeSize0, probeSize0);
  cascadeResolution[1] = Point2(probeSize1, probeSize1);

  volmap[0].init("dagi_ambient_volmap0", cascadeResolution[0], xz_dimensions, y_dim0, 0, y_dim0 + y_dim1, 0);
  volmap[1].init("dagi_ambient_volmap1", cascadeResolution[1], xz_dimensions, y_dim1, y_dim0, y_dim0 + y_dim1, 1);

  bool setRes0 = volmap[0].setResolution(cascadeResolution[0]);
  bool setRes1 = volmap[1].setResolution(cascadeResolution[1]);
  if (setRes0 || setRes1)
    updateVolmapCB();

  invalidateGiCascades();

  common.rt.createTextures(qs, xz_dimensions, y_dim0 + y_dim1);
}

void GI3D::init(int xz, int y, float scene25d_xz_size, float scene25d_scene_y_size)
{
  scene25D.init(getQuality() == ONLY_AO, scene25d_xz_size, scene25d_scene_y_size);
  irradiance25D.init(getQuality() == ONLY_AO);
  // validate that it is correct
  const float traceDist = scene25D.getVoxelSizeXZ() * irradiance25D.getSceneCoordTraceDist();
  if (irradiance25D.getXZWidth() * 0.5f + traceDist > scene25D.getXZWidth() * 0.5f)
    logerr("25d scene size %f should be bigger than 25d light probe field size (%f) + traceDist = %f", scene25D.getXZWidth() * 0.5f,
      irradiance25D.getXZWidth() * 0.5f, traceDist);
  scene25dXZSize = scene25d_xz_size;
  scene25dYSize = scene25d_scene_y_size;
  xz_dimensions = xz;
  y_dimensions = y;
  initSceneVoxels();
  int y_dim0 = y_dimensions;
  int y_dim1 = y_dimensions * 3 / 4;
  common.init(xz_dimensions, y_dim0 + y_dim1, xz_dimensions * xz_dimensions * max(y_dim1, y_dim0), getQuality() == ONLY_AO);

  Point4 fullVolmapResolution = Point4(xz_dimensions, xz_dimensions, (y_dim0 + y_dim1) * 6, -0.5);
  volmap[0].params.fullVolmapResolution = volmap[1].params.fullVolmapResolution = fullVolmapResolution;
  Point3 invFullVolmapResolution =
    Point3(safeinv(fullVolmapResolution.x), safeinv(fullVolmapResolution.y), safeinv(fullVolmapResolution.z));
  volmap[0].params.invFullVolmapResolution = volmap[1].params.invFullVolmapResolution = invFullVolmapResolution;
  volmap[0].init("dagi_ambient_volmap0", cascadeResolution[0], xz_dimensions, y_dim0, 0, y_dim0 + y_dim1, 0);
  volmap[1].init("dagi_ambient_volmap1", cascadeResolution[1], xz_dimensions, y_dim1, y_dim0, y_dim0 + y_dim1, 1);
  updateVolmapCB();

  // preselect by probability count:
  // min(max(MAX_VISIBLE_VOXELS*4, ambientVoxelsXZ*ambientVoxelsXZ*y_dimensions)/2, ambientVoxelsXZ*ambientVoxelsXZ*y_dimensions)
  // ssgi_draw_debug.reset(new_compute_shader("ssgi_draw_debug"));
  common.rt.createTextures(common.quality, xz_dimensions, y_dim0 + y_dim1);
}

void GI3D::VolmapCascade::setVars() const
{
  // todo: set volmapbox variablse
}

void GI3D::setVars()
{
  volmap[0].setVars();
  common.ssgiTemporalWeight.setVar();
  common.cube.setVar();
}

void GI3D::setQuality(QualitySettings set)
{
  if (set == RAYTRACING)
    return; // don't double init
  common.quality = set;
  init(xz_dimensions, y_dimensions, scene25dXZSize, scene25dYSize);
  invalidateGiCascades();
  // todo: resurrect windows buffer
  ShaderGlobal::set_int(gi_qualityVarId, common.quality);
}

void GI3D::setBouncingMode(BouncingMode mode)
{
  bouncingMode = mode;
  float temporalWeightLimit = SSGI_TEMPORAL_MAX_VALUE + 0.1;
  switch (mode)
  {
    case BouncingMode::ONE_BOUNCE: temporalWeightLimit = SSGI_TEMPORAL_VALID_VALUE; break;
    case BouncingMode::TILL_CONVERGED: temporalWeightLimit = SSGI_TEMPORAL_MAX_VALUE; break;
    case BouncingMode::CONTINUOUS:
      temporalWeightLimit = SSGI_TEMPORAL_MAX_VALUE + 0.1; // limit will never be reached
      break;
    default: break;
  }
  ShaderGlobal::set_real(ssgi_temporal_weight_limitVarId, temporalWeightLimit - 0.001);
}

bool GI3D::VolmapCascade::setResolution(const Point2 &sz)
{
  if (params.voxelSize == sz)
    return false;
  toroidalOrigin = IPoint3(0, -10000, 0);
  resetted = true;
  params.voxelSize.x = sz.x;
  params.voxelSize.y = sz.y;
  fillBoxParams();
  return true;
}

void GI3D::setMaxVoxelsPerBin(float intersected, float non_intersected, float initial)
{
  bool changed = volmap[0].setMaxVoxelsPerBin(intersected * MAX_SELECTED_INTERSECTED_VOXELS,
    non_intersected * MAX_SELECTED_NON_INTERSECTED_VOXELS, initial * MAX_SELECTED_INITIAL_VOXELS);

  changed |= volmap[1].setMaxVoxelsPerBin(intersected * MAX_SELECTED_INTERSECTED_VOXELS,
    non_intersected * MAX_SELECTED_NON_INTERSECTED_VOXELS, initial * MAX_SELECTED_INITIAL_VOXELS);
  if (changed && !volmap[0].resetted && !volmap[1].resetted)
    updateVolmapCB();
}

bool GI3D::VolmapCascade::setMaxVoxelsPerBin(uint32_t intersected, uint32_t non_intersected, uint32_t initial)
{
  uint4 max_voxels;
  max_voxels.x = clamp(intersected, 16U, (uint32_t)MAX_SELECTED_INTERSECTED_VOXELS);
  max_voxels.y = clamp(non_intersected, 16U, (uint32_t)MAX_SELECTED_NON_INTERSECTED_VOXELS);
  max_voxels.z = clamp(initial, 256U, (uint32_t)MAX_SELECTED_INITIAL_VOXELS);
  max_voxels.w = max_voxels.x + max_voxels.y;
  if (memcmp(&params.max_voxels_per_bin, &max_voxels, sizeof(max_voxels)) != 0)
  {
    params.max_voxels_per_bin = max_voxels;
    return true;
  }
  return false;
}

void GI3D::updateVolmapResolution(float xz_0, float y_0, float xz_1, float y_1)
{
  if (cascadeResolution[0] == Point2(xz_0, y_0) && cascadeResolution[1] == Point2(xz_1, y_1))
    return;
  cascadeResolution[0] = Point2(xz_0, y_0);
  cascadeResolution[1] = Point2(xz_1, y_1);
  bool setRes0 = volmap[0].setResolution(cascadeResolution[0]);
  bool setRes1 = volmap[1].setResolution(cascadeResolution[1]);
  if (setRes0 || setRes1)
    updateVolmapCB();
}

void GI3D::VolmapCascade::fillBoxParams()
{
  const BBox3 box = getBox();

  const Point3 resolution = getResolution();
  params.alignedOrigin = mul(point3(toroidalOrigin), resolution);
  const Point3 wd = box.width();
  const Point3 invW = div(Point3(1, 1, 1), wd);
  const Point3 lt = box[0];
  params.box0 = invW;
  params.box1 = -mul(lt, invW);
  params.coord_to_world_add = 0.5 * resolution + lt;

  params.crd_ofs_xz.x = getDimXZ() ? (getDimXZ() - toroidalOrigin.x % int(getDimXZ())) % getDimXZ() : 0;
  params.crd_ofs_xz.y = getDimXZ() ? (getDimXZ() - toroidalOrigin.z % int(getDimXZ())) % getDimXZ() : 0;
  params.tc_ofs = getDimXZ() ? Point2(params.crd_ofs_xz.x, params.crd_ofs_xz.y) / getDimXZ() : Point2(0, 0);
  params.crd_pofs_uint2 = uint2{uint(getDimXZ() + params.crd_ofs_xz.x), uint(getDimXZ() + params.crd_ofs_xz.y)};
  params.crd_nofs_uint2 = uint2{uint(getDimXZ() - params.crd_ofs_xz.x), uint(getDimXZ() - params.crd_ofs_xz.y)};
  // debug("%d: fillBoxParams params.crd_ofs.y = %@ toroidalOrigin = %@", cascadeId, params.crd_ofs.y, toroidalOrigin);
}

bool GI3D::VolmapCascade::moveY(VolmapCommonData &common, int move_y)
{
  TIME_D3D_PROFILE(ssgi_move_y);
  // debug("! move_y=%d", move_y);
  if (abs(move_y) < getDimY())
  {
    const int startI = move_y < 0 ? -move_y : getDimY() - 1 - move_y, endI = move_y < 0 ? getDimY() : -1;
    const int maxLines = common.maxVoxels / (getDimXZ() * getDimXZ() * 7);
    for (int i = startI, step = move_y < 0 ? 1 : -1; i != endI;)
    {
      d3d::resource_barrier({common.ssgiTemporalWeight.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      d3d::resource_barrier({common.cube.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      ShaderGlobal::set_color4(ssgi_copy_y_indices0VarId, getDimXZ(), getDimY(), params.cascade_z_ofs, step);
      int to = (i + move_y + getDimY()) % getDimY();
      ShaderGlobal::set_color4(ssgi_copy_y_indices1VarId, i, to, 0, 0);
      // debug("move %d to %d (%d)", i, to, params.cascade_z_ofs);
      d3d::set_rwbuffer(STAGE_CS, 6, common.frustumVisibleAmbientVoxels.getBuf());
      const int cnt = (i + step == endI || maxLines == 1
                         ? 1
                         : (i + step * 2 == endI || maxLines == 2 ? 2 : (i + step * 3 == endI || maxLines == 3 ? 3 : 4)));
      // debug("cnt = %d", cnt);
      common.move_y_from_cs->dispatch((getDimXZ() + 7) / 8, (getDimXZ() + 7) / 8, cnt);

      d3d::resource_barrier({common.frustumVisibleAmbientVoxels.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

      d3d::set_rwtex(STAGE_CS, 6, common.cube.getVolTex(), 0, 0);
      d3d::set_rwtex(STAGE_CS, 7, common.ssgiTemporalWeight.getVolTex(), 0, 0);
      common.move_y_to_cs->dispatch((getDimXZ() + 7) / 8, (getDimXZ() + 7) / 8, cnt);

      d3d::set_rwtex(STAGE_CS, 7, 0, 0, 0);
      i += step * cnt;
    }
    d3d::resource_barrier({common.ssgiTemporalWeight.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({common.cube.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    d3d::set_rwtex(STAGE_CS, 6, 0, 0, 0);
#if D3D_HAS_RAY_TRACING
    if (common.quality == RAYTRACING)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, OCTAHEDRAL_DISTANCES_REGISTER, VALUE, 0, 0),
        common.rt.octahedralDistances.getArrayTex());
      int step = move_y < 0 ? 1 : -1;
      ShaderGlobal::set_color4(ssgi_copy_y_indices0VarId, getDimXZ(), getDimY(), params.cascade_z_ofs, step);
      for (int i = startI; i != endI; i += step)
      {
        int to = (i + move_y + getDimY()) % getDimY();
        ShaderGlobal::set_color4(ssgi_copy_y_indices1VarId, i, to, 0, 0);
        common.rt.move_y_octahderal_distances_cs->dispatch(getDimXZ(), getDimXZ(), 1);
        common.rt.move_y_dead_probes_cs->dispatch(getDimXZ(), getDimXZ(), 1);
      }
    }
#endif
  }
  // toroidalOrigin.y -= move_y;
  // toroidalOfsY = (toroidalOrigin.y%getDimY() + getDimY())%getDimY();
  // debug("move_y=%d", move_y);
  // fillBoxParams();
  // cb->updateDataWithLock(0, sizeof(params), &params, VBLOCK_WRITEONLY|VBLOCK_DISCARD);
  return true;
}

void GI3D::VolmapCascade::copyFromNext(VolmapCommonData &common)
{
  TIME_D3D_PROFILE(ssgi_copy_course);
  ShaderGlobal::set_color4(ssgi_copy_indicesVarId, 1, getDimXZ(), getDimXZ() * getDimXZ(), 0);
  const int maxLines = common.maxVoxels / (getDimXZ() * getDimXZ() * 6);
  G_UNUSED(maxLines);
  G_ASSERT(maxLines >= 4);
  for (int i = 0; i < getDimY(); i += 4)
  {
    d3d::resource_barrier({common.cube.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    ShaderGlobal::set_color4(ssgi_start_copy_sliceVarId, 0, 0, i, 0);

    static int voxelColors_uav_no = ShaderGlobal::get_slot_by_name("ssgi_copy_from_volmap_cs_voxelColors_uav_no");
    d3d::set_rwbuffer(STAGE_CS, voxelColors_uav_no, common.frustumVisibleAmbientVoxels.getBuf());
    common.ssgi_copy_from_volmap_cs->dispatch((getDimXZ() + 3) / 4, (getDimXZ() + 3) / 4, 1);
    d3d::resource_barrier({common.frustumVisibleAmbientVoxels.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    d3d::set_rwbuffer(STAGE_CS, voxelColors_uav_no, 0);

    d3d::resource_barrier({common.ssgiTemporalWeight.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});

    static int gi_ambient_volmap_uav_no = ShaderGlobal::get_slot_by_name("ssgi_copy_to_volmap_cs_gi_ambient_volmap_uav_no");
    static int ssgi_ambient_volmap_temporal_uav_no =
      ShaderGlobal::get_slot_by_name("ssgi_copy_to_volmap_cs_ssgi_ambient_volmap_temporal_uav_no");

    d3d::set_rwtex(STAGE_CS, gi_ambient_volmap_uav_no, common.cube.getVolTex(), 0, 0);
    d3d::set_rwtex(STAGE_CS, ssgi_ambient_volmap_temporal_uav_no, common.ssgiTemporalWeight.getVolTex(), 0, 0);

    // todo: call copy only where needed, not full box
    common.ssgi_copy_to_volmap_cs->dispatch((getDimXZ() + 3) / 4, (getDimXZ() + 3) / 4, 1);

    d3d::set_rwtex(STAGE_CS, gi_ambient_volmap_uav_no, 0, 0, 0);
    d3d::set_rwtex(STAGE_CS, ssgi_ambient_volmap_temporal_uav_no, 0, 0, 0);
  }
}

bool GI3D::VolmapCascade::toroidalUpdateVolmap(const Point3 &origin, VolmapCommonData &common, int last_cascade_id,
  const BBox3 &sceneBox)
{
  TIME_D3D_PROFILE_NAME(cascade, cascadeId == 0 ? "cascade0" : "cascade1");
  IPoint3 newTexelsOrigin = ipoint3(floor(div(origin, getResolution()) + Point3(0.5, 0.5, 0.5)));

  IPoint3 imove = (toroidalOrigin - newTexelsOrigin);
  IPoint3 amove = abs(imove);
  static constexpr int THRESHOLD = GI_MOVEMENT_THRESHOLD;
  if ((amove.x < THRESHOLD && amove.z < THRESHOLD && amove.y < THRESHOLD))
    return false;
  if (amove.y != 0 && max(amove.x, amove.z) < getDimXZ())
  {
    moveY(common, imove.y);
  }
  // d3d::set_const_buffer(STAGE_CS, 3, cb.get());
  {
    ShaderGlobal::set_color4(ambient_voxels_move_ofsVarId, imove.x, imove.y, imove.z, cascadeId); //==00
    const bool shouldCopyFromCoarse = (cascadeId < last_cascade_id) && getDimY() >= 24; // because we process 4 slices at a time
    // fixme: we'd better organize based on direction, and support dims less than 24

    if (shouldCopyFromCoarse)
    {
      copyFromNext(common);
    }
    else
    {
      TIME_D3D_PROFILE(ssgi_clear_temporal_w);
      d3d::set_rwtex(STAGE_CS, 6, common.cube.getVolTex(), 0, 0);
      d3d::set_rwtex(STAGE_CS, 7, common.ssgiTemporalWeight.getVolTex(), 0, 0);
      // todo: call clear only where needed, not full box
      common.ssgi_clear_volmap_cs->dispatch((getDimXZ() + 3) / 4, (getDimXZ() + 3) / 4, (getDimY() + 3) / 4);
      d3d::set_rwtex(STAGE_CS, 6, 0, 0, 0);
      d3d::set_rwtex(STAGE_CS, 7, 0, 0, 0);
    }

    const bool raycast_initial = true;
    const bool raycast_initial_prev_cascade = true; // todo: this is quality setting
    if (raycast_initial && (raycast_initial_prev_cascade || cascadeId == last_cascade_id))
    {
      d3d::resource_barrier({common.ssgiTemporalWeight.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      d3d::resource_barrier({common.cube.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
      if (max(amove.x, amove.z) >= getDimXZ() || amove.y >= getDimY()) // invalidated whole cascade
      {
        TIME_D3D_PROFILE(raycast_initialize_cs);
#if D3D_HAS_RAY_TRACING
        if (common.quality == RAYTRACING)
        {
          d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, common.rt.sceneTop);
        }
#endif
        // d3d::set_const_buffer(STAGE_CS, 6, cb.get());
        d3d::set_rwtex(STAGE_CS, 6, common.cube.getVolTex(), 0, 0);
        common.light_initialize_ambient_voxels_cs->dispatch(getDimXZ(), getDimXZ(), getDimY());
        d3d::set_rwtex(STAGE_CS, 6, 0, 0, 0);
#if D3D_HAS_RAY_TRACING
        if (common.quality == RAYTRACING)
        {
          d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, 0);
          common.rt.createOctahedralDistances(getDimXZ(), getDimY());
        }
#endif
      }
      else
      {
        // limit dispatch to changed only
        TIME_D3D_PROFILE(racyast_initialize_partial_cs);
        d3d::set_rwbuffer(STAGE_CS, 0, common.visibleAmbientVoxelsIndirect.get());
        d3d::set_rwbuffer(STAGE_CS, 1, common.frustumVisibleAmbientVoxels.getBuf());
        common.light_initialize_clear_indirect_cs->dispatch(1, 1, 1);
        d3d::resource_barrier({common.visibleAmbientVoxelsIndirect.get(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
        if (cascadeId == last_cascade_id)
          common.light_initialize_culling_cs->dispatch((getDimXZ() + 3) / 4, (getDimXZ() + 3) / 4, (getDimY() + 3) / 4);
        else
          common.light_initialize_culling_check_age_cs->dispatch((getDimXZ() + 3) / 4, (getDimXZ() + 3) / 4, (getDimY() + 3) / 4);
        d3d::set_rwbuffer(STAGE_CS, 1, 0);
        d3d::set_rwbuffer(STAGE_CS, 0, 0);

        d3d::resource_barrier({common.visibleAmbientVoxelsIndirect.get(), RB_RO_INDIRECT_BUFFER});
        d3d::resource_barrier({common.frustumVisibleAmbientVoxels.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

        // d3d::set_const_buffer(STAGE_CS, 6, cb.get());
        d3d::set_rwtex(STAGE_CS, 6, common.cube.getVolTex(), 0, 0);
#if D3D_HAS_RAY_TRACING
        if (common.quality == RAYTRACING)
        {
          d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, common.rt.sceneTop);
        }
#endif
        common.light_partial_initialize_ambient_voxels_cs->dispatch_indirect(common.visibleAmbientVoxelsIndirect.get(), 0);
        d3d::set_rwtex(STAGE_CS, 6, 0, 0, 0);
#if D3D_HAS_RAY_TRACING
        if (common.quality == RAYTRACING)
        {
          d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, 0);
          common.rt.createOctahedralDistances(getDimXZ(), getDimY());
        }
#endif
      }
    }
    d3d::resource_barrier({common.cube.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
  toroidalOrigin = newTexelsOrigin;
  G_UNUSED(sceneBox);
#if DAGOR_DBGLEVEL > 0 && 0
  // currently it does happen, because we update only one voxelcascade per frame, and so GI is volmap is updatign first
  // this is done due to we want voxelization happens with GI data
  // but that is chicken&egg problem - GI is only correct if there is voxelized data
  // voxelized data is only correct if it was voxelized where there is GI
  //  the only work around is to keep voxel data bigger by at least it's movement threshold
  //  and always use same origin, rather than "current" origin
  BBox3 box = getBox();
  if (box[0].x < sceneBox[0].x || box[1].x > sceneBox[1].x || box[0].z < sceneBox[0].z || box[1].z > sceneBox[1].z ||
      box[0].y < sceneBox[0].y || box[1].y > sceneBox[1].y)
  {
    logerr("cascade %d with %@ box is going to be rendered outside scene voxelBox %@", cascadeId, getBox(), sceneBox);
  }
#endif
  fillBoxParams();
  return true;
}

bool GI3D::VolmapCascade::updateOrigin(const Point3 &center, VolmapCommonData &common, int last_cascade_id, const BBox3 &sceneBox)
{
  if (!getDimY() || !getDimXZ())
    return false;
  resetted = false;
  return toroidalUpdateVolmap(center, common, last_cascade_id, sceneBox);
}

void GI3D::updateVolmapCB()
{
  VolmapCB cascades[MAX_VOLMAP_CASCADES] = {volmap[0].getParams(), volmap[1].getParams()};
  common.volmapCB->updateDataWithLock(0, sizeof(cascades), &cascades, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
}

float GI3D::getLightDist3D() const
{
  if (!common.cube)
    return 0;
  float distXZ = volmap[1].getDimXZ() * volmap[1].getVoxelSizeXZ(), distY = volmap[1].getDimY() * volmap[1].getVoxelSizeY();
  return 0.5f * sqrtf(distXZ * distXZ * 2 + distY * distY);
}

float GI3D::getLightMaxDist() const { return max(getLightDist3D(), irradiance25D.getMaxDist()); }

extern ConVarT<bool, false> gi_25d_force_update_scene, gi_25d_force_update_volmap;

void GI3D::updateOrigin(const Point3 &center, const dagi25d::voxelize_scene_fun_cb &voxelize_25d_cb,
  const render_scene_fun_cb_t &preclear_cb, const render_scene_fun_cb_t &voxelize_cb, int scenes)
{
  TIME_D3D_PROFILE(gi_update_origin);
  calcEnviCube();
  bool shouldUpdate3D = true;
  bool voxelsUpdated = false;
  updateWallsPos(center);
  {
    TIME_D3D_PROFILE(gi_update_origin_25d);
    dagi25d::UpdateResult sceneMoved = scene25D.updateOrigin(center, voxelize_25d_cb);
    if (sceneMoved == dagi25d::UpdateResult::TELEPORTED && !gi_25d_force_update_scene.get())
      shouldUpdate3D = false; // only update 3d scene if we hadn't updated 2D scene
    else
    {
      // only update 3d scene if we hadn't updated 2D scene&irradiance
      if (irradiance25D.updateOrigin(center) == irradiance25D.UPDATE_TELEPORTED && !gi_25d_force_update_volmap.get())
        shouldUpdate3D = false;
    }
  }
#if DAGOR_DBGLEVEL > 0
  if (cascadeResolution[1].x * xz_dimensions + 2. * MAX_TRACE_DIST > sceneDistanceXZ() - sceneDistanceMoveThreshold() * 2)
  {
    logerr("%d: wrong GI configuration."
           " Probe field of resolution %d*size=%f + traceDist=%f*2 supposed to be totally inside scene of size %f-2*%f",
      1, xz_dimensions, cascadeResolution[1].x, MAX_TRACE_DIST, sceneDistanceXZ(), sceneDistanceMoveThreshold());
  }
#endif
  ShaderGlobal::set_color4(ssgi_current_world_originVarId, center.x, center.y, center.z, 0);
  bool hasSceneTeleported = false;
  if (shouldUpdate3D && common.cube)
  {
    updateWindowsPos(center);
    hasSceneTeleported = isSceneTeleported(center);
    if (hasSceneTeleported)
      updateOriginScene(center, preclear_cb, voxelize_cb, scenes, false);
    // todo: we need 3 volmap cascades. With that we can increase quality and keep speed, and probably even decrease mem usage
    // todo: each cascade should be completely within it's scene cascade and it's trace within scene cascade + 1
    bool changed = volmap[1].updateOrigin(center, common, volmap.size() - 1, getSceneBox(1));                          // cascade + 1
    changed |= volmap[0].updateOrigin(center, common, volmap[0].isResetted() ? 0 : volmap.size() - 1, getSceneBox(1)); // avoid copy if
                                                                                                                       // it is whole
                                                                                                                       // cascade
    if (changed)
      updateVolmapCB();
    if (!hasSceneTeleported || getQuality() != ONLY_AO) // if we already teleported and rendered everything, and we don't provide
                                                        // multiple bounce, than there is no need to revoxelize the scene
      voxelsUpdated = updateOriginScene(center, preclear_cb, voxelize_cb, scenes, hasSceneTeleported);
  }

  if (hasSceneTeleported || !(sceneVoxelsColor.getVolTex() || sceneVoxelsAlpha.getVolTex())) // whole scene was voxelized anyway, or
                                                                                             // there is no need to call anything
    clear_and_shrink(invalidBoxes);

  if (!voxelsUpdated)
  {
    size_t tabsize = invalidBoxes.size();
    if (tabsize)
    {
      size_t size = ::min<size_t>(tabsize, MAX_BOXES_INV_PER_FRAME);
      if (invalidBoxesSB->updateDataWithLock(0, size * sizeof(bbox3f), (float *)make_span(invalidBoxes).last(size).data(), 0))
      {
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), sceneVoxelsColor.getVolTex());
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), sceneVoxelsAlpha.getVolTex());
        ssgi_invalidate_voxels_cs->dispatch(size, sceneCascades.size(), 1);
        if (sceneVoxelsColor.getVolTex())
          d3d::resource_barrier({sceneVoxelsColor.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
        if (sceneVoxelsAlpha.getVolTex())
          d3d::resource_barrier({sceneVoxelsAlpha.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
        // fixme: we need to re-voxelize the scene inside invalid boxes! not just clear the scene!
        // todo: we need to re-voxelize the scene inside invalid boxes! not just clear the scene!
        invalidBoxes.resize(tabsize - size);
      }
    }
  }
}

void GI3D::VolmapCascade::cull(VolmapCommonData &common, mat44f_cref globtm)
{
  G_ASSERT(common.cull_ambient_voxels_cs);

  Frustum frustum(globtm);

  TIME_D3D_PROFILE(cull_frustum_voxels_cs); // todo:we can speed up by checking chunks first
  set_frustum_planes(frustum);
  unsigned v[4] = {0, 0, 0, 0};
  d3d::clear_rwbufi(common.frustumVisibleAmbientVoxelsCount.get(), v); // todo: to be cleared once
  d3d::resource_barrier({common.frustumVisibleAmbientVoxelsCount.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  d3d::set_rwbuffer(STAGE_CS, 0, common.frustumVisibleAmbientVoxelsCount.get());
  d3d::set_rwbuffer(STAGE_CS, 1, common.frustumVisibleAmbientVoxels.getBuf());
// old way
#if CULL_ALL_CASCADE
  common.cull_ambient_voxels_cs->dispatch((getDimXZ() + 3) / 4, (getDimXZ() + 3) / 4, (getDimY() + 3) / 4);
#else
  bbox3f frustumBox;
  frustum.calcFrustumBBox(frustumBox);
  vec4f voxelSizeV = v_make_vec4f(getVoxelSizeXZ(), getVoxelSizeY(), getVoxelSizeXZ(), 1.f);
  vec4i ltCoordi =
    v_make_vec4i(toroidalOrigin.x - getDimXZ() / 2, toroidalOrigin.y - getDimY() / 2, toroidalOrigin.z - getDimXZ() / 2, 0);
  vec4i rbCoordi = v_addi(ltCoordi, v_make_vec4i(getDimXZ() - 1, getDimY() - 1, getDimXZ() - 1, 0));
  vec4i coordMin = v_maxi(v_cvt_floori(v_div(frustumBox.bmin, voxelSizeV)), ltCoordi),
        coordMax = v_mini(v_cvt_ceili(v_div(frustumBox.bmax, voxelSizeV)), rbCoordi);
  alignas(16) IPoint4 widthi, starti;
  v_sti(&widthi.x, v_addi(v_subi(coordMax, coordMin), v_splatsi(1)));
  v_sti(&starti.x, coordMin);
  if (widthi.x > 0 && widthi.y > 0 && widthi.z > 0)
  {
    ShaderGlobal::set_color4(ambient_voxels_visible_startVarId, starti.x, starti.y, starti.z, widthi.x * widthi.z);
    ShaderGlobal::set_color4(ambient_voxels_visible_widthVarId, widthi.x, toroidalOrigin.y - getDimY() / 2, 0,
      widthi.x * widthi.z * widthi.y);
    if (common.warpSize == 64)
      common.cull_ambient_voxels_cs_warp_64->dispatch((widthi.x * widthi.y * widthi.z + 63) / 64, 1, 1);
    else
      common.cull_ambient_voxels_cs->dispatch((widthi.x * widthi.y * widthi.z + 31) / 32, 1, 1);
  }
  else
  {
    ShaderGlobal::set_color4(ambient_voxels_visible_startVarId, starti.x, starti.y, starti.z, 0);
    ShaderGlobal::set_color4(ambient_voxels_visible_widthVarId, widthi.x, toroidalOrigin.y - getDimY() / 2, 0, 0);
  }
#endif
  d3d::set_rwbuffer(STAGE_CS, 1, 0);

  {
    TIME_D3D_PROFILE(copy);

    d3d::set_rwbuffer(STAGE_CS, 0, common.visibleAmbientVoxelsIndirect.get());
    d3d::resource_barrier({common.frustumVisibleAmbientVoxelsCount.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
    d3d::set_buffer(STAGE_CS, 1, common.frustumVisibleAmbientVoxelsCount.get());
    common.create_point_occlusion_dispatch_cs->dispatch(1, 1, 1);
    d3d::set_buffer(STAGE_CS, 1, 0);
    d3d::set_rwbuffer(STAGE_CS, 0, 0);
    d3d::resource_barrier({common.visibleAmbientVoxelsIndirect.get(), RB_RO_INDIRECT_BUFFER});
#if GI3D_VERBOSE_DEBUG
    uint bins2[1] = {0};
    load_data(bins2, common.frustumVisibleAmbientVoxelsCount.get(), sizeof(bins2));
    debug("total visible probes %d", bins2[0]);
#if 0 // debug for vulkan bullshit write
        SmallTab<uint32_t, TmpmemAlloc> frustumVoxels;frustumVoxels.resize(bins2[0]);
        load_data(frustumVoxels.data(), common.frustumVisibleAmbientVoxels.getBuf(), data_size(frustumVoxels));
        for (int fi = 0; fi < frustumVoxels.size(); ++fi)
        {
          if ((frustumVoxels[fi]>>24) > 1)
            debug("%d: bin invalid %d!", fi, (frustumVoxels[fi]>>24));
            //debug("%d: bin invalid %d!", fi, frustumVoxels[fi]);
        }
#endif
    uint bins[3] = {0};
    load_data(bins, common.visibleAmbientVoxelsIndirect.get(), sizeof(bins));
    debug("dispatch indirect for stochastic %d %d %d", bins[0], bins[1], bins[2]);
#endif
  }
}

void GI3D::VolmapCascade::render(VolmapCommonData &common, mat44f_cref globtm)
{
  TIME_D3D_PROFILE_NAME(cascade, cascadeId == 0 ? "cascade0" : "cascade1");
  ShaderGlobal::set_int(cascade_idVarId, cascadeId);

  {
    if (cascadeId != CASCADES_3D - 1)
    {
      // we need to insert barrier, as we have used RB_NONE previously
      d3d::resource_barrier({common.ssgiTemporalWeight.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    }
    cull(common, globtm);
    {
      TIME_D3D_PROFILE(choose_random_points);
      {
        d3d::resource_barrier({common.frustumVisibleAmbientVoxels.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
        d3d::resource_barrier({voxelChoiceProbability[currentProbability].get(), RB_RO_SRV | RB_STAGE_COMPUTE});

        static int prev_probability_const_no =
          ShaderGlobal::get_slot_by_name("random_point_occlusion_ambient_voxels_cs_prev_probability_const_no");
        static int frustum_visible_ambient_voxels_count_const_no =
          ShaderGlobal::get_slot_by_name("random_point_occlusion_ambient_voxels_cs_frustum_visible_ambient_voxels_count_const_no");
        static int visible_ambient_voxels_uav_no =
          ShaderGlobal::get_slot_by_name("random_point_occlusion_ambient_voxels_cs_visible_ambient_voxels_uav_no");
        static int voxelsWeightHistogram_uav_no =
          ShaderGlobal::get_slot_by_name("random_point_occlusion_ambient_voxels_cs_voxelsWeightHistogram_uav_no");

        d3d::set_buffer(STAGE_CS, prev_probability_const_no, voxelChoiceProbability[currentProbability].get());
        d3d::set_buffer(STAGE_CS, frustum_visible_ambient_voxels_count_const_no, common.frustumVisibleAmbientVoxelsCount.get());
        d3d::set_rwbuffer(STAGE_CS, voxelsWeightHistogram_uav_no, voxelsWeightHistogram.get());
        d3d::set_rwbuffer(STAGE_CS, visible_ambient_voxels_uav_no, common.frustumVisiblePointVoxels.getBuf());
        common.random_point_occlusion_ambient_voxels_cs->dispatch_indirect(common.visibleAmbientVoxelsIndirect.get(), 0);
        d3d::set_rwbuffer(STAGE_CS, visible_ambient_voxels_uav_no, 0);
        d3d::set_rwbuffer(STAGE_CS, voxelsWeightHistogram_uav_no, 0);
        d3d::set_buffer(STAGE_CS, frustum_visible_ambient_voxels_count_const_no, 0);
        d3d::set_buffer(STAGE_CS, prev_probability_const_no, 0);
      }

      {
        const int nextProbability = 1 - currentProbability;

        static int prev_probability_const_no = ShaderGlobal::get_slot_by_name("ssgi_change_probability_cs_prev_probability_const_no");
        static int next_probability_uav_no = ShaderGlobal::get_slot_by_name("ssgi_change_probability_cs_next_probability_uav_no");
        static int dispatchIndirectBuffer_uav_no =
          ShaderGlobal::get_slot_by_name("ssgi_change_probability_cs_dispatchIndirectBuffer_uav_no");
        static int voxelsWeightHistogram_uav_no =
          ShaderGlobal::get_slot_by_name("ssgi_change_probability_cs_voxelsWeightHistogram_uav_no");

        d3d::set_rwbuffer(STAGE_CS, next_probability_uav_no, voxelChoiceProbability[nextProbability].get());
        d3d::set_rwbuffer(STAGE_CS, dispatchIndirectBuffer_uav_no, common.visibleAmbientVoxelsIndirect.get());
        d3d::resource_barrier({voxelsWeightHistogram.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
        d3d::set_rwbuffer(STAGE_CS, voxelsWeightHistogram_uav_no, voxelsWeightHistogram.get());
        d3d::set_buffer(STAGE_CS, prev_probability_const_no, voxelChoiceProbability[currentProbability].get());

#if GI3D_VERBOSE_DEBUG
        // debug
        {
          uint bins[MAX_BINS * 2 + 2] = {0};
          load_data(bins, voxelsWeightHistogram.get(), sizeof(bins));

          debug("histogram, all passed = %d: initialize = %d, bin0: passed = %d(from %d), bin1: passed = %d(from %d)",
            bins[2 * MAX_BINS], bins[INITIALIZE_VOXELS_INDEX], bins[0], bins[MAX_BINS + 0], bins[1], bins[MAX_BINS + 1]);
        }
#endif

        common.ssgi_change_probability_cs->dispatch(1, 1, 1);
        d3d::set_rwbuffer(STAGE_CS, next_probability_uav_no, 0);
        d3d::set_rwbuffer(STAGE_CS, dispatchIndirectBuffer_uav_no, 0);
        d3d::set_rwbuffer(STAGE_CS, voxelsWeightHistogram_uav_no, 0);
        d3d::set_buffer(STAGE_CS, prev_probability_const_no, 0);
        d3d::resource_barrier({common.visibleAmbientVoxelsIndirect.get(), RB_RO_INDEX_BUFFER});
        currentProbability = nextProbability;
#if GI3D_VERBOSE_DEBUG
        uint bins[5 * 3] = {0};
        load_data(bins, common.visibleAmbientVoxelsIndirect.get(), sizeof(bins));
        debug("dispatch indirect for temporal copy (%d %d %d), re-light (%d %d %d), combine (%d %d %d)"
              "inital light (%d %d %d)(planes %d %d %d)",
          bins[3 * 0 + 0], bins[3 * 0 + 1], bins[3 * 0 + 2], bins[3 * 1 + 0], bins[3 * 1 + 1], bins[3 * 1 + 2], bins[3 * 2 + 0],
          bins[3 * 2 + 1], bins[3 * 2 + 2], bins[3 * 3 + 0], bins[3 * 3 + 1], bins[3 * 3 + 2], bins[3 * 4 + 0], bins[3 * 4 + 1],
          bins[3 * 4 + 2]);
#endif
      }
    }

    {
      TIME_D3D_PROFILE(temporal_ambient_voxels_cs);
      if (cascadeId != CASCADES_3D - 1)
      {
        // we need to insert barrier, as we have used RB_NONE previously
        d3d::resource_barrier({common.cube.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      }
      d3d::resource_barrier({voxelsWeightHistogram.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
      d3d::resource_barrier({common.frustumVisiblePointVoxels.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

      static int voxelsWeightHistogram_const_no =
        ShaderGlobal::get_slot_by_name("temporal_ambient_voxels_cs_voxelsWeightHistogram_const_no");
      static int visible_ambient_voxels_uav_no =
        ShaderGlobal::get_slot_by_name("temporal_ambient_voxels_cs_visible_ambient_voxels_uav_no");
      static int visible_ambient_voxels_walls_planes_uav_no =
        ShaderGlobal::get_slot_by_name("temporal_ambient_voxels_cs_visible_ambient_voxels_walls_planes_uav_no");

      d3d::set_buffer(STAGE_CS, voxelsWeightHistogram_const_no, voxelsWeightHistogram.get());
      d3d::set_rwbuffer(STAGE_CS, visible_ambient_voxels_uav_no, common.selectedAmbientVoxels.get());
      d3d::set_rwbuffer(STAGE_CS, visible_ambient_voxels_walls_planes_uav_no, common.selectedAmbientVoxelsPlanes.get());
      common.temporal_ambient_voxels_cs->dispatch_indirect(common.visibleAmbientVoxelsIndirect.get(), 0 * 12);
      d3d::set_buffer(STAGE_CS, voxelsWeightHistogram_const_no, 0);
      d3d::set_rwbuffer(STAGE_CS, visible_ambient_voxels_walls_planes_uav_no, 0);
      d3d::set_rwbuffer(STAGE_CS, visible_ambient_voxels_uav_no, 0);
    }

    {
      TIME_D3D_PROFILE(lit_voxels_cs);
      static int traceResults_uav_no = ShaderGlobal::get_slot_by_name("light_ss_ambient_voxels_cs_traceResults_uav_no");
      static int visible_ambient_voxels_walls_planes_const_no =
        ShaderGlobal::get_slot_by_name("light_ss_ambient_voxels_cs_visible_ambient_voxels_walls_planes_const_no");

      d3d::set_rwbuffer(STAGE_CS, traceResults_uav_no, common.traceRayResults.get());
      d3d::resource_barrier({common.selectedAmbientVoxelsPlanes.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
      d3d::set_buffer(STAGE_CS, visible_ambient_voxels_walls_planes_const_no, common.selectedAmbientVoxelsPlanes.get());
#if D3D_HAS_RAY_TRACING
      if (common.quality == RAYTRACING)
      {
        d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, common.rt.sceneTop);
      }
#endif
      common.light_ss_ambient_voxels_cs->dispatch_indirect(common.visibleAmbientVoxelsIndirect.get(), 1 * 12);
#if D3D_HAS_RAY_TRACING
      if (common.quality == RAYTRACING)
      {
        d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, 0);
      }
#endif
      d3d::set_buffer(STAGE_CS, visible_ambient_voxels_walls_planes_const_no, 0);
      d3d::set_rwbuffer(STAGE_CS, traceResults_uav_no, 0);
    }

    {
      if (common.calc_initial_ambient_walls_dist_cs)
      {
        TIME_D3D_PROFILE(initial_voxels_planes_cs);
        static int visible_ambient_voxels_walls_planes_uav_no =
          ShaderGlobal::get_slot_by_name("calc_initial_ambient_walls_dist_cs_visible_ambient_voxels_walls_planes_uav_no");

        d3d::set_rwbuffer(STAGE_CS, visible_ambient_voxels_walls_planes_uav_no, common.selectedAmbientVoxelsPlanes.get());
        common.calc_initial_ambient_walls_dist_cs->dispatch_indirect(common.visibleAmbientVoxelsIndirect.get(), 4 * 12);
        d3d::set_rwbuffer(STAGE_CS, visible_ambient_voxels_walls_planes_uav_no, 0);
        d3d::resource_barrier({common.selectedAmbientVoxelsPlanes.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
      }

      static int gi_ambient_volmap_init_uav_no = ShaderGlobal::get_slot_by_name("gi_ambient_volmap_init_uav_no");
      static int ssgi_ambient_volmap_temporal_uav_no = ShaderGlobal::get_slot_by_name("ssgi_ambient_volmap_temporal_uav_no");

      d3d::set_rwtex(STAGE_CS, gi_ambient_volmap_init_uav_no, common.cube.getVolTex(), 0, 0);
      d3d::set_rwtex(STAGE_CS, ssgi_ambient_volmap_temporal_uav_no, common.ssgiTemporalWeight.getVolTex(), 0, 0);

      {
        TIME_D3D_PROFILE(initial_lit_voxels_cs);
        static int visible_ambient_voxels_walls_planes_const_no =
          ShaderGlobal::get_slot_by_name("light_initial_ambient_voxels_cs_visible_ambient_voxels_walls_planes_const_no");

        d3d::set_buffer(STAGE_CS, visible_ambient_voxels_walls_planes_const_no, common.selectedAmbientVoxelsPlanes.get());
#if D3D_HAS_RAY_TRACING
        if (common.quality == RAYTRACING)
        {
          d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, common.rt.sceneTop);
        }
#endif
        common.light_initial_ambient_voxels_cs->dispatch_indirect(common.visibleAmbientVoxelsIndirect.get(), 3 * 12);
#if D3D_HAS_RAY_TRACING
        if (common.quality == RAYTRACING)
        {
          d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, 0);
        }
#endif
        d3d::set_buffer(STAGE_CS, visible_ambient_voxels_walls_planes_const_no, 0);
      }

      {
        TIME_D3D_PROFILE(combine_lit_voxels_cs);
        d3d::resource_barrier({common.cube.getVolTex(), RB_NONE, 0, 0}); // we never write to same probes in different selection
                                                                         // (initial or main)
        d3d::resource_barrier({common.ssgiTemporalWeight.getVolTex(), RB_NONE, 0, 0});
        d3d::resource_barrier({common.selectedAmbientVoxels.get(), RB_RO_SRV | RB_STAGE_COMPUTE});

        static int visible_ambient_voxels_const_no =
          ShaderGlobal::get_slot_by_name("light_ss_combine_ambient_voxels_cs_visible_ambient_voxels_const_no");
        static int visible_ambient_voxels_trace_results_const_no =
          ShaderGlobal::get_slot_by_name("light_ss_combine_ambient_voxels_cs_visible_ambient_voxels_trace_results_const_no");

        d3d::set_buffer(STAGE_CS, visible_ambient_voxels_const_no, common.selectedAmbientVoxels.get());
        d3d::resource_barrier({common.traceRayResults.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
        d3d::set_buffer(STAGE_CS, visible_ambient_voxels_trace_results_const_no, common.traceRayResults.get());
        common.light_ss_combine_ambient_voxels_cs->dispatch_indirect(common.visibleAmbientVoxelsIndirect.get(), 2 * 12);
        d3d::set_buffer(STAGE_CS, visible_ambient_voxels_trace_results_const_no, 0);
        d3d::set_buffer(STAGE_CS, visible_ambient_voxels_const_no, 0);
      }
      // we never write to same probes in different cascades
      if (cascadeId == 0) // we render last->0, so in the end we need to insert barrier
      {
        d3d::resource_barrier({common.ssgiTemporalWeight.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
        d3d::resource_barrier({common.cube.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
      }
      else
      {
        // we never write to same probes in different cascades, so set NONE barrier
        d3d::resource_barrier({common.cube.getVolTex(), RB_NONE, 0, 0});
        d3d::resource_barrier({common.ssgiTemporalWeight.getVolTex(), RB_NONE, 0, 0});
      }

      d3d::set_rwtex(STAGE_CS, gi_ambient_volmap_init_uav_no, 0, 0, 0);
      d3d::set_rwtex(STAGE_CS, ssgi_ambient_volmap_temporal_uav_no, 0, 0, 0);
    }
  }

#if GI3D_VERBOSE_DEBUG
  // debug
  float probability[MAX_BINS] = {-1};
  load_data(probability, voxelChoiceProbability[currentProbability].get(), sizeof(probability));

  debug("prob = %g %g", probability[0], probability[1]);
#endif
}

void GI3D::afterReset()
{
  common.afterReset();
  afterResetScene();
  invalidateGiCascades();
  scene25D.invalidate();
  irradiance25D.invalidate();
  invalidateWindows();
  invalidateWalls();
}

static int ssgi_current_frame = 0;

void GI3D::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix4_vec4 &glob_tm)
{
  ShaderGlobal::set_int(gi_qualityVarId, common.quality);
  if (!common.cube)
    return;
  TIME_D3D_PROFILE(dagi);
  setVars();
  ShaderGlobal::set_int(ssgi_current_frameVarId, (++ssgi_current_frame) & 0xFFFF);
  d3d::settm(TM_WORLD, TMatrix::IDENT);

  bool resetted = false;
  for (int i = volmap.size() - 1; i >= 0; --i)
  {
    if (volmap[i].resetted)
    {
      volmap[i].updateOrigin(mul(volmap[i].toroidalOrigin, volmap[i].getResolution()), common, 0, getSceneBox(i + 1));
      resetted = true;
    }
  }
  if (resetted)
    updateVolmapCB();
  for (int i = volmap.size() - 1; i >= 0; --i)
    volmap[i].render(common, reinterpret_cast<mat44f_cref>(glob_tm));
  debugLastGTM = reinterpret_cast<mat44f_cref>(glob_tm);

  // volmap[0].copyFromNext(common);
  // move to different

  if (debug_inline_rt.get())
  {
    DebugInlineRt(view_tm, proj_tm);
  }
}

void GI3D::invalidateGiCascades()
{
  for (auto &v : volmap)
  {
    v.toroidalOrigin += IPoint3(-1000, -10000, 100000);
    v.resetted = true;
    v.fillBoxParams();
  }
}

void GI3D::invalidateDeferred()
{
  scene25D.invalidate();
  irradiance25D.invalidate();
  invalidateVoxelScene();
  invalidateGiCascades();
}

void GI3D::invalidateEnvi(bool force)
{
  common.enviCubeValid = false;
  if (force)
  {
    // scene25D.invalidate();//scene should not change if environment has changed
    irradiance25D.invalidate();
    invalidateVoxelScene();
    invalidateGiCascades();
  }
  if (!force && common.ssgiTemporalWeight.getVolTex())
  {
    // todo: mark just all voxels that are not completely invalid - semi-valid (1/255, for example);
    float val[4] = {4.f / 255.f, 4.f / 255.f, 4.f / 255.f, 4.f / 255.f};
    d3d::clear_rwtexf(common.ssgiTemporalWeight.getVolTex(), val, 0, 0);
    d3d::resource_barrier({common.ssgiTemporalWeight.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    // clear_rw is not working on XB1, do something like
    // TextureInfo ti;
    // ssgiTemporalWeight.getVolTex()->getinfo(ti);
    // d3d::set_rwtex(STAGE_CS, 7, ssgiTemporalWeight.getVolTex(),0,0);
    // ssgi_full_clear_temporal_volmap_cs->dispatch((ti.w+3)/4,(ti.h+3)/4,(ti.d+3)/4);
  }
}

void GI3D::debugScene25DVoxelsRayCast(dagi25d::SceneDebugType debug_scene) { scene25D.debugRayCast(debug_scene); }

void GI3D::debugVolmap25D(dagi25d::IrradianceDebugType type)
{
  if (!debugVolmapDrawIndirect)
    initDebug();
  irradiance25D.drawDebug(type);
}

void GI3D::DebugInlineRt(const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  // enable debug visualizer with: `render.debug_inline_rt 1`
  // show debug texture with: `render.show_tex debugInlineRtTarget`
  // change debug visualization from voxel color to ray distance: `render.shaderVar debug_inline_rt_raydist 1`

  TIME_D3D_PROFILE(DebugInlineRt);
  struct ConstData
  {
    Point4 pos;
    float minMax[4];
    float zn_zfar[4];
    float iResolution[2];
  };
  if (!debugInlineRtConstants)
  {
    debugInlineRtConstants = dag::buffers::create_one_frame_cb((sizeof(ConstData) + 15) / 16, "debug_inline_rt_constants");
  }
  if (!debugInlineRtTarget)
  {
    int w, h;
    d3d::get_render_target_size(w, h, NULL);
    debugInlineRtTarget = dag::create_tex(NULL, w, h, TEXCF_UNORDERED, 1, "debugInlineRtTarget");
  }

  if (!debug_inline_rt_cs)
    debug_inline_rt_cs.reset(new_compute_shader("debug_inline_rt_cs"));

  ConstData *cData = nullptr;
  if (!debugInlineRtConstants->lock(0, 0, (void **)&cData, VBLOCK_DISCARD | VBLOCK_WRITEONLY) || !cData)
    return;

  TMatrix viewRot = view_tm;
  Point3 worldPos = inverse(viewRot).getcol(3);
  viewRot.setcol(3, 0.f, 0.f, 0.f);

  TMatrix4 viewRotProjInv = inverse44(TMatrix4(viewRot) * proj_tm);

  static int view_vecLTVarId = get_shader_variable_id("view_vecLT");
  static int view_vecRTVarId = get_shader_variable_id("view_vecRT");
  static int view_vecLBVarId = get_shader_variable_id("view_vecLB");
  static int view_vecRBVarId = get_shader_variable_id("view_vecRB");

  Point3 viewVectLT = Point3(-1.f, 1.f, 1.f) * viewRotProjInv;
  Point3 viewVectRT = Point3(1.f, 1.f, 1.f) * viewRotProjInv;
  Point3 viewVectLB = Point3(-1.f, -1.f, 1.f) * viewRotProjInv;
  Point3 viewVectRB = Point3(1.f, -1.f, 1.f) * viewRotProjInv;

  ShaderGlobal::set_color4(view_vecLTVarId, Color4(&viewVectLT.x));
  ShaderGlobal::set_color4(view_vecRTVarId, Color4(&viewVectRT.x));
  ShaderGlobal::set_color4(view_vecLBVarId, Color4(&viewVectLB.x));
  ShaderGlobal::set_color4(view_vecRBVarId, Color4(&viewVectRB.x));

  cData->pos = Point4::xyz0(worldPos);
  cData->minMax[0] = 0.01f;
  cData->minMax[1] = 40.0f;
  Driver3dPerspective p;
  p.zn = 0.5f;
  p.zf = 100;
  d3d::getpersp(p); // can fail
  cData->zn_zfar[0] = p.zn;
  cData->zn_zfar[1] = p.zf;
  cData->zn_zfar[2] = p.zf / p.zn - 1;

  TextureInfo info;
  debugInlineRtTarget->getinfo(info);
  cData->iResolution[0] = 1.0f / info.w;
  cData->iResolution[1] = 1.0f / info.h;
  debugInlineRtConstants->unlock();

  static int outputImage_uav_no = ShaderGlobal::get_slot_by_name("debug_inline_rt_cs_outputImage_uav_no");
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, outputImage_uav_no, VALUE, 0, 0), debugInlineRtTarget.getTex2D());
#if D3D_HAS_RAY_TRACING
  STATE_GUARD_NULLPTR(d3d::set_top_acceleration_structure(STAGE_CS, TOP_ACC_STRUCTURE_REGISTER, VALUE), common.rt.sceneTop);
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, DEAD_PROBES_REGISTER, VALUE, 0, 0), common.rt.deadProbes.getArrayTex());
#endif
  debug_inline_rt_cs->dispatch((info.w - 1) / 8 + 1, (info.h - 1) / 8 + 1, 1);
}

void GI3D::invalidate(const BBox3 &box, const TMatrix &tm, const BBox3 &approx)
{
  const int cascade = sceneCascades.size() - 1;
  const Point3 voxelSize = sceneVoxelSize * sceneCascades[cascade].scale;
  const Point3 bmin = sceneCascades[cascade].origin;
  const Point3 bmax = bmin + mul(voxelSize, Point3(IPoint3::xzy(VOXEL_RESOLUTION)));
  const BBox3 cascade_bbox(bmin, bmax);

  if (!(approx & cascade_bbox))
  {
    return;
  }

  mat44f mat;
  v_mat44_make_from_43cu_unsafe(mat, tm.m[0]);

  bbox3f bbox;
  v_bbox3_init(bbox, mat, v_ldu_bbox3(box));
  invalidBoxes.push_back(bbox);
}
