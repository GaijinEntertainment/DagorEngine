// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shStateBlock.h"
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_buffers.h>

using namespace shaders;

struct SBSamplerState
{
  uint32_t start;           // we start with PS textures, than VS textures
  uint8_t psCount, vsCount; // don't think we will need more than 4 bits
  uint8_t psBase, vsBase;   // don't think we will need more than 4 bits

  void apply(int tex_level)
  {
    uint32_t from = start;

    const auto setTex = [&](TEXTUREID tid, ShaderStage stage, int slot) {
      mark_managed_tex_lfu(tid, tex_level);
      const auto sampler = get_texture_separate_sampler(tid);
      if (sampler != d3d::INVALID_SAMPLER_HANDLE)
      {
        d3d::set_tex(stage, slot, D3dResManagerData::getBaseTex(tid), false);
        d3d::set_sampler(stage, slot, sampler);
      }
      else
        d3d::set_tex(stage, slot, D3dResManagerData::getBaseTex(tid));
    };

    for (uint32_t i = psBase, e = (uint32_t)psBase + psCount; i < e; ++i, ++from)
      setTex(data[from], STAGE_PS, i);
    for (uint32_t i = vsBase, e = (uint32_t)vsBase + vsCount; i < e; ++i, ++from)
      setTex(data[from], STAGE_VS, i);
  }

  void reqTexLevel(int tex_level)
  {
    uint32_t from = start;
    for (uint32_t i = psBase, e = (uint32_t)psBase + psCount; i < e; ++i, ++from)
    {
      mark_managed_tex_lfu(data[from], tex_level);
    }
    for (uint32_t i = vsBase, e = (uint32_t)vsBase + vsCount; i < e; ++i, ++from)
    {
      mark_managed_tex_lfu(data[from], tex_level);
    }
  }

  static TexStateIdx create(const TEXTUREID *ps, uint8_t ps_base, uint8_t ps_cnt, const TEXTUREID *vs, uint8_t vs_base, uint8_t vs_cnt)
  {
    OSSpinlockScopedLock lock(mutex);
    int id = all.find_if([&](const SBSamplerState &c) {
      if (c.psCount != ps_cnt || c.vsCount != vs_cnt || c.psBase != ps_base || c.vsBase != vs_base)
        return false;
      auto d = &data[c.start];
      if (memcmp(d, ps, ps_cnt * sizeof(*ps)) == 0 && memcmp(d + ps_cnt, vs, vs_cnt * sizeof(*vs)) == 0)
        return true;
      return false;
    });
    if (id >= 0)
      return static_cast<TexStateIdx>(id);

    uint32_t did = data.append(nullptr, ps_cnt + vs_cnt);
    auto d = &data[did];
    memcpy(d, ps, ps_cnt * sizeof(*ps));
    memcpy(d + ps_cnt, vs, vs_cnt * sizeof(*vs));

    return static_cast<TexStateIdx>(all.push_back(SBSamplerState{did, ps_cnt, vs_cnt, ps_base, vs_base}));
  }
  static void clear()
  {
    data.clear();
    all.clear();
    all.push_back(SBSamplerState{0, 0, 0, 0, 0});
  }

  static NonRelocatableCont<TEXTUREID, 4096> data;
  static NonRelocatableCont<SBSamplerState, 1024> all; // first one is default
  static OSSpinlock mutex;
};

decltype(SBSamplerState::data) SBSamplerState::data;
decltype(SBSamplerState::all) SBSamplerState::all; // first one is default
decltype(SBSamplerState::mutex) SBSamplerState::mutex;


struct ConstState
{
  uint32_t constsStart;
  uint8_t constsCount;

  void apply(ConstStateIdx self_idx)
  {
    Sbuffer *constBuf = allConstBuf[eastl::to_underlying(self_idx)];
    if (!constBuf)
      return;

    d3d::set_const_buffer(STAGE_VS, 1, constBuf);
    d3d::set_const_buffer(STAGE_PS, 1, constBuf);
  }

