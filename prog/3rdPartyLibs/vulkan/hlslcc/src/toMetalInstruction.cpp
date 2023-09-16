#include "internal_includes/toMetal.h"
#include "internal_includes/HLSLccToolkit.h"
#include "internal_includes/languages.h"
#include "internal_includes/HLSLCrossCompilerContext.h"
#include "bstrlib.h"
#include "stdio.h"
#include <stdlib.h>
#include <algorithm>
#include "internal_includes/debug.h"
#include "internal_includes/Shader.h"
#include "internal_includes/Instruction.h"
#include "hlslcc.h"

using namespace HLSLcc;

bstring operator << (bstring a, const std::string &b)
{
	bcatcstr(a, b.c_str());
	return a;
}

static int AreAliasing(Operand* a, Operand* b)
{
    return (a->eType == b->eType && a->ui32RegisterNumber == b->ui32RegisterNumber);
}


void ToMetal::TranslateUDiv(Instruction* psInst,
                            int qoutient, int remainder, int divident, int divisor)
{
  int tempLocation = -1;
  if (AreAliasing(&psInst->asOperands[qoutient], &psInst->asOperands[divident]) || AreAliasing(&psInst->asOperands[qoutient], &psInst->asOperands[divisor]))
  {
    if (AreAliasing(&psInst->asOperands[remainder], &psInst->asOperands[divident]))
    {
       //CopyOperandToTemp(psInst, divident);
       tempLocation = divident;
    }
    else if (AreAliasing(&psInst->asOperands[remainder], &psInst->asOperands[divisor]))
    {
      //CopyOperandToTemp(psInst, divisor);
      tempLocation = divisor;
    }
    CallBinaryOp("%", psInst, remainder, divident, divisor, SVT_UINT);// , false, tempLocation);
    CallBinaryOp("/", psInst, qoutient, divident, divisor, SVT_UINT);// , false, tempLocation);
  }
  else
  {
    CallBinaryOp("/", psInst, qoutient, divident, divisor, SVT_UINT);// , false, tempLocation);
    CallBinaryOp("%", psInst, remainder, divident, divisor, SVT_UINT);// , false, tempLocation);
  }
}

// This function prints out the destination name, possible destination writemask, assignment operator
// and any possible conversions needed based on the eSrcType+ui32SrcElementCount (type and size of data expected to be coming in)
// As an output, pNeedsParenthesis will be filled with the amount of closing parenthesis needed
// and pSrcCount will be filled with the number of components expected
// ui32CompMask can be used to only write to 1 or more components (used by MOVC)
void ToMetal::AddOpAssignToDestWithMask(const Operand* psDest,
	SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, const char *szAssignmentOp, int *pNeedsParenthesis, uint32_t ui32CompMask)
{
	uint32_t ui32DestElementCount = psDest->GetNumSwizzleElements(ui32CompMask);
	bstring glsl = *psContext->currentGLSLString;
	SHADER_VARIABLE_TYPE eDestDataType = psDest->GetDataType(psContext);
	ASSERT(pNeedsParenthesis != NULL);

	*pNeedsParenthesis = 0;

	glsl << TranslateOperand(psDest, TO_FLAG_DESTINATION, ui32CompMask);

	// Simple path: types match.
	if (eDestDataType == eSrcType)
	{
		// Cover cases where the HLSL language expects the rest of the components to be default-filled
		// eg. MOV r0, c0.x => Temp[0] = vec4(c0.x);
		if (ui32DestElementCount > ui32SrcElementCount)
		{
			bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForTypeMetal(eDestDataType, ui32DestElementCount));
			*pNeedsParenthesis = 1;
		}
		else
			bformata(glsl, " %s ", szAssignmentOp);
		return;
	}
	// Up/downscaling with cast. The monster of condition there checks if the underlying datatypes are the same, just with prec differences
	if (((eDestDataType == SVT_FLOAT || eDestDataType == SVT_FLOAT16 || eDestDataType == SVT_FLOAT10) && (eSrcType == SVT_FLOAT || eSrcType == SVT_FLOAT16 || eSrcType == SVT_FLOAT10))
		|| ((eDestDataType == SVT_INT || eDestDataType == SVT_INT16 || eDestDataType == SVT_INT12) && (eSrcType == SVT_INT || eSrcType == SVT_INT16 || eSrcType == SVT_INT12))
		|| ((eDestDataType == SVT_UINT || eDestDataType == SVT_UINT16) && (eSrcType == SVT_UINT || eSrcType == SVT_UINT16)))
	{
		bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForTypeMetal(eDestDataType, ui32DestElementCount));
		*pNeedsParenthesis = 1;
		return;
	}

	switch (eDestDataType)
	{
	case SVT_INT:
	case SVT_INT12:
	case SVT_INT16:
		// Bitcasts from lower precisions are ambiguous
		ASSERT(eSrcType != SVT_FLOAT10 && eSrcType != SVT_FLOAT16);
		if (eSrcType == SVT_FLOAT)
		{
			if(ui32DestElementCount > 1)
				bformata(glsl, " %s int%d(", szAssignmentOp, ui32DestElementCount);
			else
				bformata(glsl, " %s int(", szAssignmentOp);

			// Cover cases where the HLSL language expects the rest of the components to be default-filled
			if (ui32DestElementCount > ui32SrcElementCount)
			{
				bformata(glsl, "%s(", GetConstructorForTypeMetal(eSrcType, ui32DestElementCount));
				(*pNeedsParenthesis)++;
			}
		}
		else
			bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForTypeMetal(eDestDataType, ui32DestElementCount));

		(*pNeedsParenthesis)++;
		break;
	case SVT_UINT:
	case SVT_UINT16:
		ASSERT(eSrcType != SVT_FLOAT10 && eSrcType != SVT_FLOAT16);
		if (eSrcType == SVT_FLOAT)
		{
			if (ui32DestElementCount > 1)
				bformata(glsl, " %s uint%d(", szAssignmentOp, ui32DestElementCount);
			else
				bformata(glsl, " %s uint(", szAssignmentOp);
			// Cover cases where the HLSL language expects the rest of the components to be default-filled
			if (ui32DestElementCount > ui32SrcElementCount)
			{
				bformata(glsl, "%s(", GetConstructorForTypeMetal(eSrcType, ui32DestElementCount));
				(*pNeedsParenthesis)++;
			}
		}
		else
			bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForTypeMetal(eDestDataType, ui32DestElementCount));

		(*pNeedsParenthesis)++;
		break;

	case SVT_FLOAT:
	case SVT_FLOAT10:
	case SVT_FLOAT16:
		ASSERT(eSrcType != SVT_INT12 || (eSrcType != SVT_INT16 && eSrcType != SVT_UINT16));
		if (psContext->psShader.ui32MajorVersion > 3)
		{
			if (ui32DestElementCount > 1)
				bformata(glsl, " %s float%d(", szAssignmentOp, ui32DestElementCount);
			else
				bformata(glsl, " %s float(", szAssignmentOp);
			// Cover cases where the HLSL language expects the rest of the components to be default-filled
			if (ui32DestElementCount > ui32SrcElementCount)
			{
				bformata(glsl, "%s(", GetConstructorForTypeMetal(eSrcType, ui32DestElementCount));
				(*pNeedsParenthesis)++;
			}
		}
		else
			bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForTypeMetal(eDestDataType, ui32DestElementCount));

		(*pNeedsParenthesis)++;
		break;
	default:
		// TODO: Handle bools?
		ASSERT(0);
		break;
	}
	return;
}

void ToMetal::AddAssignToDest(const Operand* psDest,
	SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, int* pNeedsParenthesis)
{
	AddOpAssignToDestWithMask(psDest, eSrcType, ui32SrcElementCount, "=", pNeedsParenthesis, OPERAND_4_COMPONENT_MASK_ALL);
}

void ToMetal::AddAssignPrologue(int numParenthesis)
{
	bstring glsl = *psContext->currentGLSLString;
	while (numParenthesis != 0)
	{
		bcatcstr(glsl, ")");
		numParenthesis--;
	}
	bcatcstr(glsl, ";\n");

}

void ToMetal::AddComparison(Instruction* psInst, ComparisonType eType,
	uint32_t typeFlag)
{
	// Multiple cases to consider here:
	// OPCODE_LT, _GT, _NE etc: inputs are floats, outputs UINT 0xffffffff or 0. typeflag: TO_FLAG_NONE
	// OPCODE_ILT, _IGT etc: comparisons are signed ints, outputs UINT 0xffffffff or 0 typeflag TO_FLAG_INTEGER
	// _ULT, UGT etc: inputs unsigned ints, outputs UINTs typeflag TO_FLAG_UNSIGNED_INTEGER
	//


	bstring glsl = *psContext->currentGLSLString;
	const uint32_t destElemCount = psInst->asOperands[0].GetNumSwizzleElements();
	const uint32_t s0ElemCount = psInst->asOperands[1].GetNumSwizzleElements();
	const uint32_t s1ElemCount = psInst->asOperands[2].GetNumSwizzleElements();
	int isBoolDest = psInst->asOperands[0].GetDataType(psContext) == SVT_BOOL;
	const uint32_t destMask = psInst->asOperands[0].GetAccessMask();

	int needsParenthesis = 0;
	if (typeFlag == TO_FLAG_NONE
		&& psInst->asOperands[1].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[1].GetDataType(psContext) == SVT_FLOAT16)
		typeFlag = TO_FLAG_FORCE_HALF;
	ASSERT(s0ElemCount == s1ElemCount || s1ElemCount == 1 || s0ElemCount == 1);
	if ((s0ElemCount != s1ElemCount) && (destElemCount > 1))
	{
		// Set the proper auto-expand flag is either argument is scalar
		typeFlag |= (TO_AUTO_EXPAND_TO_VEC2 << (std::min(std::max(s0ElemCount, s1ElemCount), destElemCount) - 2));
	}
	if (destElemCount > 1)
	{
		const char* glslOpcode[] = {
			"==",
			"<",
			">=",
			"!=",
		};
		psContext->AddIndentation();
		if (isBoolDest)
		{
			glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION | TO_FLAG_BOOL);
			bcatcstr(glsl, " = ");
		}
		else
		{
			AddAssignToDest(&psInst->asOperands[0], SVT_UINT, destElemCount, &needsParenthesis);

			bcatcstr(glsl, GetConstructorForTypeMetal(SVT_UINT, destElemCount));
			bcatcstr(glsl, "(");
		}
		bcatcstr(glsl, "(");
		glsl << TranslateOperand(&psInst->asOperands[1], typeFlag, destMask);
		bformata(glsl, "%s", glslOpcode[eType]);
		glsl << TranslateOperand(&psInst->asOperands[2], typeFlag, destMask);
		bcatcstr(glsl, ")");
		if (!isBoolDest)
		{
			bcatcstr(glsl, ")");
			bcatcstr(glsl, " * 0xFFFFFFFFu");
		}

		AddAssignPrologue(needsParenthesis);
	}
	else
	{
		const char* glslOpcode[] = {
			"==",
			"<",
			">=",
			"!=",
		};

		//Scalar compare

		psContext->AddIndentation();
		if (isBoolDest)
		{
			glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION | TO_FLAG_BOOL);
			bcatcstr(glsl, " = ");
		}
		else
		{
			AddAssignToDest(&psInst->asOperands[0], SVT_UINT, destElemCount, &needsParenthesis);
			bcatcstr(glsl, "(");
		}
		glsl << TranslateOperand(&psInst->asOperands[1], typeFlag, destMask);
		bformata(glsl, "%s", glslOpcode[eType]);
		glsl << TranslateOperand(&psInst->asOperands[2], typeFlag, destMask);
		if (!isBoolDest)
		{
			bcatcstr(glsl, ") ? 0xFFFFFFFFu : 0u"); 
		}
		AddAssignPrologue(needsParenthesis);
	}
}


void ToMetal::AddMOVBinaryOp(const Operand *pDest, Operand *pSrc)
{
	bstring glsl = *psContext->currentGLSLString;
	int numParenthesis = 0;
	int srcSwizzleCount = pSrc->GetNumSwizzleElements();
	uint32_t writeMask = pDest->GetAccessMask();

	const SHADER_VARIABLE_TYPE eSrcType = pSrc->GetDataType(psContext, pDest->GetDataType(psContext));
	uint32_t flags = SVTTypeToFlag(eSrcType);

	AddAssignToDest(pDest, eSrcType, srcSwizzleCount, &numParenthesis);
	glsl << TranslateOperand(pSrc, flags, writeMask);

	AddAssignPrologue(numParenthesis);
}

