#include <shaders/dag_shStateBlockBindless.h>
#include "shStateBlock.h"
#include <EASTL/deque.h>
#include <EASTL/bitset.h>
#include <3d/dag_drv3d_buffers.h>
#include <3d/dag_lockSbuffer.h>


struct BindlessTexRecord
{
  TEXTUREID texId;
  uint32_t bindlessId;
  uint32_t lastFrame;
  uint8_t bestReqLevel;
};

struct BindlessState;

const uint32_t REGS_IN_CONST_BUF = 4096;
const uint32_t MAX_CONST_BUFFER_SIZE = REGS_IN_CONST_BUF;
static_assert(MAX_CONST_BUFFER_SIZE <= REGS_IN_CONST_BUF, "Const buffer should be less than max value available by graphics APIs.");

#if _TARGET_C1 || _TARGET_C2

#else
const int32_t INVALID_SAMPLER_ID = -1;
#endif

// TODO: generalize this to custom container that verifies invariant of non relocation on insertion
using ConstantsContainer = eastl::deque<Point4, EASTLAllocatorType, 4096>;
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

enum PackedConstsStateBits
{
  PACKED_STCODE_BITS = 12,
  BUFFER_BITS = 7,
  LOCAL_STATE_BITS = 13
};

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
    packedDataConsts.resize(cellId + 1);
    packedConstBuf.resize(cellId + 1);
    packedAll.resize(cellId + 1);
    bufferNeedsUpdate.resize(cellId + 1);
    G_ASSERTF(cellId < INVALID_MAT_ID, "!(%d < %d)", cellId, INVALID_MAT_ID);
    stcodeIdToPacked[stcode_id] = cellId;
  }

  return stcodeIdToPacked[stcode_id];
}

class PackedConstsState
{
  static_assert(PACKED_STCODE_BITS + BUFFER_BITS + LOCAL_STATE_BITS == 32, "We want to use the whole uint32_t for state id.");

  uint32_t stateId;

public:
  PackedConstsState(int packed_stcode_id, uint32_t buffer_id, uint32_t local_state_id)
  {
    G_ASSERT(
      packed_stcode_id < (1 << PACKED_STCODE_BITS) && buffer_id < (1 << BUFFER_BITS) && local_state_id < (1 << LOCAL_STATE_BITS));
    stateId = (packed_stcode_id < 0)
                ? local_state_id
                : (((((uint32_t)(packed_stcode_id + 1) << BUFFER_BITS) | buffer_id) << LOCAL_STATE_BITS) | local_state_id);
  }

  PackedConstsState(uint32_t state) : stateId(state) {}

  int getPackedStcodeId() const { return (int)(stateId >> (LOCAL_STATE_BITS + BUFFER_BITS)) - 1; }

  uint32_t getGlobalStateId() const { return stateId & ((1 << LOCAL_STATE_BITS) - 1); }

  uint32_t getLocalStateId() const;

  uint32_t getBufferId() const { return (stateId >> LOCAL_STATE_BITS) & ((1 << BUFFER_BITS) - 1); }

  uint32_t getMaterialId() const { return (stateId >> LOCAL_STATE_BITS) & ((1 << (PACKED_STCODE_BITS + BUFFER_BITS)) - 1); }

  operator uint32_t() const { return stateId; }
};

struct BindlessState
{
  uint32_t bindlessTexStart, constsStart;
  uint8_t bindlessTexCount, constsCount;

  void updateTexForPackedMaterial(uint32_t self_idx, int tex_level)
  {
    PackedConstsState stateId(self_idx);
    const int packedId = stateId.getPackedStcodeId();
    G_ASSERT(packedId != -1);

    Sbuffer *constBuf = packedConstBuf[packedId][stateId.getBufferId()];
    if (!constBuf)
      return;
    const uint32_t frameNo = bindlessTexCount == 0 ? 0 : dagor_frame_no(); // We use frame no only if bindless textures are used, so
                                                                           // set it to 0 otherwise.
    for (auto it = begin(bindlessConstParams) + bindlessTexStart, ite = it + bindlessTexCount; it != ite; ++it)
    {
      const BindlessConstParams &constParams = *it;
      BindlessTexRecord &texRecord = uniqBindlessTex[constParams.texId];
      if (texRecord.texId == BAD_TEXTUREID)
        continue;

      if (texRecord.lastFrame != frameNo)
      {
        d3d::update_bindless_resource(texRecord.bindlessId, D3dResManagerData::getBaseTex(texRecord.texId));
        texRecord.lastFrame = frameNo;
        texRecord.bestReqLevel = 0;
      }
      if (tex_level > texRecord.bestReqLevel)
      {
        mark_managed_tex_lfu(texRecord.texId, tex_level);
        texRecord.bestReqLevel = tex_level;
      }

      IPoint4 &bindlessConsts = *reinterpret_cast<IPoint4 *>(&packedDataConsts[packedId][constsStart + constParams.constIdx]);
      if (EASTL_UNLIKELY(bindlessConsts.y <= INVALID_SAMPLER_ID))
      {
        bindlessConsts.y = d3d::register_bindless_sampler(D3dResManagerData::getBaseTex(texRecord.texId));
        bufferNeedsUpdate[packedId].set(stateId.getBufferId());
      }
    }
  }

