// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "namedConst.h"
#include "shTargetContext.h"
#include "transcodeCommon.h"
#include "globalConfig.h"
#include "shLog.h"
#include "linkShaders.h"
#include "cppStcode.h"
#include "const3d.h"
#include <math/dag_mathBase.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shUtils.h>
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>
#include "hash.h"
#include "fast_isalnum.h"
#include <EASTL/bitvector.h>
#include <EASTL/string_view.h>

NamedConstBlock::CstHandle NamedConstBlock::addConst(int stage, const char *name, HlslRegisterSpace reg_space, int sz,
  int hardcoded_reg, bool is_dynamic, bool is_global_const_block)
{
  G_ASSERT(sz > 0);
  G_ASSERT(is_dynamic || hardcoded_reg == -1);

  RegisterProperties NamedConstBlock::*propsField = nullptr;
  HlslRegAllocator *regAlloc = nullptr;
  if (reg_space == HLSL_RSPACE_C && (!is_dynamic || is_global_const_block))
  {
    propsField = &NamedConstBlock::bufferedConstProps;
    regAlloc = &bufferedConstsRegAllocator;
  }
  else if (!is_dynamic)
  {
    G_ASSERT(!shc::config().enableBindless);
    G_ASSERT(reg_space == HLSL_RSPACE_T || reg_space == HLSL_RSPACE_S);
    G_ASSERT(hardcoded_reg == -1);

    // @TODO: assert sampler-texture slot coherency
    propsField = stage == STAGE_VS ? &NamedConstBlock::vertexProps : &NamedConstBlock::pixelProps;
    regAlloc = reg_space == HLSL_RSPACE_T
                 ? (stage == STAGE_VS ? &slotTextureSuballocators.vsTex : &slotTextureSuballocators.psTex)
                 : (stage == STAGE_VS ? &slotTextureSuballocators.vsSamplers : &slotTextureSuballocators.psSamplers);
  }
  else if (stage == STAGE_VS)
  {
    propsField = &NamedConstBlock::vertexProps;
    regAlloc = &vertexRegAllocators[reg_space];
  }
  else
  {
    propsField = &NamedConstBlock::pixelProps;
    regAlloc = &pixelOrComputeRegAllocators[reg_space];
  }

  RegisterProperties &props = this->*propsField;

  auto &xsn = props.sn;
  auto &xsc = props.sc;

  auto nameProvider = makeInfoProvider(propsField, reg_space);

  int id = xsn.getNameId(name);
  if (id == -1)
  {
    int regIndex = 0;
    if (hardcoded_reg >= 0)
    {
      // @TODO: this would be nicer with monadic ops on Expected
      if (auto res = regAlloc->reserve(HlslSlotSemantic::HARDCODED, hardcoded_reg, sz); !res)
      {
        report_reg_reserve_failed(name, hardcoded_reg, sz, reg_space, HlslSlotSemantic::HARDCODED, res.error(), *regAlloc,
          nameProvider);
      }

      regIndex = hardcoded_reg;
    }
    else
    {
      regIndex = regAlloc->allocate(sz);
      if (regIndex == -1)
      {
        sh_debug(SHLOG_ERROR, "Failed to allocate %i register%s for param '%s'\n%s", sz, sz > 1 ? "s" : "", name,
          get_reg_alloc_dump(*regAlloc, reg_space, nameProvider).c_str());
      }
    }
    id = xsn.addNameId(name);
    xsc.resize(id + 1);
    xsc[id].rspace = reg_space;
    xsc[id].size = sz;
    xsc[id].regIndex = regIndex;
    xsc[id].isDynamic = is_dynamic;

    return {ShaderStage(stage), id, &props == &bufferedConstProps};
  }
  return CstHandle::makeInvalidHandle(ShaderStage(stage), &props == &bufferedConstProps);
}

