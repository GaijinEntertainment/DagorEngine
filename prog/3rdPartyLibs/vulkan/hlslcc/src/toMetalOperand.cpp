#include <stdio.h>
#include "internal_includes/HLSLccToolkit.h"
#include "internal_includes/HLSLCrossCompilerContext.h"
#include "hlslcc.h"
#include "internal_includes/debug.h"
#include "internal_includes/Shader.h"
#include "internal_includes/toMetal.h"
#include <cmath>
#include <sstream>

#include <float.h>
#include <stdlib.h>

#include <algorithm>

using namespace HLSLcc;

#ifdef _MSC_VER
#define snprintf _snprintf
#define fpcheck(x) (_isnan(x) || !_finite(x))
#else
#define fpcheck(x) (std::isnan(x) || std::isinf(x))
#endif

// Returns nonzero if types are just different precisions of the same underlying type
static bool AreTypesCompatible(SHADER_VARIABLE_TYPE a, uint32_t ui32TOFlag)
{
	SHADER_VARIABLE_TYPE b = TypeFlagsToSVTType(ui32TOFlag);

	if (a == b)
		return true;

	// Special case for array indices: both uint and int are fine
	if ((ui32TOFlag & TO_FLAG_INTEGER) && (ui32TOFlag & TO_FLAG_UNSIGNED_INTEGER) &&
		(a == SVT_INT || a == SVT_INT16 || a == SVT_UINT || a == SVT_UINT16))
		return true;

	return false;
}

extern uint32_t getStrideSizeForDynamicIndexing(const ShaderVarType* var);

std::string ToMetal::TranslateOperandSwizzle(const Operand* psOperand, uint32_t ui32ComponentMask, int iRebase, bool includeDot /*= true*/)
{
	std::ostringstream oss;
	uint32_t accessMask = ui32ComponentMask & psOperand->GetAccessMask();
	if(psOperand->eType == OPERAND_TYPE_INPUT)
	{
    RegisterSpace regSpace = psOperand->GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);
		// Skip swizzle for scalar inputs, but only if we haven't redirected them
		if (regSpace == RegisterSpace::per_vertex)
		{
			if ((psContext->psShader.asPhases[psContext->currentPhase].acInputNeedsRedirect[psOperand->ui32RegisterNumber] == 0) &&
				(psContext->psShader.abScalarInput[(int)regSpace][psOperand->ui32RegisterNumber] & accessMask))
			{
				return "";
			}
		}
		else
		{
			if ((psContext->psShader.asPhases[psContext->currentPhase].acPatchConstantsNeedsRedirect[psOperand->ui32RegisterNumber] == 0) &&
				(psContext->psShader.abScalarInput[(int)regSpace][psOperand->ui32RegisterNumber] & accessMask))
			{
				return "";
			}
		}
	}
	if (psOperand->eType == OPERAND_TYPE_OUTPUT)
	{
    RegisterSpace regSpace = psOperand->GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);
		// Skip swizzle for scalar outputs, but only if we haven't redirected them
		if (regSpace == RegisterSpace::per_vertex)
		{
			if ((psContext->psShader.asPhases[psContext->currentPhase].acOutputNeedsRedirect[psOperand->ui32RegisterNumber] == 0) &&
				(psContext->psShader.abScalarOutput[(int)regSpace][psOperand->ui32RegisterNumber] & accessMask))
			{
				return "";
			}
		}
		else
		{
			if ((psContext->psShader.asPhases[psContext->currentPhase].acPatchConstantsNeedsRedirect[psOperand->ui32RegisterNumber] == 0) &&
				(psContext->psShader.abScalarOutput[(int)regSpace][psOperand->ui32RegisterNumber] & accessMask))
			{
				return "";
			}
		}
	}

	if(psOperand->iWriteMaskEnabled &&
	   psOperand->iNumComponents != 1)
	{
		//Component Mask
		if(psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
		{
			uint32_t mask;
			if (psOperand->ui32CompMask != 0)
				mask = psOperand->ui32CompMask & ui32ComponentMask;
			else
				mask = ui32ComponentMask;

			if(mask != 0 && mask != OPERAND_4_COMPONENT_MASK_ALL)
			{
				if (includeDot)
                    oss << ".";
				if(mask & OPERAND_4_COMPONENT_MASK_X)
				{
					ASSERT(iRebase == 0);
					oss << "x";
				}
				if(mask & OPERAND_4_COMPONENT_MASK_Y)
				{
					ASSERT(iRebase <= 1);
					oss << "xy"[1 - iRebase];
				}
				if(mask & OPERAND_4_COMPONENT_MASK_Z)
				{
					ASSERT(iRebase <= 2);
					oss << "xyz"[2 - iRebase];
				}
				if(mask & OPERAND_4_COMPONENT_MASK_W)
				{
					ASSERT(iRebase <= 3);
					oss << "xyzw"[3 - iRebase];
				}
			}
		}
		else
		//Component Swizzle
		if(psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
		{
			if (ui32ComponentMask != OPERAND_4_COMPONENT_MASK_ALL ||
				!(psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_X &&
					psOperand->aui32Swizzle[1] == OPERAND_4_COMPONENT_Y &&
					psOperand->aui32Swizzle[2] == OPERAND_4_COMPONENT_Z &&
					psOperand->aui32Swizzle[3] == OPERAND_4_COMPONENT_W
				 )
				)
			{
				uint32_t i;

                if (includeDot)
                    oss << ".";

				for (i = 0; i < 4; ++i)
				{
					if (!(ui32ComponentMask & (OPERAND_4_COMPONENT_MASK_X << i)))
						continue;

					if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_X)
					{
						ASSERT(iRebase == 0);
						oss << "x";
					}
					else if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Y)
					{
						ASSERT(iRebase <= 1);
						oss << "xy"[1 - iRebase];
					}
					else if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Z)
					{
						ASSERT(iRebase <= 2);
						oss << "xyz"[2 - iRebase];
					}
					else if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_W)
					{
						ASSERT(iRebase <= 3);
						oss << "xyzw"[3 - iRebase];
					}
				}
			}
		}
		else
		if(psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE) // ui32ComponentMask is ignored in this case
		{
            if (includeDot)
                oss << ".";

			if(psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_X)
			{
				ASSERT(iRebase == 0);
				oss << "x";
			}
			else
			if(psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Y)
			{
				ASSERT(iRebase <= 1);
				oss << "xy"[1 - iRebase];
			}
			else
			if(psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Z)
			{
				ASSERT(iRebase <= 2);
				oss << "xyz"[2 - iRebase];
			}
			else
			if(psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_W)
			{
				ASSERT(iRebase <= 3);
				oss << "xyzw"[3 - iRebase];
			}
		}
	}
	return oss.str();
}

