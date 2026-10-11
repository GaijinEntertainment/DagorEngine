// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bitfield.h"
#include "byte_units.h"
#include "driver.h"
#include "dynamic_array.h"
#include "render_state.h"
#include "shader_program_id.h"
#include "tagged_handles.h"
#include "constants.h"

#include <perfMon/dag_autoFuncProf.h>
#include <perfMon/dag_cpuFreq.h>
#include <dag/dag_vectorSet.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include "drv_log_defs.h"
#include <EASTL/array.h>
#include <EASTL/optional.h>
#include <EASTL/variant.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/sort.h>
#include <generic/dag_bitset.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_rwLock.h>
#include <shadersBinaryData.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_hashedKeyMap.h>
#include <util/dag_watchdog.h>

#include <shaders/shader_name_format.h>


inline constexpr uint32_t MAX_VERTEX_ATTRIBUTES = 16;
inline constexpr uint32_t MAX_VERTEX_INPUT_STREAMS = 4;
inline constexpr uint32_t MAX_SEMANTIC_INDEX = VSDR_TEXC14 + 1;

namespace drv3d_dx12
{
class DeviceContext;
class Image;

class Device;
// This data structure is used as is for binary storage on disk for caches and its bit pattern is hashed,
// so its layout has to be tightly packed and avoid padding of members, otherwise initialized bits may
// yield different results for "equal" instances.
struct InputLayout
{
  // for each location we use two corresponding bits to store index into streams
  uint64_t vertexAttributeLocationSourceStream = 0;
  // format uses top 5 bits and the remaining 11 bits store the offset as dword count (8k bytes of
  // offset max)
  uint16_t vertexAttributeCompactedFormatAndOffset[MAX_SEMANTIC_INDEX]{};
  // only needs MAX_VERTEX_INPUT_STREAMS buts, placed here to avoid padding
  uint8_t inputStreamUsageMask = 0;
  // only needs MAX_VERTEX_INPUT_STREAMS buts, placed here to avoid padding
  uint8_t inputStreamStepRateMask = 0;
  // We use 25 unique locations, each one bit, placed here to avoid padding
  uint32_t vertexAttributeLocationMask = 0;

  // keep this in sync with member types
  static constexpr size_t expected_size =
    sizeof(uint64_t) + sizeof(uint16_t) * MAX_SEMANTIC_INDEX + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t);

  static constexpr uint32_t vsdt_format_shift = 16;
  static constexpr uint32_t vsdt_max_value = VSDT_UINT4 >> vsdt_format_shift;
  static constexpr uint32_t format_index_bits = BitsNeeded<vsdt_max_value>::VALUE;
  static constexpr uint32_t offset_bits = sizeof(uint16_t) * 8 - format_index_bits;
  static constexpr uint16_t offset_mask = (1u << offset_bits) - 1;
  static constexpr uint16_t format_index_mask = ~offset_mask;
  static constexpr uint32_t validate_format_index_mask = ((1u << format_index_bits) - 1) << vsdt_format_shift;

  void useLocation(uint32_t index)
  {
    G_ASSERT(index < MAX_SEMANTIC_INDEX);
    vertexAttributeLocationMask |= 1u << index;
  }

  void setLocationStreamSource(uint32_t location, uint32_t stream)
  {
    G_ASSERT(stream < MAX_VERTEX_INPUT_STREAMS);
    G_STATIC_ASSERT(MAX_VERTEX_INPUT_STREAMS - 1 <= 3); // make sure we only need 2 bits for
                                                        // stream index
    vertexAttributeLocationSourceStream |= static_cast<uint64_t>(stream & 3) << (location * 2);
  }

  void setLocationStreamOffset(uint32_t location, uint32_t offset)
  {
    G_ASSERT(0 == (offset % 4));
    G_ASSERT((offset / 4) == ((offset / 4) & offset_mask));
    vertexAttributeCompactedFormatAndOffset[location] =
      (vertexAttributeCompactedFormatAndOffset[location] & format_index_mask) | ((offset / 4) & offset_mask);
  }

  void setLocationFormatIndex(uint32_t location, uint32_t index)
  {
    G_ASSERT(index == (index & validate_format_index_mask));
    vertexAttributeCompactedFormatAndOffset[location] =
      (vertexAttributeCompactedFormatAndOffset[location] & offset_mask) | ((index >> vsdt_format_shift) << offset_bits);
  }

  constexpr bool usesLocation(uint32_t index) const { return 0 != (1 & (vertexAttributeLocationMask >> index)); }

  constexpr uint32_t getLocationStreamSource(uint32_t location) const
  {
    return static_cast<uint32_t>(vertexAttributeLocationSourceStream >> (location * 2)) & 3;
  }

  constexpr uint32_t getLocationStreamOffset(uint32_t location) const
  {
    return static_cast<uint32_t>(vertexAttributeCompactedFormatAndOffset[location] & offset_mask) * 4;
  }

  constexpr uint32_t getLocationFormatIndex(uint32_t location) const
  {
    return (vertexAttributeCompactedFormatAndOffset[location] >> offset_bits) << vsdt_format_shift;
  }

  /*constexpr*/ DXGI_FORMAT getLocationFormat(uint32_t location) const
  {
    switch (((vertexAttributeCompactedFormatAndOffset[location] & format_index_mask) >> offset_bits) << vsdt_format_shift)
    {
      case VSDT_FLOAT1: return DXGI_FORMAT_R32_FLOAT;
      case VSDT_FLOAT2: return DXGI_FORMAT_R32G32_FLOAT;
      case VSDT_FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT;
      case VSDT_FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
      case VSDT_INT1: return DXGI_FORMAT_R32_SINT;
      case VSDT_INT2: return DXGI_FORMAT_R32G32_SINT;
      case VSDT_INT3: return DXGI_FORMAT_R32G32B32_SINT;
      case VSDT_INT4: return DXGI_FORMAT_R32G32B32A32_SINT;
      case VSDT_UINT1: return DXGI_FORMAT_R32_UINT;
      case VSDT_UINT2: return DXGI_FORMAT_R32G32_UINT;
      case VSDT_UINT3: return DXGI_FORMAT_R32G32B32_UINT;
      case VSDT_UINT4: return DXGI_FORMAT_R32G32B32A32_UINT;
      case VSDT_HALF2: return DXGI_FORMAT_R16G16_FLOAT;
      case VSDT_SHORT2N: return DXGI_FORMAT_R16G16_SNORM;
      case VSDT_SHORT2: return DXGI_FORMAT_R16G16_SINT;
      case VSDT_USHORT2N: return DXGI_FORMAT_R16G16_UNORM;

      case VSDT_HALF4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
      case VSDT_SHORT4N: return DXGI_FORMAT_R16G16B16A16_SNORM;
      case VSDT_SHORT4: return DXGI_FORMAT_R16G16B16A16_SINT;
      case VSDT_USHORT4N: return DXGI_FORMAT_R16G16B16A16_UNORM;

      case VSDT_UDEC3: return DXGI_FORMAT_R10G10B10A2_UINT;
      case VSDT_DEC3N: return DXGI_FORMAT_R10G10B10A2_UNORM;

      case VSDT_E3DCOLOR: return DXGI_FORMAT_B8G8R8A8_UNORM;
      case VSDT_UBYTE4: return DXGI_FORMAT_R8G8B8A8_UINT;
      default: G_ASSERTF(false, "invalid vertex declaration type"); break;
    }
    return DXGI_FORMAT_UNKNOWN;
  }

  void useStream(uint32_t index)
  {
    G_ASSERT(index < MAX_VERTEX_INPUT_STREAMS);
    inputStreamUsageMask |= 1u << index;
  }

  void setStreamStepRate(uint32_t index, D3D12_INPUT_CLASSIFICATION rate)
  {
    G_ASSERT(static_cast<uint32_t>(rate) <= 1); // -V547
    inputStreamStepRateMask |= static_cast<uint32_t>(rate) << index;
  }

  constexpr bool usesStream(uint32_t index) const { return 0 != (1 & (inputStreamUsageMask >> index)); }

  constexpr D3D12_INPUT_CLASSIFICATION getStreamStepRate(uint32_t index) const
  {
    return static_cast<D3D12_INPUT_CLASSIFICATION>(1 & (inputStreamStepRateMask >> index));
  }

  template <typename T>
  void generateInputElementDescriptions(T clb) const
  {
    auto attirbMask = vertexAttributeLocationMask;
    for (uint32_t i = 0; i < MAX_SEMANTIC_INDEX && attirbMask; ++i, attirbMask >>= 1)
    {
      if (0 == (1 & attirbMask))
        continue;

      auto semanticInfo = dxil::getSemanticInfoFromIndex(i);
      G_ASSERT(semanticInfo);
      if (!semanticInfo)
        break;

      D3D12_INPUT_ELEMENT_DESC desc;
      desc.SemanticName = semanticInfo->name;
      desc.SemanticIndex = semanticInfo->index;
      desc.Format = getLocationFormat(i);
      desc.InputSlot = getLocationStreamSource(i);
      desc.AlignedByteOffset = getLocationStreamOffset(i);
      desc.InputSlotClass = getStreamStepRate(desc.InputSlot);
      desc.InstanceDataStepRate = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA == desc.InputSlotClass ? 0 : 1;
      clb(desc);
    }
  }

  constexpr bool validateAttributeUse(uint32_t expected) const { return expected == (vertexAttributeLocationMask & expected); }

  constexpr bool matchesAttributeUse(uint32_t expected) const { return vertexAttributeLocationMask == expected; }

  // This creates a copy of this with all attributes removed that are not
  // part of the used mask, it also removes any input stream that might no
  // longer be needed.
  InputLayout getLayoutForAttributeUse(uint32_t used) const
  {
    InputLayout other;
    uint32_t aMask = vertexAttributeLocationMask & used;
    for (uint32_t i = 0; i < MAX_SEMANTIC_INDEX && aMask; ++i, aMask >>= 1)
    {
      if (0 == (1 & aMask))
        continue;
      other.useLocation(i);
      auto streamIndex = getLocationStreamSource(i);
      other.setLocationStreamSource(i, streamIndex);
      other.useStream(streamIndex);
      other.setLocationStreamOffset(i, getLocationStreamOffset(i));
      other.setLocationFormatIndex(i, getLocationFormatIndex(i));
    }
    uint32_t sMask = other.inputStreamUsageMask;
    for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS && sMask; ++i, sMask >>= 1)
    {
      if (0 == (1 & sMask))
        continue;

      other.setStreamStepRate(i, getStreamStepRate(i));
    }

    return other;
  }

  struct DecodeContext
  {
    uint32_t ofs = 0;
    uint32_t streamIndex = 0;
  };

  void fromVdecl(const VSDTYPE *decl)
  {
    *this = {};
    DecodeContext context{};

    while (fromVdecl(context, *decl++))
      ;
  }
  bool fromVdecl(DecodeContext &context, const VSDTYPE &decl);
};

inline bool operator==(const InputLayout &l, const InputLayout &r)
{
  G_STATIC_ASSERT(sizeof(InputLayout) == InputLayout::expected_size);
  return l.inputStreamUsageMask == r.inputStreamUsageMask && l.inputStreamStepRateMask == r.inputStreamStepRateMask &&
         l.vertexAttributeLocationMask == r.vertexAttributeLocationMask &&
         l.vertexAttributeLocationSourceStream == r.vertexAttributeLocationSourceStream &&
         eastl::equal(eastl::begin(l.vertexAttributeCompactedFormatAndOffset), eastl::end(l.vertexAttributeCompactedFormatAndOffset),
           eastl::begin(r.vertexAttributeCompactedFormatAndOffset));
}

inline bool operator!=(const InputLayout &l, const InputLayout &r) { return !(l == r); }

struct ShaderIdentifier
{
  dxil::HashValue shaderHash;
  uint32_t shaderSize;
};

struct StageShaderModuleHeader
{
  ShaderIdentifier ident = {};
  dxil::ShaderHeader header = {};
  eastl::string debugName;
};

struct StageShaderModule : StageShaderModuleHeader
{
  ShaderSource source;
  uint32_t bytecodeOffset = 0;
  uint32_t bytecodeSize = 0;

  explicit operator bool() const { return 0 != bytecodeSize; }
};

struct VertexShaderModule : StageShaderModule
{
  VertexShaderModule() = default;
  ~VertexShaderModule() = default;

  VertexShaderModule(VertexShaderModule &&) = default;
  VertexShaderModule &operator=(VertexShaderModule &&) = default;

  VertexShaderModule(StageShaderModule &&module) : StageShaderModule{eastl::move(module)} {}
  eastl::unique_ptr<StageShaderModule> geometryShader;
  eastl::unique_ptr<StageShaderModule> hullShader;
  eastl::unique_ptr<StageShaderModule> domainShader;
  DynamicArray<dxil::StreamOutputComponentInfo> streamOutputDesc;
};

#if _TARGET_XBOXONE
// XB1 has no mesh shader stage
inline constexpr bool is_mesh(const VertexShaderModule &) { return false; }
#else
inline bool is_mesh(const VertexShaderModule &module)
{
  return dxil::ShaderStage::MESH == static_cast<dxil::ShaderStage>(module.header.shaderType);
}
#endif

using PixelShaderModule = StageShaderModule;
using ComputeShaderModule = StageShaderModule;

#if D3D_HAS_RAY_TRACING
struct RaytraceProgram
{
  eastl::unique_ptr<ShaderID[]> shaders;
  eastl::unique_ptr<RaytraceShaderGroup[]> shaderGroups;
  uint32_t groupCount = 0;
  uint32_t shaderCount = 0;
  uint32_t maxRecursionDepth = 0;
};
using RaytraceShaderModule = StageShaderModule;
#endif

struct ShaderStageResourceUsageMask
{
  Bitset<dxil::MAX_B_REGISTERS> bRegisterMask;
  Bitset<dxil::MAX_T_REGISTERS> tRegisterMask;
  Bitset<dxil::MAX_S_REGISTERS> sRegisterMask;
  Bitset<dxil::MAX_U_REGISTERS> uRegisterMask;

  ShaderStageResourceUsageMask() = default;
  ShaderStageResourceUsageMask(const ShaderStageResourceUsageMask &) = default;
  ShaderStageResourceUsageMask &operator=(const ShaderStageResourceUsageMask &) = default;
  // broadcast a single value to all masks
  explicit ShaderStageResourceUsageMask(uint32_t v) : bRegisterMask{v}, tRegisterMask{v}, sRegisterMask{v}, uRegisterMask{v} {}
  explicit ShaderStageResourceUsageMask(const dxil::ShaderHeader &header) :
    bRegisterMask{header.resourceUsageTable.bRegisterUseMask},
    tRegisterMask{header.resourceUsageTable.tRegisterUseMask},
    sRegisterMask{header.resourceUsageTable.sRegisterUseMask},
    uRegisterMask{header.resourceUsageTable.uRegisterUseMask}
  {}

