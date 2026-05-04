// Copyright (C) Gaijin Games KFT.  All rights reserved.

// clang-format off
#include "asmShaderDXIL.h"
#include "../encodedBits.h"

#include <dxil/compiler.h>
#include <drv/shadersMetaData/dxil/utility.h>

#include <generic/dag_smallTab.h>
#include <generic/dag_align.h>

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

eastl::vector<uint8_t> create_shader_container(eastl::vector<uint8_t> &&data, dxil::ShaderContainerType type)
{
  bindump::ReservingMemoryWriter writer;
  bindump::Master<dxil::ShaderContainer> container;
  container.type = type;
  container.dataHash = dxil::HashValue::calculate(data.data(), data.size());
  container.data = eastl::move(data);

  bindump::streamWrite(container, writer);
  writer.mData.resize((writer.mData.size() + 3) & ~3);
  return eastl::move(writer.mData);
}

eastl::vector<uint8_t> create_shader_with_stream_output(eastl::vector<uint8_t> &&data,
  dag::ConstSpan<dxil::StreamOutputComponentInfo> stream_output_components)
{
  bindump::ReservingMemoryWriter writer;
  bindump::Master<dxil::ShaderWithStreamOutput> shader;
  shader.data = eastl::move(data);
  shader.streamOutputComponents = stream_output_components;

  bindump::streamWrite(shader, writer);
  return eastl::move(writer.mData);
}

bindump::Master<dxil::Shader> pack_shader_layout(const eastl::vector<uint8_t> &dxil_blob, const eastl::vector<char> &shader_source,
  const ::dxil::ShaderHeader &header)
{
  bindump::Master<dxil::Shader> shader;
  shader.shaderHeader = header;

  shader.bytecodeOffset = 0;
  shader.bytecodeSize = dxil_blob.size();

  if (!shader_source.empty())
  {
    int level = 18;
#if ENABLE_PHASE_ONE_SOURCE_COMPRESSION
    // we compress the source to keep memory usage low, in some situations instances of the compiler can use up more than 1GB
    // and with many instances this can lead to high system memory pressure, also with disk cache we want to keep cache objects
    // for the first phase small.
    level = -1;
#endif
    bindump::VecHolder<char> source = shader_source;
    shader.shaderSource.compress(source, level);
  }
  return shader;
}

eastl::vector<uint8_t> pack_shader_without_stream_output(const eastl::vector<uint8_t> &dxil_blob,
  const eastl::vector<char> &shader_source, const ::dxil::ShaderHeader &header)
{
  bindump::Master<dxil::Shader> shader = pack_shader_layout(dxil_blob, shader_source, header);
  bindump::ReservingMemoryWriter writer;
  bindump::streamWrite(shader, writer);
  return create_shader_container(eastl::move(writer.mData), {dxil::StoredShaderType::singleShader, false});
}

eastl::vector<uint8_t> pack_shader_with_stream_output(const eastl::vector<uint8_t> &dxil_blob,
  const eastl::vector<char> &shader_source, const ::dxil::ShaderHeader &header,
  dag::ConstSpan<dxil::StreamOutputComponentInfo> stream_output_components)
{
  bindump::Master<dxil::Shader> shader = pack_shader_layout(dxil_blob, shader_source, header);
  bindump::ReservingMemoryWriter writer;
  bindump::streamWrite(shader, writer);

  auto shaderWithStreamOutput = create_shader_with_stream_output(eastl::move(writer.mData), stream_output_components);
  return create_shader_container(eastl::move(shaderWithStreamOutput), {dxil::StoredShaderType::singleShader, true});
}

eastl::vector<uint8_t> pack_shader(const eastl::vector<uint8_t> &dxil_blob, const eastl::vector<char> &shader_source,
  const ::dxil::ShaderHeader &header, dag::ConstSpan<dxil::StreamOutputComponentInfo> stream_output_components)
{
  if (stream_output_components.empty())
    return pack_shader_without_stream_output(dxil_blob, shader_source, header);
  return pack_shader_with_stream_output(dxil_blob, shader_source, header, stream_output_components);
}

struct DecodedShaderContainer
{
  bindump::Master<dxil::Shader> shader;
  eastl::vector<uint8_t> decompressedBuffer;
  eastl::string_view source;
  dag::ConstSpan<dxil::StreamOutputComponentInfo> streamOutputComponents;
};

dag::ConstSpan<uint8_t> extract_stream_output_desc_and_get_shader(const bindump::Mapper<dxil::ShaderContainer> *container,
  dag::ConstSpan<dxil::StreamOutputComponentInfo> &targetSo)
{
  if (!container->type.hasStreamOutput)
    return container->data;

  auto shaderWithStreamOutput = bindump::map<dxil::ShaderWithStreamOutput>(container->data.data());
  targetSo = shaderWithStreamOutput->streamOutputComponents;
  return shaderWithStreamOutput->data;
}

DecodedShaderContainer decode_shader_container(dag::ConstSpan<uint8_t> container, bool decompress_source)
{
  DecodedShaderContainer result;
  if (container.empty())
    return result;

  auto *shaderContainer = bindump::map<dxil::ShaderContainer>(container.data());
  G_ASSERT(shaderContainer);
  G_ASSERT(shaderContainer->type.shaderType == dxil::StoredShaderType::singleShader);

  auto shaderData = extract_stream_output_desc_and_get_shader(shaderContainer, result.streamOutputComponents);

  bindump::MemoryReader reader(shaderData.data(), shaderData.size());
  bindump::streamRead(result.shader, reader);
  if (decompress_source)
  {
    auto *mapped = bindump::map<dxil::Shader>(shaderData.data());
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
eastl::wstring generate_root_signature_string(dxil::ComputeRootSignatureExtraProperties properties,
  const ::dxil::ShaderResourceUsageTable &header)
{
  struct GeneratorCallback
  {
    eastl::wstring result;
    void begin() { result += L"\""; }
    void end() { result += L"\""; }
    void beginFlags() { result += L"RootFlags("; }
    void endFlags()
    {
      if (result.back() == L'(')
      {
        result += L"0";
      }
      result += L")";
    }
    void hasAccelerationStructure()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"XBOX_RAYTRACING";
    }
    void specialConstants(uint32_t space, uint32_t index) { rootConstantBuffer(space, index, 1); }
    // no extensions on GDK
    void nvidiaExtension(uint32_t, uint32_t) {}
    void amdExtension(uint32_t, uint32_t) {}
    void useResourceDescriptorHeapIndexing()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED";
    }
    void useSamplerDescriptorHeapIndexing()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"SAMPLER_HEAP_DIRECTLY_INDEXED";
    }
    void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
    {
      result += L", RootConstants(num32BitConstants = ";
      result += eastl::to_wstring(dwords);
      result += L", b";
      result += eastl::to_wstring(index);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L")";
    }
    void beginConstantBuffers() {}
    void endConstantBuffers() {}
    void constantBuffer(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      G_UNUSED(linear_index);
      result += L", CBV(b";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L")";
    }
    void beginSamplers() { result += L", DescriptorTable("; }
    void endSamplers() { result += L")"; }
    void sampler(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"Sampler(s";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = 1, offset = ";
      result += eastl::to_wstring(linear_index);
      result += L")";
    }
    void beginBindlessSamplers() { result += L", DescriptorTable("; }
    void endBindlessSamplers() { result += L")"; }
    void bindlessSamplers(uint32_t space, uint32_t index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"Sampler(s";
      result += eastl::to_wstring(index);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = unbounded, offset = 0)";
    }
    void beginShaderResourceViews() { result += L", DescriptorTable("; }
    void endShaderResourceViews() { result += L")"; }
    void shaderResourceView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"SRV(t";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = ";
      result += eastl::to_wstring(descriptor_count);
      result += L", offset = ";
      result += eastl::to_wstring(linear_index);
      result += L")";
    }
    void beginBindlessShaderResourceViews() { result += L", DescriptorTable("; }
    void endBindlessShaderResourceViews() { result += L")"; }
    void bindlessShaderResourceViews(uint32_t space, uint32_t index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"SRV(t";
      result += eastl::to_wstring(index);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = unbounded, offset = 0)";
    }
    void beginUnorderedAccessViews() { result += L", DescriptorTable("; }
    void endUnorderedAccessViews() { result += L")"; }
    void unorderedAccessView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"UAV(u";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = ";
      result += eastl::to_wstring(descriptor_count);
      result += L", offset = ";
      result += eastl::to_wstring(linear_index);
      result += L")";
    }
  };
  GeneratorCallback generator;
  decode_compute_root_signature(properties, header, generator);
  return generator.result;
}

