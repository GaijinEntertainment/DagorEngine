// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shStateBlockBindless.h>

#include <generic/dag_enumerate.h>
#include <EASTL/deque.h>
#include <EASTL/bitset.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_lockSbuffer.h>
#include <vecmath/dag_vecMath.h>

#include "shStateBlock.h"

using namespace shaders;

struct BindlessTexRecord
{
  TEXTUREID texId;
  uint32_t bindlessId;
  uint32_t lastFrame;
  uint8_t bestReqLevel;
};

struct BindlessState;

static constexpr uint32_t REGS_IN_CONST_BUF = 4096;
static constexpr uint32_t MAX_CONST_BUFFER_SIZE = REGS_IN_CONST_BUF;
static_assert(MAX_CONST_BUFFER_SIZE <= REGS_IN_CONST_BUF, "Const buffer should be less than max value available by graphics APIs.");
static constexpr int32_t DEFAULT_SAMPLER_ID = 0;

static constexpr uint32_t PACKED_STCODE_BITS = 12;
static constexpr uint32_t BUFFER_BITS = 7;
static constexpr uint32_t GLOBAL_STATE_BITS = 13;

// TODO: generalize this to custom container that verifies invariant of non relocation on insertion
using ConstantsContainer = NonRelocatableCont<Point4, MAX_CONST_BUFFER_SIZE>;
using StatesContainer = eastl::deque<BindlessState, EASTLAllocatorType, 512>;
using BuffersContainer = eastl::deque<Sbuffer *, EASTLAllocatorType, 512>; // TODO: Use BufPtr instead of pointer

static ConstantsContainer dataConsts;
static StatesContainer all;          // first one is default
static BuffersContainer allConstBuf; // parallel to all
static eastl::deque<BindlessConstParams, EASTLAllocatorType, 8192> bindlessConstParams;
static eastl::deque<BindlessTexRecord, EASTLAllocatorType, 1024> uniqBindlessTex;
static OSSpinlock mutex;

constexpr uint8_t INVALID_MAT_ID = UCHAR_MAX;
static eastl::deque<eastl::remove_const_t<decltype(INVALID_MAT_ID)>, EASTLAllocatorType, 2048> stcodeIdToPacked;

template <typename T>
using PackedCont = eastl::deque<T, EASTLAllocatorType, 32>;
static PackedCont<StatesContainer> packedAll;
static PackedCont<ConstantsContainer> packedDataConsts;
static PackedCont<eastl::deque<Sbuffer *, EASTLAllocatorType, 4>> packedConstBuf;
static PackedCont<eastl::bitset<1 << BUFFER_BITS>> bufferNeedsUpdate;

static uint32_t allocate_packed_cell(int stcode_id)
{
  G_FAST_ASSERT(stcode_id >= 0);

  if (stcodeIdToPacked.size() <= stcode_id)
    stcodeIdToPacked.resize(stcode_id + 1, INVALID_MAT_ID);

  if (stcodeIdToPacked[stcode_id] == INVALID_MAT_ID)
  {
    uint32_t cellId = packedDataConsts.size();
    packedDataConsts.push_back();
    packedConstBuf.resize(cellId + 1);
    packedAll.resize(cellId + 1);
    bufferNeedsUpdate.resize(cellId + 1);
    G_ASSERTF(cellId < INVALID_MAT_ID, "!(%d < %d)", cellId, INVALID_MAT_ID);
    stcodeIdToPacked[stcode_id] = cellId;
  }

  return stcodeIdToPacked[stcode_id];
}

static uint32_t register_bindless_sampler(TEXTUREID tid, bool after_reset = false)
{
  const auto sampler = get_texture_separate_sampler(tid);
  if (sampler != d3d::INVALID_SAMPLER_HANDLE)
    return d3d::register_bindless_sampler(sampler);

  if (tid == BAD_TEXTUREID)
    return DEFAULT_SAMPLER_ID;

  auto tex = D3dResManagerData::getBaseTex(tid);
  if (tex == nullptr)
  {
    if (!after_reset) // It's ok for texture to become unavailable after initial registration
      logerr("register_bindless_sampler: Texture (tid=%d, idx=%d) is not available. Check initialization order (RMGR, bindless).", tid,
        tid.index());
    return DEFAULT_SAMPLER_ID;
  }

  return d3d::register_bindless_sampler(tex);
}