  ShaderStageResourceUsageMask &operator|=(const ShaderStageResourceUsageMask &other)
  {
    bRegisterMask |= other.bRegisterMask;
    tRegisterMask |= other.tRegisterMask;
    sRegisterMask |= other.sRegisterMask;
    uRegisterMask |= other.uRegisterMask;
    return *this;
  }

  ShaderStageResourceUsageMask operator&(const ShaderStageResourceUsageMask &other) const
  {
    ShaderStageResourceUsageMask cpy{*this};
    cpy.bRegisterMask &= other.bRegisterMask;
    cpy.tRegisterMask &= other.tRegisterMask;
    cpy.sRegisterMask &= other.sRegisterMask;
    cpy.uRegisterMask &= other.uRegisterMask;
    return cpy;
  }

  ShaderStageResourceUsageMask &operator^=(const ShaderStageResourceUsageMask &other)
  {
    bRegisterMask ^= other.bRegisterMask;
    tRegisterMask ^= other.tRegisterMask;
    sRegisterMask ^= other.sRegisterMask;
    uRegisterMask ^= other.uRegisterMask;
    return *this;
  }
};

struct GraphicsProgramUsageInfo
{
  InputLayoutID input = InputLayoutID::Null();
  GraphicsProgramID programId = GraphicsProgramID::Null();
  uint32_t vsImplicitCbufRegCount = 0;
  uint32_t psImplicitCbufRegCount = 0;
};

struct ComputeProgramUsageInfo
{
  ProgramID programId = ProgramID::Null();
  uint32_t implicitCbufRegCount = 0;
};

uint16_t decode_implicit_cbuf_reg_count(dag::ConstSpan<uint8_t> metadata, bool expect_vertex_pipeline);

class IdManager
{
  uint32_t lastAllocatedId = 0;
  dag::Vector<uint32_t> freeIds;

public:
  uint32_t allocate()
  {
    uint32_t id;
    if (!freeIds.empty())
    {
      id = freeIds.back();
      freeIds.pop_back();
    }
    else
    {
      id = ++lastAllocatedId;
    }
    return id;
  }
  void free(uint32_t id)
  {
    G_ASSERT(id <= lastAllocatedId);
    if (id == lastAllocatedId)
    {
      --lastAllocatedId;
      while (!freeIds.empty())
      {
        if (freeIds.back() != lastAllocatedId)
        {
          break;
        }
        --lastAllocatedId;
        freeIds.pop_back();
      }
    }
    else
    {
      freeIds.insert(eastl::lower_bound(eastl::begin(freeIds), eastl::end(freeIds), id), id);
    }
  }
  void clear()
  {
    lastAllocatedId = 0;
    freeIds.clear();
  }

  template <typename T>
  void iterateAllocated(T t) const
  {
    uint32_t at = 1;
    for (auto se : freeIds)
    {
      for (; at < se; ++at)
      {
        t(at);
      }

      at = se + 1;
    }
    for (; at <= lastAllocatedId; ++at)
    {
      t(at);
    }
  }
};

template <typename T>
inline void ensure_container_space(T &v, size_t space)
{
  if (v.size() < space)
    v.resize(space);
}
template <typename T, typename U>
inline void ensure_container_space(T &v, size_t space, U &&u)
{
  if (v.size() < space)
    v.resize(space, u);
}

template <typename C, typename V>
inline void set_at(C &c, size_t i, V v)
{
  if (c.size() == i)
  {
    c.push_back(eastl::move(v));
  }
  else
  {
    ensure_container_space(c, i + 1);
    c[i] = eastl::move(v);
  }
}

struct GraphicsProgramTemplate
{
  ShaderID vertexShader;
  ShaderID pixelShader;
};

struct GraphicsProgramInstance
{
  InputLayoutID inputLayout = InputLayoutID::Null();
  GraphicsProgramID internalProgram = GraphicsProgramID::Null();
  int refCount = 0;
  void addRef() { interlocked_increment(refCount); }
  void deleteAllReferences()
  {
    refCount = 0;
    internalProgram = GraphicsProgramID::Null();
  }
  int getRefCount() { return interlocked_acquire_load(refCount); }
  bool delRef()
  {
    if (interlocked_decrement(refCount) != 0)
      return false;
    internalProgram = GraphicsProgramID::Null();
    return true;
  }
};

template <typename T>
void inspect_scripted_shader_bin_dump(ScriptedShadersBinDumpOwner *dump, T inspector)
{
  AutoFuncProfT<AFP_MSEC, 50> _prof("DX12: 'inspect_scripted_shader_bin_dump' takes %d ms");

  auto v2 = dump->getDumpV2();
  if (!v2)
  {
    return;
  }

  int64_t tSetup = ref_time_ticks();
  inspector.vertexShaderCount(v2->vprCount);
  inspector.pixelOrComputeShaderCount(v2->fshCount);
  int setupUsec = get_time_usec(tSetup);

  static constexpr uint32_t BucketCount = 32;
  class UniquePrograms
  {
    eastl::array<dag::VectorSet<uint32_t, eastl::less<uint32_t>, framemem_allocator>, BucketCount> buckets;

  public:
    UniquePrograms()
    {
      for (auto &bucket : buckets)
        bucket.reserve(1024);
    }

    bool tryAddProgram(uint16_t vpr_id, uint16_t fsh_id)
    {
      G_STATIC_ASSERT(is_pow2(BucketCount));

      uint32_t programId = (uint32_t(vpr_id) << 16) | fsh_id;
      uint32_t bucketIndex = (uint32_t(vpr_id) + fsh_id) & (BucketCount - 1);

      return buckets[bucketIndex].insert(programId).second;
    }
  } uniquePrograms;

  uint32_t totalRefs = 0, uniqueCount = 0;
  uint32_t graphicsCount = 0, graphicsNullPsCount = 0, computeCount = 0;
  int64_t dedupUsec = 0, graphicsUsec = 0, graphicsNullPsUsec = 0, computeUsec = 0;

  for (auto &cls : v2->classes)
  {
    for (auto &prog : cls.shrefStorage)
    {
      ::watchdog_kick(); // REMOVEME: dirty, dirty hack to work around watchdog killing us during pipeline cache loading

      ++totalRefs;
      int64_t tDedup = ref_time_ticks();
      bool isNew = uniquePrograms.tryAddProgram(prog.vprId, prog.fshId);
      dedupUsec += get_time_usec(tDedup);

      if (!isNew)
        continue;
      ++uniqueCount;

      if (prog.vprId != 0xFFFF && prog.fshId != 0xFFFF)
      {
        int64_t t0 = ref_time_ticks();
        inspector.addGraphicsProgram(prog.vprId, prog.fshId);
        graphicsUsec += get_time_usec(t0);
        ++graphicsCount;
      }
      else if (prog.vprId != 0xFFFF)
      {
        int64_t t0 = ref_time_ticks();
        inspector.addGraphicsProgramWithNullPixelShader(prog.vprId);
        graphicsNullPsUsec += get_time_usec(t0);
        ++graphicsNullPsCount;
      }
      else
      {
        int64_t t0 = ref_time_ticks();
        inspector.addComputeProgram(prog.fshId);
        computeUsec += get_time_usec(t0);
        ++computeCount;
      }
    }
  }

  logdbg("DX12: inspect_scripted_shader_bin_dump breakdown:"
         " setup=%dus, totalRefs=%u, unique=%u,"
         " dedup=%lldus,"
         " graphics=%u/%lldus, graphicsNullPs=%u/%lldus, compute=%u/%lldus",
    setupUsec, totalRefs, uniqueCount, dedupUsec, graphicsCount, graphicsUsec, graphicsNullPsCount, graphicsNullPsUsec, computeCount,
    computeUsec);
}

class ShaderProgramGroup
{
  friend class ShaderProgramGroups;
  ScriptedShadersBinDumpOwner *dump = nullptr;
  dag::Vector<eastl::variant<eastl::monostate, ShaderID, ProgramID>> pixelShaderComputeProgramIDMap;
  dag::Vector<GraphicsProgramTemplate> graphicsProgramTemplates;
  dag::Vector<uint16_t> dumpShaderImplicitCbufRegCount;
  dag::Vector<uint16_t> pixelShaderDumpIndex;
  dag::Vector<uint16_t> computeProgramDumpIndex;
  uint32_t vertexShaderCount = 0;
  uint32_t pixelShaderCount = 0;
  uint32_t computeShaderCount = 0;

  struct ScriptedShaderBinDumpInspector
  {
    uint32_t groupID;
    ShaderID nullPixelShader;
    ShaderProgramGroup *target;
    void vertexShaderCount(uint32_t count) { target->vertexShaderCount = count; }
    void pixelOrComputeShaderCount(uint32_t count) { target->pixelShaderComputeProgramIDMap.resize(count); }
    void addGraphicsProgram(uint16_t vs_index, uint16_t ps_index)
    {
      ShaderID psID;
      if (auto shader = eastl::get_if<ShaderID>(&target->pixelShaderComputeProgramIDMap[ps_index]))
      {
        psID = *shader;
      }
      else
      {
        psID = ShaderID::make(groupID, target->pixelShaderCount++);
        target->pixelShaderComputeProgramIDMap[ps_index] = psID;
        target->pixelShaderDumpIndex.push_back(target->vertexShaderCount + ps_index);
      }
      auto vsID = ShaderID::make(groupID, vs_index);
      target->graphicsProgramTemplates.push_back({vsID, psID});
    }
    void addGraphicsProgramWithNullPixelShader(uint16_t vs_index)
    {
      auto vsID = ShaderID::make(groupID, vs_index);
      target->graphicsProgramTemplates.push_back({vsID, nullPixelShader});
    }
    void addComputeProgram(uint32_t shader_index)
    {
      if (!eastl::holds_alternative<ProgramID>(target->pixelShaderComputeProgramIDMap[shader_index]))
      {
        target->pixelShaderComputeProgramIDMap[shader_index] = ProgramID::asComputeProgram(groupID, target->computeShaderCount++);
        target->computeProgramDumpIndex.push_back(target->vertexShaderCount + shader_index);
      }
    }
  };

  void decodeImplicitCbufRegCounts()
  {
    if (!dump->getDump())
      return;
    const auto &metadata = dump->getDump()->shaders_metadata;
    const uint32_t shaderCount = min<uint32_t>(vertexShaderCount + pixelShaderComputeProgramIDMap.size(), metadata.size());
    dumpShaderImplicitCbufRegCount.resize(shaderCount);
    for (uint32_t i = 0; i < shaderCount; ++i)
    {
      dumpShaderImplicitCbufRegCount[i] =
        decode_implicit_cbuf_reg_count(make_span_const(metadata[i].data(), metadata[i].size()), i < vertexShaderCount);
    }
  }

protected:
  void clear()
  {
    dump = nullptr;
    pixelShaderComputeProgramIDMap.clear();
    graphicsProgramTemplates.clear();
    dumpShaderImplicitCbufRegCount.clear();
    pixelShaderDumpIndex.clear();
    computeProgramDumpIndex.clear();
    vertexShaderCount = 0;
    pixelShaderCount = 0;
    computeShaderCount = 0;
  }

  void setup(const ShaderProgramGroup *base, ScriptedShadersBinDumpOwner *d, ShaderID null_pixel_shader)
  {
    dump = d;
    inspect_scripted_shader_bin_dump(d, ScriptedShaderBinDumpInspector{getGroupID(base), null_pixel_shader, this});
    decodeImplicitCbufRegCounts();
    graphicsProgramTemplates.shrink_to_fit();
    logdbg("DX12: Shader bindump %p contains %u vertex shaders, %u pixel shaders, %u graphics programs, %u compute programs", d,
      vertexShaderCount, pixelShaderCount, graphicsProgramTemplates.size(), computeShaderCount);
  }

  bool isEmpty() const { return nullptr == dump; }

  bool isAssociatedWith(const ScriptedShadersBinDumpOwner *d) const { return dump == d; }

  void trim()
  {
    pixelShaderComputeProgramIDMap.shrink_to_fit();
    graphicsProgramTemplates.shrink_to_fit();
    pixelShaderDumpIndex.shrink_to_fit();
    computeProgramDumpIndex.shrink_to_fit();
  }

  uint32_t getGroupID(const ShaderProgramGroup *base) const { return 1 + (this - base); }

  uint32_t getDumpShaderImplicitCbufRegCount(uint32_t dump_index) const
  {
    return dump_index < dumpShaderImplicitCbufRegCount.size() ? dumpShaderImplicitCbufRegCount[dump_index]
                                                              : UNKNOWN_CONST_REGISTER_COUNT_REQUIREMENT;
  }

  uint32_t getVertexShaderImplicitCbufRegCount(ShaderID id) const
  {
    return id.getIndex() < vertexShaderCount ? getDumpShaderImplicitCbufRegCount(id.getIndex())
                                             : UNKNOWN_CONST_REGISTER_COUNT_REQUIREMENT;
  }

  uint32_t getPixelShaderImplicitCbufRegCount(ShaderID id) const
  {
    return id.getIndex() < pixelShaderDumpIndex.size() ? getDumpShaderImplicitCbufRegCount(pixelShaderDumpIndex[id.getIndex()])
                                                       : UNKNOWN_CONST_REGISTER_COUNT_REQUIREMENT;
  }

  uint32_t getComputeProgramImplicitCbufRegCount(ProgramID id) const
  {
    return id.getIndex() < computeProgramDumpIndex.size() ? getDumpShaderImplicitCbufRegCount(computeProgramDumpIndex[id.getIndex()])
                                                          : UNKNOWN_CONST_REGISTER_COUNT_REQUIREMENT;
  }

  template <typename T>
  void iterateComputeShaders(T clb) const
  {
    for (const auto &e : pixelShaderComputeProgramIDMap)
    {
      if (eastl::holds_alternative<ProgramID>(e))
      {
        clb(eastl::get<ProgramID>(e));
      }
    }
  }

  template <typename T>
  void iteratePixelShaders(T clb) const
  {
    for (const auto &e : pixelShaderComputeProgramIDMap)
    {
      if (eastl::holds_alternative<ShaderID>(e))
      {
        clb(eastl::get<ShaderID>(e));
      }
    }
  }

