// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shader.h"
#include "d3d12_error_handling.h"
#include "device_context.h"
#include "pipeline/blk_cache.h"

#include <EASTL/fixed_vector.h>

using namespace drv3d_dx12;

#define g_main debug_pixel_shader
#if _TARGET_XBOXONE
#include "shaders/debug.ps.x.h"
#elif _TARGET_SCARLETT
#include "shaders/debug.ps.xs.h"
#else
#include "shaders/debug.ps.h"
#endif
#undef g_main

#define g_main debug_vertex_shader
#if _TARGET_XBOXONE
#include "shaders/debug.vs.x.h"
#elif _TARGET_SCARLETT
#include "shaders/debug.vs.xs.h"
#else
#include "shaders/debug.vs.h"
#endif
#undef g_main

#define g_main null_pixel_shader
#if _TARGET_XBOXONE
#include "shaders/null.ps.x.h"
#elif _TARGET_SCARLETT
#include "shaders/null.ps.xs.h"
#else
#include "shaders/null.ps.h"
#endif
#undef g_main

bool InputLayout::fromVdecl(DecodeContext &context, const VSDTYPE &decl)
{
  if (VSD_END == decl)
  {
    return false;
  }

  const auto op = decl & VSDOP_MASK;
  if (op == VSDOP_INPUT)
  {
    if (decl & VSD_SKIPFLG)
    {
      context.ofs += GET_VSDSKIP(decl) * 4;
      return true;
    }

    const auto data = decl & VSDT_MASK;

    uint32_t locationIndex = GET_VSDREG(decl);
    useLocation(locationIndex);
    setLocationStreamSource(locationIndex, context.streamIndex);
    setLocationStreamOffset(locationIndex, context.ofs);
    setLocationFormatIndex(locationIndex, data);

    uint32_t sz = 0; // size of entry
    //-V::1037
    switch (data)
    {
      case VSDT_FLOAT1: sz = 32; break;
      case VSDT_FLOAT2: sz = 32 + 32; break;
      case VSDT_FLOAT3: sz = 32 + 32 + 32; break;
      case VSDT_FLOAT4: sz = 32 + 32 + 32 + 32; break;
      case VSDT_INT1: sz = 32; break;
      case VSDT_INT2: sz = 32 + 32; break;
      case VSDT_INT3: sz = 32 + 32 + 32; break;
      case VSDT_INT4: sz = 32 + 32 + 32 + 32; break;
      case VSDT_UINT1: sz = 32; break;
      case VSDT_UINT2: sz = 32 + 32; break;
      case VSDT_UINT3: sz = 32 + 32 + 32; break;
      case VSDT_UINT4: sz = 32 + 32 + 32 + 32; break;
      case VSDT_HALF2: sz = 16 + 16; break;
      case VSDT_SHORT2N: sz = 16 + 16; break;
      case VSDT_SHORT2: sz = 16 + 16; break;
      case VSDT_USHORT2N: sz = 16 + 16; break;

      case VSDT_HALF4: sz = 16 + 16 + 16 + 16; break;
      case VSDT_SHORT4N: sz = 16 + 16 + 16 + 16; break;
      case VSDT_SHORT4: sz = 16 + 16 + 16 + 16; break;
      case VSDT_USHORT4N: sz = 16 + 16 + 16 + 16; break;

      case VSDT_UDEC3: sz = 10 + 10 + 10 + 2; break;
      case VSDT_DEC3N: sz = 10 + 10 + 10 + 2; break;

      case VSDT_E3DCOLOR: sz = 8 + 8 + 8 + 8; break;
      case VSDT_UBYTE4: sz = 8 + 8 + 8 + 8; break;
      default: G_ASSERTF_RETURN(false, false, "invalid vertex declaration type 0x%08X", data); break;
    }
    context.ofs += sz / 8;
  }
  else if (op == VSDOP_STREAM)
  {
    context.streamIndex = GET_VSDSTREAM(decl);
    context.ofs = 0;
    useStream(context.streamIndex);
    if (decl & VSDS_PER_INSTANCE_DATA)
    {
      setStreamStepRate(context.streamIndex, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA);
    }
    else
    {
      setStreamStepRate(context.streamIndex, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
    }
  }
  else
  {
    G_ASSERTF_RETURN(0, false, "Invalid vsd opcode 0x%08X", decl);
  }

  return true;
}

StageShaderModule drv3d_dx12::shader_layout_to_module(const bindump::Mapper<dxil::Shader> *layout)
{
  StageShaderModule result;
  result.header = layout->shaderHeader;
  result.byteCode = eastl::make_unique<uint8_t[]>(layout->bytecode.size());
  eastl::copy(layout->bytecode.begin(), layout->bytecode.end(), result.byteCode.get());
  result.byteCodeSize = layout->bytecode.size();
  return result;
}

StageShaderModule drv3d_dx12::decode_shader_binary(const void *data, uint32_t size)
{
  StageShaderModule result;
  // TODO use size bounds checking
  G_UNUSED(size);
  auto fileHeader = reinterpret_cast<const dxil::FileHeader *>(data);
  auto chunkHeaders = reinterpret_cast<const dxil::ChunkHeader *>(fileHeader + 1);
  auto dataStart = reinterpret_cast<const uint8_t *>(chunkHeaders + fileHeader->chunkCount);
  const dxil::ShaderHeader *shaderHeader = nullptr;
  const uint8_t *shaderModule = nullptr;
  const char *shaderName = nullptr;
  size_t shaderNameLength = 0;
  uint32_t shaderModuleSize = 0;
  dxil::HashValue shaderModuleHash = {};
  // NOTE redo shader binary format from chunk based to fixed layout with bits to indicate what is available.
  for (auto &&header : make_span(chunkHeaders, fileHeader->chunkCount))
  {
    switch (header.type)
    {
      case dxil::ChunkType::SHADER_HEADER:
        shaderHeader = reinterpret_cast<decltype(shaderHeader)>(dataStart + header.offset);
        if (header.hash != dxil::HashValue::calculate(shaderHeader, 1))
        {
          D3D_ERROR("DX12: Error while decoding shader, shader header hash does not match");
          return result;
        }
        break;
      case dxil::ChunkType::DXIL:
        if (!shaderModule)
        {
          shaderModuleHash = header.hash;
          shaderModule = dataStart + header.offset;
          shaderModuleSize = header.size;
          if (header.hash != dxil::HashValue::calculate(shaderModule, shaderModuleSize))
          {
            D3D_ERROR("DX12: Error while decoding shader, shader module hash does not match");
            return result;
          }
        }
        break;
      case dxil::ChunkType::DXBC: logwarn("DX12: DXBC shader chunk seen while deconding a shader module"); break;
      case dxil::ChunkType::SHADER_NAME:
        shaderName = reinterpret_cast<const char *>(dataStart + header.offset);
        shaderNameLength = header.size;
        if (header.hash != dxil::HashValue::calculate(shaderName, shaderNameLength))
        {
          // non fatal as its just some extra info
          logwarn("DX12: Error while decoding shader, shader name hash does not match");
          shaderName = nullptr;
          shaderNameLength = 0;
        }
        break;
      default:
        D3D_ERROR("DX12: Error while decoding shader, unrecognized chunk header id %u", static_cast<uint32_t>(header.type));
        return result;
        break;
    }
  }

  if (!shaderHeader)
  {
    D3D_ERROR("DX12: Error while decoding shader, unable to locate header chunk");
    return result;
  }
  if (!shaderModule)
  {
    D3D_ERROR("DX12: Error while decoding shader, unable to locate shader module chunk");
    return result;
  }

  result.ident.shaderHash = shaderModuleHash;
  result.ident.shaderSize = shaderModuleSize;
  result.header = *shaderHeader;
  result.byteCode = eastl::make_unique<uint8_t[]>(shaderModuleSize);
  eastl::copy(shaderModule, shaderModule + shaderModuleSize, result.byteCode.get());
  result.byteCodeSize = shaderModuleSize;
  if (shaderName)
    result.debugName.assign(shaderName, shaderName + shaderNameLength);
  return result;
}

eastl::unique_ptr<VertexShaderModule> drv3d_dx12::decode_vertex_shader(const void *data, uint32_t size)
{
  eastl::unique_ptr<VertexShaderModule> vs;
  auto fileHeader = reinterpret_cast<const dxil::FileHeader *>(data);
  if (fileHeader->ident == dxil::COMBINED_SHADER_UNCOMPRESSED_IDENT)
  {
    auto sectionChunksHeaders = reinterpret_cast<const dxil::CombinedChunk *>(fileHeader + 1);
    auto dataStart = reinterpret_cast<const uint8_t *>(sectionChunksHeaders + fileHeader->chunkCount);

    eastl::unique_ptr<StageShaderModule> gs, hs, ds;
    for (auto &&sHeader : eastl::span<const dxil::CombinedChunk>(sectionChunksHeaders, fileHeader->chunkCount))
    {
      auto basicModule = decode_shader_binary(dataStart + sHeader.offset, sHeader.size);
      if (!basicModule)
      {
        vs.reset();
        break;
      }
      auto shaderType = static_cast<dxil::ShaderStage>(basicModule.header.shaderType);
      switch (shaderType)
      {
#if !_TARGET_XBOXONE
        case dxil::ShaderStage::MESH:
#endif
        case dxil::ShaderStage::VERTEX: vs = eastl::make_unique<VertexShaderModule>(eastl::move(basicModule)); break;
#if !_TARGET_XBOXONE
        case dxil::ShaderStage::AMPLIFICATION:
#endif
        case dxil::ShaderStage::GEOMETRY: gs = eastl::make_unique<StageShaderModule>(eastl::move(basicModule)); break;
        case dxil::ShaderStage::DOMAIN: ds = eastl::make_unique<StageShaderModule>(eastl::move(basicModule)); break;
        case dxil::ShaderStage::HULL: hs = eastl::make_unique<StageShaderModule>(eastl::move(basicModule)); break;
        case dxil::ShaderStage::PIXEL:
        case dxil::ShaderStage::COMPUTE:
        default:
          D3D_ERROR("DX12: Error while decoding vertex shader, unexpected combined shader stage type "
                    "%u",
            basicModule.header.shaderType);
          return vs;
      }
    }

    if (vs)
    {
      vs->geometryShader = eastl::move(gs);
      vs->hullShader = eastl::move(hs);
      vs->domainShader = eastl::move(ds);
    }
  }
  else if (fileHeader->ident == dxil::SHADER_UNCOMPRESSED_IDENT)
  {
    auto basicModule = decode_shader_binary(data, size);
    if (basicModule)
    {
      vs = eastl::make_unique<VertexShaderModule>(eastl::move(basicModule));
    }
  }
  else
  {
    auto basicModule = decode_shader_layout<VertexShaderModule>((const uint8_t *)data);
    if (basicModule)
    {
      vs = eastl::make_unique<VertexShaderModule>(eastl::move(basicModule));
    }
    else
    {
      D3D_ERROR("DX12: Error while decoding vertex shader, unexpected shader identifier 0x%08X", fileHeader->ident);
    }
  }

  return vs;
}

eastl::unique_ptr<PixelShaderModule> drv3d_dx12::decode_pixel_shader(const void *data, uint32_t size)
{
  eastl::unique_ptr<PixelShaderModule> ps;
  auto fileHeader = reinterpret_cast<const dxil::FileHeader *>(data);
  if (fileHeader->ident == dxil::SHADER_UNCOMPRESSED_IDENT)
  {
    auto basicModule = decode_shader_binary(data, size);
    if (basicModule)
    {
      ps = eastl::make_unique<PixelShaderModule>(eastl::move(basicModule));
    }
  }
  else
  {
    auto basicModule = decode_shader_layout<PixelShaderModule>((const uint8_t *)data);
    if (basicModule)
    {
      ps = eastl::make_unique<PixelShaderModule>(eastl::move(basicModule));
    }
    else
    {
      D3D_ERROR("DX12: Error while decoding pixel shader, unexpected shader identifier 0x%08X", fileHeader->ident);
    }
  }
  return ps;
}

StageShaderModuleInBinaryRef drv3d_dx12::shader_layout_to_module_ref(const bindump::Mapper<dxil::Shader> *layout)
{
  StageShaderModuleInBinaryRef result;
  result.header = layout->shaderHeader;
  result.byteCode = {layout->bytecode.begin(), layout->bytecode.end()};
  return result;
}

StageShaderModuleInBinaryRef drv3d_dx12::decode_shader_binary_ref(const void *data, uint32_t size)
{
  StageShaderModuleInBinaryRef result;
  // TODO use size bounds checking
  G_UNUSED(size);
  auto fileHeader = reinterpret_cast<const dxil::FileHeader *>(data);
  auto chunkHeaders = reinterpret_cast<const dxil::ChunkHeader *>(fileHeader + 1);
  auto dataStart = reinterpret_cast<const uint8_t *>(chunkHeaders + fileHeader->chunkCount);
  const dxil::ShaderHeader *shaderHeader = nullptr;
  const uint8_t *shaderModule = nullptr;
  const char *shaderName = nullptr;
  size_t shaderNameLength = 0;
  uint32_t shaderModuleSize = 0;
  dxil::HashValue shaderModuleHash = {};
  // NOTE redo shader binary format from chunk based to fixed layout with bits to indicate what is available.
  for (auto &&header : make_span(chunkHeaders, fileHeader->chunkCount))
  {
    switch (header.type)
    {
      case dxil::ChunkType::SHADER_HEADER:
        shaderHeader = reinterpret_cast<decltype(shaderHeader)>(dataStart + header.offset);
        if (header.hash != dxil::HashValue::calculate(shaderHeader, 1))
        {
          D3D_ERROR("DX12: Error while decoding shader, shader header hash does not match");
          return result;
        }
        break;
      case dxil::ChunkType::DXIL:
        if (!shaderModule)
        {
          shaderModuleHash = header.hash;
          shaderModule = dataStart + header.offset;
          shaderModuleSize = header.size;
          if (header.hash != dxil::HashValue::calculate(shaderModule, shaderModuleSize))
          {
            D3D_ERROR("DX12: Error while decoding shader, shader module hash does not match");
            return result;
          }
        }
        break;
      case dxil::ChunkType::DXBC: logwarn("DX12: DXBC shader chunk seen while deconding a shader module"); break;
      case dxil::ChunkType::SHADER_NAME:
        shaderName = reinterpret_cast<const char *>(dataStart + header.offset);
        shaderNameLength = header.size;
        if (header.hash != dxil::HashValue::calculate(shaderName, shaderNameLength))
        {
          // non fatal as its just some extra info
          logwarn("DX12: Error while decoding shader, shader name hash does not match");
          shaderName = nullptr;
          shaderNameLength = 0;
        }
        break;
      default:
        D3D_ERROR("DX12: Error while decoding shader, unrecognized chunk header id %u", static_cast<uint32_t>(header.type));
        return result;
        break;
    }
  }

  if (!shaderHeader)
  {
    D3D_ERROR("DX12: Error while decoding shader, unable to locate header chunk");
    return result;
  }
  if (!shaderModule)
  {
    D3D_ERROR("DX12: Error while decoding shader, unable to locate shader module chunk");
    return result;
  }

  result.ident.shaderHash = shaderModuleHash;
  result.ident.shaderSize = shaderModuleSize;
  result.header = *shaderHeader;
  result.byteCode = {shaderModule, shaderModule + shaderModuleSize};
  if (shaderName)
    result.debugName.assign(shaderName, shaderName + shaderNameLength);
  return result;
}

VertexShaderModuleInBinaryRef drv3d_dx12::decode_vertex_shader_ref(const void *data, uint32_t size)
{
  VertexShaderModuleInBinaryRef vs;
  auto fileHeader = reinterpret_cast<const dxil::FileHeader *>(data);
  if (fileHeader->ident == dxil::COMBINED_SHADER_UNCOMPRESSED_IDENT)
  {
    auto sectionChunksHeaders = reinterpret_cast<const dxil::CombinedChunk *>(fileHeader + 1);
    auto dataStart = reinterpret_cast<const uint8_t *>(sectionChunksHeaders + fileHeader->chunkCount);

    auto &gs = vs.geometryShader;
    auto &hs = vs.hullShader;
    auto &ds = vs.domainShader;
    for (auto &&sHeader : eastl::span<const dxil::CombinedChunk>(sectionChunksHeaders, fileHeader->chunkCount))
    {
      auto basicModule = decode_shader_binary_ref(dataStart + sHeader.offset, sHeader.size);
      if (basicModule.byteCode.empty())
      {
        vs = {};
        break;
      }
      auto shaderType = static_cast<dxil::ShaderStage>(basicModule.header.shaderType);
      switch (shaderType)
      {
#if !_TARGET_XBOXONE
        case dxil::ShaderStage::MESH:
#endif
        case dxil::ShaderStage::VERTEX: static_cast<StageShaderModuleInBinaryRef &>(vs) = basicModule; break;
#if !_TARGET_XBOXONE
        case dxil::ShaderStage::AMPLIFICATION:
#endif
        case dxil::ShaderStage::GEOMETRY: gs = basicModule; break;
        case dxil::ShaderStage::DOMAIN: ds = basicModule; break;
        case dxil::ShaderStage::HULL: hs = basicModule; break;
        case dxil::ShaderStage::PIXEL:
        case dxil::ShaderStage::COMPUTE:
        default:
          D3D_ERROR("DX12: Error while decoding vertex shader, unexpected combined shader stage type "
                    "%u",
            basicModule.header.shaderType);
          return vs;
      }
    }
  }
  else if (fileHeader->ident == dxil::SHADER_UNCOMPRESSED_IDENT)
  {
    static_cast<StageShaderModuleInBinaryRef &>(vs) = decode_shader_binary_ref(data, size);
  }
  else
  {
    vs = decode_shader_layout_ref<VertexShaderModuleInBinaryRef>((const uint8_t *)data);
  }
  if (vs.byteCode.empty())
  {
    D3D_ERROR("DX12: Error while decoding vertex shader, unexpected shader identifier 0x%08X", fileHeader->ident);
  }

  return vs;
}

PixelShaderModuleInBinaryRef drv3d_dx12::decode_pixel_shader_ref(const void *data, uint32_t size)
{
  PixelShaderModuleInBinaryRef result;
  auto fileHeader = reinterpret_cast<const dxil::FileHeader *>(data);
  if (fileHeader->ident == dxil::SHADER_UNCOMPRESSED_IDENT)
  {
    result = decode_shader_binary_ref(data, size);
  }
  else
  {
    result = decode_shader_layout_ref<PixelShaderModuleInBinaryRef>((const uint8_t *)data);
  }
  if (result.byteCode.empty())
  {
    D3D_ERROR("DX12: Error while decoding pixel shader, unexpected shader identifier 0x%08X", fileHeader->ident);
  }
  return result;
}

void ShaderProgramDatabase::initDebugProgram(DeviceContext &ctx)
{
  dxil::ShaderHeader debugVSHeader = {};
  debugVSHeader.shaderType = static_cast<uint16_t>(dxil::ShaderStage::VERTEX);
  debugVSHeader.inOutSemanticMask = 1ul << dxil::getIndexFromSementicAndSemanticIndex("POSITION", 0);
  debugVSHeader.inOutSemanticMask |= 1ul << dxil::getIndexFromSementicAndSemanticIndex("COLOR", 0);
  debugVSHeader.resourceUsageTable.bRegisterUseMask = 1ul << 0;

  dxil::ShaderHeader debugPSHeader = {};
  debugPSHeader.shaderType = static_cast<uint16_t>(dxil::ShaderStage::PIXEL);
  debugPSHeader.inOutSemanticMask = 0x0000000F;

  VSDTYPE ilDefAry[] = //
    {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT3), VSD_REG(VSDR_DIFF, VSDT_E3DCOLOR), VSD_END};
  InputLayout ilDef;
  ilDef.fromVdecl(ilDefAry);

  auto vs = newRawVertexShader(ctx, debugVSHeader, make_span(debug_vertex_shader, sizeof(debug_vertex_shader)));
  auto ps = newRawPixelShader(ctx, debugPSHeader, make_span(debug_pixel_shader, sizeof(debug_pixel_shader)));
  auto il = registerInputLayoutInternal(ctx, ilDef);

  debugProgram = newGraphicsProgram(ctx, il, vs, ps);
}

