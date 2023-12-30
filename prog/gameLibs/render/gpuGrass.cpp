#include <render/gpuGrass.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <3d/dag_textureIDHolder.h>
#include <util/dag_convar.h>
#include <util/dag_stlqsort.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_drv3d.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_frustum.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_adjpow2.h>
#include <math/dag_bounds2.h>
#include <math/dag_bounds3.h>

#include <render/scopeRenderTarget.h>
#include <util/dag_bitArray.h>
#include <render/noiseTex.h>
#include <math/random/dag_random.h>
#include <render/toroidal_update.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <webui/editVarNotifications.h>
#include <3d/dag_quadIndexBuffer.h>
#include <shaders/dag_overrideStates.h>

#define USE_QUADS_INDEX_BUFFER 0
#define GRASS_USE_QUADS        D3D_HAS_QUADS // xbox one & ps4

static constexpr int CELLS_AROUND = 4;
enum
{
  MAX_LOD_NO = 2,
  GRASS_LOD_COUNT = MAX_LOD_NO + 1
};

#define GLOBAL_VARS_LIST                                    \
  VAR(grass_pass)                                           \
  VAR(world_to_grass)                                       \
  VAR_OPT(world_to_grass_ofs)                               \
  VAR_OPT(grass_eraser_culling_rect)                        \
  VAR(world_to_grass_position)                              \
  VAR(grass_inv_vis_distance)                               \
  VAR(grass_gen_lod)                                        \
  VAR_OPT(grass_gen_lod_index)                              \
  VAR(grass_gen_order)                                      \
  VAR(grass_grid_params)                                    \
  VAR(grass_draw_instances_buffer)                          \
  VAR(grass_draw_instances_indirect_buffer)                 \
  VAR_OPT(grass_average_ht__ht_extent__avg_hor__hor_extent) \
  VAR_OPT(grass_instancing)

//  VAR(world_to_grass_tex_ofs)

#define VAR_OPT(a) static int a##VarId = -1;
#define VAR(a)     static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR
#undef VAR_OPT

void set_frustum_planes(const Frustum &frustum);

CONSOLE_BOOL_VAL("grass", grassLod, true);
CONSOLE_BOOL_VAL("grass", grassGenerate, true);
CONSOLE_BOOL_VAL("grass", grassGpuSort, false);
// CONSOLE_BOOL_VAL("grass", grassInstancing, false);
CONSOLE_INT_VAL("grass", grassCount, 0, 0, 100000);
CONSOLE_FLOAT_VAL("grass", grassGridSizeMultiplier, 1.0f);


void GPUGrassBase::close()
{
  shaders::overrides::destroy(grassAfterPrepassOverride);
  if (grassGenerator)
    release_64_noise();
  generated = false;
  clear_and_shrink(grasses);
  clear_and_shrink(grassColors);
  grassDescriptions.clear();
  // grassInstancesCountConst.reset(0);
  grassInstancesIndirect.close();
  grassInstances.close();
  createIndirect.reset(0);
  grassGenerator.reset(0);
  grassRandomTypesCB.close();
  grassColorsVSCB.close();
  if (indexBufferInited)
    index_buffer::release_quads_32bit();
  alreadyLoaded = false;
}

static inline float3 srgb_color4(E3DCOLOR ec)
{
  Color3 c = color3(ec);
  c.r = powf(c.r, 2.2f);
  c.g = powf(c.g, 2.2f);
  c.b = powf(c.b, 2.2f);
  return *(float3 *)&c;
}

static float get_quality_multiplier(GrassQuality quality)
{
  switch (quality)
  {
    case GRASS_QUALITY_MINIMUM: return 2.0;
    case GRASS_QUALITY_LOW: return 1.5;
    default: return 1.0;
  }
}

