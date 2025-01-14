// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bitfield.h"
#include "byte_units.h"
#include "driver.h"
#include "dynamic_array.h"
#include "render_state.h"
#include "shader_program_id.h"
#include "tagged_handles.h"

#include <dag/dag_vectorSet.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <drv/shadersMetaData/dxil/utility.h>
#include <EASTL/array.h>
#include <EASTL/sort.h>
#include <EASTL/unordered_set.h>
#include <EASTL/variant.h>
#include <generic/dag_bitset.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_rwLock.h>
#include <shadersBinaryData.h>
#include <util/dag_hashedKeyMap.h>
#include <util/dag_watchdog.h>


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
  eastl::unique_ptr<uint8_t[]> byteCode;
  size_t byteCodeSize = 0;

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
  Bitset<dxil::MAX_B_REGISTERS> bRegisterMask;
  Bitset<dxil::MAX_T_REGISTERS> tRegisterMask;
  Bitset<dxil::MAX_S_REGISTERS> sRegisterMask;
  Bitset<dxil::MAX_U_REGISTERS> uRegisterMask;

  ShaderStageResouceUsageMask() = default;
  ShaderStageResouceUsageMask(const ShaderStageResouceUsageMask &) = default;
  ShaderStageResouceUsageMask &operator=(const ShaderStageResouceUsageMask &) = default;
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

struct GraphicsProgramUsageInfo
{
  InputLayoutID input = InputLayoutID::Null();
  GraphicsProgramID programId = GraphicsProgramID::Null();
};

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

template <bool VisitOnce = false, typename T>
void inspect_scripted_shader_bin_dump(ScriptedShadersBinDumpOwner *dump, T inspector)
{
  auto v2 = dump->getDumpV2();
  if (!v2)
  {
    return;
  }
  inspector.vertexShaderCount(v2->vprCount);
  inspector.pixelOrComputeShaderCount(v2->fshCount);
  eastl::conditional_t<VisitOnce, dag::VectorSet<uint32_t, eastl::less<uint32_t>, framemem_allocator>, uint32_t> visited;
  for (auto &cls : v2->classes)
  {
    if constexpr (VisitOnce)
      visited.reserve(visited.capacity() + cls.shrefStorage.size());
    for (auto &prog : cls.shrefStorage)
    {
      ::watchdog_kick(); // REMOVEME: dirty, dirty hack to work around watchdog killing us during pipeline cache loading

      if constexpr (VisitOnce)
        if (!visited.insert((uint32_t(prog.vprId) << 16) | prog.fshId).second)
          continue;

      if (prog.vprId != 0xFFFF)
      {
        if (prog.fshId != 0xFFFF)
        {
          inspector.addGraphicsProgram(prog.vprId, prog.fshId);
        }
        else
        {
          inspector.addGraphicsProgramWithNullPixelShader(prog.vprId);
        }
      }
      else
      {
        if (prog.fshId != 0xFFFF)
        {
          inspector.addComputeProgram(prog.fshId);
        }
      }
    }
  }
}

class ShaderProgramGroup
{
  friend class ShaderProgramGroups;
  ScriptedShadersBinDumpOwner *dump = nullptr;
  dag::Vector<eastl::variant<eastl::monostate, ShaderID, ProgramID>> pixelShaderComputeProgramIDMap;
  dag::Vector<GraphicsProgramTemplate> graphicsProgramTemplates;
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
      }
      auto vsID = ShaderID::make(groupID, vs_index);
      auto ref = eastl::find_if(begin(target->graphicsProgramTemplates), end(target->graphicsProgramTemplates),
        [vsID, psID](auto &prog) { return vsID == prog.vertexShader && psID == prog.pixelShader; });
      if (end(target->graphicsProgramTemplates) == ref)
      {
        GraphicsProgramTemplate prog{vsID, psID};
        target->graphicsProgramTemplates.push_back(prog);
      }
    }
    void addGraphicsProgramWithNullPixelShader(uint16_t vs_index)
    {
      auto psID = nullPixelShader;
      auto vsID = ShaderID::make(groupID, vs_index);
      auto ref = eastl::find_if(begin(target->graphicsProgramTemplates), end(target->graphicsProgramTemplates),
        [vsID, psID](auto &prog) { return vsID == prog.vertexShader && psID == prog.pixelShader; });
      if (end(target->graphicsProgramTemplates) == ref)
      {
        GraphicsProgramTemplate prog{vsID, psID};
        target->graphicsProgramTemplates.push_back(prog);
      }
    }
    void addComputeProgram(uint32_t shader_index)
    {
      if (!eastl::holds_alternative<ProgramID>(target->pixelShaderComputeProgramIDMap[shader_index]))
      {
        target->pixelShaderComputeProgramIDMap[shader_index] = ProgramID::asComputeProgram(groupID, target->computeShaderCount++);
      }
    }
  };

