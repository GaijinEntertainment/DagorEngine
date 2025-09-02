// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <dxil/compiler.h>
#include <supp/dag_comPtr.h>

#include <util/dag_string.h>
#include <math/dag_bits.h>

#include <windows.h>
#include <unknwn.h>
#include <dxc/dxcapi.h>
#include "supplements/auto.h"
#include <debug/dag_debug.h>
#include <EASTL/variant.h>
#include <EASTL/optional.h>

using namespace dxil;

#ifndef D3D_SIT_RTACCELERATIONSTRUCTURE
#define D3D_SIT_RTACCELERATIONSTRUCTURE (D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER + 1)
#endif

#ifndef D3D_SIT_UAV_FEEDBACKTEXTURE
#define D3D_SIT_UAV_FEEDBACKTEXTURE (D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER + 2)
#endif

namespace
{
struct WrappedBlob final : public IDxcBlobEncoding
{
  WrappedBlob(const eastl::vector<uint8_t> &s) : source{s} {}
  const eastl::vector<uint8_t> &source;
  ULONG STDMETHODCALLTYPE AddRef()
  {
    // never do anything, stack managed
    return 1;
  }
  ULONG STDMETHODCALLTYPE Release()
  {
    // never do anything, stack managed
    return 1;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(const IID &id, void **iface)
  {
    if (!iface)
      return E_INVALIDARG;
    if (id == __uuidof(IUnknown) || id == __uuidof(IDxcBlobEncoding) || id == __uuidof(IDxcBlob))
    {
      AddRef();
      *iface = this;
      return NOERROR;
    }
    return E_NOINTERFACE;
  }
  LPVOID STDMETHODCALLTYPE GetBufferPointer(void) { return (void *)source.data(); }
  SIZE_T STDMETHODCALLTYPE GetBufferSize(void) { return source.size(); }

  HRESULT STDMETHODCALLTYPE GetEncoding(_Out_ BOOL *pKnown, _Out_ UINT32 *pCodePage)
  {
    *pKnown = TRUE;
    *pCodePage = CP_ACP;
    return 0;
  }
};

String make_bound_resource_table(const D3D12_SHADER_DESC &desc, ID3D12ShaderReflection *info)
{
  const char horizonzal_line[] = "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
  String result;
  result.append("Resource info:\n");
  result.append(horizonzal_line);
  result.append("Const Buffer Info:\n");
  result.append(horizonzal_line);
  for (UINT cbi = 0; cbi < desc.ConstantBuffers; ++cbi)
  {
    auto constBufferInfo = info->GetConstantBufferByIndex(cbi);
    if (constBufferInfo)
    {
      D3D12_SHADER_BUFFER_DESC constBufferDesc = {};
      constBufferInfo->GetDesc(&constBufferDesc);
      result.aprintf(128, "Constant buffer #%u with %u dwords\n", cbi, constBufferDesc.Size / sizeof(uint32_t));
    }
  }
  result.append(horizonzal_line);
  result.append("Resource binding Info:\n");
  result.append(horizonzal_line);
  result.append("Type                      | Register | Space | Count\n");
  for (UINT bri = 0; bri < desc.BoundResources; ++bri)
  {
    D3D12_SHADER_INPUT_BIND_DESC boundResourceInfo = {};
    info->GetResourceBindingDesc(bri, &boundResourceInfo);

    const char *typeName = "<unknown>";
    char registerPrefix = '?';

    switch (boundResourceInfo.Type)
    {
      case D3D_SIT_CBUFFER:
        typeName = "Constant Buffer";
        registerPrefix = 'b';
        break;
      case D3D_SIT_TBUFFER:
        typeName = "Texture Buffer";
        registerPrefix = 't';
        break;
      case D3D_SIT_TEXTURE:
        typeName = "Texture";
        registerPrefix = 't';
        break;
      case D3D_SIT_SAMPLER:
        typeName = (boundResourceInfo.uFlags & D3D_SIF_COMPARISON_SAMPLER) ? "Sampler Comparison" : "Sampler";
        registerPrefix = 's';
        break;
      case D3D_SIT_UAV_RWTYPED:
        typeName = "UAV Typed";
        registerPrefix = 'u';
        break;
      case D3D_SIT_STRUCTURED:
        typeName = "Structured";
        registerPrefix = 't';
        break;
      case D3D_SIT_UAV_RWSTRUCTURED:
        typeName = "UAV RW Structured";
        registerPrefix = 'u';
        break;
      case D3D_SIT_BYTEADDRESS:
        typeName = "Byte Address";
        registerPrefix = 't';
        break;
      case D3D_SIT_UAV_RWBYTEADDRESS:
        typeName = "UAV RB Byte Address";
        registerPrefix = 'u';
        break;
      case D3D_SIT_UAV_APPEND_STRUCTURED:
        typeName = "UAV Append Structured";
        registerPrefix = 'u';
        break;
      case D3D_SIT_UAV_CONSUME_STRUCTURED:
        typeName = "UAV Consume Structured";
        registerPrefix = 'u';
        break;
      case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        typeName = "UAV RW Structured Counter";
        registerPrefix = 'u';
        break;
      case D3D_SIT_RTACCELERATIONSTRUCTURE:
        typeName = "RT Acceleration Structure";
        registerPrefix = 't';
        break;
      case D3D_SIT_UAV_FEEDBACKTEXTURE:
        typeName = "UAV Feedback Texture";
        registerPrefix = 'u';
        break;
    }

    result.aprintf(128, "%-25s | %c%-7u | %-5u | ", typeName, registerPrefix, boundResourceInfo.BindPoint, boundResourceInfo.Space);
    if (boundResourceInfo.BindCount)
    {
      result.aprintf(32, "%u\n", boundResourceInfo.BindCount);
    }
    else
    {
      result.append("Bindless\n");
    }
  }
  result.append(horizonzal_line);
  return result;
}

bool is_supported_bindless_type(D3D_SHADER_INPUT_TYPE type)
{
  switch (type)
  {
    default:
    case D3D_SIT_CBUFFER:
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
    case D3D_SIT_UAV_FEEDBACKTEXTURE: return false;
    case D3D_SIT_TBUFFER:
    case D3D_SIT_TEXTURE:
    case D3D_SIT_SAMPLER:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
    case D3D_SIT_RTACCELERATIONSTRUCTURE: return true;
  }
}

bool is_srv_type(D3D_SHADER_INPUT_TYPE type)
{
  switch (type)
  {
    default:
    case D3D_SIT_CBUFFER:
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
    case D3D_SIT_UAV_FEEDBACKTEXTURE:
    case D3D_SIT_SAMPLER: return false;
    case D3D_SIT_TBUFFER:
    case D3D_SIT_TEXTURE:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
    case D3D_SIT_RTACCELERATIONSTRUCTURE: return true;
  }
}

bool is_uav_type(D3D_SHADER_INPUT_TYPE type)
{
  switch (type)
  {
    default:
    case D3D_SIT_CBUFFER:
    case D3D_SIT_TBUFFER:
    case D3D_SIT_TEXTURE:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
    case D3D_SIT_RTACCELERATIONSTRUCTURE:
    case D3D_SIT_SAMPLER: return false;
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
    case D3D_SIT_UAV_FEEDBACKTEXTURE: return true;
  }
}

bool is_uav_with_counter_type(D3D_SHADER_INPUT_TYPE type)
{
  switch (type)
  {
    default:
    case D3D_SIT_CBUFFER:
    case D3D_SIT_TBUFFER:
    case D3D_SIT_TEXTURE:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
    case D3D_SIT_RTACCELERATIONSTRUCTURE:
    case D3D_SIT_SAMPLER:
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_FEEDBACKTEXTURE: return false;
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER: return true;
  }
}

bool is_cbv_type(D3D_SHADER_INPUT_TYPE type) { return D3D_SIT_CBUFFER == type; }

bool is_sampler_type(D3D_SHADER_INPUT_TYPE type) { return D3D_SIT_SAMPLER == type; }

ComPtr<IDxcUtils> load_utilities(void *dxc_lib)
{
  ComPtr<IDxcUtils> utilities;
  if (!dxc_lib)
  {
    return utilities;
  }

  DxcCreateInstanceProc DxcCreateInstance = nullptr;
  reinterpret_cast<FARPROC &>(DxcCreateInstance) = GetProcAddress(static_cast<HMODULE>(dxc_lib), "DxcCreateInstance");

  if (!DxcCreateInstance)
  {
    return utilities;
  }

  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utilities));
  if (!utilities)
  {
    return utilities;
  }
  return utilities;
}