eastl::wstring generate_root_signature_string(dxil::GraphicsRootSignatureExtraProperties properties,
  const ::dxil::ShaderResourceUsageTable &vs, const ::dxil::ShaderResourceUsageTable &hs, const ::dxil::ShaderResourceUsageTable &ds,
  const ::dxil::ShaderResourceUsageTable &gs, const ::dxil::ShaderResourceUsageTable &ps)
{
  struct GeneratorCallback
  {
    eastl::wstring result;
    uint32_t signatureCost = 0;
    const wchar_t *currentVisibility = L"SHADER_VISIBILITY_ALL";
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
    void begin() { result += L"\""; }
    void end() { result += L"\""; }
    void beginFlags() { result += L"RootFlags("; }
    void endFlags()
    {
      if (result.back() == L'(')
      {
        result += L"0";
      }
      result += L")";
    }
    void hasVertexInputs()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT";
    }
    void hasAccelerationStructure()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"XBOX_RAYTRACING";
    }
    void noVertexShaderResources()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"DENY_VERTEX_SHADER_ROOT_ACCESS";
    }
    void noPixelShaderResources()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"DENY_PIXEL_SHADER_ROOT_ACCESS";
    }
    void noHullShaderResources()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"DENY_HULL_SHADER_ROOT_ACCESS";
    }
    void noDomainShaderResources()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"DENY_DOMAIN_SHADER_ROOT_ACCESS";
    }
    void noGeometryShaderResources()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"DENY_GEOMETRY_SHADER_ROOT_ACCESS";
    }
    void hasStreamOutput()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"ALLOW_STREAM_OUTPUT";
    }
    void setVisibilityPixelShader() { currentVisibility = L"SHADER_VISIBILITY_PIXEL"; }
    void setVisibilityVertexShader() { currentVisibility = L"SHADER_VISIBILITY_VERTEX"; }
    void setVisibilityHullShader() { currentVisibility = L"SHADER_VISIBILITY_HULL"; }
    void setVisibilityDomainShader() { currentVisibility = L"SHADER_VISIBILITY_DOMAIN"; }
    void setVisibilityGeometryShader() { currentVisibility = L"SHADER_VISIBILITY_GEOMETRY"; }
    void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
    {
      result += L", RootConstants(num32BitConstants=";
      result += eastl::to_wstring(dwords);
      result += L", b";
      result += eastl::to_wstring(index);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void specialConstants(uint32_t space, uint32_t index)
    {
      currentVisibility = L"SHADER_VISIBILITY_ALL";
      rootConstantBuffer(space, index, 1);
    }
    // no extensions on GDK
    void nvidiaExtension(uint32_t, uint32_t) {}
    void amdExtension(uint32_t, uint32_t) {}
    void useResourceDescriptorHeapIndexing()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED";
    }
    void useSamplerDescriptorHeapIndexing()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"SAMPLER_HEAP_DIRECTLY_INDEXED";
    }
    void beginConstantBuffers() {}
    void endConstantBuffers() {}
    void constantBuffer(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      G_UNUSED(linear_index);
      result += L", CBV(b";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void beginSamplers() { result += L", DescriptorTable("; }
    void endSamplers()
    {
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void sampler(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"Sampler(s";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = 1, offset = ";
      result += eastl::to_wstring(linear_index);
      result += L")";
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
      eastl::wstring bindlessSamplerDef;
      eastl::sort(eastl::begin(bindlessSamplerSpaces), eastl::end(bindlessSamplerSpaces));
      bindlessSamplerDef += L", DescriptorTable(";
      for (auto &space : bindlessSamplerSpaces)
      {
        bindlessSamplerDef += L"Sampler(s";
        bindlessSamplerDef += eastl::to_wstring(space.index);
        bindlessSamplerDef += L", space = ";
        bindlessSamplerDef += eastl::to_wstring(space.space);
        bindlessSamplerDef += L", numDescriptors = unbounded, offset = 0), ";
      }
      bindlessSamplerDef += L"visibility = ";
      if (1 == bindlessSamplerStageCount)
      {
        bindlessSamplerDef += currentVisibility;
      }
      else
      {
        bindlessSamplerDef += L"SHADER_VISIBILITY_ALL";
      }
      bindlessSamplerDef += L")";
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
    void beginShaderResourceViews() { result += L", DescriptorTable("; }
    void endShaderResourceViews()
    {
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void shaderResourceView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"SRV(t";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = ";
      result += eastl::to_wstring(descriptor_count);
      result += L", offset = ";
      result += eastl::to_wstring(linear_index);
      result += L")";
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
      eastl::wstring bindlessResourceDef;
      eastl::sort(eastl::begin(bindlessResourceSpaces), eastl::end(bindlessResourceSpaces));
      bindlessResourceDef += L", DescriptorTable(";
      for (auto &space : bindlessResourceSpaces)
      {
        bindlessResourceDef += L"SRV(t";
        bindlessResourceDef += eastl::to_wstring(space.index);
        bindlessResourceDef += L", space = ";
        bindlessResourceDef += eastl::to_wstring(space.space);
        bindlessResourceDef += L", numDescriptors = unbounded, offset = 0), ";
      }
      bindlessResourceDef += L"visibility = ";
      if (1 == bindlessResourceStageCount)
      {
        bindlessResourceDef += currentVisibility;
      }
      else
      {
        bindlessResourceDef += L"SHADER_VISIBILITY_ALL";
      }
      bindlessResourceDef += L")";
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
    void beginUnorderedAccessViews() { result += L", DescriptorTable("; }
    void endUnorderedAccessViews()
    {
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void unorderedAccessView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"UAV(u";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = ";
      result += eastl::to_wstring(descriptor_count);
      result += L", offset = ";
      result += eastl::to_wstring(linear_index);
      result += L")";
    }
  };
  GeneratorCallback generator;
  decode_graphics_root_signature(properties, vs, ps, hs, ds, gs, generator);
  return generator.result;
}

eastl::wstring generate_mesh_root_signature_string(dxil::GraphicsMeshRootSignatureExtraProperties properties,
  bool has_amplification_stage, const ::dxil::ShaderResourceUsageTable &ms, const ::dxil::ShaderResourceUsageTable &as,
  const ::dxil::ShaderResourceUsageTable &ps)
{
  struct GeneratorCallback
  {
    eastl::wstring result;
    uint32_t signatureCost = 0;
    const wchar_t *currentVisibility = L"SHADER_VISIBILITY_ALL";
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
    void begin() { result += L"\""; }
    void end() { result += L"\""; }
    void beginFlags() { result += L"RootFlags("; }
    void endFlags()
    {
      if (result.back() == L'(')
      {
        result += L"0";
      }
      result += L")";
    }
    void hasAccelerationStructure()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"XBOX_RAYTRACING";
    }
    void hasAmplificationStage()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"XBOX_FORCE_MEMORY_BASED_ABI";
    }
    void noMeshShaderResources()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"DENY_MESH_SHADER_ROOT_ACCESS";
    }
    void noPixelShaderResources()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"DENY_PIXEL_SHADER_ROOT_ACCESS";
    }
    void noAmplificationShaderResources()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"DENY_AMPLIFICATION_SHADER_ROOT_ACCESS";
    }
    void hasStreamOutput()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"ALLOW_STREAM_OUTPUT";
    }
    void setVisibilityPixelShader() { currentVisibility = L"SHADER_VISIBILITY_PIXEL"; }
    void setVisibilityMeshShader() { currentVisibility = L"SHADER_VISIBILITY_MESH"; }
    void setVisibilityAmplificationShader() { currentVisibility = L"SHADER_VISIBILITY_AMPLIFICATION"; }
    void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
    {
      result += L", RootConstants(num32BitConstants=";
      result += eastl::to_wstring(dwords);
      result += L", b";
      result += eastl::to_wstring(index);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void specialConstants(uint32_t space, uint32_t index)
    {
      currentVisibility = L"SHADER_VISIBILITY_ALL";
      rootConstantBuffer(space, index, 1);
    }
    // no extensions on GDK
    void nvidiaExtension(uint32_t, uint32_t) {}
    void amdExtension(uint32_t, uint32_t) {}
    void useResourceDescriptorHeapIndexing()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED";
    }
    void useSamplerDescriptorHeapIndexing()
    {
      if (result.back() != L'(')
        result += L" | ";
      result += L"SAMPLER_HEAP_DIRECTLY_INDEXED";
    }
    void beginConstantBuffers() {}
    void endConstantBuffers() {}
    void constantBuffer(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      G_UNUSED(linear_index);
      result += L", CBV(b";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void beginSamplers() { result += L", DescriptorTable("; }
    void endSamplers()
    {
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void sampler(uint32_t space, uint32_t slot, uint32_t linear_index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"Sampler(s";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = 1, offset = ";
      result += eastl::to_wstring(linear_index);
      result += L")";
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
      eastl::wstring bindlessSamplerDef;
      eastl::sort(eastl::begin(bindlessSamplerSpaces), eastl::end(bindlessSamplerSpaces));
      bindlessSamplerDef += L", DescriptorTable(";
      for (auto &space : bindlessSamplerSpaces)
      {
        bindlessSamplerDef += L"Sampler(s";
        bindlessSamplerDef += eastl::to_wstring(space.index);
        bindlessSamplerDef += L", space = ";
        bindlessSamplerDef += eastl::to_wstring(space.space);
        bindlessSamplerDef += L", numDescriptors = unbounded, offset = 0), ";
      }
      bindlessSamplerDef += L"visibility = ";
      if (1 == bindlessSamplerStageCount)
      {
        bindlessSamplerDef += currentVisibility;
      }
      else
      {
        bindlessSamplerDef += L"SHADER_VISIBILITY_ALL";
      }
      bindlessSamplerDef += L")";
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
    void beginShaderResourceViews() { result += L", DescriptorTable("; }
    void endShaderResourceViews()
    {
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void shaderResourceView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != '(')
        result += L", ";
      result += L"SRV(t";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = ";
      result += eastl::to_wstring(descriptor_count);
      result += L", offset = ";
      result += eastl::to_wstring(linear_index);
      result += L")";
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
      eastl::wstring bindlessResourceDef;
      eastl::sort(eastl::begin(bindlessResourceSpaces), eastl::end(bindlessResourceSpaces));
      bindlessResourceDef += L", DescriptorTable(";
      for (auto &space : bindlessResourceSpaces)
      {
        bindlessResourceDef += L"SRV(t";
        bindlessResourceDef += eastl::to_wstring(space.index);
        bindlessResourceDef += L", space = ";
        bindlessResourceDef += eastl::to_wstring(space.space);
        bindlessResourceDef += L", numDescriptors = unbounded, offset = 0), ";
      }
      bindlessResourceDef += L"visibility = ";
      if (1 == bindlessResourceStageCount)
      {
        bindlessResourceDef += currentVisibility;
      }
      else
      {
        bindlessResourceDef += L"SHADER_VISIBILITY_ALL";
      }
      bindlessResourceDef += L")";
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
    void beginUnorderedAccessViews() { result += L", DescriptorTable("; }
    void endUnorderedAccessViews()
    {
      result += L", visibility = ";
      result += currentVisibility;
      result += L")";
    }
    void unorderedAccessView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
    {
      if (result.back() != L'(')
        result += L", ";
      result += L"UAV(u";
      result += eastl::to_wstring(slot);
      result += L", space = ";
      result += eastl::to_wstring(space);
      result += L", numDescriptors = ";
      result += eastl::to_wstring(descriptor_count);
      result += L", offset = ";
      result += eastl::to_wstring(linear_index);
      result += L")";
    }
  };
  GeneratorCallback generator;
  decode_graphics_mesh_root_signature(properties, ms, ps, has_amplification_stage ? &as : nullptr, generator);
  return generator.result;
}

struct ShaderCompileResult
{
  eastl::vector<uint8_t> dxil;
  eastl::vector<uint8_t> reflectionData;
  eastl::vector<uint8_t> dxbc;
  eastl::string errorLog;
  eastl::string messageLog;
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
    case 'l': return ::dxil::ShaderStage::LIBRARY;
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
  eastl::wstring_view root_signature_def, unsigned phase, bool pipeline_has_ts, bool pipeline_has_gs, bool scarlett_w32,
  bool warnings_as_errors, DebugLevel debug_level, bool embed_source, bool pipeline_has_stream_output)
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
  compileConfig.pipelineHasStreamOutput = pipeline_has_stream_output;
  compileConfig.pipelineHasTesselationStage = pipeline_has_ts;
  compileConfig.scarlettWaveSize32 = scarlett_w32;

  compileConfig.rootSignatureDefine = root_signature_def;
  compileConfig.warningsAsErrors = warnings_as_errors;

  compileConfig.hlsl2021 = hlsl2021;
  compileConfig.enableFp16 = enableFp16;

  dag::Vector<::dxil::DXCDefine> defines;
  defines.reserve(16);

  defines.emplace_back(L"SHADER_COMPILER_DXC", L"1");
  defines.emplace_back(L"__HLSL_VERSION", hlsl2021 ? L"2021" : L"2018");
  defines.emplace_back(L"NV_SHADER_EXTN_SLOT", L"u0");
  defines.emplace_back(L"NV_SHADER_EXTN_REGISTER_SPACE", L"space99");
  defines.emplace_back(L"REGISTER(name, space)", L"register(name,space)");

  if (enableFp16)
    defines.emplace_back(L"SHADER_COMPILER_FP16_ENABLED", L"1");

#if REPLACE_REGISTER_WITH_MACRO
  defines.emplace_back(L"REGISTER(name)", L"register(name,AUTO_DX12_REGISTER_SPACE)");

  static const char register_key_word[] = "register";
  static const char register_macro[] = "REGISTER";

  // scan for 'register' key word. Then check if the register is c# or not,
  // for c# register DXC will emit an error if it has a space parameter.
  // So for c we skip it and for all others we replace 'register' with
  // the macro 'REGISTER' that includes the space parameter.
  auto at = eastl::search(source.begin(), source.end(), register_key_word, register_key_word + sizeof(register_key_word) - 1);
  while (at != source.end())
  {
    auto nextChar = skip_space(at + sizeof(register_key_word) - 1, source.end());
    if (nextChar == source.end())
      break;

    if (*nextChar == '(')
    {
      nextChar = skip_space(nextChar + 1, source.end());
      if (*nextChar != 'c')
      {
        eastl::copy(register_macro, register_macro + sizeof(register_macro) - 1, at);
      }
    }
    at = eastl::search(at + sizeof(register_key_word) - 1, source.end(), register_key_word,
      register_key_word + sizeof(register_key_word) - 1);
  }
#endif

  defines.emplace_back(L"__DSC_INTERNAL_PP_CAT(x, y)", L"x##y");
  defines.emplace_back(L"BINDLESS_REGISTER(type,index)",
    L"register(__DSC_INTERNAL_PP_CAT(type, 0), __DSC_INTERNAL_PP_CAT(space, index))");

  // TODO: this is a workaround for PIX crashing when trying to display non-preprocessed
  // shaders through PDBs. Ideally we don't want to do that, or at least format the preprocessing
  // result to make it look not that awful.

  auto ppResult = ::dxil::preprocessHLSLWithDXC(profile, source, defines, pinDxcLib.get());

  // empty pp result is correct (empty in -> empty out) so lets just
  // fail on any pp error/warning whatsoever
  if (!ppResult.errorLog.empty())
  {
    result.errorLog += "Preprocessing of shader failed because of ";
    result.errorLog += ppResult.errorLog;
    return result;
  }
  else if (!ppResult.messageLog.empty())
  {
    result.messageLog += "Preprocessing log:\n";
    result.messageLog += ppResult.messageLog;
  }

  // To generate a GPU ISA binary we need to provide the root signature to DXC.
  if (!compileConfig.rootSignatureDefine.empty())
    compileConfig.defines.emplace_back(L"_AUTO_GENERATED_ROOT_SIGNATURE", compileConfig.rootSignatureDefine);

  auto compileResult = ::dxil::compileHLSLWithDXC({ppResult.preprocessedSource.data(), intptr_t(ppResult.preprocessedSource.size())},
    entry, profile, compileConfig, pinDxcLib.get(), platformInfo.dxcVersion);

  result.messageLog += compileResult.messageLog;

  if (compileResult.byteCode.empty())
  {
    result.errorLog += "Compilation of shader failed because of ";
    result.errorLog += compileResult.errorLog;
    return result;
  }

  if (!compileResult.errorLog.empty())
    result.errorLog += compileResult.errorLog;

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

eastl::tuple<eastl::vector<uint8_t>, eastl::vector<uint8_t>> recompile_shader(const char *encoded_shader_source,
  const ::dxil::ShaderHeader &header, dag::ConstSpan<dxil::StreamOutputComponentInfo> stream_output_desc,
  const eastl::wstring &root_signature_def, ::dx12::dxil::Platform platform, bool pipeline_has_ts, bool pipeline_has_gs,
  wchar_t *pdb_dir, DebugLevel debug_level, bool embed_source, bool pipeline_has_stream_output)
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
    false, debug_level, embed_source, pipeline_has_stream_output);
  if (compileResult.dxil.empty() && compileResult.dxbc.empty())
  {
    debug("recompile_shader failed: %s", compileResult.errorLog.c_str());
    return {};
  }
  return eastl::make_tuple(pack_shader(compileResult.dxil, {}, header, stream_output_desc), compileResult.dxil);
}

