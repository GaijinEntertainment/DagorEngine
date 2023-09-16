#include <generic/dag_tabWithLock.h>
#include <render/clipmapDecals.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_range.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <3d/dag_materialData.h>
#include <math/dag_bounds2.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <gameRes/dag_gameResources.h>
#include <perfMon/dag_statDrv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <generic/dag_staticTab.h>
#include <memory/dag_framemem.h>
#include <math/dag_bounds2.h>
#include <3d/dag_quadIndexBuffer.h>
#include <util/dag_console.h>

static const char *clipmap_decals_config_filename = "gamedata/clipmap_decals.blk";

ClipmapDecals::ClipmapDecals()
{
  mem_set_0(instData);
  cameraPosition = Point3(0, 0, 0);
}

ClipmapDecals::~ClipmapDecals()
{
  for (int i = 0; i < decalTypes.size(); i++)
  {
    if (!decalTypes[i]->arrayTextures && isRenderingSetup)
    {
      release_managed_tex(decalTypes[i]->dtex);
      release_managed_tex(decalTypes[i]->ntex);
      release_managed_tex(decalTypes[i]->mtex);
    }

    decalTypes[i]->dtex = BAD_TEXTUREID;
    decalTypes[i]->ntex = BAD_TEXTUREID;
    decalTypes[i]->mtex = BAD_TEXTUREID;

    decalTypes[i]->material.close();
    decalTypes[i]->decalDataVS.close();

    clear_and_shrink(decalTypes[i]->diffuseList);
    clear_and_shrink(decalTypes[i]->normalList);

    decalTypes[i]->diffuseArray.close();
    decalTypes[i]->normalArray.close();

    del_it(decalTypes[i]);
  }
  if (inited)
    release();
}

void ClipmapDecals::init(bool stub_render_mode, const char *shader_name)
{
  stubMode = stub_render_mode;
  defaultMaterial = shader_name;

  typesCount = 0;

  index_buffer::init_quads_32bit(MAX_DECALS_PER_TYPE);

  inited = true;

  if (stubMode)
    return;

  alphaHeightScaleVarId = ::get_shader_variable_id("clipmap_decals_alpha_height_scale", true);
  decalParametersVarId = ::get_shader_variable_id("clipmap_decals_parameters", true);
  useDisplacementMaskVarId = ::get_shader_variable_id("clipmap_decal_use_displacement_mask", true);
  displacementMaskVarId = ::get_shader_variable_id("clipmap_decal_displacement_mask", true);
  displacementFalloffRadiusVarId = ::get_shader_variable_id("clipmap_decals_displacement_falloff_radius", true);

  if (dd_file_exists(clipmap_decals_config_filename))
  {
    DataBlock paramsBlk(clipmap_decals_config_filename);
    initDecalTypes(paramsBlk);
  }
}

// release system
void ClipmapDecals::release() { index_buffer::release_quads_32bit(); }

// remove all
void ClipmapDecals::clear()
{
  for (int i = 0; i < typesCount; i++)
  {
    mem_set_0(decalTypes[i]->decals);
    decalTypes[i]->newDecal = 0;
    decalTypes[i]->activeCount = 0;
  }
}