bool NamedConstBlock::initSlotTextureSuballocators(int vs_tex_count, int ps_tex_count)
{
  G_ASSERT(!shc::config().enableBindless);

  int vsTexRangeBase = 0;
  int psTexRangeBase = 0;
  int vsSamplerRangeBase = 0;
  int psSamplerRangeBase = 0;

  if (vs_tex_count)
  {
    vsTexRangeBase = vertexRegAllocators[HLSL_RSPACE_T].allocate(vs_tex_count);
    vsSamplerRangeBase = vertexRegAllocators[HLSL_RSPACE_S].allocate(vs_tex_count);
  }
  if (ps_tex_count)
  {
    psTexRangeBase = pixelOrComputeRegAllocators[HLSL_RSPACE_T].allocate(ps_tex_count);
    psSamplerRangeBase = pixelOrComputeRegAllocators[HLSL_RSPACE_S].allocate(ps_tex_count);
  }

  if (vsTexRangeBase < 0 || psTexRangeBase < 0 || vsSamplerRangeBase < 0 || psSamplerRangeBase < 0)
  {
    eastl::string errorMessage{};
    auto reportIfFailed = [&](int base, int count, ShaderStage stage, HlslRegisterSpace rspace) {
      if (base < 0)
      {
        auto NamedConstBlock::*propsField = stage == STAGE_VS ? &NamedConstBlock::vertexProps : &NamedConstBlock::pixelProps;
        auto &props = this->*propsField;
        auto &allocators = stage == STAGE_VS ? vertexRegAllocators : pixelOrComputeRegAllocators;
        errorMessage.append_sprintf("\nFailed to allocate %d contiguous static %s %s.\n", vs_tex_count,
          SHADER_STAGE_SHORT_NAMES[stage], rspace == HLSL_RSPACE_S ? "samplers" : "textures");
        errorMessage += get_reg_alloc_dump(allocators[rspace], rspace, makeInfoProvider(propsField, rspace));
      }
    };
    reportIfFailed(vsTexRangeBase, vs_tex_count, STAGE_VS, HLSL_RSPACE_T);
    reportIfFailed(vsSamplerRangeBase, vs_tex_count, STAGE_VS, HLSL_RSPACE_S);
    reportIfFailed(psTexRangeBase, ps_tex_count, STAGE_PS, HLSL_RSPACE_T);
    reportIfFailed(psSamplerRangeBase, ps_tex_count, STAGE_PS, HLSL_RSPACE_S);
    return false;
  }

  if (vs_tex_count)
  {
    slotTextureSuballocators.vsTex =
      HlslRegAllocator{HlslRegAllocator::Policy{uint32_t(vsTexRangeBase), uint32_t(vsTexRangeBase + vs_tex_count)}};
    slotTextureSuballocators.vsSamplers =
      HlslRegAllocator{HlslRegAllocator::Policy{uint32_t(vsSamplerRangeBase), uint32_t(vsSamplerRangeBase + vs_tex_count)}};
  }
  if (ps_tex_count)
  {
    slotTextureSuballocators.psTex =
      HlslRegAllocator{HlslRegAllocator::Policy{uint32_t(psTexRangeBase), uint32_t(psTexRangeBase + ps_tex_count)}};
    slotTextureSuballocators.psSamplers =
      HlslRegAllocator{HlslRegAllocator::Policy{uint32_t(psSamplerRangeBase), uint32_t(psSamplerRangeBase + ps_tex_count)}};
  }

  return true;
}

void NamedConstBlock::addHlslDecl(CstHandle hnd, String &&hlsl_decl, String &&hlsl_postfix)
{
  auto &slot = getSlot(hnd);
  slot.hlslDecl = eastl::move(hlsl_decl);
  slot.hlslPostfix = eastl::move(hlsl_postfix);
}

CryptoHash NamedConstBlock::getDigest(ShaderStage stage, const MergedVariablesData &merged_vars) const
{
  CryptoHasher hasher;
  hasher.update(suppBlk.size());

  for (int i = 0; i < suppBlk.size(); i++)
    hasher.update(suppBlk[i]);
  auto constUpdates = [&](auto &n, auto &c) {
    hasher.update(n.nameCount());
    for (int id = 0, e = n.nameCount(); id < e; id++)
    {
      hasher.update(n.getName(id));
      hasher.update(c[id].rspace);
      hasher.update(c[id].size);
      hasher.update(c[id].regIndex);
    }
  };
  if (stage == STAGE_PS || stage == STAGE_CS)
    constUpdates(pixelProps.sn, pixelProps.sc);
  else
    constUpdates(vertexProps.sn, vertexProps.sc);

  {
    buildAllHlsl(nullptr, merged_vars, stage);
    auto &source = stage == STAGE_VS ? cachedVertexHlsl : cachedPixelOrComputeHlsl;
    hasher.update(source.data(), source.length());
  }

  return hasher.hash();
}

