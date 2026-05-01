// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "preshaderCompilation.h"
#include "variantSemantic.h"
#include "variantAssembly.h"
#include "shVariantContext.h"
#include "stcodeBytecode.h"
#include "shExprParser.h"
#include "globalConfig.h"
#include "shLog.h"
#include <shaders/shOpcode.h>
#include <shaders/shOpcodeFormat.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>

using namespace ShaderTerminal;
using namespace ShaderParser;

struct PreshaderCompilationContext
{
  const PreshaderCompilationInput *input;
  PreshaderCompilationOutput *output;

  shc::VariantContext *vctx;

  StcodeBytecodeAccumulator bytecodeAccum;
  VariablesMerger varMerger{};

  dag::VectorSet<int> assembledSamplerIds{};

  dag::VectorMap<eastl::string_view, Symbol *> uavGlobalShadervarRefs{}, srvGlobalShadervarRefs{}, uavLocalShadervarRefs{},
    srvLocalShadervarRefs{};

  // For multidraw validation
  bool hasDynStcodeRelyingOnMaterialParams = false;
  Symbol *exprWithDynamicAndMaterialTerms = nullptr;
};

#define CHECK(call_) \
  do                 \
  {                  \
    if (!(call_))    \
      return false;  \
  } while (0)

static bool reserve_special_cbuffer_at(HlslSlotSemantic cbuffer_sem, int reg, PreshaderCompilationContext &ctx)
{
  NamedConstBlock &outNcTable = ctx.output->code.namedConstTable;
  Parser &parser = ctx.vctx->tgtCtx().sourceParseState().parser;

  G_ASSERT(cbuffer_sem >= HlslSlotSemantic::RESERVED_FOR_IMPLICIT_CONST_CBUF);
  auto vr = outNcTable.vertexRegAllocators[HLSL_RSPACE_B].reserve(cbuffer_sem, reg);
  auto pcr = outNcTable.pixelOrComputeRegAllocators[HLSL_RSPACE_B].reserve(cbuffer_sem, reg);
  if (!vr)
  {
    G_ASSERT(!vr.error().outOfRange);
    report_reg_reserve_failed(nullptr, reg, 1, HLSL_RSPACE_B, cbuffer_sem, vr.error(), outNcTable.vertexRegAllocators[HLSL_RSPACE_B],
      outNcTable.makeInfoProvider(&NamedConstBlock::vertexProps, HLSL_RSPACE_B, parser.get_lexer()));
  }
  if (!pcr)
  {
    G_ASSERT(!pcr.error().outOfRange);
    report_reg_reserve_failed(nullptr, reg, 1, HLSL_RSPACE_B, cbuffer_sem, pcr.error(),
      outNcTable.pixelOrComputeRegAllocators[HLSL_RSPACE_B],
      outNcTable.makeInfoProvider(&NamedConstBlock::pixelProps, HLSL_RSPACE_B, parser.get_lexer()));
  }
  return vr && pcr;
}