void ClipmapDecals::initDecalTypes(const DataBlock &blk)
{
  const DataBlock *decalTypesConfig = blk.getBlockByNameEx("decalTypes");
  for (uint32_t i = 0, n = decalTypesConfig->blockCount(); i < n; i++)
  {
    const DataBlock &decalTypeParams = *decalTypesConfig->getBlock(i);
    const char *decalTypeName = decalTypeParams.getBlockName();
    int decalTypeId;
    bool updateOnly = false;
    int decalsCount = decalTypeParams.getInt("decalsCount", 1000);
    auto iter = decalTypesByName.find(decalTypeName);
    if (iter != decalTypesByName.end())
    {
      updateOnly = true;
      decalTypeId = iter->second;
    }
    else
    {
      decalTypes.push_back();
      int newDecalTypeId = decalTypes.size() - 1;
      decalTypes[newDecalTypeId] = new ClipmapDecalType(decalsCount);
      decalTypesByName[decalTypeName] = newDecalTypeId;
      decalTypeId = newDecalTypeId;
    }
    ClipmapDecalType &decalType = *decalTypes[decalTypeId];
    const char *diffuseTex = decalTypeParams.getStr("diffuse_tex", nullptr);
    const char *normalTex = decalTypeParams.getStr("normal_tex", nullptr);
    if (diffuseTex)
    {
      decalType.diffuseTex = dag::get_tex_gameres(diffuseTex);
      decalType.dtex = decalType.diffuseTex.getTexId();
    }
    if (normalTex)
    {
      decalType.normalTex = dag::get_tex_gameres(normalTex);
      decalType.ntex = decalType.normalTex.getTexId();
    }
    decalType.alphaThreshold = decalTypeParams.getInt("alpha_threshold", 255) / 255.0;
    decalType.displacementMin = decalTypeParams.getReal("displacementMin", 0.0);
    decalType.displacementMax = decalTypeParams.getReal("displacementMax", 0.0);
    decalType.displacementScale = decalTypeParams.getReal("displacementScale", 1.0);
    decalType.reflectance = decalTypeParams.getReal("reflectance", 0.5);
    decalType.ao = decalTypeParams.getReal("ao", 1.0);
    decalType.microdetail = decalTypeParams.getInt("microdetail", 0) / 255.0;
    decalType.displacementFalloffRadius = decalTypeParams.getReal("displacementFalloffRadius", -1.0);
    decalType.sizeScale = decalTypeParams.getReal("sizeScale", 1.0);
    if (decalType.activeCount)
      decalType.needUpdate = true;
    if (decalTypeParams.getBool("displacementMask", false))
    {
      String bufName;
      bufName.printf(0, "decal_displacement_mask_type%d", decalTypeId);
      decalType.displacementMaskBuf = dag::create_sbuffer(4, decalsCount * sizeof(DisplacementMask) / 4,
        SBCF_DYNAMIC | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES | SBCF_MAYBELOST, 0, bufName.c_str());
      decalType.displacementMasks.resize(decalsCount);
      decalType.displacementMaskSizes.resize(decalsCount);
    }
    if (!updateOnly)
    {
      createDecalType(decalType.diffuseTex.getTexId(), decalType.normalTex.getTexId(), BAD_TEXTUREID, decalsCount, nullptr, false,
        decalTypeId);
      createDecalSubType(decalTypeId, Point4(0.0, 0.0, 1.0, 1.0), 1, nullptr, nullptr, nullptr);
    }
  }
}

int ClipmapDecals::createDecalType(TEXTUREID d_tex_id, TEXTUREID n_tex_id, TEXTUREID m_tex_id, int decals_count,
  const char *shader_name, bool use_array_textures, int existing_decal_type)
{
  decals_count = min<int>(MAX_DECALS_PER_TYPE, decals_count);
  int newDecalType = existing_decal_type;
  if (existing_decal_type < 0 || existing_decal_type >= decalTypes.size())
  {
    decalTypes.push_back();
    newDecalType = decalTypes.size() - 1;
    decalTypes[newDecalType] = new ClipmapDecalType(decals_count);
  }
  ClipmapDecalType &decalType = *decalTypes[newDecalType];

  decalType.arrayTextures = use_array_textures;
  decalType.needUpdate = false;

  decalType.newDecal = 0;
  decalType.material.close();
  if (shader_name != NULL)
    decalType.material.init(shader_name, NULL, 0, "clipmap decal shader");
  else
    decalType.material.init(defaultMaterial, NULL, 0, "clipmap decal shader");

  String bufferName;
  bufferName.printf(0, "decal_data_vs_type%d", newDecalType);

  decalType.decalDataVS = dag::create_sbuffer(sizeof(Point4), decals_count * sizeof(InstData) / sizeof(Point4),
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_ZEROMEM, TEXFMT_A32B32G32R32F, bufferName);

  decalType.dtex = d_tex_id;
  decalType.ntex = n_tex_id;
  decalType.mtex = m_tex_id;

  unsigned writeAlbedo = d_tex_id != BAD_TEXTUREID ? (WRITEMASK_RED0 | WRITEMASK_GREEN0 | WRITEMASK_BLUE0) : 0;
  unsigned writeNormal = n_tex_id != BAD_TEXTUREID ? (WRITEMASK_RED1 | WRITEMASK_GREEN1) : 0;
  unsigned writeMaterial = m_tex_id != BAD_TEXTUREID ? (WRITEMASK_RED2 | WRITEMASK_GREEN2 | WRITEMASK_BLUE2) : 0;

  shaders::OverrideState state;
  state.colorWr = writeAlbedo | writeNormal | writeMaterial;
  decalType.decalOverride = shaders::overrides::create(state);

  typesCount = newDecalType + 1;
  return typesCount - 1;
}