std::string ToMetal::TranslateOperandIndex(const Operand* psOperand, int index)
{
	int i = index;
	std::ostringstream oss;
	ASSERT(index < psOperand->iIndexDims);

	switch(psOperand->eIndexRep[i])
	{
		case OPERAND_INDEX_IMMEDIATE32:
		{
			oss << "[" << psOperand->aui32ArraySizes[i] << "]";
			return oss.str();
		}
		case OPERAND_INDEX_RELATIVE:
		{
			oss << "[" << TranslateOperand(psOperand->m_SubOperands[i].get(), TO_FLAG_INTEGER) << "]";
			return oss.str();
		}
		case OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
		{
			oss << "[" << TranslateOperand(psOperand->m_SubOperands[i].get(), TO_FLAG_INTEGER) << " + "<< psOperand->aui32ArraySizes[i] <<"]";
			return oss.str();
		}
		default:
		{
			ASSERT(0);
			return "";
			break;
		}
	}
}

/*static std::string GetBitcastOp(HLSLCrossCompilerContext *psContext, SHADER_VARIABLE_TYPE from, SHADER_VARIABLE_TYPE to, uint32_t numComponents)
{
	if (psContext->psShader.eTargetLanguage == LANG_METAL)
	{
		std::ostringstream oss;
		oss << "as_type<";
		oss << GetConstructorForTypeMetal(to, numComponents);
		oss << ">";
		return oss.str();
	}
	else
	{
		if ((to == SVT_FLOAT || to == SVT_FLOAT16 || to == SVT_FLOAT10) && from == SVT_INT)
			return "intBitsToFloat";
		else if ((to == SVT_FLOAT || to == SVT_FLOAT16 || to == SVT_FLOAT10) && from == SVT_UINT)
			return "uintBitsToFloat";
		else if (to == SVT_INT && from == SVT_FLOAT)
			return "floatBitsToInt";
		else if (to == SVT_UINT && from == SVT_FLOAT)
			return "floatBitsToUint";
	}

	ASSERT(0);
	return "ERROR missing components in GetBitcastOp()";
}*/


// Helper function to print floats with full precision
static std::string printFloat(float f)
{
	char temp[30];

	snprintf(temp, 30, "%.9g", f);
	char * ePos = strchr(temp, 'e');
	char * pointPos = strchr(temp, '.');

	if (ePos == NULL && pointPos == NULL && !fpcheck(f))
		return std::string(temp) + ".0";
	else
		return std::string(temp);
}

// Helper function to print out a single 32-bit immediate value in desired format
static std::string printImmediate32(uint32_t value, SHADER_VARIABLE_TYPE eType)
{
	std::ostringstream oss;
	int needsParenthesis = 0;

	// Print floats as bit patterns.
	if ((eType == SVT_FLOAT || eType == SVT_FLOAT16 || eType == SVT_FLOAT10) && fpcheck(*((float *)(&value))))
	{
		oss << "float(";
		eType = SVT_INT;
		needsParenthesis = 1;
	}

	switch (eType)
	{
	default:
		ASSERT(0);
	case SVT_INT:
	case SVT_INT16:
	case SVT_INT12:
		// Need special handling for anything >= uint 0x3fffffff
		if (value > 0x3ffffffe)
			oss << "int(0x" << std::hex << value << "u)";
		else
			oss << "0x" << std::hex << value << "";
		break;
	case SVT_UINT:
	case SVT_UINT16:
		oss << "0x" << std::hex << value << "u";
		break;
	case SVT_FLOAT:
	case SVT_FLOAT10:
	case SVT_FLOAT16:
		oss << printFloat(*((float *)(&value)));
		break;
	case SVT_BOOL:
		if (value == 0)
			oss << "false";
		else
			oss << "true";
	}
	if (needsParenthesis)
		oss << ")";

	return oss.str();
}