static bool process_supports(supports_stat &s, PreshaderCompilationContext &ctx)
{
  NamedConstBlock &outNcTable = ctx.output->code.namedConstTable;
  shc::VariantContext &vctx = *ctx.vctx;
  Parser &parser = vctx.tgtCtx().sourceParseState().parser;

  for (int i = 0; i < s.name.size(); i++)
  {
    if (s.name[i]->num != SHADER_TOKENS::SHTOK_none)
    {
      using namespace std::string_view_literals;
      if (s.name[i]->text == "__static_cbuf"sv)
      {
        CHECK(
          reserve_special_cbuffer_at(HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF, MATERIAL_PARAMS_CONST_BUF_REGISTER, ctx));
        continue;
      }
      if (s.name[i]->text == "__static_multidraw_cbuf"sv)
      {
        // Currently we support multidraw constbuffers only for bindless material version.
        CHECK(
          reserve_special_cbuffer_at(HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF, MATERIAL_PARAMS_CONST_BUF_REGISTER, ctx));
        outNcTable.multidrawCbuf = true;
        continue;
      }
      if (s.name[i]->text == "__draw_id"sv)
      {
        continue;
      }
    }
    ShaderStateBlock *b = (s.name[i]->num == SHADER_TOKENS::SHTOK_none) ? vctx.tgtCtx().blocks().emptyBlock()
                                                                        : vctx.tgtCtx().blocks().findBlock(s.name[i]->text);
    if (!b)
    {
      report_error(parser, s.name[i], "can't support undefined block <%s>", s.name[i]->text);
      return false;
    }

    bool found = (b == outNcTable.globConstBlk);
    for (int j = 0; j < outNcTable.suppBlk.size(); j++)
      if (outNcTable.suppBlk[j] == b)
      {
        found = true;
        break;
      }
    if (found)
    {
      report_error(parser, s.name[i], "double support for block <%s>", s.name[i]->text);
      return false;
    }

    if (b->layerLevel < ShaderBlockLevel::SHADER && !b->canBeSupportedBy(vctx.shCtx().blockLevel()))
    {
      report_error(parser, s.name[i], "block <%s>, layer %d, cannot be supported here", s.name[i]->text, int(b->layerLevel));
      return false;
    }

    if (b->shConst.multidrawCbuf)
      outNcTable.multidrawCbuf = true;

    if (b->layerLevel == ShaderBlockLevel::GLOBAL_CONST)
      outNcTable.globConstBlk = b;
    else
    {
      bool ok = true;
      for_each_hlsl_reg_space([&](HlslRegisterSpace space) {
        auto vr = outNcTable.vertexRegAllocators[space].reserveAllFrom(b->shConst.vertexRegAllocators[space]);
        auto pcr = outNcTable.pixelOrComputeRegAllocators[space].reserveAllFrom(b->shConst.pixelOrComputeRegAllocators[space]);

        auto reportFailure = [&](ShaderStage stage) {
          auto propsField = stage == STAGE_VS ? &NamedConstBlock::vertexProps : &NamedConstBlock::pixelProps;
          auto regAllocsField =
            stage == STAGE_VS ? &NamedConstBlock::vertexRegAllocators : &NamedConstBlock::pixelOrComputeRegAllocators;
          auto errorMsg = string_f("Registers in supported blocks overlap at space %c for %s shader\n", HLSL_RSPACE_ALL_SYMBOLS[space],
            SHADER_STAGE_NAMES[stage]);
          errorMsg.append_sprintf("Trying to support block %s. ", b->name.c_str());
          errorMsg += get_reg_alloc_dump((b->shConst.*regAllocsField)[stage], space,
            b->shConst.makeInfoProvider(propsField, space, parser.get_lexer()));

          errorMsg.append("\nConflicting blocks:\n");
          eastl::vector_set<const ShaderStateBlock *> conflictingBlocks{};
          auto collectBlocks = [&](const NamedConstBlock &consts, auto &&self) -> void {
            auto processOneBlock = [&](const auto *blk) {
              auto regalloc = (blk->shConst.*regAllocsField)[space]; // Copy out to simulate collision
              if (auto allocRes = regalloc.reserveAllFrom((b->shConst.*regAllocsField)[space]); !allocRes)
              {
                conflictingBlocks.insert(blk);
                errorMsg.append_sprintf("%s, ", blk->name.c_str());
                errorMsg += get_reg_alloc_dump((blk->shConst.*regAllocsField)[stage], space,
                  blk->shConst.makeInfoProvider(propsField, space, parser.get_lexer()));
              }
            };
            for (const auto &blk : consts.suppBlk)
            {
              if (conflictingBlocks.find(blk.get()) != conflictingBlocks.end())
                continue;
              self(blk->shConst, self);
              processOneBlock(blk.get());
            }
            if (consts.globConstBlk && conflictingBlocks.find(consts.globConstBlk) == conflictingBlocks.end())
            {
              self(consts.globConstBlk->shConst, self);
              processOneBlock(consts.globConstBlk);
            }
          };

          collectBlocks(outNcTable, collectBlocks);
        };

        if (!vr)
          reportFailure(STAGE_VS);
        if (!pcr)
          reportFailure(ctx.input->isCompute ? STAGE_CS : STAGE_PS);
        ok &= vr && pcr;
      });
      if (!ok)
        return false;
      outNcTable.suppBlk.push_back(b);
    }
  }

  return true;
}