void ToMetal::AddMOVCBinaryOp(const Operand *pDest, const Operand *src0, Operand *src1, Operand *src2)
{
	bstring glsl = *psContext->currentGLSLString;
	uint32_t destElemCount = pDest->GetNumSwizzleElements();
	uint32_t s0ElemCount = src0->GetNumSwizzleElements();
	uint32_t s1ElemCount = src1->GetNumSwizzleElements();
	uint32_t s2ElemCount = src2->GetNumSwizzleElements();
	uint32_t destWriteMask = pDest->GetAccessMask();
	uint32_t destElem;

	const SHADER_VARIABLE_TYPE eDestType = pDest->GetDataType(psContext);
	/*
	for each component in dest[.mask]
	if the corresponding component in src0 (POS-swizzle)
	has any bit set
	{
	copy this component (POS-swizzle) from src1 into dest
	}
	else
	{
	copy this component (POS-swizzle) from src2 into dest
	}
	endfor
	*/

	/* Single-component conditional variable (src0) */
	if (s0ElemCount == 1 || src0->IsSwizzleReplicated())
	{
		int numParenthesis = 0;
		SHADER_VARIABLE_TYPE s0Type = src0->GetDataType(psContext);
		psContext->AddIndentation();
		AddAssignToDest(pDest, eDestType, destElemCount, &numParenthesis);
		bcatcstr(glsl, "(");
		if (s0Type == SVT_UINT || s0Type == SVT_UINT16)
			glsl << TranslateOperand(src0, TO_AUTO_BITCAST_TO_UINT, OPERAND_4_COMPONENT_MASK_X);
		else if (s0Type == SVT_BOOL)
			glsl << TranslateOperand(src0, TO_FLAG_BOOL, OPERAND_4_COMPONENT_MASK_X);
		else
			glsl << TranslateOperand(src0, TO_AUTO_BITCAST_TO_INT, OPERAND_4_COMPONENT_MASK_X);

		if (psContext->psShader.ui32MajorVersion < 4)
		{
			//cmp opcode uses >= 0
			bcatcstr(glsl, " >= 0) ? ");
		}
		else
		{
			if (s0Type == SVT_UINT || s0Type == SVT_UINT16)
				bcatcstr(glsl, " != 0u) ? ");
			else if (s0Type == SVT_BOOL)
				bcatcstr(glsl, ") ? ");
			else
				bcatcstr(glsl, " != 0) ? ");
		}

		if (s1ElemCount == 1 && destElemCount > 1)
			glsl << TranslateOperand(src1, SVTTypeToFlag(eDestType) | ElemCountToAutoExpandFlag(destElemCount));
		else
			glsl << TranslateOperand(src1, SVTTypeToFlag(eDestType), destWriteMask);

		bcatcstr(glsl, " : ");
		if (s2ElemCount == 1 && destElemCount > 1)
			glsl << TranslateOperand(src2, SVTTypeToFlag(eDestType) | ElemCountToAutoExpandFlag(destElemCount));
		else
			glsl << TranslateOperand(src2, SVTTypeToFlag(eDestType), destWriteMask);

		AddAssignPrologue(numParenthesis);
	}
	else
	{
		// TODO: We can actually do this in one op using mix().
		int srcElem = -1;
		SHADER_VARIABLE_TYPE s0Type = src0->GetDataType(psContext);
		for (destElem = 0; destElem < 4; ++destElem)
		{
			int numParenthesis = 0;
			srcElem++;
			if (pDest->eSelMode == OPERAND_4_COMPONENT_MASK_MODE && pDest->ui32CompMask != 0 && !(pDest->ui32CompMask & (1 << destElem)))
				continue;

			psContext->AddIndentation();
			AddOpAssignToDestWithMask(pDest, eDestType, 1, "=", &numParenthesis, 1 << destElem);
			bcatcstr(glsl, "(");
			if (s0Type == SVT_BOOL)
			{
				glsl << TranslateOperand(src0, TO_FLAG_BOOL, 1 << srcElem);
				bcatcstr(glsl, ") ? ");
			}
			else
			{
				glsl << TranslateOperand(src0, TO_AUTO_BITCAST_TO_INT, 1 << srcElem);

				if (psContext->psShader.ui32MajorVersion < 4)
				{
					//cmp opcode uses >= 0
					bcatcstr(glsl, " >= 0) ? ");
				}
				else
				{
					bcatcstr(glsl, " != 0) ? ");
				}
			}

			glsl << TranslateOperand(src1, SVTTypeToFlag(eDestType), 1 << srcElem);
			bcatcstr(glsl, " : ");
			glsl << TranslateOperand(src2, SVTTypeToFlag(eDestType), 1 << srcElem);

			AddAssignPrologue(numParenthesis);
		}
	}
}

void ToMetal::CallBinaryOp(const char* name, Instruction* psInst,
	int dest, int src0, int src1, SHADER_VARIABLE_TYPE eDataType)
{
	uint32_t ui32Flags = SVTTypeToFlag(eDataType);
	bstring glsl = *psContext->currentGLSLString;
	uint32_t destMask = psInst->asOperands[dest].GetAccessMask();
	uint32_t src1SwizCount = psInst->asOperands[src1].GetNumSwizzleElements(destMask);
	uint32_t src0SwizCount = psInst->asOperands[src0].GetNumSwizzleElements(destMask);
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();
	int needsParenthesis = 0;

	if (eDataType == SVT_FLOAT
		&& psInst->asOperands[dest].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src0].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src1].GetDataType(psContext) == SVT_FLOAT16)
		ui32Flags = TO_FLAG_FORCE_HALF;

	uint32_t maxElems = std::max(src1SwizCount, src0SwizCount);
	if (src1SwizCount != src0SwizCount)
	{
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], eDataType, dstSwizCount, &needsParenthesis);

/*	bool s0NeedsUpscaling = false, s1NeedsUpscaling = false;
	SHADER_VARIABLE_TYPE s0Type = psInst->asOperands[src0].GetDataType(psContext);
	SHADER_VARIABLE_TYPE s1Type = psInst->asOperands[src1].GetDataType(psContext);

	if((s0Type == SVT_FLOAT10 || s0Type == SVT_FLOAT16) && (s1Type != s)
	*/
	glsl << TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bformata(glsl, " %s ", name);
	glsl << TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);

	AddAssignPrologue(needsParenthesis);
}



void ToMetal::CallTernaryOp(const char* op1, const char* op2, Instruction* psInst,
	int dest, int src0, int src1, int src2, uint32_t dataType)
{
	bstring glsl = *psContext->currentGLSLString;
	uint32_t destMask = psInst->asOperands[dest].GetAccessMask();
	uint32_t src2SwizCount = psInst->asOperands[src2].GetNumSwizzleElements(destMask);
	uint32_t src1SwizCount = psInst->asOperands[src1].GetNumSwizzleElements(destMask);
	uint32_t src0SwizCount = psInst->asOperands[src0].GetNumSwizzleElements(destMask);
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();

	uint32_t ui32Flags = dataType;
	int numParenthesis = 0;

	if (dataType == TO_FLAG_NONE
		&& psInst->asOperands[dest].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src0].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src1].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src2].GetDataType(psContext) == SVT_FLOAT16)
		ui32Flags = TO_FLAG_FORCE_HALF;

	if (src1SwizCount != src0SwizCount || src2SwizCount != src0SwizCount)
	{
		uint32_t maxElems = std::max(src2SwizCount, std::max(src1SwizCount, src0SwizCount));
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], TypeFlagsToSVTType(dataType), dstSwizCount, &numParenthesis);

	glsl << TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bformata(glsl, " %s ", op1);
	glsl << TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);
	bformata(glsl, " %s ", op2);
	glsl << TranslateOperand(&psInst->asOperands[src2], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToMetal::CallHelper3(const char* name, Instruction* psInst,
	int dest, int src0, int src1, int src2, int paramsShouldFollowWriteMask)
{
	uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT;
	bstring glsl = *psContext->currentGLSLString;
	uint32_t destMask = paramsShouldFollowWriteMask ? psInst->asOperands[dest].GetAccessMask() : OPERAND_4_COMPONENT_MASK_ALL;
	uint32_t src2SwizCount = psInst->asOperands[src2].GetNumSwizzleElements(destMask);
	uint32_t src1SwizCount = psInst->asOperands[src1].GetNumSwizzleElements(destMask);
	uint32_t src0SwizCount = psInst->asOperands[src0].GetNumSwizzleElements(destMask);
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();
	int numParenthesis = 0;

	if (psInst->asOperands[dest].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src0].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src1].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src2].GetDataType(psContext) == SVT_FLOAT16)
		ui32Flags = TO_FLAG_FORCE_HALF | TO_AUTO_BITCAST_TO_FLOAT;

	if ((src1SwizCount != src0SwizCount || src2SwizCount != src0SwizCount) && paramsShouldFollowWriteMask)
	{
		uint32_t maxElems = std::max(src2SwizCount, std::max(src1SwizCount, src0SwizCount));
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], ui32Flags & TO_FLAG_FORCE_HALF ? SVT_FLOAT16 : SVT_FLOAT, dstSwizCount, &numParenthesis);

	bformata(glsl, "%s(", name);
	numParenthesis++;
	glsl << TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	glsl << TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	glsl << TranslateOperand(&psInst->asOperands[src2], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToMetal::CallHelper2(const char* name, Instruction* psInst,
	int dest, int src0, int src1, int paramsShouldFollowWriteMask)
{
	uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT;
	bstring glsl = *psContext->currentGLSLString;
	uint32_t destMask = paramsShouldFollowWriteMask ? psInst->asOperands[dest].GetAccessMask() : OPERAND_4_COMPONENT_MASK_ALL;
	uint32_t src1SwizCount = psInst->asOperands[src1].GetNumSwizzleElements(destMask);
	uint32_t src0SwizCount = psInst->asOperands[src0].GetNumSwizzleElements(destMask);
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();

	int isDotProduct = (strncmp(name, "dot", 3) == 0) ? 1 : 0;
	int numParenthesis = 0;

	if (psInst->asOperands[dest].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src0].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src1].GetDataType(psContext) == SVT_FLOAT16)
		ui32Flags = TO_FLAG_FORCE_HALF | TO_AUTO_BITCAST_TO_FLOAT;


	if ((src1SwizCount != src0SwizCount) && paramsShouldFollowWriteMask)
	{
		uint32_t maxElems = std::max(src1SwizCount, src0SwizCount);
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	psContext->AddIndentation();
	AddAssignToDest(&psInst->asOperands[dest], ui32Flags & TO_FLAG_FORCE_HALF ? SVT_FLOAT16 : SVT_FLOAT, isDotProduct ? 1 : dstSwizCount, &numParenthesis);

	bformata(glsl, "%s(", name);
	numParenthesis++;

	glsl << TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	glsl << TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);

	AddAssignPrologue(numParenthesis);
}

void ToMetal::CallHelper2Int(const char* name, Instruction* psInst,
	int dest, int src0, int src1, int paramsShouldFollowWriteMask)
{
	uint32_t ui32Flags = TO_AUTO_BITCAST_TO_INT;
	bstring glsl = *psContext->currentGLSLString;
	uint32_t destMask = paramsShouldFollowWriteMask ? psInst->asOperands[dest].GetAccessMask() : OPERAND_4_COMPONENT_MASK_ALL;
	uint32_t src1SwizCount = psInst->asOperands[src1].GetNumSwizzleElements(destMask);
	uint32_t src0SwizCount = psInst->asOperands[src0].GetNumSwizzleElements(destMask);
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();
	int numParenthesis = 0;

	if ((src1SwizCount != src0SwizCount) && paramsShouldFollowWriteMask)
	{
		uint32_t maxElems = std::max(src1SwizCount, src0SwizCount);
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], SVT_INT, dstSwizCount, &numParenthesis);

	bformata(glsl, "%s(", name);
	numParenthesis++;
	glsl << TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	glsl << TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToMetal::CallHelper2UInt(const char* name, Instruction* psInst,
	int dest, int src0, int src1, int paramsShouldFollowWriteMask)
{
	uint32_t ui32Flags = TO_AUTO_BITCAST_TO_UINT;
	bstring glsl = *psContext->currentGLSLString;
	uint32_t destMask = paramsShouldFollowWriteMask ? psInst->asOperands[dest].GetAccessMask() : OPERAND_4_COMPONENT_MASK_ALL;
	uint32_t src1SwizCount = psInst->asOperands[src1].GetNumSwizzleElements(destMask);
	uint32_t src0SwizCount = psInst->asOperands[src0].GetNumSwizzleElements(destMask);
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();
	int numParenthesis = 0;

	if ((src1SwizCount != src0SwizCount) && paramsShouldFollowWriteMask)
	{
		uint32_t maxElems = std::max(src1SwizCount, src0SwizCount);
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], SVT_UINT, dstSwizCount, &numParenthesis);

	bformata(glsl, "%s(", name);
	numParenthesis++;
	glsl << TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	glsl << TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToMetal::CallHelper1(const char* name, Instruction* psInst,
	int dest, int src0, int paramsShouldFollowWriteMask)
{
	uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT;
	bstring glsl = *psContext->currentGLSLString;
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();
	uint32_t destMask = paramsShouldFollowWriteMask ? psInst->asOperands[dest].GetAccessMask() : OPERAND_4_COMPONENT_MASK_ALL;
	int numParenthesis = 0;

	psContext->AddIndentation();
	if (psInst->asOperands[dest].GetDataType(psContext) == SVT_FLOAT16
		&& psInst->asOperands[src0].GetDataType(psContext) == SVT_FLOAT16)
		ui32Flags = TO_FLAG_FORCE_HALF | TO_AUTO_BITCAST_TO_FLOAT;

	AddAssignToDest(&psInst->asOperands[dest], ui32Flags & TO_FLAG_FORCE_HALF ? SVT_FLOAT16 : SVT_FLOAT, dstSwizCount, &numParenthesis);

	bformata(glsl, "%s(", name);
	numParenthesis++;
	glsl << TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

//Result is an int.
void ToMetal::CallHelper1Int(
	const char* name,
	Instruction* psInst,
	const int dest,
	const int src0,
	int paramsShouldFollowWriteMask)
{
	uint32_t ui32Flags = TO_AUTO_BITCAST_TO_INT;
	bstring glsl = *psContext->currentGLSLString;
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();
	uint32_t destMask = paramsShouldFollowWriteMask ? psInst->asOperands[dest].GetAccessMask() : OPERAND_4_COMPONENT_MASK_ALL;
	int numParenthesis = 0;

	psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], SVT_INT, dstSwizCount, &numParenthesis);

	bformata(glsl, "%s(", name);
	numParenthesis++;
	glsl << TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToMetal::TranslateTexelFetch(
	Instruction* psInst,
	const ResourceBinding* psBinding,
	bstring glsl)
{
	int numParenthesis = 0;
	uint32_t destCount = psInst->asOperands[0].GetNumSwizzleElements();
	psContext->AddIndentation();
	AddAssignToDest(&psInst->asOperands[0], psContext->psShader.sInfo.GetTextureDataType(psInst->asOperands[2].ui32RegisterNumber), 4, &numParenthesis);
	glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
	bcatcstr(glsl, ".read(");

	switch (psBinding->eDimension)
	{
		case REFLECT_RESOURCE_DIMENSION_BUFFER:
		{
			psContext->m_Reflection.OnDiagnostics("Buffer resources not supported in Metal (in texel fetch)", 0, true);
			return;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X);
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X);
			bcatcstr(glsl, ", ");
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_W);
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
			bcatcstr(glsl, ", ");
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_W); // Lod level
			break;
		}

		case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
			bcatcstr(glsl, ", ");
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_Z); // Array index
			bcatcstr(glsl, ", ");
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_W); // Lod level
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
			bcatcstr(glsl, ", ");
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_W); // Lod level
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
			bcatcstr(glsl, ", ");
			glsl << TranslateOperand(&psInst->asOperands[3], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X); // Sample index
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
		case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
		case REFLECT_RESOURCE_DIMENSION_BUFFEREX:
		default:
		{
			// Shouldn't happen. Cubemap reads are not supported in HLSL
			ASSERT(0);
			break;
		}
	}
	bcatcstr(glsl, ")");

	AddSwizzleUsingElementCount(glsl, destCount);
	AddAssignPrologue(numParenthesis);
}