int ClipmapDecals::createDecalSubType(int decal_type, Point4 tc, int random_count, const char *diffuse_name, const char *normal_name,
  const char *material_name)
{
  G_UNUSED(material_name);
  int retSubTYpe = 0;
  ClipmapDecalType &decalType = *decalTypes[decal_type];
  if (!decalType.arrayTextures)
  {
    decalType.subtypes.push_back();
    int decalSubtype = decalType.subtypes.size() - 1;
    decalType.subtypes[decalSubtype].tc = tc;
    decalType.subtypes[decalSubtype].variants = random_count;
    retSubTYpe = decalSubtype;
  }
  else
  {
    if (decalType.dtex != BAD_TEXTUREID)
    {
      if (diffuse_name == NULL)
      {
        debug("empty decal suptype!");
        return -1;
      }
      for (int i = 0; i < decalType.diffuseList.size(); i++)
      {
        if (strcmp(diffuse_name, decalType.diffuseList[i]) == 0)
        {
          // this subtype already created
          return i;
        }
      }
    }
    if (decalType.ntex != BAD_TEXTUREID)
    {
      if (normal_name == NULL)
      {
        debug("empty decal suptype!");
        return -1;
      }
      for (int i = 0; i < decalType.normalList.size(); i++)
      {
        if (strcmp(normal_name, decalType.normalList[i]) == 0)
        {
          // this subtype already created
          return i;
        }
      }
    }
    decalType.subtypes.push_back();
    int decalSubtype = decalType.subtypes.size() - 1;

    retSubTYpe = decalSubtype;

    decalType.subtypes[decalSubtype].tc = Point4(0, 0, 1, 1);
    decalType.subtypes[decalSubtype].variants = random_count;

    if (decalType.dtex != BAD_TEXTUREID)
    {
      decalType.diffuseList.push_back(String(diffuse_name));
      decalType.diffuseArray.close();

      Tab<const char *> arrnames;
      for (int i = 0; i < decalType.diffuseList.size(); i++)
      {
        arrnames.push_back(decalType.diffuseList[i]);
      }
      String texname;
      texname.printf(0, "decal_diffuse_texture%d", decal_type);
      decalType.diffuseArray = dag::add_managed_array_texture(texname, arrnames);
      if (!decalType.diffuseArray)
      {
        fatal("array texture not created");
      }
    }
    if (decalType.ntex != BAD_TEXTUREID)
    {
      decalType.normalList.push_back(String(normal_name));
      decalType.normalArray.close();

      Tab<const char *> arrnames;
      for (int i = 0; i < decalType.normalList.size(); i++)
      {
        arrnames.push_back(decalType.normalList[i]);
      }
      String texname;
      texname.printf(0, "decal_normal_texture%d", decal_type);
      decalType.normalArray = dag::add_managed_array_texture(texname, arrnames);
      if (!decalType.normalArray)
      {
        fatal("array texture not created");
      }
    }
  }
  return retSubTYpe;
}

void ClipmapDecals::afterReset()
{
  for (int i = 0; i < typesCount; i++)
    decalTypes[i]->needUpdate = true;
}

void ClipmapDecals::setupRendering()
{
  if (isRenderingSetup)
    return;
  isRenderingSetup = true;

  for (int i = 0; i < typesCount; i++)
  {
    ClipmapDecalType &decalType = *decalTypes[i];
    if (!decalType.arrayTextures)
    {
      if (decalType.dtex != BAD_TEXTUREID)
        acquire_managed_tex(decalType.dtex);
      if (decalType.ntex != BAD_TEXTUREID)
        acquire_managed_tex(decalType.ntex);
      if (decalType.mtex != BAD_TEXTUREID)
        acquire_managed_tex(decalType.mtex);
    }
  }
}