void GPUGrassBase::loadGrassTypes(const DataBlock &grassSettings, const eastl::hash_map<eastl::string, int> &grassTypesUsed)
{
  G_ASSERT(grassDescriptions.empty());
  grasses.resize(GRASS_MAX_TYPES);
  mem_set_0(grasses);
  grassColors.resize(GRASS_MAX_TYPES);
  mem_set_0(grassColors);
  grassDescriptions.resize(GRASS_MAX_TYPES);
  alreadyLoaded = false;

  Tab<const char *> diffuseList, normalList;
  eastl::hash_map<eastl::string, int> diffuse, normal;

  const DataBlock *grassTypesBlock = grassSettings.getBlockByName("grass_types");
  if (!grassTypesBlock)
  {
    G_ASSERTF(0, "there is no grass_types, should be no grass. Not implemented");
    return;
  }

  hor_size_mul = grassSettings.getReal("hor_size_mul", 1);
  hor_size_mul *= get_quality_multiplier(grassQuality);
  height_mul = grassSettings.getReal("height_mul", 1);
  G_ASSERTF(grassTypesUsed.size() <= GRASS_MAX_TYPES + 1, "too many different grass types (%d) required! while maximum %d",
    grassTypesUsed.size() - 1, // -1 because one of it is "nograss"
    GRASS_MAX_TYPES);

  maxGrassHeightSize = 0;
  maxGrassHorSize = 0;
  // for (int i = 0; i < grassTypesBlock->blockCount(); ++i)
  for (eastl::hash_map<eastl::string, int>::const_iterator used = grassTypesUsed.begin(); used != grassTypesUsed.end(); ++used)
  {
    int id = used->second;
    if (id >= GRASS_MAX_TYPES)
      continue;
    const char *grassName = used->first.c_str();
    const DataBlock *grassType = grassTypesBlock->getBlockByName(grassTypesBlock->getNameId(grassName), -1, true); // check if has
                                                                                                                   // duplicates in
                                                                                                                   // grassTypesBlock!

    G_ASSERT(id >= 0);
    G_ASSERT(id < grasses.size());
    if (id < 0 || id > grasses.size())
      continue;
    const char *diffuseName = grassType->getStr("diffuse", NULL);
    const char *normalName = grassType->getStr("normal", NULL);
    int texture_id = -1;
    if (diffuseName == NULL || normalName == NULL)
    {
      G_ASSERT(diffuseName == normalName);
      debug("grass <%s> is no grass", grassName);
    }
    else
    {
      eastl::hash_map<eastl::string, int>::const_iterator diffuseUsed = diffuse.find_as(diffuseName),
                                                          normalUsed = normal.find_as(normalName);
      if (diffuseUsed == diffuse.end())
      {
        G_UNUSED(normalUsed);
        G_ASSERTF(normalUsed == normal.end(),
          "currently diffuse and normal textures supposed to be unique for each grass. check grass <%s> ", grassName);
        diffuse[diffuseName] = texture_id = diffuseList.size();
        diffuseList.push_back(diffuseName);
        normal[normalName] = normalList.size();
        normalList.push_back(normalName);
      }
      else
      {
        G_ASSERTF(normalUsed != normal.end(),
          "currently diffuse and normal textures supposed to be unique for each grass. check grass <%s> ", grassName);
        G_ASSERTF(normalUsed->second == diffuseUsed->second,
          "currently diffuse and normal textures supposed to be unique for each grass. check grass <%s> ", grassName);
        texture_id = diffuseUsed->second;
      }
    }

    GrassTypeDesc &grassDesc = grassDescriptions[id];
    grassDesc.isValid = true;
    grassDesc.name = grassName;
    grassDesc.hasTexture = texture_id >= 0;
    grassDesc.isHorizontal = grassType->getBool("horizontal_grass", false);
    grassDesc.isUnderwater = grassType->getBool("underwater", false);
    grassDesc.isHorizontalValue.value = grassDesc.isHorizontal;
    grassDesc.isUnderwaterValue.value = grassDesc.isUnderwater;

    {
      auto readFloatSlider = [&](EditableFloatHolderSlider &variable, const char *variable_name, float def, float min, float max,
                               float step) {
        variable.value = grassType->getReal(variable_name, def);
        if (varNotification)
          variable.registerValue(varNotification, min, max, step, grassName, variable_name);
      };
      readFloatSlider(grassDesc.height, "height", 0.0f, 0.0f, 3.0f, 0.05f);
      readFloatSlider(grassDesc.size_lod_mul, "size_lod_mul", 1.3f, 0.5f, 2.0f, 0.01f);
      readFloatSlider(grassDesc.size_lod_wide_mul, "size_lod_wide_mul", 1.f, 0.5f, 8.0f, 0.01f);
      readFloatSlider(grassDesc.ht_rnd_add, "ht_rnd_add", 0.0f, 0.0f, 1.0f, 0.05f);
      readFloatSlider(grassDesc.hor_size, "hor_size", 0.25f, 0.0f, 1.0f, 0.05f);
      readFloatSlider(grassDesc.hor_size_rnd_add, "hor_size_rnd_add", 0.1f, 0.0f, 1.0f, 0.05f);
      readFloatSlider(grassDesc.height_from_weight_mul, "height_from_weight_mul", 1.0f, 0.0f, 1.0f, 0.05f);
      readFloatSlider(grassDesc.height_from_weight_add, "height_from_weight_add", 0.0f, 0.0f, 1.0f, 0.05f);
      readFloatSlider(grassDesc.density_from_weight_mul, "density_from_weight_mul", 0.0f, 0.0f, 1.0f, 0.05f);
      readFloatSlider(grassDesc.density_from_weight_add, "density_from_weight_add", 1.0f, 0.0f, 1.0f, 0.05f);
      readFloatSlider(grassDesc.vertical_angle_add, "vertical_angle_add", grassDesc.isHorizontal ? HALFPI : -0.1f, -PI, PI, 0.05f);
      readFloatSlider(grassDesc.vertical_angle_mul, "vertical_angle_mul", grassDesc.isHorizontal ? -0.003f : 0.2f, 0.0f, 1.0f, 0.05f);
    }

    {
      auto readColor = [&](EditableE3dcolorHolderEditbox &variable, const char *variable_name) {
        variable.value = grassType->getE3dcolor(variable_name, 0);
        if (varNotification)
          variable.registerValue(varNotification, grassName, variable_name);
      };
      readColor(grassDesc.colors[0], "color_mask_r_from");
      readColor(grassDesc.colors[1], "color_mask_r_to");
      readColor(grassDesc.colors[2], "color_mask_g_from");
      readColor(grassDesc.colors[3], "color_mask_g_to");
      readColor(grassDesc.colors[4], "color_mask_b_from");
      readColor(grassDesc.colors[5], "color_mask_b_to");
    }

    GrassColorVS &colors = grassColors[id];
    colors.grassTextureType = max(texture_id, 0);
    colors.grassVariations = grassType->getInt("variations", 1);
    grassDesc.stiffness.value = colors.stiffness = clamp(grassType->getReal("stiffness", 1), 0.f, 1.f);
    grassDesc.tile_tc_x.value = colors.tile_tc_x = clamp(grassType->getReal("tile_tc_x", 1), 0.5f, 8.f);
    if (varNotification)
    {
      grassDesc.tile_tc_x.registerValue(varNotification, 0.5, 8, 0.1, grassName, "tile_tc_x");
      grassDesc.stiffness.registerValue(varNotification, 0, 1, 0.002f, grassName, "stiffness");
      grassDesc.isHorizontalValue.registerValue(varNotification, 0, 1, grassName, "horizontal (bool)");
      grassDesc.isUnderwaterValue.registerValue(varNotification, 0, 1, grassName, "underwater (bool)");
    }

    debug("grass <%s> loaded (id=%d variations = %d)", grassName, (int)colors.grassTextureType, (int)colors.grassVariations);
  }
  updateGrassTypes();
  updateGrassColors();

  debug("grass max Ht size = %f max Hor size = %f", maxGrassHeightSize, maxGrassHorSize);

  grassTex = dag::add_managed_array_texture("grass_diffuse_texture*", diffuseList, "grass_texture");
  grassNTex = dag::add_managed_array_texture("grass_normal_texture*", normalList, "grass_texture_n");
  eastl::vector<eastl::string> alphaListStr(diffuseList.size());
  Tab<const char *> alphaList;
  alphaList.resize(diffuseList.size());
  const char *alphaSuffix = grassSettings.getStr("alphaSuffix", ::dgs_get_game_params()->getStr("grassAlphaSuffix", "_alpha"));
  for (int i = 0; i < diffuseList.size(); ++i)
  {
    alphaListStr[i].sprintf("%s%s", diffuseList[i], alphaSuffix);
    alphaList[i] = alphaListStr[i].c_str();
  }
  grassAlphaTex = dag::add_managed_array_texture("grass_alpha_texture*", alphaList, "grass_texture_a");

  // this code is needed only so check_managed_texture_loaded will return true. textures are already clamped
  if (!grassTex) //-V1051
  {
    DAG_FATAL("array texture not created");
    return;
  }
  grassTex->texaddrv(TEXADDR_CLAMP);

  if (!grassAlphaTex)
  {
    DAG_FATAL("array texture not created");
    return;
  }
  grassAlphaTex->texaddrv(TEXADDR_CLAMP);

  grassNTex->texaddrv(TEXADDR_CLAMP);
}

