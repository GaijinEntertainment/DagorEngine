#include "internal_includes/toGLSLOperand.h"
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
#include "internal_includes/toGLSL.h"

//#undef _DEBUG
//#define _DEBUG 1


using namespace HLSLcc;


// This function prints out the destination name, possible destination writemask, assignment operator
// and any possible conversions needed based on the eSrcType+ui32SrcElementCount (type and size of data expected to be coming in)
// As an output, pNeedsParenthesis will be filled with the amount of closing parenthesis needed
// and pSrcCount will be filled with the number of components expected
// ui32CompMask can be used to only write to 1 or more components (used by MOVC)
void ToGLSL::AddOpAssignToDestWithMask(const Operand* psDest,
	SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, const char *szAssignmentOp, int *pNeedsParenthesis, uint32_t ui32CompMask)
{
	uint32_t ui32DestElementCount = psDest->GetNumSwizzleElements(ui32CompMask);
	bstring glsl = *psContext->currentGLSLString;
	SHADER_VARIABLE_TYPE eDestDataType = psDest->GetDataType(psContext);
	ASSERT(pNeedsParenthesis != NULL);

	*pNeedsParenthesis = 0;

	TranslateOperand(psDest, TO_FLAG_DESTINATION, ui32CompMask);

	// Simple path: types match.
	if (DoAssignmentDataTypesMatch(eDestDataType, eSrcType))
	{
		// Cover cases where the HLSL language expects the rest of the components to be default-filled
		// eg. MOV r0, c0.x => Temp[0] = vec4(c0.x);
		if (ui32DestElementCount > ui32SrcElementCount)
		{
			bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForTypeGLSL(eDestDataType, ui32DestElementCount, false));
			*pNeedsParenthesis = 1;
		}
		else
			bformata(glsl, " %s ", szAssignmentOp);
		return;
	}

	switch (eDestDataType)
	{
	case SVT_INT:
	case SVT_INT12:
	case SVT_INT16:
		// Bitcasts from lower precisions are ambiguous
		ASSERT(eSrcType != SVT_FLOAT10 && eSrcType != SVT_FLOAT16);
		if (eSrcType == SVT_FLOAT && psContext->psShader.ui32MajorVersion > 3)
		{
			bformata(glsl, " %s floatBitsToInt(", szAssignmentOp);
			// Cover cases where the HLSL language expects the rest of the components to be default-filled
			if (ui32DestElementCount > ui32SrcElementCount)
			{
				bformata(glsl, "%s(", GetConstructorForTypeGLSL(eSrcType, ui32DestElementCount, false));
				(*pNeedsParenthesis)++;
			}
		}
		else
			bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForTypeGLSL(eDestDataType, ui32DestElementCount, false));

		(*pNeedsParenthesis)++;
		break;
	case SVT_UINT:
	case SVT_UINT16:
		ASSERT(eSrcType != SVT_FLOAT10 && eSrcType != SVT_FLOAT16);
		if (eSrcType == SVT_FLOAT && psContext->psShader.ui32MajorVersion > 3)
		{
			bformata(glsl, " %s floatBitsToUint(", szAssignmentOp);
			// Cover cases where the HLSL language expects the rest of the components to be default-filled
			if (ui32DestElementCount > ui32SrcElementCount)
			{
				bformata(glsl, "%s(", GetConstructorForTypeGLSL(eSrcType, ui32DestElementCount, false));
				(*pNeedsParenthesis)++;
			}
		}
		else
			bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForTypeGLSL(eDestDataType, ui32DestElementCount, false));

		(*pNeedsParenthesis)++;
		break;

	case SVT_FLOAT:
	case SVT_FLOAT10:
	case SVT_FLOAT16:
		ASSERT(eSrcType != SVT_INT12 || (eSrcType != SVT_INT16 && eSrcType != SVT_UINT16));
		if (psContext->psShader.ui32MajorVersion > 3)
		{
			if (eSrcType == SVT_INT)
				bformata(glsl, " %s intBitsToFloat(", szAssignmentOp);
			else
				bformata(glsl, " %s uintBitsToFloat(", szAssignmentOp);
			// Cover cases where the HLSL language expects the rest of the components to be default-filled
			if (ui32DestElementCount > ui32SrcElementCount)
			{
				bformata(glsl, "%s(", GetConstructorForTypeGLSL(eSrcType, ui32DestElementCount, false));
				(*pNeedsParenthesis)++;
			}
		}
		else
			bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForTypeGLSL(eDestDataType, ui32DestElementCount, false));

		(*pNeedsParenthesis)++;
		break;
	default:
		// TODO: Handle bools?
		ASSERT(0);
		break;
	}
	return;
}

void ToGLSL::AddAssignToDest(const Operand* psDest,
	SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, int* pNeedsParenthesis)
{
	AddOpAssignToDestWithMask(psDest, eSrcType, ui32SrcElementCount, "=", pNeedsParenthesis, OPERAND_4_COMPONENT_MASK_ALL);
}

void ToGLSL::AddAssignPrologue(int numParenthesis, bool isEmbedded /* = false*/)
{
	bstring glsl = *psContext->currentGLSLString;
	while (numParenthesis != 0)
	{
		bcatcstr(glsl, ")");
		numParenthesis--;
	}
	if(!isEmbedded)
		bcatcstr(glsl, ";\n");

}


void ToGLSL::AddComparison(Instruction* psInst, ComparisonType eType,
	uint32_t typeFlag)
{
	// Multiple cases to consider here:
	// For shader model <=3: all comparisons are floats
	// otherwise:
	// OPCODE_LT, _GT, _NE etc: inputs are floats, outputs UINT 0xffffffff or 0. typeflag: TO_FLAG_NONE
	// OPCODE_ILT, _IGT etc: comparisons are signed ints, outputs UINT 0xffffffff or 0 typeflag TO_FLAG_INTEGER
	// _ULT, UGT etc: inputs unsigned ints, outputs UINTs typeflag TO_FLAG_UNSIGNED_INTEGER
	//
	// Additional complexity: if dest swizzle element count is 1, we can use normal comparison operators, otherwise glsl intrinsics.


	bstring glsl = *psContext->currentGLSLString;
	const uint32_t destElemCount = psInst->asOperands[0].GetNumSwizzleElements();
	const uint32_t s0ElemCount = psInst->asOperands[1].GetNumSwizzleElements();
	const uint32_t s1ElemCount = psInst->asOperands[2].GetNumSwizzleElements();
	int isBoolDest = psInst->asOperands[0].GetDataType(psContext) == SVT_BOOL;

	int floatResult = 0;

	ASSERT(s0ElemCount == s1ElemCount || s1ElemCount == 1 || s0ElemCount == 1);
	if (s0ElemCount != s1ElemCount)
	{
		// Set the proper auto-expand flag is either argument is scalar
		typeFlag |= (TO_AUTO_EXPAND_TO_VEC2 << (std::max(s0ElemCount, s1ElemCount) - 2));
	}

	if (psContext->psShader.ui32MajorVersion < 4)
	{
		floatResult = 1;
	}

	if (destElemCount > 1)
	{
		const char* glslOpcode[] = {
			"equal",
			"lessThan",
			"greaterThanEqual",
			"notEqual",
		};

		int needsParenthesis = 0;
		psContext->AddIndentation();
		if (isBoolDest)
		{
			TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION | TO_FLAG_BOOL);
			bcatcstr(glsl, " = ");
		}
		else
		{
			AddAssignToDest(&psInst->asOperands[0], floatResult ? SVT_FLOAT : SVT_UINT, destElemCount, &needsParenthesis);

			bcatcstr(glsl, GetConstructorForTypeGLSL(floatResult ? SVT_FLOAT : SVT_UINT, destElemCount, false));
			bcatcstr(glsl, "(");
		}
		bformata(glsl, "%s(", glslOpcode[eType]);
		TranslateOperand(&psInst->asOperands[1], typeFlag);
		bcatcstr(glsl, ", ");
		TranslateOperand(&psInst->asOperands[2], typeFlag);
		bcatcstr(glsl, ")");
		TranslateOperandSwizzle(psContext, &psInst->asOperands[0], 0);
		if (!isBoolDest)
		{
			bcatcstr(glsl, ")");
			if (!floatResult)
			{
				bcatcstr(glsl, " * 0xFFFFFFFFu");
			}
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

		const bool workaroundAdrenoBugs = psContext->psShader.eTargetLanguage == LANG_ES_300;

		if (workaroundAdrenoBugs)
		{
			// Workarounds for bug cases 777617, 735299, 776827
			bcatcstr(glsl, "#ifdef UNITY_ADRENO_ES3\n");

			int needsParenthesis = 0;
			psContext->AddIndentation();
			if (isBoolDest)
			{
				TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION | TO_FLAG_BOOL);
				bcatcstr(glsl, " = !!(");
				needsParenthesis += 1;
				TranslateOperand(&psInst->asOperands[1], typeFlag);
				bformata(glsl, "%s", glslOpcode[eType]);
				TranslateOperand(&psInst->asOperands[2], typeFlag);
				AddAssignPrologue(needsParenthesis);
			}
			else
			{
				bcatcstr(glsl, "{ bool cond = ");
				TranslateOperand(&psInst->asOperands[1], typeFlag);
				bformata(glsl, "%s", glslOpcode[eType]);
				TranslateOperand(&psInst->asOperands[2], typeFlag);
				bcatcstr(glsl, "; ");
				AddAssignToDest(&psInst->asOperands[0], floatResult ? SVT_FLOAT : SVT_UINT, destElemCount, &needsParenthesis);
				bcatcstr(glsl, "!!cond ? ");
				if (floatResult)
					bcatcstr(glsl, "1.0 : 0.0");
				else
					bcatcstr(glsl, "0xFFFFFFFFu : uint(0u)"); // Adreno can't handle 0u.
				AddAssignPrologue(needsParenthesis, true);
				bcatcstr(glsl, "; }\n");
			}

			bcatcstr(glsl, "#else\n");
		}

		int needsParenthesis = 0;
		psContext->AddIndentation();
		if (isBoolDest)
		{
			TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION | TO_FLAG_BOOL);
			bcatcstr(glsl, " = ");
		}
		else
		{
			AddAssignToDest(&psInst->asOperands[0], floatResult ? SVT_FLOAT : SVT_UINT, destElemCount, &needsParenthesis);
			bcatcstr(glsl, "(");
		}
		TranslateOperand(&psInst->asOperands[1], typeFlag);
		bformata(glsl, "%s", glslOpcode[eType]);
		TranslateOperand(&psInst->asOperands[2], typeFlag);
		if (!isBoolDest)
		{
			if (floatResult)
			{
				bcatcstr(glsl, ") ? 1.0 : 0.0");
			}
			else
			{
				bcatcstr(glsl, ") ? 0xFFFFFFFFu : uint(0u)"); // Adreno can't handle 0u.
			}
		}
		AddAssignPrologue(needsParenthesis);

		if (workaroundAdrenoBugs)
			bcatcstr(glsl, "#endif\n");
	}
}


void ToGLSL::AddMOVBinaryOp(const Operand *pDest, Operand *pSrc, bool isEmbedded /* = false*/)
{
	int numParenthesis = 0;
	int srcSwizzleCount = pSrc->GetNumSwizzleElements();
	uint32_t writeMask = pDest->GetAccessMask();

	const SHADER_VARIABLE_TYPE eSrcType = pSrc->GetDataType(psContext, pDest->GetDataType(psContext));
	uint32_t flags = SVTTypeToFlag(eSrcType);

	AddAssignToDest(pDest, eSrcType, srcSwizzleCount, &numParenthesis);
	TranslateOperand(pSrc, flags, writeMask);

	AddAssignPrologue(numParenthesis, isEmbedded);
}