  GraphicsProgramID findGraphicsProgram(const ShaderProgramGroup *base, ShaderID vs, ShaderID ps) const
  {
    auto ref = eastl::find_if(begin(graphicsProgramTemplates), end(graphicsProgramTemplates),
      [vs, ps](const auto &prog) { return vs == prog.vertexShader && ps == prog.pixelShader; });
    if (ref != end(graphicsProgramTemplates))
    {
      return GraphicsProgramID::make(getGroupID(base), ref - begin(graphicsProgramTemplates));
    }
    return GraphicsProgramID::Null();
  }

  GraphicsProgramID addGraphicsProgram(const ShaderProgramGroup *base, ShaderID vs, ShaderID ps)
  {
    GraphicsProgramID id;
    auto ref = eastl::find_if(begin(graphicsProgramTemplates), end(graphicsProgramTemplates),
      [](const auto &prog) { return ShaderID::Null() == prog.vertexShader; });
    if (ref == end(graphicsProgramTemplates))
    {
      id = GraphicsProgramID::make(getGroupID(base), graphicsProgramTemplates.size());
      graphicsProgramTemplates.push_back(GraphicsProgramTemplate{vs, ps});
    }
    else
    {
      id = GraphicsProgramID::make(getGroupID(base), ref - begin(graphicsProgramTemplates));
      ref->vertexShader = vs;
      ref->pixelShader = ps;
    }
    return id;
  }

  GraphicsProgramTemplate getShadersOfGraphicsProgram(GraphicsProgramID gpid) const
  {
    return graphicsProgramTemplates[gpid.getIndex()];
  }

  template <typename T>
  void iterateGraphicsPrograms(const ShaderProgramGroup *base, T clb)
  {
    auto gid = getGroupID(base);
    for (size_t i = 0; i < graphicsProgramTemplates.size(); ++i)
    {
      clb(GraphicsProgramID::make(gid, i));
    }
  }

  template <typename T>
  void removeGraphicsProgramWithMatchingVertexShader(const ShaderProgramGroup *base, ShaderID vs, T clb)
  {
    auto gid = getGroupID(base);
    for (size_t i = 0; i < graphicsProgramTemplates.size(); ++i)
    {
      auto &gp = graphicsProgramTemplates[i];
      if (gp.vertexShader == vs)
      {
        clb(GraphicsProgramID::make(gid, i));
        gp.vertexShader = ShaderID::Null();
        gp.pixelShader = ShaderID::Null();
      }
    }
  }

  template <typename T>
  void removeGraphicsProgramWithMatchingPixelShader(const ShaderProgramGroup *base, ShaderID ps, T clb)
  {
    auto gid = getGroupID(base);
    for (size_t i = 0; i < graphicsProgramTemplates.size(); ++i)
    {
      auto &gp = graphicsProgramTemplates[i];
      if (gp.pixelShader == ps)
      {
        clb(GraphicsProgramID::make(gid, i));
        gp.vertexShader = ShaderID::Null();
        gp.pixelShader = ShaderID::Null();
      }
    }
  }

  dxil::ShaderStage shaderFromIndex(const ShaderProgramGroup *base, uint32_t shader_index, ShaderCodeType type, void *ident)
  {
    auto groupID = getGroupID(base);
    if (ShaderCodeType::VERTEX == type)
    {
      G_ASSERT_RETURN(shader_index < vertexShaderCount, dxil::ShaderStage::VERTEX);
      *static_cast<VPROG *>(ident) = ShaderID::make(groupID, shader_index).exportValue();
      return dxil::ShaderStage::VERTEX;
    }
    G_ASSERT_RETURN(shader_index < pixelShaderComputeProgramIDMap.size(), dxil::ShaderStage::VERTEX);
    if (auto shaderID = eastl::get_if<ShaderID>(&pixelShaderComputeProgramIDMap[shader_index]))
    {
      *static_cast<FSHADER *>(ident) = shaderID->exportValue();
      return dxil::ShaderStage::PIXEL;
    }
    if (auto programID = eastl::get_if<ProgramID>(&pixelShaderComputeProgramIDMap[shader_index]))
    {
      *static_cast<FSHADER *>(ident) = programID->exportValue();
      return dxil::ShaderStage::COMPUTE;
    }
    return dxil::ShaderStage::VERTEX;
  }
};

class ShaderProgramGroups
{
  // shaderGroup 0 is always to "global" shaderGroup and is not handled by groups
  eastl::array<ShaderProgramGroup, max_scripted_shaders_bin_groups - 1> groups{};
  // For shaders we only track used ids. When new shaders are created or
  // shaders are deleted we just manage the ids and hand over ownership
  // to the device context backend with the appropriate commands.
  IdManager vertexShaderIds;
  IdManager pixelShaderIds;
  IdManager computeIds;
  dag::Vector<uint16_t> vertexShaderImplicitCbufRegCount;
  dag::Vector<uint16_t> pixelShaderImplicitCbufRegCount;
  dag::Vector<uint16_t> computeProgramImplicitCbufRegCount;

  static void setImplicitCbufRegCount(dag::Vector<uint16_t> &table, uint32_t index, uint16_t count)
  {
    if (table.size() <= index)
      table.resize(index + 1, 0);
    table[index] = count;
  }

  static uint32_t getImplicitCbufRegCount(const dag::Vector<uint16_t> &table, uint32_t index)
  {
    return index < table.size() ? table[index] : UNKNOWN_CONST_REGISTER_COUNT_REQUIREMENT;
  }

  typedef uint64_t hash_key_t;
  HashedKeyMap<hash_key_t, uint32_t, hash_key_t(0), oa_hashmap_util::MumStepHash<hash_key_t>> graphicsProgramsHashMap;

  dag::Vector<GraphicsProgramTemplate> graphicsProgramTemplates;
  dag::Vector<GraphicsProgramInstance> graphicsProgramInstances;
  uint32_t graphicsProgramsHashMapDead = 0;

  ShaderProgramGroup *getShaderGroupForShader(ShaderID id)
  {
    if (auto index = id.getGroup())
    {
      G_ASSERT(index > 0 && (index - 1) < groups.size());
      return &groups[index - 1];
    }
    return nullptr;
  }

  const ShaderProgramGroup *getShaderGroupForShader(ShaderID id) const
  {
    if (auto index = id.getGroup())
    {
      G_ASSERT(index > 0 && (index - 1) < groups.size());
      return &groups[index - 1];
    }
    return nullptr;
  }

  ShaderProgramGroup *getShaderGroupForShaders(ShaderID vs, ShaderID ps)
  {
    if (auto index = vs.getGroup())
    {
      G_ASSERT(index > 0 && (index - 1) < groups.size());
      if (0 == ps.getGroup() || ps.getGroup() == index)
      {
        return &groups[index - 1];
      }
    }
    return nullptr;
  }

  const ShaderProgramGroup *getShaderGroupForShaders(ShaderID vs, ShaderID ps) const
  {
    if (auto index = vs.getGroup())
    {
      G_ASSERT(index > 0 && (index - 1) < groups.size());
      if (0 == ps.getGroup() || ps.getGroup() == index)
      {
        return &groups[index - 1];
      }
    }
    return nullptr;
  }

  ShaderProgramGroup *getShaderGroupForProgram(ProgramID id)
  {
    if (auto index = id.getGroup())
    {
      G_ASSERT(index > 0 && (index - 1) < groups.size());
      return &groups[index - 1];
    }
    return nullptr;
  }

  const ShaderProgramGroup *getShaderGroupForProgram(ProgramID id) const
  {
    if (auto index = id.getGroup())
    {
      G_ASSERT(index > 0 && (index - 1) < groups.size());
      return &groups[index - 1];
    }
    return nullptr;
  }

  ShaderProgramGroup *getShaderGroupForGraphicsProgram(GraphicsProgramID id)
  {
    if (auto index = id.getGroup())
    {
      G_ASSERT(index > 0 && (index - 1) < groups.size());
      return &groups[index - 1];
    }
    return nullptr;
  }

  const ShaderProgramGroup *getShaderGroupForGraphicsProgram(GraphicsProgramID id) const
  {
    if (auto index = id.getGroup())
    {
      G_ASSERT(index > 0 && (index - 1) < groups.size());
      return &groups[index - 1];
    }
    return nullptr;
  }

public:
  void trim(ShaderProgramGroup *shaderGroup) { shaderGroup->trim(); }
  void removePixelShader(ShaderID shader) { pixelShaderIds.free(shader.getIndex()); }
  void removeVertexShader(ShaderID shader)
  {
    G_ASSERTF(0 == shader.getGroup(), "Invalid to call this function with a shader with a shaderGroup id "
                                      "other than 0");
    vertexShaderIds.free(shader.getIndex());
  }
  void removeComputeProgram(ProgramID program) { computeIds.free(program.getIndex()); }
  template <typename T>
  void iterateAllPixelShaders(T clb)
  {
    for (auto &g : groups)
    {
      g.iteratePixelShaders(clb);
    }
    pixelShaderIds.iterateAllocated([&](uint32_t id) { clb(ShaderID::make(0, id)); });
  }
  template <typename T>
  void itarateAllVertexShaders(T clb)
  {
    vertexShaderIds.iterateAllocated([&](uint32_t id) { clb(ShaderID::make(0, id)); });
  }
  template <typename T>
  void iterateAllComputeShaders(T clb)
  {
    for (auto &g : groups)
    {
      g.iterateComputeShaders(clb);
    }
    computeIds.iterateAllocated([&](uint32_t id) { clb(ProgramID::asComputeProgram(0, id)); });
  }
  ProgramID addComputeShaderProgram(uint32_t implicit_cbuf_reg_count)
  {
    auto program = ProgramID::asComputeProgram(0, computeIds.allocate());
    setImplicitCbufRegCount(computeProgramImplicitCbufRegCount, program.getIndex(), implicit_cbuf_reg_count);
    return program;
  }
  ShaderID addPixelShader(uint32_t implicit_cbuf_reg_count)
  {
    auto id = ShaderID::make(0, pixelShaderIds.allocate());
    setImplicitCbufRegCount(pixelShaderImplicitCbufRegCount, id.getIndex(), implicit_cbuf_reg_count);
    return id;
  }
  ShaderID addVertexShader(uint32_t implicit_cbuf_reg_count)
  {
    auto id = ShaderID::make(0, vertexShaderIds.allocate());
    setImplicitCbufRegCount(vertexShaderImplicitCbufRegCount, id.getIndex(), implicit_cbuf_reg_count);
    return id;
  }
  uint32_t getVertexShaderImplicitCbufRegCount(ShaderID id) const
  {
    if (ShaderID::Null() == id)
      return UNKNOWN_CONST_REGISTER_COUNT_REQUIREMENT;
    if (auto shaderGroup = getShaderGroupForShader(id))
      return shaderGroup->getVertexShaderImplicitCbufRegCount(id);
    return getImplicitCbufRegCount(vertexShaderImplicitCbufRegCount, id.getIndex());
  }
  uint32_t getPixelShaderImplicitCbufRegCount(ShaderID id) const
  {
    if (ShaderID::Null() == id)
      return UNKNOWN_CONST_REGISTER_COUNT_REQUIREMENT;
    if (auto shaderGroup = getShaderGroupForShader(id))
      return shaderGroup->getPixelShaderImplicitCbufRegCount(id);
    return getImplicitCbufRegCount(pixelShaderImplicitCbufRegCount, id.getIndex());
  }
  uint32_t getComputeProgramImplicitCbufRegCount(ProgramID id) const
  {
    if (ProgramID::Null() == id)
      return UNKNOWN_CONST_REGISTER_COUNT_REQUIREMENT;
    if (auto shaderGroup = getShaderGroupForProgram(id))
      return shaderGroup->getComputeProgramImplicitCbufRegCount(id);
    return getImplicitCbufRegCount(computeProgramImplicitCbufRegCount, id.getIndex());
  }
  void reset()
  {
    for (auto &shaderGroup : groups)
      shaderGroup.clear();
    vertexShaderIds.clear();
    pixelShaderIds.clear();
    computeIds.clear();
    vertexShaderImplicitCbufRegCount.clear();
    pixelShaderImplicitCbufRegCount.clear();
    computeProgramImplicitCbufRegCount.clear();
    graphicsProgramTemplates.clear();
    clearGraphicsProgramsInstances();
  }

  GraphicsProgramID findGraphicsProgram(ShaderID vs, ShaderID ps) const
  {
    if (auto shaderGroup = getShaderGroupForShaders(vs, ps))
    {
      return shaderGroup->findGraphicsProgram(groups.data(), vs, ps);
    }
    auto ref = eastl::find_if(begin(graphicsProgramTemplates), end(graphicsProgramTemplates),
      [vs, ps](const auto &pair) { return vs == pair.vertexShader && ps == pair.pixelShader; });
    if (ref == end(graphicsProgramTemplates))
    {
      return GraphicsProgramID::Null();
    }
    else
    {
      return GraphicsProgramID::make(0, ref - begin(graphicsProgramTemplates));
    }
  }

  GraphicsProgramID addGraphicsProgram(ShaderID vs, ShaderID ps)
  {
    if (auto shaderGroup = getShaderGroupForShaders(vs, ps))
    {
      return shaderGroup->addGraphicsProgram(groups.data(), vs, ps);
    }
    GraphicsProgramID id;
    auto ref = eastl::find_if(begin(graphicsProgramTemplates), end(graphicsProgramTemplates),
      [](const auto &pair) { return ShaderID::Null() == pair.vertexShader; });
    if (ref == end(graphicsProgramTemplates))
    {
      id = GraphicsProgramID::make(0, graphicsProgramTemplates.size());
      graphicsProgramTemplates.push_back(GraphicsProgramTemplate{vs, ps});
    }
    else
    {
      id = GraphicsProgramID::make(0, ref - begin(graphicsProgramTemplates));
      ref->vertexShader = vs;
      ref->pixelShader = ps;
    }
    return id;
  }

  template <typename T>
  void iterateAllGraphicsPrograms(T clb)
  {
    for (auto &shaderGroup : groups)
    {
      shaderGroup.iterateGraphicsPrograms(groups.data(), clb);
    }
    for (size_t i = 0; i < graphicsProgramTemplates.size(); ++i)
    {
      if (ShaderID::Null() != graphicsProgramTemplates[i].vertexShader)
      {
        clb(GraphicsProgramID::make(0, i));
      }
    }
  }