static Sbuffer *create_static_cb(uint32_t register_count, const char *name)
{
#if _TARGET_C1 || _TARGET_C2




#else
  return d3d::buffers::create_persistent_cb(register_count, name);
#endif
}

class PackedConstsState
{
  static_assert(PACKED_STCODE_BITS + BUFFER_BITS + GLOBAL_STATE_BITS == 32, "We want to use the whole uint32_t for state id.");

  eastl::underlying_type_t<ConstStateIdx> stateId;

public:
  PackedConstsState(int packed_stcode_id, uint32_t buffer_id, uint32_t global_state_id)
  {
    G_ASSERTF(packed_stcode_id < (1 << PACKED_STCODE_BITS), "PackedConstsState: packed_stcode_id=%d", packed_stcode_id);
    G_ASSERTF(buffer_id < (1 << BUFFER_BITS), "PackedConstsState: buffer_id=%" PRIu32, buffer_id);
    G_ASSERTF(global_state_id < (1 << GLOBAL_STATE_BITS),
      "PackedConstsState: global_state_id=%" PRIu32 ". (Hint: check if too many materials are created!)", global_state_id);

    stateId = (packed_stcode_id < 0)
                ? global_state_id
                : (((((uint32_t)(packed_stcode_id + 1) << BUFFER_BITS) | buffer_id) << GLOBAL_STATE_BITS) | global_state_id);
  }

  PackedConstsState(ConstStateIdx state) : stateId(eastl::to_underlying(state)) {}

  int getPackedStcodeId() const { return (int)(stateId >> (GLOBAL_STATE_BITS + BUFFER_BITS)) - 1; }

  uint32_t getGlobalStateId() const { return stateId & ((1 << GLOBAL_STATE_BITS) - 1); }

  uint32_t getLocalStateOffset() const;

  uint32_t getBufferId() const { return (stateId >> GLOBAL_STATE_BITS) & ((1 << BUFFER_BITS) - 1); }

  uint32_t getMaterialId() const { return (stateId >> GLOBAL_STATE_BITS) & ((1 << (PACKED_STCODE_BITS + BUFFER_BITS)) - 1); }

  operator ConstStateIdx() const { return static_cast<ConstStateIdx>(stateId); }
};

struct BindlessState
{
  uint32_t bindlessTexStart, constsStart; // Note: not indexes, might have higher bits set
  uint8_t bindlessTexCount, constsCount;

  template <typename IterCb>
  void iterateBindlessRange(ConstantsContainer &container, const IterCb &callback)
  {
    for (auto it = begin(bindlessConstParams) + bindlessTexStart, ite = it + bindlessTexCount; it != ite; ++it)
    {
      const BindlessConstParams &constParams = *it;
      BindlessTexRecord &texRecord = uniqBindlessTex[constParams.texId];
      IPoint4 &bindlessConsts = *reinterpret_cast<IPoint4 *>(&container[constsStart + constParams.constIdx]);
      callback(texRecord, bindlessConsts);
    }
  }

