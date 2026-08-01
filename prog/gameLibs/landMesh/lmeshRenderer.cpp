// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/lmeshWeightAtlas.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_resetDevice.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_bounds3.h>
#include <math/dag_frustum.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaders.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IBBox2.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <landMesh/lmeshRenderer.h>
#include <landMesh/lmeshManager.h>
#include "heightmap/heightmapHandler.h"
#include "lmeshRendererGlue.h"
#include "lmeshRenderingModeState.h"
#include <memory/dag_framemem.h>
#include <EASTL/bitset.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <perfMon/dag_cpuFreq.h>
#include "lmeshRendererCommon.h"


#define ACQUIRE_MANAGED_TEX(t) ((t) == BAD_TEXTUREID ? NULL : acquire_managed_tex((t)))
#define RELEASE_MANAGED_TEX(t) ((t) == BAD_TEXTUREID ? ((void)0) : release_managed_tex((t)))

#define TILE_CELLS_DIST2 2
namespace var
{
#define VAR(a) ShaderVariableInfo a(#a, true);
LMESH_VAR_LIST
#undef VAR
} // namespace var

static int land_mesh_object_blkid[LandMeshRenderer::RENDER_TYPES_COUNT] = {-1, -1, -1, -1, -1};
static const char *rtypes_block_name[LandMeshRenderer::RENDER_TYPES_COUNT] = {
  "land_mesh_with_splatting",
  "land_mesh_prepare_clipmap",
  "land_mesh_grass_mask",
  "land_mesh_with_clipmap",
  "land_mesh_with_clipmap_reflection", //"land_mesh_grass_color",
  "land_mesh_render_depth",
  "land_mesh_render_depth", // RENDER_ONE_SHADER //feedback, heightmap, vsm - uses only one shader for all landmeshes and
                            // landmesh_combined
  "land_mesh_prepare_clipmap",
};

static DynamicShaderHelper bboxShader;
const int BOX_VERTEXES_COUNT = 6 * 6;
static const float BOTTOM_ZBIAS = -1e-6f;

static const int MAX_CR_SURVEYS = 1;
static bool useConditionalRendering = true;
static uint8_t landmesh_cr_surveys = MAX_CR_SURVEYS;

// Shader-constant register slots. Resolved once from int shader vars in the ctor
// (kept at the listed default when the slot is absent from the shader).

#define VAR(a, def) int a = def;
LMESH_CONST_LIST
#undef VAR

carray<int, LandMeshManager::DECALS_OVERRIDE_SAMPLERS_COUNT> decals_overrideSampler_const_no_array;

int get_shader_int_constant(const char *name, int def)
{
  int varId = ::get_shader_variable_id(name, true);
  if (VariableMap::isGlobVariablePresent(varId))
    return ShaderGlobal::get_int_fast(varId);
  return def;
}


Vbuffer *one_quad = NULL;
static const char *landclassShaderName[LC_COUNT] = {"land_mesh_landclass_simple", "land_mesh_landclass_detailed",
  "land_mesh_landclass_detailed_r", "land_mesh_landclass_mega_nonormal", "land_mesh_landclass_mega_detailed",
  "land_mesh_landclass_trivial"};

eastl::vector<ShaderInfo> landclassShader;
eastl::vector<uint64_t> landclassShaderFailed;


static void init_one_quad()
{
  del_d3dres(one_quad);

  one_quad = d3d::create_vb(4 * 4 * 2, 0, "lm-1quad", RESTAG_LAND);
  d3d_err(one_quad);
  static const short int minus1 = -32768;
  signed short vertQuad[4][4] = {minus1, 0, minus1, 32767, minus1, 0, 32767, 32767, 32767, 0, minus1, 32767, 32767, 0, 32767, 32767};
  one_quad->updateDataWithLock(0, sizeof(vertQuad), vertQuad, VBLOCK_WRITEONLY);
}

static void lmesh_after_reset_device(bool)
{
  if (one_quad)
    init_one_quad();
}

REGISTER_D3D_AFTER_RESET_FUNC(lmesh_after_reset_device);


int LandMeshRenderer::lod1_switch_radius = 4096;
enum
{
  BOTTOM_ABOVE = 0,
  BOTTOM_BELOW = 1,
  BOTTOM_COUNT,
};


void LandMeshRenderer::MirroredCellState::init(int borderX, int borderY, int x0, int y0, int x1, int y1, float cellSize,
  float gridCellSize, Point4 posToWorld[2], Color4 detMapTc, bool to_be_excluded)
{
  int mirrorX = LandMeshCullingState::mirror_x(borderX, x0, x1);
  int mirrorY = LandMeshCullingState::mirror_x(borderY, y0, y1);
  cellX = mirrorX - x0;
  cellY = mirrorY - y0;

  posToWorldSet[0] = posToWorld[0];
  posToWorldSet[1] = posToWorld[1];
  mirrorScaleState.xz = 0;

  detMapTcSet = detMapTc;

  if ((borderX != mirrorX) || (borderY != mirrorY))
  {
    if (borderX != mirrorX)
    {
      posToWorldSet[0].x = -posToWorld[0].x;
      float ofs = (borderX - mirrorX) * cellSize;
      if (borderX > x1)
      {
        posToWorldSet[1].x = posToWorld[1].x + ofs;
        detMapTcSet[2] = -detMapTc[2] + (ofs - gridCellSize + cellSize) * detMapTc[0];
        mirrorScaleState.xz = CellState::MIRRORED_POS;
      }
      else
      {
        posToWorldSet[1].x = posToWorld[1].x + ofs;
        detMapTcSet[2] = -detMapTc[2] + (ofs + cellSize) * detMapTc[0];
        mirrorScaleState.xz = CellState::MIRRORED_NEG;
      }
      detMapTcSet[0] = -detMapTc[0];
    }
    if (borderY != mirrorY)
    {
      posToWorldSet[0].z = -posToWorld[0].z;
      float ofs = (borderY - mirrorY) * cellSize;
      if (borderY > y1)
      {
        posToWorldSet[1].z = posToWorld[1].z + ofs;
        detMapTcSet[3] = -detMapTc[3] + (ofs - gridCellSize + cellSize) * detMapTc[1];
        mirrorScaleState.xz += 3 * CellState::MIRRORED_POS;
      }
      else
      {
        posToWorldSet[1].z = posToWorld[1].z + ofs;
        detMapTcSet[3] = -detMapTc[3] + (ofs + cellSize) * detMapTc[1];
        mirrorScaleState.xz += 3 * CellState::MIRRORED_NEG;
      }
      detMapTcSet[1] = -detMapTc[1];
    }
  }

  invcull = (mirrorScaleState.xz % 3 ? 1 : 0) ^ (mirrorScaleState.xz / 3 ? 1 : 0);
  excluded = to_be_excluded ? 1 : 0;
}

void LandMeshRenderer::MirroredCellState::startRender(RenderPassCtx &pass_ctx)
{
  set_ps_const1_opt(lmesh_ps_const__mirror_scale, 1.f, 1.f);
  pass_ctx.mirrorMask = 0;
}

void LandMeshRenderer::MirroredCellState::setPosConsts(bool render_at_0) const
{
  if (render_at_0)
  {
    Point4 posToWorldSetZeroY[2];
    posToWorldSetZeroY[0] = posToWorldSet[0];
    posToWorldSetZeroY[1] = posToWorldSet[1];
    posToWorldSetZeroY[0].y = 0;
    posToWorldSetZeroY[1].y = 0;
    d3d::set_vs_const(lmesh_vs_const__pos_to_world, (float *)&posToWorldSetZeroY[0].x, 2);
  }
  else
    d3d::set_vs_const(lmesh_vs_const__pos_to_world, (float *)&posToWorldSet[0].x, 2);
}

void LandMeshRenderer::MirroredCellState::setDetMapTc() const
{
  d3d::set_vs_const(lmesh_vs_const__mul_offset_base + 8, &detMapTcSet.r, 1);
}

static const uint32_t mirror_mask_table = (0 << 0) | (1 << 2) | (1 << 4)      // z = 0
                                          | (2 << 6) | (3 << 8) | (3 << 10)   // z = 1
                                          | (2 << 12) | (3 << 14) | (3 << 16) // z = 2
  ;

void LandMeshRenderer::MirroredCellState::setPsMirror(RenderPassCtx &pass_ctx) const
{
  uint32_t mirror_mask = 0x3 & (mirror_mask_table >> (mirrorScaleState.xz << 1));
  if (lmesh_ps_const__mirror_scale >= 0 && mirror_mask != pass_ctx.mirrorMask)
  {
    d3d::set_ps_const1(lmesh_ps_const__mirror_scale, (mirror_mask & 1) ? -1.f : 1.f, (mirror_mask & 2) ? -1.f : 1.f, 0, 0);
    pass_ctx.mirrorMask = mirror_mask;
  }
}

bool LandMeshRenderer::MirroredCellState::setFlipCull(LandMeshRenderer *renderer, RenderPassCtx &pass_ctx) const
{
  if (getInvCull() != pass_ctx.cullFlipped)
  {
    if (pass_ctx.cullFlipped)
    {
      G_ASSERT(shaders::overrides::get_current() == pass_ctx.cullFlippedCurStateId);
      renderer->resetOverride(pass_ctx.cullFlippedPrevStateId);
    }
    else
    {
      G_ASSERT(!pass_ctx.cullFlippedPrevStateId);
      pass_ctx.cullFlippedPrevStateId = renderer->setStateFlipCull(true);
      pass_ctx.cullFlippedCurStateId = shaders::overrides::get_current();
    }
    pass_ctx.cullFlipped = !pass_ctx.cullFlipped;
    return true;
  }
  return false;
}

void landmesh::resolve_lmesh_shader_constants()
{
  if (d3d::get_driver_desc().info.vendor == GpuVendor::QUALCOMM)
    var::skip_detail_lod_calculation.set_int(1);

#define VAR(a, def) a = get_shader_int_constant(#a, a);
  LMESH_CONST_LIST
#undef VAR
  if (lmesh_sampler__land_detail_array1 < 0)
    lmesh_ps_const_land_detail_array_slices = -1;
  if (lmesh_ps_const_land_detail_array_slices < 0)
    lmesh_sampler__land_detail_array1 = -1;

  G_ASSERT(lmesh_sampler__land_detail_map >= 0);
  G_ASSERT(lmesh_sampler__land_detail_tex1 >= 0);

  for (int i = 0; i < decals_overrideSampler_const_no_array.size(); ++i)
  {
    String smpReg(64, "decals_overrideSampler_%i_const_no", i);
    decals_overrideSampler_const_no_array[i] = get_shader_int_constant(smpReg, -1);
  }
}

LandMeshRenderer::LandMeshRenderer(LandMeshManager &provider) : optScn(NULL), tWidth(0)
{
  verLabel = VER_LABEL;
  debugCells = false;

  cellStates = NULL;

  regionCallback = NULL;
  numBorderCellsXPos = 0;
  numBorderCellsXNeg = 0;
  numBorderCellsZPos = 0;
  numBorderCellsZNeg = 0;

  centerCell = IPoint2(0xDEAFBABE, 0xDEAFBABE);

  invGeomLodDist = 1.f / (sqrt(2.0f) * lod1_switch_radius);

  int default_objectid = ShaderGlobal::getBlockId("land_mesh_game_scene");
  for (int i = 0; i < RENDER_TYPES_COUNT; ++i)
  {
    land_mesh_object_blkid[i] = ShaderGlobal::getBlockId(rtypes_block_name[i]);
    if (land_mesh_object_blkid[i] < 0)
    {
      // if (i == RENDER_COMBINED_LAST)
      //   land_mesh_object_blkid[i] = land_mesh_object_blkid[RENDER_WITH_CLIPMAP];
      // else
      land_mesh_object_blkid[i] = default_objectid;
    }
  }
  static CompiledShaderChannelId channels[1] = {
    {SCTYPE_FLOAT3, SCUSAGE_POS, 0, 0},
  };

  useConditionalRendering = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("useConditionalRendering", false);
  int num_surveys = ::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("conditionalBatchSize", MAX_CR_SURVEYS);
  landmesh_cr_surveys = uint8_t(min<int>(num_surveys, MAX_CR_SURVEYS));
#if _TARGET_PC
  useConditionalRendering = false; // conditional rendering not support
#endif
  if (useConditionalRendering) //-V547
  {
    bboxShader.init("occlusion_box", channels, countof(channels), "LensFlareFx_occlusion", true);
    if (!bboxShader.material || !bboxShader.shader)
    {
      bboxShader.close();
      useConditionalRendering = false;
    }
  }

  vtex = provider.getVtexRenderer();
  G_ASSERT(vtex);
}

