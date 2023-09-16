#include "hlslcc.h"
#include "internal_includes/Declaration.h"
#include "internal_includes/toGLSLOperand.h"
#include "internal_includes/toGLSL.h"
#include "internal_includes/languages.h"
#include "internal_includes/HLSLccToolkit.h"
#include "internal_includes/Shader.h"
#include "internal_includes/HLSLCrossCompilerContext.h"
#include "bstrlib.h"
#include "internal_includes/debug.h"
#include <math.h>
#include <float.h>
#include <sstream>
#include <algorithm>
#include "internal_includes/toGLSL.h"

using namespace HLSLcc;

#ifdef _MSC_VER
#define fpcheck(x) (_isnan(x) || !_finite(x))
#else
#include <cmath>
#define fpcheck(x) ((std::isnan(x)) || (std::isinf(x)))
#endif

static void DeclareConstBufferShaderVariable(const HLSLCrossCompilerContext *psContext, const char* Name, const struct ShaderVarType* psType, uint32_t baseOffset, int unsizedArray, bool addUniformPrefix, bool is_struct)
	//const SHADER_VARIABLE_CLASS eClass, const SHADER_VARIABLE_TYPE eType,
    //const char* pszName)
{
	bstring glsl = *psContext->currentGLSLString;

  if (psContext->flags & HLSLCC_FLAG_GLSL_EXPLICIT_UNIFORM_OFFSET && !is_struct)
  {
    bformata(glsl, "layout(offset = %u) ", baseOffset + psType->Offset);
  }

	if (psType->Class == SVC_STRUCT)
	{
		bformata(glsl, "\t%s%s_Type %s", addUniformPrefix ? "UNITY_UNIFORM " : "", Name, Name);
		if (psType->Elements > 1)
		{
			bformata(glsl, "[%d]", psType->Elements);
		}
	}
	else if(psType->Class == SVC_MATRIX_COLUMNS || psType->Class == SVC_MATRIX_ROWS)
	{
		if (psContext->flags & HLSLCC_FLAG_TRANSLATE_MATRICES)
		{
			// Translate matrices into vec4 arrays
			bformata(glsl, "\t%s%s " HLSLCC_TRANSLATE_MATRIX_FORMAT_STRING "%s", addUniformPrefix ? "UNITY_UNIFORM " : "", HLSLcc::GetConstructorForType(psContext, psType->Type, 4), psType->Rows, psType->Columns, Name);
			uint32_t elemCount = (psType->Class == SVC_MATRIX_COLUMNS ? psType->Columns : psType->Rows);
			if (psType->Elements > 1)
			{
				elemCount *= psType->Elements;
			}
			bformata(glsl, "[%d]", elemCount);
		}
		else
		{
			bformata(glsl, "\t%s%s %s", addUniformPrefix ? "UNITY_UNIFORM " : "", HLSLcc::GetMatrixTypeName(psContext, psType->Type, psType->Columns, psType->Rows).c_str(), Name);
			if (psType->Elements > 1)
			{
				bformata(glsl, "[%d]", psType->Elements);
			}
		}
	}
	else
	if (psType->Class == SVC_VECTOR && psType->Columns > 1)
	{
		bformata(glsl, "\t%s%s %s", addUniformPrefix ? "UNITY_UNIFORM " : "", HLSLcc::GetConstructorForType(psContext, psType->Type, psType->Columns), Name);

		if(psType->Elements > 1)
		{
			bformata(glsl, "[%d]", psType->Elements);
		}
	}
	else
	if ((psType->Class == SVC_SCALAR) ||
		(psType->Class == SVC_VECTOR && psType->Columns == 1))
	{
		if (psType->Type == SVT_BOOL)
			{
				//Use int instead of bool.
				//Allows implicit conversions to integer and
				//bool consumes 4-bytes in HLSL and GLSL anyway.
				((ShaderVarType *)psType)->Type = SVT_INT;
		}

		bformata(glsl, "\t%s%s %s", addUniformPrefix ? "UNITY_UNIFORM " : "", HLSLcc::GetConstructorForType(psContext, psType->Type, 1), Name);

		if(psType->Elements > 1)
		{
			bformata(glsl, "[%d]", psType->Elements);
		}
	}
	if(unsizedArray)
		bformata(glsl, "[]");
	bformata(glsl, ";\n");
}

//In GLSL embedded structure definitions are not supported.
static void PreDeclareStructType(const HLSLCrossCompilerContext *psContext, const std::string &name, const struct ShaderVarType* psType)
{
	bstring glsl = *psContext->currentGLSLString;
	uint32_t i;

	for(i=0; i<psType->MemberCount; ++i)
	{
		if(psType->Members[i].Class == SVC_STRUCT)
		{
			PreDeclareStructType(psContext, psType->Members[i].name, &psType->Members[i]);
		}
	}

	if(psType->Class == SVC_STRUCT)
	{
		//Not supported at the moment
		ASSERT(name != "$Element");

		bformata(glsl, "struct %s_Type {\n", name.c_str());

		for(i=0; i<psType->MemberCount; ++i)
		{
			ASSERT(psType->Members.size() != 0);

			DeclareConstBufferShaderVariable(psContext, psType->Members[i].name.c_str(), &psType->Members[i], 0, 0, false, true);
		}

		bformata(glsl, "};\n");
	}
}


static const char* GetInterpolationString(INTERPOLATION_MODE eMode, GLLang lang)
{
	switch(eMode)
	{
		case INTERPOLATION_CONSTANT:
		{
			return "flat ";
		}
		case INTERPOLATION_LINEAR:
		{
			return "";
		}
		case INTERPOLATION_LINEAR_CENTROID:
		{
			return "centroid ";
		}
		case INTERPOLATION_LINEAR_NOPERSPECTIVE:
		{
			return lang <= LANG_ES_310 ? "" : "noperspective ";
			break;
		}
		case INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID:
		{
			return  lang <= LANG_ES_310 ? "centroid " : "noperspective centroid ";
		}
		case INTERPOLATION_LINEAR_SAMPLE:
		{
			return "sample ";
		}
		case INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE:
		{
			return  lang <= LANG_ES_310 ? "" : "noperspective sample ";
		}
		default:
		{
			return "";
		}
	}
}

static void DeclareInput(
    HLSLCrossCompilerContext* psContext,
    const Declaration* psDecl,
    const char* Interpolation, const char* StorageQualifier, const char* Precision, int iNumComponents, OPERAND_INDEX_DIMENSION eIndexDim, const char* InputName, const uint32_t ui32CompMask)
{
	Shader& psShader = psContext->psShader;
	bstring glsl = *psContext->currentGLSLString;
  RegisterSpace regSpace = psDecl->asOperands[0].GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);
	uint32_t ui32Reg = psDecl->asOperands[0].ui32RegisterNumber;
	const ShaderInfo::InOutSignature *psSig = NULL;

	// This falls within the specified index ranges. The default is 0 if no input range is specified

	if (regSpace == RegisterSpace::per_vertex)
		psContext->psShader.sInfo.GetInputSignatureFromRegister(ui32Reg, ui32CompMask, &psSig);
	else
		psContext->psShader.sInfo.GetPatchConstantSignatureFromRegister(ui32Reg, ui32CompMask, &psSig);

	ASSERT(psSig != NULL);

	// No need to declare input pos 0 on HS control point phases, it's always position
	// Also no point in declaring the builtins
	if (psShader.eShaderType == HULL_SHADER && psShader.asPhases[psContext->currentPhase].ePhase == HS_CTRL_POINT_PHASE)
	{
		if (regSpace == RegisterSpace::per_vertex)
		{
			if (psSig->semanticName ==  "POS" && psSig->ui32SemanticIndex == 0)
				return;
		}
	}

	if((ui32CompMask & ~psShader.acInputDeclared[(int)regSpace][ui32Reg]) != 0)
	{
		const char* vecType = "vec";
		const char* scalarType = "float";

		switch(psSig->eComponentType)
		{
			case INOUT_COMPONENT_UINT32:
			{
				vecType = "uvec";
				scalarType = "uint";
				break;
			}
			case INOUT_COMPONENT_SINT32:
			{
				vecType = "ivec";
				scalarType = "int";
				break;
			}
			case INOUT_COMPONENT_FLOAT32:
			{
				break;
			}
			default:
			{
				ASSERT(0);
				break;
			}
		}

		if(psShader.eShaderType == PIXEL_SHADER)
		{
			psContext->psDependencies.SetInterpolationMode(ui32Reg, psDecl->value.eInterpolation);
		}

		std::string locationQualifier = "";

		if (HaveInOutLocationQualifier(psContext->psShader.eTargetLanguage))
		{
			bool addLocation = false;

			// Add locations to vertex shader inputs unless disabled in flags
			if (psShader.eShaderType == VERTEX_SHADER && !(psContext->flags & HLSLCC_FLAG_DISABLE_EXPLICIT_LOCATIONS))
				addLocation = true;

			// Add intra-shader locations if requested in flags
			if (psShader.eShaderType != VERTEX_SHADER && (psContext->flags & HLSLCC_FLAG_SEPARABLE_SHADER_OBJECTS))
				addLocation = true;

			if (addLocation)
			{
				std::ostringstream oss;
				oss << "layout(location = " << psContext->psDependencies.GetVaryingLocation(std::string(InputName), psShader.eShaderType, true) << ") ";
				locationQualifier = oss.str();
			}
		}

		psShader.acInputDeclared[(int)regSpace][ui32Reg] = (char)psSig->ui32Mask;
		
		// Do the reflection report on vertex shader inputs
		if (psShader.eShaderType == VERTEX_SHADER)
		{
			psContext->m_Reflection.OnInputBinding(std::string(InputName), psContext->psDependencies.GetVaryingLocation(std::string(InputName), VERTEX_SHADER, true));
		}

		switch (eIndexDim)
		{
			case INDEX_2D:
			{
				if(iNumComponents == 1)
				{
					const uint32_t regNum =  psDecl->asOperands[0].ui32RegisterNumber;
					const uint32_t arraySize = psDecl->asOperands[0].aui32ArraySizes[0];

					psContext->psShader.abScalarInput[(int)regSpace][regNum] |= (int)ui32CompMask;

					if(psShader.eShaderType == HULL_SHADER || psDecl->asOperands[0].eType == OPERAND_TYPE_INPUT_CONTROL_POINT)
						bformata(glsl, "%s%s %s %s %s [];\n", locationQualifier.c_str(), StorageQualifier, Precision, scalarType, InputName);
					else
						bformata(glsl, "%s%s %s %s %s [%d];\n", locationQualifier.c_str(), StorageQualifier, Precision, scalarType, InputName, arraySize);
				}
				else
				{
					if (psShader.eShaderType == HULL_SHADER || psDecl->asOperands[0].eType == OPERAND_TYPE_INPUT_CONTROL_POINT)
						bformata(glsl, "%s%s %s %s%d %s [];\n", locationQualifier.c_str(), StorageQualifier, Precision, vecType, iNumComponents, InputName);
					else
						bformata(glsl, "%s%s %s %s%d %s [%d];\n", locationQualifier.c_str(), StorageQualifier, Precision, vecType, iNumComponents, InputName,
							psDecl->asOperands[0].aui32ArraySizes[0]);
				}
				break;
			}
			default:
			{
				if(iNumComponents == 1)
				{
					psContext->psShader.abScalarInput[(int)regSpace][ui32Reg] |= (int)ui32CompMask;

					bformata(glsl, "%s%s%s %s %s %s;\n", locationQualifier.c_str(), Interpolation, StorageQualifier, Precision, scalarType, InputName);
				}
				else
				{
					if(psShader.aIndexedInput[(int)regSpace][ui32Reg] > 0)
					{
						bformata(glsl, "%s%s%s %s %s%d %s", locationQualifier.c_str(), Interpolation, StorageQualifier, Precision, vecType, iNumComponents, InputName);
						if (psShader.eShaderType == HULL_SHADER)
							bcatcstr(glsl, "[];\n");
						else
							bcatcstr(glsl, ";\n");
					}
					else
					{
						if (psShader.eShaderType == HULL_SHADER)
							bformata(glsl, "%s%s%s %s %s%d %s[];\n", locationQualifier.c_str(), Interpolation, StorageQualifier, Precision, vecType, iNumComponents, InputName);
						else
							bformata(glsl, "%s%s%s %s %s%d %s;\n", locationQualifier.c_str(), Interpolation, StorageQualifier, Precision, vecType, iNumComponents, InputName);
					}
				}
				break;
			}
		}
	}
}

