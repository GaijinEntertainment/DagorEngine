// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_refinedBlock.h>
#include <shaders/dag_bindumpReloadListener.h>
#include <shaders/dag_stcode.h>
#include <util/dag_oaHashNameMap.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_rwResource.h>
#include <3d/dag_resMgr.h>
#include <3d/dag_lockSbuffer.h>
#include <EASTL/fixed_vector.h>
#include <generic/dag_enumerate.h>

#include "shadersBinaryData.h"
#include "refinedBlockExec.h"

namespace refined_block
{

static constexpr uint32_t invalid_block_id = static_cast<uint32_t>(-1);

static int get_internal_id(int var_id)
{
  G_ASSERTF_RETURN(var_id >= 0, -1, "RefinedBlock: invalid var_id=%d", var_id);
  G_ASSERTF_RETURN(var_id < (int)shGlobalData().maxShadervarCnt(), -1, "RefinedBlock: var_id=%d exceeds maximum shader variable count",
    var_id);
  const int gid = shGlobalData().globvarIndexMap[var_id];
  G_ASSERTF_RETURN(gid < SHADERVAR_IDX_ABSENT, -1, "RefinedBlock: var_id=%d has no global var mapping", var_id);
  return gid;
}

template <typename BlockName>
dag::Vector<BlockName> &get_blocks()
{
  static dag::Vector<BlockName> blocks;
  return blocks;
}

template <typename T>
static bool is_valid_id(uint32_t id)
{
  return id < get_blocks<T>().size();
}

struct BindlessTexRecord
{
  uint32_t bindlessId;
  D3DResourceType type;
};

static dag::VectorMap<BaseTexture *, BindlessTexRecord> &get_bindless_records()
{
  static dag::VectorMap<BaseTexture *, BindlessTexRecord> bindlessRecords;
  return bindlessRecords;
}

struct BaseBlock
{
  eastl::string name;
  ska::flat_hash_map<int, VarValue> vars;
  ska::flat_hash_map<int, VarValue> gidVars;
  BaseBlock(const char *name) : name(name) {}
};

template <typename BlockHandleType>
struct ParentBlock
{
  OAHashNameMap<false> childNameMap;
  dag::VectorMap<uint32_t, typename BlockHandleType::ChildHandle> childNameToChildIdMap;
};

template <typename BlockHandleType>
struct ChildBlock
{
  typename BlockHandleType::ParentHandle parentId;
  ChildBlock(typename BlockHandleType::ParentHandle parent_id) : parentId(parent_id) {}
};

struct GlobalBlock : public BaseBlock, public ParentBlock<GlobalBlockHandle>
{
  GlobalBlock(const char *name) : BaseBlock(name) {}
};

struct ViewBlock : public BaseBlock, public ParentBlock<ViewBlockHandle>, public ChildBlock<ViewBlockHandle>
{
  ViewBlock(const char *name, GlobalBlockHandle parent_id) : BaseBlock(name), ChildBlock(parent_id) {}
};

struct PassBlock : public BaseBlock, public ChildBlock<PassBlockHandle>
{
  PassBlock(const char *name, ViewBlockHandle parent_id) : BaseBlock(name), ChildBlock(parent_id) {}

  static constexpr int CBUF_REGS = 4096;
  static constexpr int CBUF_RING_SIZE = 3;
  eastl::array<UniqueBuf, CBUF_RING_SIZE> cbufRing{};
  int currentCbufIdx = 0;

