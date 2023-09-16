#include <memory>

#include "internal_includes/tokens.h"
#include "internal_includes/decode.h"
#include "stdlib.h"
#include "stdio.h"
#include "bstrlib.h"
#include "internal_includes/toGLSL.h"
#include "internal_includes/toGLSLOperand.h"
#include "internal_includes/Declaration.h"
#include "internal_includes/languages.h"
#include "internal_includes/debug.h"
#include "internal_includes/HLSLccToolkit.h"
#include "internal_includes/UseDefineChains.h"
#include "internal_includes/DataTypeAnalysis.h"
#include "internal_includes/Shader.h"
#include "internal_includes/HLSLCrossCompilerContext.h"
#include "internal_includes/Instruction.h"
#include "internal_includes/LoopTransform.h"
#include <algorithm>
#include <sstream>

// In GLSL, the input and output names cannot clash.
// Also, the output name of previous stage must match the input name of the next stage.
// So, do gymnastics depending on which shader we're running on and which other shaders exist in this program.
//
void ToGLSL::SetIOPrefixes()
{
  switch (psContext->psShader.eShaderType)
  {
  case VERTEX_SHADER:
    psContext->inputPrefix = "in_";
    psContext->outputPrefix = "vs_";
    break;

  case HULL_SHADER:
    // Input always coming from vertex shader
    psContext->inputPrefix = "vs_";
    psContext->outputPrefix = "hs_";
    break;

  case DOMAIN_SHADER:
    // There's no domain shader without hull shader
    psContext->inputPrefix = "hs_";
    psContext->outputPrefix = "ds_";
    break;

  case GEOMETRY_SHADER:
    // The input depends on whether there's a tessellation shader before us
    if (psContext->psDependencies.getProgramStagesMask() & PS_FLAG_DOMAIN_SHADER)
      psContext->inputPrefix = "ds_";
    else
      psContext->inputPrefix = "vs_";

    psContext->outputPrefix = "gs_";
    break;

  case PIXEL_SHADER:
    // The inputs can come from geom shader, domain shader or directly from vertex shader
    if (psContext->psDependencies.getProgramStagesMask() & PS_FLAG_GEOMETRY_SHADER)
    {
      psContext->inputPrefix = "gs_";
    }
    else if (psContext->psDependencies.getProgramStagesMask() & PS_FLAG_DOMAIN_SHADER)
    {
      psContext->inputPrefix = "ds_";
    }
    else
    {
      psContext->inputPrefix = "vs_";
    }
    psContext->outputPrefix = "";
    break;


  case COMPUTE_SHADER:
  default:
    // No prefixes
    psContext->inputPrefix = "";
    psContext->outputPrefix = "";
    break;
  }
}