bool ClipmapDecals::checkDecalTexturesLoaded()
{
  bool loaded = true;
  for (const ClipmapDecalType *decalType : decalTypes)
  {
    if (!decalType->arrayTextures)
    {
      if (decalType->dtex != BAD_TEXTUREID)
        loaded &= prefetch_and_check_managed_texture_loaded(decalType->dtex);
      if (decalType->ntex != BAD_TEXTUREID)
        loaded &= prefetch_and_check_managed_texture_loaded(decalType->ntex);
      if (decalType->mtex != BAD_TEXTUREID)
        loaded &= prefetch_and_check_managed_texture_loaded(decalType->mtex);
    }
  }
  return loaded;
}

void ClipmapDecals::createDecal(int decalType, const Point2 &pos, float rot, const Point2 &size, int subtype, bool mirror,
  uint64_t displacement_mask, int displacement_mask_size)
{
  ClipmapDecalType &type = *decalTypes[decalType];

  if (type.arrayTextures && subtype < 0)
    return;

  type.activeCount = min<int>(type.activeCount + 1, type.decals.size());

  if (type.newDecal >= type.decals.size())
  {
    type.newDecal = 0; // use oldest decal
  }

  if (type.displacementMaskBuf.getBuf())
  {
    if (displacement_mask_size <= 0 || displacement_mask_size > 8)
    {
      LOGERR_ONCE("Invalid displacement_mask_size %d", displacement_mask_size);
      displacement_mask_size = 1;
    }
    type.displacementMasks[type.newDecal].lowBits = displacement_mask & 0xffffffff;
    type.displacementMasks[type.newDecal].highBits = displacement_mask >> 32;
    type.displacementMaskSizes[type.newDecal] = displacement_mask_size;
  }

  ClipmapDecal &decal = type.decals[type.newDecal];

  decal.pos = pos;
  decal.localX = size.x * Point2(cos(rot), sin(rot));
  decal.localY = size.y * Point2(-sin(rot), cos(rot));
  decal.lifetime = 100;

  // bbox for clipmap update
  Point2 cornersOfs = Point2(abs(decal.localX.x) + abs(decal.localY.x), abs(decal.localX.y) + abs(decal.localY.y));
  BBox2 box(decal.pos - type.sizeScale * cornersOfs, decal.pos + type.sizeScale * cornersOfs);

  Point4 processedTc = type.subtypes[subtype].tc;
  Point4 baseTc = type.subtypes[subtype].tc;
  // choose random seed of decal
  {
    float start = (float)rnd_int(0, type.subtypes[subtype].variants - 1);
    float imgScale = 1.0f / ((float)type.subtypes[subtype].variants);
    processedTc.x = baseTc.x + start * imgScale * (baseTc.z - baseTc.x);
    processedTc.z = baseTc.x + (start + 1.0f) * imgScale * (baseTc.z - baseTc.x);
    if (mirror)
    {
      float t = processedTc.x;
      processedTc.x = processedTc.z;
      processedTc.z = t;
    }
  }
  decal.tc = processedTc;

  if (type.arrayTextures)
    decal.tc.y = subtype;

  float square = (box[1].x - box[0].x) * (box[1].y - box[0].y);
  float treshold = delayedRegionSizeFactor * length(pos - Point2(cameraPosition.x, cameraPosition.z));
  treshold *= treshold;
  // too large decal
  if (square > treshold && delayed_regions.size() < maxDelayedRegionsCount && useDelayedRegions)
  {
    BBox2 delayedBox;
    // extremely large decal
    if (square > 8 * treshold)
    {
      delayedBox = box;
      delayedBox[1].x = (box[0].x + box[1].x) * 0.5;
      delayedBox[1].y = (box[0].y + box[1].y) * 0.5;
      delayed_regions.push_back(delayedBox);

      delayedBox[0].x = (box[0].x + box[1].x) * 0.5;
      delayedBox[1].x = box[1].x;
      delayed_regions.push_back(delayedBox);

      // shrink original box to half
      box[0].y = (box[0].y + box[1].y) * 0.5;
    }
    delayedBox = box;
    delayedBox[1].x = (box[0].x + box[1].x) * 0.5;
    delayed_regions.push_back(delayedBox);
    box[0].x = (box[0].x + box[1].x) * 0.5;
  }

  updated_regions.push_back(box);
  type.newDecal++;
  type.needUpdate = true;
}