bool has_acceleration_structure(const dxil::ShaderHeader &header)
{
  for (int i = 0; i < dxil::MAX_T_REGISTERS; ++i)
    if (static_cast<D3D_SHADER_INPUT_TYPE>(header.tRegisterTypes[i] & 0xF) == 12 /*D3D_SIT_RTACCELERATIONSTRUCTURE*/)
      return true;
  return false;
}

#ifndef D3D_SHADER_REQUIRES_RESOURCE_DESCRIPTOR_HEAP_INDEXING
#define D3D_SHADER_REQUIRES_RESOURCE_DESCRIPTOR_HEAP_INDEXING 0x02000000
#endif

#ifndef D3D_SHADER_REQUIRES_SAMPLER_DESCRIPTOR_HEAP_INDEXING
#define D3D_SHADER_REQUIRES_SAMPLER_DESCRIPTOR_HEAP_INDEXING 0x04000000
#endif

bool uses_resource_descriptor_heap_indexing(const dxil::ShaderHeader &header)
{
  const uint64_t featureFlags = static_cast<uint64_t>(header.deviceRequirement.shaderFeatureFlagsLow) |
                                (static_cast<uint64_t>(header.deviceRequirement.shaderFeatureFlagsHigh) << 32u);
  return 0 != (D3D_SHADER_REQUIRES_RESOURCE_DESCRIPTOR_HEAP_INDEXING & featureFlags);
}

bool uses_sampler_descriptor_heap_indexing(const dxil::ShaderHeader &header)
{
  const uint64_t featureFlags = static_cast<uint64_t>(header.deviceRequirement.shaderFeatureFlagsLow) |
                                (static_cast<uint64_t>(header.deviceRequirement.shaderFeatureFlagsHigh) << 32u);
  return 0 != (D3D_SHADER_REQUIRES_SAMPLER_DESCRIPTOR_HEAP_INDEXING & featureFlags);
}

