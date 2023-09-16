
#include "internal_includes/HLSLCrossCompilerContext.h"
#include "internal_includes/HLSLccToolkit.h"
#include "internal_includes/Shader.h"
#include "internal_includes/DataTypeAnalysis.h"
#include "internal_includes/Declaration.h"
#include "internal_includes/debug.h"
#include "internal_includes/Translator.h"
#include "internal_includes/ControlFlowGraph.h"
#include <sstream>

using namespace std;

void HLSLCrossCompilerContext::DoDataTypeAnalysis(ShaderPhase& phase)
{
  phase.psTempDeclaration = find_if(phase.psDecl.begin(), phase.psDecl.end(), [](const Declaration& decl) {return decl.eOpcode == OPCODE_DCL_TEMPS; });
  if (phase.psTempDeclaration == phase.psDecl.end())
    return;

  phase.ui32TotalTemps = phase.psTempDeclaration->value.ui32NumTemps;

  if (phase.ui32TotalTemps == 0)
    return;

  phase.ui32OrigTemps = phase.ui32TotalTemps;

  // The split table is a table containing the index of the original register this register was split out from, or 0xffffffff
  // Format: lowest 16 bits: original register. bits 16-23: rebase (eg value of 1 means .yzw was changed to .xyz): bits 24-31: component count
  phase.pui32SplitInfo.resize(phase.ui32TotalTemps * 2);

  // Build use-define chains and split temps based on those.
  {
    DefineUseChains duChains;
    UseDefineChains udChains;

    BuildUseDefineChains(phase.ui32TotalTemps, duChains, udChains, phase.GetCFG());

    CalculateStandaloneDefinitions(duChains, phase.ui32TotalTemps);

    // Only do sampler precision downgrade on pixel shaders.
    if (psShader.eShaderType == PIXEL_SHADER)
      UpdateSamplerPrecisions(psShader.sInfo, duChains, phase.ui32TotalTemps);

    UDSplitTemps(&phase.ui32TotalTemps, duChains, udChains, phase.pui32SplitInfo);

    WriteBackUsesAndDefines(duChains);
  }

  phase.peTempTypes = HLSLcc::DataTypeAnalysis::SetDataTypes(this, phase.psInst, phase.ui32TotalTemps);

  if (phase.ui32OrigTemps != phase.ui32TotalTemps)
    phase.psTempDeclaration->value.ui32NumTemps = phase.ui32TotalTemps;
}

void HLSLCrossCompilerContext::ClearDependencyData()
{
  switch (psShader.eShaderType)
  {
  case PIXEL_SHADER:
    psDependencies.ClearCrossDependencyData();
    break;
  case HULL_SHADER:
    psDependencies.setTessPartitioning(TESSELLATOR_PARTITIONING_UNDEFINED);
    psDependencies.setTessOutPrimitive(TESSELLATOR_OUTPUT_UNDEFINED);
    break;
  default:
    break;
  }
}

void HLSLCrossCompilerContext::AddIndentation()
{
  for (int i = 0; i < indent; ++i)
    bcatcstr(*currentGLSLString, "    ");
}

void HLSLCrossCompilerContext::beginBlock()
{
  bcatcstr(*currentGLSLString, "{\n");
  ++indent;
  AddIndentation();
}

void HLSLCrossCompilerContext::endBlock()
{
  bcatcstr(*currentGLSLString, "}\n");
  --indent;
  AddIndentation();
}


std::string HLSLCrossCompilerContext::GetDeclaredInputName(const Operand& psOperand, int *piRebase, bool iIgnoreRedirect, uint32_t *puiIgnoreSwizzle) const
{
  std::ostringstream oss;
  RegisterSpace regSpace = psOperand.GetRegisterSpace(psShader.eShaderType, psShader.asPhases[currentPhase].ePhase);

  if (iIgnoreRedirect == false)
  {
    if ((regSpace == RegisterSpace::per_vertex && psShader.asPhases[currentPhase].acInputNeedsRedirect[psOperand.ui32RegisterNumber] == 0xfe)
      ||
      (regSpace == RegisterSpace::per_patch && psShader.asPhases[currentPhase].acPatchConstantsNeedsRedirect[psOperand.ui32RegisterNumber] == 0xfe))
    {
      oss << "phase" << currentPhase << "_Input" << (int)regSpace << "_" << psOperand.ui32RegisterNumber;
      if (piRebase)
        *piRebase = 0;
      return oss.str();
    }
  }

  const ShaderInfo::InOutSignature* psIn = nullptr;
  if (regSpace == RegisterSpace::per_vertex)
    psShader.sInfo.GetInputSignatureFromRegister(psOperand.ui32RegisterNumber, psOperand.GetAccessMask(), &psIn, true);
  else
    psShader.sInfo.GetPatchConstantSignatureFromRegister(psOperand.ui32RegisterNumber, psOperand.GetAccessMask(), &psIn, true);

  if (psIn && piRebase)
    *piRebase = psIn->iRebase;

  std::string res = "";
  bool skipPrefix = false;
  if (psTranslator->TranslateSystemValue(&psOperand, psIn, res, puiIgnoreSwizzle, psShader.aIndexedInput[(int)regSpace][psOperand.ui32RegisterNumber] != 0, true, &skipPrefix))
  {
    if (psShader.eTargetLanguage == LANG_METAL && (iIgnoreRedirect == false) && !skipPrefix)
      return inputPrefix + res;
    else
      return res;
  }

  ASSERT(psIn != NULL);
  oss << inputPrefix << (regSpace == RegisterSpace::per_patch ? "patch" : "") << psIn->semanticName << psIn->ui32SemanticIndex;
  return oss.str();
}