std::string ToMetal::TranslateVariableName(const Operand* psOperand, uint32_t ui32TOFlag, uint32_t* pui32IgnoreSwizzle, uint32_t ui32CompMask, int *piRebase)
{
	std::ostringstream oss;
	int numParenthesis = 0;
	int hasCtor = 0;
	int needsBoolUpscale = 0; // If nonzero, bools need * 0xffffffff in them
	SHADER_VARIABLE_TYPE requestedType = TypeFlagsToSVTType(ui32TOFlag);
	SHADER_VARIABLE_TYPE eType = psOperand->GetDataType(psContext, requestedType);
	int numComponents = psOperand->GetNumSwizzleElements(ui32CompMask);
	int requestedComponents = 0;
	int scalarWithSwizzle = 0;

	*pui32IgnoreSwizzle = 0;

	if (psOperand->eType == OPERAND_TYPE_TEMP)
	{
		// Check for scalar
		if (psContext->psShader.GetTempComponentCount(eType, psOperand->ui32RegisterNumber) == 1 && psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
		{
			scalarWithSwizzle = 1; // Going to need a constructor
		}
	}

	if (psOperand->eType == OPERAND_TYPE_INPUT)
	{
		// Check for scalar
		if (psContext->psShader.abScalarInput[(int)psOperand->GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase)][psOperand->ui32RegisterNumber] & psOperand->GetAccessMask()
			&& psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
		{
			scalarWithSwizzle = 1;
			*pui32IgnoreSwizzle = 1;
		}
	}

	if (piRebase)
		*piRebase = 0;

	if (ui32TOFlag & TO_AUTO_EXPAND_TO_VEC2)
		requestedComponents = 2;
	else if (ui32TOFlag & TO_AUTO_EXPAND_TO_VEC3)
		requestedComponents = 3;
	else if (ui32TOFlag & TO_AUTO_EXPAND_TO_VEC4)
		requestedComponents = 4;

	requestedComponents = std::max(requestedComponents, numComponents);

	if (!(ui32TOFlag & (TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY | TO_FLAG_DECLARATION_NAME)))
	{
		if (psOperand->eType == OPERAND_TYPE_IMMEDIATE32 || psOperand->eType == OPERAND_TYPE_IMMEDIATE64)
		{
			// Mark the operand type to match whatever we're asking for in the flags.
			((Operand *)psOperand)->aeDataType[0] = requestedType;
			((Operand *)psOperand)->aeDataType[1] = requestedType;
			((Operand *)psOperand)->aeDataType[2] = requestedType;
			((Operand *)psOperand)->aeDataType[3] = requestedType;
		}

		if (AreTypesCompatible(eType, ui32TOFlag) == 0)
		{
			if (CanDoDirectCast(eType, requestedType))
			{
				oss << GetConstructorForType(psContext, requestedType, requestedComponents, false) << "(";
				numParenthesis++;
				hasCtor = 1;
				if (eType == SVT_BOOL)
					needsBoolUpscale = 1;
			}
			else
			{
				// Direct cast not possible, need to do bitcast.
				oss << ""<< GetConstructorForTypeMetal(requestedType, requestedComponents) << "(";
				hasCtor = 1;
				numParenthesis++;
			}
		}

		// Add ctor if needed (upscaling). Type conversion is already handled above, so here we must
		// use the original type to not make type conflicts in bitcasts
		if (((numComponents < requestedComponents)||(scalarWithSwizzle != 0)) && (hasCtor == 0))
		{
			oss << GetConstructorForType(psContext, requestedType, requestedComponents, false) << "(";

			numParenthesis++;
			hasCtor = 1;
		}
	}


	switch(psOperand->eType)
	{
		case OPERAND_TYPE_IMMEDIATE32:
		{
			if(psOperand->iNumComponents == 1)
			{
				oss << printImmediate32(*((unsigned int*)(&psOperand->afImmediates[0])), requestedType);
			}
			else
			{
				int i;
				int firstItemAdded = 0;
				if (hasCtor == 0)
				{
					oss << GetConstructorForTypeMetal(requestedType, requestedComponents) << "(";
					numParenthesis++;
					hasCtor = 1;
				}
				for (i = 0; i < 4; i++)
				{
					uint32_t uval;
					if (!(ui32CompMask & (1 << i)))
						continue;

					if (firstItemAdded)
						oss << ", ";
					uval = *((uint32_t*)(&psOperand->afImmediates[i >= psOperand->iNumComponents ? psOperand->iNumComponents-1 : i]));
					oss << printImmediate32(uval, requestedType);
					firstItemAdded = 1;
				}
				oss << ")";
				*pui32IgnoreSwizzle = 1;
				numParenthesis--;
			}
			break;
		}
		case OPERAND_TYPE_IMMEDIATE64:
		{
			ASSERT(0); // doubles not supported on Metal
			break;
		}
		case OPERAND_TYPE_INPUT:
		{
      RegisterSpace regSpace = psOperand->GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);
			switch(psOperand->iIndexDims)
			{
				case INDEX_2D:
				{
					const ShaderInfo::InOutSignature *psSig = NULL;
					psContext->psShader.sInfo.GetInputSignatureFromRegister(psOperand->ui32RegisterNumber, psOperand->ui32CompMask, &psSig);
					if ((psSig->eSystemValueType == NAME_POSITION && psSig->ui32SemanticIndex == 0) ||
						(psSig->semanticName == "POS" && psSig->ui32SemanticIndex == 0) ||
						(psSig->semanticName == "SV_POSITION" && psSig->ui32SemanticIndex == 0))
					{
						// Shouldn't happen on Metal?
						ASSERT(0);
						break;
//						bcatcstr(glsl, "gl_in");
//						TranslateOperandIndex(psOperand, 0);//Vertex index
//						bcatcstr(glsl, ".gl_Position");
					}
					else
					{
						oss << psContext->GetDeclaredInputName(*psOperand, piRebase, 0, pui32IgnoreSwizzle);

						oss << TranslateOperandIndex(psOperand, 0);//Vertex index
					}
					break;
				}
				default:
				{
					if(psOperand->eIndexRep[0] == OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE)
					{
						ASSERT(psContext->psShader.aIndexedInput[(int)regSpace][psOperand->ui32RegisterNumber] != 0);
						oss << "phase" << psContext->currentPhase << "_Input" << (int)regSpace << "_" << psOperand->ui32RegisterNumber << "[";
						oss << TranslateOperand(psOperand->m_SubOperands[0].get(), TO_FLAG_INTEGER);
						oss << "]";
					}
					else
					{
						if(psContext->psShader.aIndexedInput[(int)regSpace][psOperand->ui32RegisterNumber] != 0)
						{
							const uint32_t parentIndex = psContext->psShader.aIndexedInputParents[(int)regSpace][psOperand->ui32RegisterNumber];
							oss << "phase" << psContext->currentPhase << "_Input" << (int)regSpace << "_" << parentIndex << "[" << (psOperand->ui32RegisterNumber - parentIndex) << "]";
						}
						else
						{
							oss << psContext->GetDeclaredInputName(*psOperand, piRebase, 0, pui32IgnoreSwizzle);
						}
					}
					break;
				}
			}
			break;
		}
		case OPERAND_TYPE_OUTPUT:
		case OPERAND_TYPE_OUTPUT_DEPTH:
		case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
		case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
		{

			int stream = 0;
			oss << psContext->GetDeclaredOutputName(*psOperand, &stream, pui32IgnoreSwizzle, piRebase, 0);
			if (psOperand->m_SubOperands[0].get())
			{
				oss << "[";
				oss << TranslateOperand(psOperand->m_SubOperands[0].get(), TO_AUTO_BITCAST_TO_INT);
				oss << "]";
			}
			break;
		}
		case OPERAND_TYPE_TEMP:
		{
			SHADER_VARIABLE_TYPE eTempType = psOperand->GetDataType(psContext);
			oss << HLSLCC_TEMP_PREFIX;
			ASSERT(psOperand->ui32RegisterNumber < 0x10000); // Sanity check after temp splitting.
			switch (eTempType)
			{
			case SVT_FLOAT:
				ASSERT(psContext->psShader.psFloatTempSizes[psOperand->ui32RegisterNumber] != 0);
				if (psContext->psShader.psFloatTempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			case SVT_FLOAT16:
				ASSERT(psContext->psShader.psFloat16TempSizes[psOperand->ui32RegisterNumber] != 0);
				oss << ("16_");
				if (psContext->psShader.psFloat16TempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			case SVT_FLOAT10:
				ASSERT(psContext->psShader.psFloat10TempSizes[psOperand->ui32RegisterNumber] != 0);
				oss << ("10_");
				if (psContext->psShader.psFloat10TempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			case SVT_INT:
				ASSERT(psContext->psShader.psIntTempSizes[psOperand->ui32RegisterNumber] != 0);
				oss << ("i");
				if (psContext->psShader.psIntTempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			case SVT_INT16:
				ASSERT(psContext->psShader.psInt16TempSizes[psOperand->ui32RegisterNumber] != 0);
				oss << ("i16_");
				if (psContext->psShader.psInt16TempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			case SVT_INT12:
				ASSERT(psContext->psShader.psInt12TempSizes[psOperand->ui32RegisterNumber] != 0);
				oss << ("i12_");
				if (psContext->psShader.psInt12TempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			case SVT_UINT:
				ASSERT(psContext->psShader.psUIntTempSizes[psOperand->ui32RegisterNumber] != 0);
				oss << ("u");
				if (psContext->psShader.psUIntTempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			case SVT_UINT16:
				ASSERT(psContext->psShader.psUInt16TempSizes[psOperand->ui32RegisterNumber] != 0);
				oss << ("u16_");
				if (psContext->psShader.psUInt16TempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			case SVT_DOUBLE:
				ASSERT(psContext->psShader.psDoubleTempSizes[psOperand->ui32RegisterNumber] != 0);
				oss << ("d");
				if (psContext->psShader.psDoubleTempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			case SVT_BOOL:
				ASSERT(psContext->psShader.psBoolTempSizes[psOperand->ui32RegisterNumber] != 0);
				oss << ("b");
				if (psContext->psShader.psBoolTempSizes[psOperand->ui32RegisterNumber] == 1 && pui32IgnoreSwizzle)
					*pui32IgnoreSwizzle = 1;
				break;
			default:
				ASSERT(0 && "Should never get here!");
			}
			oss << psOperand->ui32RegisterNumber;
			break;
		}
		case OPERAND_TYPE_SPECIAL_IMMCONSTINT:
		case OPERAND_TYPE_SPECIAL_IMMCONST:
		case OPERAND_TYPE_SPECIAL_OUTBASECOLOUR:
		case OPERAND_TYPE_SPECIAL_OUTOFFSETCOLOUR:
		case OPERAND_TYPE_SPECIAL_FOG:
		case OPERAND_TYPE_SPECIAL_ADDRESS:
		case OPERAND_TYPE_SPECIAL_LOOPCOUNTER:
		case OPERAND_TYPE_SPECIAL_TEXCOORD:
		{
			ASSERT(0 && "DX9 shaders no longer supported!");
			break;
		}
		case OPERAND_TYPE_SPECIAL_POSITION:
		{
			ASSERT(0 && "TODO normal shader support");
//			bcatcstr(glsl, "gl_Position");
			break;
		}
		case OPERAND_TYPE_SPECIAL_POINTSIZE:
		{
			ASSERT(0 && "TODO normal shader support");
			//			bcatcstr(glsl, "gl_PointSize");
			break;
		}
		case OPERAND_TYPE_CONSTANT_BUFFER:
		{
			const ConstantBuffer* psCBuf = NULL;
			const ShaderVarType* psVarType = NULL;
			int32_t index = -1;
			std::vector<uint32_t> arrayIndices;
			bool isArray;
			psContext->psShader.sInfo.GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, psOperand->aui32ArraySizes[0], &psCBuf);
			ASSERT(psCBuf != NULL);

			if(ui32TOFlag & TO_FLAG_DECLARATION_NAME)
			{
				pui32IgnoreSwizzle[0] = 1;
			}
			std::string cbName = "";
			if(psCBuf)
			{
				//$Globals.
				if(psCBuf->name[0] == '$')
				{
					cbName = "Globals";
				}
				else
				{
					cbName = psCBuf->name;
				}
				cbName += ".";
			}

			if(ui32TOFlag & TO_FLAG_DECLARATION_NAME)
			{
				pui32IgnoreSwizzle[0] = 1;
			}

			if((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
			{
				//Work out the variable name. Don't apply swizzle to that variable yet.
				int32_t rebase = 0;

				if(psCBuf)
				{
					uint32_t componentsNeeded = 1;
					uint32_t minSwiz = 3;
					uint32_t maxSwiz = 0;
					if (psOperand->eSelMode != OPERAND_4_COMPONENT_SELECT_1_MODE)
					{
						int i;
						for (i = 0; i < 4; i++)
						{
							if ((ui32CompMask & (1 << i)) == 0)
								continue;
							minSwiz = std::min(minSwiz, psOperand->aui32Swizzle[i]);
							maxSwiz = std::max(maxSwiz, psOperand->aui32Swizzle[i]);
						}
						componentsNeeded = maxSwiz - minSwiz + 1;
					}
					else
					{
						minSwiz = maxSwiz = 1;
					}

					// When we have a component mask that doesn't have .x set (this basically only happens when we manually open operands into components)
					// We have to pull down the swizzle array to match the first bit that's actually set
					uint32_t tmpSwizzle[4] = { 0 };
					int firstBitSet = 0;
					if (ui32CompMask == 0)
						ui32CompMask = 0xf;
					while ((ui32CompMask & (1 << firstBitSet)) == 0)
						firstBitSet++;
					std::copy(&psOperand->aui32Swizzle[firstBitSet], &psOperand->aui32Swizzle[4], &tmpSwizzle[0]);

					ShaderInfo::GetShaderVarFromOffset(psOperand->aui32ArraySizes[1], tmpSwizzle, psCBuf, &psVarType, &isArray, &arrayIndices, &rebase, psContext->flags);
					if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE || ((componentsNeeded+minSwiz) <= psVarType->Columns))
					{
            if (psVarType->Parent && psVarType->Parent->Elements > 1 && index == -1 && psOperand->m_SubOperands[1])
            {
              std::string fullName = ShaderInfo::GetShaderVarIndexedFullName(psVarType->Parent, arrayIndices);
              oss << cbName << fullName << "[";
              SHADER_VARIABLE_TYPE eType = psOperand->m_SubOperands[1]->GetDataType(psContext);
              oss << TranslateOperand(psOperand->m_SubOperands[1].get(), TO_FLAG_UNSIGNED_INTEGER, 1);
              oss << " / " << (getStrideSizeForDynamicIndexing(psVarType->Parent) / 16);
              oss << "]." << psVarType->name.c_str();
            }
            else
            {
              // Simple case: just access one component
              std::string fullName = ShaderInfo::GetShaderVarIndexedFullName(psVarType, arrayIndices);

              if (((psContext->flags & HLSLCC_FLAG_TRANSLATE_MATRICES) != 0) && ((psVarType->Class == SVC_MATRIX_ROWS) || (psVarType->Class == SVC_MATRIX_COLUMNS)))
              {
                // We'll need to add the prefix only to the last section of the name
                size_t commaPos = fullName.find_last_of('.');
                char prefix[256];
                sprintf(prefix, HLSLCC_TRANSLATE_MATRIX_FORMAT_STRING, psVarType->Rows, psVarType->Columns);
                if (commaPos == std::string::npos)
                {
                  fullName.insert(0, prefix);
                }
                else
                {
                  fullName.insert(commaPos + 1, prefix);
                }

                oss << cbName << fullName;
              }
              else
              {
                oss << cbName << fullName;
              }
            }
					}
					else
					{
						// Non-simple case: build vec4 and apply mask
						uint32_t i;
						std::vector<uint32_t> tmpArrayIndices;
						bool tmpIsArray;
						int32_t tmpRebase;
						int firstItemAdded = 0;

            oss << GetConstructorForType(psContext, psVarType->Type, GetNumberBitsSet(ui32CompMask), false) << "(";
						for (i = 0; i < 4; i++)
						{
							const ShaderVarType *tmpVarType = NULL;
							if ((ui32CompMask & (1 << i)) == 0)
								continue;
							tmpRebase = 0;
              if (firstItemAdded != 0)
              {
                oss << ", ";
              }
              else
              {
                firstItemAdded = 1;
              }

							memset(tmpSwizzle, 0, sizeof(uint32_t) * 4);
							std::copy(&psOperand->aui32Swizzle[i], &psOperand->aui32Swizzle[4], &tmpSwizzle[0]);

							ShaderInfo::GetShaderVarFromOffset(psOperand->aui32ArraySizes[1], tmpSwizzle, psCBuf, &tmpVarType, &tmpIsArray, &tmpArrayIndices, &tmpRebase, psContext->flags);
							std::string fullName = ShaderInfo::GetShaderVarIndexedFullName(tmpVarType, tmpArrayIndices);

              {
                  std::string indexName;
                  SHADER_VARIABLE_TYPE eType = psOperand->m_SubOperands[1]->GetDataType(psContext);
                  indexName = TranslateOperand(psOperand->m_SubOperands[1].get(), TO_FLAG_UNSIGNED_INTEGER, 1);
                  indexName += " / ";
                  char tmp[64];
                  sprintf(tmp, "%i", getStrideSizeForDynamicIndexing(psVarType->Parent) / 16);
                  indexName += std::string(tmp);

                  int k = fullName.find("[");
                  int k2 = fullName.find("]");

                  fullName.erase(k + 1, k2 - k - 1);
                  fullName.insert(k + 1, indexName);
              }

							if (tmpVarType->Class == SVC_SCALAR)
							{
                oss << cbName << fullName;
							}
							else
							{
								uint32_t swizzle;
								tmpRebase /= 4; // 0 => 0, 4 => 1, 8 => 2, 12 /= 3
								swizzle = psOperand->aui32Swizzle[i] - tmpRebase;

                oss << cbName << fullName << "." << ("xyzw"[swizzle]);
							}
						}
            oss << ")";
						// Clear rebase, we've already done it.
						rebase = 0;
						// Also swizzle.
						*pui32IgnoreSwizzle = 1;
					}
				}
				else // We don't have a semantic for this variable, so try the raw dump appoach.
				{
					ASSERT(0);
					//bformata(glsl, "cb%d.data", psOperand->aui32ArraySizes[0]);//
					//index = psOperand->aui32ArraySizes[1];
				}

				if (isArray)
					index = arrayIndices.back();

				//Dx9 only?
				if(psOperand->m_SubOperands[0].get() != NULL)
				{
					// Array of matrices is treated as array of vec4s in HLSL,
					// but that would mess up uniform types in GLSL. Do gymnastics.
					uint32_t opFlags = TO_FLAG_INTEGER;

					if (((psVarType->Class == SVC_MATRIX_COLUMNS) || (psVarType->Class == SVC_MATRIX_ROWS)) && (psVarType->Elements > 1) && ((psContext->flags & HLSLCC_FLAG_TRANSLATE_MATRICES) == 0))
					{
						// Special handling for matrix arrays
            oss << "[(" << TranslateOperand(psOperand->m_SubOperands[0].get(), opFlags) << ") / 4]";
						{
              oss << "[((" << TranslateOperand(psOperand->m_SubOperands[0].get(), opFlags, OPERAND_4_COMPONENT_MASK_X) << ") %% 4)]";
						}
					}
					else
					{
						oss << "[" << TranslateOperand(psOperand->m_SubOperands[0].get(), opFlags) << "]";
					}
				}
				else
				if(index != -1 && psOperand->m_SubOperands[1].get() != NULL)
				{
					// Array of matrices is treated as array of vec4s in HLSL,
					// but that would mess up uniform types in GLSL. Do gymnastics.
					SHADER_VARIABLE_TYPE eType = psOperand->m_SubOperands[1].get()->GetDataType(psContext);
					uint32_t opFlags = TO_FLAG_INTEGER;
					if (eType != SVT_INT && eType != SVT_UINT)
						opFlags = TO_AUTO_BITCAST_TO_INT;

					if (((psVarType->Class == SVC_MATRIX_COLUMNS) ||( psVarType->Class == SVC_MATRIX_ROWS)) && (psVarType->Elements > 1) && ((psContext->flags & HLSLCC_FLAG_TRANSLATE_MATRICES) == 0))
					{
						// Special handling for matrix arrays
            oss << "[(" << TranslateOperand(psOperand->m_SubOperands[1].get(), opFlags) << " + " << index << ") / 4]";
						{
              oss << "[((" << TranslateOperand(psOperand->m_SubOperands[1].get(), opFlags) << " + " << index << ") %% 4)]";
						}
					}
					else
					{
            oss << "[" << TranslateOperand(psOperand->m_SubOperands[1].get(), opFlags);
						if (index != 0)
            {
              oss << " + " << index << "]";
            }
						else
            {
              oss << "]";
            }
					}
				}
				else if(index != -1)
				{
					if (((psVarType->Class == SVC_MATRIX_COLUMNS) || (psVarType->Class == SVC_MATRIX_ROWS)) && (psVarType->Elements > 1) && ((psContext->flags & HLSLCC_FLAG_TRANSLATE_MATRICES) == 0))
					{
						// Special handling for matrix arrays, open them up into vec4's
						size_t matidx = index / 4;
						size_t rowidx = index - (matidx*4);
            oss << "[" << matidx << "][" << rowidx << "]";
					}
					else
					{
            oss << "[" << index << "]";
					}
				}
				else if(psOperand->m_SubOperands[1].get() != NULL)
				{
					//bcatcstr(glsl, "[");
					//TranslateOperand(psOperand->m_SubOperands[1].get(), TO_FLAG_INTEGER);
					//bcatcstr(glsl, "]");
				}

				if(psVarType && psVarType->Class == SVC_VECTOR && !*pui32IgnoreSwizzle)
				{
					switch(rebase)
					{
						case 4:
						{
							if(psVarType->Columns == 2)
							{
								//.x(GLSL) is .y(HLSL). .y(GLSL) is .z(HLSL)
                  oss << ".xxyx";
							}
							else if(psVarType->Columns == 3)
							{
								//.x(GLSL) is .y(HLSL). .y(GLSL) is .z(HLSL) .z(GLSL) is .w(HLSL)
                  oss << ".xxyz";
							}
							break;
						}
						case 8:
						{
							if(psVarType->Columns == 2)
							{
								//.x(GLSL) is .z(HLSL). .y(GLSL) is .w(HLSL)
                oss << ".xxxy";
							}
							break;
						}
						case 0:
						default:
						{
							//No rebase, but extend to vec4 if needed
							uint32_t maxComp = psOperand->GetMaxComponent();
							if(psVarType->Columns == 2 && maxComp > 2)
							{
                  oss << ".xyxx";
							}
							else if(psVarType->Columns == 3 && maxComp > 3)
							{
								oss << ".xyzx";
							}
							break;
						}
					}
				}

				if(psVarType && psVarType->Class == SVC_SCALAR)
				{
					*pui32IgnoreSwizzle = 1;

          // CB arrays are all declared as 4-component vectors to match DX11 data layout.
          // Therefore add swizzle here to access the element corresponding to the scalar var.
          if ((psVarType->Elements > 0) && (psContext->psShader.eShaderType == COMPUTE_SHADER))
          {
              oss << ".x";
          }
				}
			}
			break;
		}
		case OPERAND_TYPE_RESOURCE:
		{
			oss << ResourceName(RGROUP_TEXTURE, psOperand->ui32RegisterNumber);
			*pui32IgnoreSwizzle = 1;
			break;
		}
		case OPERAND_TYPE_SAMPLER:
		{
			oss << ResourceName(RGROUP_SAMPLER, psOperand->ui32RegisterNumber);
			*pui32IgnoreSwizzle = 1;
			break;
		}
		case OPERAND_TYPE_FUNCTION_BODY:
		{
			ASSERT(0);
			break;
		}
		case OPERAND_TYPE_INPUT_FORK_INSTANCE_ID:
		case OPERAND_TYPE_INPUT_JOIN_INSTANCE_ID:
		{
			// Not supported on Metal
			ASSERT(0);
			break;
		}
		case OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
		{
			oss << "ImmCB_" << psContext->currentPhase
				<< "_" << psOperand->ui32RegisterNumber
				<< "_" << psOperand->m_Rebase;
			if (psOperand->m_SubOperands[0].get())
			{
				//Indexes must be integral. Offset is already taken care of above.
				oss << "[" << TranslateOperand(psOperand->m_SubOperands[0].get(), TO_FLAG_INTEGER) << "]";
			}
			if (psOperand->m_Size == 1)
				*pui32IgnoreSwizzle = 1;
			break;
		}
		case OPERAND_TYPE_INPUT_DOMAIN_POINT:
		{
			// Not supported on Metal
			ASSERT(0);
			break;
		}
		case OPERAND_TYPE_INPUT_CONTROL_POINT:
		{
			// Not supported on Metal
			ASSERT(0);
			break;
		}
		case OPERAND_TYPE_NULL:
		{
			// Null register, used to discard results of operations
			oss << "//null";
			break;
		}
		case OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID:
		{
			// Not supported on Metal
			ASSERT(0);
			break;
		}
		case OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
		{
			oss << "mtl_CoverageMask";
			*pui32IgnoreSwizzle = 1;
			break;
		}
		case OPERAND_TYPE_INPUT_COVERAGE_MASK:
		{
			oss << "mtl_CoverageMask";
			//Skip swizzle on scalar types.
			*pui32IgnoreSwizzle = 1;
			break;
		}
		case OPERAND_TYPE_INPUT_THREAD_ID://SV_DispatchThreadID
		{
			oss << "mtl_ThreadID";
			break;
		}
		case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP://SV_GroupThreadID
		{
			oss << "mtl_ThreadIDInGroup";
			break;
		}
		case OPERAND_TYPE_INPUT_THREAD_GROUP_ID://SV_GroupID
		{
			oss << "mtl_ThreadGroupID";
			break;
		}
		case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED://SV_GroupIndex
		{
			oss << "mtl_ThreadIndexInThreadGroup";
			*pui32IgnoreSwizzle = 1; // No swizzle meaningful for scalar.
			break;
		}
		case OPERAND_TYPE_UNORDERED_ACCESS_VIEW:
		{
			oss << ResourceName(RGROUP_UAV, psOperand->ui32RegisterNumber);
			break;
		}
		case OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY:
		{
			oss << "TGSM" << psOperand->ui32RegisterNumber;
			*pui32IgnoreSwizzle = 1;
			break;
		}
		case OPERAND_TYPE_INPUT_PRIMITIVEID:
		{
			// Not supported on Metal
			ASSERT(0);
			break;
		}
		case OPERAND_TYPE_INDEXABLE_TEMP:
		{
			oss << "TempArray" << psOperand->aui32ArraySizes[0] << "[";
			if (psOperand->aui32ArraySizes[1] != 0 || !psOperand->m_SubOperands[1].get())
				oss << psOperand->aui32ArraySizes[1];

			if(psOperand->m_SubOperands[1].get())
			{
				if (psOperand->aui32ArraySizes[1] != 0)
					oss << "+";
				oss << TranslateOperand(psOperand->m_SubOperands[1].get(), TO_FLAG_INTEGER);

			}
			oss << "]";
			break;
		}
		case OPERAND_TYPE_STREAM:
		{
			// Not supported on Metal
			ASSERT(0);
			break;
		}
		case OPERAND_TYPE_INPUT_GS_INSTANCE_ID:
		{
			// Not supported on Metal
			ASSERT(0);
			break;
		}
		case OPERAND_TYPE_THIS_POINTER:
		{
			ASSERT(0); // Nope.
			break;
		}
		case OPERAND_TYPE_INPUT_PATCH_CONSTANT:
		{
			// Not supported on Metal
			ASSERT(0);

			break;
		}
		default:
		{
			ASSERT(0);
			break;
		}
	}

	if (hasCtor && (*pui32IgnoreSwizzle == 0))
	{
		oss << TranslateOperandSwizzle(psOperand, ui32CompMask, piRebase ? *piRebase : 0);
		*pui32IgnoreSwizzle = 1;
	}

	if (needsBoolUpscale)
	{
		if (requestedType == SVT_UINT || requestedType == SVT_UINT16 || requestedType == SVT_UINT8)
			oss << ") * 0xffffffffu";
		else
			oss << ") * int(0xffffffffu)";
		numParenthesis--;
	}

	while (numParenthesis != 0)
	{
		oss << ")";
		numParenthesis--;
	}
	return oss.str();
}

std::string ToMetal::TranslateOperand(const Operand* psOperand, uint32_t ui32TOFlag, uint32_t ui32ComponentMask)
{
	std::ostringstream oss;
	uint32_t ui32IgnoreSwizzle = 0;
	int iRebase = 0;

	// in single-component mode there is no need to use mask
	if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
		ui32ComponentMask = OPERAND_4_COMPONENT_MASK_ALL;

	if(ui32TOFlag & TO_FLAG_NAME_ONLY)
	{
		return TranslateVariableName(psOperand, ui32TOFlag, &ui32IgnoreSwizzle, OPERAND_4_COMPONENT_MASK_ALL, &iRebase);
	}

	switch (psOperand->eModifier)
	{
		case OPERAND_MODIFIER_NONE:
		{
			break;
		}
		case OPERAND_MODIFIER_NEG:
		{
			oss << ("(-");
			break;
		}
		case OPERAND_MODIFIER_ABS:
		{
			oss << ("abs(");
			break;
		}
		case OPERAND_MODIFIER_ABSNEG:
		{
			oss << ("-abs(");
			break;
		}
	}

	oss << TranslateVariableName(psOperand, ui32TOFlag, &ui32IgnoreSwizzle, ui32ComponentMask, &iRebase);

	if (!ui32IgnoreSwizzle)
	{
		oss << TranslateOperandSwizzle(psOperand, ui32ComponentMask, iRebase);
	}

	switch (psOperand->eModifier)
	{
		case OPERAND_MODIFIER_NONE:
		{
			break;
		}
		case OPERAND_MODIFIER_NEG:
		{
			oss << (")");
			break;
		}
		case OPERAND_MODIFIER_ABS:
		{
			oss << (")");
			break;
		}
		case OPERAND_MODIFIER_ABSNEG:
		{
			oss << (")");
			break;
		}
	}
	return oss.str();
}