static void AddBuiltinInput(HLSLCrossCompilerContext* /*psContext*/, const Declaration* /*psDecl*/, const char* /*builtinName*/)
{

}



void ToGLSL::AddBuiltinOutput(const Declaration* psDecl, int arrayElements, const char* builtinName)
{
	bstring glsl = *psContext->currentGLSLString;
	Shader& psShader = psContext->psShader;

	if (psDecl->asOperands[0].eSpecialName != NAME_CLIP_DISTANCE)
		return;

	psContext->psShader.asPhases[psContext->currentPhase].hasPostShaderCode = 1;

	if(psContext->OutputNeedsDeclaring(&psDecl->asOperands[0], arrayElements ? arrayElements : 1))
	{
		const ShaderInfo::InOutSignature* psSignature = NULL;

		psShader.sInfo.GetOutputSignatureFromRegister(
			psDecl->asOperands[0].ui32RegisterNumber,
			psDecl->asOperands[0].ui32CompMask,
			0,
			&psSignature);
		psContext->currentGLSLString = &psContext->psShader.asPhases[psContext->currentPhase].postShaderCode;
		glsl = *psContext->currentGLSLString;
		psContext->indent++;
		if(arrayElements)
		{

		}
		else
		{
			// Case 828454 : For some reason DX compiler seems to inject clip distance declaration to the hull shader sometimes
			// even though it's not used at all, and overlaps some completely unrelated patch constant declarations. We'll just ignore this now.
			// Revisit this if this actually pops up elsewhere.
			if(psDecl->asOperands[0].eSpecialName == NAME_CLIP_DISTANCE && psContext->psShader.eShaderType != HULL_SHADER)
			{
				int max = psDecl->asOperands[0].GetMaxComponent();

        if (IsESLanguage(psShader.eTargetLanguage))
          psContext->RequireExtension("GL_EXT_clip_cull_distance");

				int applySwizzle = psDecl->asOperands[0].GetNumSwizzleElements() > 1 ? 1 : 0;
				int index;
				int i;
				int multiplier = 1;
				const char* swizzle[] = {".x", ".y", ".z", ".w"};

				ASSERT(psSignature!=NULL);

				index = psSignature->ui32SemanticIndex;

				//Clip distance can be spread across 1 or 2 outputs (each no more than a vec4).
				//Some examples:
				//float4 clip[2] : SV_ClipDistance; //8 clip distances
				//float3 clip[2] : SV_ClipDistance; //6 clip distances
				//float4 clip : SV_ClipDistance; //4 clip distances
				//float clip : SV_ClipDistance; //1 clip distance.

				//In GLSL the clip distance built-in is an array of up to 8 floats.
				//So vector to array conversion needs to be done here.
				if(index == 1)
				{
					const ShaderInfo::InOutSignature* psFirstClipSignature;
					if (psShader.sInfo.GetOutputSignatureFromSystemValue(NAME_CLIP_DISTANCE, 1, &psFirstClipSignature))
					{
						if(psFirstClipSignature->ui32Mask & (1 << 3))
						{
							multiplier = 4;
						}
						else
						if(psFirstClipSignature->ui32Mask & (1 << 2))
						{
							multiplier = 3;
						}
						else
						if(psFirstClipSignature->ui32Mask & (1 << 1))
						{
							multiplier = 2;
						}
					}
				}

				// Add a specially crafted comment so runtime knows to enable clip planes.
				// We may end up doing 2 of these, so at runtime OR the results
				uint32_t clipmask = psDecl->asOperands[0].GetAccessMask();
				if(index != 0)
					clipmask <<= multiplier;
				bformata(psContext->glsl, "// HLSLcc_ClipDistances_%x\n", clipmask);

				psContext->psShader.asPhases[psContext->currentPhase].acOutputNeedsRedirect[psSignature->ui32Register] = 0xff;
				bformata(psContext->glsl, "vec4 phase%d_glClipDistance%d;\n", psContext->currentPhase, index);

				for(i=0; i<max; ++i)
				{
					psContext->AddIndentation();
					bformata(glsl, "%s[%d] = (", builtinName, i + multiplier*index);
					TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NONE);
					if(applySwizzle)
					{
						bformata(glsl, ")%s;\n", swizzle[i]);
					}
					else
					{
						bformata(glsl, ");\n");
					}
				}
			}

		}
		psContext->indent--;
		psContext->currentGLSLString = &psContext->glsl;
	}
}

void ToGLSL::HandleOutputRedirect(const Declaration *psDecl, const char *Precision)
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
		bformata(glsl, "%s vec4 phase%d_Output%d_%d;\n", Precision, psContext->currentPhase, regSpace, psOperand->ui32RegisterNumber);

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

			numComps = GetNumberBitsSet(psSig->ui32Mask);
			mask = psSig->ui32Mask;

			((Operand *)psOperand)->ui32CompMask = 1 << comp;
			psContext->AddIndentation();
			TranslateOperand(psOperand, TO_FLAG_NAME_ONLY);

			bcatcstr(psPhase->postShaderCode, " = ");

			if (psSig->eComponentType == INOUT_COMPONENT_SINT32)
			{
				bformata(psPhase->postShaderCode, "floatBitsToInt(");
				hasCast = 1;
			}
			else if (psSig->eComponentType == INOUT_COMPONENT_UINT32)
			{
				bformata(psPhase->postShaderCode, "floatBitsToUint(");
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

void ToGLSL::AddUserOutput(const Declaration* psDecl)
{
	bstring glsl = *psContext->currentGLSLString;
	bstring extensions = psContext->extensions;
	Shader& psShader = psContext->psShader;

	if(psContext->OutputNeedsDeclaring(&psDecl->asOperands[0], 1))
	{
		const Operand* psOperand = &psDecl->asOperands[0];
		const char* Precision = "";
		int iNumComponents;
		bstring type = nullptr;
    RegisterSpace regSpace = psDecl->asOperands[0].GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);
		uint32_t ui32Reg = psDecl->asOperands[0].ui32RegisterNumber;

		const ShaderInfo::InOutSignature* psSignature = NULL;

		if (regSpace == RegisterSpace::per_vertex)
			psShader.sInfo.GetOutputSignatureFromRegister(
			ui32Reg,
			psDecl->asOperands[0].ui32CompMask,
			psShader.ui32CurrentVertexOutputStream,
			&psSignature);
		else
			psShader.sInfo.GetPatchConstantSignatureFromRegister(ui32Reg, psDecl->asOperands[0].ui32CompMask, &psSignature);

		if (psSignature->semanticName == "POS" && psOperand->ui32RegisterNumber == 0 && psContext->psShader.eShaderType == VERTEX_SHADER)
			return;
		
		iNumComponents = GetNumberBitsSet(psSignature->ui32Mask);
		if (iNumComponents == 1 && ui32Reg != 0xFFFFFFFF)
			psContext->psShader.abScalarOutput[(int)regSpace][ui32Reg] |= (int)psDecl->asOperands[0].ui32CompMask;

		switch (psSignature->eComponentType)
		{
			case INOUT_COMPONENT_UINT32:
			{
				if (iNumComponents > 1)
					type = bformat("uvec%d", iNumComponents);
				else
					type = bformat("uint");
				break;
			}
			case INOUT_COMPONENT_SINT32:
			{
				if (iNumComponents > 1)
					type = bformat("ivec%d", iNumComponents);
				else
					type = bformat("int");
				break;
			}
			case INOUT_COMPONENT_FLOAT32:
			{
				if (iNumComponents > 1)
					type = bformat("vec%d", iNumComponents);
				else
					type = bformat("float");
				break;
			}
            default:
                ASSERT(0);
                break;
		}

		if(HavePrecisionQualifers(psShader.eTargetLanguage))
		{
			switch(psOperand->eMinPrecision)
			{
				case OPERAND_MIN_PRECISION_DEFAULT:
				{
					Precision = "highp ";
					break;
				}
				case OPERAND_MIN_PRECISION_FLOAT_16:
				{
					Precision = "mediump ";
					break;
				}
				case OPERAND_MIN_PRECISION_FLOAT_2_8:
				{
					Precision = "lowp ";
					break;
				}
				case OPERAND_MIN_PRECISION_SINT_16:
				{
					Precision = "mediump ";
					//type = "ivec";
					break;
				}
				case OPERAND_MIN_PRECISION_UINT_16:
				{
					Precision = "mediump ";
					//type = "uvec";
					break;
				}
			}
		}

		switch(psShader.eShaderType)
		{
			case PIXEL_SHADER:
			{
				switch(psDecl->asOperands[0].eType)
				{
					case OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
					case OPERAND_TYPE_OUTPUT_DEPTH:
					{

						break;
					}
					case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
					{
						bcatcstr(extensions, "#ifdef GL_ARB_conservative_depth\n");
						bcatcstr(extensions, "#extension GL_ARB_conservative_depth : enable\n");
						bcatcstr(extensions, "#endif\n");
						bcatcstr(glsl, "#ifdef GL_ARB_conservative_depth\n");
						bcatcstr(glsl, "layout (depth_greater) out float gl_FragDepth;\n");
						bcatcstr(glsl, "#endif\n");
						break;
					}
					case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
					{
						bcatcstr(extensions, "#ifdef GL_ARB_conservative_depth\n");
						bcatcstr(extensions, "#extension GL_ARB_conservative_depth : enable\n");
						bcatcstr(extensions, "#endif\n");
						bcatcstr(glsl, "#ifdef GL_ARB_conservative_depth\n");
						bcatcstr(glsl, "layout (depth_less) out float gl_FragDepth;\n");
						bcatcstr(glsl, "#endif\n");
						break;
					}
					default:
					{
						if(WriteToFragData(psContext->psShader.eTargetLanguage))
						{
							bformata(glsl, "#define Output%d gl_FragData[%d]\n", psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber);
						}
						else
						{
							char OutputName[512];
							bstring oname;
							oname = bformat("%s%s%d", psContext->outputPrefix, psSignature->semanticName.c_str(), psSignature->ui32SemanticIndex);
							strncpy(OutputName, (char *)oname->data, 512);
							bdestroy(oname);
              
              uint32_t renderTarget = psDecl->asOperands[0].ui32RegisterNumber;
              uint32_t index = 0;

							if (HaveInOutLocationQualifier(psContext->psShader.eTargetLanguage) ||
								HaveLimitedInOutLocationQualifier(psContext->psShader.eTargetLanguage, psContext->psShader.extensions))
							{
								if((psContext->flags & HLSLCC_FLAG_DUAL_SOURCE_BLENDING) && DualSourceBlendSupported(psContext->psShader.eTargetLanguage))
								{
									if(renderTarget > 0)
									{
										renderTarget = 0;
										index = 1;
									}
									bformata(glsl, "layout(location = %d, index = %d) ", renderTarget, index);
								}
								else
								{
									bformata(glsl, "layout(location = %d) ", renderTarget);
								}
							}

							bformata(glsl, "out %s%s %s;\n", Precision, type->data, OutputName);
              psContext->psDependencies.FragmentColorTarget(OutputName, renderTarget, index);
						}
						break;
					}
				}
				break;
			}
			case VERTEX_SHADER:
			case GEOMETRY_SHADER:
			case DOMAIN_SHADER:
			case HULL_SHADER:
			{
				const char* Interpolation = "";
				char OutputName[512];
				bstring oname;
				oname = bformat("%s%s%s%d", psContext->outputPrefix, regSpace == RegisterSpace::per_vertex ? "" : "patch", psSignature->semanticName.c_str(), psSignature->ui32SemanticIndex);
				strncpy(OutputName, (char *)oname->data, 512);
				bdestroy(oname);

				if (psShader.eShaderType == VERTEX_SHADER)
				{
					if (psSignature->eComponentType == INOUT_COMPONENT_UINT32 || 
						psSignature->eComponentType == INOUT_COMPONENT_SINT32) // GLSL spec requires that integer vertex outputs always have "flat" interpolation
					{
						Interpolation = GetInterpolationString(INTERPOLATION_CONSTANT, psContext->psShader.eTargetLanguage);
					}
					else // For floats we get the interpolation that was resolved from the fragment shader input
					{
						Interpolation = GetInterpolationString(psContext->psDependencies.GetInterpolationMode(psDecl->asOperands[0].ui32RegisterNumber), psContext->psShader.eTargetLanguage);
					}
				}

				if (HaveInOutLocationQualifier(psContext->psShader.eTargetLanguage) && (psContext->flags & HLSLCC_FLAG_SEPARABLE_SHADER_OBJECTS))
				{
					bformata(glsl, "layout(location = %d) ", psContext->psDependencies.GetVaryingLocation(std::string(OutputName), psShader.eShaderType, false));
				}

				if(InOutSupported(psContext->psShader.eTargetLanguage))
				{
					if (psContext->psShader.eShaderType == HULL_SHADER)
					{
						// In Hull shaders outputs are either per-vertex (and need []) or per-patch (need 'out patch')
						if (regSpace == RegisterSpace::per_vertex)
							bformata(glsl, "%sout %s%s %s[];\n", Interpolation, Precision, type->data, OutputName);
						else
							bformata(glsl, "patch %sout %s%s %s;\n", Interpolation, Precision, type->data, OutputName);
					}
					else
						bformata(glsl, "%sout %s%s %s;\n", Interpolation, Precision, type->data, OutputName);
				}
				else
				{
					bformata(glsl, "%svarying %s%s %s;\n", Interpolation, Precision, type->data, OutputName);
				}

				break;
			}
            default:
                ASSERT(0);
                break;
                
		}
		HandleOutputRedirect(psDecl, Precision);
    if(type)
      bdestroy(type);
	}

}

