// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shStateBlockBindless.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <dag/dag_vector.h>
#include <EASTL/fixed_vector.h>
#include <memory/dag_framemem.h>
#include <shaders/dag_shaders.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint4.h>
#include <generic/dag_patchTab.h>
#include <shaders/dag_renderStateId.h>
#include <shaders/slotTexturesRange.h>
#include <osApiWrappers/dag_spinlock.h>
#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_info.h>

#include "concurrentElementPool.h"
#include "shStateBlk.h"

#if _TARGET_XBOX || _TARGET_C1 || _TARGET_C2
#define PLATFORM_HAS_BINDLESS true
#elif _TARGET_PC || _TARGET_ANDROID || _TARGET_C3 || _TARGET_PC_MACOSX
#define PLATFORM_HAS_BINDLESS d3d::get_driver_desc().caps.hasBindless
#else
#define PLATFORM_HAS_BINDLESS false
#endif

enum class BindlessTexId : uint32_t
{
  Invalid = 0
};

struct BindlessConstParams
{
  uint32_t constIdx;
  BindlessTexId texId; // if BindlessTexId::Invalid -- not yet added to to uniqBindlessTex

  bool operator==(const BindlessConstParams &other) const { return constIdx == other.constIdx && texId == other.texId; }
};

void apply_bindless_state(shaders::ConstStateIdx const_state_idx, int tex_level);
void clear_bindless_states();
void req_tex_level_bindless(shaders::ConstStateIdx const_state_idx, int tex_level);
using added_bindless_textures_t = eastl::fixed_vector<uint32_t, 4, /*overflow*/ true, framemem_allocator>;
BindlessTexId find_or_add_bindless_tex(TEXTUREID tid, added_bindless_textures_t &added_bindless_textures);
void reset_bindless_samplers_in_all_collections();
void dump_bindless_states_stat();

void apply_slot_textures_state(shaders::ConstStateIdx const_state_idx, shaders::TexStateIdx sampler_state_id, int tex_level);
void clear_slot_textures_states();
void dump_slot_textures_states_stat();
void slot_textures_req_tex_level(shaders::TexStateIdx sampler_state_id, int tex_level);

// NOTE: these magic constants should actually be `FIRST_ID` of various `ConcurrentElementPool`
// specializations, but due to header dependency loops we cannot use them directly, so we hard-code
// magic constants and assert them to match `FIRST_ID` in various other places.
// TODO: resolve include loops and use `FIRST_ID` directly
inline constexpr ShaderStateBlockId DEFAULT_SHADER_STATE_BLOCK_ID = static_cast<ShaderStateBlockId>(1 << 11);
inline constexpr shaders::TexStateIdx DEFAULT_TEX_STATE_ID = static_cast<shaders::TexStateIdx>(1 << 10);
inline constexpr shaders::ConstStateIdx DEFAULT_CONST_STATE_ID = static_cast<shaders::ConstStateIdx>(1 << 11);
inline constexpr shaders::ConstStateIdx DEFAULT_BINDLESS_CONST_STATE_ID = static_cast<shaders::ConstStateIdx>(1 << 9);

struct ShaderStateBlock
{
  shaders::RenderStateId stateIdx = {};
  shaders::TexStateIdx samplerIdx = {};
  shaders::ConstStateIdx constIdx = {};
#if _TARGET_STATIC_LIB
  uint16_t refCount = 0;
#else
  uint32_t refCount = 0;
#endif
  uint16_t texLevel = 0;

  static ConcurrentElementPool<ShaderStateBlockId, ShaderStateBlock, 11> blocks;
  static int deleted_blocks;

  static_assert(DEFAULT_SHADER_STATE_BLOCK_ID == decltype(blocks)::FIRST_ID);

  bool operator==(const ShaderStateBlock &b) const
  {
    return stateIdx == b.stateIdx && samplerIdx == b.samplerIdx && constIdx == b.constIdx;
  }

  static ShaderStateBlockId addBlock(ShaderStateBlock &&b, const shaders::RenderState &state)
  {
    shaders_internal::BlockAutoLock autoLock;

    G_FAST_ASSERT(b.stateIdx == shaders::RenderStateId::Invalid);
    b.stateIdx = shaders::render_states::create(state);

    G_ASSERT(b.refCount == 0);

    // find equivalent block and use it, when exists
    const auto bid = blocks.findIf([&](const ShaderStateBlock &eb) {
      return b == eb && eb.refCount < eastl::numeric_limits<decltype(ShaderStateBlock::refCount)>::max();
    });
    if (bid != ShaderStateBlockId::Invalid)
    {
      blocks[bid].refCount++;
      return bid;
    }

    b.refCount = 1;
    const auto result = blocks.allocate();
    blocks[result] = b;
    return result;
  }

  static void delBlock(ShaderStateBlockId id)
  {
    shaders_internal::BlockAutoLock autoLock;
    ShaderStateBlock &b = blocks[id];
    if (b.refCount == 0)
      logmessage(DAGOR_DBGLEVEL > 0 ? LOGLEVEL_ERR : LOGLEVEL_WARN, "trying to remove deleted/broken state block, refCount = %d",
        b.refCount);
    else if (--b.refCount)
      return;
    deleted_blocks++;
    interlocked_compare_exchange(shaders_internal::cached_state_block, eastl::to_underlying(ShaderStateBlockId::Invalid),
      eastl::to_underlying(id));
    return;
  }

public:
  void apply(int tex_level = 15)
  {
    texLevel = tex_level;
#ifndef __SANITIZE_THREAD__ // we might inc. this refCount in other thread right now, benign data race, don't complain about it
    G_FAST_ASSERT(refCount > 0);
#endif
    if (PLATFORM_HAS_BINDLESS)
    {
      apply_bindless_state(constIdx, tex_level);
    }
    else
    {
      apply_slot_textures_state(constIdx, samplerIdx, tex_level);
    }
    shaders::render_states::set(stateIdx);
  }
  void reqTexLevel(int tex_level)
  {
    texLevel = tex_level;
    if (PLATFORM_HAS_BINDLESS)
    {
      req_tex_level_bindless(constIdx, tex_level);
    }
    else
    {
      slot_textures_req_tex_level(samplerIdx, tex_level);
    }
  }
  static void clear()
  {
    shaders_internal::BlockAutoLock autoLock;
    blocks.clear();
    if (PLATFORM_HAS_BINDLESS)
    {
      clear_bindless_states();
      blocks[blocks.allocate()] = ShaderStateBlock{{}, {}, DEFAULT_BINDLESS_CONST_STATE_ID, 0, 0};
    }
    else
    {
      clear_slot_textures_states();
      blocks[blocks.allocate()] = ShaderStateBlock{{}, DEFAULT_TEX_STATE_ID, DEFAULT_CONST_STATE_ID, 0, 0};
    }
    deleted_blocks = 0;
  }

  static void reset_samplers()
  {
    if (!PLATFORM_HAS_BINDLESS)
      return;
    shaders_internal::BlockAutoLock autoLock;
    reset_bindless_samplers_in_all_collections();
  }
};

ShaderStateBlock create_bindless_state(const BindlessConstParams *bindless_data, uint8_t bindless_count, const Point4 *consts_data,
  uint8_t consts_count, dag::ConstSpan<uint32_t> added_bindless_textures, bool static_block, int stcode_id);

ShaderStateBlock create_slot_textures_state(const TEXTUREID *ps, SlotTexturesRangeInfo ps_range, const TEXTUREID *vs,
  SlotTexturesRangeInfo vs_range, const Point4 *consts_data, uint8_t consts_count, bool static_block);