void ClipmapDecals::updateBuffers(int type_no)
{
  ClipmapDecalType &type = *decalTypes[type_no];

  if (!type.needUpdate)
    return;

  Tab<InstData> data;
  data.resize(type.activeCount);

  void *destData = 0;

  bool useDisplacementMask = type.displacementMaskBuf.getBuf() != nullptr;

  // lock and rewrite all buffer contents
  if (!type.decalDataVS.getBuf()->lock(0, 0, &destData, VBLOCK_WRITEONLY))
  {
    debug("%s can't lock buffer for decals", d3d::get_last_error());
    return;
  }

  for (int i = 0; i < type.activeCount; i++)
  {
    data[i].localX = type.decals[i].localX;
    data[i].localY = type.decals[i].localY;
    data[i].tc = type.decals[i].tc;
    data[i].pos = Point4(type.decals[i].pos.x, type.decals[i].pos.y, 0, 0);
    if (useDisplacementMask)
      data[i].pos.z = type.displacementMaskSizes[i];
  }

  memcpy(destData, data.data(), sizeof(InstData) * type.activeCount);

  type.decalDataVS.getBuf()->unlock();

  if (useDisplacementMask)
  {
    type.displacementMaskBuf.getBuf()->updateData(0, sizeof(type.displacementMasks[0]) * type.activeCount,
      type.displacementMasks.data(), VBLOCK_DISCARD);
  }

  type.needUpdate = false;
}

void ClipmapDecals::renderByCount(int count) { d3d::drawind(PRIM_TRILIST, 0, count * 2, 0); }

void ClipmapDecals::render(bool override_writemask)
{
  TIME_D3D_PROFILE(clipmap_decals);

  static int diffuseTexVar = ::get_shader_variable_id("clipmap_decal_diffuse_tex", true);
  static int normalTexVar = ::get_shader_variable_id("clipmap_decal_normal_tex", true);
  static int materialTexVar = ::get_shader_variable_id("clipmap_decal_material_tex", true);
  static int useArrayTexVar = ::get_shader_variable_id("clipmap_decal_array_tex", true);

  d3d::setvsrc_ex(0, NULL, 0, 0);
  index_buffer::Quads32BitUsageLock lock;

  // render
  shaders::OverrideStateId savedStateId = shaders::overrides::get_current();
  for (int i = 0; i < typesCount; i++)
  {
    ClipmapDecalType &decalType = *decalTypes[i];
    if (decalType.activeCount > 0)
    {
      updateBuffers(i);
      ShaderGlobal::set_int(useArrayTexVar, decalType.arrayTextures ? 1 : 0);
      ShaderGlobal::set_color4(alphaHeightScaleVarId, decalType.alphaThreshold, decalType.displacementMin, decalType.displacementMax,
        decalType.displacementScale);
      ShaderGlobal::set_color4(decalParametersVarId, decalType.microdetail, decalType.ao, decalType.reflectance, decalType.sizeScale);
      ShaderGlobal::set_real(displacementFalloffRadiusVarId, decalType.displacementFalloffRadius);
      ShaderGlobal::set_int(useDisplacementMaskVarId, decalType.displacementMaskBuf.getBuf() != nullptr);
      ShaderGlobal::set_buffer(displacementMaskVarId, decalType.displacementMaskBuf.getBufId());

      if (decalType.arrayTextures)
      {
        ShaderGlobal::set_texture(diffuseTexVar,
          (decalType.dtex != BAD_TEXTUREID) ? decalType.diffuseArray.getTexId() : BAD_TEXTUREID);
        ShaderGlobal::set_texture(normalTexVar, (decalType.ntex != BAD_TEXTUREID) ? decalType.normalArray.getTexId() : BAD_TEXTUREID);
      }
      else
      {
        if (override_writemask)
        {
          shaders::overrides::reset();
          shaders::overrides::set(decalType.decalOverride);
        }

        ShaderGlobal::set_texture(diffuseTexVar, decalType.dtex);
        ShaderGlobal::set_texture(normalTexVar, decalType.ntex);
        ShaderGlobal::set_texture(materialTexVar, decalType.mtex);
      }
      d3d::set_buffer(STAGE_VS, 8, decalType.decalDataVS.getBuf());

      if (!decalType.material.shader->setStates(0, true))
        continue;

      renderByCount(decalType.activeCount);
    }
    if (override_writemask)
    {
      shaders::overrides::reset();
      shaders::overrides::set(savedStateId);
    }
  }
  ShaderGlobal::set_texture(diffuseTexVar, BAD_TEXTUREID);
  ShaderGlobal::set_texture(normalTexVar, BAD_TEXTUREID);
  ShaderGlobal::set_texture(materialTexVar, BAD_TEXTUREID);
}