LandVtexRenderer::LandVtexRenderer(LandMeshManager &provider, dag::ConstSpan<LandClassDetailTextures> land_classes,
  int biome_land_class_idx, TEXTUREID vert_tex_id, TEXTUREID vert_nm_tex_id, TEXTUREID vert_det_tex_id, TEXTUREID tile_tex,
  d3d::SamplerHandle tile_smp, real tile_x, real tile_y) :
  tileTexId(tile_tex), tileTexSmp(tile_smp), tileXSize(tile_x), tileYSize(tile_y)
{
  mem_set_0(megaDetailsArray);
  landClasses = land_classes;
  biomeLandClassIdx = biome_land_class_idx;
  vertTexId = vert_tex_id;
  vertNmTexId = vert_nm_tex_id;
  vertDetTexId = vert_det_tex_id;

  if (lmesh_physmats__buffer_idx > 0)
    physmatIdsBuf = d3d::buffers::create_one_frame_sr_byte_address(DET_TEX_NUM * 4, "physmats_IDS", RESTAG_LAND);

  ShaderGlobal::set_texture(var::vertical_tex, vertTexId);
  ShaderGlobal::set_texture(var::vertical_nm_tex, vertNmTexId);
  ShaderGlobal::set_texture(var::vertical_det_tex, vertDetTexId);

  int detMapElemSize, size;
  provider.getDetailMapSize(detMapElemSize, size);
  float cellSize = provider.getLandCellSize();
  detMapTcScale = detMapElemSize / (size * cellSize);

  for (int i = 0; i < landClasses.size(); ++i)
  {
    if (landClasses[i].lcType != LC_SIMPLE)
    {
      has_detailed_land_classes = true;
      break;
    }
  }

  landclassShader.resize(LC_COUNT);
  for (int li = 0; li < LC_COUNT; ++li)
  {
    landclassShader[li].material.reset(new_shader_material_by_name_optional(landclassShaderName[li], ""));
    if (!landclassShader[li].material)
    {
      if (li != LC_MEGA_DETAILED && li != LC_DETAILED_R)
        DAG_FATAL("can't create ShaderMaterial for '%s'", landclassShaderName[li]);
    }
    else
      landclassShader[li].elem = landclassShader[li].material->make_elem();
    landclassShader[li].lc_textures_sampler = lmesh_sampler__land_detail_tex1 + 1;
    landclassShader[li].vs_const_offset = lmesh_vs_const__mul_offset_base + 1;
    landclassShader[li].lc_detail_const_offset = lmesh_ps_const_land_detail_array_slices;
  }

  if (!one_quad)
    init_one_quad();
}

LandMeshRenderer::~LandMeshRenderer()
{
  delete[] optScn;

  if (cellStates)
    delete[] cellStates;
  cellStates = NULL;

  if (useConditionalRendering)
    bboxShader.close();
}

LandVtexRenderer::~LandVtexRenderer()
{
  resetTextures();

  for (const ShaderVariableInfo *v : {&var::vertical_tex, &var::vertical_nm_tex, &var::vertical_det_tex})
    ShaderGlobal::set_texture(*v, BAD_TEXTUREID);
  del_d3dres(one_quad);
  del_d3dres(physmatIdsBuf);
  landclassShader.clear();
  landclassShaderFailed.clear();
}

// Shared override-state create/lookup logic. Each caller passes its own cache map, so
// LandMeshRenderer (geometry) and LandVtexRenderer (splatting) stay independent.
shaders::OverrideStateId landmesh::set_override(landmesh::OverrideStateCache &overrideStateMap,
  const shaders::OverrideState &new_state)
{
  shaders::OverrideState combinedState;
  shaders::OverrideStateId curStateId = shaders::overrides::get_current();
  bool resetFlipCull = false;
  if (curStateId)
  {
    shaders::OverrideState curState = shaders::overrides::get(curStateId);
    resetFlipCull = curState.isOn(shaders::OverrideState::FLIP_CULL) && new_state.isOn(shaders::OverrideState::FLIP_CULL);

    if (curState.isOn(new_state.bits) && curState.zBias == new_state.zBias && curState.sblend == new_state.sblend &&
        curState.dblend == new_state.dblend && !resetFlipCull)
      return curStateId;

    shaders::overrides::reset();
    combinedState = curState;
  }

  combinedState.set(new_state.bits);
  if (resetFlipCull)
    combinedState.reset(shaders::OverrideState::FLIP_CULL);
  if (new_state.isOn(shaders::OverrideState::Z_BIAS))
    combinedState.zBias = new_state.zBias;
  if (new_state.isOn(shaders::OverrideState::BLEND_SRC_DEST))
  {
    combinedState.sblend = new_state.sblend;
    combinedState.dblend = new_state.dblend;
  }
  auto &combinedStates = overrideStateMap[(uint32_t)curStateId];

  for (auto &combinedStateId : combinedStates)
    if (shaders::overrides::get(combinedStateId) == combinedState)
    {
      shaders::overrides::set(combinedStateId);
      return curStateId;
    }

  combinedStates.push_back(shaders::overrides::create(combinedState));
  shaders::OverrideStateId combinedStateId = combinedStates.back().get();
  shaders::overrides::set(combinedStateId);
  return curStateId;
}

void landmesh::reset_override(shaders::OverrideStateId &prev_state)
{
  if (shaders::overrides::get_current() == prev_state)
  {
    prev_state = shaders::OverrideStateId();
    return;
  }
  shaders::overrides::reset();
  if (prev_state)
    shaders::overrides::set(prev_state);
  prev_state = shaders::OverrideStateId();
}

shaders::OverrideStateId LandMeshRenderer::setOverride(const shaders::OverrideState &new_state)
{
  return landmesh::set_override(overrideStateMap, new_state);
}

void LandMeshRenderer::resetOverride(shaders::OverrideStateId &prev_state) { landmesh::reset_override(prev_state); }

shaders::OverrideStateId LandMeshRenderer::setStateFlipCull(bool flip_cull)
{
  G_UNREFERENCED(flip_cull);
  shaders::OverrideState state;
  state.set(shaders::OverrideState::FLIP_CULL);
  return setOverride(state);
}

shaders::OverrideStateId LandMeshRenderer::setStateDepthBias(StateDepthBias depth_bias)
{
  G_UNREFERENCED(depth_bias);
  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_BIAS);
  state.zBias = 0;
  return setOverride(state);
}

shaders::OverrideStateId LandVtexRenderer::setStateBlend()
{
  shaders::OverrideState state;
  state.set(shaders::OverrideState::BLEND_SRC_DEST);
  state.sblend = BLEND_ONE;
  state.dblend = BLEND_ONE;
  return landmesh::set_override(overrideStateMap, state);
}

void LandVtexRenderer::evictSplattingData()
{
  resetTextures();
  for (int i = 0; i < landClassesLoaded.size(); ++i)
    landClassesLoaded[i] = LCTexturesLoaded();
}

void LandMeshRenderer::evictSplattingData()
{
  vtex->evictSplattingData();

  if (cellStates) // They keep pointers to unreferenced textures and become invalid as the cells themselves.
    delete[] cellStates;
  cellStates = NULL;
}

void LandVtexRenderer::resetTextures()
{
  d3d::GpuAutoLock gpuLock;
  if (lmesh_sampler__land_detail_map >= 0)
    d3d::settex(lmesh_sampler__land_detail_map, NULL);

  for (int i = 0; i < DET_TEX_NUM; ++i)
  {
    d3d::settex(lmesh_sampler__land_detail_tex1 + i, NULL);
  }
}

void LandMeshRenderer::resetTextures() { vtex->resetTextures(); }

// Forwarders to the land-class / virtual-texture renderer held in `vtex`.
Tab<LandClassDetailTextures> &LandMeshRenderer::getLandClasses() { return vtex->getLandClasses(); }
TEXTUREID LandMeshRenderer::getDetailTileTex() { return vtex->getDetailTileTex(); }
d3d::SamplerHandle LandMeshRenderer::getDetailTileSmp() const { return vtex->getDetailTileSmp(); }
void LandMeshRenderer::setCustomLcTextures() { vtex->setCustomLcTextures(); }
void LandMeshRenderer::updateCustomSamplers(LandMeshManager &provider) { vtex->updateCustomSamplers(provider); }
bool LandMeshRenderer::reloadGrassMaskTex(int land_class_id, TEXTUREID newGrassMaskTexId)
{
  return vtex->reloadGrassMaskTex(land_class_id, newGrassMaskTexId);
}

void LandMeshRenderer::prepare(LandMeshManager &provider, const HmapOrigin &origin)
{
  if (provider.isDecodedToWorldPos() && !optScn)
  {
    optScn = new landmesh::OptimizedScene[LandMeshManager::LOD_COUNT];
    for (int lod = 0; lod < LandMeshManager::LOD_COUNT; ++lod)
      landmesh::buildOptSceneData(optScn[lod], provider, lod);
  }

  if (provider.isInTools() && !provider.forceHeightmapRendering)
  {
    delete[] cellStates;
    cellStates = NULL;
  }
  else if (provider.getHmapHandler())
  {
    // useExclBox (hide land cells where the heightmap fills) is the only persistent bit kept here; the
    // per-pass heightmap draw decision now lives in LandMeshCullingData::hmapDrawType (set by the cull/render()).
    provider.cullingState.useExclBox = provider.mayRenderHmap && provider.getHmapHandler()->prepare(origin.pos, origin.distanceMul);
  }

  if (!cellStates && !provider.getDetailMap().cells.empty())
  {
    vtex->prepareLandClasses(provider);
    cellStates = new CellState[provider.getNumCellsY() * provider.getNumCellsX()];
    G_STATIC_ASSERT(sizeof(cellStates[0].lcIds[0]) == 1);
    eastl::bitset<1 << (8 * sizeof(cellStates[0].lcIds[0]))> errorSignalled;
    for (int y = 0; y < provider.getNumCellsY(); ++y)
      for (int x = 0; x < provider.getNumCellsX(); ++x)
      {
        getCellState(provider, x, y, cellStates[x + y * provider.getNumCellsX()]);
        int validId = -1;
        for (int di = 0; di < cellStates[x + y * provider.getNumCellsX()].numDetailTextures; ++di)
        {
          const LCTexturesLoaded &landLoaded = vtex->getLandClassesLoaded()[cellStates[x + y * provider.getNumCellsX()].lcIds[di]];
          if (landLoaded.colorMap.tid)
          {
            validId = cellStates[x + y * provider.getNumCellsX()].lcIds[di];
            break;
          }
        }
        if (validId < 0)
          for (int di = 0; di < vtex->getLandClassesLoaded().size(); ++di)
          {
            const LCTexturesLoaded &landLoaded = vtex->getLandClassesLoaded()[di];
            if (landLoaded.colorMap.tid)
            {
              validId = di;
              break;
            }
          }
        for (int di = 0; di < cellStates[x + y * provider.getNumCellsX()].numDetailTextures; ++di)
        {
          auto &lc = cellStates[x + y * provider.getNumCellsX()].lcIds[di];
          const LCTexturesLoaded &landLoaded = vtex->getLandClassesLoaded()[lc];
          if (!landLoaded.colorMap.tid)
          {
            if (!errorSignalled.test(lc))
            {
              errorSignalled.set(lc, true);
              logerr("land class %d without colormap was used in cell! replaced with %d", lc, validId);
            }
            lc = validId;
          }
        }
      }
  }
  if (!mirroredCells.size())
  {
    if (provider.isInTools())
      numBorderCellsZNeg = numBorderCellsZPos = numBorderCellsXNeg = numBorderCellsXPos = 0;
    int tHeight = (provider.getNumCellsY() + numBorderCellsZNeg + numBorderCellsZPos);
    tWidth = (provider.getNumCellsX() + numBorderCellsXNeg + numBorderCellsXPos);
    IPoint2 lt, rb;
    lt.x = provider.getCellOrigin().x;
    lt.y = provider.getCellOrigin().y;
    rb.x = lt.x + provider.getNumCellsX() - 1;
    rb.y = lt.y + provider.getNumCellsY() - 1;
    float landCellSize = provider.getLandCellSize(), gridCellSize = provider.getGridCellSize();

    clear_and_resize(mirroredCells, tWidth * tHeight);
    for (int y = 0; y < tHeight; ++y)
      for (int x = 0; x < tWidth; ++x)
      {
        int borderX = x - numBorderCellsXNeg + provider.getCellOrigin().x,
            borderY = y - numBorderCellsZNeg + provider.getCellOrigin().y;
        int mirrorX = LandMeshCullingState::mirror_x(borderX, lt.x, rb.x);
        int mirrorY = LandMeshCullingState::mirror_x(borderY, lt.y, rb.y);
        int cellX = mirrorX - provider.getCellOrigin().x;
        int cellY = mirrorY - provider.getCellOrigin().y;
        CellState &cellState = cellStates[cellX + cellY * provider.getNumCellsX()];
        const IBBox2 &exclBox = provider.cullingState.exclBox;

        bool hide_land = borderX >= exclBox[0].x && borderY >= exclBox[0].y && borderX < exclBox[1].x && borderY < exclBox[1].y;
        mirroredCells[y * tWidth + x].init(borderX, borderY, lt.x, lt.y, rb.x, rb.y, landCellSize, gridCellSize, cellState.posToWorld,
          cellState.detMapTc, hide_land);
      }

    float minX = 2.0f * landCellSize * provider.getCellOrigin().x;
    float maxX = 2.0f * (landCellSize * (provider.getNumCellsX() + provider.getCellOrigin().x) - gridCellSize);

    float minZ = 2.0f * landCellSize * provider.getCellOrigin().y;
    float maxZ = 2.0f * (landCellSize * (provider.getNumCellsY() + provider.getCellOrigin().y) - gridCellSize);
    for (int i = 0; i < 9; ++i)
    {
      int x = i % 3, z = i / 3;
      worldMulPos[i][0] = Point4(x ? -1 : 1, 1, z ? -1 : 1, 0);
      worldMulPos[i][1] = Point4(x == CellState::MIRRORED_POS   ? maxX
                                 : x == CellState::MIRRORED_NEG ? minX
                                                                : 0,
        0,
        z == CellState::MIRRORED_POS   ? maxZ
        : z == CellState::MIRRORED_NEG ? minZ
                                       : 0,
        1);
    }
  }

  IPoint2 cc;
  float cellSize = provider.getLandCellSize();
  Point3 meshOffset = provider.getOffset();
  Point2 centerCellPos = Point2::xz(origin.pos - meshOffset);
  cc.x = int(floorf(centerCellPos.x / cellSize));
  cc.y = int(floorf(centerCellPos.y / cellSize));
  centerCellFract = centerCellPos - Point2(cc.x * cellSize, cc.y * cellSize);
  if (cc == centerCell)
    return;

  centerCell = cc;
  prepared = true;
}


