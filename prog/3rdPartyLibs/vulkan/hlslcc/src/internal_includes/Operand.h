#pragma once

#include "internal_includes/tokens.h"
#include <vector>
#include <memory>

enum{ MAX_SUB_OPERANDS = 3 };
class Operand;
class HLSLCrossCompilerContext;
struct Instruction;

#if _MSC_VER
// We want to disable the "array will be default-initialized" warning, as that's exactly what we want
#pragma warning(disable: 4351)
#endif

enum class RegisterSpace
{
  per_vertex = 0,
  per_patch = 1,
};

class Operand
{
public:
  typedef std::shared_ptr<Operand> SubOperandPtr;

  Operand() = default;
  // get swizzle applied to operand as value
  uint32_t GetSwizzleValue() const;
  // Retrieve the mask of all the components this operand accesses (either reads from or writes to).
  // Note that destination writemask does affect the effective access mask.
  uint32_t GetAccessMask() const;
  // Returns the index of the highest accessed component, based on component mask
  int GetMaxComponent() const;
  bool IsSwizzleReplicated() const;

  // Get the number of elements returned by operand, taking additional component mask into account
  //e.g.
  //.z = 1
  //.x = 1
  //.yw = 2
  uint32_t GetNumSwizzleElements(uint32_t ui32CompMask = OPERAND_4_COMPONENT_MASK_ALL) const;

  // When this operand is used as an input declaration, how many components does it have?
  int GetNumInputElements(const HLSLCrossCompilerContext *psContext) const;

  // Retrieve the operand data type.
  SHADER_VARIABLE_TYPE GetDataType(HLSLCrossCompilerContext* psContext, SHADER_VARIABLE_TYPE ePreferredTypeForImmediates = SVT_INT) const;

  RegisterSpace GetRegisterSpace(SHADER_TYPE eShaderType, SHADER_PHASE_TYPE eShaderPhaseType) const;

  // Maps REFLECT_RESOURCE_PRECISION into OPERAND_MIN_PRECISION as much as possible
  static OPERAND_MIN_PRECISION ResourcePrecisionToOperandPrecision(REFLECT_RESOURCE_PRECISION ePrec);

  int iExtended = 0;
  OPERAND_TYPE eType = OPERAND_TYPE_TEMP;
  OPERAND_MODIFIER eModifier = OPERAND_MODIFIER_NONE;
  OPERAND_MIN_PRECISION eMinPrecision = OPERAND_MIN_PRECISION_DEFAULT;
  int iIndexDims = 0;
  int iWriteMask = 0;
  int iGSInput = 0;
  int iPSInOut = 0;
  int iWriteMaskEnabled = 1;
  int iArrayElements = 0;
  int iNumComponents = 0;

  OPERAND_4_COMPONENT_SELECTION_MODE eSelMode = OPERAND_4_COMPONENT_MASK_MODE;
  uint32_t ui32CompMask = 0;
  uint32_t ui32Swizzle = 0;
  uint32_t aui32Swizzle[4] = {};

  uint32_t aui32ArraySizes[3] = {};
  uint32_t ui32RegisterNumber = 0;
  //If eType is OPERAND_TYPE_IMMEDIATE32
  float afImmediates[4] = {};
  //If eType is OPERAND_TYPE_IMMEDIATE64
  double adImmediates[4] = {};

  SPECIAL_NAME eSpecialName = NAME_UNDEFINED;
  std::string specialName;

  OPERAND_INDEX_REPRESENTATION eIndexRep[3] = { OPERAND_INDEX_IMMEDIATE32, OPERAND_INDEX_IMMEDIATE32, OPERAND_INDEX_IMMEDIATE32 };

  SubOperandPtr m_SubOperands[MAX_SUB_OPERANDS];

  //One type for each component.
  SHADER_VARIABLE_TYPE aeDataType[4] = { SVT_FLOAT, SVT_FLOAT, SVT_FLOAT, SVT_FLOAT };

  uint32_t m_Rebase = 0; // Rebase value, for constant array accesses.
  uint32_t m_Size = 0; // Component count, only for constant array access.

  struct Define
  {
    Define() = default;
    Define(const Define &) = default;
    Define(Instruction *inst, Operand *op) : m_Inst(inst), m_Op(op) {}

    Instruction *m_Inst = nullptr;  // Instruction that writes to the temp
    Operand     *m_Op = nullptr;    // The (destination) operand within that instruction.
  };

  std::vector<Define> m_Defines; // Array of instructions whose results this operand can use. (only if eType == OPERAND_TYPE_TEMP)
  uint32_t m_ForLoopInductorName = 0; // If non-zero, this (eType==OPERAND_TYPE_TEMP) is an inductor variable used in for loop, and it has a special number as given here (overrides ui32RegisterNumber)

#ifdef _DEBUG
  uint64_t id = 0;
#endif
};

