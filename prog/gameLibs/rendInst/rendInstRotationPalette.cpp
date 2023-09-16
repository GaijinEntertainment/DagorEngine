#include <rendInst/rotation_palette_consts.hlsli>
#include "riGen/riRotationPalette.h"
#include "riGen/landClassData.h"

#include <3d/dag_drv3d_multi.h>
#include <math/dag_mathBase.h>
#include <gameRes/dag_stdGameResId.h>

#include <EASTL/algorithm.h>


using namespace rendinstgen;

uint32_t RotationPaletteManager::version = 0;

RotationPaletteManager::Palette::Palette(int cnt, int ofs, const PaletteEntry *rot, const Point3 &tilt) :
  count(cnt), impostor_data_offset(ofs), rotations(rot), tiltLimit(tilt)
{}

RotationPaletteManager::PaletteEntry::PaletteEntry(int id, int max)
{
  if (max > 0)
    rotationY = float(id) / (max)*2 * M_PI;
  else
    rotationY = 0;
  sincos(rotationY, sinY, cosY);
}

RotationPaletteManager::RotationPaletteManager() : impostorData(midmem), rotations(midmem)
{
  // By default nothing is rendered using palettes
  // If one rendinst uses palettes, it will be turned on globally
  // The reason for this is to avoid extra cpu overhead of switching
  if (RendInstGenData::renderResRequired)
    ShaderGlobal::set_int(ScopedDisablePaletteRotation::get_var_id(), 0);
  clear();
}

void RotationPaletteManager::clear()
{
  version++;
  impostorDataBuffer.close();
  entityIdToPaletteInfo.clear();
  // Dummy impostor data with identity rotation to serve as default value for anything that doesn't have rotation palettes
  impostorData.resize(IMPOSTOR_DATA_PALETTE_OFFSET + 1);
  for (int i = 0; i < IMPOSTOR_DATA_PALETTE_OFFSET; ++i)
    impostorData[i] = Point4(0, 0, 0, 0);
  impostorData[IMPOSTOR_DATA_PALETTE_OFFSET] = Point4(0, 1, 0, 0);
  rotations.resize(1);
  rotations[0] = {};
}