static bool compile_stat(const PreshaderStat &stat, PreshaderCompilationContext &ctx)
{
  RegionMemAlloc rm_alloc(4 << 20, 4 << 20);

  CompiledPreshader &outCode = ctx.output->code;
  NamedConstBlock &outNcTable = outCode.namedConstTable;
  StcodePass &outCppStcodePass = outCode.cppStcode;
  StcodeBytecodeAccumulator &outBytecodeAccum = ctx.bytecodeAccum;

  shc::VariantContext &vctx = *ctx.vctx;
  Parser &parser = vctx.tgtCtx().sourceParseState().parser;

  outCppStcodePass.cppStcode.reportStageUsage(stat.stage);

  const auto parsedDefMaybe =
    semantic::parse_named_const_definition(*stat.stat, stat.stage, semantic::VariableType(stat.vt), vctx, &rm_alloc);
  if (!parsedDefMaybe)
    return false;

  const semantic::NamedConstDefInfo &def = *parsedDefMaybe;

  if (ctx.input->isCompute && !def.isDynamic && vctx.shCtx().blockLevel() != ShaderBlockLevel::GLOBAL_CONST)
  {
    report_error(parser, stat.stat, "Found static preshader var '%s'. Compute shaders can't have static preshader vars", def.baseName);
    return false;
  }

  if (def.bindlessVarId >= 0 && def.shvarTexType != SHVT_TEX_UNKNOWN)
  {
    auto &svts = outCode.shadervarTexTypes;
    if (svts.size() <= def.bindlessVarId)
      svts.resize(def.bindlessVarId + 1, SHVT_TEX_UNKNOWN);
    svts[def.bindlessVarId] = def.shvarTexType;
  }

  ctx.hasDynStcodeRelyingOnMaterialParams |= def.hasDynStcodeRelyingOnMaterialParams;
  if (def.exprWithDynamicAndMaterialTerms)
    ctx.exprWithDynamicAndMaterialTerms = def.exprWithDynamicAndMaterialTerms;

  // Validate that the same shadervar has not been used both as a uav and an srv resource in the same pass
  if (def.shvarType == SHVT_TEXTURE || def.shvarType == SHVT_BUFFER)
  {
    const bool isRw = def.type == semantic::VariableType::uav;
    const auto setForVar = [&](bool isRw, bool isGlobal) -> decltype(ctx.uavGlobalShadervarRefs) & {
      return isGlobal ? (isRw ? ctx.uavGlobalShadervarRefs : ctx.srvGlobalShadervarRefs)
                      : (isRw ? ctx.uavLocalShadervarRefs : ctx.srvLocalShadervarRefs);
    };

    for (const auto &elem : def.initializer)
    {
      G_ASSERT(elem.isGlobalVar() || elem.isMaterialVar());
      bool isGlobal = elem.isGlobalVar();
      const char *varName = elem.varName();

      auto &set = setForVar(isRw, isGlobal);
      const auto &conflictSet = setForVar(!isRw, isGlobal);

      if (auto it = conflictSet.find(varName); it != conflictSet.end())
      {
        const auto strForUsage = [](bool isRw) { return isRw ? "UAV" : "SRV"; };
        report_error(parser, stat.stat, "The same %s shadervar is used as a %s, while being previously used as a %s at %s(%d, %d)",
          isGlobal ? "global" : "material", strForUsage(isRw), strForUsage(!isRw),
          parser.get_lexer().get_filename(it->second->file_start), it->second->line_start, it->second->col_start);
        return false;
      }

      set.emplace(varName, stat.stat);
    }
  }

  {
    Terminal *const var = def.varTerm;
    const int hardcoded_reg = def.hardcodedRegister;
    const int registers_count = def.registerSize;
    const ShaderStage stage = def.stage;
    const String &varName = def.mangledName;
    const HlslRegisterSpace reg_space = def.regSpace;

    const auto hnd = outNcTable.addConst(stage, varName, var, parser.get_lexer(), reg_space, registers_count, hardcoded_reg,
      def.isDynamic, vctx.shCtx().blockLevel() == ShaderBlockLevel::GLOBAL_CONST);
    if (is_error(hnd))
    {
      return false;
    }
    else if (!is_valid(hnd))
    {
      // @TODO: implement correct expression comparison to not allow silent redecls
      if (!hnd.isBufConst)
      {
        report_error(parser, var, "Redeclaration for variable <%s> in external block is not allowed", varName.c_str());
        return false;
      }
      else
      {
        report_debug_message(parser, *var,
          "Redeclaration for variable <%s> for %s is skipped. If it's declaration differs from previous, this is UB.", varName.c_str(),
          def.isDynamic ? "global const block" : "static cbuf");
      }
      return true;
    }

    const int reg = outNcTable.getSlot(hnd).regIndex;

    auto builtHlslMaybe =
      assembly::build_hlsl_decl_for_named_const(def, vctx, reg, ctx.varMerger.constVarsMaps, ctx.varMerger.bufferedVarsMap);
    if (!builtHlslMaybe)
      return false;

    assembly::NamedConstDeclarationHlsl &builtHlsl = *builtHlslMaybe;

    outNcTable.addHlslDecl(hnd, eastl::move(builtHlsl.definition), eastl::move(builtHlsl.postfix));

    if (!assembly::build_stcode_for_named_const<assembly::StcodeBuildFlagsBits::ALL>(
          def, reg, vctx, &outCppStcodePass, &outBytecodeAccum,
          [&](int id, void *var_node) {
            if (auto [_, isNew] = ctx.assembledSamplerIds.insert(id); isNew)
            {
              if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
                outCode.preshaderVarsNeedingSamplerAsm.push_back(uintptr_t(var_node));
              return true;
            }
            return false;
          },
          &rm_alloc))
    {
      return false;
    }

    outCode.stcodeVars.push_back(var);
  }

  if (def.pairSamplerTmpDecl)
  {
    G_ASSERT(vctx.shCtx().blockLevel() != ShaderBlockLevel::GLOBAL_CONST);

    const String samplerConstName{0, "%s%s", def.mangledName.c_str(), def.pairSamplerBindSuffix};
    const auto hnd = outNcTable.addConst(def.stage, samplerConstName, def.varTerm, parser.get_lexer(), HLSL_RSPACE_S, 1,
      def.hardcodedRegister, def.isDynamic, false);
    if (is_error(hnd))
    {
      return false;
    }
    else if (!is_valid(hnd))
    {
      report_error(parser, def.varTerm, "Redeclaration for variable <%s> with implicit pair sampler is not allowed",
        def.mangledName.c_str());
      return false;
    }

    const int reg = outNcTable.getSlot(hnd).regIndex;
    String hlsl = assembly::build_hlsl_for_pair_sampler(def.mangledName.c_str(), def.pairSamplerIsShadow, reg);
    outNcTable.addHlslDecl(hnd, eastl::move(hlsl), {});

    if (def.isDynamic && def.hardcodedRegister == -1)
    {
      if (def.pairSamplerIsGlobal)
      {
        vctx.tgtCtx().samplers().add(*def.pairSamplerTmpDecl, parser);
      }
      else
      {
        add_dynamic_sampler_for_stcode(vctx.parsedSemCode(), vctx.shCtx().compiledShader(), *def.pairSamplerTmpDecl, parser,
          vctx.tgtCtx().varNameMap());
        outCode.dynamicSamplerImplicitVars.push_back(def.pairSamplerName);
      }

      auto [samplerVarId, varType, isGlobal] = semantic::lookup_state_var(*def.pairSamplerTmpDecl->name, vctx);
      G_ASSERT(isGlobal == def.pairSamplerIsGlobal);
      G_ASSERT(varType == SHVT_SAMPLER);

      assembly::build_stcode_for_pair_sampler<assembly::StcodeBuildFlagsBits::ALL>(samplerConstName.c_str(),
        def.pairSamplerName.c_str(), reg, def.stage, samplerVarId, def.pairSamplerIsGlobal, &outCppStcodePass, &outBytecodeAccum);
    }
  }

  return true;
}

