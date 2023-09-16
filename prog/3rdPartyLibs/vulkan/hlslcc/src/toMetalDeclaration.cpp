
#include "internal_includes/toMetal.h"
#include "internal_includes/debug.h"
#include "internal_includes/HLSLccToolkit.h"
#include "internal_includes/Declaration.h"
#include <algorithm>
#include <sstream>

#include "buffBindPoints.h"

#ifdef _MSC_VER
#define fpcheck(x) (_isnan(x) || !_finite(x))
#else
#include <cmath>
#define fpcheck(x) ((std::isnan(x)) || (std::isinf(x)))
#endif


bool ToMetal::TranslateSystemValue(const Operand *psOperand, const ShaderInfo::InOutSignature *sig, std::string &result, uint32_t *pui32IgnoreSwizzle, bool isIndexed, bool isInput, bool *outSkipPrefix)
{
	if (sig && (sig->eSystemValueType == NAME_POSITION || (sig->semanticName == "POS" && sig->ui32SemanticIndex == 0)) && psContext->psShader.eShaderType == VERTEX_SHADER)
	{
		result = "mtl_Position";
		return true;
	}

	if (sig)
	{
	switch (sig->eSystemValueType)
	{
	case NAME_POSITION:
		ASSERT(psContext->psShader.eShaderType == PIXEL_SHADER);
		result = "mtl_FragCoord";
		if (outSkipPrefix != NULL) *outSkipPrefix = true;
		return true;
	case NAME_RENDER_TARGET_ARRAY_INDEX:
		result = "mtl_Layer";
		if (outSkipPrefix != NULL) *outSkipPrefix = true;
		return true;
	case NAME_CLIP_DISTANCE:
		result = "mtl_ClipDistance";
		if (outSkipPrefix != NULL) *outSkipPrefix = true;
		return true;
		/*	case NAME_VIEWPORT_ARRAY_INDEX:
		result = "gl_ViewportIndex";
		if (puiIgnoreSwizzle)
		*puiIgnoreSwizzle = 1;
		return true;*/
	case NAME_VERTEX_ID:
		result = "mtl_VertexID";
		if (outSkipPrefix != NULL) *outSkipPrefix = true;
		if (pui32IgnoreSwizzle)
			*pui32IgnoreSwizzle = 1;
		return true;
	case NAME_INSTANCE_ID:
		result = "mtl_InstanceID";
		if (pui32IgnoreSwizzle)
			*pui32IgnoreSwizzle = 1;
		if (outSkipPrefix != NULL) *outSkipPrefix = true;
		return true;
	case NAME_IS_FRONT_FACE:
		result = "mtl_FrontFace";
		if (outSkipPrefix != NULL) *outSkipPrefix = true;
		if (pui32IgnoreSwizzle)
			*pui32IgnoreSwizzle = 1;
		return true;
	case NAME_SAMPLE_INDEX:
		result = "mtl_SampleID";
		if (outSkipPrefix != NULL) *outSkipPrefix = true;
		if (pui32IgnoreSwizzle)
			*pui32IgnoreSwizzle = 1;
		return true;

	default:
		break;
	}
	}

	switch (psOperand->eType)
	{
		case OPERAND_TYPE_INPUT_COVERAGE_MASK:
		case OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
			result = "mtl_CoverageMask";
			if (outSkipPrefix != NULL) *outSkipPrefix = true;
			if (pui32IgnoreSwizzle)
				*pui32IgnoreSwizzle = 1;
			return true;
		case OPERAND_TYPE_INPUT_THREAD_ID:
			result = "mtl_ThreadID";
			if (outSkipPrefix != NULL) *outSkipPrefix = true;
			return true;
		case OPERAND_TYPE_INPUT_THREAD_GROUP_ID:
			result = "mtl_ThreadGroupID";
			if (outSkipPrefix != NULL) *outSkipPrefix = true;
			return true;
		case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP:
			result = "mtl_ThreadIDInGroup";
			if (outSkipPrefix != NULL) *outSkipPrefix = true;
			return true;
		case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED:
			result = "mtl_ThreadIndexInThreadGroup";
			if (outSkipPrefix != NULL) *outSkipPrefix = true;
			if (pui32IgnoreSwizzle)
				*pui32IgnoreSwizzle = 1;
			return true;
		case OPERAND_TYPE_OUTPUT_DEPTH:
		case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
		case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
			result = "mtl_Depth";
			if (outSkipPrefix != NULL) *outSkipPrefix = true;
			if (pui32IgnoreSwizzle)
				*pui32IgnoreSwizzle = 1;
			return true;
		case OPERAND_TYPE_OUTPUT:
		case OPERAND_TYPE_INPUT:
		{
			std::ostringstream oss;
			ASSERT(sig != NULL);
			oss << sig->semanticName << sig->ui32SemanticIndex;
			result = oss.str();
			if (HLSLcc::WriteMaskToComponentCount(sig->ui32Mask) == 1 && pui32IgnoreSwizzle != NULL)
				*pui32IgnoreSwizzle = 1;
			return true;
		}
		default:
            ASSERT(0);
            break;
	}



	return false;
}

void ToMetal::DeclareBuiltinInput(const Declaration *psDecl)
{
	const SPECIAL_NAME eSpecialName = psDecl->asOperands[0].eSpecialName;

	switch (eSpecialName)
	{
	case NAME_POSITION:
		ASSERT(psContext->psShader.eShaderType == PIXEL_SHADER);
		m_StructDefinitions[""].m_Members.push_back("float4 mtl_FragCoord [[ position ]]");
		break;
	case NAME_RENDER_TARGET_ARRAY_INDEX:
#if 0
		// Only supported on a Mac
		m_StructDefinitions[""].m_Members.push_back("uint mtl_Layer [[ render_target_array_index ]]");
#else
		// Not on Metal
		ASSERT(0);
#endif
		break;
	case NAME_CLIP_DISTANCE:
		ASSERT(0); // Should never be an input
		break;
	case NAME_VIEWPORT_ARRAY_INDEX:
		// Not on Metal
		ASSERT(0);
		break;
	case NAME_INSTANCE_ID:
		m_StructDefinitions[""].m_Members.push_back("uint mtl_InstanceID [[ instance_id ]]");
		break;
	case NAME_IS_FRONT_FACE:
		m_StructDefinitions[""].m_Members.push_back("bool mtl_FrontFace [[ front_facing ]]");
		break;
	case NAME_SAMPLE_INDEX:
		m_StructDefinitions[""].m_Members.push_back("uint mtl_SampleID [[ sample_id ]]");
		break;
	case NAME_VERTEX_ID:
		m_StructDefinitions[""].m_Members.push_back("uint mtl_VertexID [[ vertex_id ]]");
		break;
	case NAME_PRIMITIVE_ID:
		// Not on Metal
		ASSERT(0);
		break;
	default:
		m_StructDefinitions[""].m_Members.push_back(std::string("float4 ").append(psDecl->asOperands[0].specialName));
		ASSERT(0); // Catch this to see what's happening
		break;
	}
}

void ToMetal::DeclareBuiltinOutput(const Declaration *psDecl)
{
	std::string out = GetOutputStructName();

	switch (psDecl->asOperands[0].eSpecialName)
	{
		case NAME_POSITION:
			m_StructDefinitions[out].m_Members.push_back("float4 mtl_Position [[ position ]]");
			break;
		case NAME_RENDER_TARGET_ARRAY_INDEX:
#if 0
			// Only supported on a Mac
			m_StructDefinitions[out].m_Members.push_back("uint mtl_Layer [[ render_target_array_index ]]");
#else
			// Not on Metal
			ASSERT(0);
#endif
			break;
		case NAME_CLIP_DISTANCE:
			m_StructDefinitions[out].m_Members.push_back("float4 mtl_ClipDistance [[ clip_distance ]]");
			break;

		case NAME_VIEWPORT_ARRAY_INDEX:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_VERTEX_ID:
			ASSERT(0); //VertexID is not an output
			break;
		case NAME_PRIMITIVE_ID:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_INSTANCE_ID:
			ASSERT(0); //InstanceID is not an output
			break;
		case NAME_IS_FRONT_FACE:
			ASSERT(0); //FrontFacing is not an output
			break;
		case NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_LINE_DENSITY_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_LINE_DETAIL_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_TRI_INSIDE_TESSFACTOR:
		case NAME_FINAL_QUAD_U_INSIDE_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		case NAME_FINAL_QUAD_V_INSIDE_TESSFACTOR:
			// Not on Metal
			ASSERT(0);
			break;
		default:
			// This might be SV_Position (because d3dcompiler is weird). Get signature and check
            const ShaderInfo::InOutSignature *sig = NULL;
			psContext->psShader.sInfo.GetOutputSignatureFromRegister(psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].GetAccessMask(), 0, &sig);
			ASSERT(sig != NULL);
			if (sig->eSystemValueType == NAME_POSITION && sig->ui32SemanticIndex == 0)
			{
				m_StructDefinitions[out].m_Members.push_back("float4 mtl_Position [[ position ]]");
				break;
			}

			ASSERT(0); // Wut
			break;
	}
}

