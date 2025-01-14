// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstGenRender.h>

#include <rendInst/gpuObjects.h>
#include <rendInst/rotation_palette_consts.hlsli>
#include <rendInst/impostorTextureMgr.h>
#include <rendInst/rendInstExtraRender.h>

#include "riGen/riGenExtra.h"
#include "riGen/riGenData.h"
#include "riGen/genObjUtil.h"
#include "riGen/riRotationPalette.h"
#include "riGen/riGenRenderer.h"
#include "render/genRender.h"
#include "render/impostor.h"
#include "render/extraRender.h"
#include "render/gpuObjects.h"
#include "render/riShaderConstBuffers.h"
#include "visibility/genVisibility.h"
#include "debug/collisionVisualization.h"

#include <3d/dag_quadIndexBuffer.h>
#include <stdio.h>
#include <shaders/dag_shaderBlock.h>
#include <math/dag_frustum.h>
#include <perfMon/dag_statDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <util/dag_stlqsort.h>
#include <util/dag_globDef.h>
#include <memory/dag_framemem.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_lock.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <scene/dag_occlusion.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/array.h>
#include <gpuObjects/gpuObjects.h>
#include <math/dag_mathUtils.h>
#include <render/debugMesh.h>
#include <3d/dag_resPtr.h>
#include <math/dag_half.h>
#include <3d/dag_multidrawContext.h>

#include <debug/dag_debug3d.h>
#include <drv/3d/dag_resetDevice.h>


#define debug(...) logmessage(_MAKE4C('RGEN'), __VA_ARGS__)

// todo make custom mipmap generation for some of last mipmaps - weighted filter, weights only non-clipped pixels

#define DEBUG_RI 0

bool check_occluders = true, add_occluders = true;

int globalRendinstRenderTypeVarId = -1;

int globalTranspVarId = -1;

static int drawOrderVarId = -1;
int useRiGpuInstancingVarId = -1;

static int invLod0RangeVarId = -1;

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

RenderStateContext::RenderStateContext() { d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB())); }

namespace rendinst::render
{

static uint32_t packed_format = TEXFMT_A16B16G16R16S;
static uint32_t unpacked_format = TEXFMT_A32B32G32R32F;

bool use_color_padding = true;

bool avoidStaticShadowRecalc = false;
bool per_instance_visibility = false;
bool per_instance_visibility_for_everyone = true; // todo: change in tank to true and in plane to false
bool per_instance_front_to_back = true;
bool useConditionalRendering = false; // obsolete - not working
shaders::UniqueOverrideStateId afterDepthPrepassOverride;

bool vertical_billboards = false;
static bool ri_extra_render_enabled = true;
static bool use_bbox_in_cbuffer = false;
bool use_tree_lod0_offset = true;
bool use_lods_by_distance_update = true;
float lods_by_distance_range_sq = sqr(200);

static VDECL rendinstDepthOnlyVDECL = BAD_VDECL;
static int build_normal_type = -2;
int normal_type = -1;

Tab<UniqueBuf> riGenPerDrawDataForLayer;
MultidrawContext<rendinst::render::RiGenPerInstanceParameters> riGenMultidrawContext("ri_gen_multidraw");

Vbuffer *oneInstanceTmVb = nullptr;
Vbuffer *rotationPaletteTmVb = nullptr;
float globalDistMul = 1;
float globalCullDistMul = 1;
float globalLodCullDistMul = 1;
float settingsDistMul = 1;
float settingsMinLodBasedCullDistMul = 1.f;
float settingsMinCullDistMul = 0.f;
float lodsShiftDistMul = 1;
float additionalRiExtraLodDistMul = 1;
float riExtraLodsShiftDistMul = 1.f;
float riExtraMulScale = 1;
float riExtraLodsShiftDistMulForCulling = 1.f;
int globalFrameBlockId = -1;
int rendinstSceneBlockId = -1;
int rendinstSceneTransBlockId = -1;
int rendinstDepthSceneBlockId = -1;
bool forceImpostors = false;
bool useCellSbuffer = false;

int instancingTypeVarId = -1;
CoordType cur_coord_type = COORD_TYPE_TM;

void setCoordType(CoordType type)
{
  cur_coord_type = type;
  ShaderGlobal::set_int_fast(instancingTypeVarId, cur_coord_type);
}

void setApexInstancing() { ShaderGlobal::set_int_fast(instancingTypeVarId, 5); }

bool setBlock(int block_id, const UniqueBuf &per_draw_data)
{
  const bool reapplyBlock = setPerDrawData(per_draw_data);
  const int currentBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
  const bool needToSetBlock = currentBlock == -1;
  if (needToSetBlock)
    ShaderGlobal::setBlock(block_id, ShaderGlobal::LAYER_SCENE);
  else if (reapplyBlock)
    ShaderGlobal::setBlock(currentBlock, ShaderGlobal::LAYER_SCENE);

  return needToSetBlock;
}

int rendinstRenderPassVarId = -1;
int rendinstShadowTexVarId = -1;
int lods_shift_dist_mul_varId = -1;

int gpuObjectDecalVarId = -1;
int disable_rendinst_alpha_for_normal_pass_with_zprepassVarId = -1;

int dynamic_impostor_texture_const_no = -1;
int ri_vertex_data_no = -1;
float rendinst_ao_mul = 2.0f;
// mip map build

bool use_ri_depth_prepass = true;
static bool depth_prepass_for_cells = true;
static bool depth_prepass_for_impostors = false;

bool useRiDepthPrepass(bool use)
{
  bool ret = use_ri_depth_prepass;
  use_ri_depth_prepass = use;
  return ret;
}

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
} // namespace rendinst::render

static __forceinline void get_local_up_left(const Point3 &imp_fwd, const Point3 &imp_up, const Point3 &cam_fwd, Point3 &out_up,
  Point3 &out_left)
{
  Point3 fwd = cam_fwd * imp_fwd > 0 ? cam_fwd : -cam_fwd;
  out_left = normalize(imp_up % fwd);
  out_up = normalize(fwd % out_left);
}

void rendinst::render::renderRendinstShadowsToTextures(const Point3 &sunDir0)
{
  if (rendinstClipmapShadows)
    spin_wait([&] { return !renderRIGenClipmapShadowsToTextures(sunDir0, false) && !d3d::device_lost(nullptr); });

  if (rendinstGlobalShadows)
    spin_wait([&] { return !renderRIGenGlobalShadowsToTextures(sunDir0) && !d3d::device_lost(nullptr); });
}

