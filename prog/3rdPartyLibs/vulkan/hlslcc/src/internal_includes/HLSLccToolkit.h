#pragma once
#include "hlslcc.h"
#include "bstrlib.h"
#include <vector>
#include <string>

#include "internal_includes/Instruction.h"
#include "internal_includes/Operand.h"

class HLSLCrossCompilerContext;

namespace HLSLcc
{
  uint32_t GetNumberBitsSet(uint32_t a);

  uint32_t SVTTypeToFlag(const SHADER_VARIABLE_TYPE eType);

  SHADER_VARIABLE_TYPE TypeFlagsToSVTType(const uint32_t typeflags);

  const char * GetConstructorForType(const HLSLCrossCompilerContext *psContext, const SHADER_VARIABLE_TYPE eType, const int components, bool useGLSLPrecision = true);

  const char * GetConstructorForTypeGLSL(const SHADER_VARIABLE_TYPE eType,
    const int components, bool useGLSLPrecision);

  const char * GetConstructorForTypeMetal(const SHADER_VARIABLE_TYPE eType,
    const int components);

  std::string GetMatrixTypeName(const HLSLCrossCompilerContext *psContext, const SHADER_VARIABLE_TYPE eBaseType, const int columns, const int rows);

  void AddSwizzleUsingElementCount(bstring dest, uint32_t count);

  int WriteMaskToComponentCount(uint32_t writeMask);

  uint32_t BuildComponentMaskFromElementCount(int count);

  // Returns true if we can do direct assignment between types (mostly for mediump<->highp floats etc)
  bool DoAssignmentDataTypesMatch(SHADER_VARIABLE_TYPE dest, SHADER_VARIABLE_TYPE src);

  // Convert resource return type to SVT_ flags
  uint32_t ResourceReturnTypeToFlag(const RESOURCE_RETURN_TYPE eType);

  SHADER_VARIABLE_TYPE ResourceReturnTypeToSVTType(const RESOURCE_RETURN_TYPE eType, const REFLECT_RESOURCE_PRECISION ePrec);

  uint32_t ElemCountToAutoExpandFlag(uint32_t elemCount);

  bool IsOperationCommutative(int /* OPCODE_TYPE */ eOpCode);

  bool AreTempOperandsIdentical(const Operand * psA, const Operand * psB);

  int GetNumTextureDimensions(int /* RESOURCE_DIMENSION */ eResDim);

  SHADER_VARIABLE_TYPE SelectHigherType(SHADER_VARIABLE_TYPE a, SHADER_VARIABLE_TYPE b);

  // Returns true if the instruction adds 1 to the destination temp register
  bool IsAddOneInstruction(const Instruction *psInst);

  bool CanDoDirectCast(SHADER_VARIABLE_TYPE src, SHADER_VARIABLE_TYPE dest);

  // Helper function to print floats with full precision
  void PrintFloat(bstring b, float f);

  // Flags for ForeachOperand
  // Process suboperands
#define FEO_FLAG_SUBOPERAND 1
  // Process src operands
#define FEO_FLAG_SRC_OPERAND 2
  // Process destination operands
#define FEO_FLAG_DEST_OPERAND 4
  // Convenience: Process all operands, both src and dest, and all suboperands
#define FEO_FLAG_ALL (FEO_FLAG_SUBOPERAND | FEO_FLAG_SRC_OPERAND | FEO_FLAG_DEST_OPERAND)

  // For_each for all operands within a range of instructions. Flags above.
  template<typename ItrType, typename F>
  void ForEachOperand(ItrType _begin, ItrType _end, int flags, F callback)
  {
    ItrType inst = _begin;
    for (;inst != _end; ++inst)
    {
      if (flags & (FEO_FLAG_DEST_OPERAND | FEO_FLAG_SUBOPERAND))
      {
        for (uint32_t i = 0; i < inst->ui32FirstSrc; i++)
        {
          if (flags & FEO_FLAG_SUBOPERAND)
            for(auto&& sub : inst->asOperands[i].m_SubOperands)
              if (sub)
                callback(inst, sub.get(), FEO_FLAG_SUBOPERAND);

          if (flags & FEO_FLAG_DEST_OPERAND)
            callback(inst, &inst->asOperands[i], FEO_FLAG_DEST_OPERAND);
        }
      }

      if (flags & (FEO_FLAG_SRC_OPERAND | FEO_FLAG_SUBOPERAND))
      {
        for (uint32_t i = inst->ui32FirstSrc; i < inst->ui32NumOperands; i++)
        {
          if (flags & FEO_FLAG_SUBOPERAND)
            for (auto&& sub : inst->asOperands[i].m_SubOperands)
              if (sub)
                callback(inst, sub.get(), FEO_FLAG_SUBOPERAND);

          if (flags & FEO_FLAG_SRC_OPERAND)
            callback(inst, &inst->asOperands[i], FEO_FLAG_SRC_OPERAND);
        }
      }
    }
  }


};
