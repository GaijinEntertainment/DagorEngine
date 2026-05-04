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
#include <util/dag_finally.h>

String NodeBasedShaderManager::toolsPath;
String NodeBasedShaderManager::rootPath;

void NodeBasedShaderManager::initCompilation()
{
  toolsPath = ::dgs_get_settings()->getBlockByNameEx("debug")->getStr("toolsPathWin", "");
  rootPath = ::dgs_get_settings()->getBlockByNameEx("debug")->getStr("rootPathWin", "");
  if (rootPath.empty())
  {
    rootPath = toolsPath + "/../../..";
  }
}

static uint32_t calc_dshl_shader_source_hash(NodeBasedShaderType shader, const DataBlock &shader_blk)
{
  uint32_t finalShaderSourceHash = 0;
  String code = ShaderGraphRecompiler::substituteDshl(shader, shader_blk);
  finalShaderSourceHash = calc_crc32(reinterpret_cast<const unsigned char *>(code.c_str()), code.size(), finalShaderSourceHash);
  return finalShaderSourceHash;
}

bool NodeBasedShaderManager::compileScriptedShaders(const String &shader_name, const DataBlock &shader_blk, String &out_errors)
{
  const uint32_t newShaderSourceHash = calc_dshl_shader_source_hash(shader, shader_blk);
  if (newShaderSourceHash == lastShaderSourceHash)
    return lastCompileIsSuccess;

  if (scriptedShadersDumpHandle == INVALID_BINDUMP_HANDLE)
  {
    logerr("Recompiling NBS shader that failed to load initially is not supported yet.");
    return false;
  }

  // If our dump generation is out of date, then another nbs shader using the same dump has recompiled it, no need to do it again
  if (uint32_t curBindumpGen = get_shaders_bindump_generation(scriptedShadersDumpHandle); cachedDumpGeneration < curBindumpGen)
  {
    cachedDumpGeneration = curBindumpGen;
    lastShaderSourceHash = newShaderSourceHash;
    lastCompileIsSuccess = true;
    return true;
  }

  if (toolsPath.empty())
  {
    out_errors += "NodeBasedShaderManager::initCompilation was not called before recompiling shaders";
    return false;
  }

  const String shName = buildScriptedShaderName(shader_name);
  DataBlock shaderBlk = shader_blk; // @TODO: mutable
  shaderBlk.setStr("shader_name", "\n" + shName);
  const String tmpDumpNameBase(32, "%s.tmp", shName.str());
  const String tmpDumpName(32, "%s%s.ps50.shdump.bin", tmpDumpNameBase,
    get_nbsm_platform() == VULKAN_BINDLESS || get_nbsm_platform() == MTL_BINDLESS ? ".bindless" : "");

  auto writeFile = [&](const String &name, const String &content) {
    FullFileSaveCB file(name.str());
    if (!file.fileHandle)
    {
      out_errors.aprintf(0, "Cannot write to file '%s'", name.str());
      return false;
    }

    file.write(content.str(), content.length());
    return true;
  };
  auto readFile = [&](const String &name, String &output) {
    FullFileLoadCB file(name.str());
    if (!file.fileHandle)
    {
      out_errors.aprintf(0, "Cannot read file '%s'", name.str());
      return false;
    }

    output.resize(file.getTargetDataSize());
    file.read(output.str(), output.length());
    return true;
  };

  const String outputDshl = shName + ".tmp";
  const String outputDscBlk = shName + ".blk.tmp";

  FINALLY([&] {
    dd_erase(outputDshl.str());
    dd_erase(outputDscBlk.str());
    dd_erase(tmpDumpName.str());
    for (const alefind_t &ff : dd_find_iterator("./ShaderLog*", DA_FILE))
      dd_erase(ff.name);
  });

#if _TARGET_PC_WIN
  constexpr bool SKIP_WIN_CONSOLES = false;
  constexpr bool SKIP_VULKAN = false;
  constexpr bool SKIP_METAL = false;
#elif _TARGET_PC_LINUX
  constexpr bool SKIP_WIN_CONSOLES = true;
  constexpr bool SKIP_VULKAN = false;
  constexpr bool SKIP_METAL = true;
#elif _TARGET_PC_MACOSX
  constexpr bool SKIP_WIN_CONSOLES = true;
  constexpr bool SKIP_VULKAN = true;
  constexpr bool SKIP_METAL = false;
#else
  constexpr bool SKIP_WIN_CONSOLES = true;
  constexpr bool SKIP_VULKAN = true;
  constexpr bool SKIP_METAL = true;
#endif

  static constexpr struct
  {
    const char *platform = nullptr;
    const char *dscSuff = nullptr;
    const char *dumpSuff = nullptr;
    const char *additionalArgs = nullptr;
    bool skip = false;
  } DSC_TABLE[NodeBasedShaderManager::PLATFORM::COUNT] = {
    {"dx11", "hlsl11", "", "", SKIP_WIN_CONSOLES},
    {"", "", "", "", true},
    {"ps4", "ps4", "PS4", "", SKIP_WIN_CONSOLES},
    {"spirv", "spirv", "SpirV", "", SKIP_VULKAN},
    {"dx12", "dx12", "DX12", "", SKIP_WIN_CONSOLES},
    {"dx12x", "dx12", "DX12x", "-platform-xbox-one", SKIP_WIN_CONSOLES},
    {"dx12xs", "dx12", "DX12xs", "-platform-xbox-scarlett", SKIP_WIN_CONSOLES},
    {"ps5", "ps5", "PS5", "", SKIP_WIN_CONSOLES},
    {"metal", "metal", "MTL", "", SKIP_METAL},
    {"spirvb", "spirv", "SpirV", "-enableBindless:on", SKIP_VULKAN},
    {"metalb", "metal", "MTL", "-enableBindless:on", SKIP_METAL},
  };

  G_ASSERT_RETURN(!DSC_TABLE[get_nbsm_platform()].skip, false);

  String dscBlk = String(0,
    R"(
      shader_root_dir:t="."
      outDumpName:t="%s%s"
      engineRootDir:t="%s"
      compileCppStcode:b=no
      source {
        file:t="%s"
        includePath:t="%s/prog/gameLibs/webui/plugins/shaderEditors"
        includePath:t="%s/prog/gameLibs/render/shaders"
        includePath:t="%s/prog/gameLibs/daSDF/shaders"
      }
      Compile {
        fsh:t = 5.0
        additional_dump:b = yes
      }
    )",
    tmpDumpNameBase, DSC_TABLE[get_nbsm_platform()].dumpSuff, rootPath.str(), outputDshl, rootPath.str(), rootPath.str(),
    rootPath.str());
  String dshl = ShaderGraphRecompiler::substituteDshl(shader, shaderBlk);

  if (!writeFile(outputDshl, dshl))
    return false;
  if (!writeFile(outputDscBlk, dscBlk))
    return false;

  String tmpOutputFile(0, "tmplog_%s.log", outputDscBlk);