bool rendinst::render::notRenderedStaticShadowsBBox(BBox3 &box, bool add_instance_box)
{
  if (RendInstGenData::isLoading)
    return false;

  BBox3 b;
  box.setempty();
  bool res = false;
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (rgl->notRenderedStaticShadowsBBox(b))
      res = true;
    box += b;
  }

  if (!add_instance_box)
    return res;

  BBox3 ncibox = riExTiledScenes.getNewlyCreatedInstBoxAndReset();
  if (!ncibox.isempty())
  {
    box += ncibox;
    // debug("getNewlyCreatedInstBoxAndReset=%@ -> box=%@", ncibox, box);
    res = true;
  }
  return res;
}

BBox3 rendinst::render::get_newly_created_instance_box_and_reset() { return riExTiledScenes.getNewlyCreatedInstBoxAndReset(); }

namespace rendinst::render
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

}; // namespace rendinst::render

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

namespace rendinst::render
{

void init_depth_VDECL()
{
  static VSDTYPE vsdDepthOnly[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT3), VSD_END};
  if (rendinst::render::rendinstDepthOnlyVDECL != BAD_VDECL)
    d3d::delete_vdecl(rendinst::render::rendinstDepthOnlyVDECL);

  rendinst::render::rendinstDepthOnlyVDECL = d3d::create_vdecl(vsdDepthOnly);
  build_normal_type = normal_type;
}

void close_depth_VDECL()
{
  if (rendinstDepthOnlyVDECL != BAD_VDECL)
    d3d::delete_vdecl(rendinstDepthOnlyVDECL);
  rendinstDepthOnlyVDECL = BAD_VDECL;
  build_normal_type = -2;
}

static void getPerDrawBufferOptions(int &elements, int &riGenElements, bool &riUseStructuredBuffer)
{
  uint32_t perDrawElems = min(rendinst::riex_max_type(),
    (uint32_t)::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("riMaxModelsPerLevel", INT_MAX));
  uint32_t riGenPerDrawElems =
    min(perDrawElems, (uint32_t)::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("riGenMaxModelsPerLevel", INT_MAX));
  elements = (sizeof(rendinst::render::RiShaderConstBuffers) / sizeof(Point4)) * perDrawElems;
  riGenElements = (sizeof(rendinst::render::RiShaderConstBuffers) / sizeof(Point4)) * riGenPerDrawElems;
  // do not create buffer view if hardware is unable to use it

  // WT cannot use structured buffers becase of outdated DX10 Intel GPU.
  int smallSampledBuffersVarId = ::get_shader_variable_id("small_sampled_buffers", true);
  riUseStructuredBuffer = ShaderGlobal::get_interval_current_value(smallSampledBuffersVarId) == 1;
}

void ensurePerDrawBufferExists(int layer)
{
  if (riGenPerDrawDataForLayer.size() <= layer)
    riGenPerDrawDataForLayer.resize(layer + 1);

  UniqueBuf &riGenPerDrawData = riGenPerDrawDataForLayer[layer];
  if (!riGenPerDrawData)
  {
    int elements, riGenElements;
    bool riUseStructuredBuffer;
    getPerDrawBufferOptions(elements, riGenElements, riUseStructuredBuffer);

    eastl::string name = "riGenPerDrawInstanceDataForLayer[0]";
    name[name.size() - 2] = '0' + layer;
    riGenPerDrawData =
      dag::create_sbuffer(sizeof(Point4), riGenElements, SBCF_BIND_SHADER_RES | (riUseStructuredBuffer ? SBCF_MISC_STRUCTURED : 0),
        riUseStructuredBuffer ? 0 : TEXFMT_A32B32G32R32F, name.c_str());
  }
}

}; // namespace rendinst::render

static void allocatePaletteVB()
{
  // The size of the palette is determined later than allocateRendInstVBs is called
  if (!rendinst::render::rotationPaletteTmVb)
  {
    const int maxCount = (1 << PALETTE_ID_BIT_COUNT) + 1; // +1 for identity rotation
    const int vecsCnt = maxCount * 3;
    // TODO: Fill the buffer on device reset
    if (rendinst::render::useCellSbuffer)
      rendinst::render::rotationPaletteTmVb =
        d3d::buffers::create_persistent_sr_structured(sizeof(Point4), vecsCnt, "rotationPaletteTmVb");
    else
      rendinst::render::rotationPaletteTmVb =
        d3d::buffers::create_persistent_sr_tbuf(vecsCnt, TEXFMT_A32B32G32R32F, "rotationPaletteTmVb");
    d3d_err(rendinst::render::rotationPaletteTmVb);
  }
}

