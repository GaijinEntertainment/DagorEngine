// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_bitFlagsMask.h>
#include <3d/dag_materialData.h>
#include <shaders/dag_shaders.h>
#include <daFracture/core/meshSlicing.h>
#include <dag/dag_vector.h>
#include <math/dag_Point3.h>
#include <EASTL/bit.h>

class DataBlock;
namespace rendinst
{
struct RendInstDesc;
}
namespace destructables
{
struct DestrObjProps;
struct DestrAnimData;
} // namespace destructables

namespace rendinstdestr
{

struct RiDestrMaterial
{
  Ptr<MaterialData> matData;
  Ptr<ShaderMaterial> shMat;
  Ptr<ShaderElement> shElem;
};

enum class FrxDestrFlag : uint8_t
{
  Shatter = 1 << 0
};
using FrxDestrFlags = BitFlagsMask<FrxDestrFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(FrxDestrFlag);

struct FrxDestrProps
{
  FrxDestrFlags flags;
  frx::ShatterMaterialProfile shatterProfile;
  const RiDestrMaterial *interiorMat = nullptr;

  void loadFromBlk(const DataBlock &blk);
};

// per rendinst type by value: params{} block in presets, flat keys in the rendinst block
struct RiDestrParams
{
  void loadFromBlk(const DataBlock &blk);
};

struct RiNearbyGroupsProps
{
  struct Actions
  {
    uint32_t reveal : 1 = 0;
    uint32_t frxInclude : 1 = 0;
    uint32_t frxInvert : 1 = 0;
    uint32_t unused : 29 = 0;

    uint32_t mask() const { return eastl::bit_cast<uint32_t>(*this); }
  };
  static_assert(sizeof(Actions) == sizeof(uint32_t));
  struct Group
  {
    Actions actions;
    Point3 extent = Point3::ZERO;
    uint32_t poolsBegin = 0, poolsEnd = 0;
  };
  dag::Vector<Group> groups;
  dag::Vector<int> pools;

  void loadFromBlk(const char *ri_name, const DataBlock &ri_blk);
};

struct RiDestrProps
{
  RiDestrParams params;
  const FrxDestrProps *frxDestr = nullptr;
  const destructables::DestrObjProps *destrObj = nullptr;
  const destructables::DestrAnimData *destrAnim = nullptr;
  const RiNearbyGroupsProps *nearbyGroups = nullptr;
};

void init_ri_destr_props();
void shutdown_ri_destr_props();

// null for a rendinst type without a destruction entry; props and features live until the rendinst config is cleared
const RiDestrProps *get_ri_destr_props(const rendinst::RendInstDesc &desc);
const RiDestrMaterial *get_or_load_ri_destr_material(const char *name);

} // namespace rendinstdestr