void GPUGrassBase::updateGrassColors()
{
  for (int id = 0; id < grassColors.size(); id++)
  {
    GrassTypeDesc &desc = grassDescriptions[id];
    if (!desc.isValid)
      continue;

    GrassColorVS &colors = grassColors[id];
    colors.mask_r_color0 = srgb_color4(desc.colors[0].value);
    colors.mask_r_color1 = srgb_color4(desc.colors[1].value);
    colors.mask_g_color0 = srgb_color4(desc.colors[2].value);
    colors.mask_g_color1 = srgb_color4(desc.colors[3].value);
    colors.mask_b_color0 = srgb_color4(desc.colors[4].value);
    colors.mask_b_color1 = srgb_color4(desc.colors[5].value);
    colors.stiffness = desc.stiffness.value;
    colors.tile_tc_x = desc.tile_tc_x.value;
  }
  GrassColorVS *destDataVS = 0;
  bool ret = grassColorsVSCB->lock(0, 0, (void **)&destDataVS, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  d3d_err(ret);
  if (!ret || !destDataVS)
    return;
  memcpy(destDataVS, grassColors.data(), min<int>(sizeof(GrassColorVS) * GRASS_MAX_TYPES, data_size(grassColors)));
  grassColorsVSCB->unlock();

  if (varNotification)
    varNotification->varChanged = false;
}

bool GPUGrassBase::setQuality(GrassQuality quality)
{
  float gridSizeMult = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("grassGridSizeMultiplier", 1.0f);
  gridSizeMult *= get_quality_multiplier(quality);
  bool need_invalidate = fabsf(grassGridSizeMultiplier.get() - gridSizeMult) > 1e-6;
  grassGridSizeMultiplier.set(gridSizeMult);
  return need_invalidate;
}

void GPUGrassBase::init(const DataBlock &grassSettings)
{
  if (d3d::get_driver_desc().shaderModel < 5.0_sm)
    return;
#define VAR(a)     a##VarId = get_shader_variable_id(#a);
#define VAR_OPT(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef VAR_OPT
  close();

  createIndirect.reset(new_compute_shader("grass_create_indirect"));
  if (!createIndirect)
    return;
  grassGenerator.reset(new_compute_shader("grass_generate_cs"));
  if (!grassGenerator)
    return;
  init_and_get_argb8_64_noise().setVar();

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_WRITE_DISABLE);
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_EQUAL;
  grassAfterPrepassOverride.reset(shaders::overrides::create(state));

  grassIsSorted = false;

  grassGridSize = grassSettings.getReal("grass_grid_size", 0.5f);
  grassDistance = grassSettings.getReal("grass_distance", 100);

  if (grassSettings.getBlockByName("grassify"))
    extraGrassInstances = true; // Needs more grass instances for this feature

  G_STATIC_ASSERT(sizeof(GrassChannel) % 16 == 0 && sizeof(GrassType) % 16 == 0);
  grassRandomTypesCB = dag::buffers::create_persistent_cb(dag::buffers::cb_struct_reg_count<GrassTypesCB>(), "grass_rnd_types_buf");
  grassColorsVSCB =
    dag::buffers::create_persistent_cb(dag::buffers::cb_array_reg_count<GrassColorVS>(GRASS_MAX_TYPES), "grass_colors_buf");

  indirectSrv = get_shader_variable_id("grass_instances_indirect", true) >= 0;
  grassInstancesIndirect = dag::create_sbuffer(4, 5 + ((GRASS_LOD_COUNT + 3) & ~3),
    (indirectSrv ? SBCF_BIND_SHADER_RES : 0) | SBCF_UA_INDIRECT, 0, "grass_instances_indirect");
  grassRenderer.init("grass_render_billboards", NULL, 0, "grass");
  generated = false;

  const DataBlock *grassTypesBlock = grassSettings.getBlockByName("grass_types");
  if (!grassTypesBlock)
  {
    G_ASSERTF(0, "there is no grass_types, should be no grass. Not implemented");
    return;
  }

  enum
  {
    DECAL_CHANNEL = 3,
    NUM_CHANNELS = DECAL_CHANNEL + 1
  };
  const char *channel_name[NUM_CHANNELS] = {"redMask", "greenMask", "blueMask", "decals"};
  const char *nograss = "nograss";
  int totalGrassTypesUsedCnt = 0;
  eastl::hash_map<eastl::string, int> grassTypesUsed;
  grassTypesUsed[nograss] = 255; // invalid index
  const DataBlock *grassGroup = grassSettings.getBlockByNameEx(channel_name[DECAL_CHANNEL]);
  // debug("grass decals blocks %d", grassGroup->blockCount());

  Bitarray randomGrassesUsed;
  randomGrassesUsed.resize(GRASS_MAX_CHANNELS);
  randomGrassesUsed.reset();
  int decal_channel_first = 0;
  for (; decal_channel_first < DECAL_CHANNEL; ++decal_channel_first)
    if (!grassSettings.getBlockByName(channel_name[decal_channel_first]))
      break;

  for (int i = 0; i < grassGroup->blockCount() + decal_channel_first; i++)
  {
    const DataBlock *grassRandomBlock =
      i < decal_channel_first ? grassSettings.getBlockByName(channel_name[i]) : grassGroup->getBlock(i - decal_channel_first);

    if (i < decal_channel_first && !grassRandomBlock)
    {
      randomGrassesUsed.set(i);
      grassChannels.push_back(); // default
      G_ASSERTF_CONTINUE(0, "default grass channel block <%s> not exist!", channel_name[i]);
    }
    int id = i < decal_channel_first ? i : grassRandomBlock->getInt("id", -1);
    G_ASSERTF_CONTINUE(i < decal_channel_first || id >= 0, "grass channel <%s> id= %d is invalid ", grassRandomBlock->getBlockName(),
      id);
    if (i >= decal_channel_first && decal_channel_first > 0)
      id += decal_channel_first - 1;

    G_ASSERTF_CONTINUE(id < GRASS_MAX_CHANNELS, "too big id = %d at <%s>", id, grassRandomBlock->getBlockName());
    G_ASSERTF_CONTINUE(!randomGrassesUsed[id], "grass with id %d is already initialized (on <%s> grass_type)", id,
      grassRandomBlock->getBlockName());

    randomGrassesUsed.set(id);

    Tab<eastl::pair<const char *, float>> channelGrasses;
    float noGrassWeight = 0;
    {
      eastl::hash_set<const char *> grassesFound;
      for (int i = 0; i < grassRandomBlock->paramCount(); i++)
      {
        if (grassRandomBlock->getParamType(i) != DataBlock::TYPE_REAL)
          continue;
        if (strcmp(grassRandomBlock->getParamName(i), "density_from_weight_mul") == 0 ||
            strcmp(grassRandomBlock->getParamName(i), "density_from_weight_add") == 0)
          continue;
        float weight = grassRandomBlock->getReal(i);
        if (weight <= 0)
          continue;
        const char *grassName = grassRandomBlock->getParamName(i);
        auto it = grassesFound.insert(grassName);
        G_ASSERTF_CONTINUE(it.second == true, "there is duplicated grass <%s> in block <%s>", grassName,
          grassRandomBlock->getBlockName());

        if (!grassTypesBlock->getBlockByName(grassName))
        {
          if (strcmp(grassName, nograss) != 0)
            logerr("%s: grass <%s> not found in grass_types", grassRandomBlock->getBlockName(), grassName);
          noGrassWeight += weight;
        }
        else
        {
          channelGrasses.push_back(eastl::pair<const char *, float>(grassName, weight));
        }
      }
    }

    if (channelGrasses.size() == 0 && noGrassWeight < 1e-5 && grassTypesBlock->getBlockByName(grassRandomBlock->getBlockName())) // old
                                                                                                                                 // way
    {
      channelGrasses.push_back(eastl::pair<const char *, float>(grassRandomBlock->getBlockName(), 1));
    }

    G_ASSERTF(channelGrasses.size() <= MAX_TYPES_PER_CHANNEL,
      "we currently support no more than %d different grassTypes per grass channel. There are %d in <%s>", MAX_TYPES_PER_CHANNEL,
      channelGrasses.size(), grassRandomBlock->getBlockName());

    if (channelGrasses.size() > MAX_TYPES_PER_CHANNEL)
    {
      struct GrassTypeWeighted
      {
        bool operator()(const eastl::pair<const char *, float> &a, const eastl::pair<const char *, float> &b) const
        {
          return a.second > b.second;
        }
      };
      stlsort::sort(channelGrasses.begin(), channelGrasses.end(), GrassTypeWeighted()); // keep most heavy weights
      channelGrasses.resize(MAX_TYPES_PER_CHANNEL);
    }

    float totalWeight = noGrassWeight;
    for (const auto &it : channelGrasses)
    {
      if (grassTypesUsed.find_as(it.first) == grassTypesUsed.end())
      {
        grassTypesUsed[it.first] = totalGrassTypesUsedCnt++;
        debug("grass <%s> became %d type", it.first, grassTypesUsed[it.first]);
      }
      totalWeight += it.second;
    }

    GrassChannel tp;
    tp.density_from_weight_mul = grassRandomBlock->getReal("density_from_weight_mul",
      grassTypesBlock->getBlockByNameEx(grassRandomBlock->getBlockName())->getReal("density_from_weight_mul", 1.f));
    tp.density_from_weight_add = grassRandomBlock->getReal("density_from_weight_add",
      grassTypesBlock->getBlockByNameEx(grassRandomBlock->getBlockName())->getReal("density_from_weight_add", 0.f));

    float sumWeight = 0;
    float randomWeights[MAX_TYPES_PER_CHANNEL] = {0};
    uint8_t randomTypes[MAX_TYPES_PER_CHANNEL] = {0xFF};
    for (int ri = 0; ri < channelGrasses.size() && ri < MAX_TYPES_PER_CHANNEL; ++ri)
    {
      sumWeight += channelGrasses[ri].second;
      randomWeights[ri] = sumWeight / totalWeight;
      randomTypes[ri] = grassTypesUsed[channelGrasses[ri].first];
    }
    tp.random_weights0 = float4(randomWeights[0], randomWeights[1], randomWeights[2], randomWeights[3]);
    tp.random_weights1 = float4(randomWeights[4], randomWeights[5], randomWeights[6], randomWeights[7]);
    // memcpy(&tp.random_weights0.x, randomWeights, MAX_TYPES_PER_CHANNEL*sizeof(float));// implicit, but potentially dangerous
    memcpy(&tp.random_types, randomTypes, MAX_TYPES_PER_CHANNEL * sizeof(uint8_t));

    if (id >= grassChannels.size())
      grassChannels.resize(id + 1);
    grassChannels[id] = tp;
  }

  loadGrassTypes(grassSettings, grassTypesUsed);
  applyAnisotropy();
  updateGrassTypes();
}