void RotationPaletteManager::createPalette(const RendintsInfo &ri_info)
{
  version++;
  int layerId = ri_info.rtData->layerIdx;
  // Add assets from landclass, that are registered with rotation palette
  for (uint32_t i = 0; i < ri_info.landClassCount; ++i)
  {
    const RendInstGenData::LandClassRec &landClass = ri_info.landClasses[i];
    const rendinstgenland::AssetData &land = *landClass.asset;
    if (!land.planted)
      continue;
    debug("[RI] Registering vegetation assets with rotation palette for landclass: <%s>", landClass.landClassName.get());
    for (const rendinstgenland::SingleGenEntityGroup &group : land.planted->data)
    {
      for (const rendinstgenland::SingleGenEntityGroup::Obj &obj : group.obj)
      {
        if (obj.entityIdx == -1) // perfectly valid case for no-RI entities referenced by land class
          continue;
        const int poolId = landClass.riResMap[obj.entityIdx];
        EntryId id = {layerId, poolId};
        if (!ri_info.rtData->riPaletteRotation[poolId])
          continue;
        uint32_t count = ri_info.rtData->riRes[poolId]->getRotationPaletteSize();
        G_ASSERTF(count > 0, "An RI is supposed to have a rotation palette, but its size is set to 0 <%s>",
          ri_info.rtData->riResName[poolId]);
        G_ASSERTF(count <= PALETTE_ID_MULTIPLIER, "An RI has invalid palette size <%s> (given: %d, max: %d)",
          ri_info.rtData->riResName[poolId], count, PALETTE_ID_MULTIPLIER);
        if (entityIdToPaletteInfo.find(id) != entityIdToPaletteInfo.end())
          continue;
        count = clamp(count, 1u, uint32_t(PALETTE_ID_MULTIPLIER));
        addPalette(ri_info.rtData->riResName[poolId], ri_info.rtData->riRes[poolId]->getImpostorParams(), id, count);
      }
    }
  }

  // Add manually placed assets, which were not present in the landclass (regardless of rotation palette)
  for (int i = 0; i < ri_info.pregenEntCount; ++i)
  {
    const int entityId = i;
    if (!ri_info.pregenEnt[i].paletteRotation)
      continue;
    EntryId id = {layerId, entityId};
    uint32_t count = ri_info.pregenEnt[i].paletteRotationCount;
    G_ASSERTF(count > 0, "A pregenEnt is supposed to use palette rotation, but the given palette size is 0");
    G_ASSERTF(strcmp(ri_info.rtData->riResName[entityId], ri_info.pregenEnt[i].riName.get()) == 0,
      "Pregen ent identified incorrectly: #%d <%s> != <%s>", i, ri_info.rtData->riResName[entityId],
      ri_info.pregenEnt[i].riName.get());
    if (count != ri_info.pregenEnt[i].riRes->getRotationPaletteSize() && RendInstGenData::renderResRequired)
      logwarn("[RI] The level binary was exported with a different palette size than currently "
              "specified for an RI: <%s> (level binary: %d != asset: %d)",
        ri_info.rtData->riResName[entityId], count, ri_info.pregenEnt[i].riRes->getRotationPaletteSize());
    G_ASSERTF(count <= PALETTE_ID_MULTIPLIER, "An RI has invalid palette size <%s> (given: %d, max: %d)",
      ri_info.rtData->riResName[entityId], count, PALETTE_ID_MULTIPLIER);
    if (entityIdToPaletteInfo.find(id) != entityIdToPaletteInfo.end())
      continue;
    count = clamp(count, 1u, uint32_t(PALETTE_ID_MULTIPLIER));
    // This entity is not registered in the currently used landclass, but it's placed down manually. Default rotation limits are used
    addPalette(ri_info.pregenEnt[i].riName.get(), ri_info.rtData->riRes[entityId]->getImpostorParams(), id, count);
  }

  // Add all landclass assets without rotation palette
  for (uint32_t i = 0; i < ri_info.landClassCount; ++i)
  {
    const RendInstGenData::LandClassRec &landClass = ri_info.landClasses[i];
    const rendinstgenland::AssetData &land = *landClass.asset;
    if (!land.planted)
      continue;
    debug("[RI] Registering vegetation assets without rotation palette for landclass: <%s>", landClass.landClassName.get());
    for (const rendinstgenland::SingleGenEntityGroup &group : land.planted->data)
    {
      for (const rendinstgenland::SingleGenEntityGroup::Obj &obj : group.obj)
      {
        if (obj.entityIdx == -1) // perfectly valid case for no-RI entities referenced by land class
          continue;
        const int poolId = landClass.riResMap[obj.entityIdx];
        EntryId id = {layerId, poolId};
        if (entityIdToPaletteInfo.find(id) != entityIdToPaletteInfo.end())
          continue;
        // Palette with only the identity rotation
        addPalette(ri_info.rtData->riResName[poolId], ri_info.rtData->riRes[poolId]->getImpostorParams(), id, 1);
      }
    }
  }

  if (RendInstGenData::renderResRequired)
    ShaderGlobal::set_int(ScopedDisablePaletteRotation::get_var_id(), 1);
  recreateBuffer();
}

void RotationPaletteManager::recreateBuffer()
{
  if (RendInstGenData::renderResRequired)
  {
    impostorDataBuffer.close();
    impostorDataBuffer = dag::buffers::create_persistent_sr_tbuf(impostorData.size(), TEXFMT_A32B32G32R32F, "impostor_data_buffer");
    fillBuffer();
  }
}

void RotationPaletteManager::fillBuffer()
{
  if (impostorDataBuffer.getBuf())
  {
    if (!impostorDataBuffer.getBuf()->updateDataWithLock(0, impostorData.size() * sizeof(Point4), impostorData.data(), 0))
      logerr("Could not upload palette rotations");
  }
}

bool RotationPaletteManager::hasPalette(const EntryId &id) const
{
  return entityIdToPaletteInfo.find(id) != entityIdToPaletteInfo.end();
}