static constexpr const char MATERIAL_PROPS_NAME[] = "materialProps";

void NamedConstBlock::buildDrawcallIdHlslDecl(String &out_text) const
{
  const eastl::string_view drawCallIdDeclaration = (multidrawCbuf || !bufferedConstsRegAllocator.hasRegs())
                                                     ? "static uint DRAW_CALL_ID = 10000;\n"
                                                     : "static const uint DRAW_CALL_ID = 0;\n";
  out_text.append(drawCallIdDeclaration.data(), drawCallIdDeclaration.length());
}

static constexpr eastl::array<eastl::string_view, 13> TYPE_NAMES = {
  "float4x4 ",
  "float4 ",
  "uint4 ",
  "int4 ",

  "float3 ",
  "float2 ",
  "float ",
  "uint3 ",
  "uint2 ",
  "uint ",
  "int3 ",
  "int2 ",
  "int ",
};
static constexpr eastl::array<int, 13> TYPE_PADDINGS = {
  0, // float4x4
  0, // float4
  0, // uint4
  0, // int4

  1, // float3
  2, // float2
  3, // float
  1, // uint3
  2, // uint2
  3, // uint
  1, // int3
  2, // int2
  3, // int
};

void NamedConstBlock::buildStaticConstBufHlslDecl(String &out_text, const MergedVariablesData &merged_vars) const
{
  using MergedVarInfo = ShaderParser::VariablesMerger::MergedVarInfo;
  using MergedVars = ShaderParser::VariablesMerger::MergedVars;

  if (bufferedConstProps.sc.empty())
  {
    G_ASSERT(!bufferedConstsRegAllocator.hasRegs());
    return;
  }

  G_ASSERT(bufferedConstsRegAllocator.hasRegs());

  if (shc::config().enableBindless)
  {
#if _CROSS_TARGET_C1 || _CROSS_TARGET_C2

#elif _CROSS_TARGET_SPIRV
    const eastl::string_view bindlessProlog =
      "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] Texture2D static_textures[];\n"
      "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] TextureCube static_textures_cube[];\n"
      "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] Texture2DArray static_textures_array[];\n"
      "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] TextureCube static_textures_cube_array[];\n"
      "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] TextureCube static_textures3d[];\n"
      "[[vk::binding(0, BINDLESS_SAMPLER_SET_META_ID)]] SamplerState static_samplers[];\n";
#elif _CROSS_TARGET_METAL
    constexpr eastl::string_view bindlessProlog = "[[vk::binding(0, 32)]] Texture2D static_textures[];\n"
                                                  "[[vk::binding(1, 32)]] TextureCube static_textures_cube[];\n"
                                                  "[[vk::binding(2, 32)]] Texture2DArray static_textures_array[];\n"
                                                  "[[vk::binding(3, 32)]] TextureCube static_textures_cube_array[];\n"
                                                  "[[vk::binding(4, 32)]] TextureCube static_textures3d[];\n"
                                                  "[[vk::binding(0, 33)]] SamplerState static_samplers[];\n";
#else
    constexpr eastl::string_view bindlessProlog = "Texture2D static_textures[] : BINDLESS_REGISTER(t, 1);\n"
                                                  "SamplerState static_samplers[] : BINDLESS_REGISTER(s, 1);\n"
                                                  "TextureCube static_textures_cube[] : BINDLESS_REGISTER(t, 2);\n"
                                                  "Texture2DArray static_textures_array[] : BINDLESS_REGISTER(t, 3);\n"
                                                  "TextureCube static_textures_cube_array[] : BINDLESS_REGISTER(t, 4);\n"
                                                  "TextureCube static_textures3d[] : BINDLESS_REGISTER(t, 5);\n";
#endif
    const eastl::string_view bindlessType =
      multidrawCbuf ? eastl::string_view{"#define BINDLESS_CBUFFER_ARRAY\n"} : eastl::string_view{"#define BINDLESS_CBUFFER_SINGLE\n"};

    out_text.append(bindlessType.data(), bindlessType.length());
    out_text.append(bindlessProlog.data(), bindlessProlog.length());
  }

  out_text.append("struct MaterialProperties\n{\n");

  String postfix;

  int regOffsetInStruct = 0;

  for (const auto &[ind, statConst] : enumerate(bufferedConstProps.sc))
  {
    G_ASSERT(!statConst.isDynamic);
    const char *constName = bufferedConstProps.sn.getName(ind);
    eastl::string_view hlsl_decl{statConst.hlslDecl.c_str(), size_t(statConst.hlslDecl.length())};

    // This validates that the allocation of static cbuf registers is perfectly linear and allocates registers as if they were
    // consecutive fields. This is not extremely robust, but always true now. @TODO make more robust, maybe read allocator data to
    // mimic it with the struct layout.
    G_ASSERT(regOffsetInStruct == statConst.regIndex);
    regOffsetInStruct += statConst.size;

    eastl::string_view type_name;
    int padding = 0;
    for (auto [ind, type] : enumerate(TYPE_NAMES))
    {
      size_t found_pos = hlsl_decl.find(type);
      if (found_pos != eastl::string_view::npos)
      {
        type_name = type;
        padding = TYPE_PADDINGS[ind];
        break;
      }
    }

    if (type_name.empty())
      G_ASSERTF(0, "Static cbuf can not contain other than consts (f1-4, i1-4)!");

    out_text.aprintf(0, "  %.*s\n", hlsl_decl.length(), hlsl_decl.data());
    for (int i = 0; i < padding; ++i)
      out_text.aprintf(0, "  float pad%d__;\n", 4 * regOffsetInStruct + i);

    // If this constant is a key in the merger's map, it means that it is not a normal constant, but a few constants
    // packed together into one register. In this case, we have to define getters for each individual packed const
    if (const MergedVars *originalVars = merged_vars.findOriginalBufferedVarsInfo(constName))
    {
      for (const MergedVarInfo &varInfo : *originalVars)
      {
        postfix.aprintf(0, "%s get_%s() { return %s[DRAW_CALL_ID].%s.%s; }\n", varInfo.getType(), varInfo.name, MATERIAL_PROPS_NAME,
          constName, varInfo.getSwizzle().c_str());
      }
    }
    // Otherwise, this is a regular constant which is used as-is, and we just define the getter for the const itself.
    else
    {
      postfix.aprintf(0, "%.*s get_%s() { return %s[DRAW_CALL_ID].%s; }\n", type_name.length(), type_name, constName,
        MATERIAL_PROPS_NAME, constName);
    }

    postfix.append(statConst.hlslPostfix.c_str(), statConst.hlslPostfix.length());
  }

  const uint32_t propSetsCount = multidrawCbuf ? MAX_CBUFFER_VECTORS / regOffsetInStruct : 1;

  out_text.aprintf(0,
    "\n};\n\n"
    "#define MATERIAL_PROPS_SIZE %d\n"
    "cbuffer shader_static_cbuf:register(b1) { MaterialProperties %s[MATERIAL_PROPS_SIZE]; };\n\n",
    propSetsCount, MATERIAL_PROPS_NAME);

  out_text.append(postfix.data(), postfix.length());
  out_text.append("\n\n");
}