void ShaderProgramDatabase::initNullPixelShader(DeviceContext &ctx)
{
  dxil::ShaderHeader nullHeader = {};
  nullHeader.shaderType = static_cast<uint16_t>(dxil::ShaderStage::PIXEL);
  auto nullShader = make_span(null_pixel_shader, countof(null_pixel_shader));
  auto nPSH = newRawPixelShader(ctx, nullHeader, nullShader);
  nullPixelShader = nPSH;
}

ShaderID ShaderProgramDatabase::newRawVertexShader(DeviceContext &ctx, const dxil::ShaderHeader &header,
  dag::ConstSpan<uint8_t> byte_code)
{
  auto module = eastl::make_unique<VertexShaderModule>();
  module->ident.shaderHash = dxil::HashValue::calculate(byte_code.data(), byte_code.size());
  module->ident.shaderSize = static_cast<uint32_t>(byte_code.size());
  module->header = header;
  module->byteCode = eastl::make_unique<uint8_t[]>(byte_code.size());
  eastl::copy(byte_code.begin(), byte_code.end(), module->byteCode.get());
  module->byteCodeSize = byte_code.size();
  auto id = shaderProgramGroups.addVertexShader();
  ctx.addVertexShader(id, eastl::move(module));
  return id;
}

ShaderID ShaderProgramDatabase::newRawPixelShader(DeviceContext &ctx, const dxil::ShaderHeader &header,
  dag::ConstSpan<uint8_t> byte_code)
{
  auto module = eastl::make_unique<PixelShaderModule>();
  module->ident.shaderHash = dxil::HashValue::calculate(byte_code.data(), byte_code.size());
  module->ident.shaderSize = static_cast<uint32_t>(byte_code.size());
  module->header = header;
  module->byteCode = eastl::make_unique<uint8_t[]>(byte_code.size());
  eastl::copy(byte_code.begin(), byte_code.end(), module->byteCode.get());
  module->byteCodeSize = byte_code.size();
  auto id = shaderProgramGroups.addPixelShader();
  ctx.addPixelShader(id, eastl::move(module));
  return id;
}