template <typename T>
ComPtr<T> create_reflection(IDxcUtils *utils, eastl::span<const uint8_t> data)
{
  DxcBuffer blob;
  blob.Encoding = DXC_CP_ACP;
  blob.Ptr = data.data();
  blob.Size = data.size();

  ComPtr<T> result;
  utils->CreateReflection(&blob, IID_PPV_ARGS(&result));
  return result;
}

eastl::string get_function_name(const D3D12_FUNCTION_DESC &func)
{
  // Name is mangled like it would be c++
  // convention is ?<name>@@<param and return type encoding>
  auto mangledName = func.Name;
  uint32_t start = 0;
  for (; mangledName[start] != '\0'; ++start)
  {
    if (isalpha(mangledName[start]))
    {
      break;
    }
  }
  uint32_t stop = mangledName[start] != '\0' ? start + 1 : start;
  for (; mangledName[stop] != '\0'; ++stop)
  {
    if (!isalpha(mangledName[stop]))
    {
      break;
    }
  }
  return {mangledName + start, stop - start};
}

enum class ExtensionOpCheckResult
{
  NotExtension,
  ValidExtension,
  InvalidExtension,
};

ExtensionOpCheckResult check_nvidia_extension_meta_slot(const D3D12_SHADER_INPUT_BIND_DESC &info)
{
  return (extension::nvidia::register_space_index != info.Space) ? ExtensionOpCheckResult::NotExtension
         : (extension::nvidia::register_index != info.BindPoint) ? ExtensionOpCheckResult::InvalidExtension
                                                                 : ExtensionOpCheckResult::ValidExtension;
}
ExtensionOpCheckResult check_amd_extension_meta_slot(const D3D12_SHADER_INPUT_BIND_DESC &info)
{
  return (extension::amd::register_space_index != info.Space) ? ExtensionOpCheckResult::NotExtension
         : (extension::amd::register_index != info.BindPoint) ? ExtensionOpCheckResult::InvalidExtension
                                                              : ExtensionOpCheckResult::ValidExtension;
}
} // namespace

