// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shStateBlockBindless.h>

#include <generic/dag_enumerate.h>
#include <EASTL/string.h>
#include <EASTL/deque.h>
#include <EASTL/bitset.h>
#include <EASTL/bit.h>
#include <EASTL/sort.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_lockSbuffer.h>
#include <vecmath/dag_vecMath.h>
#include <osApiWrappers/dag_miscApi.h>

#include "shStateBlock.h"
#include "concurrentRangePool.h"
#include "concurrentElementPool.h"

using namespace shaders;

struct BindlessTexRecord
{
  TEXTUREID texId;
  uint32_t bindlessId;
  uint32_t lastFrame;
  uint16_t lastTexStreamingGeneration;
  uint8_t bestReqLevel;
  D3DResourceType type;
};

struct BindlessState;

static constexpr uint32_t REGS_IN_CONST_BUF = 4096;
static constexpr uint32_t MAX_CONST_BUFFER_SIZE = REGS_IN_CONST_BUF;
static_assert(MAX_CONST_BUFFER_SIZE <= REGS_IN_CONST_BUF, "Const buffer should be less than max value available by graphics APIs.");
static constexpr int32_t DEFAULT_SAMPLER_ID = 0;

static constexpr uint16_t NULL_TEX_STREAMING_GENERATION = 65535;
static constexpr uint16_t INVALID_TEX_STREAMING_GENERATION = 65534;

static constexpr uint32_t PACKED_STCODE_BITS = 10;
static constexpr uint32_t BUFFER_BITS = 6;
static constexpr uint32_t GLOBAL_STATE_BITS = 16;

enum class PackedStcodeId : uint16_t
{
  Invalid = 0
};

enum class GlobalStateId : uint16_t
{
  Invalid = 0
};

enum class PackedBufferId : uint8_t
{
  Invalid = 0
};

using ConstantsContainer = ConcurrentRangePool<Point4, get_const_log2(MAX_CONST_BUFFER_SIZE), 8, true>;

// NOTE 9+6 <= GLOBAL_STATE_BITS because IDs from these need to fit there
using StatesContainer = ConcurrentElementPool<GlobalStateId, BindlessState, 9, 6>;
using BuffersContainer = ConcurrentElementPool<GlobalStateId, Sbuffer *, 9, 6>;

static ConstantsContainer dataConsts;
static StatesContainer all;          // first one is default
static BuffersContainer allConstBuf; // parallel to all
using BindlessConstParamsStorage = ConcurrentRangePool<BindlessConstParams, get_const_log2(MAX_CONST_BUFFER_SIZE)>;
static BindlessConstParamsStorage bindlessConstParams;
static ConcurrentElementPool<BindlessTexId, BindlessTexRecord, 10> uniqBindlessTex;
static dag::Vector<eastl::tuple<D3DResourceType, uint32_t, uint32_t>> allocatedBindlessRanges;
static OSSpinlock mutex;

// NOTE: the "default" state is considered to be non-packed
static_assert(DEFAULT_BINDLESS_CONST_STATE_ID == static_cast<ConstStateIdx>(decltype(all)::FIRST_ID));
static_assert(DEFAULT_BINDLESS_CONST_STATE_ID == static_cast<ConstStateIdx>(decltype(allConstBuf)::FIRST_ID));

static eastl::deque<PackedStcodeId, EASTLAllocatorType, 2048> stcodeIdToPacked;

// NOTE: 2+4 <= BUFFER_BITS because IDs from this need to fit there
using PackedBuffersContainer = ConcurrentElementPool<PackedBufferId, Sbuffer *, 2, 4>;

// NOTE: 5+5 <= PACKED_STCODE_BITS because IDs from these need to fit there
template <typename T>
using PackedCont = ConcurrentElementPool<PackedStcodeId, T, 5, 5>;

static PackedCont<StatesContainer> packedAll;
static PackedCont<ConstantsContainer> packedDataConsts;
static PackedCont<PackedBuffersContainer> packedConstBuf;