ProgramID ShaderProgramDatabase::newComputeProgram(DeviceContext &ctx, const void *data, CSPreloaded preloaded)
{
  auto basicModule = decode_shader_layout<ComputeShaderModule>((const uint8_t *)data);
  if (!basicModule)
  {
    basicModule = decode_shader_binary(data, ~uint32_t(0));
    if (!basicModule)
    {
      return ProgramID::Null();
    }
  }

  auto module = eastl::make_unique<ComputeShaderModule>(eastl::move(basicModule));

  ProgramID program;
  {
    ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
    program = shaderProgramGroups.addComputeShaderProgram();
  }
  ctx.addComputeProgram(program, eastl::move(module), preloaded);

  return program;
}

ProgramID ShaderProgramDatabase::newGraphicsProgram(DeviceContext &ctx, InputLayoutID vdecl, ShaderID vs, ShaderID ps)
{
  const auto key = shaderProgramGroups.getGraphicsProgramKey(vdecl, vs, ps);
  {
    ScopedLockReadTemplate<OSReadWriteLock> lock(dataGuard);
    ProgramID hashed = shaderProgramGroups.incrementCachedGraphicsProgram(key);
    if (hashed != ProgramID::Null())
      return hashed;
  }
  ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);

  // null pixel shader has to be patched here
  if (ps == ShaderID::Null())
  {
    ps = nullPixelShader;
  }

  GraphicsProgramID gid = shaderProgramGroups.findGraphicsProgram(vs, ps);
  if (GraphicsProgramID::Null() == gid)
  {
    gid = shaderProgramGroups.addGraphicsProgram(vs, ps);

    dataGuard.unlockWrite();
    ctx.addGraphicsProgram(gid, vs, ps);
    dataGuard.lockWrite();
  }
  // kick off loading of grouped graphics pipeline
  else if (gid.getGroup() != 0)
  {
    dataGuard.unlockWrite();
    ctx.addGraphicsProgram(gid, vs, ps);
    dataGuard.lockWrite();
  }

  return shaderProgramGroups.instanciateGraphicsProgram(key, gid, vdecl);
}

