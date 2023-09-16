#pragma once

#include "riGen/riGenData.h"

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/string.h>
#include <cstdint>

namespace rendinstgen
{
inline constexpr int CACHE_VERSION_BITS = 32;
inline constexpr uint64_t CACHE_OFFSET_MASK = (1ull << CACHE_VERSION_BITS) - 1;

class RotationPaletteManager
{
public:
  struct PaletteEntry
  {
    float rotationY = 0;
    float cosY = 1;
    float sinY = 0;
    PaletteEntry() = default;
    PaletteEntry(int id, int palette_size); // gerenates ith rotation in a palette if palette_size
  };

  struct Palette
  {
    Palette() = default;
    Palette(int count, int impostor_data_offset, const PaletteEntry *rotations, const Point3 &tiltLimit);
    uint16_t count = 0;
    uint16_t impostor_data_offset = 0;
    const PaletteEntry *rotations = nullptr;
    Point3 tiltLimit;
    const char *riName = nullptr;
  };

  struct EntryId
  {
    int layerId;
    int poolNo;
    EntryId(int layer_id, int pool_no) : layerId(layer_id), poolNo(pool_no) {}
    bool operator==(const EntryId &b) const { return layerId == b.layerId && poolNo == b.poolNo; }
  };
  struct EntryIdHash
  {
    size_t operator()(const EntryId &id) const { return (eastl::hash<int>()(id.layerId) * 3) ^ eastl::hash<int>()(id.poolNo); }
  };

  RotationPaletteManager();

  RotationPaletteManager(const RotationPaletteManager &) = delete;
  RotationPaletteManager &operator=(const RotationPaletteManager &) = delete;

  struct RendintsInfo
  {
    uint32_t landClassCount = 0;
    const RendInstGenData::LandClassRec *landClasses = nullptr;
    const RendInstGenData::RtData *rtData = nullptr;
    uint32_t pregenEntCount = 0;
    const RendInstGenData::PregenEntPoolDesc *pregenEnt = nullptr;
  };

  void createPalette(const RendintsInfo &ri_info);
  void clear();

  Palette getPalette(const EntryId &id) const;
  bool hasPalette(const EntryId &id) const;

  // Cache stores a version number and the return value
  // this function checks if the cache is valid and updates it if not
  uint32_t getImpostorDataBufferOffset(const EntryId &id, uint64_t &cache);
  uint32_t getImpostorDataBufferOffset(const EntryId &id, const char *name, RenderableInstanceLodsResource *res);
  uint32_t getRotationsOffset(const EntryId &id) const;

  static Point3 clamp_euler_angles(const Palette &palette, const Point3 rotation, int32_t *palette_index = nullptr);
  static TMatrix clamp_euler_angles(const Palette &palette, const TMatrix &tm, int32_t *palette_index = nullptr);

  static TMatrix get_tm(const Palette &palette, int32_t palette_index);
  static TMatrix get_tm(const PaletteEntry &rotation);

  friend void rotation_palette_after_reset(bool /*full_reset*/);

  static quat4f get_quat(const Palette &palette, int32_t palette_index);
  static quat4f get_quat(const PaletteEntry rotation);

private:
  static const uint16_t INVALID_OFFSET = eastl::numeric_limits<uint16_t>::max();

  // Incremented on build/clear
  static uint32_t version;

  struct PaletteInfo
  {
    uint16_t impostor_data_offset;
    uint16_t count;
    Point3 tiltLimit;
    uint32_t rotationsOffset;
    eastl::string riName;
  };

  using ImpostorDataEntry = Point4;
  ska::flat_hash_map<EntryId, PaletteInfo, EntryIdHash> entityIdToPaletteInfo;
  Tab<ImpostorDataEntry> impostorData;
  UniqueBufHolder impostorDataBuffer;
  Tab<PaletteEntry> rotations;

  PaletteInfo &addPalette(const char *asset_name, const RenderableInstanceLodsResource::ImpostorParams &impostors_params,
    const EntryId &id, uint32_t rotation_count);
  void recreateBuffer();
  void fillBuffer();
};

// Palette rotation is identity rotation while this is active
struct ScopedDisablePaletteRotation
{
  int rotationMode;
  ScopedDisablePaletteRotation();
  ~ScopedDisablePaletteRotation();
  ScopedDisablePaletteRotation(const ScopedDisablePaletteRotation &) = delete;
  ScopedDisablePaletteRotation &operator=(const ScopedDisablePaletteRotation &) = delete;

  static int get_var_id();
};

RotationPaletteManager *get_rotation_palette_manager();

} // namespace rendinstgen