static void DeclareUBOConstants(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint,
							const ConstantBuffer* psCBuf,
							bstring glsl)
{
	uint32_t i;

	bool skipUnused = false;
	
	if((psContext->flags & HLSLCC_FLAG_REMOVE_UNUSED_GLOBALS) && psCBuf->name == "$Globals")
		skipUnused = true;
	
	
	std::string Name = psCBuf->name;
	if(Name == "$Globals") 
	{
		// Need to tweak Globals struct name to prevent clashes between shader stages
		char prefix = 'A';
		switch (psContext->psShader.eShaderType)
		{
		default:
			ASSERT(0);
			break;
		case COMPUTE_SHADER:
			prefix = 'C';
			break;
		case VERTEX_SHADER:
			prefix = 'V';
			break;
		case PIXEL_SHADER:
			prefix = 'P';
			break;
		case GEOMETRY_SHADER:
			prefix = 'G';
			break;
		case HULL_SHADER:
			prefix = 'H';
			break;
		case DOMAIN_SHADER:
			prefix = 'D';
			break;
		}

		Name[0] = prefix;
	}

	for(i=0; i < psCBuf->asVars.size(); ++i)
	{
		if(skipUnused && !psCBuf->asVars[i].sType.m_IsUsed)
			continue;
		
		PreDeclareStructType(psContext,
			psCBuf->asVars[i].name,
		    &psCBuf->asVars[i].sType);
	}

	if (psContext->flags & HLSLCC_FLAG_WRAP_UBO)
		bformata(glsl, "#ifndef HLSLCC_DISABLE_UNIFORM_BUFFERS\n#define UNITY_UNIFORM\n");

	/* [layout (location = X)] uniform vec4 HLSLConstantBufferName[numConsts]; */
	if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0)
	{
    const ResourceBinding* psBinding = NULL;
    psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_CBUFFER, ui32BindingPoint, &psBinding);
    GLSLCrossDependencyDataInterface::VulkanResourceBinding binding = psContext->psDependencies.GetVulkanResourceBinding(Name, psContext->psShader.eShaderType, *psBinding, false, false, 1, psCBuf->ui32TotalSizeInBytes);
		bformata(glsl, "layout(set = %d, binding = %d, std140) ", binding.first, binding.second);
	}
	else
	{
		if (HaveUniformBindingsAndLocations(psContext->psShader.eTargetLanguage, psContext->psShader.extensions, psContext->flags))
			bformata(glsl, "layout(binding = %d, std140) ", ui32BindingPoint);
		else
			bcatcstr(glsl, "layout(std140) ");
	}

	bformata(glsl, "uniform %s {\n", Name.c_str());

	if (psContext->flags & HLSLCC_FLAG_WRAP_UBO)
		bformata(glsl, "#else\n#define UNITY_UNIFORM uniform\n#endif\n");

  uint32_t aliasCheckOffset = 0;
	for(i=0; i < psCBuf->asVars.size(); ++i)
	{
		if(skipUnused && !psCBuf->asVars[i].sType.m_IsUsed)
			continue;
    // detect aliasing variables and comment them out if they are not used
    // or produce an syntax error if it is used, as aliasing variables
    // are not allowed in GLSL
    uint32_t varStart = psCBuf->asVars[i].ui32StartOffset;
    uint32_t varEnd = varStart + psCBuf->asVars[i].ui32Size;
    if(!psCBuf->asVars[i].sType.m_IsUsed)
    {
      if(aliasCheckOffset>=varEnd)
      {
        bformata(glsl, "// skip aliasing ");
      }
    }
    else
    {
      if(aliasCheckOffset>=varEnd)
      {
        bformata(glsl, "!?!!? error aliasing variable is used !!!! -> ");
      }
    }
    aliasCheckOffset = varEnd;
		
		DeclareConstBufferShaderVariable(psContext,
			psCBuf->asVars[i].name.c_str(),
		    &psCBuf->asVars[i].sType, psCBuf->asVars[i].ui32StartOffset, 0, psContext->flags & HLSLCC_FLAG_WRAP_UBO ? true : false, false);
	}

	if (psContext->flags & HLSLCC_FLAG_WRAP_UBO)
		bformata(glsl, "#ifndef HLSLCC_DISABLE_UNIFORM_BUFFERS\n");
	bcatcstr(glsl, "};\n");
	if (psContext->flags & HLSLCC_FLAG_WRAP_UBO)
		bformata(glsl, "#endif\n#undef UNITY_UNIFORM\n");
}

bool DeclareRWStructuredBufferTemplateTypeAsInteger(HLSLCrossCompilerContext* psContext, const Operand* psOperand,bstring glsl)
{
    // with cases like: RWStructuredBuffer<int4> myBuffer; /*...*/ AtomicMin(myBuffer[0].x , myInt);
    // if we translate RWStructuredBuffer template type to uint, incorrect version of the function might be called ( AtomicMin(uint..) instead of AtomicMin(int..) )
    // we try to avoid this case by using integer type in those cases
    bool ret = false;
    if (psContext && psOperand)
    {
      uint32_t ui32BindingPoint = psOperand->ui32RegisterNumber;
      const ResourceBinding* psBinding = NULL;
      psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_UAV, ui32BindingPoint, &psBinding);
      if (psBinding)
      {
        const ConstantBuffer* psBuffer = NULL;
        psContext->psShader.sInfo.GetConstantBufferFromBindingPoint(RGROUP_UAV, psBinding->ui32BindPoint, &psBuffer);

        if (psBuffer && psBuffer->asVars.size() == 1 && psBuffer->asVars[0].sType.Type == SVT_INT /*&& psContext->IsSwitch()*/)
        {
          bformata(glsl, "// buffer int type, rt type %u\n", psBinding->ui32ReturnType);
          ret = true;
        }

        //RWStructuredBuffer<int> case
        ret |= psBinding->isIntUAV;
        if (psBinding->isIntUAV)
        {
          bformata(glsl, "// forced int type\n");
        }
      }
    }
    return ret;
}

static void DeclareBufferVariable(HLSLCrossCompilerContext* psContext, uint32_t ui32BindingPoint,
	const Operand* psOperand, const uint32_t ui32GloballyCoherentAccess,
	const uint32_t isRaw, const uint32_t isUAV, const uint32_t hasCounter, const uint32_t stride, bstring glsl)
{
	const bool isVulkan = (psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0;
  const bool enbedCounter = (psContext->flags & HLSLCC_FLAG_GLSL_EMBEDED_ATOMIC_COUNTER);
	bstring BufNamebstr = bfromcstr("");
	// Use original HLSL bindings for UAVs only. For non-UAV buffers we have resolved new binding points from the same register space.
	if (!isUAV && !isVulkan)
		ui32BindingPoint = psContext->psShader.aui32StructuredBufferBindingPoints[psContext->psShader.ui32CurrentStructuredBufferIndex++];

	ResourceName(BufNamebstr, psContext, isUAV ? RGROUP_UAV : RGROUP_TEXTURE, psOperand->ui32RegisterNumber, 0);

	char *btmp = bstr2cstr(BufNamebstr, '\0');
	std::string BufName = btmp;
	bcstrfree(btmp);
	bdestroy(BufNamebstr);

	// Declare the struct type for structured buffers
	if (!isRaw)
	{
		const char* typeStr = "uint";
		if (isUAV && DeclareRWStructuredBufferTemplateTypeAsInteger(psContext, psOperand, glsl))
			typeStr = "int";
		bformata(glsl, " struct %s_type {\n\t%s[%d] value;\n};\n\n", BufName.c_str(), typeStr, stride / 4);
	}

	if (isVulkan)
	{
    const ResourceBinding* psBinding = NULL;
    psContext->psShader.sInfo.GetResourceFromBindingPoint(isUAV ? RGROUP_UAV : RGROUP_TEXTURE, ui32BindingPoint, &psBinding);
    GLSLCrossDependencyDataInterface::VulkanResourceBinding binding = psContext->psDependencies.GetVulkanResourceBinding(BufName, psContext->psShader.eShaderType, *psBinding, false);
		bformata(glsl, "layout(set = %d, binding = %d, std430) ", binding.first, binding.second);
	}
	else
	{
		bformata(glsl, "layout(std430, binding = %d) ", ui32BindingPoint);
	}

	if (ui32GloballyCoherentAccess & GLOBALLY_COHERENT_ACCESS)
		bcatcstr(glsl, "coherent ");

	if (!isUAV)
		bcatcstr(glsl, "readonly ");

	bformata(glsl, "buffer %s {\n\t", BufName.c_str());

  if (hasCounter && enbedCounter)
  {
    bformata(glsl, "layout(offset = 0) uint %s_counter;\n\tlayout(offset = 256) ", BufName.c_str());
  }

	if (isRaw)
		bcatcstr(glsl, "uint");
	else
		bformata(glsl, "%s_type", BufName.c_str());

	bformata(glsl, " %s_buf[];\n};\n", BufName.c_str());

}

void ToGLSL::DeclareStructConstants(const uint32_t ui32BindingPoint,
							const ConstantBuffer* psCBuf, const Operand* psOperand,
							bstring glsl)
{
	uint32_t i;
	int useGlobalsStruct = 1;
	bool skipUnused = false;

	if((psContext->flags & HLSLCC_FLAG_DISABLE_GLOBALS_STRUCT) && psCBuf->name[0] == '$')
		useGlobalsStruct = 0;

	if((psContext->flags & HLSLCC_FLAG_REMOVE_UNUSED_GLOBALS) && psCBuf->name == "$Globals")
		skipUnused = true;

	if ((psContext->flags & HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT) == 0)
		useGlobalsStruct = 0;

	
	
	for(i=0; i < psCBuf->asVars.size(); ++i)
	{
		if(skipUnused && !psCBuf->asVars[i].sType.m_IsUsed)
			continue;
		
		PreDeclareStructType(psContext,
			psCBuf->asVars[i].name,
		    &psCBuf->asVars[i].sType);
	}

	/* [layout (location = X)] uniform vec4 HLSLConstantBufferName[numConsts]; */
	if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0)
	{
		ASSERT(0); // Catch this to see what's going on
		std::string bname = "wut";
    const ResourceBinding* psBinding = NULL;
    psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_CBUFFER, ui32BindingPoint, &psBinding);
    GLSLCrossDependencyDataInterface::VulkanResourceBinding binding = psContext->psDependencies.GetVulkanResourceBinding(bname, psContext->psShader.eShaderType, *psBinding, false);
		bformata(glsl, "layout(set = %d, binding = %d) ", binding.first, binding.second);
	}
	else
	{
		if (HaveUniformBindingsAndLocations(psContext->psShader.eTargetLanguage, psContext->psShader.extensions, psContext->flags))
			bformata(glsl, "layout(location = %d) ", ui32BindingPoint);
	}
	if(useGlobalsStruct)
	{
		bcatcstr(glsl, "uniform struct ");
		TranslateOperand(psOperand, TO_FLAG_DECLARATION_NAME);

		bcatcstr(glsl, "_Type {\n");
	}

	for(i=0; i < psCBuf->asVars.size(); ++i)
	{
		if(skipUnused && !psCBuf->asVars[i].sType.m_IsUsed)
			continue;
		
		if(!useGlobalsStruct)
			bcatcstr(glsl, "uniform ");

		DeclareConstBufferShaderVariable(psContext,
			psCBuf->asVars[i].name.c_str(),
		    &psCBuf->asVars[i].sType, psCBuf->asVars[i].ui32StartOffset, 0, false, useGlobalsStruct);
	}

	if(useGlobalsStruct)
	{
		bcatcstr(glsl, "} ");

		TranslateOperand(psOperand, TO_FLAG_DECLARATION_NAME);

		bcatcstr(glsl, ";\n");
	}
}

