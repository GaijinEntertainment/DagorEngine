// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../compileResult.h"
#include <spirv/compiler_dxc.h>
#include <generic/dag_span.h>

// Every member defaults to off, so a caller only spells out what it needs and a new option costs no
// churn at unrelated call sites. Mirrors dx12::dxil::CompileInputs on the DX12 backend.
struct SpirVCompileInputs
{
  const spirv::DXCContext *dxcCtx = nullptr;
  const char *source = nullptr;
  const char *profile = nullptr;
  const char *entry = nullptr;
  const char *shaderName = nullptr;
  uint64_t shaderVariantHash = 0;
  int maxConstantsNo = 0;
  bool needDisasm = false;
  bool hlsl2021 = false;
  bool enableFp16 = false;
  bool skipValidation = false;
  bool optimize = false;
  bool enableBindless = false;
  bool embedDebugData = false;
  bool dumpSpirvOnly = false;
  bool validateGlobalConstsOffsetOrder = false;
  bool noConversionWarnings = false;
  // Validate buffer layouts with Vulkan scalar block layout instead of the strict VULKAN_1_0 rules,
  // which demand 16-aligned float3. Requires the scalarBlockLayout device feature at runtime.
  bool useScalarLayout = false;
};

CompileResult compileShaderSpirV(const SpirVCompileInputs &inputs);

eastl::string disassembleShaderSpirV(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata);