CompileResult build_shader_libray_as_shader([[maybe_unused]] const dx12::dxil::CompileInputs &inputs,
  const ShaderCompileResult &compile_result)
{
  auto libHeaderCompileResult = ::dxil::compileLibraryShaderPropertiesFromReflectionData(
    0, [](::dxil::ShaderStage, const eastl::string &) -> ::dxil::FunctionExtraInfo { return {}; }, compile_result.reflectionData,
    pinDxcLib.get());

  CompileResult result;

  result.errors = eastl::move(libHeaderCompileResult.logMessage);

  if (!libHeaderCompileResult.isOk)
  {
    return result;
  }

  uint32_t rayGenCount = 0;
  uint32_t anyHitCount = 0;
  uint32_t closestHitCount = 0;
  uint32_t missCount = 0;
  bool hadError = false;
  const ::dxil::LibraryShaderProperties *rayGenProperties = nullptr;
  const ::dxil::LibraryShaderProperties *anyHitProperties = nullptr;
  const ::dxil::LibraryShaderProperties *closestHitProperties = nullptr;
  const ::dxil::LibraryShaderProperties *missProperties = nullptr;
  for (auto &property : libHeaderCompileResult.properties)
  {
    switch (static_cast<::dxil::ShaderStage>(property.shaderType))
    {
      default:
        result.errors.append_sprintf("Shader library contains unexpected shader <%s> of type %u\n",
          libHeaderCompileResult.names[&property - libHeaderCompileResult.properties.data()].c_str(), property.shaderType);
        hadError = true;
        break;
      case ::dxil::ShaderStage::RAY_GEN:
        ++rayGenCount;
        rayGenProperties = &property;
        break;
      case ::dxil::ShaderStage::ANY_HIT:
        ++anyHitCount;
        anyHitProperties = &property;
        break;
      case ::dxil::ShaderStage::CLOSEST_HIT:
        ++closestHitCount;
        closestHitProperties = &property;
        break;
      case ::dxil::ShaderStage::MISS:
        ++missCount;
        missProperties = &property;
        break;
    }
  }

  if (rayGenCount != 1)
  {
    result.errors.append_sprintf("Found %u ray gen shaders, a library has to have exactly one\n", rayGenCount);
    hadError = true;
  }
  if (anyHitCount > 1)
  {
    result.errors.append_sprintf("Found %u any hit shaders, only up to one is allowed\n", anyHitCount);
    hadError = true;
  }
  if (closestHitCount > 1)
  {
    result.errors.append_sprintf("Found %u closest hit shaders, only up to one is allowed\n", closestHitCount);
    hadError = true;
  }
  if (missCount > 1)
  {
    result.errors.append_sprintf("Found %u miss shaders, only up to one is allowed\n", missCount);
    hadError = true;
  }
  if (hadError)
  {
    return result;
  }

  auto entryWithVendorExt = eastl::find_if(libHeaderCompileResult.properties.begin(), libHeaderCompileResult.properties.end(),
    [](auto &props) { return ::dxil::ExtensionVendor::NoExtensionsUsed != props.deviceRequirement.extensionVendor; });
  if (entryWithVendorExt != libHeaderCompileResult.properties.end())
  {
    auto dxilAsm = ::dxil::disassemble(compile_result.dxil, pinDxcLib.get());
    if (dxilAsm.empty())
    {
      result.errors += "Failed to disassemble DXIL to inspect for used vendor extensions\n";
      return result;
    }

    uint16_t vendorExtensionMask = 0;
    if (::dxil::ExtensionVendor::NVIDIA == entryWithVendorExt->deviceRequirement.extensionVendor)
    {
      auto extensionMask = ::dxil::parse_NVIDIA_extension_use(dxilAsm, result.errors);
      // when the mask is empty then we failed to figure out which op code sets where used, the driver can not correctly handle this
      // and so this is a compilation error
      if (0 == extensionMask)
      {
        result.errors.append_sprintf(
          "Detected use of NVIDIA HLSL extensions, but used extension op codes could not be determined, compiler "
          "may needs to be updated, to properly detect the used extensions\n");
        result.errors.append_sprintf("DXIL:\n%s", dxilAsm.c_str());
        return result;
      }
      vendorExtensionMask = extensionMask;
      libHeaderCompileResult.libInfo.resourceUsageTable.specialConstantsMask |= dxil::SC_NVIDIA_EXTENSION;
    }
    else if (::dxil::ExtensionVendor::AMD == entryWithVendorExt->deviceRequirement.extensionVendor)
    {
      auto extensionMask = ::dxil::parse_AMD_extension_use(dxilAsm, result.errors);
      // when the mask is empty then we failed to figure out which op code sets where used, the driver can not correctly handle this
      // and so this is a compilation error
      if (0 == extensionMask)
      {
        result.errors.append_sprintf(
          "Detected use of AMD HLSL extensions, but used extension op codes could not be determined, compiler may "
          "needs to be updated, to properly detect the used extensions\n");
        result.errors.append_sprintf("DXIL:\n%s", dxilAsm.c_str());
        return result;
      }
      vendorExtensionMask = extensionMask;
      libHeaderCompileResult.libInfo.resourceUsageTable.specialConstantsMask |= dxil::SC_AMD_EXTENSION;
    }
    else
    {
      result.errors.append_sprintf("Missing implementation for a vendor (%u) to handle vendor extensions\n",
        (uint32_t)entryWithVendorExt->deviceRequirement.extensionVendor);
    }

    for (auto &entryProperties : libHeaderCompileResult.properties)
    {
      if (::dxil::ExtensionVendor::NoExtensionsUsed == entryProperties.deviceRequirement.extensionVendor)
      {
        continue;
      }
      if (entryProperties.deviceRequirement.extensionVendor != entryWithVendorExt->deviceRequirement.extensionVendor)
      {
        result.errors.append_sprintf("Uses of different vendor extensions detected, this is unsupported\n");
        return result;
      }
      entryProperties.deviceRequirement.vendorExtensionMask = vendorExtensionMask;
    }
  }

  // This packages a ShaderLibraryContainer container, that is also used for full blown RT libraries, into
  // a ShaderLibraryContainerAsShader wrapping container. This wrapper stores the name table we need.
  // Then this is stored as bytecode in a ShaderContainer of type shaderLibraryContainerAsShader.
  // This layout makes it easier to reuse code and avoid special handling for this kind of container and
  // resulting pipeline object.

  bindump::Master<dxil::ShaderLibraryContainer> dx12LibContainer;
  dx12LibContainer.resourceUsageInfo = libHeaderCompileResult.libInfo;
  dx12LibContainer.shaderProperties = libHeaderCompileResult.properties;
  dx12LibContainer.dxilBinary = compile_result.dxil;
  dx12LibContainer.binaryHash = ShaderHashValue::calculate(compile_result.dxil.data(), compile_result.dxil.size());

  bindump::ReservingMemoryWriter dx12LibContainerWriter;
  bindump::streamWrite(dx12LibContainer, dx12LibContainerWriter);

  bindump::Master<dxil::ShaderLibraryContainerAsShader> shaderLibAsShader;
  shaderLibAsShader.nameTable.resize(libHeaderCompileResult.names.size());
  eastl::copy(eastl::begin(libHeaderCompileResult.names), eastl::end(libHeaderCompileResult.names),
    eastl::begin(shaderLibAsShader.nameTable));
  shaderLibAsShader.libBinary = eastl::move(dx12LibContainerWriter.mData);
  if (anyHitProperties || closestHitProperties)
  {
    shaderLibAsShader.hitGroups.resize(1);
    shaderLibAsShader.hitGroups[0].anyHitIndex =
      anyHitProperties ? (anyHitProperties - libHeaderCompileResult.properties.data()) : ~0u;
    shaderLibAsShader.hitGroups[0].closestHitIndex =
      closestHitProperties ? (closestHitProperties - libHeaderCompileResult.properties.data()) : ~0u;
  }
  if (missProperties)
  {
    shaderLibAsShader.missGroups.resize(1);
    shaderLibAsShader.missGroups[0].missIndex = missProperties - libHeaderCompileResult.properties.data();
  }
  shaderLibAsShader.rayGenGroup.rayGenIndex = rayGenProperties - libHeaderCompileResult.properties.data();

  bindump::ReservingMemoryWriter shaderLibAsShaderWriter;
  bindump::streamWrite(shaderLibAsShader, shaderLibAsShaderWriter);

  ::dxil::ShaderHeader header{
    .maxConstantCount = 0,
    .bonesConstantsUsed = 0,
    .resourceUsageTable = libHeaderCompileResult.libInfo.resourceUsageTable,
    .sRegisterCompareUseMask = libHeaderCompileResult.libInfo.sRegisterCompareUseMask,
    .inOutSemanticMask = 0,
    .shaderType = static_cast<uint8_t>(::dxil::ShaderStage::LIBRARY),
    .inputPrimitive = 0,
    .deviceRequirement = libHeaderCompileResult.properties[0].deviceRequirement,
  };

  eastl::copy(eastl::begin(libHeaderCompileResult.libInfo.tRegisterTypes), eastl::end(libHeaderCompileResult.libInfo.tRegisterTypes),
    eastl::begin(header.tRegisterTypes));
  eastl::copy(eastl::begin(libHeaderCompileResult.libInfo.uRegisterTypes), eastl::end(libHeaderCompileResult.libInfo.uRegisterTypes),
    eastl::begin(header.uRegisterTypes));

  bindump::Master<dxil::Shader> wrappedShader = pack_shader_layout(shaderLibAsShaderWriter.mData, {}, header);

  bindump::ReservingMemoryWriter wrappedShaderWriter;
  bindump::streamWrite(wrappedShader, wrappedShaderWriter);

  result.metadata = create_shader_container(eastl::move(wrappedShaderWriter.mData),
    {
      .shaderType = ::dxil::StoredShaderType::shaderLibraryContainerAsShader,
      .hasStreamOutput = 0,
    });
  result.bytecode = shaderLibAsShaderWriter.mData;
  //  ray gen is always 1 x 1 x 1
  result.computeShaderInfo = {.threadGroupSizeX = 1, .threadGroupSizeY = 1, .threadGroupSizeZ = 1, .scarlettWave32 = false};
  return result;
}
} // namespace
#define REPLACE_REGISTER_WITH_MACRO 0

