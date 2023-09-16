#include <rendInst/rendInstGenGpuObjects.h>
#include <rendInst/rotation_palette_consts.hlsli>
#include "riGen/riGenRender.h"
#include "riGen/riGenExtraRender.h"
#include "riGen/riGenExtra.h"
#include "riGen/riGenData.h"
#include "riGen/genObjUtil.h"
#include "riGen/riUtil.h"
#include "riGen/riRotationPalette.h"
#include "riGen/riShaderConstBuffers.h"
#include "riGen/riRotationPalette.h"
#include "debug/collisionVisualization.h"

#include <3d/dag_quadIndexBuffer.h>
#include <stdio.h>
#include <gameRes/dag_gameResources.h>
#include <shaders/dag_shaderBlock.h>
#include <fx/dag_leavesWind.h>
#include <3d/dag_render.h>
#include <3d/dag_drv3d_platform.h>
#include <math/dag_frustum.h>
#include <perfMon/dag_statDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_adjpow2.h>
#include <render/dxtcompress.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_TMatrix4more.h>
#include <util/dag_stlqsort.h>
#include <memory/dag_framemem.h>
#include <3d/dag_ringDynBuf.h>
#include <3d/dag_drv3dCmd.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <scene/dag_occlusion.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/array.h>
#include <gpuObjects/gpuObjects.h>
#include <shaders/dag_computeShaders.h>
#include <util/dag_convar.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_sbufferIDHolder.h>
#include <render/debugMesh.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_gpuConfig.h>
#include <math/dag_half.h>
#include <3d/dag_indirectDrawcallsBuffer.h>
#include <3d/dag_dynLinearAllocBuffer.h>
#include <util/dag_console.h>

#include <debug/dag_debug3d.h>
#include <3d/dag_drv3dReset.h>
#include <util/dag_console.h>


#define LOGLEVEL_DEBUG _MAKE4C('RGEN')

#define MIN_RI_SHADOW_SUN_Y         0.3f
#define CLIPMAP_SHADOW_SPHERE_SCALE 1.5f
#define SHADOW_APPEAR_PART          0.1
#define IMPOSTORS_CLEAR_COL         0x00
#define IMPOSTOR_NOAUTO_MIPS        5
#define LOD0_DISSOLVE_RANGE         0.0f
#define LOD1_DISSOLVE_RANGE         6.0f
#define TOTAL_CROSS_DISSOLVE_DIST   (LOD0_DISSOLVE_RANGE + LOD1_DISSOLVE_RANGE)
// todo make custom mipmap generation for some of last mipmaps - weighted filter, weights only non-clipped pixels

#define DEBUG_RI 0

struct SortByY
{
  bool operator()(const IPoint2 &a, const IPoint2 &b) const { return a.y < b.y; }
};

struct SortByDescY
{
  bool operator()(const IPoint2 &a, const IPoint2 &b) const { return a.y > b.y; }
};

bool check_occluders = true, add_occluders = true;

int globalRendinstRenderTypeVarId = -1;

int globalTranspVarId = -1;

static int worldViewPosVarId = -1;

static int drawOrderVarId = -1;
int useRiGpuInstancingVarId = -1;

static int invLod0RangeVarId = -1;

static int impostorShadowXVarId = -1;
static int impostorShadowYVarId = -1;
static int impostorShadowZVarId = -1;
static int impostorShadowVarId = -1;

static int impostorLastMipSize = 1;

static int isRenderByCellsVarId = -1;

static int gpuObjectVarId = -1;

int perinstBuffNo = -1;
int instanceBuffNo = -1;

// TODO: elemSizeMul should be 2, after half->float migration, const it when RENDINST_FLOAT_POS will be removed
static constexpr uint32_t ri_cellsVB_elem_size_mul = RENDINST_FLOAT_POS ? (RENDER_ELEM_SIZE_UNPACKED / RENDER_ELEM_SIZE_PACKED) : 1;

static uint32_t ri_cellsVB_min_allocation_size = (1u << 20) * ri_cellsVB_elem_size_mul;
static uint32_t ri_cellsVB_geom_growth_heap_size_threshold = (8u << 20) * ri_cellsVB_elem_size_mul;
static uint32_t ri_cellsVB_arithmetic_grow_size = (2u << 20) * ri_cellsVB_elem_size_mul;
static uint32_t ri_cellsVB_page_size_mask = (1 << 16) - 1;
static uint32_t ri_cellsVB_max_size = (64 << 20) * ri_cellsVB_elem_size_mul;

static struct
{
  float brightness = .5;
  float falloffStart = .25;
  float falloffStop = 1.;
  bool enabled = false;
} treeCrownTransmittance;

struct GpuObjectsEntry
{
  String name;
  int grid_tile, grid_size;
  float cell_size;
  gpu_objects::PlacingParameters parameters;
};
eastl::unique_ptr<gpu_objects::GpuObjects> gpu_objects_mgr;
eastl::vector<GpuObjectsEntry> gpu_objects_to_add;

static inline int remap_per_instance_lod(int l)
{
  if (l <= RiGenVisibility::PI_LAST_MESH_LOD)
    return l;
  return RiGenVisibility::PI_IMPOSTOR_LOD;
};

static inline int remap_per_instance_lod_inv(int l)
{
  if (l <= RiGenVisibility::PI_LAST_MESH_LOD)
    return l;
  return l >= RiGenVisibility::PI_IMPOSTOR_LOD ? l - 2 : RiGenVisibility::PI_LAST_MESH_LOD;
};

RenderStateContext::RenderStateContext() { d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB())); }

namespace rendinstgenrender
{
static uint32_t MIN_DYNAMIC_IMPOSTOR_TEX_SIZE = 32;
static uint32_t MAX_DYNAMIC_IMPOSTOR_TEX_SIZE = 256;

#define IMPOSTOR_MAX_ASPECT_RATIO 2

static uint32_t packed_format = TEXFMT_A16B16G16R16S;
static uint32_t unpacked_format = TEXFMT_A32B32G32R32F;

bool use_color_padding = true;
Point3_vec4 dir_from_sun(0, -1, 0);

bool avoidStaticShadowRecalc = false;
bool per_instance_visibility = false;
bool use_cross_dissolve = true;
bool per_instance_visibility_for_everyone = true; // todo: change in tank to true and in plane to false
bool per_instance_front_to_back = true;
bool useConditionalRendering = false; // obsolete - not working
shaders::UniqueOverrideStateId afterDepthPrepassOverride;

static bool vertical_billboards = false;
auto compIPoint2 = [](const IPoint2 &a, const IPoint2 &b) { return a.y < b.y || a.x < b.x; };
static eastl::vector_map<IPoint2, UniqueTex, decltype(compIPoint2)> impostorDepthTextures(compIPoint2);
static bool ri_extra_render_enabled = true;
static bool use_bbox_in_cbuffer = false;
static bool use_tree_lod0_offset = true;

static VDECL rendinstDepthOnlyVDECL = BAD_VDECL;
static int build_normal_type = -2;
int normal_type = -1;

IndirectDrawcallsBuffer<DrawIndexedIndirectArgs> indirectDrawCalls("RI_drawcalls");
DynLinearAllocBuffer<rendinst::PerInstanceParameters> indirectDrawCallIds("ri_drawcall_ids");

Vbuffer *oneInstanceTmVb = NULL;
static constexpr uint32_t ROTATION_PALETTE_TM_VB_STRIDE = 3; // 3 float4
Vbuffer *rotationPaletteTmVb = nullptr;
float globalDistMul = 1;
float globalCullDistMul = 1;
float settingsDistMul = 1;
float settingsMinCullDistMul = 0.f;
float lodsShiftDistMul = 1;
float additionalRiExtraLodDistMul = 1;
float riExtraLodsShiftDistMul = 1.f;
float riExtraMulScale = 1;
int globalFrameBlockId = -1;
int rendinstSceneBlockId = -1;
int rendinstSceneTransBlockId = -1;
int rendinstDepthSceneBlockId = -1;
bool forceImpostors = false;
bool useCellSbuffer = false;


static bool impostor_vb_replication = false;

#if !D3D_HAS_QUADS
static const int impostor_vb_replication_count = 16300; // 16-bit indices, 4 vertices.
#endif

int instancingTypeVarId = -1;
CoordType cur_coord_type = COORD_TYPE_TM;

void setCoordType(CoordType type)
{
  cur_coord_type = type;
  ShaderGlobal::set_int_fast(instancingTypeVarId, cur_coord_type);
}

void setApexInstancing() { ShaderGlobal::set_int_fast(instancingTypeVarId, 5); }

int rendinstRenderPassVarId = -1;
int rendinstShadowTexVarId = -1;
int render_cross_dissolved_varId = -1;

int gpuObjectDecalVarId = -1;
int useRiTrackdirtOffsetVarId = -1;
int disable_rendinst_alpha_for_normal_pass_with_zprepassVarId = -1;

int dynamic_impostor_texture_const_no = -1;
float rendinst_ao_mul = 2.0f;
// mip map build

bool use_ri_depth_prepass = true;
static bool depth_prepass_for_cells = true;
static bool depth_prepass_for_impostors = false;

void useRiDepthPrepass(bool use) { use_ri_depth_prepass = use; }

void disableRendinstAlphaForNormalPassWithZPrepass()
{
  if (use_ri_depth_prepass)
    ShaderGlobal::set_int(disable_rendinst_alpha_for_normal_pass_with_zprepassVarId, 1);
}

void restoreRendinstAlphaForNormalPassWithZPrepass()
{
  if (use_ri_depth_prepass)
    ShaderGlobal::set_int(disable_rendinst_alpha_for_normal_pass_with_zprepassVarId, 0);
}

void useRiCellsDepthPrepass(bool use) { depth_prepass_for_cells = use; }

void useImpostorDepthPrepass(bool use) { depth_prepass_for_impostors = use; }
} // namespace rendinstgenrender

static __forceinline void get_local_up_left(const Point3 &imp_fwd, const Point3 &imp_up, const Point3 &cam_fwd, Point3 &out_up,
  Point3 &out_left)
{
  Point3 fwd = cam_fwd * imp_fwd > 0 ? cam_fwd : -cam_fwd;
  out_left = normalize(imp_up % fwd);
  out_up = normalize(fwd % out_left);
}

inline int get_last_mip_idx(Texture *tex, int dest_size)
{
  TextureInfo ti;
  tex->getinfo(ti);
  int src = min(ti.w, ti.h);
  return min(tex->level_count() - 1, int(src > dest_size ? get_log2i(src / dest_size) : 0));
}

#include "rendinstGenImpostor.inc.cpp"

namespace rendinstgenrender
{

void cell_set_encoded_bbox(RiShaderConstBuffers &cb, vec4f origin, float grid2worldcellSz, float ht)
{
  vec4f cbbox[2];
  cbbox[0] = origin;
  cbbox[1] = v_make_vec4f(grid2worldcellSz, ht, grid2worldcellSz, ENCODED_RENDINST_RESCALE);

  if (!RENDINST_FLOAT_POS && packed_format == TEXFMT_A16B16G16R16)
  {
    // pos decoding is c1*shrt + c0. If we remap pos so pos is ushrt = shrt + 32768/65535, than decode is c1*(pos*65535/32768 - 1) +c0
    // so pos decoding is (c1*65535/32768)*usht + (c0-c1)
    // scale decoding is just (65535/32768.*scale-1)*ENCODED_RENDINST_RESCALE = (ENCODED_RENDINST_RESCALE*65535/32768)*scale -
    // ENCODED_RENDINST_RESCALE
    cbbox[0] = v_sub(cbbox[0], cbbox[1]);
    cbbox[0] = v_sel(cbbox[0], v_splats(-ENCODED_RENDINST_RESCALE), V_CI_MASK0001);
    cbbox[1] = v_mul(cbbox[1], v_splats(65535.f / 32768.f));
  }
  else
  {
    cbbox[0] = v_and(cbbox[0], V_CI_MASK1110);
  }
  cb.setBBox(cbbox);
}

}; // namespace rendinstgenrender

void rendinst::setTextureMinMipWidth(int textureSize, int impostorSize, float textureSizeMul, float impostorSizeMul)
{
  if (!RendInstGenData::renderResRequired)
    return;

  static int lastTexSize = -1;
  static int lastImpSize = -1;

  if (textureSize > 0)
    lastTexSize = textureSize;
  else
    textureSize = lastTexSize;

  if (impostorSize > 0)
    lastImpSize = impostorSize;
  else
    impostorSize = lastImpSize;

  if (textureSizeMul >= 0.f)
    textureSize *= textureSizeMul;

  if (impostorSizeMul >= 0.f)
    impostorSize *= impostorSizeMul;

  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      rgl->rtData->setTextureMinMipWidth(max(textureSize, 1), max(impostorSize, 1));

  debug("rendinst::setTextureMinMipWidth: %d, %d", textureSize, impostorSize);
}

namespace rendinstgenrender
{

void init_depth_VDECL()
{
  static VSDTYPE vsdDepthOnly[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT3), VSD_END};
  if (rendinstgenrender::rendinstDepthOnlyVDECL != BAD_VDECL)
    d3d::delete_vdecl(rendinstgenrender::rendinstDepthOnlyVDECL);

  rendinstgenrender::rendinstDepthOnlyVDECL = d3d::create_vdecl(vsdDepthOnly);
  build_normal_type = normal_type;
}

void close_depth_VDECL()
{
  if (rendinstDepthOnlyVDECL != BAD_VDECL)
    d3d::delete_vdecl(rendinstDepthOnlyVDECL);
  rendinstDepthOnlyVDECL = BAD_VDECL;
  build_normal_type = -2;
}

}; // namespace rendinstgenrender

static void allocatePaletteVB()
{
  // The size of the palette is determined later than allocateRendInstVBs is called
  if (!rendinstgenrender::rotationPaletteTmVb)
  {
    int maxCount = (1 << PALETTE_ID_BIT_COUNT) + 1; // +1 for identity rotation
    int flags = SBCF_DYNAMIC | SBCF_MAYBELOST | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE;
    int format = TEXFMT_A32B32G32R32F;
    if (rendinstgenrender::useCellSbuffer)
    {
      flags |= SBCF_MISC_STRUCTURED;
      format = 0;
    }
    rendinstgenrender::rotationPaletteTmVb = d3d::create_sbuffer(16, maxCount * 3, flags, format, "rotationPaletteTmVb");
    d3d_err(rendinstgenrender::rotationPaletteTmVb);
  }
}

static void allocateRendInstVBs()
{
  if (rendinstgenrender::oneInstanceTmVb) // already created
    return;
  rendinstgenrender::init_instances_tb();
  // Create VB to render one instance of rendinst using IB converted to VB.
  if (rendinstgenrender::useCellSbuffer)
    rendinstgenrender::oneInstanceTmVb = d3d_buffers::create_persistent_sr_structured(sizeof(TMatrix4), 1, "onInstanceTmVb");
  else
    rendinstgenrender::oneInstanceTmVb = d3d_buffers::create_persistent_sr_tbuf(4, TEXFMT_A32B32G32R32F, "onInstanceTmVb");

  d3d_err(rendinstgenrender::oneInstanceTmVb);

  G_STATIC_ASSERT(sizeof(rendinstgenrender::RiShaderConstBuffers) <= 256); // we just keep it smaller than 256 bytes for performance
                                                                           // reasons
  rendinstgenrender::perDrawCB =
    d3d_buffers::create_one_frame_cb(dag::buffers::cb_struct_reg_count<rendinstgenrender::RiShaderConstBuffers>(), "perDrawInstCB");
  d3d_err(rendinstgenrender::perDrawCB);
  uint32_t perDrawElems = rendinst::riex_max_type();
  const int structSize = sizeof(Point4);
  int elements = (sizeof(rendinstgenrender::RiShaderConstBuffers) / sizeof(Point4)) * perDrawElems;
  // do not create buffer view if hardware is unable to use it
  bool useStructuredBind;
  if (rendinst::perDrawInstanceDataBufferType == 1)
  {
    useStructuredBind = false;
    if (d3d::get_driver_desc().issues.hasSmallSampledBuffers)
      elements = min(elements, 65536); // The minimum guaranteed supported Buffer on Vulkan.
  }
  else if (rendinst::perDrawInstanceDataBufferType == 2)
    useStructuredBind = true;
  else
    useStructuredBind = d3d::get_driver_desc().issues.hasSmallSampledBuffers;

  debug("perDrawInstanceData %d (%d)", elements, (int)useStructuredBind);

  rendinst::perDrawData =
    dag::create_sbuffer(structSize, elements, SBCF_MAYBELOST | SBCF_BIND_SHADER_RES | (useStructuredBind ? SBCF_MISC_STRUCTURED : 0),
      useStructuredBind ? 0 : TEXFMT_A32B32G32R32F, "perDrawInstanceData");

#if !D3D_HAS_QUADS
  index_buffer::init_quads_16bit();
#endif
}

static void fillRendInstVBs();

static void initRendInstVBs()
{
  allocateRendInstVBs();
  fillRendInstVBs();
}

static void fillRendInstVBs()
{
  if (!rendinstgenrender::oneInstanceTmVb)
    return;

  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);

  float src[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  float *data;
  if (rendinstgenrender::oneInstanceTmVb->lock(0, 0, (void **)&data, VBLOCK_WRITEONLY) && data)
  {
    memcpy(data, src, sizeof(src));
    rendinstgenrender::oneInstanceTmVb->unlock();
  }

  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
}


void RendInstGenData::termRenderGlobals()
{
  rendinst::closeImpostorShadowTempTex();

  if (!RendInstGenData::renderResRequired)
    return;

  gpu_objects_mgr.reset();
  shaders::overrides::destroy(rendinstgenrender::afterDepthPrepassOverride);
  rendinstgenrender::close_instances_tb();
  rendinst::perDrawData.close();
  del_d3dres(rendinstgenrender::perDrawCB);
  del_d3dres(rendinstgenrender::oneInstanceTmVb);
  del_d3dres(rendinstgenrender::rotationPaletteTmVb);
  rendinstgenrender::indirectDrawCalls.close();
  rendinstgenrender::indirectDrawCallIds.close();
  index_buffer::release_quads_16bit();
  rendinstgenrender::closeClipmapShadows();
  rendinstgenrender::close_depth_VDECL();
  rendinst::close_debug_collision_visualization();
  termImpostorsGlobals();
}

static __forceinline bool isTexInstancingOn() { return false; }

float cascade0Dist = -1.f;