void ToMetal::TranslateTexelFetchOffset(
	Instruction* psInst,
	const ResourceBinding* psBinding,
	bstring glsl)
{
	int numParenthesis = 0;
	uint32_t destCount = psInst->asOperands[0].GetNumSwizzleElements();
	psContext->AddIndentation();
	AddAssignToDest(&psInst->asOperands[0], psContext->psShader.sInfo.GetTextureDataType(psInst->asOperands[2].ui32RegisterNumber), 4, &numParenthesis);

	glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
	bcatcstr(glsl, ".read(");

	switch (psBinding->eDimension)
	{
		case REFLECT_RESOURCE_DIMENSION_BUFFER:
		{
			psContext->m_Reflection.OnDiagnostics("Buffer resources not supported in Metal (in texel fetch)", 0, true);
			return;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
		{
			psContext->m_Reflection.OnDiagnostics("Multisampled texture arrays not supported in Metal (in texel fetch)", 0, true);
			return;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X);
			bformata(glsl, " + %d", psInst->iUAddrOffset);
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X);
			bformata(glsl, " + %d, ", psInst->iUAddrOffset);

			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_Y);
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
			bformata(glsl, "+ uint2(%d, %d), ", psInst->iUAddrOffset, psInst->iVAddrOffset);
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_W); // Lod level
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
			bformata(glsl, "+ uint2(%d, %d), ", psInst->iUAddrOffset, psInst->iVAddrOffset);
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_Z); // Array index
			bcatcstr(glsl, ", ");
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_W); // Lod level
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
			bformata(glsl, "+ uint3(%d, %d, %d), ", psInst->iUAddrOffset, psInst->iVAddrOffset, psInst->iWAddrOffset);
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_W); // Lod level
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
		{
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
			bformata(glsl, "+ uint2(%d, %d), ", psInst->iUAddrOffset, psInst->iVAddrOffset);
			glsl << TranslateOperand(&psInst->asOperands[3], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X); // Sample index
			break;
		}
		case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
		case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
		case REFLECT_RESOURCE_DIMENSION_BUFFEREX:
		default:
		{
			// Shouldn't happen. Cubemap reads are not supported in HLSL
			ASSERT(0);
			break;
		}
	}
	bcatcstr(glsl, ")");

	AddSwizzleUsingElementCount(glsl, destCount);
	AddAssignPrologue(numParenthesis);
}


//Makes sure the texture coordinate swizzle is appropriate for the texture type.
//i.e. vecX for X-dimension texture.
//Currently supports floating point coord only, so not used for texelFetch.
void ToMetal::TranslateTexCoord(
	const RESOURCE_DIMENSION eResDim,
	Operand* psTexCoordOperand)
{
	uint32_t flags = TO_AUTO_BITCAST_TO_FLOAT;
	uint32_t opMask = OPERAND_4_COMPONENT_MASK_ALL;

	switch (eResDim)
	{
	case RESOURCE_DIMENSION_TEXTURE1D:
	{
		//Vec1 texcoord. Mask out the other components.
		opMask = OPERAND_4_COMPONENT_MASK_X;
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE2D:
	case RESOURCE_DIMENSION_TEXTURE1DARRAY:
	{
		//Vec2 texcoord. Mask out the other components.
		opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
		flags |= TO_AUTO_EXPAND_TO_VEC2;
		break;
	}
	case RESOURCE_DIMENSION_TEXTURECUBE:
	case RESOURCE_DIMENSION_TEXTURE3D:
	{
		//Vec3 texcoord. Mask out the other components.
		opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z;
		flags |= TO_AUTO_EXPAND_TO_VEC3;
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE2DARRAY:
	{
		opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
		flags |= TO_AUTO_EXPAND_TO_VEC2;
		
		bstring glsl = *psContext->currentGLSLString;
		glsl << TranslateOperand(psTexCoordOperand, flags, opMask);
		
		bcatcstr(glsl, ", ");

		opMask = OPERAND_4_COMPONENT_MASK_Z;
		flags = TO_AUTO_BITCAST_TO_FLOAT;

		break;
	}
	case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
	{
		flags |= TO_AUTO_EXPAND_TO_VEC4;
		break;
	}
	default:
	{
		ASSERT(0);
		break;
	}
	}

	//FIXME detect when integer coords are needed.
	bstring glsl = *psContext->currentGLSLString;
	glsl << TranslateOperand(psTexCoordOperand, flags, opMask);
}

void ToMetal::GetResInfoData(Instruction* psInst, int index, int destElem)
{
	bstring glsl = *psContext->currentGLSLString;
	int numParenthesis = 0;
	const RESINFO_RETURN_TYPE eResInfoReturnType = psInst->eResInfoReturnType;
	const int isUAV = (psInst->asOperands[2].eType == OPERAND_TYPE_UNORDERED_ACCESS_VIEW);

  const ResourceBinding* binding = nullptr;
  psContext->psShader.sInfo.GetResourceFromBindingPoint(isUAV ? RGROUP_UAV : RGROUP_TEXTURE,
                                                         psInst->asOperands[2].ui32RegisterNumber,
                                                         &binding);

	psContext->AddIndentation();
	AddOpAssignToDestWithMask(&psInst->asOperands[0], eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT ? SVT_UINT : SVT_FLOAT, 1, "=", &numParenthesis, 1 << destElem);

	const char *metalGetters[] = { ".get_width()", ".get_height()", ".get_depth()", ".get_num_mip_levels()" };
  int dim = 0;
  if (binding)
  {
    switch (binding->eDimension)
    {
      case REFLECT_RESOURCE_DIMENSION_UNKNOWN:
      case REFLECT_RESOURCE_DIMENSION_BUFFER:
      case REFLECT_RESOURCE_DIMENSION_BUFFEREX:
          break;
      case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
          dim = 1;
          break;
      case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
      case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
      case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
      case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
          dim = 2;
          break;
      case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
      case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
      case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
      case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
          dim = 3;
          break;
    }
  }
  else
  {
    bformata(glsl, "#error #GetResInfoData# lookup failed for %u (%u)", psInst->asOperands[2].ui32RegisterNumber, isUAV);
  }

	if (dim < (index + 1) && index != 3)
	{
		bcatcstr(glsl, eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT ? "0u" : "0.0");
	}
	else
	{
		if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_FLOAT)
		{
			bcatcstr(glsl, "float(");
			numParenthesis++;
		}
		else if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_RCPFLOAT)
		{
			bcatcstr(glsl, "1.0f / float(");
			numParenthesis++;
		}
		glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
		if (index == 2 &&
			(psInst->eResDim == RESOURCE_DIMENSION_TEXTURE1DARRAY ||
				psInst->eResDim == RESOURCE_DIMENSION_TEXTURE2DARRAY ||
				psInst->eResDim == RESOURCE_DIMENSION_TEXTURE2DMSARRAY))
		{
			bcatcstr(glsl, ".get_array_size()");
		}
		else
			bcatcstr(glsl, metalGetters[index]);

		// TODO Metal has no way to query for info on lower mip levels, now always returns info for highest.
	}
	AddAssignPrologue(numParenthesis);
}

