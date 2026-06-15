// Copyright (C) Gaijin Games KFT.  All rights reserved.

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
#include <memory/dag_framemem.h>
#include <EASTL/string.h>
#include <EASTL/bitset.h>
#include <EASTL/hash_map.h>
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


eastl::hash_map<eastl::string, int> shadersNames;


int LandMeshRenderer::lod1_switch_radius = 4096;
enum
{
  BOTTOM_ABOVE = 0,
  BOTTOM_BELOW = 1,
  BOTTOM_COUNT,
};
static bool skip_bottom_rendering = false;


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

void LandMeshRenderer::MirroredCellState::startRender()
{
  set_ps_const1_opt(lmesh_ps_const__mirror_scale, 1.f, 1.f);
  current_mirror_mask = 0;
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

void LandMeshRenderer::MirroredCellState::setPsMirror() const
{
  uint32_t mirror_mask = 0x3 & (mirror_mask_table >> (mirrorScaleState.xz << 1));
  if (lmesh_ps_const__mirror_scale >= 0 && mirror_mask != current_mirror_mask)
  {
    d3d::set_ps_const1(lmesh_ps_const__mirror_scale, (mirror_mask & 1) ? -1.f : 1.f, (mirror_mask & 2) ? -1.f : 1.f, 0, 0);
    current_mirror_mask = mirror_mask;
  }
}

bool LandMeshRenderer::MirroredCellState::setFlipCull(LandMeshRenderer *renderer) const
{
  if (getInvCull() != currentCullFlipped)
  {
    if (currentCullFlipped)
    {
      G_ASSERT(shaders::overrides::get_current() == currentCullFlippedCurStateId);
      renderer->resetOverride(currentCullFlippedPrevStateId);
    }
    else
    {
      G_ASSERT(!currentCullFlippedPrevStateId);
      currentCullFlippedPrevStateId = renderer->setStateFlipCull(true);
      currentCullFlippedCurStateId = shaders::overrides::get_current();
    }
    currentCullFlipped = !currentCullFlipped;
    return true;
  }
  return false;
}

uint32_t LandMeshRenderer::MirroredCellState::current_mirror_mask = 0;
bool LandMeshRenderer::MirroredCellState::currentCullFlipped = false;
shaders::OverrideStateId LandMeshRenderer::MirroredCellState::currentCullFlippedPrevStateId;
shaders::OverrideStateId LandMeshRenderer::MirroredCellState::currentCullFlippedCurStateId;

LandMeshRenderer::LandMeshRenderer(LandMeshManager &provider, dag::ConstSpan<LandClassDetailTextures> land_classes,
  int biome_land_class_idx, TEXTUREID vert_tex_id, TEXTUREID vert_nm_tex_id, TEXTUREID vert_det_tex_id, TEXTUREID tile_tex,
  d3d::SamplerHandle tile_smp, real tile_x, real tile_y) :
  tileTexId(tile_tex),
  tileTexSmp(tile_smp),
  tileXSize(tile_x),
  tileYSize(tile_y),
  shouldForceLowQuality(false),
  shouldRenderTrivially(false),
  optScn(NULL),
  undetailedLCMicroDetail(0.971f),
  detailedLCMicroDetail(0.251),
  tWidth(0)
{
  mem_set_0(megaDetailsArray);
  renderHeightmapType = NO_HMAP;
  useHmapTankDetail = 0;
  hmapSubDivLod0 = 0;
  verLabel = VER_LABEL;
  debugCells = false;

  cellStates = NULL;
  landClasses = land_classes;
  biomeLandClassIdx = biome_land_class_idx;

  regionCallback = NULL;
  numBorderCellsXPos = 0;
  numBorderCellsXNeg = 0;
  numBorderCellsZPos = 0;
  numBorderCellsZNeg = 0;
  vertTexId = vert_tex_id;
  vertNmTexId = vert_nm_tex_id;
  vertDetTexId = vert_det_tex_id;

  if (d3d::get_driver_desc().info.vendor == GpuVendor::QUALCOMM)
    var::skip_detail_lod_calculation.set_int(1);

#define VAR(a, def) a = get_shader_int_constant(#a, a);
  LMESH_CONST_LIST
#undef VAR
  if (lmesh_sampler__land_detail_array1 < 0)
    lmesh_ps_const_land_detail_array_slices = -1;
  if (lmesh_ps_const_land_detail_array_slices < 0)
    lmesh_sampler__land_detail_array1 = -1;

  if (lmesh_physmats__buffer_idx > 0)
    physmatIdsBuf = d3d::buffers::create_one_frame_sr_byte_address(DET_TEX_NUM * 4, "physmats_IDS", RESTAG_LAND);
  else
    physmatIdsBuf = NULL;

  G_ASSERT(lmesh_sampler__land_detail_map >= 0);
  G_ASSERT(lmesh_sampler__land_detail_tex1 >= 0);

  for (int i = 0; i < decals_overrideSampler_const_no_array.size(); ++i)
  {
    String smpReg(64, "decals_overrideSampler_%i_const_no", i);
    decals_overrideSampler_const_no_array[i] = get_shader_int_constant(smpReg, -1);
  }

  centerCell = IPoint2(0xDEAFBABE, 0xDEAFBABE);

  float cellSize = provider.getLandCellSize();
  landmeshCombinedDistSq = 4096;
  landmeshCombinedDistSq *= landmeshCombinedDistSq;
  invGeomLodDist = 1.f / (sqrt(2.0f) * lod1_switch_radius);

  int detMapElemSize, size;
  provider.getDetailMapSize(detMapElemSize, size);

  scaleVisRange = (int)floor(4096.0f / cellSize + 0.5f);

  ShaderGlobal::set_texture(var::vertical_tex, vertTexId);
  ShaderGlobal::set_texture(var::vertical_nm_tex, vertNmTexId);
  ShaderGlobal::set_texture(var::vertical_det_tex, vertDetTexId);

  detMapTcScale = detMapElemSize / (size * cellSize);
  detMapTcOfs = 0.f;

  has_detailed_land_classes = false;
  for (int i = 0; i < landClasses.size(); ++i)
  {
    if (landClasses[i].lcType != LC_SIMPLE)
    {
      has_detailed_land_classes = true;
      break;
    }
  }

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
  resetTextures();

  for (const ShaderVariableInfo *v : {&var::vertical_tex, &var::vertical_nm_tex, &var::vertical_det_tex})
    ShaderGlobal::set_texture(*v, BAD_TEXTUREID);
  if (cellStates)
    delete[] cellStates;
  cellStates = NULL;

  if (useConditionalRendering)
    bboxShader.close();
  del_d3dres(one_quad);
  del_d3dres(physmatIdsBuf);
  landclassShader.clear();
  shadersNames.erase(shadersNames.begin(), shadersNames.end());
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

shaders::OverrideStateId LandMeshRenderer::setStateBlend()
{
  shaders::OverrideState state;
  state.set(shaders::OverrideState::BLEND_SRC_DEST);
  state.sblend = BLEND_ONE;
  state.dblend = BLEND_ONE;
  return setOverride(state);
}

void LandMeshRenderer::evictSplattingData()
{
  resetTextures();
  for (int i = 0; i < landClassesLoaded.size(); ++i)
    landClassesLoaded[i] = LCTexturesLoaded();

  if (cellStates) // They keep pointers to unreferenced textures and become invalid as the cells themselves.
    delete[] cellStates;
  cellStates = NULL;
}

void LandMeshRenderer::resetTextures()
{
  d3d::GpuAutoLock gpuLock;
  if (lmesh_sampler__land_detail_map >= 0)
    d3d::settex(lmesh_sampler__land_detail_map, NULL);
  if (lmesh_sampler__land_detail_map2 >= 0)
    d3d::settex(lmesh_sampler__land_detail_map2, NULL);

  for (int i = 0; i < DET_TEX_NUM; ++i)
  {
    d3d::settex(lmesh_sampler__land_detail_tex1 + i, NULL);
  }
}

void LandMeshRenderer::prepare(LandMeshManager &provider, const Point3 &pos, float hmap_camera_height)
{
  prepare(provider, pos, hmap_camera_height, HeightmapHeightCulling::NO_WATER_ON_LEVEL);
}

void LandMeshRenderer::prepare(LandMeshManager &provider, const Point3 &pos, float hmap_camera_height, float water_level)
{
  G_UNUSED(hmap_camera_height); // HeightmapHandler::prepare no longer depends on camera height or water level
  G_UNUSED(water_level);
  if (provider.isDecodedToWorldPos() && !optScn)
  {
    optScn = new landmesh::OptimizedScene[LandMeshManager::LOD_COUNT];
    for (int lod = 0; lod < LandMeshManager::LOD_COUNT; ++lod)
      landmesh::buildOptSceneData(optScn[lod], provider, lod);
  }

  if (provider.isInTools())
  {
    delete[] cellStates;
    cellStates = NULL;
  }
  else if (provider.getHmapHandler())
  {
    renderHeightmapType = (provider.mayRenderHmap && provider.getHmapHandler()->prepare(pos)) ? TESSELATED_HMAP : NO_HMAP;
    provider.cullingState.useExclBox = renderHeightmapType != NO_HMAP;
  }

  if (!cellStates && !provider.getDetailMap().cells.empty())
  {
    prepareLandClasses(provider);
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
          const LCTexturesLoaded &landLoaded = landClassesLoaded[cellStates[x + y * provider.getNumCellsX()].lcIds[di]];
          if (landLoaded.colorMap.tid)
          {
            validId = cellStates[x + y * provider.getNumCellsX()].lcIds[di];
            break;
          }
        }
        if (validId < 0)
          for (int di = 0; di < landClassesLoaded.size(); ++di)
          {
            const LCTexturesLoaded &landLoaded = landClassesLoaded[di];
            if (landLoaded.colorMap.tid)
            {
              validId = di;
              break;
            }
          }
        for (int di = 0; di < cellStates[x + y * provider.getNumCellsX()].numDetailTextures; ++di)
        {
          auto &lc = cellStates[x + y * provider.getNumCellsX()].lcIds[di];
          const LCTexturesLoaded &landLoaded = landClassesLoaded[lc];
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
  Point2 centerCellPos = Point2::xz(pos - meshOffset);
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
  TEXTUREID tex1Id, tex2Id;
  provider.getLandDetailTexture(mirrorX, mirrorY, tex1Id, tex2Id, curState.lcIds.data());
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

  if (!landClasses.size())
  {
    mul[0] = Color4(1, 1, 1, 1);
    mul[1] = Color4(1, 1, 1, 1);
  }
  else
  {
    mul[0] = Color4(curState.lcIds[0] == 0xFF ? 1.f : landClasses[curState.lcIds[0]].tile,
      curState.lcIds[1] == 0xFF ? 1.f : landClasses[curState.lcIds[1]].tile,
      curState.lcIds[2] == 0xFF ? 1.f : landClasses[curState.lcIds[2]].tile,
      curState.lcIds[3] == 0xFF ? 1.f : landClasses[curState.lcIds[3]].tile);

    mul[1] = Color4(curState.lcIds[4] == 0xFF ? 1.f : landClasses[curState.lcIds[4]].tile,
      curState.lcIds[5] == 0xFF ? 1.f : landClasses[curState.lcIds[5]].tile,
      curState.lcIds[6] == 0xFF ? 1.f : landClasses[curState.lcIds[6]].tile, 1);

    for (int i = 0; i < 3; ++i)
      ofs[i] = Color4(curState.lcIds[i * 2] == 0xFF ? 0.f : landClasses[curState.lcIds[i * 2]].offset.x,
        curState.lcIds[i * 2] == 0xFF ? 0.f : landClasses[curState.lcIds[i * 2]].offset.y,
        curState.lcIds[i * 2 + 1] == 0xFF ? 0.f : landClasses[curState.lcIds[i * 2 + 1]].offset.x,
        curState.lcIds[i * 2 + 1] == 0xFF ? 0.f : landClasses[curState.lcIds[i * 2 + 1]].offset.y);

    ofs[3] = Color4(curState.lcIds[6] == 0xFF ? 0.f : landClasses[curState.lcIds[6]].offset.x,
      curState.lcIds[6] == 0xFF ? 0.f : landClasses[curState.lcIds[6]].offset.y, 0, 0);

    for (int i = 0; i < 4; ++i)
    {
      ofs[i].r -= meshOffset.x;
      ofs[i].g -= meshOffset.z;
      ofs[i].b -= meshOffset.x;
      ofs[i].a -= meshOffset.z;
    }
  }


  curState.detMapTc = Color4(detMapTcScale, detMapTcScale, detMapTcOfs - mirrorX * cellSize - meshOffset.x,
    detMapTcOfs - mirrorY * cellSize - meshOffset.z);
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
  curState.map1 = ACQUIRE_MANAGED_TEX(tex1Id);
  RELEASE_MANAGED_TEX(tex1Id);
  curState.map2 = ACQUIRE_MANAGED_TEX(tex2Id);
  RELEASE_MANAGED_TEX(tex2Id);

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
    if (curState.lcIds[i] < landClassesLoaded.size())
    {
      curState.invTexSizes[i >> 2][i & 3] = landClassesLoaded[lcId].invColormapSize.x;
      if (!landClassesLoaded[lcId].colorMap.tid)
      {
        if (provider.isInTools())
          curState.numDetailTextures = i;
      }
      else if (landClasses[lcId].lcType != LC_SIMPLE)
        curState.trivial = false;
    }
  }
  if (!curState.map1)
    curState.numDetailTextures = 1;
  else if (!curState.map2)
    curState.numDetailTextures = min(curState.numDetailTextures, 5);
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

void LandMeshRenderer::renderGeomCellsLM(LandMeshManager &provider, dag::ConstSpan<uint16_t> cells, int lodNo, RenderType rtype,
  uint8_t hide_excluded, bool force_set_states)
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
  //  - honorSkipBottom    : in DEPTH path, BELOW returns false and ABOVE skips bias adjust when skip_bottom_rendering is set
  struct RosdCB : landmesh::IRosdSetStatesCB
  {
    LandMeshRenderer *renderer;
    mutable shaders::OverrideStateId prevStateId;
    bool setStatesFlag, applyDepthBiasFlag, honorSkipBottom;

    RosdCB(LandMeshRenderer *r, bool set_states, bool apply_depth_bias, bool honor_skip_bottom) :
      renderer(r),
      prevStateId(shaders::overrides::get_current()),
      setStatesFlag(set_states),
      applyDepthBiasFlag(apply_depth_bias),
      honorSkipBottom(honor_skip_bottom)
    {}

    virtual bool applyMat(ShaderElement *mat, int bottom_type) const override
    {
      if (setStatesFlag && !mat->setStates(0, true))
        return false;
      if (applyDepthBiasFlag && VariableMap::isGlobVariablePresent(var::bottom))
      {
        if (honorSkipBottom && skip_bottom_rendering)
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
    landmesh::renderOptSceneData(optScn[lodNo], RosdCB(this, /*setStates*/ force_set_states, /*depthBias*/ true, /*skipBottom*/ true));
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

bool LandMeshRenderer::renderCellDecals(LandMeshManager &provider, const MirroredCellState &mirroredCell,
  bool force_samplers_no_mipbias)
{
  ShaderMesh *decalm = provider.getCellDecalShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY);
  if (!decalm || !(lmesh_render_flags & RENDER_DECALS))
    return false;
  int id = mirroredCell.cellX + mirroredCell.cellY * provider.getNumCellsX();

  bool renderedAnything = false;
  if (!provider.isInTools() && provider.getDecalElems().size() &&
      provider.getDecalElems()[id].elemBoxes.size() == decalm->getAllElems().size())
  {
    bool setSamplersNoMipbias =
      (getLMeshRenderingMode() == LMeshRenderingMode::RENDERING_CLIPMAP || force_samplers_no_mipbias) &&
      eastl::find_if(decals_overrideSampler_const_no_array.begin(), decals_overrideSampler_const_no_array.end(),
        [](int smp_reg) { return smp_reg >= 0; }) != decals_overrideSampler_const_no_array.end();

    DECL_ALIGN16(IBBox2, subCellBox);
    Point3 meshOffset = provider.getOffset();
    float cellSize = provider.getLandCellSize();
    IPoint2 cellOfs(provider.getCellOrigin().x * 65535, provider.getCellOrigin().y * 65535);
    if (!renderInBBox.isempty())
    {
      cellSize /= 65535.f;
      subCellBox[0].x = (int)floorf((renderInBBox[0].x - meshOffset.x) / cellSize);
      subCellBox[0].y = (int)floorf((renderInBBox[0].z - meshOffset.z) / cellSize);
      subCellBox[1].x = (int)floorf((renderInBBox[1].x - meshOffset.x) / cellSize);
      subCellBox[1].y = (int)floorf((renderInBBox[1].z - meshOffset.z) / cellSize);
      subCellBox[0] -= cellOfs;
      subCellBox[1] -= cellOfs;
    }
    else
    {
      vec4f invGridCellSzV = v_splats(65535.0f / cellSize);
      vec4f ofs = v_make_vec4f(meshOffset.x, meshOffset.z, meshOffset.x, meshOffset.z);
      vec4f worldBboxXZ = v_perm_xzac(frustumWorldBBox.bmin, frustumWorldBBox.bmax);
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
    if (!provider.isInTools() && getLMeshRenderingMode() == LMeshRenderingMode::RENDERING_CLIPMAP)
      LOGERR_ONCE("renderCellDecals: Fallback branch was used, samplerState might be incorrect during RENDERING_CLIPMAP.");
  }

  return renderedAnything;
}


void LandMeshRenderer::renderCell(LandMeshManager &provider, int cellNo, int lodNo, RenderType rtype, RenderPurpose rpurpose,
  bool skip_combined_not_marked_as_big = false)
{
  // fixme:
  //  Get cell geometry.

  // Set shader vars for mirroring.
  // int cellX = mirrorX + provider.getCellOrigin().x, cellY = mirrorY + provider.getCellOrigin().y;
  const MirroredCellState &mirroredCell = mirroredCells[cellNo];

  bool hide_land = provider.cullingState.useExclBox && mirroredCell.excluded;

  bool isDecoded = provider.isDecodedToWorldPos();

  if ((rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES || rtype == RENDER_GRASS_MASK) || !isDecoded)
    mirroredCell.setPosConsts(rpurpose == RENDER_FOR_GRASS);
  else
    d3d::set_vs_const(lmesh_vs_const__pos_to_world, &worldMulPos[mirroredCell.mirrorScaleState.xz][0].x, 2);
  CellState &curState = cellStates[mirroredCell.cellX + mirroredCell.cellY * provider.getNumCellsX()];

  mirroredCell.setFlipCull(this);
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
    curState.set(rtype == RENDER_GRASS_MASK, landClassesLoaded, shouldRenderTrivially, false,
      rtype == RENDER_GRASS_MASK ? physmatIdsBuf : NULL);
  }


  // Render.

  if (rtype == RENDER_WITH_CLIPMAP || rtype == RENDER_REFLECTION)
  {
    if (debugCells && VariableMap::isGlobVariablePresent(var::landmesh_debug_cells_scale))
    {
      float s = ((mirroredCell.cellX + mirroredCell.cellY) & 1) ? 0.2f : 1.0f;
      ShaderGlobal::set_float4(var::landmesh_debug_cells_scale, Color4(s, s, s, s));
    }
    mirroredCell.setPsMirror();
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
    renderCellDecals(provider, mirroredCell);
  }
  else if (rtype == RENDER_CLIPMAP)
  {
    if (curState.trivial || shouldRenderTrivially)
    {
      if (landclassShader[LC_TRIVIAL].elem->setStates(0, true)) // trivial case: all landclasses are simple
      {
        d3d::setvsrc_ex(0, one_quad, 0, sizeof(short) * 4);
        d3d::draw(PRIM_TRISTRIP, 0, 2);
      }
    }
    else
      renderLandclasses(curState);

    renderCellDecals(provider, mirroredCell);

    ShaderMesh *combinedm = provider.getCellCombinedShaderMeshOffseted(mirroredCell.cellX, mirroredCell.cellY);
    if (combinedm && (lmesh_render_flags & RENDER_COMBINED))
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
      renderLandclasses(curState, true, LC_CUSTOM);
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
      skip_bottom_rendering = false;
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

LMeshRenderingMode LandMeshRenderer::setLMeshRenderingMode(LMeshRenderingMode mode)
{
  G_ASSERT(mode < LMeshRenderingMode::LMESH_MAX);
  if (lmeshRenderingMode == mode)
    return lmeshRenderingMode;

  ShaderGlobal::set_int(var::lmesh_rendering_mode, static_cast<int>(mode));
  return eastl::exchange(lmeshRenderingMode, mode);
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
  if (lmesh_vs_const__pos_to_world < 0)
    return;
  if (!culledData.count)
  {
    if (provider.isInTools())
      return;
    if (renderHeightmapType != NO_HMAP &&
        (!provider.getHmapHandler() || rtype == RENDER_GRASS_MASK || rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES))
      return;
  }

  if (!one_quad) // In case of d3d reset.
    init_one_quad();

  if (rtype == RENDER_GRASS_MASK) // Touch the grass mask textures that were used in the last update of the grass mask.
  {
    for (const auto &landClass : landClassesLoaded)
      landClass.lastUsedGrassMask = false;
  }
  else if (rtype == RENDER_WITH_CLIPMAP)
  {
    for (const auto &landClass : landClassesLoaded)
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
    prepareLandClasses(provider);
  // CellState::initMirrorScaleState();
  IPoint2 lt, rb;
  lt.x = provider.getCellOrigin().x;
  lt.y = provider.getCellOrigin().y;
  rb.x = lt.x + provider.getNumCellsX() - 1;
  rb.y = lt.y + provider.getNumCellsY() - 1;

  if (rtype == RENDER_WITH_CLIPMAP && !shouldForceLowQuality)
  {
    float scaleMicroDetail = undetailedLCMicroDetail;
    if (!shouldRenderTrivially && has_detailed_land_classes)
      scaleMicroDetail = detailedLCMicroDetail;
  }

  // int opt_vs = (!renderInBBox.isempty() && renderInBBox.width().length() > big_clipmap_criterio);
  // ShaderGlobal::set_int_fast(optimizeVsId, opt_vs);

  ShaderGlobal::set_float4(var::world_view_pos, view_pos.x, view_pos.y, view_pos.z, 1.f);

  if (rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES || rtype == RENDER_GRASS_MASK)
    for (int i = 0; i <= lmesh_sampler__max_used_sampler; ++i)
      d3d::settex(i, nullptr);

  if (rtype == RENDER_WITH_CLIPMAP && provider.noVertTexHeightmap())
    ShaderGlobal::set_texture(var::vertical_tex, BAD_TEXTUREID);

  ShaderGlobal::setBlock(land_mesh_object_blkid[rtype], ShaderGlobal::LAYER_SCENE);
  if (!provider.isInTools() && (lmesh_render_flags & RENDER_HEIGHTMAP) && renderHeightmapType != NO_HMAP &&
      provider.getHmapHandler() && rtype != RENDER_GRASS_MASK && rtype != RENDER_CLIPMAP && rtype != RENDER_PATCHES)
  {
    if (renderHeightmapType == ONEQUAD_HMAP)
    {
      provider.getHmapHandler()->renderOnePatch();
    }
    else
    {
      provider.getHmapHandler()->renderCulled(culledData.heightmapData);
    }
    if (rtype == RENDER_WITH_CLIPMAP && provider.noVertTexHeightmap())
    {
      ShaderGlobal::set_texture(var::vertical_tex, vertTexId);
      ShaderGlobal::setBlock(land_mesh_object_blkid[rtype], ShaderGlobal::LAYER_SCENE);
    }
  }

  if (!provider.isInTools() && !culledData.count)
  {
    if (oldScene != ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE))
      ShaderGlobal::setBlock(oldScene, ShaderGlobal::LAYER_SCENE);
    return;
  }
  LandMeshCullingState cullingState;

  cullingState.copyLandmeshState(provider, *this);

  LandMeshRenderer::MirroredCellState::startRender();

  // The bootom has its own bias when rendered to scene. That bias will conflict with external one resulting
  // in selfshadowing for example, so skip the bottom rendering and use the external bias for the main land.
  skip_bottom_rendering = false;
  if (rtype == RENDER_DEPTH && VariableMap::isGlobVariablePresent(var::bottom))
  {
    shaders::OverrideState prevState = shaders::overrides::get(shaders::overrides::get_current());
    if (prevState.isOn(shaders::OverrideState::Z_BIAS) && !is_equal_float(prevState.zBias, 0.f, 1e-9f))
      skip_bottom_rendering = true;
  }

  shaders::OverrideStateId prevStateId = shaders::overrides::get_current();
  if (rtype == RENDER_ONE_SHADER || rtype == RENDER_DEPTH)
  {
    ShaderMesh *landm = provider.getCellLandShaderMesh(centerCell.x, centerCell.y, 0);
    if (!landm)
      landm = provider.getCellLandShaderMesh(provider.getCellOrigin().x, provider.getCellOrigin().y, 0);
    landm->getAllElems()[0].e->setStates(0, true);
    if (VariableMap::isGlobVariablePresent(var::bottom) && !skip_bottom_rendering)
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
      (debugCells && (rtype == RENDER_WITH_CLIPMAP || rtype == RENDER_REFLECTION)))
  {
    if (regionCallback)
      regionCallback->startRenderCellRegion(0);

    for (int i = 0; i < culledData.count; i++)
      for (int y = 0; y <= cellsArr[i].countY; y++)
        for (int x = 0; x <= cellsArr[i].countX; x++)
        {
          int borderX = cellsArr[i].borderX + x, borderY = cellsArr[i].borderY + y;
          renderCell(provider,
            (borderY - provider.getCellOrigin().y + numBorderCellsZNeg) * tWidth +
              (borderX - provider.getCellOrigin().x + numBorderCellsXNeg),
            0, rtype, rpurpose, false);
        }

    if (regionCallback)
      regionCallback->endRenderCellRegion(0);
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
    int regionCallbackCnt = regionCallback ? regionCallback->regionsCount() - 1 : 0;
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
                IPoint2 centerDir = IPoint2(borderX, borderY) - centerCell;
                Point2 dir = Point2(centerDir.x * cellSize, centerDir.y * cellSize);
                dir.x += centerDir.x < 0 ? cellSize - centerCellFract.x : centerDir.x > 0 ? -centerCellFract.x : 0;
                dir.y += centerDir.y < 0 ? cellSize - centerCellFract.y : centerDir.y > 0 ? -centerCellFract.y : 0;
                int lod = clamp((int)(length(dir) * invGeomLodDist), 0, LandMeshManager::LOD_COUNT - 1);
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
        if (regionCallback)
          regionCallback->startRenderCellRegion(regioni);

        for (int mirror = 0; mirror < MIRRORS; ++mirror)
          for (int lod = 0; lod < LandMeshManager::LOD_COUNT; ++lod)
            for (; cellI < ctx->endRegion[regioni][mirror][lod]; cellI++)
              renderCell(provider, ctx->cellsFlattenedData[cellI], lod, rtype, rpurpose, lod > 0);

        if (regionCallback)
          regionCallback->endRenderCellRegion(regioni);
      }
    }
    else
    {
      uint8_t use_exclusion = provider.cullingState.useExclBox;
      for (int startCellI = 0, regioni = 0; regioni < regionsCount; ++regioni)
      {
        if (startCellI >= ctx->endRegion[regioni][LAST_MIRROR][LandMeshManager::LOD_COUNT - 1])
          continue;
        // debug("rtype=%d, regioni=%d, cells=%d", rtype, regioni,
        // ctx->endRegion[regioni][LAST_MIRROR][LandMeshManager::LOD_COUNT-1]-startCellI);
        if (regionCallback)
          regionCallback->startRenderCellRegion(regioni);

        auto makeCellSpan = [&ctx](int start_id, int end_id) -> eastl::span<uint16_t> {
          G_ASSERT(start_id >= 0 && end_id >= 0);
          G_ASSERT(start_id < end_id);
          G_ASSERT(end_id <= ctx->cellsFlattenedData.size());
          return eastl::span(ctx->cellsFlattenedData.data() + start_id, end_id - start_id);
        };

        if (lmesh_render_flags & RENDER_COMBINED)
        {
          for (int cellI = startCellI, mirrorI = 0; mirrorI < MIRRORS; ++mirrorI)
          {
            for (int lod = 0; lod < LandMeshManager::LOD_COUNT; cellI = ctx->endRegion[regioni][mirrorI][lod], ++lod)
            {
              if (ctx->endRegion[regioni][mirrorI][lod] > cellI)
              {
                // set mirror
                const MirroredCellState &mirroredCell = mirroredCells[ctx->cellsFlattenedData[cellI]];
                mirroredCell.setPsMirror();
                const bool cullFlipped = mirroredCell.setFlipCull(this);
                d3d::set_vs_const(lmesh_vs_const__pos_to_world, &worldMulPos[mirroredCell.mirrorScaleState.xz][0].x, 2);
                renderGeomCellsCM(provider, makeCellSpan(cellI, ctx->endRegion[regioni][mirrorI][lod]), rtype, lod > 0, cullFlipped);
              }
            }
          }
        }

        if (lmesh_render_flags & RENDER_LANDMESH)
        {
          for (int cellI = startCellI, mirrorI = 0; mirrorI < MIRRORS; ++mirrorI)
            for (int lod = 0; lod < LandMeshManager::LOD_COUNT; cellI = ctx->endRegion[regioni][mirrorI][lod], ++lod)
            {
              if (ctx->endRegion[regioni][mirrorI][lod] > cellI)
              {
                const MirroredCellState &mirroredCell = mirroredCells[ctx->cellsFlattenedData[cellI]];
                mirroredCell.setPsMirror();
                const bool cullFlipped = mirroredCell.setFlipCull(this);
                d3d::set_vs_const(lmesh_vs_const__pos_to_world, &worldMulPos[mirroredCell.mirrorScaleState.xz][0].x, 2);
                renderGeomCellsLM(provider, makeCellSpan(cellI, ctx->endRegion[regioni][mirrorI][lod]), lod, rtype, use_exclusion,
                  cullFlipped);
              }
            }
        }
        if (rtype == RENDER_WITH_SPLATTING && (lmesh_render_flags & RENDER_DECALS))
        {
          for (int cellI = startCellI, mirrorI = 0; mirrorI < MIRRORS; ++mirrorI)
            for (int lod = 0; lod < LandMeshManager::LOD_COUNT; cellI = ctx->endRegion[regioni][mirrorI][lod], ++lod)
            {
              if (ctx->endRegion[regioni][mirrorI][lod] > cellI)
              {
                for (int ci = cellI; ci < ctx->endRegion[regioni][mirrorI][lod]; ++ci)
                {
                  const MirroredCellState &mirroredCell = mirroredCells[ctx->cellsFlattenedData[ci]];
                  mirroredCell.setPsMirror();
                  mirroredCell.setFlipCull(this);
                  mirroredCell.setPosConsts(false);
                  renderCellDecals(provider, mirroredCell);
                }
              }
            }
        }
        if (regionCallback)
          regionCallback->endRenderCellRegion(regioni);
        startCellI = ctx->endRegion[regioni][LAST_MIRROR][LandMeshManager::LOD_COUNT - 1];
      }
    }
  }

  if (MirroredCellState::currentCullFlipped)
  {
    MirroredCellState::currentCullFlipped = false;
    MirroredCellState::currentCullFlippedCurStateId = shaders::OverrideStateId();
    resetOverride(MirroredCellState::currentCullFlippedPrevStateId);
  }

  resetOverride(prevStateId);

  skip_bottom_rendering = false;

  if (oldScene != ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE))
    ShaderGlobal::setBlock(oldScene, ShaderGlobal::LAYER_SCENE);
}


bool LandMeshRenderer::renderDecals(LandMeshManager &provider, RenderType rtype, const TMatrix4 &globtm, bool compatibility_mode,
  bool use_samplers_no_mipbias)
{
  LandMeshCullingState state;
  state.copyLandmeshState(provider, *this);

  Frustum frustum(globtm);
  frustum.calcFrustumBBox(frustumWorldBBox);
  state.useExclBox = false;
  if (!state.renderInBBox.isempty())
    state.cullMode = state.NO_CULLING; // faster culling - no bbox testing

  Point3 hmapOriginPos(0.f, 0.f, 0.f);
  if (provider.getHmapHandler())
    hmapOriginPos = provider.getHmapHandler()->getPreparedOriginPos();
  LandMeshCullingData defaultCullData(framemem_ptr());
  state.frustumCulling(provider, defaultCullData, NULL, 0, HeightmapFrustumCullingInfo{hmapOriginPos, frustum, NULL, NULL, false});

  LandMeshRenderer::MirroredCellState::startRender();

  int oldScene = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
  ShaderGlobal::setBlock(land_mesh_object_blkid[rtype], ShaderGlobal::LAYER_SCENE);
  bool renderedAnything = renderCulledDecals(provider, defaultCullData, compatibility_mode, use_samplers_no_mipbias);
  if (oldScene != ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE))
    ShaderGlobal::setBlock(oldScene, ShaderGlobal::LAYER_SCENE);

  return renderedAnything;
}

bool LandMeshRenderer::renderCulledDecals(LandMeshManager &provider, const LandMeshCullingData &culledData, bool compatibility_mode,
  bool use_samplers_no_mipbias)
{
  G_ASSERT(cellStates);
  if (!cellStates)
    return false;

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
        if (!decalm || !(lmesh_render_flags & RENDER_DECALS) || decalm->getAllElems().empty())
          continue;

        mirroredCell.setPsMirror();
        mirroredCell.setPosConsts(false);
        CellState &curState = cellStates[mirroredCell.cellX + mirroredCell.cellY * provider.getNumCellsX()];
        mirroredCell.setDetMapTc();
        curState.set(false, landClassesLoaded, shouldRenderTrivially, true);

        if (!compatibility_mode)
        {
          for (int detailI = 0; detailI < curState.numDetailTextures; ++detailI)
          {
            set_ps_const1_opt(lmesh_ps_const__invtexturesizes, curState.invTexSizes[detailI >> 2][detailI & 3]);
            const LCTexturesLoaded &landLoaded = landClassesLoaded[curState.lcIds[detailI]];
            set_ps_const_opt(lmesh_ps_const__bumpscales, landLoaded.bumpScales);
            set_ps_const_opt(lmesh_ps_const__water_decal_bump_scale, landLoaded.waterDecalBumpScale);
          }
        }

        renderedAnything |= renderCellDecals(provider, mirroredCell, use_samplers_no_mipbias);
      }

  return renderedAnything;
}

void LandMeshRenderer::render(LandMeshManager &provider, RenderType rtype, const Point3 &view_pos, RenderPurpose rpurpose)
{
  if (lmesh_vs_const__pos_to_world < 0)
    return;
  mat44f globtm;
  d3d::getglobtm(globtm);
  Frustum frustum(globtm);
  TMatrix4 proj;
  d3d::gettm(TM_PROJ, &proj);
  return render(globtm, proj, frustum, provider, rtype, view_pos, rpurpose);
}

void LandMeshRenderer::render(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, LandMeshManager &provider,
  RenderType rtype, const Point3 &view_pos, RenderPurpose rpurpose)
{
  if (lmesh_vs_const__pos_to_world < 0)
    return;

  HmapRenderType prevRenderHeightmapType = renderHeightmapType;

  LandMeshCullingState state;
  state.copyLandmeshState(provider, *this);

  if ((rtype == RENDER_WITH_SPLATTING || rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES || rtype == RENDER_GRASS_MASK ||
        (debugCells && (rtype == RENDER_WITH_CLIPMAP || rtype == RENDER_REFLECTION))) &&
      (lmesh_render_flags & RENDER_DECALS))
    frustum.calcFrustumBBox(frustumWorldBBox);

  Point3 hmapOriginPos(0.f, 0.f, 0.f);
  if (provider.getHmapHandler())
    hmapOriginPos = provider.getHmapHandler()->getPreparedOriginPos();

  LandMeshCullingData defaultCullData(framemem_ptr());
  if (rtype == RENDER_DEPTH)
    defaultCullData.heightmapData.useHWTesselation = false;
  defaultCullData.heightmapData.frustum = frustum;
  HeightmapFrustumCullingInfo fi{hmapOriginPos, frustum, NULL, NULL, false,
    {proj_to_distance_scale(proj), int8_t(lmeshRenderingMode == LMeshRenderingMode::RENDERING_HEIGHTMAP ? -100 : 0),
      lmeshRenderingMode == LMeshRenderingMode::RENDERING_HEIGHTMAP ? HeightmapMetricsQuality::FASTEST
                                                                    : HeightmapMetricsQuality::BEST}};

  if (rtype == RENDER_CLIPMAP || rtype == RENDER_PATCHES || rtype == RENDER_GRASS_MASK ||
      (debugCells && (rtype == RENDER_WITH_CLIPMAP || rtype == RENDER_REFLECTION)))
  {
    state.useExclBox = false;
    if (!state.renderInBBox.isempty())
      state.cullMode = state.NO_CULLING; // faster culling - no bbox testing
    state.frustumCulling(provider, defaultCullData, NULL, 0, fi);
  }
  else
  {
    if (!provider.isInTools() && provider.getHmapHandler())
    {
      // mat44f m2;
      // v_mat44_transpose(m2, globtm);
      // vec4f is_ortho = v_cmp_gt(V_C_EPS_VAL, v_abs(v_sub(m2.col3, V_C_UNIT_0001)));
      // if (v_test_vec_x_eqi_0(v_splat_w(is_ortho)))//ortho matrix is probably 0,0,0,1, however, we just test .w to be 1. Enough in
      // 100% of our test cases
      if (fabsf(v_extract_w(globtm.col3) - 1.0f) < 0.001f)
      {
        if (rtype == RENDER_ONE_SHADER)
        {
          vec4f vert = v_abs(v_and(globtm.col1, v_cast_vec4f(V_CI_MASK1100)));
          vert = v_add(vert, v_rot_1(vert));
          renderHeightmapType = v_test_vec_x_gt(V_C_EPS_VAL, vert) ? ONEQUAD_HMAP : TESSELATED_HMAP; // todo: use only when rendering
                                                                                                     // heightmap
        }
        else
          renderHeightmapType = TESSELATED_HMAP;
        provider.cullingState.useExclBox = true;
      }
    }
    fi.useDetailedHmap = renderHeightmapType == TESSELATED_HMAP;
    if (regionCallback)
      state.frustumCulling(provider, defaultCullData, regionCallback->regions(), regionCallback->regionsCount(), fi);
    else
      state.frustumCulling(provider, defaultCullData, NULL, 0, fi);
  }
  renderCulled(provider, rtype, defaultCullData, view_pos, rpurpose);

  if (!provider.isInTools())
  {
    renderHeightmapType = prevRenderHeightmapType;
    provider.cullingState.useExclBox = renderHeightmapType != NO_HMAP;
  }
  else
  {
    Point4 posToWorld[2] = {Point4(1.f, 1.f, 1., 0), Point4(0, 0, 0, 1)};
    d3d::set_vs_const(lmesh_vs_const__pos_to_world, (float *)&posToWorld[0].x, 2);
    set_ps_const_opt(lmesh_ps_const__mirror_scale, posToWorld[0]);
  }
}


void LandMeshRenderer::setMirroring(LandMeshManager &provider, int num_border_cells_x_pos, int num_border_cells_x_neg,
  int num_border_cells_z_pos, int num_border_cells_z_neg)
{
  numBorderCellsXPos = num_border_cells_x_pos * scaleVisRange;
  numBorderCellsXNeg = num_border_cells_x_neg * scaleVisRange;
  numBorderCellsZPos = num_border_cells_z_pos * scaleVisRange;
  numBorderCellsZNeg = num_border_cells_z_neg * scaleVisRange;

  int visRange = provider.getVisibilityRangeCells();
  if (visRange < 0)
  {
    int x0, x1, y0, y1;

    x0 = provider.getCellOrigin().x;
    x1 = x0 + provider.getNumCellsX() - 1;

    y0 = provider.getCellOrigin().y;
    y1 = y0 + provider.getNumCellsY() - 1;

    if (numBorderCellsXPos > x1 - x0 + 1)
      numBorderCellsXPos = x1 - x0 + 1;

    if (numBorderCellsXNeg > x1 - x0 + 1)
      numBorderCellsXNeg = x1 - x0 + 1;

    if (numBorderCellsZPos > y1 - y0 + 1)
      numBorderCellsZPos = y1 - y0 + 1;

    if (numBorderCellsZNeg > y1 - y0 + 1)
      numBorderCellsZNeg = y1 - y0 + 1;
  }
  else
  {
    numBorderCellsXPos = numBorderCellsXNeg = numBorderCellsZPos = numBorderCellsZNeg = 0;
  }
}