void LandMeshRenderer::getCellState(LandMeshManager &provider, int cell_x, int cell_y, CellState &curState)
{
  int mirrorX = cell_x + provider.getCellOrigin().x;
  int mirrorY = cell_y + provider.getCellOrigin().y;
  // Setup.
  provider.getLandDetailTexIds(mirrorX, mirrorY, curState.lcIds.data());
  Point3 meshOffset = provider.getOffset();
  float cellSize = provider.getLandCellSize();
  // debug("  render land mesh %d %d", x, y);
  BBox3 box = provider.getBBox();
  float yofs = 0.5f * (box[1].y + box[0].y);

  curState.posToWorld[0] = Point4(cellSize * 0.5f, 0.5f * (box[1].y - box[0].y), cellSize * 0.5f, 0);
  curState.posToWorld[1] = Point4(cellSize * (mirrorX + 0.5f) + meshOffset.x, yofs, cellSize * (mirrorY + 0.5f) + meshOffset.z, 1);
  // expect 7 valid textures. ignore 8th value (file has 0xff index)
  G_ASSERT(DET_TEX_NUM == 7);

  Color4 mul[2];
  Color4 ofs[4];

  if (!vtex->getLandClasses().size())
  {
    mul[0] = Color4(1, 1, 1, 1);
    mul[1] = Color4(1, 1, 1, 1);
  }
  else
  {
    mul[0] = Color4(curState.lcIds[0] == 0xFF ? 1.f : vtex->getLandClasses()[curState.lcIds[0]].tile,
      curState.lcIds[1] == 0xFF ? 1.f : vtex->getLandClasses()[curState.lcIds[1]].tile,
      curState.lcIds[2] == 0xFF ? 1.f : vtex->getLandClasses()[curState.lcIds[2]].tile,
      curState.lcIds[3] == 0xFF ? 1.f : vtex->getLandClasses()[curState.lcIds[3]].tile);

    mul[1] = Color4(curState.lcIds[4] == 0xFF ? 1.f : vtex->getLandClasses()[curState.lcIds[4]].tile,
      curState.lcIds[5] == 0xFF ? 1.f : vtex->getLandClasses()[curState.lcIds[5]].tile,
      curState.lcIds[6] == 0xFF ? 1.f : vtex->getLandClasses()[curState.lcIds[6]].tile, 1);

    for (int i = 0; i < 3; ++i)
      ofs[i] = Color4(curState.lcIds[i * 2] == 0xFF ? 0.f : vtex->getLandClasses()[curState.lcIds[i * 2]].offset.x,
        curState.lcIds[i * 2] == 0xFF ? 0.f : vtex->getLandClasses()[curState.lcIds[i * 2]].offset.y,
        curState.lcIds[i * 2 + 1] == 0xFF ? 0.f : vtex->getLandClasses()[curState.lcIds[i * 2 + 1]].offset.x,
        curState.lcIds[i * 2 + 1] == 0xFF ? 0.f : vtex->getLandClasses()[curState.lcIds[i * 2 + 1]].offset.y);

    ofs[3] = Color4(curState.lcIds[6] == 0xFF ? 0.f : vtex->getLandClasses()[curState.lcIds[6]].offset.x,
      curState.lcIds[6] == 0xFF ? 0.f : vtex->getLandClasses()[curState.lcIds[6]].offset.y, 0, 0);

    for (int i = 0; i < 4; ++i)
    {
      ofs[i].r -= meshOffset.x;
      ofs[i].g -= meshOffset.z;
      ofs[i].b -= meshOffset.x;
      ofs[i].a -= meshOffset.z;
    }
  }


  // with a weight atlas detMapCoord is the intra-cell fraction; the page
  // lookup in GET_WEIGHTS does the rest (see land_inc.dshl)
  float tcScale = provider.getWeightAtlas() ? 1.f / cellSize : vtex->getDetMapTcScale();
  curState.detMapTc = Color4(tcScale, tcScale, vtex->getDetMapTcOfs() - mirrorX * cellSize - meshOffset.x,
    vtex->getDetMapTcOfs() - mirrorY * cellSize - meshOffset.z);
  curState.detMapTc[2] *= curState.detMapTc[0];
  curState.detMapTc[3] *= curState.detMapTc[1];
  for (int i = 0; i < 4; ++i)
  {
    float mul1 = mul[i >> 1][(i & 1) * 2 + 0], mul2 = mul[i >> 1][(i & 1) * 2 + 1];
    Color4 mul3(mul1, -mul1, mul2, -mul2);

    curState.mul_offset[0][0][i] = mul3;
    curState.mul_offset[0][0][i + 4] = Color4(ofs[i][0] * mul3[0], ofs[i][1] * mul3[1], ofs[i][2] * mul3[2], ofs[i][3] * mul3[3]);
  }
  float minX = provider.getCellOrigin().x * cellSize, minZ = provider.getCellOrigin().y * cellSize;

  curState.prepareMirrorMul(minX, cellSize * provider.getNumCellsX() + minX - provider.getGridCellSize(), minZ,
    cellSize * provider.getNumCellsY() + minZ - provider.getGridCellSize());
  curState.map1 = provider.getWeightAtlas() ? provider.getWeightAtlas()->tex : NULL;

  curState.numDetailTextures = LandMeshRenderer::DET_TEX_NUM;
  curState.invTexSizes[0] = curState.invTexSizes[1] = Color4(1, 1, 1, 1) / 1024.0;
  curState.trivial = true;
  for (int i = 0; i < LandMeshRenderer::DET_TEX_NUM; ++i)
  {
    int lcId = curState.lcIds[i];
    if (lcId == 0xFF && curState.numDetailTextures > i)
      curState.numDetailTextures = i;
    if (!provider.isInTools())
      G_ASSERTF((i >= curState.numDetailTextures) == (lcId == 0xFF),
        "Detail textures for land mesh should be countinuous. See log for missing textures");
    if (curState.lcIds[i] < vtex->getLandClassesLoaded().size())
    {
      curState.invTexSizes[i >> 2][i & 3] = vtex->getLandClassesLoaded()[lcId].invColormapSize.x;
      if (!vtex->getLandClassesLoaded()[lcId].colorMap.tid)
      {
        if (provider.isInTools())
          curState.numDetailTextures = i;
      }
      else if (vtex->getLandClasses()[lcId].lcType != LC_SIMPLE)
        curState.trivial = false;
    }
  }
  if (!curState.map1)
    curState.numDetailTextures = 1;
  if (curState.numDetailTextures == 0)
  {
    static int last_t = 0, happens = 0;
    if (get_time_msec() > last_t + 1000)
    {
      logerr("land cell (%d,%d) has no land class (and %d similar errors)", cell_x, cell_y, happens);
      happens = 0;
      last_t = get_time_msec();
    }
    else
      happens++;
  }
  mem_set_0(curState.lmeshElems);
  for (int lod = 0; lod < LandMeshManager::LOD_COUNT; ++lod)
  {
    ShaderMesh *landLod = provider.getCellLandShaderMesh(mirrorX, mirrorY, lod);
    G_ASSERT(landLod->getAllElems().size() <= curState.landBottom[lod].size()); // if this happen, increase carray size
    curState.lmeshElems[lod] = const_cast<ShaderMesh::RElem *>(landLod->getAllElems().data());

    for (int ei = 0; ei < landLod->getAllElems().size(); ++ei)
    {
      int bottom = 0;
      landLod->getAllElems()[ei].mat->getIntVariable(var::bottom.get_var_id(), bottom);
      curState.landBottom[lod][ei] = bottom < 2 ? BOTTOM_ABOVE : BOTTOM_BELOW;
    }
  }
}

__forceinline ShaderMesh *LandMeshRenderer::prepareGeomCellsLM(LandMeshManager &provider, int cellNo, int lodNo)
{
  G_UNREFERENCED(provider);
  G_UNREFERENCED(cellNo);
  G_UNREFERENCED(lodNo);
  return NULL;
}

__forceinline ShaderMesh *LandMeshRenderer::prepareGeomCellsCM(LandMeshManager &provider, int cellNo, bool **isCombinedBig)
{
  const MirroredCellState &mirroredCell = mirroredCells[cellNo];
  return provider.getCellCombinedShaderMeshRaw(mirroredCell.cellX, mirroredCell.cellY, isCombinedBig);
}

