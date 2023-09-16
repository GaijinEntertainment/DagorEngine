
#include "internal_includes/toMetal.h"
#include "internal_includes/HLSLCrossCompilerContext.h"
#include "internal_includes/Shader.h"
#include "internal_includes/debug.h"

#include "internal_includes/Declaration.h"
#include "internal_includes/toGLSL.h"
#include "internal_includes/LoopTransform.h"
#include "internal_includes/HLSLccToolkit.h"
#include <algorithm>

static void PrintStructDeclaration(HLSLCrossCompilerContext *psContext, bstring glsl, std::string &sname, StructDefinitions &defs)
{
	StructDefinition &d = defs[sname];
	if (d.m_IsPrinted)
		return;
	d.m_IsPrinted = true;


	std::for_each(d.m_Dependencies.begin(), d.m_Dependencies.end(), [&psContext, &glsl, &defs](std::string &depName)
	{
		PrintStructDeclaration(psContext, glsl, depName, defs);
	});

	bformata(glsl, "struct %s\n{\n", sname.c_str());
	psContext->indent++;
	std::for_each(d.m_Members.begin(), d.m_Members.end(), [&psContext, &glsl](std::string &mem)
	{
		psContext->AddIndentation();
		bcatcstr(glsl, mem.c_str());
		bcatcstr(glsl, ";\n");
	});

	psContext->indent--;
	bcatcstr(glsl, "};\n\n");
}

void ToMetal::PrintStructDeclarations(StructDefinitions &defs)
{
	bstring glsl = *psContext->currentGLSLString;
	StructDefinition &args = defs[""];
	std::for_each(args.m_Dependencies.begin(), args.m_Dependencies.end(), [this, glsl, &defs](std::string &sname)
	{
		PrintStructDeclaration(psContext, glsl, sname, defs);
	});

}

bool ToMetal::Translate()
{
	bstring glsl;
	uint32_t i;
	Shader& psShader = psContext->psShader;
	psContext->psTranslator = this;

	SetIOPrefixes();
	psShader.ExpandSWAPCs();
	psShader.ForcePositionToHighp();
	psShader.AnalyzeIOOverlap();
	psShader.FindUnusedGlobals(psContext->flags);

	psContext->indent = 0;

	glsl = bfromcstralloc(1024 * 10, "");
	bstring bodyglsl = bfromcstralloc(1024 * 10, "");

	psContext->glsl = glsl;
	for (i = 0; i < psShader.asPhases.size(); ++i)
	{
		psShader.asPhases[i].postShaderCode = bfromcstralloc(1024 * 5, "");
		psShader.asPhases[i].earlyMain = bfromcstralloc(1024 * 5, "");
	}

	psContext->currentGLSLString = &glsl;
	psShader.eTargetLanguage = LANG_METAL;
	psShader.extensions = NULL;
	psContext->currentPhase = MAIN_PHASE;

	psContext->ClearDependencyData();

	ClampPartialPrecisions();

	psShader.PrepareStructuredBufferBindingSlots();

	ShaderPhase &phase = psShader.asPhases[0];
	phase.UnvectorizeImmMoves();
	psContext->DoDataTypeAnalysis(phase);
	phase.ResolveUAVProperties();
	psShader.ResolveStructuredBufferBindingSlots(&phase);
	phase.PruneConstArrays();
	HLSLcc::DoLoopTransform(phase);

	psShader.PruneTempRegisters();

	bcatcstr(glsl, "#include <metal_stdlib>\n#include <metal_texture>\nusing namespace metal;\n");


	for (i = 0; i < psShader.asPhases[0].psDecl.size(); ++i)
	{
		TranslateDeclaration(&psShader.asPhases[0].psDecl[i]);
	}

	if (m_StructDefinitions[GetInputStructName()].m_Members.size() > 0)
	{
		m_StructDefinitions[""].m_Members.push_back(GetInputStructName() + " input [[ stage_in ]]");
		m_StructDefinitions[""].m_Dependencies.push_back(GetInputStructName());
	}

	if (psShader.eShaderType != COMPUTE_SHADER)
	{
		if (m_StructDefinitions[GetOutputStructName()].m_Members.size() > 0)
		{
			m_StructDefinitions[""].m_Dependencies.push_back(GetOutputStructName());
		}
	}

	PrintStructDeclarations(m_StructDefinitions);

  bool is_void = false;

  if (psShader.eShaderType == PIXEL_SHADER)
  {
    std::string sname("Mtl_FragmentOut");
    StructDefinition &args = m_StructDefinitions[sname];

    if (!args.m_IsPrinted)
    {
      is_void = true;
    }
  }

	psContext->currentGLSLString = &bodyglsl;

	switch (psShader.eShaderType)
	{
		case VERTEX_SHADER:
			bcatcstr(bodyglsl, "vertex Mtl_VertexOut xlatMtlMain(\n");
			break;
		case PIXEL_SHADER:
      if (is_void)
      {
        bcatcstr(bodyglsl, "fragment void xlatMtlMain(\n");
      }
      else
      {
        bcatcstr(bodyglsl, "fragment Mtl_FragmentOut xlatMtlMain(\n");
      }
			break;
		case COMPUTE_SHADER:
			bcatcstr(bodyglsl, "kernel void computeMain(\n");
			break;
		default:
			// Not supported
			ASSERT(0);
			return false;
	}
	psContext->indent++;
	for (auto itr = m_StructDefinitions[""].m_Members.begin(); itr != m_StructDefinitions[""].m_Members.end(); itr++)
	{
		psContext->AddIndentation();
		bcatcstr(bodyglsl, itr->c_str());
		if (itr + 1 != m_StructDefinitions[""].m_Members.end())
			bcatcstr(bodyglsl, ",\n");
	}

	bcatcstr(bodyglsl, ")\n{\n");
	if (psShader.eShaderType != COMPUTE_SHADER && !is_void)
	{
		psContext->AddIndentation();
		bcatcstr(bodyglsl, GetOutputStructName().c_str());
		bcatcstr(bodyglsl, " output;\n");
	}

	if (psContext->psShader.asPhases[0].earlyMain->slen > 1)
	{
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(bodyglsl, "//--- Start Early Main ---\n");
#endif
		bconcat(bodyglsl, psContext->psShader.asPhases[0].earlyMain);
#ifdef _DEBUG
		psContext->AddIndentation();
		bcatcstr(bodyglsl, "//--- End Early Main ---\n");
#endif
	}

	for (i = 0; i < psShader.asPhases[0].psInst.size(); ++i)
	{
		TranslateInstruction(&psShader.asPhases[0].psInst[i]);
	}

	psContext->indent--;

	bcatcstr(bodyglsl, "}\n");

  if (is_void)
  {
    const char* str = "return output;";
    int len = strlen(str);

    char* ptr = strstr((char*)bodyglsl->data, str);

    int offset = ptr - (char*)bodyglsl->data;

    int len2 = strlen(ptr) - len;

    int l = 0;
    for (l = 0; l < len2; l++)
    {
      bodyglsl->data[offset + l] = bodyglsl->data[offset + l + len];
    }

    bodyglsl->data[offset + l] = 0;
    bodyglsl->slen -= len;
  }

	psContext->currentGLSLString = &glsl;
	
	bcatcstr(glsl, m_ExtraGlobalDefinitions.c_str());
	
	// Print out extra functions we generated
	std::for_each(m_FunctionDefinitions.begin(), m_FunctionDefinitions.end(), [&glsl](const FunctionDefinitions::value_type &p)
	{
		bcatcstr(glsl, p.second.c_str());
		bcatcstr(glsl, "\n");
	});

	// And then the actual function body
	bconcat(glsl, bodyglsl);
	bdestroy(bodyglsl);

	return true;
}

