// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <SPIRV/GlslangToSpv.h>
#include <spirv/compiler.h>
#include <memory>
#include <sstream>

using namespace std;

namespace
{
// values are the default values of glslang
static TBuiltInResource Resources = //
  {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,

    /* .limits = */
    {
      /* .nonInductiveForLoops = */ 1,
      /* .whileLoops = */ 1,
      /* .doWhileLoops = */ 1,
      /* .generalUniformIndexing = */ 1,
      /* .generalAttributeMatrixVectorIndexing = */ 1,
      /* .generalVaryingIndexing = */ 1,
      /* .generalSamplerIndexing = */ 1,
      /* .generalVariableIndexing = */ 1,
      /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

struct GlSlangLoader
{
  GlSlangLoader() { glslang::InitializeProcess(); }

  ~GlSlangLoader() { glslang::FinalizeProcess(); }
};

GlSlangLoader &get_loader()
{
  static GlSlangLoader glSlangLoader;
  return glSlangLoader;
}

struct InputLocationEntry
{
  const char *name;
  uint32_t loc;
};

static const InputLocationEntry inputMappingTable[] = //
  {{"POSITION", 0x0}, {"POSITION0", 0x0}, {"NORMAL", 0x2}, {"PSIZE", 0x5}, {"COLOR", 0x3}, {"TEXCOORD0", 0x8}, {"TEXCOORD1", 0x9},
    {"TEXCOORD2", 0xA}, {"TEXCOORD3", 0xB}, {"TEXCOORD4", 0xC}, {"TEXCOORD5", 0xD}, {"TEXCOORD6", 0xE}, {"TEXCOORD7", 0xF},
    {"TEXCOORD8", 0x5}, {"TEXCOORD9", 0x2}, {"TEXCOORD10", 0x3}, {"TEXCOORD11", 0x4}, {"TEXCOORD12", 0x1}, {"TEXCOORD13", 0x6},
    {"TEXCOORD14", 0x7}, {"NORMAL0", 0x2}, {"COLOR0", 0x3}, {"COLOR1", 0x4}, {"PSIZE0", 0x5}};

static const size_t inputMappingTableFirstNonPos = 2;
static const size_t inputMappingTableLength = sizeof(inputMappingTable) / sizeof(inputMappingTable[0]);

enum class ResolveErrorCode
{
  // Index into the register was out of valid range.
  // Those ranges are arbitrary and can be adjusted as needed,
  // header types for register index may need to be changed.
  REGISTER_INDEX_OUT_OF_RANGE = 400,
  // Per resource limit was hit, those are hardware related limits
  OUT_OF_RESOURCE_TYPE_SLOTS = 401,
  // Total number of resources hit the limit, this can be increased
  // until a relative high hardware limit is reached
  OUT_OF_RESOURCE_SLOTS = 402,
  // Render/Compiler needs an update to support used type
  UNSUPPORTED_TYPE = 403,
  // Unrecognized semantic name, unable to map to slot index
  INVALID_SEMANTIC_NAME = 404,
  // Resolver error, needs a bug fix if hit
  BINDING_RESOLVE_FAILED = 405,
  // Same as above
  SET_RESOLVE_FAILED = 406,
};

enum class ResolveRangeType
{
  B_REGISTER,
  T_REGISTER,
  U_REGISTER,
  COLOR_TARGET,
};

enum class ResolveCountType
{
  UNIFORM_BUFFER,
  STORAGE_BUFFER,
  STORAGE_IMAGE,
  SAMPLED_IMAGE,
  T_REGISTER,
  U_REGISTER,
};

enum class SourceRegisterType
{
  B,
  T,
  U,

  ERR = -1
};

ostream &operator<<(ostream &os, ResolveErrorCode rec)
{
  os << "[R" << static_cast<uint32_t>(rec) << "]";
  return os;
}

ostream &operator<<(ostream &os, ResolveRangeType rrt)
{
  switch (rrt)
  {
    case ResolveRangeType::B_REGISTER: os << "b register"; break;
    case ResolveRangeType::T_REGISTER: os << "t register"; break;
    case ResolveRangeType::U_REGISTER: os << "u register"; break;
    case ResolveRangeType::COLOR_TARGET: os << "color target"; break;
    default: os << "<! ResolveRangeType::(" << static_cast<uint32_t>(rrt) << ") !>"; break;
  }
  return os;
}

ostream &operator<<(ostream &os, ResolveCountType rct)
{
  switch (rct)
  {
    case ResolveCountType::UNIFORM_BUFFER: os << "uniform buffer"; break;
    case ResolveCountType::STORAGE_BUFFER: os << "storage buffer"; break;
    case ResolveCountType::STORAGE_IMAGE: os << "storage image"; break;
    case ResolveCountType::SAMPLED_IMAGE: os << "sampled image"; break;
    case ResolveCountType::T_REGISTER: os << "t register"; break;
    case ResolveCountType::U_REGISTER: os << "u register"; break;
    default: os << "<! ResolveCountType::(" << static_cast<uint32_t>(rct) << ") !>"; break;
  }
  return os;
}

class BasicIoResolver : public glslang::TIoMapResolver
{
protected:
  spirv::ShaderHeader &header;
  stringstream messageLog;
  bool isOk = true;
  // if this is set to true, then the location index of all in and output varyings are kept as is.
  // Needed to support direct GLSL loading for debug builds of the backend (used in step test).
  bool varyingPassThrough = false;
  vector<const glslang::TType *> compiled;

  ostream &err()
  {
    isOk = false;
    return messageLog;
  }

  bool checkRange(ResolveErrorCode ec, ResolveRangeType range, uint32_t count, uint32_t index)
  {
    if (index >= count)
    {
      err() << ec << " Index " << index << " into " << range << " is out of range"
            << " valid range is [0-" << count << ")" << endl;
      return false;
    }
    return true;
  }

  bool checkCount(ResolveErrorCode ec, ResolveCountType type, uint32_t count, uint32_t index)
  {
    if (index >= count)
    {
      err() << ec << " Too many " << type << "(s), limit is " << count << endl;
      return false;
    }
    return true;
  }

  bool invalidType(ResolveErrorCode ec, const char *name)
  {
    err() << ec << " Invalid type " << name << endl;
    return false;
  }
  bool invalidType(ResolveErrorCode ec, const string &name)
  {
    err() << ec << " Invalid type " << name << endl;
    return false;
  }

  bool invalidSemanticName(ResolveErrorCode ec, const char *name, bool is_in)
  {
    err() << ec << " Invalid semantic name " << name << " for " << (is_in ? "in" : "out") << "varying" << endl;
    return false;
  }

  bool resolveFailed(ResolveErrorCode ec)
  {
    err() << ec << " Resolve failed" << endl;
    return false;
  }

  string storageTypeName(const glslang::TType &type)
  {
    // FIXME: incorrect, returns struct name instead of "uniform buffer"
    return type.getTypeName().c_str();
  }

  bool isAtomicCounterMemberName(const glslang::TString &member_name, const glslang::TString &struct_name)
  {
    // generated by hlslcc
    static const char hlsl_atomic_counter_postifx[] = "_counter";
    if (!member_name.compare(0, struct_name.length(), struct_name) &&
        !member_name.compare(struct_name.length(), sizeof(hlsl_atomic_counter_postifx) - 1, hlsl_atomic_counter_postifx))
      return true;
    // generated by glslang when compiling hlsl
    static const char glslang_atomic_counter_postifx[] = "@count";
    if (!member_name.compare(member_name.length() - sizeof(glslang_atomic_counter_postifx) - 1,
          sizeof(glslang_atomic_counter_postifx) - 1, glslang_atomic_counter_postifx))
      return true;

    return false;
  }

  VkDescriptorType getDescriptorType(const glslang::TType &type)
  {
    if (type.getBasicType() == glslang::EbtSampler)
    {
      const glslang::TSampler &sampler = type.getSampler();
      if (sampler.isTexture())
      {
        switch (sampler.dim)
        {
          case glslang::Esd1D:
          case glslang::Esd2D:
          case glslang::Esd3D:
          case glslang::EsdCube: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          case glslang::EsdBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
          case glslang::EsdRect:
          case glslang::EsdSubpass:
          case glslang::EsdNone: break;
        }
      }
      else if (sampler.isImage())
      {
        switch (sampler.dim)
        {
          case glslang::Esd1D:
          case glslang::Esd2D:
          case glslang::Esd3D:
          case glslang::EsdCube: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
          case glslang::EsdBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
          case glslang::EsdRect:
          case glslang::EsdSubpass:
          case glslang::EsdNone: break;
        }
      }
    }
    else if (type.getQualifier().storage == glslang::EvqBuffer)
    {
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    else if (type.getQualifier().storage == glslang::EvqUniform)
    {
      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    }

    invalidType(ResolveErrorCode::UNSUPPORTED_TYPE, storageTypeName(type));
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
  }

  VkImageViewType getImageViewType(const glslang::TType &type)
  {
    if (type.getBasicType() == glslang::EbtSampler)
    {
      const glslang::TSampler &sampler = type.getSampler();
      if (sampler.isTexture())
      {
        switch (sampler.dim)
        {
          case glslang::Esd1D: return sampler.isArrayed() ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
          case glslang::Esd2D: return sampler.isArrayed() ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
          case glslang::Esd3D: return VK_IMAGE_VIEW_TYPE_3D;
          case glslang::EsdCube: return sampler.isArrayed() ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
          case glslang::EsdBuffer:
          case glslang::EsdRect:
          case glslang::EsdSubpass:
          case glslang::EsdNone: break;
        }
      }
      else if (sampler.isImage())
      {
        switch (sampler.dim)
        {
          case glslang::Esd1D: return sampler.isArrayed() ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
          case glslang::Esd2D: return sampler.isArrayed() ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
          case glslang::Esd3D: return VK_IMAGE_VIEW_TYPE_3D;
          case glslang::EsdCube: return sampler.isArrayed() ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
          case glslang::EsdBuffer:
          case glslang::EsdRect:
          case glslang::EsdSubpass:
          case glslang::EsdNone: break;
        }
      }
    }
    return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
  }

  bool hasAtomicCounter(const glslang::TType &type)
  {
    if (type.getQualifier().storage == glslang::EvqBuffer)
    {
      if (!type.getQualifier().readonly)
      {
        switch (type.getQualifier().declaredBuiltIn)
        {
          default:
          case glslang::EbvRWStructuredBuffer:
          case glslang::EbvRWByteAddressBuffer:
          case glslang::EbvNone:
            // may has a counter
            // TODO: this breaks if the way how atomic counters for buffers are stored
            // changes!
            if (type.getStruct() && type.getStruct()->size() > 1)
            {
              const glslang::TTypeLoc &mayBeCounter = (*type.getStruct())[0];
              const glslang::TType *counterType = mayBeCounter.type;
              if (counterType->getBasicType() == glslang::EbtUint || counterType->getBasicType() == glslang::EbtInt)
              {
                if (isAtomicCounterMemberName(counterType->getFieldName(), type.getTypeName()))
                {
                  const glslang::TTypeLoc &dataStore = (*type.getStruct())[1];
                  const glslang::TType *dataType = dataStore.type;
                  if (dataType->getQualifier().layoutOffset == spirv::ATOMIC_COUNTER_REGION_SIZE)
                  {
                    return true;
                  }
                }
              }
            }
            return false;
          // has always a counter
          case glslang::EbvAppendConsume: return true;
          // has never a counter
          case glslang::EbvStructuredBuffer:
          case glslang::EbvByteAddressBuffer: return false;
        }
      }
    }
    return false;
  }

  uint32_t getRegisterIndexBase(const glslang::TType &type)
  {
    if (type.getBasicType() == glslang::EbtSampler)
    {
      const glslang::TSampler &sampler = type.getSampler();
      if (sampler.isTexture())
      {
        switch (sampler.dim)
        {
          case glslang::Esd1D:
          case glslang::Esd2D:
          case glslang::Esd3D:
          case glslang::EsdCube:
            return sampler.isShadow() ? spirv::T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET : spirv::T_SAMPLED_IMAGE_OFFSET;
          case glslang::EsdBuffer: return spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET;
          case glslang::EsdRect:
          case glslang::EsdSubpass:
          case glslang::EsdNone: break;
        }
      }
      else if (sampler.isImage())
      {
        switch (sampler.dim)
        {
          case glslang::Esd1D:
          case glslang::Esd2D:
          case glslang::Esd3D:
          case glslang::EsdCube: return spirv::U_IMAGE_OFFSET;
          case glslang::EsdBuffer: return spirv::U_BUFFER_IMAGE_OFFSET; break;
          case glslang::EsdRect:
          case glslang::EsdSubpass:
          case glslang::EsdNone: break;
        }
      }
    }
    else if (type.getQualifier().storage == glslang::EvqBuffer)
    {
      if (!type.getQualifier().readonly)
        return spirv::U_BUFFER_OFFSET;
      else
        return spirv::T_BUFFER_OFFSET;
    }
    else if (type.getQualifier().storage == glslang::EvqUniform)
    {
      return spirv::B_CONST_BUFFER_OFFSET;
    }

    invalidType(ResolveErrorCode::UNSUPPORTED_TYPE, storageTypeName(type));
    return -1;
  }
  SourceRegisterType getSourceRegisterType(const glslang::TType &type)
  {
    if (type.getBasicType() == glslang::EbtSampler)
    {
      const glslang::TSampler &sampler = type.getSampler();
      if (sampler.isTexture())
      {
        return SourceRegisterType::T;
      }
      else if (sampler.isImage())
      {
        return SourceRegisterType::U;
      }
    }
    else if (type.getQualifier().storage == glslang::EvqBuffer)
    {
      if (type.getQualifier().readonly)
      {
        return SourceRegisterType::T;
      }
      else
      {
        return SourceRegisterType::U;
      }
    }
    else if (type.getQualifier().storage == glslang::EvqUniform)
    {
      return SourceRegisterType::B;
    }
    else
    {
      invalidType(ResolveErrorCode::UNSUPPORTED_TYPE, storageTypeName(type));
    }
    return SourceRegisterType::ERR;
  }

  struct CounterInfo
  {
    uint32_t uniformBufferCount = 0;
    uint32_t sampledImageCount = 0;
    uint32_t storageBufferCount = 0;
    uint32_t storageImageCount = 0;
    uint32_t descriptorCounts[spirv::SHADER_HEADER_DECRIPTOR_COUNT_SIZE] = {};
  };

  bool checkCheckTotalRegisterCount(const glslang::TType &type, SourceRegisterType set)
  {
    switch (set)
    {
      case SourceRegisterType::B:
        return checkCount(ResolveErrorCode::OUT_OF_RESOURCE_SLOTS, ResolveCountType::UNIFORM_BUFFER, spirv::REGISTER_ENTRIES,
          header.registerCount);
      case SourceRegisterType::T:
        return checkCount(ResolveErrorCode::OUT_OF_RESOURCE_SLOTS, ResolveCountType::T_REGISTER, spirv::REGISTER_ENTRIES,
          header.registerCount);
      case SourceRegisterType::U:
        return checkCount(ResolveErrorCode::OUT_OF_RESOURCE_SLOTS, ResolveCountType::U_REGISTER, spirv::REGISTER_ENTRIES,
          header.registerCount);
      default: return invalidType(ResolveErrorCode::UNSUPPORTED_TYPE, storageTypeName(type));
    }
  }

  bool checkRegisterIndex(SourceRegisterType set, uint32_t binding_index)
  {
    switch (set)
    {
      case SourceRegisterType::B:
        return checkRange(ResolveErrorCode::REGISTER_INDEX_OUT_OF_RANGE, ResolveRangeType::B_REGISTER, spirv::B_REGISTER_INDEX_MAX,
          binding_index);
      case SourceRegisterType::T:
        return checkRange(ResolveErrorCode::REGISTER_INDEX_OUT_OF_RANGE, ResolveRangeType::T_REGISTER, spirv::T_REGISTER_INDEX_MAX,
          binding_index);
      case SourceRegisterType::U:
        return checkRange(ResolveErrorCode::REGISTER_INDEX_OUT_OF_RANGE, ResolveRangeType::U_REGISTER, spirv::U_REGISTER_INDEX_MAX,
          binding_index);
      default: return false;
    }
  }

  bool checkAndUpdateCounters(VkDescriptorType desc_type, CounterInfo &counter_info)
  {
    switch (desc_type)
    {
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        if (checkCount(ResolveErrorCode::OUT_OF_RESOURCE_TYPE_SLOTS, ResolveCountType::STORAGE_BUFFER,
              spirv::platform::MAX_STORAGE_BUFFERS_PER_STAGE, counter_info.storageBufferCount))
        {
          ++counter_info.storageBufferCount;
          return true;
        }
        break;
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        if (checkCount(ResolveErrorCode::OUT_OF_RESOURCE_TYPE_SLOTS, ResolveCountType::STORAGE_IMAGE,
              spirv::platform::MAX_STORAGE_IMAGES_PER_STAGE, counter_info.storageImageCount))
        {
          ++counter_info.storageImageCount;
          return true;
        }
        break;
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        if (checkCount(ResolveErrorCode::OUT_OF_RESOURCE_TYPE_SLOTS, ResolveCountType::SAMPLED_IMAGE,
              spirv::platform::MAX_SAMPLED_IMAGES_PER_STAGE, counter_info.sampledImageCount))
        {
          ++counter_info.sampledImageCount;
          return true;
        }
        break;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        if (checkCount(ResolveErrorCode::OUT_OF_RESOURCE_TYPE_SLOTS, ResolveCountType::UNIFORM_BUFFER,
              spirv::platform::MAX_UNIFORM_BUFFERS_PER_STAGE, counter_info.uniformBufferCount))
        {
          ++counter_info.uniformBufferCount;
          return true;
        }
        break;
    }
    return true;
  }

  void compileResource(const glslang::TType &type, CounterInfo &counter_info)
  {
    const SourceRegisterType setIndex = getSourceRegisterType(type);
    const VkDescriptorType descType = getDescriptorType(type);
    const uint32_t bindingIndex = type.getQualifier().layoutBinding;
    if (!checkCheckTotalRegisterCount(type, setIndex))
      return;
    if (!checkRegisterIndex(setIndex, bindingIndex))
      return;
    if (!checkAndUpdateCounters(descType, counter_info))
      return;

    const VkImageViewType imageViewType = getImageViewType(type);
    const uint32_t registerBase = getRegisterIndexBase(type);

    header.descriptorTypes[header.registerCount] = descType;
    header.imageViewTypes[header.registerCount] = imageViewType;
    header.registerIndex[header.registerCount] = registerBase + bindingIndex;

    switch (registerBase)
    {
      case spirv::T_SAMPLED_IMAGE_OFFSET:
      case spirv::T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET: header.imageCheckIndices[header.imageCount++] = header.registerCount;
      case spirv::U_IMAGE_OFFSET: break;
      case spirv::U_BUFFER_IMAGE_OFFSET:
      case spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET: header.bufferViewCheckIndices[header.bufferViewCount++] = header.registerCount; break;
      case spirv::T_BUFFER_OFFSET:
      case spirv::U_BUFFER_OFFSET:
      case spirv::B_CONST_BUFFER_OFFSET: header.constBufferCheckIndices[header.constBufferCount++] = header.registerCount; break;
    }

    switch (registerBase)
    {
      case spirv::T_SAMPLED_IMAGE_OFFSET:
        switch (imageViewType)
        {
          case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
          case VK_IMAGE_VIEW_TYPE_1D: header.missingTableIndex[header.registerCount] = spirv::MISSING_IS_FATAL_INDEX; break;
          case VK_IMAGE_VIEW_TYPE_2D: header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_2D_INDEX; break;
          case VK_IMAGE_VIEW_TYPE_3D: header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_3D_INDEX; break;
          case VK_IMAGE_VIEW_TYPE_CUBE:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_CUBE_INDEX;
            break;
          case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_2D_ARRAY_INDEX;
            break;
          case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_CUBE_ARRAY_INDEX;
            break;
        }
        break;
      case spirv::T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET:
        switch (imageViewType)
        {
          case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
          case VK_IMAGE_VIEW_TYPE_1D: header.missingTableIndex[header.registerCount] = spirv::MISSING_IS_FATAL_INDEX; break;
          case VK_IMAGE_VIEW_TYPE_2D:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX;
            break;
          case VK_IMAGE_VIEW_TYPE_3D:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX;
            break;
          case VK_IMAGE_VIEW_TYPE_CUBE:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_INDEX;
            break;
          case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_ARRAY_INDEX;
            break;
          case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_ARRAY_INDEX;
            break;
        }
        break;
      case spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET:
        header.missingTableIndex[header.registerCount] = spirv::MISSING_BUFFER_SAMPLED_IMAGE_INDEX;
        break;
      case spirv::T_BUFFER_OFFSET: header.missingTableIndex[header.registerCount] = spirv::MISSING_BUFFER_INDEX; break;
      case spirv::U_IMAGE_OFFSET:
        switch (imageViewType)
        {
          default:
          case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
          case VK_IMAGE_VIEW_TYPE_1D: header.missingTableIndex[header.registerCount] = spirv::MISSING_IS_FATAL_INDEX; break;
          case VK_IMAGE_VIEW_TYPE_2D: header.missingTableIndex[header.registerCount] = spirv::MISSING_STORAGE_IMAGE_2D_INDEX; break;
          case VK_IMAGE_VIEW_TYPE_3D: header.missingTableIndex[header.registerCount] = spirv::MISSING_STORAGE_IMAGE_3D_INDEX; break;
          case VK_IMAGE_VIEW_TYPE_CUBE:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_STORAGE_IMAGE_CUBE_INDEX;
            break;
          case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_STORAGE_IMAGE_2D_ARRAY_INDEX;
            break;
          case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
            header.missingTableIndex[header.registerCount] = spirv::MISSING_STORAGE_IMAGE_CUBE_ARRAY_INDEX;
            break;
        }
        break;
      case spirv::U_BUFFER_IMAGE_OFFSET:
        header.missingTableIndex[header.registerCount] = spirv::MISSING_STORAGE_BUFFER_SAMPLED_IMAGE_INDEX;
        break;
      case spirv::U_BUFFER_OFFSET: header.missingTableIndex[header.registerCount] = spirv::MISSING_STORAGE_BUFFER_INDEX; break;
      case spirv::B_CONST_BUFFER_OFFSET:
        header.missingTableIndex[header.registerCount] =
          (0 == bindingIndex) ? spirv::FALLBACK_TO_C_GLOBAL_BUFFER : spirv::MISSING_CONST_BUFFER_INDEX;
        break;
    }
    uint32_t descTypeIndex = spirv::descrior_type_to_index(descType);
    if (descTypeIndex < (sizeof(counter_info.descriptorCounts) / sizeof(counter_info.descriptorCounts[0])))
      ++counter_info.descriptorCounts[descTypeIndex];

    switch (setIndex)
    {
      case SourceRegisterType::B: header.bRegisterUseMask |= 1ul << bindingIndex; break;
      case SourceRegisterType::T: header.tRegisterUseMask |= 1ul << bindingIndex; break;
      case SourceRegisterType::U:
        header.uRegisterUseMask |= 1ul << bindingIndex;
        break;
        // silence compiler/sa - this can never reached, all tests beforehand will
        // stop the remapping process
      case SourceRegisterType::ERR: break;
    }

    ++header.registerCount;
  }

  void compileDescriptorCountTable(const CounterInfo &counter_info)
  {
    for (uint32_t i = 0; i < spirv::SHADER_HEADER_DECRIPTOR_COUNT_SIZE; ++i)
    {
      if (0 == counter_info.descriptorCounts[i])
        continue;

      VkDescriptorPoolSize &target = header.descriptorCounts[header.descriptorCountsCount++];
      target.type = spirv::descriptor_index_to_type(i);
      target.descriptorCount = counter_info.descriptorCounts[i];
    }
  }

public:
  bool hasError() const { return !isOk; }
  string getMessageLog() const { return messageLog.str(); }
  BasicIoResolver(spirv::ShaderHeader &h, bool varying_pass_through) : header(h), varyingPassThrough(varying_pass_through) {}
  // Should return true if the resulting/current binding would be okay.
  // Basic idea is to do aliasing binding checks with this.
  bool validateBinding(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override
  {
    SourceRegisterType setIndex = getSourceRegisterType(type);
    ResolveRangeType rangeType = ResolveRangeType::B_REGISTER;
    uint32_t rangeLimit = 0;
    auto bindingIndex = type.getQualifier().layoutBinding;
    if (SourceRegisterType::B == setIndex)
    {
      rangeType = ResolveRangeType::B_REGISTER;
      rangeLimit = spirv::B_REGISTER_INDEX_MAX;
    }
    else if (SourceRegisterType::T == setIndex)
    {
      rangeType = ResolveRangeType::T_REGISTER;
      rangeLimit = spirv::T_REGISTER_INDEX_MAX;
    }
    else if (SourceRegisterType::U == setIndex)
    {
      rangeType = ResolveRangeType::U_REGISTER;
      rangeLimit = spirv::U_REGISTER_INDEX_MAX;
    }
    else
    {
      return invalidType(ResolveErrorCode::UNSUPPORTED_TYPE, storageTypeName(type));
    }
    if (!checkRange(ResolveErrorCode::REGISTER_INDEX_OUT_OF_RANGE, rangeType, rangeLimit, bindingIndex))
      return false;

    bindingIndex += getRegisterIndexBase(type);
    for (uint32_t i = 0; i < header.registerCount; ++i)
      if (header.registerIndex[i] == bindingIndex)
        return true;

    return invalidType(ResolveErrorCode::UNSUPPORTED_TYPE, storageTypeName(type));
  }
  // Should return a value >= 0 if the current binding should be overridden.
  // Return -1 if the current binding (including no binding) should be kept.
  int resolveBinding(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override
  {
    SourceRegisterType setIndex = getSourceRegisterType(type);
    auto bindingIndex = type.getQualifier().layoutBinding;
    if ((SourceRegisterType::B == setIndex) || (SourceRegisterType::T == setIndex) || (SourceRegisterType::U == setIndex))
    {
      bindingIndex += getRegisterIndexBase(type);
      for (uint32_t i = 0; i < header.registerCount; ++i)
        if (header.registerIndex[i] == bindingIndex)
          return i;
    }
    resolveFailed(ResolveErrorCode::BINDING_RESOLVE_FAILED);
    return -1;
  }
  // Should return a value >= 0 if the current location should be overridden.
  // Return -1 if the current location (including no location) should be kept.
  int resolveUniformLocation(EShLanguage, const char *, const glslang::TType &, bool) override { return -1; }
  // Notification of a uniform variable
  void notifyBinding(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override { compiled.push_back(&type); }
  // Called by mapIO when it has finished the notify pass
  void endNotifications(EShLanguage) override
  {
    // sort first, so that the meta data of shaders with the same layout is exactly the
    // same
    sort(begin(compiled), end(compiled),
      [this](const glslang::TType *l, const glslang::TType *r) //
      {
        const auto li = static_cast<uint32_t>(getSourceRegisterType(*l));
        const auto ri = static_cast<uint32_t>(getSourceRegisterType(*r));
        const auto lb = l->getQualifier().layoutBinding;
        const auto rb = r->getQualifier().layoutBinding;

        if (li < ri)
          return true;
        else if (li > ri)
          return false;
        else
          return lb < rb;
      });

    CounterInfo counterInfo;

    // check all entries and build header
    for (auto &&t : compiled)
      compileResource(*t, counterInfo);

    compileDescriptorCountTable(counterInfo);
  }
  // Called by mapIO when it starts its notify pass for the given stage
  void beginNotifications(EShLanguage) override {}
  // Called by mipIO when it starts its resolve pass for the given stage
  void beginResolve(EShLanguage) override {}
  // Called by mapIO when it has finished the resolve pass
  void endResolve(EShLanguage) override {}
};

class ComputeShaderIoResolver final : public BasicIoResolver
{
public:
  using BasicIoResolver::BasicIoResolver;
  // Should return a value >= 0 if the current set should be overridden.
  // Return -1 if the current set (including no set) should be kept.
  int resolveSet(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override
  {
    return spirv::compute::REGISTERS_SET_INDEX;
  }
  // Should return true if the resulting/current setup would be okay.
  // Basic idea is to do aliasing checks and reject invalid semantic names.
  bool validateInOut(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override
  {
    // return true here, or we reject buildin stuff like workgroup size
    return true;
  }
  // Should return a value >= 0 if the current location should be overridden.
  // Return -1 if the current location (including no location) should be kept.
  int resolveInOutLocation(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override { return -1; }
  // Should return a value >= 0 if the current component index should be overridden.
  // Return -1 if the current component index (including no index) should be kept.
  int resolveInOutComponent(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override { return -1; }
  // Should return a value >= 0 if the current color index should be overridden.
  // Return -1 if the current color index (including no index) should be kept.
  int resolveInOutIndex(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override { return -1; }
  // Notification of a in or out variable
  void notifyInOut(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override {}
};

class VertexShaderIoResolver final : public BasicIoResolver
{
public:
  using BasicIoResolver::BasicIoResolver;
  // Should return a value >= 0 if the current set should be overridden.
  // Return -1 if the current set (including no set) should be kept.
  int resolveSet(EShLanguage, const char *, const glslang::TType &type, bool) override
  {
    return spirv::graphics::vertex::REGISTERS_SET_INDEX;
  }
  // Should return true if the resulting/current setup would be okay.
  // Basic idea is to do aliasing checks and reject invalid semantic names.
  bool validateInOut(EShLanguage, const char *name, const glslang::TType &type, bool) override
  {
    auto semanticName = type.getQualifier().semanticName;
    if (glslang::EbvNone == type.getQualifier().builtIn && glslang::EbtBlock != type.getBasicType())
    {
      if (glslang::EvqVaryingIn == type.getQualifier().storage)
      {
        if (varyingPassThrough)
          return true;

        const char *str = semanticName ? semanticName : name + 3; // skip over in_
        for (int i = 0; i < inputMappingTableLength; ++i)
          if (!strcmp(str, inputMappingTable[i].name))
            return true;
        return invalidSemanticName(ResolveErrorCode::INVALID_SEMANTIC_NAME, str, true);
      }
      else if (glslang::EvqVaryingOut == type.getQualifier().storage)
      {
        if (varyingPassThrough)
          return true;

        const char *str = semanticName ? semanticName : name + 3; // skip over vs_
        for (int i = 0; i < inputMappingTableLength; ++i)
          if (!strcmp(str, inputMappingTable[i].name))
            return true;
        return invalidSemanticName(ResolveErrorCode::INVALID_SEMANTIC_NAME, str, false);
      }
      else
      {
        return false;
      }
    }
    return true;
  }
  // Should return a value >= 0 if the current location should be overridden.
  // Return -1 if the current location (including no location) should be kept.
  int resolveInOutLocation(EShLanguage, const char *name, const glslang::TType &type, bool) override
  {
    auto semanticName = type.getQualifier().semanticName;
    if (glslang::EbvNone == type.getQualifier().builtIn && glslang::EbtBlock != type.getBasicType())
    {
      if (glslang::EvqVaryingIn == type.getQualifier().storage)
      {
        if (varyingPassThrough)
        {
          header.inputMask |= 1ul << type.getQualifier().layoutLocation;
          return type.getQualifier().layoutLocation;
        }

        const char *str = semanticName ? semanticName : name + 3; // skip over in_
        for (int i = 0; i < inputMappingTableLength; ++i)
        {
          if (!strcmp(str, inputMappingTable[i].name))
          {
            uint32_t index = inputMappingTable[i].loc;
            header.inputMask |= 1ul << index;
            return index;
          }
        }
        resolveFailed(ResolveErrorCode::BINDING_RESOLVE_FAILED);
      }
      else if (glslang::EvqVaryingOut == type.getQualifier().storage)
      {
        if (varyingPassThrough)
        {
          header.outputMask |= 1ul << type.getQualifier().layoutLocation;
          return type.getQualifier().layoutLocation;
        }

        const char *str = semanticName ? semanticName : name + 3; // skip over vs_
        for (int i = inputMappingTableFirstNonPos; i < inputMappingTableLength; ++i)
        {
          if (!strcmp(str, inputMappingTable[i].name))
          {
            uint32_t index = i - inputMappingTableFirstNonPos;
            header.outputMask |= 1ul << index;
            return index;
          }
        }
        resolveFailed(ResolveErrorCode::BINDING_RESOLVE_FAILED);
      }
      else
      {
        return -1;
      }
    }
    return -1;
  }
  // Should return a value >= 0 if the current component index should be overridden.
  // Return -1 if the current component index (including no index) should be kept.
  int resolveInOutComponent(EShLanguage, const char *, const glslang::TType &, bool) override { return -1; }
  // Should return a value >= 0 if the current color index should be overridden.
  // Return -1 if the current color index (including no index) should be kept.
  int resolveInOutIndex(EShLanguage, const char *, const glslang::TType &, bool) override { return -1; }
  // Notification of a in or out variable
  void notifyInOut(EShLanguage, const char *, const glslang::TType &, bool) override {}
};

class FragmentShaderIoResolver final : public BasicIoResolver
{
public:
  using BasicIoResolver::BasicIoResolver;
  // Should return a value >= 0 if the current set should be overridden.
  // Return -1 if the current set (including no set) should be kept.
  int resolveSet(EShLanguage, const char *, const glslang::TType &type, bool) override
  {
    return spirv::graphics::fragment::REGISTERS_SET_INDEX;
  }
  // Should return true if the resulting/current setup would be okay.
  // Basic idea is to do aliasing checks and reject invalid semantic names.
  bool validateInOut(EShLanguage, const char *name, const glslang::TType &type, bool) override
  {
    auto semanticName = type.getQualifier().semanticName;
    if (glslang::EbvNone == type.getQualifier().builtIn && glslang::EbtBlock != type.getBasicType())
    {
      if (glslang::EvqVaryingIn == type.getQualifier().storage)
      {
        if (varyingPassThrough)
          return true;

        const char *str = semanticName ? semanticName : name + 3; // skip over vs_
        for (int i = inputMappingTableFirstNonPos; i < inputMappingTableLength; ++i)
          if (!strcmp(str, inputMappingTable[i].name))
            return true;

        return invalidSemanticName(ResolveErrorCode::INVALID_SEMANTIC_NAME, str, true);
      }
      else if (glslang::EvqVaryingOut == type.getQualifier().storage)
      {
        return checkRange(ResolveErrorCode::REGISTER_INDEX_OUT_OF_RANGE, ResolveRangeType::COLOR_TARGET,
          spirv::platform::MAX_COLOR_ATTACHMENTS, type.getQualifier().layoutLocation);
      }
      else
      {
        return false;
      }
    }
    return true;
  }
  // Should return a value >= 0 if the current location should be overridden.
  // Return -1 if the current location (including no location) should be kept.
  int resolveInOutLocation(EShLanguage, const char *name, const glslang::TType &type, bool) override
  {
    auto semanticName = type.getQualifier().semanticName;
    if (glslang::EbvNone == type.getQualifier().builtIn && glslang::EbtBlock != type.getBasicType())
    {
      if (glslang::EvqVaryingIn == type.getQualifier().storage)
      {
        if (varyingPassThrough)
        {
          header.inputMask |= 1ul << type.getQualifier().layoutLocation;
          return type.getQualifier().layoutLocation;
        }

        const char *str = semanticName ? semanticName : name + 3; // skip over vs_
        for (int i = inputMappingTableFirstNonPos; i < inputMappingTableLength; ++i)
        {
          if (!strcmp(str, inputMappingTable[i].name))
          {
            uint32_t index = i - inputMappingTableFirstNonPos;
            header.inputMask |= 1ul << index;
            return index;
          }
        }

        resolveFailed(ResolveErrorCode::BINDING_RESOLVE_FAILED);
      }
      else if (glslang::EvqVaryingOut == type.getQualifier().storage)
      {
        header.outputMask |= 1ul << type.getQualifier().layoutLocation;
        return type.getQualifier().layoutLocation;
      }
      else
      {
        return -1;
      }
    }
    return -1;
  }
  // Should return a value >= 0 if the current component index should be overridden.
  // Return -1 if the current component index (including no index) should be kept.
  int resolveInOutComponent(EShLanguage, const char *, const glslang::TType &, bool) override { return -1; }
  // Should return a value >= 0 if the current color index should be overridden.
  // Return -1 if the current color index (including no index) should be kept.
  int resolveInOutIndex(EShLanguage, const char *, const glslang::TType &, bool) override { return -1; }
  // Notification of a in or out variable
  void notifyInOut(EShLanguage, const char *, const glslang::TType &, bool) override {}
};

class GeometryShaderIoResolver final : public BasicIoResolver
{
public:
  using BasicIoResolver::BasicIoResolver;
  // Should return a value >= 0 if the current set should be overridden.
  // Return -1 if the current set (including no set) should be kept.
  int resolveSet(EShLanguage, const char *, const glslang::TType &type, bool) override
  {
    return spirv::graphics::geometry::REGISTERS_SET_INDEX;
  }
  // Should return true if the resulting/current setup would be okay.
  // Basic idea is to do aliasing checks and reject invalid semantic names.
  bool validateInOut(EShLanguage, const char *name, const glslang::TType &type, bool) override
  {
    auto semanticName = type.getQualifier().semanticName;
    if (glslang::EbvNone == type.getQualifier().builtIn && glslang::EbtBlock != type.getBasicType())
    {
      if (glslang::EvqVaryingIn == type.getQualifier().storage || glslang::EvqVaryingOut == type.getQualifier().storage)
      {
        if (varyingPassThrough)
          return true;

        const char *str = semanticName ? semanticName : name + 3; // skip over vs_ / gs_
        for (int i = inputMappingTableFirstNonPos; i < inputMappingTableLength; ++i)
          if (!strcmp(str, inputMappingTable[i].name))
            return true;
        return invalidSemanticName(ResolveErrorCode::INVALID_SEMANTIC_NAME, str, true);
      }
      else
      {
        return false;
      }
    }
    return true;
  }
  // Should return a value >= 0 if the current location should be overridden.
  // Return -1 if the current location (including no location) should be kept.
  int resolveInOutLocation(EShLanguage, const char *name, const glslang::TType &type, bool) override
  {
    auto semanticName = type.getQualifier().semanticName;
    if (glslang::EbvNone == type.getQualifier().builtIn && glslang::EbtBlock != type.getBasicType())
    {
      if (glslang::EvqVaryingIn == type.getQualifier().storage || glslang::EvqVaryingOut == type.getQualifier().storage)
      {
        if (varyingPassThrough)
        {
          if (glslang::EvqVaryingIn == type.getQualifier().storage)
            header.inputMask |= 1ul << type.getQualifier().layoutLocation;
          else if (glslang::EvqVaryingOut == type.getQualifier().storage)
            header.outputMask |= 1ul << type.getQualifier().layoutLocation;
          return type.getQualifier().layoutLocation;
        }

        const char *str = semanticName ? semanticName : name + 3; // skip over vs_ / gs_
        for (int i = inputMappingTableFirstNonPos; i < inputMappingTableLength; ++i)
        {
          if (!strcmp(str, inputMappingTable[i].name))
          {
            uint32_t index = i - inputMappingTableFirstNonPos;
            if (glslang::EvqVaryingIn == type.getQualifier().storage)
              header.inputMask |= 1ul << index;
            else if (glslang::EvqVaryingOut == type.getQualifier().storage)
              header.outputMask |= 1ul << index;
            return index;
          }
        }
        resolveFailed(ResolveErrorCode::BINDING_RESOLVE_FAILED);
      }
      else
      {
        return -1;
      }
    }
    return -1;
  }
  // Should return a value >= 0 if the current component index should be overridden.
  // Return -1 if the current component index (including no index) should be kept.
  int resolveInOutComponent(EShLanguage, const char *, const glslang::TType &, bool) override { return -1; }
  // Should return a value >= 0 if the current color index should be overridden.
  // Return -1 if the current color index (including no index) should be kept.
  int resolveInOutIndex(EShLanguage, const char *, const glslang::TType &, bool) override { return -1; }
  // Notification of a in or out variable
  void notifyInOut(EShLanguage, const char *, const glslang::TType &, bool) override {}
};

class TessControlShaderIoResolver final : public BasicIoResolver
{
public:
  using BasicIoResolver::BasicIoResolver;
  // Should return a value >= 0 if the current set should be overridden.
  // Return -1 if the current set (including no set) should be kept.
  int resolveSet(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override
  {
    return spirv::graphics::control::REGISTERS_SET_INDEX;
  }
  // Should return true if the resulting/current setup would be okay.
  // Basic idea is to do aliasing checks and reject invalid semantic names.
  bool validateInOut(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override
  {
    // return true here, or we reject buildin stuff like workgroup size
    return true;
  }
  // Should return a value >= 0 if the current location should be overridden.
  // Return -1 if the current location (including no location) should be kept.
  int resolveInOutLocation(EShLanguage, const char *name, const glslang::TType &type, bool) override
  {
    auto semanticName = type.getQualifier().semanticName;
    if (glslang::EbvNone == type.getQualifier().builtIn && glslang::EbtBlock != type.getBasicType())
    {
      if (glslang::EvqVaryingIn == type.getQualifier().storage || glslang::EvqVaryingOut == type.getQualifier().storage)
      {
        if (varyingPassThrough)
        {
          if (glslang::EvqVaryingIn == type.getQualifier().storage)
            header.inputMask |= 1ul << type.getQualifier().layoutLocation;
          else if (glslang::EvqVaryingOut == type.getQualifier().storage)
            header.outputMask |= 1ul << type.getQualifier().layoutLocation;
          return type.getQualifier().layoutLocation;
        }

        const char *str = semanticName ? semanticName : name + 3; // skip over vs_ / gs_
        for (int i = inputMappingTableFirstNonPos; i < inputMappingTableLength; ++i)
        {
          if (!strcmp(str, inputMappingTable[i].name))
          {
            uint32_t index = i - inputMappingTableFirstNonPos;
            if (glslang::EvqVaryingIn == type.getQualifier().storage)
              header.inputMask |= 1ul << index;
            else if (glslang::EvqVaryingOut == type.getQualifier().storage)
              header.outputMask |= 1ul << index;
            return index;
          }
        }
        resolveFailed(ResolveErrorCode::BINDING_RESOLVE_FAILED);
      }
      else
      {
        return -1;
      }
    }
    return -1;
  }
  // Should return a value >= 0 if the current component index should be overridden.
  // Return -1 if the current component index (including no index) should be kept.
  int resolveInOutComponent(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override { return -1; }
  // Should return a value >= 0 if the current color index should be overridden.
  // Return -1 if the current color index (including no index) should be kept.
  int resolveInOutIndex(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override { return -1; }
  // Notification of a in or out variable
  void notifyInOut(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override {}
};

class TessEvaluationShaderIoResolver final : public BasicIoResolver
{
public:
  using BasicIoResolver::BasicIoResolver;
  // Should return a value >= 0 if the current set should be overridden.
  // Return -1 if the current set (including no set) should be kept.
  int resolveSet(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override
  {
    return spirv::graphics::evaluation::REGISTERS_SET_INDEX;
  }
  // Should return true if the resulting/current setup would be okay.
  // Basic idea is to do aliasing checks and reject invalid semantic names.
  bool validateInOut(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override
  {
    // return true here, or we reject buildin stuff like workgroup size
    return true;
  }
  // Should return a value >= 0 if the current location should be overridden.
  // Return -1 if the current location (including no location) should be kept.
  int resolveInOutLocation(EShLanguage, const char *name, const glslang::TType &type, bool) override
  {
    auto semanticName = type.getQualifier().semanticName;
    if (glslang::EbvNone == type.getQualifier().builtIn && glslang::EbtBlock != type.getBasicType())
    {
      if (glslang::EvqVaryingIn == type.getQualifier().storage || glslang::EvqVaryingOut == type.getQualifier().storage)
      {
        if (varyingPassThrough)
        {
          if (glslang::EvqVaryingIn == type.getQualifier().storage)
            header.inputMask |= 1ul << type.getQualifier().layoutLocation;
          else if (glslang::EvqVaryingOut == type.getQualifier().storage)
            header.outputMask |= 1ul << type.getQualifier().layoutLocation;
          return type.getQualifier().layoutLocation;
        }

        const char *str = semanticName ? semanticName : name + 3; // skip over vs_ / gs_
        for (int i = inputMappingTableFirstNonPos; i < inputMappingTableLength; ++i)
        {
          if (!strcmp(str, inputMappingTable[i].name))
          {
            uint32_t index = i - inputMappingTableFirstNonPos;
            if (glslang::EvqVaryingIn == type.getQualifier().storage)
              header.inputMask |= 1ul << index;
            else if (glslang::EvqVaryingOut == type.getQualifier().storage)
              header.outputMask |= 1ul << index;
            return index;
          }
        }
        resolveFailed(ResolveErrorCode::BINDING_RESOLVE_FAILED);
      }
      else
      {
        return -1;
      }
    }
    return -1;
  }
  // Should return a value >= 0 if the current component index should be overridden.
  // Return -1 if the current component index (including no index) should be kept.
  int resolveInOutComponent(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override { return -1; }
  // Should return a value >= 0 if the current color index should be overridden.
  // Return -1 if the current color index (including no index) should be kept.
  int resolveInOutIndex(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override { return -1; }
  // Notification of a in or out variable
  void notifyInOut(EShLanguage, const char *name, const glslang::TType &type, bool is_live) override {}
};


spirv::CompileToSpirVResult compile(glslang::TShader &shader, EShLanguage target, EShMessages rules, bool debug_build,
  bool varying_pass_through, unsigned int SourceLangVersion)
{
  spirv::CompileToSpirVResult result = {};
  bool ok = shader.parse(&Resources, SourceLangVersion, true, rules);
  const char *infoLog = shader.getInfoLog();
  const char *debugLog = shader.getInfoDebugLog();
  if (infoLog && infoLog[0])
    result.infoLog.push_back(infoLog);
  if (debugLog && debugLog[0])
    result.infoLog.push_back(debugLog);

  if (ok)
  {
    // needs to be on the heap or it may corrupt the stack
    auto program = make_unique<glslang::TProgram>();
    program->addShader(&shader);
    ok = program->link(EShMessages(EShMsgSpvRules | EShMsgVulkanRules));
    infoLog = program->getInfoLog();
    debugLog = program->getInfoDebugLog();
    if (infoLog && infoLog[0])
      result.infoLog.push_back(infoLog);
    if (debugLog && debugLog[0])
      result.infoLog.push_back(debugLog);

    if (ok)
    {
      unique_ptr<BasicIoResolver> resolver;
      if (EShLangCompute == target)
        resolver = make_unique<ComputeShaderIoResolver>(result.header, varying_pass_through);
      else if (EShLangVertex == target)
        resolver = make_unique<VertexShaderIoResolver>(result.header, varying_pass_through);
      else if (EShLangFragment == target)
        resolver = make_unique<FragmentShaderIoResolver>(result.header, varying_pass_through);
      else if (EShLangGeometry == target)
        resolver = make_unique<GeometryShaderIoResolver>(result.header, varying_pass_through);
      else if (EShLangTessControl == target)
        resolver = make_unique<TessControlShaderIoResolver>(result.header, varying_pass_through);
      else if (EShLangTessEvaluation == target)
        resolver = make_unique<TessEvaluationShaderIoResolver>(result.header, varying_pass_through);

      if (resolver)
      {
        if (program->mapIO(resolver.get()))
        {
          glslang::SpvOptions spvOptions;
          spvOptions.generateDebugInfo = debug_build;
          glslang::GlslangToSpv(*program->getIntermediate(shader.getStage()), result.byteCode, &spvOptions);
        }
        else
        {
          result.infoLog.push_back(resolver->getMessageLog().c_str());
        }
      }
      else
      {
        result.infoLog.push_back("resolver for target stage is not implemented");
      }
    }
  }
  return result;
}
} // namespace
spirv::CompileToSpirVResult spirv::compileGLSL(dag::ConstSpan<const char *> sources, EShLanguage target, CompileFlags flags)
{
  get_loader();
  constexpr EShMessages SourceLangRules = EShMessages(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);
  constexpr unsigned int SourceLangVersion = 440;
  const bool debug_build = (flags & CompileFlags::DEBUG_BUILD) != CompileFlags::NONE;
  const bool varying_pass_through = (flags & CompileFlags::VARYING_PASS_THROUGH) != CompileFlags::NONE;
  // needs to be on the heap or it may corrupt the stack
  auto shader = make_unique<glslang::TShader>(target);
  shader->setStrings(sources.data(), sources.size());
  return compile(*shader, target, SourceLangRules, debug_build, varying_pass_through, SourceLangVersion);
}

spirv::CompileToSpirVResult spirv::compileHLSL(dag::ConstSpan<const char *> sources, const char *entry, EShLanguage target,
  CompileFlags flags)
{
  get_loader();
  constexpr EShMessages SourceLangRules =
    EShMessages(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault | EShMsgReadHlsl | EShMsgHlslOffsets);
  constexpr unsigned int SourceLangVersion = 440;
  const bool debug_build = (flags & CompileFlags::DEBUG_BUILD) != CompileFlags::NONE;
  const bool varying_pass_through = (flags & CompileFlags::VARYING_PASS_THROUGH) != CompileFlags::NONE;
  // needs to be on the heap or it may corrupt the stack
  auto shader = make_unique<glslang::TShader>(target);
  shader->setStrings(sources.data(), sources.size());
  shader->setTextureSamplerTransformMode(EShTexSampTransUpgradeTextureRemoveSampler);
  shader->setEntryPoint("main");
  shader->setSourceEntryPoint(entry);
  shader->setHlslBufferWithCounterMode(EShHlslBufferWithCounterEmbedded);
  shader->setHlslBufferWithCounterEmbeddedPayloadOffset(256);
  return compile(*shader, target, SourceLangRules, debug_build, varying_pass_through, SourceLangVersion);
}