void NamedConstBlock::buildGlobalConstBufHlslDecl(String &out_text) const
{
  if (!bufferedConstProps.sc.empty())
  {
    out_text += "cbuffer global_const_block : register(b2) {\n";
    for (const NamedConst &nc : bufferedConstProps.sc)
    {
      G_ASSERT(nc.isDynamic);
      if (const char *hlsl = nc.hlslDecl.c_str())
        out_text.append(hlsl);
    }
    out_text += "\n};\n\n";
    for (const NamedConst &nc : bufferedConstProps.sc)
    {
      if (const char *post = nc.hlslPostfix.c_str())
        out_text.append(post);
    }
    out_text += "\n\n";
  }
}

void NamedConstBlock::doBuildHlslDeclText(String &out_text, ShaderStage stage,
  eastl::vector_set<const NamedConstBlock *> &built_blocks) const
{
  if (built_blocks.find(this) != built_blocks.end())
    return;

  built_blocks.insert(this);

  if (globConstBlk && built_blocks.find(&globConstBlk->shConst) == built_blocks.end())
  {
    globConstBlk->shConst.buildGlobalConstBufHlslDecl(out_text);
    built_blocks.insert(&globConstBlk->shConst);
  }
  for (const ShaderStateBlock *sb : suppBlk)
    sb->shConst.doBuildHlslDeclText(out_text, stage, built_blocks);

  auto &props = (stage == STAGE_VS) ? vertexProps : pixelProps;
  for (const NamedConst &nc : props.sc)
  {
    if (const char *hlsl = nc.hlslDecl.c_str())
      out_text += hlsl;
  }
  for (const NamedConst &nc : props.sc)
  {
    if (const char *hlsl = nc.hlslPostfix.c_str())
      out_text += hlsl;
  }
}