  void updateTexForPackedMaterial(ConstStateIdx self_idx, int tex_level)
  {
    PackedConstsState stateId(self_idx);
    const int packedId = stateId.getPackedStcodeId();
    G_ASSERT(packedId != -1);

    Sbuffer *constBuf = packedConstBuf[packedId][stateId.getBufferId()];
    if (!constBuf)
      return;
    const uint32_t frameNo = bindlessTexCount == 0 ? 0 : dagor_frame_no(); // We use frame no only if bindless textures are used, so
                                                                           // set it to 0 otherwise.

    iterateBindlessRange(packedDataConsts[packedId], [tex_level, frameNo](BindlessTexRecord &tex_record, IPoint4 &bindless_consts) {
      if (tex_record.texId == BAD_TEXTUREID)
      {
        update_tex_record_bindless_slot_to_null(tex_record, frameNo);
        return;
      }

      if (tex_record.lastFrame != frameNo)
      {
        BaseTexture *baseTex = D3dResManagerData::getBaseTex(tex_record.texId);
        if (EASTL_UNLIKELY(baseTex == nullptr))
        {
          update_tex_record_bindless_slot_to_null(tex_record, frameNo);
          logerr("Couldn't update bindless texture (tid=%d, idx=%d) for packed material: BaseTex is null.", tex_record.texId,
            tex_record.texId.index());
          return;
        }
        d3d::update_bindless_resource(tex_record.bindlessId, baseTex);
        tex_record.lastFrame = frameNo;
        tex_record.bestReqLevel = 0;
      }
      if (tex_level > tex_record.bestReqLevel)
      {
        mark_managed_tex_lfu(tex_record.texId, tex_level);
        tex_record.bestReqLevel = tex_level;
      }
    });

    if (bufferNeedsUpdate[packedId].test(stateId.getBufferId()))
    {
      bufferNeedsUpdate[packedId].reset(stateId.getBufferId());
      uint32_t beginIdx = stateId.getBufferId() * MAX_CONST_BUFFER_SIZE;
      const auto &dataConsts = packedDataConsts[packedId];
      uint32_t beginId = dataConsts.indexToId(beginIdx);
      uint32_t count = min(dataConsts.countInSubArray(beginId), MAX_CONST_BUFFER_SIZE);
      if (count > 0)
        constBuf->updateData(0, count * sizeof(Point4), &dataConsts[beginId], VBLOCK_DISCARD);
    }
  }

  void resetPackedSamplers(uint32_t collection_id)
  {
    iterateBindlessRange(packedDataConsts[collection_id], [](BindlessTexRecord &tex_record, IPoint4 &bindless_consts) {
      bindless_consts.y = register_bindless_sampler(tex_record.texId, true);
    });

    for (size_t bufferId = 0; bufferId < packedConstBuf[collection_id].size(); ++bufferId)
      if (packedConstBuf[collection_id][bufferId])
        bufferNeedsUpdate[collection_id].set(bufferId);
  }

  void resetSamplers(uint32_t self_idx)
  {
    Sbuffer *constBuf = allConstBuf[self_idx];
    if (!constBuf)
      return;
    iterateBindlessRange(dataConsts, [](BindlessTexRecord &tex_record, IPoint4 &bindless_consts) {
      bindless_consts.y = register_bindless_sampler(tex_record.texId, true);
    });
    if (constsCount)
      constBuf->updateData(0, constsCount * sizeof(Point4), &dataConsts[constsStart], VBLOCK_DISCARD);
  }

  void applyPackedMaterial(ConstStateIdx self_idx)
  {
    PackedConstsState stateId(self_idx);
    const int packedId = stateId.getPackedStcodeId();
    G_ASSERT(packedId != -1);

    Sbuffer *constBuf = packedConstBuf[packedId][stateId.getBufferId()];
    if (!constBuf)
      return;

    d3d::set_const_buffer(STAGE_VS, 1, constBuf);
    d3d::set_const_buffer(STAGE_PS, 1, constBuf);
  }

  void apply(ConstStateIdx self_idx, int tex_level)
  {
    PackedConstsState stateId(self_idx);
    const int stcode_id = stateId.getPackedStcodeId();
    G_ASSERT(stcode_id == -1);

    Sbuffer *constBuf = allConstBuf[stateId.getGlobalStateId()];
    if (!constBuf)
      return;
    const uint32_t frameNo = bindlessTexCount == 0 ? 0 : dagor_frame_no(); // We use frame no only if bindless textures are used, so
                                                                           // set it to 0 otherwise.
    bool requireToUpdateConstBuf = false;

    iterateBindlessRange(dataConsts, [frameNo, tex_level](BindlessTexRecord &tex_record, IPoint4 &bindless_consts) {
      if (tex_record.texId == BAD_TEXTUREID)
      {
        update_tex_record_bindless_slot_to_null(tex_record, frameNo);
        return;
      }

      mark_managed_tex_lfu(tex_record.texId, tex_level);
      if (tex_record.lastFrame != frameNo)
      {
        d3d::update_bindless_resource(tex_record.bindlessId, D3dResManagerData::getBaseTex(tex_record.texId));
        tex_record.lastFrame = frameNo;
        tex_record.bestReqLevel = 0;
      }
      if (tex_level > tex_record.bestReqLevel)
      {
        mark_managed_tex_lfu(tex_record.texId, tex_level);
        tex_record.bestReqLevel = tex_level;
      }
    });

    d3d::set_const_buffer(STAGE_VS, 1, constBuf);
    d3d::set_const_buffer(STAGE_PS, 1, constBuf);
  }

