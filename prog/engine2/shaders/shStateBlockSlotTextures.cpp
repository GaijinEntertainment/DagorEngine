#include "shStateBlock.h"
#include <3d/dag_drv3d_buffers.h>

struct SBSamplerState
{
  uint32_t start;           // we start with PS textures, than VS textures
  uint8_t psCount, vsCount; // don't think we will need more than 4 bits
  uint8_t psBase, vsBase;   // don't think we will need more than 4 bits

  void apply(int tex_level)
  {
    uint32_t from = start;
    for (uint32_t i = psBase, e = (uint32_t)psBase + psCount; i < e; ++i, ++from)
    {
      mark_managed_tex_lfu(data[from], tex_level);
      d3d::set_tex(STAGE_PS, i, D3dResManagerData::getBaseTex(data[from]));
    }
    for (uint32_t i = vsBase, e = (uint32_t)vsBase + vsCount; i < e; ++i, ++from)
    {
      mark_managed_tex_lfu(data[from], tex_level);
      d3d::set_tex(STAGE_VS, i, D3dResManagerData::getBaseTex(data[from]));
    }
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

  static uint32_t create(const TEXTUREID *ps, uint8_t ps_base, uint8_t ps_cnt, const TEXTUREID *vs, uint8_t vs_base, uint8_t vs_cnt)
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
      return id;

    uint32_t did = data.append(nullptr, ps_cnt + vs_cnt);
    auto d = &data[did];
    memcpy(d, ps, ps_cnt * sizeof(*ps));
    memcpy(d + ps_cnt, vs, vs_cnt * sizeof(*vs));

    return all.push_back(SBSamplerState{did, ps_cnt, vs_cnt, ps_base, vs_base});
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

  void apply(uint32_t self_idx)
  {
    Sbuffer *constBuf = allConstBuf[self_idx];
    if (!constBuf)
      return;

    d3d::set_const_buffer(STAGE_VS, 1, constBuf);
    d3d::set_const_buffer(STAGE_PS, 1, constBuf);
  }

  static uint32_t create(const Point4 *consts_data, uint8_t consts_count)
  {
    OSSpinlockScopedLock lock(mutex);
    int cid = all.find_if([&](const ConstState &c) {
      if (c.constsCount != consts_count)
        return false;

      return memcmp(&dataConsts[c.constsStart], consts_data, consts_count * sizeof(*consts_data)) == 0; // -V1014
    });
    if (cid >= 0)
      return cid;

    uint32_t did = dataConsts.append(consts_data, consts_count);
    uint32_t id = all.push_back(ConstState{did, consts_count});
    uint32_t acid = allConstBuf.push_back(nullptr);
    G_FAST_ASSERT(id == acid);
    G_UNUSED(acid);
    return id;
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

  static void prepareConstBuf(int idx)
  {
    const ConstState &state = all[idx];
    if (!interlocked_acquire_load_ptr(allConstBuf[idx]) && state.constsCount)
    {
      String s(0, "staticCbuf%d", idx);
      Sbuffer *constBuf = d3d_buffers::create_persistent_cb(state.constsCount, s.c_str());
      if (!constBuf)
      {
        logerr("Could not create Sbuffer for const buf.");
        return;
      }
      constBuf->updateDataWithLock(0, sizeof(Point4) * state.constsCount, &dataConsts[state.constsStart], VBLOCK_DISCARD);
      if (EASTL_UNLIKELY(interlocked_compare_exchange_ptr(allConstBuf[idx], constBuf, (Sbuffer *)nullptr) != nullptr))
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

void apply_slot_textures_state(uint32_t const_state_idx, uint32_t sampler_state_id, int tex_level)
{
  SBSamplerState::all[sampler_state_id].apply(tex_level);
  ConstState::all[const_state_idx].apply(const_state_idx);
}

void clear_slot_textures_states()
{
  SBSamplerState::clear();
  ConstState::clear();
}

void slot_textures_req_tex_level(uint32_t sampler_state_id, int tex_level)
{
  SBSamplerState::all[sampler_state_id].reqTexLevel(tex_level);
}

void dump_slot_textures_states_stat()
{
  debug("%d const states (%d total), %d texture states (%d total)", ConstState::all.getTotalCount(),
    ConstState::dataConsts.getTotalCount(), SBSamplerState::all.getTotalCount(), SBSamplerState::data.getTotalCount());
}
