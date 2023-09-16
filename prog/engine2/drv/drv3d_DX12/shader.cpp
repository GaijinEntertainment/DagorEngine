#include <EASTL/fixed_vector.h>
#include "device.h"

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

namespace
{
void validate_null_layout_use(const GraphicsProgramIOMask &mask)
{
#if DX12_VALIDATE_INPUT_LAYOUT_USES
  if (0 == mask.usedAttributeLocations)
    return;
  for (uint32_t i = 0; i < MAX_SEMANTIC_INDEX; ++i)
  {
    const auto bit = 1u << i;
    if (0 == (mask.usedAttributeLocations & bit))
      continue;

    auto info = dxil::getSemanticInfoFromIndex(i);
    G_ASSERTF(false, "Null input layout was used for a shader that needs the attribute %s [%u]", info->name, info->index);
  }
#else
  G_UNUSED(mask);
#endif
}
void validate_input_layout_use(const InputLayout &layout, const GraphicsProgramIOMask &mask)
{
#if DX12_VALIDATE_INPUT_LAYOUT_USES
  if (mask.usedAttributeLocations == (layout.vertexAttributeSet.locationMask & mask.usedAttributeLocations))
    return;

  // input layout does not provide all inputs!
  for (uint32_t i = 0; i < MAX_SEMANTIC_INDEX; ++i)
  {
    const auto bit = 1u << i;
    if (0 == (mask.usedAttributeLocations & bit))
      continue;
    if (0 != (layout.vertexAttributeSet.locationMask & bit))
      continue;

    auto info = dxil::getSemanticInfoFromIndex(i);
    G_ASSERTF(false, "Provided input layout for shader does not define the attribute %s [%u]", info->name, info->index);
  }
#else
  G_UNUSED(layout);
  G_UNUSED(mask);
#endif
}
} // namespace

void InputLayout::fromVdecl(const VSDTYPE *decl)
{
  const VSDTYPE *__restrict vt = decl;
  inputStreamSet = VertexStreamsDesc{};
  vertexAttributeSet = VertexAttributesDesc{};
  uint32_t ofs = 0;
  uint32_t streamIndex = 0;

  for (; *vt != VSD_END; ++vt)
  {
    if ((*vt & VSDOP_MASK) == VSDOP_INPUT)
    {
      if (*vt & VSD_SKIPFLG)
      {
        ofs += GET_VSDSKIP(*vt) * 4;
        continue;
      }

      uint32_t locationIndex = GET_VSDREG(*vt);
      vertexAttributeSet.useLocation(locationIndex);
      vertexAttributeSet.setLocationStreamSource(locationIndex, streamIndex);
      vertexAttributeSet.setLocationStreamOffset(locationIndex, ofs);
      vertexAttributeSet.setLocationFormatIndex(locationIndex, *vt & VSDT_MASK);

      uint32_t sz = 0; // size of entry
      //-V::1037
      switch (*vt & VSDT_MASK)
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
        default: G_ASSERTF(false, "invalid vertex declaration type"); break;
      }
      ofs += sz / 8;
    }
    else if ((*vt & VSDOP_MASK) == VSDOP_STREAM)
    {
      streamIndex = GET_VSDSTREAM(*vt);
      ofs = 0;
      inputStreamSet.useStream(streamIndex);
      if (*vt & VSDS_PER_INSTANCE_DATA)
        inputStreamSet.setStreamStepRate(streamIndex, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA);
      else
        inputStreamSet.setStreamStepRate(streamIndex, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
    }
    else
    {
      G_ASSERTF(0, "Invalid vsd opcode 0x%08X", *vt);
    }
  }
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
  auto il = registerInputLayoutInternal(ilDef);

  debugProgram = newGraphicsProgram(ctx, il, vs, ps);
}

void ShaderProgramDatabase::initNullPixelShader(DeviceContext &ctx)
{
  dxil::ShaderHeader nullHeader = {};
  nullHeader.shaderType = static_cast<uint16_t>(dxil::ShaderStage::PIXEL);
  auto nullShader = make_span(null_pixel_shader, array_size(null_pixel_shader));
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
  auto id = shaderProgramGroups.addVertexShader(ShaderStageResouceUsageMask{module->header}, module->header.inOutSemanticMask);
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
  auto id = shaderProgramGroups.addPixelShader(ShaderStageResouceUsageMask{module->header}, module->header.inOutSemanticMask);
  ctx.addPixelShader(id, eastl::move(module));
  return id;
}

