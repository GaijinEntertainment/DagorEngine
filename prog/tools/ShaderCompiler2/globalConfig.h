// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shCompilationInfo.h"
#include "DebugLevel.h"
#include "cppStcodePlatformInfo.h"

#include <ioSys/dag_dataBlock.h>
#include <dag/dag_vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
#elif _CROSS_TARGET_DX11
#include <d3dcompiler.h>
#elif _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
#include <spirv/compiler_dxc.h>
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
  const char *version = "2.77"; // logging only

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
  SimpleString engineRootDir;
  ShHardwareOptions singleOptions{4.0_sm};

  dag::Vector<String> dscIncludePaths;
  // use power_of_two strategy, because we have good enough hashes for strings
  ska::flat_hash_map<eastl::string, eastl::string> fileToFullPath;

  size_t dictionarySizeInKb = 4096;
  size_t shGroupSizeInKb = 1024;

  static constexpr int DEFAULT_COMPRESSION_LEVEL = 11;

  int shGroupCompressionLevel = DEFAULT_COMPRESSION_LEVEL;
  int shDumpCompressionLevel = DEFAULT_COMPRESSION_LEVEL;

  unsigned numProcesses = -1;
  unsigned numWorkers = 0;

  int hlslMaximumVsfAllowed = 2048;
  int hlslMaximumPsfAllowed = 2048;
  int hlslOptimizationLevel = 4;

  const DataBlock *assumedVarsConfig = &DataBlock::emptyBlock;

  DebugLevel hlslDebugLevel = DebugLevel::NONE;

  shader_layout::ExternalStcodeMode cppStcodeMode = shader_layout::ExternalStcodeMode::NONE;
  StcodeDynlibConfig cppStcodeCompConfig = StcodeDynlibConfig::DEV;
  StcodeTargetArch cppStcodeArch = StcodeTargetArch::DEFAULT;
  StcodeTargetPlatform cppStcodePlatform = StcodeTargetPlatform::DEFAULT;
  const char *cppStcodeCustomTag = nullptr;

  bool generateCppStcodeValidationData : 1 = false;

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
  bool constrainCompressedBindumpSize : 1 = true;
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
  bool logExactCompilationTimes : 1 = false;
  bool logFullPerFileCompilationStats : 1 = false;
  bool useSha1Cache : 1 = true;
  bool writeSha1Cache : 1 = true;
  bool purgeSha1 : 1 = false;
  bool enableFp16Override : 1 = false;
  bool saveDumpOnCrash : 1 = false;
  bool singleBuild : 1 = false;
  bool relinkOnly : 1 = false;
  bool useThreadpool : 1 = true;
  bool cppStcodeUnityBuild : 1 = false;
  bool cppStcodeDeleteDebugInfo : 1 = true;
  bool disallowHlslHardcodedRegs : 1 = false;

#if _CROSS_TARGET_DX12
  dx12::dxil::Platform targetPlatform = dx12::dxil::Platform::PC;
#elif _CROSS_TARGET_SPIRV
  bool compilerHlslCc : 1 = false;
  bool compilerDXC : 1 = false;
  bool usePcToken : 1 = true;
  bool dumpSpirvOnly : 1 = false;
#elif _CROSS_TARGET_METAL
  bool useIosToken : 1 = false;
  bool useBinaryMsl : 1 = false;
#endif

#if _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
  bool allowCompilationWithoutDxc : 1 = false;
  String dxcPath;
  dag::Vector<String> dxcParams;
  // @TODO: move to CompilationContext/ShCompilationInfo
  spirv::DXCContext *dxcContext;
#endif

  const char *getShaderSrcRoot() const { return shaderSrcRoot.empty() ? nullptr : shaderSrcRoot.c_str(); }
  bool compileCppStcode() const { return cppStcodeMode != shader_layout::ExternalStcodeMode::NONE; }
};

const CompilerConfig &config();
CompilerConfig &acquire_rw_config(); // For filling in init

} // namespace shc