void ToMetal::TranslateTextureSample(Instruction* psInst,
	uint32_t ui32Flags)
{
	bstring glsl = *psContext->currentGLSLString;
	int numParenthesis = 0;
	int hasParamOffset = (ui32Flags & TEXSMP_FLAG_PARAMOFFSET) ? 1 : 0;

	Operand* psDest = &psInst->asOperands[0];
	Operand* psDestAddr = &psInst->asOperands[1];
	Operand* psSrcOff = (ui32Flags & TEXSMP_FLAG_PARAMOFFSET) ? &psInst->asOperands[2] : 0;
	Operand* psSrcTex = &psInst->asOperands[2 + hasParamOffset];
	Operand* psSrcSamp = &psInst->asOperands[3 + hasParamOffset];
	Operand* psSrcRef = (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE) ? &psInst->asOperands[4 + hasParamOffset] : 0;
	Operand* psSrcLOD = (ui32Flags & TEXSMP_FLAG_LOD) ? &psInst->asOperands[4] : 0;
	Operand* psSrcDx = (ui32Flags & TEXSMP_FLAG_GRAD) ? &psInst->asOperands[4] : 0;
	Operand* psSrcDy = (ui32Flags & TEXSMP_FLAG_GRAD) ? &psInst->asOperands[5] : 0;
	Operand* psSrcBias = (ui32Flags & TEXSMP_FLAG_BIAS) ? &psInst->asOperands[4] : 0;

	const char *funcName = "";
	const char* offset = "";
	const char* gradSwizzle = "";
	const char *gradientName = "";

	uint32_t ui32NumOffsets = 0;

	const RESOURCE_DIMENSION eResDim = psContext->psShader.aeResourceDims[psSrcTex->ui32RegisterNumber];

	if (ui32Flags & TEXSMP_FLAG_GATHER)
	{
		if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
			funcName = "gather_compare";
		else
			funcName = "gather";
	}
	else
	{
		if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
			funcName = "sample_compare";
		else
			funcName = "sample";
	}

	switch (eResDim)
	{
	case RESOURCE_DIMENSION_TEXTURE1D:
	{
		gradSwizzle = ".x";
		ui32NumOffsets = 1;
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE2D:
	{
		gradSwizzle = ".xy";
		gradientName = "gradient2d";
		ui32NumOffsets = 2;
		break;
	}
	case RESOURCE_DIMENSION_TEXTURECUBE:
	{
		gradSwizzle = ".xyz";
		ui32NumOffsets = 3;
		gradientName = "gradientcube";
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE3D:
	{
		gradSwizzle = ".xyz";
		ui32NumOffsets = 3;
		gradientName = "gradient3d";
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE1DARRAY:
	{
		gradSwizzle = ".x";
		ui32NumOffsets = 1;
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE2DARRAY:
	{
		gradSwizzle = ".xy";
		ui32NumOffsets = 2;
		gradientName = "gradient2d";
		break;
	}
	case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
	{
		gradSwizzle = ".xyz";
		ui32NumOffsets = 3;
		gradientName = "gradientcube";
		break;
	}
	default:
	{
		ASSERT(0);
		break;
	}
	}


	SHADER_VARIABLE_TYPE dataType = psContext->psShader.sInfo.GetTextureDataType(psSrcTex->ui32RegisterNumber);
	psContext->AddIndentation();
	AddAssignToDest(psDest, dataType, psSrcTex->GetNumSwizzleElements(), &numParenthesis);
		
	std::string texName = TranslateOperand(psSrcTex, TO_FLAG_NAME_ONLY);

	// TextureName.FuncName(
	glsl << texName;
	bformata(glsl, ".%s(", funcName);

	// Sampler name
	//TODO: Is it ok to use fixed shadow sampler in all cases of depth compare or would we need more
	// accurate way of detecting shadow cases (atm all depth compares are interpreted as shadow usage)
	if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
	{
    bcatcstr(glsl, texName.c_str());
    bcatcstr(glsl, "_samplerstate");
	}
	else
	{
		std::string sampName = TranslateOperand(psSrcSamp, TO_FLAG_NAME_ONLY);

		// insert the "sampler" prefix if the sampler name is equal to the texture name (default sampler)
		if (texName == sampName)
			sampName.insert(0, "sampler");
		glsl << sampName;
	}

	bcatcstr(glsl, ", ");

	// Texture coordinates
	TranslateTexCoord(eResDim, psDestAddr);

	// Depth compare reference value
	if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
	{
		bcatcstr(glsl, ", saturate("); // TODO: why the saturate here?
		glsl << TranslateOperand(psSrcRef, TO_AUTO_BITCAST_TO_FLOAT);
		bcatcstr(glsl, ")");
	}

	// lod_options (LOD/grad/bias) based on the flags
	if (ui32Flags & TEXSMP_FLAG_LOD)
	{
		bcatcstr(glsl, ", level(");
		glsl << TranslateOperand(psSrcLOD, TO_AUTO_BITCAST_TO_FLOAT);
		if (psContext->psShader.ui32MajorVersion < 4)
		{
			bcatcstr(glsl, ".w");
		}
		bcatcstr(glsl, ")");
	}
	else if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
	{
		bcatcstr(glsl, ", level(0.0)");
	}
	else if (ui32Flags & TEXSMP_FLAG_GRAD)
	{
		glsl << std::string(", ") << std::string(gradientName) << std::string("(float4(");
		glsl << TranslateOperand(psSrcDx, TO_AUTO_BITCAST_TO_FLOAT);
		bcatcstr(glsl, ")");
		bcatcstr(glsl, gradSwizzle);
		bcatcstr(glsl, ", float4(");
		glsl << TranslateOperand(psSrcDy, TO_AUTO_BITCAST_TO_FLOAT);
		bcatcstr(glsl, ")");
		bcatcstr(glsl, gradSwizzle);
		bcatcstr(glsl, ")");
	}
	else if (ui32Flags & (TEXSMP_FLAG_BIAS))
	{
		glsl << std::string(", bias(") << TranslateOperand(psSrcBias, TO_AUTO_BITCAST_TO_FLOAT) << std::string(")");
	}

	bool hadOffset = false;
	
	// Add offset param 
	if (psInst->bAddressOffset)
	{
		hadOffset = true;
		if (ui32NumOffsets == 1)
		{
			bformata(glsl, ", %d",
				psInst->iUAddrOffset);
		}
		else
		if (ui32NumOffsets == 2)
		{
			bformata(glsl, ", int2(%d, %d)",
				psInst->iUAddrOffset,
				psInst->iVAddrOffset);
		}
		else
		if (ui32NumOffsets == 3)
		{
			bformata(glsl, ", int3(%d, %d, %d)",
				psInst->iUAddrOffset,
				psInst->iVAddrOffset,
				psInst->iWAddrOffset);
		}
	}
	// HLSL gather has a variant with separate offset operand
	else if (ui32Flags & TEXSMP_FLAG_PARAMOFFSET)
	{
		hadOffset = true;
        uint32_t mask = OPERAND_4_COMPONENT_MASK_X;
        if (ui32NumOffsets > 1)
            mask |= OPERAND_4_COMPONENT_MASK_Y;
        if (ui32NumOffsets > 2)
            mask |= OPERAND_4_COMPONENT_MASK_Z;
        
		bcatcstr(glsl, ",");
		glsl << TranslateOperand(psSrcOff, TO_FLAG_INTEGER, mask);
	}

	// Add texture gather component selection if needed
    if ((ui32Flags & TEXSMP_FLAG_GATHER) && psSrcSamp->GetNumSwizzleElements() > 0)
    {
        ASSERT(psSrcSamp->GetNumSwizzleElements() == 1);
        if (psSrcSamp->aui32Swizzle[0] != OPERAND_4_COMPONENT_X)
        {
            if (!(ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE))
            {
				// Need to add offset param to match func overload 
				if (!hadOffset)
				{
					if (ui32NumOffsets == 1)
						bcatcstr(glsl, ", 0");
					else
						bformata(glsl, ", int%d(0)", ui32NumOffsets);
				}
				
                bcatcstr(glsl, ", component::");
                glsl << TranslateOperandSwizzle(psSrcSamp, OPERAND_4_COMPONENT_MASK_ALL, 0, false);
            }
            else
            {
                psContext->m_Reflection.OnDiagnostics("Metal supports gather compare only for the first component.", 0, true);
            }
        }
    }

	bcatcstr(glsl, ")");

	if (!(ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE) || (ui32Flags & TEXSMP_FLAG_GATHER))
	{
		// iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
		// does not make sense. But need to re-enable to correctly swizzle this particular instruction.
		psSrcTex->iWriteMaskEnabled = 1;
		glsl << TranslateOperandSwizzle(psSrcTex, psDest->GetAccessMask(), 0);
	}
	AddAssignPrologue(numParenthesis);
}

static const char* swizzleString[] = { ".x", ".y", ".z", ".w" };

// Handle cases where vector components are accessed with dynamic index ([] notation).
// A bit ugly hack because compiled HLSL uses byte offsets to access data in structs => we are converting
// the offset back to vector component index in runtime => calculating stuff back and forth.
// TODO: Would be better to eliminate the offset calculation ops and use indexes straight on. Could be tricky though...
void ToMetal::TranslateDynamicComponentSelection(const ShaderVarType* psVarType, const Operand* psByteAddr, uint32_t offset, uint32_t mask)
{
	bstring glsl = *psContext->currentGLSLString;
	ASSERT(psVarType->Class == SVC_VECTOR);

	bcatcstr(glsl, "["); // Access vector component with [] notation 
	if (offset > 0)
		bcatcstr(glsl, "(");

	// The var containing byte address to the requested element
	glsl << TranslateOperand(psByteAddr, TO_FLAG_UNSIGNED_INTEGER, mask);

	if (offset > 0)// If the vector is part of a struct, there is an extra offset in our byte address
		bformata(glsl, " - %du)", offset); // Subtract that first

	bcatcstr(glsl, " >> 0x2u"); // Convert byte offset to index: div by four
	bcatcstr(glsl, "]");
}

void ToMetal::TranslateShaderStorageStore(Instruction* psInst)
{
	bstring glsl = *psContext->currentGLSLString;
	int component;
	int srcComponent = 0;

	Operand* psDest = 0;
	Operand* psDestAddr = 0;
	Operand* psDestByteOff = 0;
	Operand* psSrc = 0;


	switch (psInst->eOpcode)
	{
	case OPCODE_STORE_STRUCTURED:
		psDest = &psInst->asOperands[0];
		psDestAddr = &psInst->asOperands[1];
		psDestByteOff = &psInst->asOperands[2];
		psSrc = &psInst->asOperands[3];

		break;
	case OPCODE_STORE_RAW:
		psDest = &psInst->asOperands[0];
		psDestByteOff = &psInst->asOperands[1];
		psSrc = &psInst->asOperands[2];
		break;
    default:
        ASSERT(0);
        break;
	}

	uint32_t dstOffFlag = TO_FLAG_UNSIGNED_INTEGER;
	SHADER_VARIABLE_TYPE dstOffType = psDestByteOff->GetDataType(psContext);
	if (dstOffType == SVT_INT || dstOffType == SVT_INT16 || dstOffType == SVT_INT12)
		dstOffFlag = TO_FLAG_INTEGER;

	for (component = 0; component < 4; component++)
	{
		ASSERT(psInst->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
		if (psInst->asOperands[0].ui32CompMask & (1 << component))
		{
			psContext->AddIndentation();
			glsl << TranslateOperand(psDest, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);
			
			if (psDestAddr)
			{
				bcatcstr(glsl, "[");
				glsl << TranslateOperand(psDestAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
				bcatcstr(glsl, "].value");
			}

			bcatcstr(glsl, "[(");
			glsl << TranslateOperand(psDestByteOff, dstOffFlag);
			bcatcstr(glsl, " >> 2");
			if (dstOffFlag == TO_FLAG_UNSIGNED_INTEGER)
				bcatcstr(glsl, "u");
			bcatcstr(glsl, ")");

			if (component != 0)
			{
				bformata(glsl, " + %d", component);
				if (dstOffFlag == TO_FLAG_UNSIGNED_INTEGER)
					bcatcstr(glsl, "u");
			}

			bcatcstr(glsl, "]");

			//Dest type is currently always a uint array.
			bcatcstr(glsl, " = as_type<uint>(");
			if (psSrc->GetNumSwizzleElements() > 1)
				glsl << TranslateOperand(psSrc, TO_FLAG_DECLARATION_NAME/*TO_FLAG_UNSIGNED_INTEGER*/, 1 << (srcComponent++));
			else
				glsl << TranslateOperand(psSrc, TO_FLAG_DECLARATION_NAME/*TO_FLAG_UNSIGNED_INTEGER*/, OPERAND_4_COMPONENT_MASK_X);

			bformata(glsl, ");\n");
		}
	}
}

void ToMetal::TranslateShaderStorageLoad(Instruction* psInst)
{
	bstring glsl = *psContext->currentGLSLString;
	int component;
	Operand* psDest = 0;
	Operand* psSrcAddr = 0;
	Operand* psSrcByteOff = 0;
	Operand* psSrc = 0;

	switch (psInst->eOpcode)
	{
	case OPCODE_LD_STRUCTURED:
		psDest = &psInst->asOperands[0];
		psSrcAddr = &psInst->asOperands[1];
		psSrcByteOff = &psInst->asOperands[2];
		psSrc = &psInst->asOperands[3];
		break;
	case OPCODE_LD_RAW:
		psDest = &psInst->asOperands[0];
		psSrcByteOff = &psInst->asOperands[1];
		psSrc = &psInst->asOperands[2];
		break;
    default:
        ASSERT(0);
        break;
	}

	uint32_t destCount = psDest->GetNumSwizzleElements();
	uint32_t destMask = psDest->GetAccessMask();

	int numParenthesis = 0;
	int firstItemAdded = 0;
	SHADER_VARIABLE_TYPE destDataType = psDest->GetDataType(psContext);
	uint32_t srcOffFlag = TO_FLAG_UNSIGNED_INTEGER;
	SHADER_VARIABLE_TYPE srcOffType = psSrcByteOff->GetDataType(psContext);
	if (srcOffType == SVT_INT || srcOffType == SVT_INT16 || srcOffType == SVT_INT12)
		srcOffFlag = TO_FLAG_INTEGER;

	psContext->AddIndentation();
	AddAssignToDest(psDest, destDataType, destCount, &numParenthesis);
	if (destCount > 1)
	{
		bformata(glsl, "%s(", GetConstructorForTypeMetal(destDataType, destCount));
		numParenthesis++;
	}
	for (component = 0; component < 4; component++)
	{
		bool addedBitcast = false;
		if (!(destMask & (1 << component)))
			continue;

		if (firstItemAdded)
			bcatcstr(glsl, ", ");
		else
			firstItemAdded = 1;

		// always uint array atm
		if (destDataType == SVT_FLOAT)
		{
			// input already in uints, need bitcast
			bcatcstr(glsl, "as_type<float>(");
			addedBitcast = true;
		}
		else if (destDataType == SVT_INT || destDataType == SVT_INT16 || destDataType == SVT_INT12)
		{
			bcatcstr(glsl, "as_type<int>(");
			addedBitcast = true;
		}

		glsl << TranslateOperand(psSrc, TO_FLAG_NAME_ONLY);

		if (psSrcAddr)
		{
			bcatcstr(glsl, "[");
			glsl << TranslateOperand(psSrcAddr, TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_INTEGER);
			bcatcstr(glsl, "].value");
		}
		bcatcstr(glsl, "[(");
		glsl << TranslateOperand(psSrcByteOff, srcOffFlag);
		bcatcstr(glsl, " >> 2");
		if (srcOffFlag == TO_FLAG_UNSIGNED_INTEGER)
			bcatcstr(glsl, "u");

		bformata(glsl, ") + %d", psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
		if (srcOffFlag == TO_FLAG_UNSIGNED_INTEGER)
			bcatcstr(glsl, "u");

		bcatcstr(glsl, "]");
		
		if (addedBitcast)
			bcatcstr(glsl, ")");
	}
	AddAssignPrologue(numParenthesis);
}

void ToMetal::TranslateAtomicMemOp(Instruction* psInst)
{
	bstring glsl = *psContext->currentGLSLString;
	int numParenthesis = 0;
	uint32_t ui32DataTypeFlag = TO_FLAG_INTEGER;
	const char* func = "";
	Operand* dest = 0;
	Operand* previousValue = 0;
	Operand* destAddr = 0;
	Operand* src = 0;
	Operand* compare = 0;
	int texDim = 0;
	bool isUint = true;
	bool shouldAddFailMemoryOrder = false;
	bool shouldExtractCompare = false;

	switch (psInst->eOpcode)
	{
	case OPCODE_IMM_ATOMIC_IADD:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_IADD\n");
#endif
		func = "atomic_fetch_add_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		break;
	}
	case OPCODE_ATOMIC_IADD:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ATOMIC_IADD\n");
#endif
		func = "atomic_fetch_add_explicit";
		dest = &psInst->asOperands[0];
		destAddr = &psInst->asOperands[1];
		src = &psInst->asOperands[2];
		break;
	}
	case OPCODE_IMM_ATOMIC_AND:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_AND\n");
#endif
		func = "atomic_fetch_and_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		break;
	}
	case OPCODE_ATOMIC_AND:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ATOMIC_AND\n");
#endif
		func = "atomic_fetch_and_explicit";
		dest = &psInst->asOperands[0];
		destAddr = &psInst->asOperands[1];
		src = &psInst->asOperands[2];
		break;
	}
	case OPCODE_IMM_ATOMIC_OR:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_OR\n");
#endif
		func = "atomic_fetch_or_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		break;
	}
	case OPCODE_ATOMIC_OR:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ATOMIC_OR\n");
#endif
		func = "atomic_fetch_or_explicit";
		dest = &psInst->asOperands[0];
		destAddr = &psInst->asOperands[1];
		src = &psInst->asOperands[2];
		break;
	}
	case OPCODE_IMM_ATOMIC_XOR:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_XOR\n");
#endif
		func = "atomic_fetch_xor_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		break;
	}
	case OPCODE_ATOMIC_XOR:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ATOMIC_XOR\n");
#endif
		func = "atomic_fetch_xor_explicit";
		dest = &psInst->asOperands[0];
		destAddr = &psInst->asOperands[1];
		src = &psInst->asOperands[2];
		break;
	}

	case OPCODE_IMM_ATOMIC_EXCH:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_EXCH\n");
#endif
		func = "atomic_exchange_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		break;
	}
	case OPCODE_IMM_ATOMIC_CMP_EXCH:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_CMP_EXC\n");
#endif
		func = "atomic_compare_exchange_weak_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		compare = &psInst->asOperands[3];
		src = &psInst->asOperands[4];
		shouldAddFailMemoryOrder = true;
		shouldExtractCompare = true;
		break;
	}
	case OPCODE_ATOMIC_CMP_STORE:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ATOMIC_CMP_STORE\n");
#endif
		func = "atomic_compare_exchange_weak_explicit";
		previousValue = 0;
		dest = &psInst->asOperands[0];
		destAddr = &psInst->asOperands[1];
		compare = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		shouldAddFailMemoryOrder = true;
		shouldExtractCompare = true;
		break;
	}
	case OPCODE_IMM_ATOMIC_UMIN:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_UMIN\n");
#endif
		func = "atomic_fetch_min_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		break;
	}
	case OPCODE_ATOMIC_UMIN:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ATOMIC_UMIN\n");
#endif
		func = "atomic_fetch_min_explicit";
		dest = &psInst->asOperands[0];
		destAddr = &psInst->asOperands[1];
		src = &psInst->asOperands[2];
		break;
}
	case OPCODE_IMM_ATOMIC_IMIN:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_IMIN\n");
#endif
		func = "atomic_fetch_min_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		break;
	}
	case OPCODE_ATOMIC_IMIN:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ATOMIC_IMIN\n");
#endif
		func = "atomic_fetch_min_explicit";
		dest = &psInst->asOperands[0];
		destAddr = &psInst->asOperands[1];
		src = &psInst->asOperands[2];
		break;
	}
	case OPCODE_IMM_ATOMIC_UMAX:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_UMAX\n");
#endif
		func = "atomic_fetch_max_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		break;
	}
	case OPCODE_ATOMIC_UMAX:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ATOMIC_UMAX\n");
#endif
		func = "atomic_fetch_max_explicit";
		dest = &psInst->asOperands[0];
		destAddr = &psInst->asOperands[1];
		src = &psInst->asOperands[2];
		break;
	}
	case OPCODE_IMM_ATOMIC_IMAX:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_IMAX\n");
#endif
		func = "atomic_fetch_max_explicit";
		previousValue = &psInst->asOperands[0];
		dest = &psInst->asOperands[1];
		destAddr = &psInst->asOperands[2];
		src = &psInst->asOperands[3];
		break;
	}
	case OPCODE_ATOMIC_IMAX:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ATOMIC_IMAX\n");
#endif
		func = "atomic_fetch_max_explicit";
		dest = &psInst->asOperands[0];
		destAddr = &psInst->asOperands[1];
		src = &psInst->asOperands[2];
		break;
	}
    default:
        ASSERT(0);
        break;
	}

	psContext->AddIndentation();

	if (dest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
	{
		const ResourceBinding* psBinding = 0;
		psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_UAV, dest->ui32RegisterNumber, &psBinding);

		if (psBinding->eType == RTYPE_UAV_RWTYPED)
		{
			isUint = (psBinding->ui32ReturnType == RETURN_TYPE_UINT);

			// Find out if it's texture and of what dimension
			switch (psBinding->eDimension)
			{
			case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
				texDim = 1;
				break;
			case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
			case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
			case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
			case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
				texDim = 2;
				break;
			case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
			case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
			case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
			case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
				texDim = 3;
				break;
            default:
                ASSERT(0);
                break;
			}
		}
	}

	if (texDim > 0)
	{
		psContext->m_Reflection.OnDiagnostics("Texture atomics are not supported in Metal", 0, true);
		return;
	}

	if (isUint)
		ui32DataTypeFlag = TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_BITCAST_TO_UINT;
	else
		ui32DataTypeFlag = TO_FLAG_INTEGER | TO_AUTO_BITCAST_TO_INT;

	if (shouldExtractCompare)
	{
		bcatcstr(glsl, "{\n");
		psContext->AddIndentation();
		psContext->AddIndentation();
		bcatcstr(glsl, "uint compare_value = ");
		glsl << TranslateOperand(compare, ui32DataTypeFlag);
		bcatcstr(glsl, "; ");
	}

	if (previousValue)
		AddAssignToDest(previousValue, isUint ? SVT_UINT : SVT_INT, 1, &numParenthesis);

	bcatcstr(glsl, func);
	bcatcstr(glsl, "(");

	uint32_t destAddrFlag = TO_FLAG_UNSIGNED_INTEGER;
	SHADER_VARIABLE_TYPE destAddrType = destAddr->GetDataType(psContext);
	if (destAddrType == SVT_INT || destAddrType == SVT_INT16 || destAddrType == SVT_INT12)
		destAddrFlag = TO_FLAG_INTEGER;

	if(dest->eType == OPERAND_TYPE_UNORDERED_ACCESS_VIEW)
		bcatcstr(glsl, "reinterpret_cast<device atomic_uint *>(&");
	else
		bcatcstr(glsl, "reinterpret_cast<threadgroup atomic_uint *>(&");
	glsl << TranslateOperand(dest, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);
	bcatcstr(glsl, "[");
	glsl << TranslateOperand(destAddr, destAddrFlag, OPERAND_4_COMPONENT_MASK_X);
	
	// Structured buf if we have both x & y swizzles. Raw buf has only x -> no .value[]
	if (destAddr->GetNumSwizzleElements(OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y) == 2)
	{
		bcatcstr(glsl, "]");
		bcatcstr(glsl, ".value[");
		glsl << TranslateOperand(destAddr, destAddrFlag, OPERAND_4_COMPONENT_MASK_Y);
	}
	
	bcatcstr(glsl, " >> 2");//bytes to floats
	if (destAddrFlag == TO_FLAG_UNSIGNED_INTEGER)
		bcatcstr(glsl, "u");

	bcatcstr(glsl, "]), ");

	if (compare)
	{
		if (shouldExtractCompare)
		{
			bcatcstr(glsl, "&compare_value, ");
		}
		else
		{
			glsl << TranslateOperand(compare, ui32DataTypeFlag);
			bcatcstr(glsl, ", ");
		}
	}

	glsl << TranslateOperand(src, ui32DataTypeFlag);
	bcatcstr(glsl, ", memory_order::memory_order_relaxed");
	if (shouldAddFailMemoryOrder)
		bcatcstr(glsl, ", memory_order::memory_order_relaxed");
	bcatcstr(glsl, ")");
	if (previousValue)
	{
		AddAssignPrologue(numParenthesis);
	}
	else
		bcatcstr(glsl, ";\n");

	if (shouldExtractCompare)
	{
		psContext->AddIndentation();
		bcatcstr(glsl, "}\n");
	}
}

