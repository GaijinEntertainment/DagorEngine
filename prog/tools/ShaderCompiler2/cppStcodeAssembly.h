// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cppStcodeBuilder.h"
#include <memory/dag_mem.h>
#include <shaders/shExprTypes.h>
#include <shaders/shFunc.h>
#include <shaders/dag_shaderVarType.h>
#include <drv/3d/dag_consts.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>
#include <EASTL/string.h>
#include <dag/dag_vector.h>

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

class StcodeExpression
{
public:
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

private:
  eastl::string content;

  static eastl::string getFunctionCallTemplate(functional::FunctionId func_id, const char *name, int arg_cnt);
};

struct StcodeRegisters
{
  struct VarLocation
  {
    eastl::string name;
    int initReg = -1;
    int patchedReg = -1;

    int getRegNum() const { return patchedReg == -1 ? initReg : patchedReg; }
  };
  dag::Vector<VarLocation> content;

  void add(const char *name, int reg) { content.push_back(VarLocation{eastl::string(name), reg, -1}); }

  void patch(int old_reg, int new_reg)
  {
    for (VarLocation &loc : content)
    {
      // @NOTE: all regs with same old reg get patched (so, tex-smp pair get patched together)
      if (loc.patchedReg == -1 && loc.initReg == old_reg)
        loc.patchedReg = new_reg;
    }
  }

  void clear() { content.clear(); }

  void merge(StcodeRegisters &&other)
  {
    content.insert(content.end(), eastl::make_move_iterator(other.content.begin()), eastl::make_move_iterator(other.content.end()));
  }
};

class StcodeRoutine
{
public:
  enum class Type
  {
    DYNSTCODE,
    STBLKCODE
  };
  enum class ResourceType
  {
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

  explicit StcodeRoutine(Type a_type) : type(a_type) {}

  StcodeRoutine(StcodeRoutine &&other) = default;
  StcodeRoutine &operator=(StcodeRoutine &&other) = default;

  // Non-copyable
  StcodeRoutine(const StcodeRoutine &other) = delete;
  StcodeRoutine &operator=(const StcodeRoutine &other) = delete;

  void reset()
  {
    code.clear();
    vertexRegs.clear();
    pixelRegs.clear();
  }

  void merge(StcodeRoutine &&other)
  {
    code.merge(eastl::move(other.code));
    vertexRegs.merge(eastl::move(other.vertexRegs));
    pixelRegs.merge(eastl::move(other.pixelRegs));
  }

  static constexpr size_t STATIC_VAR_ABSCENT_ID = (size_t)(-1);

  StcodeAccumulator releaseAssembledCode(size_t routine_idx, size_t static_var_idx = STATIC_VAR_ABSCENT_ID);

  bool hasCode() const { return !code.empty(); }

  void addGlobalShaderResource(ShaderStage stage, ResourceType type, const char *name, const char *var_name, bool var_is_global,
    int id);
  void addShaderResource(ShaderStage stage, ResourceType type, const char *name, const char *var_name, int id);
  void addShaderGlobMatrix(ShaderStage stage, GlobMatrixType type, const char *name, int id);
  void addShaderGlobVec(ShaderStage stage, GlobVecType type, const char *name, int id, int comp_id);

  void addLocalVar(ShaderVarType type, const char *name, const char *val);
  void addLocalVar(ShaderVarType type, const char *name, StcodeExpression &&expr)
  {
    addLocalVar(type, name, expr.releaseAssembledCode().c_str());
  }

  void addShaderConst(ShaderStage stage, ShaderVarType type, const char *name, int id, const char *val, int array_index = 0);
  void addShaderConst(ShaderStage stage, ShaderVarType type, const char *name, int id, StcodeExpression &&expr, int array_index = 0)
  {
    addShaderConst(stage, type, name, id, expr.releaseAssembledCode().c_str(), array_index);
  }

  void addStmt(const char *val);
  void addStmt(StcodeExpression &&expr) { addStmt(expr.releaseAssembledCode().c_str()); }

  void patchConstLocation(ShaderStage stage, int old_id, int new_id);

private:
  Type type;

  StcodeBuilder code;
  StcodeRegisters vertexRegs, pixelRegs;

  StcodeRegisters &regsForStage(ShaderStage stage) { return stage == STAGE_VS ? vertexRegs : pixelRegs; }

  static eastl::string genSetConstStmt(ShaderStage stage, const char *type, size_t type_reg_size, const char *location,
    const char *expr, int array_index);

  static bool resourceIsStoredDirectly(ResourceType resType)
  {
    return resType == ResourceType::SAMPLER || resType == ResourceType::TLAS;
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
  bool hasCode = false;
  StcodeAccumulator code;
  StcodeGlobalVars globVars{StcodeGlobalVars::Type::REFERENCED_BY_SHADER};
  eastl::string shaderName;
  int curStvarId = 0;

  void reset() { *this = StcodeShader(); }

  void addStaticVarLocations(StcodeRegisters &&locations);
  void addCode(StcodeRoutine &&script, size_t routine_idx);
};

struct StcodeInterface
{
  static constexpr int ROUTINE_NOT_PRESENT = -1;

  struct RoutineData
  {
    eastl::string namespacedName{};
    int id = ROUTINE_NOT_PRESENT;
  };
  dag::Vector<RoutineData> routines;

  void addRoutine(int global_routine_idx, int local_routine_idx, const char *fn);
  void patchRoutineGlobalId(int prev_id, int new_id);
  eastl::string releaseAssembledCode();
};