static std::string BuildOperandTypeString(OPERAND_MIN_PRECISION ePrec, INOUT_COMPONENT_TYPE eType, int numComponents)
{
	SHADER_VARIABLE_TYPE t = SVT_FLOAT;
	switch (eType)
	{
		case INOUT_COMPONENT_FLOAT32:
			t = SVT_FLOAT;
			break;
		case INOUT_COMPONENT_UINT32:
			t = SVT_UINT;
			break;
		case INOUT_COMPONENT_SINT32:
			t = SVT_INT;
			break;
		default:
			ASSERT(0);
			break;
	}
	// Can be overridden by precision
	switch (ePrec)
	{
		case OPERAND_MIN_PRECISION_DEFAULT:
			break;

		case OPERAND_MIN_PRECISION_FLOAT_16:
			ASSERT(eType == INOUT_COMPONENT_FLOAT32);
			t = SVT_FLOAT16;
			break;

		case OPERAND_MIN_PRECISION_FLOAT_2_8:
			ASSERT(eType == INOUT_COMPONENT_FLOAT32);
			t = SVT_FLOAT10;
			break;

		case OPERAND_MIN_PRECISION_SINT_16:
			ASSERT(eType == INOUT_COMPONENT_SINT32);
			t = SVT_INT16;
			break;
		case OPERAND_MIN_PRECISION_UINT_16:
			ASSERT(eType == INOUT_COMPONENT_UINT32);
			t = SVT_UINT16;
			break;
	}
	return HLSLcc::GetConstructorForTypeMetal(t, numComponents);
}

void ToMetal::HandleOutputRedirect(const Declaration *psDecl, const std::string &typeName)
{
	const Operand *psOperand = &psDecl->asOperands[0];
	Shader& psShader = psContext->psShader;
	bstring glsl = *psContext->currentGLSLString;
	int needsRedirect = 0;
	const ShaderInfo::InOutSignature *psSig = NULL;

  RegisterSpace regSpace = psOperand->GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);

  if (psOperand->ui32RegisterNumber != -1)
  {
	  if (regSpace == RegisterSpace::per_vertex && psShader.asPhases[psContext->currentPhase].acOutputNeedsRedirect[psOperand->ui32RegisterNumber] == 0xff)
	  {
		  needsRedirect = 1;
	  }
	  else if (regSpace == RegisterSpace::per_patch && psShader.asPhases[psContext->currentPhase].acPatchConstantsNeedsRedirect[psOperand->ui32RegisterNumber] == 0xff)
	  {
		  needsRedirect = 1;
	  }
  }

	if (needsRedirect == 1)
	{
		// TODO What if this is indexed?
		ShaderPhase *psPhase = &psShader.asPhases[psContext->currentPhase];
		int comp = 0;
		uint32_t origMask = psOperand->ui32CompMask;

		ASSERT(psContext->psShader.aIndexedOutput[(int)regSpace][psOperand->ui32RegisterNumber] == 0);

		psContext->AddIndentation();
		bformata(psPhase->earlyMain, "%s phase%d_Output%d_%d;\n", typeName.c_str(), psContext->currentPhase, regSpace, psOperand->ui32RegisterNumber);

		psPhase->hasPostShaderCode = 1;
		psContext->currentGLSLString = &psPhase->postShaderCode;

		while (comp < 4)
		{
			int numComps = 0;
			int hasCast = 0;
			uint32_t mask, i;
			psSig = NULL;
			if (regSpace == RegisterSpace::per_vertex)
				psContext->psShader.sInfo.GetOutputSignatureFromRegister(psOperand->ui32RegisterNumber, 1 << comp, psContext->psShader.ui32CurrentVertexOutputStream, &psSig, true);
			else
				psContext->psShader.sInfo.GetPatchConstantSignatureFromRegister(psOperand->ui32RegisterNumber, 1 << comp, &psSig, true);

			// The register isn't necessarily packed full. Continue with the next component.
			if (psSig == NULL)
			{
				comp++;
				continue;
			}

			numComps = HLSLcc::GetNumberBitsSet(psSig->ui32Mask);
			mask = psSig->ui32Mask;

			((Operand *)psOperand)->ui32CompMask = 1 << comp;
			psContext->AddIndentation();
			bcatcstr(psPhase->postShaderCode, TranslateOperand(psOperand, TO_FLAG_NAME_ONLY).c_str());

			bcatcstr(psPhase->postShaderCode, " = ");

			if (psSig->eComponentType == INOUT_COMPONENT_SINT32)
			{
				bformata(psPhase->postShaderCode, "int(");
				hasCast = 1;
			}
			else if (psSig->eComponentType == INOUT_COMPONENT_UINT32)
			{
				bformata(psPhase->postShaderCode, "uint(");
				hasCast = 1;
			}
			bformata(psPhase->postShaderCode, "phase%d_Output%d_%d.", psContext->currentPhase, regSpace, psOperand->ui32RegisterNumber);
			// Print out mask
			for (i = 0; i < 4; i++)
			{
				if ((mask & (1 << i)) == 0)
					continue;

				bformata(psPhase->postShaderCode, "%c", "xyzw"[i]);
			}

			if (hasCast)
				bcatcstr(psPhase->postShaderCode, ")");
			comp += numComps;
			bcatcstr(psPhase->postShaderCode, ";\n");
		}

		psContext->currentGLSLString = &psContext->glsl;

		((Operand *)psOperand)->ui32CompMask = origMask;
		if (regSpace == RegisterSpace::per_vertex)
			psShader.asPhases[psContext->currentPhase].acOutputNeedsRedirect[psOperand->ui32RegisterNumber] = 0xfe;
		else
			psShader.asPhases[psContext->currentPhase].acPatchConstantsNeedsRedirect[psOperand->ui32RegisterNumber] = 0xfe;
	}
}

void ToMetal::HandleInputRedirect(const Declaration *psDecl, const std::string &typeName)
{
	Operand *psOperand = (Operand *)&psDecl->asOperands[0];
	Shader& psShader = psContext->psShader;
	bstring glsl = *psContext->currentGLSLString;
	int needsRedirect = 0;
	const ShaderInfo::InOutSignature *psSig = NULL;

  RegisterSpace regSpace = psOperand->GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);
	if (regSpace == RegisterSpace::per_vertex && psShader.asPhases[psContext->currentPhase].acInputNeedsRedirect[psOperand->ui32RegisterNumber] == 0xff)
	{
		needsRedirect = 1;
	}

	if (needsRedirect == 1)
	{
		// TODO What if this is indexed?
		ShaderPhase *psPhase = &psShader.asPhases[psContext->currentPhase];
		int needsLooping = 0;
		int i = 0;
		uint32_t origArraySize = 0;
		uint32_t origMask = psOperand->ui32CompMask;

		ASSERT(psContext->psShader.aIndexedInput[(int)regSpace][psOperand->ui32RegisterNumber] == 0);

		psContext->currentGLSLString = &psPhase->earlyMain;
		psContext->AddIndentation();

		bcatcstr(psPhase->earlyMain, "    ");
		bformata(psPhase->earlyMain, "%s phase%d_Input%d_%d;\n", typeName.c_str(), psContext->currentPhase, regSpace, psOperand->ui32RegisterNumber);

		// Do a conditional loop. In normal cases needsLooping == 0 so this is only run once.
		do
		{
			int comp = 0;
			bcatcstr(psPhase->earlyMain, "    ");
			if (needsLooping)
				bformata(psPhase->earlyMain, "phase%d_Input%d_%d[%d] = %s(", psContext->currentPhase, regSpace, psOperand->ui32RegisterNumber, i, typeName.c_str());
			else
				bformata(psPhase->earlyMain, "phase%d_Input%d_%d = %s(", psContext->currentPhase, regSpace, psOperand->ui32RegisterNumber, typeName.c_str());

			while (comp < 4)
			{
				int numComps = 0;
				int hasCast = 0;
				int hasSig = 0;
				if (regSpace == RegisterSpace::per_vertex)
					hasSig = psContext->psShader.sInfo.GetInputSignatureFromRegister(psOperand->ui32RegisterNumber, 1 << comp, &psSig, true);
				else
					hasSig = psContext->psShader.sInfo.GetPatchConstantSignatureFromRegister(psOperand->ui32RegisterNumber, 1 << comp, &psSig, true);

				if (hasSig)
				{
					numComps = HLSLcc::GetNumberBitsSet(psSig->ui32Mask);
					if (psSig->eComponentType != INOUT_COMPONENT_FLOAT32)
					{
						if (numComps > 1)
							bformata(psPhase->earlyMain, "float%d(", numComps);
						else
							bformata(psPhase->earlyMain, "float(");
						hasCast = 1;
					}

					// Override the array size of the operand so TranslateOperand call below prints the correct index
					if (needsLooping)
						psOperand->aui32ArraySizes[0] = i;

					// And the component mask
					psOperand->ui32CompMask = 1 << comp;

					bformata(psPhase->earlyMain, TranslateOperand(psOperand, TO_FLAG_NAME_ONLY).c_str());

					// Restore the original array size value and mask
					psOperand->ui32CompMask = origMask;
					if (needsLooping)
						psOperand->aui32ArraySizes[0] = origArraySize;

					if (hasCast)
						bcatcstr(psPhase->earlyMain, ")");
					comp += numComps;
				}
				else // no signature found -> fill with zero
				{
					bcatcstr(psPhase->earlyMain, "0");
					comp++;
				}
				
				if (comp < 4)
					bcatcstr(psPhase->earlyMain, ", ");
			}
			bcatcstr(psPhase->earlyMain, ");\n");

		} while ((--i) >= 0);

		psContext->currentGLSLString = &psContext->glsl;

		if (regSpace == RegisterSpace::per_vertex)
			psShader.asPhases[psContext->currentPhase].acInputNeedsRedirect[psOperand->ui32RegisterNumber] = 0xfe;
		else
			psShader.asPhases[psContext->currentPhase].acPatchConstantsNeedsRedirect[psOperand->ui32RegisterNumber] = 0xfe;
	}
}