void LandMeshRenderer::renderGeomCellsLM(LandMeshManager &provider, const RenderPassCtx &pass_ctx, dag::ConstSpan<uint16_t> cells,
  int lodNo, RenderType rtype, uint8_t hide_excluded, bool force_set_states)
{
  if (!cells.size() || !optScn)
    return;
  // DEBUG_CTX("%d: render %d", rtype, cells_count);
  landmesh::VisibilityData &visData = optScn[lodNo].visData;
  for (auto id : cells)
  {
    const MirroredCellState &mirroredCell = mirroredCells[id];
    if (hide_excluded & mirroredCell.excluded)
      continue;
    ShaderMesh *landm = provider.getCellLandShaderMeshRaw(mirroredCell.cellX, mirroredCell.cellY, lodNo);
    for (int ei = 0; ei < landm->getAllElems().size(); ++ei)
    {
      const ShaderMesh::RElem &re = landm->getAllElems()[ei];
      visData.mark(re.vdOrderIndex);
      // debug("[%d]%p.%d: %d,%d,%d,%d,  %d ->%d",
      //   cell_no[ci], landm, ei, re.sv, re.numv, re.si, re.numf, re.baseVertex, re.vdOrderIndex);
    }
  }

  // Single CB unifying the four prior shapes. Policy flags:
  //  - setStatesFlag      : call mat->setStates(0, true) (skipped for ONE_SHADER no-states path)
  //  - applyDepthBiasFlag : adjust depth bias per BOTTOM_ABOVE/BELOW (off for ONE_SHADER, on for FULL and DEPTH)
  //  - skipBottom         : in DEPTH path, BELOW returns false and ABOVE skips bias adjust (pass_ctx decision)
  struct RosdCB : landmesh::IRosdSetStatesCB
  {
    LandMeshRenderer *renderer;
    mutable shaders::OverrideStateId prevStateId;
    bool setStatesFlag, applyDepthBiasFlag, skipBottom;

    RosdCB(LandMeshRenderer *r, bool set_states, bool apply_depth_bias, bool skip_bottom) :
      renderer(r),
      prevStateId(shaders::overrides::get_current()),
      setStatesFlag(set_states),
      applyDepthBiasFlag(apply_depth_bias),
      skipBottom(skip_bottom)
    {}

    virtual bool applyMat(ShaderElement *mat, int bottom_type) const override
    {
      if (setStatesFlag && !mat->setStates(0, true))
        return false;
      if (applyDepthBiasFlag && VariableMap::isGlobVariablePresent(var::bottom))
      {
        if (skipBottom)
          return bottom_type != BOTTOM_BELOW; // BELOW: cull; ABOVE: keep but skip bias adjust
        renderer->resetOverride(prevStateId);
        renderer->setStateDepthBias(bottom_type == BOTTOM_BELOW ? STATE_DEPTH_BIAS_BOTTOM : STATE_DEPTH_BIAS_ZERO);
      }
      return true;
    }
  };

  shaders::OverrideStateId prevStateId = shaders::overrides::get_current();
  if (rtype == RENDER_ONE_SHADER)
    landmesh::renderOptSceneData(optScn[lodNo], RosdCB(this, /*setStates*/ force_set_states, /*depthBias*/ false, false));
  else if (rtype == RENDER_DEPTH)
    landmesh::renderOptSceneData(optScn[lodNo],
      RosdCB(this, /*setStates*/ force_set_states, /*depthBias*/ true, pass_ctx.skipBottomRendering));
  else
    landmesh::renderOptSceneData(optScn[lodNo], RosdCB(this, /*setStates*/ true, /*depthBias*/ true, /*skipBottom*/ false));
  resetOverride(prevStateId);
}

void LandMeshRenderer::renderGeomCellsCM(LandMeshManager &provider, dag::ConstSpan<uint16_t> cells, RenderType rtype,
  bool skip_not_big, bool force_set_states)
{
  if (!cells.size() || !provider.getCombinedVdata())
    return;
  provider.getCombinedVdata()->setToDriver();
  // todo: glueing and separating by materials
  switch (rtype)
  {
    case RENDER_ONE_SHADER:
    case RENDER_DEPTH:
      for (auto id : cells)
      {
        bool *isCombinedBig;
        ShaderMesh *combinedm = prepareGeomCellsCM(provider, id, skip_not_big ? &isCombinedBig : NULL);
        if (!combinedm)
          continue;
        for (int ei = 0; ei < combinedm->getAllElems().size(); ++ei)
        {
          if (skip_not_big && !isCombinedBig[ei])
            continue;
          const ShaderMesh::RElem &re = combinedm->getAllElems()[ei];
          if (!force_set_states || re.e->setStates(0, true))
            d3d_err(re.drawIndTriList());
        }
      }
      break;

    case RENDER_WITH_CLIPMAP:
    case RENDER_REFLECTION:
      for (auto id : cells)
      {
        bool *isCombinedBig;
        ShaderMesh *combinedm = prepareGeomCellsCM(provider, id, &isCombinedBig);
        if (!combinedm)
          continue;
        for (int ei = 0; ei < combinedm->getAllElems().size(); ++ei)
        {
          if (skip_not_big && !isCombinedBig[ei])
            continue;
          const ShaderMesh::RElem &re = combinedm->getAllElems()[ei];
          if (re.e->setStates(0, true))
          {
            d3d_err(re.drawIndTriList());
          }
        }
      }
      break;
    default: // RENDER_CLIPMAP, RENDER_GRASS_MASK
      break;
  }
}

uint32_t landmesh::lmesh_render_flags = 0xFFFFFFFF;
uint32_t &LandMeshRenderer::lmesh_render_flags = landmesh::lmesh_render_flags;

inline bool shouldRenderMeshElem(const LandMeshManager &provider, int cellId, int elemId)
{
  return provider.getDecalElems().data()[cellId].shouldRenderElem.empty() ||
         provider.getDecalElems().data()[cellId].shouldRenderElem[elemId];
}

bool LandMeshRenderer::renderCellDecals(LandMeshManager &provider, const RenderPassCtx &pass_ctx,
  const MirroredCellState &mirroredCell, bool force_samplers_no_mipbias)
{
  ShaderMesh *decalm = provider.getCellDecalShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY);
  if (!decalm || !(pass_ctx.desc.renderFlags & RENDER_DECALS))
    return false;
  int id = mirroredCell.cellX + mirroredCell.cellY * provider.getNumCellsX();

  bool renderedAnything = false;
  if ((!provider.isInTools() || provider.forceHeightmapRendering) && provider.getDecalElems().size() &&
      provider.getDecalElems()[id].elemBoxes.size() == decalm->getAllElems().size())
  {
    bool setSamplersNoMipbias =
      (pass_ctx.desc.mode == LMeshRenderingMode::RENDERING_CLIPMAP || force_samplers_no_mipbias) &&
      eastl::find_if(decals_overrideSampler_const_no_array.begin(), decals_overrideSampler_const_no_array.end(),
        [](int smp_reg) { return smp_reg >= 0; }) != decals_overrideSampler_const_no_array.end();

    DECL_ALIGN16(IBBox2, subCellBox);
    Point3 meshOffset = provider.getOffset();
    float cellSize = provider.getLandCellSize();
    IPoint2 cellOfs(provider.getCellOrigin().x * 65535, provider.getCellOrigin().y * 65535);
    const BBox3 &renderBBox = pass_ctx.desc.renderInBBox;
    if (!renderBBox.isempty())
    {
      cellSize /= 65535.f;
      subCellBox[0].x = (int)floorf((renderBBox[0].x - meshOffset.x) / cellSize);
      subCellBox[0].y = (int)floorf((renderBBox[0].z - meshOffset.z) / cellSize);
      subCellBox[1].x = (int)floorf((renderBBox[1].x - meshOffset.x) / cellSize);
      subCellBox[1].y = (int)floorf((renderBBox[1].z - meshOffset.z) / cellSize);
      subCellBox[0] -= cellOfs;
      subCellBox[1] -= cellOfs;
    }
    else
    {
      vec4f invGridCellSzV = v_splats(65535.0f / cellSize);
      vec4f ofs = v_make_vec4f(meshOffset.x, meshOffset.z, meshOffset.x, meshOffset.z);
      vec4f worldBboxXZ = v_perm_xzac(pass_ctx.frustumWorldBBox.bmin, pass_ctx.frustumWorldBBox.bmax);
      vec4f regionV = v_sub(worldBboxXZ, ofs);
      regionV = v_mul(regionV, invGridCellSzV);
      vec4i regionI = v_cvt_floori(regionV);
      regionI = v_subi(regionI, v_cast_vec4i(v_perm_xyxy(v_ldu_half(&cellOfs.x))));
      v_sti(&subCellBox[0].x, regionI);
    }

#if _TARGET_SIMD_SSE
    vec4i subCellBoxV = v_cast_vec4i(v_ld((float *)&subCellBox[0].x));
    subCellBoxV = v_cast_vec4i(v_perm_zwxy(v_cast_vec4f(subCellBoxV)));
#endif
    GlobalVertexData *vertexData = NULL;
    ShaderElement *e = NULL;
    bool curShaderValid = false;
    int stored_sv = 0, stored_numv = 0, stored_si = 0, stored_numf = 0, stored_baseVertex = 0;
    carray<d3d::SamplerHandle, LandMeshManager::DECALS_OVERRIDE_SAMPLERS_COUNT> currentSamplers;
    eastl::fill(currentSamplers.begin(), currentSamplers.end(), d3d::SamplerHandle::Invalid);

    for (int i = 0; i < decalm->getAllElems().size(); ++i)
    {
      if (!shouldRenderMeshElem(provider, id, i))
        continue;

#if _TARGET_SIMD_SSE
      vec4i elemBoxV = *(vec4i *)(&provider.getDecalElems().data()[id].elemBoxes.data()[i][0].x);
      int mask = _mm_movemask_ps(v_cast_vec4f(_mm_cmpgt_epi32(elemBoxV, subCellBoxV))); // v_cmp_gti(elemBoxV,
                                                                                        // v_perm_zwxy(subCellBoxV))
      if (((mask | (~mask >> 2)) & (1 | 2)))
        continue;
#else
      IBBox2 ib = provider.getDecalElems().data()[id].elemBoxes[i];
      if (!unsafe_overlap(ib, subCellBox))
        continue;
#endif
      const ShaderMesh::RElem &re = decalm->getAllElems().data()[i];
      const dag::Span<const d3d::SamplerHandle> reSamplers =
        provider.getCellDecalElemSamplersNoMipbiasMeshOffseted(mirroredCell.cellX, mirroredCell.cellY, i);
      G_ASSERT(!setSamplersNoMipbias || reSamplers.size() == currentSamplers.size());

      if (!re.e)
        continue;
      if (re.vertexData != vertexData)
      {
        if (stored_numf && curShaderValid)
        {
          d3d_err(d3d::drawind(PRIM_TRILIST, stored_si, stored_numf, stored_baseVertex));
          renderedAnything = true;
        }
        vertexData = re.vertexData;
        vertexData->setToDriver();
        stored_numf = 0;
      }
      if (setSamplersNoMipbias &&
          !eastl::equal(currentSamplers.begin(), currentSamplers.end(), reSamplers.begin(),
            [](const auto &curSmp, const auto &reSmp) { return reSmp == d3d::SamplerHandle::Invalid || reSmp == curSmp; }))
      {
        if (stored_numf && curShaderValid)
        {
          d3d_err(d3d::drawind(PRIM_TRILIST, stored_si, stored_numf, stored_baseVertex));
          renderedAnything = true;
        }
        for (int j = 0; j < currentSamplers.size(); ++j)
        {
          if (reSamplers[j] != d3d::SamplerHandle::Invalid)
          {
            currentSamplers[j] = reSamplers[j];
            d3d::set_sampler(STAGE_PS, decals_overrideSampler_const_no_array[j], currentSamplers[j]);
          }
        }
        stored_numf = 0;
      }
      if (e != re.e)
      {
        if (stored_numf && curShaderValid)
        {
          d3d_err(d3d::drawind(PRIM_TRILIST, stored_si, stored_numf, stored_baseVertex));
          renderedAnything = true;
        }
        e = re.e;
        curShaderValid = e->setStates(0, true);
        stored_sv = re.sv, stored_numv = re.numv, stored_si = re.si, stored_numf = re.numf, stored_baseVertex = re.baseVertex;
      }
      else
      {
        if (stored_numf && (stored_baseVertex != re.baseVertex || stored_numf * 3 + stored_si != re.si))
        {
          if (curShaderValid)
          {
            d3d_err(d3d::drawind(PRIM_TRILIST, stored_si, stored_numf, stored_baseVertex));
            renderedAnything = true;
          }
          stored_sv = re.sv, stored_numv = re.numv, stored_si = re.si, stored_numf = re.numf, stored_baseVertex = re.baseVertex;
        }
        else
        {
          if (!stored_numf)
            stored_sv = re.sv, stored_numv = re.numv, stored_si = re.si, stored_numf = re.numf, stored_baseVertex = re.baseVertex;
          else
          {
            int ev = re.sv + re.numv;
            int stored_ev = stored_sv + stored_numv;
            stored_ev = max(stored_ev, ev);
            stored_sv = min(stored_sv, re.sv);
            stored_numv = stored_ev - stored_sv;
            stored_numf += re.numf;
          }
        }
      }
    }
    if (stored_numf && curShaderValid)
    {
      d3d_err(d3d::drawind(PRIM_TRILIST, stored_si, stored_numf, stored_baseVertex));
      renderedAnything = true;
    }
  }
  else
  {
    decalm->render();
    renderedAnything = true;
    if (!provider.isInTools() && pass_ctx.desc.mode == LMeshRenderingMode::RENDERING_CLIPMAP)
      LOGERR_ONCE("renderCellDecals: Fallback branch was used, samplerState might be incorrect during RENDERING_CLIPMAP.");
  }

  return renderedAnything;
}