void GPUGrassBase::applyAnisotropy()
{
  // intenionally less anisotropy on grass, and only if aniso is more than 4
  int grassAnisotropy = dgs_tex_anisotropy > 4 ? dgs_tex_anisotropy / 2 : 1;

  if (grassTex)
  {
    grassTex->setAnisotropy(grassAnisotropy);
  }
  if (grassAlphaTex)
  {
    grassAlphaTex->setAnisotropy(grassAnisotropy);
  }
  if (grassNTex)
  {
    // intenionally no anisotropy on normalmap
    grassNTex->setAnisotropy(1);
  }
}

void GPUGrassBase::driverReset()
{
  updateGrassColors();
  updateGrassTypes();
  initedInstance = false;
}

void GPUGrassBase::updateGrassTypes()
{
  maxGrassHeightSize = maxGrassHorSize = 0;
  for (int i = 0; i < GRASS_MAX_TYPES; i++)
  {
    GrassTypeDesc &grassDesc = grassDescriptions[i];
    if (!grassDesc.isValid)
      continue;

    GrassType &grass = grasses[i];
    grassDesc.isHorizontal = grassDesc.isHorizontalValue.value;
    grassDesc.isUnderwater = grassDesc.isUnderwaterValue.value;

    grass.height = grassDesc.height.value;
    grass.ht_rnd_add = grassDesc.ht_rnd_add.value;
    grass.hor_size = grassDesc.hor_size.value;
    grass.hor_size_rnd_add = grassDesc.hor_size_rnd_add.value;

    if (!is_equal_float(grass.height_from_weight_mul, grassDesc.height_from_weight_mul.value))
    {
      grass.height_from_weight_mul = grassDesc.height_from_weight_mul.value;
      grassDesc.height_from_weight_add.value = grass.height_from_weight_add = 1.0f - grass.height_from_weight_mul;
    }
    else if (!is_equal_float(grass.height_from_weight_add, grassDesc.height_from_weight_add.value))
    {
      grass.height_from_weight_add = grassDesc.height_from_weight_add.value;
      grassDesc.height_from_weight_mul.value = grass.height_from_weight_mul = 1.0f - grass.height_from_weight_add;
    }

    grass.density_from_weight_mul = grassDesc.density_from_weight_mul.value;
    grass.density_from_weight_add = grassDesc.density_from_weight_add.value;
    grass.vertical_angle_add = grassDesc.vertical_angle_add.value;
    grass.vertical_angle_mul = grassDesc.vertical_angle_mul.value;

    G_ASSERTF(grass.height_from_weight_add >= 0.f && is_equal_float(grass.height_from_weight_mul + grass.height_from_weight_add, 1.0f),
      "height_from_weight_add + height_from_weight_mul should be 1");
    G_ASSERTF(((grass.density_from_weight_mul + grass.density_from_weight_add) > 0.0f || grass.density_from_weight_add > 0.0) &&
                ((grass.density_from_weight_mul + grass.density_from_weight_add) < 1.0f || grass.density_from_weight_add < 1.0f),
      "density_from_weight_add=%f + mask*density_from_weight_mul=%f should be at least sometimes between 0 and 1",
      grass.density_from_weight_add, grass.density_from_weight_mul);

    grass.hor_size *= hor_size_mul;
    grass.hor_size_rnd_add *= hor_size_mul;
    grass.height *= height_mul;
    grass.ht_rnd_add *= height_mul;
    grass.size_lod_mul.x = grassDesc.size_lod_mul.value;
    grass.size_lod_mul.y = grass.size_lod_mul.x * grassDesc.size_lod_wide_mul.value;
    if (grassDesc.isUnderwater)
      grass.size_lod_mul.x = -grass.size_lod_mul.x;
    if (!grassDesc.hasTexture)
    {
      G_ASSERTF(grass.density_from_weight_add + grass.density_from_weight_mul < 0.0001f,
        "there should be density=0 for grass without textures");
      grass.density_from_weight_add = grass.density_from_weight_mul = 0;
    }
    if (grassDesc.isHorizontal)
      grass.height_from_weight_mul = -grass.height_from_weight_mul;

    const float sizeFromWeight = (grass.height_from_weight_add + fabsf(grass.height_from_weight_mul));
    maxGrassHeightSize = max(maxGrassHeightSize, (grass.height + max(grass.ht_rnd_add, 0.f)) * sizeFromWeight);
    maxGrassHorSize = max(maxGrassHorSize, (grass.hor_size + max(grass.hor_size_rnd_add, 0.f)) * sizeFromWeight);
  }
  GrassTypesCB *grassTypesData = 0;
  bool ret = grassRandomTypesCB->lock(0, 0, (void **)&grassTypesData, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  d3d_err(ret);
  if (!ret || !grassTypesData)
    return;
  memset(grassTypesData, 0, sizeof(GrassTypesCB));
  G_ASSERT(data_size(grasses) <= sizeof(grassTypesData->grass_type_params));
  G_ASSERT(data_size(grassChannels) <= sizeof(grassTypesData->grass_channels));
  // G_ASSERT(data_size(grassRGBweights) <= sizeof(grassTypesData->grass_random_types));
  memcpy(grassTypesData->grass_channels, grassChannels.data(), data_size(grassChannels));
  memcpy(grassTypesData->grass_type_params, grasses.data(), data_size(grasses));
  grassRandomTypesCB->unlock();
}

float GPUGrassBase::getGridSizeEffective() const { return grassGridSize * grassGridSizeMultiplier.get(); }
float GPUGrassBase::getDistanceEffective() const
{
  const float grassGridSizeMultiplied = getGridSizeEffective();
  return ceilf(grassDistance / grassGridSizeMultiplied) * grassGridSizeMultiplied;
}
float GPUGrassBase::getDistanceWithBorder() const { return getDistanceEffective() + CELLS_AROUND * getGridSizeEffective(); }
float GPUGrassBase::alignTo() const
{
  const float grassGridSizeMultiplied = getGridSizeEffective();
  return grassLod.get() ? grassGridSizeMultiplied * 4.0f : grassGridSizeMultiplied * 2.0f;
}

namespace
{
static constexpr float nextLodDistMul = 2.5f;
static constexpr float nextLodGridMul = 2.f;
static constexpr float lod0DistancePart = 0.2;
// for first lod we want to hide lods as much as possible. for next lods use 25%
static constexpr float lod1StartPart = 0.5, lodNStartPart = 0.75;
// 5 percent before previous lod start disappear, this one should fully appear
static constexpr float lod1FullPart = 0.7, lodNFullPart = 0.8;
} // namespace

void GPUGrassBase::renderGrassLods(RenderGrassLodCallback renderGrassLod) const
{
  const float grassGridSizeMultiplied = getGridSizeEffective();
  const float grassDistanceMultiplied = getDistanceEffective();

  float currentGridSize = grassGridSizeMultiplied;
  float prevLodDistancePart = lod0DistancePart;
  float currentGrassDistance = grassDistanceMultiplied * prevLodDistancePart;
  // a*d0 + b = 1 //disappear
  // a*d1 + b = 0 //full appear
  // a = 1/(d0-d1);
  // b = -d1/(d0-d1)
  auto calcLinearLod = [](float full_disappear_dist, float full_appear_dist) // mul, add
  { return Point2(1.f / (full_disappear_dist - full_appear_dist), -full_appear_dist / (full_disappear_dist - full_appear_dist)); };

  const float lod0StartDisappearing = 0.75; // 25% for disappearing
  const Point2 disappearTo = calcLinearLod(1.0f, lod0StartDisappearing);
  ShaderGlobal::set_color4(grass_gen_lodVarId, 0, 0, disappearTo.x / currentGrassDistance, disappearTo.y);
  ShaderGlobal::set_real(grass_gen_lod_indexVarId, 0);
  renderGrassLod(currentGridSize, currentGrassDistance);

  // todo: generation can be even faster, if we rely on 2d frustum to cull definetly not visible groups.
  // todo: generation can be even faster, if we rely on 3d frustum to cull not visible groups, using dispatch indirect (storing
  // groups).
  for (int lod = 1, lodCount = grassLod.get() ? MAX_LOD_NO : 1; lod <= lodCount; ++lod)
  {
    // change for new lod
    const float lodStartDistancePart = currentGrassDistance * (lod == 1 ? lod1StartPart : lodNStartPart);
    const float lodFullDistancePart = currentGrassDistance * (lod == 1 ? lod1FullPart : lodNFullPart);
    currentGridSize *= nextLodGridMul;
    prevLodDistancePart = min(1.f, prevLodDistancePart * nextLodDistMul);
    currentGrassDistance = lod == lodCount ? grassDistance : grassDistance * prevLodDistancePart; //?
    // ShaderGlobal::set_color4(grass_gen_lodVarId,
    //   10*lodEquationA/currentGrassDistance, 10*lodEquationB - 9.4, 4./currentGrassDistance, -3);

    const Point2 appearFrom = calcLinearLod(lodStartDistancePart, lodFullDistancePart);
    // 10 percent for disappearing if there is other lod
    const Point2 disappearTo = calcLinearLod(currentGrassDistance, currentGrassDistance * ((lod == lodCount) ? 0.75 : 0.9));
    ShaderGlobal::set_color4(grass_gen_lodVarId, appearFrom.x, appearFrom.y, disappearTo.x, disappearTo.y);
    ShaderGlobal::set_real(grass_gen_lod_indexVarId, lod);
    d3d::resource_barrier({{grassInstancesIndirect.getBuf(), grassInstances.getBuf()}, {RB_NONE, RB_NONE}});
    renderGrassLod(currentGridSize, currentGrassDistance);
    if (currentGrassDistance == grassDistance)
      break;
  }
}

void GPUGrassBase::generateGrass(const Point2 &pos, const Point3 &view_dir, float min_ht_, float max_ht_,
  const frustum_heights_cb_t &cb, GrassPreRenderCallback pre_render_cb)
{
  TIME_D3D_PROFILE(genGrass)

  const float grassGridSizeMultiplied = getGridSizeEffective();
  const float grassDistanceMultiplied = getDistanceEffective();
  const float alignSize = alignTo(); // align to cs grid size
  const float distance = getDistanceWithBorder();
  const Point2 next_pos = floor(pos / (2 * alignSize) + Point2(0.5f, 0.5f)) * (2 * alignSize);
  const float fullDistance = 2 * distance;

  ShaderGlobal::set_color4(world_to_grass_positionVarId, 1.0f / fullDistance, 1.0f / fullDistance, -next_pos.x / fullDistance + 0.5f,
    -next_pos.y / fullDistance + 0.5);


  {
    float currentGridSize = grassGridSizeMultiplied;
    float prevLodDistancePart = lod0DistancePart;
    float currentGrassDistance = grassDistanceMultiplied * prevLodDistancePart;
    int quad0 = int(currentGrassDistance / currentGridSize);
    int instances = min(quad0 * quad0 * 4, (int)ceilf((quad0 + 1) * (quad0 + 1) * PI));
    eastl::array<int, GRASS_LOD_COUNT> lodInstancesCount = {instances, 0, 0};

    // ExtraGrassInstances used for grass in flying islands in ActiveMatter, we can't calculate actual count of
    // extra grass instances we need for this feature, so we just estimate it as 3x more grass instances
    const int grassInstancesMul = extraGrassInstances ? 3 : 1;

    for (int lod = 1, lodCount = grassLod.get() ? MAX_LOD_NO : 1; lod <= lodCount; ++lod)
    {
      currentGridSize *= nextLodGridMul;
      const float lodStartDistancePart = max(0.f, currentGrassDistance * (lod == 1 ? lod1StartPart : lodNStartPart) - currentGridSize);
      prevLodDistancePart = min(1.f, prevLodDistancePart * nextLodDistMul);
      currentGrassDistance = lod == lodCount ? grassDistanceMultiplied : grassDistanceMultiplied * prevLodDistancePart; //?
      const int quad_size = int(currentGrassDistance / currentGridSize);
      int lodInstCount = max(0,
        min(quad_size * quad_size * 4, (int)ceilf((sqr(quad_size + 1) - sqr(floorf(lodStartDistancePart / currentGridSize))) * PI)));

      lodInstCount *= grassInstancesMul;
      lodInstancesCount[lod] = lodInstCount;
      instances += lodInstCount;
      // substract inner circle
    }

    instances = min<int>(MAX_GRASS * grassInstancesMul, instances);

    if (instances != maxInstanceCount)
    {
      maxInstanceCount = instances;
      debug("grass: maxInstanceCount = %d (%d %d %d)", maxInstanceCount, lodInstancesCount[0], lodInstancesCount[1],
        lodInstancesCount[2]);
      grassInstances.close();
      eastl::array<int, GRASS_LOD_COUNT + 1> startsAt;
      if (instances)
      {
        int actualCount = 0;
        for (auto ic : lodInstancesCount)
          actualCount += ic;
        if (actualCount != instances)
          for (auto &ic : lodInstancesCount)
            ic = ic * instances / actualCount;
        startsAt[0] = 0;
        for (size_t i = 1; i < startsAt.size(); ++i)
          startsAt[i] = startsAt[i - 1] + lodInstancesCount[i - 1];
      }
      ShaderGlobal::set_color4(get_shader_variable_id("lod_grass_instances_start_at", true), startsAt[0], startsAt[1], startsAt[2],
        startsAt[3]);
      if (indexBufferInited)
      {
        index_buffer::release_quads_32bit();
        indexBufferInited = false;
      }
#if !GRASS_USE_QUADS
      if (USE_QUADS_INDEX_BUFFER)
      {
        index_buffer::init_quads_32bit(maxInstanceCount);
        indexBufferInited = true;
      }
#endif
      ShaderGlobal::set_int(get_shader_variable_id("grass_use_quads", true), GRASS_USE_QUADS);
      grassInstances = dag::buffers::create_ua_sr_structured(sizeof(GrassInstance), maxInstanceCount, "grass_insts_buf");
    }
  }

  // it is always faster to render without instancing. However, if there are not much instances it can be otherwise
  // ShaderGlobal::set_int(grass_instancingVarId, grassInstancing.get() ? 1 : 0);
  ShaderGlobal::set_int(grass_instancingVarId, 0);

  // wait for deform hmap finish
  if (pre_render_cb)
    pre_render_cb();

  grassIsSorted = false;
  {
    TIME_D3D_PROFILE(generate)
    mat44f globtm;
    d3d::getglobtm(globtm);
    Frustum frustum(globtm);
    set_frustum_planes(frustum);
    Point2 alignedCenterPos = next_pos;
    Point2 view_XZ = normalize(Point2::xz(view_dir));
    ShaderGlobal::set_real(grass_inv_vis_distanceVarId, 1. / grassDistance);
    ShaderGlobal::set_color4(grass_gen_orderVarId, fabsf(view_XZ.x) > fabsf(view_XZ.y) ? 1 : 0, view_XZ.x < 0 ? 1 : 0,
      view_XZ.y < 0 ? 1 : 0, 0);
    ShaderGlobal::set_buffer(grass_draw_instances_indirect_bufferVarId, grassInstancesIndirect.getBufId());
    ShaderGlobal::set_buffer(grass_draw_instances_bufferVarId, grassInstances.getBufId());
    createIndirect->dispatch(1, 1, 1); // clear
    d3d::resource_barrier({{grassInstancesIndirect.getBuf(), grassInstances.getBuf()},
      {RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE}});

    auto renderGrassLod = [&](const float currentGridSize, const float currentGrassDistance) {
      const int quadSize = currentGrassDistance / currentGridSize;
      float min_ht = min_ht_, max_ht = max_ht_;
      cb(BBox2(alignedCenterPos - Point2(currentGrassDistance, currentGrassDistance),
           alignedCenterPos + Point2(currentGrassDistance, currentGrassDistance)),
        min_ht, max_ht);
      ShaderGlobal::set_color4(grass_average_ht__ht_extent__avg_hor__hor_extentVarId,
        0.5 * (max_ht + maxGrassHeightSize + min_ht), // average grass Ht
        0.5 * (max_ht + maxGrassHeightSize - min_ht), // grass Ht extents
        currentGridSize * 0.5,                        // average grid extents
        currentGridSize * 0.5 + maxGrassHorSize       // max hor extents
      );
      ShaderGlobal::set_color4(grass_grid_paramsVarId, alignedCenterPos.x, alignedCenterPos.y, currentGridSize, quadSize);
      grassGenerator->dispatch((2 * quadSize + GRASS_WARP_SIZE_X - 1) / GRASS_WARP_SIZE_X,
        (2 * quadSize + GRASS_WARP_SIZE_Y - 1) / GRASS_WARP_SIZE_Y, 1);
    };

    renderGrassLods(renderGrassLod);
    d3d::set_const_buffer(STAGE_CS, 1, 0);

    d3d::resource_barrier({{grassInstancesIndirect.getBuf(), grassInstances.getBuf()},
      {(indirectSrv ? RB_RO_SRV | RB_STAGE_VERTEX | RB_RO_INDIRECT_BUFFER : RB_RO_INDIRECT_BUFFER), RB_RO_SRV | RB_STAGE_VERTEX}});
  }
  generated = true;
}

void GPUGrassBase::generate(const Point3 &pos, const Point3 &view_dir, const frustum_heights_cb_t &cb,
  GrassPreRenderCallback pre_render_cb)
{
  if (!isInited())
    return;
  generateGrass(Point2::xz(pos), view_dir, pos.y - 1000.f, pos.y + 1000.f, cb, eastl::move(pre_render_cb));
}

void GPUGrassBase::render(RenderType rtype)
{
  if (!generated)
    return;
  if (!alreadyLoaded && !check_managed_texture_loaded(grassAlphaTex.getTexId()))
    return;
  alreadyLoaded = true;
  if (varNotification && varNotification->varChanged)
  {
    updateGrassTypes();
    updateGrassColors();
  }

  TIME_D3D_PROFILE(renderGrass);
  ShaderGlobal::set_int(grass_passVarId, rtype);
  if (rtype == GRASS_AFTER_PREPASS)
    shaders::overrides::set(grassAfterPrepassOverride);
  grassRenderer.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);

#if GRASS_USE_QUADS
  d3d::draw_indirect(PRIM_QUADLIST, grassInstancesIndirect.getBuf(), 0);
#else
  if (indexBufferInited)
  {
    index_buffer::Quads32BitUsageLock lock;
    d3d::draw_indexed_indirect(PRIM_TRILIST, grassInstancesIndirect.getBuf(), 0);
  }
  else
    d3d::draw_indirect(PRIM_TRILIST, grassInstancesIndirect.getBuf(), 0);
#endif
  // d3d::draw(PRIM_TRILIST, 0, min(grassCount.get()>0 ? grassCount.get() : 10000000, grassInstancesCount)*2);
  if (rtype == GRASS_AFTER_PREPASS)
    shaders::overrides::reset();
}