static void AddVersionDependentCode(HLSLCrossCompilerContext* psContext)
{
  bstring glsl = *psContext->currentGLSLString;
  bool isES = (psContext->psShader.eTargetLanguage >= LANG_ES_100 && psContext->psShader.eTargetLanguage <= LANG_ES_310);

  if (psContext->psShader.ui32MajorVersion > 3 && psContext->psShader.eTargetLanguage != LANG_ES_300 && psContext->psShader.eTargetLanguage != LANG_ES_310 && !(psContext->psShader.eTargetLanguage >= LANG_330))
  {
    psContext->RequireExtension("GL_ARB_shader_bit_encoding");
  }

  if (!HaveCompute(psContext->psShader.eTargetLanguage))
  {
    if (psContext->psShader.eShaderType == COMPUTE_SHADER)
    {
      psContext->RequireExtension("GL_ARB_compute_shader");
    }

    if (psContext->psShader.aiOpcodeUsed[OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_DCL_RESOURCE_STRUCTURED] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_DCL_RESOURCE_RAW])
    {
      psContext->RequireExtension("GL_ARB_shader_storage_buffer_object");
    }
  }

  if (!HaveAtomicMem(psContext->psShader.eTargetLanguage) ||
    !HaveAtomicCounter(psContext->psShader.eTargetLanguage))
  {
    if (psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_ALLOC] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_CONSUME] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED])
    {
      psContext->RequireExtension("GL_ARB_shader_atomic_counters");
    }
  }

  if (!HaveImageAtomics(psContext->psShader.eTargetLanguage))
  {
    if (psContext->psShader.aiOpcodeUsed[OPCODE_ATOMIC_CMP_STORE] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_AND] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_ATOMIC_AND] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_IADD] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_ATOMIC_IADD] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_ATOMIC_OR] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_ATOMIC_XOR] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_ATOMIC_IMIN] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_ATOMIC_UMIN] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_IMAX] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_IMIN] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_UMAX] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_UMIN] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_OR] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_XOR] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_EXCH] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_IMM_ATOMIC_CMP_EXCH])
    {
      if (isES)
        psContext->RequireExtension("GL_OES_shader_image_atomic");
      else
        psContext->RequireExtension("GL_ARB_shader_image_load_store");
    }
  }

  if (!HaveGather(psContext->psShader.eTargetLanguage))
  {
    if (psContext->psShader.aiOpcodeUsed[OPCODE_GATHER4] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_GATHER4_PO_C] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_GATHER4_PO] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_GATHER4_C])
    {
      psContext->RequireExtension("GL_ARB_texture_gather");
    }
  }

  if (!HaveGatherNonConstOffset(psContext->psShader.eTargetLanguage))
  {
    if (psContext->psShader.aiOpcodeUsed[OPCODE_GATHER4_PO_C] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_GATHER4_PO])
    {
      psContext->RequireExtension("GL_ARB_gpu_shader5");
    }
  }

  if (!HaveQueryLod(psContext->psShader.eTargetLanguage))
  {
    if (psContext->psShader.aiOpcodeUsed[OPCODE_LOD])
    {
      psContext->RequireExtension("GL_ARB_texture_query_lod");
    }
  }

  if (!HaveQueryLevels(psContext->psShader.eTargetLanguage))
  {
    if (psContext->psShader.aiOpcodeUsed[OPCODE_RESINFO])
    {
      psContext->RequireExtension("GL_ARB_texture_query_levels");
    }
  }

  if (!HaveImageLoadStore(psContext->psShader.eTargetLanguage))
  {
    if (psContext->psShader.aiOpcodeUsed[OPCODE_STORE_UAV_TYPED] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_STORE_RAW] ||
      psContext->psShader.aiOpcodeUsed[OPCODE_STORE_STRUCTURED])
    {
      psContext->RequireExtension("GL_ARB_shader_image_load_store");
      psContext->RequireExtension("GL_ARB_shader_bit_encoding");
    }
    else
      if (psContext->psShader.aiOpcodeUsed[OPCODE_LD_UAV_TYPED] ||
        psContext->psShader.aiOpcodeUsed[OPCODE_LD_RAW] ||
        psContext->psShader.aiOpcodeUsed[OPCODE_LD_STRUCTURED])
      {
        psContext->RequireExtension("GL_ARB_shader_image_load_store");
      }
  }

  if (!HaveGeometryShaderARB(psContext->psShader.eTargetLanguage))
  {
    if (psContext->psShader.eShaderType == GEOMETRY_SHADER)
    {
      psContext->RequireExtension("GL_ARB_geometry_shader");
    }
  }

  if (psContext->psShader.eTargetLanguage == LANG_ES_300 || psContext->psShader.eTargetLanguage == LANG_ES_310)
  {
    if (psContext->psShader.eShaderType == GEOMETRY_SHADER)
    {
      psContext->RequireExtension("GL_OES_geometry_shader");
      psContext->RequireExtension("GL_EXT_geometry_shader");
    }
  }

  if (psContext->psShader.eTargetLanguage == LANG_ES_300 || psContext->psShader.eTargetLanguage == LANG_ES_310)
  {
    if (psContext->psShader.eShaderType == HULL_SHADER || psContext->psShader.eShaderType == DOMAIN_SHADER)
    {
      psContext->RequireExtension("GL_OES_tessellation_shader");
      psContext->RequireExtension("GL_EXT_tessellation_shader");
    }
  }

  //Handle fragment shader default precision
  if ((psContext->psShader.eShaderType == PIXEL_SHADER) &&
    (psContext->psShader.eTargetLanguage == LANG_ES_100 || psContext->psShader.eTargetLanguage == LANG_ES_300 || psContext->psShader.eTargetLanguage == LANG_ES_310))
  {
    // Float default precision is patched during runtime in GlslGpuProgramGLES.cpp:PatchupFragmentShaderText()
    // Except on Vulkan
    if (psContext->flags & HLSLCC_FLAG_VULKAN_BINDINGS)
      bcatcstr(glsl, "precision highp float;\n");


    // Define default int precision to highp to avoid issues on platforms that actually implement mediump 
    bcatcstr(glsl, "precision highp int;\n");
  }

  if (psContext->psShader.eShaderType == PIXEL_SHADER && psContext->psShader.eTargetLanguage >= LANG_120 && !HaveFragmentCoordConventions(psContext->psShader.eTargetLanguage))
  {
    psContext->RequireExtension("GL_ARB_fragment_coord_conventions");
  }

  if (psContext->psShader.eShaderType == PIXEL_SHADER && psContext->psShader.eTargetLanguage >= LANG_150)
  {
    if (psContext->flags & HLSLCC_FLAG_ORIGIN_UPPER_LEFT)
      bcatcstr(glsl, "layout(origin_upper_left) in vec4 gl_FragCoord;\n");

    if (psContext->flags & HLSLCC_FLAG_PIXEL_CENTER_INTEGER)
      bcatcstr(glsl, "layout(pixel_center_integer) in vec4 gl_FragCoord;\n");
  }


  /*
      OpenGL 4.1 API spec:
      To use any built-in input or output in the gl_PerVertex block in separable
      program objects, shader code must redeclare that block prior to use.
  */
  /* DISABLED FOR NOW */
/*	if(psContext->psShader.eShaderType == VERTEX_SHADER && psContext->psShader.eTargetLanguage >= LANG_410)
    {
        bcatcstr(glsl, "out gl_PerVertex {\n");
        bcatcstr(glsl, "vec4 gl_Position;\n");
        bcatcstr(glsl, "float gl_PointSize;\n");
        bcatcstr(glsl, "float gl_ClipDistance[];");
        bcatcstr(glsl, "};\n");
    }*/
}

static GLLang ChooseLanguage(Shader& psShader)
{
  // Depends on the HLSL shader model extracted from bytecode.
  switch (psShader.ui32MajorVersion)
  {
  case 5: return LANG_430;
  case 4: return LANG_330;
  default: return LANG_120;
  }
}

#define VERSION(num) case LANG_##num: return "#version "#num"\n"
#define VERSION_ES(num) case LANG_ES_##num: return "#version "#num" es\n"
static const char* GetVersionString(GLLang language)
{
  switch (language)
  {
  case LANG_ES_100: return "#version 100\n";  // special case, no es because core GL has no 100 GLSL
  VERSION_ES(300);
  VERSION_ES(310);
  VERSION(120);
  VERSION(130);
  VERSION(140);
  VERSION(150);
  VERSION(330);
  VERSION(410);
  VERSION(420);
  VERSION(430);
  VERSION(440);
  default:
    return "";
  }
}
#undef VERSION_ES
#undef VERSION

static const char * GetPhaseFuncName(SHADER_PHASE_TYPE eType)
{
  switch (eType)
  {
  default:
  case MAIN_PHASE: return "";
  case HS_GLOBAL_DECL_PHASE: return "hs_global_decls";
  case HS_FORK_PHASE: return "fork_phase";
  case HS_CTRL_POINT_PHASE: return "control_point_phase";
  case HS_JOIN_PHASE: return "join_phase";
  }
}

