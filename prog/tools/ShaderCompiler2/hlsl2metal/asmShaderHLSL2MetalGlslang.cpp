// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "asmShaderHLSL2MetalGlslang.h"

#include <algorithm>
#include <sstream>
#include <osApiWrappers/dag_localConv.h>

#include <spirv2Metal/spirv.hpp>
#include <spirv2Metal/spirv_msl.hpp>
#include <spirv2Metal/spirv_cpp.hpp>

#include <spirv.hpp>
#include <SPIRV/disassemble.h>
#include <SPIRV/GlslangToSpv.h>

#include <glslang/Public/ShaderLang.h>
#include <glslang/MachineIndependent/iomapper.h>

#include <vulkan/vulkan.h>

#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

#include <ioSys/dag_fileIo.h>
#include <util/dag_string.h>

#include "HLSL2MetalCommon.h"
#include "spirv2MetalCompiler.h"
#include "buffBindPoints.h"
#include "../debugSpitfile.h"

#include <spirv/compiler.h>

#include "dataAccumulator.h"

#ifdef __APPLE__
#include <unistd.h>
#endif
#include <vector>
#include <fstream>

using namespace std;
using namespace spv;

struct GlSlangLoader
{
  GlSlangLoader() { glslang::InitializeProcess(); }

  ~GlSlangLoader() { glslang::FinalizeProcess(); }
};

GlSlangLoader glSlangLoader;

static const EShLanguage EShLangInvalid = EShLangCount;
static EShLanguage profile_to_stage(const char *profile)
{
  switch (profile[0])
  {
    case 'c': return EShLangCompute;
    case 'v': return EShLangVertex;
    case 'p': return EShLangFragment;
    default: return EShLangInvalid;
  }
  return EShLangInvalid;
}

static ostream &operator<<(ostream &os, const spv_position_t &position)
{
  os << position.line << ", " << position.column << " (" << position.index << ")";
  return os;
}

static constexpr uint32_t B_REGISTER_SET = 4;
static constexpr uint32_t T_REGISTER_SET = 5;
static constexpr uint32_t U_REGISTER_SET = 6;

struct VsInputs
{
  int reg = -1;
  int vsdr = -1;
  int vecsize = 0;
};
struct Resource
{
  int set = -1;
  int bindPoint = -1;
  int remappedBindPoint = -1;
  bool isTexture = false;
  bool isSampler = false;
};

static SemanticValue *lookupSemantic(const string &key)
{
  for (int k = 0; k < getSemanticValueCount(); k++)
  {
    SemanticValue &val = getSemanticValue(k);

    if (key == string(val.name) + to_string(val.index) || (val.index == 0 && key == string(val.name)))
      return &val;
  }
  return nullptr;
}

static spirv::ReflectionInfo::Type GetReflectionType(const glslang::TType &type, const glslang::TQualifier &qual)
{
  auto btype = type.getBasicType();
  if (btype == glslang::EbtSampler && type.getSampler().sampler == true)
    return spirv::ReflectionInfo::Type::Sampler;
  else if (btype == glslang::EbtSampler && type.getSampler().sampler == false)
    return spirv::ReflectionInfo::Type::Texture;
  else if (qual.declaredBuiltIn == glslang::EbvStructuredBuffer || qual.declaredBuiltIn == glslang::EbvByteAddressBuffer ||
           qual.declaredBuiltIn == glslang::EbvRWStructuredBuffer || qual.declaredBuiltIn == glslang::EbvRWByteAddressBuffer ||
           qual.declaredBuiltIn == glslang::EbvAppendConsume)
    return spirv::ReflectionInfo::Type::StructuredBuffer;
  return spirv::ReflectionInfo::Type::ConstantBuffer;
}