#if _TARGET_PC_WIN
  String cmd(0,
    "call %s\\dsc2-%s-dev.exe %s -q -shaderOn -nodisassembly -commentPP -codeDumpErr -r %s -o %s\\_output\\lshader\\shaders~%s -quiet "
    "> %s",
    toolsPath.str(), DSC_TABLE[get_nbsm_platform()].dscSuff, outputDscBlk, DSC_TABLE[get_nbsm_platform()].additionalArgs,
    rootPath.str(), DSC_TABLE[get_nbsm_platform()].platform, tmpOutputFile);
  cmd.replaceAll("/", "\\");
#elif _TARGET_PC_LINUX || _TARGET_PC_MACOSX
  String cmd(0,
    "%s/dsc2-%s-dev %s -q -shaderOn -nodisassembly -commentPP -codeDumpErr -r %s -o %s/_output/lshader/shaders~%s -quiet > %s",
    toolsPath.str(), DSC_TABLE[get_nbsm_platform()].dscSuff, outputDscBlk, DSC_TABLE[get_nbsm_platform()].additionalArgs,
    rootPath.str(), DSC_TABLE[get_nbsm_platform()].platform, tmpOutputFile);
#else
  String cmd("");
#endif

  if (cmd.empty())
    return false;

  FINALLY([&] { dd_erase(tmpOutputFile.c_str()); });

  if (system(cmd.str()) != 0)
  {
    String output{};
    if (readFile(tmpOutputFile, output))
      out_errors += output;
    return false;
  }

  if (!reload_shaders_bindump(scriptedShadersDumpHandle, tmpDumpNameBase.c_str(), scriptedShadersDumpName.c_str(), d3d::sm50))
    return false;

  cachedDumpGeneration = get_shaders_bindump_generation(scriptedShadersDumpHandle);
  lastShaderSourceHash = newShaderSourceHash;
  lastCompileIsSuccess = true;
  return true;
}