void LandMeshRenderer::renderCell(LandMeshManager &provider, RenderPassCtx &pass_ctx, int cellNo, int lodNo, RenderType rtype,
  RenderPurpose rpurpose, bool skip_combined_not_marked_as_big = false)
{
  // fixme:
  //  Get cell geometry.

  // Set shader vars for mirroring.
  // int cellX = mirrorX + provider.getCellOrigin().x, cellY = mirrorY + provider.getCellOrigin().y;
  const MirroredCellState &mirroredCell = mirroredCells[cellNo];

  bool hide_land = pass_ctx.useExclBox && mirroredCell.excluded;

  bool isDecoded = provider.isDecodedToWorldPos();

  if ((rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES || rtype == RENDER_GRASS_MASK) || !isDecoded)
    mirroredCell.setPosConsts(rpurpose == RENDER_FOR_GRASS);
  else
    d3d::set_vs_const(lmesh_vs_const__pos_to_world, &worldMulPos[mirroredCell.mirrorScaleState.xz][0].x, 2);
  CellState &curState = cellStates[mirroredCell.cellX + mirroredCell.cellY * provider.getNumCellsX()];

  mirroredCell.setFlipCull(this, pass_ctx);
  if (rtype == RENDER_ONE_SHADER)
  {
    if (!hide_land)
    {
      ShaderMesh *landm = provider.getCellLandShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY, lodNo);
      if (landm)
        landm->renderRawImmediate(false);
    }
    ShaderMesh *combinedm = provider.getCellCombinedShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY);
    if (combinedm)
      combinedm->renderRawImmediate(false);
    return;
  }
  if (rtype == RENDER_DEPTH)
  {
    // todo: split in two passes
    shaders::OverrideStateId prevStateId;
    if (VariableMap::isGlobVariablePresent(var::bottom))
      prevStateId = setStateDepthBias(STATE_DEPTH_BIAS_ZERO);
    bool *isCombinedBig;
    ShaderMesh *combinedm = provider.getCellCombinedShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY, &isCombinedBig);
    GlobalVertexData *vertexData = NULL;
    if (combinedm)
    {
      for (int ei = 0; ei < combinedm->getAllElems().size(); ++ei)
      {
        const ShaderMesh::RElem &re = combinedm->getAllElems()[ei];
        G_ASSERT(isCombinedBig);
        if (skip_combined_not_marked_as_big && !isCombinedBig[ei])
          continue;

        if (re.vertexData != vertexData)
        {
          vertexData = re.vertexData;
          vertexData->setToDriver();
        }
        d3d_err(re.drawIndTriList());
      }
    }

    if (VariableMap::isGlobVariablePresent(var::bottom))
      resetOverride(prevStateId);

    if (hide_land)
      return;
    ShaderMesh *landm = provider.getCellLandShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY, lodNo);
    if (!landm)
      return;

    prevStateId = shaders::overrides::get_current();
    for (int ei = 0; ei < landm->getAllElems().size(); ++ei)
    {
      const ShaderMesh::RElem &re = landm->getAllElems()[ei];

      if (re.vertexData != vertexData)
      {
        vertexData = re.vertexData;
        vertexData->setToDriver();
      }
      if (VariableMap::isGlobVariablePresent(var::bottom))
      {
        resetOverride(prevStateId);
        prevStateId =
          setStateDepthBias(curState.landBottom[lodNo][ei] == BOTTOM_BELOW ? STATE_DEPTH_BIAS_BOTTOM : STATE_DEPTH_BIAS_ZERO);
      }
      d3d_err(re.drawIndTriList());
    }
    resetOverride(prevStateId);
    return;
  }

  if (rtype <= MAX_RENDER_SPLATTING__)
  {
    mirroredCell.setDetMapTc();
    curState.set(rtype == RENDER_GRASS_MASK, vtex->getLandClassesLoaded(), false,
      rtype == RENDER_GRASS_MASK ? vtex->getPhysmatIdsBuf() : NULL);
  }


  // Render.

  if (rtype == RENDER_WITH_CLIPMAP || rtype == RENDER_REFLECTION)
  {
    if (pass_ctx.desc.debugCells && VariableMap::isGlobVariablePresent(var::landmesh_debug_cells_scale))
    {
      float s = ((mirroredCell.cellX + mirroredCell.cellY) & 1) ? 0.2f : 1.0f;
      ShaderGlobal::set_float4(var::landmesh_debug_cells_scale, Color4(s, s, s, s));
    }
    mirroredCell.setPsMirror(pass_ctx);
    // reverse order - landmesh combined first
    bool *isCombinedBig;
    ShaderMesh *combinedm = provider.getCellCombinedShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY, &isCombinedBig);
    if (combinedm)
    {
      GlobalVertexData *vertexData = NULL;
      for (int ei = 0; ei < combinedm->getAllElems().size(); ++ei)
      {
        const ShaderMesh::RElem &re = combinedm->getAllElems()[ei];
        G_ASSERT(isCombinedBig);
        if (skip_combined_not_marked_as_big && !isCombinedBig[ei])
          continue;

        if (re.e->setStates(0, true))
        {
          if (re.vertexData != vertexData)
          {
            vertexData = re.vertexData;
            vertexData->setToDriver();
          }
          d3d_err(re.drawIndTriList());
        }
      }
    }
    if (!hide_land)
    {
      ShaderMesh *landm = provider.getCellLandShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY, lodNo);
      if (landm)
      {
        GlobalVertexData *vertexData = NULL;
        for (int ei = 0; ei < landm->getAllElems().size(); ++ei)
        {
          const ShaderMesh::RElem &re = landm->getAllElems()[ei];
          if (re.e->setStates(0, true))
          {
            if (re.vertexData != vertexData)
            {
              vertexData = re.vertexData;
              vertexData->setToDriver();
            }
            d3d_err(re.drawIndTriList());
          }
        }
      }
    }
  }
  else if (rtype == RENDER_WITH_SPLATTING)
  {
    renderCellDecals(provider, pass_ctx, mirroredCell);
  }
  else if (rtype == RENDER_CLIPMAP)
  {
    if (curState.trivial)
    {
      if (landclassShader[LC_TRIVIAL].elem->setStates(0, true)) // trivial case: all landclasses are simple
      {
        d3d::setvsrc_ex(0, one_quad, 0, sizeof(short) * 4);
        d3d::draw(PRIM_TRISTRIP, 0, 2);
      }
    }
    else
      vtex->renderLandclasses(curState);

    renderCellDecals(provider, pass_ctx, mirroredCell);

    ShaderMesh *combinedm = provider.getCellCombinedShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY);
    if (combinedm && (pass_ctx.desc.renderFlags & RENDER_COMBINED))
    {
      if (isDecoded)
        d3d::set_vs_const(lmesh_vs_const__pos_to_world, &worldMulPos[mirroredCell.mirrorScaleState.xz][0].x, 2);
      combinedm->render();
    }
  }
  else if (rtype == RENDER_PATCHES)
  {
    ShaderMesh *patches = provider.getCellPatchesShaderMesh(mirroredCell.cellX, mirroredCell.cellY);
    if (patches)
      patches->render();
  }
  else if (rtype == RENDER_GRASS_MASK) //<= MAX_RENDER_SPLATTING__ || rtype == RENDER_COMBINED_LAST
  {
    if (landclassShader[LC_TRIVIAL].elem->setStates(0, true)) // trivial case: all landclasses are simple
    {
      d3d::setvsrc_ex(0, one_quad, 0, sizeof(short) * 4);
      d3d::draw(PRIM_TRISTRIP, 0, 2);
    }

    // render indexed landclasses, if any
    if (landclassShader.size() > LC_COUNT)
    {
      vtex->renderLandclasses(curState, true, LC_CUSTOM);
    }

    ShaderMesh *decalm = provider.getCellDecalShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY);
    if (decalm)
    {
      int id = mirroredCell.cellX + mirroredCell.cellY * provider.getNumCellsX();
      const LandMeshManager::ElemsData *decalElems = provider.getDecalElems().data();

      if (id < provider.getDecalElems().size() && !decalElems[id].shouldRenderElem.empty())
      {
        const SmallTab<bool, MidmemAlloc> &shouldRenderElems = decalElems[id].shouldRenderElem;
        decalm->renderWithFilter([&](int i) { return shouldRenderElems[i]; });
      }
      else
        decalm->render();
    }
    ShaderMesh *combinedm = provider.getCellCombinedShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY);
    if (combinedm)
    {
      if (isDecoded)
        d3d::set_vs_const(lmesh_vs_const__pos_to_world, &worldMulPos[mirroredCell.mirrorScaleState.xz][0].x, 2);
      combinedm->render();
    }
  }
  else
    G_ASSERT(0);
}

#define EQ(A, B) (fabsf((A) - (B)) <= 0.1 * min(fabs(A), fabs(B)) || (fabsf(A) < 0.001 && fabsf(B) < 0.001))
static bool matrices_are_equal(const TMatrix4 &l, const TMatrix4 &r)
{
  return EQ(l._11, r._11) && EQ(l._12, r._12) && EQ(l._13, r._13) && EQ(l._14, r._14) && EQ(l._21, r._21) && EQ(l._22, r._22) &&
         EQ(l._23, r._23) && EQ(l._24, r._24) && EQ(l._31, r._31) && EQ(l._32, r._32) && EQ(l._33, r._33) && EQ(l._34, r._34) &&
         EQ(l._41, r._41) && EQ(l._42, r._42) && EQ(l._43, r._43) && EQ(l._44, r._44);
}
#undef EQ

#if _TARGET_PC
static bool after_lost_device = false;
void LandMeshRenderer::afterLostDevice() { after_lost_device = true; }
#endif
void LandMeshRenderer::resetOptSceneAndStates()
{
  if (optScn)
    delete[] optScn;
  optScn = nullptr;
  if (cellStates)
    delete[] cellStates;
  cellStates = NULL;
}

enum
{
  LAST_MIRROR = 8,
  MIRRORS = 9
};
static constexpr int LANDMESH_MAX_CELLS_LEAVES = 1024;
struct RenderCulledCtx
{
  carray<carray<uint16_t, LANDMESH_MAX_CELLS_LEAVES>, MIRRORS * LandMeshManager::LOD_COUNT - 1> tmpCellsLodData;
  carray<uint16_t, LANDMESH_MAX_CELLS_LEAVES * MIRRORS * LandMeshManager::LOD_COUNT> cellsFlattenedData;

  carray<uint16_t[MIRRORS][LandMeshManager::LOD_COUNT], MAX_LAND_MESH_REGIONS> endRegion;
  uint16_t cellCounter[MIRRORS][LandMeshManager::LOD_COUNT];
};