std::string HLSLCrossCompilerContext::GetDeclaredOutputName(const Operand& psOperand,
                                                            int* /*piStream*/,
                                                            uint32_t *puiIgnoreSwizzle,
                                                            int *piRebase,
                                                            bool iIgnoreRedirect) const
{
  std::ostringstream oss;
  RegisterSpace regSpace = psOperand.GetRegisterSpace(psShader.eShaderType, psShader.asPhases[currentPhase].ePhase);

  if (iIgnoreRedirect == false && psOperand.ui32RegisterNumber != -1)
  {
    if ((regSpace == RegisterSpace::per_vertex && psShader.asPhases[currentPhase].acOutputNeedsRedirect[psOperand.ui32RegisterNumber] == 0xfe)
        || (regSpace == RegisterSpace::per_patch && psShader.asPhases[currentPhase].acPatchConstantsNeedsRedirect[psOperand.ui32RegisterNumber] == 0xfe))
    {
      oss << "phase" << currentPhase << "_Output" << (int)regSpace << "_" << psOperand.ui32RegisterNumber;
      if (piRebase)
        *piRebase = 0;
      return oss.str();
    }
  }

  const ShaderInfo::InOutSignature* psOut = nullptr;
  if (regSpace == RegisterSpace::per_vertex)
    psShader.sInfo.GetOutputSignatureFromRegister(psOperand.ui32RegisterNumber, psOperand.GetAccessMask(), psShader.ui32CurrentVertexOutputStream, &psOut, true);
  else
    psShader.sInfo.GetPatchConstantSignatureFromRegister(psOperand.ui32RegisterNumber, psOperand.GetAccessMask(), &psOut, true);


  if (psOut && piRebase)
    *piRebase = psOut->iRebase;

  if (psOut && (psOut->isIndexed.find(currentPhase) != psOut->isIndexed.end()))
  {
    // Need to route through temp output variable
    oss << "phase" << currentPhase << "_Output" << (int)regSpace << "_" << psOut->indexStart.find(currentPhase)->second;
    if (!psOperand.m_SubOperands[0].get())
    {
      oss << "[" << psOperand.ui32RegisterNumber << "]";
    }
    if (piRebase)
      *piRebase = 0;
    return oss.str();
  }

  std::string res = "";

  if (psOperand.ui32RegisterNumber == -1 && psShader.eTargetLanguage == LANG_METAL)
  {
    if (iIgnoreRedirect == false)
    {
      oss << outputPrefix << psOut->semanticName << psOut->ui32SemanticIndex;
    }
    else
    {
      oss << psOut->semanticName << psOut->ui32SemanticIndex;
    }

    return oss.str();
  }

  if (psOperand.ui32RegisterNumber != -1)
  {
    if (psTranslator->TranslateSystemValue(&psOperand, psOut, res, puiIgnoreSwizzle, psShader.aIndexedOutput[(int)regSpace][psOperand.ui32RegisterNumber], false))
    {
      if (psShader.eTargetLanguage == LANG_METAL && (iIgnoreRedirect == false))
        return outputPrefix + res;
      else
        return res;
    }
  }

  ASSERT(psOut != NULL);

  oss << outputPrefix << (regSpace == RegisterSpace::per_patch ? "patch" : "") << psOut->semanticName << psOut->ui32SemanticIndex;
  return oss.str();
}

bool HLSLCrossCompilerContext::OutputNeedsDeclaring(const Operand* psOperand, const int count)
{
  char compMask = (char)psOperand->ui32CompMask;
  RegisterSpace regSpace = psOperand->GetRegisterSpace(psShader.eShaderType, psShader.asPhases[currentPhase].ePhase);
  uint32_t startIndex = psOperand->ui32RegisterNumber + (psShader.ui32CurrentVertexOutputStream * 1024); // Assume less than 1K input streams
  ASSERT(psShader.ui32CurrentVertexOutputStream < 4);

  // First check for various builtins, mostly depth-output ones.
  if (psShader.eShaderType == PIXEL_SHADER)
  {
    if (psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL ||
        psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL)
    {
      return true;
    }

    if (psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH)
    {
      // GL doesn't need declaration, Metal does.
      return psShader.eTargetLanguage == LANG_METAL;
    }
  }

  // Needs declaring if any of the components hasn't been already declared
  if ((compMask & ~psShader.acOutputDeclared[(int)regSpace][startIndex]) != 0)
  {
    const ShaderInfo::InOutSignature* psSignature = NULL;

    if (psOperand->eSpecialName == NAME_UNDEFINED)
    {
      // Need to fetch the actual comp mask
      if (regSpace == RegisterSpace::per_vertex)
        psShader.sInfo.GetOutputSignatureFromRegister(
          psOperand->ui32RegisterNumber,
          psOperand->ui32CompMask,
          psShader.ui32CurrentVertexOutputStream,
          &psSignature);
      else
        psShader.sInfo.GetPatchConstantSignatureFromRegister(
          psOperand->ui32RegisterNumber,
          psOperand->ui32CompMask,
          &psSignature);

      compMask = (char)psSignature->ui32Mask;
    }
    for (int offset = 0; offset < count; offset++)
    {
      psShader.acOutputDeclared[(int)regSpace][startIndex + offset] |= compMask;
    }
    return true;
  }

  return false;
}