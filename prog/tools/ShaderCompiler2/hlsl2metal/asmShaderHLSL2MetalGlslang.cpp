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

#include "dataAccumulator.h"

#ifdef __APPLE__
#include <unistd.h>
#endif
#include <vector>
#include <fstream>

using namespace std;

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

void saveTexuredata(spirv_cross::CompilerMSL &msl, DataAccumulator &header, spirv_cross::SmallVector<spirv_cross::Resource> &images,
  dag::ConstSpan<int> textureRemap, int offset)
{
  if (images.size() == 0)
    return;

  for (const auto &image : images)
  {
    auto &type = msl.get_type(image.base_type_id);
    const auto &imageType = type.image;
    MetalImageType img_tp = translateImageType(imageType);

    int slot = offset + msl.get_decoration(image.id, spv::DecorationBinding);

    header.appendValue(slot);

    int internalSlot = textureRemap[slot];
    G_ASSERT(internalSlot >= 0);
    header.appendValue(internalSlot);

    header.appendValue(img_tp);
  }
}

class MslIoResolver : public glslang::TIoMapResolver
{
public:
  MslIoResolver(unordered_map<string, Resource> &resourceMap, unordered_map<string, VsInputs> &inputMap, bool use_ios) :
    resourceMap(resourceMap), inputMap(inputMap), use_ios_token(use_ios)
  {}

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
      G_ASSERT(resourceMap.find(bname) == resourceMap.end());
      resourceMap[bname].set = registerId;
      resourceMap[bname].bindPoint = bindingPoint;
      resourceMap[bname].remappedBindPoint = remappedPoint;
      resourceMap[bname].isTexture = type.getBasicType() == glslang::EbtSampler && type.getSampler().sampler == false;
      resourceMap[bname].isSampler = type.getBasicType() == glslang::EbtSampler && type.getSampler().sampler == true;
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

    if (type.getBasicType() == glslang::EbtSampler)
    {
      ent.newSet = type.getSampler().image ? U_REGISTER_SET : T_REGISTER_SET;
      if (type.getSampler().sampler)
        ent.newSet += 16;
    }
    else if (type.getBasicType() == glslang::EbtBlock)
    {
      if (qual.declaredBuiltIn == glslang::EbvStructuredBuffer || qual.declaredBuiltIn == glslang::EbvByteAddressBuffer)
      {
        ent.newSet = T_REGISTER_SET;
      }
      else if (qual.declaredBuiltIn == glslang::EbvRWStructuredBuffer || qual.declaredBuiltIn == glslang::EbvRWByteAddressBuffer ||
               qual.declaredBuiltIn == glslang::EbvAppendConsume)
      {
        G_ASSERT(qual.storage == glslang::EvqBuffer);
        ent.newSet = U_REGISTER_SET;
      }
      else
        ent.newSet = B_REGISTER_SET;
    }
    else
      G_ASSERT(0 && "why are we here?");
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