#if DAGOR_DBGLEVEL > 0
bool LandMeshRenderer::check_cull_matrix(const TMatrix &realView, const TMatrix4 &realProj, const Driver3dPerspective &persp,
  const TMatrix4 &realGlobtm, const char *marker, const LandMeshCullingData &data, bool do_fatal)
{
  TMatrix realWtm;
  d3d::gettm(TM_WORLD, realWtm);

  if (persp.ox == 0.f && persp.oy == 0.f && !matrices_are_equal(realGlobtm, data.culltm))
  {
    debug("check_cull_matrix, %s", marker);
#if _TARGET_PC
    if (after_lost_device)
    {
      debug("check_cull_matrix, happened after_lost_device");
      // skip one frame after DEVICE LOST, if we cannot render it correctly
      after_lost_device = false;
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
      return true;
    }
#endif

    TMatrix4 globtm = data.culltm;

    debug("culling matrix = [%g %g %g %g] [%g %g %g %g] [%g %g %g %g] [%g %g %g %g]\n", globtm._11, globtm._12, globtm._13, globtm._14,
      globtm._21, globtm._22, globtm._23, globtm._24, globtm._31, globtm._32, globtm._33, globtm._34, globtm._41, globtm._42,
      globtm._43, globtm._44);
    debug("rendering matrix = [%g %g %g %g] [%g %g %g %g] [%g %g %g %g] [%g %g %g %g]\n", realGlobtm._11, realGlobtm._12,
      realGlobtm._13, realGlobtm._14, realGlobtm._21, realGlobtm._22, realGlobtm._23, realGlobtm._24, realGlobtm._31, realGlobtm._32,
      realGlobtm._33, realGlobtm._34, realGlobtm._41, realGlobtm._42, realGlobtm._43, realGlobtm._44);

    debug("rendering wtm = [%g %g %g] [%g %g %g] [%g %g %g] [%g %g %g]\n", realWtm[0][0], realWtm[0][1], realWtm[0][2], realWtm[1][0],
      realWtm[1][1], realWtm[1][2], realWtm[2][0], realWtm[2][1], realWtm[2][2], realWtm[3][0], realWtm[3][1], realWtm[3][2]);

    debug("rendering view = [%g %g %g] [%g %g %g] [%g %g %g] [%g %g %g]\n", realView[0][0], realView[0][1], realView[0][2],
      realView[1][0], realView[1][1], realView[1][2], realView[2][0], realView[2][1], realView[2][2], realView[3][0], realView[3][1],
      realView[3][2]);

    debug("rendering proj = [%g %g %g %g] [%g %g %g %g] [%g %g %g %g] [%g %g %g %g]\n", realProj._11, realProj._12, realProj._13,
      realProj._14, realProj._21, realProj._22, realProj._23, realProj._24, realProj._31, realProj._32, realProj._33, realProj._34,
      realProj._41, realProj._42, realProj._43, realProj._44);

    debug("rendering persp = wk=%g hk=%g zn=%g zf=%g ox=%g oy=%g", persp.wk, persp.hk, persp.zn, persp.zf, persp.ox, persp.oy);

    if (do_fatal)
    {
      G_ASSERTF(0, "LandMeshRenderer::renderCulled: "
                   "trying to use for render other matrix than was used for culling:\n");
    }
    return false;
  }
  return true;
}
#endif

LMeshRenderingMode landmesh::set_rendering_mode_shadervar(LMeshRenderingMode mode)
{
  G_ASSERT(mode < LMeshRenderingMode::LMESH_MAX);
  if (lmesh_ambient_rendering_mode == mode)
    return lmesh_ambient_rendering_mode;
  ShaderGlobal::set_int(var::lmesh_rendering_mode, static_cast<int>(mode));
  return eastl::exchange(lmesh_ambient_rendering_mode, mode);
}

LMeshRenderingMode LandMeshRenderer::setLMeshRenderingMode(LMeshRenderingMode mode)
{
  lmeshRenderingMode = mode;
  return landmesh::set_rendering_mode_shadervar(mode);
}

LandMeshRenderDesc LandMeshRenderer::legacyRenderDesc() const
{
  LandMeshRenderDesc desc;
  // desc-less compat entries draw with the renderer's configured mode; if it diverges from the
  // process-global ambient mode (changed renderer-free since), the legacy fill picks a stale mode
  G_ASSERTF(lmeshRenderingMode == landmesh::get_rendering_mode_shadervar(), "legacy desc mode %d vs ambient %d",
    (int)lmeshRenderingMode, (int)landmesh::get_rendering_mode_shadervar());
  desc.mode = lmeshRenderingMode;
  desc.renderFlags = lmesh_render_flags;
  desc.renderInBBox = renderInBBox;
  desc.invGeomLodDist = invGeomLodDist;
  desc.regionCb = regionCallback;
  desc.debugCells = debugCells;
  return desc;
}

void LandMeshRenderer::renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshCullingData &culledData,
  const Point3 &view_pos, RenderPurpose rpurpose)
{
  return renderCulled(provider, rtype, culledData, nullptr, nullptr, nullptr, nullptr, view_pos, false, rpurpose);
}

void LandMeshRenderer::renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshCullingData &culledData,
  const TMatrix *realView, const TMatrix4 *realProj, const Driver3dPerspective *persp, const TMatrix4 *realGlobtm,
  const Point3 &view_pos, bool check_matrices, RenderPurpose rpurpose)
{
  // Compat entry: the pass desc is assembled from the legacy setter members.
  RenderPassCtx passCtx;
  passCtx.desc = legacyRenderDesc();
  renderCulledImpl(provider, rtype, culledData, realView, realProj, persp, realGlobtm, view_pos, check_matrices, rpurpose, passCtx);
}

void LandMeshRenderer::renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshRenderDesc &desc,
  const LandMeshCullingData &culledData, const Point3 &view_pos, RenderPurpose rpurpose, const Frustum *cull_frustum)
{
  renderCulled(provider, rtype, desc, culledData, nullptr, nullptr, nullptr, nullptr, view_pos, false, rpurpose, cull_frustum);
}

void LandMeshRenderer::renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshRenderDesc &desc,
  const LandMeshCullingData &culledData, const TMatrix *realView, const TMatrix4 *realProj, const Driver3dPerspective *persp,
  const TMatrix4 *realGlobtm, const Point3 &view_pos, bool check_matrices, RenderPurpose rpurpose, const Frustum *cull_frustum)
{
  // Keep the mode shadervar (and the legacy mode member) in sync while setter-based consumers
  // remain: skyquake decals3d relies on the sticky lmesh_rendering_mode var between passes.
  setLMeshRenderingMode(desc.mode);
  RenderPassCtx passCtx;
  passCtx.desc = desc;
  if (passCtx.desc.invGeomLodDist < 0) // unset: inherit the renderer's lod distance (0 stays the force-LOD0 value)
    passCtx.desc.invGeomLodDist = invGeomLodDist;
  renderCulledImpl(provider, rtype, culledData, realView, realProj, persp, realGlobtm, view_pos, check_matrices, rpurpose, passCtx,
    cull_frustum);
}

void LandMeshRenderer::renderCulledImpl(LandMeshManager &provider, RenderType rtype, const LandMeshCullingData &culledData,
  const TMatrix *realView, const TMatrix4 *realProj, const Driver3dPerspective *persp, const TMatrix4 *realGlobtm,
  const Point3 &view_pos, bool check_matrices, RenderPurpose rpurpose, RenderPassCtx &passCtx, const Frustum *cull_frustum)
{
  // Empty data is fine (a pass may draw before its first cull); data with cells must be a cull
  // result, or geometry LOD would silently be measured from the default (0,0) center.
  G_ASSERTF(culledData.fromCull || !culledData.count, "culling data with cells must come from landmesh::frustum_cull");
  if (lmesh_vs_const__pos_to_world < 0)
    return;
  if (!culledData.count)
  {
    if (provider.isInTools() && !provider.forceHeightmapRendering)
      return;
    if (culledData.hmapDrawType != HmapDrawType::NONE &&
        (!provider.getHmapHandler() || rtype == RENDER_GRASS_MASK || rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES))
      return;
  }

  if (!one_quad) // In case of d3d reset.
    init_one_quad();

  if (rtype == RENDER_GRASS_MASK) // Touch the grass mask textures that were used in the last update of the grass mask.
  {
    for (const auto &landClass : vtex->getLandClassesLoaded())
      landClass.lastUsedGrassMask = false;
  }
  else if (rtype == RENDER_WITH_CLIPMAP)
  {
    for (const auto &landClass : vtex->getLandClassesLoaded())
      if (landClass.lastUsedGrassMask)
      {
        mark_managed_tex_lfu(landClass.grassMask.tid);
        // Avoid intersections with the shader blocks.
        d3d::set_tex(STAGE_CS, 15, D3dResManagerData::getBaseTex(landClass.grassMask.tid));
        d3d::set_sampler(STAGE_CS, 15, landClass.grassMask.sampler);
      }
    d3d::set_tex(STAGE_CS, 15, NULL);
  }

  int oldScene = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
  if (provider.isInTools() && rtype != RENDER_DEPTH && rtype != RENDER_ONE_SHADER)
    vtex->prepareLandClasses(provider);
  // CellState::initMirrorScaleState();
  IPoint2 lt, rb;
  lt.x = provider.getCellOrigin().x;
  lt.y = provider.getCellOrigin().y;
  rb.x = lt.x + provider.getNumCellsX() - 1;
  rb.y = lt.y + provider.getNumCellsY() - 1;

  if (const LandWeightAtlas *wa = provider.getWeightAtlas())
  {
    // the whole table once per pass, since the shader resolves the cell itself
    wa->bindCells();
    wa->setCellMapping(provider.getLandCellSize(), provider.getGridCellSize(), provider.getOffset(), provider.getCellOrigin());
  }

  ShaderGlobal::set_float4(var::world_view_pos, view_pos.x, view_pos.y, view_pos.z, 1.f);

  if (rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES || rtype == RENDER_GRASS_MASK)
    for (int i = 0; i <= lmesh_sampler__max_used_sampler; ++i)
      d3d::settex(i, nullptr);

  if (rtype == RENDER_WITH_CLIPMAP && provider.noVertTexHeightmap())
    ShaderGlobal::set_texture(var::vertical_tex, BAD_TEXTUREID);

  ShaderGlobal::setBlock(land_mesh_object_blkid[rtype], ShaderGlobal::LAYER_SCENE);
  if ((!provider.isInTools() || provider.forceHeightmapRendering) && (passCtx.desc.renderFlags & RENDER_HEIGHTMAP) &&
      culledData.hmapDrawType != HmapDrawType::NONE && provider.getHmapHandler() && rtype != RENDER_GRASS_MASK &&
      rtype != RENDER_CLIPMAP && rtype != RENDER_PATCHES)
  {
    if (culledData.hmapDrawType == HmapDrawType::ONEQUAD)
    {
      // ONEQUAD is produced only by renderImpl, which always supplies its own frustum here.
      G_ASSERT(cull_frustum);
      if (cull_frustum)
        provider.getHmapHandler()->renderOnePatch(*cull_frustum);
    }
    else
    {
      provider.getHmapHandler()->renderCulled(culledData.heightmapData);
    }
    if (rtype == RENDER_WITH_CLIPMAP && provider.noVertTexHeightmap())
    {
      ShaderGlobal::set_texture(var::vertical_tex, vtex->getVertTexId());
      ShaderGlobal::setBlock(land_mesh_object_blkid[rtype], ShaderGlobal::LAYER_SCENE);
    }
  }

  if (!provider.isInTools() && !culledData.count)
  {
    if (oldScene != ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE))
      ShaderGlobal::setBlock(oldScene, ShaderGlobal::LAYER_SCENE);
    return;
  }
  passCtx.useExclBox = culledData.useExclBox;
  passCtx.frustumWorldBBox = culledData.frustumWorldBBox;
  LandMeshRenderer::MirroredCellState::startRender(passCtx);

  // The bootom has its own bias when rendered to scene. That bias will conflict with external one resulting
  // in selfshadowing for example, so skip the bottom rendering and use the external bias for the main land.
  if (rtype == RENDER_DEPTH && VariableMap::isGlobVariablePresent(var::bottom))
  {
    shaders::OverrideState prevState = shaders::overrides::get(shaders::overrides::get_current());
    if (prevState.isOn(shaders::OverrideState::Z_BIAS) && !is_equal_float(prevState.zBias, 0.f, 1e-9f))
      passCtx.skipBottomRendering = true;
  }

  shaders::OverrideStateId prevStateId = shaders::overrides::get_current();
  if (rtype == RENDER_ONE_SHADER || rtype == RENDER_DEPTH)
  {
    ShaderMesh *landm = provider.getCellLandShaderMesh(culledData.centerCell.x, culledData.centerCell.y, 0);
    if (!landm)
      landm = provider.getCellLandShaderMesh(provider.getCellOrigin().x, provider.getCellOrigin().y, 0);
    landm->getAllElems()[0].e->setStates(0, true);
    if (VariableMap::isGlobVariablePresent(var::bottom) && !passCtx.skipBottomRendering)
      prevStateId = setStateDepthBias(STATE_DEPTH_BIAS_ZERO);
  }

