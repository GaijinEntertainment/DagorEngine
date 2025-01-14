// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/randomGrass.h>
#include <render/landMask.h>
#include <render/noiseTex.h>
#include <debug/dag_log.h>
#include <debug/dag_debug3d.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_frustum.h>
#include <math/dag_bounds2.h>
#include <math/random/dag_random.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <util/dag_oaHashNameMap.h>
#include <3d/dag_texIdSet.h>
#include <3d/dag_texPackMgr2.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_platform.h>
#include <3d/dag_dynAtlas.h>
#include <supp/dag_prefetch.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_rendInstRes.h>
#include <gameRes/dag_stdGameResId.h>
#include <shaders/dag_postFxRenderer.h>
#include <startup/dag_globalSettings.h>
#include <scene/dag_occlusion.h>
#include <shaders/dag_overrideStates.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_hlsl_floatx.h>
#include <render/randomGrassInstance.hlsli>
#include <render/vertexDataCompression.hlsli>
#include <3d/dag_lockSbuffer.h>
#include <webui/editVarNotifications.h>
#include <frustumCulling/frustumPlanes.h>
#include <bvh/bvh_connection.h>

#define LMESH_DELTA      5.f
#define SORT_SQUARE_SIZE 2.f
#define MAX_INSTANCES    72

#define DEFAULT_MIN_DENSITY    0.1f
#define DEFAULT_MAX_DENSITY    1.f
#define DEFAULT_LOD_FADE_DELTA 10.f
#define DEFAULT_MAX_RADIUS     300.f
#define DEFAULT_FADE_DELTA     70.f

enum
{
  RENDER_DEPTH_OPAQUE = 0,
  RENDER_DEPTH_DISSOLVE = 1,
  RENDER_COLOR_OPAQUE = 2,
  RENDER_COLOR_DISSOLVE = 3,
  RENDER_COLOR_ALPHA = 4,
};

enum
{
  SHV_RENDER_TYPE,
  SHV_DEPTH_PREPASS,
  SHV_SCALE_OFFSET,
  SHV_WIND_STATE,
  SHV_MASK_NO,
  SHV_LOD_NO,
  SHV_LOD_RANGE,

  SHV_CELL_TO_SQUARE_WORLD_REG,
  SHV_GRASS_LAYER_IMMEDIATE_NO,

  SHV_GRASS_RANGE,
  SHV_GRASS_TEX,
  SHV_GRASS_TEX_ALPHA,
  SHV_GRASS_LAYERS_COUNT,
  SHV_GRASS_ALPHA_TO_COVERAGE,
  SHV_WORLD_VIEW_POS,
  SHV_COUNT
};

static int grass_bumpVarId = -1;

BVHConnection *RandomGrass::bvhConnection = nullptr;

struct CellVertex
{
  struct
  {
    uint32_t x;
  } pos;
  struct
  {
    uint32_t x;
  } texcoord_drawId;
};

struct CellVertexGL
{
  struct
  {
    uint16_t x, y, z, w;
  } pos;
  struct
  {
    uint16_t x, y, z, w;
  } texcoord;
};

struct MeshVertex
{
  Point3 pos;
  Point3 norm;
  Point2 texcoord;
};

bool is_box_inside_sphere(const BSphere3 &sphere, const BBox3 &bbox)
{
  if (sphere.r <= 0.f)
    return false;
  return (bbox.width().length() / 2 + (sphere.c - bbox.center()).length()) < sphere.r;
}

RandomGrass::RandomGrass(const DataBlock &level_grass_blk, const DataBlock &params_blk) :
  useSorting(true),
  vboForMaxElements(false),
  isDissolve(false),
  isColorDebug(false),
  centerPos(0.f, 0.f, 0.f),
  squareCenter(0, 0),
  minFarLodRadius(0.f),
  gridIndices(midmem),
  layers(midmem),
  combinedLods(midmem),
  waterLevel(MAX_REAL),
  shouldRender(true),
  globalDensityMul(1.f),
  gridBoxes(),
  lmeshPoints(),
  randomGrassGenerator(NULL),
  randomGrassIndirect(NULL)
{
  cellSize = 1;
  baseNumInstances = 0;
  maxLodCount = 0;
  maxLayerCount = 0;

#if DAGOR_DBGLEVEL > 0
  varNotification = eastl::make_unique<EditableVariablesNotifications>();
#endif

  // shader vars
  shaderVars.resize(SHV_COUNT);

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_WRITE_DISABLE);
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_EQUAL;
  afterPrepassOverride.reset(shaders::overrides::create(state));

  shaderVars[SHV_RENDER_TYPE] = ::get_shader_variable_id("grass_render_type");
  shaderVars[SHV_DEPTH_PREPASS] = ::get_shader_variable_id("grass_depth_prepass");
  shaderVars[SHV_SCALE_OFFSET] = ::get_shader_variable_id("grass_scale_offset");
  shaderVars[SHV_WIND_STATE] = ::get_shader_variable_id("grass_wind_state", true);
  shaderVars[SHV_LOD_NO] = ::get_shader_variable_id("grass_lod_id");

  shaderVars[SHV_GRASS_RANGE] = ::get_shader_variable_id("random_grass_max_radius", true);

  shaderVars[SHV_GRASS_TEX] = ::get_shader_variable_id("grass_tex", true);
  shaderVars[SHV_GRASS_TEX_ALPHA] = ::get_shader_variable_id("grass_tex_alpha", true);

  shaderVars[SHV_GRASS_LAYERS_COUNT] = ::get_shader_variable_id("grass_layers_count", true);
  shaderVars[SHV_GRASS_ALPHA_TO_COVERAGE] = ::get_shader_variable_id("grass_alpha_to_coverage", true);
  shaderVars[SHV_WORLD_VIEW_POS] = ::get_shader_variable_id("world_view_pos");

  shaderVars[SHV_CELL_TO_SQUARE_WORLD_REG] = ShaderGlobal::get_int_fast(get_shader_variable_id("cell_to_square_world_const_no"));
  shaderVars[SHV_GRASS_LAYER_IMMEDIATE_NO] = ShaderGlobal::get_int_fast(get_shader_variable_id("grass_layer_no_immediate_const_no"));

  grass_bumpVarId = ::get_shader_variable_id("grass_bump", true);

  grassRadiusMul = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("grassRadiusMul", 1.f);
  grassDensityMul = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("grassDensityMul", 1.f);

  // shader consts
  randomGrassIndirect = new_compute_shader("random_grass_create_indirect");
  randomGrassGenerator = new_compute_shader("random_grass_generate_cs");

  randomGrassShader.init("random_grass_gpu", NULL, 0, "random grass shader");
  grassBufferGenerated = false;

  ShaderGlobal::set_int(shaderVars[SHV_GRASS_ALPHA_TO_COVERAGE], alphaToCoverage ? 1 : 0);

  // level params
  loadParams(level_grass_blk, params_blk);
  init_and_get_argb8_64_noise().setVar();
}


void RandomGrass::deleteBuffers() { clear_and_shrink(combinedLods); }

