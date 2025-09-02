// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "module_nodes.h"
#include <spirv/module_builder.h>
#include <spirv/compiler.h>

#include <EASTL/sort.h>

using namespace spirv;

namespace
{
enum SourceRegisterType
{
  B = 0,
  T = 1,
  U = 2,
  Err = -1
};

struct TaggedSamplerBind
{
  bool pairFound; // set if we found related "paired" texture bind
  NodePointer<NodeOpVariable> node;
};

SourceRegisterType get_source_register_type(NodePointer<NodeOpVariable> var)
{
  auto ptrType = as<NodeOpTypePointer>(var->resultType);

  if (ptrType->storageClass == StorageClass::UniformConstant)
  {
    auto valueType = as<NodeTypedef>(ptrType->type);
    // Handles Texture{Type}<T>, Buffer<T>, RWTexture{Type}<T> and RWBuffer<T>
    if (is<NodeOpTypeSampledImage>(valueType))
      return SourceRegisterType::T;
    else if (is<NodeOpTypeImage>(valueType))
    {
      if (as<NodeOpTypeImage>(valueType)->sampled.value == 2)
        return SourceRegisterType::U;
      return SourceRegisterType::T;
    }
    else if (is<NodeOpTypeAccelerationStructureKHR>(valueType))
      return SourceRegisterType::T;
    return SourceRegisterType::U;
  }
  else if (ptrType->storageClass == StorageClass::Uniform)
  {
    switch (get_buffer_kind(var))
    {
      case BufferKind::Uniform: return SourceRegisterType::B;
      case BufferKind::ReadOnlyStorage: return SourceRegisterType::T;
      case BufferKind::Storage: return SourceRegisterType::U;
      default:
        // error case
        break;
    }
  }
  return SourceRegisterType::Err;
}

// vulkan profile for spir-v ignores depth param (DXC generates types with depth mode 2, undefined)
// and in hlsl the info if a image is a depth image comes from the used sampler. To figure out if a
// texture needs depth compare, we need to check all consumers and see if anyone uses depth compare.
// NOTE: may auto gen list of instructions with depth compare with a script and use it here
// NOTE: this can be rewritten the other way around, from var drilling down to the consumers
//       but currently this way is faster, just look at all instructions once.
//       As soon direct refs to consumers are available then this can be redone.
eastl::vector<NodePointer<NodeOpVariable>> find_all_depth_textures(ModuleBuilder &builder, ErrorHandler &e_handler)
{
  eastl::vector<NodePointer<NodeOpVariable>> result;
  builder.enumerateAllNodes([&](auto node) //
    {
      NodePointer<NodeOperation> sampledImage;
      if (is<NodeOpImageSampleDrefImplicitLod>(node))
      {
        sampledImage = as<NodeOpImageSampleDrefImplicitLod>(node)->sampledImage;
      }
      else if (is<NodeOpImageSampleDrefExplicitLod>(node))
      {
        sampledImage = as<NodeOpImageSampleDrefExplicitLod>(node)->sampledImage;
      }
      else if (is<NodeOpImageSampleProjDrefImplicitLod>(node))
      {
        sampledImage = as<NodeOpImageSampleProjDrefImplicitLod>(node)->sampledImage;
      }
      else if (is<NodeOpImageSampleProjDrefExplicitLod>(node))
      {
        sampledImage = as<NodeOpImageSampleProjDrefExplicitLod>(node)->sampledImage;
      }
      else if (is<NodeOpImageDrefGather>(node))
      {
        sampledImage = as<NodeOpImageDrefGather>(node)->sampledImage;
      }
      else if (is<NodeOpImageSparseSampleDrefImplicitLod>(node))
      {
        sampledImage = as<NodeOpImageSparseSampleDrefImplicitLod>(node)->sampledImage;
      }
      else if (is<NodeOpImageSparseSampleDrefExplicitLod>(node))
      {
        sampledImage = as<NodeOpImageSparseSampleDrefExplicitLod>(node)->sampledImage;
      }
      else if (is<NodeOpImageSparseSampleProjDrefImplicitLod>(node))
      {
        sampledImage = as<NodeOpImageSparseSampleProjDrefImplicitLod>(node)->sampledImage;
      }
      else if (is<NodeOpImageSparseSampleProjDrefExplicitLod>(node))
      {
        sampledImage = as<NodeOpImageSparseSampleProjDrefExplicitLod>(node)->sampledImage;
      }
      else if (is<NodeOpImageSparseDrefGather>(node))
      {
        sampledImage = as<NodeOpImageSparseDrefGather>(node)->sampledImage;
      }

      if (!sampledImage)
        return;

      // we either came from direct load or sampled image composition
      NodePointer<NodeOpLoad> imageLoad;
      if (is<NodeOpSampledImage>(sampledImage))
      {
        imageLoad = as<NodeOpSampledImage>(sampledImage)->image;
      }
      else if (is<NodeOpLoad>(sampledImage))
      {
        imageLoad = sampledImage;
      }
      else
      {
        e_handler.onFatalError("compileHeader: Unexpected chain of operations while searching "
                               "depth textures");
      }

      if (imageLoad)
      {
        auto var = as<NodeOpVariable>(imageLoad->pointer);
        auto ref = eastl::find(begin(result), end(result), var);
        if (end(result) == ref)
        {
          result.push_back(var);
        }
      }
    });
  return result;
}

bool is_depth_texture(NodePointer<NodeOpVariable> var, const eastl::vector<NodePointer<NodeOpVariable>> &set)
{
  return end(set) != eastl::find(begin(set), end(set), var);
}

uint8_t find_push_constant_count(ModuleBuilder &builder, ErrorHandler &e_handler)
{
  NodePointer<NodeOpConstant> result;
  bool anyPushConstant = false;
  builder.enumerateAllTypes([&](auto type) //
    {
      // try detect push constant type pointer to struct + array, i.e. this pattern
      //%_arr_uint_uint_4 = OpTypeArray %uint %uint_4
      //%type_PushConstant_ImmDwords = OpTypeStruct %_arr_uint_uint_4
      //%_ptr_PushConstant_type_PushConstant_ImmDwords = OpTypePointer PushConstant %type_PushConstant_ImmDwords
      if (!is<NodeOpTypePointer>(type))
        return;

      auto typePtr = as<NodeOpTypePointer>(type);
      if (typePtr->storageClass != StorageClass::PushConstant)
        return;

      anyPushConstant |= true;
      if (!is<NodeOpTypeStruct>(typePtr->type))
        return;

      auto structPtr = as<NodeOpTypeStruct>(typePtr->type);
      if (structPtr->param1.size() != 1 || !is<NodeOpTypeArray>(structPtr->param1[0]))
        return;

      auto arrayPtr = as<NodeOpTypeArray>(structPtr->param1[0]);
      if (!is<NodeOpTypeInt>(arrayPtr->elementType) || !is<NodeOpConstant>(arrayPtr->length))
        return;

      auto lengthPtr = as<NodeOpConstant>(arrayPtr->length);
      if (!is<NodeOpTypeInt>(lengthPtr->resultType))
        return;

      if (lengthPtr->value.value > 255)
        return;

      if (result)
        e_handler.onFatalError("compileHeader: Multiple TypePointers with PushConstant and related requirements found, not supported");
      result = lengthPtr;
    });

  if (anyPushConstant && !result)
    e_handler.onFatalError("compileHeader: PushConstant usage detected, but size analysis failed");

  return result ? result->value.value : 0;
}

struct VariableToCompileInfo
{
  NodePointer<NodeOpVariable> var;
  PropertyBinding *binding = nullptr;
  PropertyDescriptorSet *set = nullptr;
  PropertyInputAttachmentIndex *inputAttachment = nullptr;
  SourceRegisterType registerType = SourceRegisterType::Err;
  NodePointer<NodeOpVariable> combinedSampler;
};

struct CounterInfo
{
  uint32_t uniformBufferCount = 0;
  uint32_t sampledImageCount = 0;
  uint32_t storageBufferCount = 0;
  uint32_t storageImageCount = 0;
  uint32_t descriptorCounts[SHADER_HEADER_DECRIPTOR_COUNT_SIZE] = {};
};

enum class CheckType
{
  Image,
  BufferView,
  Buffer,
  ConstBuffer,
  AccelerationStructure,
  InputAttachment
};

struct ResourceCompileResult
{
  VkDescriptorType descriptorType;
  VkImageViewType imageViewType;
  uint32_t registerBase;
  uint8_t missingIndex;
  CheckType checkType;
};

ResourceCompileResult compile_resource_image(const VariableToCompileInfo &info,
  const eastl::vector<NodePointer<NodeOpVariable>> &depth_tex_list)
{
  ResourceCompileResult result = {};

  auto ptrType = as<NodeOpTypePointer>(info.var->resultType);
  auto valueType = ptrType->type;

  result.imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
  result.missingIndex = MISSING_IS_FATAL_INDEX;

  bool isDepthImage = end(depth_tex_list) != eastl::find(begin(depth_tex_list), end(depth_tex_list), info.var);
  bool isStorage = true;
  if (is<NodeOpTypeSampledImage>(valueType))
  {
    isStorage = false;
    valueType = as<NodeOpTypeSampledImage>(valueType)->imageType;
  }
  else if (is<NodeOpTypeAccelerationStructureKHR>(valueType))
  {
    result.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    result.registerBase = T_ACCELERATION_STRUCTURE_OFFSET;
    result.checkType = CheckType::AccelerationStructure;
    result.missingIndex = MISSING_TLAS_INDEX;
    return result;
  }

  // avoid silent crash if we fed with another opcode type that will not be handled well
  G_ASSERTF(is<NodeOpTypeImage>(valueType), "unhandled opcode for resource image %u", (size_t)valueType.opCode());
  auto imageType = as<NodeOpTypeImage>(valueType);
  isStorage = imageType->sampled.value == 2;
  bool isArrayed = imageType->arrayed.value != 0;
  // patch depth field, even vulkan should ignore it, some drivers do not and crash (nswitch for
  // example)
  imageType->depth.value = isDepthImage ? 1 : 0;

  if (!isStorage)
  {
    if (imageType->dim == Dim::Buffer)
    {
      result.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
      result.registerBase = T_BUFFER_SAMPLED_IMAGE_OFFSET;
      result.missingIndex = MISSING_BUFFER_SAMPLED_IMAGE_INDEX;
    }
    else
    {
      result.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      if (!isDepthImage)
      {
        result.registerBase = T_SAMPLED_IMAGE_OFFSET;
        if (imageType->dim == Dim::Dim2D)
        {
          if (!isArrayed)
            result.missingIndex = MISSING_SAMPLED_IMAGE_2D_INDEX;
          else
            result.missingIndex = MISSING_SAMPLED_IMAGE_2D_ARRAY_INDEX;
        }
        else if (imageType->dim == Dim::Dim3D)
        {
          result.missingIndex = MISSING_SAMPLED_IMAGE_3D_INDEX;
        }
        else if (imageType->dim == Dim::Cube)
        {
          if (!isArrayed)
            result.missingIndex = MISSING_SAMPLED_IMAGE_CUBE_INDEX;
          else
            result.missingIndex = MISSING_SAMPLED_IMAGE_CUBE_ARRAY_INDEX;
        }
      }
      else
      {
        result.registerBase = T_SAMPLED_IMAGE_OFFSET;
        if (imageType->dim == Dim::Dim2D)
        {
          if (!isArrayed)
            result.missingIndex = MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX;
          else
            result.missingIndex = MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_ARRAY_INDEX;
        }
        else if (imageType->dim == Dim::Dim3D)
        {
          result.missingIndex = MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX;
        }
        else if (imageType->dim == Dim::Cube)
        {
          if (!isArrayed)
            result.missingIndex = MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_INDEX;
          else
            result.missingIndex = MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_ARRAY_INDEX;
        }
      }
    }
  }
  else
  {
    if (imageType->dim == Dim::Buffer)
    {
      result.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
      result.registerBase = U_BUFFER_IMAGE_OFFSET;
      result.missingIndex = MISSING_STORAGE_BUFFER_SAMPLED_IMAGE_INDEX;
    }
    else
    {
      result.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      result.registerBase = U_IMAGE_OFFSET;

      if (imageType->dim == Dim::Dim2D)
      {
        if (!isArrayed)
          result.missingIndex = MISSING_STORAGE_IMAGE_2D_INDEX;
        else
          result.missingIndex = MISSING_STORAGE_IMAGE_2D_ARRAY_INDEX;
      }
      else if (imageType->dim == Dim::Dim3D)
      {
        result.missingIndex = MISSING_STORAGE_IMAGE_3D_INDEX;
      }
      else if (imageType->dim == Dim::Cube)
      {
        if (!isArrayed)
          result.missingIndex = MISSING_STORAGE_IMAGE_CUBE_INDEX;
        else
          result.missingIndex = MISSING_STORAGE_IMAGE_CUBE_ARRAY_INDEX;
      }
    }
  }

  if (imageType->dim == Dim::Buffer)
  {
    result.checkType = CheckType::BufferView;
  }
  else
  {
    result.checkType = CheckType::Image;
  }

  if (imageType->dim == Dim::Dim2D)
  {
    if (!isArrayed)
    {
      result.imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    }
    else
    {
      result.imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
  }
  else if (imageType->dim == Dim::Dim3D)
  {
    result.imageViewType = VK_IMAGE_VIEW_TYPE_3D;
  }
  else if (imageType->dim == Dim::Cube)
  {
    if (!isArrayed)
    {
      result.imageViewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }
    else
    {
      result.imageViewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    }
  }
  // else if (imageType->dim == Dim::Buffer) {}
  // else if (imageType->dim == Dim::SubpassData) {}

  if (info.inputAttachment)
  {
    result.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    result.missingIndex = MISSING_IS_FATAL_INDEX;
    result.registerBase = T_INPUT_ATTACHMENT_OFFSET;
    result.checkType = CheckType::InputAttachment;
  }

  return result;
}

ResourceCompileResult compile_resource_buffer(const VariableToCompileInfo &info, ErrorHandler &e_handler)
{
  ResourceCompileResult result = {};

  result.imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
  result.missingIndex = spirv::MISSING_IS_FATAL_INDEX;

  auto kind = get_buffer_kind(info.var);

  if (BufferKind::Uniform == kind)
  {
    result.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    result.registerBase = B_CONST_BUFFER_OFFSET;
    result.missingIndex = info.binding->bindingPoint.value == 0 ? FALLBACK_TO_C_GLOBAL_BUFFER : MISSING_CONST_BUFFER_INDEX;
    result.checkType = CheckType::ConstBuffer;
  }
  else if (BufferKind::Storage == kind || BufferKind::ReadOnlyStorage == kind)
  {
    result.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    result.checkType = CheckType::Buffer;
    if (BufferKind::ReadOnlyStorage == kind)
    {
      result.registerBase = T_BUFFER_OFFSET;
      result.missingIndex = MISSING_BUFFER_INDEX;
    }
    else
    {
      result.registerBase = U_BUFFER_OFFSET;
      result.missingIndex = MISSING_STORAGE_BUFFER_INDEX;
    }
  }
  else
  {
    e_handler.onFatalError("compileHeader: Unable to identify interface block type, no decoration "
                           "was found to indicate the intended use (Block, BufferBlock or "
                           "NonWriteable (per member)");
  }

  return result;
}

void compile_separate_samplers(eastl::vector<TaggedSamplerBind> &samplers, ShaderHeader &header, CounterInfo &counter_info,
  unsigned d_set, ErrorHandler &e_handler)
{
  for (TaggedSamplerBind &i : samplers)
  {
    if (i.pairFound)
      continue;
    PropertyBinding *binding = find_property<PropertyBinding>(i.node);
    if (PropertyDescriptorSet *set = find_property<PropertyDescriptorSet>(i.node))
      set->descriptorSet.value = d_set;
    auto sourceBindingIndex = binding->bindingPoint.value % REGISTER_ENTRIES;
    bool fitsRegLimits = true;
    const int maxTbinding = REGISTER_ENTRIES * 3;
    fitsRegLimits &= sourceBindingIndex < T_REGISTER_INDEX_MAX;
    fitsRegLimits &= binding->bindingPoint.value < maxTbinding;
    header.sRegisterUseMask |= 1u << sourceBindingIndex;
    if (!fitsRegLimits)
    {
      eastl::string err;
      PropertyName *name = find_property<PropertyName>(i.node);
      err.sprintf("compileHeader: Separate sampler binding %u (raw %u) for var %u, name: %s is out of limits", sourceBindingIndex,
        binding->bindingPoint.value, i.node->resultId, name ? name->name.c_str() : "<unknown>");
      e_handler.onFatalError(err.c_str());
      return;
    }
    header.inputAttachmentIndex[header.registerCount] = INVALID_INPUT_ATTACHMENT_INDEX;
    header.descriptorTypes[header.registerCount] = VK_DESCRIPTOR_TYPE_SAMPLER;
    header.imageViewTypes[header.registerCount] = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    header.registerIndex[header.registerCount] = S_SAMPLER_OFFSET + sourceBindingIndex;
    header.missingTableIndex[header.registerCount] = MISSING_IS_FATAL_INDEX;
    uint32_t descTypeIndex = spirv::descrior_type_to_index(VK_DESCRIPTOR_TYPE_SAMPLER);
    if (descTypeIndex < (sizeof(counter_info.descriptorCounts) / sizeof(counter_info.descriptorCounts[0])))
      ++counter_info.descriptorCounts[descTypeIndex];
    binding->bindingPoint.value = header.registerCount;
    ++header.registerCount;
    if (header.registerCount >= REGISTER_ENTRIES)
    {
      e_handler.onFatalError("compileHeader: Too many resources in use, software limit reached. "
                             "REGISTER_ENTRIES needs to be adjusted!");
      return;
    }
  }
  samplers.clear();
}

void compile_resource(const VariableToCompileInfo &info, const eastl::vector<NodePointer<NodeOpVariable>> &depth_tex_list,
  eastl::vector<TaggedSamplerBind> &samplers, unsigned d_set, ShaderHeader &header, CounterInfo &counter_info, CompileFlags flags,
  ErrorHandler &e_handler)
{
  if (bool(flags & CompileFlags::ENABLE_BINDLESS_SUPPORT))
  {
    G_ASSERTF(d_set >= spirv::bindless::MAX_SETS, "destination set can't be a set used for bindless resources");

    // resources with meta bindless set id's are skipped from processing and their
    // set id remapped to the appropriate bindless set
    auto &setv = info.set->descriptorSet.value;
    if (setv >= bindless::FIRST_DESCRIPTOR_SET_META_INDEX && setv <= bindless::MAX_DESCRIPTOR_SET_META_INDEX)
    {
      setv -= bindless::FIRST_DESCRIPTOR_SET_META_INDEX;
      G_ASSERTF(setv <= bindless::MAX_DESCRIPTOR_SET_ACTUAL_INDEX, "unknown bindless descriptor set %u", setv);
      return;
    }
  }

  if (header.registerCount >= REGISTER_ENTRIES)
  {
    e_handler.onFatalError("compileHeader: Too many resources in use, software limit reached. "
                           "REGISTER_ENTRIES needs to be adjusted!");
    return;
  }
  // TODO: check any sorts of limits! (see old glslang io remapper pass)
  // note: we remapping 1d array to 2d array with REGISTER_ENTRIES stride
  auto sourceBindingIndex = info.binding->bindingPoint.value % REGISTER_ENTRIES;

  auto ptrType = as<NodeOpTypePointer>(info.var->resultType);
  auto valueType = ptrType->type;

  ResourceCompileResult resourceCompileInfo = {};
  if (ptrType->storageClass == StorageClass::UniformConstant)
  {
    // any image type (including (rw)buffer)
    resourceCompileInfo = compile_resource_image(info, depth_tex_list);
  }
  else if (ptrType->storageClass == StorageClass::Uniform)
  {
    // structured or const buffer
    resourceCompileInfo = compile_resource_buffer(info, e_handler);
  }

  bool fitsRegLimits = true;
  if (info.registerType == SourceRegisterType::B)
  {
    // ref: wchar_t spacingS
    const int maxBbinding = REGISTER_ENTRIES * 2;
    fitsRegLimits &= sourceBindingIndex < B_REGISTER_INDEX_MAX;
    fitsRegLimits &= info.binding->bindingPoint.value < maxBbinding;
    header.bRegisterUseMask |= 1u << sourceBindingIndex;
  }
  else if (info.registerType == SourceRegisterType::T)
  {
    // ref: wchar_t spacingT
    const int maxTbinding = REGISTER_ENTRIES * 3;
    fitsRegLimits &= sourceBindingIndex < T_REGISTER_INDEX_MAX;
    fitsRegLimits &= info.binding->bindingPoint.value < maxTbinding;
    header.tRegisterUseMask |= 1u << sourceBindingIndex;
  }
  else if (info.registerType == SourceRegisterType::U)
  {
    // ref: wchar_t spacingU
    const int maxUbinding = REGISTER_ENTRIES * 4;
    fitsRegLimits &= sourceBindingIndex < U_REGISTER_INDEX_MAX;
    fitsRegLimits &= info.binding->bindingPoint.value < maxUbinding;
    header.uRegisterUseMask |= 1u << sourceBindingIndex;
  }
  else
  {
    eastl::string err;
    PropertyName *name = find_property<PropertyName>(info.var);
    err.sprintf("compileHeader: Unable to deduce source register (b, t or u) for var %u, name: %s",
      name ? name->name.c_str() : "<unknown>");
    e_handler.onFatalError(err.c_str());
    return;
  }

  if (!fitsRegLimits)
  {
    eastl::string err;
    PropertyName *name = find_property<PropertyName>(info.var);
    err.sprintf("compileHeader: Register type %u binding %u (raw %u) for var %u, name: %s is out of limits", info.registerType,
      sourceBindingIndex, info.binding->bindingPoint.value, info.var->resultId, name ? name->name.c_str() : "<unknown>");
    e_handler.onFatalError(err.c_str());
    return;
  }

  if (info.inputAttachment)
    ++header.inputAttachmentCount;
  header.inputAttachmentIndex[header.registerCount] =
    info.inputAttachment ? (uint8_t)info.inputAttachment->attachmentIndex.value : INVALID_INPUT_ATTACHMENT_INDEX;

  header.imageViewTypes[header.registerCount] = resourceCompileInfo.imageViewType;
  header.registerIndex[header.registerCount] = resourceCompileInfo.registerBase + sourceBindingIndex;
  header.missingTableIndex[header.registerCount] = resourceCompileInfo.missingIndex;

  switch (resourceCompileInfo.checkType)
  {
    case CheckType::Image: header.imageCheckIndices[header.imageCount++] = header.registerCount; break;
    case CheckType::BufferView: header.bufferViewCheckIndices[header.bufferViewCount++] = header.registerCount; break;
    case CheckType::Buffer: header.bufferCheckIndices[header.bufferCount++] = header.registerCount; break;
    case CheckType::ConstBuffer: header.constBufferCheckIndices[header.constBufferCount++] = header.registerCount; break;
    case CheckType::AccelerationStructure:
      header.accelerationStructureCheckIndices[header.accelerationStructureCount++] = header.registerCount;
      break;
    default: break;
  }

  info.set->descriptorSet.value = d_set;
  info.binding->bindingPoint.value = header.registerCount;

  // for combo type find sampler and map it onto same slot
  // this is perfectly valid
  if (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER == resourceCompileInfo.descriptorType)
  {
    if (info.combinedSampler.isValid())
    {
      auto combinedSamplerSet = find_property<PropertyDescriptorSet>(info.combinedSampler);
      auto combinedSamplerBinding = find_property<PropertyBinding>(info.combinedSampler);
      if (combinedSamplerBinding)
      {
        combinedSamplerBinding->bindingPoint.value = header.registerCount;
      }
      else
      {
        eastl::string err;
        PropertyName *name = find_property<PropertyName>(info.combinedSampler);
        err.sprintf("compileHeader: could not resolve binding data for sampler %s", name ? name->name.c_str() : "<unknown>");
        e_handler.onFatalError(err.c_str());
      }

      if (combinedSamplerSet)
      {
        combinedSamplerSet->descriptorSet.value = d_set;
      }
    }
    else
      resourceCompileInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  }

  uint32_t descTypeIndex = spirv::descrior_type_to_index(resourceCompileInfo.descriptorType);
  if (descTypeIndex < (sizeof(counter_info.descriptorCounts) / sizeof(counter_info.descriptorCounts[0])))
    ++counter_info.descriptorCounts[descTypeIndex];
  header.descriptorTypes[header.registerCount] = resourceCompileInfo.descriptorType;

  ++header.registerCount;
}

void compile_descriptor_count_table(const CounterInfo &counter_info, ShaderHeader &header)
{
  for (uint32_t i = 0; i < SHADER_HEADER_DECRIPTOR_COUNT_SIZE; ++i)
  {
    if (0 == counter_info.descriptorCounts[i])
      continue;

    VkDescriptorPoolSize &target = header.descriptorCounts[header.descriptorCountsCount++];
    target.type = descriptor_index_to_type(i);
    target.descriptorCount = counter_info.descriptorCounts[i];
  }
}

void find_all_combined_samplers(ModuleBuilder &builder, eastl::vector<VariableToCompileInfo> &uniforms,
  eastl::vector<TaggedSamplerBind> &samplers, ErrorHandler &e_handler)
{
  struct SampledImageCombination
  {
    NodePointer<NodeOpVariable> image;
    NodePointer<NodeOpVariable> sampler;
    bool paired = false;
  };
  eastl::vector<SampledImageCombination> uniqueSampledImageCombinations;

  builder.enumerateAllNodes([&](auto node) {
    // filter only unique OpSampledImage for image and sampler loads from variables
    if (!is<NodeOpSampledImage>(node))
      return;
    auto sampledImage = as<NodeOpSampledImage>(node);
    if (!is<NodeOpLoad>(sampledImage->image) || !is<NodeOpLoad>(sampledImage->sampler))
      return;
    auto imageLoad = as<NodeOpLoad>(sampledImage->image);
    auto samplerLoad = as<NodeOpLoad>(sampledImage->sampler);
    if (!is<NodeOpVariable>(imageLoad->pointer) || !is<NodeOpVariable>(samplerLoad->pointer))
      return;
    auto image = as<NodeOpVariable>(imageLoad->pointer);
    auto sampler = as<NodeOpVariable>(samplerLoad->pointer);

    auto registeredUniqueCombination = eastl::find_if(uniqueSampledImageCombinations.begin(), uniqueSampledImageCombinations.end(),
      [&](const SampledImageCombination &combination) {
        const bool sameImage = image->resultId == combination.image->resultId;
        const bool sameSampler = sampler->resultId == combination.sampler->resultId;
        // if currently processed pair was processed before or it's image or sampler are already combined, then such pair is invalid
        return (sameImage && sameSampler) || ((sameImage || sameSampler) && combination.paired);
      });
    if (registeredUniqueCombination != uniqueSampledImageCombinations.end())
      return;
    uniqueSampledImageCombinations.emplace_back(image, sampler, false);

    // check that image and sampler are assigned to the same slot (descriptor set and binding)
    auto imageSet = find_property<PropertyDescriptorSet>(image);
    auto imageBinding = find_property<PropertyBinding>(image);
    auto samplerSet = find_property<PropertyDescriptorSet>(sampler);
    auto samplerBinding = find_property<PropertyBinding>(sampler);
    if (!imageSet || !imageBinding || !samplerSet || !samplerBinding)
      return;
    if (imageSet->descriptorSet.value != samplerSet->descriptorSet.value ||
        (imageBinding->bindingPoint.value % REGISTER_ENTRIES) != (samplerBinding->bindingPoint.value % REGISTER_ENTRIES))
      return;

    // check that image and sampler are uniforms that were not already paired with something else in previous iterations
    // this may happen when the same sampler is used for several images or vice versa
    auto imageUniform = eastl::find_if(uniforms.begin(), uniforms.end(),
      [&](const VariableToCompileInfo &imageInfo) { return imageInfo.var->resultId == image->resultId; });
    auto samplerUniform = eastl::find_if(samplers.begin(), samplers.end(),
      [&](const TaggedSamplerBind &samplerBind) { return samplerBind.node->resultId == sampler->resultId; });
    if (imageUniform == uniforms.end() || samplerUniform == samplers.end())
      return;
    if (imageUniform->combinedSampler.isValid() || samplerUniform->pairFound)
      return;
    imageUniform->combinedSampler = sampler;
    samplerUniform->pairFound = true;
    uniqueSampledImageCombinations.back().paired = true;
  });
}

} // namespace

ShaderHeader spirv::compileHeader(ModuleBuilder &builder, CompileFlags flags, ErrorHandler &e_handler)
{
  ShaderHeader result = {};

  ExecutionModel execModel = ExecutionModel::Max;
  builder.enumerateEntryPoints([&](auto, auto em, auto, auto &) { execModel = em; });

  unsigned descriptorSetIndex = ~0;
  switch (execModel)
  {
    case ExecutionModel::Vertex: descriptorSetIndex = graphics::vertex::REGISTERS_SET_INDEX; break;
    case ExecutionModel::Geometry: descriptorSetIndex = graphics::geometry::REGISTERS_SET_INDEX; break;
    case ExecutionModel::Fragment: descriptorSetIndex = graphics::fragment::REGISTERS_SET_INDEX; break;
    case ExecutionModel::TessellationControl: descriptorSetIndex = graphics::control::REGISTERS_SET_INDEX; break;
    case ExecutionModel::TessellationEvaluation: descriptorSetIndex = graphics::evaluation::REGISTERS_SET_INDEX; break;
    case ExecutionModel::GLCompute: descriptorSetIndex = compute::REGISTERS_SET_INDEX; break;
    case ExecutionModel::RayGenerationNV:
    case ExecutionModel::IntersectionNV:
    case ExecutionModel::AnyHitNV:
    case ExecutionModel::ClosestHitNV:
    case ExecutionModel::MissNV:
    case ExecutionModel::CallableNV: descriptorSetIndex = raytrace::REGISTERS_SET_INDEX; break;
    case ExecutionModel::Kernel:
    case ExecutionModel::TaskNV:
    case ExecutionModel::MeshNV:
    default:
      eastl::string fatalMsg("compileHeader: Unsupported shader execution model (either, "
                             "tessellation, mesh, task, OpenCL kernel or unknown was encountered");
      fatalMsg.append_sprintf("execModel = %u)", execModel);
      e_handler.onFatalError(fatalMsg);
      return result;
  }

  // if bindless features enabled the first sets are used for bindless resource ranges
  // for any other resource the per shader stage destination set id is shifted
  if (bool(flags & CompileFlags::ENABLE_BINDLESS_SUPPORT))
  {
    descriptorSetIndex += spirv::bindless::MAX_SETS;
  }

  eastl::vector<VariableToCompileInfo> uniforms;
  eastl::vector<TaggedSamplerBind> samplers;
  // firs gather uniforms and mark in/out mask on the fly
  builder.enumerateAllGlobalVariables([&](auto var) //
    {
      auto ptrType = as<NodeOpTypePointer>(var->resultType);
      if (ptrType->storageClass == StorageClass::Input)
      {
        auto location = find_property<PropertyLocation>(var);
        // can be null for built in
        if (location)
          result.inputMask |= 1u << location->location.value;
      }
      else if (ptrType->storageClass == StorageClass::Output)
      {
        auto location = find_property<PropertyLocation>(var);
        // can be null for built in
        if (location)
          result.outputMask |= 1u << location->location.value;
      }
      else if (ptrType->storageClass == StorageClass::UniformConstant || ptrType->storageClass == StorageClass::Uniform)
      {
        if (is<NodeOpTypeSampler>(ptrType->type))
        {
          samplers.push_back({false, var});
        }
        else
        {
          VariableToCompileInfo info;
          info.combinedSampler.reset();
          info.var = var;
          info.binding = find_property<PropertyBinding>(var);
          info.set = find_property<PropertyDescriptorSet>(var);
          info.inputAttachment = find_property<PropertyInputAttachmentIndex>(var);
          info.registerType = info.inputAttachment ? SourceRegisterType::T : get_source_register_type(var);
          if (info.registerType == SourceRegisterType::Err)
          {
            e_handler.onFatalError("compileHeader: Unable to deduce source register (b, t or u)");
            return;
          }
          uniforms.push_back(info);
        }
      }
    });

  eastl::sort(begin(uniforms), end(uniforms),
    [=](const VariableToCompileInfo &l, const VariableToCompileInfo &r) //
    {
      const auto li = static_cast<uint32_t>(l.registerType);
      const auto ri = static_cast<uint32_t>(r.registerType);
      const auto lb = l.binding->bindingPoint.value;
      const auto rb = r.binding->bindingPoint.value;

      if (li < ri)
        return true;
      else if (li > ri)
        return false;
      else
        return lb < rb;
    });

  find_all_combined_samplers(builder, uniforms, samplers, e_handler);

  auto texturesWithCompare = find_all_depth_textures(builder, e_handler);

  CounterInfo counterInfo;

  for (auto &&uniform : uniforms)
    compile_resource(uniform, texturesWithCompare, samplers, descriptorSetIndex, result, counterInfo, flags, e_handler);

  if (!samplers.empty())
    // patch shader T use bits + add registers with plain sampler descriptors based on samplers array
    compile_separate_samplers(samplers, result, counterInfo, descriptorSetIndex, e_handler);

  compile_descriptor_count_table(counterInfo, result);

  result.pushConstantsCount = find_push_constant_count(builder, e_handler);

  return result;
}

ComputeShaderInfo spirv::resolveComputeShaderInfo(ModuleBuilder &builder)
{
  ComputeShaderInfo compute_shader_info = {};
  builder.enumerateExecutionModes([&](auto, auto em) {
    if (em->mode == ExecutionMode::LocalSize)
    {
      compute_shader_info.threadGroupSizeX = reinterpret_cast<spirv::ExecutionModeLocalSize *>(em)->xSize.value;
      compute_shader_info.threadGroupSizeY = reinterpret_cast<spirv::ExecutionModeLocalSize *>(em)->ySize.value;
      compute_shader_info.threadGroupSizeZ = reinterpret_cast<spirv::ExecutionModeLocalSize *>(em)->zSize.value;
    }
  });
  return compute_shader_info;
}