static int GetReflectionSet(const glslang::TType &type, const glslang::TQualifier &qual)
{
  if (type.getBasicType() == glslang::EbtSampler)
  {
    int set = type.getSampler().image ? U_REGISTER_SET : T_REGISTER_SET;
    if (type.getSampler().sampler)
      set += 16;
    return set;
  }
  else if (type.getBasicType() == glslang::EbtBlock)
  {
    if (qual.declaredBuiltIn == glslang::EbvStructuredBuffer || qual.declaredBuiltIn == glslang::EbvByteAddressBuffer)
    {
      return T_REGISTER_SET;
    }
    else if (qual.declaredBuiltIn == glslang::EbvRWStructuredBuffer || qual.declaredBuiltIn == glslang::EbvRWByteAddressBuffer ||
             qual.declaredBuiltIn == glslang::EbvAppendConsume)
    {
      G_ASSERT(qual.storage == glslang::EvqBuffer);
      return U_REGISTER_SET;
    }
    else
      return B_REGISTER_SET;
  }
  else
    G_ASSERT(0 && "why are we here?");

  return -1;
}

class MslIoResolver : public glslang::TIoMapResolver
{
public:
  MslIoResolver(eastl::vector<spirv::ReflectionInfo> &resourceMap, bool use_ios) : resourceMap(resourceMap), use_ios_token(use_ios) {}