static std::string TranslateResourceDeclaration(HLSLCrossCompilerContext* psContext,
	const Declaration *psDecl,
	bool isDepthSampler, bool isUAV)
{
	std::ostringstream oss;
	const ResourceBinding* psBinding = 0;
	const RESOURCE_DIMENSION eDimension = psDecl->value.eResourceDimension;
	const uint32_t ui32RegisterNumber = psDecl->asOperands[0].ui32RegisterNumber;
	REFLECT_RESOURCE_PRECISION ePrec = REFLECT_RESOURCE_PRECISION_UNKNOWN;
	RESOURCE_RETURN_TYPE eType = RETURN_TYPE_UNORM;
	std::string access = "sample";

	if (isUAV)
	{
		if ((psDecl->sUAV.ui32AccessFlags & ACCESS_FLAG_WRITE) != 0)
		{
			access = "write";
			if (psContext->psShader.eShaderType != COMPUTE_SHADER)
				psContext->m_Reflection.OnDiagnostics("This shader might not work on all Metal devices because of texture writes on non-compute shaders.", 0, false);
			
			if ((psDecl->sUAV.ui32AccessFlags & ACCESS_FLAG_READ) != 0)
			{
				access = "read_write";
			}
		}
		else
		{
			access = "read";
			eType = psDecl->sUAV.Type;
		}
		int found;
		found = psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_UAV, ui32RegisterNumber, &psBinding);
		if (found)
		{
			ePrec = psBinding->ePrecision;
			eType = (RESOURCE_RETURN_TYPE)psBinding->ui32ReturnType;
			// Figured out by reverse engineering bitcode. flags b00xx means float1, b01xx = float2, b10xx = float3 and b11xx = float4
		}

	}
	else
	{
		int found;
		found = psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_TEXTURE, ui32RegisterNumber, &psBinding);
		if (found)
		{
			eType = (RESOURCE_RETURN_TYPE)psBinding->ui32ReturnType;
			ePrec = psBinding->ePrecision;

			// TODO: it might make sense to propagate float earlier (as hlslcc might declare other variables depending on sampler prec)
			// metal supports ONLY float32 depth textures
			if(isDepthSampler)
			{
				switch(eDimension)
				{
					case RESOURCE_DIMENSION_TEXTURE2D: case RESOURCE_DIMENSION_TEXTURE2DMS: case RESOURCE_DIMENSION_TEXTURECUBE:
					case RESOURCE_DIMENSION_TEXTURE2DARRAY: case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
						ePrec = REFLECT_RESOURCE_PRECISION_HIGHP, eType = RETURN_TYPE_FLOAT; break;
					default:
						break;
				}
			}
		}
		if (eDimension == RESOURCE_DIMENSION_BUFFER)
			access = "read";
	}

	std::string typeName = HLSLcc::GetConstructorForTypeMetal(HLSLcc::ResourceReturnTypeToSVTType(eType, ePrec), 1);

	switch (eDimension)
	{
	case RESOURCE_DIMENSION_BUFFER:
	{
		oss << "texture1d<" << typeName << ", access::"<< access <<" >";
		return oss.str();
		break;
	}

	case RESOURCE_DIMENSION_TEXTURE1D:
	{
		oss << "texture1d<" << typeName << ", access::" << access << " >";
		return oss.str();
		break;
	}

	case RESOURCE_DIMENSION_TEXTURE2D:
	{
		oss << (isDepthSampler ? "depth2d<" : "texture2d<") << typeName << ", access::" << access << " >";
		return oss.str();
		break;
	}

	case RESOURCE_DIMENSION_TEXTURE2DMS:
	{
		oss << (isDepthSampler ? "depth2d_ms<" : "texture2d_ms<") << typeName << ", access::" << access << " >";
		return oss.str();
		break;
	}

	case RESOURCE_DIMENSION_TEXTURE3D:
	{
		oss << "texture3d<" << typeName << ", access::" << access << " >";
		return oss.str();
		break;
	}

	case RESOURCE_DIMENSION_TEXTURECUBE:
	{
		oss << (isDepthSampler ? "depthcube<" : "texturecube<") << typeName << ", access::" << access << " >";
		return oss.str();
		break;
	}

	case RESOURCE_DIMENSION_TEXTURE1DARRAY:
	{
		oss << "texture1d_array<" << typeName << ", access::" << access << " >";
		return oss.str();
		break;
	}

	case RESOURCE_DIMENSION_TEXTURE2DARRAY:
	{
		oss << (isDepthSampler ? "depth2d_array<" : "texture2d_array<") << typeName << ", access::" << access << " >";
		return oss.str();
		break;
	}

	case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
	{
		// Not really supported in Metal but let's print it here anyway
		oss << "texture2d_ms_array<" << typeName << ", access::" << access << " >";
		return oss.str();
		break;
	}

	case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
	{
		oss << (isDepthSampler ? "depthcube_array<" : "texturecube_array<") << typeName << ", access::" << access << " >";
		return oss.str();
		break;
	}
	default:
		ASSERT(0);
		oss << "texture2d<" << typeName << ", access::" << access << " >";
		return oss.str();
	}

}

static std::string GetInterpolationString(INTERPOLATION_MODE eMode)
{
	switch (eMode)
	{
		case INTERPOLATION_CONSTANT:
			return " [[ flat ]]";

		case INTERPOLATION_LINEAR:
			return "";

		case INTERPOLATION_LINEAR_CENTROID:
			return " [[ centroid ]]";

		case INTERPOLATION_LINEAR_NOPERSPECTIVE:
			return " [[ center_perspective ]]";

		case INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID:
			return " [[ centroid_noperspective ]]";

		case INTERPOLATION_LINEAR_SAMPLE:
			return " [[ sample_perspective ]]";

		case INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE:
			return " [[ sample_noperspective ]]";
		default:
			ASSERT(0);
			return "";
	}
}


void ToMetal::DeclareStructVariable(const std::string &parentName, const ShaderVar &var, bool withinCB, uint32_t cumulativeOffset)
{
	DeclareStructVariable(parentName, var.sType, withinCB, cumulativeOffset + var.ui32StartOffset);
}