  void resetPackedSamplers(uint32_t collection_id)
  {
    for (auto it = begin(bindlessConstParams) + bindlessTexStart, ite = it + bindlessTexCount; it != ite; ++it)
    {
      IPoint4 &bindlessConsts = *reinterpret_cast<IPoint4 *>(&packedDataConsts[collection_id][constsStart + it->constIdx]);
      bindlessConsts.y = INVALID_SAMPLER_ID;
    }
  }

  void resetSamplers()
  {
    for (auto it = begin(bindlessConstParams) + bindlessTexStart, ite = it + bindlessTexCount; it != ite; ++it)
    {
      IPoint4 &bindlessConsts = *reinterpret_cast<IPoint4 *>(&dataConsts[constsStart + it->constIdx]);
      bindlessConsts.y = INVALID_SAMPLER_ID;
    }
  }

  void applyPackedMaterial(uint32_t self_idx)
  {
    PackedConstsState stateId(self_idx);
    const int packedId = stateId.getPackedStcodeId();
    G_ASSERT(packedId != -1);

    Sbuffer *constBuf = packedConstBuf[packedId][stateId.getBufferId()];
    if (!constBuf)
      return;

    if (bufferNeedsUpdate[packedId].test(stateId.getBufferId()))
    {
      const uint32_t beginIdx = stateId.getBufferId() * MAX_CONST_BUFFER_SIZE;
      const uint32_t endIdx = min((stateId.getBufferId() + 1) * MAX_CONST_BUFFER_SIZE, (uint32_t)packedDataConsts[packedId].size());

      // TODO: Change const buffers container to be able to call updateData here, because data for one const buffer is in continuous
      // piece of memory
      if (auto bufData = lock_sbuffer<Point4>(constBuf, 0, endIdx - beginIdx, VBLOCK_DISCARD))
        bufData.updateDataRange(0, packedDataConsts[packedId].begin() + beginIdx, endIdx - beginIdx);
      bufferNeedsUpdate[packedId].reset(stateId.getBufferId());
    }

    d3d::set_const_buffer(STAGE_VS, 1, constBuf);
    d3d::set_const_buffer(STAGE_PS, 1, constBuf);
  }

  void apply(uint32_t self_idx, int tex_level)
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
    for (auto it = begin(bindlessConstParams) + bindlessTexStart, ite = it + bindlessTexCount; it != ite; ++it)
    {
      const BindlessConstParams &constParams = *it;
      BindlessTexRecord &texRecord = uniqBindlessTex[constParams.texId];
      if (texRecord.texId == BAD_TEXTUREID)
        continue;

      mark_managed_tex_lfu(texRecord.texId, tex_level);
      if (texRecord.lastFrame != frameNo)
      {
        d3d::update_bindless_resource(texRecord.bindlessId, D3dResManagerData::getBaseTex(texRecord.texId));
        texRecord.lastFrame = frameNo;
        texRecord.bestReqLevel = 0;
      }
      if (tex_level > texRecord.bestReqLevel)
      {
        mark_managed_tex_lfu(texRecord.texId, tex_level);
        texRecord.bestReqLevel = tex_level;
      }

      IPoint4 &bindlessConsts = *reinterpret_cast<IPoint4 *>(&dataConsts[constsStart + constParams.constIdx]);
      if (EASTL_UNLIKELY(bindlessConsts.y <= INVALID_SAMPLER_ID))
      {
        requireToUpdateConstBuf = true;
        bindlessConsts.y = d3d::register_bindless_sampler(D3dResManagerData::getBaseTex(texRecord.texId));
      }
    }