  // Should return true if the resulting/current binding would be okay.
  // Basic idea is to do aliasing binding checks with this.
  bool validateBinding(EShLanguage, glslang::TVarEntryInfo &ent) override { return true; }
  // Should return a value >= 0 if the current binding should be overridden.
  // Return -1 if the current binding (including no binding) should be kept.
  int resolveBinding(EShLanguage, glslang::TVarEntryInfo &ent) override
  {
    auto &type = ent.symbol->getType();
    auto &qual = type.getQualifier();

    int bindingPoint = qual.layoutBinding, remappedPoint = bindingPoint, registerId = -1;
    if (type.getBasicType() == glslang::EbtSampler)
    {
      registerId = T_REGISTER_SET;
      if (type.getSampler().image)
        registerId = U_REGISTER_SET;

      if (type.getSampler().sampler)
      {
        registerId += 16;
        remappedPoint = samplerIndex++;
      }
      else
        remappedPoint = textureIndex++;
    }
    else if (type.getBasicType() == glslang::EbtBlock)
    {
      if (type.getTypeName() == "$Global")
      {
        registerId = B_REGISTER_SET;
        bindingPoint = 0;
        remappedPoint = drv3d_metal::BIND_POINT;

        int last_offset = -1;
        auto &struc = *type.getStruct();
        for (auto &field : struc)
        {
          if (last_offset > field.type->getQualifier().layoutOffset)
            errors += field.type->getFieldName().c_str() + std::string(" has incorrect offset ") +
                      std::to_string(field.type->getQualifier().layoutOffset) + "\n";
          last_offset = field.type->getQualifier().layoutOffset;
        }
      }
      else if (bindingPoint == 65535)
        return bindingPoint;
      else
      {
        remappedPoint = bufferIndex;
        if (qual.declaredBuiltIn == glslang::EbvStructuredBuffer || qual.declaredBuiltIn == glslang::EbvByteAddressBuffer)
        {
          registerId = T_REGISTER_SET;
          bufferIndex++;
        }
        else if (qual.declaredBuiltIn == glslang::EbvRWStructuredBuffer || qual.declaredBuiltIn == glslang::EbvRWByteAddressBuffer ||
                 qual.declaredBuiltIn == glslang::EbvAppendConsume)
        {
          G_ASSERT(qual.storage == glslang::EvqBuffer);
          registerId = U_REGISTER_SET;
          bufferIndex++;
        }
        else
        {
          registerId = B_REGISTER_SET;
          if (bindingPoint == 0)
            remappedPoint = drv3d_metal::BIND_POINT;
          else
            bufferIndex++;
        }
      }
    }
    else
      G_ASSERT(0 && "why are we here?");

    {
      std::string bname = type.getBasicType() != glslang::EbtSampler ? type.getTypeName().c_str() : ent.symbol->getName().c_str();

      spirv::ReflectionInfo info;
      info.name = bname.c_str();
      info.type = GetReflectionType(type, qual);
      info.set = GetReflectionSet(type, qual);
      info.uav = registerId == U_REGISTER_SET;
      info.binding = bindingPoint;

      resourceMap.push_back(info);
    }
    ent.newBinding = bindingPoint;
    return bindingPoint;
  }
  // Should return a value >= 0 if the current location should be overridden.
  // Return -1 if the current location (including no location) should be kept.
  int resolveUniformLocation(EShLanguage, glslang::TVarEntryInfo &ent) override { return ent.newLocation; }
  // Should return a value >= 0 if the current set should be overridden.
  // Return -1 if the current set (including no set) should be kept.
  int resolveSet(EShLanguage, glslang::TVarEntryInfo &ent) override
  {
    auto &type = ent.symbol->getType();
    auto &qual = type.getQualifier();

    ent.newSet = GetReflectionSet(type, qual);

    return ent.newSet;
  }
  // Should return true if the resulting/current setup would be okay.
  // Basic idea is to do aliasing checks and reject invalid semantic names.
  bool validateInOut(EShLanguage, glslang::TVarEntryInfo &ent) override
  {
    // return true here, or we reject buildin stuff like workgroup size
    return true;
  }
  // Should return a value >= 0 if the current location should be overridden.
  // Return -1 if the current location (including no location) should be kept.
  int resolveInOutLocation(EShLanguage lang, glslang::TVarEntryInfo &ent) override
  {
    auto &type = ent.symbol->getType();
    auto &qual = type.getQualifier();
    if (qual.builtIn != glslang::EbvNone)
    {
      if (
        !use_ios_token && qual.storage != glslang::EvqVaryingIn && qual.semanticName && strcmp(qual.semanticName, "SV_POSITION") == 0)
        ent.symbol->getQualifier().invariant = true;
      return ent.newLocation;
    }
    bool isInput = qual.storage == glslang::EvqVaryingIn;
    if (ent.stage != EShLangFragment)
    {
      if (isInput)
      {
        std::string name = ent.symbol->getName().c_str();
        std::replace(name.begin(), name.end(), '.', '_');
        ent.newLocation = ent.stage == EShLangVertex ? CacheVsInput(qual.semanticName, name, type.getVectorSize()) : inputsCounter++;
      }
      else
        ent.newLocation = CacheInOut(qual.semanticName, isInput);
    }
    else if (isInput)
    {
      ent.newLocation = CacheInOut(qual.semanticName, isInput);
    }
    return ent.newLocation;
  }
  // Should return a value >= 0 if the current component index should be overridden.
  // Return -1 if the current component index (including no index) should be kept.
  int resolveInOutComponent(EShLanguage, glslang::TVarEntryInfo &ent) override { return ent.newComponent; }
  // Should return a value >= 0 if the current color index should be overridden.
  // Return -1 if the current color index (including no index) should be kept.
  int resolveInOutIndex(EShLanguage, glslang::TVarEntryInfo &ent) override { return ent.newIndex; }
  // Notification of a in or out variable
  void notifyInOut(EShLanguage, glslang::TVarEntryInfo &ent) override {}
  // Notification of a uniform variable
  void notifyBinding(EShLanguage, glslang::TVarEntryInfo &ent) override {}
  // Called by mapIO when it has finished the notify pass
  void endNotifications(EShLanguage) override {}
  // Called by mapIO when it starts its notify pass for the given stage
  void beginNotifications(EShLanguage) override {}
  // Called by mipIO when it starts its resolve pass for the given stage
  void beginResolve(EShLanguage) override {}
  // Called by mapIO when it has finished the resolve pass
  void endResolve(EShLanguage) override {}

  // Called by mapIO when it starts its symbol collect for teh given stage
  virtual void beginCollect(EShLanguage stage) override {}