void RendInstGenData::initRenderGlobals(bool use_color_padding, bool should_init_gpu_objects)
{
  ShaderGlobal::get_int_by_name("per_draw_instance_data_buffer_type", rendinst::perDrawInstanceDataBufferType);
  ShaderGlobal::get_int_by_name("instance_positions_buffer_type", rendinst::instancePositionsBufferType);

#define GET_REG_NO(a)                                              \
  {                                                                \
    int a##VarId = get_shader_variable_id(#a);                     \
    if (a##VarId >= 0)                                             \
      rendinstgenrender::a = ShaderGlobal::get_int_fast(a##VarId); \
  }
  rendinstgenrender::settingsDistMul = clamp(::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("rendinstDistMul", 1.f),
    MIN_SETTINGS_RENDINST_DIST_MUL, MAX_SETTINGS_RENDINST_DIST_MUL);
  rendinstgenrender::globalDistMul = rendinstgenrender::settingsDistMul;
  rendinstgenrender::riExtraMulScale = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("riExtraMulScale", 1.0f);
  rendinstgenrender::lodsShiftDistMul =
    clamp(::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("lodsShiftDistMul", 1.f), 1.f, 1.3f);

  rendinstgenrender::use_color_padding =
    use_color_padding && ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("riColorPadding", true);
  debug("rendinstgenrender::use_color_padding=%d", (int)rendinstgenrender::use_color_padding);

  globalRendinstRenderTypeVarId = ::get_shader_variable_id("rendinst_render_type", true);
  // check that var is assumed or missing, to avoid using specific rendering options when they are not available
  if (!VariableMap::isVariablePresent(globalRendinstRenderTypeVarId) || ShaderGlobal::is_var_assumed(globalRendinstRenderTypeVarId))
    globalRendinstRenderTypeVarId = -1;

  debug("rendinstgenrender::globalRendinstRenderTypeVarId %s", globalRendinstRenderTypeVarId == -1 ? "assumed/missing" : "dynamic");

  bool forceUniversalRIByConfig = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("forceUniversalRIRenderingPath", false);
  // non universal RI rendering depends on base instance id
  bool forceUniversalRIByDriver = d3d::get_driver_desc().issues.hasBrokenBaseInstanceID;
  if (forceUniversalRIByConfig || forceUniversalRIByDriver)
  {
    debug("rendinstgenrender::globalRendinstRenderTypeVarId forced assumed/missing by %s %s", forceUniversalRIByConfig ? "config" : "",
      forceUniversalRIByDriver ? "driver" : "");
    globalRendinstRenderTypeVarId = -1;
  }


  globalTranspVarId = ::get_shader_glob_var_id("global_transp_r");
  useRiGpuInstancingVarId = ::get_shader_glob_var_id("use_ri_gpu_instancing", true);
  impostorShadowXVarId = ::get_shader_variable_id("impostor_shadow_x", true);
  impostorShadowYVarId = ::get_shader_variable_id("impostor_shadow_y", true);
  impostorShadowZVarId = ::get_shader_variable_id("impostor_shadow_z", true);
  impostorShadowVarId = ::get_shader_variable_id("impostor_shadow", true);
  isRenderByCellsVarId = ::get_shader_variable_id("is_render_by_cells", true);

  gpuObjectVarId = ::get_shader_variable_id("gpu_object_id", true);

  rendinstgenrender::useCellSbuffer = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("useCellSBuffer", false);
  debug("rendinst: cell buffer type <%s>", rendinstgenrender::useCellSbuffer ? "structured" : "tex");

  if (VariableMap::isGlobVariablePresent(VariableMap::getVariableId("rendinst_perinst_buff_no")) &&
      VariableMap::isGlobVariablePresent(VariableMap::getVariableId("rendinst_instance_buff_no")))
  {
    if (!ShaderGlobal::get_int_by_name("rendinst_perinst_buff_no", perinstBuffNo))
      logerr("'rendinst_perinst_buff_no' variable is missing!");
    if (!ShaderGlobal::get_int_by_name("rendinst_instance_buff_no", instanceBuffNo))
      logerr("'rendinst_instance_buff_no' variable is missing!");
  }
  else
    instanceBuffNo = 4, perinstBuffNo = 5;

  rendinstgenrender::globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");
  rendinstgenrender::rendinstSceneBlockId = ShaderGlobal::getBlockId("rendinst_scene");
  rendinstgenrender::rendinstSceneTransBlockId = ShaderGlobal::getBlockId("rendinst_scene_trans");
  if (rendinstgenrender::rendinstSceneTransBlockId < 0)
    rendinstgenrender::rendinstSceneTransBlockId = rendinstgenrender::rendinstSceneBlockId;
  rendinstgenrender::rendinstDepthSceneBlockId = ShaderGlobal::getBlockId("rendinst_depth_scene");
  if (rendinstgenrender::rendinstDepthSceneBlockId < 0)
    rendinstgenrender::rendinstDepthSceneBlockId = rendinstgenrender::rendinstSceneBlockId;

  rendinstgenrender::dynamic_impostor_texture_const_no =
    ShaderGlobal::get_int_fast(::get_shader_glob_var_id("dynamic_impostor_texture_const_no"));
  rendinstgenrender::rendinst_ao_mul = ShaderGlobal::get_real_fast(::get_shader_glob_var_id("rendinst_ao_mul"));

  invLod0RangeVarId = ::get_shader_glob_var_id("rendinst_inv_lod0_range", true);

  worldViewPosVarId = get_shader_glob_var_id("world_view_pos");
  drawOrderVarId = get_shader_glob_var_id("draw_order", true);
  rendinstgenrender::rendinstShadowTexVarId = ::get_shader_glob_var_id("rendinst_shadow_tex");
  rendinstgenrender::rendinstRenderPassVarId = ::get_shader_glob_var_id("rendinst_render_pass");
  rendinstgenrender::gpuObjectDecalVarId = ::get_shader_glob_var_id("gpu_object_decal", true);
  rendinstgenrender::useRiTrackdirtOffsetVarId = ::get_shader_glob_var_id("use_ri_trackdirt_offset", true);
  rendinstgenrender::disable_rendinst_alpha_for_normal_pass_with_zprepassVarId =
    get_shader_variable_id("disable_rendinst_alpha_for_normal_pass_with_zprepass", true);

  rendinstgenrender::instancingTypeVarId = ::get_shader_glob_var_id("instancing_type");
  rendinstgenrender::render_cross_dissolved_varId =
    ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("riCrossDissolve", true)
      ? ::get_shader_glob_var_id("rendinst_dissolve", true)
      : -1;
  rendinstgenrender::use_cross_dissolve = VariableMap::isVariablePresent(rendinstgenrender::render_cross_dissolved_varId);
  rendinstgenrender::baked_impostor_multisampleVarId = ::get_shader_variable_id("baked_impostor_multisample", true);
  rendinstgenrender::vertical_impostor_slicesVarId = ::get_shader_variable_id("vertical_impostor_slices", true);
  rendinstgenrender::use_tree_lod0_offset = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("useTreeLod0Offset", true);

  rendinstgenrender::use_bbox_in_cbuffer = ::get_shader_variable_id("useBboxInCbuffer", true) >= 0;

  rendinstgenrender::initClipmapShadows();
  rendinstgenrender::per_instance_visibility = d3d::get_driver_desc().caps.hasInstanceID;

  int impostor_vb_replicationVarId = get_shader_variable_id("impostor_vb_replication", true);
  rendinstgenrender::impostor_vb_replication = false;
#if !D3D_HAS_QUADS && _TARGET_PC_WIN                     // Separate render path with PRIM_QUADLIST, no IB support.
  if (d3d_get_gpu_cfg().primaryVendor != D3D_VENDOR_ATI) // Treat non-ATI as Nvidia to be safe on Optimus.
  {
    if (d3d::get_driver_code() == _MAKE4C('DX11') &&                // do not use replication on new API (VULKAN/DX12)
        !d3d::get_driver_desc().caps.hasConservativeRassterization) // do not use replication on new NVIDIA hardware. has nothing
                                                                    // to do with conservative, just caps for new chip
      rendinstgenrender::impostor_vb_replication = true; // Works faster on older GF and doesn't crash on GF660 with TDR in instancing.
  }
#endif
  if (!VariableMap::isGlobVariablePresent(impostor_vb_replicationVarId))
    rendinstgenrender::impostor_vb_replication = false;
  ShaderGlobal::set_int(impostor_vb_replicationVarId, rendinstgenrender::impostor_vb_replication ? 1 : 0);

  rendinstgenrender::init_depth_VDECL();

  initRendInstVBs();
  initImpostorsGlobals();

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_FUNC | shaders::OverrideState::Z_WRITE_DISABLE);
  state.zFunc = CMPF_EQUAL;
  rendinstgenrender::afterDepthPrepassOverride.reset(shaders::overrides::create(state));

  if (should_init_gpu_objects)
  {
    gpu_objects_mgr = nullptr;
    gpu_objects_mgr = eastl::make_unique<gpu_objects::GpuObjects>();
  }

  const DataBlock *cellsVBcfg = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBlockByNameEx("cellsVBcfg");

  ri_cellsVB_min_allocation_size =
    cellsVBcfg->getInt("min_allocation_size", ri_cellsVB_min_allocation_size / ri_cellsVB_elem_size_mul) * ri_cellsVB_elem_size_mul;
  ri_cellsVB_geom_growth_heap_size_threshold =
    cellsVBcfg->getInt("geom_growth_heap_size_threshold", ri_cellsVB_geom_growth_heap_size_threshold / ri_cellsVB_elem_size_mul) *
    ri_cellsVB_elem_size_mul;
  ri_cellsVB_arithmetic_grow_size =
    cellsVBcfg->getInt("arithmetic_grow_size", ri_cellsVB_arithmetic_grow_size / ri_cellsVB_elem_size_mul) * ri_cellsVB_elem_size_mul;
  ri_cellsVB_max_size = cellsVBcfg->getInt("max_size", ri_cellsVB_max_size / ri_cellsVB_elem_size_mul) * ri_cellsVB_elem_size_mul;
  ri_cellsVB_page_size_mask = cellsVBcfg->getInt("page_size_mask", ri_cellsVB_page_size_mask);
  debug("rendinst: cellsVB: alloc config - min %u Mb, geomTh %u Mb, arithGrow %u Mb, maxSize %u Mb, pageMask %X",
    ri_cellsVB_min_allocation_size >> 20, ri_cellsVB_geom_growth_heap_size_threshold >> 20, ri_cellsVB_arithmetic_grow_size >> 20,
    ri_cellsVB_max_size >> 20, ri_cellsVB_page_size_mask);
}

#define MIN_TREES_FAR_MUL 0.3177f
void RendInstGenData::RtData::setDistMul(float dist_mul, float cull_mul, bool scale_lod1, bool set_preload_dist, bool no_mul_limit,
  float impostors_far_dist_additional_mul)
{
  rendinstDistMul = dist_mul;
  impostorsFarDistAdditionalMul = rendinst::isRgLayerPrimary(layerIdx) ? impostors_far_dist_additional_mul : 1.f;
  float mulLimit = no_mul_limit ? 0.f : MIN_TREES_FAR_MUL;
  if (set_preload_dist)
    preloadDistance = max(mulLimit, cull_mul) * settingsPreloadDistance * max(impostorsFarDistAdditionalMul, 1.f);
  if (scale_lod1)
  {
    transparencyDeltaRcp = cull_mul * settingsTransparencyDeltaRcp;
    rendinstDistMulFar = cull_mul;
    rendinstDistMulFarImpostorTrees = max(mulLimit, cull_mul);
    rendinstDistMulImpostorTrees = rendinstDistMulFarImpostorTrees > 1.0f ? sqrtf(max(mulLimit, dist_mul)) : max(mulLimit, dist_mul);
  }
}

void RendInstGenData::initPaletteVb() { allocatePaletteVB(); }

void RendInstGenData::initRender(const DataBlock *level_blk)
{
  logwarn("We need to fix format to ushort, from half of short. change exporting tools");
  bool main_layer = rendinst::isRgLayerPrimary(rtData->layerIdx);
  clear_and_resize(rtData->rtPoolData, rtData->riRes.size());
  mem_set_0(rtData->rtPoolData);
  rtData->riImpTexIds.reset();
  Tab<RenderableInstanceLodsResource *> riRes;
  riRes.reserve(rtData->riRes.size() + rendinst::riExtra.size());
  for (int i = 0; i < rtData->riRes.size(); i++)
    if (rtData->riRes[i])
      riRes.push_back(rtData->riRes[i]);
  for (int i = 0; i < rendinst::riExtra.size(); i++)
    if (rendinst::riExtra[i].res)
      riRes.push_back(rendinst::riExtra[i].res);
  unitedvdata::riUnitedVdata.addRes(make_span(riRes));

  for (int i = 0; i < rtData->riRes.size(); i++)
    if (rtData->riRes[i] && rendinst::getPersistentPackType(rtData->riRes[i], 2) != 2 && rtData->riResHideMask[i] != 0xFF)
    {
      rtData->rtPoolData[i] = new rendinstgenrender::RtPoolData(rtData->riRes[i]);
      if (rtData->riRes[i]->hasImpostor())
      {
        if (rtData->riRes[i]->isBakedImpostor())
        {
          int bufferOffset = rendinstgen::get_rotation_palette_manager()->getImpostorDataBufferOffset({rtData->layerIdx, i},
            rtData->riResName[i], rtData->riRes[i]);
          rendinstgen::RotationPaletteManager::Palette palette =
            rendinstgen::get_rotation_palette_manager()->getPalette({rtData->layerIdx, i});
          uint32_t shadowAtlasSize = palette.count;
          G_ASSERT(get_impostor_texture_mgr() != nullptr);
          int preshadowAtlasMipOffset =
            get_impostor_texture_mgr() ? get_impostor_texture_mgr()->getPreferredShadowAtlasMipOffset() : 0;
          int preshadowFormat = get_impostor_texture_mgr() && get_impostor_texture_mgr()->hasBcCompression()
                                  ? TEXFMT_ATI1N | TEXCF_UPDATE_DESTINATION
                                  : TEXCF_RTARGET | TEXFMT_L8 | TEXCF_GENERATEMIPS;
          rtData->riRes[i]->prepareTextures(rtData->riResName[i], shadowAtlasSize, preshadowAtlasMipOffset, preshadowFormat);
          Tab<ShaderMaterial *> mats;
          rtData->riRes[i]->lods.back().scene->gatherUsedMat(mats);
          for (size_t j = 0; j < mats.size(); ++j)
          {
            if (!rtData->riRes[i]->setImpostorVars(mats[j], bufferOffset))
              logerr("Could not set all impostor shader variables for <%s> (material=%s, lod=%d)", rtData->riResName[i],
                mats[j]->getShaderClassName(), rtData->riRes[i]->lods.size() - 1);
          }

          // penultimate lod could also have impostor if it is transition lod
          if (rtData->riRes[i]->lods.size() > 1)
          {
            mats.clear();
            int transitionLodId = rtData->riRes[i]->lods.size() - 2;
            rtData->riRes[i]->lods[transitionLodId].scene->gatherUsedMat(mats);
            for (size_t j = 0; j < mats.size(); ++j)
            {
              if (rtData->riRes[i]->setImpostorVars(mats[j], bufferOffset))
                rtData->rtPoolData[i]->flags |= rendinstgenrender::RtPoolData::HAS_TRANSITION_LOD;
            }
          }

          rtData->riImpTexIds.add(rtData->riRes[i]->getImpostorTextures().albedo_alpha);
        }
        rtData->riRes[i]->gatherUsedTex(rtData->riImpTexIds);
      }
    }

  if (is_managed_textures_streaming_load_on_demand())
    prefetch_managed_textures(rtData->riImpTexIds);

  {
    rtData->rendinstMaxLod0Dist = ::dgs_get_game_params()->getReal("rendinstMaxLod0Dist", 10000.f);
    rtData->rendinstMaxDestructibleSizeSum = ::dgs_get_game_params()->getReal("rendinstMaxDestructibleSizeSum", 20.f);

    if (level_blk != nullptr && level_blk->paramExists("rendinstPreloadDistance"))
    {
      float preloadDist = level_blk->getReal("rendinstPreloadDistance");
      debug("rendinstPreloadDistance was overritten by level setting: %f", preloadDist);
      rtData->preloadDistance = rtData->settingsPreloadDistance = preloadDist;
    }
    else
      rtData->preloadDistance = rtData->settingsPreloadDistance = ::dgs_get_game_params()->getReal(
        main_layer ? "rendinstPreloadDistance" : "rendinstPreloadDistanceSL", rtData->settingsPreloadDistance);

    float delta = ::dgs_get_game_params()->getReal(main_layer ? "rendinstTransparencyDelta" : "rendinstTransparencyDeltaSL",
      rtData->preloadDistance * 0.1f);
    rtData->transparencyDeltaRcp = rtData->settingsTransparencyDeltaRcp =
      rtData->settingsPreloadDistance * (delta > 0.f ? 1.f / delta : 1.f);

    applyLodRanges();
  }


  float secondLayerDistMul = min(rendinstgenrender::globalDistMul, 0.75f);

  if (main_layer)
    rtData->setDistMul(rendinstgenrender::globalDistMul, rendinstgenrender::globalCullDistMul);
  else
    rtData->setDistMul(secondLayerDistMul, secondLayerDistMul);

  rtData->initImpostors();

  rtData->occlusionBoxHalfSize = v_div(V_C_HALF, invGridCellSzV);

#if DAGOR_DBGLEVEL > 0
  gpu_objects::setup_parameter_validation(::dgs_get_settings()->getBlockByNameEx("gpuobject_parameter_check_limits"));
#endif
  {
    const DataBlock *settings = ::dgs_get_game_params();
    const DataBlock *block = settings->getBlockByNameEx("treeCrownTransmittance");
    treeCrownTransmittance.brightness = block->getReal("brightness", 0.7);
    treeCrownTransmittance.falloffStart = block->getReal("falloffStart", 0.25);
    treeCrownTransmittance.falloffStop = block->getReal("falloffStop", 1.);
    treeCrownTransmittance.enabled = block->getBool("enabled", false);
    if (!treeCrownTransmittance.enabled)
      cascade0Dist = -1.f;
  }
}

void RendInstGenData::applyLodRanges()
{
  float maxDist = 0.f;
  float averageFarPlane = 0.f;
  unsigned int averageFarPlaneCount = 0;

  for (int i = 0; i < rtData->riRes.size(); i++)
  {
    if (!rtData->riRes[i] || !rtData->riResLodCount(i))
      continue;
    const DataBlock *ri_ovr = rtData->riResName[i] ? rendinst::ri_lod_ranges_ovr.getBlockByName(rtData->riResName[i]) : NULL;

    int lastLodNo = rtData->riResLodCount(i) - 1;
    float last_lod_range = min(rtData->riResLodRange(i, lastLodNo, ri_ovr), rtData->preloadDistance);
    maxDist = max(maxDist, last_lod_range);
    averageFarPlane += last_lod_range;
    averageFarPlaneCount++;

    if (!RendInstGenData::renderResRequired || !rtData->rtPoolData[i])
      continue;
    for (int lodI = 0; lodI < rtData->riResLodCount(i); lodI++)
      rtData->rtPoolData[i]->lodRange[lodI] = rtData->riResLodRange(i, lodI, ri_ovr);

    rtData->rtPoolData[i]->lodRange[lastLodNo] = last_lod_range;
    rtData->rtPoolData[i]->lodRange[0] = min(rtData->rtPoolData[i]->lodRange[0], rtData->rendinstMaxLod0Dist);
  }

  rtData->rendinstFarPlane = maxDist;
  if (averageFarPlaneCount == 0)
    rtData->averageFarPlane = rtData->rendinstFarPlane;
  else
    rtData->averageFarPlane = averageFarPlane / averageFarPlaneCount;

  rtData->preloadDistance = rtData->settingsPreloadDistance = max(rtData->rendinstFarPlane, rtData->settingsPreloadDistance);
}

void RendInstGenData::clearDelayedRi()
{
  if (rtData)
    for (auto &debris : rtData->riDebris)
      debris.clearDelayedRi();
}

void RendInstGenData::RtData::clear()
{
  rendinstgenrender::normal_type = -1;
  for (int i = 0; i < rtPoolData.size(); i++)
    del_it(rtPoolData[i]);
  clear_and_shrink(rtPoolData);
  maxDebris[0] = maxDebris[1] = curDebris[0] = curDebris[1] = 0;
  ShaderGlobal::reset_textures(true);
  for (auto &debris : riDebris)
    debris.clearDelayedRi();
}

void RendInstGenData::RtData::setTextureMinMipWidth(int textureSize, int impostorSize)
{
  G_ASSERT(textureSize > 0 && impostorSize > 0);
  impostorLastMipSize = impostorSize;

  for (unsigned int poolNo = 0; poolNo < rtPoolData.size(); poolNo++)
  {
    if (!rtPoolData[poolNo])
      continue;

    G_ASSERT(riRes[poolNo] != NULL);
    if (riResLodCount(poolNo) <= 1)
      continue;
    RenderableInstanceResource *sourceScene = riRes[poolNo]->lods.back().scene;
    ShaderMesh *mesh = sourceScene->getMesh()->getMesh()->getMesh();
    dag::Span<ShaderMesh::RElem> elems = mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_decal);
    if (elems.empty())
      continue;

    ShaderMaterial *mat = elems[0].mat;

    if (!strstr(mat->getInfo().str(), "rendinst_impostor") && !strstr(mat->getInfo().str(), "rendinst_baked_impostor")) // has impostor
                                                                                                                        // lod
      continue;

    ShaderMesh *impostorSourceMesh = riResLodScene(poolNo, riResLodCount(poolNo) - 2)->getMesh()->getMesh()->getMesh();
    dag::Span<ShaderMesh::RElem> imp_elems = impostorSourceMesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_decal);
    for (unsigned int elemNo = 0; elemNo < imp_elems.size(); elemNo++)
    {
      mat = imp_elems[elemNo].mat;

      if (!strstr(mat->getInfo().str(), "tree"))
        continue;

      TEXTUREID texId = mat->get_texture(0);

      if (texId == BAD_TEXTUREID)
        continue;

      BaseTexture *bTex = acquire_managed_tex(texId);
      G_ASSERT(bTex);

      if (bTex->restype() == RES3D_TEX)
      {
        Texture *tex = (Texture *)bTex;
        int lastMip = get_last_mip_idx(tex, textureSize);
        tex->texmiplevel(0, lastMip);
      }

      release_managed_tex(texId);
    }
  }
}