  void reqTexLevel(int tex_level)
  {
    for (auto it = begin(bindlessConstParams) + bindlessTexStart, ite = it + bindlessTexCount; it != ite; ++it)
    {
      const BindlessConstParams &constParams = *it;
      BindlessTexRecord &texRecord = uniqBindlessTex[constParams.texId];
      if (texRecord.texId == BAD_TEXTUREID)
        continue;

      if (tex_level > texRecord.bestReqLevel)
      {
        mark_managed_tex_lfu(texRecord.texId, tex_level);
        texRecord.bestReqLevel = tex_level;
      }
    }
  }

  static ConstStateIdx create(const BindlessConstParams *bindless_data, uint8_t bindless_count, const Point4 *consts_data,
    uint8_t consts_count, dag::Span<uint32_t> addedBindlessTextures, int stcode_id = -1)
  {
    G_FAST_ASSERT(stcode_id >= -1);

    OSSpinlockScopedLock lock(mutex);
    const bool usePackedConstants = stcode_id >= 0;

    uint32_t packedId = usePackedConstants ? allocate_packed_cell(stcode_id) : INVALID_MAT_ID;
    auto &dataConstToAdd = usePackedConstants ? packedDataConsts[packedId] : dataConsts;
    auto &states = usePackedConstants ? packedAll[packedId] : all;
    auto foundIt = eastl::find_if(begin(states), end(states), [&](const BindlessState &c) {
      if (c.bindlessTexCount != bindless_count || c.constsCount != consts_count)
        return false;
      if (!eastl::equal(bindless_data, bindless_data + bindless_count, begin(bindlessConstParams) + c.bindlessTexStart))
        return false;

      auto p4btcmp = [](const Point4 &a, const Point4 &b) {
        return v_signmask(v_cmp_eqi(v_ld(&a.x), v_ld(&b.x))) == 15; // bitwise compare to be able collapse bitwise-identical NaNs
      };
      uint32_t beginConst = 0;
      // if there are exist bindless resources we compare all dataConstToAdd ranges EXCEPT bindless ones,
      // since it might not be applied yet (register_bindless_sampler isn't called)
      for (uint32_t i = c.bindlessTexStart, ie = i + bindless_count; i < ie; ++i)
      {
        uint32_t endConst = bindlessConstParams[i].constIdx;
        if (!eastl::equal(consts_data + beginConst, consts_data + endConst, &dataConstToAdd[c.constsStart + beginConst], p4btcmp))
          return false;
        beginConst = endConst + 1;
      }
      return eastl::equal(consts_data + beginConst, consts_data + consts_count, &dataConstToAdd[c.constsStart + beginConst], p4btcmp);
    });
    if (foundIt != end(states))
    {
      const uint32_t globalStateId = eastl::distance(begin(states), foundIt);
      const uint32_t bufferId = consts_count == 0 ? 0 : (globalStateId / (MAX_CONST_BUFFER_SIZE / consts_count));
      if (consts_count != 0 && stcode_id >= 0)
        bufferNeedsUpdate[packedId].set(bufferId);
      return PackedConstsState(stcode_id < 0 ? -1 : packedId, bufferId, globalStateId);
    }

    uint32_t bid = bindlessConstParams.size();

    if (addedBindlessTextures.empty())
      bindlessConstParams.insert(bindlessConstParams.end(), bindless_data, bindless_data + bindless_count);
    else
      bindlessConstParams.resize(bindlessConstParams.size() + bindless_count);
    auto bdata = begin(bindlessConstParams) + bid;
    if (!addedBindlessTextures.empty())
    {
      uint32_t brange = d3d::allocate_bindless_resource_range(RES3D_TEX, addedBindlessTextures.size());
      for (auto &tex : addedBindlessTextures)
      {
        uniqBindlessTex.push_back(BindlessTexRecord{TEXTUREID(tex), brange++, 0, 0});
        tex = uniqBindlessTex.size() - 1;
      }

      auto bdataToFill = bdata;
      uint32_t j = 0;
      for (auto it = bindless_data, end = bindless_data + bindless_count; it != end; ++it, ++bdataToFill)
      {
        if (it->texId >= 0)
          *bdataToFill = *it;
        else
          *bdataToFill = BindlessConstParams{it->constIdx, (int)addedBindlessTextures[j++]};
      }
      G_FAST_ASSERT(j == addedBindlessTextures.size());
    }
#ifdef _DEBUG_TAB_
    for (auto it = bdata, ite = it + bindless_count; it != ite; ++it)
      G_FAST_ASSERT(it->constIdx < consts_count);
#endif
    // Align to 4K blocks for 2nd and further subarrays
    const uint32_t cBufferAlignment = usePackedConstants ? MAX_CONST_BUFFER_SIZE : 1;
    if ((dataConstToAdd.getTotalCount() + consts_count) % cBufferAlignment != 0 &&
        dataConstToAdd.getTotalCount() / cBufferAlignment != (dataConstToAdd.getTotalCount() + consts_count) / cBufferAlignment)
      dataConstToAdd.append(nullptr, cBufferAlignment - dataConstToAdd.getTotalCount() % cBufferAlignment);
    uint32_t did = dataConstToAdd.append(consts_data, consts_count);
#if DAGOR_DBGLEVEL > 0
    // If NonRelocatableCont's subarrays gets re-allocated then all thread safety guarantees are lost.
    // Investigate why did it happened - it's either bad merge of consts or perhaps subarray size need to be increased.
    if (DAGOR_UNLIKELY(dataConstToAdd.getTotalCount() >= dataConstToAdd.MaxNonRelocatableContSize))
      logerr("Overflow thread-safe bindless data consts limit @ %d, subArraySize=%d, states[%d].size=%d, bindlessConstParams.size=%d",
        dataConstToAdd.getTotalCount(), dataConstToAdd.MaxNonRelocatableContSize, packedId, states.size(), bindlessConstParams.size());
#endif
    auto cdata = &dataConstToAdd[did];
    for (auto it = bdata, ite = it + bindless_count; it != ite; ++it)
    {
      cdata[it->constIdx].x = bitwise_cast<float, uint32_t>(uniqBindlessTex[it->texId].bindlessId);
      cdata[it->constIdx].y = bitwise_cast<float, uint32_t>(register_bindless_sampler(uniqBindlessTex[it->texId].texId));
    }
    uint32_t id = states.size();
    states.push_back(BindlessState{bid, did, bindless_count, consts_count});
    uint32_t bufferId = 0;
    if (!usePackedConstants)
    {
      G_FAST_ASSERT(allConstBuf.size() == id);
      allConstBuf.push_back(nullptr);
    }
    else
    {
      bufferId = ConstantsContainer::idToIndex(did) / MAX_CONST_BUFFER_SIZE;
      packedConstBuf[packedId].resize(bufferId + 1);
      bufferNeedsUpdate[packedId].set(bufferId);
    }
    return PackedConstsState(stcode_id < 0 ? -1 : packedId, bufferId, id);
  }
  static void clear()
  {
    dataConsts.clear();
    decltype(bindlessConstParams)().swap(bindlessConstParams);
    decltype(all)().swap(all);
    all.push_back(BindlessState{0, 0, 0, 0});
    eastl::for_each(begin(allConstBuf), end(allConstBuf), [](auto &s) { destroy_d3dres(s); });
    decltype(allConstBuf)().swap(allConstBuf);
    allConstBuf.push_back(nullptr);
    packedDataConsts.clear();
    for (auto &buffers : packedConstBuf)
      for (auto &buffer : buffers)
        destroy_d3dres(buffer);
    decltype(packedConstBuf)().swap(packedConstBuf);
    decltype(packedAll)().swap(packedAll);
    if (uniqBindlessTex.size())
    {
      eastl::fixed_vector<eastl::pair<uint32_t, uint32_t>, 1, true, framemem_allocator> ranges;
      int rfirst = -1, rlast = -1;
      eastl::for_each(begin(uniqBindlessTex), end(uniqBindlessTex), [&](const BindlessTexRecord &btr) {
        if (rfirst < 0)
          rfirst = rlast = (int)btr.bindlessId;
        else if (rlast + 1 == (int)btr.bindlessId)
          ++rlast;
        else
        {
          ranges.emplace_back(rfirst, (rlast - rfirst + 1));
          rfirst = -1;
        }
      });
      if (rfirst >= 0)
        ranges.emplace_back(rfirst, (rlast - rfirst + 1));
      for (int i = ranges.size() - 1; i >= 0; --i)
        d3d::free_bindless_resource_range(RES3D_TEX, ranges[i].first, ranges[i].second);
      decltype(uniqBindlessTex)().swap(uniqBindlessTex);
    }
    decltype(stcodeIdToPacked)().swap(stcodeIdToPacked);
  }