CompileResult dx12::dxil::compileShader(const CompileInputs &inputs)
{
  auto compileResult = ::compileShader(inputs.source, inputs.profile, inputs.entry, inputs.compilationOptions.hlsl2021,
    inputs.compilationOptions.enableFp16, inputs.compilationOptions.skipValidation, inputs.compilationOptions.optimize,
    inputs.compilationOptions.debugInfo, inputs.PDBDir, inputs.platform, {}, 1, true, true, inputs.compilationOptions.scarlettW32,
    inputs.warningsAsErrors, inputs.debugLevel, inputs.embedSource, !inputs.streamOutputComponents.empty());

  CompileResult result;
  result.logs = eastl::move(compileResult.messageLog);

  if (compileResult.dxil.empty())
  {
    result.errors = eastl::move(compileResult.errorLog);
    return result;
  }

  auto stage = get_shader_stage_from_profile(inputs.profile);
  if (::dxil::ShaderStage::LIBRARY == stage)
  {
    return build_shader_libray_as_shader(inputs, compileResult);
  }
  auto headerCompileResult = ::dxil::compileHeaderFromReflectionData(stage, compileResult.reflectionData, inputs.maxConstantsNo,
    inputs.streamOutputComponents, pinDxcLib.get());

  if (!headerCompileResult.isOk)
  {
    result.errors.append_sprintf("Compilation of shader header failed because of %s", headerCompileResult.logMessage.c_str());
    return result;
  }

  if ((::dxil::ShaderStage::MESH == stage) || (::dxil::ShaderStage::AMPLIFICATION == stage) || (::dxil::ShaderStage::COMPUTE == stage))
  {
    result.computeShaderInfo.threadGroupSizeX = headerCompileResult.computeShaderInfo.threadGroupSizeX;
    result.computeShaderInfo.threadGroupSizeY = headerCompileResult.computeShaderInfo.threadGroupSizeY;
    result.computeShaderInfo.threadGroupSizeZ = headerCompileResult.computeShaderInfo.threadGroupSizeZ;
#if _CROSS_TARGET_DX12
    result.computeShaderInfo.scarlettWave32 = inputs.compilationOptions.scarlettW32;
#endif
  }

  if (::dxil::ExtensionVendor::NoExtensionsUsed != headerCompileResult.header.deviceRequirement.extensionVendor)
  {
    auto dxilAsm = ::dxil::disassemble(compileResult.dxil, pinDxcLib.get());
    if (dxilAsm.empty())
    {
      result.errors.append_sprintf("Failed to disassemble DXIL to inspect for used vendor extensions");
      return result;
    }

    if (::dxil::ExtensionVendor::NVIDIA == headerCompileResult.header.deviceRequirement.extensionVendor)
    {
      auto extensionMask = ::dxil::parse_NVIDIA_extension_use(dxilAsm, result.errors);
      // when the mask is empty then we failed to figure out which op code sets where used, the driver can not correctly handle this
      // and so this is a compilation error
      if (0 == extensionMask)
      {
        result.errors.append_sprintf(
          "Detected use of NVIDIA HLSL extensions, but used extension op codes could not be determined, compiler "
          "may needs to be updated, to properly detect the used extensions");
        result.errors.append_sprintf("DXIL:\n%s", dxilAsm.c_str());
        return result;
      }
      headerCompileResult.header.deviceRequirement.vendorExtensionMask = extensionMask;
    }
    else if (::dxil::ExtensionVendor::AMD == headerCompileResult.header.deviceRequirement.extensionVendor)
    {
      auto extensionMask = ::dxil::parse_AMD_extension_use(dxilAsm, result.errors);
      // when the mask is empty then we failed to figure out which op code sets where used, the driver can not correctly handle this
      // and so this is a compilation error
      if (0 == extensionMask)
      {
        result.errors.append_sprintf(
          "Detected use of AMD HLSL extensions, but used extension op codes could not be determined, compiler may "
          "needs to be updated, to properly detect the used extensions");
        result.errors.append_sprintf("DXIL:\n%s", dxilAsm.c_str());
        return result;
      }
      headerCompileResult.header.deviceRequirement.vendorExtensionMask = extensionMask;
    }
    else
    {
      result.errors.append_sprintf("Missing implementation for a vendor (%u) to handle vendor extensions",
        (uint32_t)headerCompileResult.header.deviceRequirement.extensionVendor);
    }
  }

  eastl::vector<char> shaderSource;
  if (::dx12::dxil::Platform::PC != inputs.platform)
  {
    // for xbox we either recompile with root signature now compute shaders, or store source
    // and some metadata for later recompile when all shaders used by the shader pass are
    // finished and can be recompiled with combined root signature.
    if (::dxil::ShaderStage::COMPUTE == stage)
    {
      // compute is on its own, so we can compile phase two right now
      auto rootSignatureDefine = generate_root_signature_string(
        ::dxil::ComputeRootSignatureExtraProperties{
          .hasAccelerationStructure = has_acceleration_structure(headerCompileResult.header),
          .useResourceDescriptorHeapIndexing = uses_resource_descriptor_heap_indexing(headerCompileResult.header),
          .useSamplerDescriptorHeapIndexing = uses_sampler_descriptor_heap_indexing(headerCompileResult.header),
        },
        headerCompileResult.header.resourceUsageTable);

      compileResult = ::compileShader(inputs.source, inputs.profile, inputs.entry, inputs.compilationOptions.hlsl2021,
        inputs.compilationOptions.enableFp16, inputs.compilationOptions.skipValidation, inputs.compilationOptions.optimize,
        inputs.compilationOptions.debugInfo, inputs.PDBDir, inputs.platform, rootSignatureDefine, 2, false, false,
        inputs.compilationOptions.scarlettW32, inputs.warningsAsErrors, inputs.debugLevel, inputs.embedSource,
        !inputs.streamOutputComponents.empty());
      result.logs += compileResult.messageLog;
      if (compileResult.dxil.empty())
      {
        result.errors.append_sprintf("Error while recompiling compute shader with root signature: "
                                     "%s",
          compileResult.errorLog.c_str());
        return result;
      }
    }
    else
    {
      // On xbox phase two we also store the source
      auto entryLength = strlen(inputs.entry);
      // <compression><skipValidation><optimize><debug_info><scarlett_w32><hlsl2021><enableFp16><profile><entry>\0<source>\0
      shaderSource.reserve(encoded_bits::BitCount + encoded_bits::ExtraBytes + entryLength + inputs.source.size() + 1);
      shaderSource.resize(encoded_bits::BitCount);
      shaderSource[encoded_bits::SkipValidation] = '0' + inputs.compilationOptions.skipValidation; // ends up being either '0' or '1'
      shaderSource[encoded_bits::Optimize] = '0' + inputs.compilationOptions.optimize;             // ends up being either '0' or '1'
      shaderSource[encoded_bits::DebugInfo] = '0' + inputs.compilationOptions.debugInfo;           // ends up being either '0' or '1'
      shaderSource[encoded_bits::ScarlettW32] = '0' + inputs.compilationOptions.scarlettW32;       // ends up being either '0' or '1'
      shaderSource[encoded_bits::HLSL2021] = '0' + inputs.compilationOptions.hlsl2021;             // ends up being either '0' or '1'
      shaderSource[encoded_bits::EnableFp16] = '0' + inputs.compilationOptions.enableFp16;         // ends up being either '0' or '1'
      shaderSource.insert(shaderSource.end(), inputs.profile, inputs.profile + encoded_bits::ProfileLength);
      shaderSource.insert(shaderSource.end(), inputs.entry, inputs.entry + entryLength);
      // we insert here a null terminator for ease of use later, consumer will know that source will
      // start after that
      shaderSource.push_back('\0');
      shaderSource.insert(shaderSource.end(), inputs.source.data(), inputs.source.data() + inputs.source.size());
      // make sure we are null terminated properly, somewhere inside DXC it is expected to be null
      // terminated, even when it is querying the size of the string...
      shaderSource.push_back('\0');
    }
  }

  result.errors = eastl::move(compileResult.errorLog);
  // only provide name in debug builds
  result.bytecode = compileResult.dxil;
  result.metadata =
    pack_shader(compileResult.dxil, shaderSource, headerCompileResult.header, headerCompileResult.streamOutputComponents);
  return result;
}

void dx12::dxil::combineShaders(SmallTab<unsigned, TmpmemAlloc> &meta, SmallTab<unsigned, TmpmemAlloc> &bytecode,
  const ShaderStageData &vs, const ShaderStageData &hs, const ShaderStageData &ds, const ShaderStageData &gs, unsigned id)
{
  bindump::ReservingMemoryWriter writer;
  ::dxil::StoredShaderType type = ::dxil::StoredShaderType::combinedVertexShader;
  dag::ConstSpan<::dxil::StreamOutputComponentInfo> streamOutputComponents;

  auto vertex_shader = decode_shader_container(vs.metadata, false);
  if (!vertex_shader.streamOutputComponents.empty())
    streamOutputComponents = vertex_shader.streamOutputComponents;
  vertex_shader.shader.bytecodeOffset = 0;
  vertex_shader.shader.bytecodeSize = data_size(vs.bytecode);
  if (vertex_shader.shader.shaderHeader.shaderType == (uint16_t)::dxil::ShaderStage::MESH)
  {
    uint32_t ms_size = dag::align_up(data_size(vs.bytecode), sizeof(unsigned));
    uint32_t as_size = dag::align_up(data_size(gs.bytecode), sizeof(unsigned));
    bytecode.resize((ms_size + as_size) / sizeof(unsigned));

    bindump::Master<::dxil::MeshShaderPipeline> layout;
    *layout.meshShader = vertex_shader.shader;
    memcpy(&bytecode[0], vs.bytecode.data(), data_size(vs.bytecode));
    if (!gs.empty())
    {
      layout.amplificationShader.create();
      *layout.amplificationShader = decode_shader_container(gs.metadata, false).shader;
      layout.amplificationShader->bytecodeOffset = ms_size;
      layout.amplificationShader->bytecodeSize = data_size(gs.bytecode);
      memcpy(&bytecode[ms_size / sizeof(unsigned)], gs.bytecode.data(), data_size(gs.bytecode));
      G_ASSERT(layout.amplificationShader->shaderHeader.shaderType == (uint16_t)::dxil::ShaderStage::AMPLIFICATION);
    }
    G_ASSERT(ds.empty() && hs.empty());
    bindump::streamWrite(layout, writer);
    type = ::dxil::StoredShaderType::meshShader;
  }
  else
  {
    uint32_t vs_size = dag::align_up(data_size(vs.bytecode), sizeof(unsigned));
    uint32_t hs_size = dag::align_up(data_size(hs.bytecode), sizeof(unsigned));
    uint32_t ds_size = dag::align_up(data_size(ds.bytecode), sizeof(unsigned));
    uint32_t gs_size = dag::align_up(data_size(gs.bytecode), sizeof(unsigned));

    uint32_t offset = 0;
    bytecode.resize((vs_size + hs_size + ds_size + gs_size) / sizeof(unsigned));

    bindump::Master<::dxil::VertexShaderPipeline> layout;
    *layout.vertexShader = vertex_shader.shader;

    G_ASSERT(offset + data_size(vs.bytecode) <= bytecode.size() * sizeof(unsigned));
    memcpy(&bytecode[offset / sizeof(unsigned)], vs.bytecode.data(), data_size(vs.bytecode));
    offset += vs_size;

    if (!hs.empty())
    {
      layout.hullShader.create();
      auto hull_shader = decode_shader_container(hs.metadata, false);
      hull_shader.shader.bytecodeOffset = offset;
      hull_shader.shader.bytecodeSize = data_size(hs.bytecode);
      G_ASSERT(offset + data_size(hs.bytecode) <= bytecode.size() * sizeof(unsigned));
      memcpy(&bytecode[offset / sizeof(unsigned)], hs.bytecode.data(), data_size(hs.bytecode));
      offset += hs_size;
      if (!hull_shader.streamOutputComponents.empty())
        streamOutputComponents = hull_shader.streamOutputComponents;
      *layout.hullShader = hull_shader.shader;
    }
    if (!ds.empty())
    {
      layout.domainShader.create();
      *layout.domainShader = decode_shader_container(ds.metadata, false).shader;
      layout.domainShader->bytecodeOffset = offset;
      layout.domainShader->bytecodeSize = data_size(ds.bytecode);
      G_ASSERT(offset + data_size(ds.bytecode) <= bytecode.size() * sizeof(unsigned));
      memcpy(&bytecode[offset / sizeof(unsigned)], ds.bytecode.data(), data_size(ds.bytecode));
      offset += ds_size;
    }
    if (!gs.empty())
    {
      layout.geometryShader.create();
      auto geometry_shader = decode_shader_container(gs.metadata, false);
      if (!geometry_shader.streamOutputComponents.empty())
        streamOutputComponents = geometry_shader.streamOutputComponents;
      *layout.geometryShader = geometry_shader.shader;
      layout.geometryShader->bytecodeOffset = offset;
      layout.geometryShader->bytecodeSize = data_size(gs.bytecode);
      G_ASSERT(offset + data_size(gs.bytecode) <= bytecode.size() * sizeof(unsigned));
      memcpy(&bytecode[offset / sizeof(unsigned)], gs.bytecode.data(), data_size(gs.bytecode));
      offset += gs_size;
    }
    G_ASSERT(offset == bytecode.size() * sizeof(unsigned));
    bindump::streamWrite(layout, writer);
  }

  eastl::vector<uint8_t> packed;
  if (streamOutputComponents.empty())
    packed = create_shader_container(eastl::move(writer.mData), {type, false});
  else
  {
    auto programWithStreamOutput = create_shader_with_stream_output(eastl::move(writer.mData), streamOutputComponents);
    packed = create_shader_container(eastl::move(programWithStreamOutput), {type, true});
  }

  const size_t dwordsToFitPacked = (packed.size() - 1) / elem_size(meta) + 1;
  meta.resize(dwordsToFitPacked + 1); // +1 for id at the beginning

  meta[0] = id;
  eastl::copy(packed.begin(), packed.end(), (uint8_t *)&meta[1]);
}