void RandomGrass::closeTextures()
{
  for (unsigned int layerNo = 0; layerNo < layers.size(); layerNo++)
  {
    GrassLayer *layer = layers[layerNo];
    release_managed_tex(layer->lods[0].alphaTexId);
    release_managed_tex(layer->lods[1].alphaTexId);
    release_managed_tex(layer->lods[0].diffuseTexId);
    release_managed_tex(layer->lods[1].diffuseTexId);
  }
}

RandomGrass::~RandomGrass()
{
  release_64_noise();
  resetLayers();
  closeTextures();

  deleteBuffers();

  for (unsigned int layerNo = 0; layerNo < layers.size(); layerNo++)
  {
    GrassLayer *layer = layers[layerNo];
    ::release_game_resource((GameResource *)layer->resource);
    delete layer;
  }

  del_it(randomGrassIndirect);
  del_it(randomGrassGenerator);
}


void RandomGrass::loadParams(const DataBlock &level_grass_blk, const DataBlock &params_blk)
{
  minDensity = level_grass_blk.getReal("minLodDensity", DEFAULT_MIN_DENSITY);
  maxDensity = level_grass_blk.getReal("density", DEFAULT_MAX_DENSITY);

  refMaxRadius = curMaxRadius = level_grass_blk.getReal("maxRadius", DEFAULT_MAX_RADIUS);
  refFadeDelta = curFadeDelta = level_grass_blk.getReal("fadeDelta", DEFAULT_FADE_DELTA);

  lodFadeDelta = level_grass_blk.getReal("lodFadeDelta", DEFAULT_LOD_FADE_DELTA);

  Point2 noiseRnd = Point2(level_grass_blk.getReal("noise_first_rnd", 0.f), level_grass_blk.getReal("noise_second_rnd", 0.f));

  Point4 noiseVal = Point4(level_grass_blk.getReal("noise_first_val_mul", 1.f), level_grass_blk.getReal("noise_first_val_add", 0.f),
    level_grass_blk.getReal("noise_second_val_mul", 1.f), level_grass_blk.getReal("noise_second_val_add", 0.f));


  // layers params
  auto getBitMask = [](const DataBlock &blk, uint64_t def) {
    if (!blk.paramExists("id"))
      return def;
    uint64_t bitMask = 0;
    for (int i = blk.findParam("id", -1); i >= 0; i = blk.findParam("id", i))
      bitMask |= 1ULL << uint64_t(blk.getInt(i));
    return bitMask;
  };
  uint64_t rgbBitMask[3] = {1, 2, 4};
  rgbBitMask[0] = getBitMask(*level_grass_blk.getBlockByNameEx("redMaskBiomes"), 1);
  rgbBitMask[1] = getBitMask(*level_grass_blk.getBlockByNameEx("greenMaskBiomes"), 2);
  rgbBitMask[2] = getBitMask(*level_grass_blk.getBlockByNameEx("blueMaskBiomes"), 4);

  for (unsigned int blockNo = 0; blockNo < level_grass_blk.blockCount(); blockNo++)
  {
    const DataBlock *layerBlk = level_grass_blk.getBlock(blockNo % level_grass_blk.blockCount());
    if (stricmp(layerBlk->getBlockName(), "layer") != 0)
      continue;

    GrassLayerInfo layer_info;

    layer_info.density = layerBlk->getReal("density", 1.f);
    if (layer_info.density <= 0)
      continue;
    uint64_t bitMask = getBitMask(*layerBlk, 0);
    if (layerBlk->paramExists("maskChannel"))
    {
      Color4 maskChannel = Color4::xyzw(layerBlk->getPoint4("maskChannel", Point4(1, 0, 0, 0)));
      G_ASSERTF(rgbsum(maskChannel) > 0, "grass maskChannel in layer <%d>, res=<%s> has zero rgb!", blockNo,
        layerBlk->getStr("res", ""));
      for (int mi = 0; mi < 3; ++mi)
        bitMask |= maskChannel[mi] > 0.0f ? rgbBitMask[mi] : 0;
    }
    if (bitMask == 0)
    {
      G_ASSERTF(bitMask != 0, "grass should have at least one id or maskChannel in layer <%d>, res=<%s> has zero mask!", blockNo,
        layerBlk->getStr("res", ""));
      bitMask = ~uint64_t(0);
    }
    debug("grass <%d:%s> maskis = %X", blockNo, layerBlk->getStr("res", ""), bitMask);

    layer_info.bitMask = bitMask;
    layer_info.radiusMul = min(1.f, layerBlk->getReal("radiusMul", 1.f));
    layer_info.windMul = layerBlk->getReal("windMul", 1.f);

    layer_info.crossBlend = layerBlk->getReal("crossBlend", 0);

    if (layer_info.crossBlend < 0.)
      layer_info.crossBlend = 0.;
    if (layer_info.crossBlend > 1.)
      layer_info.crossBlend = 1.;

    layer_info.resName = layerBlk->getStr("res", "");

    layer_info.maxScale.x = layerBlk->getReal("horScale", 1.f);
    layer_info.maxScale.y = layerBlk->getReal("verScale", 1.f);

    layer_info.minScale.x = layerBlk->getReal("horMinScale", 0.4f);
    layer_info.minScale.y = layerBlk->getReal("verMinScale", 0.2f);

    layer_info.colors[CHANNEL_RED].start = color4(layerBlk->getE3dcolor("color_mask_r_from", E3DCOLOR(0, 0, 0)));
    layer_info.colors[CHANNEL_RED].end = color4(layerBlk->getE3dcolor("color_mask_r_to", E3DCOLOR(255, 255, 255)));

    layer_info.colors[CHANNEL_GREEN].start = color4(layerBlk->getE3dcolor("color_mask_g_from", E3DCOLOR(0, 0, 0)));
    layer_info.colors[CHANNEL_GREEN].end = color4(layerBlk->getE3dcolor("color_mask_g_to", E3DCOLOR(128, 128, 128)));

    layer_info.colors[CHANNEL_BLUE].start = color4(layerBlk->getE3dcolor("color_mask_b_from", E3DCOLOR(0, 0, 0)));
    layer_info.colors[CHANNEL_BLUE].end = color4(layerBlk->getE3dcolor("color_mask_b_to", E3DCOLOR(255, 255, 255)));

    addLayer(layer_info);
  }

#if DAGOR_DBGLEVEL > 0
  initWebTools();
#endif

  replaceVdecls();
  fillLayers(grassRadiusMul, params_blk);


  // shader vars
  ShaderGlobal::set_color4_fast(::get_shader_variable_id("grass_noise_rnd"), noiseRnd.x, noiseRnd.y, 0, 0);
  ShaderGlobal::set_color4_fast(::get_shader_variable_id("grass_noise_val"), noiseVal.x, noiseVal.y, noiseVal.z, noiseVal.w);
}

void RandomGrass::onReset()
{
  resetLayers();

  for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
    layers[layerIdx]->info.resetLayerVB = true;

  resetLayersVB();
}