  static ConstStateIdx create(const Point4 *consts_data, uint8_t consts_count)
  {
    OSSpinlockScopedLock lock(mutex);
    int cid = all.find_if([&](const ConstState &c) {
      if (c.constsCount != consts_count)
        return false;

      return memcmp(&dataConsts[c.constsStart], consts_data, consts_count * sizeof(*consts_data)) == 0; // -V1014
    });
    if (cid >= 0)
      return static_cast<ConstStateIdx>(cid);

    uint32_t did = dataConsts.append(consts_data, consts_count);
    uint32_t id = all.push_back(ConstState{did, consts_count});
    uint32_t acid = allConstBuf.push_back(nullptr);
    G_FAST_ASSERT(id == acid);
    G_UNUSED(acid);
    return static_cast<ConstStateIdx>(id);
  }
  static void clear()
  {
    dataConsts.clear();
    all.clear();
    all.push_back(ConstState{0, 0});
    allConstBuf.iterate([](auto &s) { destroy_d3dres(s); });
    allConstBuf.clear();
    allConstBuf.push_back(nullptr);
  }

  static void prepareConstBuf(ConstStateIdx const_state_idx)
  {
    const auto idx = eastl::to_underlying(const_state_idx);
    const ConstState &state = all[idx];
    if (!interlocked_acquire_load_ptr(allConstBuf[idx]) && state.constsCount)
    {
      String s(0, "staticCbuf%d", idx);
      Sbuffer *constBuf = d3d::buffers::create_persistent_cb(state.constsCount, s.c_str());
      if (!constBuf)
      {
        logerr("Could not create Sbuffer for const buf.");
        return;
      }
      constBuf->updateDataWithLock(0, sizeof(Point4) * state.constsCount, &dataConsts[state.constsStart], VBLOCK_DISCARD);
      if (DAGOR_UNLIKELY(interlocked_compare_exchange_ptr(allConstBuf[idx], constBuf, (Sbuffer *)nullptr) != nullptr))
        constBuf->destroy(); // unlikely case when other thread created/written buffer for this slot first
    }
  }

  static NonRelocatableCont<Point4, 8192> dataConsts;
  static NonRelocatableCont<ConstState, 2048> all;        // first one is default
  static NonRelocatableCont<Sbuffer *, 2048> allConstBuf; // parallel to all
  static OSSpinlock mutex;
};

decltype(ConstState::dataConsts) ConstState::dataConsts;
decltype(ConstState::all) ConstState::all;                 // first one is default
decltype(ConstState::allConstBuf) ConstState::allConstBuf; // parallel to all
decltype(ConstState::mutex) ConstState::mutex;

ShaderStateBlock create_slot_textures_state(const TEXTUREID *ps, uint8_t ps_base, uint8_t ps_cnt, const TEXTUREID *vs, uint8_t vs_base,
  uint8_t vs_cnt, const Point4 *consts_data, uint8_t consts_count, bool static_block)
{
  ShaderStateBlock block;
  block.samplerIdx = SBSamplerState::create(ps, ps_base, ps_cnt, vs, vs_base, vs_cnt);
  block.constIdx = ConstState::create(consts_data, consts_count);
  if (static_block)
    ConstState::prepareConstBuf(block.constIdx);
  return block;
}

void apply_slot_textures_state(ConstStateIdx const_state_idx, TexStateIdx sampler_state_id, int tex_level)
{
  SBSamplerState::all[eastl::to_underlying(sampler_state_id)].apply(tex_level);
  ConstState::all[eastl::to_underlying(const_state_idx)].apply(const_state_idx);
}

void clear_slot_textures_states()
{
  SBSamplerState::clear();
  ConstState::clear();
}

void slot_textures_req_tex_level(TexStateIdx sampler_state_id, int tex_level)
{
  SBSamplerState::all[eastl::to_underlying(sampler_state_id)].reqTexLevel(tex_level);
}

void dump_slot_textures_states_stat()
{
  debug("%d const states (%d total), %d texture states (%d total)", ConstState::all.getTotalCount(),
    ConstState::dataConsts.getTotalCount(), SBSamplerState::all.getTotalCount(), SBSamplerState::data.getTotalCount());
}
