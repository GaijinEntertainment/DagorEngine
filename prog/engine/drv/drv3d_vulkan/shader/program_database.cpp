// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "program_database.h"
#include "shader.h"
#include "device_context.h"
#include "frontend.h"

using namespace drv3d_vulkan;

// TODO: use smol-v compressed instead?
alignas(uint32_t) static const uint8_t null_frag_shader[] = //
  {0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00,
    0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64,
    0x2e, 0x34, 0x35, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00,
    0x06, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xb8, 0x01,
    0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x6f, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x1e, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x07, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2b, 0x00,
    0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x2c, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x36, 0x00,
    0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00,
    0x01, 0x00};

void ShaderProgramDatabase::initShaders(DeviceContext &dc)
{
  ShaderModuleHeader smh = {};
  spirv::ShaderHeader &spvHeader = smh.header;
  spvHeader.outputMask = 1;

  smh.hash = spirv::HashValue::calculate(&spvHeader, 1);
  smh.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

  ShaderModuleBlob smb = {};
  auto from = reinterpret_cast<const uint32_t *>(null_frag_shader);
  auto to = reinterpret_cast<const uint32_t *>(null_frag_shader + sizeof(null_frag_shader));
  smb.blob.insert(end(smb.blob), from, to);
  smb.hash = spirv::HashValue::calculate(smb.blob.data(), smb.blob.size());

  nullFragmentShader = newShader(dc, smh, smb);
}