static void allocateRendInstVBs()
{
  if (rendinst::render::oneInstanceTmVb) // already created
    return;
  rendinst::render::init_instances_tb();
  // Create VB to render one instance of rendinst using IB converted to VB.
  if (rendinst::render::useCellSbuffer)
    rendinst::render::oneInstanceTmVb = d3d::buffers::create_persistent_sr_structured(sizeof(TMatrix4), 1, "oneInstanceTmVb");
  else
    rendinst::render::oneInstanceTmVb = d3d::buffers::create_persistent_sr_tbuf(4, TEXFMT_A32B32G32R32F, "oneInstanceTmVb");

  d3d_err(rendinst::render::oneInstanceTmVb);

  G_STATIC_ASSERT(sizeof(rendinst::render::RiShaderConstBuffers) <= 256); // we just keep it smaller than 256 bytes for performance
                                                                          // reasons
  rendinst::render::perDrawCB =
    d3d::buffers::create_one_frame_cb(dag::buffers::cb_struct_reg_count<rendinst::render::RiShaderConstBuffers>(), "perDrawInstCB");
  d3d_err(rendinst::render::perDrawCB);

  int elements, riGenElements;
  bool riUseStructuredBuffer;
  rendinst::render::getPerDrawBufferOptions(elements, riGenElements, riUseStructuredBuffer);

  rendinst::render::riExtraPerDrawData =
    dag::create_sbuffer(sizeof(Point4), elements, SBCF_BIND_SHADER_RES | (riUseStructuredBuffer ? SBCF_MISC_STRUCTURED : 0),
      riUseStructuredBuffer ? 0 : TEXFMT_A32B32G32R32F, "riExtraPerDrawInstanceData");

  rendinst::render::ensurePerDrawBufferExists(0);

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
  if (!rendinst::render::oneInstanceTmVb)
    return;

  d3d::GpuAutoLock gpuLock;

  float src[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  float *data;
  if (rendinst::render::oneInstanceTmVb->lock(0, 0, (void **)&data, VBLOCK_WRITEONLY) && data)
  {
    memcpy(data, src, sizeof(src));
    rendinst::render::oneInstanceTmVb->unlock();
  }
}


void RendInstGenData::termRenderGlobals()
{
  rendinst::closeImpostorShadowTempTex();

  if (!RendInstGenData::renderResRequired)
    return;

  rendinst::gpuobjects::shutdown();
  shaders::overrides::destroy(rendinst::render::afterDepthPrepassOverride);
  rendinst::render::close_instances_tb();
  rendinst::render::riExtraPerDrawData.close();
  for (UniqueBuf &riGenPerDrawData : rendinst::render::riGenPerDrawDataForLayer)
    riGenPerDrawData.close();
  del_d3dres(rendinst::render::perDrawCB);
  del_d3dres(rendinst::render::oneInstanceTmVb);
  del_d3dres(rendinst::render::rotationPaletteTmVb);
  rendinst::render::riExMultidrawContext.close();
  rendinst::render::riGenMultidrawContext.close();
  index_buffer::release_quads_16bit();
  rendinst::render::closeClipmapShadows();
  rendinst::render::close_depth_VDECL();
  rendinst::close_debug_collision_visualization();
  termImpostorsGlobals();
}

static __forceinline bool isTexInstancingOn() { return false; }

float cascade0Dist = -1.f;

void RendInstGenData::initRenderGlobals(bool use_color_padding, bool should_init_gpu_objects)
{
#define GET_REG_NO(a)                                             \
  {                                                               \
    int a##VarId = get_shader_variable_id(#a);                    \
    if (a##VarId >= 0)                                            \
      rendinst::render::a = ShaderGlobal::get_int_fast(a##VarId); \
  }
  rendinst::render::settingsDistMul = clamp(::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("rendinstDistMul", 1.f),
    MIN_SETTINGS_RENDINST_DIST_MUL, MAX_SETTINGS_RENDINST_DIST_MUL);
  rendinst::render::globalDistMul = rendinst::render::settingsDistMul;
  rendinst::render::riExtraMulScale = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("riExtraMulScale", 1.0f);
  rendinst::render::lodsShiftDistMul =
    clamp(::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("lodsShiftDistMul", 1.f), 1.f, 1.3f);

  rendinst::render::use_color_padding =
    use_color_padding && ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("riColorPadding", true);
  debug("rendinst::render::use_color_padding=%d", (int)rendinst::render::use_color_padding);

  globalRendinstRenderTypeVarId = ::get_shader_variable_id("rendinst_render_type", true);
  // check that var is assumed or missing, to avoid using specific rendering options when they are not available
  if (!VariableMap::isVariablePresent(globalRendinstRenderTypeVarId) || ShaderGlobal::is_var_assumed(globalRendinstRenderTypeVarId))
    globalRendinstRenderTypeVarId = -1;

  debug("rendinst::render::globalRendinstRenderTypeVarId %s", globalRendinstRenderTypeVarId == -1 ? "assumed/missing" : "dynamic");

  bool forceUniversalRIByConfig = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("forceUniversalRIRenderingPath", false);
  // non universal RI rendering depends on base instance id
  bool forceUniversalRIByDriver = d3d::get_driver_desc().issues.hasBrokenBaseInstanceID;
  if (forceUniversalRIByConfig || forceUniversalRIByDriver)
  {
    debug("rendinst::render::globalRendinstRenderTypeVarId forced assumed/missing by %s %s", forceUniversalRIByConfig ? "config" : "",
      forceUniversalRIByDriver ? "driver" : "");
    globalRendinstRenderTypeVarId = -1;
  }


  globalTranspVarId = ::get_shader_glob_var_id("global_transp_r");
  useRiGpuInstancingVarId = ::get_shader_glob_var_id("use_ri_gpu_instancing", true);
  isRenderByCellsVarId = ::get_shader_variable_id("is_render_by_cells", true);

  gpuObjectVarId = ::get_shader_variable_id("gpu_object_id", true);

  int rendinstUseCellSbufferVarId = ::get_shader_variable_id("rendinst_use_cell_sbuffer", true);
  rendinst::render::useCellSbuffer = ShaderGlobal::get_interval_current_value(rendinstUseCellSbufferVarId) == 1;

  debug("rendinst: cell buffer type <%s>", rendinst::render::useCellSbuffer ? "structured" : "tex");

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

  rendinst::render::globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");
  rendinst::render::rendinstSceneBlockId = ShaderGlobal::getBlockId("rendinst_scene");
  rendinst::render::rendinstSceneTransBlockId = ShaderGlobal::getBlockId("rendinst_scene_trans");
  if (rendinst::render::rendinstSceneTransBlockId < 0)
    rendinst::render::rendinstSceneTransBlockId = rendinst::render::rendinstSceneBlockId;
  rendinst::render::rendinstDepthSceneBlockId = ShaderGlobal::getBlockId("rendinst_depth_scene");
  if (rendinst::render::rendinstDepthSceneBlockId < 0)
    rendinst::render::rendinstDepthSceneBlockId = rendinst::render::rendinstSceneBlockId;

  rendinst::render::dynamic_impostor_texture_const_no =
    ShaderGlobal::get_int_fast(::get_shader_glob_var_id("dynamic_impostor_texture_const_no"));
  rendinst::render::rendinst_ao_mul = ShaderGlobal::get_real_fast(::get_shader_glob_var_id("rendinst_ao_mul", true));

  invLod0RangeVarId = ::get_shader_glob_var_id("rendinst_inv_lod0_range", true);

  ShaderGlobal::get_int_by_name("ri_vertex_data_no", rendinst::render::ri_vertex_data_no);

  drawOrderVarId = get_shader_glob_var_id("draw_order", true);
  rendinst::render::rendinstShadowTexVarId = ::get_shader_glob_var_id("rendinst_shadow_tex", true);
  rendinst::render::rendinstRenderPassVarId = ::get_shader_glob_var_id("rendinst_render_pass");
  rendinst::render::gpuObjectDecalVarId = ::get_shader_glob_var_id("gpu_object_decal", true);
  rendinst::render::disable_rendinst_alpha_for_normal_pass_with_zprepassVarId =
    get_shader_variable_id("disable_rendinst_alpha_for_normal_pass_with_zprepass", true);

  rendinst::render::instancingTypeVarId = ::get_shader_glob_var_id("instancing_type");
  rendinst::render::lods_shift_dist_mul_varId = ::get_shader_glob_var_id("lods_shift_dist_mul");

  auto graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  rendinst::render::use_tree_lod0_offset = graphicsBlk->getBool("useTreeLod0Offset", true);
  rendinst::render::use_lods_by_distance_update = graphicsBlk->getBool("useLodsByDistanceUpdate", true);
  rendinst::render::lods_by_distance_range_sq = sqr(graphicsBlk->getReal("lodsByDistanceRange", 100));
  debug("rendinst: lods_by_distance %s %g", (rendinst::render::use_lods_by_distance_update ? "on" : "off"),
    sqrtf(rendinst::render::lods_by_distance_range_sq));

  rendinst::render::use_bbox_in_cbuffer = ::get_shader_variable_id("useBboxInCbuffer", true) >= 0;

  rendinst::render::initClipmapShadows();
  rendinst::render::per_instance_visibility = d3d::get_driver_desc().caps.hasInstanceID;

  rendinst::render::init_depth_VDECL();

  initRendInstVBs();
  initImpostorsGlobals();

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_FUNC | shaders::OverrideState::Z_WRITE_DISABLE);
  state.zFunc = CMPF_EQUAL;
  rendinst::render::afterDepthPrepassOverride.reset(shaders::overrides::create(state));

  if (should_init_gpu_objects)
  {
    rendinst::gpuobjects::startup();
  }

  rendinst::init_debug_collision_visualization();

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

constexpr float MIN_TREES_FAR_MUL = 0.3177f;
void RendInstGenData::RtData::setDistMul(float dist_mul, float cull_mul, bool scale_lod1, bool set_preload_dist, bool no_mul_limit,
  float impostors_far_dist_additional_mul)
{
  bool shouldUpdatePerDrawData = false;
  const auto setValue = [&shouldUpdatePerDrawData](float &var, float value) {
    if (eastl::exchange(var, value) != value)
      shouldUpdatePerDrawData = true;
  };
  rendinstDistMul = dist_mul;
  setValue(impostorsFarDistAdditionalMul, rendinst::isRgLayerPrimary(layerIdx) ? impostors_far_dist_additional_mul : 1.f);
  float mulLimit = no_mul_limit ? 0.f : MIN_TREES_FAR_MUL;
  if (set_preload_dist)
    preloadDistance = max(mulLimit, cull_mul) * settingsPreloadDistance * max(impostorsFarDistAdditionalMul, 1.f);
  if (scale_lod1)
  {
    setValue(transparencyDeltaRcp, cull_mul * settingsTransparencyDeltaRcp);
    setValue(rendinstDistMulFar, cull_mul);
    setValue(rendinstDistMulFarImpostorTrees, max(mulLimit, cull_mul));
    setValue(rendinstDistMulImpostorTrees,
      rendinstDistMulFarImpostorTrees > 1.0f ? sqrtf(max(mulLimit, dist_mul)) : max(mulLimit, dist_mul));
  }
  if (shouldUpdatePerDrawData)
    rendinst::render::RiGenRenderer::updatePerDrawData(*this, rendinst::rgLayer[layerIdx]->perInstDataDwords);
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
      rtData->rtPoolData[i] = new rendinst::render::RtPoolData(rtData->riRes[i]);
      if (rtData->riRes[i]->hasImpostor())
      {
        if (rtData->riRes[i]->isBakedImpostor())
        {
          rendinst::gen::RotationPaletteManager::Palette palette =
            rendinst::gen::get_rotation_palette_manager()->getPalette({rtData->layerIdx, i});
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
            if (!rtData->riRes[i]->setImpostorVars(mats[j]))
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
              if (rtData->riRes[i]->setImpostorVars(mats[j]))
                rtData->rtPoolData[i]->flags |= rendinst::render::RtPoolData::HAS_TRANSITION_LOD;
            }
          }

          rtData->riImpTexIds.add(rtData->riRes[i]->getImpostorTextures().albedo_alpha);
        }
        rtData->riRes[i]->gatherUsedTex(rtData->riImpTexIds);
      }
      if (rtData->riResElemMask[i].plod)
      {
        const auto r = rtData->rtPoolData[i]->sphereRadius;
        const float approxSurfArea = 4 * M_PI * r * r;
        Tab<dag::Span<const ShaderMesh::RElem>> elems{};
        rtData->riRes[i]->lods.back().getAllElems(elems);
        uint totalCount = 0;
        for (const auto &elem : elems.front())
          totalCount += elem.numv;
        const float approxDensity = static_cast<float>(totalCount) / approxSurfArea;
        rtData->rtPoolData[i]->plodRadius = 1 / std::sqrt(M_PI * approxDensity);
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


  float secondLayerDistMul = min(rendinst::render::globalDistMul, 0.75f);

  if (main_layer)
    rtData->setDistMul(rendinst::render::globalDistMul, rendinst::render::globalCullDistMul);
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

  rendinst::render::RiGenRenderer::updatePerDrawData(*rtData, perInstDataDwords);
  rendinst::render::RiGenRenderer::updatePackedData(rtData->layerIdx);
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
    const DataBlock *ri_ovr = rtData->riResName[i] ? rendinst::ri_lod_ranges_ovr.getBlockByName(rtData->riResName[i]) : nullptr;

    if (!RendInstGenData::renderResRequired || !rtData->rtPoolData[i])
      continue;
    if (!rtData->riRes[i]->hasImpostor())
    {
      for (int lodI = 0; lodI < rtData->riResLodCount(i); lodI++)
        rtData->rtPoolData[i]->lodRange[lodI] = rtData->riResLodRange(i, lodI, ri_ovr);
    }
    else
    {
      float subCellOfsSize = grid2world * cellSz *
                             ((rendinst::render::per_instance_visibility_for_everyone ? 0.75f : 0.25f) *
                               (rendinst::render::globalDistMul * 1.f / RendInstGenData::SUBCELL_DIV));
      rtData->applyImpostorRange(i, ri_ovr, subCellOfsSize);
    }
    int lastLodNo = rtData->riResLodCount(i) - 1;
    float last_lod_range = min(rtData->rtPoolData[i]->lodRange[lastLodNo], rtData->settingsPreloadDistance);
    maxDist = max(maxDist, last_lod_range);
    averageFarPlane += last_lod_range;
    averageFarPlaneCount++;
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
    rtData->riDebrisDelayedRi.clear();
}

void RendInstGenData::RtData::clear()
{
  rendinst::render::normal_type = -1;
  for (int i = 0; i < rtPoolData.size(); i++)
    del_it(rtPoolData[i]);
  clear_and_shrink(rtPoolData);
  maxDebris[0] = maxDebris[1] = curDebris[0] = curDebris[1] = 0;
  ShaderGlobal::reset_textures(true);
  riDebrisDelayedRi.clear();
}

void RendInstGenData::RtData::setTextureMinMipWidth(int textureSize, int impostorSize)
{
  G_ASSERT(textureSize > 0 && impostorSize > 0);
  impostorLastMipSize = impostorSize;

  for (unsigned int poolNo = 0; poolNo < rtPoolData.size(); poolNo++)
  {
    if (!rtPoolData[poolNo])
      continue;

    G_ASSERT(riRes[poolNo] != nullptr);
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
#if _TARGET_C1






#else
        tex->texmiplevel(0, lastMip);
#endif
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
  CellRtDataLoaded tmp = eastl::move(*(RendInstGenData::CellRtDataLoaded *)this);
  if (cellVbId)
    rtData->cellsVb.free(cellVbId);
  rtData->updateVbResetCS.unlock();
}

RendInstGenData::RtData::RtData(int layer_idx) :
  cellsVb(SbufferHeapManager(String(128, "cells_vb_%d", layer_idx), //-V730
    RENDER_ELEM_SIZE, SBCF_BIND_SHADER_RES | (rendinst::render::useCellSbuffer ? SBCF_MISC_STRUCTURED : 0),
    rendinst::render::useCellSbuffer ? 0 : (RENDINST_FLOAT_POS ? rendinst::render::unpacked_format : rendinst::render::packed_format)))
{
  cellsVb.getManager().setShouldCopyToNewHeap(false);
  layerIdx = layer_idx;
  loadedCellsBBox = IBBox2(IPoint2(10000, 10000), IPoint2(-10000, -10000));
  lastPoi = Point2(-1000000, 1000000);
  maxDebris[0] = maxDebris[1] = curDebris[0] = curDebris[1] = 0;
  rendinstDistMul = rendinstDistMulFar = rendinstDistMulImpostorTrees = rendinstDistMulFarImpostorTrees =
    impostorsFarDistAdditionalMul = impostorsDistAdditionalMul = 1.0f;
  impostorsMinRange = eastl::numeric_limits<float>::infinity();
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
    if constexpr (RENDINST_FLOAT_POS)
    {
      vec3f v_cell_add = cellOrigin;
      vec3f v_cell_mul =
        v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(rgd.grid2world * rgd.cellSz, cellHeight, rgd.grid2world * rgd.cellSz, 0));

      const vec4f v_palette_mul = v_make_vec4f(1.f / (PALETTE_ID_MULTIPLIER * PALETTE_SCALE_MULTIPLIER),
        1.f / (PALETTE_ID_MULTIPLIER * PALETTE_SCALE_MULTIPLIER), 1.f / (PALETTE_ID_MULTIPLIER * PALETTE_SCALE_MULTIPLIER),
        1.f / (PALETTE_ID_MULTIPLIER * PALETTE_SCALE_MULTIPLIER));

      constexpr int unpackRatio = RENDER_ELEM_SIZE_UNPACKED / RENDER_ELEM_SIZE_PACKED;

      int transferSize = 0;
      dag::Vector<uint8_t, framemem_allocator> tmpMem;

      for (int p = 0, pe = pools.size(); p < pe; ++p)
      {
        const EntPool &pool = pools[p];
        bool posInst = rtData->riPosInst[p];
        int srcStride = RIGEN_STRIDE_B(posInst, rtData->riZeroInstSeeds[p], rgd.perInstDataDwords);
        int dstStride = srcStride * unpackRatio;
        int dstSize = pool.total * dstStride;

        if (pool.total == 0)
          continue;

        if (tmpMem.size() < dstSize)
          tmpMem.resize(dstSize);

        transferSize += dstSize;
        const uint8_t *src = sysMemData.get() + pool.baseOfs;
        uint8_t *dst = tmpMem.data();
        if (posInst)
        {
          for (int i = 0; i < pool.total; ++i)
          {
            vec4f pos, scale;
            vec4i palette_id;
            rendinst::gen::unpack_tm_pos(pos, scale, (const int16_t *)(src + i * srcStride), v_cell_add, v_cell_mul, true,
              &palette_id);
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
            rendinst::gen::unpack_tm_full(tm, (const int16_t *)(src + i * srcStride), v_cell_add, v_cell_mul);
            v_mat44_transpose(tm, tm);
            uint8_t *d = dst + i * dstStride;
            v_stu(d + sizeof(Point4) * 0, tm.col0);
            v_stu(d + sizeof(Point4) * 1, tm.col1);
            v_stu(d + sizeof(Point4) * 2, tm.col2);

#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS
            for (int j = 0; j < rgd.perInstDataDwords; ++j)
            {
              uint16_t packed = *((const uint16_t *)(src + i * srcStride + RIGEN_TM_STRIDE_B(false, 0) + j * sizeof(uint16_t)));
              uint32_t unpacked = packed; // Written as uint32, and read back with asuint from the shader.
              memcpy(d + RIGEN_TM_STRIDE_B(false, 0) * unpackRatio + j * sizeof(float), &unpacked, sizeof(uint32_t));
            }
#endif
          }
        }
        rtData->cellsVb.getHeap().getBuf()->updateData(vbInfo.offset + pool.baseOfs * unpackRatio, dstSize, dst, VBLOCK_WRITEONLY);
      }
      G_ASSERTF(transferSize == size * unpackRatio, "RendInstGenData::CellRtData::update dstSize != transferSize (%d/%d)",
        size * unpackRatio, transferSize);
      G_UNUSED(size);
    }
    else
    {
      rtData->cellsVb.getHeap().getBuf()->updateData(vbInfo.offset, size, sysMemData.get(), VBLOCK_WRITEONLY);
      G_UNUSED(rgd);
    }

    heapGen = rtData->cellsVb.getManager().getHeapGeneration();
  }

  if (rendinst::render::avoidStaticShadowRecalc)
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

eastl::array<uint32_t, 2> rendinst::render::getCommonImmediateConstants()
{
  const auto halfFaloffStart = static_cast<uint32_t>(float_to_half(treeCrownTransmittance.falloffStart));
  const auto halfBrightness = static_cast<uint32_t>(float_to_half(treeCrownTransmittance.brightness));
  const auto halfCascade0Dist = static_cast<uint32_t>(float_to_half(cascade0Dist));
  const auto halfFalloffStop = static_cast<uint32_t>(float_to_half(treeCrownTransmittance.falloffStop));
  return {(halfFaloffStart << 16) | halfBrightness, (halfCascade0Dist << 16) | halfFalloffStop};
}

void RendInstGenData::renderOptimizationDepthPrepass(const RiGenVisibility &visibility, const TMatrix &view_itm)
{
  if (!visibility.hasOpaque())
    return;
  TIME_D3D_PROFILE_NAME(riOptimizationPrepass,
    rendinst::isRgLayerPrimary(rtData->layerIdx) ? "ri_depth_prepass" : "ri_depth_prepass_sec");

  set_up_left_to_shader(view_itm);
  ShaderGlobal::set_real_fast(rendinst::render::lods_shift_dist_mul_varId,
    rendinst::render::lodsShiftDistMul / rtData->rendinstDistMulImpostorTrees / rtData->impostorsDistAdditionalMul);

  int layerIdx = rtData->layerIdx;
  rendinst::render::setPerDrawData(rendinst::render::riGenPerDrawDataForLayer[layerIdx]);
  ShaderGlobal::setBlock(rendinst::render::rendinstDepthSceneBlockId, ShaderGlobal::LAYER_SCENE);
  const rendinst::RenderPass renderPass = rendinst::RenderPass::Depth; // even rendinst::RenderPass::Normal WILL optimize pass, due
                                                                       // to color_write off
  ShaderGlobal::set_int_fast(rendinst::render::rendinstRenderPassVarId, eastl::to_underlying(renderPass));

  rendinst::render::startRenderInstancing();

  rendinst::render::RiGenRenderer renderer(renderPass, {}, true, false, rendinst::render::depth_prepass_for_impostors,
    rendinst::render::depth_prepass_for_cells, perInstDataDwords, drawOrderVarId);

  if (rendinst::render::per_instance_visibility)
    renderer.addInstanceVisibleObjects(*rtData, visibility);

  if (rendinst::render::depth_prepass_for_cells)
  {
    ShaderGlobal::set_int_fast(isRenderByCellsVarId, 1);

    WinAutoLock lock(rtData->updateVbResetCS);
    d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rtData->cellsVb.getHeap().getBuf());
    renderer.addCellVisibleObjects(*rtData, visibility, cells, {cellNumW, cellNumH});
  }

  renderer.render(*rtData, visibility);

  rendinst::render::endRenderInstancing();
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
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
  if (!visibility.hasOpaque() || !rendinst::render::per_instance_visibility || fabsf(viewDir.y) > 0.5f)
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

  for (int lodI = 0; lodI < visibility.PI_IMPOSTOR_LOD; ++lodI)
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

void RendInstGenData::updateHeapVbNoLock()
{
  TIME_D3D_PROFILE(updateHeapVb);

  // To consider: maintain one combined generation for all cels and skip whole this loop when it's equal

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

void RendInstGenData::renderPreparedOpaque(rendinst::RenderPass render_pass, rendinst::LayerFlags layer_flags,
  const RiGenVisibility &visibility, const TMatrix &view_itm, bool depth_optimized)
{
  TIME_D3D_PROFILE_NAME(render_prepared_opaque,
    rendinst::isRgLayerPrimary(rtData->layerIdx) ? "render_prepared_opaque" : "render_prepared_opaque_sec");

  if (!visibility.hasOpaque() && !visibility.hasTransparent())
    return;

  set_up_left_to_shader(view_itm);
  ShaderGlobal::set_real_fast(rendinst::render::lods_shift_dist_mul_varId,
    rendinst::render::lodsShiftDistMul / rtData->rendinstDistMulImpostorTrees / rtData->impostorsDistAdditionalMul);

  const int blockToSet = (render_pass == rendinst::RenderPass::Depth) || (render_pass == rendinst::RenderPass::ToShadow)
                           ? rendinst::render::rendinstDepthSceneBlockId
                           : rendinst::render::rendinstSceneBlockId;

  int layerIdx = rtData->layerIdx;
  const bool needToSetBlock = rendinst::render::setBlock(blockToSet, rendinst::render::riGenPerDrawDataForLayer[layerIdx]);

  ShaderGlobal::set_int_fast(rendinst::render::rendinstRenderPassVarId,
    eastl::to_underlying(render_pass)); // rendinst_render_pass_to_shadow.
  ShaderGlobal::set_int_fast(rendinst::render::baked_impostor_multisampleVarId, 1);
  ShaderGlobal::set_int_fast(rendinst::render::vertical_impostor_slicesVarId, rendinst::render::vertical_billboards ? 1 : 0);

  ShaderGlobal::set_real_fast(globalTranspVarId, 1.f); // Force not-transparent branch.

  rendinst::render::RiGenRenderer renderer(render_pass, layer_flags, false, depth_optimized,
    rendinst::render::depth_prepass_for_impostors, rendinst::render::depth_prepass_for_cells, perInstDataDwords, drawOrderVarId);

  rendinst::render::startRenderInstancing();

  if (rendinst::render::per_instance_visibility &&
      !(render_pass == rendinst::RenderPass::Normal && layer_flags == rendinst::LayerFlag::Decals))
  {
    ShaderGlobal::set_int_fast(isRenderByCellsVarId, 0);
    renderer.addInstanceVisibleObjects(*rtData, visibility);
  }

  ScopedLockRead lock(rtData->riRwCs);

  updateHeapVbNoLock();

  ShaderGlobal::set_int_fast(isRenderByCellsVarId, 1);

  {
    WinAutoLock lock(rtData->updateVbResetCS);
    d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rtData->cellsVb.getHeap().getBuf());
    renderer.addCellVisibleObjects(*rtData, visibility, cells, {cellNumW, cellNumH});
  }

  renderer.render(*rtData, visibility);

  rendinst::render::endRenderInstancing();

  if (needToSetBlock)
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
}

void RendInstGenData::render(rendinst::RenderPass render_pass, const RiGenVisibility &visibility, const TMatrix &view_itm,
  rendinst::LayerFlags layer_flags, bool depth_optimized)
{
  if (layer_flags & (rendinst::LayerFlag::NotExtra | rendinst::LayerFlag::Decals | rendinst::LayerFlag::Transparent))
    renderPreparedOpaque(render_pass, layer_flags, visibility, view_itm, depth_optimized);
}

bool prepass_trees = true;
static bool render_sec_layer = true;

bool rendinst::render::enableSecLayerRender(bool en)
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

bool rendinst::render::isSecLayerRenderEnabled() { return render_sec_layer; }

bool rendinst::render::enablePrimaryLayerRender(bool en)
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

bool rendinst::render::enableRiExtraRender(bool en)
{
  bool v = rendinst::render::ri_extra_render_enabled;
  rendinst::render::ri_extra_render_enabled = en;
  return v;
}

void rendinst::render::renderGpuObjectsFromVisibility(RenderPass render_pass, const RiGenVisibility *visibility,
  LayerFlags layer_flags)
{
  if ((layer_flags & LayerFlag::Opaque) && ri_extra_render_enabled)
  {
    gpuobjects::render_layer(render_pass, visibility, layer_flags, LayerFlag::Opaque);
  }
}

void rendinst::render::renderRIGen(RenderPass render_pass, const RiGenVisibility *visibility, const TMatrix &view_itm,
  LayerFlags layer_flags, OptimizeDepthPass depth_optimized, uint32_t instance_count_mul, AtestStage atest_stage,
  const RiExtraRenderer *riex_renderer, TexStreamingContext texCtx)
{
  if (!RendInstGenData::renderResRequired || RendInstGenData::isLoading || !unitedvdata::riUnitedVdata.getIB())
    return;

  G_ASSERT_RETURN(visibility, );

  const OptimizeDepthPass optimize_depth = !(layer_flags & LayerFlag::ForGrass) ? OptimizeDepthPass::Yes : OptimizeDepthPass::No;

  bool depthOrShadowPass = (render_pass == rendinst::RenderPass::Depth) || (render_pass == rendinst::RenderPass::ToShadow);
  if (depthOrShadowPass)
    texCtx = TexStreamingContext(0);

  set_up_left_to_shader(view_itm);
  if (layer_flags & (LayerFlag::Opaque | LayerFlag::Transparent | LayerFlag::RendinstClipmapBlend | LayerFlag::RendinstHeightmapPatch |
                      LayerFlag::Distortion))
  {
    ShaderGlobal::set_int_fast(rendinstRenderPassVarId, eastl::to_underlying(render_pass));
    if ((layer_flags & LayerFlag::Opaque) && ri_extra_render_enabled)
    {
      renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
        LayerFlag::Opaque, instance_count_mul, texCtx, atest_stage, riex_renderer);

      gpuobjects::render_layer(render_pass, visibility, layer_flags, LayerFlag::Opaque);
    }

    if ((layer_flags & LayerFlag::RendinstClipmapBlend) && ri_extra_render_enabled)
      renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
        LayerFlag::RendinstClipmapBlend, instance_count_mul, texCtx);

    if ((layer_flags & LayerFlag::RendinstHeightmapPatch) && ri_extra_render_enabled)
      renderRIGenExtra(visibility[0], render_pass, OptimizeDepthPass::No, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
        LayerFlag::RendinstHeightmapPatch, instance_count_mul, texCtx, AtestStage::All);

    if ((layer_flags & LayerFlag::Transparent) && ri_extra_render_enabled)
    {
      renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
        LayerFlag::Transparent, instance_count_mul, texCtx);

      gpuobjects::render_layer(render_pass, visibility, layer_flags, LayerFlag::Transparent);
    }

    if ((layer_flags & LayerFlag::Distortion) && ri_extra_render_enabled)
    {
      renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
        LayerFlag::Distortion, instance_count_mul, texCtx);

      gpuobjects::render_layer(render_pass, visibility, layer_flags, LayerFlag::Distortion);
    }
  }

  const bool tree_depth_optimized = (depth_optimized == OptimizeDepthPass::Yes) && prepass_trees;
  if (layer_flags & LayerFlag::NotExtra)
  {
    ShaderGlobal::set_int_fast(rendinstRenderPassVarId, eastl::to_underlying(render_pass));
    G_ASSERT(render_pass == rendinst::RenderPass::Normal || !tree_depth_optimized);
    disableRendinstAlphaForNormalPassWithZPrepass();
    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      rgl->render(render_pass, visibility[_layer], view_itm, layer_flags & ~LayerFlag::Decals, tree_depth_optimized);
    restoreRendinstAlphaForNormalPassWithZPrepass();
  }

  if ((layer_flags & LayerFlag::Decals) && ri_extra_render_enabled)
  {
    renderRIGenExtra(visibility[0], render_pass, optimize_depth, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
      LayerFlag::Decals, instance_count_mul, texCtx);

    gpuobjects::render_layer(render_pass, visibility, layer_flags, LayerFlag::Decals);

    disableRendinstAlphaForNormalPassWithZPrepass();
    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      rgl->render(render_pass, visibility[_layer], view_itm, LayerFlag::Decals, tree_depth_optimized);
    restoreRendinstAlphaForNormalPassWithZPrepass();
  }
}