ShaderHeaderCompileResult dxil::compileHeaderFromReflectionData(ShaderStage stage, const eastl::vector<uint8_t> &reflection,
  uint32_t max_const_count, dag::ConstSpan<dxil::StreamOutputComponentInfo> stream_output_components, void *dxc_lib)
{
  ShaderHeaderCompileResult result = {};
#if _TARGET_PC_WIN

  auto utilitiesLoadResult = load_utilities(dxc_lib);
  if (!utilitiesLoadResult)
  {
    return result;
  }

  auto shaderInfo = create_reflection<ID3D12ShaderReflection>(utilitiesLoadResult /*->*/.Get(), reflection);
  if (!shaderInfo)
  {
    return result;
  }

  result.header.shaderType = static_cast<uint16_t>(stage);
  result.header.maxConstantCount = max_const_count;
  result.header.bonesConstantsUsed = -1; // @TODO: remove

  result.isOk = true;
  D3D12_SHADER_DESC desc = {};
  shaderInfo->GetDesc(&desc);

  uint64_t reqFlags = shaderInfo->GetRequiresFlags();

  result.header.deviceRequirement.shaderModel = static_cast<uint8_t>(desc.Version);
  result.header.deviceRequirement.shaderFeatureFlagsLow = static_cast<uint32_t>(reqFlags);
  result.header.deviceRequirement.shaderFeatureFlagsHigh = static_cast<uint32_t>(reqFlags >> 32);
  result.header.deviceRequirement.extensionVendor = ::dxil::ExtensionVendor::NoExtensionsUsed;
  result.header.deviceRequirement.vendorExtensionMask = 0;

  result.header.inputPrimitive = desc.InputPrimitive;

  for (UINT bri = 0; bri < desc.BoundResources; ++bri)
  {
    D3D12_SHADER_INPUT_BIND_DESC boundResourceInfo = {};
    shaderInfo->GetResourceBindingDesc(bri, &boundResourceInfo);

    // Both NVidia and AMD use a special UAV buffer to "record" extension commands to.
    // The device driver will then decode this recordings to this special buffer into
    // vendor specific commands.
    auto nvExtensionUse = check_nvidia_extension_meta_slot(boundResourceInfo);
    if (ExtensionOpCheckResult::ValidExtension == nvExtensionUse)
    {
      result.header.resourceUsageTable.specialConstantsMask |= SC_NVIDIA_EXTENSION;
      result.header.deviceRequirement.extensionVendor = ::dxil::ExtensionVendor::NVIDIA;
      result.logMessage += "dxil::compileHeaderFromShader: NVidia extension detected\n";
      continue;
    }
    if (ExtensionOpCheckResult::InvalidExtension == nvExtensionUse)
    {
      result.isOk = false;
      result.logMessage += "dxil::compileHeaderFromShader: Invalid use of NVidia extension\n";
      continue;
    }

    auto amdExtensionUse = check_amd_extension_meta_slot(boundResourceInfo);
    if (ExtensionOpCheckResult::ValidExtension == amdExtensionUse)
    {
      result.header.resourceUsageTable.specialConstantsMask |= SC_AMD_EXTENSION;
      result.header.deviceRequirement.extensionVendor = ::dxil::ExtensionVendor::AMD;
      result.logMessage += "dxil::compileHeaderFromShader: AMD extension detected\n";
      continue;
    }
    if (ExtensionOpCheckResult::InvalidExtension == amdExtensionUse)
    {
      result.isOk = false;
      result.logMessage += "dxil::compileHeaderFromShader: Invalid use of AMD extension\n";
      continue;
    }

    if (0 == boundResourceInfo.BindCount)
    {
      // bindless
      if (!is_supported_bindless_type(boundResourceInfo.Type))
      {
        result.isOk = false;
        result.logMessage += "dxil::compileHeaderFromShader: Use of bindless on a unsupported "
                             "resource type\n";
      }
      if (0 != boundResourceInfo.BindPoint)
      {
        result.isOk = false;
        result.logMessage += "dxil::compileHeaderFromShader: Bindless use on register index other "
                             "than 0\n";
      }

      if (result.isOk)
      {
        if (is_sampler_type(boundResourceInfo.Type))
        {
          if ((boundResourceInfo.Space < BINDLESS_SAMPLERS_SPACE_OFFSET) ||
              (boundResourceInfo.Space >= (BINDLESS_SAMPLERS_SPACE_OFFSET + BINDLESS_SAMPLERS_SPACE_COUNT)))
          {
            result.isOk = false;
            String msg(256,
              "dxil::compileHeaderFromShader: Bindless sampler used on space%u, which is "
              "not in range of space%u to space%u\n",
              boundResourceInfo.Space, BINDLESS_SAMPLERS_SPACE_OFFSET,
              BINDLESS_SAMPLERS_SPACE_OFFSET + BINDLESS_SAMPLERS_SPACE_COUNT - 1);
            result.logMessage += msg;
          }

          if (result.isOk)
          {
            result.header.resourceUsageTable.bindlessUsageMask |= 1u << (boundResourceInfo.Space - 1);
          }
        }
        else
        {
          if ((boundResourceInfo.Space < BINDLESS_RESOURCES_SPACE_OFFSET) ||
              (boundResourceInfo.Space >= (BINDLESS_RESOURCES_SPACE_OFFSET + BINDLESS_RESOURCES_SPACE_COUNT)))
          {
            result.isOk = false;
            String msg(256,
              "dxil::compileHeaderFromShader: Bindless resource used on space%u, which is "
              "not in range of space%u to space%u\n",
              boundResourceInfo.Space, BINDLESS_RESOURCES_SPACE_OFFSET,
              BINDLESS_RESOURCES_SPACE_OFFSET + BINDLESS_RESOURCES_SPACE_COUNT - 1);
            result.logMessage += msg;
          }

          if (result.isOk)
          {
            result.header.resourceUsageTable.bindlessUsageMask |= 1u << (boundResourceInfo.Space + 1);
          }
        }
      }
    }
    else
    {
      if (is_cbv_type(boundResourceInfo.Type))
      {
        if (boundResourceInfo.Space > 0)
        {
          if (boundResourceInfo.BindPoint == SPECIAL_CONSTANTS_REGISTER_INDEX)
          {
            if (boundResourceInfo.Space == DRAW_ID_REGISTER_SPACE)
            {
              result.header.resourceUsageTable.specialConstantsMask |= SC_DRAW_ID;
            }
            else
            {
              result.isOk = false;
              String msg(256, "dxil::compileHeaderFromShader: Special constants uses invalid space%i", boundResourceInfo.Space);
              result.logMessage += msg;
            }
          }
          else if (boundResourceInfo.BindPoint == ROOT_CONSTANT_BUFFER_REGISTER_INDEX)
          {
            if (boundResourceInfo.Space <= ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET)
            {
              result.isOk = false;
              String msg(256, "dxil::compileHeaderFromShader: Root constant buffer uses invalid space%i", boundResourceInfo.Space);
              result.logMessage += msg;
            }

            if (result.isOk)
            {
              result.header.resourceUsageTable.rootConstantDwords =
                boundResourceInfo.Space - ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET;
            }
          }
          else
          {
            result.isOk = false;
            String msg(256,
              "dxil::compileHeaderFromShader: Root constant buffer uses invalid b%i "
              "register",
              boundResourceInfo.BindPoint);
            result.logMessage += msg;
          }
        }
        else
        {
          if ((boundResourceInfo.BindPoint >= MAX_B_REGISTERS))
          {
            result.isOk = false;
            String msg(256,
              "dxil::compileHeaderFromShader: Const buffer register b%u out of range of b0 "
              "to b%u\n",
              boundResourceInfo.BindPoint, MAX_B_REGISTERS - 1);
            result.logMessage += msg;
          }

          if (result.isOk)
          {
            result.header.resourceUsageTable.bRegisterUseMask |= 1ul << boundResourceInfo.BindPoint;
          }
        }
      }
      else if (is_sampler_type(boundResourceInfo.Type))
      {
        if (boundResourceInfo.Space != REGULAR_RESOURCES_SPACE_INDEX)
        {
          result.isOk = false;
          String msg(256,
            "dxil::compileHeaderFromShader: Sampler space%u is invalid only space0 is "
            "valid for non-bindless resources",
            boundResourceInfo.Space);
          result.logMessage += msg;
        }
        if (boundResourceInfo.BindPoint >= MAX_T_REGISTERS)
        {
          result.isOk = false;
          String msg(256,
            "dxil::compileHeaderFromShader: Sampler register s%u out of range of s0 to "
            "s%u\n",
            boundResourceInfo.BindPoint, MAX_T_REGISTERS - 1);
          result.logMessage += msg;
        }
        if (result.isOk)
        {
          result.header.resourceUsageTable.sRegisterUseMask |= 1ul << boundResourceInfo.BindPoint;
          if (0 != (boundResourceInfo.uFlags & D3D_SIF_COMPARISON_SAMPLER))
            result.header.sRegisterCompareUseMask |= 1ul << boundResourceInfo.BindPoint;
        }
      }
      else if (is_srv_type(boundResourceInfo.Type))
      {
        if (boundResourceInfo.BindPoint >= MAX_T_REGISTERS)
        {
          result.isOk = false;
          String msg(256, "dxil::compileHeaderFromShader: SRV register t%u out of range of t0 to t%u\n", boundResourceInfo.BindPoint,
            MAX_T_REGISTERS - 1);
          result.logMessage += msg;
        }

        if (result.isOk)
        {
          for (UINT i = 0; i < boundResourceInfo.BindCount; ++i)
          {
            result.header.resourceUsageTable.tRegisterUseMask |= 1ul << (boundResourceInfo.BindPoint + i);
            result.header.tRegisterTypes[boundResourceInfo.BindPoint + i] =
              boundResourceInfo.Type | (boundResourceInfo.Dimension << 4);
          }
        }
      }
      else if (is_uav_type(boundResourceInfo.Type))
      {
        if (is_uav_with_counter_type(boundResourceInfo.Type))
        {
          result.isOk = false;
          result.logMessage += "dxil::compileHeaderFromShader: Use of UAV resource with embedded "
                               "counter, this is unsupported\n";
        }

        if (boundResourceInfo.BindPoint >= MAX_U_REGISTERS)
        {
          result.isOk = false;
          String msg(256, "dxil::compileHeaderFromShader: UAV register u%u out of range of u0 to u%u\n", boundResourceInfo.BindPoint,
            MAX_U_REGISTERS - 1);
          result.logMessage += msg;
        }
        if (result.isOk)
        {
          for (UINT i = 0; i < boundResourceInfo.BindCount; ++i)
          {
            result.header.resourceUsageTable.uRegisterUseMask |= 1ul << (boundResourceInfo.BindPoint + i);
            result.header.uRegisterTypes[boundResourceInfo.BindPoint + i] =
              boundResourceInfo.Type | (boundResourceInfo.Dimension << 4);
          }
        }
      }
    }
  }

  if (ShaderStage::VERTEX == stage)
  {
    for (UINT ipi = 0; ipi < desc.InputParameters; ++ipi)
    {
      D3D12_SIGNATURE_PARAMETER_DESC inputInfo = {};
      shaderInfo->GetInputParameterDesc(ipi, &inputInfo);
      if (inputInfo.SystemValueType == D3D_NAME_UNDEFINED)
      {
        uint32_t id = getIndexFromSementicAndSemanticIndex(inputInfo.SemanticName, inputInfo.SemanticIndex);
        if (id >= 32)
        {
          result.isOk = false;
          String msg(256, "Unable to map input semantic %s with index %u to a semantic index value\n", inputInfo.SemanticName,
            inputInfo.SemanticIndex);
          result.logMessage += msg;
        }
        if (result.isOk)
        {
          result.header.inOutSemanticMask |= 1ul << id;
        }
      }
    }
  }

  if (!stream_output_components.empty())
  {
    if (ShaderStage::PIXEL == stage || ShaderStage::DOMAIN == stage || ShaderStage::AMPLIFICATION == stage)
    {
      result.isOk = false;
      result.logMessage += "dxil::compileHeaderFromShader: Stream output is not supported from PIXEL, DOMAIN, AMPLIFICATION stages\n";
    }
    for (const auto comp : stream_output_components)
    {
      bool hasSemantic = false;
      for (UINT opi = 0; opi < desc.OutputParameters; ++opi)
      {
        D3D12_SIGNATURE_PARAMETER_DESC outputInfo = {};
        shaderInfo->GetOutputParameterDesc(opi, &outputInfo);
        if (strcmp(outputInfo.SemanticName, comp.semanticName) != 0 || outputInfo.SemanticIndex != comp.semanticIndex)
          continue;
        hasSemantic = true;
        if (outputInfo.Stream != 0)
        {
          result.isOk = false;
          String msg(256,
            "dxil::compileHeaderFromShader: Stream out is supported with stream 0 only now, semantic %s%d (stream: %d)\n",
            outputInfo.SemanticName, outputInfo.SemanticIndex, outputInfo.Stream);
          result.logMessage += msg;
          break;
        }
      }
      if (!hasSemantic)
      {
        result.isOk = false;
        String msg(256, "dxil::compileHeaderFromShader: Stream out semantic %s%d not found in the shader\n", comp.semanticName,
          comp.semanticIndex);
        result.logMessage += msg;
        break;
      }
      uint32_t numEntries = eastl::count_if(stream_output_components.begin(), stream_output_components.end(),
        [&comp](const dxil::StreamOutputComponentInfo &otherComp) {
          return strcmp(otherComp.semanticName, comp.semanticName) == 0 && otherComp.semanticIndex == comp.semanticIndex;
        });
      if (numEntries > 1)
      {
        result.isOk = false;
        String msg(256,
          "dxil::compileHeaderFromShader: Stream out semantic %s%d (slot: %d, mask: 0x%x) is used more than once, but only one SO "
          "entry is allowed per semantic (due to Xbox limitations)\n",
          comp.semanticName, comp.semanticIndex, comp.slot, comp.mask);
        result.logMessage += msg;
        break;
      }
      const uint8_t startComponent = __bsf(comp.mask);
      const uint8_t componentCount = __popcount(comp.mask);
      if (comp.mask != (((1 << componentCount) - 1) << startComponent))
      {
        result.isOk = false;
        String msg(256,
          "dxil::compileHeaderFromShader: Stream out semantic %s%d (slot: %d, mask: 0x%x): only consecutive components are supported "
          "for streamout (due to Xbox limitations)\n",
          comp.semanticName, comp.semanticIndex, comp.slot, comp.mask);
        result.logMessage += msg;
        break;
      }
    }
    result.streamOutputComponents = stream_output_components;
  }

  if (ShaderStage::PIXEL == stage)
  {
    for (UINT opi = 0; opi < desc.OutputParameters; ++opi)
    {
      D3D12_SIGNATURE_PARAMETER_DESC outputInfo = {};
      shaderInfo->GetOutputParameterDesc(opi, &outputInfo);
      if (outputInfo.SystemValueType == D3D_NAME_TARGET)
      {
        result.header.inOutSemanticMask |= uint32_t((~outputInfo.ReadWriteMask) & 0xF) << (outputInfo.SemanticIndex * 4);
      }
    }
    debug("PS output mask %08x", result.header.inOutSemanticMask);
  }

  if ((ShaderStage::COMPUTE == stage) || (ShaderStage::MESH == stage) || (ShaderStage::AMPLIFICATION == stage))
  {
    UINT total = shaderInfo->GetThreadGroupSize(&result.computeShaderInfo.threadGroupSizeX, &result.computeShaderInfo.threadGroupSizeY,
      &result.computeShaderInfo.threadGroupSizeZ);
    if (total < 1)
    {
      result.logMessage = "GetThreadGroupSize returned a product of group sizes of 0\n";
    }
    if (result.computeShaderInfo.threadGroupSizeX < 1)
    {
      result.isOk = false;
      result.logMessage += "GetThreadGroupSize returned for group size X of 0\n";
    }
    if (result.computeShaderInfo.threadGroupSizeY < 1)
    {
      result.isOk = false;
      result.logMessage += "GetThreadGroupSize returned for group size Y of 0\n";
    }
    if (result.computeShaderInfo.threadGroupSizeZ < 1)
    {
      result.isOk = false;
      result.logMessage += "GetThreadGroupSize returned for group size Z of 0\n";
    }
  }

  if (!result.isOk)
  {
    result.logMessage += make_bound_resource_table(desc, shaderInfo.Get());
    result.logMessage += "Error while building shader resource usage header.\n";
  }

#endif
  return result;
}