InputLayoutID ShaderProgramDatabase::getInputLayoutForGraphicsProgram(ProgramID program)
{
  return shaderProgramGroups.getGraphicsProgramInstanceInputLayout(program);
}

GraphicsProgramUsageInfo ShaderProgramDatabase::getGraphicsProgramForStateUpdate(ProgramID program)
{
  ScopedLockReadTemplate<OSReadWriteLock> lock(dataGuard);
  return shaderProgramGroups.getUsageInfo(program);
}

InputLayoutID ShaderProgramDatabase::registerInputLayoutInternal(DeviceContext &ctx, const InputLayout &layout)
{
  auto ref = eastl::find(begin(publicImputLayoutTable), end(publicImputLayoutTable), layout);

  InputLayoutID ident;
  if (ref == end(publicImputLayoutTable))
  {
    ref = publicImputLayoutTable.emplace(ref, layout);
    ident = InputLayoutID{int(ref - begin(publicImputLayoutTable))};
    ctx.registerInputLayout(ident, layout);
  }
  else
  {
    ident = InputLayoutID{int(ref - begin(publicImputLayoutTable))};
  }

  return ident;
}

InputLayoutID ShaderProgramDatabase::registerInputLayout(DeviceContext &ctx, const InputLayout &layout)
{
  ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
  return registerInputLayoutInternal(ctx, layout);
}