  static void preparePackedConstBuf(ConstStateIdx idx)
  {
    PackedConstsState stateId(idx);
    const int packedId = stateId.getPackedStcodeId();
    G_ASSERT(packedId != -1);

    const BindlessState &state = packedAll[packedId][stateId.getGlobalStateId()];
    auto &constBuffer = packedConstBuf[packedId][stateId.getBufferId()];
    if (!interlocked_acquire_load_ptr(constBuffer) && state.constsCount)
    {
      String s(0, "staticCbuf%d_%d", packedId, stateId.getBufferId());
      Sbuffer *constBuf = create_static_cb(MAX_CONST_BUFFER_SIZE, s.c_str());
      if (!constBuf)
      {
        logerr("Could not create Sbuffer for const buf.");
        return;
      }
      if (DAGOR_UNLIKELY(interlocked_compare_exchange_ptr(constBuffer, constBuf, (Sbuffer *)nullptr) != nullptr))
        constBuf->destroy(); // unlikely case when other thread created/written buffer for this slot first
    }
  }

  static void prepareConstBuf(ConstStateIdx const_state_idx)
  {
    const auto idx = eastl::to_underlying(const_state_idx);
    const BindlessState &state = all[idx];
    if (!interlocked_acquire_load_ptr(allConstBuf[idx]) && state.constsCount)
    {
      String s(0, "staticCbuf%d", idx);
      Sbuffer *constBuf = create_static_cb(state.constsCount, s.c_str());
      if (!constBuf)
      {
        logerr("Could not create Sbuffer for const buf.");
        return;
      }
      constBuf->updateData(0, state.constsCount * sizeof(Point4), &dataConsts[state.constsStart], VBLOCK_DISCARD);
      if (DAGOR_UNLIKELY(interlocked_compare_exchange_ptr(allConstBuf[idx], constBuf, (Sbuffer *)nullptr) != nullptr))
        constBuf->destroy(); // unlikely case when other thread created/written buffer for this slot first
    }
  }