  template <typename T>
  void removeGraphicsProgramWithMatchingVertexShader(ShaderID vs, T clb)
  {
    if (auto shaderGroup = getShaderGroupForShader(vs))
    {
      shaderGroup->removeGraphicsProgramWithMatchingVertexShader(groups.data(), vs, clb);
    }

    // should a vertex shader with shaderGroup 1 be used with a pixel shader of shaderGroup 2
    // it will end up be in the global shaderGroup
    for (size_t i = 0; i < graphicsProgramTemplates.size(); ++i)
    {
      if (vs == graphicsProgramTemplates[i].vertexShader)
      {
        clb(GraphicsProgramID::make(0, i));
        graphicsProgramTemplates[i].vertexShader = ShaderID::Null();
        graphicsProgramTemplates[i].pixelShader = ShaderID::Null();
      }
    }
  }

  template <typename T>
  void removeGraphicsProgramWithMatchingPixelShader(ShaderID ps, T clb)
  {
    if (auto shaderGroup = getShaderGroupForShader(ps))
    {
      shaderGroup->removeGraphicsProgramWithMatchingPixelShader(groups.data(), ps, clb);
    }
    else
    {
      // ps with shaderGroup 0 can be put in any shaderGroup, the vs will pull it in
      for (auto &shaderGroup : groups)
      {
        shaderGroup.removeGraphicsProgramWithMatchingPixelShader(groups.data(), ps, clb);
      }
    }

    // either if in shaderGroup 0 or in a shaderGroup other than 1 and combined with a vs that is not in the same shaderGroup
    // results being put into the global shaderGroup
    for (size_t i = 0; i < graphicsProgramTemplates.size(); ++i)
    {
      if (ps == graphicsProgramTemplates[i].pixelShader)
      {
        clb(GraphicsProgramID::make(0, i));
        graphicsProgramTemplates[i].vertexShader = ShaderID::Null();
        graphicsProgramTemplates[i].pixelShader = ShaderID::Null();
      }
    }
  }

  void clearGraphicsProgramsInstances()
  {
    graphicsProgramsHashMap.clear();
    graphicsProgramsHashMapDead = 0;
    graphicsProgramInstances.clear();
  }
  ProgramID incrementCachedGraphicsProgram(hash_key_t key)
  {
    uint32_t i = graphicsProgramsHashMap.findOr(key, ~uint32_t(0));
    if (i != ~uint32_t(0))
    {
      G_ASSERT(graphicsProgramInstances[i].getRefCount() > 0);
      graphicsProgramInstances[i].addRef();
      return ProgramID::asGraphicsProgram(0, i);
    }
    return ProgramID::Null();
  }
  hash_key_t getGraphicsProgramKey(InputLayoutID vdecl, ShaderID vs, ShaderID ps) const
  {
    constexpr int psBits = 24, vsBits = 24, vdeclBits = sizeof(hash_key_t) * 8 - psBits - vsBits; // 16 bits
    const hash_key_t vsId = int(vs.exportValue());
    const hash_key_t psId = int(ps.exportValue() + 1);
    G_ASSERTF(vsId < (1 << vsBits) && psId < (1 << psBits) && uint32_t(vdecl.get() + 1) < (1 << vdeclBits), "%d %d %d",
      uint32_t(vdecl.get()), vsId, psId);
    return hash_key_t(uint32_t(vdecl.get() + 1)) | (vsId << vdeclBits) | (psId << (vdeclBits + vsBits));
  }
  ProgramID instanciateGraphicsProgram(hash_key_t key, GraphicsProgramID gp, InputLayoutID il)
  {
    auto refEmpty = end(graphicsProgramInstances);
    auto refSame = eastl::find_if(begin(graphicsProgramInstances), end(graphicsProgramInstances), [&](auto &instance) {
      if (GraphicsProgramID::Null() == instance.internalProgram)
        refEmpty = &instance;
      return instance.internalProgram == gp && instance.inputLayout == il;
    });
    if (refSame != end(graphicsProgramInstances))
    {
      refSame->addRef();
      return ProgramID::asGraphicsProgram(0, refSame - begin(graphicsProgramInstances));
    }

    if (refEmpty == end(graphicsProgramInstances))
      refEmpty = graphicsProgramInstances.emplace(refEmpty);
    refEmpty->addRef();
    refEmpty->inputLayout = il;
    refEmpty->internalProgram = gp;
    const uint32_t pid = refEmpty - begin(graphicsProgramInstances);
    graphicsProgramsHashMap.emplace_if_missing(key, pid);
    return ProgramID::asGraphicsProgram(0, pid);
  }

  GraphicsProgramUsageInfo getUsageInfo(ProgramID program) const
  {
    GraphicsProgramUsageInfo info;
    const auto &inst = graphicsProgramInstances[program.getIndex()];
    info.input = inst.inputLayout;
    info.programId = inst.internalProgram;
    if (GraphicsProgramID::Null() != inst.internalProgram)
    {
      GraphicsProgramTemplate shaders;
      if (auto shaderGroup = getShaderGroupForGraphicsProgram(inst.internalProgram))
        shaders = shaderGroup->getShadersOfGraphicsProgram(inst.internalProgram);
      else
        shaders = graphicsProgramTemplates[inst.internalProgram.getIndex()];
      info.vsImplicitCbufRegCount = getVertexShaderImplicitCbufRegCount(shaders.vertexShader);
      info.psImplicitCbufRegCount = getPixelShaderImplicitCbufRegCount(shaders.pixelShader);
    }

    return info;
  }

  InputLayoutID getGraphicsProgramInstanceInputLayout(ProgramID prog) const
  {
    return graphicsProgramInstances[prog.getIndex()].inputLayout;
  }
  void removeInstanceFromHashmap(uint32_t id)
  {
    // we could store key inside GraphicsProgramInstance, but those 64 bit are only used during remove of last refCount, and it is very
    // rare
    graphicsProgramsHashMap.iterate([&](auto, uint32_t &v) {
      if (v == id)
      {
        v = ~uint32_t(0);
        graphicsProgramsHashMapDead++;
      }
    });
  }
  void removeAllDeads()
  {
    if (graphicsProgramsHashMapDead == 0)
      return;
    decltype(graphicsProgramsHashMap) temp;
    temp.reserve(eastl::max<int>(0, graphicsProgramsHashMap.size() - graphicsProgramsHashMapDead));
    graphicsProgramsHashMap.iterate([&](auto key, auto v) {
      if (v != ~uint32_t(0))
        temp.emplace_if_missing(key, v);
    });
    graphicsProgramsHashMap.swap(temp);
  }
  void removeGraphicsProgramInstance(ProgramID prog)
  {
    if (graphicsProgramInstances[prog.getIndex()].delRef())
    {
      removeInstanceFromHashmap(prog.getIndex());
      if (graphicsProgramsHashMapDead > graphicsProgramsHashMap.size() / 20) // too many deads, rehash
        removeAllDeads();
    }
  }

  void removeGraphicsProgramInstancesUsingMatchingTemplate(GraphicsProgramID templ)
  {
    for (auto &inst : graphicsProgramInstances)
    {
      if (inst.internalProgram == templ)
      {
        inst.deleteAllReferences();
        removeInstanceFromHashmap(&inst - begin(graphicsProgramInstances));
      }
    }
  }

  void setGroup(uint32_t index, ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader)
  {
    groups[index - 1].setup(groups.data(), dump, null_pixel_shader);
  }
  void dropGroup(uint32_t index)
  {
    groups[index - 1].clear();
    for (auto &inst : graphicsProgramInstances)
    {
      if (inst.internalProgram.getGroup() == index)
      {
        inst.deleteAllReferences();
        removeInstanceFromHashmap(&inst - begin(graphicsProgramInstances));
      }
    }
    // tidy up the hash map
    removeAllDeads();
  }
  dxil::ShaderStage shaderFromIndex(uint32_t group_index, uint32_t shader_index, ShaderCodeType type, void *ident)
  {
    return groups[group_index - 1].shaderFromIndex(groups.data(), shader_index, type, ident);
  }
};

eastl::unique_ptr<VertexShaderModule> decode_vertex_shader(const ShaderSourceExt &source);
eastl::unique_ptr<PixelShaderModule> decode_pixel_shader(const ShaderSourceExt &source);
ComputeShaderModule decode_compute_shader(const ShaderSource &source);

struct StageShaderModuleInBinaryRef : StageShaderModuleHeader
{
  eastl::span<const uint8_t> metadata;
  eastl::span<const uint8_t> byteCode;
};
inline uint32_t offset_to_base(const void *base, const StageShaderModuleInBinaryRef &shader)
{
  auto value = shader.byteCode.data() - reinterpret_cast<const uint8_t *>(base);
  if (value < 0)
  {
    DAG_FATAL("DX12: Code offset %lld was negative", int64_t{value});
  }
  if (value > eastl::numeric_limits<uint32_t>::max())
  {
    DAG_FATAL("DX12: Code offset %lld exceeded 32 bits range", int64_t{value});
  }

  return static_cast<uint32_t>(value);
}
using PixelShaderModuleInBinaryRef = StageShaderModuleInBinaryRef;

struct VertexShaderModuleInBinaryRef : StageShaderModuleInBinaryRef
{
  // TODO remove shader identifier as its only relevant for the vertex shader it self
  StageShaderModuleInBinaryRef geometryShader;
  StageShaderModuleInBinaryRef hullShader;
  StageShaderModuleInBinaryRef domainShader;
  eastl::span<const dxil::StreamOutputComponentInfo> streamOutputDesc;
};

VertexShaderModuleInBinaryRef decode_vertex_shader_ref(const void *data, uint32_t size, const uint8_t *bytecode);
PixelShaderModuleInBinaryRef decode_pixel_shader_ref(const void *data, uint32_t size, const uint8_t *bytecode);

struct InputLayoutIDWithHash
{
  InputLayoutID id;
  dxil::HashValue hash;
};

class PipelineCache;
class ShaderProgramDatabase
{
  mutable OSReadWriteLock dataGuard;
  dag::Vector<InputLayout> publicImputLayoutTable;

  ProgramID debugProgram = ProgramID::Null();
  ShaderID nullPixelShader = ShaderID::Null();
  bool disablePreCache = false;

  ShaderProgramGroups shaderProgramGroups;

  void initDebugProgram(DeviceContext &ctx);
  void initNullPixelShader(DeviceContext &ctx);

public:
  ProgramID newComputeProgram(DeviceContext &ctx, const ShaderSourceExt &source, CSPreloaded preloaded);
  ProgramID newGraphicsProgram(DeviceContext &ctx, InputLayoutID vdecl, ShaderID vs, ShaderID ps);
  InputLayoutID getInputLayoutForGraphicsProgram(ProgramID program);
  GraphicsProgramUsageInfo getGraphicsProgramForStateUpdate(ProgramID program);
  ComputeProgramUsageInfo getComputeProgramForStateUpdate(ProgramID program);
  InputLayoutID registerInputLayoutInternal(DeviceContext &ctx, const InputLayout &layout);
  InputLayoutID registerInputLayout(DeviceContext &ctx, const InputLayout &layout);
  void setup(DeviceContext &ctx, bool disable_precache);
  void shutdown(DeviceContext &ctx);
  ShaderID newVertexShader(DeviceContext &ctx, const ShaderSourceExt &source);
  ShaderID newPixelShader(DeviceContext &ctx, const ShaderSourceExt &source);
  ProgramID getDebugProgram();
  void removeProgram(DeviceContext &ctx, ProgramID prog);
  void deleteVertexShader(DeviceContext &ctx, ShaderID shader);
  void deletePixelShader(DeviceContext &ctx, ShaderID shader);
  ShaderID newRawVertexShader(DeviceContext &ctx, const dxil::ShaderHeader &header, dag::ConstSpan<uint8_t> byte_code);
  ShaderID newRawPixelShader(DeviceContext &ctx, const dxil::ShaderHeader &header, dag::ConstSpan<uint8_t> byte_code);

  void registerShaderBinDump(DeviceContext &ctx, ScriptedShadersBinDumpOwner *dump, const char *name);
  void getBindumpShader(DeviceContext &ctx, uint32_t index, ShaderCodeType type, void *ident);
  DynamicArray<InputLayoutIDWithHash> loadInputLayoutFromBlk(DeviceContext &ctx, const DataBlock *blk, const char *default_format);
  ShaderID getNullPixelShader(DeviceContext &ctx)
  {
    if (ShaderID::Null() == nullPixelShader)
    {
      initNullPixelShader(ctx);
    }
    return nullPixelShader;
  }
};

inline dxil::HashValue calculate_hash(const ScriptedShadersBinDump &v1, const shaderbindump::ShaderClass &shader_class)
{
  ShaderHashValue result{};
  {
    ShaderHashValue::CalculateContext context{result};
    context(shader_class.code.size());
    shader_class.stVariants.appendToHashContext(context, [&v1](auto &ctx, uint32_t id) {
      auto &name = v1.varMap[id];
      ctx(name.c_str(), name.length());
    });
    for (auto &d : shader_class.code)
    {
      context(d.passes.size());
      d.dynVariants.appendToHashContext(context, [&v1](auto &ctx, uint32_t id) {
        auto &name = v1.varMap[id];
        ctx(name.c_str(), name.length());
      });
    }
  }
  return result;
}

namespace backend
{
class InputLayoutManager
{
  struct MaskToInternalMapEntry
  {
    uint32_t mask;
    InternalInputLayoutID id;
  };
  struct ExternalToInternalMapEntry
  {
    InputLayout layout;
    dag::Vector<MaskToInternalMapEntry> mapTable;
  };
  dag::Vector<InputLayout> internalLayoutTable;
  dag::Vector<ExternalToInternalMapEntry> externalToInternalMap;
  InternalInputLayoutID nullLayoutID;

public:
  void init() { nullLayoutID = addInternalLayout({}); }
  void registerInputLayout(InputLayoutID id, const InputLayout &layout)
  {
    if (externalToInternalMap.size() <= id.get())
    {
      externalToInternalMap.resize(id.get() + 1);
    }
    auto &e = externalToInternalMap[id.get()];
    e.layout = layout;
  }

  InternalInputLayoutID remapInputLayout(InputLayoutID id, uint32_t layout_mask)
  {
    if (InputLayoutID::Null() == id)
    {
      return nullLayoutID;
    }

    if (externalToInternalMap.size() <= id.get())
    {
      D3D_ERROR("dx12: trying to use invalid input layout ID %u registered layouts %u", id.get(), externalToInternalMap.size());
      return nullLayoutID;
    }

    auto &e = externalToInternalMap[id.get()];
    auto ref = eastl::find_if(begin(e.mapTable), end(e.mapTable), [layout_mask](auto &value) { return value.mask == layout_mask; });
    if (end(e.mapTable) != ref)
    {
      return ref->id;
    }
    auto internalID = addInternalLayout(e.layout.getLayoutForAttributeUse(layout_mask));
    MaskToInternalMapEntry v{layout_mask, internalID};
    e.mapTable.push_back(v);
    return internalID;
  }