static bool process_stat(const PreshaderStat &stat, PreshaderCompilationContext &ctx)
{
  using semantic::VariableType;

  CompiledPreshader &outputCode = ctx.output->code;
  shc::VariantContext &vctx = *ctx.vctx;
  Parser &parser = vctx.tgtCtx().sourceParseState().parser;

  auto &[st, stage, vt] = stat;

  G_ASSERT(st->var || st->arr || st->reg || st->reg_arr);

  if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
    outputCode.usedPreshaderStatements.push_back(uintptr_t(st));

  G_ASSERT(st->var || st->arr || st->reg || st->reg_arr);
  if (!st->var || st->var->val->builtin_var)
    return compile_stat(stat, ctx);

  // clang-format off
  switch (VariableType(vt))
  {
    case VariableType::f1:
    case VariableType::f2:
    case VariableType::f3:
    case VariableType::i1:
    case VariableType::i2:
    case VariableType::i3:
    case VariableType::u1:
    case VariableType::u2:
    case VariableType::u3:
      break;
    default:
      return compile_stat(stat, ctx);
  }
  // clang-format on

  auto exprIsDynamic = [&] {
    if (!st->var->val->expr)
      return true;

    ComplexExpression colorExpr(st->var->val->expr, shexpr::VT_COLOR4);

    if (!ExpressionParser{vctx}.parseExpression(*st->var->val->expr, &colorExpr,
          ExpressionParser::Context{shexpr::VT_COLOR4, semantic::vt_is_integer(VariableType(vt)), st->var->var->name}))
    {
      return false;
    }

    if (Symbol *s = colorExpr.hasDynamicAndMaterialTermsAt())
    {
      ctx.hasDynStcodeRelyingOnMaterialParams = true;
      ctx.exprWithDynamicAndMaterialTerms = s;
    }

    if (!colorExpr.collapseNumbers(parser))
      return false;

    Color4 v;
    if (colorExpr.evaluate(v, parser))
      return false;

    return colorExpr.isDynamic();
  };

  if (vctx.shCtx().blockLevel() != ShaderBlockLevel::GLOBAL_CONST && exprIsDynamic())
    ctx.varMerger.addConstStat(*st, stage);
  else
    ctx.varMerger.addBufferedStat(*st, stage);

  return true;
}