  static int find_or_add_bindless_tex(TEXTUREID tid, added_bindless_textures_t &addedBindlessTextures)
  {
    OSSpinlockScopedLock lock(mutex);
    auto it =
      eastl::find_if(begin(uniqBindlessTex), end(uniqBindlessTex), [=](const BindlessTexRecord &rec) { return rec.texId == tid; });
    if (it != end(uniqBindlessTex))
      return eastl::distance(begin(uniqBindlessTex), it);
    addedBindlessTextures.push_back((uint32_t)tid);
    return -1;
  }

private:
  static void update_tex_record_bindless_slot_to_null(BindlessTexRecord &tex_record, uint32_t frame_no)
  {
    if (tex_record.lastFrame != 0)
      return;
    d3d::update_bindless_resources_to_null(RES3D_TEX, tex_record.bindlessId, 1);
    tex_record.lastFrame = frame_no;
  }
};

ShaderStateBlock create_bindless_state(const BindlessConstParams *bindless_data, uint8_t bindless_count, const Point4 *consts_data,
  uint8_t consts_count, dag::Span<uint32_t> added_bindless_textures, bool static_block, int stcode_id)
{
  ShaderStateBlock block;
  block.constIdx = BindlessState::create(bindless_data, bindless_count, consts_data, consts_count, added_bindless_textures, stcode_id);
  if (static_block)
  {
    if (stcode_id < 0)
      BindlessState::prepareConstBuf(block.constIdx);
    else
      BindlessState::preparePackedConstBuf(block.constIdx);
  }
  return block;
}

