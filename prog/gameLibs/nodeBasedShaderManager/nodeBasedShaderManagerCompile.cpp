// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <hlslCompiler/hlslCompiler.h>
#include <hash/crc32.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_info.h>
#include <startup/dag_globalSettings.h>
#include <folders/folders.h>
#include <webui/shaderEditors.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_fileIo.h>
#include "platformUtils.h"
#include <debug/dag_assert.h>

void NodeBasedShaderManager::initCompilation()
{
  const char *dllPath = ::dgs_get_settings()->getBlockByNameEx("debug")->getStr("toolsPathWin", "");
  G_VERIFYF(hlsl_compiler::try_init_dynlib(dllPath),
    "Make sure you have the dll for compute shader compilation if you are using Node-based shader compilation. Otherwise (for "
    "example, in release) you shouldn't be using this code!");
}

static String get_target_path_for_bin(String shader_path, const String &shader_file_suffix,
  const NodeBasedShaderManager::PLATFORM platform_id)
{
#if !IS_OFFLINE_SHADER_COMPILER
  shader_path = String(folders::get_temp_dir()) + "/nbs/" + shader_path;
#endif

  const char *found = strstr(shader_path.str(), ".bin");
  if (found && (int(found - shader_path.str()) == shader_path.length() - 4))
    return shader_path;

  return shader_path + "." + PLATFORM_LABELS[platform_id] + ".cs50." + shader_file_suffix + ".bin";
}

static hlsl_compiler::Platform node_based_to_compiler_platform(NodeBasedShaderManager::PLATFORM platform)
{
  G_ASSERT(platform < NodeBasedShaderManager::PLATFORM::COUNT);

  // This is currently 1 to 1, but a handrolled switch case is change-proof
  switch (platform)
  {
    case NodeBasedShaderManager::DX11: return hlsl_compiler::Platform::DX11;
    case NodeBasedShaderManager::PS4: return hlsl_compiler::Platform::PS4;
    case NodeBasedShaderManager::VULKAN: return hlsl_compiler::Platform::VULKAN;
    case NodeBasedShaderManager::DX12: return hlsl_compiler::Platform::DX12;
    case NodeBasedShaderManager::DX12_XBOX_ONE: return hlsl_compiler::Platform::DX12_XBOX_ONE;
    case NodeBasedShaderManager::DX12_XBOX_SCARLETT: return hlsl_compiler::Platform::DX12_XBOX_SCARLETT;
    case NodeBasedShaderManager::PS5: return hlsl_compiler::Platform::PS5;
    case NodeBasedShaderManager::MTL: return hlsl_compiler::Platform::MTL;
    case NodeBasedShaderManager::VULKAN_BINDLESS: return hlsl_compiler::Platform::VULKAN_BINDLESS;
    case NodeBasedShaderManager::MTL_BINDLESS: return hlsl_compiler::Platform::MTL_BINDLESS;
  }

  G_ASSERT_RETURN(0, hlsl_compiler::Platform::DX11);
}

using SubstituteFunction = String (*)(NodeBasedShaderType shader_type, uint32_t variant_id, const DataBlock &shader_blk);

static const eastl::array<SubstituteFunction, NodeBasedShaderManager::PLATFORM::COUNT> SUBSTITUTE_FUNCTIONS = {{
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitutePs4,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitutePs5,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
}};

static uint32_t calc_shader_source_hash(NodeBasedShaderType shader, const DataBlock &shader_blk)
{
  uint32_t finalShaderSourceHash = 0;
  uint32_t variantCount = get_shader_variant_count(shader);
  for (uint32_t variantId = 0; variantId < variantCount; ++variantId)
  {
    String code = ShaderGraphRecompiler::substitute(shader, variantId, shader_blk);
    finalShaderSourceHash = calc_crc32(reinterpret_cast<const unsigned char *>(code.c_str()), code.size(), finalShaderSourceHash);
  }
  return finalShaderSourceHash;
}

bool NodeBasedShaderManager::compileShaderProgram(const DataBlock &shader_blk, String &out_errors, PLATFORM platform_id)
{
  const uint32_t newShaderSourceHash = calc_shader_source_hash(shader, shader_blk);
  if (newShaderSourceHash == lastShaderSourceHash)
    return lastCompileIsSuccess;

  uint32_t variantCount = get_shader_variant_count(shader);
  lastCompileIsSuccess = true;

  auto compileShaderForPlatform = [&, this](uint32_t platform_id) {
    for (uint32_t variantId = 0; variantId < variantCount && lastCompileIsSuccess; ++variantId)
    {
      ShaderBin &permutation = shaderBin[platform_id][variantId][permutationId];
      String substitutedCode = SUBSTITUTE_FUNCTIONS[platform_id](shader, variantId, shader_blk);

      bool res = hlsl_compiler::compile_compute_shader(node_based_to_compiler_platform((PLATFORM)platform_id), substitutedCode,
        shaderName, "cs_5_0", permutation, out_errors);

      if (!res)
        out_errors.aprintf(0, "\nIn shader:\n%s\n\n", ShaderGraphRecompiler::enumerateLines(substitutedCode).str());

      lastCompileIsSuccess &= res;
    }
  };

#if IS_OFFLINE_SHADER_COMPILER

  if (platform_id == NodeBasedShaderManager::PLATFORM::ALL)
  {
    for (uint32_t platformId = 0; platformId < NodeBasedShaderManager::PLATFORM::COUNT && lastCompileIsSuccess; ++platformId)
      compileShaderForPlatform((NodeBasedShaderManager::PLATFORM)platformId);
  }
  else
    compileShaderForPlatform(platform_id);

#elif !NBSM_COMPILE_ONLY

  G_ASSERT(platform_id == NodeBasedShaderManager::PLATFORM::ALL);
  compileShaderForPlatform(get_nbsm_platform());

#else
#error "Invalid config: NBSM_COMPILE_ONLY can not be defined if IS_OFFLINE_SHADER_COMPILER is not defined"
#endif

  if (lastCompileIsSuccess)
    lastShaderSourceHash = newShaderSourceHash;
  return lastCompileIsSuccess;
}

