//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/span.h>

namespace dxil
{
struct CompileResult
{
  eastl::vector<uint8_t> byteCode;
  // with the version 3 interface of DXC we can extract reflection data as separate blob
  eastl::vector<uint8_t> reflectionData;
  eastl::string logMessage;
};
enum class PDBMode
{
  NONE,
  SMALL,
  FULL
};
enum class DXCVersion
{
  // Limitations dictated by found version
  PC,
  // XBOX_* recognizes #define __XBOX_STRIP_DXIL 1 to remove DXIL
  XBOX_ONE,
  XBOX_SCARLETT,
};
inline bool recognizesXboxStripDxilMacro(DXCVersion v) { return DXCVersion::XBOX_ONE == v || DXCVersion::XBOX_SCARLETT == v; }
struct DXCSettings
{
  // when used auto binding of resources (eg c or no register set)
  // resources are placed in this binding space
  uint32_t autoBindSpace = 0;
  // Enable fp16 type support.
  bool enableFp16 = false;
  bool warningsAsErrors = false;
  bool enforceIEEEStrictness = false;
  bool disableValidation = false;
  bool assumeAllResourcesBound = false;
  bool xboxPhaseOne = false;
  bool pipelineHasGeoemetryStage = false;
  bool pipelineHasTesselationStage = false;
  bool pipelineIsMesh = false;
  bool pipelineHasAmplification = false;
  bool scarlettWaveSize32 = false;
  bool saveHlslToBlob = false;
  bool hlsl2021 = false;
  // 0 (disable) - 3 (best)
  uint32_t optimizeLevel = 3;
  PDBMode pdbMode = PDBMode::NONE;
  eastl::wstring_view PDBBasePath;
  eastl::string_view rootSignatureDefine;
};
CompileResult compileHLSLWithDXC(dag::ConstSpan<char> src, const char *entry, const char *profile, DXCSettings settings, void *dxc_lib,
  DXCVersion dxc_version);
eastl::string disassemble(eastl::span<const uint8_t> data, void *dxc_lib);
} // namespace dxil
