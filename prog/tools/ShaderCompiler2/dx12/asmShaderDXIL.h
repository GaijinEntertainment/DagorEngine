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

// NOTE: platform has to be the same for each call for the currently running instance
// otherwise the selected platform is undefined, especially when multiple threads at
// the same time call this function.
CompileResult compileShader(dag::ConstSpan<char> source, const char *profile, const char *entry, bool need_disasm, bool hlsl2021,
  bool enableFp16, bool skipValidation, bool optimize, bool debug_info, wchar_t *pdb_dir, int max_constants_no, const char *name,
  Platform platform, bool scarlett_w32, bool warnings_as_errors, DebugLevel debug_level, bool embed_source,
  dag::ConstSpan<::dxil::StreamOutputComponentInfo> stream_output_components);

void combineShaders(SmallTab<unsigned, TmpmemAlloc> &target, dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs,
  dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, unsigned id);

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
};

struct CompilationOptions
{
  bool optimize;
  bool skipValidation;
  bool debugInfo;
  bool scarlettW32;
  bool hlsl2021;
  bool enableFp16;
};

RootSignatureStore generateRootSignatureDefinition(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs,
  dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, dag::ConstSpan<unsigned> ps);

bool comparePhaseOneVertexProgram(dag::ConstSpan<unsigned> combined_vprog, dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs,
  dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, const RootSignatureStore &signature);

bool comparePhaseOnePixelShader(dag::ConstSpan<unsigned> combined_psh, dag::ConstSpan<unsigned> ps,
  const RootSignatureStore &signature);

eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>> combinePhaseOneVertexProgram(dag::ConstSpan<unsigned> vs,
  dag::ConstSpan<unsigned> hs, dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, const RootSignatureStore &signature,
  unsigned id, CompilationOptions options);

eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>> combinePhaseOnePixelShader(dag::ConstSpan<unsigned> ps,
  const RootSignatureStore &signature, unsigned id, bool has_gs, bool has_ts, CompilationOptions options);

eastl::optional<eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>>> recompileVertexProgram(dag::ConstSpan<unsigned> source,
  Platform platform, wchar_t *pdb_dir, DebugLevel debug_level, bool embed_source);

eastl::optional<eastl::unique_ptr<SmallTab<unsigned, TmpmemAlloc>>> recompilePixelSader(dag::ConstSpan<unsigned> source,
  Platform platform, wchar_t *pdb_dir, DebugLevel debug_level, bool embed_source);
} // namespace dxil
} // namespace dx12
