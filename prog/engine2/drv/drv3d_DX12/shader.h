#pragma once

#include <EASTL/array.h>
#include <EASTL/sort.h>
#include <EASTL/variant.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_rwLock.h>
#include <util/dag_hashedKeyMap.h>
#include <dxil/compiled_shader_header.h>
#include <dxil/utility.h>
#include <osApiWrappers/dag_atomic.h>
#include <shadersBinaryData.h>

static constexpr uint32_t MAX_VERTEX_ATTRIBUTES = 16;
static constexpr uint32_t MAX_VERTEX_INPUT_STREAMS = 4;
static constexpr uint32_t MAX_SEMANTIC_INDEX = VSDR_TEXC14 + 1;

namespace drv3d_dx12
{
class DeviceContext;
class Image;

class Device;
struct InputLayout
{
  struct VertexAttributesDesc
  {
    // We use 25 unique locations, each one bit
    uint32_t locationMask = 0;
    // for each location we use two corresponding bits to store index into streams
    uint64_t locationSourceStream = 0;
    // format uses top 5 bits and the remaining 11 bits store the offset as dword count (8k bytes of
    // offset max)
    uint16_t compactedFormatAndOffset[MAX_SEMANTIC_INDEX]{};

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
      locationMask |= 1u << index;
    }
    void setLocationStreamSource(uint32_t location, uint32_t stream)
    {
      G_ASSERT(stream < MAX_VERTEX_INPUT_STREAMS);
      G_STATIC_ASSERT(MAX_VERTEX_INPUT_STREAMS - 1 <= 3); // make sure we only need 2 bits for
                                                          // stream index
      locationSourceStream |= static_cast<uint64_t>(stream & 3) << (location * 2);
    }
    void setLocationStreamOffset(uint32_t location, uint32_t offset)
    {
      G_ASSERT(0 == (offset % 4));
      G_ASSERT((offset / 4) == ((offset / 4) & offset_mask));
      compactedFormatAndOffset[location] = (compactedFormatAndOffset[location] & format_index_mask) | ((offset / 4) & offset_mask);
    }
    void setLocationFormatIndex(uint32_t location, uint32_t index)
    {
      G_ASSERT(index == (index & validate_format_index_mask));
      compactedFormatAndOffset[location] =
        (compactedFormatAndOffset[location] & offset_mask) | ((index >> vsdt_format_shift) << offset_bits);
    }

    constexpr bool usesLocation(uint32_t index) const { return 0 != (1 & (locationMask >> index)); }
    constexpr uint32_t getLocationStreamSource(uint32_t location) const
    {
      return static_cast<uint32_t>(locationSourceStream >> (location * 2)) & 3;
    }
    constexpr uint32_t getLocationStreamOffset(uint32_t location) const
    {
      return static_cast<uint32_t>(compactedFormatAndOffset[location] & offset_mask) * 4;
    }
    constexpr uint32_t getLocationFormatIndex(uint32_t location) const
    {
      return (compactedFormatAndOffset[location] >> offset_bits) << vsdt_format_shift;
    }
    /*constexpr*/ DXGI_FORMAT getLocationFormat(uint32_t location) const
    {
      switch (((compactedFormatAndOffset[location] & format_index_mask) >> offset_bits) << vsdt_format_shift)
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
  };

  struct VertexStreamsDesc
  {
    uint8_t usageMask : MAX_VERTEX_INPUT_STREAMS;
    uint8_t stepRateMask : MAX_VERTEX_INPUT_STREAMS;
    constexpr VertexStreamsDesc() : usageMask{0}, stepRateMask{0} {}

    void useStream(uint32_t index)
    {
      G_ASSERT(index < MAX_VERTEX_INPUT_STREAMS);
      usageMask |= 1u << index;
    }
    void setStreamStepRate(uint32_t index, D3D12_INPUT_CLASSIFICATION rate)
    {
      G_ASSERT(static_cast<uint32_t>(rate) <= 1); // -V547
      stepRateMask |= static_cast<uint32_t>(rate) << index;
    }