void ToMetal::DeclareStructVariable(const std::string &parentName, const ShaderVarType &var, bool withinCB, uint32_t cumulativeOffset)
{
	// CB arrays need to be defined as 4 component vectors to match DX11 data layout
	bool arrayWithinCB = (withinCB && (var.Elements > 1) && (psContext->psShader.eShaderType == COMPUTE_SHADER));
	bool doDeclare = true;

	if (var.Class == SVC_STRUCT)
	{
		std::ostringstream oss;
		if (m_StructDefinitions.find(var.name + "_Type") == m_StructDefinitions.end())
			DeclareStructType(var.name + "_Type", var.Members, withinCB, cumulativeOffset + var.Offset);
		oss << var.name << "_Type " << var.name;
		if (var.Elements > 1)
		{
			oss << "[" << var.Elements << "]";
		}
		m_StructDefinitions[parentName].m_Members.push_back(oss.str());
		m_StructDefinitions[parentName].m_Dependencies.push_back(var.name + "_Type");
		return;
	}

	else if (var.Class == SVC_MATRIX_COLUMNS || var.Class == SVC_MATRIX_ROWS)
	{
		std::ostringstream oss;
		if (psContext->flags & HLSLCC_FLAG_TRANSLATE_MATRICES)
		{
			// Translate matrices into vec4 arrays
			char prefix[256];
			sprintf(prefix, HLSLCC_TRANSLATE_MATRIX_FORMAT_STRING, var.Rows, var.Columns);
			oss << HLSLcc::GetConstructorForType(psContext, var.Type, 4) << " " << prefix << var.name;
			
			uint32_t elemCount = (var.Class == SVC_MATRIX_COLUMNS ? var.Columns : var.Rows);
			if (var.Elements > 1)
			{
				elemCount *= var.Elements;
			}
			oss << "[" << elemCount << "]";
			
			if(withinCB)
			{
				// On compute shaders we need to reflect the vec array as it is to support all possible matrix sizes correctly.
				// On non-compute we can fake that we still have a matrix, as CB upload code will fill the data correctly on 4x4 matrices.
				// That way we avoid the issues with mismatching types for builtins etc.
				if (psContext->psShader.eShaderType == COMPUTE_SHADER)
					doDeclare = psContext->m_Reflection.OnConstant(var.fullName, var.Offset + cumulativeOffset, var.Type, 4, 1, false, elemCount);
				else
					doDeclare = psContext->m_Reflection.OnConstant(var.fullName, var.Offset + cumulativeOffset, var.Type, var.Rows, var.Columns, true, var.Elements);
			}
		}
		else
		{
			oss << HLSLcc::GetMatrixTypeName(psContext, var.Type, var.Columns, var.Rows);
			oss << " " << var.name;
			if (var.Elements > 1)
			{
				oss << "[" << var.Elements << "]";
			}
			
			// TODO Verify whether the offset is from the beginning of the CB or from the beginning of the struct
			if(withinCB)
				doDeclare = psContext->m_Reflection.OnConstant(var.fullName, var.Offset + cumulativeOffset, var.Type, var.Rows, var.Columns, true, var.Elements);
		}

		if (doDeclare)
			m_StructDefinitions[parentName].m_Members.push_back(oss.str());
	}
	else
	if (var.Class == SVC_VECTOR && var.Columns > 1)
	{
		std::ostringstream oss;
		oss << HLSLcc::GetConstructorForTypeMetal(var.Type, arrayWithinCB ? 4 : var.Columns);
		oss << " " << var.name;
		if (var.Elements > 1)
		{
			oss << "[" << var.Elements << "]";
		}

		if (withinCB)
			doDeclare = psContext->m_Reflection.OnConstant(var.fullName, var.Offset + cumulativeOffset, var.Type, 1, var.Columns, false, var.Elements);

		if (doDeclare)
			m_StructDefinitions[parentName].m_Members.push_back(oss.str());
	}
	else
	if ((var.Class == SVC_SCALAR) ||
		(var.Class == SVC_VECTOR && var.Columns == 1))
	{
		if (var.Type == SVT_BOOL)
		{
			//Use int instead of bool.
			//Allows implicit conversions to integer and
			//bool consumes 4-bytes in HLSL and GLSL anyway.
			((ShaderVarType &)var).Type = SVT_INT;
		}

		std::ostringstream oss;
		oss << HLSLcc::GetConstructorForTypeMetal(var.Type, arrayWithinCB ? 4 : 1);
		oss << " " << var.name;
		if (var.Elements > 1)
		{
			oss << "[" << var.Elements << "]";
		}

		if (withinCB)
			doDeclare = psContext->m_Reflection.OnConstant(var.fullName, var.Offset + cumulativeOffset, var.Type, 1, 1, false, var.Elements);

		if (doDeclare)
			m_StructDefinitions[parentName].m_Members.push_back(oss.str());
	}
	else
	{
		ASSERT(0);
	}
}

void ToMetal::DeclareStructType(const std::string &name, const std::vector<ShaderVar> &contents, bool withinCB, uint32_t cumulativeOffset, bool stripUnused /* = false */)
{
	for (std::vector<ShaderVar>::const_iterator itr = contents.begin(); itr != contents.end(); itr++)
	{
		if(stripUnused && !itr->sType.m_IsUsed)
			continue;
		
		DeclareStructVariable(name, *itr, withinCB, cumulativeOffset);
	}
}

void ToMetal::DeclareStructType(const std::string &name, const std::vector<ShaderVarType> &contents, bool withinCB, uint32_t cumulativeOffset)
{
	for (std::vector<ShaderVarType>::const_iterator itr = contents.begin(); itr != contents.end(); itr++)
	{
		DeclareStructVariable(name, *itr, withinCB, cumulativeOffset);
	}
}

void ToMetal::DeclareConstantBuffer(const ConstantBuffer *psCBuf, uint32_t ui32BindingPoint)
{
	std::string cbname = psCBuf->name.c_str();
	
	const bool isGlobals = (cbname == "$Globals");
	const bool stripUnused = isGlobals && (psContext->flags & HLSLCC_FLAG_REMOVE_UNUSED_GLOBALS);
	
	if (cbname[0] == '$')
		cbname = cbname.substr(1);
	
	// Note: if we're stripping unused members, both ui32TotalSizeInBytes and individual offsets into reflection will be completely off.
	// However, the reflection layer re-calculates both to match Metal alignment rules anyway, so we're good.
	if (!psContext->m_Reflection.OnConstantBuffer(cbname, psCBuf->ui32TotalSizeInBytes, psCBuf->GetMemberCount(stripUnused)))
		return;

	DeclareStructType(cbname + "_Type", psCBuf->asVars, true, 0, stripUnused);
	std::ostringstream oss;
  uint32_t slot = drv3d_metal::RemapBufferSlot(drv3d_metal::CONST_BUFFER, ui32BindingPoint);// m_BufferSlots.GetBindingSlot(ui32BindingPoint, BindingSlotAllocator::ConstantBuffer);

  if (strcmp(cbname.c_str(), "Globals") == 0)
  {
    slot = drv3d_metal::BIND_POINT;
  }

	oss << "constant " << cbname << "_Type& " << cbname << " [[ buffer("<< slot <<") ]]";
	m_StructDefinitions[""].m_Members.push_back(oss.str());
	m_StructDefinitions[""].m_Dependencies.push_back(cbname + "_Type");

	psContext->m_Reflection.OnConstantBufferBinding(cbname, slot);


}

void ToMetal::DeclareBufferVariable(const Declaration *psDecl, const bool isRaw, const bool isUAV)
{
	uint32_t ui32BindingPoint = psDecl->asOperands[0].ui32RegisterNumber;
	std::string BufName, BufType;

	BufName = "";
	BufType = "";

	// Use original HLSL bindings for UAVs only. For non-UAV buffers we have resolved new binding points from the same register space.
	//if (!isUAV)
		//ui32BindingPoint = psContext->psShader.aui32StructuredBufferBindingPoints[psContext->psShader.ui32CurrentStructuredBufferIndex++];

	BufName = ResourceName(isUAV ? RGROUP_UAV : RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber);

	if (!isRaw) // declare struct containing uint array when needed
	{
		std::ostringstream typeoss;
		BufType = BufName + "_Type";

    if (strcmp(BufName.c_str(), "g_gauss_input") == 0 ||
        strcmp(BufName.c_str(), "g_omega_input") == 0)
    {
        typeoss << "float value[";
    }
    else
    {
        typeoss << "uint value[";
    }

		typeoss << psDecl->ui32BufferStride / 4 << "]";
		m_StructDefinitions[BufType].m_Members.push_back(typeoss.str());
		m_StructDefinitions[""].m_Dependencies.push_back(BufType);
	}

	std::ostringstream oss;
	
	if (!isUAV || ((psDecl->sUAV.ui32AccessFlags & ACCESS_FLAG_WRITE) == 0))
	{
		oss << "const ";
	}
	else
	{	
		if (psContext->psShader.eShaderType != COMPUTE_SHADER)
			psContext->m_Reflection.OnDiagnostics("This shader might not work on all Metal devices because of buffer writes on non-compute shaders.", 0, false);
	}

	if (isRaw)
		oss << "device uint *" << BufName;
	else
		oss << "device " << BufType << " *" << BufName;

  uint32_t loc = drv3d_metal::RemapBufferSlot(drv3d_metal::STRUCT_BUFFER, ui32BindingPoint); //m_BufferSlots.GetBindingSlot(ui32BindingPoint, BindingSlotAllocator::RWBuffer);

  if (isUAV)
  {
    loc = drv3d_metal::RemapBufferSlot(drv3d_metal::RW_BUFFER, ui32BindingPoint);
  }

	oss << " [[ buffer(" << loc << ") ]]";

	m_StructDefinitions[""].m_Members.push_back(oss.str());
	psContext->m_Reflection.OnBufferBinding(BufName, loc, isUAV);
}