namespace
{
enum class ShaderFunctionRelfectionError
{
  NoError,
  NoDesc,
  InvalidBindlessResourceType,
  InvalidBindlessBindingPoint,
  BindlessSamplerSpaceOutOfRange,
  BindlessResourceSpaceOutOfRange,
  InvalidSpecialConstantRegister,
  InvalidRootConstantRegister,
  InvalidResourceSpace,
  BRegisterIndexOutOfRange,
  InvalidSamplerSpaceIndex,
  SRegisterIndexOutOfRange,
  TRegisterIndexOufOfRange,
  UAVHasEmbeddedCounter,
  URegisterIndexOutOfRange,
  NotYetImplemented,
};

template <typename T, typename U>
ShaderFunctionRelfectionError parse_resource_definition(const D3D12_SHADER_INPUT_BIND_DESC &desc, T &target, U &library_info)
{
  if (0 == desc.BindCount)
  {
    if (!is_supported_bindless_type(desc.Type))
    {
      return ShaderFunctionRelfectionError::InvalidBindlessResourceType;
    }
    if (0 != desc.BindPoint)
    {
      return ShaderFunctionRelfectionError::InvalidBindlessBindingPoint;
    }
    if (is_sampler_type(desc.Type))
    {
      if ((desc.Space < dxil::BINDLESS_SAMPLERS_SPACE_OFFSET) ||
          (desc.Space >= (dxil::BINDLESS_SAMPLERS_SPACE_OFFSET + dxil::BINDLESS_SAMPLERS_SPACE_COUNT)))
      {
        return ShaderFunctionRelfectionError::BindlessSamplerSpaceOutOfRange;
      }

      target.resourceUsageTable.bindlessUsageMask |= 1u << (desc.Space - 1);
      library_info.resourceUsageTable.bindlessUsageMask |= 1u << (desc.Space - 1);
    }
    else
    {
      if ((desc.Space < dxil::BINDLESS_RESOURCES_SPACE_OFFSET) ||
          (desc.Space >= (dxil::BINDLESS_RESOURCES_SPACE_OFFSET + dxil::BINDLESS_RESOURCES_SPACE_COUNT)))
      {
        return ShaderFunctionRelfectionError::BindlessResourceSpaceOutOfRange;
      }

      target.resourceUsageTable.bindlessUsageMask |= 1u << (desc.Space + 1);
      library_info.resourceUsageTable.bindlessUsageMask |= 1u << (desc.Space + 1);
    }
  }
  else
  {
    if (is_cbv_type(desc.Type))
    {
      if (desc.Space > 0)
      {
        if (desc.BindPoint == dxil::SPECIAL_CONSTANTS_REGISTER_INDEX)
        {
          if (desc.Space == dxil::DRAW_ID_REGISTER_SPACE)
          {
            target.resourceUsageTable.specialConstantsMask |= dxil::SC_DRAW_ID;
            library_info.resourceUsageTable.specialConstantsMask |= dxil::SC_DRAW_ID;
          }
          else
          {
            return ShaderFunctionRelfectionError::InvalidSpecialConstantRegister;
          }
        }
        else if (desc.BindPoint == dxil::ROOT_CONSTANT_BUFFER_REGISTER_INDEX)
        {
          if (desc.Space <= dxil::ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET)
          {
            return ShaderFunctionRelfectionError::InvalidRootConstantRegister;
          }

          target.resourceUsageTable.rootConstantDwords = desc.Space - dxil::ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET;
          library_info.resourceUsageTable.rootConstantDwords = desc.Space - dxil::ROOT_CONSTANT_BUFFER_REGISTER_SPACE_OFFSET;
        }
        else
        {
          return ShaderFunctionRelfectionError::InvalidResourceSpace;
        }
      }
      else
      {
        if ((desc.BindPoint >= dxil::MAX_B_REGISTERS))
        {
          return ShaderFunctionRelfectionError::BRegisterIndexOutOfRange;
        }

        target.resourceUsageTable.bRegisterUseMask |= 1ul << desc.BindPoint;
        library_info.resourceUsageTable.bRegisterUseMask |= 1ul << desc.BindPoint;
      }
    }
    else if (is_sampler_type(desc.Type))
    {
      if (desc.Space != dxil::REGULAR_RESOURCES_SPACE_INDEX)
      {
        return ShaderFunctionRelfectionError::InvalidSamplerSpaceIndex;
      }
      if (desc.BindPoint >= dxil::MAX_T_REGISTERS)
      {
        return ShaderFunctionRelfectionError::SRegisterIndexOutOfRange;
      }
      target.resourceUsageTable.sRegisterUseMask |= 1ul << desc.BindPoint;
      library_info.resourceUsageTable.sRegisterUseMask |= 1ul << desc.BindPoint;
      if (0 != (desc.uFlags & D3D_SIF_COMPARISON_SAMPLER))
      {
        library_info.sRegisterCompareUseMask |= 1ul << desc.BindPoint;
      }
    }
    else if (is_srv_type(desc.Type))
    {
      if (desc.BindPoint >= dxil::MAX_T_REGISTERS)
      {
        return ShaderFunctionRelfectionError::TRegisterIndexOufOfRange;
      }

      for (UINT i = 0; i < desc.BindCount; ++i)
      {
        target.resourceUsageTable.tRegisterUseMask |= 1ul << (desc.BindPoint + i);
        library_info.resourceUsageTable.tRegisterUseMask |= 1ul << (desc.BindPoint + i);
        library_info.tRegisterTypes[desc.BindPoint + i] = desc.Type | (desc.Dimension << 4);
      }
    }
    else if (is_uav_type(desc.Type))
    {
      if (is_uav_with_counter_type(desc.Type))
      {
        return ShaderFunctionRelfectionError::UAVHasEmbeddedCounter;
      }

      if (desc.BindPoint >= dxil::MAX_U_REGISTERS)
      {
        return ShaderFunctionRelfectionError::URegisterIndexOutOfRange;
      }
      for (UINT i = 0; i < desc.BindCount; ++i)
      {
        target.resourceUsageTable.uRegisterUseMask |= 1ul << (desc.BindPoint + i);
        library_info.resourceUsageTable.uRegisterUseMask |= 1ul << (desc.BindPoint + i);
        library_info.uRegisterTypes[desc.BindPoint + i] = desc.Type | (desc.Dimension << 4);
      }
    }
  }
  return ShaderFunctionRelfectionError::NoError;
}

uint16_t translate_shader_type(UINT version)
{
  dxil::ShaderStage value = dxil::ShaderStage::INVALID_STAGE;
  switch (version >> 16)
  {
    case 16: // explicit invalid case
    default: break;
    case 0: value = dxil::ShaderStage::PIXEL; break;
    case 1: value = dxil::ShaderStage::VERTEX; break;
    case 2: value = dxil::ShaderStage::GEOMETRY; break;
    case 3: value = dxil::ShaderStage::HULL; break;
    case 4: value = dxil::ShaderStage::DOMAIN; break;
    case 5: value = dxil::ShaderStage::COMPUTE; break;
    case 6: value = dxil::ShaderStage::LIBRARY; break;
    case 7: value = dxil::ShaderStage::RAY_GEN; break;
    case 8: value = dxil::ShaderStage::INTERSECTION; break;
    case 9: value = dxil::ShaderStage::ANY_HIT; break;
    case 10: value = dxil::ShaderStage::CLOSEST_HIT; break;
    case 11: value = dxil::ShaderStage::MISS; break;
    case 12: value = dxil::ShaderStage::CALLABLE; break;
    case 13: value = dxil::ShaderStage::MESH; break;
    case 14: value = dxil::ShaderStage::AMPLIFICATION; break;
    case 15: value = dxil::ShaderStage::NODE; break;
  }
  return static_cast<uint16_t>(value);
}

uint32_t get_shader_minimum_recursion(dxil::ShaderStage stage)
{
  // ray gen shader has to have at least a recursion of 1 to allow to shoot rays
  return (dxil::ShaderStage::RAY_GEN == stage) ? 1 : 0;
}

bool is_a_shader_with_recursion(dxil::ShaderStage stage)
{
  return (dxil::ShaderStage::RAY_GEN == stage) || (dxil::ShaderStage::CLOSEST_HIT == stage) || (dxil::ShaderStage::ANY_HIT == stage) ||
         (dxil::ShaderStage::MISS == stage) || (dxil::ShaderStage::CALLABLE == stage);
}

bool is_a_shader_with_payload(dxil::ShaderStage stage)
{
  return (dxil::ShaderStage::RAY_GEN == stage) || (dxil::ShaderStage::CLOSEST_HIT == stage) || (dxil::ShaderStage::ANY_HIT == stage) ||
         (dxil::ShaderStage::MISS == stage);
}

bool is_a_shader_with_attributes(dxil::ShaderStage stage)
{
  return (dxil::ShaderStage::CLOSEST_HIT == stage) || (dxil::ShaderStage::ANY_HIT == stage) ||
         (dxil::ShaderStage::INTERSECTION == stage);
}

ShaderFunctionRelfectionError relfect(uint32_t default_playload_size_in_bytes, const FunctionExtraDataQuery &function_extra_data_query,
  ID3D12FunctionReflection &function, LibraryResourceInformation &library_info, dxil::LibraryShaderProperties &output)
{
  D3D12_FUNCTION_DESC functionDesc{};
  if (FAILED(function.GetDesc(&functionDesc)))
  {
    return ShaderFunctionRelfectionError::NoDesc;
  }

  auto functionName = get_function_name(functionDesc);

  output.shaderType = translate_shader_type(functionDesc.Version);
  output.deviceRequirement.shaderFeatureFlagsLow = static_cast<uint32_t>(functionDesc.RequiredFeatureFlags);
  output.deviceRequirement.shaderFeatureFlagsHigh = static_cast<uint32_t>(functionDesc.RequiredFeatureFlags >> 32);
  output.deviceRequirement.shaderModel = functionDesc.Version & 0xFF;

  // currently we need to specify this values externally as there is no way to get them, other than decoding the DXIL
  // container and looking at disassembly strings.
  uint32_t recursionDepth = get_shader_minimum_recursion(static_cast<dxil::ShaderStage>(output.shaderType));
  uint32_t maxPayLoadSizeInBytes = default_playload_size_in_bytes;
  uint32_t maxAttributeSizeInBytes = sizeof(float) * 2;
  auto functionData = function_extra_data_query(static_cast<dxil::ShaderStage>(output.shaderType), functionName);
  if (functionData)
  {
    recursionDepth = functionData->recursionDepth;
    maxPayLoadSizeInBytes = functionData->maxPayLoadSizeInBytes;
    maxAttributeSizeInBytes = functionData->maxAttributeSizeInBytes;
  }
  output.maxRecusionDepth = is_a_shader_with_recursion(static_cast<dxil::ShaderStage>(output.shaderType)) ? recursionDepth : 0;
  output.maxPayloadSizeInBytes =
    is_a_shader_with_payload(static_cast<dxil::ShaderStage>(output.shaderType)) ? maxPayLoadSizeInBytes : 0;
  output.maxAttributeSizeInBytes =
    is_a_shader_with_attributes(static_cast<dxil::ShaderStage>(output.shaderType)) ? maxAttributeSizeInBytes : 0;

  if (functionDesc.BoundResources > 0)
  {
    for (UINT bi = 0; bi < functionDesc.BoundResources; ++bi)
    {
      D3D12_SHADER_INPUT_BIND_DESC boundResourceInfo{};
      function.GetResourceBindingDesc(bi, &boundResourceInfo);
      auto result = parse_resource_definition(boundResourceInfo, output, library_info);
      if (ShaderFunctionRelfectionError::NoError != result)
      {
        return result;
      }
    }
  }

  return ShaderFunctionRelfectionError::NoError;
}

void append_to_log(eastl::string &log, ShaderFunctionRelfectionError error)
{
  switch (error)
  {
    case ShaderFunctionRelfectionError::NoError: log += "No error"; break;
    case ShaderFunctionRelfectionError::NoDesc: log += "Unable to retrieve description"; break;
    case ShaderFunctionRelfectionError::InvalidBindlessResourceType: log += "Invalid bindless resource type"; break;
    case ShaderFunctionRelfectionError::InvalidBindlessBindingPoint: log += "Invalid bindless binding point"; break;
    case ShaderFunctionRelfectionError::BindlessSamplerSpaceOutOfRange: log += "Bindless samplers out of space range"; break;
    case ShaderFunctionRelfectionError::BindlessResourceSpaceOutOfRange: log += "Bindless resource out of space ranget"; break;
    case ShaderFunctionRelfectionError::InvalidSpecialConstantRegister: log += "Invalid special const register"; break;
    case ShaderFunctionRelfectionError::InvalidRootConstantRegister: log += "Invalid root const register"; break;
    case ShaderFunctionRelfectionError::InvalidResourceSpace: log += "Invalid resource space"; break;
    case ShaderFunctionRelfectionError::BRegisterIndexOutOfRange: log += "B register index out of range"; break;
    case ShaderFunctionRelfectionError::InvalidSamplerSpaceIndex: log += "Invalid sampler space index"; break;
    case ShaderFunctionRelfectionError::SRegisterIndexOutOfRange: log += "S register index out of range"; break;
    case ShaderFunctionRelfectionError::TRegisterIndexOufOfRange: log += "T register index out of range"; break;
    case ShaderFunctionRelfectionError::UAVHasEmbeddedCounter: log += "UAV resource has embedded counter"; break;
    case ShaderFunctionRelfectionError::URegisterIndexOutOfRange: log += "U register index out of range"; break;
    case ShaderFunctionRelfectionError::NotYetImplemented: log += "Functionality is not implemented yet"; break;
  }
}
} // namespace