  // Called by mapIO when it has finished the symbol collect
  virtual void endCollect(EShLanguage stage) override {}

  // Called by TSlotCollector to resolve storage locations or bindings
  virtual void reserverStorageSlot(glslang::TVarEntryInfo &ent, TInfoSink &infoSink) override {}

  // Called by TSlotCollector to resolve resource locations or bindings
  virtual void reserverResourceSlot(glslang::TVarEntryInfo &ent, TInfoSink &infoSink) override {}

  // Called by mapIO.addStage to set shader stage mask to mark a stage be added to this pipeline
  void addStage(EShLanguage stage, glslang::TIntermediate &stageIntermediate) override {}

  int CacheVsInput(const string &key, const string &name, int vecsize)
  {
    G_ASSERT(name != "");
    auto it = inputMap.find(name);
    if (it != end(inputMap))
      return it->second.reg;

    SemanticValue *val = lookupSemantic(key);
    if (val)
    {
      int reg = (int)inputMap.size();
      inputMap[name].reg = reg;
      inputMap[name].vecsize = vecsize;
      inputMap[name].vsdr = val->vsdr;
      return reg;
    }

    debug("what's that? %s", name.c_str());
    return -1;
  }

  int CacheInOut(const string &name, bool isInput)
  {
    for (int k = 0; k < getSemanticValueCount(); k++)
    {
      SemanticValue &val = getSemanticValue(k);

      if (name == string(val.name) + to_string(val.index) || (val.index == 0 && name == string(val.name)))
        return k;
    }

    int index = isInput ? inputsCounter++ : outputCounter++;
    return 21 + index;
  }

  unordered_map<string, VsInputs> inputMap;
  int inputsCounter = 0;
  int outputCounter = 0;
  int bufferIndex = drv3d_metal::CONST_BUFFER_POINT;
  int textureIndex = 0;
  int samplerIndex = 0;
  eastl::vector<spirv::ReflectionInfo> &resourceMap;
  std::string errors;
  bool use_ios_token = false;
};