void ToMetal::TranslateDeclaration(const Declaration* psDecl)
{
	bstring glsl = *psContext->currentGLSLString;
	Shader& psShader = psContext->psShader;

	switch (psDecl->eOpcode)
	{

	case OPCODE_DCL_INPUT_SGV:
	case OPCODE_DCL_INPUT_PS_SGV:
		DeclareBuiltinInput(psDecl);
		break;
	case OPCODE_DCL_OUTPUT_SIV:
		DeclareBuiltinOutput(psDecl);
		break;
	case OPCODE_DCL_INPUT:
	case OPCODE_DCL_INPUT_PS_SIV:
	case OPCODE_DCL_INPUT_SIV:
	case OPCODE_DCL_INPUT_PS:
	{
		const Operand* psOperand = &psDecl->asOperands[0];

		uint32_t ui32Reg = psDecl->asOperands[0].ui32RegisterNumber;
		uint32_t ui32CompMask = psDecl->asOperands[0].ui32CompMask;

		std::string name = psContext->GetDeclaredInputName(*psOperand, nullptr, 1, nullptr);

		//Already declared as part of an array?
		if (psShader.aIndexedInput[0][psDecl->asOperands[0].ui32RegisterNumber] == -1)
		{
			ASSERT(0); // Find out what's happening
			break;
		}
		// Already declared?
		if ((ui32CompMask != 0) && ((ui32CompMask & ~psShader.acInputDeclared[0][ui32Reg]) == 0))
		{
			ASSERT(0); // Catch this
			break;
		}

		if (psOperand->eType == OPERAND_TYPE_INPUT_COVERAGE_MASK)
		{
			std::ostringstream oss;
			oss << "uint " << name << " [[ sample_mask ]]";
			m_StructDefinitions[""].m_Members.push_back(oss.str());
			break;
		}

		if (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID)
		{
			std::ostringstream oss;
			oss << "uint3 " << name << " [[ thread_position_in_grid ]]";
			m_StructDefinitions[""].m_Members.push_back(oss.str());
			break;
		}

		if (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_GROUP_ID)
		{
			std::ostringstream oss;
			oss << "uint3 " << name << " [[ threadgroup_position_in_grid ]]";
			m_StructDefinitions[""].m_Members.push_back(oss.str());
			break;
		}

		if (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP)
		{
			std::ostringstream oss;
			oss << "uint3 " << name << " [[ thread_position_in_threadgroup ]]";
			m_StructDefinitions[""].m_Members.push_back(oss.str());
			break;
		}
		if (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED)
		{
			std::ostringstream oss;
			oss << "uint " << name << " [[ thread_index_in_threadgroup ]]";
			m_StructDefinitions[""].m_Members.push_back(oss.str());
			break;
		}
		
		if(psDecl->eOpcode == OPCODE_DCL_INPUT_PS_SIV && psOperand->eSpecialName == NAME_POSITION)
		{
			m_StructDefinitions[""].m_Members.push_back("float4 mtl_FragCoord [[ position ]]");
			break;
		}
		if (psShader.eShaderType == PIXEL_SHADER)
		{
			psContext->psDependencies.SetInterpolationMode(ui32Reg, psDecl->value.eInterpolation);
		}

        const ShaderInfo::InOutSignature *psSig = NULL;
        psContext->psShader.sInfo.GetInputSignatureFromRegister(ui32Reg, ui32CompMask, &psSig);
        
        int iNumComponents = psOperand->GetNumInputElements(psContext);
		psShader.acInputDeclared[0][ui32Reg] = (char)psSig->ui32Mask;

		std::string typeName = BuildOperandTypeString(psOperand->eMinPrecision, psSig->eComponentType, iNumComponents);

		std::string semantic;
		if (psContext->psShader.eShaderType == VERTEX_SHADER)
		{
			std::ostringstream oss;
			uint32_t loc = psContext->psDependencies.GetVaryingLocation(name, VERTEX_SHADER, true);
			oss << "attribute(" << loc << ")";
			semantic = oss.str();
			psContext->m_Reflection.OnInputBindingEx(name, loc, iNumComponents);
		}
		else
		{
			std::ostringstream oss;

			// UNITY_FRAMEBUFFER_FETCH_AVAILABLE
			// special case mapping for inout color, see HLSLSupport.cginc
			if (psOperand->iPSInOut && name.size() == 10 && !strncmp(name.c_str(), "SV_Target", 9))
			{
				// Metal allows color(X) declared in input/output structs
				//
				// TODO: Improve later when GLES3 support arrives, it requires
				// single declaration through inout
				oss << "color(" << psSig->ui32SemanticIndex << ")";
			}
			else
			{
				oss << "user(" << name << ")";
			}
			semantic = oss.str();
		}

		std::string interpolation = "";
		if (psDecl->eOpcode == OPCODE_DCL_INPUT_PS)
		{
			interpolation = GetInterpolationString(psDecl->value.eInterpolation);
		}

		std::string declString;
		if ((OPERAND_INDEX_DIMENSION)psOperand->iIndexDims == INDEX_2D)
		{
			std::ostringstream oss;
			oss << typeName << " " << name << " [ " << psOperand->aui32ArraySizes[0] << " ] " << " [[ " << semantic << " ]] " << interpolation;
			declString = oss.str();
		}
		else
		{
			std::ostringstream oss;
			oss << typeName << " " << name << " [[ " << semantic << " ]] " << interpolation;
			declString = oss.str();
		}

		m_StructDefinitions[GetInputStructName()].m_Members.push_back(declString);

		HandleInputRedirect(psDecl, BuildOperandTypeString(psOperand->eMinPrecision, INOUT_COMPONENT_FLOAT32, 4));
		break;
	}
	case OPCODE_DCL_TEMPS:
	{
		uint32_t i = 0;
		const uint32_t ui32NumTemps = psDecl->value.ui32NumTemps;
		glsl = psContext->psShader.asPhases[psContext->currentPhase].earlyMain;
		for (i = 0; i < ui32NumTemps; i++)
		{
			if (psShader.psFloatTempSizes[i] != 0)
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_FLOAT, psShader.psFloatTempSizes[i]), i);
			if (psShader.psFloat16TempSizes[i] != 0)
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "16_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_FLOAT16, psShader.psFloat16TempSizes[i]), i);
			if (psShader.psFloat10TempSizes[i] != 0)
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "10_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_FLOAT10, psShader.psFloat10TempSizes[i]), i);
			if (psShader.psIntTempSizes[i] != 0)
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "i%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_INT, psShader.psIntTempSizes[i]), i);
			if (psShader.psInt16TempSizes[i] != 0)
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "i16_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_INT16, psShader.psInt16TempSizes[i]), i);
			if (psShader.psInt12TempSizes[i] != 0)
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "i12_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_INT12, psShader.psInt12TempSizes[i]), i);
			if (psShader.psUIntTempSizes[i] != 0)
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "u%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_UINT, psShader.psUIntTempSizes[i]), i);
			if (psShader.psUInt16TempSizes[i] != 0)
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "u16_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_UINT16, psShader.psUInt16TempSizes[i]), i);
			if (psShader.fp64 && (psShader.psDoubleTempSizes[i] != 0))
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "d%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_DOUBLE, psShader.psDoubleTempSizes[i]), i);
			if (psShader.psBoolTempSizes[i] != 0)
				bformata(glsl, "    %s " HLSLCC_TEMP_PREFIX "b%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_BOOL, psShader.psBoolTempSizes[i]), i);
		}
		break;
	}
	case OPCODE_SPECIAL_DCL_IMMCONST:
	{
		ASSERT(0 && "DX9 shaders no longer supported!");
		break;
	}
	case OPCODE_DCL_CONSTANT_BUFFER:
	{
		const ConstantBuffer* psCBuf = NULL;
		psContext->psShader.sInfo.GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, psDecl->asOperands[0].aui32ArraySizes[0], &psCBuf);
		ASSERT(psCBuf != NULL);
		DeclareConstantBuffer(psCBuf, psDecl->asOperands[0].aui32ArraySizes[0]);
		break;
	}
	case OPCODE_DCL_RESOURCE:
	{
		DeclareResource(psDecl);
		break;
	}
	case OPCODE_DCL_OUTPUT:
	{
		DeclareOutput(psDecl);
		break;
	}

	case OPCODE_DCL_GLOBAL_FLAGS:
	{
		uint32_t ui32Flags = psDecl->value.ui32GlobalFlags;

		if (ui32Flags & GLOBAL_FLAG_FORCE_EARLY_DEPTH_STENCIL)
		{
//			bcatcstr(glsl, "layout(early_fragment_tests) in;\n");
		}
		if (!(ui32Flags & GLOBAL_FLAG_REFACTORING_ALLOWED))
		{
			//TODO add precise
			//HLSL precise - http://msdn.microsoft.com/en-us/library/windows/desktop/hh447204(v=vs.85).aspx
		}
		if (ui32Flags & GLOBAL_FLAG_ENABLE_DOUBLE_PRECISION_FLOAT_OPS)
		{
			// Not supported on Metal
//			bcatcstr(glsl, "#extension GL_ARB_gpu_shader_fp64 : enable\n");
//			psShader.fp64 = 1;
		}
		break;
	}
	case OPCODE_DCL_THREAD_GROUP:
	{
		// Send this info to reflecion: Metal gives this at runtime as a param
		psContext->m_Reflection.OnThreadGroupSize(psDecl->value.aui32WorkGroupSize[0],
												  psDecl->value.aui32WorkGroupSize[1],
												  psDecl->value.aui32WorkGroupSize[2]);
		break;
	}
	case OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_TESS_DOMAIN:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_TESS_PARTITIONING:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_GS_INPUT_PRIMITIVE:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_INTERFACE:
	{
		// Are interfaces ever even used?
		ASSERT(0);
		break;
	}
	case OPCODE_DCL_FUNCTION_BODY:
	{
		ASSERT(0);
		break;
	}
	case OPCODE_DCL_FUNCTION_TABLE:
	{
		ASSERT(0);
		break;
	}
	case OPCODE_CUSTOMDATA:
	{
		// TODO: This is only ever accessed as a float currently. Do trickery if we ever see ints accessed from an array.
		// Walk through all the chunks we've seen in this phase.
		ShaderPhase &sp = psShader.asPhases[psContext->currentPhase];
		std::for_each(sp.m_ConstantArrayInfo.m_Chunks.begin(), sp.m_ConstantArrayInfo.m_Chunks.end(), [this](const std::pair<uint32_t, ConstantArrayChunk> &chunk)
		{
			bstring glsl = *psContext->currentGLSLString;
			uint32_t componentCount = chunk.second.m_ComponentCount;
			// Just do the declaration here and contents to earlyMain.
			if (componentCount == 1)
				bformata(glsl, "constant float ImmCB_%d_%d_%d[%d] =\n{\n", psContext->currentPhase, chunk.first, chunk.second.m_Rebase, chunk.second.m_Size);
			else
				bformata(glsl, "constant float%d ImmCB_%d_%d_%d[%d] =\n{\n", componentCount, psContext->currentPhase, chunk.first, chunk.second.m_Rebase, chunk.second.m_Size);

			Declaration *psDecl = psContext->psShader.asPhases[psContext->currentPhase].m_ConstantArrayInfo.m_OrigDeclaration;
			if (componentCount == 1)
			{
				for (uint32_t i = 0; i < chunk.second.m_Size; i++)
				{
					if (i != 0)
						bcatcstr(glsl, ",\n");
					float val[4] = {
						*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].a,
						*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].b,
						*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].c,
						*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].d
					};
					if (fpcheck(val[chunk.second.m_Rebase]))
						bformata(glsl, "\tfloat(%Xu)", *(uint32_t *)&val[chunk.second.m_Rebase]);
					else
					{
						bcatcstr(glsl, "\t");
						HLSLcc::PrintFloat(glsl, val[chunk.second.m_Rebase]);
					}
				}
				bcatcstr(glsl, "\n};\n");
			}
			else
			{
				for (uint32_t i = 0; i < chunk.second.m_Size; i++)
		{
					if (i != 0)
						bcatcstr(glsl, ",\n");
			float val[4] = {
						*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].a,
						*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].b,
						*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].c,
						*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].d
			};
					bformata(glsl, "\tfloat%d(", componentCount);
					for (uint32_t k = 0; k < componentCount; k++)
			{
						if (k != 0)
							bcatcstr(glsl, ", ");
				if (fpcheck(val[k]))
							bformata(glsl, "float(%Xu)", *(uint32_t *)&val[k + chunk.second.m_Rebase]);
				else
							HLSLcc::PrintFloat(glsl, val[k + chunk.second.m_Rebase]);
			}
					bcatcstr(glsl, ")");
			}
				bcatcstr(glsl, "\n};\n");
		}

		});

		break;
	}
	case OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
	case OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT:
		break; // Nothing to do

	case OPCODE_DCL_INDEXABLE_TEMP:
	{
		const uint32_t ui32RegIndex = psDecl->sIdxTemp.ui32RegIndex;
		const uint32_t ui32RegCount = psDecl->sIdxTemp.ui32RegCount;
		const uint32_t ui32RegComponentSize = psDecl->sIdxTemp.ui32RegComponentSize;
		bformata(psContext->psShader.asPhases[psContext->currentPhase].earlyMain, "float%d TempArray%d[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
		break;
	}
	case OPCODE_DCL_INDEX_RANGE:
	{
		switch (psDecl->asOperands[0].eType)
		{
		case OPERAND_TYPE_OUTPUT:
		case OPERAND_TYPE_INPUT:
		{
			const ShaderInfo::InOutSignature* psSignature = NULL;
			const char* type = "float";
			uint32_t startReg = 0;
			uint32_t i;
			bstring *oldString;
      RegisterSpace regSpace = psDecl->asOperands[0].GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);
			int isInput = psDecl->asOperands[0].eType == OPERAND_TYPE_INPUT ? 1 : 0;

			if (regSpace == RegisterSpace::per_vertex)
			{
				if (isInput)
					psShader.sInfo.GetInputSignatureFromRegister(
						psDecl->asOperands[0].ui32RegisterNumber,
						psDecl->asOperands[0].ui32CompMask,
						&psSignature);
				else
					psShader.sInfo.GetOutputSignatureFromRegister(
						psDecl->asOperands[0].ui32RegisterNumber,
						psDecl->asOperands[0].ui32CompMask,
						psShader.ui32CurrentVertexOutputStream,
						&psSignature);
			}
			else
				psShader.sInfo.GetPatchConstantSignatureFromRegister(psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32CompMask, &psSignature);

			ASSERT(psSignature != NULL);

			switch (psSignature->eComponentType)
			{
			case INOUT_COMPONENT_UINT32:
			{
				type = "uint";
				break;
			}
			case INOUT_COMPONENT_SINT32:
			{
				type = "int";
				break;
			}
			case INOUT_COMPONENT_FLOAT32:
			{
				break;
			}
			default:
				ASSERT(0);
				break;
			}

			switch (psSignature->eMinPrec) // TODO What if the inputs in the indexed range are of different precisions?
			{
				default:
					break;
				case MIN_PRECISION_ANY_16:
					ASSERT(0); // Wut?
					break;
				case MIN_PRECISION_FLOAT_16:
				case MIN_PRECISION_FLOAT_2_8:
					type = "half";
					break;
				case MIN_PRECISION_SINT_16:
					type = "short";
					break;
				case MIN_PRECISION_UINT_16:
					type = "ushort";
					break;
			}
	
			startReg = psDecl->asOperands[0].ui32RegisterNumber;
			bformata(psContext->psShader.asPhases[psContext->currentPhase].earlyMain, "%s4 phase%d_%sput%d_%d[%d];\n", type, psContext->currentPhase, isInput ? "In" : "Out", regSpace, startReg, psDecl->value.ui32IndexRange);
			oldString = psContext->currentGLSLString;
			glsl = isInput ? psContext->psShader.asPhases[psContext->currentPhase].earlyMain : psContext->psShader.asPhases[psContext->currentPhase].postShaderCode;
			psContext->currentGLSLString = &glsl;
			if (isInput == 0)
				psContext->psShader.asPhases[psContext->currentPhase].hasPostShaderCode = 1;
			for (i = 0; i < psDecl->value.ui32IndexRange; i++)
			{
				int dummy = 0;
				std::string realName;
				uint32_t destMask = psDecl->asOperands[0].ui32CompMask;
				uint32_t rebase = 0;
				const ShaderInfo::InOutSignature *psSig = NULL;
				bool regSpace = psDecl->asOperands[0].GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase) == RegisterSpace::per_vertex;

				if (regSpace)
					if (isInput)
						psContext->psShader.sInfo.GetInputSignatureFromRegister(startReg + i, destMask, &psSig);
					else
						psContext->psShader.sInfo.GetOutputSignatureFromRegister(startReg + i, destMask, 0, &psSig);
				else
					psContext->psShader.sInfo.GetPatchConstantSignatureFromRegister(startReg + i, destMask, &psSig);

				ASSERT(psSig != NULL);

				if ((psSig->ui32Mask & destMask) == 0)
					continue; // Skip dummy writes (vec2 texcoords get filled to vec4 with zeroes etc)

				while ((psSig->ui32Mask & (1 << rebase)) == 0)
					rebase++;

				((Declaration *)psDecl)->asOperands[0].ui32RegisterNumber = startReg + i;

				if (isInput)
				{
					realName = psContext->GetDeclaredInputName(psDecl->asOperands[0], &dummy, 1, NULL);

					psContext->AddIndentation();

					bformata(glsl, "phase%d_Input%d_%d[%d]", psContext->currentPhase, regSpace, startReg, i);

					if (destMask != OPERAND_4_COMPONENT_MASK_ALL)
					{
						int k;
						const char *swizzle = "xyzw";
						bcatcstr(glsl, ".");
						for (k = 0; k < 4; k++)
						{
							if ((destMask & (1 << k)) && (psSig->ui32Mask & (1 << k)))
							{
								bformata(glsl, "%c", swizzle[k]);
							}
						}
					}
					bcatcstr(glsl, " = ");
					bcatcstr(glsl, realName.c_str());
					if (destMask != OPERAND_4_COMPONENT_MASK_ALL && destMask != psSig->ui32Mask)
					{
						int k;
						const char *swizzle = "xyzw";
						bcatcstr(glsl, ".");
						for (k = 0; k < 4; k++)
						{
							if ((destMask & (1 << k)) && (psSig->ui32Mask & (1 << k)))
							{
								bformata(glsl, "%c", swizzle[k - rebase]);
							}
						}
					}
				}
				else
				{
					realName = psContext->GetDeclaredOutputName(psDecl->asOperands[0], &dummy, NULL, NULL, 1);

					psContext->AddIndentation();
					bcatcstr(glsl, realName.c_str());
					if (destMask != OPERAND_4_COMPONENT_MASK_ALL && destMask != psSig->ui32Mask)
					{
						int k;
						const char *swizzle = "xyzw";
						bcatcstr(glsl, ".");
						for (k = 0; k < 4; k++)
						{
							if ((destMask & (1 << k)) && (psSig->ui32Mask & (1 << k)))
							{
								bformata(glsl, "%c", swizzle[k - rebase]);
							}
						}
					}

					bformata(glsl, " = phase%d_Output%d_%d[%d]", psContext->currentPhase, regSpace, startReg, i);

					if (destMask != OPERAND_4_COMPONENT_MASK_ALL)
					{
						int k;
						const char *swizzle = "xyzw";
						bcatcstr(glsl, ".");
						for (k = 0; k < 4; k++)
						{
							if ((destMask & (1 << k)) && (psSig->ui32Mask & (1 << k)))
							{
								bformata(glsl, "%c", swizzle[k]);
							}
						}
					}
				}

				bcatcstr(glsl, ";\n");
			}

			((Declaration *)psDecl)->asOperands[0].ui32RegisterNumber = startReg;
			psContext->currentGLSLString = oldString;
			glsl = *psContext->currentGLSLString;

			for (i = 0; i < psDecl->value.ui32IndexRange; i++)
			{
				if (regSpace == RegisterSpace::per_vertex)
				{
					if (isInput)
						psShader.sInfo.GetInputSignatureFromRegister(
							psDecl->asOperands[0].ui32RegisterNumber + i,
							psDecl->asOperands[0].ui32CompMask,
							&psSignature);
					else
						psShader.sInfo.GetOutputSignatureFromRegister(
							psDecl->asOperands[0].ui32RegisterNumber + i,
							psDecl->asOperands[0].ui32CompMask,
							psShader.ui32CurrentVertexOutputStream,
							&psSignature);
				}
				else
					psShader.sInfo.GetPatchConstantSignatureFromRegister(psDecl->asOperands[0].ui32RegisterNumber + i, psDecl->asOperands[0].ui32CompMask, &psSignature);

				ASSERT(psSignature != NULL);

				((ShaderInfo::InOutSignature *)psSignature)->isIndexed.insert(psContext->currentPhase);
				((ShaderInfo::InOutSignature *)psSignature)->indexStart[psContext->currentPhase] = startReg;
				((ShaderInfo::InOutSignature *)psSignature)->index[psContext->currentPhase] = i;
			}


			break;
		}
		default:
			// TODO Input index ranges.
			ASSERT(0);
		}
		break;
	}

	case OPCODE_HS_DECLS:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_INPUT_CONTROL_POINT_COUNT:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT:
	{
		// Not supported
		break;
	}
	case OPCODE_HS_FORK_PHASE:
	{
		// Not supported
		break;
	}
	case OPCODE_HS_JOIN_PHASE:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_SAMPLER:
	{
		// Find out if the sampler is good for a builtin
		std::string name = TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NAME_ONLY);
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);
		bool linear = (name.find("linear") != std::string::npos);
		bool point = (name.find("point") != std::string::npos);
		bool clamp = (name.find("clamp") != std::string::npos);
		bool repeat = (name.find("repeat") != std::string::npos);
		
		// Declare only builtin samplers here. Default samplers are declared together with the texture.
		if ((linear != point) && (clamp != repeat))
		{
			std::ostringstream oss;
			oss << "constexpr sampler " << TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NAME_ONLY) << "(";
			oss << (linear ? "filter::linear, " : "filter::nearest, ");
			oss << (clamp ? "address::clamp_to_edge" : "address::repeat");
			oss << ")";
			
			bformata(psContext->psShader.asPhases[psContext->currentPhase].earlyMain, "\t%s;\n", oss.str().c_str());
		}
		break;
	}
	case OPCODE_DCL_HS_MAX_TESSFACTOR:
	{
		// Not supported
		break;
	}
	case OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
	{
		std::string samplerTypeName = TranslateResourceDeclaration(psContext,
			psDecl, false, true);
		std::string texName = ResourceName(RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber);
    uint32_t slot = psDecl->asOperands[0].ui32RegisterNumber + 16;// m_TextureSlots.GetBindingSlot(psDecl->asOperands[0].ui32RegisterNumber, BindingSlotAllocator::UAV);
    std::ostringstream oss;
		oss << samplerTypeName << " " << texName
			<< " [[ texture (" << slot << ") ]] ";

		m_StructDefinitions[""].m_Members.push_back(oss.str());
		psContext->m_Reflection.OnTextureBinding(texName, slot, TD_2D, true); // TODO: translate psDecl->value.eResourceDimension into HLSLCC_TEX_DIMENSION

		break;
	}

	case OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
	{
		if (psDecl->sUAV.bCounter)
		{
			std::ostringstream oss;
			std::string bufName = ResourceName(RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber);
			
			// Some GPUs don't allow memory access below buffer binding offset in the shader so always bind compute buffer
			// at offset 0 instead of GetDataOffset() to access counter value and translate the buffer pointer in the shader.
			oss << "device atomic_uint *" << bufName << "_counter = reinterpret_cast<device atomic_uint *> (" << bufName << ");";
			oss << "\n    " << bufName << " = reinterpret_cast<device " << bufName << "_Type *> (reinterpret_cast<device atomic_uint *> (" << bufName << ") + 1)";
			bformata(psContext->psShader.asPhases[psContext->currentPhase].earlyMain, "    %s;\n", oss.str().c_str());
		}

		DeclareBufferVariable(psDecl, 0, 1);
		break;
	}
	case OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
	{
		if (psDecl->sUAV.bCounter)
		{
			std::ostringstream oss;
			std::string bufName = ResourceName(RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber);
			oss << "device atomic_uint *" << bufName << "_counter = reinterpret_cast<atomic_uint *> (" << bufName << ") - 1";
			bformata(psContext->psShader.asPhases[psContext->currentPhase].earlyMain, "\t%s;\n", oss.str().c_str());		}

		DeclareBufferVariable(psDecl, 1, 1);

		break;
	}
	case OPCODE_DCL_RESOURCE_STRUCTURED:
	{
		DeclareBufferVariable(psDecl, 0, 0);
		break;
	}
	case OPCODE_DCL_RESOURCE_RAW:
	{
		DeclareBufferVariable(psDecl, 1, 0);
		break;
	}
	case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
	{
		ShaderVarType* psVarType = &psShader.sInfo.sGroupSharedVarType[psDecl->asOperands[0].ui32RegisterNumber];
		std::ostringstream oss;
		oss << "uint value[" << psDecl->sTGSM.ui32Stride / 4 << "]";
		m_StructDefinitions[TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NAME_ONLY) + "_Type"].m_Members.push_back(oss.str());
		m_StructDefinitions[""].m_Dependencies.push_back(TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NAME_ONLY) + "_Type");
		oss.str("");
		oss << "threadgroup " << TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NAME_ONLY)
			<< "_Type " << TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NAME_ONLY)
			<< "[" << psDecl->sTGSM.ui32Count << "]";

		bformata(psContext->psShader.asPhases[psContext->currentPhase].earlyMain, "\t%s;\n", oss.str().c_str());

		psVarType->name = "$Element";

		psVarType->Columns = psDecl->sTGSM.ui32Stride / 4;
		psVarType->Elements = psDecl->sTGSM.ui32Count;
		break;
	}
	case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
	{
		ShaderVarType* psVarType = &psShader.sInfo.sGroupSharedVarType[psDecl->asOperands[0].ui32RegisterNumber];

		std::ostringstream oss;
		oss << "threadgroup uint " << TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NAME_ONLY)
			<< "[" << (psDecl->sTGSM.ui32Count / psDecl->sTGSM.ui32Stride) << "]";

		bformata(psContext->psShader.asPhases[psContext->currentPhase].earlyMain, "\t%s;\n", oss.str().c_str());

		psVarType->name = "$Element";

		psVarType->Columns = 1;
		psVarType->Elements = psDecl->sTGSM.ui32Count / psDecl->sTGSM.ui32Stride;
		break;
	}

	case OPCODE_DCL_STREAM:
	{
		// Not supported on Metal
		break;
	}
	case OPCODE_DCL_GS_INSTANCE_COUNT:
	{
		// Not supported on Metal
		break;
	}

	default:
		ASSERT(0);
		break;
	}
}