static void DoHullShaderPassthrough(HLSLCrossCompilerContext *psContext)
{
  uint32_t i;
  bstring glsl = psContext->glsl;

  for (i = 0; i < psContext->psShader.sInfo.psInputSignatures.size(); i++)
  {
    ShaderInfo::InOutSignature *psSig = &psContext->psShader.sInfo.psInputSignatures[i];
    const char *Type;
    uint32_t ui32NumComponents = HLSLcc::GetNumberBitsSet(psSig->ui32Mask);
    switch (psSig->eComponentType)
    {
    default:
    case INOUT_COMPONENT_FLOAT32:
      Type = ui32NumComponents > 1 ? "vec" : "float";
      break;
    case INOUT_COMPONENT_SINT32:
      Type = ui32NumComponents > 1 ? "ivec" : "int";
      break;
    case INOUT_COMPONENT_UINT32:
      Type = ui32NumComponents > 1 ? "uvec" : "uint";
      break;
    }
    if ((psSig->eSystemValueType == NAME_POSITION || psSig->semanticName == "POS") && psSig->ui32SemanticIndex == 0)
      continue;

    std::string inputName;

    {
      std::ostringstream oss;
      oss << psContext->inputPrefix << psSig->semanticName << psSig->ui32SemanticIndex;
      inputName = oss.str();
    }

    std::string outputName;
    {
      std::ostringstream oss;
      oss << psContext->outputPrefix << psSig->semanticName << psSig->ui32SemanticIndex;
      outputName = oss.str();
    }

    const char * prec = "";
    if (HavePrecisionQualifers(psContext->psShader.eTargetLanguage))
    {
     if (psSig->eMinPrec != MIN_PRECISION_DEFAULT)
        prec = "mediump ";
     else
        prec = "highp ";
    }

    int inLoc = psContext->psDependencies.GetVaryingLocation(inputName, HULL_SHADER, true);
    int outLoc = psContext->psDependencies.GetVaryingLocation(outputName, HULL_SHADER, false);

    psContext->AddIndentation();
    if (ui32NumComponents > 1)
      bformata(glsl, "layout(location = %d) in %s%s%d %s%s%d[];\n", inLoc, prec, Type, ui32NumComponents, psContext->inputPrefix, psSig->semanticName.c_str(), psSig->ui32SemanticIndex);
    else
      bformata(glsl, "layout(location = %d) in %s%s %s%s%d[];\n", inLoc, prec, Type, psContext->inputPrefix, psSig->semanticName.c_str(), psSig->ui32SemanticIndex);

    psContext->AddIndentation();
    if (ui32NumComponents > 1)
      bformata(glsl, "layout(location = %d) out %s%s%d %s%s%d[];\n", outLoc, prec, Type, ui32NumComponents, psContext->outputPrefix, psSig->semanticName.c_str(), psSig->ui32SemanticIndex);
    else
      bformata(glsl, "layout(location = %d) out %s%s %s%s%d[];\n", outLoc, prec, Type, psContext->outputPrefix, psSig->semanticName.c_str(), psSig->ui32SemanticIndex);
  }

  psContext->AddIndentation();
  bcatcstr(glsl, "void passthrough_ctrl_points()\n");
  psContext->AddIndentation();
  bcatcstr(glsl, "{\n");
  psContext->indent++;

  for (i = 0; i < psContext->psShader.sInfo.psInputSignatures.size(); i++)
  {
    const ShaderInfo::InOutSignature *psSig = &psContext->psShader.sInfo.psInputSignatures[i];

    psContext->AddIndentation();

    if ((psSig->eSystemValueType == NAME_POSITION || psSig->semanticName == "POS") && psSig->ui32SemanticIndex == 0)
      bformata(glsl, "gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n");
    else
      bformata(glsl, "%s%s%d[gl_InvocationID] = %s%s%d[gl_InvocationID];\n", psContext->outputPrefix, psSig->semanticName.c_str(), psSig->ui32SemanticIndex, psContext->inputPrefix, psSig->semanticName.c_str(), psSig->ui32SemanticIndex);
  }

  psContext->indent--;
  psContext->AddIndentation();
  bcatcstr(glsl, "}\n");
}

GLLang ToGLSL::SetLanguage(GLLang suggestedLanguage)
{
  if (suggestedLanguage == LANG_DEFAULT)
    language = ChooseLanguage(psContext->psShader);
  else
    language = suggestedLanguage;
  return language;
}

static void fixConstantBufferDeclaration(HLSLCrossCompilerContext *psContext, Declaration* psDecl)
{
  // only constant buffers need special preparation
  if (psDecl->eOpcode != OPCODE_DCL_CONSTANT_BUFFER)
    return;

  const Operand* psOperand = &psDecl->asOperands[0];
  const uint32_t ui32BindingPoint = psOperand->aui32ArraySizes[0];

  const ConstantBuffer* psCBuf = NULL;
  psContext->psShader.sInfo.GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, ui32BindingPoint, &psCBuf);

  // need to modify it
  // TODO: add entry point to get non cost version
  ConstantBuffer* psBuf = const_cast<ConstantBuffer*>(psCBuf);

  // propaget offset into type
  //for (uint32_t i = 0; i < psBuf->asVars.size(); ++i)
  //  psBuf->asVars[i].sType.Offset = psBuf->asVars[i].ui32StartOffset;

  std::sort(psBuf->asVars.begin(),
            psBuf->asVars.end(),
            [] (const ShaderVar& l, const ShaderVar& r)
  {
    return l.ui32StartOffset < r.ui32StartOffset;
  });
}