void apply_bindless_state(ConstStateIdx const_state_idx, int tex_level)
{
  const PackedConstsState state(const_state_idx);
  const int packedStcodeId = state.getPackedStcodeId();
  if (packedStcodeId >= 0)
    packedAll[packedStcodeId][state.getGlobalStateId()].applyPackedMaterial(const_state_idx);
  else
    all[state.getGlobalStateId()].apply(const_state_idx, tex_level);
}

void clear_bindless_states() { BindlessState::clear(); }

void req_tex_level_bindless(ConstStateIdx const_state_idx, int tex_level)
{
  const PackedConstsState state(const_state_idx);
  const int packedStcodeId = state.getPackedStcodeId();
  if (packedStcodeId < 0)
    all[state.getGlobalStateId()].reqTexLevel(tex_level);
}

int find_or_add_bindless_tex(TEXTUREID tid, added_bindless_textures_t &added_bindless_textures)
{
  return BindlessState::find_or_add_bindless_tex(tid, added_bindless_textures);
}

void dump_bindless_states_stat() { debug(" %d bindless states (%d total)", all.size(), dataConsts.getTotalCount()); }

void update_bindless_state(ConstStateIdx const_state_idx, int tex_level)
{
  const PackedConstsState state(const_state_idx);
  G_ASSERT(state.getPackedStcodeId() >= 0);
  packedAll[state.getPackedStcodeId()][state.getGlobalStateId()].updateTexForPackedMaterial(const_state_idx, tex_level);
}

uint32_t PackedConstsState::getLocalStateOffset() const
{
  const uint32_t constsCount = packedAll[getPackedStcodeId()][getGlobalStateId()].constsCount;
  if (!constsCount)
    return 0;
  // NOTE: the secret knowledge here is that for each packedStcodeId we have a set of materials with
  // different const params which all share the same stcode and hence constsCount (although the contents of
  // the consts are different), so we pack them all into a set of buffers, each of which is MAX_CONST_BUFFER_SIZE,
  // and then interpret these buffers as a T[MAX_CONST_BUFFER_SIZE/constsCount] inside of shaders, where
  // sizeof(T) = constsCount. A "local offset" is the offset into this structured buffer that should be used
  // by shaders, and it is calculated as follows.
  return getGlobalStateId() - (MAX_CONST_BUFFER_SIZE / constsCount) * getBufferId();
}

uint32_t get_material_offset(ConstStateIdx const_state_idx)
{
  const PackedConstsState state(const_state_idx);
  G_ASSERT(state.getPackedStcodeId() >= 0);
  return state.getLocalStateOffset();
}

uint32_t get_material_id(ConstStateIdx const_state_idx)
{
  const PackedConstsState state(const_state_idx);
  return state.getMaterialId();
}

bool is_packed_material(ConstStateIdx const_state_idx) { return PLATFORM_HAS_BINDLESS && get_material_id(const_state_idx) != 0; }

void reset_bindless_samplers_in_all_collections()
{
  for (uint32_t collectionId = 0; collectionId < packedAll.size(); ++collectionId)
    for (auto &state : packedAll[collectionId])
      state.resetPackedSamplers(collectionId);
  for (auto [idx, state] : enumerate(all))
    state.resetSamplers(idx);
}