// values are the default values of glslang
static TBuiltInResource Resources = //
  {32,                              // int maxLights;
    6,                              // int maxClipPlanes;
    32,                             // int maxTextureUnits;
    32,                             // int maxTextureCoords;
    64,                             // int maxVertexAttribs;
    4096,                           // int maxVertexUniformComponents;
    64,                             // int maxVaryingFloats;
    64,                             // int maxVertexTextureImageUnits;
    80,                             // int maxCombinedTextureImageUnits;
    32,                             // int maxTextureImageUnits;
    4096,                           // int maxFragmentUniformComponents;
    32,                             // int maxDrawBuffers;
    128,                            // int maxVertexUniformVectors;
    8,                              // int maxVaryingVectors;
    16,                             // int maxFragmentUniformVectors;
    16,                             // int maxVertexOutputVectors;
    15,                             // int maxFragmentInputVectors;
    -8,                             // int minProgramTexelOffset;
    7,                              // int maxProgramTexelOffset;
    8,                              // int maxClipDistances;
    65535,                          // int maxComputeWorkGroupCountX;
    65535,                          // int maxComputeWorkGroupCountY;
    65535,                          // int maxComputeWorkGroupCountZ;
    1024,                           // int maxComputeWorkGroupSizeX;
    1024,                           // int maxComputeWorkGroupSizeY;
    64,                             // int maxComputeWorkGroupSizeZ;
    1024,                           // int maxComputeUniformComponents;
    16,                             // int maxComputeTextureImageUnits;
    8,                              // int maxComputeImageUniforms;
    8,                              // int maxComputeAtomicCounters;
    1,                              // int maxComputeAtomicCounterBuffers;
    60,                             // int maxVaryingComponents;
    64,                             // int maxVertexOutputComponents;
    64,                             // int maxGeometryInputComponents;
    128,                            // int maxGeometryOutputComponents;
    128,                            // int maxFragmentInputComponents;
    8,                              // int maxImageUnits;
    8,                              // int maxCombinedImageUnitsAndFragmentOutputs;
    8,                              // int maxCombinedShaderOutputResources;
    0,                              // int maxImageSamples;
    0,                              // int maxVertexImageUniforms;
    0,                              // int maxTessControlImageUniforms;
    0,                              // int maxTessEvaluationImageUniforms;
    0,                              // int maxGeometryImageUniforms;
    8,                              // int maxFragmentImageUniforms;
    8,                              // int maxCombinedImageUniforms;
    16,                             // int maxGeometryTextureImageUnits;
    256,                            // int maxGeometryOutputVertices;
    1024,                           // int maxGeometryTotalOutputComponents;
    1024,                           // int maxGeometryUniformComponents;
    64,                             // int maxGeometryVaryingComponents;
    128,                            // int maxTessControlInputComponents;
    128,                            // int maxTessControlOutputComponents;
    16,                             // int maxTessControlTextureImageUnits;
    1024,                           // int maxTessControlUniformComponents;
    4096,                           // int maxTessControlTotalOutputComponents;
    128,                            // int maxTessEvaluationInputComponents;
    128,                            // int maxTessEvaluationOutputComponents;
    16,                             // int maxTessEvaluationTextureImageUnits;
    1024,                           // int maxTessEvaluationUniformComponents;
    120,                            // int maxTessPatchComponents;
    32,                             // int maxPatchVertices;
    64,                             // int maxTessGenLevel;
    16,                             // int maxViewports;
    0,                              // int maxVertexAtomicCounters;
    0,                              // int maxTessControlAtomicCounters;
    0,                              // int maxTessEvaluationAtomicCounters;
    0,                              // int maxGeometryAtomicCounters;
    8,                              // int maxFragmentAtomicCounters;
    8,                              // int maxCombinedAtomicCounters;
    1,                              // int maxAtomicCounterBindings;
    0,                              // int maxVertexAtomicCounterBuffers;
    0,                              // int maxTessControlAtomicCounterBuffers;
    0,                              // int maxTessEvaluationAtomicCounterBuffers;
    0,                              // int maxGeometryAtomicCounterBuffers;
    1,                              // int maxFragmentAtomicCounterBuffers;
    1,                              // int maxCombinedAtomicCounterBuffers;
    16384,                          // int maxAtomicCounterBufferSize;
    4,                              // int maxTransformFeedbackBuffers;
    64,                             // int maxTransformFeedbackInterleavedComponents;
    8,                              // int maxCullDistances;
    8,                              // int maxCombinedClipAndCullDistances;
    8,                              // int maxSamples;
    0,                              // int maxMeshOutputVerticesNV;
    0,                              // int maxMeshOutputPrimitivesNV;
    0,                              // int maxMeshWorkGroupSizeX_NV;
    0,                              // int maxMeshWorkGroupSizeY_NV;
    0,                              // int maxMeshWorkGroupSizeZ_NV;
    0,                              // int maxTaskWorkGroupSizeX_NV;
    0,                              // int maxTaskWorkGroupSizeY_NV;
    0,                              // int maxTaskWorkGroupSizeZ_NV;
    0,                              // int maxMeshViewCountNV;
    0,                              // int maxDualSourceDrawBuffersEXT;
    {
      true, // bool nonInductiveForLoops;
      true, // bool whileLoops;
      true, // bool doWhileLoops;
      true, // bool generalUniformIndexing;
      true, // bool generalAttributeMatrixVectorIndexing;
      true, // bool generalVaryingIndexing;
      true, // bool generalSamplerIndexing;
      true, // bool generalVariableIndexing;
      true, // bool generalConstantMatrixVectorIndexing;
    }};

struct GLSLToSpirVResult
{
  std::vector<std::string> infoLog;
  std::vector<uint32_t> byteCode;
};