static const char* GetSamplerType(HLSLCrossCompilerContext* psContext,
					 const RESOURCE_DIMENSION eDimension,
					 const uint32_t ui32RegisterNumber)
{
	const ResourceBinding* psBinding = 0;
	RESOURCE_RETURN_TYPE eType = RETURN_TYPE_UNORM;
	int found;
	found = psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_TEXTURE, ui32RegisterNumber, &psBinding);
	if(found)
	{
		eType = (RESOURCE_RETURN_TYPE)psBinding->ui32ReturnType;
	}
	switch(eDimension)
	{
		case RESOURCE_DIMENSION_BUFFER:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isamplerBuffer";
				case RETURN_TYPE_UINT:
					return "usamplerBuffer";
				default:
					return "samplerBuffer";
			}
			break;
		}

		case RESOURCE_DIMENSION_TEXTURE1D:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isampler1D";
				case RETURN_TYPE_UINT:
					return "usampler1D";
				default:
					return "sampler1D";
			}
			break;
		}

		case RESOURCE_DIMENSION_TEXTURE2D:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isampler2D";
				case RETURN_TYPE_UINT:
					return "usampler2D";
				default:
					return "sampler2D";
			}
			break;
		}

		case RESOURCE_DIMENSION_TEXTURE2DMS:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isampler2DMS";
				case RETURN_TYPE_UINT:
					return "usampler2DMS";
				default:
					return "sampler2DMS";
			}
			break;
		}

		case RESOURCE_DIMENSION_TEXTURE3D:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isampler3D";
				case RETURN_TYPE_UINT:
					return "usampler3D";
				default:
					return "sampler3D";
			}
			break;
		}

		case RESOURCE_DIMENSION_TEXTURECUBE:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isamplerCube";
				case RETURN_TYPE_UINT:
					return "usamplerCube";
				default:
					return "samplerCube";
			}
			break;
		}

		case RESOURCE_DIMENSION_TEXTURE1DARRAY:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isampler1DArray";
				case RETURN_TYPE_UINT:
					return "usampler1DArray";
				default:
					return "sampler1DArray";
			}
			break;
		}

		case RESOURCE_DIMENSION_TEXTURE2DARRAY:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isampler2DArray";
				case RETURN_TYPE_UINT:
					return "usampler2DArray";
				default:
					return "sampler2DArray";
			}
			break;
		}

		case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isampler2DMSArray";
				case RETURN_TYPE_UINT:
					return "usampler2DMSArray";
				default:
					return "sampler2DMSArray";
			}
			break;
		}

		case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
		{
			switch(eType)
			{
				case RETURN_TYPE_SINT:
					return "isamplerCubeArray";
				case RETURN_TYPE_UINT:
					return "usamplerCubeArray";
				default:
					return "samplerCubeArray";
			}
			break;
		}
        default:
            ASSERT(0);
            break;

	}

	return "sampler2D";
}

static const char *GetSamplerPrecision(GLLang langVersion, REFLECT_RESOURCE_PRECISION ePrec)
{
	if (!HavePrecisionQualifers(langVersion))
		return " ";

	switch (ePrec)
	{
	default:
	case REFLECT_RESOURCE_PRECISION_UNKNOWN:
	case REFLECT_RESOURCE_PRECISION_LOWP:
		return "lowp ";
	case REFLECT_RESOURCE_PRECISION_HIGHP:
		return "highp ";
	case REFLECT_RESOURCE_PRECISION_MEDIUMP:
		return "mediump ";
	}
}

static void TranslateResourceTexture(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, uint32_t samplerCanDoShadowCmp)
{
	bstring glsl = *psContext->currentGLSLString;
	Shader& psShader = psContext->psShader;
	const char *samplerPrecision = NULL;

	const char* samplerTypeName = GetSamplerType(psContext,
		psDecl->value.eResourceDimension,
		psDecl->asOperands[0].ui32RegisterNumber);

  if (psDecl->value.eResourceDimension == RESOURCE_DIMENSION_TEXTURECUBEARRAY
      && !HaveCubemapArray(psContext->psShader.eTargetLanguage))
  {
    // Need to enable extension (either OES or ARB), but we only need to add it once
    if (IsESLanguage(psContext->psShader.eTargetLanguage))
      psContext->RequireExtension("GL_OES_texture_cube_map_array");
    else
      psContext->RequireExtension("GL_ARB_texture_cube_map_array");
  }

	const ResourceBinding *psBinding = NULL;
	psShader.sInfo.GetResourceFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &psBinding);
	ASSERT(psBinding != NULL);

	samplerPrecision = GetSamplerPrecision(psContext->psShader.eTargetLanguage, psBinding ? psBinding->ePrecision : REFLECT_RESOURCE_PRECISION_UNKNOWN);

	if (psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS)
	{
		if(samplerCanDoShadowCmp && psDecl->ui32IsShadowTex)
		{
			for (auto i = psDecl->samplersUsed.begin(); i != psDecl->samplersUsed.end(); i++)
			{
				std::string tname = TextureSamplerName(psShader.sInfo, psDecl->asOperands[0].ui32RegisterNumber, *i, 1);
				if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0)
				{
          const ResourceBinding* resBinding = NULL;
          psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &resBinding);
          GLSLCrossDependencyDataInterface::VulkanResourceBinding binding = psContext->psDependencies.GetVulkanResourceBinding(tname, psContext->psShader.eShaderType, *resBinding, true);
					bformata(glsl, "layout(set = %d, binding = %d) ", binding.first, binding.second);
				}
				bcatcstr(glsl, "uniform ");
				bcatcstr(glsl, samplerPrecision);
				bcatcstr(glsl, samplerTypeName);
				bcatcstr(glsl, "Shadow ");
				bcatcstr(glsl, tname.c_str());
				bcatcstr(glsl, ";\n");
			}
		}
		for (auto i = psDecl->samplersUsed.begin(); i != psDecl->samplersUsed.end(); i++)
		{
			std::string tname = TextureSamplerName(psShader.sInfo, psDecl->asOperands[0].ui32RegisterNumber, *i, 0);
			if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0)
			{
        const ResourceBinding* resBinding = NULL;
        psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &resBinding);
        GLSLCrossDependencyDataInterface::VulkanResourceBinding binding = psContext->psDependencies.GetVulkanResourceBinding(tname, psContext->psShader.eShaderType, *resBinding, false);
				bformata(glsl, "layout(set = %d, binding = %d) ", binding.first, binding.second);
			}
			bcatcstr(glsl, "uniform ");
			bcatcstr(glsl, samplerPrecision);
			bcatcstr(glsl, samplerTypeName);
			bcatcstr(glsl, " ");
			bcatcstr(glsl, tname.c_str());
			bcatcstr(glsl, ";\n");
		}
	}
  else
  {

    if (samplerCanDoShadowCmp && psDecl->ui32IsShadowTex)
    {
      //Create shadow and non-shadow sampler.
      //HLSL does not have separate types for depth compare, just different functions.
      std::string tname = ResourceName(psContext, RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, 1);

      if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0)
      {
        const ResourceBinding* resBinding = NULL;
        psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &resBinding);
        GLSLCrossDependencyDataInterface::VulkanResourceBinding binding = psContext->psDependencies.GetVulkanResourceBinding(tname, psContext->psShader.eShaderType, *resBinding, true);
        bformata(glsl, "layout(set = %d, binding = %d) ", binding.first, binding.second);
      }

      bcatcstr(glsl, "uniform ");
      bcatcstr(glsl, samplerPrecision);
      bcatcstr(glsl, samplerTypeName);
      bcatcstr(glsl, "Shadow ");
      bcatcstr(glsl, tname.c_str());
      bcatcstr(glsl, ";\n");
    }
    else
    {
      std::string tname = ResourceName(psContext, RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, 0);

      if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0)
      {
        const ResourceBinding* resBinding = NULL;
        psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &resBinding);
        GLSLCrossDependencyDataInterface::VulkanResourceBinding binding = psContext->psDependencies.GetVulkanResourceBinding(tname, psContext->psShader.eShaderType, *resBinding, false);
        bformata(glsl, "layout(set = %d, binding = %d) ", binding.first, binding.second);
      }
      bcatcstr(glsl, "uniform ");
      bcatcstr(glsl, samplerPrecision);
      bcatcstr(glsl, samplerTypeName);
      bcatcstr(glsl, " ");
      bcatcstr(glsl, tname.c_str());
      bcatcstr(glsl, ";\n");
    }
  }
}

void ToGLSL::HandleInputRedirect(const Declaration *psDecl, const char *Precision)
{
	Operand *psOperand = (Operand *)&psDecl->asOperands[0];
	Shader& psShader = psContext->psShader;
	bstring glsl = *psContext->currentGLSLString;
	int needsRedirect = 0;
	const ShaderInfo::InOutSignature *psSig = NULL;

  RegisterSpace regSpace = psOperand->GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);
	if (regSpace == RegisterSpace::per_vertex)
	{
        if (psShader.asPhases[psContext->currentPhase].acInputNeedsRedirect[psOperand->ui32RegisterNumber] == 0xff)
            needsRedirect = 1;
	}
	else if (psShader.asPhases[psContext->currentPhase].acPatchConstantsNeedsRedirect[psOperand->ui32RegisterNumber] == 0xff)
	{
		psContext->psShader.sInfo.GetPatchConstantSignatureFromRegister(psOperand->ui32RegisterNumber, psOperand->ui32CompMask, &psSig);
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

		psContext->AddIndentation();
		// Does the input have multiple array components (such as geometry shader input, or domain shader control point input)
		if ((psShader.eShaderType == DOMAIN_SHADER && regSpace == RegisterSpace::per_vertex) || (psShader.eShaderType == GEOMETRY_SHADER))
		{
			// The count is actually stored in psOperand->aui32ArraySizes[0]
			origArraySize = psOperand->aui32ArraySizes[0];
			bformata(glsl, "%s vec4 phase%d_Input%d_%d[%d];\n", Precision, psContext->currentPhase, (int)regSpace, psOperand->ui32RegisterNumber, origArraySize);
			needsLooping = 1;
			i = origArraySize - 1;
		}
		else
			bformata(glsl, "%s vec4 phase%d_Input%d_%d;\n", Precision, psContext->currentPhase, (int)regSpace, psOperand->ui32RegisterNumber);

		psContext->currentGLSLString = &psPhase->earlyMain;
		psContext->indent++;

		// Do a conditional loop. In normal cases needsLooping == 0 so this is only run once.
		do 
		{
			int comp = 0;
			psContext->AddIndentation();
			if (needsLooping)
				bformata(psPhase->earlyMain, "phase%d_Input%d_%d[%d] = vec4(", psContext->currentPhase, (int)regSpace, psOperand->ui32RegisterNumber, i);
			else
				bformata(psPhase->earlyMain, "phase%d_Input%d_%d = vec4(", psContext->currentPhase, (int)regSpace, psOperand->ui32RegisterNumber);

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
					numComps = GetNumberBitsSet(psSig->ui32Mask);
					if (psSig->eComponentType == INOUT_COMPONENT_SINT32)
					{
						bformata(psPhase->earlyMain, "intBitsToFloat(");
						hasCast = 1;
					}
					else if (psSig->eComponentType == INOUT_COMPONENT_UINT32)
					{
						bformata(psPhase->earlyMain, "uintBitsToFloat(");
						hasCast = 1;
					}

					// Override the array size of the operand so TranslateOperand call below prints the correct index
					if (needsLooping)
						psOperand->aui32ArraySizes[0] = i;

					// And the component mask
					psOperand->ui32CompMask = 1 << comp;

					TranslateOperand(psOperand, TO_FLAG_NAME_ONLY);

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
		psContext->indent--;

		if (regSpace == RegisterSpace::per_vertex)
			psShader.asPhases[psContext->currentPhase].acInputNeedsRedirect[psOperand->ui32RegisterNumber] = 0xfe;
		else
			psShader.asPhases[psContext->currentPhase].acPatchConstantsNeedsRedirect[psOperand->ui32RegisterNumber] = 0xfe;
	}
}

/*
 * this is the place to detect the use of cbuffer and/or uav hacks to get
 * extensions into HLSL
 */
