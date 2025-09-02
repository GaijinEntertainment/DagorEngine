// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cppStcodeBuilder.h"
#include "hash.h"
#include "hlslRegisters.h"
#include <shaders/slotTexturesRange.h>
#include <memory/dag_mem.h>
#include <shaders/shExprTypes.h>
#include <shaders/shFunc.h>
#include <shaders/dag_shaderVarType.h>
#include <drv/3d/dag_consts.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>
#include <EASTL/string.h>
#include <dag/dag_vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <shaders/shader_layout.h>

class ShaderSemCode;
class SemanticShaderPass;

namespace semantic
{
enum class VariableType;
}

struct StcodeStrings
{
  eastl::string cppCode, headerCode;
};

struct StcodeAccumulator
{
  StcodeBuilder cppStrings, headerStrings;

  StcodeStrings release() { return StcodeStrings{cppStrings.release(), headerStrings.release()}; }
  void merge(StcodeAccumulator &&other)
  {
    cppStrings.merge(eastl::move(other.cppStrings));
    headerStrings.merge(eastl::move(other.headerStrings));
  }
};

struct StcodeExpression
{
  eastl::string content;

  static constexpr char EXPR_ELEMENT_PLACEHOLDER = '?';
  static constexpr char EXPR_UNARY_OP_PLACEHOLDER = '#';

  static constexpr char DEFAULT_TEMPLATE[] = {EXPR_ELEMENT_PLACEHOLDER, '\0'};

  // @NOTE: use placeholder constants instead of literal ? and # in the template
  StcodeExpression(const char *template_content = DEFAULT_TEMPLATE) : content(template_content) {}

  enum class ElementType
  {
    COMPLEX_SINGLE_OP,
    COMPLEX_MULTIPLE_OPS,
    COLORVAL,
    GLOBVAR,
    LOCVAR,
    SHVAR,
    REAL_CONST,
    INT_CONST,
    COLOR_CONST,
    SINGLE_CHANNEL_MASK,
    MULTIPLE_CHANNEL_MASK,
    FUNC,
  };

  void specifyNextExprElement(ElementType elem_type, const void *arg1 = nullptr, const void *arg2 = nullptr,
    const void *arg3 = nullptr, const void *arg4 = nullptr);
  void specifyNextExprUnaryOp(shexpr::UnaryOperator op);

  eastl::string releaseAssembledCode();

  static eastl::string getFunctionCallTemplate(functional::FunctionId func_id, const char *name, int arg_cnt);
};

struct StcodeRegisters
{
  struct VarLocation
  {
    eastl::string name;
    int reg = -1;

    bool isNumeric = false;
    int regSize = 1;
  };
  dag::Vector<VarLocation> content;

  void add(const char *name, int reg, bool is_numeric = false, int reg_size = 1)
  {
    content.push_back(VarLocation{eastl::string(name), reg, is_numeric, reg_size});
  }

  void clear() { content.clear(); }

  void merge(StcodeRegisters &&other)
  {
    content.insert(content.end(), eastl::make_move_iterator(other.content.begin()), eastl::make_move_iterator(other.content.end()));
  }
};

struct StcodeStaticVars
{
  struct Var
  {
    eastl::string name;
    int initId = -1;
    int patchedId = -1;

    int getId() const { return patchedId == -1 ? initId : patchedId; }
  };
  dag::Vector<Var> content;

  void add(const char *name, int id) { content.push_back(Var{eastl::string(name), id, -1}); }

  void patch(int old_reg, int new_reg)
  {
    for (Var &loc : content)
    {
      // @NOTE: all regs with same old reg get patched (so, tex-smp pair get patched together)
      if (loc.patchedId == -1 && loc.initId == old_reg)
        loc.patchedId = new_reg;
    }
  }
};

struct StcodeRoutine
{
  enum class ResourceType
  {
    UNKNOWN,
    TEXTURE,
    SAMPLER,
    BUFFER,
    CONST_BUFFER,
    TLAS,
    RWTEX,
    RWBUF,
  };
  enum class GlobMatrixType
  {
    GLOB,
    PROJ,
    VIEWPROJ
  };
  enum class GlobVecType
  {
    WORLD,
    VIEW
  };

  StcodeBuilder code;
  StcodeBuilder decls;
  StcodeRegisters vertexRegs, pixelOrComputeRegs;

  eastl::unique_ptr<shader_layout::StcodeConstValidationMask> constMask{};

  int nonContributingStatementsCnt = 0;
  int blockDepth = 1; // Base function indentation is 1

  bool hasCodeUnderConditions = false;

  StcodeRoutine();

  StcodeRoutine(StcodeRoutine &&other) = default;
  StcodeRoutine &operator=(StcodeRoutine &&other) = default;

  // Non-copyable
  StcodeRoutine(const StcodeRoutine &other) = delete;
  StcodeRoutine &operator=(const StcodeRoutine &other) = delete;

  void reset()
  {
    code.clear();
    decls.clear();
    vertexRegs.clear();
    pixelOrComputeRegs.clear();
    constMask.reset();
  }