    constexpr bool usesStream(uint32_t index) const { return 0 != (1 & (usageMask >> index)); }
    constexpr D3D12_INPUT_CLASSIFICATION getStreamStepRate(uint32_t index) const
    {
      return static_cast<D3D12_INPUT_CLASSIFICATION>(1 & (stepRateMask >> index));
    }
  };

  template <typename T>
  void generateInputElementDescriptions(T clb) const
  {
    auto attirbMask = vertexAttributeSet.locationMask;
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
      desc.Format = vertexAttributeSet.getLocationFormat(i);
      desc.InputSlot = vertexAttributeSet.getLocationStreamSource(i);
      desc.AlignedByteOffset = vertexAttributeSet.getLocationStreamOffset(i);
      desc.InputSlotClass = inputStreamSet.getStreamStepRate(desc.InputSlot);
      desc.InstanceDataStepRate = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA == desc.InputSlotClass ? 0 : 1;
      clb(desc);
    }
  }

  constexpr bool validateAttributeUse(uint32_t expected) const { return expected == (vertexAttributeSet.locationMask & expected); }

  constexpr bool matchesAttributeUse(uint32_t expected) const { return vertexAttributeSet.locationMask == expected; }

  // This creates a copy of this with all attributes removed that are not
  // part of the used mask, it also removes any input stream that might no
  // longer be needed.
  InputLayout getLayoutForAttributeUse(uint32_t used) const
  {
    InputLayout other;
    uint32_t aMask = vertexAttributeSet.locationMask & used;
    for (uint32_t i = 0; i < MAX_SEMANTIC_INDEX && aMask; ++i, aMask >>= 1)
    {
      if (0 == (1 & aMask))
        continue;
      other.vertexAttributeSet.useLocation(i);
      auto streamIndex = vertexAttributeSet.getLocationStreamSource(i);
      other.vertexAttributeSet.setLocationStreamSource(i, streamIndex);
      other.inputStreamSet.useStream(streamIndex);
      other.vertexAttributeSet.setLocationStreamOffset(i, vertexAttributeSet.getLocationStreamOffset(i));
      other.vertexAttributeSet.setLocationFormatIndex(i, vertexAttributeSet.getLocationFormatIndex(i));
    }
    uint32_t sMask = other.inputStreamSet.usageMask;
    for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS && sMask; ++i, sMask >>= 1)
    {
      if (0 == (1 & sMask))
        continue;

      other.inputStreamSet.setStreamStepRate(i, inputStreamSet.getStreamStepRate(i));
    }

    return other;
  }

  VertexAttributesDesc vertexAttributeSet;
  VertexStreamsDesc inputStreamSet;

  void fromVdecl(const VSDTYPE *decl);
};

inline bool operator==(const InputLayout::VertexAttributesDesc &l, const InputLayout::VertexAttributesDesc &r)
{
  return l.locationMask == r.locationMask && l.locationSourceStream == r.locationSourceStream &&
         eastl::equal(eastl::begin(l.compactedFormatAndOffset), eastl::end(l.compactedFormatAndOffset),
           eastl::begin(r.compactedFormatAndOffset));
}

inline bool operator!=(const InputLayout::VertexAttributesDesc &l, const InputLayout::VertexAttributesDesc &r) { return !(l == r); }

inline bool operator==(const InputLayout::VertexStreamsDesc &l, const InputLayout::VertexStreamsDesc &r)
{
  return l.usageMask == r.usageMask && l.stepRateMask == r.stepRateMask;
}

inline bool operator!=(const InputLayout::VertexStreamsDesc &l, const InputLayout::VertexStreamsDesc &r) { return !(l == r); }

inline bool operator==(const InputLayout &l, const InputLayout &r)
{
  return l.inputStreamSet == r.inputStreamSet && l.vertexAttributeSet == r.vertexAttributeSet;
}

inline bool operator!=(const InputLayout &l, const InputLayout &r) { return !(l == r); }