void ToGLSL::TranslateDeclaration(const Declaration* psDecl)
{
	bstring glsl = *psContext->currentGLSLString;
	Shader& psShader = psContext->psShader;

	switch(psDecl->eOpcode)
	{
		case OPCODE_DCL_INPUT_SGV:
		case OPCODE_DCL_INPUT_PS_SGV:
		{
			const SPECIAL_NAME eSpecialName = psDecl->asOperands[0].eSpecialName;
			switch(eSpecialName)
			{
				case NAME_POSITION:
				{
					AddBuiltinInput(psContext, psDecl, "gl_Position");
					break;
				}
				case NAME_RENDER_TARGET_ARRAY_INDEX:
				{
					AddBuiltinInput(psContext, psDecl, "gl_Layer");
					break;
				}
				case NAME_CLIP_DISTANCE:
				{
					AddBuiltinInput(psContext, psDecl, "gl_ClipDistance");
					break;
				}
				case NAME_VIEWPORT_ARRAY_INDEX:
				{
					AddBuiltinInput(psContext, psDecl, "gl_ViewportIndex");
					break;
				}
				case NAME_INSTANCE_ID:
				{
					AddBuiltinInput(psContext, psDecl, "gl_InstanceID");
					break;
				}
				case NAME_IS_FRONT_FACE:
				{
                    /*
                        Cast to int used because
                        if(gl_FrontFacing != 0) failed to compiled on Intel HD 4000.
                        Suggests no implicit conversion for bool<->int.
                    */

					AddBuiltinInput(psContext, psDecl, "gl_FrontFacing"); // Hi Adreno.
					break;
				}
				case NAME_SAMPLE_INDEX:
				{
					AddBuiltinInput(psContext, psDecl, "gl_SampleID");
					break;
				}
				case NAME_VERTEX_ID:
				{
					AddBuiltinInput(psContext, psDecl, "gl_VertexID");
					psContext->RequireExtension("GL_ARB_shader_draw_parameters");
					AddBuiltinInput(psContext, psDecl, "gl_BaseVertexARB");
					break;
				}
				case NAME_PRIMITIVE_ID:
				{
					if(psShader.eShaderType == GEOMETRY_SHADER)
						AddBuiltinInput(psContext, psDecl, "gl_PrimitiveIDIn"); // LOL opengl.
					else
						AddBuiltinInput(psContext, psDecl, "gl_PrimitiveID");
					break;
				}
				default:
				{
					bformata(glsl, "in vec4 %s;\n", psDecl->asOperands[0].specialName.c_str());
				}
			}
			break;
		}

		case OPCODE_DCL_OUTPUT_SIV:
		{
			switch(psDecl->asOperands[0].eSpecialName)
			{
				case NAME_POSITION:
				{
					AddBuiltinOutput(psDecl, 0, "gl_Position");
					break;
				}
				case NAME_RENDER_TARGET_ARRAY_INDEX:
				{
					AddBuiltinOutput(psDecl, 0, "gl_Layer");
					break;
				}
				case NAME_CLIP_DISTANCE:
				{
					AddBuiltinOutput(psDecl, 0, "gl_ClipDistance");
					break;
				}
				case NAME_VIEWPORT_ARRAY_INDEX:
				{
					AddBuiltinOutput(psDecl, 0, "gl_ViewportIndex");
					break;
				}
				case NAME_VERTEX_ID:
				{
					ASSERT(0); //VertexID is not an output
					break;
				}
				case NAME_PRIMITIVE_ID:
				{
					AddBuiltinOutput(psDecl, 0, "gl_PrimitiveID");
					break;
				}
				case NAME_INSTANCE_ID:
				{
					ASSERT(0); //InstanceID is not an output
					break;
				}
				case NAME_IS_FRONT_FACE:
				{
					ASSERT(0); //FrontFacing is not an output
					break;
				}
				case NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR:
				{
					if(psContext->psShader.aIndexedOutput[1][psDecl->asOperands[0].ui32RegisterNumber])
					{
						AddBuiltinOutput(psDecl, 4, "gl_TessLevelOuter");
					}
					else
					{
						AddBuiltinOutput(psDecl, 0, "gl_TessLevelOuter[0]");
					}
					break;
				}
				case NAME_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR:
				{
					AddBuiltinOutput(psDecl, 0, "gl_TessLevelOuter[1]");
					break;
				}
				case NAME_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR:
				{
					AddBuiltinOutput(psDecl, 0, "gl_TessLevelOuter[2]");
					break;
				}
				case NAME_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR:
				{
					AddBuiltinOutput(psDecl, 0, "gl_TessLevelOuter[3]");
					break;
				}
				case NAME_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR:
				{
					if(psContext->psShader.aIndexedOutput[1][psDecl->asOperands[0].ui32RegisterNumber])
					{
						AddBuiltinOutput(psDecl, 3, "gl_TessLevelOuter");
					}
					else
					{
						AddBuiltinOutput(psDecl, 0, "gl_TessLevelOuter[0]");
					}
					break;
				}
				case NAME_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR:
				{
					AddBuiltinOutput(psDecl, 0, "gl_TessLevelOuter[1]");
					break;
				}
				case NAME_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR:
				{
					AddBuiltinOutput(psDecl, 0, "gl_TessLevelOuter[2]");
					break;
				}
				case NAME_FINAL_LINE_DENSITY_TESSFACTOR:
				{
					if(psContext->psShader.aIndexedOutput[1][psDecl->asOperands[0].ui32RegisterNumber])
					{
						AddBuiltinOutput(psDecl, 2, "gl_TessLevelOuter");
					}
					else
					{
						AddBuiltinOutput(psDecl, 0, "gl_TessLevelOuter[0]");
					}
					break;
				}
				case NAME_FINAL_LINE_DETAIL_TESSFACTOR:
				{
					AddBuiltinOutput(psDecl, 0, "gl_TessLevelOuter[1]");
					break;
				}
				case NAME_FINAL_TRI_INSIDE_TESSFACTOR:
				case NAME_FINAL_QUAD_U_INSIDE_TESSFACTOR:
				{
					if(psContext->psShader.aIndexedOutput[1][psDecl->asOperands[0].ui32RegisterNumber])
					{
						AddBuiltinOutput(psDecl, 2, "gl_TessLevelInner");
					}
					else
					{
						AddBuiltinOutput(psDecl, 0, "gl_TessLevelInner[0]");
					}
					break;
				}
				case NAME_FINAL_QUAD_V_INSIDE_TESSFACTOR:
				{
					AddBuiltinOutput(psDecl, 0, "gl_TessLevelInner[1]");
					break;
				}
				default:
				{
					// Sometimes DX compiler seems to declare patch constant outputs like this. Anyway, nothing for us to do.
//					bformata(glsl, "out vec4 %s;\n", psDecl->asOperands[0].specialName.c_str());

/*                    bcatcstr(glsl, "#define ");
                    TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
                    bformata(glsl, " %s\n", psDecl->asOperands[0].pszSpecialName);
                    break;*/
				}
			}
			break;
		}
		case OPCODE_DCL_INPUT:
		{
			const Operand* psOperand = &psDecl->asOperands[0];

			if((psOperand->eType == OPERAND_TYPE_INPUT_DOMAIN_POINT)||
			(psOperand->eType == OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID)||
			(psOperand->eType == OPERAND_TYPE_INPUT_COVERAGE_MASK)||
			(psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID)||
			(psOperand->eType == OPERAND_TYPE_INPUT_THREAD_GROUP_ID)||
			(psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP)||
			(psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED) ||
			(psOperand->eType == OPERAND_TYPE_INPUT_FORK_INSTANCE_ID))
			{
				break;
			}

			// No need to declare patch constants read again by the hull shader.
			if ((psOperand->eType == OPERAND_TYPE_INPUT_PATCH_CONSTANT) && psContext->psShader.eShaderType == HULL_SHADER)
			{
				break;
			}
			// ...or control points
			if ((psOperand->eType == OPERAND_TYPE_INPUT_CONTROL_POINT || (psOperand->eType == OPERAND_TYPE_OUTPUT_CONTROL_POINT))
				&& psContext->psShader.eShaderType == HULL_SHADER)
			{
				break;
			}

			// Also skip position input to domain shader
			if ((psOperand->eType == OPERAND_TYPE_INPUT_CONTROL_POINT) && psContext->psShader.eShaderType == DOMAIN_SHADER)
			{
				const ShaderInfo::InOutSignature *psIn = NULL;
				psContext->psShader.sInfo.GetInputSignatureFromRegister(psOperand->ui32RegisterNumber, psOperand->GetAccessMask(), &psIn);
				ASSERT(psIn != NULL);

				if ((psIn->semanticName == "SV_POSITION" || psIn->semanticName == "POS") && psIn->ui32SemanticIndex == 0)
					break;
			}

			//Already declared as part of an array.
			if(psShader.aIndexedInput[(int)psOperand->GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase)][psDecl->asOperands[0].ui32RegisterNumber] == -1)
			{
				break;
			}

			int iNumComponents = psOperand->GetNumInputElements(psContext);
			const char* StorageQualifier = "attribute";
			std::string inputName;
			const char* Precision = "";

			inputName = psContext->GetDeclaredInputName(*psOperand, NULL, 1, NULL);

			if(InOutSupported(psContext->psShader.eTargetLanguage))
			{
				if (psOperand->eType == OPERAND_TYPE_INPUT_PATCH_CONSTANT && psContext->psShader.eShaderType == DOMAIN_SHADER)
					StorageQualifier = "patch in";
				else
					StorageQualifier = "in";
			}

			if(HavePrecisionQualifers(psShader.eTargetLanguage))
			{
				switch(psOperand->eMinPrecision)
				{
					case OPERAND_MIN_PRECISION_DEFAULT:
					{
						Precision = "highp";
						break;
					}
					case OPERAND_MIN_PRECISION_FLOAT_16:
					{
						Precision = "mediump";
						break;
					}
					case OPERAND_MIN_PRECISION_FLOAT_2_8:
					{
						Precision = "lowp";
						break;
					}
					case OPERAND_MIN_PRECISION_SINT_16:
					{
						Precision = "mediump";
						break;
					}
					case OPERAND_MIN_PRECISION_UINT_16:
					{
						Precision = "mediump";
						break;
					}
				}
			}

			DeclareInput(psContext, psDecl,
			    "", StorageQualifier, Precision, iNumComponents, (OPERAND_INDEX_DIMENSION)psOperand->iIndexDims, inputName.c_str(), psOperand->ui32CompMask);
			if (inputName == "gl_VertexID" || inputName == "gl_VertexIndex")
			{
				psContext->RequireExtension("GL_ARB_shader_draw_parameters");
				DeclareInput(psContext, psDecl,
					"", StorageQualifier, Precision, iNumComponents, (OPERAND_INDEX_DIMENSION)psOperand->iIndexDims, "gl_BaseVertexARB", psOperand->ui32CompMask);
			}
			HandleInputRedirect(psDecl, Precision);
			break;
		}
		case OPCODE_DCL_INPUT_PS_SIV:
		{
			switch(psDecl->asOperands[0].eSpecialName)
			{
				case NAME_POSITION:
				{
					AddBuiltinInput(psContext, psDecl, "gl_FragCoord");
					break;
				}
                default:
                    ASSERT(0);
                    break;
                    
			}
			break;
		}
		case OPCODE_DCL_INPUT_SIV:
		{
			if(psShader.eShaderType == PIXEL_SHADER)
			{
				psContext->psDependencies.SetInterpolationMode(psDecl->asOperands[0].ui32RegisterNumber, psDecl->value.eInterpolation);
			}
			break;
		}
		case OPCODE_DCL_INPUT_PS:
		{
			const Operand* psOperand = &psDecl->asOperands[0];
			int iNumComponents = psOperand->GetNumInputElements(psContext);
			const char* StorageQualifier = "varying";
			const char* Precision = "";
			std::string inputName;
			const char* Interpolation = "";
			int hasNoPerspective = psContext->psShader.eTargetLanguage <= LANG_ES_310 ? 0 : 1;
			inputName = psContext->GetDeclaredInputName(*psOperand, NULL, 1, NULL);
			if (InOutSupported(psContext->psShader.eTargetLanguage))
			{
				StorageQualifier = "in";
			}

			switch(psDecl->value.eInterpolation)
			{
				case INTERPOLATION_CONSTANT:
				{
					Interpolation = "flat ";
					break;
				}
				case INTERPOLATION_LINEAR:
				{
					break;
				}
				case INTERPOLATION_LINEAR_CENTROID:
				{
					Interpolation = "centroid ";
					break;
				}
				case INTERPOLATION_LINEAR_NOPERSPECTIVE:
				{
					Interpolation = hasNoPerspective ? "noperspective " : "";
					break;
				}
				case INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID:
				{
					Interpolation = hasNoPerspective ? "noperspective centroid " : "centroid" ;
					break;
				}
				case INTERPOLATION_LINEAR_SAMPLE:
				{
					Interpolation = hasNoPerspective ? "sample " : "";
					break;
				}
				case INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE:
				{
					Interpolation = hasNoPerspective ? "noperspective sample " : "";
					break;
				}
                default:
                    ASSERT(0);
                    break;
			}

			if(HavePrecisionQualifers(psShader.eTargetLanguage))
			{
				switch(psOperand->eMinPrecision)
				{
					case OPERAND_MIN_PRECISION_DEFAULT:
					{
						Precision = "highp";
						break;
					}
					case OPERAND_MIN_PRECISION_FLOAT_16:
					{
						Precision = "mediump";
						break;
					}
					case OPERAND_MIN_PRECISION_FLOAT_2_8:
					{
						Precision = "lowp";
						break;
					}
					case OPERAND_MIN_PRECISION_SINT_16:
					{
						Precision = "mediump";
						break;
					}
					case OPERAND_MIN_PRECISION_UINT_16:
					{
						Precision = "mediump";
						break;
					}
				}
			}

			DeclareInput(psContext, psDecl,
				Interpolation, StorageQualifier, Precision, iNumComponents, INDEX_1D, inputName.c_str(), psOperand->ui32CompMask);

			HandleInputRedirect(psDecl, Precision);

			break;
		}
		case OPCODE_DCL_TEMPS:
		{
			uint32_t i = 0;
			const uint32_t ui32NumTemps = psDecl->value.ui32NumTemps;
			bool usePrecision = (HavePrecisionQualifers(psShader.eTargetLanguage) != 0);

			for (i = 0; i < ui32NumTemps; i++)
			{
				if (psShader.psFloatTempSizes[i] != 0)
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_FLOAT, psShader.psFloatTempSizes[i], usePrecision), i);
				if (psShader.psFloat16TempSizes[i] != 0)
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "16_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_FLOAT16, psShader.psFloat16TempSizes[i], usePrecision), i);
				if (psShader.psFloat10TempSizes[i] != 0)
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "10_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_FLOAT10, psShader.psFloat10TempSizes[i], usePrecision), i);
				if (psShader.psIntTempSizes[i] != 0)
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "i%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_INT, psShader.psIntTempSizes[i], usePrecision), i);
				if (psShader.psInt16TempSizes[i] != 0)
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "i16_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_INT16, psShader.psInt16TempSizes[i], usePrecision), i);
				if (psShader.psInt12TempSizes[i] != 0)
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "i12_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_INT12, psShader.psInt12TempSizes[i], usePrecision), i);
				if (psShader.psUIntTempSizes[i] != 0)
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "u%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_UINT, psShader.psUIntTempSizes[i], usePrecision), i);
				if (psShader.psUInt16TempSizes[i] != 0)
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "u16_%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_UINT16, psShader.psUInt16TempSizes[i], usePrecision), i);
				if (psShader.fp64 && (psShader.psDoubleTempSizes[i] != 0))
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "d%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_DOUBLE, psShader.psDoubleTempSizes[i], usePrecision), i);
				if (psShader.psBoolTempSizes[i] != 0)
					bformata(glsl, "%s " HLSLCC_TEMP_PREFIX "b%d;\n", HLSLcc::GetConstructorForType(psContext, SVT_BOOL, psShader.psBoolTempSizes[i], usePrecision), i);
			}
			break;
		}
		case OPCODE_SPECIAL_DCL_IMMCONST:
		{
			//ASSERT(0 && "DX9 shaders no longer supported!");
			break;
		}
		case OPCODE_DCL_CONSTANT_BUFFER:
		{
			const Operand* psOperand = &psDecl->asOperands[0];
			const uint32_t ui32BindingPoint = psOperand->aui32ArraySizes[0];

			const char* StageName = "VS";

			const ConstantBuffer* psCBuf = NULL;
			psContext->psShader.sInfo.GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, ui32BindingPoint, &psCBuf);

			// We don't have a original resource name, maybe generate one???
			if(!psCBuf)
			{
				if (HaveUniformBindingsAndLocations(psContext->psShader.eTargetLanguage, psContext->psShader.extensions, psContext->flags))
					bformata(glsl, "layout(location = %d) ",ui32BindingPoint);
					
				bformata(glsl, "layout(std140) uniform ConstantBuffer%d {\n\tvec4 data[%d];\n} cb%d;\n", ui32BindingPoint,psOperand->aui32ArraySizes[1],ui32BindingPoint);
				break;
			}

			switch(psContext->psShader.eShaderType)
			{
				case PIXEL_SHADER:
				{
					StageName = "PS";
					break;
				}
				case HULL_SHADER:
				{
					StageName = "HS";
					break;
				}
				case DOMAIN_SHADER:
				{
					StageName = "DS";
					break;
				}
				case GEOMETRY_SHADER:
				{
					StageName = "GS";
					break;
				}
				case COMPUTE_SHADER:
				{
					StageName = "CS";
					break;
				}
				default:
				{
					break;
				}
			}

			if(psContext->flags & HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT)
			{
				if(psContext->flags & HLSLCC_FLAG_GLOBAL_CONSTS_NEVER_IN_UBO && psCBuf->name[0] == '$')
				{
					DeclareStructConstants(ui32BindingPoint, psCBuf, psOperand, glsl);
				}
				else
				{
					DeclareUBOConstants(psContext, ui32BindingPoint, psCBuf, glsl);
				}
			}
			else
			{
				DeclareStructConstants(ui32BindingPoint, psCBuf, psOperand, glsl);
			}
			break;
		}
		case OPCODE_DCL_RESOURCE:
		{
      if (HaveUniformBindingsAndLocations(psContext->psShader.eTargetLanguage, psContext->psShader.extensions, psContext->flags))
      {
        // Explicit layout bindings are not currently compatible with combined texture samplers. The layout below assumes there is exactly one GLSL sampler
        // for each HLSL texture declaration, but when combining textures+samplers, there can be multiple OGL samplers for each HLSL texture declaration.
        if (!(psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS))
        {
          if (!(psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS))
          {
            //Constant buffer locations start at 0. Resource locations start at ui32NumConstantBuffers.
            bformata(glsl, "layout(location = %d) ",
                     psContext->psShader.sInfo.psConstantBuffers.size() + psDecl->asOperands[0].ui32RegisterNumber);
          }
        }
      }

			switch(psDecl->value.eResourceDimension)
			{
				case RESOURCE_DIMENSION_BUFFER:
        {
          if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0)
          {
            const ResourceBinding *psBinding = NULL;
            psShader.sInfo.GetResourceFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &psBinding);
            std::string tname = psBinding->name;
            GLSLCrossDependencyDataInterface::VulkanResourceBinding binding = psContext->psDependencies.GetVulkanResourceBinding(tname, psContext->psShader.eShaderType, *psBinding, true);
            bformata(glsl, "layout(set = %d, binding = %d) ", binding.first, binding.second);
          }

          bcatcstr(glsl, "uniform ");
          if (IsESLanguage(psContext->psShader.eTargetLanguage))
            bcatcstr(glsl, "highp ");
          bformata(glsl, "%s ", GetSamplerType(psContext,
                   RESOURCE_DIMENSION_BUFFER,
                   psDecl->asOperands[0].ui32RegisterNumber));
          TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NONE);
          bcatcstr(glsl, ";\n");
          break;
        }
				case RESOURCE_DIMENSION_TEXTURE1D:
				{
					TranslateResourceTexture(psContext, psDecl, 1);
					break;
				}
				case RESOURCE_DIMENSION_TEXTURE2D:
				{
					TranslateResourceTexture(psContext, psDecl, 1);
					break;
				}
				case RESOURCE_DIMENSION_TEXTURE2DMS:
				{
					TranslateResourceTexture(psContext, psDecl, 0);
					break;
				}
				case RESOURCE_DIMENSION_TEXTURE3D:
				{
					TranslateResourceTexture(psContext, psDecl, 0);
					break;
				}
				case RESOURCE_DIMENSION_TEXTURECUBE:
				{
					TranslateResourceTexture(psContext, psDecl, 1);
					break;
				}
				case RESOURCE_DIMENSION_TEXTURE1DARRAY:
				{
					TranslateResourceTexture(psContext, psDecl, 1);
					break;
				}
				case RESOURCE_DIMENSION_TEXTURE2DARRAY:
				{
					TranslateResourceTexture(psContext, psDecl, 1);
					break;
				}
				case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
				{
					TranslateResourceTexture(psContext, psDecl, 0);
					break;
				}
				case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
				{
					TranslateResourceTexture(psContext, psDecl, 1);
					break;
				}
                default:
                    ASSERT(0);
                    break;

			}
			psShader.aeResourceDims[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->value.eResourceDimension;
			break;
		}
		case OPCODE_DCL_OUTPUT:
		{
			bool needsDeclare = true;
			if(psShader.eShaderType == HULL_SHADER && psShader.asPhases[psContext->currentPhase].ePhase == HS_CTRL_POINT_PHASE && psDecl->asOperands[0].ui32RegisterNumber==0)
			{
				// Need extra check from signature:
				const ShaderInfo::InOutSignature *sig = NULL;
				psShader.sInfo.GetOutputSignatureFromRegister(0, psDecl->asOperands->GetAccessMask(), 0, &sig, true);
				if (!sig || sig->semanticName == "POSITION" || sig->semanticName == "POS")
				{
					needsDeclare = false;
					AddBuiltinOutput(psDecl, 0, "gl_out[gl_InvocationID].gl_Position");
				}
			}
			
			if(needsDeclare)
			{
				AddUserOutput(psDecl);
			}
			break;
		}
		case OPCODE_DCL_GLOBAL_FLAGS:
		{
			uint32_t ui32Flags = psDecl->value.ui32GlobalFlags;

			if(ui32Flags & GLOBAL_FLAG_FORCE_EARLY_DEPTH_STENCIL)
			{
				bcatcstr(glsl, "layout(early_fragment_tests) in;\n");
			}
			if(!(ui32Flags & GLOBAL_FLAG_REFACTORING_ALLOWED))
			{
				//TODO add precise
				//HLSL precise - http://msdn.microsoft.com/en-us/library/windows/desktop/hh447204(v=vs.85).aspx
			}
			if(ui32Flags & GLOBAL_FLAG_ENABLE_DOUBLE_PRECISION_FLOAT_OPS)
			{
        psContext->RequireExtension("GL_ARB_gpu_shader_fp64");
				psShader.fp64 = 1;
			}
			break;
		}

		case OPCODE_DCL_THREAD_GROUP:
		{
			bformata(glsl, "layout(local_size_x = %d, local_size_y = %d, local_size_z = %d) in;\n",
			    psDecl->value.aui32WorkGroupSize[0],
			    psDecl->value.aui32WorkGroupSize[1],
			    psDecl->value.aui32WorkGroupSize[2]);
			break;
		}
		case OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
		{
			if(psContext->psShader.eShaderType == HULL_SHADER)
			{
				psContext->psShader.sInfo.eTessOutPrim = psDecl->value.eTessOutPrim;
				// Invert triangle winding order to match glsl better, except on vulkan
				if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) == 0)
				{
					if (psContext->psShader.sInfo.eTessOutPrim == TESSELLATOR_OUTPUT_TRIANGLE_CW)
						psContext->psShader.sInfo.eTessOutPrim = TESSELLATOR_OUTPUT_TRIANGLE_CCW;
					else if (psContext->psShader.sInfo.eTessOutPrim == TESSELLATOR_OUTPUT_TRIANGLE_CCW)
						psContext->psShader.sInfo.eTessOutPrim = TESSELLATOR_OUTPUT_TRIANGLE_CW;
				}
			}
			break;
		}
		case OPCODE_DCL_TESS_DOMAIN:
		{
			if(psContext->psShader.eShaderType == DOMAIN_SHADER)
			{
				switch(psDecl->value.eTessDomain)
				{
					case TESSELLATOR_DOMAIN_ISOLINE:
					{
						bcatcstr(glsl, "layout(isolines) in;\n");
						break;
					}
					case TESSELLATOR_DOMAIN_TRI:
					{
						bcatcstr(glsl, "layout(triangles) in;\n");
						break;
					}
					case TESSELLATOR_DOMAIN_QUAD:
					{
						bcatcstr(glsl, "layout(quads) in;\n");
						break;
					}
					default:
					{
						break;
					}
				}
			}
			break;
		}
		case OPCODE_DCL_TESS_PARTITIONING:
		{
			if(psContext->psShader.eShaderType == HULL_SHADER)
			{
				psContext->psShader.sInfo.eTessPartitioning = psDecl->value.eTessPartitioning;
			}
			break;
		}
		case OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
		{
			switch(psDecl->value.eOutputPrimitiveTopology)
			{
				case PRIMITIVE_TOPOLOGY_POINTLIST:
				{
					bcatcstr(glsl, "layout(points) out;\n");
					break;
				}
				case PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
				case PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
				case PRIMITIVE_TOPOLOGY_LINELIST:
				case PRIMITIVE_TOPOLOGY_LINESTRIP:
				{
					bcatcstr(glsl, "layout(line_strip) out;\n");
					break;
				}

				case PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
				case PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
				case PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
				case PRIMITIVE_TOPOLOGY_TRIANGLELIST:
				{
					bcatcstr(glsl, "layout(triangle_strip) out;\n");
					break;
				}
				default:
				{
					break;
				}
			}
			break;
		}
		case OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
		{
			bformata(glsl, "layout(max_vertices = %d) out;\n", psDecl->value.ui32MaxOutputVertexCount);
			break;
		}
		case OPCODE_DCL_GS_INPUT_PRIMITIVE:
		{
			switch(psDecl->value.eInputPrimitive)
			{
				case PRIMITIVE_POINT:
				{
					bcatcstr(glsl, "layout(points) in;\n");
					break;
				}
				case PRIMITIVE_LINE:
				{
					bcatcstr(glsl, "layout(lines) in;\n");
					break;
				}
				case PRIMITIVE_LINE_ADJ:
				{
					bcatcstr(glsl, "layout(lines_adjacency) in;\n");
					break;
				}
				case PRIMITIVE_TRIANGLE:
				{
					bcatcstr(glsl, "layout(triangles) in;\n");
					break;
				}
				case PRIMITIVE_TRIANGLE_ADJ:
				{
					bcatcstr(glsl, "layout(triangles_adjacency) in;\n");
					break;
				}
				default:
				{
					break;
				}
			}
			break;
		}
		case OPCODE_DCL_INTERFACE:
		{
			const uint32_t interfaceID = psDecl->value.interface.ui32InterfaceID;
			const uint32_t numUniforms = psDecl->value.interface.ui32ArraySize;
			const uint32_t ui32NumBodiesPerTable = psContext->psShader.funcPointer[interfaceID].ui32NumBodiesPerTable;
			ShaderVar* psVar;
			uint32_t varFound;

			const char* uniformName;

			varFound = psContext->psShader.sInfo.GetInterfaceVarFromOffset(interfaceID, &psVar);
			ASSERT(varFound);
			uniformName = &psVar->name[0];

			bformata(glsl, "subroutine uniform SubroutineType %s[%d*%d];\n", uniformName, numUniforms, ui32NumBodiesPerTable);
			break;
		}
		case OPCODE_DCL_FUNCTION_BODY:
		{
			//bformata(glsl, "void Func%d();//%d\n", psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].eType);
			break;
		}
		case OPCODE_DCL_FUNCTION_TABLE:
		{
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
					bformata(glsl, "float ImmCB_%d_%d_%d[%d];\n", psContext->currentPhase, chunk.first, chunk.second.m_Rebase, chunk.second.m_Size);
				else
					bformata(glsl, "vec%d ImmCB_%d_%d_%d[%d];\n", componentCount, psContext->currentPhase, chunk.first, chunk.second.m_Rebase, chunk.second.m_Size);

				bstring tgt = psContext->psShader.asPhases[psContext->currentPhase].earlyMain;
				Declaration *psDecl = psContext->psShader.asPhases[psContext->currentPhase].m_ConstantArrayInfo.m_OrigDeclaration;
				if (componentCount == 1)
				{
					for (uint32_t i = 0; i < chunk.second.m_Size; i++)
					{
						float val[4] = {
							*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].a,
							*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].b,
							*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].c,
							*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].d
						};
						bformata(tgt, "\tImmCB_%d_%d_%d[%d] = ", psContext->currentPhase, chunk.first, chunk.second.m_Rebase, i);
						if (fpcheck(val[chunk.second.m_Rebase]))
							bformata(tgt, "uintBitsToFloat(uint(%Xu))", *(uint32_t *)&val[chunk.second.m_Rebase]);
						else
							HLSLcc::PrintFloat(tgt, val[chunk.second.m_Rebase]);
						bcatcstr(tgt, ";\n");
					}
				}
				else
				{
					for (uint32_t i = 0; i < chunk.second.m_Size; i++)
					{
						float val[4] = {
							*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].a,
							*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].b,
							*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].c,
							*(float*)&psDecl->asImmediateConstBuffer[i + chunk.first].d
						};
						bformata(tgt, "\tImmCB_%d_%d_%d[%d] = vec%d(", psContext->currentPhase, chunk.first, chunk.second.m_Rebase, i, componentCount);
						for (uint32_t k = 0; k < componentCount; k++)
						{
							if (k != 0)
								bcatcstr(tgt, ", ");
							if (fpcheck(val[k]))
								bformata(tgt, "uintBitsToFloat(uint(%Xu))", *(uint32_t *)&val[k + chunk.second.m_Rebase]);
							else
								HLSLcc::PrintFloat(tgt, val[k + chunk.second.m_Rebase]);
						}
						bcatcstr(tgt, ");\n");
					}
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
			bformata(glsl, "vec%d TempArray%d[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
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
				const char* type = "vec";
				const char* Precision = "";
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
					type = "uvec";
					break;
				}
				case INOUT_COMPONENT_SINT32:
				{
					type = "ivec";
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

				if (HavePrecisionQualifers(psShader.eTargetLanguage))
				{
					switch (psSignature->eMinPrec) // TODO What if the inputs in the indexed range are of different precisions?
					{
					default:
					{
						Precision = "highp ";
						break;
					}
					case MIN_PRECISION_ANY_16:
					case MIN_PRECISION_FLOAT_16:
					case MIN_PRECISION_SINT_16:
					case MIN_PRECISION_UINT_16:
					{
						Precision = "mediump ";
						break;
					}
					case MIN_PRECISION_FLOAT_2_8:
					{
						Precision = "lowp ";
						break;
					}
					}
				}

				startReg = psDecl->asOperands[0].ui32RegisterNumber;
				bformata(glsl, "%s%s4 phase%d_%sput%d_%d[%d];\n", Precision, type, psContext->currentPhase, isInput ? "In" : "Out", regSpace, startReg, psDecl->value.ui32IndexRange);
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
          RegisterSpace regSpace2 = psDecl->asOperands[0].GetRegisterSpace(psContext->psShader.eShaderType, psContext->psShader.asPhases[psContext->currentPhase].ePhase);

					if (regSpace2 == RegisterSpace::per_vertex)
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

						bformata(glsl, "phase%d_Input%d_%d[%d]", psContext->currentPhase, regSpace2, startReg, i);

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

						bformata(glsl, " = phase%d_Output%d_%d[%d]", psContext->currentPhase, regSpace2, startReg, i);

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
			break;
		}
		case OPCODE_DCL_INPUT_CONTROL_POINT_COUNT:
		{
			break;
		}
		case OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT:
		{
			if(psContext->psShader.eShaderType == HULL_SHADER)
			{
				bformata(glsl, "layout(vertices=%d) out;\n", psDecl->value.ui32MaxOutputVertexCount);
			}
			break;
		}
		case OPCODE_HS_FORK_PHASE:
		{
			break;
		}
		case OPCODE_HS_JOIN_PHASE:
		{
			break;
		}
		case OPCODE_DCL_SAMPLER:
		{
			break;
		}
		case OPCODE_DCL_HS_MAX_TESSFACTOR:
		{
			//For GLSL the max tessellation factor is fixed to the value of gl_MaxTessGenLevel.
			break;
		}
		case OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
		{
			// non-float images need either 'i' or 'u' prefix.
			char imageTypePrefix[2] = { 0, 0 };
			uint32_t bindpoint = psDecl->asOperands[0].ui32RegisterNumber;
			const bool isVulkan = (psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS);
      bool skipFormat = false;

			if(psDecl->sUAV.ui32GloballyCoherentAccess & GLOBALLY_COHERENT_ACCESS)
			{
				bcatcstr(glsl, "coherent ");
			}

			if (!(psDecl->sUAV.ui32AccessFlags & ACCESS_FLAG_READ) &&
				!(psContext->flags & HLSLCC_FLAG_GLES31_IMAGE_QUALIFIERS) && !isVulkan)
			{ //Special case on desktop glsl: writeonly image does not need format qualifier
				bformata(glsl, "writeonly layout(binding=%d) ", bindpoint);
			}
      else
      {
        // Use 4 component format as a fallback if no instruction defines it
        uint32_t numComponents = psDecl->sUAV.ui32NumComponents > 0 ? psDecl->sUAV.ui32NumComponents : 4;

        if (!(psDecl->sUAV.ui32AccessFlags & ACCESS_FLAG_READ))
        {
          bcatcstr(glsl, "writeonly ");
          skipFormat = (psContext->flags & HLSLCC_FLAG_GLSL_NO_FORMAT_FOR_IMAGE_WRITES) != 0;
        }
        else if (!(psDecl->sUAV.ui32AccessFlags & ACCESS_FLAG_WRITE))
          bcatcstr(glsl, "readonly ");

        if ((psDecl->sUAV.ui32AccessFlags & ACCESS_FLAG_WRITE) && IsESLanguage(psShader.eTargetLanguage))
        {
          // Need to require the extension
          psContext->RequireExtension("GL_EXT_texture_buffer");
        }

        if (isVulkan)
        {
          std::string name = ResourceName(psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
          const ResourceBinding* psBinding = NULL;
          psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, &psBinding);
          GLSLCrossDependencyDataInterface::VulkanResourceBinding binding = psContext->psDependencies.GetVulkanResourceBinding(name, psContext->psShader.eShaderType, *psBinding, false);
          bformata(glsl, "layout(set = %d, binding = %d ", binding.first, binding.second);
        }
        else
          bformata(glsl, "layout(binding=%d", bindpoint);

        if (!skipFormat)
        {
          bcatcstr(glsl, ", ");

          //TODO: catch bad format cases. e.g. es supports only limited format set. no rgb formats on glsl
          if (numComponents >= 1)
            bcatcstr(glsl, "r");
          if (numComponents >= 2)
            bcatcstr(glsl, "g");
          if (numComponents >= 3)
            bcatcstr(glsl, "ba");

          switch (psDecl->sUAV.Type)
          {
          case RETURN_TYPE_FLOAT:
            bcatcstr(glsl, "32f) highp "); //TODO: half case?
            break;
          case RETURN_TYPE_UNORM:
            bcatcstr(glsl, "8) lowp ");
            break;
          case RETURN_TYPE_SNORM:
            bcatcstr(glsl, "8_snorm) lowp ");
            break;
          case RETURN_TYPE_UINT:
            bcatcstr(glsl, "32ui) highp "); //TODO: 16/8 cases?
            break;
          case RETURN_TYPE_SINT:
            bcatcstr(glsl, "32i) highp "); //TODO: 16/8 cases?
            break;
          default:
            ASSERT(0);
          }
        }
        else
        {
          bcatcstr(glsl, ") ");
        }
      }

			if (psDecl->sUAV.Type == RETURN_TYPE_UINT)
				imageTypePrefix[0] = 'u';
			else if (psDecl->sUAV.Type == RETURN_TYPE_SINT)
				imageTypePrefix[0] = 'i';

			// GLSL requires images to be always explicitly defined as uniforms
			switch(psDecl->value.eResourceDimension)
			{
      case RESOURCE_DIMENSION_BUFFER:
        if (IsESLanguage(psShader.eTargetLanguage))
          psContext->RequireExtension("GL_EXT_texture_buffer");
        bformata(glsl, "uniform %simageBuffer ", imageTypePrefix);
        break;
			case RESOURCE_DIMENSION_TEXTURE1D:
			{
				bformata(glsl, "uniform %simage1D ", imageTypePrefix);
				break;
			}
			case RESOURCE_DIMENSION_TEXTURE2D:
			{
				bformata(glsl, "uniform %simage2D ", imageTypePrefix);
				break;
			}
			case RESOURCE_DIMENSION_TEXTURE2DMS:
			{
				bformata(glsl, "uniform %simage2DMS ", imageTypePrefix);
				break;
			}
			case RESOURCE_DIMENSION_TEXTURE3D:
			{
				bformata(glsl, "uniform %simage3D ", imageTypePrefix);
				break;
			}
			case RESOURCE_DIMENSION_TEXTURECUBE:
			{
				bformata(glsl, "uniform %simageCube ", imageTypePrefix);
				break;
			}
			case RESOURCE_DIMENSION_TEXTURE1DARRAY:
			{
				bformata(glsl, "uniform %simage1DArray ", imageTypePrefix);
				break;
			}
			case RESOURCE_DIMENSION_TEXTURE2DARRAY:
			{
				bformata(glsl, "uniform %simage2DArray ", imageTypePrefix);
				break;
			}
			case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
			{
				bformata(glsl, "uniform %simage3DArray ", imageTypePrefix);
				break;
			}
			case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
			{
				bformata(glsl, "uniform %simageCubeArray ", imageTypePrefix);
				break;
			}
                default:
                    ASSERT(0);
                    break;
                    
			}
			TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NONE);
			bcatcstr(glsl, ";\n");
			break;
		}
		case OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
		{
			const bool isVulkan = (psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS);
			if(psDecl->sUAV.bCounter && !(psContext->flags & HLSLCC_FLAG_GLSL_EMBEDED_ATOMIC_COUNTER))
			{
				if (isVulkan) 
				{
					std::string uavname = ResourceName(psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
          const ResourceBinding* psBinding = NULL;
          psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, &psBinding);
          GLSLCrossDependencyDataInterface::VulkanResourceBinding uavBinding = psContext->psDependencies.GetVulkanResourceBinding(uavname, psContext->psShader.eShaderType, *psBinding, false, true);
          GLSLCrossDependencyDataInterface::VulkanResourceBinding counterBinding = std::make_pair(uavBinding.first, uavBinding.second+1);
					bformata(glsl, "layout(set = %d, binding = %d) buffer %s_counterBuf { highp uint %s_counter; };\n", counterBinding.first, counterBinding.second, uavname.c_str(), uavname.c_str());
				}
				else
				{
					bcatcstr(glsl, "layout (binding = 0) uniform ");

					if (HavePrecisionQualifers(psShader.eTargetLanguage))
						bcatcstr(glsl, "highp ");
					bcatcstr(glsl, "atomic_uint ");
					ResourceName(glsl, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
					bformata(glsl, "_counter; \n");
				}
			}

			DeclareBufferVariable(psContext, psDecl->asOperands[0].ui32RegisterNumber, &psDecl->asOperands[0],
				psDecl->sUAV.ui32GloballyCoherentAccess, 0, 1, psDecl->sUAV.bCounter, psDecl->ui32BufferStride, glsl);
			break;
		}
		case OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
		{
			const bool isVulkan = (psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS);
			if(psDecl->sUAV.bCounter && !(psContext->flags & HLSLCC_FLAG_GLSL_EMBEDED_ATOMIC_COUNTER))
			{
				if (isVulkan)
				{
					std::string uavname = ResourceName(psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
          const ResourceBinding* psBinding = NULL;
          psContext->psShader.sInfo.GetResourceFromBindingPoint(RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, &psBinding);
          GLSLCrossDependencyDataInterface::VulkanResourceBinding uavBinding = psContext->psDependencies.GetVulkanResourceBinding(uavname, psContext->psShader.eShaderType, *psBinding, false, true);
          GLSLCrossDependencyDataInterface::VulkanResourceBinding counterBinding = std::make_pair(uavBinding.first, uavBinding.second + 1);
					bformata(glsl, "layout(set = %d, binding = %d) buffer %s_counterBuf { highp uint %s_counter; };\n", counterBinding.first, counterBinding.second, uavname.c_str(), uavname.c_str());
				}
				else
				{
					bcatcstr(glsl, "layout (binding = 0) uniform ");
					if (HavePrecisionQualifers(psShader.eTargetLanguage))
						bcatcstr(glsl, "highp ");
					bcatcstr(glsl, "atomic_uint ");
					ResourceName(glsl, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
					bformata(glsl, "_counter; \n");
				}
			}

			DeclareBufferVariable(psContext, psDecl->asOperands[0].ui32RegisterNumber, &psDecl->asOperands[0],
				psDecl->sUAV.ui32GloballyCoherentAccess, 1, 1, psDecl->sUAV.bCounter, psDecl->ui32BufferStride, glsl);

			break;
		}
		case OPCODE_DCL_RESOURCE_STRUCTURED:
		{
			DeclareBufferVariable(psContext, psDecl->asOperands[0].ui32RegisterNumber, &psDecl->asOperands[0],
				psDecl->sUAV.ui32GloballyCoherentAccess, 0, 0, psDecl->sUAV.bCounter, psDecl->ui32BufferStride, glsl);
			break;
		}
		case OPCODE_DCL_RESOURCE_RAW:
		{
			DeclareBufferVariable(psContext, psDecl->asOperands[0].ui32RegisterNumber, &psDecl->asOperands[0],
				psDecl->sUAV.ui32GloballyCoherentAccess, 1, 0, psDecl->sUAV.bCounter, psDecl->ui32BufferStride, glsl);
			break;
		}
		case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
		{
			ShaderVarType* psVarType = &psShader.sInfo.sGroupSharedVarType[psDecl->asOperands[0].ui32RegisterNumber];

			bcatcstr(glsl, "shared struct {\n");
				bformata(glsl, "\tuint value[%d];\n", psDecl->sTGSM.ui32Stride/4);
			bcatcstr(glsl, "} ");
			TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NONE);
			bformata(glsl, "[%d];\n",
				psDecl->sTGSM.ui32Count);
			psVarType->name = "value";

			psVarType->Columns = psDecl->sTGSM.ui32Stride/4;
			psVarType->Elements = psDecl->sTGSM.ui32Count;
			break;
		}
		case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
		{
			ShaderVarType* psVarType = &psShader.sInfo.sGroupSharedVarType[psDecl->asOperands[0].ui32RegisterNumber];

			bcatcstr(glsl, "shared uint ");
			TranslateOperand(&psDecl->asOperands[0], TO_FLAG_NONE);
			bformata(glsl, "[%d];\n", psDecl->sTGSM.ui32Count / psDecl->sTGSM.ui32Stride);

			psVarType->name = "$Element";

			psVarType->Columns = 1;
			psVarType->Elements = psDecl->sTGSM.ui32Count / psDecl->sTGSM.ui32Stride;
			break;
		}
		case OPCODE_DCL_STREAM:
		{
			ASSERT(psDecl->asOperands[0].eType == OPERAND_TYPE_STREAM);


			if (psShader.eTargetLanguage >= LANG_400 && (psShader.ui32CurrentVertexOutputStream != psDecl->asOperands[0].ui32RegisterNumber))
			{
				// Only emit stream declaration for desktop GL >= 4.0, and only if we're declaring something else than the default 0
				bformata(glsl, "layout(stream = %d) out;\n", psShader.ui32CurrentVertexOutputStream);
			}
			psShader.ui32CurrentVertexOutputStream = psDecl->asOperands[0].ui32RegisterNumber;

			break;
		}
		case OPCODE_DCL_GS_INSTANCE_COUNT:
		{
			bformata(glsl, "layout(invocations = %d) in;\n", psDecl->value.ui32GSInstanceCount);
			break;
		}
		default:
		{
			ASSERT(0);
			break;
		}
	}
}