RotationPaletteManager::Palette RotationPaletteManager::getPalette(const EntryId &id) const
{
  Palette ret;
  auto itr = entityIdToPaletteInfo.find(id);
  if (itr != entityIdToPaletteInfo.end() && itr->second.count > 0)
  {
    const PaletteInfo &info = itr->second;
    ret.count = info.count;
    ret.impostor_data_offset = info.impostor_data_offset;
    ret.rotations = &rotations[info.rotationsOffset];
    ret.tiltLimit = info.tiltLimit;
    ret.riName = info.riName.c_str();
  }
  else
  {
    ret.count = 1;
    ret.impostor_data_offset = 0;
    ret.rotations = &rotations[0];
    ret.tiltLimit = Point3(0, 0, 0);
    ret.riName = "_default_";
  }
  return ret;
}

uint32_t RotationPaletteManager::getImpostorDataBufferOffset(const EntryId &id, const char *name, RenderableInstanceLodsResource *res)
{
  auto itr = entityIdToPaletteInfo.find(id);
  if (itr == entityIdToPaletteInfo.end())
  {
    // This should only ever happen in the level editor, for trees that are not present in any landclass
    debug("Register rotation palette on demand for %s", name);

    int paletteSize = 1;

    DataBlock *data =
      reinterpret_cast<DataBlock *>(get_one_game_resource_ex(GAMERES_HANDLE(impostor_data), ImpostorDataGameResClassId));
    if (data)
    {
      DataBlock *impostorBlk = data->getBlockByName(name);
      if (impostorBlk)
      {
        paletteSize = clamp(impostorBlk->getInt("rotationPaletteSize", paletteSize), 1, PALETTE_ID_MULTIPLIER);
      }
      else
        logerr("Rotation palette is registered for assets <%s>, but no data found in the impostorData asset.", name);
    }
    else
    {
      // This could happen if the level binary was not exported after adding new baked impostors on the server side.
      logerr("Rotation palette registered for <%s>, but no impostor data is available.", name);
    }

    uint32_t ret = addPalette(name, res->getImpostorParams(), id, paletteSize).impostor_data_offset;
    recreateBuffer();
    return ret;
  }
  else
  {
    G_ASSERTF(itr->second.count > 0, "Impostor data buffer requested, but the associated palette is empty: <%s>", name);
    // Just sanity check. The name is passed here to allow creation on demand
    G_ASSERTF(itr->second.riName == name, "Unexpected impostor name: %s != %s", itr->second.riName.c_str(), name);
    return itr->second.impostor_data_offset;
  }
}

uint32_t RotationPaletteManager::getImpostorDataBufferOffset(const EntryId &id, uint64_t &cache)
{
  if ((cache >> CACHE_VERSION_BITS) == version)
    return cache & CACHE_OFFSET_MASK;
  auto itr = entityIdToPaletteInfo.find(id);
  // This is a valid case in this call
  if (itr == entityIdToPaletteInfo.end())
    return 0;
  G_ASSERTF(itr->second.count > 0, "Impostor data buffer requested, but the associated palette is empty: <%s>",
    itr->second.riName.c_str());
  uint32_t ret = itr->second.impostor_data_offset;
  G_ASSERT(ret <= CACHE_OFFSET_MASK);
  cache = (uint64_t(version) << CACHE_VERSION_BITS) | uint64_t(ret);
  return ret;
}