StageShaderModule ShaderProgramDatabase::decodeShaderBinary(const void *data, uint32_t size)
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
          logerr("DX12: Error while decoding shader, shader header hash does not match");
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
            logerr("DX12: Error while decoding shader, shader module hash does not match");
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
        logerr("DX12: Error while decoding shader, unrecognized chunk header id %u", static_cast<uint32_t>(header.type));
        return result;
        break;
    }
  }

  if (!shaderHeader)
  {
    logerr("DX12: Error while decoding shader, unable to locate header chunk");
    return result;
  }
  if (!shaderModule)
  {
    logerr("DX12: Error while decoding shader, unable to locate shader module chunk");
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

static StageShaderModule shader_layout_to_module(const bindump::Mapper<dxil::Shader> *layout)
{
  StageShaderModule result;
  result.header = layout->shaderHeader;
  result.byteCode = eastl::make_unique<uint8_t[]>(layout->bytecode.size());
  eastl::copy(layout->bytecode.begin(), layout->bytecode.end(), result.byteCode.get());
  result.byteCodeSize = layout->bytecode.size();
  return result;
}

template <typename ModuleType>
ModuleType decode_shader_layout(const void *data)
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
        result.geometryShader = eastl::make_unique<VertexShaderModule>(shader_layout_to_module(&*combined->geometryShader));
      if (combined->hullShader)
        result.hullShader = eastl::make_unique<VertexShaderModule>(shader_layout_to_module(&*combined->hullShader));
      if (combined->domainShader)
        result.domainShader = eastl::make_unique<VertexShaderModule>(shader_layout_to_module(&*combined->domainShader));
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
        result.geometryShader = eastl::make_unique<VertexShaderModule>(shader_layout_to_module(&*combined->amplificationShader));
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

eastl::unique_ptr<VertexShaderModule> ShaderProgramDatabase::decodeVertexShader(const void *data, uint32_t size)
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
      auto basicModule = decodeShaderBinary(dataStart + sHeader.offset, sHeader.size);
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
          logerr("DX12: Error while decoding vertex shader, unexpected combined shader stage type "
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
    auto basicModule = decodeShaderBinary(data, size);
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
      logerr("DX12: Error while decoding vertex shader, unexpected shader identifier 0x%08X", fileHeader->ident);
    }
  }

  return vs;
}

eastl::unique_ptr<PixelShaderModule> ShaderProgramDatabase::decodePixelShader(const void *data, uint32_t size)
{
  eastl::unique_ptr<PixelShaderModule> ps;
  auto fileHeader = reinterpret_cast<const dxil::FileHeader *>(data);
  if (fileHeader->ident == dxil::SHADER_UNCOMPRESSED_IDENT)
  {
    auto basicModule = decodeShaderBinary(data, size);
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
      ps = eastl::make_unique<VertexShaderModule>(eastl::move(basicModule));
    }
    else
    {
      logerr("DX12: Error while decoding pixel shader, unexpected shader identifier 0x%08X", fileHeader->ident);
    }
  }
  return ps;
}