static bool process_locdecl(local_var_decl &s, PreshaderCompilationContext &ctx)
{
  auto parseResMaybe = semantic::parse_local_var_decl(s, *ctx.vctx);
  if (!parseResMaybe)
    return false;

  if (!parseResMaybe->var->isConst)
  {
    assembly::assemble_local_var<assembly::StcodeBuildFlagsBits::ALL>(parseResMaybe->var, parseResMaybe->expr.get(), s.name,
      &ctx.output->code.cppStcode, &ctx.bytecodeAccum);
  }

  return true;
}

#undef CHECK

static bool validate_dynamic_consts_for_multidraw(PreshaderCompilationContext &ctx)
{
  if (!ctx.output->code.namedConstTable.multidrawCbuf)
    return true;

  if (ctx.hasDynStcodeRelyingOnMaterialParams)
  {
    report_error(ctx.vctx->tgtCtx().sourceParseState().parser, ctx.exprWithDynamicAndMaterialTerms,
      "Encountered both static and dynamic operands in an stcode expression!\n\n"
      "  With bindless enabled this can break multidraw, as dynstcode is run once per combined draw call.\n"
      "  Consider splitting the expression into separate parts.");
    return false;
  }

  return true;
}

eastl::optional<PreshaderCompilationOutput> compile_variant_preshader(const PreshaderCompilationInput &input,
  shc::VariantContext &vctx)
{
#define CHECK(call_)         \
  do                         \
  {                          \
    if (!(call_))            \
      return eastl::nullopt; \
  } while (0)

  Parser &parser = vctx.tgtCtx().sourceParseState().parser;

  PreshaderCompilationOutput output{};
  PreshaderCompilationContext ctx{
    .input = &input, .output = &output, .vctx = &vctx, .bytecodeAccum = StcodeBytecodeAccumulator{parser}};

  CompiledPreshader &outCode = output.code;
  NamedConstBlock &outNcTable = outCode.namedConstTable;
  StcodePass &outCppStcodePass = ctx.output->code.cppStcode;

  for (auto *s : input.supportStats)
    process_supports(*s, ctx);

  // Push header for static stcode straight away to avoid push-front copies
  // Format:
  // 1: SHCOD_STATIC_MULTIDRAW_BLOCK/SHCOD_STATIC_BLOCK, #consts
  // 2: if not bindless: vsTexBase [8], vsTexCount [8], vsSamplerBase [8], vsSmpCount [8]
  // 3: psTexBase [8], psTexCount [8], psSamplerBase [8], psSmpCount [8]
  //    else: 0, 0, 0, 0
  const bool needsStblkcodeHeader = vctx.shCtx().blockLevel() == ShaderBlockLevel::SHADER && !input.isCompute;
  if (needsStblkcodeHeader)
  {
    ctx.bytecodeAccum.push_stblkcode(shaderopcode::makeOp1(0, 0));
    ctx.bytecodeAccum.push_stblkcode(shaderopcode::makeData4(0, 0, 0, 0));
    ctx.bytecodeAccum.push_stblkcode(shaderopcode::makeData4(0, 0, 0, 0));
  }

  // Then, process hardcoded preshader stats (to be able to allocate around them)
  for (auto &s : input.hardcodedStats)
    CHECK(compile_stat(s, ctx));

  // Then, process all numeric stats. By doing them first, we can infer whether special constbuffers are actually required, thus
  // obtaining all the info needed to allocate other cbuffer registers.
  for (auto &s : input.scalarStats)
  {
    CHECK(eastl::visit(
      [&](auto &&s) {
        if constexpr (eastl::is_same_v<eastl::decay_t<decltype(s)>, PreshaderStat>)
          return process_stat(s, ctx);
        else
          return process_locdecl(*s, ctx);
      },
      s));
  }

  ctx.varMerger.mergeAllVars({.compileStat = [&ctx](const PreshaderStat &stat) { compile_stat(stat, ctx); },
    .compileLocalVar =
      [&ctx, &vctx](const local_var_decl &decl) {
        auto [var, expr] = unwrap(semantic::parse_local_var_decl(decl, vctx, true));
        if (!var->isConst)
          assembly::assemble_local_var<assembly::StcodeBuildFlagsBits::ALL>(var, expr.get(), decl.name, &ctx.output->code.cppStcode,
            &ctx.bytecodeAccum);
        return var->reg;
      },
    .releaseRegister = [&ctx](int reg) { ctx.bytecodeAccum.regAllocator->manuallyReleaseRegister(reg); }});

  // Then, if bindless is used, we just compile the stats for static textures, as they are going to emit uint2 elements to the material
  // cbuf. @TODO: introduce packing with other material consts.
  if (needsStblkcodeHeader)
  {
    // @TODO: validate that compute stuff does NOT need an stblkcode header
    if (shc::config().enableBindless)
    {
      for (auto &s : input.staticTextureStats)
        CHECK(process_stat(s, ctx));
    }
    // Otherwise, we need to procure a contiguous range in the t space for slot textures and a range in the s space for samplers
    else
    {
      // @NOTE: order is ensured in AssembleShaderEvalCB::end_eval
      auto stagePivot = eastl::find_if(input.staticTextureStats.begin(), input.staticTextureStats.end(),
        [](const PreshaderStat &s) { return s.stage == STAGE_VS; });
      dag::ConstSpan<PreshaderStat> psTexStats{input.staticTextureStats.begin(), stagePivot - input.staticTextureStats.begin()},
        vsTexStats{stagePivot, input.staticTextureStats.end() - stagePivot};

      const auto vsNoSmpPivot = eastl::find_if(vsTexStats.begin(), vsTexStats.end(),
        [](const PreshaderStat &s) { return !semantic::vt_is_static_sampled_texture(s.vt); });
      const auto psNoSmpPivot = eastl::find_if(psTexStats.begin(), psTexStats.end(),
        [](const PreshaderStat &s) { return !semantic::vt_is_static_sampled_texture(s.vt); });

      const uint32_t vsTexRangeExtent = static_cast<uint32_t>(vsTexStats.size());
      const uint32_t psTexRangeExtent = static_cast<uint32_t>(psTexStats.size());
      const uint32_t vsSmpRangeExtent = static_cast<uint32_t>(vsNoSmpPivot - vsTexStats.begin());
      const uint32_t psSmpRangeExtent = static_cast<uint32_t>(psNoSmpPivot - psTexStats.begin());
      G_ASSERT(vsTexRangeExtent <= UINT8_MAX);
      G_ASSERT(psTexRangeExtent <= UINT8_MAX);
      G_ASSERT(vsSmpRangeExtent <= UINT8_MAX);
      G_ASSERT(psSmpRangeExtent <= UINT8_MAX);

      if (!outCode.namedConstTable.initSlotTextureSuballocators(vsTexRangeExtent, vsSmpRangeExtent, psTexRangeExtent, psSmpRangeExtent,
            parser.get_lexer()))
      {
        sh_debug(SHLOG_ERROR, "Failed to allocate %d vs static texture range and %d ps static texture range", vsTexRangeExtent,
          psTexRangeExtent);
        return eastl::nullopt;
      }

      for (auto &s : input.staticTextureStats)
        compile_stat(s, ctx);

      auto [vsTexRangeBase, _1] = outNcTable.slotTextureSuballocators.vsTex.getRange();
      auto [psTexRangeBase, _2] = outNcTable.slotTextureSuballocators.psTex.getRange();
      auto [vsSamplerRangeBase, _3] = outNcTable.slotTextureSuballocators.vsSamplers.getRange();
      auto [psSamplerRangeBase, _4] = outNcTable.slotTextureSuballocators.psSamplers.getRange();
      ctx.bytecodeAccum.stblkcode[1] = shaderopcode::makeData4(vsTexRangeBase, vsTexRangeExtent, vsSamplerRangeBase, vsSmpRangeExtent);
      ctx.bytecodeAccum.stblkcode[2] = shaderopcode::makeData4(psTexRangeBase, psTexRangeExtent, psSamplerRangeBase, psSmpRangeExtent);
    }

    auto [constBase, constCap] = outCode.namedConstTable.bufferedConstsRegAllocator.getRange();
    if (constBase > 0)
    {
      sh_debug(SHLOG_ERROR, "Invalid allocation of material cbuf registers: range is [%d, %d), but it must begin at 0", constBase,
        constCap);
      return eastl::nullopt;
    }
    const auto signOpcod = outNcTable.multidrawCbuf ? SHCOD_STATIC_MULTIDRAW_BLOCK : SHCOD_STATIC_BLOCK;
    ctx.bytecodeAccum.stblkcode[0] = shaderopcode::makeOp1(signOpcod, constCap);

    if (outNcTable.multidrawCbuf)
      outCppStcodePass.cppStblkcode.reportMutlidrawSupport();
  }

  // Now, based on gathered consts we reserve slots for cbuf-s that will hold them

  // Concervatively reserve 0 for implicit cbuf (as it may be used via hlsl-hardcoded regs)
  // @TODO: try and collect hardcoded regs from parsed code blocks to factor them into the allocators
  if (vctx.shCtx().blockLevel() != ShaderBlockLevel::GLOBAL_CONST)
    CHECK(reserve_special_cbuffer_at(HlslSlotSemantic::RESERVED_FOR_IMPLICIT_CONST_CBUF, 0, ctx));

  // Then, reserve material const buf if we have static consts (or remove reservation if we don't)
  if (vctx.shCtx().blockLevel() == ShaderBlockLevel::SHADER) // static cbuf is only used by shaders, not blocks
  {
    if (outNcTable.bufferedConstsRegAllocator.hasRegs())
    {
      G_ASSERT(!input.isCompute);
      CHECK(reserve_special_cbuffer_at(HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF, MATERIAL_PARAMS_CONST_BUF_REGISTER, ctx));
    }
    else
    {
      // If there are no regs to fill, but support blocks declared supp __static_cbuf, we unreserve
      G_VERIFY(outNcTable.vertexRegAllocators[HLSL_RSPACE_B].unreserveIfUsed(HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF,
        MATERIAL_PARAMS_CONST_BUF_REGISTER));
      G_VERIFY(outNcTable.pixelOrComputeRegAllocators[HLSL_RSPACE_B].unreserveIfUsed(
        HlslSlotSemantic::RESERVED_FOR_MATERIAL_PARAMS_CBUF, MATERIAL_PARAMS_CONST_BUF_REGISTER));
    }
  }

  // Reserve special global consts block slot if such a block was declared
  // @TODO: find out the reason why just checking supp blks is not enough (this was how it was done before too)
  if (vctx.tgtCtx().blocks().countBlock(ShaderBlockLevel::GLOBAL_CONST) > 0)
    CHECK(reserve_special_cbuffer_at(HlslSlotSemantic::RESERVED_FOR_GLOBAL_CONST_CBUF, GLOBAL_CONST_BUF_REGISTER, ctx));

  // Finally, reservations are completed, and we process automatic preshaders decls for dynamic resources
  // @TODO: use the fact that we know these to be dynamic and don't recalculate
  for (auto &s : input.dynamicResourceStats)
    CHECK(process_stat(s, ctx));

  // After assebmling everything, do validations on the code that has been assembled
  CHECK(validate_dynamic_consts_for_multidraw(ctx));

  if (shc::config().dumpRegsAlways)
  {
    eastl::string dump{};
    dump.append("All used registers:\n");
    if (!input.isCompute)
    {
      dump.append("\nVertex stage:\n");
      for_each_hlsl_reg_space([&](HlslRegisterSpace rspace) {
        dump.append("(");
        dump.append(HLSL_RSPACE_ALL_NAMES[rspace]);
        dump.append(")");
        dump.append(get_reg_alloc_dump(outNcTable.vertexRegAllocators[rspace], rspace,
          outNcTable.makeInfoProvider(&NamedConstBlock::vertexProps, rspace, parser.get_lexer())));
      });
      dump.append("\nPixel stage:\n");
    }
    else
    {
      dump.append("\nCompute stage:\n");
    }
    for_each_hlsl_reg_space([&](HlslRegisterSpace rspace) {
      dump.append("(");
      dump.append(HLSL_RSPACE_ALL_NAMES[rspace]);
      dump.append(")");
      dump.append(get_reg_alloc_dump(outNcTable.pixelOrComputeRegAllocators[rspace], rspace,
        outNcTable.makeInfoProvider(&NamedConstBlock::pixelProps, rspace, parser.get_lexer())));
    });

    if (needsStblkcodeHeader)
    {
      dump.append("\nStatic constbuffer:\n");
      dump.append(get_reg_alloc_dump(outNcTable.bufferedConstsRegAllocator, HLSL_RSPACE_C,
        outNcTable.makeInfoProvider(&NamedConstBlock::bufferedConstProps, HLSL_RSPACE_C, parser.get_lexer())));

      if (!shc::config().enableBindless)
      {
        dump.append("\nStatic texture/sampler ranges:\n");
        auto [vsTexRangeBase, vsTexRangeCap] = outNcTable.slotTextureSuballocators.vsTex.getRange();
        auto [psTexRangeBase, psTexRangeCap] = outNcTable.slotTextureSuballocators.psTex.getRange();
        auto [vsSamplerRangeBase, vsSamplerRangeCap] = outNcTable.slotTextureSuballocators.vsSamplers.getRange();
        auto [psSamplerRangeBase, psSamplerRangeCap] = outNcTable.slotTextureSuballocators.psSamplers.getRange();

        const auto dumpRange = [&dump](int base, int cap, const char *tag, HlslRegisterSpace rspace) {
          if (base >= cap)
          {
            dump.append_sprintf("No %s\n", tag);
          }
          else
          {
            dump.append_sprintf("%s range : [%c%d, %c%d]\n", tag, HLSL_RSPACE_ALL_SYMBOLS[rspace], base,
              HLSL_RSPACE_ALL_SYMBOLS[rspace], cap - 1);
          }
        };

        dumpRange(vsTexRangeBase, vsTexRangeCap, "VS textures", HLSL_RSPACE_T);
        dumpRange(vsSamplerRangeBase, vsSamplerRangeCap, "VS samplers", HLSL_RSPACE_S);
        dumpRange(psTexRangeBase, psTexRangeCap, "PS textures", HLSL_RSPACE_T);
        dumpRange(psSamplerRangeBase, psSamplerRangeCap, "PS samplers", HLSL_RSPACE_S);
      }
    }

    sh_debug(SHLOG_INFO, dump.c_str());
  }

  for (auto const &[id, var] : enumerate(vctx.parsedSemCode().vars))
  {
    if (var.used)
      outCode.usedMaterialVarIds.push_back(id);
  }

  outCode.requiredRegCount = ctx.bytecodeAccum.regAllocator->requiredRegCount();
  assembly::build_cpp_declarations_for_used_local_vars(outCode.cppStcode, vctx);

  outCode.dynStBytecode = eastl::move(ctx.bytecodeAccum.stcode);
  outCode.stBlkBytecode = eastl::move(ctx.bytecodeAccum.stblkcode);

  outCode.constPackedVarsMaps = eastl::move(ctx.varMerger.constVarsMaps);
  outCode.bufferedPackedVarsMap = eastl::move(ctx.varMerger.bufferedVarsMap);

  outCode.isCompute = input.isCompute;

  return output;
#undef CHECK
}