dxil::LibraryShaderPropertiesCompileResult dxil::compileLibraryShaderPropertiesFromReflectionData(
  uint32_t default_playload_size_in_bytes, const FunctionExtraDataQuery &function_extra_data_query,
  eastl::span<const uint8_t> reflection, void *dxc_lib_handle)
{
  LibraryShaderPropertiesCompileResult result;

  if (reflection.empty())
  {
    result.isOk = false;
    result.logMessage = "Reflection blob was empty";
    return result;
  }

  if (!dxc_lib_handle)
  {
    result.isOk = false;
    result.logMessage = "DXC lib handle was null";
    return result;
  }
  auto utilitiesLoadResult = load_utilities(dxc_lib_handle);
  if (!utilitiesLoadResult)
  {
    result.isOk = false;
    result.logMessage = "Failed to create DXC utilities library";
    return result;
  }

  auto libInfo = create_reflection<ID3D12LibraryReflection>(utilitiesLoadResult.Get(), reflection);
  if (!libInfo)
  {
    result.isOk = false;
    result.logMessage = "Unable to create reflection interface from reflection blob";
    return result;
  }

  D3D12_LIBRARY_DESC libDesc{};
  libInfo->GetDesc(&libDesc);

  result.properties.resize(libDesc.FunctionCount);
  result.names.resize(libDesc.FunctionCount);
  for (uint32_t i = 0; i < libDesc.FunctionCount; ++i)
  {
    auto &target = result.properties[i];
    auto info = libInfo->GetFunctionByIndex(i);

    D3D12_FUNCTION_DESC functionDesc{};
    info->GetDesc(&functionDesc);

    auto functionName = get_function_name(functionDesc);
    if (functionName.empty())
    {
      result.logMessage += "Unable to obtain function name from mangled name ";
      result.logMessage += functionDesc.Name;
      result.isOk = false;
      continue;
    }
    result.names[i] = functionName;

    auto relfectionResult =
      relfect(default_playload_size_in_bytes, function_extra_data_query, *info, result.libInfo, result.properties[i]);
    if (ShaderFunctionRelfectionError::NoError != relfectionResult)
    {
      result.isOk = false;
      result.logMessage += "Error while reflecting function ";
      result.logMessage += functionDesc.Name;
      result.logMessage += ": ";
      append_to_log(result.logMessage, relfectionResult);
      continue;
    }
  }

  return result;
}