namespace packed_buffs
{
// NOTE: for each const buffer, we store the value K -- length of a prefix
// which is out of date (invalid) on the GPU and needs to be re-uploaded.
// The value is in [0, 4096] range so we need 13 bits...
static PackedCont<eastl::array<eastl::atomic<uint16_t>, 1 << BUFFER_BITS>> gpuInvalidPrefix;
// We also store the length of the prefix valid on the CPU side to be able to invalidate
// the entire (usable part of the) buffer if needed.
static PackedCont<eastl::array<uint16_t, 1 << BUFFER_BITS>> cpuValidPrefix;

// NOTE: this is ONLY called under the shader_internal mutex, and always with monotonically increasing
// prefix_length, so we don't need atomic max or anything like that.
static void invalidate_prefix(PackedStcodeId packed_stcode_id, PackedBufferId buffer_id, uint16_t prefix_length)
{
  const auto bufIndex = PackedBuffersContainer::to_index(buffer_id);
  cpuValidPrefix[packed_stcode_id][bufIndex] = prefix_length;
  gpuInvalidPrefix[packed_stcode_id][bufIndex].store(prefix_length, eastl::memory_order_release);
}

// NOTE: this is also only called under the shader_internal mutex, but also only
// on the main thread (as it mutates already created constants).
static void invalidate_all(PackedStcodeId packed_stcode_id, PackedBufferId buffer_id)
{
  const auto bufIndex = PackedBuffersContainer::to_index(buffer_id);
  const auto observedValidPrefix = cpuValidPrefix[packed_stcode_id][bufIndex];
  // Release is not needed here because invalidate_all and snatch_invalid_prefix
  // are always called on the main thread.
  gpuInvalidPrefix[packed_stcode_id][bufIndex].store(observedValidPrefix, eastl::memory_order_relaxed);
}

// This can be called without a lock & at any point in time whatsoever, but only on the main thread,
// so we need to ACQUIRE the value to synchronize the last* non-atomic writes into the CPU version
// of the const buffer (packedDataConsts) through `invalidate_prefix` which we will read & upload to the GPU.
// * the word "last" only makes sense because modifications are done under a lock.
static uint16_t snatch_invalid_prefix(PackedStcodeId packed_stcode_id, PackedBufferId buffer_id)
{
  const auto bufIndex = PackedBuffersContainer::to_index(buffer_id);
  return gpuInvalidPrefix[packed_stcode_id][bufIndex].exchange(0, eastl::memory_order_acquire);
}

} // namespace packed_buffs

static ConstantsContainer::ChunkIdx packed_buffer_id_to_chunk(PackedBufferId buffer_id)
{
  return static_cast<ConstantsContainer::ChunkIdx>(PackedBuffersContainer::to_index(buffer_id));
}

static PackedStcodeId allocate_packed_cell(int stcode_id)
{
  G_FAST_ASSERT(stcode_id >= 0);

  if (stcodeIdToPacked.size() <= stcode_id)
    stcodeIdToPacked.resize(stcode_id + 1, PackedStcodeId::Invalid);

  if (stcodeIdToPacked[stcode_id] == PackedStcodeId::Invalid)
  {
    const auto cellId = packedDataConsts.allocate();
    packedConstBuf.ensureSpace(cellId);
    packedAll.ensureSpace(cellId);
    packed_buffs::gpuInvalidPrefix.ensureSpace(cellId);
    packed_buffs::cpuValidPrefix.ensureSpace(cellId);
    stcodeIdToPacked[stcode_id] = cellId;
  }

  return stcodeIdToPacked[stcode_id];
}