void ShaderProgramDatabase::setup(DeviceContext &ctx, bool disable_precache)
{
  disablePreCache = disable_precache;

  initNullPixelShader(ctx);
  initDebugProgram(ctx);
}

void ShaderProgramDatabase::shutdown(DeviceContext &ctx)
{
  STORE_RETURN_ADDRESS();
  ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);

  shaderProgramGroups.iterateAllGraphicsPrograms([&](GraphicsProgramID gp) { ctx.removeGraphicsProgram(gp); });

  shaderProgramGroups.iterateAllComputeShaders([&](ProgramID prog) { ctx.removeProgram(prog); });

  shaderProgramGroups.itarateAllVertexShaders([&](ShaderID shader) { ctx.removeVertexShader(shader); });

  shaderProgramGroups.iterateAllPixelShaders([&](ShaderID shader) { ctx.removePixelShader(shader); });

  shaderProgramGroups.reset();

  publicImputLayoutTable.clear();

  debugProgram = ProgramID::Null();
  nullPixelShader = ShaderID::Null();
}

ShaderID ShaderProgramDatabase::newVertexShader(DeviceContext &ctx, const void *data)
{
  auto vs = decode_vertex_shader(data, ~uint32_t(0));

  ShaderID id = ShaderID::Null();
  if (vs)
  {
    {
      ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
      id = shaderProgramGroups.addVertexShader();
    }
    ctx.addVertexShader(id, eastl::move(vs));
  }
  return id;
}

ShaderID ShaderProgramDatabase::newPixelShader(DeviceContext &ctx, const void *data)
{
  auto ps = decode_pixel_shader(data, ~uint32_t(0));

  ShaderID id = ShaderID::Null();
  if (ps)
  {
    {
      ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
      id = shaderProgramGroups.addPixelShader();
    }

    ctx.addPixelShader(id, eastl::move(ps));
  }
  return id;
}

