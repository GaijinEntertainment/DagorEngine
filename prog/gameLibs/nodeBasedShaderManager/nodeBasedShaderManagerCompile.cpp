#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <hash/crc32.h>
#include <3d/dag_drv3d_pc.h>
#include <folders/folders.h>
#include <webui/shaderEditors.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_fileIo.h>
#include "platformLabels.h"

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

using CompileFunction = bool (*)(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);

#if IS_OFFLINE_SHADER_COMPILER
bool compile_compute_shader_xboxone(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
bool compile_compute_shader_pssl(const char *pssl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
bool compile_compute_shader_spirv(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
bool compile_compute_shader_spirv_bindless(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
bool compile_compute_shader_dx12(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
bool compile_compute_shader_dx12_xbox_one(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
bool compile_compute_shader_dx12_xbox_scarlett(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
bool compile_compute_shader_ps5(const char *pssl_text, unsigned len, const char *entry, const char *profile, Tab<uint32_t> &shader_bin,
  String &out_err);
bool compile_compute_shader_metal(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
#endif

static const eastl::array<CompileFunction, NodeBasedShaderManager::PLATFORM::SAVE_COUNT> COMPILE_FUNCTIONS = {{
  d3d::compile_compute_shader_hlsl,
#if IS_OFFLINE_SHADER_COMPILER
  compile_compute_shader_xboxone,
  compile_compute_shader_pssl,
  compile_compute_shader_spirv,
  compile_compute_shader_dx12,
  compile_compute_shader_dx12_xbox_one,
  compile_compute_shader_dx12_xbox_scarlett,
  compile_compute_shader_ps5,
  compile_compute_shader_metal,
  compile_compute_shader_spirv_bindless,
#endif
}};

using SubstituteFunction = String (*)(NodeBasedShaderType shader_type, uint32_t variant_id, const DataBlock &shader_blk);

static const eastl::array<SubstituteFunction, NodeBasedShaderManager::PLATFORM::SAVE_COUNT> SUBSTITUTE_FUNCTIONS = {{
  ShaderGraphRecompiler::substitute,
#if IS_OFFLINE_SHADER_COMPILER
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitutePs4,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitutePs5,
  ShaderGraphRecompiler::substitute,
  ShaderGraphRecompiler::substitute,
#endif
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
  for (uint32_t platformId = 0; platformId < NodeBasedShaderManager::PLATFORM::SAVE_COUNT && lastCompileIsSuccess; ++platformId)
  {
    if (platform_id >= 0 && platform_id != platformId)
      continue;

    for (uint32_t variantId = 0; variantId < variantCount && lastCompileIsSuccess; ++variantId)
    {
      ShaderBin &permutation = shaderBin[platformId][variantId][permutationId];
      String substitutedCode = SUBSTITUTE_FUNCTIONS[platformId](shader, variantId, shader_blk);
      bool res = COMPILE_FUNCTIONS[platformId](substitutedCode, -1, shaderName, "cs_5_0", permutation, out_errors);

      if (!res)
        out_errors.aprintf(0, "\nIn shader:\n%s\n\n", ShaderGraphRecompiler::enumerateLines(substitutedCode).str());

      lastCompileIsSuccess &= res;
    }
  }
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
  G_ASSERTF(platform_id < PLATFORM::SAVE_COUNT, "Unknown platform id: %d", platform_id);
  uint32_t variantCount = get_shader_variant_count(shader);
  for (uint32_t platformId = 0; platformId < NodeBasedShaderManager::PLATFORM::SAVE_COUNT; ++platformId)
  {
    if (platform_id >= 0 && platform_id != platformId)
      continue;

    String filename = get_target_path_for_bin(shader_path, shaderFileSuffix, NodeBasedShaderManager::PLATFORM(platformId));
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
      parameters.addNewBlock(currentTextures2dNoSamplerBlk.get(), "inputs_texture2D_nosampler");
      parameters.addNewBlock(currentTextures3dNoSamplerBlk.get(), "inputs_texture3D_nosampler");
      parameters.addNewBlock(currentBuffersBlk.get(), "inputs_Buffer");
      parameters.addNewBlock(metadataBlk.get(), "metadata");

      parameters.saveToStream(shaderFile);
      const ShaderBin &permutation = shaderBin[platformId][variantId][permutationId];
      G_ASSERTF(permutation.size() > 0,
        "error: node based shader permutation is invalid: shader #%d, variant #%d, permutation #%d, platform #%d", (int)shader,
        (int)variantId, (int)permutationId, (int)platformId);

      const uint32_t shaderBinSize = permutation.size() * sizeof(uint32_t);
      shaderFile.write(&shaderBinSize, sizeof(shaderBinSize));
      shaderFile.write(permutation.data(), shaderBinSize);
    }
  }
}

#if IS_OFFLINE_SHADER_COMPILER
void NodeBasedShaderManager::getShadersBinariesFileNames(const String &shader_path, Tab<String> &file_names,
  PLATFORM platform_id) const
{
  file_names.clear();
  for (int platformId = 0; platformId < NodeBasedShaderManager::PLATFORM::SAVE_COUNT; ++platformId)
  {
    if (platform_id >= 0 && platform_id != platformId)
      continue;

    const String name = get_target_path_for_bin(shader_path, shaderFileSuffix, NodeBasedShaderManager::PLATFORM(platformId));
    file_names.push_back(name);
  }
}
#endif