void rendinst::render::renderRIGenOptimizationDepth(RenderPass render_pass, const RiGenVisibility *visibility, const TMatrix &view_itm,
  IgnoreOptimizationLimits ignore_optimization_instances_limits, SkipTrees skip_trees, RenderGpuObjects gpu_objects,
  uint32_t instance_count_mul, TexStreamingContext texCtx)
{
  if (!RendInstGenData::renderResRequired || RendInstGenData::isLoading || !unitedvdata::riUnitedVdata.getIB())
    return;

  G_ASSERT(visibility);
  ShaderGlobal::set_int_fast(rendinstRenderPassVarId, eastl::to_underlying(render_pass));
  renderRIGenExtra(visibility[0], render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::Yes, ignore_optimization_instances_limits,
    LayerFlag::Opaque, instance_count_mul, texCtx);

  if (visibility[0].gpuObjectsCascadeId != -1 && gpu_objects == RenderGpuObjects::Yes)
    gpuobjects::render_optimization_depth(render_pass, visibility, ignore_optimization_instances_limits, instance_count_mul);

  if (skip_trees == SkipTrees::No)
    renderRITreeDepth(visibility, view_itm);
}

void rendinst::render::renderRITreeDepth(const RiGenVisibility *visibility, const TMatrix &view_itm)
{
  G_ASSERT(visibility);

  if (prepass_trees)
    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      rgl->renderOptimizationDepthPrepass(visibility[_layer], view_itm);
}