static uint32_t register_bindless_sampler(TEXTUREID tid, bool after_reset = false)
{
  auto sampler = get_texture_separate_sampler(tid);

  if (sampler == d3d::INVALID_SAMPLER_HANDLE)
  {
    G_ASSERT(tid == BAD_TEXTUREID);
    return uint32_t(-1);
  }

  return d3d::register_bindless_sampler(sampler);
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
  PackedConstsState(PackedStcodeId packed_stcode_id, PackedBufferId buffer_id, GlobalStateId global_state_id)
  {
    const uint32_t packedStcodeId = eastl::to_underlying(packed_stcode_id);
    const uint32_t bufferId = eastl::to_underlying(buffer_id);
    const uint32_t globalStateId = eastl::to_underlying(global_state_id);

    G_ASSERTF(packedStcodeId < (1 << PACKED_STCODE_BITS), "PackedConstsState: packed_stcode_id=%u", packedStcodeId);
    G_ASSERTF(bufferId < (1 << BUFFER_BITS), "PackedConstsState: buffer_id=%u", bufferId);
    G_ASSERTF(globalStateId < (1 << GLOBAL_STATE_BITS),
      "PackedConstsState: local_state_id=%u. (Hint: check if too many materials are created!)", globalStateId);

    stateId = (packed_stcode_id == PackedStcodeId::Invalid)
                ? globalStateId
                : (((((uint32_t)(packedStcodeId) << BUFFER_BITS) | bufferId) << GLOBAL_STATE_BITS) | globalStateId);
  }

  /*implicit*/ PackedConstsState(ConstStateIdx state) : stateId(eastl::to_underlying(state)) {}

  PackedStcodeId getPackedStcodeId() const { return static_cast<PackedStcodeId>((stateId >> (GLOBAL_STATE_BITS + BUFFER_BITS))); }

  GlobalStateId getGlobalStateId() const { return static_cast<GlobalStateId>(stateId & ((1 << GLOBAL_STATE_BITS) - 1)); }

  uint32_t getLocalStateOffset() const;

  PackedBufferId getBufferId() const { return static_cast<PackedBufferId>((stateId >> GLOBAL_STATE_BITS) & ((1 << BUFFER_BITS) - 1)); }

  uint32_t getMaterialId() const { return (stateId >> GLOBAL_STATE_BITS) & ((1 << (PACKED_STCODE_BITS + BUFFER_BITS)) - 1); }

  operator ConstStateIdx() const { return static_cast<ConstStateIdx>(stateId); }
};

struct BindlessState
{
  BindlessConstParamsStorage::Range params;
  ConstantsContainer::Range consts;

  template <typename IterCb>
  void iterateBindlessRange(ConstantsContainer &container, const IterCb &callback)
  {
    const auto paramsView = bindlessConstParams.view(params);
    const auto constsView = container.view(consts);

    for (const auto &params : paramsView)
    {
      BindlessTexRecord &texRecord = uniqBindlessTex[params.texId];
      IPoint4 &bindlessConsts = *reinterpret_cast<IPoint4 *>(&constsView[params.constIdx]);
      callback(texRecord, bindlessConsts);
    }
  }

  void updateBindlessResources(ConstantsContainer &container, int tex_level)
  {
    // We use frame no only if bindless textures are used, so set it to 0 otherwise.
    const uint32_t frameNo = BindlessConstParamsStorage::range_size(params) == 0 ? 0 : dagor_frame_no();

    iterateBindlessRange(container, [frameNo, tex_level](BindlessTexRecord &tex_record, IPoint4 &) {
      const auto &texId = tex_record.texId;
      if (texId == BAD_TEXTUREID)
      {
        update_tex_record_bindless_slot_to_null(tex_record);
        return;
      }

      if (frameNo != tex_record.lastFrame)
      {
        constexpr bool perform_missed_update_check = (DAGOR_DBGLEVEL > 1);

        uint16_t texStreamingGeneration = get_managed_texture_streaming_generation(texId);
        bool isUpdateRequested = (tex_record.lastTexStreamingGeneration != texStreamingGeneration);

        if (isUpdateRequested || perform_missed_update_check)
        {
          BaseTexture *baseTex = D3dResManagerData::getBaseTex(tex_record.texId);
          if (DAGOR_UNLIKELY(baseTex == nullptr))
          {
            update_tex_record_bindless_slot_to_null(tex_record);
            LOGERR_ONCE("Couldn't update bindless slot %d with texture (tid=%d, idx=%d): BaseTex is null.", tex_record.bindlessId,
              texId, texId.index());
            return;
          }

          bool isUpdatePerformed = d3d::update_bindless_resource(tex_record.type, tex_record.bindlessId, baseTex);
          if (perform_missed_update_check)
          {
            bool isInternalUpdateCacheSupported = d3d::get_driver_code().is(d3d::dx12 || d3d::metal || d3d::anyPS);
            if (isUpdatePerformed && !isUpdateRequested && isInternalUpdateCacheSupported)
            {
              logerr("Bindless texture update is required, but not requested for '%s'", baseTex->getTexName());
            }
          }

          tex_record.lastTexStreamingGeneration = texStreamingGeneration;
        }

        tex_record.lastFrame = frameNo;
        tex_record.bestReqLevel = 0;
      }
      if (tex_level > tex_record.bestReqLevel)
      {
        mark_managed_tex_lfu(texId, tex_level);
        tex_record.bestReqLevel = tex_level;
      }
    });
  }