void ToMetal::TranslateConditional(
	Instruction* psInst,
	bstring glsl)
{
	const char* statement = "";
	if (psInst->eOpcode == OPCODE_BREAKC)
	{
		statement = "break";
	}
	else if (psInst->eOpcode == OPCODE_CONTINUEC)
	{
		statement = "continue";
	}
	else if (psInst->eOpcode == OPCODE_RETC) // FIXME! Need to spew out shader epilogue
	{
		if (psContext->psShader.eShaderType == COMPUTE_SHADER)
			statement = "return";
		else
			statement = "return output";
	}


	int isBool = psInst->asOperands[0].GetDataType(psContext) == SVT_BOOL;

	if (isBool)
	{
		bcatcstr(glsl, "if(");
		if (psInst->eBooleanTestType != INSTRUCTION_TEST_NONZERO)
			bcatcstr(glsl, "!");
		glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_BOOL);
		if (psInst->eOpcode != OPCODE_IF)
		{
			bformata(glsl, "){%s;}\n", statement);
		}
		else
		{
			bcatcstr(glsl, "){\n");
		}
	}
	else
	{
		if (psInst->eBooleanTestType == INSTRUCTION_TEST_ZERO)
		{
			bcatcstr(glsl, "if((");
			glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER);

			if (psInst->eOpcode != OPCODE_IF)
			{
				bformata(glsl, ")==uint(0u)){%s;}\n", statement);
			}
			else
			{
				bcatcstr(glsl, ")==uint(0u)){\n");
			}
		}
		else
		{
			ASSERT(psInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO);
			bcatcstr(glsl, "if((");
			glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER);

			if (psInst->eOpcode != OPCODE_IF)
			{
				bformata(glsl, ")!=uint(0u)){%s;}\n", statement);
			}
			else
			{
				bcatcstr(glsl, ")!=uint(0u)){\n");
			}
		}
	}
}

void ToMetal::TranslateInstruction(Instruction* psInst)
{
	bstring glsl = *psContext->currentGLSLString;
	int numParenthesis = 0;

	switch (psInst->eOpcode)
	{
	case OPCODE_FTOI:
	case OPCODE_FTOU:
	{
		uint32_t dstCount = psInst->asOperands[0].GetNumSwizzleElements();
		uint32_t srcCount = psInst->asOperands[1].GetNumSwizzleElements();
		SHADER_VARIABLE_TYPE castType = psInst->eOpcode == OPCODE_FTOU ? SVT_UINT : SVT_INT;
#ifdef _DEBUG
		psContext->AddIndentation();
		if (psInst->eOpcode == OPCODE_FTOU)
			bcatcstr(glsl, "//FTOU\n");
		else
			bcatcstr(glsl, "//FTOI\n");
#endif
		switch (psInst->asOperands[0].eMinPrecision)
		{
		case OPERAND_MIN_PRECISION_DEFAULT:
			break;
		case OPERAND_MIN_PRECISION_SINT_16:
			castType = SVT_INT16;
			ASSERT(psInst->eOpcode == OPCODE_FTOI);
			break;
		case OPERAND_MIN_PRECISION_UINT_16:
			castType = SVT_UINT16;
			ASSERT(psInst->eOpcode == OPCODE_FTOU);
			break;
		default:
			ASSERT(0); // We'd be doing bitcasts into low/mediump ints, not good.
		}
		psContext->AddIndentation();

		AddAssignToDest(&psInst->asOperands[0], castType, srcCount, &numParenthesis);
		bcatcstr(glsl, GetConstructorForTypeMetal(castType, dstCount));
		bcatcstr(glsl, "("); // 1
		glsl << TranslateOperand(&psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT, psInst->asOperands[0].GetAccessMask());
		bcatcstr(glsl, ")"); // 1
		AddAssignPrologue(numParenthesis);
		break;
	}

	case OPCODE_MOV:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//MOV\n");
#endif
		psContext->AddIndentation();
		AddMOVBinaryOp(&psInst->asOperands[0], &psInst->asOperands[1]);
		break;
	}
	case OPCODE_ITOF://signed to float
	case OPCODE_UTOF://unsigned to float
	{
		SHADER_VARIABLE_TYPE castType = SVT_FLOAT;
		uint32_t dstCount = psInst->asOperands[0].GetNumSwizzleElements();
		uint32_t srcCount = psInst->asOperands[1].GetNumSwizzleElements();

#ifdef _DEBUG
		psContext->AddIndentation();
		if (psInst->eOpcode == OPCODE_ITOF)
		{
			bcatcstr(glsl, "//ITOF\n");
		}
		else
		{
			bcatcstr(glsl, "//UTOF\n");
		}
#endif

		switch (psInst->asOperands[0].eMinPrecision)
		{
		case OPERAND_MIN_PRECISION_DEFAULT:
			break;
		case OPERAND_MIN_PRECISION_FLOAT_2_8:
			castType = SVT_FLOAT10;
			break;
		case OPERAND_MIN_PRECISION_FLOAT_16:
			castType = SVT_FLOAT16;
			break;
		default:
			ASSERT(0); // We'd be doing bitcasts into low/mediump ints, not good.
		}

		psContext->AddIndentation();
		AddAssignToDest(&psInst->asOperands[0], castType, srcCount, &numParenthesis);
		bcatcstr(glsl, GetConstructorForTypeMetal(castType, dstCount));
		bcatcstr(glsl, "("); // 1
		glsl << TranslateOperand(&psInst->asOperands[1], psInst->eOpcode == OPCODE_UTOF ? TO_AUTO_BITCAST_TO_UINT : TO_AUTO_BITCAST_TO_INT, psInst->asOperands[0].GetAccessMask());
		bcatcstr(glsl, ")"); // 1
		AddAssignPrologue(numParenthesis);
		break;
	}
	case OPCODE_MAD:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//MAD\n");
#endif
		CallTernaryOp("*", "+", psInst, 0, 1, 2, 3, TO_FLAG_NONE);
		break;
	}
	case OPCODE_IMAD:
	{
		uint32_t ui32Flags = TO_FLAG_INTEGER;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMAD\n");
#endif

		if (psInst->asOperands[0].GetDataType(psContext) == SVT_UINT)
		{
			ui32Flags = TO_FLAG_UNSIGNED_INTEGER;
		}

		CallTernaryOp("*", "+", psInst, 0, 1, 2, 3, ui32Flags);
		break;
	}
	case OPCODE_DADD:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//DADD\n");
#endif
		CallBinaryOp("+", psInst, 0, 1, 2, SVT_DOUBLE);
		break;
	}
	case OPCODE_IADD:
	{
		SHADER_VARIABLE_TYPE eType = SVT_INT;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IADD\n");
#endif
		//Is this a signed or unsigned add?
		if (psInst->asOperands[0].GetDataType(psContext) == SVT_UINT)
		{
			eType = SVT_UINT;
		}
		CallBinaryOp("+", psInst, 0, 1, 2, eType);
		break;
	}
	case OPCODE_ADD:
	{

#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ADD\n");
#endif
		CallBinaryOp("+", psInst, 0, 1, 2, SVT_FLOAT);
		break;
	}
	case OPCODE_OR:
	{
		/*Todo: vector version */
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//OR\n");
#endif
		uint32_t destMask = psInst->asOperands[0].GetAccessMask();
		if (psInst->asOperands[0].GetDataType(psContext) == SVT_BOOL)
		{
			uint32_t destMask = psInst->asOperands[0].GetAccessMask();

			int needsParenthesis = 0;
			psContext->AddIndentation();
			AddAssignToDest(&psInst->asOperands[0], SVT_BOOL, psInst->asOperands[0].GetNumSwizzleElements(), &needsParenthesis);
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_BOOL, destMask);
			bcatcstr(glsl, " || ");
			glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_BOOL, destMask);
			AddAssignPrologue(needsParenthesis);
		}
		else
			CallBinaryOp("|", psInst, 0, 1, 2, SVT_UINT);
			break;
	}
	case OPCODE_AND:
	{
		SHADER_VARIABLE_TYPE eA = psInst->asOperands[1].GetDataType(psContext);
		SHADER_VARIABLE_TYPE eB = psInst->asOperands[2].GetDataType(psContext);
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//AND\n");
#endif
		uint32_t destMask = psInst->asOperands[0].GetAccessMask();
		uint32_t dstSwizCount = psInst->asOperands[0].GetNumSwizzleElements();
		SHADER_VARIABLE_TYPE eDataType = psInst->asOperands[0].GetDataType(psContext);
		uint32_t ui32Flags = SVTTypeToFlag(eDataType);
		if (psInst->asOperands[0].GetDataType(psContext) == SVT_BOOL)
		{
			int needsParenthesis = 0;
			psContext->AddIndentation();
			AddAssignToDest(&psInst->asOperands[0], SVT_BOOL, psInst->asOperands[0].GetNumSwizzleElements(), &needsParenthesis);
			glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_BOOL, destMask);
			bcatcstr(glsl, " && ");
			glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_BOOL, destMask);
			AddAssignPrologue(needsParenthesis);
		}
		else if ((eA == SVT_BOOL || eB == SVT_BOOL) && !(eA == SVT_BOOL && eB == SVT_BOOL))
		{
			int boolOp = eA == SVT_BOOL ? 1 : 2;
			int otherOp = eA == SVT_BOOL ? 2 : 1;
			int needsParenthesis = 0;
			uint32_t i;
			psContext->AddIndentation();

			if (dstSwizCount == 1)
			{
				AddAssignToDest(&psInst->asOperands[0], eDataType, dstSwizCount, &needsParenthesis);
				glsl << TranslateOperand(&psInst->asOperands[boolOp], TO_FLAG_BOOL, destMask);
				bcatcstr(glsl, " ? ");
				glsl << TranslateOperand(&psInst->asOperands[otherOp], ui32Flags, destMask);
				bcatcstr(glsl, " : ");

				bcatcstr(glsl, GetConstructorForTypeMetal(eDataType, dstSwizCount));
				bcatcstr(glsl, "(");
				for (i = 0; i < dstSwizCount; i++)
				{
					if (i > 0)
						bcatcstr(glsl, ", ");
					switch (eDataType)
					{
					case SVT_FLOAT:
					case SVT_FLOAT10:
					case SVT_FLOAT16:
					case SVT_DOUBLE:
						bcatcstr(glsl, "0.0");
						break;
					default:
						bcatcstr(glsl, "0");

					}
				}
				bcatcstr(glsl, ")");
			}
			else if (eDataType == SVT_FLOAT)
			{
				// We can use select()
				AddAssignToDest(&psInst->asOperands[0], eDataType, dstSwizCount, &needsParenthesis);
				bcatcstr(glsl, "select(");
				bcatcstr(glsl, GetConstructorForTypeMetal(eDataType, dstSwizCount));
				bcatcstr(glsl, "(");
				for (i = 0; i < dstSwizCount; i++)
				{
					if (i > 0)
						bcatcstr(glsl, ", ");
					switch (eDataType)
					{
					case SVT_FLOAT:
					case SVT_FLOAT10:
					case SVT_FLOAT16:
					case SVT_DOUBLE:
						bcatcstr(glsl, "0.0");
						break;
					default:
						bcatcstr(glsl, "0");

					}
				}
				bcatcstr(glsl, "), ");
				glsl << TranslateOperand(&psInst->asOperands[otherOp], ui32Flags, destMask);
				bcatcstr(glsl, ", ");
                bcatcstr(glsl, GetConstructorForTypeMetal(SVT_BOOL, dstSwizCount));
				bcatcstr(glsl, "(");
				glsl << TranslateOperand(&psInst->asOperands[boolOp], TO_FLAG_BOOL, destMask);
				bcatcstr(glsl, ")");
				bcatcstr(glsl, ")");
			}
			else
			{
				AddAssignToDest(&psInst->asOperands[0], SVT_UINT, dstSwizCount, &needsParenthesis);
				bcatcstr(glsl, "(");
				bcatcstr(glsl, GetConstructorForTypeMetal(SVT_UINT, dstSwizCount));
				bcatcstr(glsl, "(");
				glsl << TranslateOperand(&psInst->asOperands[boolOp], TO_FLAG_BOOL, destMask);
				bcatcstr(glsl, ") * 0xffffffffu) & ");
				glsl << TranslateOperand(&psInst->asOperands[otherOp], TO_FLAG_UNSIGNED_INTEGER, destMask);
			}

			AddAssignPrologue(needsParenthesis);
		}
		else
		{
			CallBinaryOp("&", psInst, 0, 1, 2, SVT_UINT);
		}


		break;
	}
	case OPCODE_GE:
	{
		/*
			dest = vec4(greaterThanEqual(vec4(srcA), vec4(srcB));
			Caveat: The result is a boolean but HLSL asm returns 0xFFFFFFFF/0x0 instead.
			*/
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//GE\n");
#endif
		AddComparison(psInst, CMP_GE, TO_FLAG_NONE);
		break;
	}
	case OPCODE_MUL:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//MUL\n");