ProgramID ShaderProgramDatabase::getDebugProgram() { return debugProgram; }

void ShaderProgramDatabase::removeProgram(DeviceContext &ctx, ProgramID prog)
{
  ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);

  if (prog.isGraphics())
  {
    shaderProgramGroups.removeGraphicsProgramInstance(prog);
  }
  else if (prog.isCompute())
  {
    // only remove compute programs if not part of a shader group
    if (0 == prog.getGroup())
    {
      shaderProgramGroups.removeComputeProgram(prog);
      ctx.removeProgram(prog);
    }
  }
}

void ShaderProgramDatabase::deleteVertexShader(DeviceContext &ctx, ShaderID shader)
{
  // we never delete shaders of a bindump
  if (shader.getGroup() > 0)
    return;

  eastl::fixed_vector<GraphicsProgramID, 2, true> toRemove;
  {
    ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
    shaderProgramGroups.removeVertexShader(shader);
    shaderProgramGroups.removeGraphicsProgramWithMatchingVertexShader(shader, [this, &toRemove](GraphicsProgramID gpid) //
      {
        toRemove.push_back(gpid);
        shaderProgramGroups.removeGraphicsProgramInstancesUsingMatchingTemplate(gpid);
      });
  }

  ctx.removeVertexShader(shader);
  for (auto gpid : toRemove)
    ctx.removeGraphicsProgram(gpid);
}

void ShaderProgramDatabase::deletePixelShader(DeviceContext &ctx, ShaderID shader)
{
  // we never delete shaders of a bindump
  if (shader.getGroup() > 0)
    return;

  eastl::fixed_vector<GraphicsProgramID, 2, true> toRemove;
  {
    ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
    shaderProgramGroups.removePixelShader(shader);
    shaderProgramGroups.removeGraphicsProgramWithMatchingPixelShader(shader, [this, &toRemove](GraphicsProgramID gpid) //
      {
        toRemove.push_back(gpid);
        shaderProgramGroups.removeGraphicsProgramInstancesUsingMatchingTemplate(gpid);
      });
  }
  ctx.removePixelShader(shader);
  for (auto gpid : toRemove)
    ctx.removeGraphicsProgram(gpid);
}

void ShaderProgramDatabase::updateVertexShaderName(DeviceContext &ctx, ShaderID shader, const char *name)
{
  ctx.updateVertexShaderName(shader, name);
}

void ShaderProgramDatabase::updatePixelShaderName(DeviceContext &ctx, ShaderID shader, const char *name)
{
  ctx.updatePixelShaderName(shader, name);
}

void ShaderProgramDatabase::registerShaderBinDump(DeviceContext &ctx, ScriptedShadersBinDumpOwner *dump, const char *name)
{
  if (!dump)
  {
    shaderProgramGroups.dropGroup(1);
    ctx.removeShaderGroup(1);
    // wait until backend has processed the removal
    ctx.finish();
    return;
  }

  logdbg("DX12: registerShaderBinDump %p <%s>", dump, name);
  if (ShaderID::Null() == nullPixelShader)
  {
    initNullPixelShader(ctx);
  }

  shaderProgramGroups.setGroup(1, dump, nullPixelShader);
  ctx.addShaderGroup(1, dump, nullPixelShader, name);
}

void ShaderProgramDatabase::getBindumpShader(DeviceContext &ctx, uint32_t index, ShaderCodeType type, void *ident)
{
  auto stage = shaderProgramGroups.shaderFromIndex(1, index, type, ident);
  if (dxil::ShaderStage::COMPUTE == stage)
  {
    ctx.loadComputeShaderFromDump(ProgramID::importValue(*static_cast<FSHADER *>(ident)));
  }
}

DynamicArray<InputLayoutIDWithHash> ShaderProgramDatabase::loadInputLayoutFromBlk(DeviceContext &ctx, const DataBlock *blk,
  const char *default_format)
{
  DynamicArray<InputLayoutIDWithHash> result{blk->blockCount()};
  pipeline::DataBlockDecodeEnumarator<pipeline::InputLayoutDecoder> decoder{*blk, 0, default_format};
  for (; !decoder.completed(); decoder.next())
  {
    auto bi = decoder.index();
    auto &target = result[bi];
    InputLayout il;
    if (decoder.decode(il, target.hash))
    {
      target.id = registerInputLayout(ctx, il);
    }
    else
    {
      target.id = InputLayoutID::Null();
    }
  }
  return result;
}