void RandomGrass::replaceLayerVdecl(GrassLayer &layer)
{
  for (unsigned int lodIdx = 0; lodIdx < layer.lods.size(); ++lodIdx)
  {
    GrassLod &lod = layer.lods[lodIdx];
    static CompiledShaderChannelId chan[] = {{SCTYPE_UINT1, SCUSAGE_POS, 0, 0}, {SCTYPE_UINT1, SCUSAGE_TC, 0, 0}};
    lod.vdecl = dynrender::addShaderVdecl(chan, sizeof(chan) / sizeof(chan[0]));

    lod.mesh->getAllElems()[0].e->replaceVdecl(lod.vdecl);
  }
}

void RandomGrass::replaceVdecls()
{
  for (unsigned int layerNo = 0; layerNo < layers.size(); layerNo++)
    replaceLayerVdecl(*layers[layerNo]);
}

BBox2 RandomGrass::getRenderBbox() const
{
  BBox2 render_bbox;
  Point2 center2((float)squareCenter.x * cellSize, (float)squareCenter.y * cellSize);
  float radius2 = (float)cellSize * GRID_SIZE / 2.f;
  render_bbox[0] = center2 - Point2(radius2, radius2);
  render_bbox[1] = center2 + Point2(radius2, radius2);
  return render_bbox;
}

void RandomGrass::setWaterLevel(float v) { waterLevel = v; }

void RandomGrass::beforeRender(const Point3 &center_pos, IRandomGrassHelper &render_helper, const LandMask &land_mask,
  Occlusion *occlusion)
{
  TIME_D3D_PROFILE(RandomGrass_beforeRender);
  G_UNUSED(occlusion);

  if (!render_helper.isValid())
    return;

  prepassIsValid = false;

  centerPos.y = center_pos.y;

  bool move = false;
  int newSquareCenterX = (int)floorf(center_pos.x / cellSize + 0.5f);
  int newSquareCenterZ = (int)floorf(center_pos.z / cellSize + 0.5f);
  move = newSquareCenterX != squareCenter.x || newSquareCenterZ != squareCenter.y;
  if (move)
  {
    if (abs(newSquareCenterX - squareCenter.x) > abs(newSquareCenterZ - squareCenter.y))
      newSquareCenterZ = squareCenter.y;
    else
      newSquareCenterX = squareCenter.x;

    centerPos = center_pos;
    squareCenter.x = newSquareCenterX;
    squareCenter.y = newSquareCenterZ;
  }

  BBox2 bbox2 = land_mask.getRenderBbox();
  Point2 width = bbox2.width();
  ShaderGlobal::set_color4(grass_bumpVarId, 1.0 / land_mask.getLandTexSize(), width.x / land_mask.getLandTexSize(),
    land_mask.getMaxLandHeight() - land_mask.getMinLandHeight(), 0);

  shouldRender = true;
}

void RandomGrass::setAlphaToCoverage(bool alpha_to_coverage)
{
  alphaToCoverage = alpha_to_coverage;
  ShaderGlobal::set_int(shaderVars[SHV_GRASS_ALPHA_TO_COVERAGE], alphaToCoverage ? 1 : 0);
}

void RandomGrass::renderDepth(const LandMask &land_mask)
{
  TIME_D3D_PROFILE(grass_depth);

  if (!shouldRender)
    return;

  beginRender(land_mask);
  ShaderGlobal::set_int_fast(shaderVars[SHV_RENDER_TYPE], isDissolve ? RENDER_DEPTH_DISSOLVE : RENDER_DEPTH_OPAQUE);
  draw(land_mask, true, 0, maxLodCount);

  endRender();
}

void RandomGrass::renderTrans(const LandMask &land_mask)
{
  TIME_D3D_PROFILE(grass_trans);

  if (!shouldRender)
    return;

  beginRender(land_mask);
  ShaderGlobal::set_int_fast(shaderVars[SHV_RENDER_TYPE], RENDER_COLOR_ALPHA);
  draw(land_mask, false, 0, maxLodCount);

  endRender();
}

int opt_random_grass_lod = 2;

void RandomGrass::renderDepthOptPrepass(const LandMask &land_mask)
{
  TIME_D3D_PROFILE(grass_opaque_depth_prepass);

  if (!shouldRender)
    return;

  beginRender(land_mask);

  if (opt_random_grass_lod)
  {
    ShaderGlobal::set_int_fast(shaderVars[SHV_RENDER_TYPE], isDissolve ? RENDER_DEPTH_DISSOLVE : RENDER_DEPTH_OPAQUE);
    draw(land_mask, true, 0, opt_random_grass_lod);
  }

  endRender();

  prepassIsValid = true;
}

#if DAGOR_DBGLEVEL > 0
static inline float4 to_float4(const E3DCOLOR &v) { return float4(v.r / 255.0f, v.g / 255.0f, v.b / 255.0f, v.a / 255.0f); }
#endif

void RandomGrass::renderOpaque(const LandMask &land_mask)
{
  TIME_D3D_PROFILE(grass_opaque);

  if (!shouldRender)
    return;

#if DAGOR_DBGLEVEL > 0
  if (varNotification && varNotification->varChanged)
  {
    Tab<carray<float4, MAX_COLOR_MASKS>> grassLayerData(framemem_ptr());
    grassLayerData.resize(grassDescs.size());
    for (int i = 0; i < grassLayerData.size(); ++i)
      for (int j = 0; j < 6; ++j)
        grassLayerData[i][j] = to_float4(grassDescs[i].colors[j]);
    updateGrassColorLayer(eastl::move(grassLayerData));
    varNotification->varChanged = false;
  }
#endif

  beginRender(land_mask);
  ShaderGlobal::set_int_fast(shaderVars[SHV_RENDER_TYPE], isDissolve ? RENDER_COLOR_DISSOLVE : RENDER_COLOR_OPAQUE);
  if (prepassIsValid && opt_random_grass_lod)
  {
    shaders::overrides::set(afterPrepassOverride);
    draw(land_mask, true, 0, opt_random_grass_lod);
    shaders::overrides::reset();

    ShaderGlobal::set_int_fast(shaderVars[SHV_DEPTH_PREPASS], 1);
    draw(land_mask, true, opt_random_grass_lod, maxLodCount);
  }
  else
  {
    ShaderGlobal::set_int_fast(shaderVars[SHV_DEPTH_PREPASS], 0);
    draw(land_mask, true, 0, maxLodCount);
  }

  endRender();
}


void RandomGrass::renderDebug()
{
  for (unsigned int gridIdx = 0; gridIdx < gridIndices.size(); ++gridIdx)
  {
    const uint8_t grid = gridIndices[gridIdx];

    const int z = (grid >> 4) & 15;
    const int x = (grid >> 0) & 15;

    const BBox3 &box = gridBoxes[z * GRID_SIZE + x];
    draw_debug_box(box, E3DCOLOR(128, 128, 128, 128));
  }
}