void ToGLSL::AddMOVCBinaryOp(const Operand *pDest, const Operand *src0, Operand *src1, Operand *src2, int tempMask, const char* tempName /* = nullptr */)
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

	if (!tempName)
		tempName = "helper_temp";

	/* Single-component conditional variable (src0) */
	if (destElemCount == 1)
	{
		int numParenthesis = 0;
		SHADER_VARIABLE_TYPE s0Type = src0->GetDataType(psContext);
		psContext->AddIndentation();
		AddAssignToDest(pDest, eDestType, destElemCount, &numParenthesis);
		bcatcstr(glsl, "(");
		if (s0Type == SVT_UINT || s0Type == SVT_UINT16)
			TranslateOperand(src0, TO_AUTO_BITCAST_TO_UINT, OPERAND_4_COMPONENT_MASK_X, (tempMask & 1) ? tempName : NULL);
		else if (s0Type == SVT_BOOL)
			TranslateOperand(src0, TO_FLAG_BOOL, OPERAND_4_COMPONENT_MASK_X, (tempMask & 1) ? tempName : NULL);
		else
			TranslateOperand(src0, TO_AUTO_BITCAST_TO_INT, OPERAND_4_COMPONENT_MASK_X, (tempMask & 1) ? tempName : NULL);

		if (psContext->psShader.ui32MajorVersion < 4)
		{
			//cmp opcode uses >= 0
			bcatcstr(glsl, " >= 0) ? ");
		}
		else
		{
			if (s0Type == SVT_UINT || s0Type == SVT_UINT16)
				bcatcstr(glsl, " != uint(0u)) ? "); // Adreno doesn't understand 0u.
			else if (s0Type == SVT_BOOL)
				bcatcstr(glsl, ") ? ");
			else
				bcatcstr(glsl, " != 0) ? ");
		}

		if (s1ElemCount == 1 && destElemCount > 1)
			TranslateOperand(src1, SVTTypeToFlag(eDestType) | ElemCountToAutoExpandFlag(destElemCount), 15, (tempMask & 2) ? tempName : NULL);
		else
			TranslateOperand(src1, SVTTypeToFlag(eDestType), destWriteMask, (tempMask & 2) ? tempName : NULL);

		bcatcstr(glsl, " : ");
		if (s2ElemCount == 1 && destElemCount > 1)
			TranslateOperand(src2, SVTTypeToFlag(eDestType) | ElemCountToAutoExpandFlag(destElemCount), 15, (tempMask & 4) ? tempName : NULL);
		else
			TranslateOperand(src2, SVTTypeToFlag(eDestType), destWriteMask, (tempMask & 4) ? tempName : NULL);

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
				TranslateOperand(src0, TO_FLAG_BOOL, 1 << srcElem, (tempMask & 1) ? tempName : NULL);
				bcatcstr(glsl, ") ? ");
			}
			else
			{
				TranslateOperand(src0, TO_AUTO_BITCAST_TO_INT, 1 << srcElem, (tempMask & 1) ? tempName : NULL);

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

			TranslateOperand(src1, SVTTypeToFlag(eDestType), 1 << srcElem, (tempMask & 2) ? tempName : NULL);
			bcatcstr(glsl, " : ");
			TranslateOperand(src2, SVTTypeToFlag(eDestType), 1 << srcElem, (tempMask & 4) ? tempName : NULL);

			AddAssignPrologue(numParenthesis);
		}
	}
}

void ToGLSL::CallBinaryOp(const char* name, Instruction* psInst,
	int dest, int src0, int src1, SHADER_VARIABLE_TYPE eDataType, bool isEmbedded /* = false*/, int temp_override /*= -1*/)
{
	uint32_t ui32Flags = SVTTypeToFlag(eDataType);
	bstring glsl = *psContext->currentGLSLString;
	uint32_t destMask = psInst->asOperands[dest].GetAccessMask();
	uint32_t src1SwizCount = psInst->asOperands[src1].GetNumSwizzleElements(destMask);
	uint32_t src0SwizCount = psInst->asOperands[src0].GetNumSwizzleElements(destMask);
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();
	uint32_t src0AccessMask = psInst->asOperands[src0].GetAccessMask();
	uint32_t src1AccessMask = psInst->asOperands[src1].GetAccessMask();
	uint32_t src0AccessCount = GetNumberBitsSet(src0AccessMask);
	uint32_t src1AccessCount = GetNumberBitsSet(src1AccessMask);
	int needsParenthesis = 0;

	if (src1SwizCount != src0SwizCount)
	{
		uint32_t maxElems = std::max(src1SwizCount, src0SwizCount);
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	if(!isEmbedded)
		psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], eDataType, dstSwizCount, &needsParenthesis);

	// Horrible Adreno bug workaround:
	// All pre-ES3.1 Adreno GLES3.0 drivers fail in cases like this:
	// vec4 a.xyz = b.xyz + c.yzw;
	// Attempt to detect this and fall back to component-wise binary op.
	if ( (psContext->psShader.eTargetLanguage == LANG_ES_300) &&
		 ((src0AccessCount > 1 && !(src0AccessMask & OPERAND_4_COMPONENT_MASK_X)) || (src1AccessCount > 1 && !(src1AccessMask & OPERAND_4_COMPONENT_MASK_X))))
	{
		uint32_t i;
		int firstPrinted = 0;
		bcatcstr(glsl, GetConstructorForTypeGLSL(eDataType, dstSwizCount, false));
		bcatcstr(glsl, "(");
		for (i = 0; i < 4; i++)
		{
			if (!(destMask & (1 << i)))
				continue;

			if (firstPrinted != 0)
				bcatcstr(glsl, ", ");
			else
				firstPrinted = 1;

			// Remove the auto expand flags
			ui32Flags &= ~(TO_AUTO_EXPAND_TO_VEC2 | TO_AUTO_EXPAND_TO_VEC3 | TO_AUTO_EXPAND_TO_VEC4);

			TranslateOperand(&psInst->asOperands[src0], ui32Flags, 1 << i, src0 == temp_override ? "helper_temp" : NULL);
			bformata(glsl, " %s ", name);
			TranslateOperand(&psInst->asOperands[src1], ui32Flags, 1 << i, src1 == temp_override ? "helper_temp" : NULL);
		}
		bcatcstr(glsl, ")");
	}
	else
	{
		TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask, src0 == temp_override ? "helper_temp" : NULL);
		bformata(glsl, " %s ", name);
		TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask, src1 == temp_override ? "helper_temp" : NULL);
	}
	AddAssignPrologue(needsParenthesis, isEmbedded);
}

void ToGLSL::CallTernaryOp(const char* op1, const char* op2, Instruction* psInst,
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

	if (src1SwizCount != src0SwizCount || src2SwizCount != src0SwizCount)
	{
		uint32_t maxElems = std::max(src2SwizCount, std::max(src1SwizCount, src0SwizCount));
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], TypeFlagsToSVTType(dataType), dstSwizCount, &numParenthesis);

	TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bformata(glsl, " %s ", op1);
	TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);
	bformata(glsl, " %s ", op2);
	TranslateOperand(&psInst->asOperands[src2], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToGLSL::CallHelper3(const char* name, Instruction* psInst,
	int dest, int src0, int src1, int src2, int paramsShouldFollowWriteMask, uint32_t ui32Flags)
{
	ui32Flags |= TO_AUTO_BITCAST_TO_FLOAT;
	bstring glsl = *psContext->currentGLSLString;
	uint32_t destMask = paramsShouldFollowWriteMask ? psInst->asOperands[dest].GetAccessMask() : OPERAND_4_COMPONENT_MASK_ALL;
	uint32_t src2SwizCount = psInst->asOperands[src2].GetNumSwizzleElements(destMask);
	uint32_t src1SwizCount = psInst->asOperands[src1].GetNumSwizzleElements(destMask);
	uint32_t src0SwizCount = psInst->asOperands[src0].GetNumSwizzleElements(destMask);
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();
	int numParenthesis = 0;

	if ((src1SwizCount != src0SwizCount || src2SwizCount != src0SwizCount) && paramsShouldFollowWriteMask)
	{
		uint32_t maxElems = std::max(src2SwizCount, std::max(src1SwizCount, src0SwizCount));
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], SVT_FLOAT, dstSwizCount, &numParenthesis);

	bformata(glsl, "%s(", name);
	numParenthesis++;
	TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	TranslateOperand(&psInst->asOperands[src2], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToGLSL::CallHelper2(const char* name, Instruction* psInst,
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

	if ((src1SwizCount != src0SwizCount) && paramsShouldFollowWriteMask)
	{
		uint32_t maxElems = std::max(src1SwizCount, src0SwizCount);
		ui32Flags |= (TO_AUTO_EXPAND_TO_VEC2 << (maxElems - 2));
	}

	psContext->AddIndentation();
	AddAssignToDest(&psInst->asOperands[dest], SVT_FLOAT, isDotProduct ? 1 : dstSwizCount, &numParenthesis);

	bformata(glsl, "%s(", name);
	numParenthesis++;

	TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);

	AddAssignPrologue(numParenthesis);
}

void ToGLSL::CallHelper2Int(const char* name, Instruction* psInst,
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
	TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToGLSL::CallHelper2UInt(const char* name, Instruction* psInst,
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
	TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	bcatcstr(glsl, ", ");
	TranslateOperand(&psInst->asOperands[src1], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToGLSL::CallHelper1(const char* name, Instruction* psInst,
	int dest, int src0, int paramsShouldFollowWriteMask)
{
	uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT;
	bstring glsl = *psContext->currentGLSLString;
	uint32_t dstSwizCount = psInst->asOperands[dest].GetNumSwizzleElements();
	uint32_t destMask = paramsShouldFollowWriteMask ? psInst->asOperands[dest].GetAccessMask() : OPERAND_4_COMPONENT_MASK_ALL;
	int numParenthesis = 0;

	psContext->AddIndentation();

	AddAssignToDest(&psInst->asOperands[dest], SVT_FLOAT, dstSwizCount, &numParenthesis);

	bformata(glsl, "%s(", name);
	numParenthesis++;
	TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

//Result is an int.
void ToGLSL::CallHelper1Int(
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
	TranslateOperand(&psInst->asOperands[src0], ui32Flags, destMask);
	AddAssignPrologue(numParenthesis);
}

void ToGLSL::TranslateTexelFetch(
	Instruction* psInst,
	const ResourceBinding* psBinding,
	bstring glsl)
{
	int numParenthesis = 0;
	psContext->AddIndentation();
	AddAssignToDest(&psInst->asOperands[0], TypeFlagsToSVTType(ResourceReturnTypeToFlag(psBinding->ui32ReturnType)), 4, &numParenthesis);
	bcatcstr(glsl, "texelFetch(");

	switch (psBinding->eDimension)
	{
	case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
	case REFLECT_RESOURCE_DIMENSION_BUFFER:
	{
											  TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
											  bcatcstr(glsl, ", ");
											  TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
											  if (psBinding->eDimension != REFLECT_RESOURCE_DIMENSION_BUFFER)
											  {
												  bcatcstr(glsl, ", "); // Buffers don't have LOD
												  TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
											  }
											  bcatcstr(glsl, ")");
											  break;
	}
	case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
	case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
	{
												 TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
												 bcatcstr(glsl, ", ");
												 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
												 bcatcstr(glsl, ", ");
												 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
												 bcatcstr(glsl, ")");
												 break;
	}
	case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
	case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
	{
													  TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
													  bcatcstr(glsl, ", ");
													  TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
													  bcatcstr(glsl, ", ");
													  TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
													  bcatcstr(glsl, ")");
													  break;
	}
	case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS: // TODO does this make any sense at all?
	{
													 ASSERT(psInst->eOpcode == OPCODE_LD_MS);
													 TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
													 bcatcstr(glsl, ", ");
													 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
													 bcatcstr(glsl, ", ");
													 TranslateOperand(&psInst->asOperands[3], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
													 bcatcstr(glsl, ")");
													 break;
	}
	case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
	{
														ASSERT(psInst->eOpcode == OPCODE_LD_MS);
														TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
														bcatcstr(glsl, ", ");
														TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
														bcatcstr(glsl, ", ");
														TranslateOperand(&psInst->asOperands[3], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
														bcatcstr(glsl, ")");
														break;
	}
	case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
	case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
	case REFLECT_RESOURCE_DIMENSION_BUFFEREX:
	default:
	{
			   ASSERT(0);
			   break;
	}
	}

	TranslateOperandSwizzleWithMask(psContext, &psInst->asOperands[2], psInst->asOperands[0].GetAccessMask(), 0);
	AddAssignPrologue(numParenthesis);
}

void ToGLSL::TranslateTexelFetchOffset(
	Instruction* psInst,
	const ResourceBinding* psBinding,
	bstring glsl)
{
	int numParenthesis = 0;
	psContext->AddIndentation();
	AddAssignToDest(&psInst->asOperands[0], TypeFlagsToSVTType(ResourceReturnTypeToFlag(psBinding->ui32ReturnType)), 4, &numParenthesis);

	bcatcstr(glsl, "texelFetchOffset(");

	switch (psBinding->eDimension)
	{
	case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
	{
												 TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
												 bcatcstr(glsl, ", ");
												 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
												 bformata(glsl, ", ");
												 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
												 bformata(glsl, ", %d)", psInst->iUAddrOffset);
												 break;
	}
	case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
	{
													  TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
													  bcatcstr(glsl, ", ");
													  TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
													  bformata(glsl, ", ");
													  TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
													  bformata(glsl, ", ivec2(%d, %d))",
														  psInst->iUAddrOffset,
														  psInst->iVAddrOffset);
													  break;
	}
	case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
	{
												 TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
												 bcatcstr(glsl, ", ");
												 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
												 bformata(glsl, ", ");
												 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
												 bformata(glsl, ", ivec3(%d, %d, %d))",
													 psInst->iUAddrOffset,
													 psInst->iVAddrOffset,
													 psInst->iWAddrOffset);
												 break;
	}
	case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
	{
												 TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
												 bcatcstr(glsl, ", ");
												 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
												 bformata(glsl, ", ");
												 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
												 bformata(glsl, ", ivec2(%d, %d))", psInst->iUAddrOffset, psInst->iVAddrOffset);
												 break;
	}
	case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
	{
													  TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
													  bcatcstr(glsl, ", ");
													  TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
													  bformata(glsl, ", ");
													  TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_A);
													  bformata(glsl, ", int(%d))", psInst->iUAddrOffset);
													  break;
	}
	case REFLECT_RESOURCE_DIMENSION_BUFFER:
	case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
	case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
	case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
	case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
	case REFLECT_RESOURCE_DIMENSION_BUFFEREX:
	default:
	{
			   ASSERT(0);
			   break;
	}
	}

	TranslateOperandSwizzleWithMask(psContext, &psInst->asOperands[2], psInst->asOperands[0].GetAccessMask(), 0);
	AddAssignPrologue(numParenthesis);
}


//Makes sure the texture coordinate swizzle is appropriate for the texture type.
//i.e. vecX for X-dimension texture.
//Currently supports floating point coord only, so not used for texelFetch.
void ToGLSL::TranslateTexCoord(
  const RESOURCE_DIMENSION eResDim,
  Operand* psTexCoordOperand,
  bool needs_array_index)
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
  {
    //Vec2 texcoord. Mask out the other components.
    opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
    flags |= TO_AUTO_EXPAND_TO_VEC2;
    break;
  }
  case RESOURCE_DIMENSION_TEXTURE1DARRAY:
  {
    if (needs_array_index)
    {
      //Vec2 texcoord. Mask out the other components.
      opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
      flags |= TO_AUTO_EXPAND_TO_VEC2;
    }
    else
    {
      //Vec1 texcoord. Mask out the other components.
      opMask = OPERAND_4_COMPONENT_MASK_X;
    }
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
    if (needs_array_index)
    {
      //Vec3 texcoord. Mask out the other components.
      opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z;
      flags |= TO_AUTO_EXPAND_TO_VEC3;
    }
    else
    {
      //Vec2 texcoord. Mask out the other components.
      opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
      flags |= TO_AUTO_EXPAND_TO_VEC2;
    }
    break;
  }
  case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
  {
    if (needs_array_index)
    {
      flags |= TO_AUTO_EXPAND_TO_VEC4;
    }
    else
    {
      //Vec3 texcoord. Mask out the other components.
      opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z;
      flags |= TO_AUTO_EXPAND_TO_VEC3;
    }
    break;
  }
  default:
  {
    ASSERT(0);
    break;
  }
  }

  //FIXME detect when integer coords are needed.
  TranslateOperand(psTexCoordOperand, flags, opMask);
}

void ToGLSL::GetResInfoData(Instruction* psInst, int index, int destElem)
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

	//[width, height, depth or array size, total-mip-count]
	if (index < 3)
	{
		//int dim = GetNumTextureDimensions(psInst->eResDim);
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
		bcatcstr(glsl, "(");
		if (dim < (index + 1))
		{
			bcatcstr(glsl, eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT ? "uint(0u)" : "0.0");
		}
		else
		{
			if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT)
				bformata(glsl, "uvec%d(", dim);
			else if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_RCPFLOAT)
				bformata(glsl, "vec%d(1.0) / vec%d(", dim, dim);
			else
				bformata(glsl, "vec%d(", dim);

			if (isUAV)
				bcatcstr(glsl, "imageSize(");
			else
				bcatcstr(glsl, "textureSize(");

			TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);

			if (!isUAV)
			{
				bcatcstr(glsl, ", ");
				TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER);
			}
			bcatcstr(glsl, "))");

			switch (index)
			{
			case 0:
				bcatcstr(glsl, ".x");
				break;
			case 1:
				bcatcstr(glsl, ".y");
				break;
			case 2:
				bcatcstr(glsl, ".z");
				break;
			}
		}

		bcatcstr(glsl, ")");
	}
	else
	{
		ASSERT(!isUAV);
		if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT)
			bcatcstr(glsl, "uint(");
		else
			bcatcstr(glsl, "float(");
		bcatcstr(glsl, "textureQueryLevels(");
		TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
		bcatcstr(glsl, "))");
	}
	AddAssignPrologue(numParenthesis);
}