void ToMetal::DeclareExtraFunction(const std::string &name, const std::string &body)
{
	if (m_FunctionDefinitions.find(name) != m_FunctionDefinitions.end())
		return;
	m_FunctionDefinitions.insert(std::make_pair(name, body));
}


std::string ToMetal::GetOutputStructName() const
{
	switch(psContext->psShader.eShaderType)
	{
		case VERTEX_SHADER:
			return "Mtl_VertexOut";
		case PIXEL_SHADER:
			return "Mtl_FragmentOut";
		default:
			ASSERT(0);
			return "";
	}
}

std::string ToMetal::GetInputStructName() const
{
	switch (psContext->psShader.eShaderType)
	{
		case VERTEX_SHADER:
			return "Mtl_VertexIn";
		case PIXEL_SHADER:
			return "Mtl_FragmentIn";
		case COMPUTE_SHADER:
			return "Mtl_KernelIn";
		default:
			ASSERT(0);
			return "";
	}
}

void ToMetal::SetIOPrefixes()
{
	switch (psContext->psShader.eShaderType)
	{
		case VERTEX_SHADER:
			psContext->inputPrefix = "input.";
			psContext->outputPrefix = "output.";
			break;

		case PIXEL_SHADER:
			psContext->inputPrefix = "input.";
			psContext->outputPrefix = "output.";
			break;

		case COMPUTE_SHADER:
			psContext->inputPrefix = "";
			psContext->outputPrefix = "";
			break;
		default:
			ASSERT(0);
			break;
	}
}

void ToMetal::ClampPartialPrecisions()
{
	HLSLcc::ForEachOperand(psContext->psShader.asPhases[0].psInst.begin(), psContext->psShader.asPhases[0].psInst.end(), FEO_FLAG_ALL,
		[](std::vector<Instruction>::iterator &i, Operand *o, uint32_t flags)
	{
		if (o->eMinPrecision == OPERAND_MIN_PRECISION_FLOAT_2_8)
			o->eMinPrecision = OPERAND_MIN_PRECISION_FLOAT_16;
	});
}