void backend::ShaderModuleManager::addVertexShader(ShaderID id, VertexShaderModule *module)
{
  eastl::unique_ptr<VertexShaderModule> modulePtr{module};

  G_ASSERT_RETURN(0 == id.getGroup(), );

  ensure_container_space(shaderGroupZero.vertex, id.getIndex() + 1);
  auto &shader = shaderGroupZero.vertex[id.getIndex()];
  shader = eastl::make_unique<GroupZeroVertexShaderModule>();
  shader->header.hash = module->ident.shaderHash;
  shader->header.header = module->header;
  shader->header.debugName = module->debugName;
  shader->bytecode.bytecode = eastl::move(module->byteCode);
  shader->bytecode.bytecodeSize = module->byteCodeSize;
  dxil::ShaderHeader subShaderHeaders[3];
  StageShaderModuleBytecode subShaderBytecodes[3];
  uint32_t subShaderCount = 0;
  if (module->geometryShader)
  {
    auto &headerTarget = subShaderHeaders[subShaderCount];
    auto &bytecodeTarget = subShaderBytecodes[subShaderCount++];
    headerTarget = module->geometryShader->header;
    bytecodeTarget.bytecode = eastl::move(module->geometryShader->byteCode);
    bytecodeTarget.bytecodeSize = module->geometryShader->byteCodeSize;

    shader->header.hasGsOrAs = true;
  }
  if (module->hullShader && module->domainShader)
  {
    {
      auto &headerTarget = subShaderHeaders[subShaderCount];
      auto &bytecodeTarget = subShaderBytecodes[subShaderCount++];
      headerTarget = module->hullShader->header;
      bytecodeTarget.bytecode = eastl::move(module->hullShader->byteCode);
      bytecodeTarget.bytecodeSize = module->hullShader->byteCodeSize;
    }
    {
      auto &headerTarget = subShaderHeaders[subShaderCount];
      auto &bytecodeTarget = subShaderBytecodes[subShaderCount++];
      headerTarget = module->domainShader->header;
      bytecodeTarget.bytecode = eastl::move(module->domainShader->byteCode);
      bytecodeTarget.bytecodeSize = module->domainShader->byteCodeSize;
    }

    shader->header.hasDsAndHs = true;
  }
  if (subShaderCount > 0)
  {
    shader->header.subShaders = eastl::make_unique<dxil::ShaderHeader[]>(subShaderCount);
    eastl::copy_n(subShaderHeaders, subShaderCount, shader->header.subShaders.get());

    shader->bytecode.subShaders = eastl::make_unique<StageShaderModuleBytecode[]>(subShaderCount);
    for (uint32_t i = 0; i < subShaderCount; ++i)
    {
      shader->bytecode.subShaders[i] = eastl::move(subShaderBytecodes[i]);
    }
  }
}

void backend::ShaderModuleManager::addPixelShader(ShaderID id, PixelShaderModule *module)
{
  eastl::unique_ptr<PixelShaderModule> modulePtr{module};

  G_ASSERT_RETURN(0 == id.getGroup(), );

  ensure_container_space(shaderGroupZero.pixel, id.getIndex() + 1);
  auto &shader = shaderGroupZero.pixel[id.getIndex()];
  shader = eastl::make_unique<GroupZeroPixelShaderModule>();
  shader->header.hash = module->ident.shaderHash;
  shader->header.header = module->header;
  shader->header.debugName = module->debugName;
  shader->bytecode.bytecode = eastl::move(module->byteCode);
  shader->bytecode.bytecodeSize = module->byteCodeSize;
}

const dxil::HashValue &backend::ShaderModuleManager::getVertexShaderHash(ShaderID id) const
{
  if (0 != id.getGroup())
  {
    return shaderGroup[id.getGroup() - 1].vertex[id.getIndex()]->header.hash;
  }

  return shaderGroupZero.vertex[id.getIndex()]->header.hash;
}

void backend::ShaderModuleManager::setVertexShaderName(ShaderID id, eastl::span<const char> name)
{
  if (0 == id.getGroup())
  {
    shaderGroupZero.vertex[id.getIndex()]->header.debugName.assign(begin(name), end(name));
  }
  else
  {
    shaderGroup[id.getGroup() - 1].vertex[id.getIndex()]->header.debugName.assign(begin(name), end(name));
  }
}

const dxil::HashValue &backend::ShaderModuleManager::getPixelShaderHash(ShaderID id) const
{
  if (0 != id.getGroup())
  {
    return shaderGroup[id.getGroup() - 1].pixel[id.getIndex()]->header.hash;
  }

  return shaderGroupZero.pixel[id.getIndex()]->header.hash;
}

void backend::ShaderModuleManager::setPixelShaderName(ShaderID id, eastl::span<const char> name)
{
  if (0 != id.getGroup())
  {
    shaderGroup[id.getGroup() - 1].pixel[id.getIndex()]->header.debugName.assign(begin(name), end(name));
  }
  else
  {
    shaderGroupZero.pixel[id.getIndex()]->header.debugName.assign(begin(name), end(name));
  }
}

