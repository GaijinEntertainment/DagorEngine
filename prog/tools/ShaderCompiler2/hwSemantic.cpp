// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hwSemantic.h"
#include "globalConfig.h"
#include "shErrorReporting.h"
#include "globalConfig.h"
#include "fast_isalnum.h"
#include "shLog.h"
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_strUtil.h>

#include "shLog.h"


namespace semantic
{

bool compare_hw_token(int tok, const shc::CompilationContext &ctx)
{
  const ShHardwareOptions &hwopt = ctx.compInfo().hwopts();

  switch (tok)
  {
    // case SHADER_TOKENS::SHTOK_separate_ablend: return opt.sepABlend;
    case SHADER_TOKENS::SHTOK_fsh_4_0: return hwopt.fshVersion >= 4.0_sm;
    case SHADER_TOKENS::SHTOK_fsh_4_1: return hwopt.fshVersion >= 4.1_sm;
    case SHADER_TOKENS::SHTOK_fsh_5_0: return hwopt.fshVersion >= 5.0_sm;
    case SHADER_TOKENS::SHTOK_fsh_6_0: return hwopt.fshVersion >= 6.0_sm;
    case SHADER_TOKENS::SHTOK_fsh_6_2: return hwopt.fshVersion >= 6.2_sm;
    case SHADER_TOKENS::SHTOK_fsh_6_6: return hwopt.fshVersion >= 6.6_sm;

    case SHADER_TOKENS::SHTOK_pc: // backward comp
#if _CROSS_TARGET_DX12
      return shc::config().targetPlatform == dx12::dxil::Platform::PC;
#elif _CROSS_TARGET_DX11 | _CROSS_TARGET_EMPTY
      return true;
#elif _CROSS_TARGET_SPIRV
      return shc::config().usePcToken;
#elif _CROSS_TARGET_METAL
      return !shc::config().useIosToken;
#else
      return false;
#endif
      break;

    // @TODO: exclude _CROSS_TARGET_DX12 from here (but some obscure stuff could break, so careful)
    case SHADER_TOKENS::SHTOK_dx11:
#if _CROSS_TARGET_DX11 | _CROSS_TARGET_EMPTY
      return true;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_dx12:
#if _CROSS_TARGET_DX12
      return true;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_metaliOS:
#if _CROSS_TARGET_METAL
      return shc::config().useIosToken;
#else
      return false;
#endif
    case SHADER_TOKENS::SHTOK_metal:
#if _CROSS_TARGET_METAL
      return true;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_vulkan:
#if _CROSS_TARGET_SPIRV
      return true;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_ps4:
#if _CROSS_TARGET_C1

#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_ps5:
#if _CROSS_TARGET_C2

#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_xbox:
#if _CROSS_TARGET_DX12
      // on dx12 we have different profiles with the same compiler
      return dx12::dxil::is_xbox_platform(shc::config().targetPlatform);
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_scarlett:
#if _CROSS_TARGET_DX12
      // on dx12 we have different profiles with the same compiler
      return dx12::dxil::Platform::XBOX_SCARLETT == shc::config().targetPlatform;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_mesh:
#if _CROSS_TARGET_DX12
      // Not all DX12 targets support mesh shaders, this depends on the target platform
      return dx12::dxil::platform_has_mesh_support(shc::config().targetPlatform);
#elif _CROSS_TARGET_METAL
      return true;
#else
      return false;
#endif

    case SHADER_TOKENS::SHTOK_bindless: return shc::config().enableBindless;

    default: G_ASSERT(0);
  }

  return false;
}

static bool hlsl_profile_is_valid(const char *profile, size_t len)
{
  constexpr eastl::array VALID_PROFILES = {'p', 'c', 'v', 'h', 'd', 'g', 'm', 'a', 'e', 'l'};
  constexpr char NULL_SUFF[] = "s_null";
  constexpr char TARGET_SUFF[] = "s_#_#"; // # := any digit

  if (eastl::find(VALID_PROFILES.begin(), VALID_PROFILES.end(), profile[0]) == VALID_PROFILES.end())
    return false;

  ++profile;
  --len;

  auto compare = []<size_t N>(const char *profile, size_t len, const char(&required)[N]) {
    auto isNum = [](char c) { return c >= '0' && c <= '9'; };

    if (len != N - 1)
      return false;
    for (size_t i = 0; i < len; ++i)
    {
      if (profile[i] != required[i] && (required[i] != '#' || !isNum(profile[i])))
        return false;
    }
    return true;
  };

  // Check if @s_null
  if (compare(profile, len, NULL_SUFF))
    return true;

  // Check [p|c|v|h|d|g|m|a|e|l]s_[0-9]_[0-9]
  return compare(profile, len, TARGET_SUFF);
}

HlslCompileClass parse_hlsl_compilation_info(ShaderTerminal::hlsl_compile_class &hlsl_compile, Parser &parser,
  const shc::CompilationContext &ctx)
{
  // Get source and settings.
  const ShHardwareOptions &hwopt = ctx.compInfo().hwopts();

  String profile(hlsl_compile.profile->text + 1);
  String entry(hlsl_compile.entry->text + 1);
  erase_items(profile, profile.length() - 1, 1);
  erase_items(entry, entry.length() - 1, 1);
  if (strcmp(profile + 1, "s_set_target") == 0)
  {
    HlslCompilationStage stage = valid_profile_to_hlsl_stage(profile);
    if (!item_is_in(stage, {HLSL_VS, HLSL_PS, HLSL_CS}))
    {
      sh_debug(SHLOG_ERROR, "Yet unsupported target %s", profile.c_str());
      return {};
    }
    HlslSetDefaultTargetDirective defaultDirective{entry};
    return {defaultDirective, stage};
  }

  HlslCompileDirective compileDirective{};
  compileDirective.useHalfs = false;

  if (dd_stricmp(profile, "target_vs_for_gs") == 0)
  {
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#else
    profile = "target_vs";
#endif
  }
  if (dd_stricmp(profile, "target_vs_for_tess") == 0)
  {
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#else
    profile = "target_vs";
#endif
  }

  if (shc::config().enableFp16Override)
    compileDirective.useHalfs = true;
  else if (str_ends_with_c(profile, "_half"))
  {
    if (hwopt.enableHalfProfile)
      compileDirective.useHalfs = true;
    profile = String(profile.c_str(), profile.length() - 5);
  }

  profile.toLower();

  constexpr char TGT_PREF[] = "target_";

  const bool useDefTarget = dd_strnicmp(profile, TGT_PREF, LITSTR_LEN(TGT_PREF)) == 0;
  const char *truncProfileForDef = useDefTarget ? profile.data() + LITSTR_LEN(TGT_PREF) : nullptr;
  const HlslCompilationStage stage =
    useDefTarget ? profile_to_hlsl_stage(truncProfileForDef) : profile_to_hlsl_stage(profile.c_str(), 2);

  if (stage == HLSL_INVALID || (!useDefTarget && !hlsl_profile_is_valid(profile, profile.length())))
  {
    report_error(parser, hlsl_compile.profile,
      "Invalid user-defined target %s: target must be of form [p|c|v|h|d|g|m|a|e|l]s_[0-9]_[0-9], or one of target_#s",
      profile.c_str());
    return {};
  }

#if _CROSS_TARGET_DX12
  // Here is the only place where we check that compile for MS is actually a valid thing to do
  if ((stage == HLSL_MS || stage == HLSL_AS) && !dx12::dxil::platform_has_mesh_support(shc::config().targetPlatform))
  {
    report_error(parser, hlsl_compile.profile, "Platform doesn't support mesh pipeline, can't compile %cs shaders", profile[0]);
    return {};
  }
#endif

  if (useDefTarget)
  {
    compileDirective.useDefaultProfileIfExists = item_is_in(stage, {HLSL_VS, HLSL_PS, HLSL_CS});

    if (item_is_in(stage, {HLSL_VS, HLSL_PS, HLSL_GS}))
    {
      if (!hwopt
             .fshVersion //
             .map(6.6_sm, "%cs_6_6")
             .map(6.0_sm, "%cs_6_0")
             .map(5.0_sm, "%cs_5_0")
             .map(4.1_sm, "%cs_4_1")
             .map(4.0_sm, "%cs_4_0")
             .get([&](auto fmt) { profile.printf(0, fmt, hlsl_stage_to_profile_letter(stage)); }))
      {
        sh_debug(SHLOG_ERROR, "can't find out target for target_%s/%s", HLSL_ALL_PROFILES[stage], d3d::as_ps_string(hwopt.fshVersion));
        return {};
      }
    }
    else if (item_is_in(stage, {HLSL_CS, HLSL_HS, HLSL_DS}))
    {
      if (!hwopt
             .fshVersion //
             .map(6.6_sm, "%cs_6_6")
             .map(6.0_sm, "%cs_6_0")
             .map(5.0_sm, "%cs_5_0")
             .get([&](auto fmt) { profile.printf(0, fmt, hlsl_stage_to_profile_letter(stage)); }))
      {
        sh_debug(SHLOG_ERROR, "can't find out target for target_%s/%s", HLSL_ALL_PROFILES[stage], d3d::as_ps_string(hwopt.fshVersion));
        return {};
      }
    }
    else // MS/AS
    {
      G_ASSERT(item_is_in(stage, {HLSL_MS, HLSL_AS}));
      if (!hwopt
             .fshVersion //
             .map(6.6_sm, "%cs_6_6")
             // Requires min profile of 6.5, as mesh shaders are a optional feature, they are available on all
             // targets starting from 5.0.
             .map(6.0_sm, "%cs_6_5")
             .map(5.0_sm, "%cs_6_5")
             .get([&](auto fmt) { profile.printf(0, fmt, hlsl_stage_to_profile_letter(stage)); }))
      {
        sh_debug(SHLOG_ERROR, "can't find out target for target_%s/%s", HLSL_ALL_PROFILES[stage], d3d::as_ps_string(hwopt.fshVersion));
        return {};
      }
    }
  }

  if (strcmp(profile, "ps_null") == 0)
    return {};

  compileDirective.profile = eastl::move(profile);
  compileDirective.entry = eastl::move(entry);
  return {eastl::move(compileDirective), stage};
}

bool validate_hardcoded_regs_in_hlsl_block(const ShaderTerminal::SHTOK_hlsl_text *hlsl)
{
  if (!shc::config().disallowHlslHardcodedRegs)
    return true;

  if (!hlsl || !hlsl->text)
    return true;

  constexpr char regToken[] = "register";
  constexpr size_t tokLen = sizeof(regToken) - 1;
  for (const char *p = strstr(hlsl->text, regToken); p; p = strstr(p, regToken))
  {
    const bool isStartOfToken = (p == hlsl->text) || !fast_isalnum_or_(p[-1]); // neg index is ok because p != text beginning
    const bool isSeparateToken = isStartOfToken && !fast_isalnum_or_(p[tokLen]);
    if (isSeparateToken)
    {
      const char *begin = p, *end = p + tokLen;
      while (begin != hlsl->text && begin[-1] != '\n')
        --begin;
      while (*end && *end != '\n')
        ++end;
      const String targetLine{begin, static_cast<int>(end - begin)};
      sh_debug(SHLOG_FATAL, "Old-style raw hardcoded registers are disallowed, please use the stcode version:\n%s",
        targetLine.c_str());
      return false;
    }

    p += tokLen;
  }

  return true;
}

} // namespace semantic