void GPUGrassBase::invalidate() { generated = false; }

GPUGrassBase::~GPUGrassBase() { close(); }
GPUGrassBase::GPUGrassBase() = default;

void GPUGrass::close()
{
  masksTex.close();
  typesTex.close();
  maskTex.close();
  colorTex.close();
  copy_grass_decals.clear();
}

void GPUGrass::init(const DataBlock &grassSettings)
{
  close();
  base.init(grassSettings);
  boxesForInvalidation.reserve(32);

  shaders::OverrideState state;
  state.set(shaders::OverrideState::FLIP_CULL);
  flipCullStateId = shaders::overrides::create(state);

  lastGenPos = Point2(1000000, -100000);
  const char *masksTexName = grassSettings.getStr("grass_masks", NULL);
  if (masksTexName)
  {
    masksTex = dag::get_tex_gameres(masksTexName, "grass_masks");
  }

  const char *typesTexName = grassSettings.getStr("grass_types", NULL);
  if (typesTexName)
  {
    typesTex = dag::get_tex_gameres(typesTexName, "grass_types");
  }
  if (masksTex)
  {
    prefetch_managed_texture(masksTex.getTexId());
    masksTex->texbordercolor(0);
    masksTex->texaddr(TEXADDR_BORDER);
    G_ASSERTF(typesTex, "grass_types not loaded.");
  }

  if (typesTex)
  {
    prefetch_managed_texture(typesTex.getTexId());
    typesTex->texbordercolor(0);
    typesTex->texaddr(TEXADDR_BORDER);
    typesTex->texfilter(TEXFILTER_POINT);
    typesTex->texmipmap(TEXMIPMAP_POINT); // no mips!
    typesTex->texmiplevel(0, 0);          // no mips!
    typesTex->setAnisotropy(1);
    G_ASSERTF(masksTex, "grass_masks not loaded.");
  }
  copy_grass_decals.init("copy_grass_decals");

  maskResolution = grassSettings.getInt("grassMaskResolution", 512);
  maskTexHelper.texSize = maskResolution;
  maskTex.set(d3d::create_tex(NULL, maskResolution, maskResolution, TEXCF_RTARGET, 1, "grass_mask_tex"), "grass_mask_tex");
  colorTex.set(
    d3d::create_tex(NULL, maskResolution, maskResolution, TEXCF_SRGBREAD | TEXCF_SRGBWRITE | TEXCF_RTARGET, 1, "grass_color_tex"),
    "grass_color_tex");
  colorTex.getTex2D()->texfilter(TEXFILTER_POINT);
  colorTex.getTex2D()->texmipmap(TEXMIPMAP_POINT); // no mips!
}