backend::VertexShaderModuleRefStore backend::ShaderModuleManager::getVertexShader(ShaderID id)
{
  if (0 != id.getGroup())
  {
    auto &container = shaderGroup[id.getGroup() - 1].vertex;
    auto &shader = container[id.getIndex()];
    // when shader size is 0 we did not decoded the shader binary
    if (0 == shader->bytecode.shaderSize)
    {
      auto byteCode = getShaderByteCode(id.getGroup(), shader->bytecode.compressionGroup, shader->bytecode.compressionIndex);
      if (!byteCode.empty())
      {
        auto module = decode_vertex_shader_ref(byteCode.data(), byteCode.size());
        shader->header.header = module.header;
        shader->header.debugName = module.debugName;
        shader->bytecode.shaderOffset = offset_to_base(byteCode.data(), module);
        shader->bytecode.shaderSize = module.byteCode.size();

        dxil::ShaderHeader subShaderHeaders[3];
        StageShaderModuleBytcodeInDumpOffsets subShaderBytecodes[3];
        uint32_t subShaderCount = 0;
        if (!module.geometryShader.byteCode.empty())
        {
          auto &headerTarget = subShaderHeaders[subShaderCount];
          auto &bytecodeTarget = subShaderBytecodes[subShaderCount++];
          headerTarget = module.geometryShader.header;
          bytecodeTarget.shaderOffset = offset_to_base(byteCode.data(), module.geometryShader);
          bytecodeTarget.shaderSize = module.geometryShader.byteCode.size();

          shader->header.hasGsOrAs = true;
        }
        if (!module.hullShader.byteCode.empty() && !module.domainShader.byteCode.empty())
        {
          {
            auto &headerTarget = subShaderHeaders[subShaderCount];
            auto &bytecodeTarget = subShaderBytecodes[subShaderCount++];
            headerTarget = module.hullShader.header;
            bytecodeTarget.shaderOffset = offset_to_base(byteCode.data(), module.hullShader);
            bytecodeTarget.shaderSize = module.hullShader.byteCode.size();
          }
          {
            auto &headerTarget = subShaderHeaders[subShaderCount];
            auto &bytecodeTarget = subShaderBytecodes[subShaderCount++];
            headerTarget = module.domainShader.header;
            bytecodeTarget.shaderOffset = offset_to_base(byteCode.data(), module.domainShader);
            bytecodeTarget.shaderSize = module.domainShader.byteCode.size();
          }

          shader->header.hasDsAndHs = true;
        }
        if (subShaderCount > 0)
        {
          shader->header.subShaders = eastl::make_unique<dxil::ShaderHeader[]>(subShaderCount);
          eastl::copy_n(subShaderHeaders, subShaderCount, shader->header.subShaders.get());

          shader->bytecode.subShaders = eastl::make_unique<StageShaderModuleBytcodeInDumpOffsets[]>(subShaderCount);
          eastl::copy_n(subShaderBytecodes, subShaderCount, shader->bytecode.subShaders.get());
        }
      }
    }
    return {shader->header, VertexShaderModuleBytcodeInDumpRef{id.getGroup(), &shader->bytecode}};
  }
  auto &shader = shaderGroupZero.vertex[id.getIndex()];
  return {shader->header, VertexShaderModuleBytecodeRef{&shader->bytecode}};
}

backend::PixelShaderModuleRefStore backend::ShaderModuleManager::getPixelShader(ShaderID id)
{
  if (0 != id.getGroup())
  {
    auto &container = shaderGroup[id.getGroup() - 1].pixel;
    auto &shader = container[id.getIndex()];
    // when shader size is 0 we did not decoded the shader binary
    if (0 == shader->bytecode.shaderSize)
    {
      auto byteCode = getShaderByteCode(id.getGroup(), shader->bytecode.compressionGroup, shader->bytecode.compressionIndex);
      if (!byteCode.empty())
      {
        auto module = decode_pixel_shader_ref(byteCode.data(), byteCode.size());
        shader->header.header = module.header;
        shader->header.debugName = module.debugName;
        shader->bytecode.shaderOffset = offset_to_base(byteCode.data(), module);
        shader->bytecode.shaderSize = module.byteCode.size();
      }
    }
    return {shader->header, PixelShaderModuleBytcodeInDumpRef{id.getGroup(), &shader->bytecode}};
  }

  auto &shader = shaderGroupZero.pixel[id.getIndex()];
  return {shader->header, PixelShaderModuleBytecodeRef{&shader->bytecode}};
}

backend::ShaderModuleManager::AnyShaderModuleUniquePointer backend::ShaderModuleManager::releaseVertexShader(ShaderID id)
{
  if (0 != id.getGroup())
  {
    return eastl::move(shaderGroup[id.getGroup() - 1].vertex[id.getIndex()]);
  }
  return eastl::move(shaderGroupZero.vertex[id.getIndex()]);
}

backend::ShaderModuleManager::AnyShaderModuleUniquePointer backend::ShaderModuleManager::releasePixelShader(ShaderID id)
{
  if (0 != id.getGroup())
  {
    return eastl::move(shaderGroup[id.getGroup() - 1].pixel[id.getIndex()]);
  }
  return eastl::move(shaderGroupZero.pixel[id.getIndex()]);
}

void backend::ShaderModuleManager::resetDumpOfGroup(uint32_t group_index)
{
  ScriptedShadersBinDumpManager::resetDumpOfGroup(group_index);
  if (0 != group_index)
  {
    shaderGroup[group_index - 1].vertex.clear();
    shaderGroup[group_index - 1].pixel.clear();
  }
  else
  {
    shaderGroupZero.vertex.clear();
    shaderGroupZero.pixel.clear();
  }
}

void backend::ShaderModuleManager::reserveVertexShaderRange(uint32_t group_index, uint32_t count)
{
  shaderGroup[group_index - 1].vertex.resize(count);
}

void backend::ShaderModuleManager::setVertexShaderCompressionGroup(uint32_t group_index, uint32_t index, const dxil::HashValue &hash,
  uint32_t compression_group, uint32_t compression_index)
{
  G_ASSERT_RETURN(0 != group_index, );
  auto &container = shaderGroup[group_index - 1].vertex;
  ensure_container_space(container, index + 1);
  auto &shader = container[index];
  shader = eastl::make_unique<GroupVertexShaderModule>();
  shader->header.hash = hash;
  shader->bytecode.compressionGroup = compression_group;
  shader->bytecode.compressionIndex = compression_index;
}

void backend::ShaderModuleManager::setPixelShaderCompressionGroup(uint32_t group_index, uint32_t index, const dxil::HashValue &hash,
  uint32_t compression_group, uint32_t compression_index)
{
  G_ASSERT_RETURN(0 != group_index, );

  auto &container = shaderGroup[group_index - 1].pixel;
  ensure_container_space(container, index + 1);
  auto &shader = container[index];
  shader = eastl::make_unique<GroupPixelShaderModule>();
  shader->header.hash = hash;
  shader->bytecode.compressionGroup = compression_group;
  shader->bytecode.compressionIndex = compression_index;
}