void ToGLSL::TranslateTextureSample(Instruction* psInst,
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

	const char* funcName = "texture";
	const char* offset = "";
	const char* depthCmpCoordType = "";
	const char* gradSwizzle = "";

	uint32_t ui32NumOffsets = 0;

	const RESOURCE_DIMENSION eResDim = psContext->psShader.aeResourceDims[psSrcTex->ui32RegisterNumber];
	const int iHaveOverloadedTexFuncs = HaveOverloadedTextureFuncs(psContext->psShader.eTargetLanguage);
	const int useCombinedTextureSamplers = (psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS) ? 1 : 0;

	if (psInst->bAddressOffset)
	{
		offset = "Offset";
	}

	switch (eResDim)
	{
	case RESOURCE_DIMENSION_TEXTURE1D:
	{
		depthCmpCoordType = "vec2";
		gradSwizzle = ".x";
		ui32NumOffsets = 1;
		if (!iHaveOverloadedTexFuncs)
		{
			funcName = "texture1D";
			if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
			{
				funcName = "shadow1D";
			}
		}
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE2D:
	{
		depthCmpCoordType = "vec3";
		gradSwizzle = ".xy";
		ui32NumOffsets = 2;
		if (!iHaveOverloadedTexFuncs)
		{
			funcName = "texture2D";
			if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
			{
				funcName = "shadow2D";
			}
		}
		break;
	}
	case RESOURCE_DIMENSION_TEXTURECUBE:
	{
		depthCmpCoordType = "vec3";
		gradSwizzle = ".xyz";
		ui32NumOffsets = 3;
		if (!iHaveOverloadedTexFuncs)
		{
			funcName = "textureCube";
		}
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE3D:
	{
		depthCmpCoordType = "vec4";
		gradSwizzle = ".xyz";
		ui32NumOffsets = 3;
		if (!iHaveOverloadedTexFuncs)
		{
			funcName = "texture3D";
		}
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE1DARRAY:
	{
		depthCmpCoordType = "vec3";
		gradSwizzle = ".x";
		ui32NumOffsets = 1;
		break;
	}
	case RESOURCE_DIMENSION_TEXTURE2DARRAY:
	{
		depthCmpCoordType = "vec4";
		gradSwizzle = ".xy";
		ui32NumOffsets = 2;
		break;
	}
	case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
	{
		gradSwizzle = ".xyz";
		ui32NumOffsets = 3;
		break;
	}
	default:
	{
		ASSERT(0);
		break;
	}
	}

	if (ui32Flags & TEXSMP_FLAG_GATHER)
		funcName = "textureGather";

	uint32_t uniqueNameCounter;

	// In GLSL, for every texture sampling func overload, except for cubemap arrays, the 
	// depth compare reference value is given as the last component of the texture coord vector.
	// Cubemap array sampling as well as all the gather funcs have a separate parameter for it.
	// HLSL always provides the reference as a separate param.
	//
	// Here we create a temp texcoord var with the reference value embedded
	if ((ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE) && 
		eResDim != RESOURCE_DIMENSION_TEXTURECUBEARRAY &&
		!(ui32Flags & TEXSMP_FLAG_GATHER))
	{
		uniqueNameCounter = psContext->psShader.asPhases[psContext->currentPhase].m_NextTexCoordTemp++;
		psContext->AddIndentation();
		// Create a temp variable for the coordinate as Adrenos hate nonstandard swizzles in the texcoords
		bformata(glsl, "%s txVec%d = ", depthCmpCoordType, uniqueNameCounter);
		bformata(glsl, "%s(", depthCmpCoordType);
		TranslateTexCoord(eResDim, psDestAddr, true);
		bcatcstr(glsl, ",");
		// Last component is the reference
		TranslateOperand(psSrcRef, TO_AUTO_BITCAST_TO_FLOAT);
		bcatcstr(glsl, ");\n");
	}
	
	SHADER_VARIABLE_TYPE dataType = psContext->psShader.sInfo.GetTextureDataType(psSrcTex->ui32RegisterNumber);
	psContext->AddIndentation();
	AddAssignToDest(psDest, dataType, psSrcTex->GetNumSwizzleElements(), &numParenthesis);

	// Func name depending on the flags
	if (ui32Flags & (TEXSMP_FLAG_LOD | TEXSMP_FLAG_FIRSTLOD))
		bformata(glsl, "%sLod%s(", funcName, offset);
	else if (ui32Flags & TEXSMP_FLAG_GRAD)
		bformata(glsl, "%sGrad%s(", funcName, offset);
	else
		bformata(glsl, "%s%s(", funcName, offset);

	// Sampler name
	if (!useCombinedTextureSamplers)
		ResourceName(glsl, psContext, RGROUP_TEXTURE, psSrcTex->ui32RegisterNumber, ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE);
	else
		bcatcstr(glsl, TextureSamplerName(psContext->psShader.sInfo, psSrcTex->ui32RegisterNumber, psSrcSamp->ui32RegisterNumber, ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE).c_str());
	
	bcatcstr(glsl, ", ");

	// Texture coordinates, either from previously constructed temp
	// or straight from the psDestAddr operand
	if ((ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE) &&
		eResDim != RESOURCE_DIMENSION_TEXTURECUBEARRAY &&
		!(ui32Flags & TEXSMP_FLAG_GATHER))
		bformata(glsl, "txVec%d", uniqueNameCounter);
	else
		TranslateTexCoord(eResDim, psDestAddr, true);

	// If depth compare reference was not embedded to texcoord
	// then insert it here as a separate param
	if ((ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE) &&
		eResDim == RESOURCE_DIMENSION_TEXTURECUBEARRAY &&
		(ui32Flags & TEXSMP_FLAG_GATHER))
	{
		bcatcstr(glsl, ", ");
		TranslateOperand(psSrcRef, TO_AUTO_BITCAST_TO_FLOAT);
	}

	// Add LOD/grad parameters based on the flags
	if (ui32Flags & TEXSMP_FLAG_LOD)
	{
		bcatcstr(glsl, ", ");
		TranslateOperand(psSrcLOD, TO_AUTO_BITCAST_TO_FLOAT);
		if (psContext->psShader.ui32MajorVersion < 4)
		{
			bcatcstr(glsl, ".w");
		}
	}
	else if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
	{
		bcatcstr(glsl, ", 0.0");
	}
	else if (ui32Flags & TEXSMP_FLAG_GRAD)
	{
		bcatcstr(glsl, ", vec4(");
		TranslateOperand(psSrcDx, TO_AUTO_BITCAST_TO_FLOAT);
		bcatcstr(glsl, ")");
		bcatcstr(glsl, gradSwizzle);
		bcatcstr(glsl, ", vec4(");
		TranslateOperand(psSrcDy, TO_AUTO_BITCAST_TO_FLOAT);
		bcatcstr(glsl, ")");
		bcatcstr(glsl, gradSwizzle);
	}

	// Add offset param 
	if (psInst->bAddressOffset)
	{
		if (ui32NumOffsets == 1)
		{
			bformata(glsl, ", %d",
				psInst->iUAddrOffset);
		}
		else
		if (ui32NumOffsets == 2)
		{
			bformata(glsl, ", ivec2(%d, %d)",
				psInst->iUAddrOffset,
				psInst->iVAddrOffset);
		}
		else
		if (ui32NumOffsets == 3)
		{
			bformata(glsl, ", ivec3(%d, %d, %d)",
				psInst->iUAddrOffset,
				psInst->iVAddrOffset,
				psInst->iWAddrOffset);
		}
	}
	// HLSL gather has a variant with separate offset operand
	else if (ui32Flags & TEXSMP_FLAG_PARAMOFFSET) 
	{
		uint32_t mask = OPERAND_4_COMPONENT_MASK_X;
		if (ui32NumOffsets > 1)
			mask |= OPERAND_4_COMPONENT_MASK_Y;
		if (ui32NumOffsets > 2)
			mask |= OPERAND_4_COMPONENT_MASK_Z;

		bcatcstr(glsl, ",");
		TranslateOperand(psSrcOff, TO_FLAG_INTEGER, mask);
	}

	// Add bias if present
	if (ui32Flags & TEXSMP_FLAG_BIAS)
	{
		bcatcstr(glsl, ", ");
		TranslateOperand(psSrcBias, TO_AUTO_BITCAST_TO_FLOAT);
	}

	// Add texture gather component selection if needed
	if ((ui32Flags & TEXSMP_FLAG_GATHER) && psSrcSamp->GetNumSwizzleElements() > 0)
	{
		ASSERT(psSrcSamp->GetNumSwizzleElements() == 1);
		if (psSrcSamp->aui32Swizzle[0] != OPERAND_4_COMPONENT_X)
		{
			if (!(ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE))
			{
				bformata(glsl, ", %d", psSrcSamp->aui32Swizzle[0]);
			}
			else
			{
				// Comp selection not supported with dephth compare gather
			}
		}
	}

	bcatcstr(glsl, ")");

	if (!(ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE) || (ui32Flags & TEXSMP_FLAG_GATHER))
	{
		// iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
		// does not make sense. But need to re-enable to correctly swizzle this particular instruction.
		psSrcTex->iWriteMaskEnabled = 1;
		TranslateOperandSwizzleWithMask(psContext, psSrcTex, psDest->GetAccessMask(), 0);
	}
	AddAssignPrologue(numParenthesis);
}

const char* swizzleString[] = { ".x", ".y", ".z", ".w" };

// Handle cases where vector components are accessed with dynamic index ([] notation).
// A bit ugly hack because compiled HLSL uses byte offsets to access data in structs => we are converting
// the offset back to vector component index in runtime => calculating stuff back and forth.
// TODO: Would be better to eliminate the offset calculation ops and use indexes straight on. Could be tricky though...
void ToGLSL::TranslateDynamicComponentSelection(const ShaderVarType* psVarType, const Operand* psByteAddr, uint32_t offset, uint32_t mask)
{
	bstring glsl = *psContext->currentGLSLString;
	ASSERT(psVarType->Class == SVC_VECTOR);

	bcatcstr(glsl, "["); // Access vector component with [] notation 
	if (offset > 0)
		bcatcstr(glsl, "(");

	// The var containing byte address to the requested element
	TranslateOperand(psByteAddr, TO_FLAG_UNSIGNED_INTEGER, mask);

	if (offset > 0)// If the vector is part of a struct, there is an extra offset in our byte address
		bformata(glsl, " - %du)", offset); // Subtract that first

	bcatcstr(glsl, " >> 0x2u"); // Convert byte offset to index: div by four
	bcatcstr(glsl, "]");
}

void ToGLSL::TranslateShaderStorageStore(Instruction* psInst)
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

			TranslateOperand(psDest, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);

			if (psDest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
				bcatcstr(glsl, "_buf");

			if (psDestAddr)
			{
				bcatcstr(glsl, "[");
				TranslateOperand(psDestAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
				bcatcstr(glsl, "].value");
			}

			bcatcstr(glsl, "[(");
			TranslateOperand(psDestByteOff, dstOffFlag);
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
			bcatcstr(glsl, " = ");
			if (psSrc->GetNumSwizzleElements() > 1)
				TranslateOperand(psSrc, TO_FLAG_UNSIGNED_INTEGER, 1 << (srcComponent++));
			else
				TranslateOperand(psSrc, TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X);

			bcatcstr(glsl, ";\n");
		}
	}
}
void ToGLSL::TranslateShaderStorageLoad(Instruction* psInst)
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
	AddAssignToDest(psDest, destDataType, destCount, &numParenthesis); //TODO check this out?
	if (destCount > 1)
	{
		bformata(glsl, "%s(", GetConstructorForTypeGLSL(destDataType, destCount, false));
		numParenthesis++;
	}
	for (component = 0; component < 4; component++)
	{
		const ShaderVarType *psVar = NULL;
		int addedBitcast = 0;
		if (!(destMask & (1 << component)))
			continue;

		if (firstItemAdded)
			bcatcstr(glsl, ", ");
		else
			firstItemAdded = 1;

		// always uint array atm
		if (destDataType == SVT_FLOAT)
		{
			bcatcstr(glsl, "uintBitsToFloat(");
			addedBitcast = 1;
		}
		else if (destDataType == SVT_INT || destDataType == SVT_INT16 || destDataType == SVT_INT12)
		{
			bcatcstr(glsl, "int(");
			addedBitcast = 1;
		}

		TranslateOperand(psSrc, TO_FLAG_NAME_ONLY);

		if (psSrc->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
			bcatcstr(glsl, "_buf");

		if (psSrcAddr)
		{
			bcatcstr(glsl, "[");
			TranslateOperand(psSrcAddr, TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_INTEGER);
			bcatcstr(glsl, "].value");
		}
		bcatcstr(glsl, "[(");
		TranslateOperand(psSrcByteOff, srcOffFlag);
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

void ToGLSL::TranslateAtomicMemOp(Instruction* psInst)
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

	switch (psInst->eOpcode)
	{
	case OPCODE_IMM_ATOMIC_IADD:
	{
#ifdef _DEBUG
								   psContext->AddIndentation();
								   bcatcstr(glsl, "//IMM_ATOMIC_IADD\n");
#endif
								   func = "Add";
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
							   func = "Add";
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
								  func = "And";
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
							  func = "And";
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
								 func = "Or";
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
							 func = "Or";
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
								  func = "Xor";
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
							  func = "Xor";
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
								   func = "Exchange";
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
									   func = "CompSwap";
									   previousValue = &psInst->asOperands[0];
									   dest = &psInst->asOperands[1];
									   destAddr = &psInst->asOperands[2];
									   compare = &psInst->asOperands[3];
									   src = &psInst->asOperands[4];
									   break;
	}
	case OPCODE_ATOMIC_CMP_STORE:
	{
#ifdef _DEBUG
									psContext->AddIndentation();
									bcatcstr(glsl, "//ATOMIC_CMP_STORE\n");
#endif
									func = "CompSwap";
									previousValue = 0;
									dest = &psInst->asOperands[0];
									destAddr = &psInst->asOperands[1];
									compare = &psInst->asOperands[2];
									src = &psInst->asOperands[3];
									break;
	}
	case OPCODE_IMM_ATOMIC_UMIN:
	{
#ifdef _DEBUG
								   psContext->AddIndentation();
								   bcatcstr(glsl, "//IMM_ATOMIC_UMIN\n");
#endif
								   func = "Min";
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
							   func = "Min";
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
								   func = "Min";
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
							   func = "Min";
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
								   func = "Max";
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
							   func = "Max";
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
								   func = "Max";
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
							   func = "Max";
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

		isUint = !psBinding->isIntUAV;

		if (psBinding->eType == RTYPE_UAV_RWTYPED)
		{
			isUint = (psBinding->ui32ReturnType == RETURN_TYPE_UINT);

			// Find out if it's texture and of what dimension
			switch (psBinding->eDimension)
			{
			case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
			case REFLECT_RESOURCE_DIMENSION_BUFFER:
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

	if (isUint)
		ui32DataTypeFlag = TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_BITCAST_TO_UINT;
	else
		ui32DataTypeFlag = TO_FLAG_INTEGER | TO_AUTO_BITCAST_TO_INT;

	if (previousValue)
		AddAssignToDest(previousValue, isUint ? SVT_UINT : SVT_INT, 1, &numParenthesis);

	if (texDim > 0)
		bcatcstr(glsl, "imageAtomic");
	else
		bcatcstr(glsl, "atomic");

	bcatcstr(glsl, func);
	bcatcstr(glsl, "(");

	TranslateOperand(dest, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);
	if (texDim > 0)
	{
		bcatcstr(glsl, ", ");
		unsigned int compMask = OPERAND_4_COMPONENT_MASK_X;
		if (texDim >= 2)
			compMask |= OPERAND_4_COMPONENT_MASK_Y;
		if (texDim == 3)
			compMask |= OPERAND_4_COMPONENT_MASK_Z;

		TranslateOperand(destAddr, TO_FLAG_INTEGER, compMask);
	}
	else
	{
		if (dest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
			bcatcstr(glsl, "_buf");

		uint32_t destAddrFlag = TO_FLAG_UNSIGNED_INTEGER;
		SHADER_VARIABLE_TYPE destAddrType = destAddr->GetDataType(psContext);
		if (destAddrType == SVT_INT || destAddrType == SVT_INT16 || destAddrType == SVT_INT12)
			destAddrFlag = TO_FLAG_INTEGER;

		bcatcstr(glsl, "[");
		TranslateOperand(destAddr, destAddrFlag, OPERAND_4_COMPONENT_MASK_X);
		
		// Structured buf if we have both x & y swizzles. Raw buf has only x -> no .value[]
		if (destAddr->GetNumSwizzleElements(OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y) == 2)
		{
			bcatcstr(glsl, "]");

			bcatcstr(glsl, ".value[");
			TranslateOperand(destAddr, destAddrFlag, OPERAND_4_COMPONENT_MASK_Y);
		}

		bcatcstr(glsl, " >> 2");//bytes to floats
		if (destAddrFlag == TO_FLAG_UNSIGNED_INTEGER)
			bcatcstr(glsl, "u");

		bcatcstr(glsl, "]");
	}

	bcatcstr(glsl, ", ");

	if (compare)
	{
		TranslateOperand(compare, ui32DataTypeFlag);
		bcatcstr(glsl, ", ");
	}

	TranslateOperand(src, ui32DataTypeFlag);
	bcatcstr(glsl, ")");
	if (previousValue)
	{
		AddAssignPrologue(numParenthesis);
	}
	else
		bcatcstr(glsl, ";\n");
}

void ToGLSL::TranslateConditional(
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
		statement = "return";
	}

	SHADER_VARIABLE_TYPE argType = psInst->asOperands[0].GetDataType(psContext);
	if (argType == SVT_BOOL)
	{
		bcatcstr(glsl, "if(");
		if (psInst->eBooleanTestType != INSTRUCTION_TEST_NONZERO)
			bcatcstr(glsl, "!");
		TranslateOperand(&psInst->asOperands[0], TO_FLAG_BOOL);
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
		uint32_t oFlag = TO_FLAG_UNSIGNED_INTEGER;
		bool isInt = false;
		if (argType == SVT_INT || argType == SVT_INT16 || argType == SVT_INT12)
		{
			isInt = true;
			oFlag = TO_FLAG_INTEGER;
		}

		bcatcstr(glsl, "if(");
		TranslateOperand(&psInst->asOperands[0], oFlag);

		if (psInst->eBooleanTestType == INSTRUCTION_TEST_ZERO)
			bcatcstr(glsl, " == ");
		else
			bcatcstr(glsl, " != ");

		if (isInt)
			bcatcstr(glsl, "0)");
		else
			bcatcstr(glsl, "uint(0u))");


		if (psInst->eOpcode != OPCODE_IF)
		{
			bformata(glsl, " {%s;}\n", statement);
		}
		else
		{
			bcatcstr(glsl, " {\n");
		}
	}
}

static int AreAliasing(Operand* a, Operand* b)
{
  return (a->eType == b->eType && a->ui32RegisterNumber == b->ui32RegisterNumber);
}

void ToGLSL::FormatAndAddAccessMask(int component_mask)
{
  const char *table[] = {"x","y","z","w"};
  bstring glsl = *psContext->currentGLSLString;

  if (component_mask != 15)
  {
    bcatcstr(glsl, ".");
    for (int i = 0; i < 4; ++i)
      if (component_mask & (1u << i))
        bcatcstr(glsl, table[i]);
  }
}

void ToGLSL::CopyOperandToTemp(Instruction* psInst,
                               int index)
{
  psContext->AddIndentation();
  bstring glsl = *psContext->currentGLSLString;
  bcatcstr(glsl, "helper_temp");
  int component_mask = psInst->asOperands[index].GetAccessMask();
  FormatAndAddAccessMask(component_mask);
  bcatcstr(glsl, " = ");
  TranslateOperand(&psInst->asOperands[index], TO_FLAG_NAME_ONLY , NULL);
  AddAssignPrologue(0);
}

void ToGLSL::TranslateUDiv(Instruction* psInst,
                           int qoutient,
                           int remainder,
                           int divident,
                           int divisor)
{
  int tempLocation = -1;
  if (AreAliasing(&psInst->asOperands[qoutient], &psInst->asOperands[divident]) || AreAliasing(&psInst->asOperands[qoutient], &psInst->asOperands[divisor]))
  {
    if (AreAliasing(&psInst->asOperands[remainder], &psInst->asOperands[divident]))
    {
      CopyOperandToTemp(psInst, divident);
      tempLocation = divident;
    }
    else if (AreAliasing(&psInst->asOperands[remainder], &psInst->asOperands[divisor]))
    {
      CopyOperandToTemp(psInst, divisor);
      tempLocation = divisor;
    }
    CallBinaryOp("%", psInst, remainder, divident, divisor, SVT_UINT, false, tempLocation);
    CallBinaryOp("/", psInst, qoutient, divident, divisor, SVT_UINT, false, tempLocation);
  }
  else
  {
    CallBinaryOp("/", psInst, qoutient, divident, divisor, SVT_UINT, false, tempLocation);
    CallBinaryOp("%", psInst, remainder, divident, divisor, SVT_UINT, false, tempLocation);
  }
}

void ToGLSL::TranslateCondSwap(Instruction* psInst, int dst0, int dst1, int cmp, int src0, int src1)
{
  int dst0IsAliasing = AreAliasing(&psInst->asOperands[dst0], &psInst->asOperands[cmp])
    | (AreAliasing(&psInst->asOperands[dst0], &psInst->asOperands[src0]) << 1)
    | (AreAliasing(&psInst->asOperands[dst0], &psInst->asOperands[src1]) << 2);
  if (dst0IsAliasing)
  {
    int dst1IsAliasing = AreAliasing(&psInst->asOperands[dst1], &psInst->asOperands[cmp])
      | (AreAliasing(&psInst->asOperands[dst1], &psInst->asOperands[src0]) << 1)
      | (AreAliasing(&psInst->asOperands[dst1], &psInst->asOperands[src1]) << 2);
    if (dst1IsAliasing)
    {
      AddMOVCBinaryOp(&psInst->asOperands[dst1], &psInst->asOperands[cmp], &psInst->asOperands[src0], &psInst->asOperands[src1], dst1IsAliasing);
      // swap aliasing index 1 and 2
      dst1IsAliasing = (dst1IsAliasing & 1) | ((dst1IsAliasing >> 1) & 2) | ((dst1IsAliasing & 2) << 1);
      AddMOVCBinaryOp(&psInst->asOperands[dst0], &psInst->asOperands[cmp], &psInst->asOperands[src1], &psInst->asOperands[src0], dst1IsAliasing);
    }
    else
    {
      AddMOVCBinaryOp(&psInst->asOperands[dst1], &psInst->asOperands[cmp], &psInst->asOperands[src0], &psInst->asOperands[src1], 0);
      AddMOVCBinaryOp(&psInst->asOperands[dst0], &psInst->asOperands[cmp], &psInst->asOperands[src1], &psInst->asOperands[src0], 0);
    }
  }
  else
  {
    AddMOVCBinaryOp(&psInst->asOperands[dst0], &psInst->asOperands[cmp], &psInst->asOperands[src1], &psInst->asOperands[src0], 0);
    AddMOVCBinaryOp(&psInst->asOperands[dst1], &psInst->asOperands[cmp], &psInst->asOperands[src0], &psInst->asOperands[src1], 0);
  }
}

void ToGLSL::TranslateInstruction(Instruction* psInst, bool isEmbedded /* = false */)
{
	bstring glsl = *psContext->currentGLSLString;
	int numParenthesis = 0;
	const bool isVulkan = ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0);

	if (!isEmbedded)
	{

#ifdef _DEBUG
		psContext->AddIndentation();
		bformata(glsl, "//Instruction %p\n", psInst);
#if 0
		if (psInst->id == 73)
		{
			ASSERT(1); //Set breakpoint here to debug an instruction from its ID.
		}
#endif
#endif

		if (psInst->m_SkipTranslation)
			return;
	}

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
						bcatcstr(glsl, GetConstructorForTypeGLSL(castType, dstCount, false));
						bcatcstr(glsl, "("); // 1
						TranslateOperand(&psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT, psInst->asOperands[0].GetAccessMask());
						bcatcstr(glsl, ")"); // 1
						AddAssignPrologue(numParenthesis);
						break;
	}

	case OPCODE_MOV:
	{
#ifdef _DEBUG
		if (!isEmbedded)
		{
			psContext->AddIndentation();
			bcatcstr(glsl, "//MOV\n");
		}
#endif
		if(!isEmbedded)
			psContext->AddIndentation();

		AddMOVBinaryOp(&psInst->asOperands[0], &psInst->asOperands[1], isEmbedded);
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
						 bcatcstr(glsl, GetConstructorForTypeGLSL(castType, dstCount, false));
						 bcatcstr(glsl, "("); // 1
						 TranslateOperand(&psInst->asOperands[1], psInst->eOpcode == OPCODE_UTOF ? TO_AUTO_BITCAST_TO_UINT : TO_AUTO_BITCAST_TO_INT, psInst->asOperands[0].GetAccessMask());
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
             if (psContext->psShader.eTargetLanguage >= LANG_400 && psContext->psShader.eTargetLanguage <= LANG_440)
             {
               CallHelper3("fma", psInst, 0, 1, 2, 3, 1, TO_FLAG_FORCE_SWIZZLE);
             }
             else
             {
               CallTernaryOp("*", "+", psInst, 0, 1, 2, 3, TO_FLAG_NONE);
             }
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
						if (!isEmbedded)
						{
							psContext->AddIndentation();
							bcatcstr(glsl, "//IADD\n");
						}
#endif
						//Is this a signed or unsigned add?
						if (psInst->asOperands[0].GetDataType(psContext) == SVT_UINT)
						{
							eType = SVT_UINT;
						}
						CallBinaryOp("+", psInst, 0, 1, 2, eType, isEmbedded);
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
					  uint32_t dstSwizCount = psInst->asOperands[0].GetNumSwizzleElements();
                      uint32_t destMask = psInst->asOperands[0].GetAccessMask();
					  if (psInst->asOperands[0].GetDataType(psContext) == SVT_BOOL)
					  {
						  if (dstSwizCount == 1)
						  {
							  uint32_t destMask = psInst->asOperands[0].GetAccessMask();

							  int needsParenthesis = 0;
							  psContext->AddIndentation();
							  AddAssignToDest(&psInst->asOperands[0], SVT_BOOL, psInst->asOperands[0].GetNumSwizzleElements(), &needsParenthesis);
							  TranslateOperand(&psInst->asOperands[1], TO_FLAG_BOOL, destMask);
							  bcatcstr(glsl, " || ");
							  TranslateOperand(&psInst->asOperands[2], TO_FLAG_BOOL, destMask);
							  AddAssignPrologue(needsParenthesis);
						  }
						  else
						  {
							  // Do component-wise and, glsl doesn't support && on bvecs
							  for (uint32_t k = 0; k < 4; k++)
							  {
								  if ((destMask && (1 << k)) == 0)
									  continue;

								  int needsParenthesis = 0;
								  psContext->AddIndentation();
								  // Override dest mask temporarily
								  psInst->asOperands[0].ui32CompMask = (1 << k);
								  ASSERT(psInst->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
								  AddAssignToDest(&psInst->asOperands[0], SVT_BOOL, 1, &needsParenthesis);
								  TranslateOperand(&psInst->asOperands[1], TO_FLAG_BOOL, 1 << k);
								  bcatcstr(glsl, " || ");
								  TranslateOperand(&psInst->asOperands[2], TO_FLAG_BOOL, 1 << k);
								  AddAssignPrologue(needsParenthesis);

							  }
							  // Restore old mask
							  psInst->asOperands[0].ui32CompMask = destMask;
						  }

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
							if (dstSwizCount == 1)
							{
								int needsParenthesis = 0;
								psContext->AddIndentation();
								AddAssignToDest(&psInst->asOperands[0], SVT_BOOL, psInst->asOperands[0].GetNumSwizzleElements(), &needsParenthesis);
								TranslateOperand(&psInst->asOperands[1], TO_FLAG_BOOL, destMask);
								bcatcstr(glsl, " && ");
								TranslateOperand(&psInst->asOperands[2], TO_FLAG_BOOL, destMask);
								AddAssignPrologue(needsParenthesis);
							}
							else
							{
								// Do component-wise and, glsl doesn't support && on bvecs
								for (uint32_t k = 0; k < 4; k++)
								{
									if ((destMask & (1 << k)) == 0)
										continue;

									int needsParenthesis = 0;
									psContext->AddIndentation();
									// Override dest mask temporarily
									psInst->asOperands[0].ui32CompMask = (1 << k);
									ASSERT(psInst->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
									AddAssignToDest(&psInst->asOperands[0], SVT_BOOL, 1, &needsParenthesis);
									TranslateOperand(&psInst->asOperands[1], TO_FLAG_BOOL, 1 << k);
									bcatcstr(glsl, " && ");
									TranslateOperand(&psInst->asOperands[2], TO_FLAG_BOOL, 1 << k);
									AddAssignPrologue(needsParenthesis);

								}
								// Restore old mask
								psInst->asOperands[0].ui32CompMask = destMask;
							}
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
								TranslateOperand(&psInst->asOperands[boolOp], TO_FLAG_BOOL, destMask);
								bcatcstr(glsl, " ? ");
								TranslateOperand(&psInst->asOperands[otherOp], ui32Flags, destMask);
								bcatcstr(glsl, " : ");

								bcatcstr(glsl, GetConstructorForTypeGLSL(eDataType, dstSwizCount, false));
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
								// We can use mix()
								AddAssignToDest(&psInst->asOperands[0], eDataType, dstSwizCount, &needsParenthesis);
								bcatcstr(glsl, "mix(");
								bcatcstr(glsl, GetConstructorForTypeGLSL(eDataType, dstSwizCount, false));
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
								TranslateOperand(&psInst->asOperands[otherOp], ui32Flags, destMask);
								bcatcstr(glsl, ", ");
                // for gl_Frontfacing and such the extra constructor is needed....
								bcatcstr(glsl, GetConstructorForTypeGLSL(SVT_BOOL, dstSwizCount, false));
								bcatcstr(glsl, "(");
                int flags = TO_FLAG_BOOL;
                if (dstSwizCount == 2)
                  flags |= TO_AUTO_EXPAND_TO_VEC2;
                if (dstSwizCount == 3)
                  flags |= TO_AUTO_EXPAND_TO_VEC3;
                if (dstSwizCount == 4)
                  flags |= TO_AUTO_EXPAND_TO_VEC4;
								TranslateOperand(&psInst->asOperands[boolOp], flags, destMask);
								bcatcstr(glsl, ")");
								bcatcstr(glsl, ")");
							}
							else
							{
								AddAssignToDest(&psInst->asOperands[0], SVT_UINT, dstSwizCount, &needsParenthesis);
								bcatcstr(glsl, "(");
								bcatcstr(glsl, GetConstructorForTypeGLSL(SVT_UINT, dstSwizCount, false));
								bcatcstr(glsl, "(");
								TranslateOperand(&psInst->asOperands[boolOp], TO_FLAG_BOOL, destMask);
								bcatcstr(glsl, ") * 0xffffffffu) & ");
								TranslateOperand(&psInst->asOperands[otherOp], TO_FLAG_UNSIGNED_INTEGER, destMask);
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
					   AddAssignToDest(&psInst->asOperands[0], SVT_FLOAT, 1, &numParenthesis);
					   bcatcstr(glsl, "dot(");
					   TranslateOperand(&psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
					   bcatcstr(glsl, ", ");
					   TranslateOperand(&psInst->asOperands[2], TO_AUTO_BITCAST_TO_FLOAT | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
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
					   AddAssignToDest(&psInst->asOperands[0], SVT_FLOAT, 1, &numParenthesis);
					   bcatcstr(glsl, "dot(");
					   TranslateOperand(&psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
					   bcatcstr(glsl, ", ");
					   TranslateOperand(&psInst->asOperands[2], TO_AUTO_BITCAST_TO_FLOAT | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
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
						//check & handle situation with aliased MOVC (ex: a.xyzw = b ? a.wzyx : c)
						//being translated wrongly due to unfold
						int aliasFlag = 0;
						if (psInst->asOperands[0].GetNumSwizzleElements() > 1)
						{
							//check if command has same regs
							bool dstSNAliased[3] = {
								//s0 is aliased if reg num same & no internal bool typing
								AreAliasing(&psInst->asOperands[0], &psInst->asOperands[1]) &&
									!((psInst->asOperands[0].GetDataType(psContext) != SVT_BOOL) &&
									(psInst->asOperands[1].GetDataType(psContext) == SVT_BOOL)),
								AreAliasing(&psInst->asOperands[0], &psInst->asOperands[2]),
								AreAliasing(&psInst->asOperands[0], &psInst->asOperands[3])
							};

							//read access masks
							int acMasks[4];
							for (int i = 0; i < 4; ++i)
								acMasks[i] = psInst->asOperands[i].GetAccessMask();
							int combinedAcMask = acMasks[0];

							//check if same regs have RW to same component
							for (int i = 0; i < 3; ++i)
							{
								if (dstSNAliased[i] && ((acMasks[0] & acMasks[1+i]) > 0))
								{
									aliasFlag |= 1 << i;
									combinedAcMask |= acMasks[1+i];
								}
								else
									dstSNAliased[i] = false;
							}

							//check swizzle order, if it does not match - we surely need temp
							if (aliasFlag)
							{
								for (int i = 0; i < 3; ++i)
								{
									if ((aliasFlag & (1 << i)) == 0)
										continue;

									int swizzleDelta = psInst->asOperands[0].GetSwizzleValue() ^ psInst->asOperands[i+1].GetSwizzleValue();
									int rwChannels = acMasks[0] & acMasks[1+i];
									bool swizzleAliased = false;

									for (int j = 0; j < 4;++j)
									{
										//if swizzle does not match and channel is tagged for alias, we have
										//swizzle order that affects MOVC command and should use temp to properly translate it
										if ((swizzleDelta & 0x3) & (rwChannels & 1))
											swizzleAliased = true;

										swizzleDelta >>= 2;
										rwChannels >>= 1;
									}

									if (!swizzleAliased)
										aliasFlag &= ~(1 << i);
								}
							}

							if (aliasFlag)
							{
								psContext->AddIndentation();
								bcatcstr(glsl, "{\n");

								//find temp type
								const char* movcTempName = "movc_alias_temp";
								const SHADER_VARIABLE_TYPE eDestType = psInst->asOperands[0].GetDataType(psContext);
								int maxComps = combinedAcMask & 0x8 ? 4 :
									combinedAcMask & 0x4 ? 3 :
									combinedAcMask & 0x2 ? 2 :
									1;
								//declare temp
								psContext->AddIndentation();
								bcatcstr(glsl, GetConstructorForTypeGLSL(eDestType, maxComps, false));
								bformata(glsl, " %s;\n", movcTempName);

								//copy to temp
								psContext->AddIndentation();
								bcatcstr(glsl, movcTempName);
								FormatAndAddAccessMask(combinedAcMask);
								bformata(glsl, " = ");
								TranslateOperand(&psInst->asOperands[0], TO_FLAG_NAME_ONLY);
								FormatAndAddAccessMask(combinedAcMask);
								bcatcstr(glsl, ";\n");

								//do MOVC with temp
								AddMOVCBinaryOp(&psInst->asOperands[0], &psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3], aliasFlag, movcTempName);

								psContext->AddIndentation();
								bcatcstr(glsl, "}\n");
							}
						}

						if (!aliasFlag)
							AddMOVCBinaryOp(&psInst->asOperands[0], &psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3], 0);

						break;
	}
	case OPCODE_SWAPC:
	{
#ifdef _DEBUG
						 psContext->AddIndentation();
						 bcatcstr(glsl, "//SWAPC\n");
#endif
						 // TODO needs temps!!
             TranslateCondSwap(psInst, 0, 1, 2, 3, 4);
						 //AddMOVCBinaryOp(&psInst->asOperands[0], &psInst->asOperands[2], &psInst->asOperands[4], &psInst->asOperands[3]);
						 //AddMOVCBinaryOp(&psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3], &psInst->asOperands[4]);
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
					   CallHelper1("inversesqrt", psInst, 0, 1, 1);
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
							CallHelper1("roundEven", psInst, 0, 1, 1);
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
					   bcatcstr(glsl, "return;\n");
					   break;
	}
	case OPCODE_INTERFACE_CALL:
	{
								  const char* name;
								  ShaderVar* psVar;
								  uint32_t varFound;

								  uint32_t funcPointer;
								  uint32_t funcTableIndex;
								  uint32_t funcTable;
								  uint32_t funcBodyIndex;
								  uint32_t funcBody;
								  uint32_t ui32NumBodiesPerTable;

#ifdef _DEBUG
								  psContext->AddIndentation();
								  bcatcstr(glsl, "//INTERFACE_CALL\n");
#endif

								  ASSERT(psInst->asOperands[0].eIndexRep[0] == OPERAND_INDEX_IMMEDIATE32);

								  funcPointer = psInst->asOperands[0].aui32ArraySizes[0];
								  funcTableIndex = psInst->asOperands[0].aui32ArraySizes[1];
								  funcBodyIndex = psInst->ui32FuncIndexWithinInterface;

								  ui32NumBodiesPerTable = psContext->psShader.funcPointer[funcPointer].ui32NumBodiesPerTable;

								  funcTable = psContext->psShader.funcPointer[funcPointer].aui32FuncTables[funcTableIndex];

								  funcBody = psContext->psShader.funcTable[funcTable].aui32FuncBodies[funcBodyIndex];

								  varFound = psContext->psShader.sInfo.GetInterfaceVarFromOffset(funcPointer, &psVar);

								  ASSERT(varFound);

								  name = &psVar->name[0];

								  psContext->AddIndentation();
								  bcatcstr(glsl, name);
								  TranslateOperandIndexMAD(&psInst->asOperands[0], 1, ui32NumBodiesPerTable, funcBodyIndex);
								  //bformata(glsl, "[%d]", funcBodyIndex);
								  bcatcstr(glsl, "();\n");
								  break;
	}
	case OPCODE_LABEL:
	{
#ifdef _DEBUG
						 psContext->AddIndentation();
						 bcatcstr(glsl, "//LABEL\n");
#endif
						 --psContext->indent;
						 psContext->AddIndentation();
						 bcatcstr(glsl, "}\n"); //Closing brace ends the previous function.
						 psContext->AddIndentation();

						 bcatcstr(glsl, "subroutine(SubroutineType)\n");
						 bcatcstr(glsl, "void ");
						 TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
						 bcatcstr(glsl, "(){\n");
						 ++psContext->indent;
						 break;
	}
	case OPCODE_COUNTBITS:
	{
#ifdef _DEBUG
							 psContext->AddIndentation();
							 bcatcstr(glsl, "//COUNTBITS\n");
#endif
							 psContext->AddIndentation();
							 TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
							 bcatcstr(glsl, " = bitCount(");
							 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER);
							 bcatcstr(glsl, ");\n");
							 break;
	}
	case OPCODE_FIRSTBIT_HI:
	{
#ifdef _DEBUG
							   psContext->AddIndentation();
							   bcatcstr(glsl, "//FIRSTBIT_HI\n");
#endif
							   psContext->AddIndentation();
							   TranslateOperand(&psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_DESTINATION);
							   bcatcstr(glsl, " = findMSB(");
							   TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
							   bcatcstr(glsl, ");\n");
							   break;
	}
	case OPCODE_FIRSTBIT_LO:
	{
#ifdef _DEBUG
							   psContext->AddIndentation();
							   bcatcstr(glsl, "//FIRSTBIT_LO\n");
#endif
							   psContext->AddIndentation();
							   TranslateOperand(&psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_DESTINATION);
							   bcatcstr(glsl, " = findLSB(");
							   TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
							   bcatcstr(glsl, ");\n");
							   break;
	}
	case OPCODE_FIRSTBIT_SHI: //signed high
	{
#ifdef _DEBUG
								  psContext->AddIndentation();
								  bcatcstr(glsl, "//FIRSTBIT_SHI\n");
#endif
								  psContext->AddIndentation();
								  TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
								  bcatcstr(glsl, " = findMSB(");
								  TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER);
								  bcatcstr(glsl, ");\n");
								  break;
	}
	case OPCODE_BFREV:
	{
#ifdef _DEBUG
						 psContext->AddIndentation();
						 bcatcstr(glsl, "//BFREV\n");
#endif
						 psContext->AddIndentation();
						 TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
						 bcatcstr(glsl, " = bitfieldReverse(");
						 TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, psInst->asOperands[0].GetAccessMask());
						 bcatcstr(glsl, ");\n");
						 break;
	}
	case OPCODE_BFI:
	{
#ifdef _DEBUG
					   psContext->AddIndentation();
					   bcatcstr(glsl, "//BFI\n");
#endif
             const uint32_t writeMask = psInst->asOperands[0].GetAccessMask();
             const uint32_t ouputCount = psInst->asOperands[0].GetNumSwizzleElements();
					   psContext->AddIndentation();
					   AddAssignToDest(&psInst->asOperands[0], SVT_INT, ouputCount, &numParenthesis);

					   if (ouputCount == 1)
						   bformata(glsl, "int(");
					   else
						   bformata(glsl, "ivec%d(", ouputCount);
             uint32_t writtenCount = 0;
             for (uint32_t i = 0; i < 4; ++i)
             {
               if ((writeMask&(1 << i)) == 0)
                 continue;
               bcatcstr(glsl, "bitfieldInsert(");
               // operands in glsl are in reversed order
               for (uint32_t j = 4; j > 0; --j)
               {
                 TranslateOperand(&psInst->asOperands[j], TO_FLAG_INTEGER, 1 << i);
                 if(j!=1)
                   bcatcstr(glsl, ",");
               }
               bcatcstr(glsl, ")");
               if (++writtenCount != ouputCount)
                 bcatcstr(glsl, ",");
             }

					   bcatcstr(glsl, ")");
					   AddAssignPrologue(numParenthesis);
					   break;
	}
	case OPCODE_CUT:
	{
#ifdef _DEBUG
					   psContext->AddIndentation();
					   bcatcstr(glsl, "//CUT\n");
#endif
					   psContext->AddIndentation();
					   bcatcstr(glsl, "EndPrimitive();\n");
					   break;
	}
	case OPCODE_EMIT:
	{
#ifdef _DEBUG
						psContext->AddIndentation();
						bcatcstr(glsl, "//EMIT\n");
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
						bcatcstr(glsl, "EmitVertex();\n");
						break;
	}
	case OPCODE_EMITTHENCUT:
	{
#ifdef _DEBUG
							   psContext->AddIndentation();
							   bcatcstr(glsl, "//EMITTHENCUT\n");
#endif
							   psContext->AddIndentation();
							   bcatcstr(glsl, "EmitVertex();\n");
							   psContext->AddIndentation();
							   bcatcstr(glsl, "EndPrimitive();\n");
							   break;
	}

	case OPCODE_CUT_STREAM:
	{
#ifdef _DEBUG
							  psContext->AddIndentation();
							  bcatcstr(glsl, "//CUT_STREAM\n");
#endif
							  psContext->AddIndentation();
							  ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_STREAM);
							  if (psContext->psShader.eTargetLanguage < LANG_400 || psInst->asOperands[0].ui32RegisterNumber == 0)
							  {
								  // ES geom shaders only support one stream.
								  bcatcstr(glsl, "EndPrimitive();\n");
							  }
							  else
							  {
								  bcatcstr(glsl, "EndStreamPrimitive(");
								  TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
								  bcatcstr(glsl, ");\n");
							  }

							  break;
	}
	case OPCODE_EMIT_STREAM:
	{
#ifdef _DEBUG
							   psContext->AddIndentation();
							   bcatcstr(glsl, "//EMIT_STREAM\n");
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

							   ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_STREAM);
							   if (psContext->psShader.eTargetLanguage < LANG_400 || psInst->asOperands[0].ui32RegisterNumber == 0)
							   {
								   // ES geom shaders only support one stream.
								   bcatcstr(glsl, "EmitVertex();\n");
							   }
							   else
							   {
								   bcatcstr(glsl, "EmitStreamVertex(");
								   TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
								   bcatcstr(glsl, ");\n");
							   }
							   break;
	}
	case OPCODE_EMITTHENCUT_STREAM:
	{
#ifdef _DEBUG
									  psContext->AddIndentation();
									  bcatcstr(glsl, "//EMITTHENCUT\n");
#endif
									  ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_STREAM);
									  if (psContext->psShader.eTargetLanguage < LANG_400 || psInst->asOperands[0].ui32RegisterNumber == 0)
									  {
										  // ES geom shaders only support one stream.
										  bcatcstr(glsl, "EmitVertex();\n");
										  psContext->AddIndentation();
										  bcatcstr(glsl, "EndPrimitive();\n");
									  }
									  else
									  {
										  bcatcstr(glsl, "EmitStreamVertex(");
										  TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
										  bcatcstr(glsl, ");\n");
										  psContext->AddIndentation();
										  bcatcstr(glsl, "EndStreamPrimitive(");
										  TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
										  bcatcstr(glsl, ");\n");
									  }
									  break;
	}
	case OPCODE_REP:
	{
#ifdef _DEBUG
					   psContext->AddIndentation();
					   bcatcstr(glsl, "//REP\n");
#endif
					   //Need to handle nesting.
					   //Max of 4 for rep - 'Flow Control Limitations' http://msdn.microsoft.com/en-us/library/windows/desktop/bb219848(v=vs.85).aspx

					   psContext->AddIndentation();
					   bcatcstr(glsl, "RepCounter = ");
					   TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
					   bcatcstr(glsl, ";\n");

					   psContext->AddIndentation();
					   bcatcstr(glsl, "while(RepCounter!=0){\n");
					   ++psContext->indent;
					   break;
	}
	case OPCODE_ENDREP:
	{
#ifdef _DEBUG
						  psContext->AddIndentation();
						  bcatcstr(glsl, "//ENDREP\n");
#endif
						  psContext->AddIndentation();
						  bcatcstr(glsl, "RepCounter--;\n");

						  --psContext->indent;

						  psContext->AddIndentation();
						  bcatcstr(glsl, "}\n");
						  break;
	}
	case OPCODE_LOOP:
	{
#ifdef _DEBUG
						psContext->AddIndentation();
						bcatcstr(glsl, "//LOOP\n");
#endif
						psContext->AddIndentation();

						if (psInst->ui32NumOperands == 2)
						{
							//DX9 version
							ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_SPECIAL_LOOPCOUNTER);
							bcatcstr(glsl, "for(");
							bcatcstr(glsl, "LoopCounter = ");
							TranslateOperand(&psInst->asOperands[1], TO_FLAG_NONE);
							bcatcstr(glsl, ".y, ZeroBasedCounter = 0;");
							bcatcstr(glsl, "ZeroBasedCounter < ");
							TranslateOperand(&psInst->asOperands[1], TO_FLAG_NONE);
							bcatcstr(glsl, ".x;");

							bcatcstr(glsl, "LoopCounter += ");
							TranslateOperand(&psInst->asOperands[1], TO_FLAG_NONE);
							bcatcstr(glsl, ".z, ZeroBasedCounter++){\n");
							++psContext->indent;
						}
						else if (psInst->m_LoopInductors[1] != 0)
						{
							// Can emit as for
							bcatcstr(glsl, "for(");
							if (psInst->m_LoopInductors[0] != 0)
							{
								if (psInst->m_InductorRegister != 0)
								{
									// Do declaration here as well
									switch (psInst->m_LoopInductors[0]->asOperands[0].GetDataType(psContext))
									{
									case SVT_INT:
										bcatcstr(glsl, "int ");
										break;
									case SVT_UINT:
										bcatcstr(glsl, "uint ");
										break;
									case SVT_FLOAT:
										bcatcstr(glsl, "float ");
										break;
									default:
										ASSERT(0);
										break;
									}
								}
								TranslateInstruction(psInst->m_LoopInductors[0], true);
							}
							bcatcstr(glsl, " ; ");
							bool negateCondition = psInst->m_LoopInductors[1]->eBooleanTestType != INSTRUCTION_TEST_NONZERO;
							bool negateOrder = false;

							// Yet Another NVidia OSX shader compiler bug workaround (really nvidia, get your s#!t together):
							// For reasons unfathomable to us, this breaks SSAO effect on OSX (case 756028)
							// Broken: for(int ti_loop_1 = int(int(0xFFFFFFFCu)) ; 4 >= ti_loop_1 ; ti_loop_1++)
							// Works: for (int ti_loop_1 = int(int(0xFFFFFFFCu)); ti_loop_1 <= 4; ti_loop_1++)
							//
							// So, check if the first argument is an immediate value, and if so, switch the order or the operands
							// (and adjust condition)
							if (psInst->m_LoopInductors[1]->asOperands[1].eType == OPERAND_TYPE_IMMEDIATE32)
								negateOrder = true;

							uint32_t typeFlags = TO_FLAG_INTEGER;
							const char *cmpOp = "";
							switch (psInst->m_LoopInductors[1]->eOpcode)
							{
							case OPCODE_IGE:
								if(negateOrder)
									cmpOp = negateCondition ? ">" : "<=";
								else
									cmpOp = negateCondition ? "<" : ">=";
								break;
							case OPCODE_ILT:
								if(negateOrder)
									cmpOp = negateCondition ? "<=" : ">";
								else
									cmpOp = negateCondition ? ">=" : "<";
								break;
							case OPCODE_IEQ:
								// No need to change the comparison if negateOrder is true
								cmpOp = negateCondition ? "!=" : "==";
								if (psInst->m_LoopInductors[1]->asOperands[0].GetDataType(psContext) == SVT_UINT)
									typeFlags = TO_FLAG_UNSIGNED_INTEGER;
								break;
							case OPCODE_INE:
								// No need to change the comparison if negateOrder is true
								cmpOp = negateCondition ? "==" : "!=";
								if (psInst->m_LoopInductors[1]->asOperands[0].GetDataType(psContext) == SVT_UINT)
									typeFlags = TO_FLAG_UNSIGNED_INTEGER;
								break;
							case OPCODE_UGE:
								if(negateOrder)
									cmpOp = negateCondition ? ">" : "<=";
								else
									cmpOp = negateCondition ? "<" : ">=";
								typeFlags = TO_FLAG_UNSIGNED_INTEGER;
								break;
							case OPCODE_ULT:
								if(negateOrder)
									cmpOp = negateCondition ? "<=" : ">";
								else
									cmpOp = negateCondition ? ">=" : "<";
								typeFlags = TO_FLAG_UNSIGNED_INTEGER;
								break;

							default:
								ASSERT(0);
							}
							TranslateOperand(&psInst->m_LoopInductors[1]->asOperands[negateOrder ? 2 : 1], typeFlags);
							bcatcstr(glsl, cmpOp);
							TranslateOperand(&psInst->m_LoopInductors[1]->asOperands[negateOrder ? 1 : 2], typeFlags);

							bcatcstr(glsl, " ; ");
							// One more shortcut: translate IADD tX, tX, 1 to tX++
							if (HLSLcc::IsAddOneInstruction(psInst->m_LoopInductors[3]))
							{
								TranslateOperand(&psInst->m_LoopInductors[3]->asOperands[0], TO_FLAG_DESTINATION);
								bcatcstr(glsl, "++");
							}
							else
								TranslateInstruction(psInst->m_LoopInductors[3], true);

							bcatcstr(glsl, ")\n");
							psContext->AddIndentation();
							bcatcstr(glsl, "{\n");
							++psContext->indent;
						}
						else
						{
							bcatcstr(glsl, "while(true){\n");
							++psContext->indent;
						}
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
						 psContext->AddIndentation();
						 bcatcstr(glsl, "//ENDIF\n");
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

						if (ui32SyncFlags & SYNC_THREAD_GROUP_SHARED_MEMORY)
						{
							psContext->AddIndentation();
							bcatcstr(glsl, "memoryBarrierShared();\n");
						}
						if (ui32SyncFlags & (SYNC_UNORDERED_ACCESS_VIEW_MEMORY_GROUP | SYNC_UNORDERED_ACCESS_VIEW_MEMORY_GLOBAL))
						{
							psContext->AddIndentation();
							bcatcstr(glsl, "memoryBarrier();\n");
						}
						if (ui32SyncFlags & SYNC_THREADS_IN_GROUP)
						{
							psContext->AddIndentation();
							bcatcstr(glsl, "barrier();\n");
						}
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
						  TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER);
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
						TranslateOperand(&psInst->asOperands[0], TO_FLAG_INTEGER);
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
						   if (psContext->psShader.ui32MajorVersion <= 3)
						   {
							   bcatcstr(glsl, "if(any(lessThan((");
							   TranslateOperand(&psInst->asOperands[0], TO_FLAG_NONE);

							   if (psContext->psShader.ui32MajorVersion == 1)
							   {
								   /* SM1.X only kills based on the rgb channels */
								   bcatcstr(glsl, ").xyz, vec3(0)))){discard;}\n");
							   }
							   else
							   {
								   bcatcstr(glsl, "), vec4(0)))){discard;}\n");
							   }
						   }
						   else if (psInst->eBooleanTestType == INSTRUCTION_TEST_ZERO)
						   {
							   bcatcstr(glsl, "if(!");
							   TranslateOperand(&psInst->asOperands[0], TO_FLAG_BOOL);
							   bcatcstr(glsl, "){discard;}\n");
						   }
						   else
						   {
							   ASSERT(psInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO);
							   bcatcstr(glsl, "if(");
							   TranslateOperand(&psInst->asOperands[0], TO_FLAG_BOOL);
							   bcatcstr(glsl, "){discard;}\n");
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

					   TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
					   bcatcstr(glsl, ",");
					   TranslateTexCoord(
						   psContext->psShader.aeResourceDims[psInst->asOperands[2].ui32RegisterNumber],
						   &psInst->asOperands[1],
               false);
					   bcatcstr(glsl, ")");

					   //The swizzle on srcResource allows the returned values to be swizzled arbitrarily before they are written to the destination.

					   // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
					   // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
					   psInst->asOperands[2].iWriteMaskEnabled = 1;
					   TranslateOperandSwizzleWithMask(psContext, &psInst->asOperands[2], psInst->asOperands[0].GetAccessMask(), 0);
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
								 TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
								 bcatcstr(glsl, " = interpolateAtCentroid(");
								 //interpolateAtCentroid accepts in-qualified variables.
								 //As long as bytecode only writes vX registers in declarations
								 //we should be able to use the declared name directly.
								 TranslateOperand(&psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
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
									 TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
									 bcatcstr(glsl, " = interpolateAtSample(");
									 //interpolateAtSample accepts in-qualified variables.
									 //As long as bytecode only writes vX registers in declarations
									 //we should be able to use the declared name directly.
									 TranslateOperand(&psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
									 bcatcstr(glsl, ", ");
									 TranslateOperand(&psInst->asOperands[2], TO_FLAG_INTEGER);
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
								TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
								bcatcstr(glsl, " = interpolateAtOffset(");
								//interpolateAtOffset accepts in-qualified variables.
								//As long as bytecode only writes vX registers in declarations
								//we should be able to use the declared name directly.
								TranslateOperand(&psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
								bcatcstr(glsl, ", ");
								TranslateOperand(&psInst->asOperands[2], TO_FLAG_INTEGER);
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
								case RESOURCE_DIMENSION_BUFFER:
									compMask |= 1;
									break;
								default:
									ASSERT(0);
									break;
								}

								SHADER_VARIABLE_TYPE srcDataType;
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
								bcatcstr(glsl, "imageLoad(");
								TranslateOperand(psSrc, TO_FLAG_NAME_ONLY);
								bcatcstr(glsl, ", ");
								TranslateOperand(psSrcAddr, TO_FLAG_INTEGER, compMask);
								bcatcstr(glsl, ")");
								TranslateOperandSwizzle(psContext, &psInst->asOperands[0], 0);
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
								   uint32_t flags = TO_FLAG_INTEGER;
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

								   bcatcstr(glsl, "imageStore(");
								   TranslateOperand(&psInst->asOperands[0], TO_FLAG_NAME_ONLY);
								   bcatcstr(glsl, ", ");

								   switch (psRes->eDimension)
								   {
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
								   case REFLECT_RESOURCE_DIMENSION_BUFFER:
									   opMask = OPERAND_4_COMPONENT_MASK_X;
									   break;
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
									   opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
									   flags |= TO_AUTO_EXPAND_TO_VEC2;
									   break;
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
								   case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
								   case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
									   opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z;
									   flags |= TO_AUTO_EXPAND_TO_VEC3;
									   break;
								   case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
									   flags |= TO_AUTO_EXPAND_TO_VEC4;
									   break;
                                    default:
                                        ASSERT(0);
                                        break;
								   };

								   TranslateOperand(&psInst->asOperands[1], flags, opMask);
								   bcatcstr(glsl, ", ");
								   TranslateOperand(&psInst->asOperands[2], ResourceReturnTypeToFlag(psRes->ui32ReturnType));
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

	case OPCODE_ATOMIC_AND:
	case OPCODE_ATOMIC_OR:
	case OPCODE_ATOMIC_XOR:
	case OPCODE_ATOMIC_CMP_STORE:
	case OPCODE_ATOMIC_IADD:
	case OPCODE_ATOMIC_IMAX:
	case OPCODE_ATOMIC_IMIN:
	case OPCODE_ATOMIC_UMAX:
	case OPCODE_ATOMIC_UMIN:
	case OPCODE_IMM_ATOMIC_IADD:
	case OPCODE_IMM_ATOMIC_AND:
	case OPCODE_IMM_ATOMIC_OR:
	case OPCODE_IMM_ATOMIC_XOR:
	case OPCODE_IMM_ATOMIC_EXCH:
	case OPCODE_IMM_ATOMIC_CMP_EXCH:
	case OPCODE_IMM_ATOMIC_IMAX:
	case OPCODE_IMM_ATOMIC_IMIN:
	case OPCODE_IMM_ATOMIC_UMAX:
	case OPCODE_IMM_ATOMIC_UMIN:
	{
									   TranslateAtomicMemOp(psInst);
									   break;
	}
	case OPCODE_UBFE:
	case OPCODE_IBFE:
	{
						int numParenthesis = 0;
						int i;
						uint32_t writeMask = psInst->asOperands[0].GetAccessMask();
						SHADER_VARIABLE_TYPE dataType = psInst->eOpcode == OPCODE_UBFE ? SVT_UINT : SVT_INT;
						uint32_t flags = psInst->eOpcode == OPCODE_UBFE ? TO_AUTO_BITCAST_TO_UINT : TO_AUTO_BITCAST_TO_INT;
#ifdef _DEBUG
						psContext->AddIndentation();
						if (psInst->eOpcode == OPCODE_UBFE)
							bcatcstr(glsl, "//OPCODE_UBFE\n");
						else
							bcatcstr(glsl, "//OPCODE_IBFE\n");
#endif
						psContext->AddIndentation();
						bcatcstr(glsl, "{\n");
						const char* tempName = "uibfe_alias_temp";
						const SHADER_VARIABLE_TYPE eSrcType = psInst->asOperands[3].GetDataType(psContext);
						int combinedAcMask = psInst->asOperands[3].GetAccessMask();
						int maxComps = combinedAcMask & 0x8 ? 4 :
							combinedAcMask & 0x4 ? 3 :
							combinedAcMask & 0x2 ? 2 :
							1;

						//declare temp
						psContext->AddIndentation();
						bcatcstr(glsl, GetConstructorForTypeGLSL(eSrcType, maxComps, false));
						bformata(glsl, " %s;\n", tempName);

						//copy to temp
						psContext->AddIndentation();
						bcatcstr(glsl, tempName);
						FormatAndAddAccessMask(combinedAcMask);
						bformata(glsl, " = ");
						TranslateOperand(&psInst->asOperands[3], TO_FLAG_NAME_ONLY);
						FormatAndAddAccessMask(combinedAcMask);
						bcatcstr(glsl, ";\n");

						//do actual operation

						// Need to open this up, GLSL bitfieldextract uses same offset and width for all components
						for (i = 0; i < 4; i++)
						{
							if ((writeMask & (1 << i)) == 0)
								continue;
							psContext->AddIndentation();
							psInst->asOperands[0].ui32CompMask = (1 << i);
							psInst->asOperands[0].eSelMode = OPERAND_4_COMPONENT_MASK_MODE;
							AddAssignToDest(&psInst->asOperands[0], dataType, 1, &numParenthesis);

							bcatcstr(glsl, "bitfieldExtract(");
							TranslateOperand(&psInst->asOperands[3], flags, (1 << i), tempName);
							bcatcstr(glsl, ", ");
							TranslateOperand(&psInst->asOperands[2], TO_AUTO_BITCAST_TO_INT, (1 << i));
							bcatcstr(glsl, ", ");
							TranslateOperand(&psInst->asOperands[1], TO_AUTO_BITCAST_TO_INT, (1 << i));
							bcatcstr(glsl, ")");
							AddAssignPrologue(numParenthesis);
						}

						psContext->AddIndentation();
						bcatcstr(glsl, "}\n");
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
					   AddAssignToDest(&psInst->asOperands[0], SVT_FLOAT, srcElemCount, &numParenthesis);
					   bcatcstr(glsl, GetConstructorForTypeGLSL(SVT_FLOAT, destElemCount, false));
					   bcatcstr(glsl, "(1.0) / ");
					   bcatcstr(glsl, GetConstructorForTypeGLSL(SVT_FLOAT, destElemCount, false));
					   bcatcstr(glsl, "(");
					   numParenthesis++;
					   TranslateOperand(&psInst->asOperands[1], TO_FLAG_NONE, psInst->asOperands[0].GetAccessMask());
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
                  bcatcstr(glsl, "unpackHalf2x16(");
                  TranslateOperand(&psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER, bit);
                  for(;p >0; --p)
                    bcatcstr(glsl, ")");
                  bcatcstr(glsl, ").x;\n");
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
                  bcatcstr(glsl, "packHalf2x16(vec2(");
                  TranslateOperand(&psInst->asOperands[1], TO_FLAG_NONE, bit);
                  for (; p > 0; --p)
                    bcatcstr(glsl, ")");
                  bcatcstr(glsl, ")) & 0xFFFF;\n");
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
						TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, psInst->asOperands[0].GetAccessMask());
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
							 CallHelper1("dFdx", psInst, 0, 1, 1);
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
							 CallHelper1("dFdy", psInst, 0, 1, 1);
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
						  TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
						  bcatcstr(glsl, " = dot(vec2(");
						  TranslateOperand(&psInst->asOperands[1], TO_FLAG_NONE);
						  bcatcstr(glsl, "), vec2(");
						  TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
						  bcatcstr(glsl, ")) + ");
						  TranslateOperand(&psInst->asOperands[3], TO_FLAG_NONE);
						  bcatcstr(glsl, ";\n");
						  break;
	}
	case OPCODE_POW:
	{
#ifdef _DEBUG
					   psContext->AddIndentation();
					   bcatcstr(glsl, "//POW\n");
#endif
					   psContext->AddIndentation();
					   TranslateOperand(&psInst->asOperands[0], TO_FLAG_DESTINATION);
					   bcatcstr(glsl, " = pow(abs(");
					   TranslateOperand(&psInst->asOperands[1], TO_FLAG_NONE);
					   bcatcstr(glsl, "), ");
					   TranslateOperand(&psInst->asOperands[2], TO_FLAG_NONE);
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
		if (isVulkan)
			bcatcstr(glsl, "atomicAdd(");
		else
			bcatcstr(glsl, "atomicCounterIncrement(");
		ResourceName(glsl, psContext, RGROUP_UAV, psInst->asOperands[1].ui32RegisterNumber, 0);
		bformata(glsl, "_counter");
		if (isVulkan)
			bcatcstr(glsl, ", 1u)");
		else
			bcatcstr(glsl, ")");
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
		if (isVulkan)
			bcatcstr(glsl, "atomicAdd(");
		else
			bcatcstr(glsl, "atomicCounterDecrement(");
		ResourceName(glsl, psContext, RGROUP_UAV, psInst->asOperands[1].ui32RegisterNumber, 0);
		bformata(glsl, "_counter");
		if (isVulkan)
			bcatcstr(glsl, ", 0xffffffffu)");
		else
			bcatcstr(glsl, ")");
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
					   TranslateOperand(&psInst->asOperands[1], TO_FLAG_INTEGER, psInst->asOperands[0].GetAccessMask());
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
#ifdef _DEBUG
						   psContext->AddIndentation();
						   bcatcstr(glsl, "//RESINFO\n");
#endif
               const uint32_t targetMask = psInst->asOperands[0].GetAccessMask();
               for (uint32_t i = 0; i < 4; ++i)
						   {
                 if (targetMask & 1 << i)
                   GetResInfoData(psInst, psInst->asOperands[2].aui32Swizzle[i], i);
						   }

						   break;
	}
	case OPCODE_BUFINFO:
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(glsl, "//BUFINFO\n");
#endif
		psContext->AddIndentation();
		AddAssignToDest(&psInst->asOperands[0], SVT_INT, 1, &numParenthesis);
		TranslateOperand(&psInst->asOperands[1], TO_FLAG_NAME_ONLY);
		bcatcstr(glsl, "_buf.length()");
		AddAssignPrologue(numParenthesis);
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

		const bool workaroundAdrenoBugs = psContext->psShader.eTargetLanguage == LANG_ES_300;

		if (workaroundAdrenoBugs)
			bcatcstr(glsl, "#ifdef UNITY_ADRENO_ES3\n");

		for (int i = workaroundAdrenoBugs ? 0 : 1; i < 2; ++i)
		{
			const bool generateWorkaround = (i == 0);
			psContext->AddIndentation();
			AddAssignToDest(&psInst->asOperands[0], SVT_FLOAT, dstCount, &numParenthesis);
			bcatcstr(glsl, generateWorkaround ? "min(max(" : "clamp(");
			TranslateOperand(&psInst->asOperands[0], TO_AUTO_BITCAST_TO_FLOAT);
			bcatcstr(glsl, generateWorkaround ? ", 0.0), 1.0)" : ", 0.0, 1.0)");
			AddAssignPrologue(numParenthesis);

			if (generateWorkaround)
				bcatcstr(glsl, "#else\n");
		}

		if (workaroundAdrenoBugs)
			bcatcstr(glsl, "#endif\n");
	}
}