eastl::pair<int, int> NamedConstBlock::getStaticTexRange(ShaderStage stage) const
{
  const auto &sc = stage == STAGE_VS ? vertexProps.sc : pixelProps.sc;
  int minTex = INT_MAX, maxTex = -1;
  for (const auto &nc : sc)
  {
    // @NOTE: there are no static buffers/tlas-es => only static textures are static in T space
    if (nc.isDynamic || nc.rspace != HLSL_RSPACE_T)
      continue;
    minTex = min(minTex, nc.regIndex);
    maxTex = max(maxTex, nc.regIndex);
  }

  return {minTex, maxTex + 1};
}

template <typename TF> // TF: void(HlslRegisterSpace regt_space, int regt_id, eastl::string_view code_fragment)
static void process_hardcoded_register_declarations(const char *hlsl_src, TF &&processor)
{
  const char *text = hlsl_src, *p, *start = text;

  while ((p = strstr(text, "register")) != nullptr)
  {
    if (p <= start || fast_isalnum_or_(p[-1]) || fast_isalnum_or_(p[8]))
    {
      text = p + 8;
      continue;
    }
    const char *fragment_start = p;
    while (fragment_start > start && !strchr("\r\n", fragment_start[-1]))
      fragment_start--;

    p += 8;
    while (strchr(" \t\v\n\r", *p))
      p++;
    if (*p != '(')
    {
      text = p;
      continue;
    }
    p++;
    while (strchr(" \t\v\n\r", *p))
      p++;
    if (!strchr("tcsub", *p))
    {
      text = p;
      continue;
    }
    int regt_sym = *p;
    int idx = atoi(p + 1);

    p++;
    while (isdigit(*p))
      p++;
    const char *fragment_end = p;
    while (*fragment_end && !strchr("\r\n", *fragment_end))
      fragment_end++;

    HlslRegisterSpace rspace = symbol_to_hlsl_reg_space(regt_sym);
    if (rspace == HLSL_RSPACE_INVALID)
    {
      sh_debug(SHLOG_ERROR, "Invalid register space symbol '%c' at code: %.*s", regt_sym, fragment_end - fragment_start,
        fragment_start);
      break;
    }

    processor(rspace, idx, eastl::string_view(fragment_start, fragment_end - fragment_start));

    text = p;
  }
}