  void merge(StcodeRoutine &&other)
  {
    code.merge(eastl::move(other.code));
    decls.merge(eastl::move(other.decls));
    vertexRegs.merge(eastl::move(other.vertexRegs));
    pixelOrComputeRegs.merge(eastl::move(other.pixelOrComputeRegs));

    if (constMask && other.constMask)
    {
      auto omask = eastl::move(other.constMask);
      constMask->merge(*omask);
    }
  }

  static constexpr size_t STATIC_VAR_ABSCENT_ID = size_t(-1);

  bool hasCode() const { return code.fragCount() > nonContributingStatementsCnt; }

  CryptoHash calcHash(const CryptoHash *base = nullptr);

  StcodeRegisters &regsForStage(ShaderStage stage) { return stage == STAGE_VS ? vertexRegs : pixelOrComputeRegs; }

  void addBoolVarDecl(const char *name);
  void addLocalVarDecl(ShaderVarType type, const char *name);

  void setVarValue(const char *name, const char *val);
  void setVarValue(const char *name, StcodeExpression &&expr) { setVarValue(name, expr.releaseAssembledCode().c_str()); }

  void addStmt(const char *val);
  void addStmt(StcodeExpression &&expr) { addStmt(expr.releaseAssembledCode().c_str()); }

  void addSetConstStmt(const char *type, size_t type_size, const char *location, const char *expr, const char *dest_array,
    int array_index);

  template <class... TArgs>
  void addFormattedStatement(const char *fmt, TArgs &&...args)
  {
    G_ASSERT(blockDepth > 0);
    if (blockDepth > 1)
      hasCodeUnderConditions = true;
    ++nonContributingStatementsCnt; // For indent
    code.emplaceBack(makeIndentedLine(blockDepth));
    code.emplaceBackFmt(fmt, eastl::forward<TArgs>(args)...);
  }

  static eastl::string makeIndentedLine(int depth)
  {
    eastl::string line{};
    line.resize(depth * 2, ' ');
    return line;
  }
};

struct DynamicStcodeRoutine : StcodeRoutine
{
  bool isCompute = false;
  bool usesDynamicBranching = false;

  explicit DynamicStcodeRoutine(bool uses_dynamic_branching = false) : usesDynamicBranching{uses_dynamic_branching} {}

  void addGlobalShaderResource(ShaderStage stage, ResourceType type, const char *name, const char *var_name, int id);
  void addDynamicShaderResource(ShaderStage stage, ResourceType type, const char *name, const char *var_name, int id);
  void addShaderGlobMatrix(ShaderStage stage, GlobMatrixType type, const char *name, int id);
  void addShaderGlobVec(ShaderStage stage, GlobVecType type, const char *name, int id, int comp_id);

  void addShaderConst(ShaderStage stage, ShaderVarType shvt, semantic::VariableType vt, const char *name, int id, const char *val,
    int array_index = 0);
  void addShaderConst(ShaderStage stage, ShaderVarType shvt, semantic::VariableType vt, const char *name, int id,
    StcodeExpression &&expr, int array_index = 0)
  {
    addShaderConst(stage, shvt, vt, name, id, expr.releaseAssembledCode().c_str(), array_index);
  }

  // @TODO: make a builder for cpp bool expressions

  void addIfClause(const char *cond_text)
  {
    G_ASSERT(usesDynamicBranching);
    addFormattedStatement("if (%s) {\n", cond_text);
    ++nonContributingStatementsCnt;
    ++blockDepth;
  }
  void addElseClause()
  {
    G_ASSERT(usesDynamicBranching);
    --blockDepth;
    addFormattedStatement("} else {\n");
    ++nonContributingStatementsCnt;
    ++blockDepth;
  }
  void addBlockClosing()
  {
    G_ASSERT(usesDynamicBranching);
    --blockDepth;
    addFormattedStatement("}\n");
    ++nonContributingStatementsCnt;
  }

  void reportStageUsage(ShaderStage stage) { isCompute = stage == STAGE_CS; }

  // Needed for the following use case:
  //  for global const block registers are allocated from the same 'buffered' reg allocator, but set in subranges
  //  for each stage to different actual constbuffers. Therefore we can't use the range from the allocator, and
  //  instead have to calculate from the actual routine mapping.
  HlslRegRange collectSetRegistersRange(ShaderStage stage) const
  {
    HlslRegRange range{};
    auto &regs = stage == STAGE_VS ? vertexRegs : pixelOrComputeRegs;
    for (const auto &loc : regs.content)
    {
      if (loc.isNumeric)
        range = update_range(range, loc.reg);
    }
    return range;
  }

  StcodeAccumulator releaseAssembledCode(size_t routine_idx, size_t static_var_idx, HlslRegRange ps_or_cs_const_range,
    HlslRegRange vs_const_range);
};

struct StaticStcodeRoutine : StcodeRoutine
{
  StcodeRegisters constRegs;
  bool supportsMultidraw = false;