#endif
		CallBinaryOp("*", psInst, 0, 1, 2, SVT_FLOAT);
		break;
	}
	case OPCODE_IMUL:
	{
		SHADER_VARIABLE_TYPE eType = SVT_INT;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMUL\n");
#endif
		if (psInst->asOperands[1].GetDataType(psContext) == SVT_UINT)
		{
			eType = SVT_UINT;
		}

		ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_NULL);

		CallBinaryOp("*", psInst, 1, 2, 3, eType);
		break;
	}
	case OPCODE_UDIV:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//UDIV\n");
#endif
		//destQuotient, destRemainder, src0, src1
		//CallBinaryOp("/", psInst, 0, 2, 3, SVT_UINT);
		//CallBinaryOp("%", psInst, 1, 2, 3, SVT_UINT);
    TranslateUDiv(psInst, 0, 1, 2, 3);

		break;
	}
	case OPCODE_DIV:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//DIV\n");
#endif
		CallBinaryOp("/", psInst, 0, 1, 2, SVT_FLOAT);
		break;
	}
	case OPCODE_SINCOS:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SINCOS\n");
#endif
		// Need careful ordering if src == dest[0], as then the cos() will be reading from wrong value
		if (psInst->asOperands[0].eType == psInst->asOperands[2].eType &&
			psInst->asOperands[0].ui32RegisterNumber == psInst->asOperands[2].ui32RegisterNumber)
		{
			// sin() result overwrites source, do cos() first.
			// The case where both write the src shouldn't really happen anyway.
			if (psInst->asOperands[1].eType != OPERAND_TYPE_NULL)
			{
				CallHelper1("cos", psInst, 1, 2, 1);
			}

			if (psInst->asOperands[0].eType != OPERAND_TYPE_NULL)
			{
				CallHelper1(
					"sin", psInst, 0, 2, 1);
			}
		}
		else
		{
			if (psInst->asOperands[0].eType != OPERAND_TYPE_NULL)
			{
				CallHelper1("sin", psInst, 0, 2, 1);
			}

			if (psInst->asOperands[1].eType != OPERAND_TYPE_NULL)
			{
				CallHelper1("cos", psInst, 1, 2, 1);
			}
		}
		break;
	}

	case OPCODE_DP2:
	{
		int numParenthesis = 0;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//DP2\n");
#endif
		psContext->AddIndentation();
		SHADER_VARIABLE_TYPE dstType = psInst->asOperands[0].GetDataType(psContext);
		uint32_t typeFlags = TO_AUTO_BITCAST_TO_FLOAT | TO_AUTO_EXPAND_TO_VEC2;
		if (psInst->asOperands[1].GetDataType(psContext) == SVT_FLOAT16
			&& psInst->asOperands[2].GetDataType(psContext) == SVT_FLOAT16)
			typeFlags = TO_FLAG_FORCE_HALF | TO_AUTO_EXPAND_TO_VEC2;

		if (dstType != SVT_FLOAT16)
			dstType = SVT_FLOAT;

		AddAssignToDest(&psInst->asOperands[0], dstType, 1, &numParenthesis);
		bcatcstr(glsl, "dot(");
		glsl << TranslateOperand(&psInst->asOperands[1], typeFlags, 3 /* .xy */);
		bcatcstr(glsl, ", ");
		glsl << TranslateOperand(&psInst->asOperands[2], typeFlags, 3 /* .xy */);
		bcatcstr(glsl, ")");
		AddAssignPrologue(numParenthesis);
		break;
	}
	case OPCODE_DP3:
	{
		int numParenthesis = 0;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//DP3\n");
#endif
		psContext->AddIndentation();
		SHADER_VARIABLE_TYPE dstType = psInst->asOperands[0].GetDataType(psContext);
		uint32_t typeFlags = TO_AUTO_BITCAST_TO_FLOAT | TO_AUTO_EXPAND_TO_VEC3;
		if (psInst->asOperands[1].GetDataType(psContext) == SVT_FLOAT16
			&& psInst->asOperands[2].GetDataType(psContext) == SVT_FLOAT16)
			typeFlags = TO_FLAG_FORCE_HALF | TO_AUTO_EXPAND_TO_VEC3;

		if (dstType != SVT_FLOAT16)
			dstType = SVT_FLOAT;

		AddAssignToDest(&psInst->asOperands[0], dstType, 1, &numParenthesis);
		bcatcstr(glsl, "dot(");
		glsl << TranslateOperand(&psInst->asOperands[1], typeFlags, 7 /* .xyz */);
		bcatcstr(glsl, ", ");
		glsl << TranslateOperand(&psInst->asOperands[2], typeFlags, 7 /* .xyz */);
		bcatcstr(glsl, ")");
		AddAssignPrologue(numParenthesis);
		break;
	}
	case OPCODE_DP4:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//DP4\n");
#endif
		CallHelper2("dot", psInst, 0, 1, 2, 0);
		break;
	}
	case OPCODE_INE:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//INE\n");
#endif
		AddComparison(psInst, CMP_NE, TO_FLAG_INTEGER);
		break;
	}
	case OPCODE_NE:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//NE\n");
#endif
		AddComparison(psInst, CMP_NE, TO_FLAG_NONE);
		break;
	}
	case OPCODE_IGE:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IGE\n");
#endif
		AddComparison(psInst, CMP_GE, TO_FLAG_INTEGER);
		break;
	}
	case OPCODE_ILT:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ILT\n");
#endif
		AddComparison(psInst, CMP_LT, TO_FLAG_INTEGER);
		break;
	}
	case OPCODE_LT:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//LT\n");
#endif
		AddComparison(psInst, CMP_LT, TO_FLAG_NONE);
		break;
	}
	case OPCODE_IEQ:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IEQ\n");
#endif
		AddComparison(psInst, CMP_EQ, TO_FLAG_INTEGER);
		break;
	}
	case OPCODE_ULT:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ULT\n");
#endif
		AddComparison(psInst, CMP_LT, TO_FLAG_UNSIGNED_INTEGER);
		break;
	}
	case OPCODE_UGE:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//UGE\n");
#endif
		AddComparison(psInst, CMP_GE, TO_FLAG_UNSIGNED_INTEGER);
		break;
	}
	case OPCODE_MOVC:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//MOVC\n");
#endif
		AddMOVCBinaryOp(&psInst->asOperands[0], &psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3]);
		break;
	}
	case OPCODE_SWAPC:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SWAPC\n");
#endif
		// TODO needs temps!!
		ASSERT(0);
		AddMOVCBinaryOp(&psInst->asOperands[0], &psInst->asOperands[2], &psInst->asOperands[4], &psInst->asOperands[3]);
		AddMOVCBinaryOp(&psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3], &psInst->asOperands[4]);
		break;
	}

	case OPCODE_LOG:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//LOG\n");
#endif
		CallHelper1("log2", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_RSQ:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//RSQ\n");
#endif
		CallHelper1("rsqrt", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_EXP:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//EXP\n");
#endif
		CallHelper1("exp2", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_SQRT:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SQRT\n");
#endif
		CallHelper1("sqrt", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_ROUND_PI:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ROUND_PI\n");
#endif
		CallHelper1("ceil", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_ROUND_NI:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ROUND_NI\n");
#endif
		CallHelper1("floor", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_ROUND_Z:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ROUND_Z\n");
#endif
		CallHelper1("trunc", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_ROUND_NE:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ROUND_NE\n");
#endif
		CallHelper1("rint", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_FRC:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//FRC\n");
#endif
		CallHelper1("fract", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_IMAX:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMAX\n");
#endif
		CallHelper2Int("max", psInst, 0, 1, 2, 1);
		break;
	}
	case OPCODE_UMAX:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//UMAX\n");
#endif
		CallHelper2UInt("max", psInst, 0, 1, 2, 1);
		break;
	}
	case OPCODE_MAX:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//MAX\n");
#endif
		CallHelper2("max", psInst, 0, 1, 2, 1);
		break;
	}
	case OPCODE_IMIN:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMIN\n");
#endif
		CallHelper2Int("min", psInst, 0, 1, 2, 1);
		break;
	}
	case OPCODE_UMIN:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//UMIN\n");
#endif
		CallHelper2UInt("min", psInst, 0, 1, 2, 1);
		break;
	}
	case OPCODE_MIN:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//MIN\n");
#endif
		CallHelper2("min", psInst, 0, 1, 2, 1);
		break;
	}
	case OPCODE_GATHER4:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//GATHER4\n");
#endif
		TranslateTextureSample(psInst, TEXSMP_FLAG_GATHER);
		break;
	}
	case OPCODE_GATHER4_PO_C:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//GATHER4_PO_C\n");
#endif
		TranslateTextureSample(psInst, TEXSMP_FLAG_GATHER | TEXSMP_FLAG_PARAMOFFSET | TEXSMP_FLAG_DEPTHCOMPARE);
		break;
	}
	case OPCODE_GATHER4_PO:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//GATHER4_PO\n");
#endif
		TranslateTextureSample(psInst, TEXSMP_FLAG_GATHER | TEXSMP_FLAG_PARAMOFFSET);
		break;
	}
	case OPCODE_GATHER4_C:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//GATHER4_C\n");
#endif
		TranslateTextureSample(psInst, TEXSMP_FLAG_GATHER | TEXSMP_FLAG_DEPTHCOMPARE);
		break;
	}
	case OPCODE_SAMPLE:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SAMPLE\n");
#endif
		TranslateTextureSample(psInst, TEXSMP_FLAG_NONE);
		break;
	}
	case OPCODE_SAMPLE_L:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SAMPLE_L\n");
#endif
		TranslateTextureSample(psInst, TEXSMP_FLAG_LOD);
		break;
	}
	case OPCODE_SAMPLE_C:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SAMPLE_C\n");
#endif

		TranslateTextureSample(psInst, TEXSMP_FLAG_DEPTHCOMPARE);
		break;
	}
	case OPCODE_SAMPLE_C_LZ:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SAMPLE_C_LZ\n");
#endif

		TranslateTextureSample(psInst, TEXSMP_FLAG_DEPTHCOMPARE | TEXSMP_FLAG_FIRSTLOD);
		break;
	}
	case OPCODE_SAMPLE_D:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SAMPLE_D\n");
#endif

		TranslateTextureSample(psInst, TEXSMP_FLAG_GRAD);
		break;
	}
	case OPCODE_SAMPLE_B:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SAMPLE_B\n");
#endif

		TranslateTextureSample(psInst, TEXSMP_FLAG_BIAS);
		break;
	}
	case OPCODE_RET:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//RET\n");
#endif
		if (psContext->psShader.asPhases[psContext->currentPhase].hasPostShaderCode)
		{
#ifdef _DEBUG
			psContext->AddIndentation();
			bcatcstr(glsl, "//--- Post shader code ---\n");
#endif
			bconcat(glsl, psContext->psShader.asPhases[psContext->currentPhase].postShaderCode);
#ifdef _DEBUG
			psContext->AddIndentation();
			bcatcstr(glsl, "//--- End post shader code ---\n");
#endif
		}
		psContext->AddIndentation();
		if(psContext->psShader.eShaderType == COMPUTE_SHADER)
			bcatcstr(glsl, "return;\n");
		else
			bcatcstr(glsl, "return output;\n");

		break;
	}
	case OPCODE_INTERFACE_CALL:
	{
		ASSERT(0);
	}
	case OPCODE_LABEL:
	{
		ASSERT(0); // Never seen this
	}
	case OPCODE_COUNTBITS:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//COUNTBITS\n");
#endif
		psContext->AddIndentation();
		glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
		bcatcstr(glsl, " = popCount(");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER);
		bcatcstr(glsl, ");\n");
		break;
	}
	case OPCODE_FIRSTBIT_HI:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//FIRSTBIT_HI\n");
#endif
		DeclareExtraFunction("firstBit_hi", "template <typename UVecType> UVecType firstBit_hi(const UVecType input) { UVecType res = clz(input); return res; };");
		// TODO implement the 0-case (must return 0xffffffff)
		psContext->AddIndentation();
		glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_DESTINATION);
		bcatcstr(glsl, " = firstBit_hi(");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
		bcatcstr(glsl, ");\n");
		break;
	}
	case OPCODE_FIRSTBIT_LO:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//FIRSTBIT_LO\n");
#endif
		// TODO implement the 0-case (must return 0xffffffff)
		DeclareExtraFunction("firstBit_lo", "template <typename UVecType> UVecType firstBit_lo(const UVecType input) { UVecType res = ctz(input); return res; };");
		psContext->AddIndentation();
		glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_DESTINATION);
		bcatcstr(glsl, " = firstBit_lo(");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
		bcatcstr(glsl, ");\n");
		break;
	}
	case OPCODE_FIRSTBIT_SHI: //signed high
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//FIRSTBIT_SHI\n");
#endif
		// TODO Not at all correct for negative values yet.
		DeclareExtraFunction("firstBit_shi", "template <typename IVecType> IVecType firstBit_shi(const IVecType input) { IVecType res = clz(input); return res; };");
		psContext->AddIndentation();
		glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
		bcatcstr(glsl, " = firstBit_shi(");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER);
		bcatcstr(glsl, ");\n");
		break;
	}
	case OPCODE_BFREV:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//BFREV\n");
#endif
		DeclareExtraFunction("bitReverse", "template <typename UVecType> UVecType bitReverse(const UVecType input)\n\
		{ UVecType x = input;\n\
			x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));\n\
			x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));\n\
			x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));\n\
			x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));\n\
			return((x >> 16) | (x << 16));\n\
		}; ");
		psContext->AddIndentation();
		glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
		bcatcstr(glsl, " = bitReverse(");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER);
		bcatcstr(glsl, ");\n");
		break;
	}
	case OPCODE_BFI:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//BFI\n");
#endif
		DeclareExtraFunction("BFI", "\
		template <typename UVecType> UVecType bitFieldInsert(const UVecType width, const UVecType offset, const UVecType src2, const UVecType src3)\n\
		{\n\
			UVecType bitmask = (((1 << width)-1) << offset) & 0xffffffff;\n\
			return ((src2 << offset) & bitmask) | (src3 & ~bitmask);\n\
		}; ");
		psContext->AddIndentation();

		uint32_t destMask = psInst->asOperands[0].GetAccessMask();
		AddAssignToDest(&psInst->asOperands[0], SVT_UINT, psInst->asOperands[0].GetNumSwizzleElements(), &numParenthesis);
		bcatcstr(glsl, "bitFieldInsert(");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, destMask);
		bcatcstr(glsl, ", ");
		glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_UNSIGNED_INTEGER, destMask);
		bcatcstr(glsl, ", ");
		glsl << TranslateOperand(&psInst->asOperands[3], TO_FLAG_UNSIGNED_INTEGER, destMask);
		bcatcstr(glsl, ", ");
		glsl << TranslateOperand(&psInst->asOperands[4], TO_FLAG_UNSIGNED_INTEGER, destMask);
		bcatcstr(glsl, ")");

		AddAssignPrologue(numParenthesis);
		break;
	}
	case OPCODE_CUT:
	case OPCODE_EMITTHENCUT_STREAM:
	case OPCODE_EMIT:
	case OPCODE_EMITTHENCUT:
	case OPCODE_CUT_STREAM:
	case OPCODE_EMIT_STREAM:
	{
		ASSERT(0); // Not on metal
	}
	case OPCODE_REP:
	case OPCODE_ENDREP:
	{
		ASSERT(0); // Shouldn't see these anymore
	}
	case OPCODE_LOOP:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//LOOP\n");
