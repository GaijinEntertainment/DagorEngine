// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "variantAssembly.h"
#include "preshaderCompilation.h"
#include "codeBlocks.h"
#include "shExpr.h"
#include "shExprParser.h"
#include <shaders/shOpcode.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shUtils.h>
#include <EASTL/array.h>

namespace assembly
{

template <StcodeBuildFlags FLAGS>
void assemble_local_var(LocalVar *var, ShaderParser::ComplexExpression *rootExpr, ShaderTerminal::SHTOK_ident *decl_name,
  StcodePass *out_cppstcode, StcodeBytecodeAccumulator *out_bytecode)
{
  bool isDynamic = rootExpr->isDynamic();

  if constexpr (FLAGS & StcodeBuildFlagsBits::BYTECODE)
  {
    G_ASSERT(out_bytecode);
    Tab<int> &code = out_bytecode->get_alt_curstcode(isDynamic);

    Register varReg;
    if (var->valueType == shexpr::VT_REAL)
      varReg = out_bytecode->regAllocator->add_reg();
    else
      varReg = out_bytecode->regAllocator->add_vec_reg();

    rootExpr->assembleBytecode(code, varReg, *out_bytecode->regAllocator, var->isInteger);
    var->reg = eastl::move(varReg).release();
  }

  if constexpr (FLAGS & StcodeBuildFlagsBits::CPP)
  {
    G_ASSERT(out_cppstcode);
    StcodeRoutine &routine = isDynamic ? (StcodeRoutine &)out_cppstcode->cppStcode : (StcodeRoutine &)out_cppstcode->cppStblkcode;

    StcodeExpression expr;
    rootExpr->assembleCpp(expr, var->isInteger);
    routine.setVarValue(decl_name->text, eastl::move(expr));
  }
}

static eastl::string build_placement_specifier(int dest_reg, bool is_array, int elem_count, HlslRegisterSpace rspace,
  ShaderBlockLevel block_level, bool is_dynamic)
{
  eastl::string res{};
  G_ASSERT(elem_count > 0);
  if (is_array)
    res.append_sprintf("[%d]", elem_count);

  // for static cbuf consts we don't put any reg/offset specifier at all -- they are packed into a struct with further validation that
  // register allocation matches the struct layout
  if (!is_dynamic && rspace == HLSL_RSPACE_C)
  {
    G_ASSERT(block_level == ShaderBlockLevel::SHADER);
    return res;
  }

  // For global cbuf we use packoffset to specify position in cbuffer
  if (rspace == HLSL_RSPACE_C && block_level == ShaderBlockLevel::GLOBAL_CONST)
  {
    G_ASSERT(is_dynamic);
    res.append_sprintf(":packoffset(c%d)", dest_reg);
    return res;
  }

  // At last, for resources and implicit-cbuf consts we use good old hlsl 'register's
  res.append_sprintf(":register(%c%d)", HLSL_RSPACE_ALL_SYMBOLS[rspace], dest_reg);
  return res;
}

void validate_hlsl_block_for_invalid_symbols(eastl::string_view hlsl_block, eastl::span<const char> invalid_symbols, Parser &parser,
  const semantic::NamedConstDefInfo &def)
{
  bool inSingleLineComment = false;
  int multilineCommentDepth = 0;
  size_t index = 0;

  while (index < hlsl_block.size())
  {
    char current = hlsl_block[index];
    char next = (index + 1 < hlsl_block.size()) ? hlsl_block[index + 1] : '\0';

    // close single line comment
    if (inSingleLineComment && (current == '\n' || current == '\r'))
    {
      inSingleLineComment = false;
      ++index;
      continue;
    }

    // close multi line comment
    if (multilineCommentDepth > 0 && current == '*' && next == '/')
    {
      --multilineCommentDepth;
      index += 2;
      continue;
    }

    // open single line comment
    if (!inSingleLineComment && multilineCommentDepth == 0)
    {
      if (current == '/' && next == '/')
      {
        inSingleLineComment = true;
        index += 2;
        continue;
      }
    }

    // open multi line comment
    if (!inSingleLineComment)
    {
      if (current == '/' && next == '*')
      {
        ++multilineCommentDepth;
        index += 2;
        continue;
      }
    }


    // check restricted symbols outside the comments
    if (!inSingleLineComment && multilineCommentDepth == 0)
    {
      for (const auto symbol : invalid_symbols)
      {
        if (current == symbol)
        {
          report_error(parser, def.varTerm, "Invalid symbol <%c> found in named const <%s> HLSL hlsl definition", symbol,
            def.varTerm->text);
        }
      }
    }

    ++index;
  }
}


eastl::optional<NamedConstDeclarationHlsl> build_hlsl_decl_for_named_const(const semantic::NamedConstDefInfo &def,
  shc::VariantContext &ctx, int dest_register, const ShaderParser::VariablesMerger::MergedVarsMapsPerStage &var_merger_per_stage_maps,
  const ShaderParser::VariablesMerger::MergedVarsMap &var_merger_global_blk_map)
{
  using semantic::VariableType;
  static const char *profiles[STAGE_MAX] = {"cs", "ps", "vs"};

  Parser &parser = ctx.tgtCtx().sourceParseState().parser;

  NamedConstDeclarationHlsl hlsl{};

  if (def.hlsl)
    hlsl.definition = def.hlsl->text;

  const char *mangledVarName = def.mangledName.c_str();
  const char *baseVarName = def.varTerm->text;
  const char *nameSpaceName = def.nameSpaceTerm->text;

  const bool isGlobalConstBlock = ctx.shCtx().blockLevel() == ShaderBlockLevel::GLOBAL_CONST;

  const char *varTypeStr = nullptr;
  const char *samplerTypeStr = nullptr;
  switch (def.type)
  {
    case VariableType::f1: varTypeStr = "float"; break;
    case VariableType::f2: varTypeStr = "float2"; break;
    case VariableType::f3: varTypeStr = "float3"; break;
    case VariableType::f4: varTypeStr = "float4"; break;
    case VariableType::i1: varTypeStr = "int"; break;
    case VariableType::i2: varTypeStr = "int2"; break;
    case VariableType::i3: varTypeStr = "int3"; break;
    case VariableType::i4: varTypeStr = "int4"; break;
    case VariableType::u1: varTypeStr = "uint"; break;
    case VariableType::u2: varTypeStr = "uint2"; break;
    case VariableType::u3: varTypeStr = "uint3"; break;
    case VariableType::u4: varTypeStr = "uint4"; break;
    case VariableType::f44: varTypeStr = "float4x4"; break;
    case VariableType::tex2d: varTypeStr = "Texture2D"; break;
    case VariableType::staticSmpArray:
    case VariableType::staticTexArray:
    case VariableType::texArray:
      varTypeStr = "Texture2DArray";
      samplerTypeStr = "TextureArraySampler";
      break;
    case VariableType::staticTex3D:
    case VariableType::staticSmp3D:
    case VariableType::tex3d:
    case VariableType::smp3d:
      varTypeStr = "Texture3D";
      samplerTypeStr = "Texture3DSampler";
      break;
    case VariableType::smpCube:
    case VariableType::texCube:
    case VariableType::staticSmpCube:
    case VariableType::staticTexCube:
      varTypeStr = "TextureCube";
      samplerTypeStr = "TextureSamplerCube";
      break;
    case VariableType::staticSmpCubeArray:
    case VariableType::staticTexCubeArray:
    case VariableType::texCubeArray:
      varTypeStr = "TextureCubeArray";
      samplerTypeStr = "TextureCubeArraySampler";
      break;
    case VariableType::staticSmp:
    case VariableType::staticTex:
    case VariableType::smp2d:
      varTypeStr = "Texture2D";
      samplerTypeStr = "TextureSampler";
      break;
#if (_CROSS_TARGET_C1 | _CROSS_TARGET_C2)



#else
    case VariableType::shdArray:
    case VariableType::smpArray: varTypeStr = "Texture2DArray"; break;
    case VariableType::smpCubeArray: varTypeStr = "TextureCubeArray"; break;
#endif
    case VariableType::sampler: varTypeStr = "SamplerState"; break;
    case VariableType::cmpSampler: varTypeStr = "SamplerComparisonState"; break;
    case VariableType::shd: varTypeStr = "Texture2D"; break;
    case VariableType::tlas: varTypeStr = "RaytracingAccelerationStructure"; break;
  }

  {
    const eastl::string regSpecification =
      build_placement_specifier(dest_register, def.isArray, def.arrayElemCount, def.regSpace, ctx.shCtx().blockLevel(), def.isDynamic);

    if (!hlsl.definition.empty())
      hlsl.definition.replaceAll(def.nameSpaceTerm->text, regSpecification.c_str());

    if (varTypeStr)
    {
      if (!hlsl.definition.empty())
        hlsl.definition += '\n';
      if (def.isBindless)
        hlsl.definition.aprintf(0, "uint2 %s%s;", mangledVarName, regSpecification.c_str());
      else
        hlsl.definition.aprintf(0, "%s %s%s;", varTypeStr, baseVarName, regSpecification.c_str());
    }
  }

  if (!hlsl.definition.empty())
  {
    // validate names in HLSL block
    constexpr auto restrictedSymbols = eastl::array<const char>{'@'};
    validate_hlsl_block_for_invalid_symbols(hlsl.definition.c_str(), restrictedSymbols, parser, def);

    char tempbufLine[256];
    tempbufLine[0] = 0;
    SNPRINTF(tempbufLine, sizeof(tempbufLine), "#line %d \"%s\"\n", def.varTerm->line_start,
      parser.get_lexer().get_input_stream()->get_filename(def.varTerm->file_start));
    hlsl.definition.insert(0, tempbufLine, (uint32_t)strlen(tempbufLine));
    hlsl.definition.insert(hlsl.definition.length(), "\n", 1);
    if (strstr(hlsl.definition, "#include "))
    {
      CodeSourceBlocks cb;
      struct NullEval : public ShaderParser::ShaderBoolEvalCB
      {
        bool anyCond = false;
        ShVarBool eval_expr(bool_expr &) override
        {
          anyCond = true;
          return ShVarBool(false, true);
        }
        ShVarBool eval_bool_value(bool_value &) override
        {
          anyCond = true;
          return ShVarBool(false, true);
        }
        int eval_interval_value(const char *ival_name) override
        {
          anyCond = true;
          return -1;
        }
      } null_eval;

      if (!cb.parseSourceCode("", hlsl.definition, null_eval, false, ctx.tgtCtx()))
      {
        report_error(parser, def.varTerm, "bad HLSL decl for named const <%s>:\n%s", baseVarName, hlsl.definition);
        return eastl::nullopt;
      }

      dag::ConstSpan<char> main_src = cb.buildSourceCode(cb.getPreprocessedCode(null_eval));
      if (null_eval.anyCond)
      {
        report_error(parser, def.varTerm, "HLSL decl for named const <%s> shall not contain pre-processor branches:\n%s", baseVarName,
          hlsl.definition);
        return eastl::nullopt;
      }

      hlsl.definition.printf(0, "%.*s", main_src.size(), main_src.data());
    }
  }

  if (def.isBindless)
  {
    G_ASSERT(semantic::vt_is_static_texture(def.type));
#if _CROSS_TARGET_C1 || _CROSS_TARGET_C2

#else
    const bool isStaticCube = def.type == VariableType::staticTexCube || def.type == VariableType::staticSmpCube;
    const bool isStaticCubeArray = def.type == VariableType::staticTexCubeArray || def.type == VariableType::staticSmpCubeArray;
    const bool isStatic3D = def.type == VariableType::staticTex3D || def.type == VariableType::staticSmp3D;
    const bool isStaticArray = def.type == VariableType::staticTexArray || def.type == VariableType::staticSmpArray;
    const char *sampleSuffix = isStaticCube        ? "_cube"
                               : isStaticCubeArray ? "_cube_array"
                               : isStatic3D        ? "3d"
                               : isStaticArray     ? "_array"
                                                   : "";
#endif
    if (semantic::vt_is_static_sampled_texture(def.type))
    {
      hlsl.postfix.aprintf(0,
        "#ifndef BINDLESS_GETTER_%s\n"
        "#define BINDLESS_GETTER_%s\n"
        "%s get_%s()\n"
        "{\n"
        "  %s texSamp;\n"
        "  texSamp.tex = static_textures%s[get_%s().x];\n"
        "  texSamp.smp = static_samplers[get_%s().y];\n"
        "  return texSamp;\n"
        "}\n"
        "#endif\n",
        baseVarName, baseVarName, samplerTypeStr, baseVarName, samplerTypeStr, sampleSuffix, mangledVarName, mangledVarName);
    }
    else
    {
      hlsl.postfix.aprintf(128,
        "#ifndef BINDLESS_GETTER_%s\n"
        "#define BINDLESS_GETTER_%s\n"
        "%s get_%s() { return static_textures%s[get_%s().x]; }\n"
        "#endif\n",
        baseVarName, baseVarName, varTypeStr, baseVarName, sampleSuffix, mangledVarName);
    }
  }
  else if (semantic::vt_is_static_texture(def.type)) // Static textures, but not compiled as bindless
  {
    if (semantic::vt_is_static_sampled_texture(def.type))
    {
      hlsl.postfix.aprintf(0,
        "\n%s get_%s()\n"
        "{\n"
        "  %s texSamp;\n"
        "  texSamp.tex = %s;\n"
        "  texSamp.smp = %s_samplerstate;\n"
        "  return texSamp;\n"
        "}\n",
        samplerTypeStr, baseVarName, samplerTypeStr, baseVarName, baseVarName);
    }
    else
    {
      hlsl.postfix.aprintf(64, "\n%s get_%s() { return %s; }\n", varTypeStr, baseVarName, baseVarName);
    }
  }

  if ((def.shvarType == SHVT_COLOR4 || def.shvarType == SHVT_INT4) && def.isDynamic)
  {
    if ((def.type == VariableType::f44 && def.registerSize == 4) || def.initializer.size() == 1)
      hlsl.postfix.aprintf(0, "%s get_%s() { return %s; }", varTypeStr, baseVarName, baseVarName);
    const auto &varMergerMap = (isGlobalConstBlock ? var_merger_global_blk_map : var_merger_per_stage_maps[def.stage]);
    if (const auto varsIt = varMergerMap.find(baseVarName); varsIt != varMergerMap.end())
    {
      for (const auto &info : varsIt->second)
      {
        hlsl.postfix.aprintf(0, " static %s %s = (%s).%s; %s get_%s() { return %s; }", info.getType(), info.name.c_str(), baseVarName,
          info.getSwizzle(), info.getType(), info.name.c_str(), info.name.c_str());
      }
    }

    hlsl.postfix.aprintf(0, "\n");
  }

  hlsl.postfix.aprintf(0, "#define get_name_%s %i\n", baseVarName,
    ctx.shCtx().compiledShader().uniqueStrings.addNameId(baseVarName) + ctx.shCtx().compiledShader().messages.size());

  return hlsl;
}

template <StcodeBuildFlags FLAGS>
bool build_stcode_for_named_const(const semantic::NamedConstDefInfo &def, int dest_register, shc::VariantContext &ctx,
  StcodePass *out_cppstcode, StcodeBytecodeAccumulator *out_bytecode,
  dag::FixedMoveOnlyFunction<32, bool(int, void *)> register_sampler_var_id, IMemAlloc *tmp_memory, bool add_sampler_vars)
{
  using semantic::VariableType;
  using namespace ShaderParser;

  constexpr bool BUILD_BYTECODE = FLAGS & StcodeBuildFlagsBits::BYTECODE;
  constexpr bool BUILD_CPP = FLAGS & StcodeBuildFlagsBits::CPP;

  if constexpr (BUILD_BYTECODE)
    G_ASSERT(out_bytecode);
  if constexpr (BUILD_CPP)
    G_ASSERT(out_cppstcode);

  static_assert(BUILD_BYTECODE || BUILD_CPP);

  if (def.hardcodedRegister >= 0)
    return true;

  Parser &parser = ctx.tgtCtx().sourceParseState().parser;
  ExpressionParser exprParser{ctx};

  auto registerConstSettingForValidation = [&](int reg, int float_count) {
    if (DAGOR_UNLIKELY(shc::config().generateCppStcodeValidationData && def.isDynamic))
    {
      G_ASSERT(out_cppstcode->cppStcode.constMask);
      out_cppstcode->cppStcode.constMask->add(reg, float_count, def.stage);

      if (def.stage != STAGE_CS && def.regSpace == HLSL_RSPACE_C && ctx.shCtx().blockLevel() == ShaderBlockLevel::GLOBAL_CONST)
      {
        ShaderStage otherStage = def.stage == STAGE_VS ? STAGE_PS : STAGE_VS;
        out_cppstcode->cppStcode.constMask->add(reg, float_count, otherStage);
      }
    }
  };

  const char *varName = def.mangledName.c_str();

  int shcod = -1;
  StcodeRoutine::ResourceType resType = StcodeRoutine::ResourceType::UNKNOWN;

  {
    static const int float4_to_cod[STAGE_MAX] = {SHCOD_CS_CONST, SHCOD_FSH_CONST, SHCOD_VPR_CONST};
    static const int rwtex_to_cod[STAGE_MAX] = {SHCOD_RWTEX_CS, SHCOD_RWTEX_PS, SHCOD_RWTEX_VS};
    static const int rwbuf_to_cod[STAGE_MAX] = {SHCOD_RWBUF_CS, SHCOD_RWBUF_PS, SHCOD_RWBUF_VS};

    if (def.isBindless)
      shcod = SHCOD_REG_BINDLESS;
    else if (def.type == VariableType::uav)
    {
      // @TODO: fixme, bundle with tools update. Is still correct as stcode for shaders ignores RW instr stage.
      if (ctx.shCtx().blockLevel() == ShaderBlockLevel::SHADER)
        shcod = def.shvarType == SHVT_TEXTURE ? SHCOD_RWTEX_PS : SHCOD_RWBUF_PS;
      else
        shcod = def.shvarType == SHVT_TEXTURE ? rwtex_to_cod[def.stage] : rwbuf_to_cod[def.stage];
      resType = def.shvarType == SHVT_TEXTURE ? StcodeRoutine::ResourceType::RWTEX : StcodeRoutine::ResourceType::RWBUF;
    }
    else if (def.shvarType == SHVT_TEXTURE)
    {
      shcod = def.stage == STAGE_VS ? SHCOD_TEXTURE_VS : (def.stage == STAGE_CS ? SHCOD_TEXTURE_CS : SHCOD_TEXTURE);
      resType = StcodeRoutine::ResourceType::TEXTURE;
    }
    else if (def.shvarType == SHVT_SAMPLER)
      resType = StcodeRoutine::ResourceType::SAMPLER;
    else if (def.type == VariableType::cbuf)
    {
      shcod = SHCOD_CONST_BUFFER;
      resType = StcodeRoutine::ResourceType::CONST_BUFFER;
    }
    else if (def.type == VariableType::buf)
    {
      shcod = SHCOD_BUFFER;
      resType = StcodeRoutine::ResourceType::BUFFER;
    }
    else if (def.type == VariableType::tlas)
    {
      shcod = SHCOD_TLAS;
      resType = StcodeRoutine::ResourceType::TLAS;
    }
    else if (def.type == VariableType::sampler || def.type == VariableType::cmpSampler)
    {
      shcod = SHCOD_GLOB_SAMPLER;
      resType = StcodeRoutine::ResourceType::SAMPLER;
    }
    else
    {
      G_ASSERT(def.shvarType == SHVT_COLOR4 || def.shvarType == SHVT_INT4 || def.shvarType == SHVT_FLOAT4X4);
      shcod = float4_to_cod[def.stage];
    }
  }

  for (int initI = 0; initI < def.initializer.size(); ++initI)
  {
    if constexpr (BUILD_BYTECODE)
      out_bytecode->regAllocator->dropCachedStvars();

    const auto &value = def.initializer[initI];
    const int elemDestReg = dest_register + initI;

    if (value.isBuiltinVar())
    {
      if (def.type == VariableType::f44)
      {
        if (def.stage != STAGE_VS)
          report_error(parser, def.varTerm, "Built-in matrices can only be declared in the vertex shader");
        G_ASSERTF(unsigned(dest_register) < 0x3FF, "id=%d base=%d", dest_register, initI);
        int gmType = 0;
        StcodeRoutine::GlobMatrixType gmTypeForCpp;
        switch (value.builtinVarNum())
        {
          case SHADER_TOKENS::SHTOK_globtm:
            gmType = P1_SHCOD_G_TM_GLOBTM;
            gmTypeForCpp = StcodeRoutine::GlobMatrixType::GLOB;
            break;
          case SHADER_TOKENS::SHTOK_projtm:
            gmType = P1_SHCOD_G_TM_PROJTM;
            gmTypeForCpp = StcodeRoutine::GlobMatrixType::PROJ;
            break;
          case SHADER_TOKENS::SHTOK_viewprojtm:
            gmType = P1_SHCOD_G_TM_VIEWPROJTM;
            gmTypeForCpp = StcodeRoutine::GlobMatrixType::VIEWPROJ;
            break;
          default: G_ASSERTF(0, "builtin var token id = %d", value.builtinVarNum());
        }
        if constexpr (BUILD_CPP)
        {
          out_cppstcode->cppStcode.addShaderGlobMatrix(def.stage, gmTypeForCpp, varName, dest_register, initI);
          registerConstSettingForValidation(elemDestReg, semantic::vt_float_size(VariableType::f44));
        }
        if constexpr (BUILD_BYTECODE)
          out_bytecode->push_stcode(shaderopcode::makeOp2_8_16(SHCOD_G_TM, gmType, elemDestReg));
        continue;
      }
      int builtin_v = -1, builtin_cp = 0;
      switch (value.builtinVarNum())
      {
        case SHADER_TOKENS::SHTOK_local_view_x:
          builtin_v = SHCOD_LVIEW;
          builtin_cp = 0;
          break;
        case SHADER_TOKENS::SHTOK_local_view_y:
          builtin_v = SHCOD_LVIEW;
          builtin_cp = 1;
          break;
        case SHADER_TOKENS::SHTOK_local_view_z:
          builtin_v = SHCOD_LVIEW;
          builtin_cp = 2;
          break;
        case SHADER_TOKENS::SHTOK_local_view_pos:
          builtin_v = SHCOD_LVIEW;
          builtin_cp = 3;
          break;
        case SHADER_TOKENS::SHTOK_world_local_x:
          builtin_v = SHCOD_TMWORLD;
          builtin_cp = 0;
          break;
        case SHADER_TOKENS::SHTOK_world_local_y:
          builtin_v = SHCOD_TMWORLD;
          builtin_cp = 1;
          break;
        case SHADER_TOKENS::SHTOK_world_local_z:
          builtin_v = SHCOD_TMWORLD;
          builtin_cp = 2;
          break;
        case SHADER_TOKENS::SHTOK_world_local_pos:
          builtin_v = SHCOD_TMWORLD;
          builtin_cp = 3;
          break;
        default: G_ASSERT(false);
      }
      if (def.type > VariableType::f4 && !(def.type == VariableType::f44 && def.isRegularArray()))
        report_error(parser, def.varTerm, "Lvalue must be f1-4 variable");

      if constexpr (BUILD_BYTECODE)
      {
        Register cr = out_bytecode->regAllocator->add_vec_reg();
        out_bytecode->push_stcode(shaderopcode::makeOp2(builtin_v, int(cr), builtin_cp));
        out_bytecode->push_stcode(shaderopcode::makeOp2(shcod, elemDestReg, int(cr)));
      }
      if constexpr (BUILD_CPP)
      {
        out_cppstcode->cppStcode.addShaderGlobVec(def.stage,
          builtin_v == SHCOD_LVIEW ? StcodeRoutine::GlobVecType::VIEW : StcodeRoutine::GlobVecType::WORLD, varName, dest_register,
          builtin_cp, initI);
        registerConstSettingForValidation(elemDestReg, semantic::vt_float_size(VariableType::f4));
      }
      continue;
    }
    if (def.shvarType == SHVT_COLOR4 || def.shvarType == SHVT_INT4)
    {
      G_ASSERT(value.isArithmExpr() || value.isArithmConst());

      if constexpr (BUILD_BYTECODE)
      {
        Tab<int> cod(tmpmem);
        Register dr = out_bytecode->regAllocator->add_vec_reg();

        if (value.isArithmConst())
          Expression::assembleBytecodeForConstant(cod, value.arithmConst(), int(dr));
        else
          value.arithmExpr().assembleBytecode(cod, dr, *out_bytecode->regAllocator, def.shvarType == SHVT_INT4);

        cod.push_back(shaderopcode::makeOp2(shcod, elemDestReg, int(dr)));
        out_bytecode->append_alt_stcode(def.isDynamic, cod);
      }

      if constexpr (BUILD_CPP)
      {
        StcodeExpression cppExpr;
        if (value.isArithmConst())
          Expression::assembleCppForConstant(cppExpr, value.arithmConst());
        else
          value.arithmExpr().assembleCpp(cppExpr, def.shvarType == SHVT_INT4);

        if (def.isDynamic)
        {
          out_cppstcode->cppStcode.addShaderConst(def.stage, def.shvarType, def.type, varName, dest_register, eastl::move(cppExpr),
            initI);
        }
        else
        {
          out_cppstcode->cppStblkcode.addShaderConst(def.shvarType, def.type, varName, dest_register, eastl::move(cppExpr), initI);
        }

        // In this codepath matrices are set row-by-row, so clamp by f4
        registerConstSettingForValidation(elemDestReg,
          semantic::vt_float_size(def.type == VariableType::f44 ? VariableType::f4 : def.type));
      }
    }
    else
    {
      if (value.isGlobalVar())
      {
        if (def.shvarType == SHVT_SAMPLER)
        {
          G_ASSERT(initI == 0);
          const char *samplerName = value.varName();
          Sampler *smp = ctx.tgtCtx().samplers().get(samplerName);
          G_ASSERT(smp);
          if (smp->mIsStaticSampler && register_sampler_var_id(smp->mId, def.varTerm))
          {
            String bcName{}, amName{}, mbName{};
            int bcReg = -1, amReg = -1, mbReg = -1;
            bool bcIsConst = true, amIsConst = true, mbIsConst = true;
            Color4 bcConstFloatValue{};

            if (smp->mBorderColor)
            {
              bcName = String(32, "__border_color%i", smp->mId);
              String local_bc(32, "local float4 %s = 0;", bcName);
              auto *local_bc_stat = parse_shader_stat(local_bc, local_bc.length(), ctx.tgtCtx(), tmp_memory);
              local_bc_stat->local_decl->expr = smp->mBorderColor;

              auto [variable, rootExpr] = *semantic::parse_local_var_decl(*local_bc_stat->local_decl, ctx);
              bcIsConst = variable->isConst;

              if (variable->isConst)
              {
                bcConstFloatValue = Color4{variable->cv.c.r, variable->cv.c.g, variable->cv.c.b, variable->cv.c.a};
                smp->mSamplerInfo.border_color = static_cast<d3d::BorderColor::Color>(
                  (variable->cv.c.a ? 0xFF000000 : 0) | ((variable->cv.c.r || variable->cv.c.g || variable->cv.c.b) ? 0x00FFFFFF : 0));
              }
              else
              {
                assembly::assemble_local_var<FLAGS>(variable, rootExpr.get(), local_bc_stat->local_decl->name, out_cppstcode,
                  out_bytecode);
                G_ASSERT(!BUILD_BYTECODE || variable->reg >= 0);
                bcReg = variable->reg;
              }
            }

            if (smp->mAnisotropicMax)
            {
              amName = String(32, "__anisotropic_max%i", smp->mId);
              String local_am(32, "local float4 %s = 0;", amName);
              auto *local_am_stat = parse_shader_stat(local_am, local_am.length(), ctx.tgtCtx(), tmp_memory);
              local_am_stat->local_decl->expr = smp->mAnisotropicMax;

              auto [variable, rootExpr] = *semantic::parse_local_var_decl(*local_am_stat->local_decl, ctx);
              amIsConst = variable->isConst;

              if (variable->isConst)
                smp->mSamplerInfo.anisotropic_max = variable->cv.r;
              else
              {
                assembly::assemble_local_var<FLAGS>(variable, rootExpr.get(), local_am_stat->local_decl->name, out_cppstcode,
                  out_bytecode);
                G_ASSERT(!BUILD_BYTECODE || variable->reg >= 0);
                amReg = variable->reg;
              }
            }

            if (smp->mMipmapBias)
            {
              mbName = String(32, "__mipmap_bias%i", smp->mId);
              String local_mb(32, "local float4 %s = 0;", mbName);
              auto *local_mb_stat = parse_shader_stat(local_mb, local_mb.length(), ctx.tgtCtx(), tmp_memory);
              local_mb_stat->local_decl->expr = smp->mMipmapBias;

              auto [variable, rootExpr] = *semantic::parse_local_var_decl(*local_mb_stat->local_decl, ctx);
              mbIsConst = variable->isConst;

              if (variable->isConst)
                smp->mSamplerInfo.mip_map_bias = variable->cv.r;
              else
              {
                assembly::assemble_local_var<FLAGS>(variable, rootExpr.get(), local_mb_stat->local_decl->name, out_cppstcode,
                  out_bytecode);
                G_ASSERT(!BUILD_BYTECODE || variable->reg >= 0);
                mbReg = variable->reg;
              }
            }

            const bool samplerIsImmutable = bcIsConst && amIsConst && mbIsConst;

            // If the sampler is immutable, we won't request it in code at all, but pre-fill the shadervar
            if (samplerIsImmutable)
            {
              // @TODO: target context write access. Will have to remove when parallelizing.
              ctx.tgtCtx().samplers().reportImmutableSampler(smp->mId, value.globVarId());
            }
            else
            {

              if constexpr (BUILD_BYTECODE)
              {
                out_bytecode->push_stcode(
                  shaderopcode::makeOpFunctionCall(SHCOD_CALL_FUNCTION, functional::BF_REQUEST_SAMPLER, smp->mId, 4));
                out_bytecode->push_stcode(value.globVarId());
                out_bytecode->push_stcode(bcReg);
                out_bytecode->push_stcode(amReg);
                out_bytecode->push_stcode(mbReg);
              }

              if constexpr (BUILD_CPP)
              {
                eastl::string exprTemplate{
                  eastl::string::CtorSprintf{}, "*(uint64_t *)%s = %c", samplerName, StcodeExpression::EXPR_ELEMENT_PLACEHOLDER};

                StcodeExpression expr{exprTemplate.c_str()};

                String samplerIdVarName{0, "%s__id", samplerName};

                // @TODO: target context write access. Will have to remove when parallelizing.
                ctx.tgtCtx().cppStcode().globVars.setVar(SHVT_INT, samplerIdVarName.c_str(), false);

                const size_t argCnt = 4;

                expr.specifyNextExprElement(StcodeExpression::ElementType::FUNC, "request_sampler", &argCnt);
                expr.specifyNextExprElement(StcodeExpression::ElementType::GLOBVAR, samplerIdVarName.c_str());
                expr.specifyNextExprUnaryOp(shexpr::UOP_POSITIVE);

                auto addParam = [&](void *val, const char *name, bool is_const, StcodeExpression::ElementType const_type,
                                  const char *var_cast) {
                  if (is_const)
                    expr.specifyNextExprElement(const_type, val);
                  else
                  {
                    expr.specifyNextExprElement(StcodeExpression::ElementType::LOCVAR, name, var_cast);
                    expr.specifyNextExprUnaryOp(shexpr::UOP_POSITIVE);
                  }
                };

                if (bcIsConst)
                {
                  expr.specifyNextExprElement(StcodeExpression::ElementType::COLOR_CONST, //
                    &bcConstFloatValue.r, &bcConstFloatValue.g, &bcConstFloatValue.b, &bcConstFloatValue.a);
                }
                else
                {
                  addParam(nullptr, bcName, false, StcodeExpression::ElementType::COLOR_CONST, nullptr);
                }

                addParam(&smp->mSamplerInfo.anisotropic_max, amName, amIsConst, StcodeExpression::ElementType::REAL_CONST, "float");
                addParam(&smp->mSamplerInfo.mip_map_bias, mbName, mbIsConst, StcodeExpression::ElementType::REAL_CONST, "float");

                out_cppstcode->cppStcode.addStmt(expr.releaseAssembledCode().c_str());
              }
            }
          }

          if constexpr (BUILD_BYTECODE)
          {
            out_bytecode->push_stcode(shaderopcode::makeOpStageSlot(SHCOD_GLOB_SAMPLER, def.stage, elemDestReg, value.globVarId()));
          }
          if constexpr (BUILD_CPP)
          {
            out_cppstcode->cppStcode.addGlobalShaderResource(def.stage, StcodeRoutine::ResourceType::SAMPLER, def.varTerm->text,
              value.varName(), dest_register, initI);
          }
          continue;
        }

        if constexpr (BUILD_BYTECODE)
        {
          int gc;
          Register reg;
          switch (def.shvarType)
          {
            case SHVT_TEXTURE:
              gc = SHCOD_GET_GTEX;
              reg = out_bytecode->regAllocator->add_reg(def.shvarType);
              break;
            case SHVT_BUFFER:
              gc = SHCOD_GET_GBUF;
              reg = out_bytecode->regAllocator->add_reg(def.shvarType);
              break;
            case SHVT_TLAS:
              gc = SHCOD_GET_GTLAS;
              reg = out_bytecode->regAllocator->add_reg(def.shvarType);
              break;
            case SHVT_FLOAT4X4:
              gc = SHCOD_GET_GMAT44;
              reg = out_bytecode->regAllocator->add_vec_reg(4);
              break;
            default: G_ASSERT(0);
          }

          out_bytecode->push_stcode(shaderopcode::makeOp2(gc, int(reg), value.globVarId()));
          if (shcod == SHCOD_BUFFER || shcod == SHCOD_CONST_BUFFER || shcod == SHCOD_TLAS)
          {
            out_bytecode->push_stcode(shaderopcode::makeOpStageSlot(shcod, def.stage, elemDestReg, int(reg)));
          }
          else
          {
            out_bytecode->push_stcode(shaderopcode::makeOp2(shcod, elemDestReg, int(reg)));
            if (def.shvarType == SHVT_FLOAT4X4)
            {
              out_bytecode->push_stcode(shaderopcode::makeOp2(shcod, elemDestReg + 1, int(reg) + 1 * 4));
              out_bytecode->push_stcode(shaderopcode::makeOp2(shcod, elemDestReg + 2, int(reg) + 2 * 4));
              out_bytecode->push_stcode(shaderopcode::makeOp2(shcod, elemDestReg + 3, int(reg) + 3 * 4));
            }
          }
        }

        if constexpr (BUILD_CPP)
        {
          if (def.shvarType == SHVT_FLOAT4X4)
          {
            StcodeExpression expr;
            expr.specifyNextExprElement(StcodeExpression::ElementType::GLOBVAR, value.varName(), nullptr, (void *)true);
            expr.specifyNextExprUnaryOp(shexpr::UOP_POSITIVE);
            out_cppstcode->cppStcode.addShaderConst(def.stage, SHVT_FLOAT4X4, VariableType::f44, varName, elemDestReg,
              eastl::move(expr));

            registerConstSettingForValidation(elemDestReg, semantic::vt_float_size(VariableType::f44));
          }
          else
          {
            G_ASSERT(resType != StcodeRoutine::ResourceType::UNKNOWN);
            out_cppstcode->cppStcode.addGlobalShaderResource(def.stage, resType, varName, value.varName(), dest_register, initI);
          }
        }
      }
      else
      {
        int buffer[2];

        G_ASSERT(def.shvarType == SHVT_TEXTURE);

        if constexpr (BUILD_BYTECODE)
        {
          Register reg = out_bytecode->regAllocator->add_reg(def.shvarType);
          buffer[0] = shaderopcode::makeOp2(SHCOD_GET_TEX, int(reg), value.materialVarId());
          buffer[1] = shaderopcode::makeOp2(shcod, elemDestReg, int(reg));

          out_bytecode->append_alt_stcode(def.isDynamic, make_span_const(buffer, 2));
        }

        if constexpr (BUILD_CPP)
        {
          // @NOTE: stage gets manually changed to PS for bindless textures, for cpp stcode we need the original
          if (def.isDynamic)
          {
            out_cppstcode->cppStcode.addDynamicShaderResource(def.stage, StcodeRoutine::ResourceType::TEXTURE, varName,
              value.varName(), dest_register, initI);
          }
          else if (shc::config().enableBindless)
          {
            out_cppstcode->cppStblkcode.addBindlessShaderTexture(varName, value.varName(), elemDestReg);
            registerConstSettingForValidation(elemDestReg, semantic::vt_float_size(VariableType::i2));
          }
          else
          {
            out_cppstcode->cppStblkcode.addStaticShaderTex(def.stage, varName, value.varName(), elemDestReg);
          }
        }
      }
    }
  }

  return true;
}

String build_hlsl_for_pair_sampler(const char *const_name, bool is_shadow, int dest_register)
{
  String res{};
  if (is_shadow)
    res.aprintf(0, "SamplerComparisonState %s_cmpSampler: register(s%d);\n", const_name, dest_register);

  res.aprintf(0, "SamplerState %s_samplerstate: register(s%d);\n", const_name, dest_register);
  return res;
}

template <StcodeBuildFlags FLAGS>
void build_stcode_for_pair_sampler(const char *const_name, const char *var_name, int dest_reg, ShaderStage stage, int var_id,
  bool is_global, StcodePass *out_cppstcode, StcodeBytecodeAccumulator *out_bytecode)
{
  if constexpr (FLAGS & StcodeBuildFlagsBits::BYTECODE)
  {
    if (is_global)
      out_bytecode->push_stcode(shaderopcode::makeOpStageSlot(SHCOD_GLOB_SAMPLER, stage, dest_reg, var_id));
    else
      out_bytecode->push_stcode(shaderopcode::makeOpStageSlot(SHCOD_SAMPLER, 0, dest_reg, var_id));
  }

  if constexpr (FLAGS & StcodeBuildFlagsBits::CPP)
  {
    if (is_global)
      out_cppstcode->cppStcode.addGlobalShaderResource(stage, StcodeRoutine::ResourceType::SAMPLER, const_name, var_name, dest_reg);
    else
      out_cppstcode->cppStcode.addDynamicShaderResource(stage, StcodeRoutine::ResourceType::SAMPLER, const_name, var_name, dest_reg);
  }
}

void build_cpp_declarations_for_used_local_vars(StcodePass &out_cppstcode, const shc::VariantContext &ctx)
{
  for (const LocalVar &lvar : ctx.localStcodeVars().getVars())
  {
    if (!lvar.isConst)
    {
      StcodeRoutine &routine = lvar.isDynamic ? (StcodeRoutine &)out_cppstcode.cppStcode : (StcodeRoutine &)out_cppstcode.cppStblkcode;

      routine.addLocalVarDecl(stcode::shexpr_value_to_shadervar_type(lvar.valueType, lvar.isInteger),
        ctx.tgtCtx().varNameMap().getName(lvar.varNameId));
    }
  }
}

void build_cpp_declarations_for_used_bool_vars(StcodePass &out_cppstcode, const shc::VariantContext &ctx)
{
  ctx.localBoolVars().iterateBoolVars([&](const char *name, const BoolVar &) {
    out_cppstcode.cppStcode.addBoolVarDecl(name);
    out_cppstcode.cppStblkcode.addBoolVarDecl(name);
  });
}

// Explicit insantiations, added once they are needed
template void assemble_local_var<StcodeBuildFlagsBits::ALL>(LocalVar *, ShaderParser::ComplexExpression *,
  ShaderTerminal::SHTOK_ident *, StcodePass *out_cppstcode, StcodeBytecodeAccumulator *out_bytecode);
template void assemble_local_var<StcodeBuildFlagsBits::CPP>(LocalVar *, ShaderParser::ComplexExpression *,
  ShaderTerminal::SHTOK_ident *, StcodePass *out_cppstcode, StcodeBytecodeAccumulator *out_bytecode);
template bool build_stcode_for_named_const<StcodeBuildFlagsBits::ALL>(const semantic::NamedConstDefInfo &, int, shc::VariantContext &,
  StcodePass *out_cppstcode, StcodeBytecodeAccumulator *out_bytecode, dag::FixedMoveOnlyFunction<32, bool(int, void *)>, IMemAlloc *,
  bool);
template bool build_stcode_for_named_const<StcodeBuildFlagsBits::CPP>(const semantic::NamedConstDefInfo &, int, shc::VariantContext &,
  StcodePass *out_cppstcode, StcodeBytecodeAccumulator *out_bytecode, dag::FixedMoveOnlyFunction<32, bool(int, void *)>, IMemAlloc *,
  bool);
template void build_stcode_for_pair_sampler<StcodeBuildFlagsBits::ALL>(const char *, const char *, int, ShaderStage, int, bool,
  StcodePass *out_cppstcode, StcodeBytecodeAccumulator *out_bytecode);
template void build_stcode_for_pair_sampler<StcodeBuildFlagsBits::CPP>(const char *, const char *, int, ShaderStage, int, bool,
  StcodePass *out_cppstcode, StcodeBytecodeAccumulator *out_bytecode);

} // namespace assembly