  void reset()
  {
    StcodeRoutine::reset();
    constRegs.clear();
  }
  void merge(StaticStcodeRoutine &&other)
  {
    constRegs.merge(eastl::move(other.constRegs));
    StcodeRoutine::merge(eastl::move(other));
  }

  void addShaderConst(ShaderVarType shvt, semantic::VariableType vt, const char *name, int id, const char *val, int array_index = 0);
  void addShaderConst(ShaderVarType shvt, semantic::VariableType vt, const char *name, int id, StcodeExpression &&expr,
    int array_index = 0)
  {
    addShaderConst(shvt, vt, name, id, expr.releaseAssembledCode().c_str(), array_index);
  }

  void addStaticShaderTex(ShaderStage stage, const char *name, const char *var_name, int id);
  void addBindlessShaderTexture(const char *name, const char *var_name, int id);

  void reportMutlidrawSupport() { supportsMultidraw = true; }

  StcodeAccumulator releaseAssembledCode(size_t routine_idx, size_t static_var_idx, HlslRegRange const_range,
    SlotTexturesRangeInfo ps_tex_smp_range, SlotTexturesRangeInfo vs_tex_smp_range);

  CryptoHash calcHash(const CryptoHash *base = nullptr);
};

struct StcodePass
{
  DynamicStcodeRoutine cppStcode;
  StaticStcodeRoutine cppStblkcode;

  explicit StcodePass(bool uses_dynamic_branching = false) : cppStcode{uses_dynamic_branching}, cppStblkcode{} {}

  void reset()
  {
    cppStcode.reset();
    cppStblkcode.reset();
  }
};

struct StcodeGlobalVars
{
  enum class Type
  {
    MAIN_COLLECTION,
    REFERENCED_BY_SHADER
  };

  struct Strings
  {
    eastl::string cppCode;
    eastl::string fetchersOrFwdCppCode;
  };

  static constexpr int INVALID_IN_DUMP_VAR_ID = -1;

  StcodeBuilder cppCode;
  StcodeBuilder fetchersOrFwdCpp;
  Type type;

  StcodeGlobalVars(Type a_type) : type(a_type) {}

  void setVar(ShaderVarType shv_type, const char *name, int id_in_final_dump = INVALID_IN_DUMP_VAR_ID);
  Strings releaseAssembledCode();
};

struct StcodeShader
{
  struct RegisterTable
  {
    Tab<Tab<int>> combinedTable{};
    Tab<int> offsets{};
  };
  struct RoutineRemappingTables
  {
    Tab<int> dynamicRemapping;
    Tab<int> staticRemapping;
  };
  struct InterfaceStrings
  {
    eastl::string regTable;
    eastl::string dynamicFuncPointerTable;
    eastl::string staticFuncPointerTable;
  };

  bool hasCode = false;
  StcodeAccumulator code;
  StcodeGlobalVars globVars{StcodeGlobalVars::Type::REFERENCED_BY_SHADER};
  eastl::string shaderName;
  int curStvarId = -1;

  Tab<CryptoHash> dynamicRoutineHashes{};
  Tab<CryptoHash> staticRoutineHashes{};
  Tab<CryptoHash> stvarHashes{};
  Tab<bindump::string> dynamicRoutineNames{};
  Tab<bindump::string> staticRoutineNames{};
  RegisterTable regTable{};

  void reset() { *this = StcodeShader(); }

  void addStaticVarLocations(StcodeStaticVars &&locations);
  int addRegisterTableWithIndex(Tab<int> &&table);
  int addRegisterTableWithOffset(Tab<int> &&table);

  int addCode(DynamicStcodeRoutine &&routine, HlslRegRange ps_or_cs_const_range, HlslRegRange vs_const_range);
  int addCode(StaticStcodeRoutine &&routine, HlslRegRange const_range, SlotTexturesRangeInfo ps_tex_smp_range,
    SlotTexturesRangeInfo vs_tex_smp_range);

  RoutineRemappingTables linkRoutinesFromFile(dag::ConstSpan<CryptoHash> dynamic_routine_hashes,
    dag::ConstSpan<CryptoHash> static_routine_hashes, const char *fn);

  InterfaceStrings releaseAssembledInterfaceCode();
};

struct StcodePassRegInfo
{
  Tab<eastl::pair<eastl::string, int>> regsByName{};
  HlslRegRange vsCstRange, psOrCsCstRange;
  bool isCompute;
};

// Table processing for dynamic branches for ability to restore dyn variant info
using StcodePassRegInfoTable = ska::flat_hash_map<SemanticShaderPass *, StcodePassRegInfo>;

StcodePassRegInfoTable build_pass_stcode_reg_table(const ShaderSemCode &semcode);

struct StcodeBranchedRoutineDataTable
{
  ska::flat_hash_map<SemanticShaderPass *, Tab<int>> passRegisterTables;
  HlslRegRange commonVsConstRange{};
  HlslRegRange commonPsOrCsConstRange{};
};

StcodeBranchedRoutineDataTable build_pass_reg_tables_for_branched_dynstcode(const StcodePassRegInfoTable &passRegsTable,
  DynamicStcodeRoutine &branched_routine);