#endif
		psContext->AddIndentation();

		bcatcstr(glsl, "while(true){\n");
		++psContext->indent;
		break;
	}
	case OPCODE_ENDLOOP:
	{
		--psContext->indent;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ENDLOOP\n");
#endif
		psContext->AddIndentation();
		bcatcstr(glsl, "}\n");
		break;
	}
	case OPCODE_BREAK:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//BREAK\n");
#endif
		psContext->AddIndentation();
		bcatcstr(glsl, "break;\n");
		break;
	}
	case OPCODE_BREAKC:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//BREAKC\n");
#endif
		psContext->AddIndentation();

		TranslateConditional(psInst, glsl);
		break;
	}
	case OPCODE_CONTINUEC:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//CONTINUEC\n");
#endif
		psContext->AddIndentation();

		TranslateConditional(psInst, glsl);
		break;
	}
	case OPCODE_IF:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IF\n");
#endif
		psContext->AddIndentation();

		TranslateConditional(psInst, glsl);
		++psContext->indent;
		break;
	}
	case OPCODE_RETC:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//RETC\n");
#endif
		psContext->AddIndentation();

		TranslateConditional(psInst, glsl);
		break;
	}
	case OPCODE_ELSE:
	{
		--psContext->indent;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ELSE\n");
#endif
		psContext->AddIndentation();
		bcatcstr(glsl, "} else {\n");
		psContext->indent++;
		break;
	}
	case OPCODE_ENDSWITCH:
	case OPCODE_ENDIF:
	{
		--psContext->indent;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ENDIF\n");
#endif
		psContext->AddIndentation();
		bcatcstr(glsl, "}\n");
		break;
	}
	case OPCODE_CONTINUE:
	{
		psContext->AddIndentation();
		bcatcstr(glsl, "continue;\n");
		break;
	}
	case OPCODE_DEFAULT:
	{
		--psContext->indent;
		psContext->AddIndentation();
		bcatcstr(glsl, "default:\n");
		++psContext->indent;
		break;
	}
	case OPCODE_NOP:
	{
		break;
	}
	case OPCODE_SYNC:
	{
		const uint32_t ui32SyncFlags = psInst->ui32SyncFlags;

#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SYNC\n");
#endif
		const char *barrierFlags = "mem_none";
		if (ui32SyncFlags & SYNC_THREAD_GROUP_SHARED_MEMORY)
		{
			barrierFlags = "mem_threadgroup";
		}
		if (ui32SyncFlags & (SYNC_UNORDERED_ACCESS_VIEW_MEMORY_GROUP | SYNC_UNORDERED_ACCESS_VIEW_MEMORY_GLOBAL))
		{
			barrierFlags = "mem_device";
			if (ui32SyncFlags & SYNC_THREAD_GROUP_SHARED_MEMORY)
			{
				barrierFlags = "mem_device_and_threadgroup";
			}
		}
		psContext->AddIndentation();
		bformata(glsl, "threadgroup_barrier(mem_flags::%s);\n", barrierFlags);

		break;
	}
	case OPCODE_SWITCH:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//SWITCH\n");
#endif
		psContext->AddIndentation();
		bcatcstr(glsl, "switch(int(");
		glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER);
		bcatcstr(glsl, ")){\n");

		psContext->indent += 2;
		break;
	}
	case OPCODE_CASE:
	{
		--psContext->indent;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//case\n");
#endif
		psContext->AddIndentation();

		bcatcstr(glsl, "case ");
		glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER);
		bcatcstr(glsl, ":\n");

		++psContext->indent;
		break;
	}
	case OPCODE_EQ:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//EQ\n");
#endif
		AddComparison(psInst, CMP_EQ, TO_FLAG_NONE);
		break;
	}
	case OPCODE_USHR:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//USHR\n");
#endif
		CallBinaryOp(">>", psInst, 0, 1, 2, SVT_UINT);
		break;
	}
	case OPCODE_ISHL:
	{
		SHADER_VARIABLE_TYPE eType = SVT_INT;

#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ISHL\n");
#endif

		if (psInst->asOperands[0].GetDataType(psContext) == SVT_UINT)
		{
			eType = SVT_UINT;
		}

		CallBinaryOp("<<", psInst, 0, 1, 2, eType);
		break;
	}
	case OPCODE_ISHR:
	{
		SHADER_VARIABLE_TYPE eType = SVT_INT;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//ISHR\n");
#endif

		if (psInst->asOperands[0].GetDataType(psContext) == SVT_UINT)
		{
			eType = SVT_UINT;
		}

		CallBinaryOp(">>", psInst, 0, 1, 2, eType);
		break;
	}
	case OPCODE_LD:
	case OPCODE_LD_MS:
	{
		const ResourceBinding* psBinding = 0;
#ifdef _DEBUG
		psContext->AddIndentation();
		if (psInst->eOpcode == OPCODE_LD)
			bcatcstr(glsl, "//LD\n");
		else
			bcatcstr(glsl, "//LD_MS\n");
#endif

		psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, &psBinding);

		if (psInst->bAddressOffset)
		{
			TranslateTexelFetchOffset(psInst, psBinding, glsl);
		}
		else
		{
			TranslateTexelFetch(psInst, psBinding, glsl);
		}
		break;
	}
	case OPCODE_DISCARD:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//DISCARD\n");
#endif
		psContext->AddIndentation();
		if (psInst->eBooleanTestType == INSTRUCTION_TEST_ZERO)
		{
			bcatcstr(glsl, "if((");
			glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER);
			bcatcstr(glsl, ")==0){discard_fragment();}\n");
		}
		else
		{
			ASSERT(psInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO);
			bcatcstr(glsl, "if((");
			glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER);
			bcatcstr(glsl, ")!=0){discard_fragment();}\n");
		}
		break;
	}
	case OPCODE_LOD:
	{
#ifdef _DEBUG
					   psContext->AddIndentation();
					   bcatcstr(glsl, "//LOD\n");
#endif
					   //LOD computes the following vector (ClampedLOD, NonClampedLOD, 0, 0)

					   psContext->AddIndentation();
					   AddAssignToDest(&psInst->asOperands[0], SVT_FLOAT, 4, &numParenthesis);

					   //If the core language does not have query-lod feature,
					   //then the extension is used. The name of the function
					   //changed between extension and core.
					   if (HaveQueryLod(psContext->psShader.eTargetLanguage))
					   {
						   bcatcstr(glsl, "textureQueryLod(");
					   }
					   else
					   {
						   bcatcstr(glsl, "textureQueryLOD(");
					   }

					   glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
					   bcatcstr(glsl, ",");
					   TranslateTexCoord(
						   psContext->psShader.aeResourceDims[psInst->asOperands[2].ui32RegisterNumber],
						   &psInst->asOperands[1]);
					   bcatcstr(glsl, ")");

					   //The swizzle on srcResource allows the returned values to be swizzled arbitrarily before they are written to the destination.

					   // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
					   // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
					   psInst->asOperands[2].iWriteMaskEnabled = 1;
					   glsl << TranslateOperandSwizzle(&psInst->asOperands[2], psInst->asOperands[0].GetAccessMask(), 0);
					   AddAssignPrologue(numParenthesis);
					   break;
	}
	case OPCODE_EVAL_CENTROID:
	{
#ifdef _DEBUG
								 psContext->AddIndentation();
								 bcatcstr(glsl, "//EVAL_CENTROID\n");
#endif
								 psContext->AddIndentation();
								 glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
								 bcatcstr(glsl, " = interpolateAtCentroid(");
								 //interpolateAtCentroid accepts in-qualified variables.
								 //As long as bytecode only writes vX registers in declarations
								 //we should be able to use the declared name directly.
								 glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
								 bcatcstr(glsl, ");\n");
								 break;
	}
	case OPCODE_EVAL_SAMPLE_INDEX:
	{
#ifdef _DEBUG
									 psContext->AddIndentation();
									 bcatcstr(glsl, "//EVAL_SAMPLE_INDEX\n");
#endif
									 psContext->AddIndentation();
									 glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
									 bcatcstr(glsl, " = interpolateAtSample(");
									 //interpolateAtSample accepts in-qualified variables.
									 //As long as bytecode only writes vX registers in declarations
									 //we should be able to use the declared name directly.
									 glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
									 bcatcstr(glsl, ", ");
									 glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_INTEGER);
									 bcatcstr(glsl, ");\n");
									 break;
	}
	case OPCODE_EVAL_SNAPPED:
	{
#ifdef _DEBUG
								psContext->AddIndentation();
								bcatcstr(glsl, "//EVAL_SNAPPED\n");
#endif
								psContext->AddIndentation();
								glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
								bcatcstr(glsl, " = interpolateAtOffset(");
								//interpolateAtOffset accepts in-qualified variables.
								//As long as bytecode only writes vX registers in declarations
								//we should be able to use the declared name directly.
								glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
								bcatcstr(glsl, ", ");
								glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_INTEGER);
								bcatcstr(glsl, ".xy);\n");
								break;
	}
	case OPCODE_LD_STRUCTURED:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//LD_STRUCTURED\n");
#endif
		TranslateShaderStorageLoad(psInst);
		break;
	}
	case OPCODE_LD_UAV_TYPED:
	{
#ifdef _DEBUG
								psContext->AddIndentation();
								bcatcstr(glsl, "//LD_UAV_TYPED\n");
#endif
								Operand* psDest = &psInst->asOperands[0];
								Operand* psSrc = &psInst->asOperands[2];
								Operand* psSrcAddr = &psInst->asOperands[1];

								int srcCount = psSrc->GetNumSwizzleElements();
								int numParenthesis = 0;
								uint32_t compMask = 0;

								switch (psInst->eResDim)
								{
								case RESOURCE_DIMENSION_TEXTURE3D:
								case RESOURCE_DIMENSION_TEXTURE2DARRAY:
								case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
								case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
									compMask |= (1 << 2);
								case RESOURCE_DIMENSION_TEXTURECUBE:
								case RESOURCE_DIMENSION_TEXTURE1DARRAY:
								case RESOURCE_DIMENSION_TEXTURE2D:
								case RESOURCE_DIMENSION_TEXTURE2DMS:
									compMask |= (1 << 1);
								case RESOURCE_DIMENSION_TEXTURE1D:
									compMask |= 1;
									break;
								default:
									ASSERT(0);
									break;
								}

								SHADER_VARIABLE_TYPE srcDataType = SVT_FLOAT;
								const ResourceBinding* psBinding = 0;
								psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_UAV, psSrc->ui32RegisterNumber, &psBinding);
								switch (psBinding->ui32ReturnType)
								{
								case RETURN_TYPE_FLOAT:
									srcDataType = SVT_FLOAT;
									break;
								case RETURN_TYPE_SINT:
									srcDataType = SVT_INT;
									break;
								case RETURN_TYPE_UINT:
									srcDataType = SVT_UINT;
									break;
								default:
									ASSERT(0);
									break;
								}

								psContext->AddIndentation();
								AddAssignToDest(psDest, srcDataType, srcCount, &numParenthesis);
								glsl << TranslateOperand(psSrc, TO_FLAG_NAME_ONLY);
								bcatcstr(glsl, ".read(");
								glsl << TranslateOperand(psSrcAddr, TO_FLAG_UNSIGNED_INTEGER, compMask);
								bcatcstr(glsl, ")");
								glsl << TranslateOperandSwizzle(&psInst->asOperands[0], OPERAND_4_COMPONENT_MASK_ALL, 0);
								AddAssignPrologue(numParenthesis);
								break;
	}
	case OPCODE_STORE_RAW:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//STORE_RAW\n");
#endif
		TranslateShaderStorageStore(psInst);
		break;
	}
	case OPCODE_STORE_STRUCTURED:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//STORE_STRUCTURED\n");
#endif
		TranslateShaderStorageStore(psInst);
		break;
	}

	case OPCODE_STORE_UAV_TYPED:
	{
								   const ResourceBinding* psRes;
								   int foundResource;
								   uint32_t flags = TO_FLAG_UNSIGNED_INTEGER;
								   uint32_t opMask = OPERAND_4_COMPONENT_MASK_ALL;
#ifdef _DEBUG
								   psContext->AddIndentation();
								   bcatcstr(glsl, "//STORE_UAV_TYPED\n");
#endif
								   psContext->AddIndentation();

								   foundResource = psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_UAV,
									   psInst->asOperands[0].ui32RegisterNumber,
									   &psRes);

								   ASSERT(foundResource);

								   glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_NAME_ONLY);
								   bcatcstr(glsl, ".write(");

								   switch (psRes->eDimension)
								   {
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
									   opMask = OPERAND_4_COMPONENT_MASK_X;
									   break;
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
									   opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
									   flags |= TO_AUTO_EXPAND_TO_VEC2;
									   break;
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
								   case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
									   opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z;
									   flags |= TO_AUTO_EXPAND_TO_VEC3;
									   break;
                   case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
                   case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
                       opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
                       flags |= TO_AUTO_EXPAND_TO_VEC2;
                       break;
								   case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
									   flags |= TO_AUTO_EXPAND_TO_VEC4;
									   break;
                                    default:
                                        ASSERT(0);
                                        break;
								   };

								   glsl << TranslateOperand(&psInst->asOperands[2], ResourceReturnTypeToFlag(psRes->ui32ReturnType));
								   bcatcstr(glsl, ", ");
								   glsl << TranslateOperand(&psInst->asOperands[1], flags, opMask);

                   if (psRes->eDimension == REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY)
                   {
                       bcatcstr(glsl, ", ");
                       glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_Z);
                   }

								   bformata(glsl, ");\n");

								   break;
	}
	case OPCODE_LD_RAW:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//LD_RAW\n");