void RendInstGenData::RtData::copyVisibileImpostorsData(const RiGenVisibility &visibility, bool clear_data)
{
  if (visibility.renderRanges.size() != riRes.size())
    return;
  if (!visibility.hasImpostor())
  {
    if (!clear_data)
      return;
    for (unsigned int ri_idx = 0; ri_idx < riRes.size(); ri_idx++)
    {
      if (!rtPoolData[ri_idx] || rendinst::isResHidden(riResHideMask[ri_idx]))
        continue;
      if (!rtPoolData[ri_idx]->hasImpostor())
        continue;
      rtPoolData[ri_idx]->hadVisibleImpostor = false;
    }
    return;
  }
  for (unsigned int ri_idx = 0; ri_idx < riRes.size(); ri_idx++)
  {
    if (!rtPoolData[ri_idx] || rendinst::isResHidden(riResHideMask[ri_idx]))
      continue;
    if (!rtPoolData[ri_idx]->hasImpostor())
      continue;
    rtPoolData[ri_idx]->hadVisibleImpostor = visibility.renderRanges[ri_idx].hasImpostor();
  }
}

void RendInstGenData::CellRtData::clear()
{
  rtData->updateVbResetCS.lock();
  del_it(sysMemData);
  if (cellVbId)
    rtData->cellsVb.free(cellVbId);
  clear_and_shrink(scs);
  clear_and_shrink(bbox);
  rtData->updateVbResetCS.unlock();
}

RendInstGenData::RtData::RtData(int layer_idx) :
  cellsVb(SbufferHeapManager(String(128, "cells_vb_%d", layer_idx), //-V730
    RENDER_ELEM_SIZE, SBCF_MAYBELOST | SBCF_BIND_SHADER_RES | (rendinstgenrender::useCellSbuffer ? SBCF_MISC_STRUCTURED : 0),
    rendinstgenrender::useCellSbuffer ? 0
                                      : (RENDINST_FLOAT_POS ? rendinstgenrender::unpacked_format : rendinstgenrender::packed_format)))
{
  cellsVb.getManager().setShouldCopyToNewHeap(false);
  layerIdx = layer_idx;
  loadedCellsBBox = IBBox2(IPoint2(10000, 10000), IPoint2(-10000, -10000));
  lastPoi = Point2(-1000000, 1000000);
  maxDebris[0] = maxDebris[1] = curDebris[0] = curDebris[1] = 0;
  rendinstDistMul = rendinstDistMulFar = rendinstDistMulImpostorTrees = rendinstDistMulFarImpostorTrees =
    impostorsFarDistAdditionalMul = impostorsDistAdditionalMul = 1.0f;
  rendinstFarPlane = 1.f;
  rendinstMaxLod0Dist = 1.f;
  rendinstMaxDestructibleSizeSum = 1.f;
  preloadDistance = settingsPreloadDistance = 512.f;
  transparencyDeltaRcp = settingsTransparencyDeltaRcp = 1.f;
  averageFarPlane = 1000.f;
  viewImpostorDir = v_make_vec4f(0, 0, 1, 0);
  oldViewImpostorDir = v_zero();
  bigChangePoolNo = numImpostorsCount = oldImpostorCycle = 0;
  viewImpostorUp = v_make_vec4f(0, 1, 0, 0);
  dynamicImpostorToUpdateNo = 0;
  v_bbox3_init_empty(movedDebrisBbox);
  maxCellBbox.bmin = v_zero();
  maxCellBbox.bmax = v_zero();
  nextPoolForShadowImpostors = 0;
  nextPoolForClipmapShadows = 0;
}

void RendInstGenData::CellRtData::allocate(int idx)
{
  if (!RendInstGenData::renderResRequired || !vbSize)
    return;
  if (cellVbId)
    return;
  rtData->updateVbResetCS.lock();

  int vbSizeRes = vbSize * ri_cellsVB_elem_size_mul;
  if (!rtData->cellsVb.canAllocate(vbSizeRes)) // manual allocation strategy. Allocate at least N mb data, and use geometric (^2) until
                                               // we hit threshold; than use arithmetic growth
  {
    uint32_t nextHeapSize = rtData->cellsVb.getHeapSize();

    if (nextHeapSize < ri_cellsVB_geom_growth_heap_size_threshold)
      nextHeapSize = eastl::max(eastl::max(nextHeapSize << 1, ri_cellsVB_min_allocation_size),
        ((uint32_t)rtData->cellsVb.allocated() + vbSizeRes + ri_cellsVB_page_size_mask) & ~ri_cellsVB_page_size_mask);
    else
      nextHeapSize = nextHeapSize +
                     eastl::max(ri_cellsVB_arithmetic_grow_size, (vbSizeRes + ri_cellsVB_page_size_mask) & ~ri_cellsVB_page_size_mask);

    if (nextHeapSize > ri_cellsVB_max_size)
    {
      if (rtData->cellsVb.allocated() + vbSizeRes <= ri_cellsVB_max_size)
        nextHeapSize = ri_cellsVB_max_size;
      else
      {
#if DAGOR_DBGLEVEL > 0
        logerr("allocated too much of instances buffer %d needed, %d max_size", rtData->cellsVb.allocated() + vbSizeRes,
          ri_cellsVB_max_size);
#endif
      }
    }
    debug("rendinst: cellsVb: growing to %u", nextHeapSize);
    rtData->cellsVb.resize(nextHeapSize);
  }
  cellVbId = rtData->cellsVb.allocateInHeap(vbSizeRes);
  G_UNUSED(idx);

  rtData->updateVbResetCS.unlock();
}

void RendInstGenData::CellRtData::applyBurnDecal(const bbox3f &decal_bbox)
{
  if (bbox.size() > 0)
    if (v_bbox3_test_box_intersect_safe(decal_bbox, bbox[0]))
      burned = true;
}

void RendInstGenData::CellRtData::update(int size, RendInstGenData &rgd)
{
  if (!RendInstGenData::renderResRequired || !vbSize || !sysMemData)
    return;

  G_ASSERT(size <= (RENDINST_FLOAT_POS ? vbSize * (RENDER_ELEM_SIZE_UNPACKED / RENDER_ELEM_SIZE_PACKED) : vbSize));
  rtData->updateVbResetCS.lock();

  if (cellVbId)
  {
    auto vbInfo = rtData->cellsVb.get(cellVbId);
    G_ASSERT(size <= vbInfo.size);
#if RENDINST_FLOAT_POS
    vec3f v_cell_add = cellOrigin;
    vec3f v_cell_mul =
      v_mul(rendinstgen::VC_1div32767, v_make_vec4f(rgd.grid2world * rgd.cellSz, cellHeight, rgd.grid2world * rgd.cellSz, 0));

    const vec4f v_palette_mul =
      v_make_vec4f(1.f / (PALETTE_ID_MULTIPLIER * PALETTE_SCALE_MULTIPLIER), 1.f / (PALETTE_ID_MULTIPLIER * PALETTE_SCALE_MULTIPLIER),
        1.f / (PALETTE_ID_MULTIPLIER * PALETTE_SCALE_MULTIPLIER), 1.f / (PALETTE_ID_MULTIPLIER * PALETTE_SCALE_MULTIPLIER));

    constexpr int unpackRatio = RENDER_ELEM_SIZE_UNPACKED / RENDER_ELEM_SIZE_PACKED;

    int transferSize = 0;
    dag::Vector<uint8_t, framemem_allocator> tmpMem;

    for (int p = 0, pe = pools.size(); p < pe; ++p)
    {
      const EntPool &pool = pools[p];
      bool posInst = rtData->riPosInst[p];
      int srcStride = RIGEN_STRIDE_B(posInst, rgd.perInstDataDwords);
      int dstStride = srcStride * unpackRatio;
      int dstSize = pool.total * dstStride;

      if (pool.total == 0)
        continue;

      if (tmpMem.size() < dstSize)
        tmpMem.resize(dstSize);

      transferSize += dstSize;
      uint8_t *src = sysMemData + pool.baseOfs;
      uint8_t *dst = tmpMem.data();
      if (posInst)
      {
        for (int i = 0; i < pool.total; ++i)
        {
          vec4f pos, scale;
          vec4i palette_id;
          rendinstgen::unpack_tm_pos(pos, scale, (int16_t *)(src + i * srcStride), v_cell_add, v_cell_mul, true, &palette_id);
          vec4f palette_idf = v_mul(v_cvt_vec4f(palette_id), v_palette_mul);
          pos = v_perm_xyzW(pos, v_add(scale, palette_idf));
          v_stu(dst + i * dstStride, pos);
        }
      }
      else
      {
        for (int i = 0; i < pool.total; ++i)
        {
          mat44f tm;
          rendinstgen::unpack_tm_full(tm, (int16_t *)(src + i * srcStride), v_cell_add, v_cell_mul);
          v_mat44_transpose(tm, tm);
          uint8_t *d = dst + i * dstStride;
          v_stu(d + sizeof(Point4) * 0, tm.col0);
          v_stu(d + sizeof(Point4) * 1, tm.col1);
          v_stu(d + sizeof(Point4) * 2, tm.col2);

#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS
          for (int j = 0; j < rgd.perInstDataDwords; ++j)
          {
            uint16_t packed = *((uint16_t *)(src + i * srcStride + RIGEN_TM_STRIDE_B(0) + j * sizeof(uint16_t)));
            uint32_t unpacked = packed; // Written as uint32, and read back with asuint from the shader.
            memcpy(d + RIGEN_TM_STRIDE_B(0) * unpackRatio + j * sizeof(float), &unpacked, sizeof(uint32_t));
          }
#endif
        }
      }
      rtData->cellsVb.getHeap().getBuf()->updateData(vbInfo.offset + pool.baseOfs * unpackRatio, dstSize, dst, VBLOCK_WRITEONLY);
    }
    G_ASSERTF(transferSize == size * unpackRatio, "RendInstGenData::CellRtData::update dstSize != transferSize (%d/%d)",
      size * unpackRatio, transferSize);
    G_UNUSED(size);
#else
    rtData->cellsVb.getHeap().getBuf()->updateData(vbInfo.offset, size, sysMemData, VBLOCK_WRITEONLY);
    G_UNUSED(rgd);
#endif

    heapGen = rtData->cellsVb.getManager().getHeapGeneration();
  }

  if (rendinstgenrender::avoidStaticShadowRecalc)
    cellStateFlags = (cellStateFlags & (~(CLIPMAP_SHADOW_RENDERED_ALLCASCADE))) | LOADED;
  else
    cellStateFlags = (cellStateFlags & (~(CLIPMAP_SHADOW_RENDERED_ALLCASCADE | STATIC_SHADOW_RENDERED))) | LOADED;

  rtData->updateVbResetCS.unlock();
}

void RendInstGenData::updateVb(RendInstGenData::CellRtData &crt, int idx)
{
  if (!RendInstGenData::renderResRequired || !crt.vbSize || !crt.sysMemData)
    return;

  crt.allocate(idx);
  crt.update(crt.vbSize, *this);
}

// static vec4f curViewPos;
static __forceinline int test_box_planes(vec4f bmin, vec4f bmax, const Frustum &curFrustum, vec4f *__restrict planes, int &nplanes)
{
  vec4f center = v_add(bmax, bmin);
  vec4f extent = v_sub(bmax, bmin);

  if (!curFrustum.testBoxExtentB(center, extent))
    return Frustum::OUTSIDE;

  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  center = v_mul(center, V_C_HALF);
  extent = v_mul(extent, V_C_HALF);
  int result = Frustum::INSIDE;
  nplanes = 0;
  for (int i = 0; i < 6; i += 2)
  {
    vec4f res1, res2;
    res1 = v_add(v_dot3(v_sub(center, v_xor(extent, v_and(curFrustum.camPlanes[i], signmask))), curFrustum.camPlanes[i]),
      v_splat_w(curFrustum.camPlanes[i]));
    res2 = v_add(v_dot3(v_sub(center, v_xor(extent, v_and(curFrustum.camPlanes[i + 1], signmask))), curFrustum.camPlanes[i + 1]),
      v_splat_w(curFrustum.camPlanes[i + 1]));
    if (v_test_vec_x_lt_0(res1))
    {
      planes[nplanes] = v_perm_xyzd(curFrustum.camPlanes[i], v_add(curFrustum.camPlanes[i], curFrustum.camPlanes[i]));
      nplanes++;
      result = Frustum::INTERSECT;
    }

    if (v_test_vec_x_lt_0(res2))
    {
      planes[nplanes] = v_perm_xyzd(curFrustum.camPlanes[i + 1], v_add(curFrustum.camPlanes[i + 1], curFrustum.camPlanes[i + 1]));
      nplanes++;
      result = Frustum::INTERSECT;
    }
  }
  return result;
}

static __forceinline int test_box_planesb(vec4f bmin, vec4f bmax, const vec4f *__restrict planes, int nplanes)
{
  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  vec4f center = v_add(bmax, bmin);
  vec4f extent = v_sub(bmax, bmin);
  vec4f res = v_zero();
  if (nplanes & 1)
  {
    res = v_add_x(v_dot3_x(v_add(center, v_xor(extent, v_and(planes[0], signmask))), planes[0]), v_splat_w(planes[0]));
    planes++;
    nplanes--;
  }

  for (int i = 0; i < nplanes; i += 2, planes += 2)
  {
    res = v_or(res, v_add_x(v_dot3_x(v_add(center, v_xor(extent, v_and(planes[0], signmask))), planes[0]), v_splat_w(planes[0])));
    res = v_or(res, v_add_x(v_dot3_x(v_add(center, v_xor(extent, v_and(planes[1], signmask))), planes[1]), v_splat_w(planes[1])));
  }
#if _TARGET_SIMD_SSE
  return (~_mm_movemask_ps(res)) & 1;
#else
  return !v_test_vec_x_lt_0(res);
#endif
}

static inline bool getSubCellsVisibility(vec3f curViewPos, const Frustum &frustum, const bbox3f *__restrict bbox,
  uint16_t &ranges_count, uint8_t *ranges, float &cellDist, Occlusion *use_occlusion)
{
  G_STATIC_ASSERT(RendInstGenData::SUBCELL_DIV == 8);
  G_STATIC_ASSERT(RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV <= 256);

  int nPlanes;
  vec4f planes[6];
  int cellIntersection = test_box_planes(bbox[0].bmin, bbox[0].bmax, frustum, planes, nPlanes);
  if (cellIntersection == Frustum::OUTSIDE)
  {
    ranges_count = 0;
    return false;
  }
  if (use_occlusion)
  {
    if (use_occlusion->isOccludedBox(bbox[0].bmin, bbox[0].bmax))
    {
      ranges_count = 0;
      return false;
    }
  }
  cellDist = v_extract_x(v_sqrt_x(v_distance_sq_to_bbox_x(bbox[0].bmin, bbox[0].bmax, curViewPos)));
  if (cellIntersection == Frustum::INSIDE) //-V1051
  {
    if (use_occlusion)
    {
      int idx = 1;
      for (; idx < RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV + 1; ++idx)
        if (!use_occlusion->isOccludedBox(bbox[idx].bmin, bbox[idx].bmax))
          break;

      int idx2 = RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV;
      for (; idx2 > idx; --idx2)
        if (!use_occlusion->isOccludedBox(bbox[idx2].bmin, bbox[idx2].bmax))
          break;
      if (idx2 < idx)
      {
        ranges_count = 0;
        return false;
      }
      ranges[0] = idx - 1;
      ranges[1] = idx2 - 1;
    }
    else
    {
      ranges[0] = 0;
      ranges[1] = RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV - 1;
    }
    ranges_count = 1;
    return true;
  }


  ranges_count = 0;
  for (int idx = 1; idx < RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV + 1; ++idx)
  {
    // if (frustum.testBoxB(bbox[idx].bmin, bbox[idx].bmax))
    if (test_box_planesb(bbox[idx].bmin, bbox[idx].bmax, planes, nPlanes))
    {
      if (use_occlusion)
      {
        if (use_occlusion->isOccludedBox(bbox[idx].bmin, bbox[idx].bmax))
          continue;
      }
      if (!ranges_count || ranges[-1] != idx - 2)
      {
        ranges[0] = ranges[1] = idx - 1;
        ranges += 2;
        ranges_count++;
      }
      else
        ranges[-1] = idx - 1;
    }
  }
  return ranges_count != 0;
}

struct RendInstGenRenderPrepareData
{
  static constexpr int MAX_VISIBLE_CELLS = 256;
  int totalVisibleCells;
  struct Cell
  {
    float distance;
    uint16_t x, z;
    uint16_t rangesStart;
    uint16_t rangesCount; // it is actually only 5 bit, but for the sake of alignment, keep it 16bit
  };
  carray<Cell, MAX_VISIBLE_CELLS> cells = {};
  carray<uint8_t, MAX_VISIBLE_CELLS * 2 * (RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV / 2 + 1)> cellRanges = {};
  int visibleCellsCount() const { return totalVisibleCells; }
  RendInstGenRenderPrepareData() : totalVisibleCells(0) {}
};