static uint32_t scalarTypeSize(SHADER_VARIABLE_TYPE type)
{
  switch (type)
  {
  case SVT_VOID:
    return 0;
  case SVT_BOOL:
  case SVT_INT:
  case SVT_FLOAT:
    return 4;
  case SVT_STRING:
    return -1;
  case SVT_TEXTURE:
  case SVT_TEXTURE1D:
  case SVT_TEXTURE2D:
  case SVT_TEXTURE3D:
  case SVT_TEXTURECUBE:
  case SVT_SAMPLER:
  case SVT_PIXELSHADER:
  case SVT_VERTEXSHADER:
    return -1;
  case SVT_UINT:
    return 4;
  case SVT_UINT8:
    return 1;
  case SVT_GEOMETRYSHADER:
  case SVT_RASTERIZER:
  case SVT_DEPTHSTENCIL:
  case SVT_BLEND:
  case SVT_BUFFER:
  case SVT_CBUFFER:
  case SVT_TBUFFER:
  case SVT_TEXTURE1DARRAY:
  case SVT_TEXTURE2DARRAY:
  case SVT_RENDERTARGETVIEW:
  case SVT_DEPTHSTENCILVIEW:
  case SVT_TEXTURE2DMS:
  case SVT_TEXTURE2DMSARRAY:
  case SVT_TEXTURECUBEARRAY:
  case SVT_HULLSHADER:
  case SVT_DOMAINSHADER:
  case SVT_INTERFACE_POINTER:
  case SVT_COMPUTESHADER:
    return -1;
  case SVT_DOUBLE:
    return 8;
  case SVT_RWTEXTURE1D:
  case SVT_RWTEXTURE1DARRAY:
  case SVT_RWTEXTURE2D:
  case SVT_RWTEXTURE2DARRAY:
  case SVT_RWTEXTURE3D:
  case SVT_RWBUFFER:
  case SVT_BYTEADDRESS_BUFFER:
  case SVT_RWBYTEADDRESS_BUFFER:
  case SVT_STRUCTURED_BUFFER:
  case SVT_RWSTRUCTURED_BUFFER:
  case SVT_APPEND_STRUCTURED_BUFFER:
  case SVT_CONSUME_STRUCTURED_BUFFER:
    return -1;
  case SVT_FORCED_INT:
  case SVT_INT_AMBIGUOUS:
    return 4;
  case SVT_FLOAT10:
    return -1;
  case SVT_FLOAT16:
  case SVT_INT16:
    return 2;
  case SVT_INT12:
    return -1;
  case SVT_UINT16:
    return 2;
  }
  return -1;
}

// smalest unit is float/int of 32 bits -> 4 byte
static uint32_t basicMachineUnitSize = 4;

static uint32_t roundToNextAligned(uint32_t size, uint32_t align)
{
  return (size + align - 1) & ~(align - 1);
}

static uint32_t roundToNextMachineUnit(uint32_t size)
{
  return roundToNextAligned(size, basicMachineUnitSize);
}

static uint32_t roundToNextVec4(uint32_t size)
{
  return roundToNextAligned(size, basicMachineUnitSize * 4);
}


// returns size, align, stride, rule was used
typedef std::tuple<uint32_t, uint32_t, uint32_t, bool>(*RuleFunction_t)(const ShaderVarType& type);

static std::tuple<uint32_t, uint32_t, uint32_t, bool> applyAllRules140(const ShaderVarType& type);
static std::tuple<uint32_t, uint32_t, uint32_t, bool> applyAllRules430(const ShaderVarType& type);