#endif

		TranslateShaderStorageLoad(psInst);
		break;
	}

	case OPCODE_ATOMIC_CMP_STORE:
	case OPCODE_IMM_ATOMIC_AND:
	case OPCODE_ATOMIC_AND:
	case OPCODE_IMM_ATOMIC_IADD:
	case OPCODE_ATOMIC_IADD:
	case OPCODE_ATOMIC_OR:
	case OPCODE_ATOMIC_XOR:
	case OPCODE_ATOMIC_IMAX:
	case OPCODE_ATOMIC_IMIN:
	case OPCODE_ATOMIC_UMAX:
	case OPCODE_ATOMIC_UMIN:
	case OPCODE_IMM_ATOMIC_IMAX:
	case OPCODE_IMM_ATOMIC_IMIN:
	case OPCODE_IMM_ATOMIC_UMAX:
	case OPCODE_IMM_ATOMIC_UMIN:
	case OPCODE_IMM_ATOMIC_OR:
	case OPCODE_IMM_ATOMIC_XOR:
	case OPCODE_IMM_ATOMIC_EXCH:
	case OPCODE_IMM_ATOMIC_CMP_EXCH:
	{
		TranslateAtomicMemOp(psInst);
		break;
	}
	case OPCODE_UBFE:
	case OPCODE_IBFE:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		if (psInst->eOpcode == OPCODE_UBFE)
			bcatcstr(glsl, "//OPCODE_UBFE\n");
		else
			bcatcstr(glsl, "//OPCODE_IBFE\n");
#endif

		bool isUBFE = psInst->eOpcode == OPCODE_UBFE;
		bool isScalar = psInst->asOperands[0].GetNumSwizzleElements() == 1;

		if (isUBFE)
		{
			if (isScalar)
			{
				DeclareExtraFunction("UBFE", "\
uint bitFieldExtractU(uint width, uint offset, uint src);\n\
uint bitFieldExtractU(uint width, uint offset, uint src)\n\
{\n\
	bool isWidthZero = (width == 0);\n\
	bool needsClamp = ((width + offset) < 32);\n\
	uint clampVersion = src << (32-(width+offset));\n\
	clampVersion = clampVersion >> (32 - width);\n\
	uint simpleVersion = src >> offset;\n\
	uint res = select(simpleVersion, clampVersion, needsClamp);\n\
	return select(res, (uint)0, isWidthZero);\n\
}; ");
			}
			else
			{
				DeclareExtraFunction("UBFEV", "\
template <int N> vec<uint, N> bitFieldExtractU(const vec<uint, N> width, const vec<uint, N> offset, const vec<uint, N> src)\n\
{\n\
	vec<bool, N> isWidthZero = (width == 0);\n\
	vec<bool, N> needsClamp = ((width + offset) < 32);\n\
	vec<uint, N> clampVersion = src << (32-(width+offset));\n\
	clampVersion = clampVersion >> (32 - width);\n\
	vec<uint, N> simpleVersion = src >> offset;\n\
	vec<uint, N> res = select(simpleVersion, clampVersion, needsClamp);\n\
	return select(res, vec<uint, N>(0), isWidthZero);\n\
}; ");
			}
		}
		else
		{
			if (isScalar)
			{
				DeclareExtraFunction("IBFE", "\
template int bitFieldExtractI(uint width, uint offset, int src)\n\
{\n\
	bool isWidthZero = (width == 0);\n\
	bool needsClamp = ((width + offset) < 32);\n\
	int clampVersion = src << (32-(width+offset));\n\
	clampVersion = clampVersion >> (32 - width);\n\
	int simpleVersion = src >> offset;\n\
	int res = select(simpleVersion, clampVersion, needsClamp);\n\
	return select(res, (int)0, isWidthZero);\n\
}; ");
			}
			else
			{
				DeclareExtraFunction("IBFEV", "\
template <int N> vec<int, N> bitFieldExtractI(const vec<uint, N> width, const vec<uint, N> offset, const vec<int, N> src)\n\
{\n\
	vec<bool, N> isWidthZero = (width == 0);\n\
	vec<bool, N> needsClamp = ((width + offset) < 32);\n\
	vec<int, N> clampVersion = src << (32-(width+offset));\n\
	clampVersion = clampVersion >> (32 - width);\n\
	vec<int, N> simpleVersion = src >> offset;\n\
	vec<int, N> res = select(simpleVersion, clampVersion, needsClamp);\n\
	return select(res, vec<int, N>(0), isWidthZero);\n\
}; ");
			}
		}
		psContext->AddIndentation();

		uint32_t destMask = psInst->asOperands[0].GetAccessMask();
		AddAssignToDest(&psInst->asOperands[0], isUBFE ? SVT_UINT : SVT_INT, psInst->asOperands[0].GetNumSwizzleElements(), &numParenthesis);
		bcatcstr(glsl, "bitFieldExtract");
		bcatcstr(glsl, isUBFE ? "U" : "I");
		bcatcstr(glsl, "(");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, destMask);
		bcatcstr(glsl, ", ");
		glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_UNSIGNED_INTEGER, destMask);
		bcatcstr(glsl, ", ");
		glsl << TranslateOperand(&psInst->asOperands[3], isUBFE ? TO_FLAG_UNSIGNED_INTEGER : TO_FLAG_INTEGER, destMask);
		bcatcstr(glsl, ")");
		AddAssignPrologue(numParenthesis);
		break;
	}
	case OPCODE_RCP:
	{
		const uint32_t destElemCount = psInst->asOperands[0].GetNumSwizzleElements();
		const uint32_t srcElemCount = psInst->asOperands[1].GetNumSwizzleElements();
		int numParenthesis = 0;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//RCP\n");
#endif
		psContext->AddIndentation();

		SHADER_VARIABLE_TYPE dstType = psInst->asOperands[0].GetDataType(psContext);
		SHADER_VARIABLE_TYPE srcType = psInst->asOperands[1].GetDataType(psContext);

		uint32_t typeFlags = TO_FLAG_NONE;
		if (dstType == SVT_FLOAT16 && srcType == SVT_FLOAT16)
		{
			typeFlags = TO_FLAG_FORCE_HALF;
		}
		else
			srcType = SVT_FLOAT;

		AddAssignToDest(&psInst->asOperands[0], srcType, srcElemCount, &numParenthesis);
		bcatcstr(glsl, GetConstructorForTypeMetal(srcType, destElemCount));
		bcatcstr(glsl, "(1.0) / ");
		bcatcstr(glsl, GetConstructorForTypeMetal(srcType, destElemCount));
		bcatcstr(glsl, "(");
		numParenthesis++;
		glsl << TranslateOperand(&psInst->asOperands[1], typeFlags, psInst->asOperands[0].GetAccessMask());
		AddAssignPrologue(numParenthesis);
		break;
	}
  case OPCODE_F16TOF32:
  {
      const uint32_t accessMask = psInst->asOperands[0].GetAccessMask();
#ifdef _DEBUG
      psContext->AddIndentation();
      bcatcstr(glsl, "//F16TOF32\n");
#endif
      for (int i = 0; i < 4; ++i)
      {
          const int bit = 1 << i;
          if (accessMask & bit)
          {
              int p = 0;
              AddOpAssignToDestWithMask(&psInst->asOperands[0], SVT_FLOAT, 1, "=", &p, bit);
              bcatcstr(glsl, "as_type<half>(ushort(");
              glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, bit);
              for (; p >0; --p)
                  bcatcstr(glsl, ")");
              bcatcstr(glsl, "));\n");
          }
      }
      break;
  }
  case OPCODE_F32TOF16:
  {
      const uint32_t accessMask = psInst->asOperands[0].GetAccessMask();
      const uint32_t destElemCount = psInst->asOperands[0].GetNumSwizzleElements();
      const uint32_t s0ElemCount = psInst->asOperands[1].GetNumSwizzleElements();
#ifdef _DEBUG
      psContext->AddIndentation();
      bcatcstr(glsl, "//F32TOF16\n");
#endif
      for (int i = 0; i < 4; ++i)
      {
          const int bit = 1 << i;
          if (accessMask & bit)
          {
              int p = 0;
              AddOpAssignToDestWithMask(&psInst->asOperands[0], SVT_UINT, 1, "=", &p, bit);
              bcatcstr(glsl, "as_type<ushort>(half(");
              glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_NONE, bit);
              for (; p > 0; --p)
                  bcatcstr(glsl, ")");
              bcatcstr(glsl, "));\n");
          }
      }
      break;
  }


	case OPCODE_INEG:
	{
		int numParenthesis = 0;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//INEG\n");
#endif
		//dest = 0 - src0
		psContext->AddIndentation();

		AddAssignToDest(&psInst->asOperands[0], SVT_INT, psInst->asOperands[1].GetNumSwizzleElements(), &numParenthesis);

		bcatcstr(glsl, "0 - ");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, psInst->asOperands[0].GetAccessMask());
		AddAssignPrologue(numParenthesis);
		break;
	}
	case OPCODE_DERIV_RTX_COARSE:
	case OPCODE_DERIV_RTX_FINE:
	case OPCODE_DERIV_RTX:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//DERIV_RTX\n");
#endif
		CallHelper1("dfdx", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_DERIV_RTY_COARSE:
	case OPCODE_DERIV_RTY_FINE:
	case OPCODE_DERIV_RTY:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//DERIV_RTY\n");
#endif
		CallHelper1("dfdy", psInst, 0, 1, 1);
		break;
	}
	case OPCODE_LRP:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//LRP\n");
#endif
		CallHelper3("mix", psInst, 0, 2, 3, 1, 1);
		break;
	}
	case OPCODE_DP2ADD:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//DP2ADD\n");
#endif
		psContext->AddIndentation();
		bool isFP16 = false;
		if (psInst->asOperands[0].GetDataType(psContext) == SVT_FLOAT16
			&& psInst->asOperands[1].GetDataType(psContext) == SVT_FLOAT16
			&& psInst->asOperands[2].GetDataType(psContext) == SVT_FLOAT16
			&& psInst->asOperands[2].GetDataType(psContext) == SVT_FLOAT16)
			isFP16 = true;
		int parenthesis = 0;
		AddAssignToDest(&psInst->asOperands[0], isFP16 ? SVT_FLOAT16 : SVT_FLOAT, 2, &parenthesis);

		uint32_t flags = TO_AUTO_EXPAND_TO_VEC2;
		flags |= isFP16 ? TO_FLAG_FORCE_HALF : TO_AUTO_BITCAST_TO_FLOAT;

		bcatcstr(glsl, "dot(");
		glsl << TranslateOperand(&psInst->asOperands[1], flags);
		bcatcstr(glsl, ", ");
		glsl << TranslateOperand(&psInst->asOperands[2], flags);
		bcatcstr(glsl, ") + ");
		glsl << TranslateOperand(&psInst->asOperands[3], flags);
		AddAssignPrologue(parenthesis);
		break;
	}
	case OPCODE_POW:
	{
		// TODO Check POW opcode whether it actually needs the abs
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//POW\n");
#endif
		psContext->AddIndentation();
		glsl << TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
		bcatcstr(glsl, " = powr(abs(");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_NONE);
		bcatcstr(glsl, "), ");
		glsl << TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
		bcatcstr(glsl, ");\n");
		break;
	}

	case OPCODE_IMM_ATOMIC_ALLOC:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_ALLOC\n");
#endif
		psContext->AddIndentation();
		AddAssignToDest(&psInst->asOperands[0], SVT_UINT, 1, &numParenthesis);
		bcatcstr(glsl, "atomic_fetch_add_explicit(");
		glsl << ResourceName(RGROUP_UAV, psInst->asOperands[1].ui32RegisterNumber);
		bcatcstr(glsl, "_counter, 1, memory_order::memory_order_relaxed)");
		AddAssignPrologue(numParenthesis);
		break;
	}
	case OPCODE_IMM_ATOMIC_CONSUME:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//IMM_ATOMIC_CONSUME\n");
#endif
		psContext->AddIndentation();
		AddAssignToDest(&psInst->asOperands[0], SVT_UINT, 1, &numParenthesis);
		bcatcstr(glsl, "atomic_fetch_sub_explicit(");
		glsl << ResourceName(RGROUP_UAV, psInst->asOperands[1].ui32RegisterNumber);
		// Metal atomic sub returns previous value. Therefore minus one here to get the correct data index.
		bcatcstr(glsl, "_counter, 1, memory_order::memory_order_relaxed) - 1");
		AddAssignPrologue(numParenthesis);
		break;
	}

	case OPCODE_NOT:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//INOT\n");
#endif
		psContext->AddIndentation();
		AddAssignToDest(&psInst->asOperands[0], SVT_INT, psInst->asOperands[1].GetNumSwizzleElements(), &numParenthesis);

		bcatcstr(glsl, "~");
		glsl << TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, psInst->asOperands[0].GetAccessMask());
		AddAssignPrologue(numParenthesis);
		break;
	}
	case OPCODE_XOR:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//XOR\n");
#endif

		CallBinaryOp("^", psInst, 0, 1, 2, SVT_UINT);
		break;
	}
	case OPCODE_RESINFO:
	{

		uint32_t destElemCount = psInst->asOperands[0].GetNumSwizzleElements();
		uint32_t destElem;
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//RESINFO\n");
#endif

		for (destElem = 0; destElem < destElemCount; ++destElem)
		{
			GetResInfoData(psInst, psInst->asOperands[2].aui32Swizzle[destElem], destElem);
		}

		break;
	}

	case OPCODE_BUFINFO:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//BUFINFO\n");
#endif
		psContext->m_Reflection.OnDiagnostics("Metal shading language does not support buffer size query from shader. Pass the size to shader as const instead.\n", 0, false); // TODO: change this into error after modifying gfx-test 450
		break;
	}

	case OPCODE_DMAX:
	case OPCODE_DMIN:
	case OPCODE_DMUL:
	case OPCODE_DEQ:
	case OPCODE_DGE:
	case OPCODE_DLT:
	case OPCODE_DNE:
	case OPCODE_DMOV:
	case OPCODE_DMOVC:
	case OPCODE_DTOF:
	case OPCODE_FTOD:
	case OPCODE_DDIV:
	case OPCODE_DFMA:
	case OPCODE_DRCP:
	case OPCODE_MSAD:
	case OPCODE_DTOI:
	case OPCODE_DTOU:
	case OPCODE_ITOD:
	case OPCODE_UTOD:
	default:
	{
			   ASSERT(0);
			   break;
	}
	}

	if (psInst->bSaturate) //Saturate is only for floating point data (float opcodes or MOV)
	{
		int dstCount = psInst->asOperands[0].GetNumSwizzleElements();
		psContext->AddIndentation();
		bool isFP16 = false;
		if (psInst->asOperands[0].GetDataType(psContext) == SVT_FLOAT16)
			isFP16 = true;
		AddAssignToDest(&psInst->asOperands[0], isFP16 ? SVT_FLOAT16 : SVT_FLOAT, dstCount, &numParenthesis);
		bcatcstr(glsl, "clamp(");

		glsl << TranslateOperand(&psInst->asOperands[0], isFP16 ? TO_FLAG_FORCE_HALF : TO_AUTO_BITCAST_TO_FLOAT);
		if(isFP16)
			bcatcstr(glsl, ", 0.0h, 1.0h)");
		else
			bcatcstr(glsl, ", 0.0f, 1.0f)");
		AddAssignPrologue(numParenthesis);
	}
}