#if DAGOR_DBGLEVEL > 0
  if (check_matrices && realView && realProj && realGlobtm && persp)
    check_cull_matrix(*realView, *realProj, *persp, *realGlobtm, "renderCulled_main", culledData, true);
#else
  G_UNUSED(realView);
  G_UNUSED(realProj);
  G_UNUSED(realGlobtm);
  (void)(check_matrices);
#endif

  const LandMeshCellDesc *cellsArr = culledData.cells;

  if (rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES || rtype == RENDER_GRASS_MASK ||
      (passCtx.desc.debugCells && (rtype == RENDER_WITH_CLIPMAP || rtype == RENDER_REFLECTION)))
  {
    if (passCtx.desc.regionCb)
      passCtx.desc.regionCb->startRenderCellRegion(0);

    for (int i = 0; i < culledData.count; i++)
      for (int y = 0; y <= cellsArr[i].countY; y++)
        for (int x = 0; x <= cellsArr[i].countX; x++)
        {
          int borderX = cellsArr[i].borderX + x, borderY = cellsArr[i].borderY + y;
          renderCell(provider, passCtx,
            (borderY - provider.getCellOrigin().y + numBorderCellsZNeg) * tWidth +
              (borderX - provider.getCellOrigin().x + numBorderCellsXNeg),
            0, rtype, rpurpose, false);
        }

    if (passCtx.desc.regionCb)
      passCtx.desc.regionCb->endRenderCellRegion(0);
  }
  else
  {
    float cellSize = provider.getLandCellSize();
    float gridCellSize = provider.getGridCellSize();

    eastl::unique_ptr<RenderCulledCtx, framememDeleter> ctx(new (framemem_ptr()) RenderCulledCtx);

    auto calcLodOffset = [](int mirror, int lod) -> int {
      // [0][0] is special offset, it's excluded
      return mirror * LandMeshManager::LOD_COUNT + lod - 1;
    };

    int regionsCount = culledData.regionsCount;
    int regionCallbackCnt = passCtx.desc.regionCb ? passCtx.desc.regionCb->regionsCount() - 1 : 0;
    int lastRegion = -1;
    {
      memset(ctx->cellCounter, 0, sizeof(ctx->cellCounter)); // reset cell indices
      for (int srcRegioni = 0, dstRegioni = 0; srcRegioni < regionsCount; ++srcRegioni)
      {
        if (culledData.regions[srcRegioni].head != LANDMESH_INVALID_CELL)
        {
          for (int i = culledData.regions[srcRegioni].head; i != LANDMESH_INVALID_CELL; i = culledData.cells[i].next)
            for (int y = 0; y <= cellsArr[i].countY; y++)
              for (int x = 0; x <= cellsArr[i].countX; x++)
              {
                int borderX = cellsArr[i].borderX + x, borderY = cellsArr[i].borderY + y;
                // geometry LOD is measured from the cull batch's own center, not the prepared one
                IPoint2 centerDir = IPoint2(borderX, borderY) - culledData.centerCell;
                Point2 dir = Point2(centerDir.x * cellSize, centerDir.y * cellSize);
                dir.x += centerDir.x < 0   ? cellSize - culledData.centerCellFract.x
                         : centerDir.x > 0 ? -culledData.centerCellFract.x
                                           : 0;
                dir.y += centerDir.y < 0   ? cellSize - culledData.centerCellFract.y
                         : centerDir.y > 0 ? -culledData.centerCellFract.y
                                           : 0;
                int lod = clamp((int)(length(dir) * passCtx.desc.invGeomLodDist), 0, LandMeshManager::LOD_COUNT - 1);
                int cellNo = (borderY - provider.getCellOrigin().y + numBorderCellsZNeg) * tWidth +
                             (borderX - provider.getCellOrigin().x + numBorderCellsXNeg);
                const MirroredCellState &mirroredCell = mirroredCells[cellNo];
                int mirror = mirroredCell.mirrorScaleState.xz;

                auto &cellIndex = ctx->cellCounter[mirror][lod];
                if (mirror || lod)
                  ctx->tmpCellsLodData[calcLodOffset(mirror, lod)][cellIndex] = cellNo;
                else
                  ctx->cellsFlattenedData[cellIndex] = cellNo;
                cellIndex = min(cellIndex + 1, LANDMESH_MAX_CELLS_LEAVES - 1);
              }
        }

        if (dstRegioni < regionCallbackCnt || srcRegioni == regionsCount - 1)
        {
          auto &cellIndexOffset = ctx->cellCounter[0][0];
          lastRegion = dstRegioni;
          ctx->endRegion[dstRegioni][0][0] = cellIndexOffset;
          for (int mirror = 0; mirror < MIRRORS; ++mirror)
            for (int lod = 0; lod < LandMeshManager::LOD_COUNT; ++lod)
              if (mirror || lod)
              {
                auto cellIndexCnt = ctx->cellCounter[mirror][lod];
                if (cellIndexCnt)
                {
                  G_ASSERT(cellIndexCnt <= LANDMESH_MAX_CELLS_LEAVES);
                  G_ASSERT(cellIndexOffset + cellIndexCnt <= ctx->cellsFlattenedData.size());
                  memcpy(ctx->cellsFlattenedData.data() + cellIndexOffset, ctx->tmpCellsLodData[calcLodOffset(mirror, lod)].data(),
                    cellIndexCnt * sizeof(ctx->cellsFlattenedData[0]));

                  cellIndexOffset += ctx->cellCounter[mirror][lod];
                  ctx->cellCounter[mirror][lod] = 0;
                }
                ctx->endRegion[dstRegioni][mirror][lod] = cellIndexOffset;
              }
          memset(&ctx->cellCounter[0][1], 0, sizeof(ctx->cellCounter) - sizeof(cellIndexOffset)); // reset cell indices, except cell
                                                                                                  // offset
          dstRegioni++;
        }
      }
    }

    regionsCount = lastRegion + 1;
    if (!provider.isDecodedToWorldPos()) // can happen only in tools!
    {
      int cellI = 0;
      for (int regioni = 0; regioni < regionsCount; ++regioni)
      {
        if (cellI >= ctx->endRegion[regioni][LAST_MIRROR][LandMeshManager::LOD_COUNT - 1])
          continue;
        if (passCtx.desc.regionCb)
          passCtx.desc.regionCb->startRenderCellRegion(regioni);

        for (int mirror = 0; mirror < MIRRORS; ++mirror)
          for (int lod = 0; lod < LandMeshManager::LOD_COUNT; ++lod)
            for (; cellI < ctx->endRegion[regioni][mirror][lod]; cellI++)
              renderCell(provider, passCtx, ctx->cellsFlattenedData[cellI], lod, rtype, rpurpose, lod > 0);

        if (passCtx.desc.regionCb)
          passCtx.desc.regionCb->endRenderCellRegion(regioni);
      }
    }
    else
    {
      uint8_t use_exclusion = passCtx.useExclBox;
      for (int startCellI = 0, regioni = 0; regioni < regionsCount; ++regioni)
      {
        if (startCellI >= ctx->endRegion[regioni][LAST_MIRROR][LandMeshManager::LOD_COUNT - 1])
          continue;
        // debug("rtype=%d, regioni=%d, cells=%d", rtype, regioni,
        // ctx->endRegion[regioni][LAST_MIRROR][LandMeshManager::LOD_COUNT-1]-startCellI);
        if (passCtx.desc.regionCb)
          passCtx.desc.regionCb->startRenderCellRegion(regioni);

        auto makeCellSpan = [&ctx](int start_id, int end_id) -> eastl::span<uint16_t> {
          G_ASSERT(start_id >= 0 && end_id >= 0);
          G_ASSERT(start_id < end_id);
          G_ASSERT(end_id <= ctx->cellsFlattenedData.size());
          return eastl::span(ctx->cellsFlattenedData.data() + start_id, end_id - start_id);
        };

        if (passCtx.desc.renderFlags & RENDER_COMBINED)
        {
          for (int cellI = startCellI, mirrorI = 0; mirrorI < MIRRORS; ++mirrorI)
          {
            for (int lod = 0; lod < LandMeshManager::LOD_COUNT; cellI = ctx->endRegion[regioni][mirrorI][lod], ++lod)
            {
              if (ctx->endRegion[regioni][mirrorI][lod] > cellI)
              {
                // set mirror
                const MirroredCellState &mirroredCell = mirroredCells[ctx->cellsFlattenedData[cellI]];
                mirroredCell.setPsMirror(passCtx);
                const bool cullFlipped = mirroredCell.setFlipCull(this, passCtx);
                d3d::set_vs_const(lmesh_vs_const__pos_to_world, &worldMulPos[mirroredCell.mirrorScaleState.xz][0].x, 2);
                renderGeomCellsCM(provider, makeCellSpan(cellI, ctx->endRegion[regioni][mirrorI][lod]), rtype, lod > 0, cullFlipped);
              }
            }
          }
        }

        if (passCtx.desc.renderFlags & RENDER_LANDMESH)
        {
          for (int cellI = startCellI, mirrorI = 0; mirrorI < MIRRORS; ++mirrorI)
            for (int lod = 0; lod < LandMeshManager::LOD_COUNT; cellI = ctx->endRegion[regioni][mirrorI][lod], ++lod)
            {
              if (ctx->endRegion[regioni][mirrorI][lod] > cellI)
              {
                const MirroredCellState &mirroredCell = mirroredCells[ctx->cellsFlattenedData[cellI]];
                mirroredCell.setPsMirror(passCtx);
                const bool cullFlipped = mirroredCell.setFlipCull(this, passCtx);
                d3d::set_vs_const(lmesh_vs_const__pos_to_world, &worldMulPos[mirroredCell.mirrorScaleState.xz][0].x, 2);
                renderGeomCellsLM(provider, passCtx, makeCellSpan(cellI, ctx->endRegion[regioni][mirrorI][lod]), lod, rtype,
                  use_exclusion, cullFlipped);
              }
            }
        }
        if (rtype == RENDER_WITH_SPLATTING && (passCtx.desc.renderFlags & RENDER_DECALS))
        {
          for (int cellI = startCellI, mirrorI = 0; mirrorI < MIRRORS; ++mirrorI)
            for (int lod = 0; lod < LandMeshManager::LOD_COUNT; cellI = ctx->endRegion[regioni][mirrorI][lod], ++lod)
            {
              if (ctx->endRegion[regioni][mirrorI][lod] > cellI)
              {
                for (int ci = cellI; ci < ctx->endRegion[regioni][mirrorI][lod]; ++ci)
                {
                  const MirroredCellState &mirroredCell = mirroredCells[ctx->cellsFlattenedData[ci]];
                  mirroredCell.setPsMirror(passCtx);
                  mirroredCell.setFlipCull(this, passCtx);
                  mirroredCell.setPosConsts(false);
                  renderCellDecals(provider, passCtx, mirroredCell);
                }
              }
            }
        }
        if (passCtx.desc.regionCb)
          passCtx.desc.regionCb->endRenderCellRegion(regioni);
        startCellI = ctx->endRegion[regioni][LAST_MIRROR][LandMeshManager::LOD_COUNT - 1];
      }
    }
  }

  if (passCtx.cullFlipped)
  {
    passCtx.cullFlipped = false;
    passCtx.cullFlippedCurStateId = shaders::OverrideStateId();
    resetOverride(passCtx.cullFlippedPrevStateId);
  }

  resetOverride(prevStateId);

  if (oldScene != ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE))
    ShaderGlobal::setBlock(oldScene, ShaderGlobal::LAYER_SCENE);
}


bool LandMeshRenderer::renderDecals(LandMeshManager &provider, RenderType rtype, const TMatrix4 &globtm, HmapOrigin hmap_origin,
  bool compatibility_mode, bool use_samplers_no_mipbias)
{
  // Compat entry: the pass desc is assembled from the legacy setter members.
  LandMeshRenderDesc desc = legacyRenderDesc();
  desc.decalsCompatibilityMode = compatibility_mode;
  desc.decalsSamplersNoMipBias = use_samplers_no_mipbias;
  return renderDecals(provider, rtype, desc, globtm, hmap_origin);
}