const Tab<BBox2> &ClipmapDecals::get_updated_regions() { return updated_regions; }

void ClipmapDecals::clear_updated_regions()
{
  updated_regions.clear();
  // add delayed regions to next frame update
  if (delayed_regions.size() > 0)
  {
    updated_regions.push_back(delayed_regions[0]);
    erase_items(delayed_regions, 0, 1);
  }
}

int ClipmapDecals::getDecalTypeIdByName(const char *decal_type_name)
{
  auto iter = decalTypesByName.find(decal_type_name);
  return iter != decalTypesByName.end() ? iter->second : -1;
}

namespace clipmap_decals_mgr
{
static eastl::unique_ptr<ClipmapDecals> clipmapDecals;
void init()
{
  if (!clipmapDecals)
    clipmapDecals = eastl::make_unique<ClipmapDecals>();
  bool isStub = false;
  clipmapDecals->init(isStub, "clipmap_decal_base");
}

// release system
void release() { clipmapDecals.reset(); }
// remove all decals from map
void clear()
{
  if (clipmapDecals)
    clipmapDecals->clear();
}

int getDecalTypeIdByName(const char *decal_type_name)
{
  return clipmapDecals ? clipmapDecals->getDecalTypeIdByName(decal_type_name) : -1;
}

// if array, positive values of d_tex_id and n_tex_id means creation
// of diffuse and normal arrays
int createDecalType(TEXTUREID d_tex_id, TEXTUREID n_tex_id, TEXTUREID m_tex_id, int decals_count, const char *shader_name,
  bool use_array)
{
  if (!clipmapDecals)
    return -1;

  return clipmapDecals->createDecalType(d_tex_id, n_tex_id, m_tex_id, decals_count, shader_name, use_array);
}

// if array, diffuse_name and normal_name must be specified
int createDecalSubType(int decal_type, Point4 tc, int random_count, const char *diffuse_name, const char *normal_name,
  const char *material_name)
{
  if (!clipmapDecals)
    return -1;

  return clipmapDecals->createDecalSubType(decal_type, tc, random_count, diffuse_name, normal_name, material_name);
}

// render decals
void render(bool override_writemask)
{
  if (clipmapDecals)
    clipmapDecals->render(override_writemask);
}

void setup_rendering()
{
  if (clipmapDecals)
    clipmapDecals->setupRendering();
}

bool check_decal_textures_loaded()
{
  if (!clipmapDecals)
    return true;
  return clipmapDecals->checkDecalTexturesLoaded();
}

const Tab<BBox2> &get_updated_regions()
{
  if (!clipmapDecals)
  {
    static Tab<BBox2> empty;
    return empty;
  }

  return clipmapDecals->get_updated_regions();
}

void clear_updated_regions()
{
  if (clipmapDecals)
    clipmapDecals->clear_updated_regions();
}

void createDecal(int decal_type, const Point2 &pos, float rot, const Point2 &size, int subtype, bool mirror,
  uint64_t displacement_mask, int displacement_mask_size)
{
  clipmapDecals->createDecal(decal_type, pos, rot, size, subtype, mirror, displacement_mask, displacement_mask_size);
}

void after_reset()
{
  if (clipmapDecals)
    clipmapDecals->afterReset();
}

void set_delayed_regions_params(bool use_delayed_regions, int max_count, float size_factor)
{
  if (clipmapDecals)
    clipmapDecals->setDelayedRegionsParams(use_delayed_regions, max_count, size_factor);
}
void set_camera_position(Point3 pos)
{
  if (clipmapDecals)
    clipmapDecals->setCameraPosition(pos);
}
} // namespace clipmap_decals_mgr


static bool clipmap_decals_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("clipmap_decals", "reloadConfig", 1, 1)
  {
    if (!dd_file_exist(clipmap_decals_config_filename))
    {
      logerr("config file %s doesn't exist", clipmap_decals_config_filename);
      return found;
    }
    clipmap_decals_mgr::clipmapDecals->initDecalTypes(DataBlock(clipmap_decals_config_filename));
    clipmap_decals_mgr::clipmapDecals->updated_regions.push_back(BBox2(Point2(0.0, 0.0), 10000.0));
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(clipmap_decals_console_handler);