  eastl::vector<FlushedVar> flushedVars{};
  int flushedCbufIdx = -1;
};

template <typename BlockHandle>
const char *BaseBlockHandle<BlockHandle>::getName() const
{
  using BlockType = typename BlockHandle::BlockType;
  G_ASSERT_RETURN(is_valid_id<BlockType>(getId()), "invalid");
  const auto &block = get_blocks<BlockType>()[getId()];
  return DAGOR_UNLIKELY(block.name.empty()) ? "" : block.name.c_str();
}

template const char *BaseBlockHandle<GlobalBlockHandle>::getName() const;
template const char *BaseBlockHandle<ViewBlockHandle>::getName() const;
template const char *BaseBlockHandle<PassBlockHandle>::getName() const;

template <typename T>
struct Aliased
{
  using Type = T;
  T castFrom(const Type &v) const { return static_cast<T>(v); }
  Type castTo(const T &v) const { return static_cast<Type>(v); }
};

template <>
struct Aliased<Point4>
{
  using Type = Color4;
  Point4 castFrom(const Color4 &v) const { return Point4::rgba(v); }
  Color4 castTo(const Point4 &v) const { return Color4::xyzw(v); }
};

template <typename BlockHandle>
template <typename T>
void BaseBlockHandle<BlockHandle>::set(int var_id, const T &val)
{
  using BlockType = typename BlockHandle::BlockType;
  G_ASSERT_RETURN(is_valid_id<BlockType>(getId()), );
  const int gid = get_internal_id(var_id);
  if (DAGOR_LIKELY(gid >= 0))
  {
    const auto v = Aliased<T>().castTo(val);
    auto &b = get_blocks<BlockType>()[getId()];
    b.vars[var_id] = v;
    b.gidVars[gid] = v;
  }
}

template <typename BlockHandle>
template <typename T>
dag::Expected<T, GetError> BaseBlockHandle<BlockHandle>::get(int var_id) const
{
  using BlockType = typename BlockHandle::BlockType;
  G_ASSERT_RETURN(is_valid_id<BlockType>(getId()), dag::Unexpected(GetError::VarNotFound));
  using AliasedT = typename Aliased<T>::Type;
  const int gid = get_internal_id(var_id);
  if (gid < 0)
    return dag::Unexpected(GetError::VarNotFound);
  return getById<AliasedT>(var_id).transform([](AliasedT v) { return Aliased<T>().castFrom(v); });
}

template <typename T, auto MapMember, typename BlockHandle>
static dag::Expected<T, GetError> get_by_key(const BaseBlockHandle<BlockHandle> &handle, int key)
{
  using BlockType = typename BlockHandle::BlockType;
  G_ASSERT_RETURN(is_valid_id<BlockType>(handle.getId()), dag::Unexpected(GetError::VarNotFound));
  auto &b = get_blocks<BlockType>()[handle.getId()];
  if (const auto it = (b.*MapMember).find(key); it != (b.*MapMember).end())
  {
    if (auto *v = eastl::get_if<T>(&it->second))
      return *v;
    return dag::Unexpected(GetError::WrongVarType);
  }
  if constexpr (requires { typename BlockHandle::ParentHandle; })
    return get_by_key<T, MapMember>(b.parentId, key);
  return dag::Unexpected(GetError::VarNotFound);
}

template <typename BlockHandle>
template <typename T>
dag::Expected<T, GetError> BaseBlockHandle<BlockHandle>::getById(int var_id) const
{
  return get_by_key<T, &BaseBlock::vars>(*this, var_id);
}

template <typename BlockHandle>
template <typename T>
dag::Expected<T, GetError> BaseBlockHandle<BlockHandle>::getByGid(int gid) const
{
  return get_by_key<T, &BaseBlock::gidVars>(*this, gid);
}

template <typename ParentHandle>
typename ParentHandle::ChildHandle refine_block(const char *name, ParentHandle parent)
{
  using ParentBlockType = typename ParentHandle::BlockType;
  using ChildHandle = typename ParentHandle::ChildHandle;

  G_ASSERT_RETURN(is_valid_id<ParentBlockType>(parent.getId()), ParentHandle::ChildHandle::invalid);
  auto &b = get_blocks<ParentBlockType>()[parent.getId()];

  const int nameId = b.childNameMap.getNameId(name);
  if (nameId >= 0)
    return b.childNameToChildIdMap.find(nameId)->second;

  auto &children = get_blocks<typename ChildHandle::BlockType>();
  ChildHandle handle((uint32_t)children.size());
  children.emplace_back(name, parent);
  b.childNameToChildIdMap.emplace(b.childNameMap.addNameId(name), handle);
  return handle;
}

ViewBlockHandle GlobalBlockHandle::refineBlock(const char *name) { return refine_block(name, *this); }
PassBlockHandle ViewBlockHandle::refineBlock(const char *name) { return refine_block(name, *this); }

GlobalBlockHandle get_global()
{
  auto &globals = get_blocks<GlobalBlock>();
  if (globals.empty())
    globals.emplace_back("global");
  return GlobalBlockHandle(0);
}

#define INSTANTIATE_HANDLE_ACCESSORS(HandleType)                                                                    \
  template void BaseBlockHandle<HandleType>::set(int, const int &);                                                 \
  template void BaseBlockHandle<HandleType>::set(int, const float &);                                               \
  template void BaseBlockHandle<HandleType>::set(int, const Color4 &);                                              \
  template void BaseBlockHandle<HandleType>::set(int, const Point4 &);                                              \
  template void BaseBlockHandle<HandleType>::set(int, const IPoint4 &);                                             \
  template void BaseBlockHandle<HandleType>::set(int, const TMatrix4 &);                                            \
  template void BaseBlockHandle<HandleType>::set(int, BaseTexture *const &);                                        \
  template void BaseBlockHandle<HandleType>::set(int, Sbuffer *const &);                                            \
  template void BaseBlockHandle<HandleType>::set(int, const d3d::SamplerHandle &);                                  \
  template void BaseBlockHandle<HandleType>::set(int, RaytraceTopAccelerationStructure *const &);                   \
  template dag::Expected<int, GetError> BaseBlockHandle<HandleType>::get(int) const;                                \
  template dag::Expected<float, GetError> BaseBlockHandle<HandleType>::get(int) const;                              \
  template dag::Expected<Color4, GetError> BaseBlockHandle<HandleType>::get(int) const;                             \
  template dag::Expected<Point4, GetError> BaseBlockHandle<HandleType>::get(int) const;                             \
  template dag::Expected<IPoint4, GetError> BaseBlockHandle<HandleType>::get(int) const;                            \
  template dag::Expected<TMatrix4, GetError> BaseBlockHandle<HandleType>::get(int) const;                           \
  template dag::Expected<BaseTexture *, GetError> BaseBlockHandle<HandleType>::get(int) const;                      \
  template dag::Expected<d3d::SamplerHandle, GetError> BaseBlockHandle<HandleType>::get(int) const;                 \
  template dag::Expected<RaytraceTopAccelerationStructure *, GetError> BaseBlockHandle<HandleType>::get(int) const; \
  template dag::Expected<Sbuffer *, GetError> BaseBlockHandle<HandleType>::get(int) const;                          \
  template dag::Expected<int, GetError> BaseBlockHandle<HandleType>::getByGid(int) const;                           \
  template dag::Expected<float, GetError> BaseBlockHandle<HandleType>::getByGid(int) const;                         \
  template dag::Expected<Color4, GetError> BaseBlockHandle<HandleType>::getByGid(int) const;                        \
  template dag::Expected<IPoint4, GetError> BaseBlockHandle<HandleType>::getByGid(int) const;                       \
  template dag::Expected<TMatrix4, GetError> BaseBlockHandle<HandleType>::getByGid(int) const;                      \
  template dag::Expected<BaseTexture *, GetError> BaseBlockHandle<HandleType>::getByGid(int) const;                 \
  template dag::Expected<Sbuffer *, GetError> BaseBlockHandle<HandleType>::getByGid(int) const;                     \
  template dag::Expected<d3d::SamplerHandle, GetError> BaseBlockHandle<HandleType>::getByGid(int) const;            \
  template dag::Expected<RaytraceTopAccelerationStructure *, GetError> BaseBlockHandle<HandleType>::getByGid(int) const;

INSTANTIATE_HANDLE_ACCESSORS(GlobalBlockHandle)
INSTANTIATE_HANDLE_ACCESSORS(ViewBlockHandle)
INSTANTIATE_HANDLE_ACCESSORS(PassBlockHandle)

#undef INSTANTIATE_HANDLE_ACCESSORS

ViewBlockHandle ViewBlockHandle::invalid = ViewBlockHandle(invalid_block_id);
PassBlockHandle PassBlockHandle::invalid = PassBlockHandle(invalid_block_id);

PassBlockHandle::PassBlockHandle(uint32_t id) : BaseBlockHandle<PassBlockHandle>(id) {}

uint32_t get_or_allocate_bindless_tex(BaseTexture *tex)
{
  G_ASSERT_RETURN(tex, 0);

  auto &bindless = get_bindless_records();
  const auto it = bindless.find(tex);
  if (it != bindless.end())
    return it->second.bindlessId;

  const D3DResourceType type = tex->getType();
  const uint32_t idx = d3d::allocate_bindless_resource_range(type, 1);
  bindless.emplace(tex, BindlessTexRecord{idx, type});

  return idx;
}

static dag::Expected<dag::ConstSpan<int>, FlushError> get_stcode()
{
  const ScriptedShadersBinDumpV5 *dumpV5 = shBinDumpOwner().getDumpV5();
  if (DAGOR_UNLIKELY(!dumpV5))
    return dag::Unexpected(FlushError::NoDump);

  const int32_t stcodeId = dumpV5->refinedBlockStcodeId;
  if (DAGOR_UNLIKELY(stcodeId < 0))
    return dag::Unexpected(FlushError::InvalidStcodeId);

  const auto &stcode = shBinDump().stcode[stcodeId];
  if (DAGOR_UNLIKELY(stcode.empty()))
    return dag::Unexpected(FlushError::EmptyStcode);

  return stcode;
}

static dag::Expected<Sbuffer *, FlushError> flush(PassBlockHandle block_handle, dag::ConstSpan<int> stcode)
{
  G_ASSERT_RETURN(is_valid_id<PassBlock>(block_handle.getId()), dag::Unexpected(FlushError::CbufUpdateFailed));
  auto &block = get_blocks<PassBlock>()[block_handle.getId()];

  UniqueBuf &cbuf = block.cbufRing[block.currentCbufIdx];
  if (!cbuf.getBuf())
  {
    String bufName(64, "refined_block_%s_cb%d", block.name, block.currentCbufIdx);
    cbuf = dag::buffers::create_persistent_cb(block.CBUF_REGS, bufName.c_str(), RESTAG_REFINED_BLOCK);
  }

  if (!cbuf.getBuf())
    return dag::Unexpected(FlushError::CbufCreationFailed);

  block.flushedVars.clear();
  block.flushedCbufIdx = -1;

  auto locked = lock_sbuffer<float>(cbuf.getBuf(), 0, block.CBUF_REGS * 4, VBLOCK_DISCARD | VBLOCK_WRITEONLY);
  if (DAGOR_UNLIKELY(!locked))
    return dag::Unexpected(FlushError::CbufUpdateFailed);

  dag::Span<float> bufData(locked.get(), block.CBUF_REGS * 4);

#if CPP_STCODE
  const auto &stcodeCtx = shBinDumpOwner().stcodeCtx;
  if (stcodeCtx.dynstcodeExMode == stcode::ExecutionMode::CPP)
  {
    const ScriptedShadersBinDumpV5 *dumpV5 = shBinDumpOwner().getDumpV5();
    if (!dumpV5)
      return dag::Unexpected(FlushError::NoDump);

    const int32_t cppId = dumpV5->cppRefinedBlockStcodeId;
    if (cppId < 0)
      return dag::Unexpected(FlushError::InvalidStcodeId);

    RawBufferConstSetter rawBufSetter(bufData);
    ScopedRefinedBlock blockScope(block_handle);
    ScopedFlushedVarsCollector varsScope(block.flushedVars);
    stcode::ScopedCustomConstSetter csetOverride(&rawBufSetter);
    stcode::run_dyn_routine(stcodeCtx, cppId, nullptr);
  }
  else
#endif
  {
    exec_refined_block_stcode(stcode, block_handle, bufData, block.flushedVars);
  }

  block.flushedCbufIdx = block.currentCbufIdx;
  block.currentCbufIdx = (block.currentCbufIdx + 1) % block.CBUF_RING_SIZE;
  return cbuf.getBuf();
}

void PassBlockHandle::setState() const
{
  G_ASSERT_RETURN(is_valid_id<BlockType>(getId()), );
  auto &block = get_blocks<BlockType>()[getId()];
  if (block.flushedCbufIdx < 0)
    return;

  Sbuffer *cbuf = block.cbufRing[block.flushedCbufIdx].getBuf();
  d3d::set_const_buffer(STAGE_VS, REFINED_BLOCK_CONST_BUF_REGISTER, cbuf);
  d3d::set_const_buffer(STAGE_PS, REFINED_BLOCK_CONST_BUF_REGISTER, cbuf);
  d3d::set_const_buffer(STAGE_CS, REFINED_BLOCK_CONST_BUF_REGISTER, cbuf);

  for (const auto &v : block.flushedVars)
  {
    eastl::visit(
      [&](auto &&arg) {
        using T = eastl::decay_t<decltype(arg)>;
        if constexpr (eastl::is_same_v<T, BaseTexture *>)
          d3d::set_tex(v.stage, v.slotOrBindlessId, arg);
        else if constexpr (eastl::is_same_v<T, Sbuffer *>)
          d3d::set_buffer(v.stage, v.slotOrBindlessId, arg);
        else if constexpr (eastl::is_same_v<T, BindlessTexVar>)
        {
          const D3DResourceType resType = arg.tex ? arg.tex->getType() : D3DResourceType::TEX;
          if (arg.tex)
            d3d::update_bindless_resource(resType, v.slotOrBindlessId, arg.tex);
          else
            d3d::update_bindless_resources_to_null(resType, v.slotOrBindlessId, 1);
        }
        else if constexpr (eastl::is_same_v<T, CbufVar>)
          d3d::set_const_buffer(v.stage, v.slotOrBindlessId, arg.buf);
        else if constexpr (eastl::is_same_v<T, d3d::SamplerHandle>)
          d3d::set_sampler(v.stage, v.slotOrBindlessId, arg);
        else if constexpr (eastl::is_same_v<T, RaytraceTopAccelerationStructure *>)
        {
#if D3D_HAS_RAY_TRACING
          d3d::set_top_acceleration_structure(ShaderStage(v.stage), v.slotOrBindlessId, arg);
#endif
        }
        else if constexpr (eastl::is_same_v<T, RWTexVar>)
          d3d::set_rwtex(v.stage, v.slotOrBindlessId, arg.tex, 0, 0);
        else if constexpr (eastl::is_same_v<T, RWBufVar>)
          d3d::set_rwbuffer(v.stage, v.slotOrBindlessId, arg.buf);
      },
      v.var);
  }
}

void flush()
{
  auto stcode = get_stcode();
  if (DAGOR_UNLIKELY(!stcode))
  {
    logerr("Failed to get RefinedBlock stcode err: %d", eastl::to_underlying(stcode.error()));
    return;
  }

  for (auto [blockId, block] : enumerate(get_blocks<PassBlock>()))
  {
    if (blockId == PassBlockHandle::invalid.getId())
      continue;

    const auto flushResult = flush(PassBlockHandle(blockId), stcode.value());
    if (!flushResult)
      logerr("Failed to flush RefinedBlock '%s' err: %d", block.name, eastl::to_underlying(flushResult.error()));
  }
}

template <typename BlockType>
static void rebuild_gid_map_for_blocks()
{
  for (auto &block : get_blocks<BlockType>())
  {
    block.gidVars.clear();
    for (const auto &[id, var] : block.vars)
    {
      const int gid = get_internal_id(id);
      if (DAGOR_LIKELY(gid >= 0))
        block.gidVars[gid] = var;
    }
  }
}

void clear()
{
  auto &bindless = get_bindless_records();
  for (auto &[tid, record] : bindless)
    d3d::free_bindless_resource_range(record.type, record.bindlessId, 1);

  bindless.clear();
  get_blocks<GlobalBlock>().clear();
  get_blocks<ViewBlock>().clear();
  get_blocks<PassBlock>().clear();
}

struct RefinedBlockReloadListener final : public IShaderBindumpReloadListener
{
  void resolve() override
  {
    if (!IShaderBindumpReloadListener::staticInitDone)
      return;
    rebuild_gid_map_for_blocks<GlobalBlock>();
    rebuild_gid_map_for_blocks<ViewBlock>();
    rebuild_gid_map_for_blocks<PassBlock>();
  }
};
static RefinedBlockReloadListener refined_block_reload_listener;

} // namespace refined_block
