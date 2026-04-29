// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../compileResult.h"
#include "../DebugLevel.h"
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <generic/dag_tabFwd.h>


struct ID3DXBuffer;
struct TmpmemAlloc;
typedef int VPRTYPE;
struct ShaderStageData;

namespace dx12
{
namespace dxil
{
enum class Platform
{
  PC,
  XBOX_ONE,
  XBOX_SCARLETT
};
inline bool is_xbox_platform(Platform p) { return Platform::XBOX_ONE == p || Platform::XBOX_SCARLETT == p; }
inline bool platform_has_mesh_support(Platform p) { return Platform::XBOX_ONE != p; }

inline bool use_two_phase_compilation(Platform p) { return is_xbox_platform(p); }

struct CompilationOptions
{
  bool optimize;
  bool skipValidation;
  bool debugInfo;
  bool scarlettW32;
  bool hlsl2021;
  bool enableFp16;
};

struct CompileInputs
{
  const char *name;
  const char *profile;
  const char *entry;
  dag::ConstSpan<char> source;
  bool needDisasm;
  int maxConstantsNo;
  Platform platform;
  bool warningsAsErrors;
  bool embedSource;
  DebugLevel debugLevel;
  CompilationOptions compilationOptions;
  wchar_t *PDBDir;
  dag::ConstSpan<::dxil::StreamOutputComponentInfo> streamOutputComponents;
};
// NOTE: platform has to be the same for each call for the currently running instance
// otherwise the selected platform is undefined, especially when multiple threads at
// the same time call this function.
CompileResult compileShader(const CompileInputs &inputs);

void combineShaders(SmallTab<unsigned, TmpmemAlloc> &meta, SmallTab<unsigned, TmpmemAlloc> &bytecode, const ShaderStageData &vs,
  const ShaderStageData &hs, const ShaderStageData &ds, const ShaderStageData &gs, unsigned id);

struct RootSignatureStore
{
  ::dxil::ShaderResourceUsageTable vsResources;
  ::dxil::ShaderResourceUsageTable hsResources;
  ::dxil::ShaderResourceUsageTable dsResources;
  ::dxil::ShaderResourceUsageTable gsResources;
  ::dxil::ShaderResourceUsageTable psResources;
  bool hasVertexInput;
  bool isMesh;
  bool hasAmplificationStage;
  bool hasAccelerationStructure;
  bool hasStreamOutput;
  bool useResourceDescriptorHeapIndexing;
  bool useSamplerDescriptorHeapIndexing;
};

RootSignatureStore generateRootSignatureDefinition(dag::ConstSpan<uint8_t> vs_metadata, dag::ConstSpan<uint8_t> hs_metadata,
  dag::ConstSpan<uint8_t> ds_metadata, dag::ConstSpan<uint8_t> gs_metadata, dag::ConstSpan<uint8_t> ps_metadata);

struct CombinedShaderData
{
  dag::ConstSpan<uint8_t> metadata;
  dag::ConstSpan<unsigned> bytecode;
};

struct CombinedShaderStorage
{
  eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>> metadata{};
  eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>> bytecode{};
};

bool comparePhaseOneVertexProgram(CombinedShaderData combined_vprog, const ShaderStageData &vs, const ShaderStageData &hs,
  const ShaderStageData &ds, const ShaderStageData &gs, const RootSignatureStore &signature);

bool comparePhaseOnePixelShader(CombinedShaderData combined_psh, const ShaderStageData &ps, const RootSignatureStore &signature);

CombinedShaderStorage combinePhaseOneVertexProgram(const ShaderStageData &vs, const ShaderStageData &hs, const ShaderStageData &ds,
  const ShaderStageData &gs, const RootSignatureStore &signature, unsigned id, CompilationOptions options);

CombinedShaderStorage combinePhaseOnePixelShader(const ShaderStageData &ps, const RootSignatureStore &signature, unsigned id,
  bool has_gs, bool has_ts, CompilationOptions options);

eastl::optional<CombinedShaderStorage> recompileVertexProgram(dag::ConstSpan<uint8_t> source, Platform platform, wchar_t *pdb_dir,
  DebugLevel debug_level, bool embed_source);

eastl::optional<CombinedShaderStorage> recompilePixelShader(dag::ConstSpan<uint8_t> source, Platform platform, wchar_t *pdb_dir,
  DebugLevel debug_level, bool embed_source);
} // namespace dxil
} // namespace dx12