void RandomGrass::createCombinedBuffers()
{
  for (unsigned int lodIdx = 0; lodIdx < maxLodCount; ++lodIdx)
  {
    if (!combinedLods[lodIdx].lodIb || !combinedLods[lodIdx].lodVb)
      continue;

    int startVertexPos = 0;
    int startIndexPos = 0;

    typedef uint16_t index_t;

    index_t *ibData = NULL;
    CellVertex *vbData = NULL;

    combinedLods[lodIdx].lodIb->lock(0, 0, &ibData, VBLOCK_WRITEONLY);
    d3d_err(combinedLods[lodIdx].lodVb->lock(0, 0, (void **)&vbData, VBLOCK_WRITEONLY));

    if (ibData && vbData)
    {
      for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
      {
        if (layers[layerIdx]->lods.size() <= lodIdx)
          continue;

        GrassLod &lod = layers[layerIdx]->lods[lodIdx];

        const ShaderMesh::RElem &elem = lod.mesh->getAllElems()[0];

        if (auto *elemIb = elem.vertexData->getIBMem<index_t>(elem.si, elem.numf))
        {
          for (unsigned int indexNo = 0; indexNo < elem.numf * 3; indexNo++)
            ibData[indexNo + startIndexPos] = startVertexPos + elemIb[indexNo] - elem.sv;

          lod.startIndexlocation = startIndexPos;
          startIndexPos += elem.numf * 3;
        }
        lod.maxInstanceSize = 0.5f;
        if (auto *elemVb = elem.vertexData->getVBMem<MeshVertex>(elem.baseVertex, elem.sv, elem.numv))
        {
          for (unsigned int vertexNo = 0; vertexNo < elem.numv; vertexNo++)
            lod.maxInstanceSize = max(lod.maxInstanceSize, elemVb[vertexNo].pos.length());
          // little expand maximum asset size to avoid possible compression errors
          lod.maxInstanceSize *= 1.05;

          for (unsigned int vertexNo = 0; vertexNo < elem.numv; vertexNo++)
          {
            Point3 pos = elemVb[vertexNo].pos;

            PACK_VECTOR_TO_UINT(pos, lod.maxInstanceSize, vbData[startVertexPos + vertexNo].pos.x)

            PACK_TC2_ID_TO_UINT(Point2(saturate(elemVb[vertexNo].texcoord.x), saturate(elemVb[vertexNo].texcoord.y)), layerIdx, 13, 13,
              vbData[startVertexPos + vertexNo].texcoord_drawId.x)
          }

          startVertexPos += elem.numv;
        }
      }
    }
    combinedLods[lodIdx].lodVb->unlock();
    combinedLods[lodIdx].lodIb->unlock();
  }

  for (unsigned int lodIdx = 0; lodIdx < maxLodCount; ++lodIdx)
  {
    int instancesCount = 0;
    for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
    {
      if (layers[layerIdx]->lods.size() <= lodIdx)
        continue;

      instancesCount += layers[layerIdx]->lods[lodIdx].maxInstancesCount;
    }

    combinedLods[lodIdx].instancesCount = instancesCount;
    if (instancesCount > 0)
    {
      {
        String bufferName;
        bufferName.printf(0, "grassInstancesIndirectLod%d", lodIdx);

        combinedLods[lodIdx].grassInstancesIndirect =
          dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, layers.size(), bufferName);
      }
      {
        String bufferName;
        bufferName.printf(0, "grassGenerateIndirectLod%d", lodIdx);

        combinedLods[lodIdx].grassGenerateIndirect = dag::buffers::create_ua_indirect(dag::buffers::Indirect::Dispatch, 1, bufferName);
      }
      {
        String bufferName;
        bufferName.printf(0, "grassInstancesLod%d", lodIdx);

        combinedLods[lodIdx].grassInstances =
          dag::buffers::create_ua_sr_structured(sizeof(RandomGrassInstance), instancesCount, bufferName);
      }
      {
        String bufferName;
        bufferName.printf(0, "grassDispatchCountLod%d", lodIdx);

        combinedLods[lodIdx].grassDispatchCount = dag::buffers::create_ua_sr_byte_address(maxLayerCount, bufferName);
      }
      {
        String bufferName;
        bufferName.printf(0, "grassIndirectParams%d", lodIdx);

        combinedLods[lodIdx].grassIndirectParams =
          dag::buffers::create_ua_sr_structured(sizeof(RandomGrassIndirectParams), maxLayerCount, bufferName);
      }
      {
        String bufferName;
        bufferName.printf(0, "grassInstancesStrideLod%d", lodIdx);
        combinedLods[lodIdx].grassInstancesStride = dag::buffers::create_ua_sr_byte_address(maxLayerCount, bufferName);
      }
      {
        String bufferName;
        bufferName.printf(0, "grassLayersDataLod%d", lodIdx);
        combinedLods[lodIdx].grassLayersData =
          dag::buffers::create_persistent_sr_structured(sizeof(float), 3 * maxLayerCount, bufferName);

        Tab<float> layerData;
        layerData.resize(maxLayerCount * 3);
        mem_set_0(layerData);
        for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
        {
          layerData[layerIdx * 3 + 0] = layers[layerIdx]->lods[lodIdx].numfperinst * 3;
          layerData[layerIdx * 3 + 1] = layers[layerIdx]->lods[lodIdx].startIndexlocation;
          layerData[layerIdx * 3 + 2] = layers[layerIdx]->lods[lodIdx].gpuCellSize;
        }

        void *destData = 0;
        bool ret = combinedLods[lodIdx].grassLayersData->lock(0, 0, &destData, VBLOCK_WRITEONLY);
        d3d_err(ret);
        if (ret && destData)
        {
          memcpy(destData, layerData.data(), sizeof(float) * maxLayerCount * 3);
          combinedLods[lodIdx].grassLayersData->unlock();
        }
      }
    }
  }
}

void RandomGrass::fillLayers(float grass_mul, const DataBlock &params_blk, float min_far_lod_radius)
{
  resetLayers();

  float distanceMul = params_blk.getReal("distance_mul", 1.f);
  float distanceAdd = params_blk.getReal("distance_add", 0.f);

  float densityMul = params_blk.getReal("density_mul", 1.f);
  float densityAdd = params_blk.getReal("density_add", 0.f);

  float distanceVal = grass_mul * distanceMul + distanceAdd;
  float densityVal = grass_mul * densityMul * grassDensityMul + densityAdd;

  if (distanceVal < FLT_MIN)
    distanceVal = 0.f;

  if (densityVal < FLT_MIN)
    densityVal = 0.f;

  curMaxRadius = refMaxRadius * distanceVal;
  curFadeDelta = refFadeDelta * sqrtf(distanceVal);
  curMaxRadius += curFadeDelta;

  if (min_far_lod_radius >= 0.f)
    minFarLodRadius = clamp(min_far_lod_radius, 0.0f, curMaxRadius);

  cellSize = 2.f * curMaxRadius / (GRID_SIZE - 1);
  baseNumInstances = (unsigned int)(curMaxRadius * curMaxRadius * maxDensity * densityVal);

  for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
    layers[layerIdx]->info.resetLayerVB = true;

  resetLayersVB();
}

static inline float4 to_float4(const Color4 &v) { return float4(v.r, v.g, v.b, v.a); }