RotationPaletteManager::PaletteInfo &RotationPaletteManager::addPalette(const char *asset_name,
  const RenderableInstanceLodsResource::ImpostorParams &impostors_params, const EntryId &id, uint32_t rotation_count)
{
  PaletteInfo &info = entityIdToPaletteInfo[id];
  info.count = rotation_count;
  int paramCount = IMPOSTOR_DATA_SLICE_DATA_OFFSET;
  int impostorParamEntries = paramCount + impostors_params.perSliceParams.size() * IMPOSTOR_DATA_ENTRIES_PER_SLICE;
  info.rotationsOffset = rotations.size();
  info.impostor_data_offset = impostorData.size();
  info.riName = asset_name;
  impostorData.reserve(impostorData.size() + (rotation_count + 1) / 2 + impostorParamEntries);
  G_ASSERTF(impostors_params.perSliceParams.size() * IMPOSTOR_DATA_ENTRIES_PER_SLICE == IMPOSTOR_DATA_SLICE_DATA_SIZE,
    "Impostor params mismatch on shaders and on C++ code");
  int offset = impostorData.size();
  append_items(impostorData, paramCount);
  impostorData[offset + IMPOSTOR_DATA_SCALE] = impostors_params.scale;
  impostorData[offset + IMPOSTOR_DATA_BSP_Y__PRESHADOW] =
    Point4(impostors_params.boundingSphere.y, impostors_params.preshadowEnabled ? 1 : -1, 0, 0);
  for (int i = 0; i < 4; ++i)
    impostorData[offset + IMPOSTOR_DATA_VERTEX_OFFSET + i] = impostors_params.vertexOffsets[i];
  impostorData[offset + TRANSMITTANCE_CROWN_RAD1_DATA_OFFSET] = impostors_params.invCrownRad1;
  impostorData[offset + TRANSMITTANCE_CROWN_CENTER1_DATA_OFFSET] = impostors_params.crownCenter1;
  impostorData[offset + TRANSMITTANCE_CROWN_RAD2_DATA_OFFSET] = impostors_params.invCrownRad2;
  impostorData[offset + TRANSMITTANCE_CROWN_CENTER2_DATA_OFFSET] = impostors_params.crownCenter2;
  G_ASSERTF(impostorData.size() - offset == IMPOSTOR_DATA_SLICE_DATA_OFFSET, "Impostor data layout mismatch");
  for (int i = 0; i < impostors_params.perSliceParams.size(); ++i)
  {
    impostorData.push_back(ImpostorDataEntry{impostors_params.perSliceParams[i].sliceTm});
    impostorData.push_back(ImpostorDataEntry{impostors_params.perSliceParams[i].clippingLines});
  }
  for (int i = 0; i < rotation_count; ++i)
  {
    PaletteEntry rotation = PaletteEntry(i, rotation_count);
    if (!(i & 1))
    {
      impostorData.push_back().set(rotation.sinY, rotation.cosY, 0, 0);
    }
    else
    {
      impostorData.back().z = rotation.sinY;
      impostorData.back().w = rotation.cosY;
    }
    rotations.push_back() = eastl::move(rotation);
  }
  return info;
}

// min <= max and max - min <= 2*pi
// min is in range [-pi, 2*pi)
// max is in range [0, 3*pi)
// value is in range [-2*pi, 2*pi)
static float circular_clamp(float value, float min, float max)
{
  if (value < 0)
    value += M_PI * 2;
  // value is in [0, 2*pi)
  if (value < min - M_PI)
    value += M_PI * 2;
  else if (value >= min + M_PI)
    value -= M_PI * 2;
  // value is in [min-pi, min+pi)
  if ((value >= min && value <= max) || (value < min && value + 2 * M_PI <= max))
    return value; // value is in [min, max]
  if (value > min)
    return max; // value is closer to max
  return (min - value <= min + 2 * M_PI - max) ? min : max;
}

static float angular_distance(float a, float b)
{
  float ret = abs(fmod(a - b, 2 * M_PI));
  return min(ret, static_cast<float>(2 * M_PI) - ret);
}