void rendinst::render::renderRIGen(RenderPass render_pass, mat44f_cref globtm, const Point3 &view_pos, const TMatrix &view_itm,
  LayerFlags layer_flags, bool for_vsm, TexStreamingContext texCtx)
{
  if (!RendInstGenData::renderResRequired || RendInstGenData::isLoading || !unitedvdata::riUnitedVdata.getIB())
    return;
  Frustum frustum;
  frustum.construct(globtm);
  RiGenVisibility visibility(tmpmem);
  const bool for_shadow = render_pass == RenderPass::ToShadow;
  if (for_shadow)
    texCtx = TexStreamingContext(0);
  const uint32_t instance_count_mul = 1;
  if (auto layerForcedLod = get_forced_lod(layer_flags);
      layerForcedLod >= 0 && visibility.forcedLod < 0 && visibility.riex.forcedExtraLod < 0)
    visibility.forcedLod = visibility.riex.forcedExtraLod = layerForcedLod;

  ShaderGlobal::set_int_fast(rendinstRenderPassVarId, eastl::to_underlying(render_pass));
  set_up_left_to_shader(view_itm);

  if (layer_flags & (LayerFlag::Opaque | LayerFlag::Transparent | LayerFlag::Decals | LayerFlag::Distortion))
    if (prepareRIGenExtraVisibility(globtm, view_pos, visibility, for_shadow, nullptr, {}, rendinst::RiExtraCullIntention::MAIN, false,
          false, for_vsm))
    {
      if (layer_flags & LayerFlag::Opaque)
        renderRIGenExtra(visibility, render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
          LayerFlag::Opaque, instance_count_mul, texCtx);

      if (layer_flags & LayerFlag::Transparent)
        renderRIGenExtra(visibility, render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
          LayerFlag::Transparent, instance_count_mul, texCtx);

      if (layer_flags & LayerFlag::Distortion)
        renderRIGenExtra(visibility, render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
          LayerFlag::Distortion, instance_count_mul, texCtx);
    }

  if ((layer_flags & (LayerFlag::Opaque | LayerFlag::Transparent)) && !for_vsm)
  {
    bool shouldUseSeparateAlpha = !(layer_flags & LayerFlag::NoSeparateAlpha);
    if (shouldUseSeparateAlpha)
      disableRendinstAlphaForNormalPassWithZPrepass();

    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      if (rgl->prepareVisibility(frustum, view_pos, visibility, for_shadow, layer_flags, nullptr))
        rgl->render(render_pass, visibility, view_itm,
          (layer_flags & LayerFlag::Transparent) ? LayerFlag::Transparent : (layer_flags & ~LayerFlag::Decals), false);

    if (shouldUseSeparateAlpha)
      restoreRendinstAlphaForNormalPassWithZPrepass();
  }

  if ((layer_flags & LayerFlag::Decals) && !for_vsm)
  {
    renderRIGenExtra(visibility, render_pass, OptimizeDepthPass::Yes, OptimizeDepthPrepass::No, IgnoreOptimizationLimits::No,
      LayerFlag::Decals, instance_count_mul, texCtx);
#if _TARGET_PC_TOOLS_BUILD // only render decals for riGen in Tools
    disableRendinstAlphaForNormalPassWithZPrepass();
    FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
      if (rgl->prepareVisibility(frustum, view_pos, visibility, for_shadow, layer_flags, nullptr))
        rgl->render(render_pass, visibility, view_itm, LayerFlag::Decals, false);
    restoreRendinstAlphaForNormalPassWithZPrepass();
#endif
  }
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

  for (rendinst::render::RtPoolData *rtpool : rtData->rtPoolData)
  {
    if (!rtpool)
      continue;
    rendinst::render::RtPoolData &pool = *rtpool;
    pool.flags &= ~(rendinst::render::RtPoolData::HAS_CLEARED_LIGHT_TEX);
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

  rendinst::gpuobjects::after_device_reset();
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

void rendinst::render::renderDebug()
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
  bool updateLodRanges = rendinst::render::per_instance_visibility_for_everyone != on;
  rendinst::render::per_instance_visibility_for_everyone = on;

  if (updateLodRanges)
  {
    FOR_EACH_RG_LAYER_DO (rgl)
      rgl->applyLodRanges();
  }
}

void rendinst::render::setRIGenRenderMode(int mode)
{
  if (rendinst::ri_game_render_mode != mode)
  {
    debug("ri_mode: %d->%d", rendinst::ri_game_render_mode, mode);
  }
  rendinst::ri_game_render_mode = mode;
}
int rendinst::render::getRIGenRenderMode() { return rendinst::ri_game_render_mode; }

bool rendinst::render::pendingRebuild() { return interlocked_acquire_load(pendingRebuildCnt) != 0; }

void rendinst::render::before_draw(RenderPass render_pass, const RiGenVisibility *visibility, const Frustum &frustum,
  const Occlusion *occlusion, const char *mission_name, const char *map_name, bool gpu_instancing)
{
  rendinst::gpuobjects::before_draw(render_pass, visibility, frustum, occlusion, mission_name, map_name, gpu_instancing);
  rendinst::render::ensureElemsRebuiltRIGenExtra(gpu_instancing);
  FOR_EACH_RG_LAYER_DO (rgl)
    if (rgl->rtData)
      RiGenRenderer::updatePackedData(rgl->rtData->layerIdx);
}

void rendinst::updateHeapVb()
{
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
  {
    ScopedLockRead lock(rgl->rtData->riRwCs);
    rgl->updateHeapVbNoLock();
  }
}


REGISTER_D3D_AFTER_RESET_FUNC(rendinst_afterDeviceReset);

void rendinst::render::log_riex_number(const RiGenVisibility *visibility)
{
  eastl::vector<eastl::pair<int, eastl::string>> riList;
  for (auto poolAndCnt : visibility->riex.riexPoolOrder)
  {
    auto poolIndex = poolAndCnt & 0x3FFF;
    const char *riName = rendinst::riExtraMap.getName(poolIndex);

    uint32_t poolCnt = 0;
    uint32_t countByLod[rendinst::RiExtraPool::MAX_LODS] = {0};
    for (int lodNo = 0; lodNo < rendinst::RiExtraPool::MAX_LODS; lodNo++)
      if (poolIndex < visibility->riex.riexData[lodNo].size())
      {
        uint32_t cnt = (uint32_t)visibility->riex.riexData[lodNo][poolIndex].size() / rendinst::RIEXTRA_VECS_COUNT;
        countByLod[lodNo] += cnt;
        poolCnt += cnt;
      }

    eastl::string info(eastl::string::CtorSprintf(), "%d of '%s' (lods: ", poolCnt, riName ? riName : "?");
    for (int lodNo = 0; lodNo < rendinst::RiExtraPool::MAX_LODS; lodNo++)
      info += eastl::string(eastl::string::CtorSprintf(), "%s%d", lodNo == 0 ? "" : "/", countByLod[lodNo]);
    info += ")";

    riList.emplace_back(poolCnt, eastl::move(info));
  }
  eastl::sort(riList.begin(), riList.end(), [](auto &l, auto &r) { return l.first > r.first; });
  for (auto riInfo : riList)
    debug("%s", riInfo.second.c_str());

  eastl::string riInfo;
  for (int instPerLod : visibility->instNumberCounter)
    riInfo += eastl::string(eastl::string::CtorSprintf(), "%s%d", riInfo.empty() ? "" : "/", instPerLod);
  debug("RI lods: %s", riInfo);
}

bool rendinst::render::verify_riex_number(const RiGenVisibility *visibility)
{
  if (visibility && !canIncreaseRenderBuffer)
  {
    int safeMaxExtraRiCount = rendinst::maxExtraRiCount;
#if DAGOR_DBGLEVEL > 0
    safeMaxExtraRiCount *= 0.8f;
#endif

    if (visibility->riex.riexInstCount > safeMaxExtraRiCount)
    {
      debug("Too many RIEx to render: %d > %d (max=%d)", visibility->riex.riexInstCount, safeMaxExtraRiCount,
        rendinst::maxExtraRiCount);

      log_riex_number(visibility);
      return false;
    }
  }

  return true;
}