protected:
  void clear()
  {
    dump = nullptr;
    pixelShaderComputeProgramIDMap.clear();
    graphicsProgramTemplates.clear();
    vertexShaderCount = 0;
    pixelShaderCount = 0;
    computeShaderCount = 0;
  }

  void setup(const ShaderProgramGroup *base, ScriptedShadersBinDumpOwner *d, ShaderID null_pixel_shader)
  {
    dump = d;
    inspect_scripted_shader_bin_dump(d, ScriptedShaderBinDumpInspector{getGroupID(base), null_pixel_shader, this});
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
  ProgramID addComputeShaderProgram()
  {
    auto program = ProgramID::asComputeProgram(0, computeIds.allocate());
    return program;
  }
  ShaderID addPixelShader()
  {
    auto id = ShaderID::make(0, pixelShaderIds.allocate());
    return id;
  }
  ShaderID addVertexShader()
  {
    auto id = ShaderID::make(0, vertexShaderIds.allocate());
    return id;
  }
  void reset()
  {
    for (auto &shaderGroup : groups)
      shaderGroup.clear();
    vertexShaderIds.clear();
    pixelShaderIds.clear();
    computeIds.clear();
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

StageShaderModule shader_layout_to_module(const bindump::Mapper<dxil::Shader> *layout);
template <typename ModuleType>
inline ModuleType decode_shader_layout(const void *data)
{
  ModuleType result;
  auto *container = bindump::map<dxil::ShaderContainer>((const uint8_t *)data);
  if (!container)
    return result;

  if (container->type == dxil::StoredShaderType::combinedVertexShader)
  {
    if constexpr (eastl::is_same_v<ModuleType, VertexShaderModule>)
    {
      auto *combined = bindump::map<dxil::VertexShaderPipeline>(container->data.data());
      result = shader_layout_to_module(&*combined->vertexShader);
      result.ident.shaderHash = container->dataHash;
      result.ident.shaderSize = container->data.size();
      if (combined->geometryShader)
      {
        result.geometryShader = eastl::make_unique<StageShaderModule>(shader_layout_to_module(&*combined->geometryShader));
      }
      if (combined->hullShader)
      {
        result.hullShader = eastl::make_unique<StageShaderModule>(shader_layout_to_module(&*combined->hullShader));
      }
      if (combined->domainShader)
      {
        result.domainShader = eastl::make_unique<StageShaderModule>(shader_layout_to_module(&*combined->domainShader));
      }
    }
  }
  else if (container->type == dxil::StoredShaderType::meshShader)
  {
    if constexpr (eastl::is_same_v<ModuleType, VertexShaderModule>)
    {
      auto *combined = bindump::map<dxil::MeshShaderPipeline>(container->data.data());
      result = shader_layout_to_module(&*combined->meshShader);
      result.ident.shaderHash = container->dataHash;
      result.ident.shaderSize = container->data.size();
      if (combined->amplificationShader)
      {
        result.geometryShader = eastl::make_unique<StageShaderModule>(shader_layout_to_module(&*combined->amplificationShader));
      }
    }
  }
  else
  {
    auto *shader = bindump::map<dxil::Shader>(container->data.data());
    result = shader_layout_to_module(shader);
    result.ident.shaderHash = container->dataHash;
    result.ident.shaderSize = container->data.size();
  }
  return result;
}
StageShaderModule decode_shader_binary(const void *data, uint32_t size);
eastl::unique_ptr<VertexShaderModule> decode_vertex_shader(const void *data, uint32_t size);
eastl::unique_ptr<PixelShaderModule> decode_pixel_shader(const void *data, uint32_t size);

struct StageShaderModuleInBinaryRef : StageShaderModuleHeader
{
  eastl::span<const uint8_t> byteCode;
};
inline uint32_t offset_to_base(const void *base, const StageShaderModuleInBinaryRef &shader)
{
  return shader.byteCode.data() - reinterpret_cast<const uint8_t *>(base);
}
using PixelShaderModuleInBinaryRef = StageShaderModuleInBinaryRef;

struct VertexShaderModuleInBinaryRef : StageShaderModuleInBinaryRef
{
  // TODO remove shader identifier as its only relevant for the vertex shader it self
  StageShaderModuleInBinaryRef geometryShader;
  StageShaderModuleInBinaryRef hullShader;
  StageShaderModuleInBinaryRef domainShader;
};

StageShaderModuleInBinaryRef shader_layout_to_module_ref(const bindump::Mapper<dxil::Shader> *layout);
template <typename ModuleType>
inline ModuleType decode_shader_layout_ref(const void *data)
{
  ModuleType result;
  auto *container = bindump::map<dxil::ShaderContainer>((const uint8_t *)data);
  if (!container)
    return result;

  if (container->type == dxil::StoredShaderType::combinedVertexShader)
  {
    if constexpr (eastl::is_same_v<ModuleType, VertexShaderModuleInBinaryRef>)
    {
      auto *combined = bindump::map<dxil::VertexShaderPipeline>(container->data.data());
      static_cast<StageShaderModuleInBinaryRef &>(result) = shader_layout_to_module_ref(&*combined->vertexShader);
      result.ident.shaderHash = container->dataHash;
      result.ident.shaderSize = container->data.size();
      if (combined->geometryShader)
      {
        result.geometryShader = shader_layout_to_module_ref(&*combined->geometryShader);
      }
      if (combined->hullShader)
      {
        result.hullShader = shader_layout_to_module_ref(&*combined->hullShader);
      }
      if (combined->domainShader)
      {
        result.domainShader = shader_layout_to_module_ref(&*combined->domainShader);
      }
    }
  }
  else if (container->type == dxil::StoredShaderType::meshShader)
  {
    if constexpr (eastl::is_same_v<ModuleType, VertexShaderModuleInBinaryRef>)
    {
      auto *combined = bindump::map<dxil::MeshShaderPipeline>(container->data.data());
      static_cast<StageShaderModuleInBinaryRef &>(result) = shader_layout_to_module_ref(&*combined->meshShader);
      result.ident.shaderHash = container->dataHash;
      result.ident.shaderSize = container->data.size();
      if (combined->amplificationShader)
      {
        result.geometryShader = shader_layout_to_module_ref(&*combined->amplificationShader);
      }
    }
  }
  else
  {
    auto *shader = bindump::map<dxil::Shader>(container->data.data());
    static_cast<StageShaderModuleInBinaryRef &>(result) = shader_layout_to_module_ref(shader);
    result.ident.shaderHash = container->dataHash;
    result.ident.shaderSize = container->data.size();
  }
  return result;
}

StageShaderModuleInBinaryRef decode_shader_binary_ref(const void *data, uint32_t size);
VertexShaderModuleInBinaryRef decode_vertex_shader_ref(const void *data, uint32_t size);
PixelShaderModuleInBinaryRef decode_pixel_shader_ref(const void *data, uint32_t size);

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
  ShaderID newRawVertexShader(DeviceContext &ctx, const dxil::ShaderHeader &header, dag::ConstSpan<uint8_t> byte_code);
  ShaderID newRawPixelShader(DeviceContext &ctx, const dxil::ShaderHeader &header, dag::ConstSpan<uint8_t> byte_code);

public:
  ProgramID newComputeProgram(DeviceContext &ctx, const void *data, CSPreloaded preloaded);
  ProgramID newGraphicsProgram(DeviceContext &ctx, InputLayoutID vdecl, ShaderID vs, ShaderID ps);
  InputLayoutID getInputLayoutForGraphicsProgram(ProgramID program);
  GraphicsProgramUsageInfo getGraphicsProgramForStateUpdate(ProgramID program);
  InputLayoutID registerInputLayoutInternal(DeviceContext &ctx, const InputLayout &layout);
  InputLayoutID registerInputLayout(DeviceContext &ctx, const InputLayout &layout);
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
    G_ASSERTF(externalToInternalMap.size() > id.get(), "dx12: trying to use invalid input layout ID %u registered layouts %u",
      id.get(), externalToInternalMap.size());
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
  struct DecompressedGroup
  {
    UncompressedMemoryProvider uncompressed;
    bindump::Mapper<shader_layout::ShGroup> *shaderGroup = nullptr;
  };
  struct ScriptedShadersBinDumpState
  {
    ScriptedShadersBinDumpOwner *owner = nullptr;
    eastl::unique_ptr<DecompressedGroup[]> decompressedGroups;
    eastl::string name;
  };
  ScriptedShadersBinDumpState dumps[max_scripted_shaders_bin_groups]{};

public:
  ScriptedShadersBinDumpOwner *getDump(uint32_t group) { return dumps[group].owner; }
  template <typename T>
  void enumerateShaderFromHash(const dxil::HashValue &hash, T reciever) const
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
        if (!reciever(gi, si, v2->vprCount))
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
    target.decompressedGroups = eastl::make_unique<DecompressedGroup[]>(dump->getDump()->shGroups.size());
    target.name = name;
  }
  void resetDumpOfGroup(uint32_t shaderGroup)
  {
    auto &target = dumps[shaderGroup];
    target.owner = nullptr;
    target.decompressedGroups.reset();
    target.name.clear();
  }
  // returned memory range is valid until the end of the frame, after that its undefined.
  eastl::span<uint8_t> getShaderByteCode(uint32_t shaderGroup, uint32_t compression_group, uint32_t compression_index)
  {
    auto &dump = dumps[shaderGroup];
    if (!dump.decompressedGroups)
    {
      return {nullptr, nullptr};
    }
    auto &compGroup = dump.decompressedGroups[compression_group];
    if (!compGroup.shaderGroup)
    {
      logdbg("DX12: Decompressing shader shaderGroup %u of dump %p...", compression_group, dump.owner);
      compGroup.shaderGroup = dump.owner->getDump()->shGroups[compression_group].decompress(compGroup.uncompressed,
        bindump::BehaviorOnUncompressed::Reference, dump.owner->getDecompressionDict());
      ByteUnits uncompressedByteSize = compGroup.uncompressed.size();
      logdbg("DX12: ...decompressed into %.2f %s...", uncompressedByteSize.units(), uncompressedByteSize.name());
    }
    auto &shader = compGroup.shaderGroup->shaders[compression_index];
    return {shader.begin(), shader.end()};
  }
  // currently always evicts cache
  void evictDecompressionCache()
  {
    for (auto &dump : dumps)
    {
      if (!dump.decompressedGroups)
        continue;
      for (uint32_t i = 0; i < dump.owner->getDump()->shGroups.size(); ++i)
      {
        UncompressedMemoryProvider{}.swap(dump.decompressedGroups[i].uncompressed);
        dump.decompressedGroups[i].shaderGroup = nullptr;
      }
    }
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
      auto v2 = group.owner->getDumpV2();
      if (!v2)
      {
        continue;
      }
      auto shaderCount = v2->vprCount;
      for (uint32_t shaderIndex = 0; shaderIndex < shaderCount; ++shaderIndex)
      {
        if (v2->shaderHashes[shaderIndex] == hash)
        {
          return {groupIndex, shaderIndex};
        }
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
      auto v2 = group.owner->getDumpV2();
      if (!v2)
      {
        continue;
      }
      auto shaderIndexOffset = v2->vprCount;
      auto shaderCount = v2->shGroupsMapping.size() - shaderIndexOffset;
      for (uint32_t shaderIndex = 0; shaderIndex < shaderCount; ++shaderIndex)
      {
        if (v2->shaderHashes[shaderIndexOffset + shaderIndex] == hash)
        {
          return {groupIndex, shaderIndex};
        }
      }
    }
    return {max_scripted_shaders_bin_groups, 0};
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
};

struct StageShaderModuleBytecode
{
  eastl::unique_ptr<uint8_t[]> bytecode;
  size_t bytecodeSize = 0;
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
  uint16_t compressionGroup = 0;
  uint16_t compressionIndex = 0;
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
};

class PipelineNameGenerator : public ScriptedShadersBinDumpManager
{
  using BaseType = ScriptedShadersBinDumpManager;

  struct GeneratorContext
  {
    struct DynamicVariantInfo
    {
      uint32_t index = 0;
      dag::Vector<uint32_t> codes;
    };
    struct StaticVarantInfo
    {
      uint32_t index = 0;
      dag::Vector<uint32_t> codes;
      dag::Vector<DynamicVariantInfo> dynamicVariants;
    };
    struct ClassInfo
    {
      const shaderbindump::ShaderClass *shaderClass = nullptr;
      dag::Vector<StaticVarantInfo> staticVariants;
    };
    dag::Vector<ClassInfo> classReferences;
    ScriptedShadersBinDumpV2 *v2 = nullptr;
    uint32_t lastStaticCode = ~0u;
    uint32_t lastStaticVariantIndex = 0;
    uint32_t lastDynamicCode = ~0u;
    eastl::string_view affixes[2];
    eastl::string finalName;

    const shaderbindump::ShaderClass *lastShaderClass() const
    {
      if (classReferences.empty())
      {
        return nullptr;
      }
      return classReferences.back().shaderClass;
    }

    struct VariableValueTable
    {
      struct VariableInfo
      {
        eastl::string_view name;
        eastl::span<const float> valueRange;
        // intervals usually intervals have about 2-4 indices, instead of having a set with each value, we simply use a bit mask where
        // each bit stands for the used index
        uint32_t valueMask;
        bool isEnumeration() const
        {
          if (!valueRange.data())
          {
            return true;
          }
          for (uint32_t i = 0; i < valueRange.size(); ++i)
          {
            if (uint32_t(valueRange[i]) != i)
            {
              return false;
            }
          }
          return true;
        }
        bool usesIntegerValues() const
        {
          if (!valueRange.data())
          {
            return true;
          }
          for (auto &value : valueRange)
          {
            if (floor(value) != value)
            {
              return false;
            }
          }
          return true;
        }
        bool usesAllValues() const
        {
          if (!valueRange.data())
          {
            return true;
          }
          return 0 == (valueMask ^ (1u << (valueRange.size() + 1)) - 1);
        }
        // has 3 formatting modes:
        // - float interval, all values are appended as float (%f), all except the last index is prefixed with a '<' to match the
        // interval definition, the last entry is prefixed with '>=' to indicate its the last value representing everything else.
        // - int interval, when all values in the interval can be represented as int without loss, the values are append as int (%d),
        // prefixing is the same as for the float interval.
        // - enum, when all values in an interval match their index value, then each index is appended as uint (%u) without prefix.
        void appendValuesToString(eastl::string &target)
        {
          if (!valueMask)
          {
            return;
          }
          if (usesAllValues())
          {
            target += "any-value";
            return;
          }

          char buf[32];
          bool isInt = usesIntegerValues();
          bool isEnum = false;
          if (isInt)
          {
            isEnum = isEnumeration();
          }
          for (uint32_t i = 0; i < 32; ++i)
          {
            if (0 == (valueMask & (1u << i)))
            {
              continue;
            }
            if (isEnum || !valueRange.data())
            {
              sprintf_s(buf, "%u", i);
              target += buf;
              target.push_back(',');
              continue;
            }
            auto index = i;
            if (index >= valueRange.size())
            {
              target.push_back('>');
              target.push_back('=');
              --index;
            }
            else
            {
              target.push_back('<');
            }
            if (isInt)
            {
              sprintf_s(buf, "%d", int(valueRange[index]));
            }
            else
            {
              sprintf_s(buf, "%f", valueRange[index]);
            }
            target += buf;
            target.push_back(',');
          }
          target.pop_back();
        }
      };
      dag::Vector<VariableInfo> variables;
      void addVariableValue(eastl::string_view name, eastl::span<const float> value_range, uint32_t value)
      {
        auto ref = eastl::find_if(begin(variables), end(variables), [name](auto &info) { return name == info.name; });
        if (ref == end(variables))
        {
          VariableInfo newValue{name, value_range, 0};
          ref = variables.insert(ref, newValue);
        }
        G_ASSERT(value < 32);
        ref->valueMask |= 1u << value;
      }
      void appendToString(eastl::string &name)
      {
        for (auto &var : variables)
        {
          // don't print intervals that don't contribute
          if (var.usesAllValues())
          {
            continue;
          }
          name.append(begin(var.name), end(var.name));
          name.push_back('=');
          name.push_back('[');
          var.appendValuesToString(name);
          name.push_back(']');
          name.push_back(',');
        }
      }
      bool hasValues() const { return !variables.empty(); }
      void clear() { variables.clear(); }
    };

    void insertUniqueVariableValues(const shaderbindump::VariantTable &variants, eastl::span<const uint32_t> codes,
      VariableValueTable &target)
    {
      if (0 == variants.codePieces.size())
      {
        return;
      }
      for (auto [intervalID, intervalMul] : variants.codePieces)
      {
        auto &interval = v2->intervals[intervalID];
        for (auto code : codes)
        {
          target.addVariableValue(static_cast<eastl::string_view>(v2->varMap[interval.nameId]),
            {interval.maxVal.data(), interval.maxVal.size()}, (code / intervalMul) % interval.getValCount());
        }
      }
    }

    void start(ScriptedShadersBinDumpV2 *dump) { v2 = dump; }
    void finish()
    {
      if (classReferences.empty())
      {
        return;
      }
      finalName.clear();
      VariableValueTable variableValues;
      for (auto &cRef : classReferences)
      {
        variableValues.clear();
        finalName.append({static_cast<const char *>(cRef.shaderClass->name), cRef.shaderClass->name.size() - 1});
        for (auto &sVar : cRef.staticVariants)
        {
          insertUniqueVariableValues(cRef.shaderClass->stVariants, sVar.codes, variableValues);
        }
        if (variableValues.hasValues())
        {
          beginVariableBlock(true);
          variableValues.appendToString(finalName);
          endVariableBlock(true);
        }
        variableValues.clear();
        for (auto &sVar : cRef.staticVariants)
        {
          for (auto &dVar : sVar.dynamicVariants)
          {
            insertUniqueVariableValues(cRef.shaderClass->code[sVar.index].dynVariants, dVar.codes, variableValues);
          }
        }
        if (variableValues.hasValues())
        {
          beginVariableBlock(true);
          variableValues.appendToString(finalName);
          endVariableBlock(true);
        }
        finalName.push_back('\n');
      }
      finalName.pop_back();
      for (auto &affix : affixes)
      {
        finalName.append(begin(affix), end(affix));
      }
    }
    void setAffixes(eastl::string_view affix_1, eastl::string_view affix_2)
    {
      affixes[0] = affix_1;
      affixes[1] = affix_2;
    }

    void beginShaderClass(const shaderbindump::ShaderClass &shader_class)
    {
      if (lastShaderClass() == &shader_class)
      {
        return;
      }
      lastStaticCode = ~uint32_t{0};
      ClassInfo newClassInfo;
      newClassInfo.shaderClass = &shader_class;
      classReferences.push_back(eastl::move(newClassInfo));
    }
    void beginStaticVariant(uint32_t code)
    {
      if (lastStaticCode == code || !lastShaderClass())
      {
        return;
      }
      lastStaticCode = code;
      lastStaticVariantIndex = lastShaderClass()->stVariants.findVariant(code);
      lastDynamicCode = ~uint32_t{0};
      auto &target = classReferences.back();
      auto ref = eastl::find_if(begin(target.staticVariants), end(target.staticVariants),
        [this](const auto &s_var) { return s_var.index == this->lastStaticVariantIndex; });
      if (ref == end(target.staticVariants))
      {
        StaticVarantInfo sVar;
        sVar.index = lastStaticVariantIndex;
        ref = target.staticVariants.insert(ref, std::move(sVar));
      }
      ref->codes.push_back(code);
    }
    void beginDynamicVariant(uint32_t code)
    {
      if (lastDynamicCode == code || !lastShaderClass())
      {
        return;
      }
      lastDynamicCode = code;
      auto index = lastShaderClass()->code[lastStaticVariantIndex].dynVariants.findVariant(code);
      auto &base = classReferences.back();
      auto baseRef = eastl::find_if(begin(base.staticVariants), end(base.staticVariants),
        [this](const auto &s_var) { return s_var.index == this->lastStaticVariantIndex; });
      auto ref = eastl::find_if(begin(baseRef->dynamicVariants), end(baseRef->dynamicVariants),
        [index](const auto &d_var) { return d_var.index == index; });
      if (ref == end(baseRef->dynamicVariants))
      {
        DynamicVariantInfo dVar;
        dVar.index = index;
        ref = baseRef->dynamicVariants.insert(ref, eastl::move(dVar));
      }
      ref->codes.push_back(code);
    }
    void prepareStringSpace(size_t extra_space) { finalName.reserve(finalName.size() + extra_space); }
    void beginVariableBlock(bool is_static_vars) { finalName.push_back(is_static_vars ? '(' : '{'); }
    void endVariableBlock(bool is_static_vars) { finalName.back() = is_static_vars ? ')' : '}'; }
    void beginValueSet()
    {
      finalName.back() = '=';
      finalName.push_back('{');
    }
    void endValueSet()
    {
      finalName.back() = '}';
      finalName.push_back(' ');
    }
    void appendVariableValue(uint32_t value)
    {
      char buffer[12];
      auto ln = sprintf_s(buffer, "%u", value);
      finalName.append(buffer, buffer + ln);
      finalName.push_back(',');
    }
    void appendVariableName(eastl::string_view name)
    {
      finalName.append(begin(name), end(name));
      finalName.push_back(',');
    }

    void onShaderClassPassUse(const shaderbindump::ShaderClass &shader_class, uint32_t static_code, uint32_t dynamic_code)
    {
      beginShaderClass(shader_class);
      beginStaticVariant(static_code);
      beginDynamicVariant(dynamic_code);
    }
  };

  void iterateShaderClassesForComputeShader(const dxil::HashValue &hash, eastl::string &out_name)
  {
    auto [group, index] = findComputeShader(0, hash);
    if (group >= max_scripted_shaders_bin_groups)
    {
      return;
    }

    GeneratorContext ctx;
    auto v2 = getDump(group)->getDumpV2();
    ctx.start(v2);

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

          generateNamesForShaderClassCodes(cls, si, di, ctx);
        }
      }
    }

    ctx.finish();
    out_name = eastl::move(ctx.finalName);
  }

  template <typename U>
  void visitShaderClassesForComputeShader(const dxil::HashValue &hash, U hanlder)
  {
    auto [group, index] = findComputeShader(0, hash);
    if (group >= max_scripted_shaders_bin_groups)
    {
      return;
    }

    auto v2 = getDump(group)->getDumpV2();

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

          visitForShaderClassCodes(cls, si, di, false, false, false, false, hanlder);
        }
      }
    }
  }

  void generateNamesForShaderClassCodes(const shaderbindump::ShaderClass &shader_class, uint32_t static_code, uint32_t dynamic_code,
    GeneratorContext &ctx)
  {
    auto &sVar = shader_class.code[static_code];

    shader_class.stVariants.enumerateCodesForVariant(static_code, [&shader_class, &ctx, &sVar, dynamic_code](uint32_t s_code) {
      sVar.dynVariants.enumerateCodesForVariant(dynamic_code,
        [&shader_class, &ctx, s_code](uint32_t d_code) { ctx.onShaderClassPassUse(shader_class, s_code, d_code); });
    });
  }

  template <typename U>
  void visitForShaderClassCodes(const shaderbindump::ShaderClass &shader_class, uint32_t static_index, uint32_t dynamic_index,
    bool use_static_state_override, bool use_null_pixel_shader, bool use_with_pixel_shader_override, bool is_pixel_shader_override,
    U hanlder)
  {
    auto &sVar = shader_class.code[static_index];

    shader_class.stVariants.enumerateCodesForVariant(static_index,
      [&shader_class, &sVar, dynamic_index, &hanlder, use_static_state_override, use_null_pixel_shader, use_with_pixel_shader_override,
        is_pixel_shader_override](uint32_t s_code) {
        sVar.dynVariants.enumerateCodesForVariant(dynamic_index,
          [&shader_class, s_code, &hanlder, use_static_state_override, use_null_pixel_shader, use_with_pixel_shader_override,
            is_pixel_shader_override](uint32_t d_code) {
            hanlder(shader_class, s_code, d_code, use_static_state_override, use_null_pixel_shader, use_with_pixel_shader_override,
              is_pixel_shader_override);
          });
      });
  }

  template <typename T, typename U>
  uint32_t visitMatchingRenderPasses(uint32_t group, T checker, bool use_static_state_override, bool use_null_pixel_shader,
    bool use_with_pixel_shader_override, bool is_pixel_shader_override, U hanlder)
  {
    uint32_t visitCount = 0;
    auto v2 = getDump(group)->getDumpV2();

    for (auto &cls : v2->classes)
    {
      // shrefStorage stores all the unique combinations of shaders used by this class
      auto ref = eastl::find_if(eastl::begin(cls.shrefStorage), eastl::end(cls.shrefStorage), checker);
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
          if (checker(*dVar.rpass))
          {
            visitForShaderClassCodes(cls, si, di, use_static_state_override, use_null_pixel_shader, use_with_pixel_shader_override,
              is_pixel_shader_override, hanlder);
            ++visitCount;
          }
        }
      }
    }

    return visitCount;
  }


  template <typename T>
  uint32_t generateNamesForMatchingRenderPasses(uint32_t group, eastl::string_view post_fix, eastl::string_view post_fix_2,
    eastl::string &out_name, T clb)
  {
    uint32_t visitCount = 0;
    GeneratorContext ctx;
    ctx.setAffixes(post_fix, post_fix_2);
    auto v2 = getDump(group)->getDumpV2();
    ctx.start(v2);

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
            generateNamesForShaderClassCodes(cls, si, di, ctx);
            ++visitCount;
          }
        }
      }
    }

    ctx.finish();
    out_name.append(ctx.finalName);
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
      totalCount = count;
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

  template <typename C>
  void visitShaderClassesForGraphicsShadersWithNullPixelShader(const dxil::HashValue &vertex_shader,
    const RenderStateSystem::StaticState &static_state, C target)
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

      auto count = visitMatchingRenderPasses(
        vGroup,
        [shader = vShader, state = rState](auto &pass) {
          return shader == pass.vprId && 0xFFFF == pass.fshId && state == pass.renderStateNo;
        },
        false, false, false, false, target);
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(vGroup, rState + 1, rDist, static_state);
        if (crGroup != vGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = visitMatchingRenderPasses(
          vGroup,
          [shader = vShader, state = crState](auto &pass) {
            return shader == pass.vprId && 0xFFFF == pass.fshId && state == pass.renderStateNo;
          },
          true, false, false, false, target);
      }

      if (0 == count)
      {
        count = visitMatchingRenderPasses(
          vGroup, [shader = vShader](auto &pass) { return shader == pass.vprId && 0xFFFF == pass.fshId; }, true, false, false, false,
          target);
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

      auto count = visitMatchingRenderPasses(
        vGroup, [shader = vShader, state = rState](auto &pass) { return shader == pass.vprId && state == pass.renderStateNo; }, false,
        true, false, false, target);
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(vGroup, rState + 1, rDist, static_state);
        if (crGroup != vGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = visitMatchingRenderPasses(
          vGroup, [shader = vShader, state = crState](auto &pass) { return shader == pass.vprId && state == pass.renderStateNo; },
          true, true, false, false, target);
      }

      if (0 == count)
      {
        count = visitMatchingRenderPasses(
          vGroup, [shader = vShader](auto &pass) { return shader == pass.vprId; }, true, true, false, false, target);
      }
      totalCount += count;
      lastGroup = vGroup + 1;
    }
  }

  template <typename C>
  void visitShaderClassesForGraphicsShaders(const dxil::HashValue &vertex_shader, const dxil::HashValue &pixel_shader,
    const RenderStateSystem::StaticState &static_state, C target)
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

      auto count = visitMatchingRenderPasses(
        vGroup,
        [vert = vShader, pix = pShader, state = rState](auto &pass) {
          return vert == pass.vprId && pix == pass.fshId && state == pass.renderStateNo;
        },
        false, false, false, false, target);
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(vGroup, rState + 1, rDist, static_state);
        if (crGroup != vGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = visitMatchingRenderPasses(
          vGroup, /*"[Render state override]", {}, out_name,*/
          [vert = vShader, pix = pShader, state = crState](auto &pass) {
            return vert == pass.vprId && pix == pass.fshId && state == pass.renderStateNo;
          },
          true, false, false, false, target);
      }

      if (0 == count)
      {
        count = visitMatchingRenderPasses(
          vGroup, /*"[Render state override]", {}, out_name,*/
          [vert = vShader, pix = pShader](auto &pass) { return vert == pass.vprId && pix == pass.fshId; }, true, false, false, false,
          target);
      }
      totalCount = count;
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

      auto count = visitMatchingRenderPasses(
        vGroup, /*{}, "[with pixel shader override]", out_name,*/
        [shader = vShader, state = rState](auto &pass) { return shader == pass.vprId && state == pass.renderStateNo; }, false, false,
        true, false, target);
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(vGroup, rState + 1, rDist, static_state);
        if (crGroup != vGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = visitMatchingRenderPasses(
          vGroup, /*"[Render state override]", "[with pixel shader override]", out_name,*/
          [shader = vShader, state = crState](auto &pass) { return shader == pass.vprId && state == pass.renderStateNo; }, true, false,
          true, false, target);
      }

      if (0 == count)
      {
        count = visitMatchingRenderPasses(
          vGroup, /*"[Render state override]", "[with pixel shader override]", out_name,*/
          [shader = vShader](auto &pass) { return shader == pass.vprId; }, true, false, true, false, target);
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

      auto count = visitMatchingRenderPasses(
        pGroup, /*{}, {}, out_name,*/
        [shader = pShader, state = rState](auto &pass) { return shader == pass.fshId && state == pass.renderStateNo; }, false, false,
        true, true, target);
      while (0 == count)
      {
        auto [crGroup, crState, crDist] = walkRenderStates(pGroup, rState + 1, rDist, static_state);
        if (crGroup != pGroup)
        {
          break;
        }
        rState = crState;
        rDist = crDist;
        count = visitMatchingRenderPasses(
          pGroup, /*"[Render state override]", {}, out_name,*/
          [shader = pShader, state = crState](auto &pass) { return shader == pass.fshId && state == pass.renderStateNo; }, true, false,
          true, true, target);
      }

      if (0 == count)
      {
        count = visitMatchingRenderPasses(
          pGroup, /*"[Render state override]", {}, out_name,*/
          [shader = pShader](auto &pass) { return shader == pass.fshId; }, true, false, true, true, target);
      }
      totalCount += count;
      lastGroup = pGroup + 1;
    }

    if (0 == totalCount)
    { /*
       out_name += "[pixel shader not in dump]";
       char buf[sizeof(dxil::HashValue) * 2 + 1];
       pixel_shader.convertToString(buf, sizeof(buf));
       out_name += "{";
       out_name += buf;
       out_name += "}";
       vertex_shader.convertToString(buf, sizeof(buf));
       out_name += "{";
       out_name += buf;
       out_name += "}";*/
    }
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
  void visitShaderClassPassesForGraphicsPipeline(const VertexShaderModuleRefStore &vertex_shader,
    const PixelShaderModuleRefStore &pixel_shader, const RenderStateSystem::StaticState &static_state, C target)
  {
    if (isNullPixelShader(pixel_shader))
    {
      visitShaderClassesForGraphicsShadersWithNullPixelShader(vertex_shader.header.hash, static_state, target);
    }
    else
    {
      visitShaderClassesForGraphicsShaders(vertex_shader.header.hash, pixel_shader.header.hash, static_state, target);
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

  eastl::span<uint8_t> getBytecode(const PixelShaderModuleBytecodeRef &ref)
  {
    return {ref.bytecode->bytecode.get(), ref.bytecode->bytecode.get() + ref.bytecode->bytecodeSize};
  }

  eastl::span<uint8_t> getBytecode(const PixelShaderModuleBytcodeInDumpRef &ref)
  {
    auto binary = getShaderByteCode(ref.shaderGroup, ref.bytecode->compressionGroup, ref.bytecode->compressionIndex);
    return binary.subspan(ref.bytecode->shaderOffset, ref.bytecode->shaderSize);
  }

  eastl::span<uint8_t> getVsBytecode(const VertexShaderModuleHeader &, const VertexShaderModuleBytecodeRef &ref)
  {
    return {ref.bytecode->bytecode.get(), ref.bytecode->bytecode.get() + ref.bytecode->bytecodeSize};
  }
  eastl::span<uint8_t> getVsBytecode(const VertexShaderModuleHeader &, const VertexShaderModuleBytcodeInDumpRef &ref)
  {
    auto binary = getShaderByteCode(ref.shaderGroup, ref.bytecode->compressionGroup, ref.bytecode->compressionIndex);
    return binary.subspan(ref.bytecode->shaderOffset, ref.bytecode->shaderSize);
  }
  eastl::span<uint8_t> getMsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytecodeRef &ref)
  {
    return getVsBytecode(header, ref);
  }
  eastl::span<uint8_t> getMsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytcodeInDumpRef &ref)
  {
    return getVsBytecode(header, ref);
  }
  eastl::span<uint8_t> getGsBytecode(const VertexShaderModuleHeader &, const VertexShaderModuleBytecodeRef &ref)
  {
    return getSubShaderBytecode(0, ref);
  }
  eastl::span<uint8_t> getGsBytecode(const VertexShaderModuleHeader &, const VertexShaderModuleBytcodeInDumpRef &ref)
  {
    return getSubShaderBytecode(0, ref);
  }
  eastl::span<uint8_t> getAsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytecodeRef &ref)
  {
    return getGsBytecode(header, ref);
  }
  eastl::span<uint8_t> getAsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytcodeInDumpRef &ref)
  {
    return getGsBytecode(header, ref);
  }
  eastl::span<uint8_t> getHsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytecodeRef &ref)
  {
    return getSubShaderBytecode(header.hasGsOrAs ? 1 : 0, ref);
  }
  eastl::span<uint8_t> getHsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytcodeInDumpRef &ref)
  {
    return getSubShaderBytecode(header.hasGsOrAs ? 1 : 0, ref);
  }
  eastl::span<uint8_t> getDsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytecodeRef &ref)
  {
    return getSubShaderBytecode(header.hasGsOrAs ? 2 : 1, ref);
  }
  eastl::span<uint8_t> getDsBytecode(const VertexShaderModuleHeader &header, const VertexShaderModuleBytcodeInDumpRef &ref)
  {
    return getSubShaderBytecode(header.hasGsOrAs ? 2 : 1, ref);
  }
  eastl::span<uint8_t> getSubShaderBytecode(uint32_t sub_shader_index, const VertexShaderModuleBytecodeRef &ref)
  {
    auto &subShader = ref.bytecode->subShaders[sub_shader_index];
    return {subShader.bytecode.get(), subShader.bytecode.get() + subShader.bytecodeSize};
  }
  eastl::span<uint8_t> getSubShaderBytecode(uint32_t sub_shader_index, const VertexShaderModuleBytcodeInDumpRef &ref)
  {
    auto &subShader = ref.bytecode->subShaders[sub_shader_index];
    auto binary = getShaderByteCode(ref.shaderGroup, ref.bytecode->compressionGroup, ref.bytecode->compressionIndex);
    return binary.subspan(subShader.shaderOffset, subShader.shaderSize);
  }