void RandomGrass::resetLayersVB()
{
  closeTextures();

  int rndSeed = 20100311;

  debug("GRASS: resetLayersVB");
  maxLodCount = 0;
  for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
    maxLodCount = max<int>(maxLodCount, layers[layerIdx]->lods.size());

  maxLayerCount = layers.size();

  carray<int, 4> vertexPos;
  carray<int, 4> indexPos;
  mem_set_0(indexPos);
  mem_set_0(vertexPos);

  deleteBuffers();

  for (unsigned int lodIdx = 0; lodIdx < maxLodCount; ++lodIdx)
  {
    int indexCount = 0;
    int vertexCount = 0;
    for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
    {
      if (layers[layerIdx]->lods.size() <= lodIdx)
        continue;

      const ShaderMesh::RElem &elem = layers[layerIdx]->lods[lodIdx].mesh->getAllElems()[0];
      indexCount += elem.numf * 3;
      vertexCount += elem.numv;
    }
    if (vertexCount > 0)
    {
      GrassLodCombined &combinedLod = combinedLods.push_back();
      String indexBufferName, vertexBufferName;
      indexBufferName.printf(0, "randomGrass::combinedLodIb%d", lodIdx);
      vertexBufferName.printf(0, "randomGrass::combinedLodVb%d", lodIdx);
      combinedLod.lodIb = dag::create_ib(indexCount * sizeof(uint16_t), 0, indexBufferName);
      d3d_err(combinedLod.lodIb.getBuf());
      combinedLod.lodVb = dag::create_vb(vertexCount * sizeof(CellVertex), 0, vertexBufferName);
      d3d_err(combinedLod.lodVb.getBuf());

      static CompiledShaderChannelId chan[] = {{SCTYPE_UINT1, SCUSAGE_POS, 0, 0}, {SCTYPE_UINT1, SCUSAGE_TC, 0, 0}};
      combinedLod.vdecl = dynrender::addShaderVdecl(chan, sizeof(chan) / sizeof(chan[0]));
    }
  }

  int forcedAnisotropy = ::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("forceAlphatestAnisotropy", 0);
  for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
  {
    GrassLayer &layer = *layers[layerIdx];
    if (!layer.info.resetLayerVB)
      continue;

    layer.info.resetLayerVB = false;
    int lodRndSeed = _rnd(rndSeed);

    for (unsigned int lodIdx = 0; lodIdx < layer.lods.size(); ++lodIdx)
    {
      GrassLod &lod = layer.lods[lodIdx];

      TextureIdSet texList;
      texList.reset();
      lod.mesh->gatherUsedTex(texList);

      String bufferName(get_managed_texture_name(texList[0]));
      const char *postfix = bufferName.find('?');
      if (postfix)
        bufferName.chop(bufferName.end(0) - postfix + 1);
      bufferName.append("_a");
      lod.alphaTexId = ::get_tex_gameres(bufferName.str(), false);
      lod.diffuseTexId = texList[0];

      lod.alphaTex = acquire_managed_tex(lod.alphaTexId);
      if (lod.alphaTex && forcedAnisotropy)
        lod.alphaTex->setAnisotropy(forcedAnisotropy);
      lod.diffuseTex = acquire_managed_tex(lod.diffuseTexId);
      if (lod.diffuseTex && forcedAnisotropy)
        lod.diffuseTex->setAnisotropy(forcedAnisotropy);

      fillLayerLod(layer, lodIdx, lodRndSeed);
    }
  }

  createCombinedBuffers();
  grassBufferGenerated = false; // we need shader variant for first generation


  // create and fill constbuffers
  layerDataVS.close();
  layerDataPS.close();

  if (maxLayerCount * maxLodCount <= 0)
    return;

  layerDataVS = dag::buffers::create_persistent_sr_structured(sizeof(GrassLayersVS), maxLayerCount * maxLodCount, "grass_layers_vs");
  layerDataPS = dag::buffers::create_persistent_sr_structured(sizeof(GrassLayersPS), maxLayerCount, "grass_layers_ps");

  if (auto layerData = lock_sbuffer<GrassLayersPS>(layerDataPS.getBuf(), 0, maxLayerCount, VBLOCK_WRITEONLY))
  {
    for (uint32_t layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
    {
      layerData[layerIdx].rStart = to_float4(layers[layerIdx]->info.colors[CHANNEL_RED].start);
      layerData[layerIdx].rEnd = to_float4(layers[layerIdx]->info.colors[CHANNEL_RED].end);
      layerData[layerIdx].gStart = to_float4(layers[layerIdx]->info.colors[CHANNEL_GREEN].start);
      layerData[layerIdx].gEnd = to_float4(layers[layerIdx]->info.colors[CHANNEL_GREEN].end);
      layerData[layerIdx].bStart = to_float4(layers[layerIdx]->info.colors[CHANNEL_BLUE].start);
      layerData[layerIdx].bEnd = to_float4(layers[layerIdx]->info.colors[CHANNEL_BLUE].end);
    }
  }

  if (auto layerData = lock_sbuffer<GrassLayersVS>(layerDataVS.getBuf(), 0, maxLayerCount * maxLodCount, VBLOCK_WRITEONLY))
  {
    for (unsigned int lodNo = 0; lodNo < maxLodCount; lodNo++)
    {
      for (unsigned int layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
      {
        const GrassLayer &layer = *layers[layerIdx];
        if (layer.lods.size() <= lodNo)
          continue;

        const GrassLod &lod = layer.lods[lodNo];

        float fadeIn = max(lod.fadeIn, FLT_EPSILON);
        float fadeOut = max(lod.fadeOut, FLT_EPSILON);

        layerData[maxLayerCount * lodNo + layerIdx].grassLodRange =
          Point4(lod.minRadius, lod.maxRadius, (lod.minRadius + fadeIn) / fadeIn, -(lod.maxRadius - fadeOut) / fadeOut);
        uint64_t mask = layer.info.bitMask;
        layerData[maxLayerCount * lodNo + layerIdx].mask0_31 = mask & 0xFFFFFFFF;
        layerData[maxLayerCount * lodNo + layerIdx].mask32_63 = mask >> 32ULL;

        layerData[maxLayerCount * lodNo + layerIdx].grassInstanceSize = lod.maxInstanceSize;

        layerData[maxLayerCount * lodNo + layerIdx].grassFade = Point4(-1.f / fadeIn, 1.f / fadeOut, 0, 0);

        layerData[maxLayerCount * lodNo + layerIdx].grassScales =
          Point4(layer.info.minScale.x, layer.info.maxScale.x, layer.info.minScale.y, layer.info.maxScale.y);
      }
    }
  }
}

void RandomGrass::setLayerRes(GrassLayer *layer)
{
  const unsigned int lodCount = layer->resource->lods.size();
  for (unsigned int lodIdx = 0; lodIdx < lodCount; ++lodIdx)
  {
    GrassLod &lod = layer->lods.push_back();

    const float lodDensityFactor = lerp(1.f, minDensity, float(lodIdx) / float(max(1U, lodCount - 1)));
    lod.density = layer->info.density * lodDensityFactor;

    lod.mesh = layer->resource->lods[lodIdx].scene->getMesh()->getMesh()->getMesh();

    G_ASSERT(lod.mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest).size() == 1);
    G_ASSERT(lod.mesh->getAllElems().size() == 1);
    G_ASSERT(lod.mesh->getAllElems()[0].vertexData->getStride() == sizeof(MeshVertex));
    G_ASSERT(lod.mesh->getAllElems()[0].numf > 0);
  }
}

GameResource *RandomGrass::loadLayerResource(const char *resName)
{
  GameResource *res = NULL;
  GameResHandle resHandle = GAMERES_HANDLE_FROM_STRING(resName);
  res = get_one_game_resource_ex(resHandle, RndGrassGameResClassId);
  return res;
}

#if DAGOR_DBGLEVEL > 0
void RandomGrass::updateGrassColorLayer(Tab<carray<float4, MAX_COLOR_MASKS>> colors)
{
  if (auto layerData = lock_sbuffer<GrassLayersPS>(layerDataPS.getBuf(), 0, maxLayerCount, VBLOCK_WRITEONLY))
  {
    for (uint32_t layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
    {
      layerData[layerIdx].rStart = colors[layerIdx][0];
      layerData[layerIdx].rEnd = colors[layerIdx][1];
      layerData[layerIdx].gStart = colors[layerIdx][2];
      layerData[layerIdx].gEnd = colors[layerIdx][3];
      layerData[layerIdx].bStart = colors[layerIdx][4];
      layerData[layerIdx].bEnd = colors[layerIdx][5];
    }
  }
}

void RandomGrass::initWebTools()
{
  grassDescs.clear();
  grassDescs.resize(layers.size());

  struct RandomGrassNameCount
  {
    String name;
    int count;
  };

  Tab<RandomGrassNameCount> grassNameCount(framemem_ptr()); // there's some duplication of grass

  for (int i = 0; i < layers.size(); ++i)
  {
    GrassLayer *layer = layers[i];
    int foundIdx = -1;
    for (int j = 0; j < grassNameCount.size(); ++j)
    {
      if (strcmp(grassNameCount[j].name, layer->info.resName.str()) == 0)
      {
        foundIdx = j;
        break;
      }
    }

    if (foundIdx < 0)
    {
      grassNameCount.push_back({layer->info.resName, 0});
      foundIdx = grassNameCount.size() - 1;
    }
    else
    {
      grassNameCount[foundIdx].count++;
      grassNameCount[foundIdx].name = String(layer->info.resName + "_" + grassNameCount[foundIdx].count);
    }

    RandomGrassTypeDesc &layerDesc = grassDescs[i];
    auto readColor = [&](E3DCOLOR &variable, Color4 defaultColor, const char *variable_name) {
      variable = E3DCOLOR((char)(defaultColor.r * 255), (char)(defaultColor.g * 255), (char)(defaultColor.b * 255),
        (char)(defaultColor.a * 255));
      if (varNotification && de3_webui_check_inited())
        varNotification->registerE3dcolor(&variable, grassNameCount[foundIdx].name, variable_name);
    };

    for (int j = 0; j < CHANNEL_COUNT; ++j)
    {
      const char *colorMaskFromName = j == CHANNEL_RED     ? "color_mask_r_from"
                                      : j == CHANNEL_GREEN ? "color_mask_g_from"
                                                           : "color_mask_b_from";
      const char *colorMaskToName = j == CHANNEL_RED ? "color_mask_r_to" : j == CHANNEL_GREEN ? "color_mask_g_to" : "color_mask_b_to";
      E3DCOLOR testColor = E3DCOLOR(layer->info.colors[j].start.r, layer->info.colors[j].start.g, layer->info.colors[j].start.b,
        layer->info.colors[j].start.a);
      debug("testcolor.r = %f", testColor.r);
      readColor(layerDesc.colors[j * 2], layer->info.colors[j].start, colorMaskFromName);
      readColor(layerDesc.colors[j * 2 + 1], layer->info.colors[j].end, colorMaskToName);
    }
  }
  varNotification->varChanged = false;
}
#endif

void RandomGrass::reset_mask_number() {} // legacy


int RandomGrass::addLayer(GrassLayerInfo &layer_info)
{
  if (layer_info.density <= 0)
    return -1;

  GrassLayer *layer = new GrassLayer();

  layer->info = layer_info;
  layer->info.resetLayerVB = true;

  if (layer->info.crossBlend < 0.)
    layer->info.crossBlend = 0.;
  if (layer->info.crossBlend > 1.)
    layer->info.crossBlend = 1.;

  const char *resName = layer_info.resName.str();
  GameResource *res = loadLayerResource(resName);
  layer->resource = (RenderableInstanceLodsResource *)res;

  G_ASSERTF(layer->resource, "Cannot load '%s'", resName);
  if (!layer->resource)
  {
    delete layer;
    return -1;
  }
  G_ASSERTF(layer->resource->isSubOf(RenderableInstanceLodsResourceCID), "'%s' is not a dynres", resName);

  setLayerRes(layer);

  layers.push_back(layer);
  return layers.size() - 1;
}


void RandomGrass::resetLayer(GrassLayer &layer)
{
  for (unsigned int lodIdx = 0; lodIdx < layer.lods.size(); ++lodIdx)
  {
    GrassLod &lod = layer.lods[lodIdx];

    lod.numInstancesInCell = 0;
    lod.maxInstancesCount = 0;
  }
}

void RandomGrass::resetLayers()
{
  for (unsigned int layerNo = 0; layerNo < layers.size(); layerNo++)
  {
    resetLayer(*layers[layerNo]);
  }
}


void RandomGrass::fillLayerLod(GrassLayer &layer, unsigned int lod_idx, int rand_seed)
{
  GrassLod &lod = layer.lods[lod_idx];

  float maxLodRange = layer.resource->getMaxDist();

  const float dissolveRange = layer.info.radiusMul * lodFadeDelta;
  lod.fadeIn = lod_idx == 0 ? 0 : dissolveRange / 2;
  lod.fadeOut = lod_idx < (layer.lods.size() - 1) ? dissolveRange / 2 : layer.info.radiusMul * curFadeDelta;

  lod.minRadius = lod_idx > 0 ? layer.resource->lods[lod_idx - 1].range / maxLodRange : 0.f;
  lod.maxRadius = (lod_idx < (layer.lods.size() - 1)) ? layer.resource->lods[lod_idx].range / maxLodRange : 1;

  lod.minRadius = max(lod.minRadius * layer.info.radiusMul * curMaxRadius - lod.fadeIn, 0.f);
  lod.maxRadius = min(lod.maxRadius * layer.info.radiusMul * curMaxRadius + lod.fadeOut, layer.info.radiusMul * curMaxRadius);

  if (lod_idx == 0)
    lod.maxRadius = max(lod.maxRadius, minFarLodRadius + dissolveRange);
  else
    lod.minRadius = max(lod.minRadius, minFarLodRadius + dissolveRange * 0.5f);

  lod.maxRadius = max(lod.minRadius, lod.maxRadius);

  unsigned int numInstancesInCell = (unsigned int)(baseNumInstances * lod.density * globalDensityMul / (GRID_SIZE * GRID_SIZE) + 0.5f);

  const ShaderMesh::RElem &elem = layer.lods[lod_idx].mesh->getAllElems()[0];

  if ((lod.maxRadius - lod.minRadius) < 1.f)
    numInstancesInCell = 0;

  bool emptyLayer = false;
  if (numInstancesInCell == 0)
  {
    numInstancesInCell = 1;
    emptyLayer = true;
  }

  if (vboForMaxElements)
    // create buffer for max elements
    lod.numInstancesInCell = (unsigned int)MAX_VERTICES_IN_CELL / elem.numv;
  else
    lod.numInstancesInCell = min(numInstancesInCell, (unsigned int)MAX_VERTICES_IN_CELL / elem.numv);

  // if we'll use unclamped numInstancesInCell, grass density on aircraft maps will be too large
  // todo: fix grass density in blk
  lod.gpuCellSize = cellSize / sqrt((float)lod.numInstancesInCell / (float)(GRASS_WARP_SIZE_X * GRASS_WARP_SIZE_Y));

  lod.numInstancesInCell = GRASS_COUNT_PER_INSTANCE;
  int quadSize = ceilf(lod.maxRadius / lod.gpuCellSize);
  lod.warpSize = (quadSize * 2 + 1);
  int maxInstancesInBuffer = lod.warpSize * lod.warpSize * (GRASS_WARP_SIZE_X * GRASS_WARP_SIZE_Y); // we can't see all instances
  lod.maxInstancesCount = maxInstancesInBuffer;
  if (emptyLayer)
  {
    lod.warpSize = 0;
    lod.maxInstancesCount = 0;
  }

  lod.singleVb = false;

  lod.numv = lod.numInstancesInCell * elem.numv * (lod.singleVb ? 1 : VB_GRID_SIZE * VB_GRID_SIZE);
  lod.numf = lod.numInstancesInCell * elem.numf;
  lod.numfperinst = elem.numf;
  lod.numvperinst = elem.numv;

  lod.lodRndSeed = rand_seed;
}


void RandomGrass::fillLodVB(GrassLayer &, int) {}

void RandomGrass::beginRender(const LandMask &land_mask)
{
  float scaleXZ = cellSize * GRID_SIZE / land_mask.getWorldSize();
  float offsetXZ = (1.0f - scaleXZ) * 0.5f;
  Point2 offsetCenter = (land_mask.getCenterPos() - Point2::xy(squareCenter) * cellSize) / land_mask.getWorldSize();

  ShaderGlobal::set_color4_fast(shaderVars[SHV_SCALE_OFFSET], safediv(-1.0f, cellSize * GRID_SIZE), scaleXZ, offsetCenter.y + offsetXZ,
    offsetCenter.x + offsetXZ);
}

void RandomGrass::endRender() {}

void RandomGrass::generateGPUGrass(const LandMask &land_mask, const Frustum &frustum, const Point3 &view_dir, TMatrix4 viewTm,
  TMatrix4 projTm, Point3 view_pos)
{
  G_UNUSED(land_mask);
  TIME_D3D_PROFILE(RandomGrass_generateGPUinstances);

  ShaderGlobal::set_color4(shaderVars[SHV_WORLD_VIEW_POS], view_pos);

  d3d::set_const_buffer(STAGE_CS, 1, 0);
  d3d::set_rwbuffer(STAGE_CS, 5, 0);
  d3d::set_rwbuffer(STAGE_CS, 6, 0);

  set_frustum_planes(frustum);

  viewTm.setrow(3, 0.f, 0.f, 0.f, 1.0f);
  TMatrix4 viewRotProjInv = inverse44(viewTm * projTm);

  Point4 viewVecLT = Point4::xyz0(Point3(-1.f, 1.f, 1.f) * viewRotProjInv);
  Point4 viewVecRT = Point4::xyz0(Point3(1.f, 1.f, 1.f) * viewRotProjInv);
  Point4 viewVecLB = Point4::xyz0(Point3(-1.f, -1.f, 1.f) * viewRotProjInv);
  Point4 viewVecRB = Point4::xyz0(Point3(1.f, -1.f, 1.f) * viewRotProjInv);
  Point4 viewDir = Point4::xyz0(view_dir);

  Point2 view_XZ = normalize(Point2::xz(view_dir));
  static int grass_gen_orderVarId = ::get_shader_variable_id("grass_gen_order");
  ShaderGlobal::set_color4(grass_gen_orderVarId, fabsf(view_XZ.x) > fabsf(view_XZ.y) ? 1 : 0, view_XZ.x < 0 ? 1 : 0,
    view_XZ.y < 0 ? 1 : 0, 0);
  static int view_vecLTVarId = get_shader_variable_id("view_vecLT");
  static int view_vecRTVarId = get_shader_variable_id("view_vecRT");
  static int view_vecLBVarId = get_shader_variable_id("view_vecLB");
  static int view_vecRBVarId = get_shader_variable_id("view_vecRB");
  static int view_dirVarId = get_shader_variable_id("view_dir");

  ShaderGlobal::set_color4(view_vecLTVarId, Color4(&viewVecLT.x));
  ShaderGlobal::set_color4(view_vecRTVarId, Color4(&viewVecRT.x));
  ShaderGlobal::set_color4(view_vecLBVarId, Color4(&viewVecLB.x));
  ShaderGlobal::set_color4(view_vecRBVarId, Color4(&viewVecRB.x));
  ShaderGlobal::set_color4(view_dirVarId, Color4(&viewDir.x));

  static int grass_compress_anchor_pointVarId = get_shader_variable_id("grass_compress_anchor_point");
  Point3 anchorPoint = view_pos / cellSize;
  anchorPoint = cellSize * Point3(floorf(anchorPoint.x), floorf(anchorPoint.y), floorf(anchorPoint.z));
  ShaderGlobal::set_color4(grass_compress_anchor_pointVarId, Point4::xyz0(anchorPoint));

  static int layersCountVarId = ::get_shader_variable_id("layers_count");
  ShaderGlobal::set_int(layersCountVarId, layers.size());


  static int grassGenerationStateVarId = ::get_shader_variable_id("grass_generation_state");
  ShaderGlobal::set_int(grassGenerationStateVarId, grassBufferGenerated ? 1 : 0);

  d3d::set_buffer(STAGE_CS, 8, layerDataVS.getBuf());

  static int random_grass_use_bvhVarId = ::get_shader_variable_id("random_grass_use_bvh", true);
  static int random_grass_bvh_rangeVarId = ::get_shader_variable_id("random_grass_bvh_range", true);
  static int random_grass_bvh_max_countVarId = ::get_shader_variable_id("random_grass_bvh_max_count", true);

  bool useBvhConnection = bvhConnection && bvhConnection->isReady() && bvhConnection->getMaxRange() > 0;
  ShaderGlobal::set_int(random_grass_use_bvhVarId, useBvhConnection ? 1 : 0);
  ShaderGlobal::set_real(random_grass_bvh_rangeVarId, useBvhConnection ? bvhConnection->getMaxRange() : 0);

  if (useBvhConnection && bvhConnection->prepare())
    ShaderGlobal::set_int(random_grass_bvh_max_countVarId,
      bvhConnection->getInstancesBuffer() ? bvhConnection->getInstancesBuffer()->getNumElements() : 0);

  for (unsigned int lodIdx = 0; lodIdx < maxLodCount; ++lodIdx)
  {
    if (combinedLods[lodIdx].instancesCount == 0)
      continue;

    static int maxInstancesVarId = ::get_shader_variable_id("max_instances");
    ShaderGlobal::set_int(maxInstancesVarId, combinedLods[lodIdx].instancesCount);

    Point4 lodLayerNo = Point4(0, lodIdx + 0.5, 0, 0);
    d3d::set_cs_const(47, &lodLayerNo.x, 1);

    static ShaderVariableInfo grassInstancesVarId = ShaderVariableInfo("grass_instances");
    static ShaderVariableInfo grassInstancedBufferVarId = ShaderVariableInfo("grass_instanced_buffer");
    static ShaderVariableInfo grassIndirectParamsVarId = ShaderVariableInfo("grass_indirect_params");
    static ShaderVariableInfo grassGenerateIndirectBufferVarId = ShaderVariableInfo("grass_generate_indirect_buffer");
    static ShaderVariableInfo grassDispatchCountBufferVarId = ShaderVariableInfo("grass_dispatch_count_buffer");
    static ShaderVariableInfo grassStrideBufferVarId = ShaderVariableInfo("grass_stride_buffer");
    static ShaderVariableInfo grassLayerParamsVarId = ShaderVariableInfo("grass_layer_params");

    ShaderGlobal::set_buffer(grassIndirectParamsVarId, combinedLods[lodIdx].grassIndirectParams.getBufId());
    ShaderGlobal::set_buffer(grassInstancedBufferVarId, combinedLods[lodIdx].grassInstancesIndirect.getBufId());
    ShaderGlobal::set_buffer(grassGenerateIndirectBufferVarId, combinedLods[lodIdx].grassGenerateIndirect.getBufId());
    ShaderGlobal::set_buffer(grassDispatchCountBufferVarId, combinedLods[lodIdx].grassDispatchCount.getBufId());
    ShaderGlobal::set_buffer(grassStrideBufferVarId, combinedLods[lodIdx].grassInstancesStride.getBufId());
    ShaderGlobal::set_buffer(grassLayerParamsVarId, combinedLods[lodIdx].grassLayersData.getBufId());

    randomGrassIndirect->dispatch(1, 1, 1); // clear and generate buffers

    ShaderGlobal::set_buffer(grassInstancesVarId, combinedLods[lodIdx].grassInstances.getBufId());

    if (useBvhConnection && bvhConnection->getInstanceCounter() && bvhConnection->getMappingsBuffer())
    {
      d3d::set_rwbuffer(STAGE_CS, 2, bvhConnection->getInstanceCounter().getBuf());
      d3d::set_rwbuffer(STAGE_CS, 3, bvhConnection->getInstancesBuffer().getBuf());
      d3d::set_buffer(STAGE_CS, 12, bvhConnection->getMappingsBuffer().getBuf());
    }

    randomGrassGenerator->dispatch_indirect(combinedLods[lodIdx].grassGenerateIndirect.getBuf(), 0);
  }

  grassBufferGenerated = true;

  if (useBvhConnection)
    bvhConnection->done();
}

void RandomGrass::draw(const LandMask &land_mask, bool opaque, int startLod, int lodCount)
{
  static ShaderVariableInfo grassInstancesVarId = ShaderVariableInfo("grass_instances");
  static ShaderVariableInfo grassStrideBufferVarId = ShaderVariableInfo("grass_stride_buffer");

  G_UNUSED(land_mask);
  ShaderGlobal::set_real(shaderVars[SHV_GRASS_RANGE], curMaxRadius);

  setGrassLodLayerStates();

  ShaderGlobal::set_int(shaderVars[SHV_GRASS_LAYERS_COUNT], maxLayerCount);


  for (unsigned int lodIdx = startLod; lodIdx < lodCount; ++lodIdx)
  {
    ShaderGlobal::set_int_fast(shaderVars[SHV_WIND_STATE], lodIdx == 0 ? 1 : 0);
    ShaderGlobal::set_int_fast(shaderVars[SHV_LOD_NO], lodIdx);

    if (opaque && lodIdx < combinedLods.size())
    {
      if (combinedLods[lodIdx].instancesCount == 0 || !combinedLods[lodIdx].lodIb || !combinedLods[lodIdx].lodVb)
        continue;

      static int maxInstancesVarId = ::get_shader_variable_id("max_instances");
      static int layersCountVarId = ::get_shader_variable_id("layers_count");

      //-1 bias for clamp in shader
      ShaderGlobal::set_int(maxInstancesVarId, combinedLods[lodIdx].instancesCount - 1);
      ShaderGlobal::set_int(layersCountVarId, layers.size() - 1);

      d3d::setind(combinedLods[lodIdx].lodIb.getBuf());
      d3d::setvsrc_ex(0, combinedLods[lodIdx].lodVb.getBuf(), 0, sizeof(CellVertex));

      ShaderGlobal::set_buffer(grassInstancesVarId, combinedLods[lodIdx].grassInstances.getBufId());
      ShaderGlobal::set_buffer(grassStrideBufferVarId, combinedLods[lodIdx].grassInstancesStride.getBufId());

      if (!randomGrassShader.shader->setStates(0, true))
        continue;
      d3d::setvdecl(combinedLods[lodIdx].vdecl);

#if _TARGET_C1

#endif
      for (int i = 0; i < layers.size(); i++)
      {
        mark_managed_tex_lfu(layers[i]->lods[lodIdx].diffuseTexId);
        mark_managed_tex_lfu(layers[i]->lods[lodIdx].alphaTexId);

        if (!check_managed_texture_loaded(layers[i]->lods[lodIdx].alphaTexId) ||
            !check_managed_texture_loaded(layers[i]->lods[lodIdx].diffuseTexId))
          continue;
        d3d::set_tex(STAGE_PS, 8, layers[i]->lods[lodIdx].diffuseTex);
        d3d::set_tex(STAGE_PS, 9, layers[i]->lods[lodIdx].alphaTex);

        d3d::draw_indexed_indirect(PRIM_TRILIST, combinedLods[lodIdx].grassInstancesIndirect.getBuf(), 20 * i);
#if _TARGET_C1

#endif
      }

      // we don't support multidraw for now
#if 0
      d3d::multi_draw_indexed_indirect(PRIM_TRILIST, combinedLods[lodIdx].grassInstancesIndirect, layers.size(), 20);
#endif
    }
  }
}

void RandomGrass::setGrassLodLayerStates()
{
  d3d::set_buffer(STAGE_VS, 9, layerDataVS.getBuf());
  d3d::set_buffer(STAGE_PS, 10, layerDataPS.getBuf());
}