// TODO: rename pixel_shader to pixel_or_compute_shader everywhere applicable in this file. Or just pass shader stage instead.
void NamedConstBlock::patchHlsl(String &src, ShaderStage stage, const MergedVariablesData &merged_vars, int &max_const_no_used,
  eastl::string_view hw_defines, bool uses_dual_source_blending)
{
  max_const_no_used = -1;
  String res;
  res.reserve(src.length() + hw_defines.length());

  const bool usesPixelProps = stage == STAGE_PS || stage == STAGE_CS;

  auto NamedConstBlock::*propsField = usesPixelProps ? &NamedConstBlock::pixelProps : &NamedConstBlock::vertexProps;
  auto &props = this->*propsField;
  auto &regAllocators = usesPixelProps ? pixelOrComputeRegAllocators : vertexRegAllocators;

  process_hardcoded_register_declarations(src, [&, this](HlslRegisterSpace rspace, int regt_id, eastl::string_view fragment) {
    if (auto res = regAllocators[rspace].reserve(HlslSlotSemantic::HARDCODED, regt_id); !res)
    {
      // @TODO: implement arrays checking
      report_reg_reserve_failed(eastl::string{fragment}.c_str(), regt_id, 1, rspace, HlslSlotSemantic::HARDCODED, res.error(),
        regAllocators[rspace], makeInfoProvider(propsField, rspace));
      return;
    }
  });

  process_hardcoded_register_declarations(hw_defines.data(),
    [&, this](HlslRegisterSpace rspace, int regt_id, eastl::string_view fragment) {
      if (auto res = regAllocators[rspace].reserve(HlslSlotSemantic::RESERVED_FOR_PREDEFINES, regt_id); !res)
      {
        // @TODO: implement arrays checking
        report_reg_reserve_failed(eastl::string{fragment}.c_str(), regt_id, 1, rspace, HlslSlotSemantic::HARDCODED, res.error(),
          regAllocators[rspace], makeInfoProvider(propsField, rspace));
        return;
      }
    });

  res.append(hw_defines.data(), hw_defines.length());

  buildAllHlsl(&res, merged_vars, stage);
  dropHlslCaches();

  res.append(src.c_str(), src.length());
  src = eastl::move(res);

  const char *text = src.c_str();

  if (uses_dual_source_blending && stage == STAGE_PS)
  {
    // @NOTE: unfortunately, this does not work because of macro declarations. Still, the two latter warnings (turned to errors on -wx)
    // should help.
    //
    // if (strstr(text, "SV_Target"))
    // {
    //   sh_debug(SHLOG_ERROR,
    //     "When compiling with dual source blending (sc1/isc1/sa1/isa1) it is prohibited to use SV_Target# in code for correct spirv "
    //     "generation (for vk & metal). Use DUAL_SOURCE_BLEND_ATTACHMENTS macro instead.");
    // }

    if (!strstr(text, "DUAL_SOURCE_BLEND_ATTACHMENTS"))
    {
      sh_debug(SHLOG_WARNING, "Compiling with dual source blending (sc1/isc1/sa1/isa1) but DUAL_SOURCE_BLEND_ATTACHMENTS is not used. "
                              "This might be a bug in the shader code.");
    }
  }

  {
    auto [firstAlloc, allocCap] = regAllocators[HLSL_RSPACE_C].getRange(HlslSlotSemantic::ALLOCATED);
    auto [firstHc, hcCap] = regAllocators[HLSL_RSPACE_C].getRange(HlslSlotSemantic::HARDCODED);

    if (!((firstHc >= allocCap) || (firstAlloc >= hcCap)))
    {
      sh_debug(SHLOG_ERROR,
        "Overlap between allocated register range [%d, %d] and hardcoded registers [%d, %d] is not allowed for %s constants",
        firstAlloc, allocCap - 1, firstHc, hcCap - 1, SHADER_STAGE_SHORT_NAMES[stage]);
    }
  }

  // @TODO: validate other namespaces: s u b

  if (regAllocators[HLSL_RSPACE_C].hasRegs())
    max_const_no_used = regAllocators[HLSL_RSPACE_C].getRange().cap - 1;
}

ShaderStateBlock::ShaderStateBlock(const char *nm, ShaderBlockLevel lev, NamedConstBlock &&ncb, dag::Span<int> stcode,
  DynamicStcodeRoutine *cpp_stcode, int maxregsize, shc::TargetContext &a_ctx) :
  shConst(eastl::move(ncb)), name(nm), nameId(-1), stcodeId(-1), cppStcodeId(-1), layerLevel(lev), regSize(maxregsize)
{
  if (stcode.size())
  {
    stcodeId = add_stcode(stcode, a_ctx);
    if (shc::config().generateCppStcodeValidationData && cpp_stcode)
      add_stcode_validation_mask(stcodeId, cpp_stcode->constMask.release(), a_ctx);
  }
  if (cpp_stcode->hasCode())
  {
    const bool isGlobCbuf = lev == ShaderBlockLevel::GLOBAL_CONST;
    const HlslRegRange vsRegRange = isGlobCbuf ? cpp_stcode->collectSetRegistersRange(STAGE_VS)
                                               : shConst.vertexRegAllocators[HLSL_RSPACE_C].getRange(HlslSlotSemantic::ALLOCATED);
    const HlslRegRange psOrCsRegRange = isGlobCbuf
                                          ? cpp_stcode->collectSetRegistersRange(STAGE_PS /* STAGE_CS has the same effect */)
                                          : shConst.pixelOrComputeRegAllocators[HLSL_RSPACE_C].getRange(HlslSlotSemantic::ALLOCATED);
    cppStcodeId = a_ctx.cppStcode().addCode(eastl::move(*cpp_stcode), psOrCsRegRange, vsRegRange);
  }
}

