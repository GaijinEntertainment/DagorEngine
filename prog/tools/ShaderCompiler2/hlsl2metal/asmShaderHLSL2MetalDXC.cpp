#include "asmShaderHLSL2MetalDXC.h"

#include <algorithm>
#include <sstream>

#include <spirv2Metal/spirv.hpp>
#include <spirv2Metal/spirv_msl.hpp>
#include <spirv2Metal/spirv_cpp.hpp>

#include "hlsl2spirvCompile.h"

#include <ioSys/dag_fileIo.h>
#include <util/dag_string.h>

#include "HLSL2MetalCommon.h"
#include "buffBindPoints.h"

#include "../debugSpitfile.h"
#include "../hlsl2spirv/HLSL2SpirVCommon.h"
#include "dataAccumulator.h"
#include "spirv2MetalCompiler.h"
#include "metalShaderType.h"

#ifdef __APPLE__
#include <unistd.h>
#endif
#include <vector>
#include <fstream>
#include <optional>

#include <EASTL/span.h>

CompileResult compileShaderMetalDXC(const char *source, const char *profile, const char *entry, bool need_disasm, bool skipValidation,
  bool optimize, int max_constants_no, int bones_const_used, const char *shader_name, bool use_ios_token, bool use_binary_msl,
  uint64_t shader_variant_hash)
{
  CompileResult compileResult;

  (void)need_disasm;

  const ShaderType shaderStage = profile_to_shader_type(profile);
  if (shaderStage == ShaderType::Invalid)
  {
    compileResult.errors.sprintf("unknown target profile %s", profile);
    return compileResult;
  }

#if DAGOR_DBGLEVEL > 1
  optimize = false;
#endif

  auto hlsl2spirvResult = hlsl2spirv(source, profile, entry, skipValidation, compileResult);

  if (hlsl2spirvResult.failed)
    return compileResult;

  CompilerMSLlocal msl(hlsl2spirvResult.byteCode, use_ios_token);

  compileResult.computeShaderInfo.threadGroupSizeX = hlsl2spirvResult.computeInfo.threadGroupSizeX;
  compileResult.computeShaderInfo.threadGroupSizeY = hlsl2spirvResult.computeInfo.threadGroupSizeY;
  compileResult.computeShaderInfo.threadGroupSizeZ = hlsl2spirvResult.computeInfo.threadGroupSizeZ;

  return msl.convertToMSL(compileResult, hlsl2spirvResult.reflection, source, shaderStage, shader_name, entry, shader_variant_hash,
    use_binary_msl);
}