public:
  void addVertexShader(ShaderID id, VertexShaderModule *module);
  void addPixelShader(ShaderID id, PixelShaderModule *module);
  const dxil::HashValue &getVertexShaderHash(ShaderID id) const;
  void setVertexShaderName(ShaderID id, eastl::span<const char> name);
  const dxil::HashValue &getPixelShaderHash(ShaderID id) const;
  void setPixelShaderName(ShaderID id, eastl::span<const char> name);
  VertexShaderModuleRefStore getVertexShader(ShaderID id);
  PixelShaderModuleRefStore getPixelShader(ShaderID id);

  eastl::span<uint8_t> getBytecode(const PixelShaderModuleRefStore &ref)
  {
    return eastl::visit([this](auto &r) -> eastl::span<uint8_t> { return this->getBytecode(r); }, ref.bytecode);
  }

  eastl::span<uint8_t> getVsBytecode(const VertexShaderModuleRefStore &ref)
  {
    return eastl::visit([this, &ref](auto &r) -> eastl::span<uint8_t> { return this->getVsBytecode(ref.header, r); }, ref.bytecode);
  }

  eastl::span<uint8_t> getMsBytecode(const VertexShaderModuleRefStore &ref)
  {
    return eastl::visit([this, &ref](auto &r) -> eastl::span<uint8_t> { return this->getMsBytecode(ref.header, r); }, ref.bytecode);
  }

  eastl::span<uint8_t> getGsBytecode(const VertexShaderModuleRefStore &ref)
  {
    return eastl::visit([this, &ref](auto &r) -> eastl::span<uint8_t> { return this->getGsBytecode(ref.header, r); }, ref.bytecode);
  }

  eastl::span<uint8_t> getAsBytecode(const VertexShaderModuleRefStore &ref)
  {
    return eastl::visit([this, &ref](auto &r) -> eastl::span<uint8_t> { return this->getAsBytecode(ref.header, r); }, ref.bytecode);
  }

  eastl::span<uint8_t> getHsBytecode(const VertexShaderModuleRefStore &ref)
  {
    return eastl::visit([this, &ref](auto &r) -> eastl::span<uint8_t> { return this->getHsBytecode(ref.header, r); }, ref.bytecode);
  }

  eastl::span<uint8_t> getDsBytecode(const VertexShaderModuleRefStore &ref)
  {
    return eastl::visit([this, &ref](auto &r) -> eastl::span<uint8_t> { return this->getDsBytecode(ref.header, r); }, ref.bytecode);
  }

  using AnyShaderModuleUniquePointer =
    eastl::variant<eastl::unique_ptr<GroupZeroVertexShaderModule>, eastl::unique_ptr<GroupVertexShaderModule>,
      eastl::unique_ptr<GroupZeroPixelShaderModule>, eastl::unique_ptr<GroupPixelShaderModule>>;

  AnyShaderModuleUniquePointer releaseVertexShader(ShaderID id);
  AnyShaderModuleUniquePointer releasePixelShader(ShaderID id);
  void resetDumpOfGroup(uint32_t group_index);
  void reserveVertexShaderRange(uint32_t group_index, uint32_t count);
  void setVertexShaderCompressionGroup(uint32_t group_index, uint32_t index, const dxil::HashValue &hash, uint32_t compression_group,
    uint32_t compression_index);
  void setPixelShaderCompressionGroup(uint32_t group_index, uint32_t index, const dxil::HashValue &hash, uint32_t compression_group,
    uint32_t compression_index);
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