template <bool use_external_filter>
bool RendInstGenData::prepareVisibility(const Frustum &frustum, const Point3 &camera_pos, RiGenVisibility &visibility, bool forShadow,
  uint32_t layer_flags, Occlusion *use_occlusion, bool for_visual_collision, const rendinst::VisibilityExternalFilter &external_filter)
{
  TIME_D3D_PROFILE(prepare_ri_visibility);
  visibility.vismask = 0;
  if (for_visual_collision && rendinst::isRgLayerPrimary(rtData->layerIdx))
    return false;

  if (rendinst::ri_game_render_mode == 0)
    use_occlusion = NULL;
  vec3f curViewPos = v_ldu(&camera_pos.x);
  Tab<RenderRanges> &riRenderRanges = visibility.renderRanges;
  Frustum curFrustum = frustum;
  if (!forShadow)
  {
    float maxRIDist = rtData->get_trees_last_range(rtData->rendinstFarPlane);
    shrink_frustum_zfar(curFrustum, curViewPos, v_splats(maxRIDist));
  }

  float forcedLodDist = 0.f;
  float forcedLodDistSq = 0.f;
  Point3_vec4 viewPos = camera_pos;
  if (visibility.forcedLod >= 0)
  {
    bbox3f box;
    frustum.calcFrustumBBox(box);
    curViewPos = v_bbox3_center(box);
    v_st(&viewPos.x, curViewPos);

    float rad = v_extract_x(v_bbox3_outer_rad(box));
    forcedLodDist = rad;
    forcedLodDistSq = forcedLodDist * forcedLodDist;
  }
  bbox3f worldBBox;
  float grid2worldcellSz = grid2world * cellSz;

  curFrustum.calcFrustumBBox(worldBBox);
  if (!check_occluders)
    use_occlusion = NULL;

  vec4f worldBboxXZ = v_perm_xzac(worldBBox.bmin, worldBBox.bmax);
  vec4f grid2worldcellSzV = v_splats(grid2worldcellSz);
  worldBboxXZ = v_add(worldBboxXZ, v_perm_xzac(v_neg(grid2worldcellSzV), grid2worldcellSzV));
  vec4f regionV = v_sub(worldBboxXZ, world0Vxz);
  regionV = v_max(v_mul(regionV, invGridCellSzV), v_zero());
  regionV = v_min(regionV, lastCellXZXZ);
  vec4i regionI = v_cvt_floori(regionV);
  DECL_ALIGN16(int, regions[4]);
  v_sti(regions, regionI);
  float grid2worldcellSzDiag = grid2worldcellSz * 1.4142f;
  visibility.subCells.clear();
  visibility.resizeRanges(rtData->riRes.size(), forShadow ? 4 : 8);

  ScopedLockRead lock(rtData->riRwCs);
  RendInstGenRenderPrepareData visData;
  uint8_t *ranges = visData.cellRanges.data();

#define PREPARE_CELL(XX, ZZ)                                                                                                        \
  {                                                                                                                                 \
    const RendInstGenData::Cell &cell = cells[cellI];                                                                               \
    const RendInstGenData::CellRtData *crt_ptr = cell.isReady();                                                                    \
    if (crt_ptr && visData.totalVisibleCells < visData.MAX_VISIBLE_CELLS)                                                           \
    {                                                                                                                               \
      int cellNo = visData.visibleCellsCount();                                                                                     \
      RendInstGenRenderPrepareData::Cell &visCell = visData.cells[cellNo];                                                          \
      if (getSubCellsVisibility(curViewPos, curFrustum, crt_ptr->bbox.data(), visCell.rangesCount, ranges, visCell.distance,        \
            use_occlusion))                                                                                                         \
      {                                                                                                                             \
        visCell.x = XX, visCell.z = ZZ;                                                                                             \
        visCell.rangesStart = ranges - visData.cellRanges.data();                                                                   \
        visCell.rangesCount *= 2;                                                                                                   \
        /*visData.cellXZ[cellNo] = I16Point2(XX,ZZ);                                                                                \
        visData.cellRangesStart[cellNo] = ranges-visData.cellRanges.data();                                                         \
        visData.cellRangesCount[cellNo] *= 2;                                                                                       \
        ranges += visData.cellRangesCount[cellNo];*/                                                                                \
        ranges += visCell.rangesCount;                                                                                              \
        G_ASSERT((int)(ranges - visData.cellRanges.data()) < visData.cellRanges.size());                                            \
        /*if (visData.cellDistances[cellNo]<=grid2worldcellSzDiag && visData.totalSubCellsDistSqNo < visData.subCellsDistSq.size()) \
        {                                                                                                                           \
          int cellIdx = visData.totalSubCellsDistSqNo++;                                                                            \
          visData.cellNoToSubCellsDistSq[cellNo] = cellIdx;                                                                         \
          CellRanges &ranges = visData.cellRanges[cellNo];                                                                          \
          for (int rangeI = 0; rangeI < ranges.ranges_count; ++rangeI)                                                              \
          {                                                                                                                         \
            for (int cri = ranges.start_end[rangeI][0]; cri <= ranges.start_end[rangeI][1]; ++cri)                                  \
            {                                                                                                                       \
              visData.subCellsDistSq[cellIdx][cri]                                                                                  \
                = v_extract_x(v_distance_sq_to_bbox_x(crt_ptr->bbox[cri+1].bmin, crt_ptr->bbox[cri+1].bmax, curViewPos));           \
            }                                                                                                                       \
          }                                                                                                                         \
        } else                                                                                                                      \
          visData.cellNoToSubCellsDistSq[cellNo] = 0xFF;*/                                                                          \
        visData.totalVisibleCells++;                                                                                                \
      }                                                                                                                             \
    }                                                                                                                               \
  }

  rtData->loadedCellsBBox.clip(regions[0], regions[1], regions[2], regions[3]);

  float grid2worldCellSz = v_extract_x(invGridCellSzV);
  int startCellX = (int)((viewPos.x - world0x()) * grid2worldCellSz);
  int startCellZ = (int)((viewPos.z - world0z()) * grid2worldCellSz);

  // in not yet known circumstances, values go out of range (in release mode)
  // trying to avoid infinite loop
  if (startCellX == 0x80000000 || startCellZ == 0x80000000)
  {
    debug("prepareVisibility_overflow");
    return false;
  }

  unsigned int startCellI = startCellX + startCellZ * cellNumW;
  // if ((regions[2] - regions[0]+1)*(regions[3] - regions[1]+1) <= ld.size())
  {
    int maxRadius =
      max(max((regions[3] - startCellZ), (startCellZ - regions[1])), max((regions[2] - startCellX), (startCellX - regions[0])));
    if (startCellX >= 0 && startCellX < cellNumW && startCellZ < cellNumH && startCellZ >= 0)
    {
      int cellI = startCellI;
      PREPARE_CELL(startCellX, startCellZ);
    }
    int minX, maxX;
    int minZ, maxZ;
    for (int radius = 1; radius <= maxRadius; ++radius)
    {
      minX = startCellX - radius, maxX = startCellX + radius;
      minZ = startCellZ - radius, maxZ = startCellZ + radius;

      if (minZ >= regions[1] && minZ <= regions[3])
        for (int x = max(minX, regions[0]), cellI = minZ * cellNumW + x; x <= min(maxX, regions[2]); x++, cellI++)
          PREPARE_CELL(x, minZ);

      if (maxZ <= regions[3] && maxZ >= regions[1])
        for (int x = max(minX, regions[0]), cellI = maxZ * cellNumW + x; x <= min(maxX, regions[2]); x++, cellI++)
          PREPARE_CELL(x, maxZ);

      if (minX >= regions[0] && minX <= regions[2])
        for (int z = max(minZ + 1, regions[1]), cellI = z * cellNumW + minX; z <= min(maxZ - 1, regions[3]); z++, cellI += cellNumW)
          PREPARE_CELL(minX, z);

      if (maxX <= regions[2] && maxX >= regions[0])
        for (int z = max(minZ + 1, regions[1]), cellI = z * cellNumW + maxX; z <= min(maxZ - 1, regions[3]); z++, cellI += cellNumW)
          PREPARE_CELL(maxX, z);
    }

    // int cellXStride = cellNumW - (regions[2] - regions[0]+1);
    /*for (int z = regions[1], cellI = regions[1]*cellNumW+regions[0]; z <= regions[3]; z++, cellI += cellXStride)
    for (int x = regions[0]; x <= regions[2]; x++, cellI++)
    {
      if (x >= minX && x<=maxX && z>=minZ && z<=maxZ)
        continue;
      PREPARE_CELL(x, z);
    }*/
  }
// else//front to back sorting not working here!
/*{
  if (startCellX >= 0 && startCellX < cellNumW && startCellZ<cellNumH && startCellZ>=0)
  {
    int cellI = startCellI;
    PREPARE_CELL(startCellX, startCellZ);
  }
  for (int i = 0; i < ld.size(); i++)
  {
    int cellI = ld[i];
    RendInstGenData::Cell &cell = cells[cellI];
    if (!cell.isReady() || startCellI==cellI)
      continue;
    int x = cellI%cellNumW;
    if ( (x<regions[0] || x>regions[2]))
      continue;
    int z = cellI/cellNumW;
    if ( (z<regions[1] || z>regions[3]))
      continue;
    PREPARE_CELL(x,z);
  }
}*/
#undef PREPARE_CELL
  if (!visData.visibleCellsCount())
    return false;
  mem_set_0(visibility.instNumberCounter);
  float subCellOfsSize = grid2worldcellSz * ((rendinstgenrender::per_instance_visibility_for_everyone ? 0.75f : 0.25f) *
                                              (rendinstgenrender::globalDistMul * 1.f / RendInstGenData::SUBCELL_DIV));

  // int totalTrees = 0, totalCells = 0, totalInsideTrees = 0;

  static constexpr int MAX_PER_INSTANCE_CELLS = 7;
  vec3f v_cell_add[MAX_PER_INSTANCE_CELLS], v_cell_mul[MAX_PER_INSTANCE_CELLS];
  static constexpr int MAX_INSTANCES_TO_SORT = 256;
  carray<carray<IPoint2, MAX_INSTANCES_TO_SORT>, rendinst::MAX_LOD_COUNT> perInstanceDistance;
  carray<carray<vec4f, MAX_INSTANCES_TO_SORT>, rendinst::MAX_LOD_COUNT> sortedInstances;
  if (rendinstgenrender::per_instance_visibility)
  {
    visibility.startTreeInstances();
    for (int vi = 0; vi < min<int>(MAX_PER_INSTANCE_CELLS, visData.visibleCellsCount()); ++vi)
    {
      int x = visData.cells[vi].x;
      int z = visData.cells[vi].z;
      int cellId = x + z * cellNumW;
      RendInstGenData::Cell &cell = cells[cellId];
      RendInstGenData::CellRtData &crt = *cell.rtData;
      v_cell_add[vi] = crt.cellOrigin;
      v_cell_mul[vi] = v_mul(rendinstgen::VC_1div32767, v_make_vec4f(grid2worldcellSz, crt.cellHeight, grid2worldcellSz, 0));
    }
  }

  vec3f v_tree_min, v_tree_max;
  v_tree_min = v_tree_max = v_zero();
  carray<int, RiGenVisibility::PER_INSTANCE_LODS> prevInstancesCount;
  mem_set_0(prevInstancesCount);
  visibility.max_per_instance_instances = 0;

  for (unsigned int ri_idx = 0; ri_idx < rtData->riRes.size(); ri_idx++)
  {
    if (!rtData->rtPoolData[ri_idx] || rendinst::isResHidden(rtData->riResHideMask[ri_idx]))
      continue;

    rendinstgenrender::RtPoolData &pool = *rtData->rtPoolData[ri_idx];

    if (for_visual_collision &&
        (pool.hasImpostor() || rtData->riPosInst[ri_idx] || rtData->riResElemMask[ri_idx * rendinst::MAX_LOD_COUNT].atest))
      continue;

    bool crossDissolveForPool = false, shortAlpha = forShadow;
    if (pool.hasImpostor() && rendinstgenrender::per_instance_visibility) // fixme: allow for all types of impostors
    {
      v_tree_min = rtData->riResBb[ri_idx].bmin;
      v_tree_max = rtData->riResBb[ri_idx].bmax;
      crossDissolveForPool = !pool.hasTransitionLod() && rendinstgenrender::use_cross_dissolve && visibility.forcedLod < 0;
      shortAlpha = false;
    }
    // unsigned int stride = RIGEN_STRIDE_B(rtData->riPosInst[ri_idx], perInstDataDwords);
    int lodCnt = rtData->riResLodCount(ri_idx);
    int farLodNo = lodCnt - 1;
    int alphaFarLodNo = farLodNo + 1;
    int farLodNo_translated = RiGenVisibility::PI_LAST_MESH_LOD;
    G_ASSERT(alphaFarLodNo < rendinstgenrender::MAX_LOD_COUNT_WITH_ALPHA); // one lod is for transparence
    float farLodStartRange;
    if (farLodNo)
      farLodStartRange =
        pool.hasImpostor() ? rtData->get_trees_range(pool.lodRange[farLodNo - 1]) : rtData->get_range(pool.lodRange[farLodNo - 1]);
    else
      farLodStartRange = 0;
    float farLodEndRange =
      pool.hasImpostor() ? rtData->get_trees_last_range(pool.lodRange[farLodNo]) : rtData->get_last_range(pool.lodRange[farLodNo]);
    farLodEndRange *= visibility.riDistMul;
    farLodStartRange = min(farLodStartRange, farLodEndRange * 0.99f);
    float farLodEndRangeSq = farLodEndRange * farLodEndRange;
    float farLodStartRangeCellDist = max(0.f, farLodStartRange + grid2worldcellSzDiag); // maximum distance for cell caluclation
    float deltaRcp = rtData->transparencyDeltaRcp / farLodEndRange;
    float perInstanceAlphaBlendStartRadius = (rtData->transparencyDeltaRcp - 1.f) / deltaRcp;
    float alphaBlendStartRadius = perInstanceAlphaBlendStartRadius;
    float startAlphaDistance = shortAlpha ? grid2worldcellSz * 0.1 : grid2worldcellSzDiag; // transparent objects can either cast
                                                                                           // shadow or not cast shadow
    if (pool.hasImpostor() && !shortAlpha && rtData->rendinstDistMulFarImpostorTrees > 1.0)
      alphaBlendStartRadius /= rtData->rendinstDistMulFarImpostorTrees;
    // startAlphaDistance*=2;
    float alphaBlendOnCellRadius = alphaBlendStartRadius - startAlphaDistance;
    float alphaBlendOnSubCellRadius = alphaBlendStartRadius - startAlphaDistance * (1.f / RendInstGenData::SUBCELL_DIV);


    float lodDistancesSq[rendinstgenrender::MAX_LOD_COUNT_WITH_ALPHA] = {0};
    float lodDistancesSq_perInst[RiGenVisibility::PER_INSTANCE_LODS] = {0};
    G_ASSERT(lodCnt < rendinstgenrender::MAX_LOD_COUNT_WITH_ALPHA);
    int lodTranslation = rendinst::MAX_LOD_COUNT - lodCnt;
    bool hasImpostor = pool.hasImpostor();
    if (!hasImpostor)
    {
      G_ASSERT(lodTranslation > 0);
      if (lodTranslation > 0)
        lodTranslation--;
    }
    for (int lodI = 1; lodI < lodCnt; ++lodI)
    {
      lodDistancesSq[lodI + lodTranslation] =
        pool.hasImpostor() ? rtData->get_trees_range(pool.lodRange[lodI - 1]) : rtData->get_range(pool.lodRange[lodI - 1]);
      const float added = lodI == lodCnt - 1 && hasImpostor // fixme: increase by radius of tree bounding sphere?
                              && rendinstgenrender::use_tree_lod0_offset
                            ? subCellOfsSize // only for impostor
                            : 0;
      lodDistancesSq_perInst[remap_per_instance_lod(lodI + lodTranslation)] = lodDistancesSq[lodI + lodTranslation] + added;
    }
    if (!hasImpostor)
    {
      lodDistancesSq[rendinst::MAX_LOD_COUNT - 1] = 100000;
      lodDistancesSq_perInst[remap_per_instance_lod(rendinst::MAX_LOD_COUNT - 1)] = 100000;
    }

    if (crossDissolveForPool)
    {
      lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD1] =
        lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD0 + 1] - TOTAL_CROSS_DISSOLVE_DIST; // fixed size for cross dissolve
      lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD0] =
        lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD1] + LOD1_DISSOLVE_RANGE; // fixed size for cross dissolve
    }
    else
    {
      lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD1] = lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD0] = 100000;
    }

    for (int lodI = 1 + lodTranslation; lodI < rendinst::MAX_LOD_COUNT; ++lodI)
      lodDistancesSq[lodI] = safediv(lodDistancesSq[lodI], rendinstgenrender::lodsShiftDistMul);

    for (int lodI = 1 + lodTranslation; lodI < rendinst::MAX_LOD_COUNT; ++lodI)
      lodDistancesSq[lodI] *= lodDistancesSq[lodI];

    lodDistancesSq[lodTranslation] = -1.f; // all rendinst closer, then lod1
    lodDistancesSq_perInst[remap_per_instance_lod(lodTranslation)] = -1.f;
    if (alphaBlendOnSubCellRadius < 0)
    {
      lodDistancesSq[farLodNo_translated + 2] = lodDistancesSq[farLodNo_translated + 1];
      lodDistancesSq_perInst[visibility.PI_ALPHA_LOD] = lodDistancesSq_perInst[visibility.PI_IMPOSTOR_LOD];
    }
    else
    {
      lodDistancesSq[farLodNo_translated + 2] = alphaBlendOnSubCellRadius * alphaBlendOnSubCellRadius;
      lodDistancesSq_perInst[visibility.PI_ALPHA_LOD] = alphaBlendOnSubCellRadius + subCellOfsSize;
    }

    if (crossDissolveForPool)
    {
      lodDistancesSq_perInst[visibility.PI_ALPHA_LOD] = perInstanceAlphaBlendStartRadius;
      // lodDistancesSq_perInst[visibility.PI_ALPHA_LOD-1] = lodDistancesSq_perInst[visibility.PI_ALPHA_LOD];
      lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD0] = min(lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD0],
        lodDistancesSq_perInst[visibility.PI_ALPHA_LOD] - LOD0_DISSOLVE_RANGE);
      lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD1] = min(lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD1],
        lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD0] - LOD1_DISSOLVE_RANGE);
      visibility.crossDissolveRange[ri_idx] = lodDistancesSq_perInst[visibility.PI_DISSOLVE_LOD1];
    }

    for (int lodI = 1 + lodTranslation; lodI < RiGenVisibility::PER_INSTANCE_LODS - 1; ++lodI)
      lodDistancesSq_perInst[lodI] = safediv(lodDistancesSq_perInst[lodI], rendinstgenrender::lodsShiftDistMul);

    for (int lodI = 1 + lodTranslation; lodI < RiGenVisibility::PER_INSTANCE_LODS; ++lodI)
      lodDistancesSq_perInst[lodI] *= lodDistancesSq_perInst[lodI];

    float distanceToCheckPerInstanceSq =
      hasImpostor ? max(32.f * 32.f, lodDistancesSq_perInst[remap_per_instance_lod(visibility.PI_IMPOSTOR_LOD)])
                  : max(32.f * 32.f, lodDistancesSq_perInst[remap_per_instance_lod(visibility.PI_LAST_MESH_LOD)]);

    visibility.stride = RIGEN_STRIDE_B(pool.hasImpostor(), perInstDataDwords); // rtData->riPosInst[ri_idx]
    visibility.startRenderRange(ri_idx);
    carray<int, RiGenVisibility::PER_INSTANCE_LODS> perInstanceData;
    mem_set_ff(perInstanceData);
    bool pool_front_to_back = rendinstgenrender::per_instance_front_to_back && (cpujobs::get_core_count() > 3);

    rendinstgen::RotationPaletteManager::Palette palette =
      rendinstgen::get_rotation_palette_manager()->getPalette({rtData->layerIdx, (int)ri_idx});
    for (int vi = 0; vi < visData.visibleCellsCount(); vi++)
    {
      int x = visData.cells[vi].x;
      int z = visData.cells[vi].z;
      int cellId = x + z * cellNumW;
      const RendInstGenData::Cell &cell = cells[cellId];
      const RendInstGenData::CellRtData *crt_ptr = cell.isReady();
      if (!crt_ptr)
        continue;
      const RendInstGenData::CellRtData &crt = *crt_ptr;
      if (crt.pools[ri_idx].total - crt.pools[ri_idx].avail < 1)
        continue;

      float minDist = visData.cells[vi].distance;
      float maxDist = visibility.forcedLod < 0 ? farLodEndRange : forcedLodDist;
      if (minDist >= maxDist)
        continue;
      int startVbOfs = crt.getCellSlice(ri_idx, 0).ofs;
      // fixme: we can separate for subcells if alphaBlendOnRadius is splitting cell, so part of subcells will be blended and part not
      carray<int, rendinst::MAX_LOD_COUNT> perInstanceDistanceCnt;
      mem_set_0(perInstanceDistanceCnt);
      if ((!farLodNo && forShadow) || (minDist > farLodStartRangeCellDist && (minDist > 0.f || minDist <= alphaBlendOnCellRadius)) ||
          (layer_flags & rendinst::LAYER_FORCE_LOD_MASK) || visibility.forcedLod >= 0)
      {
        // add all ranges to far lod

        int lodI;
        if (visibility.forcedLod >= 0)
        {
          lodI = min(visibility.forcedLod, farLodNo);
        }
        else if (layer_flags & rendinst::LAYER_FORCE_LOD_MASK)
        {
          lodI = clamp(int(layer_flags & rendinst::LAYER_FORCE_LOD_MASK) >> rendinst::LAYER_FORCE_LOD_SHIFT, 1,
                   rtData->riResLodCount(ri_idx)) -
                 1;
        }
        else
        {
          lodI = (minDist > alphaBlendOnCellRadius && !pool.hasImpostor()) ? alphaFarLodNo : farLodNo;
          if (forShadow && lodI == alphaFarLodNo)
            continue;
        }

        if (use_external_filter)
        {
          if (!external_filter(crt.bbox[0].bmin, crt.bbox[0].bmax))
            continue;
        }

        rtData->riRes[ri_idx]->updateReqLod(min<int>(lodI, rtData->riRes[ri_idx]->lods.size() - 1));
        if (lodI < rtData->riRes[ri_idx]->getQlBestLod())
          lodI = rtData->riRes[ri_idx]->getQlBestLod();

        int cellAdded = -1;
        for (uint8_t *rangeI = visData.cellRanges.data() + visData.cells[vi].rangesStart,
                     *end = rangeI + visData.cells[vi].rangesCount;
             rangeI != end; rangeI += 2)
        {
          const RendInstGenData::CellRtData::SubCellSlice &scss = crt.getCellSlice(ri_idx, rangeI[0]);
          const RendInstGenData::CellRtData::SubCellSlice &scse = crt.getCellSlice(ri_idx, rangeI[1]);
          if (scse.ofs + scse.sz == scss.ofs)
            continue;
          // add to farLodNo
          if (cellAdded < 0)
            cellAdded = visibility.addCell(lodI, x, z, startVbOfs, visData.cells[vi].rangesCount >> 1);
          visibility.addRange(lodI, scss.ofs, (scse.ofs + scse.sz - scss.ofs));
          visibility.instNumberCounter[lodI] += (scse.ofs + scse.sz - scss.ofs) / visibility.stride;
        }
        if (cellAdded >= 0)
          visibility.closeRanges(lodI);
      }
      else
      {
        // separate subcells between different lods
        carray<int, rendinstgenrender::MAX_LOD_COUNT_WITH_ALPHA> cellAdded;
        int lastLod = -1;
        mem_set_ff(cellAdded);
        for (uint8_t *rangeI = visData.cellRanges.data() + visData.cells[vi].rangesStart,
                     *end = rangeI + visData.cells[vi].rangesCount;
             rangeI != end; rangeI += 2)
        {
          const RendInstGenData::CellRtData::SubCellSlice &scss = crt.getCellSlice(ri_idx, rangeI[0]);
          const RendInstGenData::CellRtData::SubCellSlice &scse = crt.getCellSlice(ri_idx, rangeI[1]);
          if (scse.ofs + scse.sz == scss.ofs)
            continue;
          for (int cri = rangeI[0]; cri <= rangeI[1]; ++cri)
          {
            const RendInstGenData::CellRtData::SubCellSlice &sc = crt.getCellSlice(ri_idx, cri);
            if (!sc.sz) // empty subcell
              continue;

            float subCellDistSq = v_extract_x(v_distance_sq_to_bbox_x(crt.bbox[cri + 1].bmin, crt.bbox[cri + 1].bmax, curViewPos));
            /*float subCellDistSq;
            if (visData.cellNoToSubCellsDistSq[vi] < visData.subCellsDistSq.size())
            {
              subCellDistSq = visData.subCellsDistSq[visData.cellNoToSubCellsDistSq[vi]][cri];
            } else
            {
              subCellDistSq = v_extract_x(v_distance_sq_to_bbox_x(crt.bbox[cri+1].bmin, crt.bbox[cri+1].bmax, curViewPos));
            }*/
            float maxDistSq = visibility.forcedLod < 0 ? farLodEndRangeSq : forcedLodDistSq;
            if (subCellDistSq >= maxDistSq)
              continue; // too far away

            if (use_external_filter)
            {
              if (!external_filter(crt.bbox[cri + 1].bmin, crt.bbox[cri + 1].bmax))
                continue;
            }
            if (vi < MAX_PER_INSTANCE_CELLS && pool.hasImpostor() && rendinstgenrender::per_instance_visibility) // todo: support not
                                                                                                                 // impostors
            {
              if (subCellDistSq < distanceToCheckPerInstanceSq)
              {
                for (int16_t *data = (int16_t *)(crt.sysMemData + sc.ofs), *data_e = data + sc.sz / 2; data < data_e; data += 4)
                {
                  if (rendinst::is_pos_rendinst_data_destroyed(data))
                    continue;
                  bool palette_rotation = rtData->riPaletteRotation[ri_idx];
                  vec4f v_pos, v_scale;
                  vec4i v_palette_id;
                  rendinstgen::unpack_tm_pos(v_pos, v_scale, data, v_cell_add[vi], v_cell_mul[vi], palette_rotation, &v_palette_id);
                  bbox3f treeBBox;
                  if (palette_rotation)
                  {
                    quat4f v_rot = rendinstgen::RotationPaletteManager::get_quat(palette, v_extract_xi(v_palette_id));
                    mat44f tm;
                    v_mat44_compose(tm, v_pos, v_rot, v_scale);
                    bbox3f bbox;
                    bbox.bmin = v_tree_min;
                    bbox.bmax = v_tree_max;
                    v_bbox3_init(treeBBox, tm, bbox);
                  }
                  else
                  {
                    treeBBox.bmin = v_add(v_pos, v_mul(v_scale, v_tree_min));
                    treeBBox.bmax = v_add(v_pos, v_mul(v_scale, v_tree_max));
                  }
                  if (frustum.testBoxExtentB(v_add(treeBBox.bmax, treeBBox.bmin), v_sub(treeBBox.bmax, treeBBox.bmin)))
                  {
                    if (use_occlusion)
                    {
                      vec3f occBmin = treeBBox.bmin, occBmax = treeBBox.bmax;
                      if (forShadow)
                      {
                        const float maxLightDistForTreeShadow = 20.0f;
                        vec3f lightDist = v_mul(v_splats((maxLightDistForTreeShadow * 2.0)),
                          reinterpret_cast<vec4f &>(rendinstgenrender::dir_from_sun));
                        vec3f far_point = v_mul(v_add(v_add(treeBBox.bmax, treeBBox.bmin), lightDist), V_C_HALF);
                        occBmin = v_min(occBmin, far_point);
                        occBmax = v_max(occBmax, far_point);
                      }

                      if (pool.hasImpostor())
                      {
                        // impostor is offsetted by cylinder_radius in vshader so check sphere occlusion instead
                        vec3f sphCenter = v_add(v_pos, v_make_vec4f(0.0f, pool.sphCenterY, 0.0f, 0.0f));
                        vec4f radius = v_mul(v_splats(pool.sphereRadius), v_scale);
                        if (use_occlusion->isOccludedSphere(sphCenter, radius))
                          continue;
                      }
                      else if (use_occlusion->isOccludedBox(occBmin, occBmax))
                        continue;
                    }
                    int lodI;
                    float instance_dist2 = crossDissolveForPool
                                             ? v_extract_x(v_length3_sq_x(v_sub(v_pos, curViewPos)))
                                             : v_extract_x(v_distance_sq_to_bbox_x(treeBBox.bmin, treeBBox.bmax, curViewPos));
                    // if (instance_dist2 >= farLodEndRangeSq)
                    //   continue;
                    for (lodI = visibility.PI_ALPHA_LOD; lodI > 0; --lodI) // alphaFarLodNo!
                      if (lodI != visibility.PI_DISSOLVE_LOD0 && instance_dist2 > lodDistancesSq_perInst[lodI])
                        break;
                    rtData->riRes[ri_idx]->updateReqLod(min<int>(lodI, rtData->riRes[ri_idx]->lods.size() - 1));
                    if (lodI < rtData->riRes[ri_idx]->getQlBestLod())
                      lodI = rtData->riRes[ri_idx]->getQlBestLod();

                    vec4f v_packed_data = rendinstgen::pack_entity_data(v_pos, v_scale, v_palette_id);
                    int instanceLod = lodI == visibility.PI_DISSOLVE_LOD1 ? visibility.PI_LAST_MESH_LOD : lodI;
                    if (lodI == RiGenVisibility::PI_DISSOLVE_LOD1 || lodI == RiGenVisibility::PI_DISSOLVE_LOD0)
                    {
                      if (lodI == visibility.PI_DISSOLVE_LOD1)
                      {
                        visibility.addTreeInstance(ri_idx, perInstanceData[RiGenVisibility::PI_DISSOLVE_LOD0], v_packed_data,
                          RiGenVisibility::PI_DISSOLVE_LOD0);
                        visibility.addTreeInstance(ri_idx, perInstanceData[RiGenVisibility::PI_DISSOLVE_LOD1], v_packed_data,
                          RiGenVisibility::PI_DISSOLVE_LOD1);
                      }
                    }
                    else if (pool_front_to_back && (lodI <= RiGenVisibility::PI_DISSOLVE_LOD1) &&
                             perInstanceDistanceCnt[instanceLod] < perInstanceDistance[instanceLod].size())
                    {
                      sortedInstances[instanceLod][perInstanceDistanceCnt[instanceLod]] = v_packed_data;
                      int distance = bitwise_cast<int, float>(instance_dist2);

                      if (forShadow) // we shall sort from front to back
                        distance = v_extract_xi(v_cvt_vec4i(v_dot3(((vec4f &)rendinstgenrender::dir_from_sun), v_pos)));

                      perInstanceDistance[instanceLod][perInstanceDistanceCnt[instanceLod]] =
                        IPoint2(perInstanceDistanceCnt[instanceLod], distance);
                      perInstanceDistanceCnt[instanceLod]++;
                      if (lodI == visibility.PI_DISSOLVE_LOD1)
                      {
                        visibility.addTreeInstance(ri_idx, perInstanceData[lodI], v_packed_data, lodI); // lod 1 dissappear, lod0 as is
                      }
                    }
                    else
                    {
                      visibility.addTreeInstance(ri_idx, perInstanceData[lodI], v_packed_data, lodI);
                      if (lodI == visibility.PI_DISSOLVE_LOD1)
                        visibility.addTreeInstance(ri_idx, perInstanceData[visibility.PI_LAST_MESH_LOD], v_packed_data,
                          visibility.PI_LAST_MESH_LOD); // lod 1 dissappear, lod0 as is
                      else if (lodI == visibility.PI_DISSOLVE_LOD0)
                        visibility.addTreeInstance(ri_idx, perInstanceData[visibility.PI_IMPOSTOR_LOD], v_packed_data,
                          visibility.PI_IMPOSTOR_LOD); // lod 0 dissappear, lod1 as is
                    }

                    visibility.instNumberCounter[remap_per_instance_lod_inv(lodI)]++;
                    if (lodI >= RiGenVisibility::PI_IMPOSTOR_LOD) // PI_ALPHA_LOD also requires an impostor texture.
                    {
                      riRenderRanges[ri_idx].vismask |= VIS_HAS_IMPOSTOR;
                    }
                  }
                }
                continue;
              }
            }
            // todo:replace to binary search?
            for (int lodI = pool.hasImpostor() ? farLodNo : alphaFarLodNo, lodE = rtData->riResFirstLod(ri_idx); lodI >= lodE; --lodI)
            {
              if (subCellDistSq > lodDistancesSq[lodI + lodTranslation] || lodI == lodE)
              {
                if (forShadow && lodI == alphaFarLodNo)
                  break;
                // add range to lodI
                if (cellAdded[lodI] < 0)
                  cellAdded[lodI] = visibility.addCell(lastLod = lodI, x, z, startVbOfs);
                visibility.addRange(lodI, sc.ofs, sc.sz);
                G_ASSERT(sc.sz % visibility.stride == 0);
                visibility.instNumberCounter[lodI] += sc.sz / visibility.stride;
                break;
              }
            }
          }
        }
        if (lastLod >= 0)
          visibility.closeRanges(lastLod);
      }
      for (int lodI = 0; lodI < perInstanceDistanceCnt.size(); ++lodI)
      {
        if (!perInstanceDistanceCnt[lodI])
          continue;
        stlsort::sort(perInstanceDistance[lodI].data(), perInstanceDistance[lodI].data() + perInstanceDistanceCnt[lodI], SortByY());
        int perInstanceStart = visibility.addTreeInstances(ri_idx, perInstanceData[lodI], NULL, perInstanceDistanceCnt[lodI], lodI);
        vec4f *dest = &visibility.instanceData[lodI][perInstanceStart];
        for (int i = 0; i < perInstanceDistanceCnt[lodI]; ++i, dest++)
          *dest = sortedInstances[lodI][perInstanceDistance[lodI][i].x];
      }
    }
    for (int lodI = 0; lodI < visibility.instanceData.size(); ++lodI)
    {
      visibility.max_per_instance_instances =
        max(visibility.max_per_instance_instances, (int)visibility.instanceData[lodI].size() - prevInstancesCount[lodI]);
      prevInstancesCount[lodI] = visibility.instanceData[lodI].size();
    }
    visibility.endRenderRange(ri_idx);
    riRenderRanges[ri_idx].vismask = 0;
    for (int lodI = 0; lodI < alphaFarLodNo; ++lodI)
    {
      if (riRenderRanges[ri_idx].hasCells(lodI))
      {
        riRenderRanges[ri_idx].vismask |= VIS_HAS_OPAQUE;
        break;
      }
    }
    riRenderRanges[ri_idx].vismask |= riRenderRanges[ri_idx].hasCells(alphaFarLodNo) ? VIS_HAS_TRANSP : 0;

    if (pool.hasImpostor())
    {
      riRenderRanges[ri_idx].vismask |=
        ((riRenderRanges[ri_idx].vismask & VIS_HAS_TRANSP) || riRenderRanges[ri_idx].hasCells(alphaFarLodNo - 1) ||
          visibility.perInstanceVisibilityCells[visibility.PI_IMPOSTOR_LOD].size() > 0 ||
          visibility.perInstanceVisibilityCells[visibility.PI_ALPHA_LOD].size() > 0) // PI_ALPHA_LOD also requires an impostor texture.
          ? VIS_HAS_IMPOSTOR
          : 0;
    }

    visibility.vismask |= riRenderRanges[ri_idx].vismask;
  }

  if (rendinstgenrender::per_instance_visibility)
  {
    visibility.closeTreeInstances();
    visibility.vismask |= (visibility.perInstanceVisibilityCells[visibility.PI_ALPHA_LOD].size() > 0) ? VIS_HAS_TRANSP : 0;
    visibility.vismask |=
      ((visibility.vismask & VIS_HAS_TRANSP) || visibility.perInstanceVisibilityCells[visibility.PI_IMPOSTOR_LOD].size() > 0)
        ? VIS_HAS_IMPOSTOR
        : 0;
    for (int i = 0; i < visibility.PI_ALPHA_LOD; ++i)
      visibility.vismask |= (visibility.perInstanceVisibilityCells[i].size() > 0) ? VIS_HAS_OPAQUE : 0;
  }
  return visibility.vismask != 0;
}