  const InputLayout &getInputLayout(InternalInputLayoutID id) const { return internalLayoutTable[id.get()]; }

  InternalInputLayoutID addInternalLayout(const InputLayout &layout)
  {
    auto ref = eastl::find(begin(internalLayoutTable), end(internalLayoutTable), layout);
    if (ref == end(internalLayoutTable))
    {
      ref = internalLayoutTable.insert(end(internalLayoutTable), layout);
    }
    return InternalInputLayoutID{int(ref - begin(internalLayoutTable))};
  }
};

class ScriptedShadersBinDumpManager
{
  using UncompressedMemoryProvider = Tab<uint8_t>;
  // Shader hashes are SHA1 digests, so any 8 leading bytes are already a well-distributed key.
  struct ShaderHashValueHasher
  {
    size_t operator()(const dxil::HashValue &h) const
    {
      size_t v = 0;
      memcpy(&v, h.value, sizeof(v));
      return v;
    }
  };
  using ShaderHashLookup = ska::flat_hash_map<dxil::HashValue, uint32_t, ShaderHashValueHasher>;
  struct ScriptedShadersBinDumpState
  {
    // randomly chosen, sounds like a good value
    static constexpr uint32_t active_slot_vector_fixed_size = 8;
    ScriptedShadersBinDumpOwner *owner = nullptr;
    eastl::unique_ptr<UncompressedMemoryProvider[]> decompressedShaders;
    eastl::fixed_vector<uint32_t, active_slot_vector_fixed_size> activeSlots;
    size_t decompressedShadersSize = 0;
    eastl::string name;
    ShaderHashLookup vprHashToIndex;
    ShaderHashLookup computeHashToIndex;

    void buildShaderHashLookup()
    {
      vprHashToIndex.clear();
      computeHashToIndex.clear();
      auto v2 = owner ? owner->getDumpV2() : nullptr;
      if (!v2)
        return;
      const uint32_t vprCount = v2->vprCount;
      const uint32_t total = v2->shaderHashes.size();
      vprHashToIndex.reserve(vprCount);
      computeHashToIndex.reserve(total > vprCount ? total - vprCount : 0);
      for (uint32_t i = 0; i < vprCount && i < total; ++i)
        vprHashToIndex.emplace(v2->shaderHashes[i], i);
      for (uint32_t i = vprCount; i < total; ++i)
        computeHashToIndex.emplace(v2->shaderHashes[i], i - vprCount);
    }
  };
  ScriptedShadersBinDumpState dumps[max_scripted_shaders_bin_groups]{};
  OSSpinlock decompressionCacheLock;

public:
  ScriptedShadersBinDumpOwner *getDump(uint32_t group) { return dumps[group].owner; }
  template <typename T>
  void enumerateShaderFromHash(const dxil::HashValue &hash, T receiver) const
  {
    for (uint32_t gi = 0; gi < countof(dumps); ++gi)
    {
      auto &group = dumps[gi];
      if (!group.owner)
      {
        continue;
      }
      auto v2 = group.owner->getDumpV2();
      if (!v2)
      {
        continue;
      }
      for (uint32_t si = 0; si < v2->shaderHashes.size(); ++si)
      {
        if (hash != v2->shaderHashes[si])
        {
          continue;
        }
        if (!receiver(gi, si, v2->vprCount))
        {
          break;
        }
      }
    }
  }
  void setDumpOfGroup(uint32_t shaderGroup, ScriptedShadersBinDumpOwner *dump, eastl::string_view name)
  {
    auto &target = dumps[shaderGroup];
    target.owner = dump;
    target.decompressedShaders = eastl::make_unique<UncompressedMemoryProvider[]>(dump->getDump()->shaders.size());
    target.decompressedShadersSize = 0;
    target.name = name;
    target.buildShaderHashLookup();
    target.activeSlots.clear(true);
  }
  void resetDumpOfGroup(uint32_t shaderGroup)
  {
    auto &target = dumps[shaderGroup];
    target.owner = nullptr;
    target.decompressedShaders.reset();
    target.decompressedShadersSize = 0;
    target.name.clear();
    ShaderHashLookup{}.swap(target.vprHashToIndex);
    ShaderHashLookup{}.swap(target.computeHashToIndex);
    target.activeSlots.clear(true);
  }
  // returned memory range is valid until the end of the frame, after that its undefined.
  struct GetShaderByteCodeResult
  {
    eastl::span<const uint8_t> byteCode;
    bool cacheHit;
  };
  struct GetShaderByteCodeResultMeta
  {
    eastl::span<const uint8_t> metadata;
    eastl::span<uint8_t> byteCode;
    bool cacheHit;
  };
  GetShaderByteCodeResultMeta getShaderByteCode(uint32_t shaderGroup, uint32_t compression_index)
  {
    OSSpinlockScopedLock lock{decompressionCacheLock};
    auto &dump = dumps[shaderGroup];
    if (!dump.decompressedShaders)
    {
      return {.byteCode = {nullptr, nullptr}, .cacheHit = false};
    }
    auto src = dump.owner->getCodeById(compression_index);
    auto &shader = dump.decompressedShaders[compression_index];
    bool cacheHit = true;
    if (!shader.size())
    {
      auto src = dump.owner->getCodeById(compression_index);
      src.uncompress(shader);
      dump.decompressedShadersSize += shader.size();
      cacheHit = false;
      dump.activeSlots.push_back(compression_index);
    }
    return {.metadata = src.metadata, .byteCode = {shader.begin(), shader.end()}, .cacheHit = cacheHit};
  }

  void evictDecompressionCache()
  {
    OSSpinlockScopedLock lock{decompressionCacheLock};
    for (auto &dump : dumps)
    {
      if (dump.activeSlots.empty())
        continue;
      // sort them first to make the access pattern predictable and improve cache hit rate
      eastl::sort(dump.activeSlots.begin(), dump.activeSlots.end());
      for (auto &i : dump.activeSlots)
      {
        UncompressedMemoryProvider{}.swap(dump.decompressedShaders[i]);
      }

      // free overflow memory as soon as we no longer need it
      dump.activeSlots.clear(dump.activeSlots.size() < dump.activeSlots.max_size());
      dump.decompressedShadersSize = 0;
    }
  }

  size_t getBytecodeCacheMemoryUsage()
  {
    OSSpinlockScopedLock lock{decompressionCacheLock};
    size_t totalMemory = 0;
    for (const auto &dump : dumps)
      totalMemory += dump.decompressedShadersSize;
    return totalMemory;
  }

  const eastl::string &getGroupName(uint32_t group) const { return dumps[group].name; }

  eastl::string_view getShaderClassName(uint32_t group, uint32_t class_index) const
  {
    if (group >= max_scripted_shaders_bin_groups)
    {
      return {"<group index out of range>"};
    }
    if (!dumps[group].owner)
    {
      return "<dump owner was null>";
    }
    auto v2 = dumps[group].owner->getDumpV2();
    if (!v2)
    {
      return {"<dump v2 was null>"};
    }
    if (class_index >= v2->classes.size())
    {
      return {"<class index out of range>"};
    }

    auto &name = v2->classes[class_index].name;
    return {static_cast<const char *>(name), name.size() - 1};
  }

  struct ShaderInGroupIndex
  {
    uint32_t group;
    uint32_t shader;
  };

  ShaderInGroupIndex findVertexShader(uint32_t start_group, const dxil::HashValue &hash) const
  {
    for (uint32_t groupIndex = start_group; groupIndex < max_scripted_shaders_bin_groups; ++groupIndex)
    {
      auto &group = dumps[groupIndex];
      if (!group.owner)
      {
        continue;
      }
      auto it = group.vprHashToIndex.find(hash);
      if (it != group.vprHashToIndex.end())
      {
        return {groupIndex, it->second};
      }
    }
    return {max_scripted_shaders_bin_groups, 0};
  }

  ShaderInGroupIndex findPixelShader(uint32_t start_group, const dxil::HashValue &hash) const
  {
    // pixel and compute shaders occupy the same second part of the shader table
    return findComputeShader(start_group, hash);
  }

  ShaderInGroupIndex findComputeShader(uint32_t start_group, const dxil::HashValue &hash) const
  {
    for (uint32_t groupIndex = start_group; groupIndex < max_scripted_shaders_bin_groups; ++groupIndex)
    {
      auto &group = dumps[groupIndex];
      if (!group.owner)
      {
        continue;
      }
      auto it = group.computeHashToIndex.find(hash);
      if (it != group.computeHashToIndex.end())
      {
        return {groupIndex, it->second};
      }
    }
    return {max_scripted_shaders_bin_groups, 0};
  }

  // A dump can hold the same byte code under several indices while findVertexShader() and
  // findPixelShader() answer with the first of them. Every index used to key a pass or to look one
  // up has to be reduced to that first index, or the two sides name the same shader differently.
  uint32_t canonicalVertexShaderIndex(uint32_t group, uint32_t vpr_id)
  {
    auto &dumpGroup = dumps[group];
    auto v2 = dumpGroup.owner ? dumpGroup.owner->getDumpV2() : nullptr;
    if (!v2 || 0xFFFF == vpr_id || vpr_id >= uint32_t(v2->vprCount) || vpr_id >= v2->shaderHashes.size())
    {
      return vpr_id;
    }
    auto it = dumpGroup.vprHashToIndex.find(v2->shaderHashes[vpr_id]);
    return it != dumpGroup.vprHashToIndex.end() ? it->second : vpr_id;
  }

  uint32_t canonicalPixelShaderIndex(uint32_t group, uint32_t fsh_id)
  {
    auto &dumpGroup = dumps[group];
    auto v2 = dumpGroup.owner ? dumpGroup.owner->getDumpV2() : nullptr;
    if (!v2 || 0xFFFF == fsh_id || fsh_id >= uint32_t(v2->fshCount))
    {
      return fsh_id;
    }
    const uint32_t dumpIndex = uint32_t(v2->vprCount) + fsh_id;
    if (dumpIndex >= v2->shaderHashes.size())
    {
      return fsh_id;
    }
    auto it = dumpGroup.computeHashToIndex.find(v2->shaderHashes[dumpIndex]);
    return it != dumpGroup.computeHashToIndex.end() ? it->second : fsh_id;
  }

  struct RenderStateIterationInfo
  {
    uint32_t group;
    uint32_t index;
    uint32_t distance;
  };

  RenderStateIterationInfo walkRenderStates(uint32_t start_group, uint32_t search_start, uint32_t current_distance,
    const RenderStateSystem::StaticState &static_state)
  {
    for (uint32_t groupIndex = start_group; groupIndex < max_scripted_shaders_bin_groups; ++groupIndex)
    {
      auto &group = dumps[groupIndex];
      if (!group.owner)
      {
        continue;
      }
      auto v2 = group.owner->getDumpV2();
      if (!v2)
      {
        continue;
      }

      constexpr uint32_t max_distance = 0xFFFFFFFF;
      uint32_t nextDistance = max_distance;
      uint32_t nextDistanceIndex = 0;
      for (uint32_t stateIndex = search_start; stateIndex < v2->renderStates.size(); ++stateIndex)
      {
        auto inDriver = RenderStateSystem::StaticState::fromRenderState(v2->renderStates[stateIndex]);
        auto distance = inDriver.distance(static_state);
        if (distance < current_distance)
        {
          continue;
        }
        if (distance > current_distance)
        {
          if (distance < nextDistance)
          {
            nextDistanceIndex = stateIndex;
            nextDistance = distance;
          }
          continue;
        }
        return {groupIndex, stateIndex, current_distance};
      }
      for (uint32_t stateIndex = 0; stateIndex < search_start; ++stateIndex)
      {
        auto inDriver = RenderStateSystem::StaticState::fromRenderState(v2->renderStates[stateIndex]);
        auto distance = inDriver.distance(static_state);
        if (distance < current_distance)
        {
          continue;
        }
        if (distance == nextDistance)
        {
          nextDistanceIndex = min(nextDistanceIndex, stateIndex);
          continue;
        }
        if (distance > current_distance)
        {
          if (distance < nextDistance)
          {
            nextDistanceIndex = stateIndex;
            nextDistance = distance;
          }
        }
      }
      if (max_distance != nextDistance)
      {
        return {groupIndex, nextDistanceIndex, nextDistance};
      }
    }
    return {max_scripted_shaders_bin_groups, 0, 0};
  }

  struct GraphicsPassIndex
  {
    struct PassRef
    {
      uint32_t classIndex;
      uint32_t staticIndex;
      uint32_t dynamicIndex;
      uint16_t renderState;
    };
    using PassRefs = dag::Vector<PassRef>;
    ska::flat_hash_map<uint32_t, PassRefs> byVsPs;
    dag::Vector<PassRefs> byVs;
    dag::Vector<PassRefs> byPs;
    dag::Vector<RenderStateSystem::StaticState> staticStates;
    bool built = false;
    bool valid = false;

    const PassRefs *passesOfVsPs(uint32_t vs, uint32_t ps) const
    {
      auto it = byVsPs.find((vs << 16) | ps);
      return it == byVsPs.end() ? nullptr : &it->second;
    }
    const PassRefs *passesOfVs(uint32_t vs) const { return vs < byVs.size() ? &byVs[vs] : nullptr; }
    const PassRefs *passesOfPs(uint32_t ps) const { return ps < byPs.size() ? &byPs[ps] : nullptr; }
  };

  struct GraphicsPassIndexCache
  {
    GraphicsPassIndex perGroup[max_scripted_shaders_bin_groups];
  };

  bool buildGraphicsPassIndex(uint32_t group, GraphicsPassIndex &index)
  {
    auto &dumpGroup = dumps[group];
    if (!dumpGroup.owner)
    {
      return false;
    }
    auto v2 = dumpGroup.owner->getDumpV2();
    if (!v2)
    {
      return false;
    }

    index.staticStates.reserve(v2->renderStates.size());
    for (auto &rs : v2->renderStates)
    {
      index.staticStates.push_back(RenderStateSystem::StaticState::fromRenderState(rs));
    }

    index.byVs.resize(v2->vprCount);
    index.byPs.resize(v2->fshCount);

    for (uint32_t ci = 0; ci < v2->classes.size(); ++ci)
    {
      auto &cls = v2->classes[ci];
      for (uint32_t si = 0; si < cls.code.size(); ++si)
      {
        auto &sVar = cls.code[si];
        for (uint32_t di = 0; di < sVar.passes.size(); ++di)
        {
          if (!sVar.passes[di].rpass)
          {
            continue;
          }
          auto &pass = *sVar.passes[di].rpass;
          const GraphicsPassIndex::PassRef ref{ci, si, di, pass.renderStateNo};
          const uint32_t vprId = canonicalVertexShaderIndex(group, pass.vprId);
          const uint32_t fshId = canonicalPixelShaderIndex(group, pass.fshId);
          index.byVsPs[(vprId << 16) | fshId].push_back(ref);
          if (vprId < index.byVs.size())
          {
            index.byVs[vprId].push_back(ref);
          }
          if (fshId < index.byPs.size())
          {
            index.byPs[fshId].push_back(ref);
          }
        }
      }
    }
    index.valid = !index.staticStates.empty();
    return index.valid;
  }

