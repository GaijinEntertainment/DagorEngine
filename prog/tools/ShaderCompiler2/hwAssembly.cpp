// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hwAssembly.h"
#include "hwSemantic.h"
#include "shCompContext.h"
#include "globalConfig.h"

#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
#endif


namespace assembly
{

static eastl::string to_upcase_str(const auto &s)
{
  eastl::string res = s;
  for (char &c : res)
  {
    if (c >= 'a' && c <= 'z')
      c += 'A' - 'a';
  }
  return res;
};

eastl::string build_common_hardware_defines_hlsl(const shc::CompilationContext &ctx)
{
  eastl::string res{};

#define ADD_HW_MACRO(TOKEN)                                          \
  if (semantic::compare_hw_token(SHADER_TOKENS::SHTOK_##TOKEN, ctx)) \
  res.append_sprintf("#define _HARDWARE_%s 1\n", to_upcase_str(#TOKEN).c_str())
  ADD_HW_MACRO(pc);
  ADD_HW_MACRO(dx11);
  ADD_HW_MACRO(dx12);
  ADD_HW_MACRO(metaliOS);
  ADD_HW_MACRO(metal);
  ADD_HW_MACRO(vulkan);
  ADD_HW_MACRO(ps4);
  ADD_HW_MACRO(ps5);
  ADD_HW_MACRO(xbox);
  ADD_HW_MACRO(scarlett);
  ADD_HW_MACRO(fsh_4_0);
  ADD_HW_MACRO(fsh_4_1);
  ADD_HW_MACRO(fsh_5_0);
  ADD_HW_MACRO(fsh_6_0);
  ADD_HW_MACRO(fsh_6_2);
  ADD_HW_MACRO(fsh_6_6);
#undef ADD_HW_MACRO

  // add target-specific predefines
  const char *predefines_str =
#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_DX12
    dx12::dxil::Platform::XBOX_ONE == shc::config().targetPlatform
      ?
#include "predefines_dx12x.hlsl.inl"
      : dx12::dxil::Platform::XBOX_SCARLETT == shc::config().targetPlatform ?
#include "predefines_dx12xs.hlsl.inl"
                                                                            :
#include "predefines_dx12.hlsl.inl"
#elif _CROSS_TARGET_DX11
#include "predefines_dx11.hlsl.inl"
#elif _CROSS_TARGET_SPIRV
#include "predefines_spirv.hlsl.inl"
#elif _CROSS_TARGET_METAL
#include "predefines_metal.hlsl.inl"
#else
    nullptr
#endif
    ;

  if (shc::config().hlslDefines.length())
    res.append(shc::config().hlslDefines.c_str());
  if (predefines_str)
    res.append(predefines_str);
  res.push_back('\n');

  return res;
}

#if _CROSS_TARGET_C2







#endif

eastl::string build_hardware_defines_hlsl(const char *profile, bool use_halfs, const shc::CompilationContext &ctx)
{
  eastl::string defines{};

  defines.sprintf("#define _SHADER_STAGE_%cS 1\n", toupper(profile[0]));
  defines += ctx.commonHlslDefines();

  if (profile)
  {
    // Format required: ..._#_#, where # are digits in the shader model version,
    // for user-defined profiles this is validated on hlsl_compile evaluation.
    auto shaderModelLe = [](const char *t1, const char *t2) {
      auto isNum = [](char c) { return c >= '0' && c <= '9'; };
      size_t l1 = strlen(t1);
      size_t l2 = strlen(t2);
      G_ASSERT(l1 >= 3 && l2 >= 3);
      t1 += l1 - 3;
      t2 += l2 - 3;
      G_ASSERT(isNum(t1[0]) && isNum(t2[0]) && isNum(t1[2]) && isNum(t2[2]));
      G_ASSERT(t1[1] == '_' && t2[1] == '_');
      return t1[0] < t2[0] || (t1[0] == t2[0] && t1[2] <= t2[2]);
    };

#define TRY_ADD_HW_MACRO(TOKEN)                                                             \
  do                                                                                        \
  {                                                                                         \
    const eastl::string tok = to_upcase_str(#TOKEN);                                        \
    if (defines.find(tok.c_str()) == eastl::string::npos && shaderModelLe(#TOKEN, profile)) \
      defines.append_sprintf("#define _HARDWARE_%s 1\n", tok.c_str());                      \
  } while (0)
    TRY_ADD_HW_MACRO(fsh_4_0);
    TRY_ADD_HW_MACRO(fsh_4_1);
    TRY_ADD_HW_MACRO(fsh_5_0);
    TRY_ADD_HW_MACRO(fsh_6_0);
    TRY_ADD_HW_MACRO(fsh_6_2);
    TRY_ADD_HW_MACRO(fsh_6_6);
#undef TRY_ADD_HW_MACRO
  }

  if (use_halfs)
    defines += "#define _USE_HALFS 1\n";

  return defines;
}

} // namespace assembly