static void switch_states_for_impostors(RenderStateContext &context, bool is_impostor, bool prev_was_impostor)
{
  if (is_impostor)
  {
    context.curVertexData = NULL;
#if D3D_HAS_QUADS
    d3d::setind(NULL);
    d3d::setvdecl(BAD_VDECL);
    d3d::setvsrc(0, NULL, 0);
#else
    index_buffer::use_quads_16bit();
    d3d::setvdecl(BAD_VDECL);
    d3d::setvsrc(0, NULL, 0);
#endif
  }
  else
  {
    if (prev_was_impostor)
    {
      d3d::setind(unitedvdata::riUnitedVdata.getIB());
      context.curVertexData = nullptr;
      context.curVertexBuf = nullptr;
    }
  }
}

eastl::array<uint32_t, 2> rendinstgenrender::getCommonImmediateConstants()
{
  const auto halfFaloffStart = static_cast<uint32_t>(float_to_half(treeCrownTransmittance.falloffStart));
  const auto halfBrightness = static_cast<uint32_t>(float_to_half(treeCrownTransmittance.brightness));
  const auto halfCascade0Dist = static_cast<uint32_t>(float_to_half(cascade0Dist));
  const auto halfFalloffStop = static_cast<uint32_t>(float_to_half(treeCrownTransmittance.falloffStop));
  return {(halfFaloffStart << 16) | halfBrightness, (halfCascade0Dist << 16) | halfFalloffStop};
}

inline void RendInstGenData::renderInstances(int ri_idx, int realLodI, const vec4f *data, int count, RenderStateContext &context,
  const int max_instances, const int atest_skip_mask, const int last_stage)
{
  if (rtData->isHiddenId(ri_idx))
  {
#if 0
    const vec4f *curData = data;
    for (int cc = 0; cc < count; cc++)
    {
      Point3 pos;
      v_st(&pos.x, curData[cc]);
      debug("hidden ri %s [%d] " FMT_P3, rtData->riResName[ri_idx], cc, P3D(pos));
    }
#endif
    return;
  }
  int lodTranslation = rendinst::MAX_LOD_COUNT - rtData->riResLodCount(ri_idx);
  if (lodTranslation > 0 && !rtData->rtPoolData[ri_idx]->hasImpostor())
    lodTranslation--;

  RenderableInstanceResource *rendInstRes = rtData->riResLodScene(ri_idx, realLodI - lodTranslation);
  uint32_t atestMask = rtData->riResElemMask[ri_idx * rendinst::MAX_LOD_COUNT + realLodI - lodTranslation].atest;
  ShaderMesh *mesh = rendInstRes->getMesh()->getMesh()->getMesh();
  dag::Span<ShaderMesh::RElem> elems = mesh->getElems(mesh->STG_opaque, last_stage);

  int debugValue = realLodI - lodTranslation;
#if DAGOR_DBGLEVEL > 0
  if (debug_mesh::is_enabled(debug_mesh::Type::drawElements))
  {
    debugValue = 0;
    uint32_t atestMaskTmp = atestMask;
    for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++, atestMask >>= 1)
    {
      if (!elems[elemNo].e)
        continue;
      if ((atestMaskTmp & 1) == atest_skip_mask)
        continue;
      debugValue++;
    }
  }
#endif
  debug_mesh::set_debug_value(debugValue);

  G_ASSERT(count > 0);
  if (count <= max_instances)
  {
    rendinstgenrender::RiShaderConstBuffers::setInstancePositions((const float *)data, count);
  }

  bool prev_impostor = true;
  const bool isBakedImpostor = rtData->riRes[ri_idx]->isBakedImpostor();
  const bool isImpostor = !isBakedImpostor && (realLodI == remap_per_instance_lod_inv(RiGenVisibility::PI_IMPOSTOR_LOD));

  for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++, atestMask >>= 1)
  {
    if (!elems[elemNo].e)
      continue;
    if ((atestMask & 1) == atest_skip_mask)
      continue;

    switch_states_for_impostors(context, isImpostor, prev_impostor);
    prev_impostor = isImpostor;

    ShaderMesh::RElem &elem = elems[elemNo];
    if (realLodI <= RiGenVisibility::PI_LAST_MESH_LOD || isBakedImpostor || context.curShader != elem.e)
    {
      SWITCH_STATES_SHADER() // Render with original vdecl without any instancing elements.
      SWITCH_STATES_VDATA()
    }
    else
    {
      G_ASSERT(rtData->riPosInst[ri_idx]); // assumption that we draw with impostor and don't need to switch shader
    }
    if (rtData->rtPoolData[ri_idx]->hasImpostor() && !isImpostor)
    {
      const auto tcConsts = rendinstgenrender::getCommonImmediateConstants();
      const auto cacheOffset =
        static_cast<uint32_t>(rtData->rtPoolData[ri_idx]->impostorDataOffsetCache & rendinstgen::CACHE_OFFSET_MASK);
      uint32_t immediateConsts[] = {cacheOffset, tcConsts[0], tcConsts[1]};
      d3d::set_immediate_const(STAGE_PS, immediateConsts, sizeof(immediateConsts) / sizeof(immediateConsts[0]));
    }
    if (count <= max_instances)
    {
      if (isImpostor)
        render_impostors_ofs(count, 0, 0);
      else
        d3d_err(elem.drawIndTriList(count));
    }
    else
    {
      const vec4f *curData = data;
      for (int cnt = count; cnt > 0; cnt -= max_instances, curData += max_instances)
      {
        int currentCount = min<int>(max_instances, cnt);
        rendinstgenrender::RiShaderConstBuffers::setInstancePositions((const float *)curData, currentCount);

        if (isImpostor)
          render_impostors_ofs(currentCount, 0, 0);
        else
        {
          d3d_err(elem.drawIndTriList(currentCount));
        }
      }
    }
    if (rtData->rtPoolData[ri_idx]->hasImpostor() && !isImpostor)
      d3d::set_immediate_const(STAGE_PS, nullptr, 0);
  }
  debug_mesh::reset_debug_value();
}

