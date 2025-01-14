//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_bitFlagsMask.h>
#include <util/dag_stdint.h>


namespace rendinst
{

// TODO: this file is bad, it should be split into logical components
// and eventually mostly eliminated, but it's a good intermediate step
// in refactoring.

inline constexpr int RIEXTRA_LODS_THRESHOLD = 2;
inline constexpr int MAX_LOD_COUNT = 5;
inline constexpr int RIEX_STAGE_COUNT = 4;

enum class GatherRiTypeFlag : uint32_t
{
  RiGenTmOnly = 1 << 0,
  RiGenPosOnly = 1 << 1,
  RiExtraOnly = 1 << 2,
  RiGenOnly = RiGenTmOnly | RiGenPosOnly,
  RiGenTmAndExtra = RiGenTmOnly | RiExtraOnly,
  RiGenAndExtra = RiGenOnly | RiExtraOnly
};
using GatherRiTypeFlags = BitFlagsMask<GatherRiTypeFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(GatherRiTypeFlag);

enum class CheckBoxRIResultFlag
{
  HasTraceableRi = 1 << 0,
  HasCollidableRi = 1 << 1,

  // TODO: the ALL name here makes zero sense, needs investigation
  All = HasTraceableRi | HasCollidableRi
};
using CheckBoxRIResultFlags = BitFlagsMask<CheckBoxRIResultFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(CheckBoxRIResultFlag);

enum class TraceFlag
{
  Meshes = 1 << 0,
  Destructible = 1 << 1,
  Trees = 1 << 2,
  Phys = 1 << 3
};
using TraceFlags = BitFlagsMask<TraceFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(TraceFlag);

enum class AddRIFlag
{
  None,
  UseShadow = 1 << 0,
  Immortal = 1 << 1,
  ForceDebris = 1 << 2,
  Dynamic = 1 << 3,
  GameresPreLoaded = 1 << 4, // Note: Set this bit to avoid using `get_one_game_resource_ex`
};
using AddRIFlags = BitFlagsMask<AddRIFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(AddRIFlag);

enum class DestrOptionFlag : uint32_t
{
  ForceDestroy = 1 << 0,
  AddDestroyedRi = 1 << 1,
  UseFullBbox = 1 << 2,
  DestroyedByExplosion = 1 << 3
};
using DestrOptionFlags = BitFlagsMask<DestrOptionFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(DestrOptionFlag);

enum class SceneSelection
{
  OnlyStatic,
  OnlyDynamic,
  All
};

// To consider: pack all this flags (and also render_pass/layer to one argument/bitmask)
enum class OptimizeDepthPrepass
{
  No,
  Yes
};
enum class OptimizeDepthPass
{
  No,
  Yes
};
enum class IgnoreOptimizationLimits
{
  No,
  Yes
};
enum class SkipTrees
{
  No,
  Yes
};
enum class RenderGpuObjects
{
  No,
  Yes
};
enum class AtestStage
{
  All,
  NoAtest,
  Atest,
  AtestAndImmDecal,
  NoImmDecal
};
// end of to consider

enum class SolidSectionsMerge : uint8_t
{
  COLL_NODE, // merge only sections in one coll node
  RI_MAT,    // merge sections in one ri with same material
  RENDINST,  // merge sections in one rendinst
  MATERIAL,  // merge sections with one material id
  ALL,       // merge all intersecting sections, material is acquired from starting section
  NONE
};

} // namespace rendinst