  void updateTexForPackedMaterial(ConstStateIdx self_idx, int tex_level)
  {
    PackedConstsState stateId(self_idx);
    const PackedStcodeId packedId = stateId.getPackedStcodeId();
    G_ASSERT(packedId != PackedStcodeId::Invalid);

    // Material has no constants => no bindless textures => nothing to update
    if (stateId.getBufferId() == PackedBufferId::Invalid)
      return;

    Sbuffer *constBuf = packedConstBuf[packedId][stateId.getBufferId()];
    if (!constBuf)
      return;

    updateBindlessResources(packedDataConsts[packedId], tex_level);

    {
      const auto invalidPrefix = packed_buffs::snatch_invalid_prefix(packedId, stateId.getBufferId());
      if (invalidPrefix > 0)
      {
        const auto &dataConsts = packedDataConsts[packedId];
        const auto chunk = packed_buffer_id_to_chunk(stateId.getBufferId());
        const auto chunkView = dataConsts.viewChunk(chunk);
        const auto validConsts = chunkView.subspan(0, invalidPrefix);
        G_FAST_ASSERT(validConsts.size() <= MAX_CONST_BUFFER_SIZE);
        constBuf->updateData(0, validConsts.size_bytes(), validConsts.data(), VBLOCK_DISCARD);
      }
    }
  }

  void resetPackedSamplers(PackedStcodeId collection_id)
  {
    iterateBindlessRange(packedDataConsts[collection_id], [](BindlessTexRecord &tex_record, IPoint4 &bindless_consts) {
      bindless_consts.y = register_bindless_sampler(tex_record.texId, true);
    });

    packedConstBuf[collection_id].iterateWithIds([collection_id](PackedBufferId bufferId, Sbuffer *buf) {
      if (buf)
        packed_buffs::invalidate_all(collection_id, bufferId);
    });
  }

  void resetSamplers(GlobalStateId self_idx)
  {
    Sbuffer *constBuf = allConstBuf[self_idx];
    if (!constBuf)
      return;
    iterateBindlessRange(dataConsts, [](BindlessTexRecord &tex_record, IPoint4 &bindless_consts) {
      bindless_consts.y = register_bindless_sampler(tex_record.texId, true);
    });
    const auto constsView = dataConsts.view(consts);
    if (!constsView.empty())
      constBuf->updateData(0, constsView.size_bytes(), constsView.data(), VBLOCK_DISCARD);
  }

  void applyPackedMaterial(ConstStateIdx self_idx)
  {
    PackedConstsState stateId(self_idx);
    const PackedStcodeId packedId = stateId.getPackedStcodeId();
    G_ASSERT(packedId != PackedStcodeId::Invalid);

    // Material has no constants
    if (stateId.getBufferId() == PackedBufferId::Invalid)
      return;

    Sbuffer *constBuf = packedConstBuf[packedId][stateId.getBufferId()];
    if (!constBuf)
      return;

    d3d::set_const_buffer(STAGE_VS, 1, constBuf);
    d3d::set_const_buffer(STAGE_PS, 1, constBuf);
  }

  void apply(ConstStateIdx self_idx, int tex_level)
  {
    PackedConstsState stateId(self_idx);
    G_ASSERT(stateId.getPackedStcodeId() == PackedStcodeId::Invalid);

    Sbuffer *constBuf = allConstBuf[stateId.getGlobalStateId()];
    if (!constBuf)
      return;

    updateBindlessResources(dataConsts, tex_level);

    d3d::set_const_buffer(STAGE_VS, 1, constBuf);
    d3d::set_const_buffer(STAGE_PS, 1, constBuf);
  }