void RendInstGenData::renderPerInstance(rendinst::RenderPass render_pass, int lodI, int realLodI, const RiGenVisibility &visibility)
{
  rendinstgenrender::RiShaderConstBuffers cb;
  cb.setOpacity(0.f, 2.f);
  d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);

  RenderStateContext context;
  bool isAlpha = false;
  if (lodI == visibility.PI_ALPHA_LOD)
    isAlpha = true;
  const int max_instances = rendinstgenrender::MAX_INSTANCES;
  const int last_stage = (render_pass == rendinst::RenderPass::Normal) ? ShaderMesh::STG_imm_decal : ShaderMesh::STG_atest;

  for (int i = 0; i < (int)visibility.perInstanceVisibilityCells[lodI].size() - 1; ++i)
  {
    int ri_idx = visibility.perInstanceVisibilityCells[lodI][i].x;
    G_ASSERT(rtData->rtPoolData[ri_idx]);
    if (!rtData->rtPoolData[ri_idx])
      continue;
    G_ASSERTF(rtData->riPosInst[ri_idx], "rgLayer[%d]: non-posInst ri[%d]=%s", rtData->layerIdx, ri_idx,
      rtData->riResName[ri_idx]); // one assumption
    G_ASSERT(rtData->rtPoolData[ri_idx]->hasImpostor());

    cb.setInstancing(0, 1,
      rendinstgen::get_rotation_palette_manager()->getImpostorDataBufferOffset({rtData->layerIdx, ri_idx},
        rtData->rtPoolData[ri_idx]->impostorDataOffsetCache));

    if (render_pass == rendinst::RenderPass::Normal)
      cb.setRandomColors(&rtData->riColPair[ri_idx * 2 + 0]);

    float range = rtData->get_trees_last_range(rtData->rtPoolData[ri_idx]->lodRange[rtData->riResLodCount(ri_idx) - 1]);
    float deltaRcp = rtData->transparencyDeltaRcp / range;

    if (visibility.forcedLod >= 0)
    {
      cb.setOpacity(0.f, 1.f, 0.f, 0.f);
    }
    else if (isAlpha)
    {
      if (render_pass == rendinst::RenderPass::Normal || render_pass == rendinst::RenderPass::Depth)
      {
        if (render_pass == rendinst::RenderPass::Normal)
        {
          float end_min_start = range / rtData->transparencyDeltaRcp;
          float start_opacity = range - end_min_start;
          float shadow_range = start_opacity * SHADOW_APPEAR_PART;
          cb.setOpacity(-deltaRcp, range * deltaRcp, -(1.f / shadow_range), 1.f / SHADOW_APPEAR_PART);
        }
        else
        {
          cb.setOpacity(-deltaRcp, range * deltaRcp, 0.f, 0.f);
        }
      }
    }
    else if (render_pass == rendinst::RenderPass::ToShadow)
    {
      cb.setOpacity(-deltaRcp, range * deltaRcp, 0.f, 0.f);
    }

    if (lodI > visibility.PI_LAST_MESH_LOD || (rtData->rtPoolData[ri_idx]->hasTransitionLod() && lodI == visibility.PI_LAST_MESH_LOD))
    {
      rtData->rtPoolData[ri_idx]->setImpostor(cb, render_pass == rendinst::RenderPass::ToShadow,
        rtData->riRes[ri_idx]->getPreshadowTexture());
    }
    else
      rtData->rtPoolData[ri_idx]->setNoImpostor(cb);
    cb.setInteractionParams(1,
      rtData->riRes[ri_idx]->hasImpostor() ? rtData->riRes[ri_idx]->bsphRad
                                           : rtData->riRes[ri_idx]->bbox.lim[1].y - rtData->riRes[ri_idx]->bbox.lim[0].y,
      0.5 * (rtData->riRes[ri_idx]->bbox.lim[1].x + rtData->riRes[ri_idx]->bbox.lim[0].x),
      0.5 * (rtData->riRes[ri_idx]->bbox.lim[1].z + rtData->riRes[ri_idx]->bbox.lim[0].z));

    cb.flushPerDraw();

    int start = visibility.perInstanceVisibilityCells[lodI][i].y;
    int count = visibility.perInstanceVisibilityCells[lodI][i + 1].y - start;
    G_ASSERT(visibility.max_per_instance_instances >= count);
    renderInstances(ri_idx, realLodI, &visibility.instanceData[lodI][start], count, context, max_instances, visibility.atest_skip_mask,
      last_stage);
  }
}

void RendInstGenData::renderCrossDissolve(rendinst::RenderPass render_pass, int lodI, int realLodI, const RiGenVisibility &visibility)
{
  ShaderGlobal::set_int(rendinstgenrender::render_cross_dissolved_varId, 1);
  RenderStateContext context;

  rendinstgenrender::RiShaderConstBuffers cb;
  const int max_instances = rendinstgenrender::MAX_INSTANCES;
  const int last_stage = (render_pass == rendinst::RenderPass::Normal) ? ShaderMesh::STG_imm_decal : ShaderMesh::STG_atest;
  d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);

  for (int i = 0; i < (int)visibility.perInstanceVisibilityCells[lodI].size() - 1; ++i)
  {
    int ri_idx = visibility.perInstanceVisibilityCells[lodI][i].x;
    G_ASSERT(rtData->riPosInst[ri_idx]); // one assumption
    G_ASSERT(rtData->rtPoolData[ri_idx]->hasImpostor());

    cb.setInstancing(0, 1,
      rendinstgen::get_rotation_palette_manager()->getImpostorDataBufferOffset({rtData->layerIdx, ri_idx},
        rtData->rtPoolData[ri_idx]->impostorDataOffsetCache));

    if (render_pass == rendinst::RenderPass::Normal)
      cb.setRandomColors(&rtData->riColPair[ri_idx * 2 + 0]);

    float lodStartRange = visibility.crossDissolveRange[ri_idx];
    lodStartRange = safediv(lodStartRange, rendinstgenrender::lodsShiftDistMul);
    float lodDissolveInvRange = 1.0f / TOTAL_CROSS_DISSOLVE_DIST;

    if (render_pass == rendinst::RenderPass::ToShadow)
    {
      float range = rtData->get_trees_last_range(rtData->rtPoolData[ri_idx]->lodRange[rtData->riResLodCount(ri_idx) - 1]);
      float deltaRcp = rtData->transparencyDeltaRcp / range;
      cb.setOpacity(-deltaRcp, range * deltaRcp, lodDissolveInvRange, -lodStartRange * lodDissolveInvRange);
    }
    else
    {
      cb.setOpacity(0., 1., lodDissolveInvRange, -lodStartRange * lodDissolveInvRange);
    }
    if (lodI > RiGenVisibility::PI_LAST_MESH_LOD)
    {
      rtData->rtPoolData[ri_idx]->setImpostor(cb, render_pass == rendinst::RenderPass::ToShadow,
        rtData->riRes[ri_idx]->getPreshadowTexture());
    }
    else
      rtData->rtPoolData[ri_idx]->setNoImpostor(cb);

    cb.setInteractionParams(1,
      rtData->riRes[ri_idx]->hasImpostor() ? rtData->riRes[ri_idx]->bsphRad
                                           : rtData->riRes[ri_idx]->bbox.lim[1].y - rtData->riRes[ri_idx]->bbox.lim[0].y,
      0.5 * (rtData->riRes[ri_idx]->bbox.lim[1].x + rtData->riRes[ri_idx]->bbox.lim[0].x),
      0.5 * (rtData->riRes[ri_idx]->bbox.lim[1].z + rtData->riRes[ri_idx]->bbox.lim[0].z));

    cb.flushPerDraw();

    int start = visibility.perInstanceVisibilityCells[lodI][i].y;
    int count = visibility.perInstanceVisibilityCells[lodI][i + 1].y - start;
    G_ASSERT(visibility.max_per_instance_instances >= count);
    renderInstances(ri_idx, realLodI, &visibility.instanceData[lodI][start], count, context, max_instances, visibility.atest_skip_mask,
      last_stage);
  }
  ShaderGlobal::set_int(rendinstgenrender::render_cross_dissolved_varId, 0);
}

void RendInstGenData::renderOptimizationDepthPrepass(const RiGenVisibility &visibility)
{
  if (!visibility.hasOpaque() || !rendinstgenrender::per_instance_visibility)
    return;
  for (int lodI = 0; lodI < min<int>(rendinst::MAX_LOD_COUNT, RiGenVisibility::PI_DISSOLVE_LOD1); ++lodI)
  {
    if (!visibility.perInstanceVisibilityCells[lodI].size())
      continue;
    TIME_D3D_PROFILE_NAME(riOptimizationPrepass,
      rendinst::isRgLayerPrimary(rtData->layerIdx) ? "ri_depth_prepass" : "ri_depth_prepass_sec");
    ShaderGlobal::setBlock(rendinstgenrender::rendinstDepthSceneBlockId, ShaderGlobal::LAYER_SCENE);
    const rendinst::RenderPass renderPass = rendinst::RenderPass::Depth; // even rendinst::RenderPass::Normal WILL optimize pass, due
                                                                         // to color_write off
    ShaderGlobal::set_int_fast(rendinstgenrender::rendinstRenderPassVarId, eastl::to_underlying(renderPass));

    rendinstgenrender::startRenderInstancing();
    rendinstgenrender::setCoordType(rendinstgenrender::COORD_TYPE_POS_CB);
    const int realLodI = remap_per_instance_lod_inv(lodI);
    renderPerInstance(renderPass, lodI, realLodI, visibility);
    rendinstgenrender::endRenderInstancing();
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
  }

  if (rendinstgenrender::depth_prepass_for_cells)
  {
    ShaderGlobal::setBlock(rendinstgenrender::rendinstDepthSceneBlockId, ShaderGlobal::LAYER_SCENE);
    ShaderGlobal::set_int_fast(rendinstgenrender::rendinstRenderPassVarId, eastl::to_underlying(rendinst::RenderPass::Depth));
    renderByCells(rendinst::RenderPass::Depth, 0, visibility, true, false);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
  }
}

void rendinst::applyBurningDecalsToRi(const bbox3f &decal_bbox)
{
  for (auto *rgl : rendinst::rgLayer)
    if (rgl)
      rgl->applyBurnDecal(decal_bbox);
}

void RendInstGenData::sortRIGenVisibility(RiGenVisibility &visibility, const Point3 &viewPos, const Point3 &viewDir, float vertivalFov,
  float horizontalFov, float areaThreshold)
{
  if (!visibility.hasOpaque() || !rendinstgenrender::per_instance_visibility || fabsf(viewDir.y) > 0.5f)
    return;

  vec4f pos = v_make_vec4f(viewPos.x, 0.0f, viewPos.z, 0.0f);
  vec4f dir = v_make_vec4f(viewDir.x, 0.0f, viewDir.z, 0.0f);
  dir = v_norm3(dir);
  float tanV = tanf(vertivalFov);
  float tanH = tanf(horizontalFov);

  struct Instance
  {
    vec4f data;
    int ri_idx;
    float area;
    float distance;
  };
  eastl::vector<Instance, framemem_allocator> instances;

  for (int lodI = 0; lodI < min<int>(rendinst::MAX_LOD_COUNT, RiGenVisibility::PI_DISSOLVE_LOD1); ++lodI)
  {
    if (visibility.perInstanceVisibilityCells[lodI].empty())
      continue;

    instances.clear();
    instances.reserve(visibility.instanceData[lodI].size());

    for (int i = 0; i < static_cast<int>(visibility.perInstanceVisibilityCells[lodI].size()) - 1; ++i)
    {
      int ri_idx = visibility.perInstanceVisibilityCells[lodI][i].x;
      int start = visibility.perInstanceVisibilityCells[lodI][i].y;
      int count = visibility.perInstanceVisibilityCells[lodI][i + 1].y - start;

      Point3 diagonal = rtData->riRes[ri_idx]->bbox.width();
      float riHalfHeight = diagonal.y / 2.0f;
      float riHalfWidth = max(diagonal.x, diagonal.z) / 2.0f;

      for (int j = start; j < start + count; ++j)
      {
        Instance &instance = instances.emplace_back();
        instance.ri_idx = ri_idx;
        instance.data = visibility.instanceData[lodI][j];
        instance.distance = v_extract_x(v_dot3_x(dir, v_sub(instance.data, pos)));

        if (fabsf(instance.distance) < riHalfWidth)
          instance.area = 1.0f;
        else if (instance.distance < 0.0f)
          instance.area = 0.0f;
        else
          instance.area = min(riHalfHeight / (tanV * instance.distance), 1.0f) * min(riHalfWidth / (tanH * instance.distance), 1.0f);
      }
    }

    auto split =
      eastl::partition(instances.begin(), instances.end(), [areaThreshold](const Instance &a) { return a.area > areaThreshold; });

    eastl::sort(instances.begin(), split, [](const Instance &a, const Instance &b) { return a.distance < b.distance; });

    eastl::sort(split, instances.end(),
      [](const Instance &a, const Instance &b) { return a.ri_idx == b.ri_idx ? a.distance < b.distance : a.ri_idx < b.ri_idx; });

    visibility.perInstanceVisibilityCells[lodI].clear();
    visibility.instanceData[lodI].clear();

    int start = 0;
    for (int i = 0; i < static_cast<int>(instances.size()) - 1; ++i)
    {
      visibility.instanceData[lodI].push_back(instances[i].data);

      if (instances[i].ri_idx != instances[i + 1].ri_idx)
      {
        visibility.perInstanceVisibilityCells[lodI].push_back({instances[i].ri_idx, start});
        start = i + 1;
      }
    }

    visibility.instanceData[lodI].push_back(instances.back().data);
    visibility.perInstanceVisibilityCells[lodI].push_back({instances.back().ri_idx, start});
    visibility.perInstanceVisibilityCells[lodI].push_back({-1, static_cast<int>(instances.size())});
  }
}

void RendInstGenData::renderPreparedOpaque(rendinst::RenderPass render_pass, const RiGenVisibility &visibility, bool depth_optimized,
  int lodI, int realLodI, bool &isStarted)
{
  if (!visibility.perInstanceVisibilityCells[lodI].size())
    return;
  if (!isStarted)
  {
    rendinstgenrender::startRenderInstancing();
    rendinstgenrender::setCoordType(rendinstgenrender::COORD_TYPE_POS_CB);
    isStarted = true;
  }
  if (lodI == visibility.PI_DISSOLVE_LOD1 || lodI == visibility.PI_DISSOLVE_LOD0)
    renderCrossDissolve(render_pass, lodI, realLodI, visibility);
  else
  {
    if (lodI <= RiGenVisibility::PI_LAST_MESH_LOD && depth_optimized)
      shaders::overrides::set(rendinstgenrender::afterDepthPrepassOverride);
    renderPerInstance(render_pass, lodI, realLodI, visibility);
    if (lodI <= RiGenVisibility::PI_LAST_MESH_LOD && depth_optimized)
      shaders::overrides::reset();
  }
}

void RendInstGenData::updateHeapVb()
{
  TIME_D3D_PROFILE(updateHeapVb);

  ScopedLockWrite lock(rtData->riRwCs);
  auto currentHeapGen = rtData->cellsVb.getManager().getHeapGeneration();
  dag::ConstSpan<int> ld = rtData->loaded.getList();
  for (auto ldi : ld)
  {
    RendInstGenData::Cell &cell = cells[ldi];
    RendInstGenData::CellRtData *crt_ptr = cell.isReady();
    if (!crt_ptr)
      continue;
    RendInstGenData::CellRtData &crt = *crt_ptr;

    if (crt.heapGen != currentHeapGen)
      updateVb(crt, ldi);
  }
}

