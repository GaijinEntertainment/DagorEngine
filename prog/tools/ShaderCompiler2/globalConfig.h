// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shHardwareOpt.h"
#include "DebugLevel.h"
#include "shadervarGenerator.h"
#include "cppStcodePlatformInfo.h"

#include <dag/dag_vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
#elif _CROSS_TARGET_DX11
#include <d3dcompiler.h>
#else
#if _TARGET_PC_WIN
#include <d3dcompiler.h>
#endif
#endif

namespace shc
{
struct CompilerConfig
{
  // See https://gaijinentertainment.github.io/DagorEngine/dagor-tools/shader-compiler/contributing_to_compiler.html#versioning
  const char *version = "2.72";

  const char *singleCompilationShName = nullptr;
  const char *intermediateDir = nullptr;
  const char *logDir = nullptr;
  const char *outDumpNameConfig = nullptr;
  const char *sha1CacheDir = nullptr;

  const char *crossCompiler =
#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_METAL
    "metal"
#elif _CROSS_TARGET_SPIRV
    "spirv"
#elif _CROSS_TARGET_EMPTY
    "stub"
#elif _CROSS_TARGET_DX12
    "dx12"
#elif _CROSS_TARGET_DX11 //_TARGET_PC is also defined
    "dx11"
#endif
    ;

#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#elif _CROSS_TARGET_DX12
  wchar_t *dx12PdbCacheDir = nullptr;
#endif

  String hlslDefines;
  SimpleString shaderSrcRoot;
  ShHardwareOptions singleOptions{4.0_sm};

  std::string shadervarsCodeTemplateFilename;
  GeneratedPathInfos generatedPathInfos;
  dag::Vector<std::string> excludeFromGeneration;

  dag::Vector<String> dscIncludePaths;
  // use power_of_two strategy, because we have good enough hashes for strings
  ska::flat_hash_map<eastl::string, eastl::string> fileToFullPath;

  size_t dictionarySizeInKb = 4096;
  size_t shGroupSizeInKb = 1024;

  unsigned numProcesses = -1;
  unsigned numWorkers = 0;

  int hlslMaximumVsfAllowed = 2048;
  int hlslMaximumPsfAllowed = 2048;
  int hlslOptimizationLevel = 4;

  DebugLevel hlslDebugLevel = DebugLevel::NONE;
  ShadervarGeneratorMode shadervarGeneratorMode = ShadervarGeneratorMode::None;
  StcodeTargetArch cppStcodeArch = StcodeTargetArch::DEFAULT;

#if USE_BINDLESS_FOR_STATIC_TEX
  bool enableBindless : 1 = true;
#else
  bool enableBindless : 1 = false;
#endif
#if _CROSS_TARGET_DX12
  bool useGitTimestamps : 1 = true;
#else
  bool useGitTimestamps : 1 = false;
#endif
  bool forceRebuild : 1 = false;
  bool forceRelink : 1 = false;
  bool shortMessages : 1 = false;
  bool noSave : 1 = false;
  bool hlslSavePPAsComments : 1 = false;
  bool isDebugModeEnabled : 1 = false;
  bool hlslEmbedSource : 1 = false;
  bool hlslSkipValidation : 1 = false;
  bool hlslNoDisassembly : 1 = false;
  bool hlslShowWarnings : 1 = false;
  bool hlslWarningsAsErrors : 1 = false;
  bool hlslDumpCodeAlways : 1 = false;
  bool hlslDumpCodeSeparate : 1 = false;
  bool hlslDumpCodeOnError : 1 = false;
  bool validateIdenticalBytecode : 1 = false;
  bool hlsl2021 : 1 = false;
  bool useCompression : 1 = true;
  bool addTextureType : 1 = false;
  bool suppressLogs : 1 = false;
  bool optionalIntervalsAsBranches : 1 = false;
  bool logExactCompilationTimes : 1 = false;
  bool logFullPerFileCompilationStats : 1 = false;
  bool autotestMode : 1 = false;
  bool useSha1Cache : 1 = true;
  bool writeSha1Cache : 1 = true;
  bool purgeSha1 : 1 = false;
  bool enableFp16 : 1 = false;
  bool saveDumpOnCrash : 1 = false;
  bool singleBuild : 1 = false;
  bool relinkOnly : 1 = false;
  bool useThreadpool : 1 = true;
  bool compileCppStcode : 1 = false;
  bool cppStcodeUnityBuild : 1 = false;
  bool disallowHlslHardcodedRegs : 1 = false;

#if _CROSS_TARGET_DX12
  dx12::dxil::Platform targetPlatform = dx12::dxil::Platform::PC;
#elif _CROSS_TARGET_SPIRV
  bool compilerHlslCc : 1 = false;
  bool compilerDXC : 1 = false;
#elif _CROSS_TARGET_METAL
  bool useIosToken : 1 = false;
  bool useBinaryMsl : 1 = false;
#endif

  const char *getShaderSrcRoot() const { return shaderSrcRoot.empty() ? nullptr : shaderSrcRoot.c_str(); }
};

const CompilerConfig &config();
CompilerConfig &acquire_rw_config(); // For filling in init

} // namespace shc