std::string ToMetal::ResourceName(ResourceGroup group, const uint32_t ui32RegisterNumber)
{
	const ResourceBinding* psBinding = 0;
	std::ostringstream oss;
	int found;

	found = psContext->psShader.sInfo.GetResourceFromBindingPoint(group, ui32RegisterNumber, &psBinding);

	if (found)
	{
		size_t i = 0;
		std::string name = psBinding->name;
		uint32_t ui32ArrayOffset = ui32RegisterNumber - psBinding->ui32BindPoint;

		while (i < name.length())
		{
			//array syntax [X] becomes _0_
			//Otherwise declarations could end up as:
			//uniform sampler2D SomeTextures[0];
			//uniform sampler2D SomeTextures[1];
			if (name[i] == '[' || name[i] == ']')
				name[i] = '_';

			++i;
		}

		if (ui32ArrayOffset)
		{
			oss << name << ui32ArrayOffset;
			return oss.str();
		}
		else
		{
			return name;
		}
	}
	else
	{
		oss << "UnknownResource" << ui32RegisterNumber;
		return oss.str();
	}
}

void ToMetal::TranslateResourceTexture(const Declaration* psDecl, uint32_t samplerCanDoShadowCmp, HLSLCC_TEX_DIMENSION texDim)
{

	std::string samplerTypeName = TranslateResourceDeclaration(psContext,
		psDecl, (samplerCanDoShadowCmp && psDecl->ui32IsShadowTex), false);

  uint32_t slot = psDecl->asOperands[0].ui32RegisterNumber;// m_TextureSlots.GetBindingSlot(psDecl->asOperands[0].ui32RegisterNumber, BindingSlotAllocator::Texture);
	std::string texName = ResourceName(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber);
	std::ostringstream oss;
	oss << samplerTypeName << " " << texName
		<< " [[ texture (" << slot << ") ]] ";

	m_StructDefinitions[""].m_Members.push_back(oss.str());
	psContext->m_Reflection.OnTextureBinding(texName, slot, texDim, false);
	oss.str("");
	// the default sampler for a texture is named after the texture with a "sampler" prefix
  oss << "sampler " << texName;

 /*if (psDecl->value.ui32GlobalFlags & TEXSMP_FLAG_DEPTHCOMPARE)
 {
     oss << "_asddasgfasgasg [[ sampler (" << slot << ") ]] ";
 }
 else*/
 {
     oss << "_samplerstate [[ sampler (" << slot << ") ]] ";
 }
	m_StructDefinitions[""].m_Members.push_back(oss.str());

	if (samplerCanDoShadowCmp && psDecl->ui32IsShadowTex)
		EnsureShadowSamplerDeclared();

}