static bool HasResource(const spirv_cross::SmallVector<spirv_cross::Resource> &resources, const std::string &res)
{
  for (const auto &resource : resources)
    if (resource.name == res)
      return true;
  return false;
}

CompileResult compileShaderMetalGlslang(const char *source, const char *profile, const char *entry, bool need_disasm, bool enable_fp16,
  bool skipValidation, bool optimize, int max_constants_no, const char *shader_name, bool use_ios_token, bool use_binary_msl,
  uint64_t shader_variant_hash)
{
  CompileResult compile_result;

  eastl::vector<spirv::ReflectionInfo> resourceMap;

  int cx = 1, cy = 1, cz = 1;

  const ShaderType shaderType = profile_to_shader_type(profile);
  const EShLanguage targetStage = profile_to_stage(profile);
  if (targetStage == EShLangInvalid)
  {
    compile_result.errors.sprintf("unknown target profile %s", profile);
    return compile_result;
  }

  std::vector<uint32_t> spirv_binary;
  GLSLToSpirVResult finalSpirV;

  EShMessages SourceLangRules =
    enable_fp16 ? EShMessages(EShMsgSpvRules | EShMsgVulkanRules | EShMsgReadHlsl | EShMsgHlslOffsets | EShMsgHlslEnable16BitTypes)
                : EShMessages(EShMsgSpvRules | EShMsgVulkanRules | EShMsgReadHlsl | EShMsgHlslOffsets);

  const char *defines = enable_fp16 ? "#define HW_VERTEX_ID uint vertexId: SV_VertexID;\n"
                                      "#define HW_BASE_VERTEX_ID error! not supported on this compiler/API\n"
                                      "#define HW_BASE_VERTEX_ID_OPTIONAL  \n"
                                      "#define USE_VERTEX_ID_WITHOUT_BASE_OFFSET(input_struct)  \n"
                                      "#define half min16float\n"
                                      "#define half1 min16float1\n"
                                      "#define half2 min16float2\n"
                                      "#define half3 min16float3\n"
                                      "#define half4 min16float4\n"
                                      "#define HALF_PRECISION 1\n"

                                    : "#define HW_VERTEX_ID uint vertexId: SV_VertexID;\n"
                                      "#define HW_BASE_VERTEX_ID error! not supported on this compiler/API\n"
                                      "#define HW_BASE_VERTEX_ID_OPTIONAL  \n"
                                      "#define USE_VERTEX_ID_WITHOUT_BASE_OFFSET(input_struct)  \n"
                                      "#define half float\n"
                                      "#define half1 float1\n"
                                      "#define half2 float2\n"
                                      "#define half3 float3\n"
                                      "#define half4 float4\n";

  GLSLToSpirVResult result = {};
  // needs to be on the heap or it may corrupt the stack
  auto shader = make_unique<glslang::TShader>(targetStage);
  shader->setPreamble(defines);
  shader->setStrings(&source, 1);
  shader->setSourceEntryPoint(entry);
  shader->setEntryPoint("main");
  shader->setEnvTargetHlslFunctionality1();

  shader->setEnvInput(glslang::EShSourceHlsl, targetStage, glslang::EShClientVulkan, 100);
  shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
  shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_1);

  TBuiltInResource Resources = {};
  bool ok = shader->parse(&Resources, 100, ENoProfile, false, false, SourceLangRules);
  const char *infoLog = shader->getInfoLog();
  const char *debugLog = shader->getInfoDebugLog();
  if (infoLog && infoLog[0])
    finalSpirV.infoLog.push_back(infoLog);
  if (debugLog && debugLog[0])
    finalSpirV.infoLog.push_back(debugLog);

  if (ok)
  {
    // needs to be on the heap or it may corrupt the stack
    auto program = make_unique<glslang::TProgram>();
    program->addShader(shader.get());
    ok = program->link(SourceLangRules);
    infoLog = program->getInfoLog();
    debugLog = program->getInfoDebugLog();
    if (infoLog && infoLog[0])
      finalSpirV.infoLog.push_back(infoLog);
    if (debugLog && debugLog[0])
      finalSpirV.infoLog.push_back(debugLog);

    if (targetStage == EShLangCompute)
    {
      auto inter = program->getIntermediate(targetStage);

      cx = inter->getLocalSize(0);
      cy = inter->getLocalSize(1);
      cz = inter->getLocalSize(2);

      compile_result.computeShaderInfo.threadGroupSizeX = cx;
      compile_result.computeShaderInfo.threadGroupSizeY = cy;
      compile_result.computeShaderInfo.threadGroupSizeZ = cz;
    }

    if (ok)
    {
      unique_ptr<MslIoResolver> resolver = make_unique<MslIoResolver>(resourceMap, use_ios_token);
      {
        if (program->mapIO(resolver.get()) && resolver->errors == "")
        {
          glslang::SpvOptions spvOptions;
          spvOptions.generateDebugInfo = !optimize;
          spvOptions.validate = true;

          spv::SpvBuildLogger logger;
          glslang::GlslangToSpv(*program->getIntermediate(shader->getStage()), finalSpirV.byteCode, &logger, &spvOptions);
          if (!logger.getAllMessages().empty())
          {
            finalSpirV.infoLog.push_back("failed generate spirv");
            finalSpirV.infoLog.push_back(logger.getAllMessages());
          }
        }
        else
        {
          finalSpirV.infoLog.push_back("failed to map io");
          finalSpirV.infoLog.push_back(resolver->errors);
        }
      }
    }
  }

  if (finalSpirV.byteCode.empty())
  {
    std::string errorMessage;
    for (auto &&msg : finalSpirV.infoLog)
    {
      errorMessage += msg;
      errorMessage += '\n';
    }
    compile_result.errors.sprintf("GLSL to Spir-V compilation failed:\n %s", errorMessage.c_str());
    return compile_result;
  }
  else if (!finalSpirV.infoLog.empty())
  {
    for (auto &&msg : finalSpirV.infoLog)
      debug(msg.c_str());
  }
  spirv_binary = finalSpirV.byteCode;

  auto spirv_version = SPV_ENV_UNIVERSAL_1_1;

  // run a spir-v tools optimize pass over it to have overall better shaders
  if (optimize)
  {
    spvtools::Optimizer optimizer{spirv_version};
    stringstream infoStream;
    optimizer.SetMessageConsumer([&infoStream](spv_message_level_t level, const char * /* source */, const spv_position_t &position,
                                   const char *message) //
      {
        infoStream << "[" << level << "][" << position << "] " << message << endl;
        ;
      });

    if (enable_fp16)
    {
      optimizer.RegisterPass(spvtools::CreateConvertRelaxedToHalfPass());
    }
    optimizer.RegisterPerformancePasses();
    optimizer.RegisterPass(spvtools::CreateLoopUnrollPass(true));

    vector<uint32_t> optimized;
    if (optimizer.Run(spirv_binary.data(), spirv_binary.size(), &optimized))
    {
      swap(optimized, spirv_binary);
    }
    else
    {
      std::string spirv_text;
      {
        spvtools::SpirvTools tools{spirv_version};
        tools.Disassemble(spirv_binary, &spirv_text);
      }
      debug("Spir-V optimization failed, using unoptimized version:\n%s\n\n%s", infoStream.str().c_str(), spirv_text.c_str());
      compile_result.errors.sprintf("Spir-V optimization failed:\n %s", infoStream.str().c_str());
      return compile_result;
    }
    string infoMessage = infoStream.str();
    if (!infoMessage.empty())
    {
      debug("Spir-V Optimizer log: %s", infoMessage.c_str());
    }
  }

  CompilerMSLlocal msl(spirv_binary, use_ios_token);

  return msl.convertToMSL(compile_result, resourceMap, source, shaderType, shader_name, entry, shader_variant_hash, use_binary_msl);
}