void RendInstGenData::renderByCells(rendinst::RenderPass render_pass, const uint32_t layer_flags, const RiGenVisibility &visibility,
  bool optimization_depth_prepass, bool depth_optimized)
{
  TIME_D3D_PROFILE_NAME(render_ri_by_cells, (optimization_depth_prepass ? "render_ri_by_cells_depth" : "render_ri_by_cells"));

  bool compatibilityMode = ::dgs_get_settings()->getBlockByNameEx("video")->getBool("compatibilityMode", false);
  float grid2worldcellSz = grid2world * cellSz;

  ScopedLockRead lock(rtData->riRwCs);

  rendinstgenrender::startRenderInstancing();
  ShaderGlobal::set_int_fast(isRenderByCellsVarId, 1);

  RenderStateContext context;
  bool isDepthPass = render_pass == rendinst::RenderPass::Depth || render_pass == rendinst::RenderPass::ToShadow;
  const bool optimizeDepthPass = isDepthPass;
  bool lastAtest = false;
  bool lastPosInst = false;
  bool lastCullNone = 0;
  bool lastBurned = false;
  int lastCellId = -1;
  // render opaque
  int wasImpostorType = -1;

  rendinstgenrender::RiShaderConstBuffers cb;
  cb.setOpacity(0.f, 2.f);

  float subCellOfsSize = grid2worldcellSz * ((rendinstgenrender::per_instance_visibility_for_everyone ? 0.75f : 0.25f) *
                                              (rendinstgenrender::globalDistMul * 1.f / RendInstGenData::SUBCELL_DIV));


  dag::ConstSpan<uint16_t> riResOrder = rtData->riResOrder;
  if (render_pass == rendinst::RenderPass::Normal && layer_flags == rendinst::LAYER_DECALS)
    riResOrder = rtData->riResIdxPerStage[get_const_log2(rendinst::LAYER_DECALS)];

#if !_TARGET_STATIC_LIB
  eastl::vector<unsigned, framemem_allocator> redirectionVector;
  eastl::vector<uint8_t, framemem_allocator> drawOrders;
#endif

  rtData->updateVbResetCS.lock();
  d3d::set_buffer(STAGE_VS, rendinstgenrender::INSTANCING_TEXREG, rtData->cellsVb.getHeap().getBuf());
  auto currentHeapGen = rtData->cellsVb.getManager().getHeapGeneration();
  G_UNUSED(currentHeapGen);
  LinearHeapAllocatorSbuffer::Region lastInfo = {};

  for (unsigned int ri_idx2 = 0; ri_idx2 < riResOrder.size(); ri_idx2++)
  {
    int ri_idx = riResOrder[ri_idx2];

    if (!rtData->rtPoolData[ri_idx])
      continue;
    if (rtData->isHiddenId(ri_idx))
      continue;

    if (rendinst::isResHidden(rtData->riResHideMask[ri_idx]))
      continue;

    if (!visibility.renderRanges[ri_idx].hasOpaque() && !visibility.renderRanges[ri_idx].hasTransparent())
      continue;

    if (rtData->rtPoolData[ri_idx]->hasImpostor() &&
        (render_pass == rendinst::RenderPass::Normal && layer_flags == rendinst::LAYER_DECALS))
      continue;

    bool posInst = rtData->riPosInst[ri_idx] ? 1 : 0;
    unsigned int stride = RIGEN_STRIDE_B(posInst, perInstDataDwords);
    const uint32_t vectorsCnt = posInst ? 1 : 3 + RIGEN_ADD_STRIDE_PER_INST_B(perInstDataDwords) / RENDER_ELEM_SIZE_PACKED;

    rendinstgenrender::setCoordType(posInst ? rendinstgenrender::COORD_TYPE_POS : rendinstgenrender::COORD_TYPE_TM);
    float range = rtData->rtPoolData[ri_idx]->lodRange[rtData->riResLodCount(ri_idx) - 1];
    range = rtData->rtPoolData[ri_idx]->hasImpostor() ? rtData->get_trees_last_range(range) : rtData->get_last_range(range);
    float deltaRcp = rtData->transparencyDeltaRcp / range;

    // set dist to lod1 (impostors for trees)
    float initialLod0Range = rtData->rtPoolData[ri_idx]->lodRange[0];
    float lod0Range =
      rtData->rtPoolData[ri_idx]->hasImpostor() ? rtData->get_trees_range(initialLod0Range) : rtData->get_range(initialLod0Range);
    ShaderGlobal::set_real_fast(invLod0RangeVarId, 1.f / max(lod0Range + subCellOfsSize, 1.f));

    if (visibility.forcedLod >= 0)
    {
      cb.setOpacity(0.f, 1.f, 0.f, 0.f);
    }
    else if (render_pass == rendinst::RenderPass::Normal)
    {
      float end_min_start = range / rtData->transparencyDeltaRcp;
      float start_opacity = range - end_min_start;
      float shadow_range = start_opacity * SHADOW_APPEAR_PART;
      cb.setOpacity(-deltaRcp, range * deltaRcp, -(1.f / shadow_range), 1.f / SHADOW_APPEAR_PART);
    }
    else
    {
      cb.setOpacity(-deltaRcp, range * deltaRcp);
    }

    if (render_pass == rendinst::RenderPass::Normal)
      cb.setRandomColors(&rtData->riColPair[ri_idx * 2 + 0]);

    rtData->rtPoolData[ri_idx]->setNoImpostor(cb);

    ShaderGlobal::set_int_fast(rendinstgenrender::render_cross_dissolved_varId, 0);

    const bool isBakedImpostor = rtData->riRes[ri_idx]->isBakedImpostor();
    for (int lodI = rtData->riResFirstLod(ri_idx), lodCnt = rtData->riResLodCount(ri_idx); lodI <= lodCnt; ++lodI)
    {
      const bool isImpostorLod = lodI >= lodCnt - 1 && posInst ? 1 : 0;
      int isImpostorType = !isBakedImpostor && rtData->rtPoolData[ri_idx]->hasImpostor() && isImpostorLod;
      const bool isImpostor = (isImpostorType || (isBakedImpostor && isImpostorLod));
      if (!rendinstgenrender::depth_prepass_for_impostors && optimization_depth_prepass && isImpostor)
        continue;
      int realLodI = remap_per_instance_lod_inv(lodI);

      bool dissolveOut = false;
      if (lodI == lodCnt && (!rendinstgenrender::vertical_billboards || !compatibilityMode))
      {
        context.curShader = NULL;
        ShaderGlobal::set_int_fast(rendinstgenrender::render_cross_dissolved_varId, 1);
        dissolveOut = true;
      }

      G_ASSERT(lodI <= rendinst::MAX_LOD_COUNT);
      uint32_t atestMask = rtData->riResElemMask[ri_idx * rendinst::MAX_LOD_COUNT + realLodI].atest;
      uint32_t cullNoneMask = rtData->riResElemMask[ri_idx * rendinst::MAX_LOD_COUNT + realLodI].cullN;

      if (!visibility.hasCells(ri_idx, lodI))
        continue;
      RenderableInstanceResource *rendInstRes = rtData->riResLodScene(ri_idx, realLodI);
      ShaderMesh *mesh = rendInstRes->getMesh()->getMesh()->getMesh();
      dag::Span<ShaderMesh::RElem> elems =
        (render_pass == rendinst::RenderPass::Normal && layer_flags == rendinst::LAYER_DECALS)
          ? mesh->getElems(mesh->STG_decal)
          : mesh->getElems(mesh->STG_opaque, render_pass == rendinst::RenderPass::Normal ? mesh->STG_imm_decal : mesh->STG_atest);
      if (lodI == rtData->riResLodCount(ri_idx) - 1 || // impostor lod, all impostored rendinsts are put to impostor lod only to shadow
          (rtData->rtPoolData[ri_idx]->hasTransitionLod() && lodI == rtData->riResLodCount(ri_idx) - 2))
      {
        rtData->rtPoolData[ri_idx]->setImpostor(cb, render_pass == rendinst::RenderPass::ToShadow,
          rtData->riRes[ri_idx]->getPreshadowTexture());

        // all impostored rendinsts are rendered with impostor only
      }

      cb.setInstancing(0, vectorsCnt,
        rendinstgen::get_rotation_palette_manager()->getImpostorDataBufferOffset({rtData->layerIdx, ri_idx},
          rtData->rtPoolData[ri_idx]->impostorDataOffsetCache));

      cb.setInteractionParams(0, rtData->riRes[ri_idx]->bbox.lim[1].y - rtData->riRes[ri_idx]->bbox.lim[0].y,
        0.5 * (rtData->riRes[ri_idx]->bbox.lim[1].x + rtData->riRes[ri_idx]->bbox.lim[0].x),
        0.5 * (rtData->riRes[ri_idx]->bbox.lim[1].z + rtData->riRes[ri_idx]->bbox.lim[0].z));

      cb.flushPerDraw();

      const bool afterDepthPrepass =
        render_pass == rendinst::RenderPass::Normal && !isImpostor && depth_optimized && rendinstgenrender::depth_prepass_for_cells;

      if (afterDepthPrepass)
        shaders::overrides::set(rendinstgenrender::afterDepthPrepassOverride);

#if !_TARGET_STATIC_LIB
      bool isUsedRedirectionVector = VariableMap::isVariablePresent(drawOrderVarId) && render_pass == rendinst::RenderPass::Normal &&
                                     layer_flags == rendinst::LAYER_DECALS;

      if (isUsedRedirectionVector)
      {
        redirectionVector.resize(elems.size());

        // We assume that there only 3 draw_orders -1, 0, 1
        const uint32_t MAX_ORDER_VARIANTS = 3;
        eastl::array<uint32_t, MAX_ORDER_VARIANTS> orderStart = {0, 0, 0};

        drawOrders.resize(elems.size());
        for (int i = 0; i < elems.size(); ++i)
        {
          int drawOrder = 0;
          elems[i].mat->getIntVariable(drawOrderVarId, drawOrder);
          drawOrders[i] = sign(drawOrder) + 1;
          orderStart[drawOrders[i]]++;
        }

        for (uint32_t i = 1; i < orderStart.size(); ++i)
          orderStart[i] += orderStart[i - 1];
        for (uint32_t i = orderStart.size() - 1; i > 0; --i)
          orderStart[i] = orderStart[i - 1];
        orderStart[0] = 0;

        for (int i = 0; i < elems.size(); ++i)
        {
          int drawOrder = drawOrders[i];
          redirectionVector[orderStart[drawOrder]] = i;
          orderStart[drawOrder]++;
        }
      }
#endif

      int debugValue = realLodI;
#if DAGOR_DBGLEVEL > 0
      if (debug_mesh::is_enabled(debug_mesh::Type::drawElements))
      {
        debugValue = 0;
        for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++, atestMask >>= 1, cullNoneMask >>= 1)
        {
          if (!elems[elemNo].e)
            continue;
          if ((atestMask & 1) == visibility.atest_skip_mask)
            continue;
        }
      }
#endif
      debug_mesh::set_debug_value(debugValue);

#if !_TARGET_STATIC_LIB
      for (unsigned int elemRedir = 0; elemRedir < elems.size(); elemRedir++, atestMask >>= 1, cullNoneMask >>= 1)
      {
        unsigned int elemNo = (isUsedRedirectionVector) ? redirectionVector[elemRedir] : elemRedir;
#else
      for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++, atestMask >>= 1, cullNoneMask >>= 1)
      {
#endif
        if (!elems[elemNo].e)
          continue;
        if ((atestMask & 1) == visibility.atest_skip_mask)
          continue;
        ShaderMesh::RElem &elem = elems[elemNo];

        if (isImpostorType != wasImpostorType || !isImpostorType || !context.curShader)
        {
          if (isImpostorType != wasImpostorType)
            context.curShader = NULL;

          bool switchVDECL = false;
          if (optimizeDepthPass)
          {
            if (context.curShader && !(atestMask & 1) && !lastAtest && lastCullNone == (cullNoneMask & 1) && context.curShaderValid &&
                (posInst == lastPosInst))
            {
              context.curShader = elem.e;
            }
            else
              switchVDECL = true;
            lastAtest = atestMask & 1;
            lastCullNone = cullNoneMask & 1;
          }
          SWITCH_STATES_SHADER()

          if (switchVDECL && !lastAtest && !dissolveOut) // tex Instancing sets it's own vdecl all the time, no matter what
          {
            d3d::setvdecl(rendinstgenrender::rendinstDepthOnlyVDECL);
          }

          lastPosInst = posInst;

          switch_states_for_impostors(context, isImpostorType, wasImpostorType);
          wasImpostorType = isImpostorType;
        }

        if (!context.curShader)
        {
          G_ASSERT(0);
          continue;
        }

        for (int i = visibility.renderRanges[ri_idx].startCell[lodI], ie = visibility.renderRanges[ri_idx].endCell[lodI]; i < ie; i++)
        {
          int x = visibility.cellsLod[lodI][i].x;
          int z = visibility.cellsLod[lodI][i].z;
          int cellId = x + z * cellNumW;
          RendInstGenData::Cell &cell = cells[cellId];
          RendInstGenData::CellRtData *crt_ptr = cell.isReady();
          if (!crt_ptr)
            continue;
          RendInstGenData::CellRtData &crt = *crt_ptr;
          G_ASSERT(crt.cellVbId);

          bool burnedChanged = lastBurned != crt.burned;
          if (lastCellId != cellId || (burnedChanged && rtData->rtPoolData[ri_idx]->hasImpostor()))
          {
            if (!RENDINST_FLOAT_POS || burnedChanged)
            {
              cb.setInteractionParams(crt.burned ? 1 : 0, rtData->riRes[ri_idx]->bsphRad,
                0.5 * (rtData->riRes[ri_idx]->bbox.lim[1].x + rtData->riRes[ri_idx]->bbox.lim[0].x),
                0.5 * (rtData->riRes[ri_idx]->bbox.lim[1].z + rtData->riRes[ri_idx]->bbox.lim[0].z));

              rendinstgenrender::cell_set_encoded_bbox(cb, crt.cellOrigin, grid2worldcellSz, crt.cellHeight);
              cb.flushPerDraw();
            }

            lastBurned = cell.rtData->burned;
            lastCellId = cellId;
            lastInfo = rtData->cellsVb.get(crt.cellVbId);
            if (crt.heapGen != currentHeapGen) // driver is incapable of copy in thread
              updateVb(crt, cellId);
          }

          G_ASSERT(visibility.getRangesCount(lodI, i) > 0);
          for (int ri = 0, re = visibility.getRangesCount(lodI, i); ri < re; ++ri)
          {
#if DEBUG_RI
            instances += visibility.getCount(lodI, i, ri);
#endif
            if (!isImpostorType)
            {
              SWITCH_STATES_VDATA()
            }
            G_ASSERT(crt.cellVbId);

#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0
            d3d::driver_command(DRV3D_COMMAND_AFTERMATH_MARKER, (void *)rtData->riResName[ri_idx], /*copyname*/ (void *)(uintptr_t)1,
              NULL);
#endif

            const uint32_t ofs = visibility.getOfs(lodI, i, ri, stride);
            const int count = visibility.getCount(lodI, i, ri);

            G_ASSERT(ofs >= crt.pools[ri_idx].baseOfs);
            G_ASSERT(count <= crt.pools[ri_idx].total);
            G_ASSERT((count * stride + lastInfo.offset + ofs) <= rtData->cellsVb.getHeap().getBuf()->ressize());
            G_ASSERT((count * stride + ofs) <= lastInfo.size);
            G_ASSERT(count > 0);

            uint32_t vecOfs = lastInfo.offset / RENDER_ELEM_SIZE + ofs * vectorsCnt / stride;
            d3d::set_immediate_const(STAGE_VS, &vecOfs, 1);

            if (isImpostorType)
              render_impostors_ofs(count, vecOfs, vectorsCnt);
            else
              d3d_err(elem.drawIndTriList(count));
          }
        }
      }
      if (afterDepthPrepass)
        shaders::overrides::reset();
    }
  }

  rtData->updateVbResetCS.unlock();
  ShaderGlobal::set_int_fast(rendinstgenrender::render_cross_dissolved_varId, 0);

  rendinstgenrender::set_no_impostor_tex();

  debug_mesh::reset_debug_value();

  ShaderGlobal::set_int_fast(isRenderByCellsVarId, 0);

  rendinstgenrender::endRenderInstancing();
}

void RendInstGenData::renderPreparedOpaque(rendinst::RenderPass render_pass, uint32_t layer_flags, const RiGenVisibility &visibility,
  const TMatrix &view_itm, bool depth_optimized)
{
  TIME_D3D_PROFILE_NAME(render_prepared_opaque,
    rendinst::isRgLayerPrimary(rtData->layerIdx) ? "render_prepared_opaque" : "render_prepared_opaque_sec");

  if (!visibility.hasOpaque() && !visibility.hasTransparent())
    return;

#if DEBUG_RI
  int instances = 0;
#endif

  set_up_left_to_shader(view_itm);
  const bool needToSetBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE) == -1;
  if (needToSetBlock)
    ShaderGlobal::setBlock(rendinstgenrender::rendinstSceneBlockId, ShaderGlobal::LAYER_SCENE);
  ShaderGlobal::set_int_fast(rendinstgenrender::rendinstRenderPassVarId,
    eastl::to_underlying(render_pass)); // rendinst_render_pass_to_shadow.
  ShaderGlobal::set_int_fast(rendinstgenrender::baked_impostor_multisampleVarId, 1);
  ShaderGlobal::set_int_fast(rendinstgenrender::vertical_impostor_slicesVarId, rendinstgenrender::vertical_billboards ? 1 : 0);

  ShaderGlobal::set_real_fast(globalTranspVarId, 1.f); // Force not-transparent branch.
  if (rendinstgenrender::per_instance_visibility &&
      !(render_pass == rendinst::RenderPass::Normal && layer_flags == rendinst::LAYER_DECALS))
  {
    bool isStarted = false;
    for (int realLodI = 0; realLodI < rendinst::MAX_LOD_COUNT - 1; ++realLodI)
    {
      renderPreparedOpaque(render_pass, visibility, depth_optimized, realLodI, realLodI, isStarted);
    }
    // cross dissolve
    renderPreparedOpaque(render_pass, visibility, depth_optimized, RiGenVisibility::PI_DISSOLVE_LOD0,
      remap_per_instance_lod_inv(RiGenVisibility::PI_LAST_MESH_LOD), isStarted);
    renderPreparedOpaque(render_pass, visibility, depth_optimized, RiGenVisibility::PI_DISSOLVE_LOD1,
      remap_per_instance_lod_inv(RiGenVisibility::PI_IMPOSTOR_LOD), isStarted);
    // impostor
    renderPreparedOpaque(render_pass, visibility, depth_optimized, RiGenVisibility::PI_IMPOSTOR_LOD,
      remap_per_instance_lod_inv(RiGenVisibility::PI_IMPOSTOR_LOD), isStarted);
    // alpha
    renderPreparedOpaque(render_pass, visibility, depth_optimized, RiGenVisibility::PI_ALPHA_LOD,
      remap_per_instance_lod_inv(RiGenVisibility::PI_IMPOSTOR_LOD), isStarted);
    if (isStarted)
      rendinstgenrender::endRenderInstancing();
  }

  renderByCells(render_pass, layer_flags, visibility, false, depth_optimized);

  if (needToSetBlock)
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);

  ShaderGlobal::set_int_fast(rendinstgenrender::render_cross_dissolved_varId, 0);

#if DEBUG_RI
  debug("opaque instances = %d", instances);
#endif
}

void RendInstGenData::render(rendinst::RenderPass render_pass, const RiGenVisibility &visibility, const TMatrix &view_itm,
  uint32_t layer_flags, bool depth_optimized)
{
  if (layer_flags & (rendinst::LAYER_NOT_EXTRA | rendinst::LAYER_DECALS))
    renderPreparedOpaque(render_pass, layer_flags, visibility, view_itm, depth_optimized);
}

bool prepass_trees = true;
static bool render_sec_layer = true;

bool rendinst::enableSecLayerRender(bool en)
{
  if (en == render_sec_layer)
    return en; // return prev value

  if (en)
    rebuildRgRenderMasks();
  else
  {
    rgRenderMaskO &= (1 << rgPrimaryLayers) - 1;
    rgRenderMaskDS &= (1 << rgPrimaryLayers) - 1;
  }
  render_sec_layer = en;
  return !en; // return prev value
}

bool rendinst::isSecLayerRenderEnabled() { return render_sec_layer; }

bool rendinst::enablePrimaryLayerRender(bool en)
{
  static bool render_layer = true;
  if (en == render_layer)
    return en; // return prev value

  if (en)
    rebuildRgRenderMasks();
  else
  {
    rgRenderMaskO &= ~((1 << rgPrimaryLayers) - 1);
    rgRenderMaskDS &= ~((1 << rgPrimaryLayers) - 1);
  }
  render_layer = en;
  return !en; // return prev value
}

bool rendinst::enableRiExtraRender(bool en)
{
  bool v = rendinstgenrender::ri_extra_render_enabled;
  rendinstgenrender::ri_extra_render_enabled = en;
  return v;
}

CONSOLE_BOOL_VAL("render", gpu_objects_enable, true);

void rendinst::renderGpuObjectsLayer(RenderPass render_pass, const RiGenVisibility *visibility, uint32_t layer_flags, uint32_t layer)
{
  if (visibility[0].gpuObjectsCascadeId != -1 && gpu_objects_enable.get())
  {
    ShaderGlobal::set_int_fast(rendinstgenrender::useRiTrackdirtOffsetVarId, 1);
    rendinst::renderRIGenExtraFromBuffer(gpu_objects_mgr->getBuffer(visibility[0].gpuObjectsCascadeId, layer),
      gpu_objects_mgr->getObjectsOffsetsAndCount(visibility[0].gpuObjectsCascadeId, layer),
      gpu_objects_mgr->getObjectsIds(visibility[0].gpuObjectsCascadeId, layer),
      gpu_objects_mgr->getObjectsLodOffsets(visibility[0].gpuObjectsCascadeId, layer), render_pass,
      !(layer_flags & LAYER_FOR_GRASS) ? OptimizeDepthPass::Yes : OptimizeDepthPass::No, OptimizeDepthPrepass::No,
      IgnoreOptimizationLimits::No, layer_flags & layer, nullptr, 1,
      gpu_objects_mgr->getGpuInstancing(visibility[0].gpuObjectsCascadeId),
      gpu_objects_mgr->getIndirectionBuffer(visibility[0].gpuObjectsCascadeId),
      gpu_objects_mgr->getOffsetsBuffer(visibility[0].gpuObjectsCascadeId));
    ShaderGlobal::set_int_fast(rendinstgenrender::useRiTrackdirtOffsetVarId, 0);
  }
}

void rendinst::renderGpuObjectsFromVisibility(RenderPass render_pass, const RiGenVisibility *visibility, uint32_t layer_flags)
{
  if ((layer_flags & LAYER_OPAQUE) && rendinstgenrender::ri_extra_render_enabled)
  {
    renderGpuObjectsLayer(render_pass, visibility, layer_flags, LAYER_OPAQUE);
  }
}

void rendinst::renderRIGen(RenderPass render_pass, const RiGenVisibility *visibility, const TMatrix &view_itm, uint32_t layer_flags,
  OptimizeDepthPass depth_optimized, uint32_t instance_count_mul, AtestStage atest_stage, const RiExtraRenderer *riex_renderer,
  TexStreamingContext texCtx)
{
  if (!RendInstGenData::renderResRequired || RendInstGenData::isLoading || !unitedvdata::riUnitedVdata.getIB())
    return;

  G_ASSERT_RETURN(visibility, );

  const OptimizeDepthPass optimize_depth = !(layer_flags & LAYER_FOR_GRASS) ? OptimizeDepthPass::Yes : OptimizeDepthPass::No;

  bool depthOrShadowPass = (render_pass == rendinst::RenderPass::Depth) || (render_pass == rendinst::RenderPass::ToShadow);
  if (depthOrShadowPass)
    texCtx = TexStreamingContext(0);

  if (layer_flags & (rendinst::LAYER_OPAQUE | rendinst::LAYER_TRANSPARENT | rendinst::LAYER_RENDINST_CLIPMAP_BLEND |
                      rendinst::LAYER_RENDINST_HEIGHTMAP_PATCH | rendinst::LAYER_DISTORTION))
  {
    ShaderGlobal::set_int_fast(rendinstgenrender::rendinstRenderPassVarId, eastl::to_underlying(render_pass));
    if ((layer_flags & LAYER_OPAQUE) && rendinstgenrender::ri_extra_render_enabled)
    {
      rendinst::renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
        layer_flags & LAYER_OPAQUE, instance_count_mul, texCtx, atest_stage, riex_renderer);

      renderGpuObjectsLayer(render_pass, visibility, layer_flags, LAYER_OPAQUE);
    }

    if ((layer_flags & LAYER_RENDINST_CLIPMAP_BLEND) && rendinstgenrender::ri_extra_render_enabled)
      rendinst::renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
        layer_flags & LAYER_RENDINST_CLIPMAP_BLEND, instance_count_mul, texCtx);

    if ((layer_flags & LAYER_RENDINST_HEIGHTMAP_PATCH) && rendinstgenrender::ri_extra_render_enabled)
      rendinst::renderRIGenExtra(visibility[0], render_pass, OptimizeDepthPass::No, OptimizeDepthPrepass::No,
        IgnoreOptimizationLimits::No, layer_flags & LAYER_RENDINST_HEIGHTMAP_PATCH, instance_count_mul, texCtx, AtestStage::All);

    if ((layer_flags & LAYER_TRANSPARENT) && rendinstgenrender::ri_extra_render_enabled)
    {
      rendinst::renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
        layer_flags & LAYER_TRANSPARENT, instance_count_mul, texCtx);

      renderGpuObjectsLayer(render_pass, visibility, layer_flags, LAYER_TRANSPARENT);
    }

    if ((layer_flags & LAYER_DISTORTION) && rendinstgenrender::ri_extra_render_enabled)
    {
      rendinst::renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
        layer_flags & LAYER_DISTORTION, instance_count_mul, texCtx);

      renderGpuObjectsLayer(render_pass, visibility, layer_flags, LAYER_DISTORTION);
    }
  }

  const bool tree_depth_optimized = (depth_optimized == OptimizeDepthPass::Yes) && prepass_trees;
  if (layer_flags & LAYER_NOT_EXTRA)
  {
    ShaderGlobal::set_int_fast(rendinstgenrender::rendinstRenderPassVarId, eastl::to_underlying(render_pass));
    G_ASSERT(render_pass == rendinst::RenderPass::Normal || !tree_depth_optimized);
    rendinstgenrender::disableRendinstAlphaForNormalPassWithZPrepass();
    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      rgl->render(render_pass, visibility[_layer], view_itm, layer_flags & ~LAYER_DECALS, tree_depth_optimized);
    rendinstgenrender::restoreRendinstAlphaForNormalPassWithZPrepass();
  }

  if ((layer_flags & LAYER_DECALS) && rendinstgenrender::ri_extra_render_enabled)
  {
    rendinst::renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
      LAYER_DECALS, instance_count_mul, texCtx);

    renderGpuObjectsLayer(render_pass, visibility, layer_flags, LAYER_DECALS);

    rendinstgenrender::disableRendinstAlphaForNormalPassWithZPrepass();
    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      rgl->render(render_pass, visibility[_layer], view_itm, LAYER_DECALS, tree_depth_optimized);
    rendinstgenrender::restoreRendinstAlphaForNormalPassWithZPrepass();
  }
}