void ToMetal::DeclareResource(const Declaration *psDecl)
{
	switch (psDecl->value.eResourceDimension)
	{
		case RESOURCE_DIMENSION_BUFFER:
		{
      uint32_t slot = psDecl->asOperands[0].ui32RegisterNumber;// m_TextureSlots.GetBindingSlot(psDecl->asOperands[0].ui32RegisterNumber, BindingSlotAllocator::Texture);
			std::string texName = TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NAME_ONLY);
			std::ostringstream oss;
			oss << "device " << TranslateResourceDeclaration(psContext,
				psDecl, false, false);

			oss << texName << " [[ texture(" << slot << ") ]]";

			m_StructDefinitions[""].m_Members.push_back(oss.str());
			psContext->m_Reflection.OnTextureBinding(texName, slot, TD_2D, false); //TODO: correct HLSLCC_TEX_DIMENSION?
			break;

		}
		default:
			ASSERT(0);
			break;

		case RESOURCE_DIMENSION_TEXTURE1D:
		{
			TranslateResourceTexture(psDecl, 1, TD_2D); //TODO: correct HLSLCC_TEX_DIMENSION?
			break;
		}
		case RESOURCE_DIMENSION_TEXTURE2D:
		{
			TranslateResourceTexture(psDecl, 1, TD_2D);
			break;
		}
		case RESOURCE_DIMENSION_TEXTURE2DMS:
		{
			TranslateResourceTexture(psDecl, 0, TD_2D);
			break;
		}
		case RESOURCE_DIMENSION_TEXTURE3D:
		{
			TranslateResourceTexture(psDecl, 0, TD_3D);
			break;
		}
		case RESOURCE_DIMENSION_TEXTURECUBE:
		{
			TranslateResourceTexture(psDecl, 1, TD_CUBE);
			break;
		}
		case RESOURCE_DIMENSION_TEXTURE1DARRAY:
		{
			TranslateResourceTexture(psDecl, 1, TD_2DARRAY); //TODO: correct HLSLCC_TEX_DIMENSION?
			break;
		}
		case RESOURCE_DIMENSION_TEXTURE2DARRAY:
		{
			TranslateResourceTexture(psDecl, 1, TD_2DARRAY);
			break;
		}
		case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
		{
			TranslateResourceTexture(psDecl, 0, TD_2DARRAY);
			break;
		}
		case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
		{
			TranslateResourceTexture(psDecl, 1, TD_CUBEARRAY);
			break;
		}
	}
	psContext->psShader.aeResourceDims[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->value.eResourceDimension;


}