struct ShaderIdentifier
{
  dxil::HashValue shaderHash;
  uint32_t shaderSize;
};

struct StageShaderModule
{
  ShaderIdentifier ident = {};
  dxil::ShaderHeader header = {};
  eastl::unique_ptr<uint8_t[]> byteCode;
  size_t byteCodeSize = 0;
  eastl::string debugName;

  explicit operator bool() const { return 0 != byteCodeSize; }
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

struct ShaderStageResouceUsageMask
{
  eastl::bitset<dxil::MAX_B_REGISTERS> bRegisterMask;
  eastl::bitset<dxil::MAX_T_REGISTERS> tRegisterMask;
  eastl::bitset<dxil::MAX_S_REGISTERS> sRegisterMask;
  eastl::bitset<dxil::MAX_U_REGISTERS> uRegisterMask;

  ShaderStageResouceUsageMask() = default;
  ShaderStageResouceUsageMask(const ShaderStageResouceUsageMask &) = default;
  // broadcast a single value to all masks
  explicit ShaderStageResouceUsageMask(uint32_t v) : bRegisterMask{v}, tRegisterMask{v}, sRegisterMask{v}, uRegisterMask{v} {}
  explicit ShaderStageResouceUsageMask(const dxil::ShaderHeader &header) :
    bRegisterMask{header.resourceUsageTable.bRegisterUseMask},
    tRegisterMask{header.resourceUsageTable.tRegisterUseMask},
    sRegisterMask{header.resourceUsageTable.sRegisterUseMask},
    uRegisterMask{header.resourceUsageTable.uRegisterUseMask}
  {}

  ShaderStageResouceUsageMask &operator|=(const ShaderStageResouceUsageMask &other)
  {
    bRegisterMask |= other.bRegisterMask;
    tRegisterMask |= other.tRegisterMask;
    sRegisterMask |= other.sRegisterMask;
    uRegisterMask |= other.uRegisterMask;
    return *this;
  }

  ShaderStageResouceUsageMask operator&(const ShaderStageResouceUsageMask &other) const
  {
    ShaderStageResouceUsageMask cpy{*this};
    cpy.bRegisterMask &= other.bRegisterMask;
    cpy.tRegisterMask &= other.tRegisterMask;
    cpy.sRegisterMask &= other.sRegisterMask;
    cpy.uRegisterMask &= other.uRegisterMask;
    return cpy;
  }

  ShaderStageResouceUsageMask &operator^=(const ShaderStageResouceUsageMask &other)
  {
    bRegisterMask ^= other.bRegisterMask;
    tRegisterMask ^= other.tRegisterMask;
    sRegisterMask ^= other.sRegisterMask;
    uRegisterMask ^= other.uRegisterMask;
    return *this;
  }
};

struct GraphicsProgramIOMask
{
  uint32_t usedAttributeLocations = 0;
  uint32_t usedOutputs = 0;
};

struct GraphicsProgramUsageInfo
{
  InputLayoutID input = InputLayoutID::Null();
  InternalInputLayoutID internalInputLayoutID = InternalInputLayoutID::Null();
  GraphicsProgramID programId = GraphicsProgramID::Null();
  ShaderStageResouceUsageMask vertexShaderStageResourceUses{~0};
  ShaderStageResouceUsageMask pixelShaderStageResourceUses{~0};
  GraphicsProgramIOMask ioMask{};
};

class IdManager
{
  uint32_t lastAllocatedId = 0;
  eastl::vector<uint32_t> freeIds;

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