int ShaderStateBlock::getVsNameId(const char *name) const
{
  int id = shConst.vertexProps.sn.getNameId(name);
  if (id != -1)
    return id;
  for (int i = 0; i < shConst.suppBlk.size(); i++)
  {
    id = shConst.suppBlk[i]->getVsNameId(name);
    if (id != -1)
      return id;
  }
  return -1;
}
int ShaderStateBlock::getPsNameId(const char *name) const
{
  int id = shConst.pixelProps.sn.getNameId(name);
  if (id != -1)
    return id;
  for (int i = 0; i < shConst.suppBlk.size(); i++)
  {
    id = shConst.suppBlk[i]->getPsNameId(name);
    if (id != -1)
      return id;
  }
  return -1;
}

ShaderBlockTable::~ShaderBlockTable()
{
  for (ShaderStateBlock *blk : blocks)
  {
    if (blk && blk != emptyBlock())
      delete blk;
  }
  blockNames.reset();
}

bool ShaderBlockTable::registerBlock(ShaderStateBlock *blk, bool allow_identical_redecl)
{
  int id = blockNames.addNameId(blk->name.c_str());
  if (id < blocks.size())
  {
    if (!allow_identical_redecl)
      return false;

    ShaderStateBlock &sb = *blocks[id];
    if (sb.layerLevel != blk->layerLevel)
    {
      sh_debug(SHLOG_ERROR, "non-ident block <%s> redecl: layerLevel=%d != %d(prev)", blk->name.c_str(), int(blk->layerLevel),
        int(sb.layerLevel));
      return false;
    }
    if (sb.stcodeId != blk->stcodeId)
    {
      sh_debug(SHLOG_ERROR, "non-ident block <%s> redecl: stcodeId=%d != %d(prev)", blk->name.c_str(), blk->stcodeId, sb.stcodeId);
      return false;
    }
    if (sb.shConst.suppBlk != blk->shConst.suppBlk)
    {
      sh_debug(SHLOG_ERROR, "non-ident block <%s> redecl: supported block list changed", blk->name.c_str());
      return false;
    }
    return true;
  }

  G_ASSERT(blocks.size() == id);
  blocks.push_back(blk);
  blk->nameId = id;
  return true;
}

ShaderStateBlock *ShaderBlockTable::findBlock(const char *name) const
{
  int id = blockNames.getNameId(name);
  return (id < 0) ? NULL : blocks[id];
}

Tab<ShaderStateBlock *> ShaderBlockTable::release() { return eastl::move(blocks); }

int ShaderBlockTable::countBlock(ShaderBlockLevel level) const
{
  if (level == ShaderBlockLevel::UNDEFINED)
    return blocks.size();
  return eastl::count_if(blocks.begin(), blocks.end(),
    [&level](const ShaderStateBlock *const blk) { return blk->layerLevel == level; });
}

void ShaderBlockTable::link(Tab<ShaderStateBlock *> &loaded_blocks, dag::ConstSpan<int> stcode_remap,
  dag::ConstSpan<int> external_stcode_remap)
{
  for (ShaderStateBlock *&block : loaded_blocks)
  {
    ShaderStateBlock &b = *block;
    for (auto &blk : b.shConst.suppBlk)
    {
      const char *bname = blk->name.c_str();
      ShaderStateBlock *sb = !blk->name.empty() ? findBlock(bname) : emptyBlock();
      if (sb)
        blk = sb;
      else
        sh_debug(SHLOG_FATAL, "undefined block <%s>", bname);
    }

    auto remapStcode = [&, this](auto &id, const auto &remap, const char *field_name) {
      if (remap.size() && id != -1)
      {
        if (id >= 0 && id < remap.size())
          id = remap[id];
        else
          sh_debug(SHLOG_FATAL, "block <%s>: %s=%d is out of range [0..%d]", b.name.c_str(), field_name, id, remap.size() - 1);
      }
    };

    remapStcode(b.stcodeId, stcode_remap, "stcodeId");
    remapStcode(b.cppStcodeId, external_stcode_remap, "cppStcodeId");

    if (!registerBlock(&b, true))
    {
      sh_debug(SHLOG_FATAL, "can't register block <%s> - definition differs from previous", b.name.c_str());
      delete &b;
    }
  }
}