ShaderID ShaderProgramDatabase::newShader(DeviceContext &ctx, const ShaderModuleHeader &smh, const ShaderModuleBlob &smb)
{
  WinAutoLock lock(dataGuard);

  auto header = shaderDesc.headers.uniqueInsert(ctx, smh);
  auto module = shaderDesc.modules.uniqueInsert(ctx, smb);

  return shaders.add(ctx, {header, module, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
}

ShaderID ShaderProgramDatabase::newShader(DeviceContext &ctx, VkShaderStageFlagBits stage, Tab<spirv::ChunkHeader> &chunks,
  Tab<uint8_t> &chunk_data)
{
  ShaderModuleHeader header;
  ShaderModuleBlob blob;

  if (!extractShaderModules(stage, chunks, chunk_data, header, blob))
    return ShaderID::Null();

  WinAutoLock l(dataGuard);
  auto newShaderId = newShader(ctx, header, blob);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  attachDebugInfo(newShaderId, spirv_extractor::getDebugInfo(chunks, chunk_data, 0));
#endif

  return newShaderId;
}

ShaderID ShaderProgramDatabase::newShader(DeviceContext &ctx, eastl::vector<VkShaderStageFlagBits> stage,
  eastl::vector<Tab<spirv::ChunkHeader>> chunks, eastl::vector<Tab<uint8_t>> chunk_data)
{
  // find and setup vertex shader stuff
  int vsIndex = -1;
  int gsIndex = -1;
  int tcIndex = -1;
  int teIndex = -1;

  for (auto iter = begin(stage); iter != end(stage); ++iter)
  {
    if (*iter == VK_SHADER_STAGE_VERTEX_BIT)
      vsIndex = iter - begin(stage);
    else if (*iter == VK_SHADER_STAGE_GEOMETRY_BIT)
      gsIndex = iter - begin(stage);
    else if (*iter == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
      tcIndex = iter - begin(stage);
    else if (*iter == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
      teIndex = iter - begin(stage);
  }

  if (vsIndex < 0)
  {
    DAG_FATAL("Combo shader has no vertex stage");
    return ShaderID::Null();
  }

  if (teIndex < 0 && tcIndex < 0 && gsIndex < 0)
  {
    DAG_FATAL("Combo shader has vertex stage only. Expected to have some of HS,DS,GS");
    return ShaderID::Null();
  }

  if (tcIndex < 0 && teIndex >= 0)
  {
    DAG_FATAL("Combo shader has HS without DS");
    return ShaderID::Null();
  }

  if (tcIndex >= 0 && teIndex < 0)
  {
    DAG_FATAL("Combo shader has DS without HS");
    return ShaderID::Null();
  }

  WinAutoLock l(dataGuard);

  CombinedChunkModules modules;
  modules.vs = {&chunks[vsIndex], &chunk_data[vsIndex]};
  if (gsIndex != -1)
    modules.gs = {&chunks[gsIndex], &chunk_data[gsIndex]};
  if (tcIndex != -1)
    modules.tc = {&chunks[tcIndex], &chunk_data[tcIndex]};
  if (teIndex != -1)
    modules.te = {&chunks[teIndex], &chunk_data[teIndex]};

  auto creationInfo = getShaderCreationInfo(ctx, modules);
  if (!creationInfo)
    return ShaderID::Null();

  auto newShaderId = shaders.add(ctx, *creationInfo);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  attachDebugInfo(newShaderId, modules);
#endif
  return newShaderId;
}

bool ShaderProgramDatabase::extractShaderModules(const VkShaderStageFlagBits stage, const Tab<spirv::ChunkHeader> &chunk_header,
  const Tab<uint8_t> &chunk_data, ShaderModuleHeader &shader_header, ShaderModuleBlob &shader_blob)
{
  auto header = spirv_extractor::getHeader(stage, chunk_header, chunk_data, 0);
  if (!header)
  {
    DAG_FATAL("missing shader header chunk");
    return false;
  }
  shader_header = *header;

  shader_blob = spirv_extractor::getBlob(chunk_header, chunk_data, 0);
  if (shader_blob.blob.empty())
  {
    DAG_FATAL("missing shader byte code chunk");
    return false;
  }

  return true;
}

eastl::optional<ShaderInfo::CreationInfo> ShaderProgramDatabase::getShaderCreationInfo(DeviceContext &ctx,
  const CombinedChunkModules &modules)
{
  struct CreationInfoFillData
  {
    VkShaderStageFlagBits stage;
    const CombinedChunkModules::Pair *module;
    UniqueShaderHeader **uniqueHeader;
    UniqueShaderModule **uniqueModule;
  };

  ShaderInfo::CreationInfo cInfo;
  carray<CreationInfoFillData, 4> cInfoFillData;
  cInfoFillData[0] = {VK_SHADER_STAGE_VERTEX_BIT, &modules.vs, &cInfo.header_a, &cInfo.module_a};
  cInfoFillData[1] = {VK_SHADER_STAGE_GEOMETRY_BIT, &modules.gs, &cInfo.header_b, &cInfo.module_b};
  cInfoFillData[2] = {VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, &modules.tc, &cInfo.header_c, &cInfo.module_c};
  cInfoFillData[3] = {VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, &modules.te, &cInfo.header_d, &cInfo.module_d};

  for (auto &fd : cInfoFillData)
  {
    if (fd.module->data && fd.module->headers)
    {
      ShaderModuleHeader header;
      ShaderModuleBlob blob;
      if (!extractShaderModules(fd.stage, *fd.module->headers, *fd.module->data, header, blob))
        return eastl::nullopt;

      *fd.uniqueHeader = shaderDesc.headers.uniqueInsert(ctx, header);
      *fd.uniqueModule = shaderDesc.modules.uniqueInsert(ctx, eastl::move(blob));
    }
  }

  return cInfo;
}

void ShaderProgramDatabase::releaseShaderDeps(DeviceContext &ctx, ShaderInfo &item)
{
  switch (item.getStage())
  {
    case VK_SHADER_STAGE_VERTEX_BIT:
      progs.graphics.enumerate([this, &ctx, &item](ProgramID id, GraphicsProgram &prog) //
        {
          if (prog.usesVertexShader(&item))
            this->progs.graphics.remove(ctx, id, true);
        });
      break;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
      progs.graphics.enumerate([this, &ctx, &item](ProgramID id, GraphicsProgram &prog) //
        {
          if (prog.usesFragmentShader(&item))
            this->progs.graphics.remove(ctx, id, true);
        });
      break;
    case VK_SHADER_STAGE_COMPUTE_BIT:
      // can't delete separate compute shaders
      break;
  }
  shaderDesc.modules.remove(ctx, item.module->id);
  shaderDesc.headers.remove(ctx, item.header->id);
}

void ShaderProgramDatabase::deleteShader(DeviceContext &ctx, ShaderID shader)
{
  WinAutoLock lock(dataGuard);

  auto item = shaders.get(shader);
  // will be removed, process deps
  if (item->refCount == 1)
    releaseShaderDeps(ctx, *item);

  shaders.remove(ctx, shader);
}

void ShaderProgramDatabase::init(bool has_bindless, DeviceContext &ctx)
{
  initShaders(ctx);
  initDebugProg(has_bindless, ctx);
  // set debug prog to states, unset shader is an error
  Frontend::State::pipe.set<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>(debugProgId);
}

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
void ShaderProgramDatabase::attachDebugInfo(ShaderID shader, const CombinedChunkModules &modules)
{
  ShaderInfo &sh = *shaders.get(shader);
  auto getDebugInfo = [&](const CombinedChunkModules::Pair &m) {
    if (m.data && m.headers)
      return spirv_extractor::getDebugInfo(*m.headers, *m.data, 0);
    else
      return ShaderDebugInfo{};
  };

  sh.debugInfo = getDebugInfo(modules.vs);

  if (sh.geometryShader)
    sh.geometryShader->debugInfo = getDebugInfo(modules.gs);

  if (sh.controlShader)
    sh.controlShader->debugInfo = getDebugInfo(modules.tc);

  if (sh.evaluationShader)
    sh.evaluationShader->debugInfo = getDebugInfo(modules.te);
}

void ShaderProgramDatabase::attachDebugInfo(ShaderID shader, const ShaderDebugInfo &debugInfo)
{
  ShaderInfo &sh = *shaders.get(shader);
  sh.debugInfo = debugInfo;
}

#endif