  GraphicsPassIndex &getGraphicsPassIndex(GraphicsPassIndexCache &cache, uint32_t group)
  {
    auto &index = cache.perGroup[group];
    if (!index.built)
    {
      index.built = true;
      buildGraphicsPassIndex(group, index);
    }
    return index;
  }
};

struct StageShaderModuleBytecode
{
  ShaderSource source;
  uint32_t bytecodeOffset = 0;
  uint32_t bytecodeSize = 0;
};

struct VertexShaderModuleBytecode : StageShaderModuleBytecode
{
  eastl::unique_ptr<StageShaderModuleBytecode[]> subShaders;
};

using PixelShaderModuleBytecode = StageShaderModuleBytecode;

struct VertexShaderModuleBytecodeRef
{
  const VertexShaderModuleBytecode *bytecode = nullptr;
};

struct PixelShaderModuleBytecodeRef
{
  const PixelShaderModuleBytecode *bytecode = nullptr;
};

struct StageShaderModuleBytcodeInDumpOffsets
{
  uint32_t shaderOffset = 0;
  uint32_t shaderSize = 0;
};

struct StageShaderModuleBytcodeInDump : StageShaderModuleBytcodeInDumpOffsets
{
  // Indexes the combined vertex plus pixel and compute shader table of the dump, so it counts up
  // to vprCount + fshCount while a pass can only name 65535 of either kind on its own.
  uint32_t compressionIndex = 0;
};

struct VertexShaderModuleBytcodeInDump : StageShaderModuleBytcodeInDump
{
  eastl::unique_ptr<StageShaderModuleBytcodeInDumpOffsets[]> subShaders;
};

using PixelShaderModuleBytcodeInDump = StageShaderModuleBytcodeInDump;

struct VertexShaderModuleBytcodeInDumpRef
{
  uint32_t shaderGroup = 0;
  const VertexShaderModuleBytcodeInDump *bytecode = nullptr;
};

struct PixelShaderModuleBytcodeInDumpRef
{
  uint32_t shaderGroup = 0;
  const PixelShaderModuleBytcodeInDump *bytecode = nullptr;
};

struct VertexShaderModuleHeader
{
  dxil::HashValue hash = {};
  dxil::ShaderHeader header = {};
  eastl::unique_ptr<dxil::ShaderHeader[]> subShaders;
  eastl::string debugName;
  DynamicArray<dxil::StreamOutputComponentInfo> streamOutputDesc;
  bool hasGsOrAs : 1;
  bool hasDsAndHs : 1;

  VertexShaderModuleHeader() : hasGsOrAs{false}, hasDsAndHs{false} {}
};

struct PixelShaderModuleHeader
{
  dxil::HashValue hash = {};
  dxil::ShaderHeader header = {};
  eastl::string debugName;
};

struct VertexShaderModuleRefStore
{
  VertexShaderModuleHeader &header;
  eastl::variant<VertexShaderModuleBytecodeRef, VertexShaderModuleBytcodeInDumpRef> bytecode;

  const uint8_t *uncompress(Tab<uint8_t> &tmpstorage) const
  {
    if (auto *vs = eastl::get_if<VertexShaderModuleBytecodeRef>(&bytecode))
    {
      vs->bytecode->source.uncompress(tmpstorage);
      return tmpstorage.data();
    }
    return nullptr;
  }
};

inline const dxil::ShaderHeader *get_gs(const VertexShaderModuleRefStore &ref)
{
  return ref.header.hasGsOrAs ? &ref.header.subShaders[0] : nullptr;
}

inline const dxil::ShaderHeader *get_hs(const VertexShaderModuleRefStore &ref)
{
  return ref.header.hasDsAndHs ? &ref.header.subShaders[ref.header.hasGsOrAs ? 1 : 0] : nullptr;
}

inline const dxil::ShaderHeader *get_ds(const VertexShaderModuleRefStore &ref)
{
  return ref.header.hasDsAndHs ? &ref.header.subShaders[ref.header.hasGsOrAs ? 2 : 1] : nullptr;
}

inline const dxil::ShaderHeader *get_as(const VertexShaderModuleRefStore &ref)
{
  return ref.header.hasGsOrAs ? &ref.header.subShaders[0] : nullptr;
}

#if _TARGET_XBOXONE
// XB1 has no mesh shader stage
inline constexpr bool is_mesh(const VertexShaderModuleRefStore &) { return false; }
#else
inline bool is_mesh(const VertexShaderModuleRefStore &ref)
{
  return dxil::ShaderStage::MESH == static_cast<dxil::ShaderStage>(ref.header.header.shaderType);
}
#endif

struct PixelShaderModuleRefStore
{
  const PixelShaderModuleHeader &header;
  eastl::variant<PixelShaderModuleBytecodeRef, PixelShaderModuleBytcodeInDumpRef> bytecode;

  const uint8_t *uncompress(Tab<uint8_t> &tmpstorage) const
  {
    if (auto *ps = eastl::get_if<PixelShaderModuleBytecodeRef>(&bytecode))
    {
      ps->bytecode->source.uncompress(tmpstorage);
      return tmpstorage.data();
    }
    return nullptr;
  }
};

class PipelineNameGenerator : public ScriptedShadersBinDumpManager
{
  using BaseType = ScriptedShadersBinDumpManager;

  void iterateShaderClassesForComputeShader(const dxil::HashValue &hash, eastl::string &out_name)
  {
    auto [group, index] = findComputeShader(0, hash);
    if (group >= max_scripted_shaders_bin_groups)
    {
      return;
    }

    auto v2 = getDump(group)->getDumpV2();
    if (!v2)
      return;

    const auto &nameSettings = getDump(group)->shaderNameFormatSettings;

    for (auto &cls : v2->classes)
    {
      auto ref = eastl::find_if(eastl::begin(cls.shrefStorage), eastl::end(cls.shrefStorage),
        [fsh = index](auto &def) { return fsh == def.fshId; });
      if (ref == eastl::end(cls.shrefStorage))
      {
        continue;
      }

      for (uint32_t si = 0; si < cls.code.size(); ++si)
      {
        auto &sVar = cls.code[si];
        for (uint32_t di = 0; di < sVar.passes.size(); ++di)
        {
          auto &dVar = sVar.passes[di];
          if (dVar.rpass->fshId != index)
          {
            continue;
          }

          generateNamesForShaderClassCodes(v2, cls, si, di, out_name, nameSettings);
        }
      }
    }
  }

  template <typename U>
  void visitShaderClassesForComputeShader(const dxil::HashValue &hash, U handler)
  {
    auto [group, index] = findComputeShader(0, hash);
    if (group >= max_scripted_shaders_bin_groups)
    {
      return;
    }

    auto v2 = getDump(group)->getDumpV2();
    if (!v2)
      return;

    for (auto &cls : v2->classes)
    {
      auto ref = eastl::find_if(eastl::begin(cls.shrefStorage), eastl::end(cls.shrefStorage),
        [fsh = index](auto &def) { return fsh == def.fshId; });
      if (ref == eastl::end(cls.shrefStorage))
      {
        continue;
      }

      for (uint32_t si = 0; si < cls.code.size(); ++si)
      {
        auto &sVar = cls.code[si];
        for (uint32_t di = 0; di < sVar.passes.size(); ++di)
        {
          auto &dVar = sVar.passes[di];
          if (dVar.rpass->fshId != index)
          {
            continue;
          }

          visitForShaderClassCodes(cls, si, di, false, false, false, false, handler);
        }
      }
    }
  }

  void generateNamesForShaderClassCodes(const ScriptedShadersBinDumpV2 *v2, const shaderbindump::ShaderClass &shader_class,
    uint32_t static_index, uint32_t dynamic_index, auto &out_name,
    const shader_name_format::VariantNameCompilationSettings &name_settings)
  {
    auto &sVar = shader_class.code[static_index];
    shader_class.stVariants.enumerateCodesForVariant(static_index, [&](uint32_t s_code) {
      sVar.dynVariants.enumerateCodesForVariant(dynamic_index, [&](uint32_t d_code) {
        if (!out_name.empty())
        {
          if (name_settings.listAllAliases)
            out_name.append(" | ");
          else
            return;
        }
        shader_name_format::compile_human_readable_variant_name_single(out_name,
          shader_name_format::VariantIdentifierRef{
            .shClassName = shader_class.name.data(), .stVarCode = int(s_code), .dynVarCode = int(d_code)},
          *v2, *v2);
      });
    });
  }

  template <typename T>
  static eastl::optional<uint32_t> firstCodeForVariant(const T &variant_table, uint32_t variant)
  {
    eastl::optional<uint32_t> result;
    variant_table.enumerateCodesForVariant(variant, [&result](uint32_t code) {
      if (!result)
        result = code;
    });
    return result;
  }

  template <typename U>
  void visitForShaderClassCodes(const shaderbindump::ShaderClass &shader_class, uint32_t static_index, uint32_t dynamic_index,
    bool use_static_state_override, bool use_null_pixel_shader, bool use_with_pixel_shader_override, bool is_pixel_shader_override,
    U handler)
  {
    auto &sVar = shader_class.code[static_index];

    auto sCode = firstCodeForVariant(shader_class.stVariants, static_index);
    if (!sCode)
      return;
    auto dCode = firstCodeForVariant(sVar.dynVariants, dynamic_index);
    if (!dCode)
      return;

    handler(shader_class, *sCode, *dCode, use_static_state_override, use_null_pixel_shader, use_with_pixel_shader_override,
      is_pixel_shader_override);
  }


  template <typename T>
  uint32_t generateNamesForMatchingRenderPasses(uint32_t group, eastl::string_view post_fix, eastl::string_view post_fix_2,
    eastl::string &out_name, T clb)
  {
    uint32_t visitCount = 0;

    auto v2 = getDump(group)->getDumpV2();
    if (!v2)
      return 0;

    const auto &nameSettings = getDump(group)->shaderNameFormatSettings;

    for (auto &cls : v2->classes)
    {
      // shrefStorage stores all the unique combinations of shaders used by this class
      auto ref = eastl::find_if(eastl::begin(cls.shrefStorage), eastl::end(cls.shrefStorage), clb);
      if (ref == eastl::end(cls.shrefStorage))
      {
        continue;
      }

      for (uint32_t si = 0; si < cls.code.size(); ++si)
      {
        auto &sVar = cls.code[si];
        for (uint32_t di = 0; di < sVar.passes.size(); ++di)
        {
          auto &dVar = sVar.passes[di];
          if (clb(*dVar.rpass))
          {
            generateNamesForShaderClassCodes(v2, cls, si, di, out_name, nameSettings);
            ++visitCount;
          }
        }
      }
    }

    if (visitCount > 0)
    {
      for (auto affix : {post_fix, post_fix_2})
        out_name.append(begin(affix), end(affix));
    }

    return visitCount;
  }

  void iterateShaderClassesForGraphicsShadersWithNullPixelShader(const dxil::HashValue &vertex_shader,
    const RenderStateSystem::StaticState &static_state, eastl::string &out_name)
  {
    uint32_t totalCount = 0;
    uint32_t lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto [rGroup, rState, rDist] = walkRenderStates(vGroup, 0, 0, static_state);
      if (rGroup != vGroup)
      {
        lastGroup = rGroup;
        continue;
      }

      auto count = generateNamesForMatchingRenderPasses(vGroup, {}, {}, out_name, [shader = vShader, state = rState](auto &pass) {
        return shader == pass.vprId && 0xFFFF == pass.fshId && state == pass.renderStateNo;
      });
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(vGroup, rState + 1, rDist, static_state);
        if (crGroup != vGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = generateNamesForMatchingRenderPasses(vGroup, "[Render state override]", {}, out_name,
          [shader = vShader, state = crState](auto &pass) {
            return shader == pass.vprId && 0xFFFF == pass.fshId && state == pass.renderStateNo;
          });
      }

      if (0 == count)
      {
        count = generateNamesForMatchingRenderPasses(vGroup, "[Render state override]", {}, out_name,
          [shader = vShader](auto &pass) { return shader == pass.vprId && 0xFFFF == pass.fshId; });
      }
      totalCount += count;
      lastGroup = vGroup + 1;
    }

    if (0 != totalCount)
    {
      // found at least one instance, we are done here
      return;
    }

    // we where unable to find anything, so this uses some shader class combined with the null pixel shader as an "override"
    lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto [rGroup, rState, rDist] = walkRenderStates(vGroup, 0, 0, static_state);
      if (rGroup != vGroup)
      {
        lastGroup = rGroup;
        continue;
      }

      auto count = generateNamesForMatchingRenderPasses(vGroup, {}, "[null pixel shader override]", out_name,
        [shader = vShader, state = rState](auto &pass) { return shader == pass.vprId && state == pass.renderStateNo; });
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(vGroup, rState + 1, rDist, static_state);
        if (crGroup != vGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = generateNamesForMatchingRenderPasses(vGroup, "[Render state override]", "[null pixel shader override]", out_name,
          [shader = vShader, state = crState](auto &pass) { return shader == pass.vprId && state == pass.renderStateNo; });
      }

