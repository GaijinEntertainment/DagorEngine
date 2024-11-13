// Copyright (C) Gaijin Games KFT.  All rights reserved.

// clang-format off
#include "asmShaderDXIL.h"
#include "../encodedBits.h"

#include <dxil/compiler.h>
#include <drv/shadersMetaData/dxil/utility.h>

#include <generic/dag_smallTab.h>

#include <perfMon/dag_autoFuncProf.h>

#include <ioSys/dag_zstdIo.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_unicode.h>

#include <supp/dag_comPtr.h>
#include <D3Dcompiler.h>

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/sort.h>

#include <mutex>

// clang-format on

// currently turned off, breaks phase one caches for some still unknown reason
#define ENABLE_PHASE_ONE_SOURCE_COMPRESSION 0

struct LibraryHandler
{
  typedef HMODULE pointer;
  void operator()(pointer ptr)
  {
    if (ptr)
    {
      FreeLibrary(ptr);
    }
  }
};

struct LibPin
{
  std::once_flag guard;
  eastl::unique_ptr<HMODULE, LibraryHandler> lib;
  void pin(const char *name)
  {
    call_once(guard, [this, name]() { lib.reset(LoadLibraryA(name)); });
  }
  void unpin() { lib.reset(); }
  HMODULE get() const { return lib.get(); }
};

LibPin pinDxcLib;
LibPin pinFxcLib;

namespace
{

eastl::vector<uint8_t> create_shader_container(eastl::vector<uint8_t> &&data, dxil::StoredShaderType type)
{
  bindump::MemoryWriter writer;
  bindump::Master<dxil::ShaderContainer> container;
  container.type = type;
  container.dataHash = dxil::HashValue::calculate(data.data(), data.size());
  container.data = eastl::move(data);

  bindump::streamWrite(container, writer);
  writer.mData.resize((writer.mData.size() + 3) & ~3);
  return writer.mData;
}

eastl::vector<uint8_t> pack_shader(const eastl::vector<uint8_t> &dxil_blob, const eastl::vector<char> &shader_source,
  const ::dxil::ShaderHeader &header)
{
  bindump::Master<dxil::Shader> shader;
  shader.shaderHeader = header;

  if (!dxil_blob.empty())
  {
    shader.bytecode = dxil_blob;
  }
  if (!shader_source.empty())
  {
    int level = 18;
#if ENABLE_PHASE_ONE_SOURCE_COMPRESSION
    // we compress the source to keep memory usage low, in some situations instances of the compiler can use up more than 1GB
    // and with many instances this can lead to high system memory pressure, also with disk cache we want to keep cache objects
    // for the first phase small.
    level = -1;
#endif
    bindump::VecHolder<char> source;
    source = shader_source;
    shader.shaderSource.compress(source, level);
  }
  bindump::MemoryWriter writer;
  bindump::streamWrite(shader, writer);

  return create_shader_container(eastl::move(writer.mData), dxil::StoredShaderType::singleShader);
}

struct DecodedShaderContainer
{
  bindump::Master<dxil::Shader> shader;
  eastl::vector<uint8_t> decompressedBuffer;
  eastl::string_view source;
};

DecodedShaderContainer decode_shader_container(dag::ConstSpan<unsigned> container, bool decompress_source)
{
  DecodedShaderContainer result;
  if (container.empty())
    return result;

  auto *shaderContainer = bindump::map<dxil::ShaderContainer>((const uint8_t *)container.data());
  G_ASSERT(shaderContainer && shaderContainer->type == dxil::StoredShaderType::singleShader);

  bindump::MemoryReader reader(shaderContainer->data.data(), shaderContainer->data.size());
  bindump::streamRead(result.shader, reader);
  if (decompress_source)
  {
    auto *mapped = bindump::map<dxil::Shader>(shaderContainer->data.data());
    if (mapped)
    {
      auto *decompressedSource =
        mapped->shaderSource.decompress(result.decompressedBuffer, bindump::BehaviorOnUncompressed::Reference);
      result.source = eastl::string_view(decompressedSource->data(), decompressedSource->size());
    }
  }
  return result;
}

DecodedShaderContainer get_null_shader_container(::dxil::ShaderHeader &default_header, bool hlsl_2021, bool enable_fp16,
  bool skip_validation, bool optimize, bool debug_info, bool scarlett_w32, char *store, size_t store_size)
{
  DecodedShaderContainer result;
  result.shader.shaderHeader = default_header;
  static char encodedSource[] = "000000ps_5_0main\0void main() {}\n";
  if (store_size >= sizeof(encodedSource))
  {
    memcpy(store, encodedSource, sizeof(encodedSource));
    // patch to the same flag set as vs
    encodedSource[encoded_bits::SkipValidation] = '0' + skip_validation;
    encodedSource[encoded_bits::Optimize] = '0' + optimize;
    encodedSource[encoded_bits::DebugInfo] = '0' + debug_info;
    encodedSource[encoded_bits::ScarlettW32] = '0' + scarlett_w32;
    encodedSource[encoded_bits::HLSL2021] = '0' + hlsl_2021;
    encodedSource[encoded_bits::EnableFp16] = '0' + enable_fp16;
    store_size = sizeof(encodedSource);
  }
  else
  {
    memset(store, 0, store_size);
  }
  result.source = eastl::string_view(store, store_size);
  return result;
}

template <typename T>
T skip_space(T from, T to)
{
  return eastl::find_if(from, to, [](auto c) { return fast_isspace(c) == 0; });
}

typedef HRESULT(WINAPI *D3DCompileProc)(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pFileName, CONST D3D_SHADER_MACRO *pDefines,
  ID3DInclude *pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs);

class NewFXCompiler
{
  D3DCompileProc d3dCompile = nullptr;

public:
  NewFXCompiler(LibPin &lib)
  {
    if (lib.lib)
    {
      reinterpret_cast<FARPROC &>(d3dCompile) = GetProcAddress(lib.lib.get(), "D3DCompile");
    }
  }

  HRESULT compile(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName, const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude,
    LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs)
  {
    if (d3dCompile)
    {
      return d3dCompile(pSrcData, SrcDataSize, pSourceName, pDefines, pInclude, pEntrypoint, pTarget, Flags1, Flags2, ppCode,
        ppErrorMsgs);
    }
    return E_NOTIMPL;
  }

  explicit operator bool() const { return nullptr != d3dCompile; }
};

// this is a signature generator for compute
eastl::string generate_root_signature_string(bool has_acceleration_structure, const ::dxil::ShaderResourceUsageTable &header)
{
  struct GeneratorCallback
  {
    eastl::string result;
    void begin() { result += "\""; }
    void end() { result += "\""; }
    void beginFlags() { result += "RootFlags("; }
    void endFlags()
    {
      if (result.back() == '(')
      {
        result += "0";
      }
      result += ")";
    }
    void hasAccelerationStructure()
    {
      if (result.back() != '(')
        result += " | ";
      result += "XBOX_RAYTRACING";
    }
    void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
    {
      result += ", RootConstants(num32BitConstants = ";
      result += eastl::to_string(dwords);
      result += ", b";
      result += eastl::to_string(index);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ")";
    }
    void beginConstantBuffers() {}
    void endConstantBuffers() {}
    void constantBuffer(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      G_UNUSED(linear_index);
      result += ", CBV(b";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ")";
    }
    void beginSamplers() { result += ", DescriptorTable("; }
    void endSamplers() { result += ")"; }
    void sampler(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "Sampler(s";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = 1, offset = ";
      result += eastl::to_string(linear_index);
      result += ")";
    }
    void beginBindlessSamplers() { result += ", DescriptorTable("; }
    void endBindlessSamplers() { result += ")"; }
    void bindlessSamplers(uint32_t space, uint32_t index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "Sampler(s";
      result += eastl::to_string(index);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = unbounded, offset = 0)";
    }
    void beginShaderResourceViews() { result += ", DescriptorTable("; }
    void endShaderResourceViews() { result += ")"; }
    void shaderResourceView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "SRV(t";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = ";
      result += eastl::to_string(descriptor_count);
      result += ", offset = ";
      result += eastl::to_string(linear_index);
      result += ")";
    }
    void beginBindlessShaderResourceViews() { result += ", DescriptorTable("; }
    void endBindlessShaderResourceViews() { result += ")"; }
    void bindlessShaderResourceViews(uint32_t space, uint32_t index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "SRV(t";
      result += eastl::to_string(index);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = unbounded, offset = 0)";
    }
    void beginUnorderedAccessViews() { result += ", DescriptorTable("; }
    void endUnorderedAccessViews() { result += ")"; }
    void unorderedAccessView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "UAV(u";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = ";
      result += eastl::to_string(descriptor_count);
      result += ", offset = ";
      result += eastl::to_string(linear_index);
      result += ")";
    }
  };
  GeneratorCallback generator;
  decode_compute_root_signature(has_acceleration_structure, header, generator);
  return generator.result;
}