ProgramID ShaderProgramDatabase::newComputeProgram(DeviceContext &ctx, const void *data)
{
  auto basicModule = decode_shader_layout<ComputeShaderModule>((const uint8_t *)data);
  if (!basicModule)
  {
    basicModule = decodeShaderBinary(data, ~uint32_t(0));
    if (!basicModule)
    {
      return ProgramID::Null();
    }
  }

  auto module = eastl::make_unique<ComputeShaderModule>(eastl::move(basicModule));

  ProgramID program;
  {
    ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
    program = shaderProgramGroups.addComputeShaderProgram(ShaderStageResouceUsageMask{module->header});
  }
  ctx.addComputeProgram(program, eastl::move(module));

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

  GraphicsProgramIOMask mask;
  mask.usedAttributeLocations = shaderProgramGroups.getVertexShaderInputMask(vs);
  // not required yet, but we are going to on a later change
  mask.usedOutputs = shaderProgramGroups.getPixelShaderOutputMask(ps);

  InternalInputLayoutID iil;
  dataGuard.unlockWrite();
  iil = getInternalInputLayout(ctx, vdecl, mask, ShaderProgramDatabase::Validation::Skip,
    ShaderProgramDatabase::ContextLockState::Unlocked);
  dataGuard.lockWrite();

  return shaderProgramGroups.instanciateGraphicsProgram(key, gid, vdecl, iil);
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

InputLayoutID ShaderProgramDatabase::registerInputLayoutInternal(const InputLayout &layout)
{
  auto ref =
    eastl::find_if(begin(publicImputLayoutTable), end(publicImputLayoutTable), [&](auto &l) { return l.fullLayout == layout; });
  if (ref == end(publicImputLayoutTable))
  {
    ref = publicImputLayoutTable.emplace(ref, layout);
  }

  return InputLayoutID(ref - begin(publicImputLayoutTable));
}

InputLayoutID ShaderProgramDatabase::registerInputLayout(const InputLayout &layout)
{
  ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
  return registerInputLayoutInternal(layout);
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

#if D3D_HAS_RAY_TRACING
  for (size_t rti = 0; rti < raytracePrograms.size(); ++rti)
  {
    if (raytracePrograms[rti].shaders)
    {
      ctx.removeProgram(ProgramID::asRaytraceProgram(0, rti));
    }
  }
  raytracePrograms.clear();
#endif

  shaderProgramGroups.iterateAllComputeShaders([&](ProgramID prog) { ctx.removeProgram(prog); });

  shaderProgramGroups.itarateAllVertexShaders([&](ShaderID shader) { ctx.removeVertexShader(shader); });

  shaderProgramGroups.iterateAllPixelShaders([&](ShaderID shader) { ctx.removePixelShader(shader); });

  shaderProgramGroups.reset();

  publicImputLayoutTable.clear();
  internalInputLayoutTable.clear();

  debugProgram = ProgramID::Null();
  nullPixelShader = ShaderID::Null();
  nullLayout = InternalInputLayoutID::Null();
}

ShaderID ShaderProgramDatabase::newVertexShader(DeviceContext &ctx, const void *data)
{
  auto vs = decodeVertexShader(data, ~uint32_t(0));

  ShaderID id = ShaderID::Null();
  if (vs)
  {
    ShaderStageResouceUsageMask shaderResourceUse{vs->header};
    if (vs->geometryShader)
    {
      shaderResourceUse |= ShaderStageResouceUsageMask{vs->geometryShader->header};
    }
    if (vs->hullShader)
    {
      shaderResourceUse |= ShaderStageResouceUsageMask{vs->hullShader->header};
    }
    if (vs->domainShader)
    {
      shaderResourceUse |= ShaderStageResouceUsageMask{vs->domainShader->header};
    }
    {
      ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
      id = shaderProgramGroups.addVertexShader(shaderResourceUse, vs->header.inOutSemanticMask);
    }
    ctx.addVertexShader(id, eastl::move(vs));
  }
  return id;
}

ShaderID ShaderProgramDatabase::newPixelShader(DeviceContext &ctx, const void *data)
{
  auto ps = decodePixelShader(data, ~uint32_t(0));

  ShaderID id = ShaderID::Null();
  if (ps)
  {
    {
      ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
      id = shaderProgramGroups.addPixelShader(ShaderStageResouceUsageMask{ps->header}, ps->header.inOutSemanticMask);
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
#if D3D_HAS_RAY_TRACING
  else if (prog.isRaytrace())
  {
    auto &target = raytracePrograms[prog.getIndex()];
    target.shaders.reset();
    target.shaderGroups.reset();
    ctx.removeProgram(prog);
  }
#endif
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

#if D3D_HAS_RAY_TRACING
ProgramID ShaderProgramDatabase::newRaytraceProgram(DeviceContext &ctx, const ShaderID *shader_ids, uint32_t shader_count,
  const RaytraceShaderGroup *shader_groups, uint32_t group_count, uint32_t max_recursion_depth)
{
  ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);

  auto ref = eastl::find_if(begin(raytracePrograms), end(raytracePrograms), [](const RaytraceProgram &rp) {
    // no shaders indicate empty slot
    return nullptr == rp.shaders.get();
  });
  if (ref == end(raytracePrograms))
  {
    ref = raytracePrograms.insert(ref, RaytraceProgram{});
  }

  // hm, may only store used id and move everything over, currently this is
  // only used to back addRaytraceProgram command of ctx
  auto &target = *ref;
  target.shaders.reset(new ShaderID[shader_count]);
  eastl::copy(shader_ids, shader_ids + shader_count, target.shaders.get());
  target.shaderGroups.reset(new RaytraceShaderGroup[group_count]);
  eastl::copy(shader_groups, shader_groups + group_count, target.shaderGroups.get());
  target.groupCount = group_count;
  target.shaderCount = shader_count;
  target.maxRecursionDepth = max_recursion_depth;
  auto id = ProgramID::asRaytraceProgram(0, ref - begin(raytracePrograms));

  ctx.addRaytraceProgram(id, max_recursion_depth, shader_count, target.shaders.get(), group_count, target.shaderGroups.get());

  return id;
}
#endif

InternalInputLayoutID ShaderProgramDatabase::getInternalInputLayout(DeviceContext &ctx, InputLayoutID external_id,
  GraphicsProgramIOMask io_mask, Validation validation, ContextLockState ctx_locked)
{
  InternalInputLayoutID ret;
  InputLayout variation{};
  bool needRegister = false;
  const bool skip_validation = validation == Validation::Skip;
  if (InputLayoutID::Null() == external_id)
  {
    if (!skip_validation)
    {
      validate_null_layout_use(io_mask);
    }
    if (!nullLayout.interlockedIsNull())
      return nullLayout;
    ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
    {
      auto internalRef = eastl::find(begin(internalInputLayoutTable), end(internalInputLayoutTable), variation);
      if (internalRef == end(internalInputLayoutTable))
      {
        internalRef = internalInputLayoutTable.insert(internalRef, variation);
        needRegister = true;
      }
      nullLayout.interlockedSet(static_cast<int>(internalRef - begin(internalInputLayoutTable)));
      ret = nullLayout;
    }
  }
  else
  {
    {
      ScopedLockReadTemplate<OSReadWriteLock> lock(dataGuard);
      auto &externalToInternal = publicImputLayoutTable[external_id.get()];
      auto ref = eastl::lower_bound(begin(externalToInternal.variations), end(externalToInternal.variations),
        eastl::pair(io_mask.usedAttributeLocations, InternalInputLayoutID{}), [](auto l, auto r) { return l.first < r.first; });
      if (ref != end(externalToInternal.variations) && ref->first == io_mask.usedAttributeLocations)
        return ref->second;
    }
    ScopedLockWriteTemplate<OSReadWriteLock> lock(dataGuard);
    auto &externalToInternal = publicImputLayoutTable[external_id.get()]; // we need to reget, if
                                                                          // there was concurrent add

    auto ref = eastl::lower_bound(begin(externalToInternal.variations), end(externalToInternal.variations),
      eastl::pair(io_mask.usedAttributeLocations, InternalInputLayoutID{}), [](auto l, auto r) { return l.first < r.first; });
    if (ref != end(externalToInternal.variations) && ref->first == io_mask.usedAttributeLocations)
      return ref->second;

    variation = externalToInternal.fullLayout.getLayoutForAttributeUse(io_mask.usedAttributeLocations);
    auto internalRef = eastl::find(begin(internalInputLayoutTable), end(internalInputLayoutTable), variation);
    if (internalRef == end(internalInputLayoutTable))
    {
      internalRef = internalInputLayoutTable.insert(internalRef, variation);
      needRegister = true;
    }
    if (!skip_validation)
    {
      validate_input_layout_use(*internalRef, io_mask);
    }
    ref = externalToInternal.variations.insert(ref,
      eastl::pair(io_mask.usedAttributeLocations,
        InternalInputLayoutID{static_cast<int>(internalRef - begin(internalInputLayoutTable))}));
    ret = ref->second;
  }
  if (needRegister)
  {
    if (ctx_locked == ContextLockState::Unlocked)
      ctx.beginStateCommit();
    ctx.registerInputLayout(ret, variation);
    if (ctx_locked == ContextLockState::Unlocked)
      ctx.endStateCommit();
  }
  return ret;
}

void ShaderProgramDatabase::loadFromCache(PipelineCache &cache)
{
  cache.enumerateInputLayouts([this](auto &layout) { internalInputLayoutTable.push_back(layout); });
}

void ShaderProgramDatabase::registerShaderBinDump(Device &, ScriptedShadersBinDumpOwner *) {}