      if (0 == count)
      {
        count = generateNamesForMatchingRenderPasses(vGroup, "[Render state override]", "[null pixel shader override]", out_name,
          [shader = vShader](auto &pass) { return shader == pass.vprId; });
      }
      totalCount += count;
      lastGroup = vGroup + 1;
    }
  }

  void iterateShaderClassesForGraphicsShadersWithNullPixelShaderWithAnyRenderState(const dxil::HashValue &vertex_shader,
    eastl::string &out_name)
  {
    uint32_t totalCount = 0;
    uint32_t lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto count = generateNamesForMatchingRenderPasses(vGroup, {}, {}, out_name,
        [shader = vShader](auto &pass) { return shader == pass.vprId && 0xFFFF == pass.fshId; });

      totalCount += count;
      lastGroup = vGroup + 1;
    }

    if (0 != totalCount)
    {
      // found at least one instance, we are done here
      return;
    }

    // we where unable to find anything, so this uses some shader class combined with the null pixel shader as an "override"
    lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto count = generateNamesForMatchingRenderPasses(vGroup, {}, "[null pixel shader override]", out_name,
        [shader = vShader](auto &pass) { return shader == pass.vprId; });

      totalCount += count;
      lastGroup = vGroup + 1;
    }
  }

  void iterateShaderClassesForGraphicsShaders(const dxil::HashValue &vertex_shader, const dxil::HashValue &pixel_shader,
    const RenderStateSystem::StaticState &static_state, eastl::string &out_name)
  {
    uint32_t totalCount = 0;
    uint32_t lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto [pGroup, pShader] = findPixelShader(vGroup, pixel_shader);
      if (pGroup != vGroup)
      {
        lastGroup = pGroup;
        continue;
      }

      auto [rGroup, rState, rDist] = walkRenderStates(vGroup, 0, 0, static_state);
      if (rGroup != vGroup)
      {
        lastGroup = rGroup;
        continue;
      }

      auto count =
        generateNamesForMatchingRenderPasses(vGroup, {}, {}, out_name, [vert = vShader, pix = pShader, state = rState](auto &pass) {
          return vert == pass.vprId && pix == pass.fshId && state == pass.renderStateNo;
        });
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(vGroup, rState + 1, rDist, static_state);
        if (crGroup != vGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = generateNamesForMatchingRenderPasses(vGroup, "[Render state override]", {}, out_name,
          [vert = vShader, pix = pShader, state = crState](auto &pass) {
            return vert == pass.vprId && pix == pass.fshId && state == pass.renderStateNo;
          });
      }

      if (0 == count)
      {
        count = generateNamesForMatchingRenderPasses(vGroup, "[Render state override]", {}, out_name,
          [vert = vShader, pix = pShader](auto &pass) { return vert == pass.vprId && pix == pass.fshId; });
      }
      totalCount += count;
      lastGroup = vGroup + 1;
    }

    if (0 != totalCount)
    {
      return;
    }

    // we where unable to find anything, so this uses some shader class combined with the null pixel shader as an "override"
    lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto [rGroup, rState, rDist] = walkRenderStates(vGroup, 0, 0, static_state);
      if (rGroup != vGroup)
      {
        lastGroup = rGroup;
        continue;
      }

      auto count = generateNamesForMatchingRenderPasses(vGroup, {}, "[with pixel shader override]", out_name,
        [shader = vShader, state = rState](auto &pass) { return shader == pass.vprId && state == pass.renderStateNo; });
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(vGroup, rState + 1, rDist, static_state);
        if (crGroup != vGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = generateNamesForMatchingRenderPasses(vGroup, "[Render state override]", "[with pixel shader override]", out_name,
          [shader = vShader, state = crState](auto &pass) { return shader == pass.vprId && state == pass.renderStateNo; });
      }

      if (0 == count)
      {
        count = generateNamesForMatchingRenderPasses(vGroup, "[Render state override]", "[with pixel shader override]", out_name,
          [shader = vShader](auto &pass) { return shader == pass.vprId; });
      }
      totalCount += count;
      lastGroup = vGroup + 1;
    }

    if (0 == totalCount)
    {
      // if we end up here, something in the shader dump is broken, as a vertex shader is in the dump, but is not used by
      // and shader class / pass.
      return;
    }

    totalCount = 0;

    // now try to find paired pixel shader
    lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [pGroup, pShader] = findPixelShader(lastGroup, pixel_shader);
      if (pGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto [rGroup, rState, rDist] = walkRenderStates(pGroup, 0, 0, static_state);
      if (rGroup != pGroup)
      {
        lastGroup = rGroup;
        continue;
      }

      auto count = generateNamesForMatchingRenderPasses(pGroup, {}, {}, out_name,
        [shader = pShader, state = rState](auto &pass) { return shader == pass.fshId && state == pass.renderStateNo; });
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(pGroup, rState + 1, rDist, static_state);
        if (crGroup != pGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = generateNamesForMatchingRenderPasses(pGroup, "[Render state override]", {}, out_name,
          [shader = pShader, state = crState](auto &pass) { return shader == pass.fshId && state == pass.renderStateNo; });
      }

      if (0 == count)
      {
        count = generateNamesForMatchingRenderPasses(pGroup, "[Render state override]", {}, out_name,
          [shader = pShader](auto &pass) { return shader == pass.fshId; });
      }
      totalCount += count;
      lastGroup = pGroup + 1;
    }

    if (0 == totalCount)
    {
      out_name += "[pixel shader not in dump]";
      char buf[sizeof(dxil::HashValue) * 2 + 1];
      pixel_shader.convertToString(buf, sizeof(buf));
      out_name += "{";
      out_name += buf;
      out_name += "}";
      vertex_shader.convertToString(buf, sizeof(buf));
      out_name += "{";
      out_name += buf;
      out_name += "}";
    }
  }

  void iterateShaderClassesForGraphicsShadersWithAnyRenderState(const dxil::HashValue &vertex_shader,
    const dxil::HashValue &pixel_shader, eastl::string &out_name)
  {
    uint32_t totalCount = 0;
    uint32_t lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto [pGroup, pShader] = findPixelShader(vGroup, pixel_shader);
      if (pGroup != vGroup)
      {
        lastGroup = pGroup;
        continue;
      }

      auto count = generateNamesForMatchingRenderPasses(vGroup, {}, {}, out_name,
        [vert = vShader, pix = pShader](auto &pass) { return vert == pass.vprId && pix == pass.fshId; });

      totalCount += count;
      lastGroup = vGroup + 1;
    }

    if (0 != totalCount)
    {
      return;
    }

    // we where unable to find anything, so this uses some shader class combined with the null pixel shader as an "override"
    lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto count = generateNamesForMatchingRenderPasses(vGroup, {}, "[with pixel shader override]", out_name,
        [shader = vShader](auto &pass) { return shader == pass.vprId; });

      totalCount += count;
      lastGroup = vGroup + 1;
    }

    if (0 == totalCount)
    {
      // if we end up here, something in the shader dump is broken, as a vertex shader is in the dump, but is not used by
      // and shader class / pass.
      return;
    }

    totalCount = 0;

    // now try to find paired pixel shader
    lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [pGroup, pShader] = findPixelShader(lastGroup, pixel_shader);
      if (pGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }

      auto count = generateNamesForMatchingRenderPasses(pGroup, {}, {}, out_name,
        [shader = pShader](auto &pass) { return shader == pass.fshId; });

      totalCount += count;
      lastGroup = pGroup + 1;
    }

    if (0 == totalCount)
    {
      out_name += "[pixel shader not in dump]";
      char buf[sizeof(dxil::HashValue) * 2 + 1];
      pixel_shader.convertToString(buf, sizeof(buf));
      out_name += "{";
      out_name += buf;
      out_name += "}";
      vertex_shader.convertToString(buf, sizeof(buf));
      out_name += "{";
      out_name += buf;
      out_name += "}";
    }
  }

  // A module taken from a dump carries the group and the index it was taken from, which is what
  // the pass index is keyed by. Byte code that is in no dump has no such answer.
  ShaderInGroupIndex vertexShaderIndexInDump(const VertexShaderModuleRefStore &vertex_shader)
  {
    auto ref = eastl::get_if<VertexShaderModuleBytcodeInDumpRef>(&vertex_shader.bytecode);
    if (!ref || !ref->bytecode || ref->shaderGroup >= max_scripted_shaders_bin_groups)
    {
      return {max_scripted_shaders_bin_groups, 0};
    }
    return {ref->shaderGroup, canonicalVertexShaderIndex(ref->shaderGroup, ref->bytecode->compressionIndex)};
  }

  ShaderInGroupIndex pixelShaderIndexInDump(const PixelShaderModuleRefStore &pixel_shader)
  {
    auto ref = eastl::get_if<PixelShaderModuleBytcodeInDumpRef>(&pixel_shader.bytecode);
    if (!ref || !ref->bytecode || ref->shaderGroup >= max_scripted_shaders_bin_groups)
    {
      return {max_scripted_shaders_bin_groups, 0};
    }
    auto dump = getDump(ref->shaderGroup);
    auto v2 = dump ? dump->getDumpV2() : nullptr;
    // the stored index addresses the whole shader table, a pass counts pixel shaders on their own
    if (!v2 || ref->bytecode->compressionIndex < uint32_t(v2->vprCount))
    {
      return {max_scripted_shaders_bin_groups, 0};
    }
    const uint32_t fshId = ref->bytecode->compressionIndex - uint32_t(v2->vprCount);
    return {ref->shaderGroup, canonicalPixelShaderIndex(ref->shaderGroup, fshId)};
  }

  bool isNullPixelShader(const PixelShaderModuleRefStore &pixel_shader) const
  {
    // TODO: this assumption is not correct
    return eastl::holds_alternative<PixelShaderModuleBytecodeRef>(pixel_shader.bytecode);
  }

public:
  eastl::string generateComputePipelineName(const ComputeShaderModule &module)
  {
    eastl::string outName;
    iterateShaderClassesForComputeShader(module.ident.shaderHash, outName);

    if (outName.empty() && !module.debugName.empty())
    {
      outName = module.debugName;
    }

    if (outName.empty())
    {
      // so engine gave it no name and we were unable to locate it in the bin dumps, we give it a hash name then
      outName.resize(sizeof(dxil::HashValue) * 2, ' ');
      module.ident.shaderHash.convertToString(outName.data(), outName.size() + 1);
    }

    return outName;
  }

  template <typename C>
  void visitShaderClassPassesForComputePipeline(const ComputeShaderModule &header, C target)
  {
    visitShaderClassesForComputeShader(header.ident.shaderHash, target);
  }

  template <typename C>
  uint32_t visitIndexedPassRefs(uint32_t group, const GraphicsPassIndex &index, const GraphicsPassIndex::PassRefs *refs,
    const RenderStateSystem::StaticState &static_state, bool use_null_pixel_shader, bool use_with_pixel_shader_override,
    bool is_pixel_shader_override, C target)
  {
    if (!refs)
    {
      return 0;
    }

    uint32_t bestState = ~0u;
    uint32_t bestDistance = ~0u;
    for (const auto &ref : *refs)
    {
      if (ref.renderState >= index.staticStates.size())
      {
        continue;
      }
      const uint32_t distance = index.staticStates[ref.renderState].distance(static_state);
      if (distance < bestDistance || (distance == bestDistance && ref.renderState < bestState))
      {
        bestDistance = distance;
        bestState = ref.renderState;
      }
    }

    auto v2 = getDump(group)->getDumpV2();
    uint32_t count = 0;
    if (~0u == bestState)
    {
      for (const auto &ref : *refs)
      {
        visitForShaderClassCodes(v2->classes[ref.classIndex], ref.staticIndex, ref.dynamicIndex, true, use_null_pixel_shader,
          use_with_pixel_shader_override, is_pixel_shader_override, target);
        ++count;
      }
      return count;
    }

    bool stateOverride = true;
    if (0 == bestDistance)
    {
      stateOverride = false;
      for (uint32_t stateIndex = 0; stateIndex < bestState && !stateOverride; ++stateIndex)
      {
        stateOverride = 0 == index.staticStates[stateIndex].distance(static_state);
      }
    }

    for (const auto &ref : *refs)
    {
      if (ref.renderState != bestState)
      {
        continue;
      }
      visitForShaderClassCodes(v2->classes[ref.classIndex], ref.staticIndex, ref.dynamicIndex, stateOverride, use_null_pixel_shader,
        use_with_pixel_shader_override, is_pixel_shader_override, target);
      ++count;
    }
    return count;
  }

  template <typename C>
  void visitShaderClassPassesForGraphicsPipelineIndexed(GraphicsPassIndexCache &indices,
    const VertexShaderModuleRefStore &vertex_shader, const PixelShaderModuleRefStore &pixel_shader,
    const RenderStateSystem::StaticState &static_state, C target)
  {
    const bool nullPixelShader = isNullPixelShader(pixel_shader);

    // phase 1: exact vertex and pixel shader pair (the null pixel shader is id 0xFFFF)
    uint32_t totalCount = 0;
    uint32_t lastGroup = 0;

    const ShaderInGroupIndex vsInDump = vertexShaderIndexInDump(vertex_shader);
    bool exactPairKnown = vsInDump.group < max_scripted_shaders_bin_groups;
    uint32_t exactPixelShader = 0xFFFF;
    if (exactPairKnown && !nullPixelShader)
    {
      const ShaderInGroupIndex psInDump = pixelShaderIndexInDump(pixel_shader);
      // a pass pairs shaders within one dump, so a pair split over two of them is not one
      exactPairKnown = psInDump.group == vsInDump.group;
      exactPixelShader = psInDump.shader;
    }

    if (exactPairKnown)
    {
      auto &index = getGraphicsPassIndex(indices, vsInDump.group);
      if (index.valid)
      {
        totalCount = visitIndexedPassRefs(vsInDump.group, index, index.passesOfVsPs(vsInDump.shader, exactPixelShader), static_state,
          false, false, false, target);
      }
    }
    while (!exactPairKnown && lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader.header.hash);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }
      lastGroup = vGroup + 1;

      uint32_t pShader = 0xFFFF;
      if (!nullPixelShader)
      {
        auto [pGroup, pIndex] = findPixelShader(vGroup, pixel_shader.header.hash);
        if (pGroup != vGroup)
        {
          lastGroup = pGroup;
          continue;
        }
        pShader = pIndex;
      }
      auto &index = getGraphicsPassIndex(indices, vGroup);
      if (!index.valid)
      {
        continue;
      }

      const uint32_t count =
        visitIndexedPassRefs(vGroup, index, index.passesOfVsPs(vShader, pShader), static_state, false, false, false, target);
      totalCount += count;
    }

    if (0 != totalCount)
    {
      return;
    }

    // phase 2: the pipeline uses some shader class with a pixel shader (or null pixel shader)
    // override, match by vertex shader only
    lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [vGroup, vShader] = findVertexShader(lastGroup, vertex_shader.header.hash);
      if (vGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }
      lastGroup = vGroup + 1;
      auto &index = getGraphicsPassIndex(indices, vGroup);
      if (!index.valid)
      {
        continue;
      }
      totalCount +=
        visitIndexedPassRefs(vGroup, index, index.passesOfVs(vShader), static_state, nullPixelShader, !nullPixelShader, false, target);
    }

    if (nullPixelShader || 0 == totalCount)
    {
      return;
    }

    // phase 3: record the classes that use this pixel shader with other vertex shaders
    // as pixel shader override usages
    lastGroup = 0;
    while (lastGroup < max_scripted_shaders_bin_groups)
    {
      auto [pGroup, pShader] = findPixelShader(lastGroup, pixel_shader.header.hash);
      if (pGroup >= max_scripted_shaders_bin_groups)
      {
        break;
      }
      lastGroup = pGroup + 1;
      auto &index = getGraphicsPassIndex(indices, pGroup);
      if (!index.valid)
      {
        continue;
      }
      visitIndexedPassRefs(pGroup, index, index.passesOfPs(pShader), static_state, false, true, true, target);
    }
  }

  eastl::string generateGraphicsPipelineName(const VertexShaderModuleRefStore &vertex_shader,
    const PixelShaderModuleRefStore &pixel_shader, const RenderStateSystem::StaticState &static_state)
  {
    eastl::string outName;
    if (isNullPixelShader(pixel_shader))
    {
      iterateShaderClassesForGraphicsShadersWithNullPixelShader(vertex_shader.header.hash, static_state, outName);
    }
    else
    {
      iterateShaderClassesForGraphicsShaders(vertex_shader.header.hash, pixel_shader.header.hash, static_state, outName);
    }

    if (outName.empty())
    {
      outName = generateGraphicsPipelineNameFromModules(vertex_shader, pixel_shader);
      if (!outName.empty())
        outName += static_state.toString();
    }

    if (outName.empty())
    {
      outName = "NotInDump={";
      char buf[sizeof(dxil::HashValue) * 2 + 1];
      vertex_shader.header.hash.convertToString(buf, sizeof(buf));
      outName += buf;
      bool isRaw = eastl::holds_alternative<VertexShaderModuleBytecodeRef>(vertex_shader.bytecode);
      outName += isRaw ? "[raw]" : "[dump]";
      outName += "}{";
      pixel_shader.header.hash.convertToString(buf, sizeof(buf));
      outName += buf;
      isRaw = eastl::holds_alternative<PixelShaderModuleBytecodeRef>(pixel_shader.bytecode);
      outName += isRaw ? "[raw]" : "[dump]";
      outName += "}";
      auto stateText = static_state.toString();
      outName += stateText;
    }

    return outName;
  }

  eastl::string generateGraphicsPipelineNameWithAnyRenderState(const VertexShaderModuleRefStore &vertex_shader,
    const PixelShaderModuleRefStore &pixel_shader)
  {
    eastl::string outName;
    if (isNullPixelShader(pixel_shader))
    {
      iterateShaderClassesForGraphicsShadersWithNullPixelShaderWithAnyRenderState(vertex_shader.header.hash, outName);
    }
    else
    {
      iterateShaderClassesForGraphicsShadersWithAnyRenderState(vertex_shader.header.hash, pixel_shader.header.hash, outName);
    }

    if (outName.empty())
      outName = generateGraphicsPipelineNameFromModules(vertex_shader, pixel_shader);

    if (outName.empty())
    {
      outName = "NotInDump={";
      char buf[sizeof(dxil::HashValue) * 2 + 1];
      vertex_shader.header.hash.convertToString(buf, sizeof(buf));
      outName += buf;
      bool isRaw = eastl::holds_alternative<VertexShaderModuleBytecodeRef>(vertex_shader.bytecode);
      outName += isRaw ? "[raw]" : "[dump]";
      outName += "}{";
      pixel_shader.header.hash.convertToString(buf, sizeof(buf));
      outName += buf;
      isRaw = eastl::holds_alternative<PixelShaderModuleBytecodeRef>(pixel_shader.bytecode);
      outName += isRaw ? "[raw]" : "[dump]";
      outName += "}";
    }

    return outName;
  }

  eastl::string generateGraphicsPipelineNameFromModules(const VertexShaderModuleRefStore &vertex_shader,
    const PixelShaderModuleRefStore &pixel_shader)
  {
    eastl::string outName;
    if (!vertex_shader.header.debugName.empty() && !pixel_shader.header.debugName.empty())
      outName = vertex_shader.header.debugName + " & " + pixel_shader.header.debugName;
    else if (!vertex_shader.header.debugName.empty())
      outName = vertex_shader.header.debugName + " &";
    else if (!pixel_shader.header.debugName.empty())
      outName = "& " + pixel_shader.header.debugName;
    return outName;
  }
};