static void ensure_path_existance(const String &file_name)
{
  String dir = file_name;
  char *p = max(strrchr(dir.str(), '\\'), strrchr(dir.str(), '/'));
  if (p)
    *p = 0;
  else
    return;

  if (!dd_dir_exists(dir))
  {
    debug("NodeBasedShaderManager: Creating directory %s", dir.str());
    dd_mkdir(dir);
  }
}

void NodeBasedShaderManager::saveToFile(const String &shader_path, PLATFORM platform_id) const
{
  G_ASSERTF(lastCompileIsSuccess, "Attempt to save not compiled shaders.");
  G_ASSERTF(platform_id < PLATFORM::COUNT, "Unknown platform id: %d", platform_id);
  uint32_t variantCount = get_shader_variant_count(shader);

  auto saveCompiledShaderToFile = [&, this](PLATFORM platform_id) {
    String filename = get_target_path_for_bin(shader_path, shaderFileSuffix, NodeBasedShaderManager::PLATFORM(platform_id));
    ensure_path_existance(filename);

    FullFileSaveCB shaderFile(filename);
    if (!shaderFile.fileHandle)
    {
      G_ASSERTF(0, "Cannot write to file '%s'", filename.str());
      return;
    }

    for (uint32_t variantId = 0; variantId < variantCount; ++variantId)
    {
      DataBlock parameters; // TODO: maybe only R/W once for all variants
      parameters.addNewBlock(currentIntParametersBlk.get(), "inputs_int");
      parameters.addNewBlock(currentFloatParametersBlk.get(), "inputs_float");
      parameters.addNewBlock(currentFloat4ParametersBlk.get(), "inputs_float4");
      parameters.addNewBlock(currentTextures2dBlk.get(), "inputs_texture2D");
      parameters.addNewBlock(currentTextures3dBlk.get(), "inputs_texture3D");
      parameters.addNewBlock(currentTextures2dShdArrayBlk.get(), "inputs_texture2D_shdArray");
      parameters.addNewBlock(currentTextures2dNoSamplerBlk.get(), "inputs_texture2D_nosampler");
      parameters.addNewBlock(currentTextures3dNoSamplerBlk.get(), "inputs_texture3D_nosampler");
      parameters.addNewBlock(currentBuffersBlk.get(), "inputs_Buffer");
      parameters.addNewBlock(metadataBlk.get(), "metadata");

      parameters.saveToStream(shaderFile);
      const ShaderBin &permutation = shaderBin[platform_id][variantId][permutationId];
      G_ASSERTF(permutation.size() > 0,
        "error: node based shader permutation is invalid: shader #%d, variant #%d, permutation #%d, platform #%d", (int)shader,
        (int)variantId, (int)permutationId, (int)platform_id);

      const uint32_t shaderBinSize = permutation.size() * sizeof(uint32_t);
      shaderFile.write(&shaderBinSize, sizeof(shaderBinSize));
      shaderFile.write(permutation.data(), shaderBinSize);
    }
  };

#if IS_OFFLINE_SHADER_COMPILER

  if (platform_id == NodeBasedShaderManager::PLATFORM::ALL)
  {
    for (uint32_t platformId = 0; platformId < NodeBasedShaderManager::PLATFORM::COUNT && lastCompileIsSuccess; ++platformId)
      saveCompiledShaderToFile((NodeBasedShaderManager::PLATFORM)platformId);
  }
  else
    saveCompiledShaderToFile(platform_id);

#elif !NBSM_COMPILE_ONLY

  G_ASSERT(platform_id == NodeBasedShaderManager::PLATFORM::ALL);
  saveCompiledShaderToFile(get_nbsm_platform());

#else
#error "Invalid config: NBSM_COMPILE_ONLY can not be defined if IS_OFFLINE_SHADER_COMPILER is not defined"
#endif
}

#if IS_OFFLINE_SHADER_COMPILER
void NodeBasedShaderManager::getShadersBinariesFileNames(const String &shader_path, Tab<String> &file_names,
  PLATFORM platform_id) const
{
  file_names.clear();
  for (int platformId = 0; platformId < NodeBasedShaderManager::PLATFORM::COUNT; ++platformId)
  {
    if (platform_id >= 0 && platform_id != platformId)
      continue;

    const String name = get_target_path_for_bin(shader_path, shaderFileSuffix, NodeBasedShaderManager::PLATFORM(platformId));
    file_names.push_back(name);
  }
}
#endif