  void reqTexLevel(int tex_level)
  {
    const auto paramsView = bindlessConstParams.view(params);
    for (const auto &params : paramsView)
    {
      BindlessTexRecord &texRecord = uniqBindlessTex[params.texId];
      if (texRecord.texId == BAD_TEXTUREID)
        continue;

      if (tex_level > texRecord.bestReqLevel)
      {
        mark_managed_tex_lfu(texRecord.texId, tex_level);
        texRecord.bestReqLevel = tex_level;
      }
    }
  }

  static ConstStateIdx create(dag::ConstSpan<BindlessConstParams> bindless, dag::ConstSpan<Point4> consts,
    dag::ConstSpan<uint32_t> added_bindless_textures, int stcode_id)
  {
    G_FAST_ASSERT(stcode_id >= -1);
    G_FAST_ASSERT(bindless.size() < eastl::numeric_limits<uint8_t>::max());
    G_FAST_ASSERT(consts.size() < eastl::numeric_limits<uint8_t>::max());

    OSSpinlockScopedLock lock(mutex);
    const bool usePackedConstants = stcode_id >= 0;

    const PackedStcodeId packedId = usePackedConstants ? allocate_packed_cell(stcode_id) : PackedStcodeId::Invalid;
    auto &constsStorage = usePackedConstants ? packedDataConsts[packedId] : dataConsts;
    auto &paramsStorage = usePackedConstants ? packedAll[packedId] : all;

    {
      const auto found = paramsStorage.findIf([&](const BindlessState &c) {
        const auto paramsView = bindlessConstParams.view(c.params);
        const auto constsView = constsStorage.view(c.consts);

        if (paramsView.size() != bindless.size() || constsView.size() != consts.size())
          return false;

        if (!eastl::equal(bindless.begin(), bindless.end(), paramsView.begin()))
          return false;

        auto p4btcmp = [](const Point4 &a, const Point4 &b) {
          // bitwise compare to be able collapse bitwise-identical NaNs
          return v_signmask(v_cmp_eqi(v_ld(&a.x), v_ld(&b.x))) == 15;
        };
        uint32_t beginConst = 0;
        // if there are exist bindless resources we compare all dataConstToAdd ranges EXCEPT bindless ones,
        // since it might not be applied yet (register_bindless_sampler isn't called)
        for (uint32_t i = 0; i < bindless.size(); ++i)
        {
          uint32_t endConst = paramsView[i].constIdx;
          if (!eastl::equal(consts.begin() + beginConst, consts.begin() + endConst, constsView.data() + beginConst, p4btcmp))
            return false;
          beginConst = endConst + 1;
        }
        return eastl::equal(consts.begin() + beginConst, consts.begin() + consts.size(), constsView.data() + beginConst, p4btcmp);
      });

      if (found != GlobalStateId::Invalid)
      {
        const PackedBufferId bufferId = //
          consts.empty()
            ? PackedBufferId::Invalid
            : PackedBuffersContainer::from_index(eastl::to_underlying(constsStorage.chunkOf(paramsStorage[found].consts)));
        return PackedConstsState(stcode_id < 0 ? PackedStcodeId::Invalid : packedId, bufferId, found);
      }
    }

    BindlessConstParamsStorage::Range ourParamsRangeFirstId;

    if (added_bindless_textures.empty())
    {
      ourParamsRangeFirstId = bindlessConstParams.allocate(bindless.size());
      const auto view = bindlessConstParams.view(ourParamsRangeFirstId);
      eastl::copy(bindless.begin(), bindless.end(), view.begin());
    }
    else
    {
      FRAMEMEM_VALIDATE;

      ourParamsRangeFirstId = bindlessConstParams.allocate(bindless.size());

      dag::Vector<eastl::pair<TEXTUREID, D3DResourceType>, framemem_allocator> added_textures_sorted(added_bindless_textures.size());
      for (size_t i = 0; i < added_bindless_textures.size(); ++i)
      {
        TEXTUREID id = TEXTUREID(added_bindless_textures[i]);
        D3dResource *baseRes = D3dResManagerData::getD3dRes(id);
        // why do we add nullptr resources?
        added_textures_sorted[i] = eastl::make_pair(id, baseRes ? baseRes->getType() : D3DResourceType::TEX);
      }
      eastl::sort(added_textures_sorted.begin(), added_textures_sorted.end(),
        [](const auto &a, const auto &b) { return a.second < b.second; });

      uint32_t tex2d_count = 0, texCube_count = 0, tex2dArray_count = 0, texVolume_count = 0;
      for (const auto &[_, type] : added_textures_sorted)
      {
        if (type == D3DResourceType::TEX)
          tex2d_count++;
        else if (type == D3DResourceType::CUBETEX)
          texCube_count++;
        else if (type == D3DResourceType::ARRTEX)
          tex2dArray_count++;
        else if (type == D3DResourceType::VOLTEX)
          texVolume_count++;
        else
          G_ASSERT(0 && "Unsupported bindless texture type");
      }

      dag::Vector<BindlessTexId, framemem_allocator> addedBindlessTexIds;

      auto fillBindlessRange = [&addedBindlessTexIds, &added_textures_sorted](D3DResourceType type, uint32_t offset, uint32_t count) {
        if (count == 0)
          return offset;
        uint32_t brange = d3d::allocate_bindless_resource_range(type, count);
        allocatedBindlessRanges.emplace_back(type, brange, count);

        for (uint32_t i = offset; i < offset + count; ++i)
        {
          G_ASSERT(type == added_textures_sorted[i].second);
          const auto texId = addedBindlessTexIds.emplace_back(uniqBindlessTex.allocate());
          uniqBindlessTex[texId] =
            BindlessTexRecord{added_textures_sorted[i].first, brange++, 0, INVALID_TEX_STREAMING_GENERATION, 0, type};
        }
        return offset + count;
      };

      // the order matters, because sort produces it in this order
      uint32_t offset = fillBindlessRange(D3DResourceType::TEX, 0, tex2d_count);
      offset = fillBindlessRange(D3DResourceType::CUBETEX, offset, texCube_count);
      offset = fillBindlessRange(D3DResourceType::VOLTEX, offset, texVolume_count);
      offset = fillBindlessRange(D3DResourceType::ARRTEX, offset, tex2dArray_count);
      G_ASSERT(offset == added_bindless_textures.size());

      {
        uint32_t j = 0;
        auto it = bindless.begin();
        const auto view = bindlessConstParams.view(ourParamsRangeFirstId);
        for (auto &bcp : view)
        {
          if (it->texId != BindlessTexId::Invalid)
            bcp = *it;
          else
            bcp = BindlessConstParams{it->constIdx, addedBindlessTexIds[j++]};
          ++it;
        }
        G_FAST_ASSERT(j == addedBindlessTexIds.size());
        G_FAST_ASSERT(it == bindless.end());
      }
    }

#ifdef _DEBUG_TAB_
    {
      const auto view = bindlessConstParams.view(ourParamsRangeFirstId);
      for (const auto &bcp : view)
        G_FAST_ASSERT(bcp.constIdx < consts.size());
    }
#endif

    const auto ourConstsRangeFirstId = constsStorage.allocate(consts.size());
    const auto constsView = constsStorage.view(ourConstsRangeFirstId);
    eastl::copy(consts.begin(), consts.end(), constsView.begin());

    {
      auto constsView = constsStorage.view(ourConstsRangeFirstId);
      auto paramsView = bindlessConstParams.view(ourParamsRangeFirstId);
      for (const auto &bcp : paramsView)
      {
        auto &tex = uniqBindlessTex[bcp.texId];
        constsView[bcp.constIdx].x = eastl::bit_cast<float>(tex.bindlessId);
        constsView[bcp.constIdx].y = eastl::bit_cast<float>(register_bindless_sampler(tex.texId));
      }
    }

    const GlobalStateId id = paramsStorage.allocate();
    paramsStorage[id] = BindlessState{ourParamsRangeFirstId, ourConstsRangeFirstId};
    PackedBufferId bufferId = PackedBufferId::Invalid;
    if (!usePackedConstants)
    {
      allConstBuf.allocate();
      G_FAST_ASSERT(allConstBuf.totalElements() == paramsStorage.totalElements());
    }
    else if (!consts.empty())
    {
      bufferId = PackedBuffersContainer::from_index(eastl::to_underlying(constsStorage.chunkOf(ourConstsRangeFirstId)));
      packedConstBuf[packedId].ensureSpace(bufferId);
      packed_buffs::invalidate_prefix(packedId, bufferId, constsStorage.offsetInChunk(ourConstsRangeFirstId) + consts.size());
    }

    if (!usePackedConstants)
    {
      if (eastl::to_underlying(id) % 64 == 0)
        debug("Non-packed global state count: %d", eastl::to_underlying(id) + 1);
    }
    else
    {
      if (eastl::to_underlying(id) % 64 == 0)
        debug("Packed (stcode ID %d) global state count: %d, buffer count %d", eastl::to_underlying(packedId),
          eastl::to_underlying(id) + 1, eastl::to_underlying(bufferId) + 1);
    }

    return PackedConstsState(stcode_id < 0 ? PackedStcodeId::Invalid : packedId, bufferId, id);
  }
  static void clear()
  {
    dataConsts.clear();
    bindlessConstParams.clear();
    all.clear();
    all[all.allocate()] = BindlessState{BindlessConstParamsStorage::Range::Empty, ConstantsContainer::Range::Empty};
    for (auto &buf : allConstBuf)
      destroy_d3dres(buf);
    allConstBuf.clear();
    allConstBuf.allocate();
    packedDataConsts.clear();
    for (auto &bufs : packedConstBuf)
      for (auto buf : bufs)
        destroy_d3dres(buf);
    packedConstBuf.clear();
    packedAll.clear();
    if (uniqBindlessTex.totalElements() > 0)
    {
      for (const auto &[type, start, count] : allocatedBindlessRanges)
        d3d::free_bindless_resource_range(type, start, count);
      allocatedBindlessRanges.clear();
      uniqBindlessTex.clear();
    }
    stcodeIdToPacked.clear();
  }