void GPUGrass::setQuality(GrassQuality quality)
{
  if (base.setQuality(quality))
    invalidate();
}

void GPUGrass::applyAnisotropy() { base.applyAnisotropy(); }

void GPUGrass::driverReset()
{
  base.driverReset();
  d3d::resource_barrier({maskTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  maskTexHelper.curOrigin += IPoint2(10000, -100000);
}

void MaskRenderCallback::start(const IPoint2 &)
{
  d3d_get_view_proj(viewproj);
  TMatrix vtm;
  vtm.setcol(0, 1, 0, 0);
  vtm.setcol(1, 0, 0, 1);
  vtm.setcol(2, 0, 1, 0);
  vtm.setcol(3, 0, 0, 0);
  d3d::settm(TM_VIEW, vtm);
}

void MaskRenderCallback::renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom)
{
  if (preRenderCB)
  {
    preRenderCB();
    preRenderCB = nullptr;
  }
  BBox2 box(point2(texelsFrom) * texelSize, point2(texelsFrom + wd) * texelSize);
  BBox3 bbox3(Point3::xVy(box[0], -3000), Point3::xVy(box[1], 3000));
  TMatrix4 proj = matrix_ortho_off_center_lh(bbox3[0].x, bbox3[1].x, bbox3[1].z, bbox3[0].z, bbox3[0].y, bbox3[1].y);
  d3d::settm(TM_PROJ, &proj);

  cm.beginRender(Point3::xVy(box.center(), 0), bbox3, proj);

  d3d::set_render_target(colorTex, 0);
  d3d::setview(lt.x, lt.y, wd.x, wd.y, 0, 1);
  d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  cm.renderColor();

  d3d::set_render_target(maskTex, 0);
  d3d::set_render_target(1, colorTex, 0);
  d3d::setview(lt.x, lt.y, wd.x, wd.y, 0, 1);
  ShaderGlobal::set_color4(grass_eraser_culling_rectVarId, box.center().x, box.center().y, box.width().x / 2, box.width().y / 2);
  if (copy_grass_decals)
    copy_grass_decals->render();
  // d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  cm.renderMask();

  cm.endRender();

  d3d::resource_barrier({maskTex, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  d3d::resource_barrier({colorTex, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
}

void MaskRenderCallback::end() { d3d_set_view_proj(viewproj); }

void GPUGrass::generate(const Point3 &pos, const Point3 &view_dir, IRandomGrassRenderHelper &cb, GrassPreRenderCallback pre_render_cb)
{
  if (!base.isInited())
    return;
  const float grassDistanceMultiplied = base.getDistanceEffective();
  const float alignSize = base.alignTo(); // align to cs grid size
  enum
  {
    TEXEL_ALIGN = 8,
    THRESHOLD = TEXEL_ALIGN * 4
  };
  const float distance = grassDistanceMultiplied + THRESHOLD * alignSize;
  const Point2 next_pos = floor(Point2::xz(pos) / (2 * alignSize) + Point2(0.5f, 0.5f)) * (2 * alignSize);

  Point2 alignedOrigin = next_pos;
  float fullDistance = 2 * distance;
  float texelSize = (fullDistance / maskTexHelper.texSize);
  IPoint2 newTexelsOrigin = (ipoint2(floor(alignedOrigin / (texelSize))) + IPoint2(TEXEL_ALIGN / 2, TEXEL_ALIGN / 2));
  newTexelsOrigin = newTexelsOrigin - (newTexelsOrigin % TEXEL_ALIGN);

  alignedOrigin = point2(maskTexHelper.curOrigin) * texelSize;

  IPoint2 move = abs(maskTexHelper.curOrigin - newTexelsOrigin);
  bool needToInvalidate = boxesForInvalidation.size() > 0;
  bool needToToroidalUpdate = move.x >= THRESHOLD || move.y >= THRESHOLD;

  if (needToToroidalUpdate || needToInvalidate)
  {
    TIME_D3D_PROFILE(grass_mask)
    SCOPE_RENDER_TARGET;
    const float fullUpdateThreshold = 0.45;
    const int fullUpdateThresholdTexels = fullUpdateThreshold * maskTexHelper.texSize;
    if (max(move.x, move.y) < fullUpdateThresholdTexels) // if distance travelled is too big, there is no need to update movement in
                                                         // two steps
    {
      if (move.x < move.y)
        newTexelsOrigin.x = maskTexHelper.curOrigin.x;
      else
        newTexelsOrigin.y = maskTexHelper.curOrigin.y;
    }
    else
    {
      needToInvalidate = false; // We're going to update everything, no invalidate boxes needed
      needToToroidalUpdate = true;
    }

    MaskRenderCallback maskcb(texelSize, cb, maskTex.getTex2D(), colorTex.getTex2D(), &copy_grass_decals, eastl::move(pre_render_cb));

    shaders::overrides::set(flipCullStateId);
    if (needToToroidalUpdate)
      toroidal_update(newTexelsOrigin, maskTexHelper, fullUpdateThresholdTexels, maskcb);
    if (needToInvalidate)
      toroidal_invalidate_boxes(maskTexHelper, texelSize, boxesForInvalidation, maskcb);
    shaders::overrides::reset();
    boxesForInvalidation.clear();

    Point2 ofs = point2((maskTexHelper.mainOrigin - maskTexHelper.curOrigin) % maskTexHelper.texSize) / maskTexHelper.texSize;
    alignedOrigin = point2(maskTexHelper.curOrigin) * texelSize;
    // ShaderGlobal::set_color4(world_to_grass_tex_ofsVarId,ofs.x, ofs.y,0,0);
    // ShaderGlobal::set_color4(world_to_grassVarId,
    //   1.0f/fullDistance, 1.0f/fullDistance, -alignedOrigin.x/fullDistance+0.5, -alignedOrigin.y/fullDistance+0.5);

    ShaderGlobal::set_color4(world_to_grassVarId, 1.0f / fullDistance, 1.0f / fullDistance,
      -alignedOrigin.x / fullDistance + 0.5f - ofs.x, -alignedOrigin.y / fullDistance + 0.5 - ofs.y);

    if (world_to_grass_ofsVarId >= 0)
      ShaderGlobal::set_color4(world_to_grass_ofsVarId, 1.0f / fullDistance, 1.0f / fullDistance,
        -alignedOrigin.x / fullDistance + 0.5f, -alignedOrigin.y / fullDistance + 0.5);

    colorTex.setVar();
    maskTex.setVar();
  }
  base.generate(
    pos, view_dir, [&](const BBox2 &box, float &mn, float &mx) { return cb.getMinMaxHt(box, mn, mx); }, eastl::move(pre_render_cb));
}

void GPUGrass::render(RenderType rtype) { base.render((GPUGrassBase::RenderType)rtype); }

void GPUGrass::invalidate(bool regenerate)
{
  maskTexHelper.curOrigin.x = -1000000;
  maskTexHelper.curOrigin.y = 100000;
  if (regenerate)
    base.invalidate();
}

void GPUGrass::invalidateBoxes(const dag::ConstSpan<BBox2> &boxes)
{
  boxesForInvalidation.insert(boxesForInvalidation.end(), boxes.begin(), boxes.end());
}

void GPUGrass::invalidateBoxes(const dag::ConstSpan<BBox3> &boxes)
{
  for (auto &box : boxes)
    boxesForInvalidation.emplace_back(Point2::xz(box.boxMin()), Point2::xz(box.boxMax()));
}

GrassQuality str_to_grass_quality(const char *quality_str)
{
  eastl::string_view quality = quality_str;
  if (quality == "minimum")
    return GrassQuality::GRASS_QUALITY_MINIMUM;
  else if (quality == "low")
    return GrassQuality::GRASS_QUALITY_LOW;
  else if (quality == "high")
    return GrassQuality::GRASS_QUALITY_HIGH;
  logerr("grass: can't convert str(`%s`) to quality", quality_str);

  return GrassQuality::GRASS_QUALITY_HIGH;
}
GPUGrass::~GPUGrass() { close(); }
GPUGrass::GPUGrass() = default;