class ShaderModuleManager : public PipelineNameGenerator
{
  using BaseType = PipelineNameGenerator;

  struct GroupZeroVertexShaderModule
  {
    VertexShaderModuleHeader header;
    VertexShaderModuleBytecode bytecode;
  };
  struct GroupVertexShaderModule
  {
    VertexShaderModuleHeader header;
    VertexShaderModuleBytcodeInDump bytecode;
  };
  struct GroupZeroPixelShaderModule
  {
    PixelShaderModuleHeader header;
    PixelShaderModuleBytecode bytecode;
  };
  struct GroupPixelShaderModule
  {
    PixelShaderModuleHeader header;
    PixelShaderModuleBytcodeInDump bytecode;
  };
  struct
  {
    dag::Vector<eastl::unique_ptr<GroupZeroVertexShaderModule>> vertex;
    dag::Vector<eastl::unique_ptr<GroupZeroPixelShaderModule>> pixel;
  } shaderGroupZero;
  struct
  {
    dag::Vector<eastl::unique_ptr<GroupVertexShaderModule>> vertex;
    dag::Vector<eastl::unique_ptr<GroupPixelShaderModule>> pixel;
  } shaderGroup[max_scripted_shaders_bin_groups - 1];

  GetShaderByteCodeResult getBytecode(const PixelShaderModuleBytecodeRef &ref, const uint8_t *bytecode)
  {
    bytecode += ref.bytecode->bytecodeOffset;
    return {.byteCode = {bytecode, ref.bytecode->bytecodeSize}, .cacheHit = true};
  }

  GetShaderByteCodeResult getBytecode(const PixelShaderModuleBytcodeInDumpRef &ref, const uint8_t *)
  {
    auto [meta, byteCode, cacheHit] = getShaderByteCode(ref.shaderGroup, ref.bytecode->compressionIndex);
    return {.byteCode = byteCode.subspan(ref.bytecode->shaderOffset, ref.bytecode->shaderSize), .cacheHit = cacheHit};
  }

  GetShaderByteCodeResult getVsBytecode(const VertexShaderModuleHeader &, const VertexShaderModuleBytecodeRef &ref,
    const uint8_t *bytecode)
  {
    bytecode += ref.bytecode->bytecodeOffset;
    return {.byteCode = {bytecode, ref.bytecode->bytecodeSize}, .cacheHit = true};
  }
  GetShaderByteCodeResult getVsBytecode(const VertexShaderModuleHeader &, const VertexShaderModuleBytcodeInDumpRef &ref,
    const uint8_t *)
  {
    auto [meta, byteCode, cacheHit] = getShaderByteCode(ref.shaderGroup, ref.bytecode->compressionIndex);
    return {.byteCode = byteCode.subspan(ref.bytecode->shaderOffset, ref.bytecode->shaderSize), .cacheHit = cacheHit};
  }
  auto getMsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytecodeRef &ref, const uint8_t *bytecode)
  {
    return getVsBytecode(header, ref, bytecode);
  }
  auto getMsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytcodeInDumpRef &ref, const uint8_t *bytecode)
  {
    return getVsBytecode(header, ref, bytecode);
  }
  auto getGsBytecode(const VertexShaderModuleHeader &, const VertexShaderModuleBytecodeRef &ref, const uint8_t *bytecode)
  {
    return getSubShaderBytecode(0, ref, bytecode);
  }
  auto getGsBytecode(const VertexShaderModuleHeader &, const VertexShaderModuleBytcodeInDumpRef &ref, const uint8_t *)
  {
    return getSubShaderBytecode(0, ref);
  }
  auto getAsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytecodeRef &ref, const uint8_t *bytecode)
  {
    return getGsBytecode(header, ref, bytecode);
  }
  auto getAsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytcodeInDumpRef &ref, const uint8_t *bytecode)
  {
    return getGsBytecode(header, ref, bytecode);
  }
  auto getHsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytecodeRef &ref, const uint8_t *bytecode)
  {
    return getSubShaderBytecode(header.hasGsOrAs ? 1 : 0, ref, bytecode);
  }
  auto getHsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytcodeInDumpRef &ref, const uint8_t *)
  {
    return getSubShaderBytecode(header.hasGsOrAs ? 1 : 0, ref);
  }
  auto getDsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytecodeRef &ref, const uint8_t *bytecode)
  {
    return getSubShaderBytecode(header.hasGsOrAs ? 2 : 1, ref, bytecode);
  }
  auto getDsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytcodeInDumpRef &ref, const uint8_t *)
  {
    return getSubShaderBytecode(header.hasGsOrAs ? 2 : 1, ref);
  }
  GetShaderByteCodeResult getSubShaderBytecode(uint32_t sub_shader_index, const VertexShaderModuleBytecodeRef &ref,
    const uint8_t *bytecode)
  {
    auto &subShader = ref.bytecode->subShaders[sub_shader_index];
    bytecode += subShader.bytecodeOffset;
    return {.byteCode = {bytecode, subShader.bytecodeSize}, .cacheHit = true};
  }
  GetShaderByteCodeResult getSubShaderBytecode(uint32_t sub_shader_index, const VertexShaderModuleBytcodeInDumpRef &ref)
  {
    auto &subShader = ref.bytecode->subShaders[sub_shader_index];
    auto [meta, byteCode, cacheHit] = getShaderByteCode(ref.shaderGroup, ref.bytecode->compressionIndex);
    return {.byteCode = byteCode.subspan(subShader.shaderOffset, subShader.shaderSize), .cacheHit = cacheHit};
  }

public:
  void addVertexShader(ShaderID id, VertexShaderModule *module);
  void addPixelShader(ShaderID id, PixelShaderModule *module);
  const dxil::HashValue &getVertexShaderHash(ShaderID id) const;
  const dxil::HashValue &getPixelShaderHash(ShaderID id) const;
  VertexShaderModuleRefStore getVertexShader(ShaderID id);
  PixelShaderModuleRefStore getPixelShader(ShaderID id);

  auto getBytecode(const PixelShaderModuleRefStore &ref, const uint8_t *bytecode)
  {
    return eastl::visit([this, bytecode](auto &r) { return this->getBytecode(r, bytecode); }, ref.bytecode);
  }

  auto getVsBytecode(const VertexShaderModuleRefStore &ref, const uint8_t *bytecode)
  {
    return eastl::visit([this, &ref, bytecode](auto &r) { return this->getVsBytecode(ref.header, r, bytecode); }, ref.bytecode);
  }

  auto getMsBytecode(const VertexShaderModuleRefStore &ref, const uint8_t *bytecode)
  {
    return eastl::visit([this, &ref, bytecode](auto &r) { return this->getMsBytecode(ref.header, r, bytecode); }, ref.bytecode);
  }

  auto getGsBytecode(const VertexShaderModuleRefStore &ref, const uint8_t *bytecode)
  {
    return eastl::visit([this, &ref, bytecode](auto &r) { return this->getGsBytecode(ref.header, r, bytecode); }, ref.bytecode);
  }

  auto getAsBytecode(const VertexShaderModuleRefStore &ref, const uint8_t *bytecode)
  {
    return eastl::visit([this, &ref, bytecode](auto &r) { return this->getAsBytecode(ref.header, r, bytecode); }, ref.bytecode);
  }

  auto getHsBytecode(const VertexShaderModuleRefStore &ref, const uint8_t *bytecode)
  {
    return eastl::visit([this, &ref, bytecode](auto &r) { return this->getHsBytecode(ref.header, r, bytecode); }, ref.bytecode);
  }

  auto getDsBytecode(const VertexShaderModuleRefStore &ref, const uint8_t *bytecode)
  {
    return eastl::visit([this, &ref, bytecode](auto &r) { return this->getDsBytecode(ref.header, r, bytecode); }, ref.bytecode);
  }

  using AnyShaderModuleUniquePointer =
    eastl::variant<eastl::unique_ptr<GroupZeroVertexShaderModule>, eastl::unique_ptr<GroupVertexShaderModule>,
      eastl::unique_ptr<GroupZeroPixelShaderModule>, eastl::unique_ptr<GroupPixelShaderModule>>;

  AnyShaderModuleUniquePointer releaseVertexShader(ShaderID id);
  AnyShaderModuleUniquePointer releasePixelShader(ShaderID id);
  void resetDumpOfGroup(uint32_t group_index);
  void reserveVertexShaderRange(uint32_t group_index, uint32_t count);
  void setVertexShaderCompressionGroup(uint32_t group_index, uint32_t index, const dxil::HashValue &hash, uint32_t compression_index);
  void setPixelShaderCompressionGroup(uint32_t group_index, uint32_t index, const dxil::HashValue &hash, uint32_t compression_index);
  ShaderID findVertexShader(const dxil::HashValue &hash) const
  {
    for (uint32_t si = 0; si < shaderGroupZero.vertex.size(); ++si)
    {
      auto &vs = shaderGroupZero.vertex[si];
      if (!vs)
      {
        continue;
      }
      if (vs->header.hash == hash)
      {
        return ShaderID::make(0, si);
      }
    }
    for (uint32_t gi = 0; gi < countof(shaderGroup); ++gi)
    {
      auto &group = shaderGroup[gi].vertex;
      for (uint32_t si = 0; si < group.size(); ++si)
      {
        auto &vs = group[si];
        if (vs->header.hash == hash)
        {
          return ShaderID::make(gi + 1, si);
        }
      }
    }
    return ShaderID::Null();
  }
  ShaderID findPixelShader(const dxil::HashValue &hash) const
  {
    for (uint32_t si = 0; si < shaderGroupZero.pixel.size(); ++si)
    {
      auto &ps = shaderGroupZero.pixel[si];
      if (!ps)
      {
        continue;
      }
      if (ps->header.hash == hash)
      {
        return ShaderID::make(0, si);
      }
    }
    for (uint32_t gi = 0; gi < countof(shaderGroup); ++gi)
    {
      auto &group = shaderGroup[gi].pixel;
      for (uint32_t si = 0; si < group.size(); ++si)
      {
        auto &ps = group[si];
        if (ps->header.hash == hash)
        {
          return ShaderID::make(gi + 1, si);
        }
      }
    }
    return ShaderID::Null();
  }
};
} // namespace backend
} // namespace drv3d_dx12