eastl::string generate_root_signature_string(bool has_acceleration_structure, uint32_t vertex_shader_input_mask,
  const ::dxil::ShaderResourceUsageTable &vs, const ::dxil::ShaderResourceUsageTable &hs, const ::dxil::ShaderResourceUsageTable &ds,
  const ::dxil::ShaderResourceUsageTable &gs, const ::dxil::ShaderResourceUsageTable &ps)
{
  struct GeneratorCallback
  {
    eastl::string result;
    uint32_t signatureCost = 0;
    const char *currentVisibility = "SHADER_VISIBILITY_ALL";
    struct BindlessInfo
    {
      uint32_t space;
      uint32_t index;

      bool operator==(const BindlessInfo &r) const { return space == r.space && index == r.index; }
      bool operator<(const BindlessInfo &r) const
      {
        if (space < r.space)
          return true;
        if (space > r.space)
          return false;
        return index < r.index;
      }
    };
    eastl::vector<BindlessInfo> bindlessSamplerSpaces;
    size_t bindlessSamplerDescriptorStringPos = eastl::string::npos;
    size_t bindlessSamplerSectionSize = 0;
    size_t bindlessSamplerStageCount = 0;

    eastl::vector<BindlessInfo> bindlessResourceSpaces;
    size_t bindlessResourceDescriptorStringPos = eastl::string::npos;
    size_t bindlessResourceSectionSize = 0;
    size_t bindlessResourceStageCount = 0;
    void begin() { result += "\""; }
    void end() { result += "\""; }
    void beginFlags() { result += "RootFlags("; }
    void endFlags()
    {
      if (result.back() == '(')
      {
        result += "0";
      }
      result += ")";
    }
    void hasVertexInputs()
    {
      if (result.back() != '(')
        result += " | ";
      result += "ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT";
    }
    void hasAccelerationStructure()
    {
      if (result.back() != '(')
        result += " | ";
      result += "XBOX_RAYTRACING";
    }
    void noVertexShaderResources()
    {
      if (result.back() != '(')
        result += " | ";
      result += "DENY_VERTEX_SHADER_ROOT_ACCESS";
    }
    void noPixelShaderResources()
    {
      if (result.back() != '(')
        result += " | ";
      result += "DENY_PIXEL_SHADER_ROOT_ACCESS";
    }
    void noHullShaderResources()
    {
      if (result.back() != '(')
        result += " | ";
      result += "DENY_HULL_SHADER_ROOT_ACCESS";
    }
    void noDomainShaderResources()
    {
      if (result.back() != '(')
        result += " | ";
      result += "DENY_DOMAIN_SHADER_ROOT_ACCESS";
    }
    void noGeometryShaderResources()
    {
      if (result.back() != '(')
        result += " | ";
      result += "DENY_GEOMETRY_SHADER_ROOT_ACCESS";
    }
    void setVisibilityPixelShader() { currentVisibility = "SHADER_VISIBILITY_PIXEL"; }
    void setVisibilityVertexShader() { currentVisibility = "SHADER_VISIBILITY_VERTEX"; }
    void setVisibilityHullShader() { currentVisibility = "SHADER_VISIBILITY_HULL"; }
    void setVisibilityDomainShader() { currentVisibility = "SHADER_VISIBILITY_DOMAIN"; }
    void setVisibilityGeometryShader() { currentVisibility = "SHADER_VISIBILITY_GEOMETRY"; }
    void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
    {
      result += ", RootConstants(num32BitConstants=";
      result += eastl::to_string(dwords);
      result += ", b";
      result += eastl::to_string(index);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void specialConstants(uint32_t space, uint32_t index)
    {
      currentVisibility = "SHADER_VISIBILITY_ALL";
      rootConstantBuffer(space, index, 1);
    }
    void beginConstantBuffers() {}
    void endConstantBuffers() {}
    void constantBuffer(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      G_UNUSED(linear_index);
      result += ", CBV(b";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void beginSamplers() { result += ", DescriptorTable("; }
    void endSamplers()
    {
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void sampler(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "Sampler(s";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = 1, offset = ";
      result += eastl::to_string(linear_index);
      result += ")";
    }
    void beginBindlessSamplers()
    {
      if (bindlessSamplerDescriptorStringPos == eastl::string::npos)
      {
        bindlessSamplerDescriptorStringPos = result.size();
      }
      ++bindlessSamplerStageCount;
    }
    void endBindlessSamplers()
    {
      // we simply rewrite the bindless section
      eastl::string bindlessSamplerDef;
      eastl::sort(eastl::begin(bindlessSamplerSpaces), eastl::end(bindlessSamplerSpaces));
      bindlessSamplerDef += ", DescriptorTable(";
      for (auto &space : bindlessSamplerSpaces)
      {
        bindlessSamplerDef += "Sampler(s";
        bindlessSamplerDef += eastl::to_string(space.index);
        bindlessSamplerDef += ", space = ";
        bindlessSamplerDef += eastl::to_string(space.space);
        bindlessSamplerDef += ", numDescriptors = unbounded, offset = 0), ";
      }
      bindlessSamplerDef += "visibility = ";
      if (1 == bindlessSamplerStageCount)
      {
        bindlessSamplerDef += currentVisibility;
      }
      else
      {
        bindlessSamplerDef += "SHADER_VISIBILITY_ALL";
      }
      bindlessSamplerDef += ")";
      result.replace(bindlessSamplerDescriptorStringPos, bindlessSamplerSectionSize, bindlessSamplerDef);
      bindlessSamplerSectionSize = bindlessSamplerDef.size();
    }
    void bindlessSamplers(uint32_t space, uint32_t index)
    {
      for (auto &knownSpace : bindlessSamplerSpaces)
      {
        if (knownSpace.space == space && knownSpace.index == index)
          return;
      }
      bindlessSamplerSpaces.push_back({space, index});
    }
    void beginShaderResourceViews() { result += ", DescriptorTable("; }
    void endShaderResourceViews()
    {
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void shaderResourceView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "SRV(t";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = ";
      result += eastl::to_string(descriptor_count);
      result += ", offset = ";
      result += eastl::to_string(linear_index);
      result += ")";
    }
    void beginBindlessShaderResourceViews()
    {
      if (bindlessResourceDescriptorStringPos == eastl::string::npos)
      {
        bindlessResourceDescriptorStringPos = result.size();
      }
      ++bindlessResourceStageCount;
    }
    void endBindlessShaderResourceViews()
    {
      // we simply rewrite the bindless section
      eastl::string bindlessResourceDef;
      eastl::sort(eastl::begin(bindlessResourceSpaces), eastl::end(bindlessResourceSpaces));
      bindlessResourceDef += ", DescriptorTable(";
      for (auto &space : bindlessResourceSpaces)
      {
        bindlessResourceDef += "SRV(t";
        bindlessResourceDef += eastl::to_string(space.index);
        bindlessResourceDef += ", space = ";
        bindlessResourceDef += eastl::to_string(space.space);
        bindlessResourceDef += ", numDescriptors = unbounded, offset = 0), ";
      }
      bindlessResourceDef += "visibility = ";
      if (1 == bindlessResourceStageCount)
      {
        bindlessResourceDef += currentVisibility;
      }
      else
      {
        bindlessResourceDef += "SHADER_VISIBILITY_ALL";
      }
      bindlessResourceDef += ")";
      result.replace(bindlessResourceDescriptorStringPos, bindlessResourceSectionSize, bindlessResourceDef);
      bindlessResourceSectionSize = bindlessResourceDef.size();
    }
    void bindlessShaderResourceViews(uint32_t space, uint32_t index)
    {
      for (auto &knownSpace : bindlessResourceSpaces)
      {
        if (knownSpace.space == space && knownSpace.index == index)
          return;
      }
      bindlessResourceSpaces.push_back({space, index});
    }
    void beginUnorderedAccessViews() { result += ", DescriptorTable("; }
    void endUnorderedAccessViews()
    {
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void unorderedAccessView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "UAV(u";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = ";
      result += eastl::to_string(descriptor_count);
      result += ", offset = ";
      result += eastl::to_string(linear_index);
      result += ")";
    }
  };
  GeneratorCallback generator;
  decode_graphics_root_signature(0 != vertex_shader_input_mask, has_acceleration_structure, vs, ps, hs, ds, gs, generator);
  return generator.result;
}

eastl::string generate_mesh_root_signature_string(bool has_acceleration_structure, bool has_amplification_stage,
  const ::dxil::ShaderResourceUsageTable &ms, const ::dxil::ShaderResourceUsageTable &as, const ::dxil::ShaderResourceUsageTable &ps)
{
  struct GeneratorCallback
  {
    eastl::string result;
    uint32_t signatureCost = 0;
    const char *currentVisibility = "SHADER_VISIBILITY_ALL";
    struct BindlessInfo
    {
      uint32_t space;
      uint32_t index;

      bool operator==(const BindlessInfo &r) const { return space == r.space && index == r.index; }
      bool operator<(const BindlessInfo &r) const
      {
        if (space < r.space)
          return true;
        if (space > r.space)
          return false;
        return index < r.index;
      }
    };
    eastl::vector<BindlessInfo> bindlessSamplerSpaces;
    size_t bindlessSamplerDescriptorStringPos = eastl::string::npos;
    size_t bindlessSamplerSectionSize = 0;
    size_t bindlessSamplerStageCount = 0;

    eastl::vector<BindlessInfo> bindlessResourceSpaces;
    size_t bindlessResourceDescriptorStringPos = eastl::string::npos;
    size_t bindlessResourceSectionSize = 0;
    size_t bindlessResourceStageCount = 0;
    void begin() { result += "\""; }
    void end() { result += "\""; }
    void beginFlags() { result += "RootFlags("; }
    void endFlags()
    {
      if (result.back() == '(')
      {
        result += "0";
      }
      result += ")";
    }
    void hasAccelerationStructure()
    {
      if (result.back() != '(')
        result += " | ";
      result += "XBOX_RAYTRACING";
    }
    void hasAmplificationStage()
    {
      if (result.back() != '(')
        result += " | ";
      result += "XBOX_FORCE_MEMORY_BASED_ABI";
    }
    void noMeshShaderResources()
    {
      if (result.back() != '(')
        result += " | ";
      result += "DENY_MESH_SHADER_ROOT_ACCESS";
    }
    void noPixelShaderResources()
    {
      if (result.back() != '(')
        result += " | ";
      result += "DENY_PIXEL_SHADER_ROOT_ACCESS";
    }
    void noAmplificationShaderResources()
    {
      if (result.back() != '(')
        result += " | ";
      result += "DENY_AMPLIFICATION_SHADER_ROOT_ACCESS";
    }
    void setVisibilityPixelShader() { currentVisibility = "SHADER_VISIBILITY_PIXEL"; }
    void setVisibilityMeshShader() { currentVisibility = "SHADER_VISIBILITY_MESH"; }
    void setVisibilityAmplificationShader() { currentVisibility = "SHADER_VISIBILITY_AMPLIFICATION"; }
    void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
    {
      result += ", RootConstants(num32BitConstants=";
      result += eastl::to_string(dwords);
      result += ", b";
      result += eastl::to_string(index);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void specialConstants(uint32_t space, uint32_t index)
    {
      currentVisibility = "SHADER_VISIBILITY_ALL";
      rootConstantBuffer(space, index, 1);
    }
    void beginConstantBuffers() {}
    void endConstantBuffers() {}
    void constantBuffer(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      G_UNUSED(linear_index);
      result += ", CBV(b";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void beginSamplers() { result += ", DescriptorTable("; }
    void endSamplers()
    {
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void sampler(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "Sampler(s";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = 1, offset = ";
      result += eastl::to_string(linear_index);
      result += ")";
    }
    void beginBindlessSamplers()
    {
      if (bindlessSamplerDescriptorStringPos == eastl::string::npos)
      {
        bindlessSamplerDescriptorStringPos = result.size();
      }
      ++bindlessSamplerStageCount;
    }
    void endBindlessSamplers()
    {
      // we simply rewrite the bindless section
      eastl::string bindlessSamplerDef;
      eastl::sort(eastl::begin(bindlessSamplerSpaces), eastl::end(bindlessSamplerSpaces));
      bindlessSamplerDef += ", DescriptorTable(";
      for (auto &space : bindlessSamplerSpaces)
      {
        bindlessSamplerDef += "Sampler(s";
        bindlessSamplerDef += eastl::to_string(space.index);
        bindlessSamplerDef += ", space = ";
        bindlessSamplerDef += eastl::to_string(space.space);
        bindlessSamplerDef += ", numDescriptors = unbounded, offset = 0), ";
      }
      bindlessSamplerDef += "visibility = ";
      if (1 == bindlessSamplerStageCount)
      {
        bindlessSamplerDef += currentVisibility;
      }
      else
      {
        bindlessSamplerDef += "SHADER_VISIBILITY_ALL";
      }
      bindlessSamplerDef += ")";
      result.replace(bindlessSamplerDescriptorStringPos, bindlessSamplerSectionSize, bindlessSamplerDef);
      bindlessSamplerSectionSize = bindlessSamplerDef.size();
    }
    void bindlessSamplers(uint32_t space, uint32_t index)
    {
      for (auto &knownSpace : bindlessSamplerSpaces)
      {
        if (knownSpace.space == space && knownSpace.index == index)
          return;
      }
      bindlessSamplerSpaces.push_back({space, index});
    }
    void beginShaderResourceViews() { result += ", DescriptorTable("; }
    void endShaderResourceViews()
    {
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void shaderResourceView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "SRV(t";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = ";
      result += eastl::to_string(descriptor_count);
      result += ", offset = ";
      result += eastl::to_string(linear_index);
      result += ")";
    }
    void beginBindlessShaderResourceViews()
    {
      if (bindlessResourceDescriptorStringPos == eastl::string::npos)
      {
        bindlessResourceDescriptorStringPos = result.size();
      }
      ++bindlessResourceStageCount;
    }
    void endBindlessShaderResourceViews()
    {
      // we simply rewrite the bindless section
      eastl::string bindlessResourceDef;
      eastl::sort(eastl::begin(bindlessResourceSpaces), eastl::end(bindlessResourceSpaces));
      bindlessResourceDef += ", DescriptorTable(";
      for (auto &space : bindlessResourceSpaces)
      {
        bindlessResourceDef += "SRV(t";
        bindlessResourceDef += eastl::to_string(space.index);
        bindlessResourceDef += ", space = ";
        bindlessResourceDef += eastl::to_string(space.space);
        bindlessResourceDef += ", numDescriptors = unbounded, offset = 0), ";
      }
      bindlessResourceDef += "visibility = ";
      if (1 == bindlessResourceStageCount)
      {
        bindlessResourceDef += currentVisibility;
      }
      else
      {
        bindlessResourceDef += "SHADER_VISIBILITY_ALL";
      }
      bindlessResourceDef += ")";
      result.replace(bindlessResourceDescriptorStringPos, bindlessResourceSectionSize, bindlessResourceDef);
      bindlessResourceSectionSize = bindlessResourceDef.size();
    }
    void bindlessShaderResourceViews(uint32_t space, uint32_t index)
    {
      for (auto &knownSpace : bindlessResourceSpaces)
      {
        if (knownSpace.space == space && knownSpace.index == index)
          return;
      }
      bindlessResourceSpaces.push_back({space, index});
    }
    void beginUnorderedAccessViews() { result += ", DescriptorTable("; }
    void endUnorderedAccessViews()
    {
      result += ", visibility = ";
      result += currentVisibility;
      result += ")";
    }
    void unorderedAccessView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += ", ";
      result += "UAV(u";
      result += eastl::to_string(slot);
      result += ", space = ";
      result += eastl::to_string(space);
      result += ", numDescriptors = ";
      result += eastl::to_string(descriptor_count);
      result += ", offset = ";
      result += eastl::to_string(linear_index);
      result += ")";
    }
  };
  GeneratorCallback generator;
  decode_graphics_mesh_root_signature(has_acceleration_structure, ms, ps, has_amplification_stage ? &as : nullptr, generator);
  return generator.result;
}

struct ShaderCompileResult
{
  eastl::vector<uint8_t> dxil;
  eastl::vector<uint8_t> reflectionData;
  eastl::vector<uint8_t> dxbc;
  eastl::string errorLog;
};

::dxil::ShaderStage get_shader_stage_from_profile(const char *profile)
{
  switch (profile[0])
  {
    case 'v': return ::dxil::ShaderStage::VERTEX;
    case 'c': return ::dxil::ShaderStage::COMPUTE;
    case 'p': return ::dxil::ShaderStage::PIXEL;
    case 'g': return ::dxil::ShaderStage::GEOMETRY; break;
    case 'h': return ::dxil::ShaderStage::HULL; break;
    case 'd': return ::dxil::ShaderStage::DOMAIN; break;
    case 'm': return ::dxil::ShaderStage::MESH;
    case 'a': return ::dxil::ShaderStage::AMPLIFICATION;
    default: return ::dxil::ShaderStage::VERTEX;
  }
}

struct PlatformInfo
{
  const wchar_t *subDirectory;
  const char *dxcName;
  const char *fxcName;
  dxil::DXCVersion dxcVersion;
};

// clang-format off
static const PlatformInfo platform_table[] = //
{
  {L"dxc-dx12/pc",            "dxcompiler.dll",   "d3dCompiler_47_new.dll",  dxil::DXCVersion::PC},
  {L"dxc-dx12/xbox_one",      "dxcompiler_x.dll", nullptr,                   dxil::DXCVersion::XBOX_ONE},
  {L"dxc-dx12/xbox_scarlett", "dxcompiler_xs.dll", nullptr,                  dxil::DXCVersion::XBOX_SCARLETT},
};
// clang-format on

// Adds given directory to user library search directory set and
// restricts library search directories to one user defined ones.
// On destruction it reverts everything it did.
// Requires windows 8 and later
struct LibraryRouter
{
  DLL_DIRECTORY_COOKIE cookie = 0;
  LibraryRouter(const wchar_t *additional)
  {
    cookie = AddDllDirectory(additional);
    // restrict upcoming dll loads to our custom paths
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
  }
  ~LibraryRouter()
  {
    if (cookie)
      RemoveDllDirectory(cookie);
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  }
};

namespace
{

// If this function is in a dll, gets the dll address instead
eastl::wstring get_module_path()
{
  HMODULE hModule = nullptr;
  GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)&get_module_path,
    &hModule);

  eastl::vector<wchar_t> buffer;
  do
  {
    // 0x7FFF should suffice, as it is the limit for extended file path length
    buffer.resize(0x7FFF + buffer.size());
    buffer.resize(GetModuleFileNameW(hModule, buffer.data(), static_cast<DWORD>(buffer.size())));
    // unfortunately there is no other way to see if it was truncated
    // there is also different behavior between win xp and later versions
    // xp always returns ERROR_SUCCESS but does not terminate the string when it truncated the name
    // later return ERROR_INSUFFICIENT_BUFFER but terminate the string
    // as we implicitly require windows 8 and later, we can ignore xp case
  } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
  return {buffer.data(), buffer.data() + buffer.size() - 1};
}

} // namespace

bool is_mesh_profile(const char *profile) { return ('m' == profile[0]) || ('a' == profile[0]); }

ShaderCompileResult compileShader(dag::ConstSpan<char> source, const char *profile, const char *entry, bool hlsl2021, bool enableFp16,
  bool skipValidation, bool optimize, bool debug_info, wchar_t *pdb_dir, ::dx12::dxil::Platform platform,
  eastl::string_view root_signature_def, unsigned phase, bool pipeline_has_ts, bool pipeline_has_gs, bool scarlett_w32,
  bool warnings_as_errors, DebugLevel debug_level, bool embed_source)
{
  ShaderCompileResult result = {};

  const auto &platformInfo = platform_table[static_cast<uint32_t>(platform)];
  auto platformLibPath = get_module_path();
  // result is path to executable, so chop off the file name
  platformLibPath.resize(platformLibPath.rfind(L'\\') + 1);
  platformLibPath += platformInfo.subDirectory;
  LibraryRouter libRouter{platformLibPath.c_str()};
  // pin the lib until program ends
  pinDxcLib.pin(platformInfo.dxcName);
  // having fxc is optional
  if (platformInfo.fxcName)
    pinFxcLib.pin(platformInfo.fxcName);
  ComPtr<ID3DBlob> ppSource;

  ::dxil::DXCSettings compileConfig;
  compileConfig.disableValidation = skipValidation;
  compileConfig.optimizeLevel = optimize ? 3 : 0;
  // debug is stored in *.lld files in the pdb subfolder of the compile bat it was kicked off.
  // Those files act as pdb for shaders, shaders know the name of the file and tools like PIX will
  // pick them up if configured correctly.
  compileConfig.PDBBasePath = pdb_dir;
  if ((debug_level == DebugLevel::FULL_DEBUG_INFO) || (debug_level == DebugLevel::AFTERMATH) || embed_source)
    compileConfig.pdbMode = ::dxil::PDBMode::FULL;
  else if (debug_level == DebugLevel::BASIC)
    compileConfig.pdbMode = ::dxil::PDBMode::SMALL;
  compileConfig.saveHlslToBlob = debug_level == DebugLevel::FULL_DEBUG_INFO || embed_source;

  Tab<char> utf8_buf;
  if (compileConfig.pdbMode != ::dxil::PDBMode::NONE && !compileConfig.PDBBasePath.empty())
    dd_mkdir(convert_to_utf8(utf8_buf, compileConfig.PDBBasePath.data(), compileConfig.PDBBasePath.size()));

  // on xbox we have to compile shaders in two phases:
  // 1) without root signature to be able to query shader header to later generate root signature
  // 2) provide root signature for combination the shader is used with
  compileConfig.xboxPhaseOne = ::dx12::dxil::Platform::PC != platform && 1 == phase;

  compileConfig.autoBindSpace = dxil::REGULAR_RESOURCES_SPACE_INDEX;
  // we turn this one by default for now - have to measure to see if it has _any_ impact though.
  compileConfig.assumeAllResourcesBound = true;
  compileConfig.pipelineIsMesh = is_mesh_profile(profile);
  // TODO should not abuse pipeline_has_gs param with a context sensitive meaning
  compileConfig.pipelineHasAmplification = compileConfig.pipelineIsMesh && pipeline_has_gs;
  compileConfig.pipelineHasGeoemetryStage = !compileConfig.pipelineIsMesh && pipeline_has_gs;
  compileConfig.pipelineHasTesselationStage = pipeline_has_ts;
  compileConfig.scarlettWaveSize32 = scarlett_w32;

  compileConfig.rootSignatureDefine = root_signature_def;
  compileConfig.warningsAsErrors = warnings_as_errors;

  compileConfig.hlsl2021 = hlsl2021;
  compileConfig.enableFp16 = enableFp16;

  {
#if REPLACE_REGISTER_WITH_MACRO
    static const char macro_def[] = "#define REGISTER(name)"
                                    " register(name, AUTO_DX12_REGISTER_SPACE)\n";
    static const char register_key_word[] = "register";
    static const char register_macro[] = "REGISTER";

    eastl::string temp;
    temp.reserve(source.size() + sizeof(macro_def));
    temp.append(macro_def, macro_def + sizeof(macro_def) - 1);
    temp.append(source.data(), source.size());
    // scan for 'register' key word. Then check if the register is c# or not,
    // for c# register DXC will emit an error if it has a space parameter.
    // So for c we skip it and for all others we replace 'register' with
    // the macro 'REGISTER' that includes the space parameter.
    auto at = eastl::search(temp.begin() + sizeof(macro_def), temp.end(), register_key_word,
      register_key_word + sizeof(register_key_word) - 1);
    while (at != temp.end())
    {
      auto nextChar = skip_space(at + sizeof(register_key_word) - 1, temp.end());
      if (nextChar == temp.end())
        break;

      if (*nextChar == '(')
      {
        nextChar = skip_space(nextChar + 1, temp.end());
        if (*nextChar != 'c')
        {
          eastl::copy(register_macro, register_macro + sizeof(register_macro) - 1, at);
        }
      }
      at = eastl::search(at + sizeof(register_key_word) - 1, temp.end(), register_key_word,
        register_key_word + sizeof(register_key_word) - 1);
    }

    const char *text = temp.c_str();
    size_t textLength = temp.length();
#else
    const char *text = source.data();
    size_t textLength = source.size();
#endif

    D3D_SHADER_MACRO preprocess[4] = {{"SHADER_COMPILER_DXC", "1"}, {"__HLSL_VERSION", hlsl2021 ? "2021" : "2018"}, {}};
    int nextSlot = 2;
    if (enableFp16)
    {
      preprocess[nextSlot].Name = "SHADER_COMPILER_FP16_ENABLED";
      preprocess[nextSlot].Definition = "1";
      nextSlot++;
    }

    // Currently we need to preprocess this with old FXC
    // as some stuff relies on its MS macro expansion behavior
    ComPtr<ID3DBlob> error;
    if (FAILED(D3DPreprocess(text, textLength, nullptr, preprocess, nullptr, &ppSource, &error)))
    {
      result.errorLog += "Preprocessor failed because ";
      result.errorLog += reinterpret_cast<const char *>(error->GetBufferPointer());
      return result;
    }
  }

  auto compileResult =
    ::dxil::compileHLSLWithDXC(make_span(reinterpret_cast<const char *>(ppSource->GetBufferPointer()), ppSource->GetBufferSize() - 1),
      entry, profile, compileConfig, pinDxcLib.get(), platformInfo.dxcVersion);

  if (compileResult.byteCode.empty())
  {
    result.errorLog += "Compilation of shader failed because of ";
    result.errorLog += compileResult.logMessage;
    return result;
  }

  if (!compileResult.logMessage.empty())
    result.errorLog += compileResult.logMessage;

  result.dxil = eastl::move(compileResult.byteCode);
  result.reflectionData = eastl::move(compileResult.reflectionData);

  {
    NewFXCompiler fxc{pinFxcLib};
    if (fxc && debug_info)
    {
      char profile51[] = "Xs_5_1";
      profile51[0] = profile[0];
      // is even sometimes stricter than DXC!
      D3D_SHADER_MACRO preprocess[] = {{"SHADER_COMPILER_DXC", "1"}, {}};

      ComPtr<ID3DBlob> dxbc, error;
      if (FAILED(fxc.compile(source.data(), source.size(), "", preprocess, nullptr, entry, profile51, D3DCOMPILE_OPTIMIZATION_LEVEL3,
            0, &dxbc, &error)))
      {
        if (error)
        {
          result.errorLog += "Error while compiling with new FXC version: ";
          result.errorLog += reinterpret_cast<const char *>(error->GetBufferPointer());
        }
        else
        {
          result.errorLog += "Error while compiling with new FXC version, but no error message was "
                             "returned";
        }
      }
      else if (dxbc)
      {
        auto from = reinterpret_cast<const uint8_t *>(dxbc->GetBufferPointer());
        auto to = from + dxbc->GetBufferSize();
        result.dxbc.assign(from, to);
      }
      else
      {
        result.errorLog += "Error while compiling with new FXC version, DXBC blob was empty but it "
                           "reported success";
      }
    }
  }

  return result;
}

eastl::vector<uint8_t> recompile_shader(const char *encoded_shader_source, const ::dxil::ShaderHeader &header,
  const eastl::string &root_signature_def, ::dx12::dxil::Platform platform, bool pipeline_has_ts, bool pipeline_has_gs,
  wchar_t *pdb_dir, DebugLevel debug_level, bool embed_source)
{
  bool skipValidation = encoded_shader_source[encoded_bits::SkipValidation] > '0';
  bool optimize = encoded_shader_source[encoded_bits::Optimize] > '0';
  bool debugInfo = encoded_shader_source[encoded_bits::DebugInfo] > '0';
  bool scarlettW32 = encoded_shader_source[encoded_bits::ScarlettW32] > '0';
  bool hlsl2021 = encoded_shader_source[encoded_bits::HLSL2021] > '0';
  bool enableFp16 = encoded_shader_source[encoded_bits::EnableFp16] > '0';
  char profileBuf[encoded_bits::ExtraBytes] = {};
  memcpy(profileBuf, &encoded_shader_source[encoded_bits::BitCount], encoded_bits::ProfileLength);
  auto entryStart = &encoded_shader_source[encoded_bits::BitCount + encoded_bits::ProfileLength];
  // a null terminator splits entry name from source code string
  auto sourceStart = entryStart + strlen(entryStart) + 1;
  // TODO could avoid strlen when encoded_shader_source would be a slice
  auto compileResult = ::compileShader(make_span(sourceStart, strlen(sourceStart)), profileBuf, entryStart, hlsl2021, enableFp16,
    skipValidation, optimize, debugInfo, pdb_dir, platform, root_signature_def, 2, pipeline_has_ts, pipeline_has_gs, scarlettW32,
    false, debug_level, embed_source);
  if (compileResult.dxil.empty() && compileResult.dxbc.empty())
  {
    debug("recompile_shader failed: %s", compileResult.errorLog.c_str());
    return {};
  }
  return pack_shader(compileResult.dxil, {}, header);
}
} // namespace

#define REPLACE_REGISTER_WITH_MACRO 0

namespace
{
bool has_acceleration_structure(const dxil::ShaderHeader &header)
{
  for (int i = 0; i < dxil::MAX_T_REGISTERS; ++i)
    if (static_cast<D3D_SHADER_INPUT_TYPE>(header.tRegisterTypes[i] & 0xF) == 12 /*D3D_SIT_RTACCELERATIONSTRUCTURE*/)
      return true;
  return false;
}
} // namespace

CompileResult dx12::dxil::compileShader(dag::ConstSpan<char> source, const char *profile, const char *entry, bool need_disasm,
  bool hlsl2021, bool enableFp16, bool skipValidation, bool optimize, bool debug_info, wchar_t *pdb_dir, int max_constants_no,
  const char *name, Platform platform, bool scarlett_w32, bool warnings_as_errors, DebugLevel debug_level, bool embed_source)
{
  auto compileResult = ::compileShader(source, profile, entry, hlsl2021, enableFp16, skipValidation, optimize, debug_info, pdb_dir,
    platform, {}, 1, true, true, scarlett_w32, warnings_as_errors, debug_level, embed_source);

  CompileResult result;
  if (compileResult.dxil.empty())
  {
    result.errors = eastl::move(compileResult.errorLog);
    return result;
  }

  auto stage = get_shader_stage_from_profile(profile);
  auto headerCompileResult =
    ::dxil::compileHeaderFromReflectionData(stage, compileResult.reflectionData, max_constants_no, pinDxcLib.get());

  if (!headerCompileResult.isOk)
  {
    result.errors.sprintf("Compilation of shader header failed because of %s", headerCompileResult.logMessage.c_str());
    return result;
  }

  if ((::dxil::ShaderStage::MESH == stage) || (::dxil::ShaderStage::AMPLIFICATION == stage) || (::dxil::ShaderStage::COMPUTE == stage))
  {
    result.computeShaderInfo.threadGroupSizeX = headerCompileResult.computeShaderInfo.threadGroupSizeX;
    result.computeShaderInfo.threadGroupSizeY = headerCompileResult.computeShaderInfo.threadGroupSizeY;
    result.computeShaderInfo.threadGroupSizeZ = headerCompileResult.computeShaderInfo.threadGroupSizeZ;
#if _CROSS_TARGET_DX12
    result.computeShaderInfo.scarlettWave32 = scarlett_w32;
#endif
  }

  if (need_disasm)
  {
    result.disassembly = ::dxil::disassemble(compileResult.dxil, pinDxcLib.get());
  }

  eastl::vector<char> shaderSource;
  if (::dx12::dxil::Platform::PC != platform)
  {
    // for xbox we either recompile with root signature now compute shaders, or store source
    // and some metadata for later recompile when all shaders used by the shader pass are
    // finished and can be recompiled with combined root signature.
    if (::dxil::ShaderStage::COMPUTE == stage)
    {
      bool hasAccelerationStructure = has_acceleration_structure(headerCompileResult.header);
      // compute is on its own, so we can compile phase two right now
      auto rootSignatureDefine =
        generate_root_signature_string(hasAccelerationStructure, headerCompileResult.header.resourceUsageTable);
      compileResult = ::compileShader(source, profile, entry, hlsl2021, enableFp16, skipValidation, optimize, debug_info, pdb_dir,
        platform, rootSignatureDefine, 2, false, false, scarlett_w32, warnings_as_errors, debug_level, embed_source);
      if (compileResult.dxil.empty())
      {
        result.errors.sprintf("Error while recompiling compute shader with root signature: "
                              "%s",
          compileResult.errorLog.c_str());
        return result;
      }
    }
    else
    {
      // On xbox phase two we also store the source
      auto entryLength = strlen(entry);
      // <compression><skipValidation><optimize><debug_info><scarlett_w32><hlsl2021><enableFp16><profile><entry>\0<source>\0
      shaderSource.reserve(encoded_bits::BitCount + encoded_bits::ExtraBytes + entryLength + source.size() + 1);
      shaderSource.resize(encoded_bits::BitCount);
      shaderSource[encoded_bits::SkipValidation] = '0' + skipValidation; // ends up being either '0' or '1'
      shaderSource[encoded_bits::Optimize] = '0' + optimize;             // ends up being either '0' or '1'
      shaderSource[encoded_bits::DebugInfo] = '0' + debug_info;          // ends up being either '0' or '1'
      shaderSource[encoded_bits::ScarlettW32] = '0' + scarlett_w32;      // ends up being either '0' or '1'
      shaderSource[encoded_bits::HLSL2021] = '0' + hlsl2021;             // ends up being either '0' or '1'
      shaderSource[encoded_bits::EnableFp16] = '0' + enableFp16;         // ends up being either '0' or '1'
      shaderSource.insert(shaderSource.end(), profile, profile + encoded_bits::ProfileLength);
      shaderSource.insert(shaderSource.end(), entry, entry + entryLength);
      // we insert here a null terminator for ease of use later, consumer will know that source will
      // start after that
      shaderSource.push_back('\0');
      shaderSource.insert(shaderSource.end(), source.data(), source.data() + source.size());
      // make sure we are null terminated properly, somewhere inside DXC it is expected to be null
      // terminated, even when it is querying the size of the string...
      shaderSource.push_back('\0');
    }
  }

  result.errors = eastl::move(compileResult.errorLog);
  // only provide name in debug builds
  result.bytecode = pack_shader(compileResult.dxil, shaderSource, headerCompileResult.header);
  return result;
}

void dx12::dxil::combineShaders(SmallTab<unsigned, TmpmemAlloc> &target, dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs,
  dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, unsigned id)
{
  bindump::MemoryWriter writer;
  ::dxil::StoredShaderType type = ::dxil::StoredShaderType::combinedVertexShader;
  auto vertex_shader = decode_shader_container(vs, false).shader;
  if (vertex_shader.shaderHeader.shaderType == (uint16_t)::dxil::ShaderStage::MESH)
  {
    bindump::Master<::dxil::MeshShaderPipeline> layout;
    *layout.meshShader = vertex_shader;
    if (!gs.empty())
    {
      layout.amplificationShader.create();
      *layout.amplificationShader = decode_shader_container(gs, false).shader;
      G_ASSERT(layout.amplificationShader->shaderHeader.shaderType == (uint16_t)::dxil::ShaderStage::AMPLIFICATION);
    }
    G_ASSERT(ds.empty() && hs.empty());
    bindump::streamWrite(layout, writer);
    type = ::dxil::StoredShaderType::meshShader;
  }
  else
  {
    bindump::Master<::dxil::VertexShaderPipeline> layout;
    *layout.vertexShader = vertex_shader;
    if (!hs.empty())
    {
      layout.hullShader.create();
      *layout.hullShader = decode_shader_container(hs, false).shader;
    }
    if (!ds.empty())
    {
      layout.domainShader.create();
      *layout.domainShader = decode_shader_container(ds, false).shader;
    }
    if (!gs.empty())
    {
      layout.geometryShader.create();
      *layout.geometryShader = decode_shader_container(gs, false).shader;
    }
    bindump::streamWrite(layout, writer);
  }

  auto packed = create_shader_container(eastl::move(writer.mData), type);

  const size_t dwordsToFitPacked = (packed.size() - 1) / elem_size(target) + 1;
  target.resize(dwordsToFitPacked + 1); // +1 for id at the beginning

  target[0] = id;
  eastl::copy(packed.begin(), packed.end(), (uint8_t *)&target[1]);
}

dx12::dxil::RootSignatureStore dx12::dxil::generateRootSignatureDefinition(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs,
  dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, dag::ConstSpan<unsigned> ps)
{
  dx12::dxil::RootSignatureStore result{};

  DecodedShaderContainer vsDecoded = decode_shader_container(vs, ps.size() == 0);
  result.hasVertexInput = 0 != vsDecoded.shader.shaderHeader.inOutSemanticMask;
  result.isMesh = ::dxil::ShaderStage::MESH == static_cast<::dxil::ShaderStage>(vsDecoded.shader.shaderHeader.shaderType);
  result.hasAmplificationStage = result.isMesh && (gs.size() > 0);
  result.vsResources = vsDecoded.shader.shaderHeader.resourceUsageTable;
  result.hasAccelerationStructure = has_acceleration_structure(vsDecoded.shader.shaderHeader);

  if (hs.empty() != ds.empty())
  {
    debug("Tessellation stage incomplete");
    return {};
  }

  if (hs.size())
  {
    auto hsDecoded = decode_shader_container(hs, false);
    result.hsResources = hsDecoded.shader.shaderHeader.resourceUsageTable;
    result.hasAccelerationStructure = result.hasAccelerationStructure || has_acceleration_structure(hsDecoded.shader.shaderHeader);
  }

  if (ds.size())
  {
    auto dsDecoded = decode_shader_container(ds, false);
    result.dsResources = dsDecoded.shader.shaderHeader.resourceUsageTable;
    result.hasAccelerationStructure = result.hasAccelerationStructure || has_acceleration_structure(dsDecoded.shader.shaderHeader);
  }

  if (gs.size())
  {
    auto gsDecoded = decode_shader_container(gs, false);
    result.gsResources = gsDecoded.shader.shaderHeader.resourceUsageTable;
    result.hasAccelerationStructure = result.hasAccelerationStructure || has_acceleration_structure(gsDecoded.shader.shaderHeader);
  }

  if (!ps.empty())
  {
    auto psDecoded = decode_shader_container(ps, false);
    result.psResources = psDecoded.shader.shaderHeader.resourceUsageTable;
    result.hasAccelerationStructure = result.hasAccelerationStructure || has_acceleration_structure(psDecoded.shader.shaderHeader);
  }

  return result;
}

constexpr uint32_t COMPILATION_FLAG_SKIP_VALIDATION = 1u << 0;
constexpr uint32_t COMPILATION_FLAG_OPTIMIZE = 1u << 1;
constexpr uint32_t COMPILATION_FLAG_DEBUG_INFO = 1u << 2;
constexpr uint32_t COMPILATION_FLAG_SCARLETT_W32 = 1u << 3;
constexpr uint32_t COMPILATION_FLAG_HAS_GS = 1u << 4;
constexpr uint32_t COMPILATION_FLAG_HAS_HD_DS = 1u << 5;
constexpr uint32_t COMPILATION_FLAG_HLSL2021 = 1u << 6;
constexpr uint32_t COMPILATION_FLAG_ENABLEFP16 = 1u << 7;

BINDUMP_BEGIN_LAYOUT(VertexProgramPhaseOne)
  BINDUMP_USING_EXTENSION()
  uint32_t magic = 0;
  VecHolder<uint32_t> vertexShader;
  VecHolder<uint32_t> hullShader;
  VecHolder<uint32_t> domainShader;
  VecHolder<uint32_t> geomentryShader;
  uint32_t compilationFlags;
  dx12::dxil::RootSignatureStore rootSignature;
BINDUMP_END_LAYOUT()

BINDUMP_BEGIN_LAYOUT(PixelShaderPhaseOne)
  BINDUMP_USING_EXTENSION()
  uint32_t magic = 0;
  VecHolder<uint32_t> pixelShader;
  uint32_t compilationFlags;
  dx12::dxil::RootSignatureStore rootSignature;
BINDUMP_END_LAYOUT()

namespace
{
bool operator==(const dx12::dxil::RootSignatureStore &l, const dx12::dxil::RootSignatureStore &r)
{
  return l.vsResources == r.vsResources && l.hsResources == r.hsResources && l.dsResources == r.dsResources &&
         l.gsResources == r.gsResources && l.psResources == r.psResources && l.hasVertexInput == r.hasVertexInput &&
         l.hasAmplificationStage == r.hasAmplificationStage && l.hasAccelerationStructure == r.hasAccelerationStructure;
}

bool operator!=(const dx12::dxil::RootSignatureStore &l, const dx12::dxil::RootSignatureStore &r) { return !(l == r); }
} // namespace

bool dx12::dxil::comparePhaseOneVertexProgram(dag::ConstSpan<unsigned> combined_vprog, dag::ConstSpan<unsigned> vs,
  dag::ConstSpan<unsigned> hs, dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, const RootSignatureStore &signature)
{
  auto *header = bindump::map<VertexProgramPhaseOne>((const uint8_t *)combined_vprog.data());
  if (!header)
  {
    return false;
  }

  if (header->rootSignature != signature)
  {
    return false;
  }

  // check if presence of optional embedded shaders are matching
  if (hs.empty() != header->hullShader.empty())
  {
    return false;
  }
  if (ds.empty() != header->domainShader.empty())
  {
    return false;
  }
  if (gs.empty() != header->geomentryShader.empty())
  {
    return false;
  }

  if (!hs.empty())
  {
    auto headerHS = decode_shader_container(header->hullShader, false);
    auto decodedHS = decode_shader_container(hs, false);
    if (decodedHS.shader.bytecode.content() != headerHS.shader.bytecode.content())
    {
      return false;
    }
  }
  if (!ds.empty())
  {
    auto headerDS = decode_shader_container(header->domainShader, false);
    auto decodedDS = decode_shader_container(ds, false);
    if (decodedDS.shader.bytecode.content() != headerDS.shader.bytecode.content())
    {
      return false;
    }
  }
  if (!gs.empty())
  {
    auto headerGS = decode_shader_container(header->geomentryShader, false);
    auto decodedGS = decode_shader_container(gs, false);
    if (decodedGS.shader.bytecode.content() != headerGS.shader.bytecode.content())
    {
      return false;
    }
  }

  auto headerVS = decode_shader_container(header->vertexShader, false);
  auto decodedVS = decode_shader_container(vs, false);
  return decodedVS.shader.bytecode.content() == headerVS.shader.bytecode.content();
}

bool dx12::dxil::comparePhaseOnePixelShader(dag::ConstSpan<unsigned> combined_psh, dag::ConstSpan<unsigned> ps,
  const RootSignatureStore &signature)
{
  auto *header = bindump::map<PixelShaderPhaseOne>((const uint8_t *)combined_psh.data());
  if (!header)
  {
    return false;
  }

  if (header->rootSignature != signature)
  {
    return false;
  }

  auto decodedA = decode_shader_container(header->pixelShader, false);
  auto decodedB = decode_shader_container(ps, false);
  return decodedA.shader.bytecode.content() == decodedB.shader.bytecode.content();
}

eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>> dx12::dxil::combinePhaseOneVertexProgram(dag::ConstSpan<unsigned> vs,
  dag::ConstSpan<unsigned> hs, dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, const RootSignatureStore &signature,
  unsigned id, CompilationOptions options)
{
  bindump::Master<VertexProgramPhaseOne> header;
  header.magic = id;
  header.compilationFlags = 0;
  header.compilationFlags |= gs.empty() ? 0 : COMPILATION_FLAG_HAS_GS;
  header.compilationFlags |= (hs.empty() || ds.empty()) ? 0 : COMPILATION_FLAG_HAS_HD_DS;
  header.compilationFlags |= options.optimize ? COMPILATION_FLAG_OPTIMIZE : 0;
  header.compilationFlags |= options.skipValidation ? COMPILATION_FLAG_SKIP_VALIDATION : 0;
  header.compilationFlags |= options.scarlettW32 ? COMPILATION_FLAG_SCARLETT_W32 : 0;
  header.compilationFlags |= options.debugInfo ? COMPILATION_FLAG_DEBUG_INFO : 0;
  header.compilationFlags |= options.hlsl2021 ? COMPILATION_FLAG_HLSL2021 : 0;
  header.compilationFlags |= options.enableFp16 ? COMPILATION_FLAG_ENABLEFP16 : 0;
  header.rootSignature = signature;

  header.vertexShader = vs;
  header.hullShader = hs;
  header.domainShader = ds;
  header.geomentryShader = gs;

  bindump::MemoryWriter writer;
  bindump::streamWrite(header, writer);

  auto target = eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>();
  target->resize((writer.mData.size() + elem_size(*target)) / elem_size(*target));
  eastl::copy(writer.mData.begin(), writer.mData.end(), (uint8_t *)target->data());
  return target;
}

eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>> dx12::dxil::combinePhaseOnePixelShader(dag::ConstSpan<unsigned> ps,
  const RootSignatureStore &signature, unsigned id, bool has_gs, bool has_ts, CompilationOptions options)
{
  bindump::Master<PixelShaderPhaseOne> header;
  header.magic = id;
  header.compilationFlags = 0;
  header.compilationFlags |= has_gs ? COMPILATION_FLAG_HAS_GS : 0;
  header.compilationFlags |= has_ts ? COMPILATION_FLAG_HAS_HD_DS : 0;
  header.compilationFlags |= options.optimize ? COMPILATION_FLAG_OPTIMIZE : 0;
  header.compilationFlags |= options.skipValidation ? COMPILATION_FLAG_SKIP_VALIDATION : 0;
  header.compilationFlags |= options.scarlettW32 ? COMPILATION_FLAG_SCARLETT_W32 : 0;
  header.compilationFlags |= options.debugInfo ? COMPILATION_FLAG_DEBUG_INFO : 0;
  header.compilationFlags |= options.hlsl2021 ? COMPILATION_FLAG_HLSL2021 : 0;
  header.compilationFlags |= options.enableFp16 ? COMPILATION_FLAG_ENABLEFP16 : 0;
  header.rootSignature = signature;

  header.pixelShader = ps;

  bindump::MemoryWriter writer;
  bindump::streamWrite(header, writer);

  auto target = eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>();
  target->resize((writer.mData.size() + elem_size(*target)) / elem_size(*target));
  eastl::copy(writer.mData.begin(), writer.mData.end(), (uint8_t *)target->data());
  return target;
}

eastl::optional<eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>>> dx12::dxil::recompileVertexProgram(dag::ConstSpan<unsigned> source,
  Platform platform, wchar_t *pdb_dir, DebugLevel debug_level, bool embed_source)
{
  auto *header = bindump::map<VertexProgramPhaseOne>((const uint8_t *)source.data());
  if (!header)
  {
    return eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>>();
  }

#if REPORT_PERFORMANCE_METRICS
  AutoFuncProfT<AFP_MSEC> funcProfiler("dx12::dxil::recompileVertexProgram: took %dms");
#endif

  eastl::string progSignature;
  if (header->rootSignature.isMesh)
  {
    progSignature =
      generate_mesh_root_signature_string(header->rootSignature.hasAccelerationStructure, header->rootSignature.hasAmplificationStage,
        header->rootSignature.vsResources, header->rootSignature.gsResources, header->rootSignature.psResources);
  }
  else
  {
    progSignature = generate_root_signature_string(header->rootSignature.hasAccelerationStructure,
      header->rootSignature.hasVertexInput, header->rootSignature.vsResources, header->rootSignature.hsResources,
      header->rootSignature.dsResources, header->rootSignature.gsResources, header->rootSignature.psResources);
  }

  auto vsDecoded = decode_shader_container(header->vertexShader, true);
  auto hsDecoded = decode_shader_container(header->hullShader, true);
  auto dsDecoded = decode_shader_container(header->domainShader, true);
  auto gsDecoded = decode_shader_container(header->geomentryShader, true);

  auto rebuildVS = recompile_shader(vsDecoded.source.data(), vsDecoded.shader.shaderHeader, progSignature, platform,
    hsDecoded.source.size() && dsDecoded.source.size(), gsDecoded.source.size(), pdb_dir, debug_level, embed_source);

  if (rebuildVS.empty())
  {
    return eastl::nullopt;
  }

  eastl::vector<uint8_t> rebuildHS;
  if (hsDecoded.source.size())
  {
    rebuildHS = recompile_shader(hsDecoded.source.data(), hsDecoded.shader.shaderHeader, progSignature, platform, true,
      gsDecoded.source.size(), pdb_dir, debug_level, embed_source);
    if (rebuildHS.empty())
    {
      return eastl::nullopt;
    }
  }

  eastl::vector<uint8_t> rebuildDS;
  if (dsDecoded.source.size())
  {
    rebuildDS = recompile_shader(dsDecoded.source.data(), dsDecoded.shader.shaderHeader, progSignature, platform, true,
      gsDecoded.source.size(), pdb_dir, debug_level, embed_source);
    if (rebuildDS.empty())
    {
      return eastl::nullopt;
    }
  }

  eastl::vector<uint8_t> rebuildGS;
  if (gsDecoded.source.size())
  {
    rebuildGS = recompile_shader(gsDecoded.source.data(), gsDecoded.shader.shaderHeader, progSignature, platform,
      hsDecoded.source.size() && dsDecoded.source.size(), true, pdb_dir, debug_level, embed_source);
    if (rebuildGS.empty())
    {
      return eastl::nullopt;
    }
  }

  auto vsSlice = make_span(reinterpret_cast<unsigned *>(rebuildVS.data()), rebuildVS.size() / sizeof(unsigned));
  auto newProg = eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>();
  if (!rebuildHS.empty() || !rebuildDS.empty() || !rebuildGS.empty())
  {
    dag::ConstSpan<unsigned> hsSlice, dsSlice, gsSlice;
    if (!rebuildHS.empty())
      hsSlice.set(reinterpret_cast<unsigned *>(rebuildHS.data()), rebuildHS.size() / sizeof(unsigned));
    if (!rebuildDS.empty())
      dsSlice.set(reinterpret_cast<unsigned *>(rebuildDS.data()), rebuildDS.size() / sizeof(unsigned));
    if (!rebuildGS.empty())
      gsSlice.set(reinterpret_cast<unsigned *>(rebuildGS.data()), rebuildGS.size() / sizeof(unsigned));

    combineShaders(*newProg, vsSlice, hsSlice, dsSlice, gsSlice, header->magic);
  }
  else
  {
    clear_and_resize(*newProg, vsSlice.size() + 1);
    (*newProg)[0] = header->magic;
    mem_copy_from(make_span(*newProg).subspan(1), vsSlice.data());
  }

  return newProg;
}

eastl::optional<eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>>> dx12::dxil::recompilePixelSader(dag::ConstSpan<unsigned> source,
  Platform platform, wchar_t *pdb_dir, DebugLevel debug_level, bool embed_source)
{
  auto *header = bindump::map<PixelShaderPhaseOne>((const uint8_t *)source.data());
  if (!header)
  {
    return eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>>{};
  }

#if REPORT_PERFORMANCE_METRICS
  AutoFuncProfT<AFP_MSEC> funcProfiler("dx12::dxil::recompilePixelSader: took %dms");
#endif

  eastl::string progSignature;
  if (header->rootSignature.isMesh)
  {
    progSignature =
      generate_mesh_root_signature_string(header->rootSignature.hasAccelerationStructure, header->rootSignature.hasAmplificationStage,
        header->rootSignature.vsResources, header->rootSignature.gsResources, header->rootSignature.psResources);
  }
  else
  {
    progSignature = generate_root_signature_string(header->rootSignature.hasAccelerationStructure,
      header->rootSignature.hasVertexInput, header->rootSignature.vsResources, header->rootSignature.hsResources,
      header->rootSignature.dsResources, header->rootSignature.gsResources, header->rootSignature.psResources);
  }

  DecodedShaderContainer psDecoded;
  // null shader check, we only store signature
  char nullShaderStore[64];
  ::dxil::ShaderHeader defaultShaderHeader = {};
  if (header->pixelShader.empty())
  {
    bool skipValidation = 0 != (header->compilationFlags & COMPILATION_FLAG_SKIP_VALIDATION);
    bool optimize = 0 != (header->compilationFlags & COMPILATION_FLAG_OPTIMIZE);
    bool debugInfo = 0 != (header->compilationFlags & COMPILATION_FLAG_DEBUG_INFO);
    bool scarlettW32 = 0 != (header->compilationFlags & COMPILATION_FLAG_SCARLETT_W32);
    bool hlsl2021 = 0 != (header->compilationFlags & COMPILATION_FLAG_HLSL2021);
    bool enableFp16 = 0 != (header->compilationFlags & COMPILATION_FLAG_ENABLEFP16);
    psDecoded = get_null_shader_container(defaultShaderHeader, hlsl2021, enableFp16, skipValidation, optimize, debugInfo, scarlettW32,
      nullShaderStore, sizeof(nullShaderStore));
  }
  else
  {
    psDecoded = decode_shader_container(header->pixelShader, true);
  }

  bool hasTS = 0 != (header->compilationFlags & COMPILATION_FLAG_HAS_HD_DS);
  bool hasGS = 0 != (header->compilationFlags & COMPILATION_FLAG_HAS_GS);
  auto rebuildPS = recompile_shader(psDecoded.source.data(), psDecoded.shader.shaderHeader, progSignature, platform, hasTS, hasGS,
    pdb_dir, debug_level, embed_source);

  if (rebuildPS.empty())
  {
    return eastl::nullopt;
  }

  auto psSlice = make_span(reinterpret_cast<unsigned *>(rebuildPS.data()), rebuildPS.size() / sizeof(unsigned));
  auto newProg = eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>();
  clear_and_resize(*newProg, psSlice.size() + 1);
  (*newProg)[0] = header->magic;
  mem_copy_from(make_span(*newProg).subspan(1), psSlice.data());

  return newProg;
}