    if (requireToUpdateConstBuf)
    {
      if (auto bufData = lock_sbuffer<Point4>(constBuf, 0, constsCount, VBLOCK_DISCARD))
        bufData.updateDataRange(0, dataConsts.begin() + constsStart, constsCount);
    }

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

  static uint32_t create(const BindlessConstParams *bindless_data, uint8_t bindless_count, const Point4 *consts_data,
    uint8_t consts_count, dag::Span<uint32_t> addedBindlessTextures, int stcode_id = -1)
  {
    G_FAST_ASSERT(stcode_id >= -1);

    OSSpinlockScopedLock lock(mutex);
    const bool usePackedConstants = stcode_id >= 0;

    uint32_t packedId = usePackedConstants ? allocate_packed_cell(stcode_id) : INVALID_MAT_ID;
    auto &dataConstToAdd = usePackedConstants ? packedDataConsts[packedId] : dataConsts;
    auto &states = usePackedConstants ? packedAll[packedId] : all;
    const uint32_t cBufferAlignment = usePackedConstants ? MAX_CONST_BUFFER_SIZE : 1;
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
        if (
          !eastl::equal(consts_data + beginConst, consts_data + endConst, begin(dataConstToAdd) + c.constsStart + beginConst, p4btcmp))
          return false;
        beginConst = endConst + 1;
      }
      return eastl::equal(consts_data + beginConst, consts_data + consts_count, begin(dataConstToAdd) + c.constsStart + beginConst,
        p4btcmp);
    });
    if (foundIt != end(states))
    {
      const uint32_t localStateId = eastl::distance(begin(states), foundIt);
      const uint32_t bufferId = consts_count == 0 ? 0 : (localStateId / (MAX_CONST_BUFFER_SIZE / consts_count));
      if (consts_count != 0 && stcode_id != -1)
        bufferNeedsUpdate[packedId].set(bufferId);
      return PackedConstsState(stcode_id == -1 ? -1 : packedId, bufferId, localStateId);
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
    uint32_t did = dataConstToAdd.size();
#if DAGOR_DBGLEVEL > 0
    auto prevPtrArray = dataConstToAdd.mpPtrArray;
#endif
    if ((did + consts_count) % cBufferAlignment != 0 && did / cBufferAlignment != (did + consts_count) / cBufferAlignment)
    {
      dataConstToAdd.resize((dataConstToAdd.size() + cBufferAlignment - 1) / cBufferAlignment * cBufferAlignment);
      did = dataConstToAdd.size();
    }
    dataConstToAdd.insert(dataConstToAdd.end(), consts_data, consts_data + consts_count);
#if DAGOR_DBGLEVEL > 0
    // If deque's subarrays gets re-allocated then all thread safety guarantees are lost.
    // Investigate why did it happened - it's either bad merge of consts or perhaps subarray size need to be increased.
    if (EASTL_UNLIKELY(dataConstToAdd.mpPtrArray != prevPtrArray))
      logerr("Overflow thread-safe bindless data consts limit @ %d, subArraySize=%d, states[%d].size=%d, bindlessConstParams.size=%d",
        dataConstToAdd.size(), dataConstToAdd.kSubarraySize, packedId, states.size(), bindlessConstParams.size());
#endif
    auto cdata = begin(dataConstToAdd) + did;
    for (auto it = bdata, ite = it + bindless_count; it != ite; ++it)
    {
      (cdata + it->constIdx)->x = bitwise_cast<float, uint32_t>(uniqBindlessTex[it->texId].bindlessId);
#if _TARGET_C1 || _TARGET_C2


#endif
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
      if ((dataConstToAdd.size() + MAX_CONST_BUFFER_SIZE - 1) / MAX_CONST_BUFFER_SIZE > packedConstBuf[packedId].size() ||
          packedConstBuf[packedId].empty())
        packedConstBuf[packedId].push_back(nullptr);
      bufferId = packedConstBuf[packedId].size() - 1;
      bufferNeedsUpdate[packedId].set(bufferId);
    }
    return PackedConstsState(stcode_id == -1 ? -1 : packedId, bufferId, id);
  }
  static void clear()
  {
    decltype(dataConsts)().swap(dataConsts);
    decltype(bindlessConstParams)().swap(bindlessConstParams);
    decltype(all)().swap(all);
    all.push_back(BindlessState{0, 0, 0, 0});
    eastl::for_each(begin(allConstBuf), end(allConstBuf), [](auto &s) { destroy_d3dres(s); });
    decltype(allConstBuf)().swap(allConstBuf);
    allConstBuf.push_back(nullptr);
    decltype(packedDataConsts)().swap(packedDataConsts);
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

  static void preparePackedConstBuf(uint32_t idx)
  {
    PackedConstsState stateId(idx);
    const int packedId = stateId.getPackedStcodeId();
    G_ASSERT(packedId != -1);

    const BindlessState &state = packedAll[packedId][stateId.getGlobalStateId()];
    auto &constBuffer = packedConstBuf[packedId][stateId.getBufferId()];
    if (!interlocked_acquire_load_ptr(constBuffer) && state.constsCount)
    {
      String s(0, "staticCbuf%d_%d", packedId, stateId.getBufferId());
      Sbuffer *constBuf = d3d::buffers::create_persistent_cb(MAX_CONST_BUFFER_SIZE, s.c_str());
      if (!constBuf)
      {
        logerr("Could not create Sbuffer for const buf.");
        return;
      }
      if (EASTL_UNLIKELY(interlocked_compare_exchange_ptr(constBuffer, constBuf, (Sbuffer *)nullptr) != nullptr))
        constBuf->destroy(); // unlikely case when other thread created/written buffer for this slot first
    }
  }

  static void prepareConstBuf(int idx)
  {
    const BindlessState &state = all[idx];
    if (!interlocked_acquire_load_ptr(allConstBuf[idx]) && state.constsCount)
    {
      String s(0, "staticCbuf%d", idx);
      Sbuffer *constBuf = d3d::buffers::create_persistent_cb(state.constsCount, s.c_str());
      if (!constBuf)
      {
        logerr("Could not create Sbuffer for const buf.");
        return;
      }
      if (auto bufData = lock_sbuffer<Point4>(constBuf, 0, state.constsCount, VBLOCK_DISCARD))
        bufData.updateDataRange(0, dataConsts.begin() + state.constsStart, state.constsCount);
      if (EASTL_UNLIKELY(interlocked_compare_exchange_ptr(allConstBuf[idx], constBuf, (Sbuffer *)nullptr) != nullptr))
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

void apply_bindless_state(uint32_t const_state_idx, int tex_level)
{
  const PackedConstsState state(const_state_idx);
  const int packedStcodeId = state.getPackedStcodeId();
  if (packedStcodeId >= 0)
    packedAll[packedStcodeId][state.getGlobalStateId()].applyPackedMaterial(const_state_idx);
  else
    all[state.getGlobalStateId()].apply(const_state_idx, tex_level);
}

void clear_bindless_states() { BindlessState::clear(); }

void req_tex_level_bindless(uint32_t const_state_idx, int tex_level)
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

void dump_bindless_states_stat() { debug(" %d bindless states (%d total)", all.size(), dataConsts.size()); }

void update_bindless_state(uint32_t const_state_idx, int tex_level)
{
  const PackedConstsState state(const_state_idx);
  G_ASSERT(state.getPackedStcodeId() != -1);
  packedAll[state.getPackedStcodeId()][state.getGlobalStateId()].updateTexForPackedMaterial(const_state_idx, tex_level);
}

uint32_t PackedConstsState::getLocalStateId() const
{
  const uint32_t constsCount = packedAll[getPackedStcodeId()][getGlobalStateId()].constsCount;
  if (!constsCount)
    return 0;
  return getGlobalStateId() - (MAX_CONST_BUFFER_SIZE / constsCount) * getBufferId();
}

uint32_t get_material_offset(uint32_t const_state_idx)
{
  const PackedConstsState state(const_state_idx);
  G_ASSERT(state.getPackedStcodeId() != -1);
  return state.getLocalStateId();
}

uint32_t get_material_id(uint32_t const_state_idx)
{
  const PackedConstsState state(const_state_idx);
  return state.getMaterialId();
}

bool is_packed_material(uint32_t const_state_idx) { return PLATFORM_HAS_BINDLESS && get_material_id(const_state_idx) != 0; }

void reset_bindless_samplers_in_all_collections()
{
  for (uint32_t collectionId = 0; collectionId < packedAll.size(); ++collectionId)
    for (auto &state : packedAll[collectionId])
      state.resetPackedSamplers(collectionId);
  for (auto &state : all)
    state.resetSamplers();
}