bool LandMeshRenderer::renderDecals(LandMeshManager &provider, RenderType rtype, const LandMeshRenderDesc &desc,
  const TMatrix4 &globtm, HmapOrigin hmap_origin)
{
  Frustum frustum(globtm);
  LandMeshCullDesc cullDesc = LandMeshCullDesc::forView(hmap_origin.pos, hmap_origin, frustum);
  cullDesc.renderInBBox = desc.renderInBBox;
  cullDesc.useDetailedHmap = false;
  cullDesc.noCulling = !desc.renderInBBox.isempty(); // faster culling - no bbox testing

  LandMeshCullingData defaultCullData(framemem_ptr());
  landmesh::frustum_cull(provider, cullDesc, defaultCullData);

  int oldScene = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
  ShaderGlobal::setBlock(land_mesh_object_blkid[rtype], ShaderGlobal::LAYER_SCENE);
  bool renderedAnything = renderCulledDecals(provider, desc, defaultCullData);
  if (oldScene != ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE))
    ShaderGlobal::setBlock(oldScene, ShaderGlobal::LAYER_SCENE);

  return renderedAnything;
}

bool LandMeshRenderer::renderCulledDecals(LandMeshManager &provider, const LandMeshCullingData &culledData, bool compatibility_mode,
  bool use_samplers_no_mipbias)
{
  // Compat entry: the pass desc is assembled from the legacy setter members.
  LandMeshRenderDesc desc = legacyRenderDesc();
  desc.decalsCompatibilityMode = compatibility_mode;
  desc.decalsSamplersNoMipBias = use_samplers_no_mipbias;
  return renderCulledDecals(provider, desc, culledData);
}

bool LandMeshRenderer::renderCulledDecals(LandMeshManager &provider, const LandMeshRenderDesc &desc,
  const LandMeshCullingData &culledData)
{
  // Decal shaders branch on the sticky lmesh_rendering_mode shadervar (clipmap decals skip
  // when it is not clipmap), so sync it here like the desc renderCulled entry does.
  setLMeshRenderingMode(desc.mode);
  G_ASSERTF(culledData.fromCull || !culledData.count, "culling data with cells must come from landmesh::frustum_cull");
  G_ASSERT(cellStates);
  if (!cellStates)
    return false;

  // Own pass ctx: also called directly (not only via renderDecals), so the mirror const must be
  // re-established here instead of trusting whatever the previous pass left behind.
  RenderPassCtx passCtx;
  passCtx.desc = desc;
  passCtx.frustumWorldBBox = culledData.frustumWorldBBox;
  MirroredCellState::startRender(passCtx);

  const LandMeshCellDesc *cellsArr = culledData.cells;

  bool renderedAnything = false;
  for (int i = 0; i < culledData.count; i++)
    for (int y = 0; y <= cellsArr[i].countY; y++)
      for (int x = 0; x <= cellsArr[i].countX; x++)
      {
        int borderX = cellsArr[i].borderX + x, borderY = cellsArr[i].borderY + y;
        int cellNo = (borderY - provider.getCellOrigin().y + numBorderCellsZNeg) * tWidth +
                     (borderX - provider.getCellOrigin().x + numBorderCellsXNeg);

        const MirroredCellState &mirroredCell = mirroredCells[cellNo];

        ShaderMesh *decalm = provider.getCellDecalShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY);
        if (!decalm || !(passCtx.desc.renderFlags & RENDER_DECALS) || decalm->getAllElems().empty())
          continue;

        mirroredCell.setPsMirror(passCtx);
        mirroredCell.setPosConsts(false);
        CellState &curState = cellStates[mirroredCell.cellX + mirroredCell.cellY * provider.getNumCellsX()];
        mirroredCell.setDetMapTc();
        curState.set(false, vtex->getLandClassesLoaded(), true);

        if (!desc.decalsCompatibilityMode)
        {
          for (int detailI = 0; detailI < curState.numDetailTextures; ++detailI)
          {
            set_ps_const1_opt(lmesh_ps_const__invtexturesizes, curState.invTexSizes[detailI >> 2][detailI & 3]);
            const LCTexturesLoaded &landLoaded = vtex->getLandClassesLoaded()[curState.lcIds[detailI]];
            set_ps_const_opt(lmesh_ps_const__bumpscales, landLoaded.bumpScales);
            set_ps_const_opt(lmesh_ps_const__water_decal_bump_scale, landLoaded.waterDecalBumpScale);
          }
        }

        renderedAnything |= renderCellDecals(provider, passCtx, mirroredCell, desc.decalsSamplersNoMipBias);
      }

  return renderedAnything;
}

void LandMeshRenderer::render(LandMeshManager &provider, RenderType rtype, const Point3 &view_pos, HmapOrigin hmap_origin,
  RenderPurpose rpurpose)
{
  if (lmesh_vs_const__pos_to_world < 0)
    return;
  mat44f globtm;
  d3d::getglobtm(globtm);
  Frustum frustum(globtm);
  TMatrix4 proj;
  d3d::gettm(TM_PROJ, &proj);
  return render(globtm, proj, frustum, provider, rtype, view_pos, hmap_origin, rpurpose);
}

void LandMeshRenderer::render(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, LandMeshManager &provider,
  RenderType rtype, const Point3 &view_pos, HmapOrigin hmap_origin, RenderPurpose rpurpose)
{
  // Compat entry: the pass desc is assembled from the legacy setter members.
  renderImpl(globtm, proj, frustum, provider, rtype, legacyRenderDesc(), view_pos, hmap_origin, rpurpose);
}

void LandMeshRenderer::render(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, LandMeshManager &provider,
  RenderType rtype, const LandMeshRenderDesc &desc, const Point3 &view_pos, HmapOrigin hmap_origin, RenderPurpose rpurpose)
{
  renderImpl(globtm, proj, frustum, provider, rtype, desc, view_pos, hmap_origin, rpurpose);
}

void LandMeshRenderer::render(LandMeshManager &provider, RenderType rtype, const LandMeshRenderDesc &desc, const Point3 &view_pos,
  HmapOrigin hmap_origin, RenderPurpose rpurpose)
{
  mat44f globtm;
  d3d::getglobtm(globtm);
  Frustum frustum(globtm);
  TMatrix4 proj;
  d3d::gettm(TM_PROJ, &proj);
  renderImpl(globtm, proj, frustum, provider, rtype, desc, view_pos, hmap_origin, rpurpose);
}

void LandMeshRenderer::renderImpl(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, LandMeshManager &provider,
  RenderType rtype, const LandMeshRenderDesc &desc, const Point3 &view_pos, HmapOrigin hmap_origin, RenderPurpose rpurpose)
{
  // The sticky mode sync happens even when the missing shader consts make the draw
  // a no-op, matching the desc renderCulled entry (its own sync then no-ops).
  setLMeshRenderingMode(desc.mode);
  if (lmesh_vs_const__pos_to_world < 0)
    return;

  // Clipmap / grass-mask / patches passes render the land mesh into a texture and never draw the terrain
  // heightmap over it, so they keep the exclusion box off and leave hmapDrawType = NONE.
  const bool clipmapLikePass = rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES || rtype == RENDER_GRASS_MASK ||
                               (desc.debugCells && (rtype == RENDER_WITH_CLIPMAP || rtype == RENDER_REFLECTION));

  // Decide how the heightmap is drawn for THIS pass (was the persistent renderHeightmapType member).
  // A desc with RENDER_HEIGHTMAP off keeps NONE, or the exclusion below would hide land cells
  // that no heightmap draw then fills.
  HmapDrawType drawType = HmapDrawType::NONE;
  if (!clipmapLikePass && (desc.renderFlags & RENDER_HEIGHTMAP) && (!provider.isInTools() || provider.forceHeightmapRendering) &&
      provider.getHmapHandler())
  {
    // ortho matrix is probably 0,0,0,1, however, we just test .w to be 1. Enough in 100% of our test cases.
    if (fabsf(v_extract_w(globtm.col3) - 1.0f) < 0.001f)
    {
      // ortho bakes (lastClip, depth/masks above) always include the heightmap, even when mayRenderHmap
      // hides it in regular views (it only gates the perspective branch below) - otherwise terrain beyond
      // the landmesh cells would go missing from the baked textures while the heightmap is hidden
      if (rtype == RENDER_ONE_SHADER)
      {
        vec4f vert = v_abs(v_and(globtm.col1, v_cast_vec4f(V_CI_MASK1100)));
        vert = v_add(vert, v_rot_1(vert));
        // a top-down ortho projection (degenerate projected Y) draws a single quad instead of the grid
        drawType = v_test_vec_x_gt(V_C_EPS_VAL, vert) ? HmapDrawType::ONEQUAD : HmapDrawType::TESSELATED;
      }
      else
        drawType = HmapDrawType::TESSELATED;
    }
    // perspective views also honor prepare()'s distance gate (shouldRenderTessellatedHmap); without it a
    // far / outside-hmap view would build and draw a tessellated heightmap and hide land cells anyway
    else if (
      provider.mayRenderHmap && provider.getHmapHandler()->shouldRenderTessellatedHmap(hmap_origin.pos, hmap_origin.distanceMul))
      drawType = HmapDrawType::TESSELATED;
  }

  LandMeshCullDesc cullDesc = LandMeshCullDesc::forView(hmap_origin.pos, hmap_origin, frustum);
  cullDesc.renderInBBox = desc.renderInBBox;
  cullDesc.useDetailedHmap = drawType == HmapDrawType::TESSELATED;
  cullDesc.hmapMetrics = {proj_to_distance_scale(proj), int8_t(desc.mode == LMeshRenderingMode::RENDERING_HEIGHTMAP ? -100 : 0),
    desc.mode == LMeshRenderingMode::RENDERING_HEIGHTMAP ? HeightmapMetricsQuality::FASTEST : HeightmapMetricsQuality::BEST};

  LandMeshCullingData defaultCullData(framemem_ptr());

  if (clipmapLikePass)
  {
    cullDesc.noCulling = !desc.renderInBBox.isempty(); // faster culling - no bbox testing
    landmesh::frustum_cull(provider, cullDesc, defaultCullData);
    // clipmap-like passes cull without exclusion but still hide land cells under the heightmap
    // exactly like other passes: the render-side decision stays prepare()'s persistent bit.
    defaultCullData.useExclBox = provider.cullingState.useExclBox;
  }
  else
  {
    // Per-pass exclusion decision follows the draw decision instead of mutating the shared
    // provider culling state other passes read (tools keep their editor-config bit).
    cullDesc.useExclBox =
      (!provider.isInTools() || provider.forceHeightmapRendering) ? drawType != HmapDrawType::NONE : provider.cullingState.useExclBox;
    if (desc.regionCb)
    {
      cullDesc.regions = desc.regionCb->regions();
      cullDesc.regionsCount = desc.regionCb->regionsCount();
    }
    landmesh::frustum_cull(provider, cullDesc, defaultCullData);
  }
  defaultCullData.hmapDrawType = drawType; // authoritative draw decision for renderCulled() (also carries ONEQUAD)
  // the ONEQUAD heightmap path culls its single patch against this pass's frustum
  renderCulled(provider, rtype, desc, defaultCullData, view_pos, rpurpose, &frustum);

  if (provider.isInTools() && !provider.forceHeightmapRendering)
  {
    Point4 posToWorld[2] = {Point4(1.f, 1.f, 1., 0), Point4(0, 0, 0, 1)};
    d3d::set_vs_const(lmesh_vs_const__pos_to_world, (float *)&posToWorld[0].x, 2);
    set_ps_const_opt(lmesh_ps_const__mirror_scale, posToWorld[0]);
  }
}


void LandMeshRenderer::setMirroring(LandMeshManager &provider, int num_border_cells_x_pos, int num_border_cells_x_neg,
  int num_border_cells_z_pos, int num_border_cells_z_neg)
{
  provider.setMirroring(num_border_cells_x_pos, num_border_cells_x_neg, num_border_cells_z_pos, num_border_cells_z_neg);
  if (provider.getVisibilityRangeCells() < 0)
    provider.getScaledBorderCells(numBorderCellsXPos, numBorderCellsXNeg, numBorderCellsZPos, numBorderCellsZNeg);
  else
    numBorderCellsXPos = numBorderCellsXNeg = numBorderCellsZPos = numBorderCellsZNeg = 0;

  // Config change invalidates the derived per-cell mirror tables; before this they silently stayed
  // stale when setMirroring was called after the first prepare().
  clear_and_shrink(mirroredCells);
}