    for (int k = 0; k < getSemanticValueCount(); k++)
    {
      SemanticValue &val = getSemanticValue(k);

      if (key == string(val.name) + to_string(val.index) || (val.index == 0 && key == string(val.name)))
      {
        int reg = (int)inputMap.size();
        inputMap[name].reg = reg;
        inputMap[name].vecsize = vecsize;
        inputMap[name].vsdr = val.vsdr;
        return reg;
      }
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

  unordered_map<string, VsInputs> &inputMap;
  int inputsCounter = 0;
  int outputCounter = 0;
  int bufferIndex = drv3d_metal::CONST_BUFFER_POINT;
  int textureIndex = 0;
  int samplerIndex = 0;
  unordered_map<string, Resource> &resourceMap;
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

#ifdef __APPLE__
std::string getTempFile()
{
  char path[] = "/tmp/shader-XXXXXXX";
  int fd = mkstemp(path);

  if (fd == -1)
    return "";

  close(fd);

  return path;
}

int system(const char *command)
{
  pid_t pid = fork();

  if (pid > 0)
  {
    int status;
    waitpid(pid, &status, 0);

    return status;
  }
  else
  {
    _exit(execl("/bin/sh", "sh", "-c", command, NULL));
  }
}

static std::vector<uint8_t> readFile(const std::string &path)
{
  std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
  if (!in)
    return std::vector<uint8_t>();

  std::vector<uint8_t> result;

  while (in)
  {
    uint8_t buf[4096];

    in.read((char *)buf, sizeof(buf));
    result.insert(result.end(), buf, buf + in.gcount());
  }

  return result;
}

static std::vector<uint8_t> compileMSL(const std::string &source, bool use_ios_token)
{
  std::string bindir = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/";
  if (use_ios_token)
    bindir = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/metal/ios/bin/";

  std::string tmps = getTempFile();
  std::string tmpc = getTempFile();

  {
    std::ofstream out(tmps);
    out << source;
  }

  std::string cmdc = bindir + "metal";

  cmdc += use_ios_token ? " -std=ios-metal2.4" : " -std=macos-metal2.2";
  cmdc += use_ios_token ? " -miphoneos-version-min=15.0" : " -mmacos-version-min=10.15";
  cmdc += " -Wno-uninitialized -Wno-unused-variable";
  cmdc += " -x metal -Os";
  cmdc += " -o " + tmpc;
  cmdc += " " + tmps;

  int rc = system(cmdc.c_str());

  if (rc != 0)
    return std::vector<uint8_t>();

  std::vector<uint8_t> result = readFile(tmpc);

  unlink(tmps.c_str());
  unlink(tmpc.c_str());

  return result;
}
#endif

CompileResult compileShaderMetalGlslang(const char *source, const char *profile, const char *entry, bool need_disasm,
  bool skipValidation, bool optimize, int max_constants_no, int bones_const_used, const char *shader_name, bool use_ios_token,
  bool use_binary_msl)
{
  CompileResult compile_result;
  int index = g_index;
  g_index++;

  unordered_map<string, VsInputs> inputMap;
  array<int, drv3d_metal::BUFFER_POINT_COUNT> bufferRemap;
  array<int, drv3d_metal::MAX_SHADER_TEXTURES * 2> textureRemap;
  array<int, drv3d_metal::MAX_SHADER_TEXTURES> samplerRemap;
  unordered_map<string, Resource> resourceMap;

  fill(bufferRemap.begin(), bufferRemap.end(), -1);
  fill(textureRemap.begin(), textureRemap.end(), -1);
  fill(samplerRemap.begin(), samplerRemap.end(), -1);

  bool is_half = dd_stricmp("ps_5_0_half", profile) == 0 || dd_stricmp("vs_5_0_half", profile) == 0;

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
    is_half ? EShMessages(EShMsgSpvRules | EShMsgVulkanRules | EShMsgReadHlsl | EShMsgHlslOffsets | EShMsgHlslEnable16BitTypes)
            : EShMessages(EShMsgSpvRules | EShMsgVulkanRules | EShMsgReadHlsl | EShMsgHlslOffsets);

  const char *defines = is_half ? "#define HW_VERTEX_ID uint vertexId: SV_VertexID;\n"
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
      unique_ptr<MslIoResolver> resolver = make_unique<MslIoResolver>(resourceMap, inputMap, use_ios_token);
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

    if (is_half)
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

  CompilerMSLlocal msl(spirv_binary);
  setupMSLCompiler(msl, use_ios_token);

  // remove unused resources
  auto resources = msl.get_shader_resources(msl.get_active_interface_variables());

  for (auto resIt = resourceMap.begin(); resIt != resourceMap.end(); /*empty*/)
  {
    std::string name = resIt->first;
    bool has = HasResource(resources.uniform_buffers, name) || HasResource(resources.storage_buffers, name) ||
               HasResource(resources.separate_images, name) || HasResource(resources.storage_images, name) || name == "$Global";
    has |= HasResource(resources.separate_samplers, name);
    if (has)
      resIt++;
    else
      resIt = resourceMap.erase(resIt);
  }

  spirv_cross::MSLResourceBinding bind = {};
  bind.stage =
    profile[0] == 'p' ? spv::ExecutionModelFragment : (profile[0] == 'c' ? spv::ExecutionModelGLCompute : spv::ExecutionModelVertex);
  uint32_t sampler = 0;
  for (const auto &[_, resource] : resourceMap)
  {
    bind.desc_set = resource.set;
    bind.binding = resource.bindPoint;
    if (resource.isTexture)
      bind.msl_texture = resource.remappedBindPoint;
    else if (resource.isSampler)
      bind.msl_sampler = sampler;
    else
      bind.msl_buffer = resource.remappedBindPoint;
    msl.add_msl_resource_binding(bind);
    if (!resource.isTexture && !resource.isSampler)
    {
      drv3d_metal::BufferType type = resource.set == T_REGISTER_SET
                                       ? drv3d_metal::STRUCT_BUFFER
                                       : (resource.set == U_REGISTER_SET ? drv3d_metal::RW_BUFFER : drv3d_metal::CONST_BUFFER);
      if (resource.bindPoint != 0 || type != drv3d_metal::CONST_BUFFER)
        bufferRemap[drv3d_metal::RemapBufferSlot(type, resource.bindPoint)] = resource.remappedBindPoint;
    }
    if (resource.isTexture)
    {
      int slot = resource.set == U_REGISTER_SET ? drv3d_metal::MAX_SHADER_TEXTURES + resource.bindPoint : resource.bindPoint;
      textureRemap[slot] = resource.remappedBindPoint;
    }
    if (resource.isSampler)
    {
      samplerRemap[resource.bindPoint] = sampler++;
    }
  }

  std::string msource;
  try
  {
    msource = msl.compile();
  }
  catch (std::exception &e)
  {
    compile_result.errors.sprintf("Spir-V to MSL compilation failed:\n %s", e.what());
    return compile_result;
  }

  if (targetStage == EShLangCompute)
  {
    std::stringstream errors;
    if (!msl.validate(errors))
    {
      compile_result.errors.sprintf("Spir-V to MSL LDS size validation failed \n%s\n", errors.str().c_str());
      return compile_result;
    }
  }

  String mtl_src = prependMetaDataAndReplaceFuncs(msource, shader_name, entry, index);

  DataAccumulator header;

  header.appendValue(shaderType);

  header.appendStr(entry, 96);

  header.appendContainer(bufferRemap);

  spirv_cross::ShaderResources res = msl.get_shader_resources(msl.get_active_interface_variables());
  if (targetStage == EShLangVertex)
  {
    std::vector<VsInputs> inputs;
    inputs.reserve(res.stage_inputs.size());
    for (const auto &stgInput : res.stage_inputs)
    {
      G_ASSERT(inputMap.find(stgInput.name) != inputMap.end()); // Every stage input should be present in map

      const auto &vsInput = inputMap.at(stgInput.name);
      const auto &membertype = msl.get_type(stgInput.base_type_id);
      G_ASSERT(vsInput.vecsize == membertype.vecsize);

      inputs.push_back(vsInput);
    }

    const int in_size = (int)inputs.size();
    header.appendValue(in_size);

    if (!inputs.empty())
    {
      sort(begin(inputs), end(inputs), [](auto &a, auto &b) { return a.reg < b.reg; });

      for (const auto &i : inputs)
      {
        header.appendValue(i.reg);

        header.appendValue(i.vecsize);

        header.appendValue(i.vsdr);
      }
    }
  }

  if (shaderType == ShaderType::Compute)
  {
    header.appendValue(cx);

    header.appendValue(cy);

    header.appendValue(cz);

    constexpr int accelerationStructureCount = 0; // Glslang doesn't support raytracing
    header.appendValue(accelerationStructureCount);
  }

  int num_reg = 0;
  if (res.uniform_buffers.size() > 0)
  {
    for (const auto &buff : res.uniform_buffers)
    {
      auto &type = msl.get_type(buff.base_type_id);
      auto slot = msl.get_decoration(buff.id, spv::DecorationBinding);
      if (slot != 0)
        continue;

      num_reg = msl.get_member_decoration(type.self, int(type.member_types.size() - 1), spv::DecorationOffset) / 16;

      auto &membertype = msl.get_type(type.member_types.back());

      int sz = membertype.columns;
      if (membertype.basetype == spirv_cross::SPIRType::Struct)
      {
        auto &lm = msl.get_type(membertype.member_types.back());
        sz = msl.get_member_decoration(membertype.self, int(membertype.member_types.size() - 1), spv::DecorationOffset) / 16;
        sz += lm.array.size() == 0 ? lm.columns : lm.columns * lm.array[0];
      }

      if (membertype.array.size() == 0)
      {
        num_reg += sz;
      }
      else
      {
        num_reg += sz * membertype.array[0];
      }
    }
  }

  header.appendValue(num_reg);

  const int imageCount = (int)res.separate_images.size() + (int)res.storage_images.size();
  header.appendValue(imageCount);

  saveTexuredata(msl, header, res.storage_images, textureRemap, drv3d_metal::MAX_SHADER_TEXTURES);
  saveTexuredata(msl, header, res.separate_images, textureRemap, 0);

  header.appendValue((int)res.separate_samplers.size());

  for (const auto &sampler : res.separate_samplers)
  {
    int slot = msl.get_decoration(sampler.id, spv::DecorationBinding);
    G_ASSERT(slot >= 0);
    header.appendValue(slot);

    int internalSlot = samplerRemap[slot];
    G_ASSERT(internalSlot >= 0);
    header.appendValue(internalSlot);
  }

  spitfile(entry, "metal", index, (void *)mtl_src.data(), (int)data_size(mtl_src));

  msource.assign(mtl_src.data());

  save2Lib(msource);

  string_view sourceToCompress = {mtl_src.c_str(), mtl_src.size()};

#ifdef __APPLE__
  std::vector<uint8_t> mtl_bin;
  if (use_binary_msl)
  {
    mtl_bin = compileMSL(mtl_src.data(), use_ios_token);
    if (mtl_bin.empty())
    {
      debug("metal source:\n%s\n", mtl_src.c_str());
      compile_result.errors.sprintf("MSL binary compilation failed:%s\n", mtl_src.c_str());
      return compile_result;
    }

    sourceToCompress = {reinterpret_cast<const char *>(mtl_bin.data()), mtl_bin.size()};
  }
#endif

  compressData(compile_result, header, sourceToCompress);
  return compile_result;
}