      // try to tidy up the free id list
      auto at = eastl::find(begin(freeIds), end(freeIds), lastAllocatedId);
      while (at != end(freeIds))
      {
        freeIds.erase(at);
        at = eastl::find(begin(freeIds), end(freeIds), --lastAllocatedId);
      }
    }
    else
    {
      freeIds.push_back(id);
    }
  }
  void clear()
  {
    lastAllocatedId = 0;
    freeIds.clear();
  }

  template <typename T>
  void iterateAllocated(T t)
  {
    // to be reasonable fast, we iterate between free slots until we reach last allocated id
    eastl::sort(begin(freeIds), end(freeIds));
    uint32_t at = 1;
    for (auto se : freeIds)
    {
      for (; at < se; ++at)
        t(at);

      at = se + 1;
    }
    for (; at <= lastAllocatedId; ++at)
      t(at);
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

struct ShaderStageResouceUsageMaskAndIOMask
{
  ShaderStageResouceUsageMask resourceUsage;
  uint32_t ioMask = 0;

  ShaderStageResouceUsageMaskAndIOMask() = default;
  ShaderStageResouceUsageMaskAndIOMask(ShaderStageResouceUsageMask rm, uint32_t iom) : resourceUsage{rm}, ioMask{iom} {}
  ShaderStageResouceUsageMaskAndIOMask(const ShaderStageResouceUsageMaskAndIOMask &) = default;
  ShaderStageResouceUsageMaskAndIOMask &operator=(const ShaderStageResouceUsageMaskAndIOMask &) = default;
};

struct GraphicsProgramTemplate
{
  ShaderID vertexShader;
  ShaderID pixelShader;
};

struct GraphicsProgramInstance
{
  InputLayoutID inputLayout = InputLayoutID::Null();
  GraphicsProgramID internalProgram = GraphicsProgramID::Null();
  InternalInputLayoutID internalLayout = InternalInputLayoutID::Null();
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

class ShaderProgramGroup
{
  friend class ShaderProgramGroups;
  eastl::vector<ShaderStageResouceUsageMaskAndIOMask> vertexShaderResourceUseAndIoMasks;
  eastl::vector<ShaderStageResouceUsageMaskAndIOMask> pixelShaderResourceUseAndIoMasks;
  eastl::vector<ShaderStageResouceUsageMask> computeProgramResourceUses;
  eastl::vector<eastl::variant<eastl::monostate, ShaderID, ProgramID>> pixelShaderComputeProgramIDMap;
  eastl::vector<GraphicsProgramTemplate> graphicsProgramTemplates;

protected:
  void clear()
  {
    vertexShaderResourceUseAndIoMasks.clear();
    pixelShaderResourceUseAndIoMasks.clear();
    computeProgramResourceUses.clear();
    pixelShaderComputeProgramIDMap.clear();
    graphicsProgramTemplates.clear();
  }

  void trim()
  {
    vertexShaderResourceUseAndIoMasks.shrink_to_fit();
    pixelShaderResourceUseAndIoMasks.shrink_to_fit();
    computeProgramResourceUses.shrink_to_fit();
    pixelShaderComputeProgramIDMap.shrink_to_fit();
    graphicsProgramTemplates.shrink_to_fit();
  }

  uint32_t getGroupID(const ShaderProgramGroup *base) const { return 1 + (this - base); }

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

  ShaderStageResouceUsageMask getComputeProgramResourceUsage(ProgramID program) const
  {
    if (program.getIndex() < computeProgramResourceUses.size())
    {
      return computeProgramResourceUses[program.getIndex()];
    }
    return {};
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

  template <typename T>
  void iterateVertexShaders(const ShaderProgramGroup *base, T clb) const
  {
    auto id = getGroupID(base);
    for (size_t i = 0; i < vertexShaderResourceUseAndIoMasks.size(); ++i)
    {
      clb(ShaderID::make(id, i));
    }
  }

  uint32_t getVertexShaderInputMask(ShaderID id) const
  {
    if (id.getIndex() < vertexShaderResourceUseAndIoMasks.size())
    {
      return vertexShaderResourceUseAndIoMasks[id.getIndex()].ioMask;
    }
    logerr("DX12: getVertexShaderInputMask called with invalid index", id.getIndex());
    return 0;
  }

  ShaderStageResouceUsageMask getVertexShaderResourceUsage(ShaderID id) const
  {
    if (id.getIndex() < vertexShaderResourceUseAndIoMasks.size())
    {
      return vertexShaderResourceUseAndIoMasks[id.getIndex()].resourceUsage;
    }
    logerr("DX12: getVertexShaderResourceUsage called with invalid index", id.getIndex());
    return {};
  }

  uint32_t getPixelShaderOutputMask(ShaderID id) const
  {
    if (id.getIndex() < pixelShaderResourceUseAndIoMasks.size())
    {
      return pixelShaderResourceUseAndIoMasks[id.getIndex()].ioMask;
    }
    logerr("DX12: getPixelShaderOutputMask called with invalid index", id.getIndex());
    return 0;
  }

  ShaderStageResouceUsageMask getPixelShaderResourceUsage(ShaderID id) const
  {
    if (id.getIndex() < pixelShaderResourceUseAndIoMasks.size())
    {
      return pixelShaderResourceUseAndIoMasks[id.getIndex()].resourceUsage;
    }
    logerr("DX12: getPixelShaderResourceUsage called with invalid index", id.getIndex());
    return {};
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
};

class ShaderProgramGroups
{
  // group 0 is always to "global" group and is not handled by groups
  eastl::array<ShaderProgramGroup, max_scripted_shaders_bin_groups - 1> groups{};
  eastl::vector<ShaderStageResouceUsageMaskAndIOMask> vertexShaderResourceUseAndIoMasks;
  eastl::vector<ShaderStageResouceUsageMaskAndIOMask> pixelShaderResourceUseAndIoMasks;
  eastl::vector<ShaderStageResouceUsageMask> computeProgramResourceUses;
  // For shaders we only track used ids. When new shaders are created or
  // shaders are deleted we just manage the ids and hand over ownership
  // to the device context backend with the appropriate commands.
  IdManager vertexShaderIds;
  IdManager pixelShaderIds;
  IdManager computeIds;

  typedef uint64_t hash_key_t;
  HashedKeyMap<hash_key_t, uint32_t, hash_key_t(0), oa_hashmap_util::MumStepHash<hash_key_t>> graphicsProgramsHashMap;

  eastl::vector<GraphicsProgramTemplate> graphicsProgramTemplates;
  eastl::vector<GraphicsProgramInstance> graphicsProgramInstances;
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
  void trim(ShaderProgramGroup *group) { group->trim(); }
  void removePixelShader(ShaderID shader) { pixelShaderIds.free(shader.getIndex()); }
  void removeVertexShader(ShaderID shader)
  {
    G_ASSERTF(0 == shader.getGroup(), "Invalid to call this function with a shader with a group id "
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
    for (auto &g : groups)
    {
      g.iterateVertexShaders(groups.data(), clb);
    }
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
  ProgramID addComputeShaderProgram(ShaderStageResouceUsageMask usage)
  {
    auto program = ProgramID::asComputeProgram(0, computeIds.allocate());
    setComputeProgramResourceUsageMask(program, usage);
    return program;
  }
  ShaderID addPixelShader(ShaderStageResouceUsageMask usage, uint32_t io_mask)
  {
    auto id = ShaderID::make(0, pixelShaderIds.allocate());
    // io mask is a color output mask with bits for each channel, we only need the target mask here,
    // so we compute it here to avoid redoing it at the needed places over and over again.
    setPixelShaderResrouceUseAndOutputMask(id, usage, color_channel_mask_to_render_target_mask(io_mask));
    return id;
  }
  ShaderID addVertexShader(ShaderStageResouceUsageMask usage, uint32_t io_mask)
  {
    auto id = ShaderID::make(0, vertexShaderIds.allocate());
    setVertexShaderResrouceUseAndInputMask(id, usage, io_mask);
    return id;
  }
  void setComputeProgramResourceUsageMask(ProgramID program, ShaderStageResouceUsageMask usage)
  {
    set_at(computeProgramResourceUses, program.getIndex(), usage);
  }
  ShaderStageResouceUsageMask getComputeProgramResourceUsage(ProgramID program) const
  {
    if (auto group = getShaderGroupForProgram(program))
    {
      return group->getComputeProgramResourceUsage(program);
    }
    else if (program.getIndex() < computeProgramResourceUses.size())
    {
      return computeProgramResourceUses[program.getIndex()];
    }
    return {};
  }
  void setVertexShaderResrouceUseAndInputMask(ShaderID id, ShaderStageResouceUsageMask usage, uint32_t io_mask)
  {
    set_at(vertexShaderResourceUseAndIoMasks, id.getIndex(), ShaderStageResouceUsageMaskAndIOMask{usage, io_mask});
  }
  void setPixelShaderResrouceUseAndOutputMask(ShaderID id, ShaderStageResouceUsageMask usage, uint32_t io_mask)
  {
    set_at(pixelShaderResourceUseAndIoMasks, id.getIndex(), ShaderStageResouceUsageMaskAndIOMask{usage, io_mask});
  }
  uint32_t getVertexShaderInputMask(ShaderID id) const
  {
    if (auto group = getShaderGroupForShader(id))
    {
      return group->getVertexShaderInputMask(id);
    }
    else if (id.getIndex() < vertexShaderResourceUseAndIoMasks.size())
    {
      return vertexShaderResourceUseAndIoMasks[id.getIndex()].ioMask;
    }
    return 0;
  }
  uint32_t getPixelShaderOutputMask(ShaderID id) const
  {
    if (auto group = getShaderGroupForShader(id))
    {
      return group->getPixelShaderOutputMask(id);
    }
    else if (id.getIndex() < pixelShaderResourceUseAndIoMasks.size())
    {
      return pixelShaderResourceUseAndIoMasks[id.getIndex()].ioMask;
    }
    return 0;
  }
  ShaderStageResouceUsageMask getVertexShaderResourceUsage(ShaderID id) const
  {
    if (auto group = getShaderGroupForShader(id))
    {
      return group->getVertexShaderResourceUsage(id);
    }
    else if (id.getIndex() < vertexShaderResourceUseAndIoMasks.size())
    {
      return vertexShaderResourceUseAndIoMasks[id.getIndex()].resourceUsage;
    }
    return {};
  }
  ShaderStageResouceUsageMask getPixelShaderResourceUsage(ShaderID id) const
  {
    if (auto group = getShaderGroupForShader(id))
    {
      return group->getPixelShaderResourceUsage(id);
    }
    else if (id.getIndex() < pixelShaderResourceUseAndIoMasks.size())
    {
      return pixelShaderResourceUseAndIoMasks[id.getIndex()].resourceUsage;
    }
    return {};
  }
  void reset()
  {
    for (auto &group : groups)
      group.clear();
    vertexShaderIds.clear();
    pixelShaderIds.clear();
    computeIds.clear();
    graphicsProgramTemplates.clear();
    clearGraphicsProgramsInstances();
  }

  GraphicsProgramID findGraphicsProgram(ShaderID vs, ShaderID ps) const
  {
    if (auto group = getShaderGroupForShaders(vs, ps))
    {
      return group->findGraphicsProgram(groups.data(), vs, ps);
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
    if (auto group = getShaderGroupForShaders(vs, ps))
    {
      return group->addGraphicsProgram(groups.data(), vs, ps);
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

  GraphicsProgramTemplate getShadersOfGraphicsProgram(GraphicsProgramID gpid) const
  {
    if (auto group = getShaderGroupForGraphicsProgram(gpid))
    {
      return group->getShadersOfGraphicsProgram(gpid);
    }
    return graphicsProgramTemplates[gpid.getIndex()];
  }

  template <typename T>
  void iterateAllGraphicsPrograms(T clb)
  {
    for (auto &group : groups)
    {
      group.iterateGraphicsPrograms(groups.data(), clb);
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
    if (auto group = getShaderGroupForShader(vs))
    {
      group->removeGraphicsProgramWithMatchingVertexShader(groups.data(), vs, clb);
    }

    // should a vertex shader with group 1 be used with a pixel shader of group 2
    // it will end up be in the global group
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
    if (auto group = getShaderGroupForShader(ps))
    {
      group->removeGraphicsProgramWithMatchingPixelShader(groups.data(), ps, clb);
    }
    else
    {
      // ps with group 0 can be put in any group, the vs will pull it in
      for (auto &group : groups)
      {
        group.removeGraphicsProgramWithMatchingPixelShader(groups.data(), ps, clb);
      }
    }

    // either if in group 0 or in a group other than 1 and combined with a vs that is not in the same group
    // results being put into the global group
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
  ProgramID instanciateGraphicsProgram(hash_key_t key, GraphicsProgramID gp, InputLayoutID il, InternalInputLayoutID iil)
  {
    auto refEmpty = end(graphicsProgramInstances);
    auto refSame = eastl::find_if(begin(graphicsProgramInstances), end(graphicsProgramInstances), [&](auto &instance) {
      if (GraphicsProgramID::Null() == instance.internalProgram)
        refEmpty = &instance;
      return instance.internalProgram == gp && instance.inputLayout == il && instance.internalLayout == iil;
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
    refEmpty->internalLayout = iil;
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
    info.internalInputLayoutID = inst.internalLayout;
    auto programShaders = getShadersOfGraphicsProgram(info.programId);

    // fixme: optimize me
    info.ioMask.usedAttributeLocations = getVertexShaderInputMask(programShaders.vertexShader);
    info.vertexShaderStageResourceUses = getVertexShaderResourceUsage(programShaders.vertexShader);

    info.pixelShaderStageResourceUses = getPixelShaderResourceUsage(programShaders.pixelShader);
    info.ioMask.usedOutputs = getPixelShaderOutputMask(programShaders.pixelShader);
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
};
class PipelineCache;
class ShaderProgramDatabase
{
  struct ExternalToInternalInputLayoutTableEntry
  {
    InputLayout fullLayout;
    // this is a table of variations of this layout of uses that use a subset of all attributes
    eastl::vector<eastl::pair<uint32_t, InternalInputLayoutID>> variations;

    ExternalToInternalInputLayoutTableEntry() = default;
    ExternalToInternalInputLayoutTableEntry(const InputLayout &l) : fullLayout{l} {}
  };
  mutable OSReadWriteLock dataGuard;
#if D3D_HAS_RAY_TRACING
  eastl::vector<RaytraceProgram> raytracePrograms;
#endif
  eastl::vector<ExternalToInternalInputLayoutTableEntry> publicImputLayoutTable;
  eastl::vector<InputLayout> internalInputLayoutTable;

  ProgramID debugProgram = ProgramID::Null();
  ShaderID nullPixelShader = ShaderID::Null();
  InternalInputLayoutID nullLayout = InternalInputLayoutID::Null();
  bool disablePreCache = false;

  ShaderProgramGroups shaderProgramGroups;

  void initDebugProgram(DeviceContext &ctx);
  void initNullPixelShader(DeviceContext &ctx);
  ShaderID newRawVertexShader(DeviceContext &ctx, const dxil::ShaderHeader &header, dag::ConstSpan<uint8_t> byte_code);
  ShaderID newRawPixelShader(DeviceContext &ctx, const dxil::ShaderHeader &header, dag::ConstSpan<uint8_t> byte_code);
  static StageShaderModule decodeShaderBinary(const void *data, uint32_t size);
  static eastl::unique_ptr<VertexShaderModule> decodeVertexShader(const void *data, uint32_t size);
  static eastl::unique_ptr<PixelShaderModule> decodePixelShader(const void *data, uint32_t size);

public:
  ProgramID newComputeProgram(DeviceContext &ctx, const void *data);
  ProgramID newGraphicsProgram(DeviceContext &ctx, InputLayoutID vdecl, ShaderID vs, ShaderID ps);
  InputLayoutID getInputLayoutForGraphicsProgram(ProgramID program);
  GraphicsProgramUsageInfo getGraphicsProgramForStateUpdate(ProgramID program);
  ShaderStageResouceUsageMask getComputeProgramResourceUsage(ProgramID program)
  {
    ScopedLockReadTemplate<OSReadWriteLock> lock(dataGuard);
    return shaderProgramGroups.getComputeProgramResourceUsage(program);
  }
  InputLayoutID registerInputLayoutInternal(const InputLayout &layout);
  InputLayoutID registerInputLayout(const InputLayout &layout);
  void setup(DeviceContext &ctx, bool disable_precache);
  void shutdown(DeviceContext &ctx);
  ShaderID newVertexShader(DeviceContext &ctx, const void *data);
  ShaderID newPixelShader(DeviceContext &ctx, const void *data);
  ProgramID getDebugProgram();
  void removeProgram(DeviceContext &ctx, ProgramID prog);
  void deleteVertexShader(DeviceContext &ctx, ShaderID shader);
  void deletePixelShader(DeviceContext &ctx, ShaderID shader);
  void updateVertexShaderName(DeviceContext &ctx, ShaderID shader, const char *name);
  void updatePixelShaderName(DeviceContext &ctx, ShaderID shader, const char *name);

#if D3D_HAS_RAY_TRACING
  ProgramID newRaytraceProgram(DeviceContext &ctx, const ShaderID *shader_ids, uint32_t shader_count,
    const RaytraceShaderGroup *shader_groups, uint32_t group_count, uint32_t max_recursion_depth);
  ShaderStageResouceUsageMask getRaytracingProgramResourceUsage(ProgramID program)
  {
    G_UNUSED(program);
    return ShaderStageResouceUsageMask{~0ul};
  }
#endif
  enum class Validation
  {
    Skip,
    Do
  };
  enum class ContextLockState
  {
    Unlocked,
    Locked
  };
  InternalInputLayoutID getInternalInputLayout(DeviceContext &ctx, InputLayoutID external_id, GraphicsProgramIOMask io_mask,
    Validation validation, ContextLockState ctx_locked);
  void loadFromCache(PipelineCache &cache);
  void registerShaderBinDump(Device &device, ScriptedShadersBinDumpOwner *dump);
};

namespace backend
{
class ShaderModuleManager
{
  eastl::vector<eastl::unique_ptr<VertexShaderModule>> vertexShaders[max_scripted_shaders_bin_groups];
  eastl::vector<eastl::unique_ptr<PixelShaderModule>> pixelShaders[max_scripted_shaders_bin_groups];

public:
  void addVertexShader(ShaderID id, VertexShaderModule *module)
  {
    set_at(vertexShaders[id.getGroup()], id.getIndex(), eastl::unique_ptr<VertexShaderModule>{module});
  }

  void addPixelShader(ShaderID id, PixelShaderModule *module)
  {
    set_at(pixelShaders[id.getGroup()], id.getIndex(), eastl::unique_ptr<PixelShaderModule>{module});
  }

  const VertexShaderModule *getVertexShader(ShaderID id) const { return vertexShaders[id.getGroup()][id.getIndex()].get(); }

  const PixelShaderModule *getPixelShader(ShaderID id) const { return pixelShaders[id.getGroup()][id.getIndex()].get(); }

  VertexShaderModule *modifyVertexShader(ShaderID id) { return vertexShaders[id.getGroup()][id.getIndex()].get(); }

  PixelShaderModule *modifyPixelShader(ShaderID id) { return pixelShaders[id.getGroup()][id.getIndex()].get(); }

  eastl::unique_ptr<VertexShaderModule> releaseVertexShader(ShaderID id)
  {
    return eastl::move(vertexShaders[id.getGroup()][id.getIndex()]);
  }

  eastl::unique_ptr<PixelShaderModule> releasePixelShader(ShaderID id)
  {
    return eastl::move(pixelShaders[id.getGroup()][id.getIndex()]);
  }
};
}; // namespace backend
} // namespace drv3d_dx12