dx12::dxil::RootSignatureStore dx12::dxil::generateRootSignatureDefinition(dag::ConstSpan<uint8_t> vs_metadata,
  dag::ConstSpan<uint8_t> hs_metadata, dag::ConstSpan<uint8_t> ds_metadata, dag::ConstSpan<uint8_t> gs_metadata,
  dag::ConstSpan<uint8_t> ps_metadata)
{
  dx12::dxil::RootSignatureStore result{};

  DecodedShaderContainer vsDecoded = decode_shader_container(vs_metadata, ps_metadata.size() == 0);
  result.hasVertexInput = 0 != vsDecoded.shader.shaderHeader.inOutSemanticMask;
  result.isMesh = ::dxil::ShaderStage::MESH == static_cast<::dxil::ShaderStage>(vsDecoded.shader.shaderHeader.shaderType);
  result.hasAmplificationStage = result.isMesh && (gs_metadata.size() > 0);
  result.vsResources = vsDecoded.shader.shaderHeader.resourceUsageTable;
  result.hasAccelerationStructure = has_acceleration_structure(vsDecoded.shader.shaderHeader);
  result.hasStreamOutput = !vsDecoded.streamOutputComponents.empty();
  result.useResourceDescriptorHeapIndexing = uses_resource_descriptor_heap_indexing(vsDecoded.shader.shaderHeader);
  result.useSamplerDescriptorHeapIndexing = uses_sampler_descriptor_heap_indexing(vsDecoded.shader.shaderHeader);

  if (hs_metadata.empty() != ds_metadata.empty())
  {
    debug("Tessellation stage incomplete");
    return {};
  }

  if (hs_metadata.size())
  {
    auto hsDecoded = decode_shader_container(hs_metadata, false);
    result.hsResources = hsDecoded.shader.shaderHeader.resourceUsageTable;
    result.hasAccelerationStructure = result.hasAccelerationStructure || has_acceleration_structure(hsDecoded.shader.shaderHeader);
    result.hasStreamOutput |= !hsDecoded.streamOutputComponents.empty();
    result.useResourceDescriptorHeapIndexing =
      result.useResourceDescriptorHeapIndexing || uses_resource_descriptor_heap_indexing(hsDecoded.shader.shaderHeader);
    result.useSamplerDescriptorHeapIndexing =
      result.useSamplerDescriptorHeapIndexing || uses_sampler_descriptor_heap_indexing(hsDecoded.shader.shaderHeader);
  }

  if (ds_metadata.size())
  {
    auto dsDecoded = decode_shader_container(ds_metadata, false);
    result.dsResources = dsDecoded.shader.shaderHeader.resourceUsageTable;
    result.hasAccelerationStructure = result.hasAccelerationStructure || has_acceleration_structure(dsDecoded.shader.shaderHeader);
    result.hasStreamOutput |= !dsDecoded.streamOutputComponents.empty();
    result.useResourceDescriptorHeapIndexing =
      result.useResourceDescriptorHeapIndexing || uses_resource_descriptor_heap_indexing(dsDecoded.shader.shaderHeader);
    result.useSamplerDescriptorHeapIndexing =
      result.useSamplerDescriptorHeapIndexing || uses_sampler_descriptor_heap_indexing(dsDecoded.shader.shaderHeader);
  }

  if (gs_metadata.size())
  {
    auto gsDecoded = decode_shader_container(gs_metadata, false);
    result.gsResources = gsDecoded.shader.shaderHeader.resourceUsageTable;
    result.hasAccelerationStructure = result.hasAccelerationStructure || has_acceleration_structure(gsDecoded.shader.shaderHeader);
    result.hasStreamOutput |= !gsDecoded.streamOutputComponents.empty();
    result.useResourceDescriptorHeapIndexing =
      result.useResourceDescriptorHeapIndexing || uses_resource_descriptor_heap_indexing(gsDecoded.shader.shaderHeader);
    result.useSamplerDescriptorHeapIndexing =
      result.useSamplerDescriptorHeapIndexing || uses_sampler_descriptor_heap_indexing(gsDecoded.shader.shaderHeader);
  }

  if (!ps_metadata.empty())
  {
    auto psDecoded = decode_shader_container(ps_metadata, false);
    result.psResources = psDecoded.shader.shaderHeader.resourceUsageTable;
    result.hasAccelerationStructure = result.hasAccelerationStructure || has_acceleration_structure(psDecoded.shader.shaderHeader);
    result.useResourceDescriptorHeapIndexing =
      result.useResourceDescriptorHeapIndexing || uses_resource_descriptor_heap_indexing(psDecoded.shader.shaderHeader);
    result.useSamplerDescriptorHeapIndexing =
      result.useSamplerDescriptorHeapIndexing || uses_sampler_descriptor_heap_indexing(psDecoded.shader.shaderHeader);
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

BINDUMP_BEGIN_LAYOUT(VertexProgramPhaseOneMetadata)
  BINDUMP_USING_EXTENSION()
  uint32_t magic = 0;
  VecHolder<uint8_t> vertexShaderMetadata;
  VecHolder<uint8_t> hullShaderMetadata;
  VecHolder<uint8_t> domainShaderMetadata;
  VecHolder<uint8_t> geomentryShaderMetadata;
  uint32_t compilationFlags;
  dx12::dxil::RootSignatureStore rootSignature;
BINDUMP_END_LAYOUT()

BINDUMP_BEGIN_LAYOUT(VertexProgramPhaseOneBytecode)
  BINDUMP_USING_EXTENSION()
  VecHolder<uint32_t> vertexShaderBytecode;
  VecHolder<uint32_t> hullShaderBytecode;
  VecHolder<uint32_t> domainShaderBytecode;
  VecHolder<uint32_t> geomentryShaderBytecode;
BINDUMP_END_LAYOUT()

BINDUMP_BEGIN_LAYOUT(PixelShaderPhaseOneMetadata)
  BINDUMP_USING_EXTENSION()
  uint32_t magic = 0;
  VecHolder<uint8_t> pixelShaderMetadata;
  uint32_t compilationFlags;
  dx12::dxil::RootSignatureStore rootSignature;
BINDUMP_END_LAYOUT()

BINDUMP_BEGIN_LAYOUT(PixelShaderPhaseOneBytecode)
  BINDUMP_USING_EXTENSION()
  VecHolder<uint32_t> pixelShaderBytecode;
BINDUMP_END_LAYOUT()

namespace
{
bool operator==(const dx12::dxil::RootSignatureStore &l, const dx12::dxil::RootSignatureStore &r)
{
  return l.vsResources == r.vsResources && l.hsResources == r.hsResources && l.dsResources == r.dsResources &&
         l.gsResources == r.gsResources && l.psResources == r.psResources && l.hasVertexInput == r.hasVertexInput &&
         l.hasAmplificationStage == r.hasAmplificationStage && l.hasAccelerationStructure == r.hasAccelerationStructure &&
         l.hasStreamOutput == r.hasStreamOutput;
}

bool operator!=(const dx12::dxil::RootSignatureStore &l, const dx12::dxil::RootSignatureStore &r) { return !(l == r); }

bool operator==(const auto &lhs, const auto &rhs)
{
  return lhs.size() == rhs.size() && (lhs.data() == rhs.data() || mem_eq(rhs, lhs.data()));
}
} // namespace

bool dx12::dxil::comparePhaseOneVertexProgram(CombinedShaderData combined_vprog, const ShaderStageData &vs, const ShaderStageData &hs,
  const ShaderStageData &ds, const ShaderStageData &gs, const RootSignatureStore &signature)
{
  auto *combinedMetadata = bindump::map<VertexProgramPhaseOneMetadata>(combined_vprog.metadata.data());
  if (!combinedMetadata)
  {
    return false;
  }

  if (combinedMetadata->rootSignature != signature)
  {
    return false;
  }

  // check if presence of optional embedded shaders are matching
  if (hs.metadata.empty() != combinedMetadata->hullShaderMetadata.empty())
  {
    return false;
  }
  if (ds.metadata.empty() != combinedMetadata->domainShaderMetadata.empty())
  {
    return false;
  }
  if (gs.metadata.empty() != combinedMetadata->geomentryShaderMetadata.empty())
  {
    return false;
  }

  auto *combinedBytecode = bindump::map<VertexProgramPhaseOneBytecode>((const uint8_t *)combined_vprog.bytecode.data());
  if (!combinedBytecode)
  {
    return false;
  }

  if (!hs.empty())
  {
    dag::ConstSpan<unsigned> combinedHSBytecode = combinedBytecode->hullShaderBytecode;
    if (hs.bytecode != combinedHSBytecode)
    {
      return false;
    }
  }
  if (!ds.empty())
  {
    dag::ConstSpan<unsigned> combinedDSBytecode = combinedBytecode->domainShaderBytecode;
    if (ds.bytecode != combinedDSBytecode)
    {
      return false;
    }
  }
  if (!gs.empty())
  {
    dag::ConstSpan<unsigned> combinedGSBytecode = combinedBytecode->geomentryShaderBytecode;
    if (gs.bytecode != combinedGSBytecode)
    {
      return false;
    }
  }

  dag::ConstSpan<unsigned> combinedVSBytecode = combinedBytecode->vertexShaderBytecode;
  return vs.bytecode == combinedVSBytecode;
}

bool dx12::dxil::comparePhaseOnePixelShader(CombinedShaderData combined_psh, const ShaderStageData &ps,
  const RootSignatureStore &signature)
{
  auto *combinedMetadata = bindump::map<PixelShaderPhaseOneMetadata>(combined_psh.metadata.data());
  if (!combinedMetadata || combinedMetadata->rootSignature != signature)
  {
    return false;
  }

  auto *combinedBytecode = bindump::map<PixelShaderPhaseOneBytecode>((const uint8_t *)combined_psh.bytecode.data());
  if (!combinedBytecode)
  {
    return false;
  }

  dag::ConstSpan<unsigned> combinedPSBytecode = combinedBytecode->pixelShaderBytecode;
  return combinedPSBytecode == ps.bytecode;
}

auto dx12::dxil::combinePhaseOneVertexProgram(const ShaderStageData &vs, const ShaderStageData &hs, const ShaderStageData &ds,
  const ShaderStageData &gs, const RootSignatureStore &signature, unsigned id, CompilationOptions options) -> CombinedShaderStorage
{
  CombinedShaderStorage result{
    eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>(), eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>()};

  bindump::Master<VertexProgramPhaseOneMetadata> combinedMetadata;
  combinedMetadata.magic = id;
  combinedMetadata.compilationFlags = 0;
  combinedMetadata.compilationFlags |= gs.empty() ? 0 : COMPILATION_FLAG_HAS_GS;
  combinedMetadata.compilationFlags |= (hs.empty() || ds.empty()) ? 0 : COMPILATION_FLAG_HAS_HD_DS;
  combinedMetadata.compilationFlags |= options.optimize ? COMPILATION_FLAG_OPTIMIZE : 0;
  combinedMetadata.compilationFlags |= options.skipValidation ? COMPILATION_FLAG_SKIP_VALIDATION : 0;
  combinedMetadata.compilationFlags |= options.scarlettW32 ? COMPILATION_FLAG_SCARLETT_W32 : 0;
  combinedMetadata.compilationFlags |= options.debugInfo ? COMPILATION_FLAG_DEBUG_INFO : 0;
  combinedMetadata.compilationFlags |= options.hlsl2021 ? COMPILATION_FLAG_HLSL2021 : 0;
  combinedMetadata.compilationFlags |= options.enableFp16 ? COMPILATION_FLAG_ENABLEFP16 : 0;
  combinedMetadata.rootSignature = signature;

  combinedMetadata.vertexShaderMetadata = vs.metadata;
  combinedMetadata.hullShaderMetadata = hs.metadata;
  combinedMetadata.domainShaderMetadata = ds.metadata;
  combinedMetadata.geomentryShaderMetadata = gs.metadata;

  {
    bindump::ReservingMemoryWriter writer;
    bindump::streamWrite(combinedMetadata, writer);
    G_ASSERT(memcmp(writer.mData.data(), &id, sizeof(id)) == 0);
    result.metadata->resize((writer.mData.size() + elem_size(*result.metadata) - 1) / elem_size(*result.metadata));
    eastl::copy(writer.mData.begin(), writer.mData.end(), (uint8_t *)result.metadata->data());
    G_ASSERT(memcmp(result.metadata->data(), &id, sizeof(id)) == 0);
  }

  bindump::Master<VertexProgramPhaseOneBytecode> combinedBytecode;
  combinedBytecode.vertexShaderBytecode = vs.bytecode;
  combinedBytecode.hullShaderBytecode = hs.bytecode;
  combinedBytecode.domainShaderBytecode = ds.bytecode;
  combinedBytecode.geomentryShaderBytecode = gs.bytecode;

  {
    bindump::ReservingMemoryWriter writer;
    bindump::streamWrite(combinedBytecode, writer);

    result.bytecode->resize((writer.mData.size() + elem_size(*result.bytecode) - 1) / elem_size(*result.bytecode));
    eastl::copy(writer.mData.begin(), writer.mData.end(), (uint8_t *)result.bytecode->data());
  }

  return result;
}

auto dx12::dxil::combinePhaseOnePixelShader(const ShaderStageData &ps, const RootSignatureStore &signature, unsigned id, bool has_gs,
  bool has_ts, CompilationOptions options) -> CombinedShaderStorage
{
  CombinedShaderStorage result{
    eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>(), eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>()};

  bindump::Master<PixelShaderPhaseOneMetadata> combinedMetadata;
  combinedMetadata.magic = id;
  combinedMetadata.compilationFlags = 0;
  combinedMetadata.compilationFlags |= has_gs ? COMPILATION_FLAG_HAS_GS : 0;
  combinedMetadata.compilationFlags |= has_ts ? COMPILATION_FLAG_HAS_HD_DS : 0;
  combinedMetadata.compilationFlags |= options.optimize ? COMPILATION_FLAG_OPTIMIZE : 0;
  combinedMetadata.compilationFlags |= options.skipValidation ? COMPILATION_FLAG_SKIP_VALIDATION : 0;
  combinedMetadata.compilationFlags |= options.scarlettW32 ? COMPILATION_FLAG_SCARLETT_W32 : 0;
  combinedMetadata.compilationFlags |= options.debugInfo ? COMPILATION_FLAG_DEBUG_INFO : 0;
  combinedMetadata.compilationFlags |= options.hlsl2021 ? COMPILATION_FLAG_HLSL2021 : 0;
  combinedMetadata.compilationFlags |= options.enableFp16 ? COMPILATION_FLAG_ENABLEFP16 : 0;
  combinedMetadata.rootSignature = signature;

  combinedMetadata.pixelShaderMetadata = ps.metadata;

  {
    bindump::ReservingMemoryWriter writer;
    bindump::streamWrite(combinedMetadata, writer);

    result.metadata->resize((writer.mData.size() + elem_size(*result.metadata) - 1) / elem_size(*result.metadata));
    eastl::copy(writer.mData.begin(), writer.mData.end(), (uint8_t *)result.metadata->data());
  }

  bindump::Master<PixelShaderPhaseOneBytecode> combinedBytecode;
  combinedBytecode.pixelShaderBytecode = ps.bytecode;

  {
    bindump::ReservingMemoryWriter writer;
    bindump::streamWrite(combinedBytecode, writer);

    result.bytecode->resize((writer.mData.size() + elem_size(*result.bytecode) - 1) / elem_size(*result.bytecode));
    eastl::copy(writer.mData.begin(), writer.mData.end(), (uint8_t *)result.bytecode->data());
  }

  return result;
}

auto dx12::dxil::recompileVertexProgram(dag::ConstSpan<uint8_t> source, Platform platform, wchar_t *pdb_dir, DebugLevel debug_level,
  bool embed_source) -> eastl::optional<CombinedShaderStorage>
{
  auto *metadata = bindump::map<VertexProgramPhaseOneMetadata>((const uint8_t *)source.data());
  if (!metadata)
  {
    return CombinedShaderStorage{};
  }

#if REPORT_PERFORMANCE_METRICS
  AutoFuncProfT<AFP_MSEC> funcProfiler("dx12::dxil::recompileVertexProgram: took %dms");
#endif

  eastl::wstring progSignature;
  if (metadata->rootSignature.isMesh)
  {
    progSignature = generate_mesh_root_signature_string(
      ::dxil::GraphicsMeshRootSignatureExtraProperties{
        .hasStreamOutput = metadata->rootSignature.hasStreamOutput,
        .hasAccelerationStructure = metadata->rootSignature.hasAccelerationStructure,
        .useResourceDescriptorHeapIndexing = metadata->rootSignature.useResourceDescriptorHeapIndexing,
        .useSamplerDescriptorHeapIndexing = metadata->rootSignature.useSamplerDescriptorHeapIndexing,
      },
      metadata->rootSignature.hasAmplificationStage, metadata->rootSignature.vsResources, metadata->rootSignature.gsResources,
      metadata->rootSignature.psResources);
  }
  else
  {
    progSignature = generate_root_signature_string(
      ::dxil::GraphicsRootSignatureExtraProperties{
        .hasVertexInputs = metadata->rootSignature.hasVertexInput,
        .hasStreamOutput = metadata->rootSignature.hasStreamOutput,
        .hasAccelerationStructure = metadata->rootSignature.hasAccelerationStructure,
        .useResourceDescriptorHeapIndexing = metadata->rootSignature.useResourceDescriptorHeapIndexing,
        .useSamplerDescriptorHeapIndexing = metadata->rootSignature.useSamplerDescriptorHeapIndexing,
      },
      metadata->rootSignature.vsResources, metadata->rootSignature.hsResources, metadata->rootSignature.dsResources,
      metadata->rootSignature.gsResources, metadata->rootSignature.psResources);
  }

  auto vsDecodedMetadata = decode_shader_container(metadata->vertexShaderMetadata, true);
  auto hsDecodedMetadata = decode_shader_container(metadata->hullShaderMetadata, true);
  auto dsDecodedMetadata = decode_shader_container(metadata->domainShaderMetadata, true);
  auto gsDecodedMetadata = decode_shader_container(metadata->geomentryShaderMetadata, true);

  auto [rebuildVSMetadata, rebuildVSBytecode] =
    recompile_shader(vsDecodedMetadata.source.data(), vsDecodedMetadata.shader.shaderHeader, vsDecodedMetadata.streamOutputComponents,
      progSignature, platform, hsDecodedMetadata.source.size() && dsDecodedMetadata.source.size(), gsDecodedMetadata.source.size(),
      pdb_dir, debug_level, embed_source, metadata->rootSignature.hasStreamOutput);

  if (rebuildVSMetadata.empty() || rebuildVSBytecode.empty())
  {
    return eastl::nullopt;
  }

  eastl::vector<uint8_t> rebuildHSMetadata, rebuildHSBytecode;
  if (hsDecodedMetadata.source.size())
  {
    eastl::tie(rebuildHSMetadata, rebuildHSBytecode) = recompile_shader(hsDecodedMetadata.source.data(),
      hsDecodedMetadata.shader.shaderHeader, hsDecodedMetadata.streamOutputComponents, progSignature, platform, true,
      gsDecodedMetadata.source.size(), pdb_dir, debug_level, embed_source, metadata->rootSignature.hasStreamOutput);
    if (rebuildHSMetadata.empty() || rebuildHSBytecode.empty())
    {
      return eastl::nullopt;
    }
  }

  eastl::vector<uint8_t> rebuildDSMetadata, rebuildDSBytecode;
  if (dsDecodedMetadata.source.size())
  {
    eastl::tie(rebuildDSMetadata, rebuildDSBytecode) =
      recompile_shader(dsDecodedMetadata.source.data(), dsDecodedMetadata.shader.shaderHeader, {}, progSignature, platform, true,
        gsDecodedMetadata.source.size(), pdb_dir, debug_level, embed_source, metadata->rootSignature.hasStreamOutput);
    if (rebuildDSMetadata.empty() || rebuildDSBytecode.empty())
    {
      return eastl::nullopt;
    }
  }

  eastl::vector<uint8_t> rebuildGSMetadata, rebuildGSBytecode;
  if (gsDecodedMetadata.source.size())
  {
    eastl::tie(rebuildGSMetadata, rebuildGSBytecode) = recompile_shader(gsDecodedMetadata.source.data(),
      gsDecodedMetadata.shader.shaderHeader, gsDecodedMetadata.streamOutputComponents, progSignature, platform,
      hsDecodedMetadata.source.size() && dsDecodedMetadata.source.size(), true, pdb_dir, debug_level, embed_source,
      metadata->rootSignature.hasStreamOutput);
    if (rebuildGSMetadata.empty() || rebuildGSBytecode.empty())
    {
      return eastl::nullopt;
    }
  }

  ShaderStageData vs{
    rebuildVSMetadata, make_span(reinterpret_cast<unsigned *>(rebuildVSBytecode.data()), rebuildVSBytecode.size() / sizeof(unsigned))};
  CombinedShaderStorage newProg = {
    eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>(), eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>()};
  if (!rebuildHSMetadata.empty() || !rebuildDSMetadata.empty() || !rebuildGSMetadata.empty())
  {
    ShaderStageData hs, ds, gs;
    if (!rebuildHSMetadata.empty())
      hs = {rebuildHSMetadata,
        make_span(reinterpret_cast<unsigned *>(rebuildHSBytecode.data()), rebuildHSBytecode.size() / sizeof(unsigned))};
    ;
    if (!rebuildDSMetadata.empty())
      ds = {rebuildDSMetadata,
        make_span(reinterpret_cast<unsigned *>(rebuildDSBytecode.data()), rebuildDSBytecode.size() / sizeof(unsigned))};
    if (!rebuildGSMetadata.empty())
      gs = {rebuildGSMetadata,
        make_span(reinterpret_cast<unsigned *>(rebuildGSBytecode.data()), rebuildGSBytecode.size() / sizeof(unsigned))};

    combineShaders(*newProg.metadata, *newProg.bytecode, vs, hs, ds, gs, metadata->magic);
  }
  else
  {
    clear_and_resize(*newProg.metadata, data_size(vs.metadata) / sizeof(unsigned) + 1);
    (*newProg.metadata)[0] = metadata->magic;
    mem_copy_from(make_span(*newProg.metadata).subspan(1), vs.metadata.data());

    clear_and_resize(*newProg.bytecode, data_size(vs.bytecode) / sizeof(unsigned));
    mem_copy_from(make_span(*newProg.bytecode), vs.bytecode.data());
  }

  return newProg;
}

auto dx12::dxil::recompilePixelShader(dag::ConstSpan<uint8_t> source, Platform platform, wchar_t *pdb_dir, DebugLevel debug_level,
  bool embed_source) -> eastl::optional<CombinedShaderStorage>
{
  auto *metadata = bindump::map<PixelShaderPhaseOneMetadata>((const uint8_t *)source.data());
  if (!metadata)
  {
    return CombinedShaderStorage{};
  }

#if REPORT_PERFORMANCE_METRICS
  AutoFuncProfT<AFP_MSEC> funcProfiler("dx12::dxil::recompilePixelSader: took %dms");
#endif

  eastl::wstring progSignature;
  if (metadata->rootSignature.isMesh)
  {
    progSignature = generate_mesh_root_signature_string(
      ::dxil::GraphicsMeshRootSignatureExtraProperties{
        .hasStreamOutput = metadata->rootSignature.hasStreamOutput,
        .hasAccelerationStructure = metadata->rootSignature.hasAccelerationStructure,
        .useResourceDescriptorHeapIndexing = metadata->rootSignature.useResourceDescriptorHeapIndexing,
        .useSamplerDescriptorHeapIndexing = metadata->rootSignature.useSamplerDescriptorHeapIndexing,
      },
      metadata->rootSignature.hasAmplificationStage, metadata->rootSignature.vsResources, metadata->rootSignature.gsResources,
      metadata->rootSignature.psResources);
  }
  else
  {
    progSignature = generate_root_signature_string(
      ::dxil::GraphicsRootSignatureExtraProperties{
        .hasVertexInputs = metadata->rootSignature.hasVertexInput,
        .hasStreamOutput = metadata->rootSignature.hasStreamOutput,
        .hasAccelerationStructure = metadata->rootSignature.hasAccelerationStructure,
        .useResourceDescriptorHeapIndexing = metadata->rootSignature.useResourceDescriptorHeapIndexing,
        .useSamplerDescriptorHeapIndexing = metadata->rootSignature.useSamplerDescriptorHeapIndexing,
      },
      metadata->rootSignature.vsResources, metadata->rootSignature.hsResources, metadata->rootSignature.dsResources,
      metadata->rootSignature.gsResources, metadata->rootSignature.psResources);
  }

  DecodedShaderContainer psDecoded;
  // null shader check, we only store signature
  char nullShaderStore[64];
  ::dxil::ShaderHeader defaultShaderHeader = {};
  if (metadata->pixelShaderMetadata.empty())
  {
    bool skipValidation = 0 != (metadata->compilationFlags & COMPILATION_FLAG_SKIP_VALIDATION);
    bool optimize = 0 != (metadata->compilationFlags & COMPILATION_FLAG_OPTIMIZE);
    bool debugInfo = 0 != (metadata->compilationFlags & COMPILATION_FLAG_DEBUG_INFO);
    bool scarlettW32 = 0 != (metadata->compilationFlags & COMPILATION_FLAG_SCARLETT_W32);
    bool hlsl2021 = 0 != (metadata->compilationFlags & COMPILATION_FLAG_HLSL2021);
    bool enableFp16 = 0 != (metadata->compilationFlags & COMPILATION_FLAG_ENABLEFP16);
    psDecoded = get_null_shader_container(defaultShaderHeader, hlsl2021, enableFp16, skipValidation, optimize, debugInfo, scarlettW32,
      nullShaderStore, sizeof(nullShaderStore));
  }
  else
  {
    psDecoded = decode_shader_container(metadata->pixelShaderMetadata, true);
  }

  bool hasTS = 0 != (metadata->compilationFlags & COMPILATION_FLAG_HAS_HD_DS);
  bool hasGS = 0 != (metadata->compilationFlags & COMPILATION_FLAG_HAS_GS);
  auto [rebuildPSMetadata, rebuildPSBytecode] = recompile_shader(psDecoded.source.data(), psDecoded.shader.shaderHeader, {},
    progSignature, platform, hasTS, hasGS, pdb_dir, debug_level, embed_source, metadata->rootSignature.hasStreamOutput);

  if (rebuildPSMetadata.empty() || rebuildPSBytecode.empty())
  {
    return eastl::nullopt;
  }

  CombinedShaderStorage newProg{
    eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>(), eastl::make_unique<SmallTab<unsigned, TmpmemAlloc>>()};
  clear_and_resize(*newProg.metadata, rebuildPSMetadata.size() / sizeof(unsigned) + 1);
  (*newProg.metadata)[0] = metadata->magic;
  mem_copy_from(make_span(*newProg.metadata).subspan(1), rebuildPSMetadata.data());

  clear_and_resize(*newProg.bytecode, rebuildPSBytecode.size() / sizeof(unsigned));
  mem_copy_from(make_span(*newProg.bytecode), rebuildPSBytecode.data());

  return newProg;
}