  static void preparePackedConstBuf(ConstStateIdx idx)
  {
    PackedConstsState stateId(idx);
    const PackedStcodeId packedId = stateId.getPackedStcodeId();
    G_ASSERT(packedId != PackedStcodeId::Invalid);

    if (stateId.getBufferId() == PackedBufferId::Invalid)
      return;

    const BindlessState &state = packedAll[packedId][stateId.getGlobalStateId()];
    auto &constBuffer = packedConstBuf[packedId][stateId.getBufferId()];
    if (ConstantsContainer::range_size(state.consts) > 0 && !interlocked_acquire_load_ptr(constBuffer))
    {
      String s(0, "staticCbuf%d_%d", eastl::to_underlying(packedId), eastl::to_underlying(stateId.getBufferId()));
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
    // NOTE: when packing is not used, the const_state_idx bytes are all for the global state id
    const auto idx = eastl::to_underlying(const_state_idx);
    const BindlessState &state = all[static_cast<GlobalStateId>(idx)];
    auto &constBuffer = allConstBuf[static_cast<GlobalStateId>(idx)];
    if (ConstantsContainer::range_size(state.consts) > 0 && !interlocked_acquire_load_ptr(constBuffer))
    {
      const auto constsView = dataConsts.view(state.consts);
      eastl::string s(eastl::string::CtorSprintf{}, "staticCbuf%d", idx);
      Sbuffer *constBuf = create_static_cb(constsView.size(), s.c_str());
      if (!constBuf)
      {
        logerr("Could not create Sbuffer for const buf.");
        return;
      }
      constBuf->updateData(0, constsView.size_bytes(), constsView.data(), VBLOCK_DISCARD);
      if (DAGOR_UNLIKELY(interlocked_compare_exchange_ptr(constBuffer, constBuf, (Sbuffer *)nullptr) != nullptr))
        constBuf->destroy(); // unlikely case when other thread created/written buffer for this slot first
    }
  }

  static BindlessTexId find_or_add_bindless_tex(TEXTUREID tid, added_bindless_textures_t &added_bindless_textures)
  {
    OSSpinlockScopedLock lock(mutex);
    const auto id = uniqBindlessTex.findIf([tid](const BindlessTexRecord &rec) { return rec.texId == tid; });
    if (id != BindlessTexId::Invalid)
      return id;
    added_bindless_textures.push_back((uint32_t)tid);
    return BindlessTexId::Invalid;
  }

private:
  static void update_tex_record_bindless_slot_to_null(BindlessTexRecord &tex_record)
  {
    if (tex_record.lastTexStreamingGeneration == NULL_TEX_STREAMING_GENERATION)
      return;
    d3d::update_bindless_resources_to_null(tex_record.type, tex_record.bindlessId, 1);
    tex_record.lastTexStreamingGeneration = NULL_TEX_STREAMING_GENERATION;
  }
};

ShaderStateBlock create_bindless_state(const BindlessConstParams *bindless_data, uint8_t bindless_count, const Point4 *consts_data,
  uint8_t consts_count, dag::ConstSpan<uint32_t> added_bindless_textures, bool static_block, int stcode_id)
{
  ShaderStateBlock block;
  block.constIdx =
    BindlessState::create({bindless_data, bindless_count}, {consts_data, consts_count}, added_bindless_textures, stcode_id);
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
  const PackedStcodeId packedStcodeId = state.getPackedStcodeId();
  if (packedStcodeId != PackedStcodeId::Invalid)
    packedAll[packedStcodeId][state.getGlobalStateId()].applyPackedMaterial(const_state_idx);
  else
    all[state.getGlobalStateId()].apply(const_state_idx, tex_level);
}

void clear_bindless_states() { BindlessState::clear(); }

void req_tex_level_bindless(ConstStateIdx const_state_idx, int tex_level)
{
  const PackedConstsState state(const_state_idx);
  const PackedStcodeId packedStcodeId = state.getPackedStcodeId();
  if (packedStcodeId == PackedStcodeId::Invalid)
    all[state.getGlobalStateId()].reqTexLevel(tex_level);
}

BindlessTexId find_or_add_bindless_tex(TEXTUREID tid, added_bindless_textures_t &added_bindless_textures)
{
  return BindlessState::find_or_add_bindless_tex(tid, added_bindless_textures);
}

void dump_bindless_states_stat() { debug(" %d bindless states (%d total)", all.totalElements(), dataConsts.totalElements()); }

void update_bindless_state(ConstStateIdx const_state_idx, int tex_level)
{
  const PackedConstsState state(const_state_idx);
  G_ASSERT(state.getPackedStcodeId() != PackedStcodeId::Invalid);
  packedAll[state.getPackedStcodeId()][state.getGlobalStateId()].updateTexForPackedMaterial(const_state_idx, tex_level);
}

uint32_t PackedConstsState::getLocalStateOffset() const
{
  const uint32_t constsCount = ConstantsContainer::range_size(packedAll[getPackedStcodeId()][getGlobalStateId()].consts);
  if (!constsCount)
    return 0;
  // NOTE: the secret knowledge here is that for each packedStcodeId we have a set of materials with
  // different const params which all share the same stcode and hence constsCount (although the contents of
  // the consts are different), so we pack them all into a set of buffers, each of which is MAX_CONST_BUFFER_SIZE,
  // and then interpret these buffers as a T[MAX_CONST_BUFFER_SIZE/constsCount] inside of shaders, where
  // sizeof(T) = constsCount. A "local offset" is the offset into this structured buffer that should be used
  // by shaders, and it is calculated as follows.
  return StatesContainer::to_index(getGlobalStateId()) -
         (MAX_CONST_BUFFER_SIZE / constsCount) * PackedBuffersContainer::to_index(getBufferId());
}

uint32_t get_material_offset(ConstStateIdx const_state_idx)
{
  const PackedConstsState state(const_state_idx);
  G_ASSERT(state.getPackedStcodeId() != PackedStcodeId::Invalid);
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
  G_ASSERT(is_main_thread());

  packedAll.iterateWithIds([](PackedStcodeId id, StatesContainer &collection) {
    for (BindlessState &state : collection)
      state.resetPackedSamplers(id);
  });
  all.iterateWithIds([&](GlobalStateId id, BindlessState &state) { state.resetSamplers(id); });
}
