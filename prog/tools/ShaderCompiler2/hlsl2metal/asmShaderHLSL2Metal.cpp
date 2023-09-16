#include "asmShaderHLSL2Metal.h"

#include "asmShaderHLSL2MetalDXC.h"
#include "asmShaderHLSL2MetalGlslang.h"

#include "../hlsl2spirv/pragmaScanner.h"

bool shouldUseDXC(const char *source)
{
  PragmaScanner scanner{source};
  while (auto pragma = scanner())
  {
    auto from = pragma.tokens();
    if (*from == std::string_view("spir-v"))
    {
      ++from;
      if (*from == std::string_view("compiler"))
      {
        ++from;
        if (*from == std::string_view("dxc"))
        {
          return true;
        }
      }
    }
  }
  return false;
}

CompileResult compileShaderMetal(const char *source, const char *profile, const char *entry, bool need_disasm, bool skipValidation,
  bool optimize, int max_constants_no, int bones_const_used, const char *shader_name, bool use_ios_token, bool use_binary_msl)
{
  const bool isDxc = shouldUseDXC(source);
  if (isDxc)
  {
    return compileShaderMetalDXC(source, profile, entry, need_disasm, skipValidation, optimize, max_constants_no, bones_const_used,
      shader_name, use_ios_token, use_binary_msl);
  }

  return compileShaderMetalGlslang(source, profile, entry, need_disasm, skipValidation, optimize, max_constants_no, bones_const_used,
    shader_name, use_ios_token, use_binary_msl);
}