Point3 RotationPaletteManager::clamp_euler_angles(const Palette &palette, const Point3 rotation, int32_t *palette_index)
{
  if (palette.count > 0)
  {
    uint8_t paletteId = 0; // closest rotation from palette
    for (uint8_t i = 1; i < palette.count; ++i)
      if (angular_distance(rotation.y, palette.rotations[i].rotationY) <
          angular_distance(rotation.y, palette.rotations[paletteId].rotationY))
        paletteId = i;
    if (palette_index)
      *palette_index = paletteId;
    float y = palette.rotations[paletteId].rotationY;
    G_ASSERT(abs(palette.tiltLimit.x) <= M_PI && abs(palette.tiltLimit.y) <= M_PI && abs(palette.tiltLimit.z) <= M_PI);
    // no need to apply modulo with 2*pi for these angle values, because the maximum interval size
    // is 2 * pi, which is handled by circular_clamp
    return Point3(circular_clamp(rotation.x, -palette.tiltLimit.x, palette.tiltLimit.x),
      circular_clamp(rotation.y, y - palette.tiltLimit.x, y + palette.tiltLimit.x),
      circular_clamp(rotation.z, -palette.tiltLimit.z, palette.tiltLimit.z));
  }
  else
  {
    if (palette_index)
      *palette_index = -1;
    return rotation;
  }
}

TMatrix RotationPaletteManager::clamp_euler_angles(const Palette &palette, const TMatrix &tm, int32_t *palette_index)
{
  if (palette.count > 0)
  {
    Point2 zDir = Point2(tm.getcol(2).x, tm.getcol(2).z); // tm * (0, 0, 1) projected to XZ
    uint8_t paletteId = 0;                                // closest rotation from palette
    // roty_tm(palette.rotations[0]) * (0, 0, 1) projected to XZ
    Point2 paletteZDir = Point2(-sinf(palette.rotations[0].rotationY), cosf(palette.rotations[0].rotationY));
    for (uint8_t i = 1; i < palette.count; ++i)
    {
      Point2 z = Point2(-sinf(palette.rotations[i].rotationY), cosf(palette.rotations[i].rotationY));
      if (dot(zDir, paletteZDir) < dot(zDir, z))
      {
        paletteId = i;
        paletteZDir = z;
      }
    }
    if (palette_index)
      *palette_index = paletteId;
    // TODO return remaining tilt
  }
  else
  {
    if (palette_index)
      *palette_index = -1;
  }
  TMatrix ret;
  ret.identity();
  return ret;
}

quat4f RotationPaletteManager::get_quat(const Palette &palette, int32_t palette_index)
{
  if (palette_index < 0 || palette_index >= palette.count)
    return V_C_UNIT_0001;
  return get_quat(palette.rotations[palette_index]);
}

quat4f RotationPaletteManager::get_quat(const PaletteEntry rotation)
{
  return v_quat_from_unit_vec_ang(V_C_UNIT_0100, v_splats(-rotation.rotationY));
}

TMatrix RotationPaletteManager::get_tm(const PaletteEntry &rotation)
{
  TMatrix tm;
  tm.identity();
  // exactly as rotyTM
  tm.m[2][2] = tm.m[0][0] = rotation.cosY;
  tm.m[2][0] = -(tm.m[0][2] = rotation.sinY);
  return tm;
}

TMatrix RotationPaletteManager::get_tm(const Palette &palette, int32_t palette_index)
{
  if (palette_index < 0 || palette_index >= palette.count)
  {
    TMatrix ret;
    ret.identity();
    return ret;
  }
  return get_tm(palette.rotations[palette_index]);
}

int ScopedDisablePaletteRotation::get_var_id()
{
  static const int palette_rotation_modeVarId = ::get_shader_glob_var_id("palette_rotation_mode", true);
  return palette_rotation_modeVarId;
}

ScopedDisablePaletteRotation::ScopedDisablePaletteRotation()
{
  G_ASSERT(RendInstGenData::renderResRequired);
  rotationMode = ShaderGlobal::get_int(get_var_id());
  ShaderGlobal::set_int(get_var_id(), 0);
}

ScopedDisablePaletteRotation::~ScopedDisablePaletteRotation() { ShaderGlobal::set_int(get_var_id(), rotationMode); }

namespace rendinstgen
{

void rotation_palette_after_reset(bool /*full_reset*/)
{
  if (RotationPaletteManager *mgr = get_rotation_palette_manager())
    mgr->fillBuffer();
}

} // namespace rendinstgen

#include <3d/dag_drv3dReset.h>
REGISTER_D3D_AFTER_RESET_FUNC(rendinstgen::rotation_palette_after_reset);
