// clang-format off
#include <dxil/compiled_shader_header.h>
#include <dxil/compiler.h>

#include <util/dag_string.h>

#include <windows.h>
#include <unknwn.h>
#include <dxc/dxcapi.h>
#include "supplements/auto.h"
#include <debug/dag_debug.h>
// clang-format on

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

} // namespace

static const SemanticInfo semantic_remap[25] = //
  {{"POSITION", 0}, {"BLENDWEIGHT", 0}, {"BLENDINDICES", 0}, {"NORMAL", 0}, {"PSIZE", 0}, {"COLOR", 0}, {"COLOR", 1},
    {"TEXCOORD", 0}, // 7
    {"TEXCOORD", 1}, {"TEXCOORD", 2}, {"TEXCOORD", 3}, {"TEXCOORD", 4}, {"TEXCOORD", 5}, {"TEXCOORD", 6}, {"TEXCOORD", 7},
    {"POSITION", 1}, {"NORMAL", 1}, {"TEXCOORD", 15}, {"TEXCOORD", 8}, {"TEXCOORD", 9}, {"TEXCOORD", 10}, {"TEXCOORD", 11},
    {"TEXCOORD", 12}, {"TEXCOORD", 13}, {"TEXCOORD", 14}};

const SemanticInfo *dxil::getSemanticInfoFromIndex(uint32_t index)
{
  auto at = eastl::begin(semantic_remap) + index;
  if (at >= eastl::end(semantic_remap))
    return nullptr;
  return at;
}
// NOTE: for something like TEXCOORD1 the input has to be "TEXCOOORD" for name and 1 for index
uint32_t dxil::getIndexFromSementicAndSemanticIndex(const char *name, uint32_t index)
{
  auto ref = eastl::find_if(eastl::begin(semantic_remap), eastl::end(semantic_remap),
    [=](const SemanticInfo &info) { return info.index == index && 0 == strcmp(name, info.name); });

  if (ref != eastl::end(semantic_remap))
  {
    return ref - eastl::begin(semantic_remap);
  }
  return ~uint32_t(0);
}

ShaderHeaderCompileResult dxil::compileHeaderFromReflectionData(ShaderStage stage, const eastl::vector<uint8_t> &reflection,
  uint32_t max_const_count, uint32_t bone_const_used, void *dxc_lib)
{
  ShaderHeaderCompileResult result = {};
#if _TARGET_PC_WIN

  result.header.shaderType = static_cast<uint16_t>(stage);
  result.header.maxConstantCount = max_const_count;
  result.header.bonesConstantsUsed = bone_const_used;
  if (!dxc_lib)
  {
    result.logMessage = "dxil::compileHeaderFromShader: no valid shader compiler library handle "
                        "provided";
    result.isOk = false;
    return result;
  }

  DxcCreateInstanceProc DxcCreateInstance = nullptr;
  reinterpret_cast<FARPROC &>(DxcCreateInstance) = GetProcAddress(static_cast<HMODULE>(dxc_lib), "DxcCreateInstance");

  if (!DxcCreateInstance)
  {
    result.logMessage = "dxil::compileHeaderFromShader: Missing entry point DxcCreateInstance in "
                        "dxcompiler.dll";
    result.isOk = false;
    return result;
  }

  IDxcUtils *utilities = nullptr;
  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utilities));
  if (!utilities)
  {
    result.logMessage = "dxil::compileHeaderFromShader: DxcCreateInstance with CLSID_DxcUtils "
                        "failed";
    result.isOk = false;
    return result;
  }

  DxcBuffer blob;
  blob.Encoding = DXC_CP_ACP;
  blob.Ptr = reflection.data();
  blob.Size = reflection.size();

  ID3D12ShaderReflection *shaderInfo = nullptr;
  utilities->CreateReflection(&blob, IID_PPV_ARGS(&shaderInfo));
  if (!shaderInfo)
  {
    utilities->Release();
    result.logMessage = "dxil::compileHeaderFromShader: IDxcUtils::CreateReflection failed";
    result.isOk = false;
    return result;
  }

  result.isOk = true;
  D3D12_SHADER_DESC desc = {};
  shaderInfo->GetDesc(&desc);

  result.header.inputPrimitive = desc.InputPrimitive;

  for (UINT bri = 0; bri < desc.BoundResources; ++bri)
  {
    D3D12_SHADER_INPUT_BIND_DESC boundResourceInfo = {};
    shaderInfo->GetResourceBindingDesc(bri, &boundResourceInfo);

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
              boundResourceInfo.Space, BINDLESS_SAMPLERS_SPACE_OFFSET, BINDLESS_SAMPLERS_SPACE_OFFSET + BINDLESS_SAMPLERS_SPACE_COUNT);
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
    shaderInfo->GetThreadGroupSize(&result.computeShaderInfo.threadGroupSizeX, &result.computeShaderInfo.threadGroupSizeY,
      &result.computeShaderInfo.threadGroupSizeZ);
  }

  if (!result.isOk)
  {
    result.logMessage += make_bound_resource_table(desc, shaderInfo);
    result.logMessage += "Error while building shader resource usage header.\n";
  }

  shaderInfo->Release();
  utilities->Release();

#endif
  return result;
}