bool ToGLSL::TranslateSystemValue(const Operand * /*psOperand*/,
                                  const ShaderInfo::InOutSignature *sig,
                                  std::string &result,
                                  uint32_t *pui32IgnoreSwizzle,
                                  bool isIndexed,
                                  bool isInput,
                                  bool * /*outSkipPrefix*/)
{
  if (!sig)
    return false;

  if (psContext->psShader.eShaderType == HULL_SHADER && sig->semanticName == "SV_TessFactor")
  {
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    ASSERT(sig->ui32SemanticIndex <= 3);
    std::ostringstream oss;
    oss << "gl_TessLevelOuter[" << sig->ui32SemanticIndex << "]";
    result = oss.str();
    return true;
  }

  if (psContext->psShader.eShaderType == HULL_SHADER && sig->semanticName == "SV_InsideTessFactor")
  {
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    ASSERT(sig->ui32SemanticIndex <= 1);
    std::ostringstream oss;
    oss << "gl_TessLevelInner[" << sig->ui32SemanticIndex << "]";
    result = oss.str();
    return true;
  }

  switch (sig->eSystemValueType)
  {
  case NAME_POSITION:
    if (psContext->psShader.eShaderType == PIXEL_SHADER)
      result = "gl_FragCoord";
    else
      result = "gl_Position";
    return true;
  case NAME_RENDER_TARGET_ARRAY_INDEX:
    result = "gl_Layer";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  case NAME_CLIP_DISTANCE:
  {
    // This is always routed through temp
    std::ostringstream oss;
    oss << "phase" << psContext->currentPhase << "_glClipDistance" << sig->ui32SemanticIndex;
    result = oss.str();
    return true;
  }
  case NAME_VIEWPORT_ARRAY_INDEX:
    result = "gl_ViewportIndex";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  case NAME_VERTEX_ID:
    if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0)
      result = "gl_VertexIndex";
    else
      result = "gl_VertexID";
    //if (pui32IgnoreSwizzle)
      //*pui32IgnoreSwizzle = 1;
    return true;
  case NAME_INSTANCE_ID:
    if ((psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS) != 0)
      result = "gl_InstanceIndex";
    else
      result = "gl_InstanceID";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  case NAME_IS_FRONT_FACE:
    result = "gl_FrontFacing";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  case NAME_PRIMITIVE_ID:
    if (isInput && psContext->psShader.eShaderType == GEOMETRY_SHADER)
      result = "gl_PrimitiveIDIn"; // LOL opengl
    else
      result = "gl_PrimitiveID";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  case NAME_SAMPLE_INDEX:
    result = "gl_SampleID";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  case NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR:
  case NAME_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR:
  case NAME_FINAL_LINE_DENSITY_TESSFACTOR:
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    if (isIndexed)
    {
      result = "gl_TessLevelOuter";
      return true;
    }
    else
    {
      result = "gl_TessLevelOuter[0]";
      return true;
    }
  case NAME_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR:
  case NAME_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR:
  case NAME_FINAL_LINE_DETAIL_TESSFACTOR:
    result = "gl_TessLevelOuter[1]";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  case NAME_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR:
  case NAME_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR:
    result = "gl_TessLevelOuter[2]";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  case NAME_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR:
    result = "gl_TessLevelOuter[3]";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;

  case NAME_FINAL_TRI_INSIDE_TESSFACTOR:
  case NAME_FINAL_QUAD_U_INSIDE_TESSFACTOR:
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    if (isIndexed)
    {
      result = "gl_TessLevelInner";
      return true;
    }
    else
    {
      result = "gl_TessLevelInner[0]";
      return true;
    }
  case NAME_FINAL_QUAD_V_INSIDE_TESSFACTOR:
    result = "gl_TessLevelInner[3]";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  default:
    break;
  }

  if (psContext->psShader.asPhases[psContext->currentPhase].ePhase == HS_CTRL_POINT_PHASE)
  {
    if ((sig->semanticName == "POS" || sig->semanticName == "POSITION" || sig->semanticName == "SV_POSITION" || sig->semanticName == "SV_Position")
          && sig->ui32SemanticIndex == 0)
    {
      result = "gl_out[gl_InvocationID].gl_Position";
      return true;
    }
    std::ostringstream oss;
    if (isInput)
      oss << psContext->inputPrefix << sig->semanticName << sig->ui32SemanticIndex;
    else
      oss << psContext->outputPrefix << sig->semanticName << sig->ui32SemanticIndex << "[gl_InvocationID]";
    result = oss.str();
    return true;
  }

  // TODO: Add other builtins here.
  if (sig->eSystemValueType == NAME_POSITION || (sig->semanticName == "POS" && sig->ui32SemanticIndex == 0 && psContext->psShader.eShaderType == VERTEX_SHADER))
  {
    result = "gl_Position";
    return true;
  }

  if (sig->semanticName == "PSIZE")
  {
    result = "gl_PointSize";
    if (pui32IgnoreSwizzle)
      *pui32IgnoreSwizzle = 1;
    return true;
  }

  return false;
}