void ToMetal::DeclareOutput(const Declaration *psDecl)
{
	bstring glsl = *psContext->currentGLSLString;
	Shader& psShader = psContext->psShader;

	if (!psContext->OutputNeedsDeclaring(&psDecl->asOperands[0], 1))
		return;

	const Operand* psOperand = &psDecl->asOperands[0];
	int iNumComponents;
  RegisterSpace regSpace = psDecl->asOperands[0].GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);
	uint32_t ui32Reg = psDecl->asOperands[0].ui32RegisterNumber;

	const ShaderInfo::InOutSignature* psSignature = NULL;
	SHADER_VARIABLE_TYPE cType = SVT_VOID;

	if (psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH ||
		psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL ||
		psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL)
	{
		iNumComponents = 1;
		cType = SVT_FLOAT;
	}
	else
	{
		if (regSpace == RegisterSpace::per_vertex)
			psShader.sInfo.GetOutputSignatureFromRegister(
				ui32Reg,
				psDecl->asOperands[0].ui32CompMask,
				psShader.ui32CurrentVertexOutputStream,
				&psSignature);
		else
			psShader.sInfo.GetPatchConstantSignatureFromRegister(ui32Reg, psDecl->asOperands[0].ui32CompMask, &psSignature);

		iNumComponents = HLSLcc::GetNumberBitsSet(psSignature->ui32Mask);

		switch (psSignature->eComponentType)
		{
		case INOUT_COMPONENT_UINT32:
		{
			cType = SVT_UINT;
			break;
		}
		case INOUT_COMPONENT_SINT32:
		{
			cType = SVT_INT;
			break;
		}
		case INOUT_COMPONENT_FLOAT32:
		{
			cType = SVT_FLOAT;
			break;
		}
		default:
			ASSERT(0);
			break;
		}
		// Don't set this for oDepth (or variants), because depth output register is in separate space from other outputs (regno 0, but others may overlap with that)
		if (iNumComponents == 1)
			psContext->psShader.abScalarOutput[(int)regSpace][ui32Reg] |= (int)psDecl->asOperands[0].ui32CompMask;

		switch (psOperand->eMinPrecision)
		{
		case OPERAND_MIN_PRECISION_DEFAULT:
			break;
		case OPERAND_MIN_PRECISION_FLOAT_16:
			cType = SVT_FLOAT16;
			break;
		case OPERAND_MIN_PRECISION_FLOAT_2_8:
			cType = SVT_FLOAT10;
			break;
		case OPERAND_MIN_PRECISION_SINT_16:
			cType = SVT_INT16;
			break;
		case OPERAND_MIN_PRECISION_UINT_16:
			cType = SVT_UINT16;
			break;
		}
	}

	std::string type = HLSLcc::GetConstructorForTypeMetal(cType, iNumComponents);
	std::string name = psContext->GetDeclaredOutputName(psDecl->asOperands[0], nullptr, nullptr, nullptr, 1);

	switch (psShader.eShaderType)
	{
	case PIXEL_SHADER:
	{
		switch (psDecl->asOperands[0].eType)
		{
		case OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
		{
			std::ostringstream oss;
			oss << type << " " << name << " [[ sample_mask ]]";
			m_StructDefinitions[GetOutputStructName()].m_Members.push_back(oss.str());
			break;
		}
		case OPERAND_TYPE_OUTPUT_DEPTH:
		{
			std::ostringstream oss;
			oss << type << " " << name << " [[ depth(any) ]]";
			m_StructDefinitions[GetOutputStructName()].m_Members.push_back(oss.str());
			break;
		}
		case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
		{
			std::ostringstream oss;
			oss << type << " " << name << " [[ depth(any) ]]";
			m_StructDefinitions[GetOutputStructName()].m_Members.push_back(oss.str());
			break;
		}
		case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
		{
			std::ostringstream oss;
			oss << type << " " << name << " [[ depth(any) ]]";
			m_StructDefinitions[GetOutputStructName()].m_Members.push_back(oss.str());
			break;
		}
		default:
		{
			std::ostringstream oss;
			oss << type << " " << name << " [[ color(" << psSignature->ui32SemanticIndex << ") ]]";
			m_StructDefinitions[GetOutputStructName()].m_Members.push_back(oss.str());
		}
		}
		break;
	}
	case VERTEX_SHADER:
	{
		std::ostringstream oss;
		oss << type << " " << name;
		if (psSignature->eSystemValueType == NAME_POSITION || (psSignature->semanticName == "POS" && psOperand->ui32RegisterNumber == 0 ))
			oss << " [[ position ]]";
		else if (psSignature->eSystemValueType == NAME_UNDEFINED && psSignature->semanticName == "PSIZE" && psSignature->ui32SemanticIndex == 0 )
			oss << " [[ point_size ]]";
		else
			oss << " [[ user(" << name << ") ]]";
		m_StructDefinitions[GetOutputStructName()].m_Members.push_back(oss.str());
		break;
	}
	case GEOMETRY_SHADER:
	case DOMAIN_SHADER:
	case HULL_SHADER:
	default:
		ASSERT(0);
		break;

	}
	HandleOutputRedirect(psDecl, HLSLcc::GetConstructorForTypeMetal(cType, 4));

	
}

void ToMetal::EnsureShadowSamplerDeclared()
{
	if (m_ShadowSamplerDeclared)
		return;

  m_ShadowSamplerDeclared = true;
}