void rendinst::renderRIGenOptimizationDepth(rendinst::RenderPass render_pass, const RiGenVisibility *visibility,
  IgnoreOptimizationLimits ignore_optimization_instances_limits, SkipTrees skip_trees, RenderGpuObjects gpu_objects,
  uint32_t instance_count_mul, TexStreamingContext texCtx)
{
  if (!RendInstGenData::renderResRequired || RendInstGenData::isLoading || !unitedvdata::riUnitedVdata.getIB())
    return;

  G_ASSERT(visibility);
  ShaderGlobal::set_int_fast(rendinstgenrender::rendinstRenderPassVarId, eastl::to_underlying(render_pass));
  rendinst::renderRIGenExtra(visibility[0], render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::Yes,
    ignore_optimization_instances_limits, LAYER_OPAQUE, instance_count_mul, texCtx);
  if (visibility[0].gpuObjectsCascadeId != -1 && gpu_objects == RenderGpuObjects::Yes &&
      !gpu_objects_mgr->getGpuInstancing(visibility[0].gpuObjectsCascadeId))
    rendinst::renderRIGenExtraFromBuffer(gpu_objects_mgr->getBuffer(visibility[0].gpuObjectsCascadeId, LAYER_OPAQUE),
      gpu_objects_mgr->getObjectsOffsetsAndCount(visibility[0].gpuObjectsCascadeId, LAYER_OPAQUE),
      gpu_objects_mgr->getObjectsIds(visibility[0].gpuObjectsCascadeId, LAYER_OPAQUE),
      gpu_objects_mgr->getObjectsLodOffsets(visibility[0].gpuObjectsCascadeId, LAYER_OPAQUE), render_pass, OptimizeDepthPass::Yes,
      OptimizeDepthPrepass::Yes, ignore_optimization_instances_limits, LAYER_OPAQUE, nullptr, instance_count_mul);

  if (skip_trees == SkipTrees::No)
    renderRITreeDepth(visibility);
}

void rendinst::renderRITreeDepth(const RiGenVisibility *visibility)
{
  G_ASSERT(visibility);

  if (prepass_trees)
    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      rgl->renderOptimizationDepthPrepass(visibility[_layer]);
}

void rendinst::renderRIGen(rendinst::RenderPass render_pass, const Point3 &view_pos, const TMatrix &view_itm, uint32_t layer_flags,
  bool for_vsm, TexStreamingContext texCtx)
{
  if (!RendInstGenData::renderResRequired || RendInstGenData::isLoading || !unitedvdata::riUnitedVdata.getIB())
    return;
  mat44f ident;
  v_mat44_ident(ident);
  d3d::settm(TM_WORLD, ident);
  mat44f globtm;
  d3d::getglobtm(globtm);
  Frustum frustum;
  frustum.construct(globtm);
  RiGenVisibility visibility(tmpmem);
  const bool for_shadow = render_pass == RenderPass::ToShadow;
  if (for_shadow)
    texCtx = TexStreamingContext(0);
  const uint32_t instance_count_mul = 1;
  if ((layer_flags & rendinst::LAYER_FORCE_LOD_MASK) && visibility.forcedLod < 0 && visibility.riex.forcedExtraLod < 0)
    visibility.forcedLod = visibility.riex.forcedExtraLod =
      (int(layer_flags & rendinst::LAYER_FORCE_LOD_MASK) >> rendinst::LAYER_FORCE_LOD_SHIFT) - 1;

  ShaderGlobal::set_int_fast(rendinstgenrender::rendinstRenderPassVarId, eastl::to_underlying(render_pass));

  if (layer_flags & (rendinst::LAYER_OPAQUE | rendinst::LAYER_TRANSPARENT | rendinst::LAYER_DECALS | rendinst::LAYER_DISTORTION))
    if (prepareRIGenExtraVisibility(globtm, view_pos, visibility, for_shadow, NULL, rendinst::RiExtraCullIntention::MAIN, false, false,
          for_vsm))
    {
      if (layer_flags & rendinst::LAYER_OPAQUE)
        rendinst::renderRIGenExtra(visibility, render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::No,
          IgnoreOptimizationLimits::No, LAYER_OPAQUE, instance_count_mul, texCtx);

      if (layer_flags & rendinst::LAYER_TRANSPARENT)
        rendinst::renderRIGenExtra(visibility, render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::No,
          IgnoreOptimizationLimits::No, LAYER_TRANSPARENT, instance_count_mul, texCtx);

      if (layer_flags & rendinst::LAYER_DISTORTION)
        rendinst::renderRIGenExtra(visibility, render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::No,
          IgnoreOptimizationLimits::No, LAYER_DISTORTION, instance_count_mul, texCtx);
    }
  if ((layer_flags & rendinst::LAYER_OPAQUE) && !for_vsm)
  {
    bool shouldUseSeparateAlpha = !(layer_flags & rendinst::LAYER_NO_SEPARATE_ALPHA);
    if (shouldUseSeparateAlpha)
      rendinstgenrender::disableRendinstAlphaForNormalPassWithZPrepass();
    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      if (rgl->prepareVisibility(frustum, view_pos, visibility, for_shadow, layer_flags, NULL))
        rgl->render(render_pass, visibility, view_itm, (layer_flags & ~rendinst::LAYER_DECALS), false);
    if (shouldUseSeparateAlpha)
      rendinstgenrender::restoreRendinstAlphaForNormalPassWithZPrepass();
  }

  if ((layer_flags & rendinst::LAYER_DECALS) && !for_vsm)
  {
    rendinst::renderRIGenExtra(visibility, render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
      LAYER_DECALS, instance_count_mul, texCtx);
#if _TARGET_PC && !_TARGET_STATIC_LIB // only render decals for riGen in Tools (De3X, AV2)
    rendinstgenrender::disableRendinstAlphaForNormalPassWithZPrepass();
    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      if (rgl->prepareVisibility(frustum, view_pos, visibility, for_shadow, layer_flags, NULL))
        rgl->render(render_pass, visibility, view_itm, LAYER_DECALS, false);
    rendinstgenrender::restoreRendinstAlphaForNormalPassWithZPrepass();
#endif
  }
}

bool rendinst::prepareRIGenVisibility(const Frustum &frustum, const Point3 &vpos, RiGenVisibility *visibility, bool forShadow,
  Occlusion *use_occlusion, bool for_visual_collision, const rendinst::VisibilityExternalFilter &external_filter)
{
  if (!RendInstGenData::renderResRequired || RendInstGenData::isLoading)
    return false;
  G_ASSERT(visibility);
  bool ret = false;
  if (!external_filter)
  {
    FOR_EACH_RG_LAYER_DO (rgl)
      if (rgl->prepareVisibility(frustum, vpos, visibility[_layer], forShadow, 0, use_occlusion, for_visual_collision))
        ret = true;
  }
  else
  {
    FOR_EACH_RG_LAYER_DO (rgl)
      if (rgl->prepareVisibility<true>(frustum, vpos, visibility[_layer], forShadow, 0, use_occlusion, for_visual_collision,
            external_filter))
        ret = true;
  }
  return ret;
}


void rendinst::sortRIGenVisibility(RiGenVisibility *visibility, const Point3 &viewPos, const Point3 &viewDir, float vertivalFov,
  float horizontalFov, float areaThreshold)
{
  G_ASSERT(visibility);
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
    rgl->sortRIGenVisibility(visibility[_layer], viewPos, viewDir, vertivalFov, horizontalFov, areaThreshold);
}

void rendinst::setRIGenVisibilityDistMul(RiGenVisibility *visibility, float dist_mul)
{
  if (visibility)
  {
    visibility[0].setRiDistMul(dist_mul);
    visibility[1].setRiDistMul(dist_mul);
  }
}

RiGenVisibility *rendinst::createRIGenVisibility(IMemAlloc *mem)
{
  RiGenVisibility *visibility = (RiGenVisibility *)mem->alloc(sizeof(RiGenVisibility) * 2);
  new (visibility, _NEW_INPLACE) RiGenVisibility(mem);
  new (visibility + 1, _NEW_INPLACE) RiGenVisibility(mem);
  return visibility;
}

void rendinst::destroyRIGenVisibility(RiGenVisibility *visibility)
{
  if (visibility)
  {
    if (gpu_objects_mgr && visibility[0].gpuObjectsCascadeId != -1)
      gpu_objects_mgr->releaseCascade(visibility[0].gpuObjectsCascadeId);
    IMemAlloc *mem = visibility[0].getmem();
    visibility[0].~RiGenVisibility();
    visibility[1].~RiGenVisibility();
    mem->free(visibility);
  }
}

void rendinst::enableGpuObjectsForVisibility(RiGenVisibility *visibility)
{
  if (gpu_objects_mgr)
    visibility[0].gpuObjectsCascadeId = visibility[1].gpuObjectsCascadeId = gpu_objects_mgr->addCascade();
}

void rendinst::disableGpuObjectsForVisibility(RiGenVisibility *visibility)
{
  if (gpu_objects_mgr && visibility[0].gpuObjectsCascadeId != -1)
    gpu_objects_mgr->releaseCascade(visibility[0].gpuObjectsCascadeId);
  visibility[0].gpuObjectsCascadeId = visibility[1].gpuObjectsCascadeId = -1;
}

void rendinst::clearGpuObjectsFromVisibility(RiGenVisibility *visibility)
{
  if (gpu_objects_mgr && visibility[0].gpuObjectsCascadeId != -1)
    gpu_objects_mgr->clearCascade(visibility[0].gpuObjectsCascadeId);
}

void rendinst::setRIGenVisibilityMinLod(RiGenVisibility *visibility, int ri_lod, int ri_extra_lod)
{
  visibility[0].forcedLod = visibility[1].forcedLod = ri_lod;
  visibility[0].riex.forcedExtraLod = visibility[1].riex.forcedExtraLod = ri_extra_lod;
}

void rendinst::setRIGenVisibilityRendering(RiGenVisibility *visibility, VisibilityRenderingFlags v) { visibility[0].rendering = v; }

void rendinst::setRIGenVisibilityAtestSkip(RiGenVisibility *visibility, bool skip_atest, bool skip_noatest)
{
  if (skip_noatest && skip_atest)
  {
    visibility[0].atest_skip_mask = visibility->RENDER_ALL;
    G_ASSERTF(0, "it doesn't make sense to render empty rigen list (there is no atest and no non-atest)");
    return;
  }
  visibility[0].atest_skip_mask =
    skip_atest ? visibility->SKIP_ATEST : (skip_noatest ? visibility->SKIP_NO_ATEST : visibility->RENDER_ALL);
}

void rendinst::copyVisibileImpostorsData(const RiGenVisibility *visibility, bool clear_data)
{
  if (!RendInstGenData::renderResRequired)
    return;
  FOR_EACH_RG_LAYER_DO (rgl)
    rgl->rtData->copyVisibileImpostorsData(visibility[_layer], clear_data);
}

void RendInstGenData::onDeviceReset()
{
  if (!rtData)
    return;

  ScopedLockWrite lock(rtData->riRwCs);
  dag::ConstSpan<int> ld = rtData->loaded.getList();
  for (auto ldi : ld)
  {
    RendInstGenData::CellRtData *crt = cells[ldi].rtData;
    if (!crt)
      continue;
    updateVb(*crt, ldi);
  }

  for (rendinstgenrender::RtPoolData *rtpool : rtData->rtPoolData)
  {
    if (!rtpool)
      continue;
    rendinstgenrender::RtPoolData &pool = *rtpool;
    pool.flags &= ~(rendinstgenrender::RtPoolData::HAS_CLEARED_LIGHT_TEX);
    pool.hasUpdatedShadowImpostor = false;
  }
}

static void rendinst_afterDeviceReset(bool /*full_reset*/)
{
  if (!RendInstGenData::renderResRequired)
    return;

  rendinst::resetRiGenImpostors();

  fillRendInstVBs();

  FOR_EACH_RG_LAYER_DO (rgl)
    rgl->onDeviceReset();

  if (gpu_objects_mgr)
    gpu_objects_mgr->invalidate();
}

void RendInstGenData::renderDebug()
{
  ScopedLockRead lock(rtData->riRwCs);
  begin_draw_cached_debug_lines(true, false);
  dag::ConstSpan<int> ld = rtData->loaded.getList();
  for (auto ldi : ld)
  {
    const RendInstGenData::CellRtData *crt_ptr = cells[ldi].isReady();
    if (!crt_ptr)
      continue;
    for (int j = 0; j < RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV + 1; ++j)
    {
      BBox3 box;
      v_stu_bbox3(box, crt_ptr->bbox[j]);
      draw_cached_debug_box(box, j ? 0xFF1F1F1F : 0xFF00FFFF);
    }
  }
  end_draw_cached_debug_lines();
}

void rendinst::renderDebug()
{
  if (!RendInstGenData::renderResRequired)
    return;
  FOR_EACH_RG_LAYER_DO (rgl)
    rgl->renderDebug();
}

void rendinst::getLodCounter(int lod, const RiGenVisibility *visibility, int &subCellNo, int &cellNo) // fixme: replace interface
{
  cellNo = visibility->instNumberCounter[lod];
  subCellNo = visibility->instNumberCounter[lod];
}

void rendinst::set_per_instance_visibility_for_any_tree(bool on)
{
  rendinstgenrender::per_instance_visibility_for_everyone = on;
  rendinstgenrender::use_cross_dissolve = (on && VariableMap::isVariablePresent(rendinstgenrender::render_cross_dissolved_varId));
}

void rendinst::setRIGenRenderMode(int mode)
{
  if (rendinst::ri_game_render_mode != mode)
  {
    debug("ri_mode: %d->%d", rendinst::ri_game_render_mode, mode);
    // debug_dump_stack("setRIGenRenderMode");
  }
  rendinst::ri_game_render_mode = mode;
}
int rendinst::getRIGenRenderMode() { return rendinst::ri_game_render_mode; }


#include <debug/dag_debug3d.h>
#include <gui/dag_stdGuiRender.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_drv3d.h>
#include <gameRes/dag_collisionResource.h>

bool rendinst::has_pending_gpu_objects() { return !gpu_objects_to_add.empty(); }

void rendinst::add_gpu_object(const String &name, int cell_tile, int grid_size, float cell_size,
  const gpu_objects::PlacingParameters &parameters)
{
  if (gpu_objects_mgr)
  {
    if (!isRiExtraLoaded())
    {
      GpuObjectsEntry entry = {name, cell_tile, grid_size, cell_size, parameters};
      gpu_objects_to_add.emplace_back(entry);
      return;
    }
    int id = rendinst::riExtraMap.getNameId(name);
    if (id == -1)
    {
      debug("GPUObjects: auto adding <%s> as riExtra.", name);
      id = addRIGenExtraResIdx(name, -1, -1, AddRIFlag::UseShadow);
      if (id < 0)
        return;
    }

    float boundingSphereRadius = rendinst::riExtra[id].sphereRadius;
    dag::ConstSpan<float> distSqLod(rendinst::riExtra[id].distSqLOD, rendinst::riExtra[id].res->lods.size());
    gpu_objects_mgr->addObject(name.c_str(), id, cell_tile, grid_size, cell_size, boundingSphereRadius, distSqLod, parameters);
    gpu_objects::PlacingParameters nodeBasedMetadataAppliedParams;
    // if node based shader has metadata block with parameters, this values are used instead of stored in entity description
    gpu_objects_mgr->getParameters(id, nodeBasedMetadataAppliedParams);
    gpu_objects_mgr->getCellData(id, cell_tile, grid_size, cell_size);
    rendinst::riExtra[id].radiusFade = grid_size * cell_size * 0.5;
    rendinst::riExtra[id].radiusFadeDrown = 0.5;
    rendinst::riExtra[id].hardness = clamp(1.0f - nodeBasedMetadataAppliedParams.hardness, 0.0f, 1.0f);
    rendinst::update_per_draw_gathered_data(id);
  }
}

void rendinst::update(const Point3 &origin)
{
  if (gpu_objects_mgr)
  {
    if (isRiExtraLoaded())
      for (eastl::vector<GpuObjectsEntry>::reverse_iterator it = gpu_objects_to_add.rbegin(); it != gpu_objects_to_add.rend(); ++it)
      {
        add_gpu_object(it->name, it->grid_tile, it->grid_size, it->cell_size, it->parameters);
        gpu_objects_to_add.erase(it);
      }
    gpu_objects_mgr->update(origin);
  }
}

void rendinst::invalidateGpuObjects()
{
  if (gpu_objects_mgr)
  {
    gpu_objects_mgr->invalidate();
  }
}

void rendinst::beforeDraw(RenderPass render_pass, const RiGenVisibility *visibility, const Frustum &frustum,
  const Occlusion *occlusion, const char *mission_name, const char *map_name, bool gpu_instancing)
{
  if (gpu_objects_mgr && visibility && visibility->gpuObjectsCascadeId != -1)
    gpu_objects_mgr->beforeDraw(render_pass, visibility->gpuObjectsCascadeId, frustum, occlusion, mission_name, map_name,
      gpu_instancing);
  rendinst::ensureElemsRebuiltRIGenExtra(gpu_instancing);
}

void rendinst::validateDisplacedGPUObjs(float displacement_tex_range)
{
  if (gpu_objects_mgr)
    gpu_objects_mgr->validateDisplacedGPUObjs(displacement_tex_range);
}

void rendinst::change_gpu_object_parameters(const String &name, const gpu_objects::PlacingParameters &parameters)
{
  d3d::GpuAutoLock lock;
  if (gpu_objects_mgr)
    gpu_objects_mgr->changeParameters(rendinst::riExtraMap.getNameId(name), parameters);
}

void rendinst::change_gpu_object_grid(const String &name, int cell_tile, int grid_size, float cell_size)
{
  d3d::GpuAutoLock lock;
  if (gpu_objects_mgr)
    gpu_objects_mgr->changeGrid(rendinst::riExtraMap.getNameId(name), cell_tile, grid_size, cell_size);
}

void rendinst::invalidate_gpu_objects_in_bbox(const BBox2 &bbox)
{
  if (gpu_objects_mgr)
  {
    d3d::GpuAutoLock lock;
    gpu_objects_mgr->invalidateBBox(bbox);
  }
}

void rendinst::updateHeapVb()
{
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
    rgl->updateHeapVb();
}


REGISTER_D3D_AFTER_RESET_FUNC(rendinst_afterDeviceReset);

static bool ri_gpu_objects_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("ri_gpu_objects", "invalidate", 1, 1)
  {
    d3d::GpuAutoLock lock;
    if (gpu_objects_mgr)
      gpu_objects_mgr->invalidate();
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(ri_gpu_objects_console_handler);