// If the member is a scalar consuming N basic machine units, the base alignment
// is N.
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule1(const ShaderVarType& type)
{
  if (type.Class == SVC_SCALAR && type.Elements < 1)
  {
    uint32_t size = scalarTypeSize(type.Type);
    uint32_t align = roundToNextMachineUnit(size);
    uint32_t stride = 0;
    return std::make_tuple(size, align, stride, true);
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is a two- or four-component vector with components consuming
// N basic machine units, the base alignment is 2N or 4N, respectively
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule2(const ShaderVarType& type)
{
  if (type.Class == SVC_VECTOR && type.Elements < 1)
  {
    if (type.Columns == 2 || type.Columns == 4)
    {
      uint32_t size = scalarTypeSize(type.Type) * type.Columns;
      uint32_t align = roundToNextMachineUnit(size);
      uint32_t stride = 0;
      return std::make_tuple(size, align, stride, true);
    }
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is a three - component vector with components consuming N
// basic machine units, the base alignment is 4N
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule3(const ShaderVarType& type)
{
  if (type.Class == SVC_VECTOR && type.Elements < 1)
  {
    if (type.Columns == 3)
    {
      uint32_t size = scalarTypeSize(type.Type) * 3;
      uint32_t align = roundToNextMachineUnit(scalarTypeSize(type.Type) * 4);
      uint32_t stride = 0;
      return std::make_tuple(size, align, stride, true);
    }
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is an array of scalars or vectors, the base alignment and array
// stride are set to match the base alignment of a single array element, according
// to rules(1), (2), and (3), and rounded up to the base alignment of a vec4.The
// array may have padding at the end; the base offset of the member following
// the array is rounded up to the next multiple of the base alignment.
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule4(const ShaderVarType& type)
{
  if ((type.Class == SVC_VECTOR || type.Class == SVC_SCALAR) && type.Elements >= 1)
  {
    static const RuleFunction_t arraySubRules[]
    {
      std140Rule1,
      std140Rule2,
      std140Rule3
    };
    ShaderVarType local = type;
    local.Elements = 0;
    uint32_t size, align, stride;
    bool applied;
    for (auto& rule : arraySubRules)
    {
      std::tie(size, align, stride, applied) = rule(local);
      if (applied)
      {
        align = roundToNextVec4(align);
        size = roundToNextMachineUnit(size);
        stride = size;
        size = stride * type.Elements;
        return std::make_tuple(size, align, stride, true);
      }
    }
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is a column - major matrix with C columns and R rows, the
// matrix is stored identically to an array of C column vectors with R components
// each, according to rule(4)
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule5(const ShaderVarType& type)
{
  if (type.Class == SVC_MATRIX_COLUMNS && type.Elements < 1)
  {
    ShaderVarType local = type;
    local.Class = SVC_VECTOR;
    local.Columns = type.Rows;
    local.Elements = type.Columns;
    return std140Rule4(local);
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is an array of S column - major matrices with C columns and
// R rows, the matrix is stored identically to a row of S × C column vectors
// with R components each, according to rule(4).
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule6(const ShaderVarType& type)
{
  if (type.Class == SVC_MATRIX_COLUMNS && type.Elements >= 1)
  {
    ShaderVarType local = type;
    local.Class = SVC_VECTOR;
    local.Columns = type.Rows;
    local.Elements = type.Columns * type.Elements;
    return std140Rule4(local);
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is a row - major matrix with C columns and R rows, the matrix
// is stored identically to an array of R row vectors with C components each,
// according to rule(4).
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule7(const ShaderVarType& type)
{
  if (type.Class == SVC_MATRIX_ROWS && type.Elements < 1)
  {
    ShaderVarType local = type;
    local.Class = SVC_VECTOR;
    local.Columns = type.Columns;
    local.Elements = type.Rows;
    return std140Rule4(local);
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is an array of S row - major matrices with C columns and R
// rows, the matrix is stored identically to a row of S × R row vectors with C
// components each, according to rule(4).
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule8(const ShaderVarType& type)
{
  if (type.Class == SVC_MATRIX_ROWS && type.Elements >= 1)
  {
    ShaderVarType local = type;
    local.Class = SVC_VECTOR;
    local.Columns = type.Columns;
    local.Elements = type.Rows * type.Elements;
    return std140Rule4(local);
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is a structure, the base alignment of the structure is N, where
// N is the largest base alignment value of any of its members, and rounded
// up to the base alignment of a vec4.The individual members of this substructure
// are then assigned offsets by applying this set of rules recursively,
// where the base offset of the first member of the sub - structure is equal to the
// aligned offset of the structure.The structure may have padding at the end;
// the base offset of the member following the sub - structure is rounded up to
// the next multiple of the base alignment of the structure.
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule9(const ShaderVarType& type)
{
  if (type.Class == SVC_STRUCT && type.Elements < 1)
  {
    uint32_t size = 0;
    uint32_t stride = 0;
    uint32_t maxAlign = roundToNextVec4(1);

    for (const auto& member : type.Members)
    {
      uint32_t memberSize, memberAlign, memberStride;
      bool applied;
      std::tie(memberSize, memberAlign, memberStride, applied) = applyAllRules140(member);
      if (!applied)
        return std::make_tuple(0, 0, 0, false);
      maxAlign = std::max(maxAlign, memberAlign);
      size = roundToNextMachineUnit(size);
      size += memberSize;
    }
    size = roundToNextMachineUnit(size);

    return std::make_tuple(size, maxAlign, stride, true);
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is an array of S structures, the S elements of the array are laid
// out in order, according to rule(9).
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std140Rule10(const ShaderVarType& type)
{
  if (type.Class == SVC_STRUCT && type.Elements >= 1)
  {
    ShaderVarType local = type;
    local.Elements = 0;
    uint32_t size, align, stride;
    bool applied;
    std::tie(size, align, stride, applied) = std140Rule9(local);
    if (applied)
    {
      align = roundToNextVec4(align);
      size = roundToNextMachineUnit(size);
      stride = size;
      size = stride * type.Elements;
      return std::make_tuple(size, align, stride, true);
    }
  }
  return std::make_tuple(0, 0, 0, false);
}

static const RuleFunction_t ruleTable140[] =
{
  std140Rule1,
  std140Rule2,
  std140Rule3,
  std140Rule4,
  std140Rule5,
  std140Rule6,
  std140Rule7,
  std140Rule8,
  std140Rule9,
  std140Rule10
};

// If the member is a scalar consuming N basic machine units, the base alignment
// is N.
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule1(const ShaderVarType& type)
{
  return std140Rule1(type);
}
// If the member is a two- or four-component vector with components consuming
// N basic machine units, the base alignment is 2N or 4N, respectively
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule2(const ShaderVarType& type)
{
  return std140Rule2(type);
}
// If the member is a three - component vector with components consuming N
// basic machine units, the base alignment is 4N
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule3(const ShaderVarType& type)
{
  return std140Rule3(type);
}
// If the member is an array of scalars or vectors, the base alignment and array
// stride are set to match the base alignment of a single array element, according
// to rules(1), (2), and (3), and rounded up to the base alignment of a vec4.The
// array may have padding at the end; the base offset of the member following
// the array is rounded up to the next multiple of the base alignment.
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule4(const ShaderVarType& type)
{
  if ((type.Class == SVC_VECTOR || type.Class == SVC_SCALAR) && type.Elements >= 1)
  {
    static const RuleFunction_t arraySubRules[]
    {
      std430Rule1,
      std430Rule2,
      std430Rule3
    };
    ShaderVarType local = type;
    local.Elements = 0;
    uint32_t size, align, stride;
    bool applied;
    for (auto& rule : arraySubRules)
    {
      std::tie(size, align, stride, applied) = rule(local);
      if (applied)
      {
        size = roundToNextMachineUnit(size);
        stride = size;
        size = stride * type.Elements;
        return std::make_tuple(size, align, stride, true);
      }
    }
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is a column - major matrix with C columns and R rows, the
// matrix is stored identically to an array of C column vectors with R components
// each, according to rule(4)
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule5(const ShaderVarType& type)
{
  return std140Rule5(type);
}
// If the member is an array of S column - major matrices with C columns and
// R rows, the matrix is stored identically to a row of S × C column vectors
// with R components each, according to rule(4).
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule6(const ShaderVarType& type)
{
  return std140Rule6(type);
}
// If the member is a row - major matrix with C columns and R rows, the matrix
// is stored identically to an array of R row vectors with C components each,
// according to rule(4).
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule7(const ShaderVarType& type)
{
  return std140Rule7(type);
}
// If the member is an array of S row - major matrices with C columns and R
// rows, the matrix is stored identically to a row of S × R row vectors with C
// components each, according to rule(4).
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule8(const ShaderVarType& type)
{
  return std140Rule8(type);
}
// If the member is a structure, the base alignment of the structure is N, where
// N is the largest base alignment value of any of its members, and rounded
// up to the base alignment of a vec4.The individual members of this substructure
// are then assigned offsets by applying this set of rules recursively,
// where the base offset of the first member of the sub - structure is equal to the
// aligned offset of the structure.The structure may have padding at the end;
// the base offset of the member following the sub - structure is rounded up to
// the next multiple of the base alignment of the structure.
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule9(const ShaderVarType& type)
{
  if (type.Class == SVC_STRUCT && type.Elements < 1)
  {
    uint32_t size = 0;
    uint32_t stride = 0;
    uint32_t maxAlign = 0;

    for (const auto& member : type.Members)
    {
      uint32_t memberSize, memberAlign, memberStride;
      bool applied;
      std::tie(memberSize, memberAlign, memberStride, applied) = applyAllRules430(member);
      if (!applied)
        return std::make_tuple(0, 0, 0, false);
      maxAlign = std::max(maxAlign, memberAlign);
      size = roundToNextMachineUnit(size);
      size += memberSize;
    }
    size = roundToNextMachineUnit(size);

    return std::make_tuple(size, maxAlign, stride, true);
  }
  return std::make_tuple(0, 0, 0, false);
}
// If the member is an array of S structures, the S elements of the array are laid
// out in order, according to rule(9).
static std::tuple<uint32_t, uint32_t, uint32_t, bool> std430Rule10(const ShaderVarType& type)
{
  if (type.Class == SVC_STRUCT && type.Elements >= 1)
  {
    ShaderVarType local = type;
    local.Elements = 0;
    uint32_t size, align, stride;
    bool applied;
    std::tie(size, align, stride, applied) = std430Rule9(local);
    if (applied)
    {
      align = roundToNextVec4(align);
      size = roundToNextMachineUnit(size);
      stride = size;
      size = stride * type.Elements;
      return std::make_tuple(size, align, stride, true);
    }
  }
  return std::make_tuple(0, 0, 0, false);
}

static const RuleFunction_t ruleTable430[] =
{
  std430Rule1,
  std430Rule2,
  std430Rule3,
  std430Rule4,
  std430Rule5,
  std430Rule6,
  std430Rule7,
  std430Rule8,
  std430Rule9,
  std430Rule10
};

static std::tuple<uint32_t, uint32_t, uint32_t, bool> applyAllRules140(const ShaderVarType& type)
{
  uint32_t align, size, stride;
  bool applied;
  for (auto& rule : ruleTable140)
  {
    std::tie(size, align, stride, applied) = rule(type);
    if (applied)
      return std::make_tuple(size, align, stride, true);
  }
  return std::make_tuple<uint32_t, uint32_t>(0, 0, 0, false);
}

static std::tuple<uint32_t, uint32_t, uint32_t, bool> applyAllRules430(const ShaderVarType& type)
{
  uint32_t align, size, stride;
  bool applied;
  for (auto& rule : ruleTable430)
  {
    std::tie(size, align, stride, applied) = rule(type);
    if (applied)
      return std::make_tuple(size, align, stride, true);
  }
  return std::make_tuple<uint32_t, uint32_t>(0, 0, 0, false);
}

bool ToGLSL::validateStructBufferDataLayout(const Declaration& psDecl)
{
  // have to reconstruct what hlslcc builds up
  // a unbound array (we use one element to get the stride)
  // of a struct with an array of uints with bufferstride / 4
  ShaderVarType type = {};
  type.Class = SVC_SCALAR;
  type.Type = SVT_UINT;
  type.Elements = psDecl.ui32BufferStride / 4;

  ShaderVarType stype = {};
  stype.Class = SVC_STRUCT;
  stype.Type = SVT_STRUCTURED_BUFFER;
  stype.Members.push_back(type);
  stype.Elements = 1;
  uint32_t size, align, stride;
  bool applied;
  std::tie(size, align, stride, applied) = applyAllRules430(stype);
  if (stride != psDecl.ui32BufferStride)
  {
    const bool isUAV = psDecl.eOpcode == OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED;
    std::string name = ResourceName(psContext, isUAV ? RGROUP_UAV : RGROUP_TEXTURE, psDecl.asOperands[0].ui32RegisterNumber, 0);
    const char* typeName = isUAV ? "UAV Structured Buffer" : "Structured Buffer";
    psContext->infoLog << typeName << " '" << name << "' translation does not match stride of HLSL, HLSL stride is " << psDecl.ui32BufferStride << " and GLSL stride is " << stride << std::endl;
    return false;
  }
  return true;
}

bool ToGLSL::validateConstBufferDataLayout(const Declaration& psDecl)
{
  const Operand& operand = psDecl.asOperands[0];
  uint32_t bindingPoint = operand.aui32ArraySizes[0];

  const ConstantBuffer* psCBuf = NULL;
  psContext->psShader.sInfo.GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, bindingPoint, &psCBuf);
  if (!psCBuf)
  {
    psContext->infoLog << "internal error, can not look up constant buffer on register t " << bindingPoint << std::endl;
    return false;
  }

  bool ok = true;
  uint32_t bufferSize = 0;
  for (const auto& var : psCBuf->asVars)
  {
    uint32_t size, align, stride;
    bool applied;
    std::tie(size, align, stride, applied) = applyAllRules140(var.sType);
    if (!applied)
    {
      psContext->infoLog << "Could not apply any GLSL std140 layout rule to '" << var.name.c_str() << "'" << std::endl;
      ok = false;
    }
    else if (size != var.ui32Size)
    {
      psContext->infoLog << "Constant variable size missmatch found for '" << var.name << "', HLSL size is " << var.ui32Size << " bytes and GLSL size is " << align << " bytes" << std::endl;
      ok = false;
    }
    bufferSize = var.ui32StartOffset + roundToNextAligned(size, align);
  }
  // HLSL rounds to next vec4, so fix this up
  bufferSize = roundToNextVec4(bufferSize);
  if (psCBuf->ui32TotalSizeInBytes != bufferSize)
  {
    psContext->infoLog << "Constant buffer '" << psCBuf->name << "' is not GLSL compatible, HLSL size is " << psCBuf->ui32TotalSizeInBytes << " bytes and GLSL size is " << bufferSize << " bytes" << std::endl;
    ok = false;
  }

  return ok;
}

bool ToGLSL::validateDeclarationDataLayout(const Declaration& psDecl)
{
  switch (psDecl.eOpcode)
  {
  case OPCODE_DCL_INPUT_SGV:
  case OPCODE_DCL_INPUT_PS_SGV:
  case OPCODE_DCL_OUTPUT_SIV:
  case OPCODE_DCL_INPUT:
  case OPCODE_DCL_INPUT_PS_SIV:
  case OPCODE_DCL_INPUT_SIV:
  case OPCODE_DCL_INPUT_PS:
  case OPCODE_DCL_TEMPS:
  case OPCODE_SPECIAL_DCL_IMMCONST:
  case OPCODE_DCL_RESOURCE:
  case OPCODE_DCL_OUTPUT:
  case OPCODE_DCL_GLOBAL_FLAGS:
  case OPCODE_DCL_THREAD_GROUP:
  case OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
  case OPCODE_DCL_TESS_DOMAIN:
  case OPCODE_DCL_TESS_PARTITIONING:
  case OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
  case OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
  case OPCODE_DCL_GS_INPUT_PRIMITIVE:
  case OPCODE_DCL_INTERFACE:
  case OPCODE_DCL_FUNCTION_BODY:
  case OPCODE_DCL_FUNCTION_TABLE:
  case OPCODE_CUSTOMDATA:
  case OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
  case OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT:
  case OPCODE_DCL_INDEXABLE_TEMP:
  case OPCODE_DCL_INDEX_RANGE:
  case OPCODE_HS_DECLS:
  case OPCODE_DCL_INPUT_CONTROL_POINT_COUNT:
  case OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT:
  case OPCODE_HS_FORK_PHASE:
  case OPCODE_HS_JOIN_PHASE:
  case OPCODE_DCL_SAMPLER:
  case OPCODE_DCL_HS_MAX_TESSFACTOR:
  case OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
  case OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
  case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
  case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
  case OPCODE_DCL_STREAM:
  case OPCODE_DCL_GS_INSTANCE_COUNT:
  case OPCODE_DCL_RESOURCE_RAW:
    return true;
  case OPCODE_DCL_CONSTANT_BUFFER:
    return validateConstBufferDataLayout(psDecl);
  case OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
  case OPCODE_DCL_RESOURCE_STRUCTURED:
    return validateStructBufferDataLayout(psDecl);
  default:
    psContext->infoLog << "Unexpected decl code 0x" << std::hex << psDecl.eOpcode << std::endl;
    psContext->infoLog << std::dec;
    return false;
  }
}

bool ToGLSL::validateDataLayout()
{
  if (psContext->flags & HLSLCC_FLAG_GLSL_CHECK_DATA_LAYOUT)
  {
    for (const auto& phase : psContext->psShader.asPhases)
      for (const auto& decl : phase.psDecl)
        if (!validateDeclarationDataLayout(decl))
          return false;
  }
  return true;
}

bool ToGLSL::Translate()
{
  Shader& psShader = psContext->psShader;

  psContext->psTranslator = this;

  if (language == LANG_DEFAULT)
    SetLanguage(LANG_DEFAULT);

  SetIOPrefixes();
  psShader.ExpandSWAPCs();
  psShader.ForcePositionToHighp();
  psShader.AnalyzeIOOverlap();
  psShader.FindUnusedGlobals(psContext->flags);

  psContext->indent = 0;

  bstring glsl = bfromcstralloc(1024 * 10, "\n");
  psContext->extensions = bfromcstralloc(1024 * 10, GetVersionString(language));

  psContext->glsl = glsl;
  for (auto&& phase : psShader.asPhases)
  {
    phase.postShaderCode = bfromcstralloc(1024 * 5, "");
    phase.earlyMain = bfromcstralloc(1024 * 5, "");
  }
  psContext->currentGLSLString = &glsl;
  psShader.eTargetLanguage = language;
  psContext->currentPhase = MAIN_PHASE;

  if (psShader.extensions)
  {
    if (psShader.extensions->ARB_explicit_attrib_location)
      psContext->RequireExtension("GL_ARB_explicit_attrib_location");
    if (psShader.extensions->ARB_explicit_uniform_location)
      psContext->RequireExtension("GL_ARB_explicit_uniform_location");
    if (psShader.extensions->ARB_shading_language_420pack)
      psContext->RequireExtension("GL_ARB_shading_language_420pack");
  }

  psContext->ClearDependencyData();

  AddVersionDependentCode(psContext);

  psShader.PrepareStructuredBufferBindingSlots();

  for (auto&& phase : psShader.asPhases)
  {
    phase.UnvectorizeImmMoves();
    psContext->DoDataTypeAnalysis(phase);
    phase.ResolveUAVProperties();
    psShader.ResolveStructuredBufferBindingSlots(&phase);
    phase.PruneConstArrays();
  }

  psShader.PruneTempRegisters();

  for (auto&& phase : psShader.asPhases)
    HLSLcc::DoLoopTransform(phase);

  for (auto& phase : psShader.asPhases)
    for (auto& decl : phase.psDecl)
      fixConstantBufferDeclaration(psContext, &decl);

  if (!validateDataLayout())
    return false;

  //Special case. Can have multiple phases.
  if (psShader.eShaderType == HULL_SHADER)
  {
    const SHADER_PHASE_TYPE ePhaseFuncCallOrder[3] = { HS_CTRL_POINT_PHASE, HS_FORK_PHASE, HS_JOIN_PHASE };
    uint32_t ui32PhaseCallIndex;
    int perPatchSectionAdded = 0;
    int hasControlPointPhase = 0;

    psShader.ConsolidateHullTempVars();

    // Find out if we have a passthrough hull shader
    for (auto&& phase : psShader.asPhases)
      if (phase.ePhase == HS_CTRL_POINT_PHASE)
        hasControlPointPhase = 1;

    // Phase 1 is always the global decls phase, no instructions
    for (auto&& decl : psShader.asPhases[1].psDecl)
      TranslateDeclaration(&decl);

    if (hasControlPointPhase == 0)
    {
      DoHullShaderPassthrough(psContext);
    }

    for (uint32_t ui32Phase = 2; ui32Phase < psShader.asPhases.size(); ui32Phase++)
    {
      ShaderPhase *psPhase = &psShader.asPhases[ui32Phase];
      psContext->currentPhase = ui32Phase;

#ifdef _DEBUG
      bformata(glsl, "//%s declarations\n", GetPhaseFuncName(psPhase->ePhase));
#endif
      for (auto&& decl : psPhase->psDecl)
        TranslateDeclaration(&decl);

      bformata(glsl, "void %s%d(int phaseInstanceID)\n{\n", GetPhaseFuncName(psPhase->ePhase), ui32Phase);
      psContext->indent++;

      if (!psPhase->psInst.empty())
      {
        //The minus one here is remove the return statement at end of phases.
        //We don't want to translate that, we'll just end the function body.
        ASSERT(psPhase->psInst[psPhase->psInst.size() - 1].eOpcode == OPCODE_RET);
        for (uint32_t i = 0; i < psPhase->psInst.size() - 1; ++i)
          TranslateInstruction(&psPhase->psInst[i]);
      }


      psContext->indent--;
      bcatcstr(glsl, "}\n");
    }

    bcatcstr(glsl, "void main()\n{\n");

    psContext->indent++;

    // There are cases when there are no control point phases and we have to do passthrough
    if (hasControlPointPhase == 0)
    {
      // Passthrough control point phase, run the rest only once per patch
      psContext->AddIndentation();
      bcatcstr(glsl, "passthrough_ctrl_points();\n");
      psContext->AddIndentation();
      bcatcstr(glsl, "barrier();\n");
      psContext->AddIndentation();
      bcatcstr(glsl, "if (gl_InvocationID == 0)\n");
      psContext->AddIndentation();
      bcatcstr(glsl, "{\n");
      psContext->indent++;
      perPatchSectionAdded = 1;
    }

    for (ui32PhaseCallIndex = 0; ui32PhaseCallIndex < 3; ui32PhaseCallIndex++)
    {
      for (uint32_t ui32Phase = 2; ui32Phase < psShader.asPhases.size(); ui32Phase++)
      {
        ShaderPhase *psPhase = &psShader.asPhases[ui32Phase];
        if (psPhase->ePhase != ePhaseFuncCallOrder[ui32PhaseCallIndex])
          continue;

        if (psPhase->earlyMain->slen > 1)
        {
#ifdef _DEBUG
          psContext->AddIndentation();
          bcatcstr(glsl, "//--- Start Early Main ---\n");
#endif
          bconcat(glsl, psPhase->earlyMain);
#ifdef _DEBUG
          psContext->AddIndentation();
          bcatcstr(glsl, "//--- End Early Main ---\n");
#endif
        }

        for (uint32_t i = 0; i < psPhase->ui32InstanceCount; i++)
        {
          psContext->AddIndentation();
          bformata(glsl, "%s%d(%d);\n", GetPhaseFuncName(psShader.asPhases[ui32Phase].ePhase), ui32Phase, i);
        }

        if (psPhase->hasPostShaderCode)
        {
#ifdef _DEBUG
          psContext->AddIndentation();
          bcatcstr(glsl, "//--- Post shader code ---\n");
#endif
          bconcat(glsl, psPhase->postShaderCode);
#ifdef _DEBUG
          psContext->AddIndentation();
          bcatcstr(glsl, "//--- End post shader code ---\n");
#endif
        }


        if (psShader.asPhases[ui32Phase].ePhase == HS_CTRL_POINT_PHASE)
        {
          // We're done printing control point phase, run the rest only once per patch
          psContext->AddIndentation();
          bcatcstr(glsl, "barrier();\n");
          psContext->AddIndentation();
          bcatcstr(glsl, "if (gl_InvocationID == 0)\n");
          psContext->AddIndentation();
          bcatcstr(glsl, "{\n");
          psContext->indent++;
          perPatchSectionAdded = 1;
        }
      }
    }

    if (perPatchSectionAdded != 0)
    {
      psContext->indent--;
      psContext->AddIndentation();
      bcatcstr(glsl, "}\n");
    }

    psContext->indent--;

    bcatcstr(glsl, "}\n");

    // Concat extensions and glsl for the final shader code.
    bconcat(psContext->extensions, glsl);
    bdestroy(glsl);
    psContext->glsl = psContext->extensions;
    glsl = NULL;
    //Save partitioning and primitive type for use by domain shader.
    psContext->psDependencies.setTessOutPrimitive(psShader.sInfo.eTessOutPrim);

    psContext->psDependencies.setTessPartitioning(psShader.sInfo.eTessPartitioning);

    return true;
  }

  if (psShader.eShaderType == DOMAIN_SHADER)
  {
    //Load partitioning and primitive type from hull shader.
    switch (psContext->psDependencies.getTessOutPrimitive())
    {
    case TESSELLATOR_OUTPUT_TRIANGLE_CCW:
    {
      bcatcstr(glsl, "layout(ccw) in;\n");
      break;
    }
    case TESSELLATOR_OUTPUT_TRIANGLE_CW:
    {
      bcatcstr(glsl, "layout(cw) in;\n");
      break;
    }
    case TESSELLATOR_OUTPUT_POINT:
    {
      bcatcstr(glsl, "layout(point_mode) in;\n");
      break;
    }
    default:
    {
      break;
    }
    }

    switch (psContext->psDependencies.getTessPartitioning())
    {
    case TESSELLATOR_PARTITIONING_FRACTIONAL_ODD:
    {
      bcatcstr(glsl, "layout(fractional_odd_spacing) in;\n");
      break;
    }
    case TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN:
    {
      bcatcstr(glsl, "layout(fractional_even_spacing) in;\n");
      break;
    }
    default:
    {
      break;
    }
    }
  }

  for (auto&& decl : psShader.asPhases[0].psDecl)
    TranslateDeclaration(&decl);
  bcatcstr(glsl, "uvec4 helper_temp;\n");
  bcatcstr(glsl, "void main()\n{\n");

  psContext->indent++;

  if (psContext->psShader.asPhases[0].earlyMain->slen > 1)
  {
#ifdef _DEBUG
    psContext->AddIndentation();
    bcatcstr(glsl, "//--- Start Early Main ---\n");
#endif
    bconcat(glsl, psContext->psShader.asPhases[0].earlyMain);
#ifdef _DEBUG
    psContext->AddIndentation();
    bcatcstr(glsl, "//--- End Early Main ---\n");
#endif
  }

  for (auto&& decl : psShader.asPhases[0].psInst)
    TranslateInstruction(&decl);

  psContext->indent--;

  bcatcstr(glsl, "}\n");

  // Concat extensions and glsl for the final shader code.
  bconcat(psContext->extensions, glsl);
  bdestroy(glsl);
  psContext->glsl = psContext->extensions;
  glsl = NULL;

  return true;
}


