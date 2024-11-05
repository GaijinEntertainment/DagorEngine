// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/sort.h>
#include "module_nodes.h"
#include <spirv/module_builder.h>

using namespace spirv;

namespace
{
struct StructureMemberInfo
{
  size_t index;
  NodePointer<NodeTypedef> type;
  PropertyOffset *offset = nullptr;
};

eastl::string getStructureName(NodePointer<NodeOpTypeStruct> type)
{
  auto name = find_base_property<PropertyName>(type);
  if (name)
    return name->name;
  return "StructureType with id " + eastl::to_string(type->resultId);
}

eastl::string getMemberName(NodePointer<NodeOpTypeStruct> struct_type, size_t index)
{
  auto structName = getStructureName(struct_type);
  auto memberNameProp = find_member_property<PropertyName>(struct_type, index);
  if (memberNameProp)
    return structName + ":" + memberNameProp->name;
  return "Member at index " + eastl::to_string(index) + " of " + structName;
}

size_t align(size_t value, size_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }

eastl::vector<StructureMemberInfo> collectStructureMemberInfo(NodePointer<NodeOpTypeStruct> struct_type, ErrorHandler &e_handler)
{
  eastl::vector<StructureMemberInfo> result;

  for (size_t i = 0; i < struct_type->param1.size(); ++i)
  {
    StructureMemberInfo info;
    info.index = i;
    info.type = struct_type->param1[i];
    info.offset = find_member_property<PropertyOffset>(struct_type, i);
    if (!info.offset)
    {
      auto structMemberName = getMemberName(struct_type, i);
      e_handler.onFatalError("validateStructureLayouts: " + structMemberName + " has no offset decoration");
      result.clear();
      return result;
    }
    result.push_back(info);
  }

  // sort them in a predictable order (by offset)
  eastl::stable_sort(begin(result), end(result),
    [](auto &l, auto &r) //
    { return l.offset->byteOffset.value < r.offset->byteOffset.value; });

  return result;
}

size_t calculateScalarAlignement(NodePointer<NodeTypedef> type)
{
  if (is<NodeOpTypeInt>(type))
  {
    return as<NodeOpTypeInt>(type)->width.value / 8;
  }
  else if (is<NodeOpTypeFloat>(type))
  {
    return as<NodeOpTypeFloat>(type)->width.value / 8;
  }
  else if (is<NodeOpTypeVector>(type))
  {
    return calculateScalarAlignement(as<NodeOpTypeVector>(type)->componentType);
  }
  else if (is<NodeOpTypeMatrix>(type))
  {
    return calculateScalarAlignement(as<NodeOpTypeMatrix>(type)->columnType);
  }
  else if (is<NodeOpTypeArray>(type))
  {
    return calculateScalarAlignement(as<NodeOpTypeArray>(type)->elementType);
  }
  else if (is<NodeOpTypeRuntimeArray>(type))
  {
    return calculateScalarAlignement(as<NodeOpTypeRuntimeArray>(type)->elementType);
  }
  else if (is<NodeOpTypeStruct>(type))
  {
    size_t maxAlignment = 1;
    for (auto &&member : as<NodeOpTypeStruct>(type)->param1)
    {
      auto memberAlign = calculateScalarAlignement(member);
      if (memberAlign > maxAlignment)
        maxAlignment = memberAlign;
    }
    return maxAlignment;
  }
  else if (is<NodeOpTypePointer>(type))
  {
    // TODO: depends on memory model...
    return 0;
  }
  else
  {
    // error...
    return 1;
  }

  return 1;
}

size_t calculateBaseAlignment(NodePointer<NodeTypedef> type, bool align_up, NodePointer<NodeOpTypeStruct> base_struct, size_t index)
{
  if (is<NodeOpTypeInt>(type))
  {
    return as<NodeOpTypeInt>(type)->width.value / 8;
  }
  else if (is<NodeOpTypeFloat>(type))
  {
    return as<NodeOpTypeFloat>(type)->width.value / 8;
  }
  else if (is<NodeOpTypeVector>(type))
  {
    auto vectorType = as<NodeOpTypeVector>(type);
    auto componentAlign = calculateBaseAlignment(vectorType->componentType, align_up, base_struct, index);
    auto componentCount = vectorType->componentCount.value;
    if (componentCount == 3)
      componentCount = 4;
    return componentAlign * componentCount;
  }
  else if (is<NodeOpTypeMatrix>(type))
  {
    auto matrxType = as<NodeOpTypeMatrix>(type);
    if (find_member_property<PropertyRowMajor>(base_struct, index))
    {
      auto scalarComponentAlign =
        calculateBaseAlignment(as<NodeOpTypeVector>(matrxType->columnType)->componentType, align_up, base_struct, index);
      auto columnCount = matrxType->columnCount.value;
      if (columnCount == 3)
        columnCount = 4;
      return scalarComponentAlign * columnCount;
    }
    else
    {
      return calculateBaseAlignment(matrxType->columnType, align_up, base_struct, index);
    }
  }
  else if (is<NodeOpTypeArray>(type))
  {
    auto elementAlign = calculateBaseAlignment(as<NodeOpTypeArray>(type)->elementType, align_up, base_struct, index);
    if (align_up)
      elementAlign = align(elementAlign, 16);
    return elementAlign;
  }
  else if (is<NodeOpTypeRuntimeArray>(type))
  {
    auto elementAlign = calculateBaseAlignment(as<NodeOpTypeRuntimeArray>(type)->elementType, align_up, base_struct, index);
    if (align_up)
      elementAlign = align(elementAlign, 16);
    return elementAlign;
  }
  else if (is<NodeOpTypeStruct>(type))
  {
    size_t maxAlignment = 1;
    auto structType = as<NodeOpTypeStruct>(type);
    auto &memberList = structType->param1;
    for (size_t i = 0; i < memberList.size(); ++i)
    {
      auto &member = memberList[i];
      auto memberAlign = calculateBaseAlignment(member, align_up, structType, i);
      if (memberAlign > maxAlignment)
        maxAlignment = memberAlign;
    }
    if (align_up)
      maxAlignment = align(maxAlignment, 16);
    return maxAlignment;
  }
  else if (is<NodeOpTypePointer>(type))
  {
    // TODO: depends on memory model...
    return 0;
  }
  else
  {
    // error...
    return 1;
  }
}

size_t evaluateArrayLength(NodePointer<NodeOpTypeArray> array_type)
{
  auto sizeOp = as<NodeOperation>(array_type->length);
  auto resultType = as<NodeTypedef>(sizeOp->resultType);
  // TODO: more sanity checking
  if (!is<NodeOpTypeInt>(resultType))
    return 0;

  // spec constant has runtime changeable value, makes no sense to calculate
  if (is<NodeSpecConstant>(sizeOp))
    return 0;

  // valid use for int type, same as const value with all constant bits set to 0
  if (is<NodeOpConstantNull>(sizeOp))
    return 0;

  if (is<NodeOpConstant>(sizeOp))
  {
    return as<NodeOpConstant>(sizeOp)->value.value;
  }

  // unexpected size op, report error!
  return 0;
}

size_t calculateSize(NodePointer<NodeTypedef> type, NodePointer<NodeOpTypeStruct> base_struct, size_t index)
{
  if (is<NodeOpTypeInt>(type))
  {
    return as<NodeOpTypeInt>(type)->width.value / 8;
  }
  else if (is<NodeOpTypeFloat>(type))
  {
    return as<NodeOpTypeFloat>(type)->width.value / 8;
  }
  else if (is<NodeOpTypeVector>(type))
  {
    auto vectorType = as<NodeOpTypeVector>(type);
    auto componentSize = calculateSize(vectorType->componentType, base_struct, index);
    auto componentCount = vectorType->componentCount.value;
    return componentSize * componentCount;
  }
  else if (is<NodeOpTypeMatrix>(type))
  {
    auto matrixType = as<NodeOpTypeMatrix>(type);
    auto matrixStride = find_member_property<PropertyMatrixStride>(base_struct, index);
    if (find_member_property<PropertyRowMajor>(base_struct, index))
    {
      auto vectorType = as<NodeOpTypeVector>(matrixType->columnType);
      auto scalarType = as<NodeTypedef>(vectorType->componentType);
      auto scalarSize = calculateSize(scalarType, base_struct, index);
      return (vectorType->componentCount.value - 1) * matrixStride->matrixStride.value + matrixType->columnCount.value * scalarSize;
    }
    else
    {
      return matrixStride->matrixStride.value * matrixType->columnCount.value;
    }
  }
  else if (is<NodeOpTypeArray>(type))
  {
    auto arrayType = as<NodeOpTypeArray>(type);
    auto arrayCount = evaluateArrayLength(arrayType);
    if (0 == arrayCount)
      return 0;
    auto elementSize = calculateSize(arrayType->elementType, base_struct, index);
    auto arrayStride = find_base_property<PropertyArrayStride>(arrayType);
    return (arrayCount - 1) * arrayStride->arrayStride.value + elementSize;
  }
  else if (is<NodeOpTypeRuntimeArray>(type))
  {
    return 0;
  }
  else if (is<NodeOpTypeStruct>(type))
  {
    size_t lastOffset = 0;
    size_t lastIndex = 0;
    auto structType = as<NodeOpTypeStruct>(type);
    auto &memberList = structType->param1;
    for (size_t i = 0; i < memberList.size(); ++i)
    {
      auto offset = find_member_property<PropertyOffset>(structType, i);
      if (offset->byteOffset.value > lastOffset)
      {
        lastOffset = offset->byteOffset.value;
        lastIndex = i;
      }
    }
    return lastOffset + calculateSize(memberList[lastIndex], structType, lastIndex);
  }
  else if (is<NodeOpTypePointer>(type))
  {
    // TODO: depends on memory model...
    return 0;
  }
  else
  {
    // error...
    return 1;
  }
}

bool isAlignedTo(size_t offset, size_t alignment)
{
  if (0 == alignment)
    return 0 == offset;
  return 0 == (offset % alignment);
}

// only valid to call with float or int typedef
size_t scalarSize(NodePointer<NodeTypedef> type)
{
  if (is<NodeOpTypeInt>(type))
    return as<NodeOpTypeInt>(type)->width.value / 8;
  else if (is<NodeOpTypeFloat>(type))
    return as<NodeOpTypeFloat>(type)->width.value / 8;
  return 0;
}

bool hasImproperStraddle(NodePointer<NodeOpTypeVector> type, size_t offset)
{
  // valid to call with non vector type, then type parameter is null
  if (!type)
    return false;

  auto size = type->componentCount.value * scalarSize(type->componentType);
  auto length = offset + size - 1;
  if (size <= 16)
  {
    if ((offset >> 4) != (length >> 4))
      return true;
  }
  else
  {
    if (offset % 16 != 0)
      return true;
  }
  return false;
}

bool validateStructureLayout(NodePointer<NodeOpTypeStruct> type, StructureValidationRules rules, ErrorHandler &e_handler,
  bool is_uniform, size_t base_offset)
{
  auto members = collectStructureMemberInfo(type, e_handler);
  // if members vector is empty, than something happened during collect phase (missing offset)
  if (members.empty())
    return false;

  size_t validOffset = 0;
  for (auto &&member : members)
  {
    auto offset = member.offset->byteOffset.value + base_offset;
    size_t alignment;
    if (StructureValidationRules::SCALAR == rules)
      alignment = calculateScalarAlignement(member.type);
    else
      alignment = calculateBaseAlignment(member.type, is_uniform, type, member.index);
    auto size = calculateSize(member.type, type, member.index);

    if (StructureValidationRules::RELAXED == rules && is<NodeOpTypeVector>(member.type))
    {
      auto vecType = as<NodeOpTypeVector>(member.type);
      auto scalarAlignment = calculateScalarAlignement(vecType);
      if (!isAlignedTo(offset, scalarAlignment))
      {
        e_handler.onFatalError("validateStructureLayouts: " + getMemberName(type, member.index) +
                               " violates relaxed alignment rules, the offset " + eastl::to_string(offset) + " is not aligned to " +
                               eastl::to_string(scalarAlignment) +
                               ". With relaxed layout, vectors have to be aligned by the scalar "
                               "base types alignment");
        e_handler.onMessage("Explanation: no example for this seen yet, contact Vulkan maintainer "
                            "to find a fix.");
        return false;
      }
    }
    else
    {
      if (!isAlignedTo(offset, alignment))
      {
        e_handler.onFatalError("validateStructureLayouts: " + getMemberName(type, member.index) +
                               " violates alignment rules, the offset " + eastl::to_string(offset) + " is not aligned to " +
                               eastl::to_string(alignment) +
                               ". With basic or scalar layout, offset has to be aligned by its "
                               "types alignment.");
        e_handler.onMessage("Explanation: With unextended Vulkan structure members have to have an "
                            "offset that is an multiple of their alignment. Most common case, of "
                            "this error, is if a vector member has a scalar predecessor, then the "
                            "scalar predecessor pushes the vector into an unaligned offset. To fix "
                            "this, either reorder structure members or fuse them.");
        return false;
      }
    }

    if (offset < validOffset)
    {
      e_handler.onFatalError("validateStructureLayouts: " + getMemberName(type, member.index) +
                             " overlaps with other structure members, this is invalid! Next offset "
                             "was expected to be " +
                             eastl::to_string(validOffset) + " or larger, but " + eastl::to_string(offset) + " was found.");
      e_handler.onMessage("Explanation: Unextended Vulkan requires alignment of structures to "
                          "their largest member type. If the type of structure member it self is a "
                          "structure and not aligned by its largest type (float3 followed by int "
                          "for example, which in it self is problematic, as the float3 is aligned "
                          "as float4. This results in a structure size of 2 x float4), then the "
                          "next member will fall into the alignment of the previous member.");
      return false;
    }
    if (StructureValidationRules::RELAXED == rules)
    {
      if (hasImproperStraddle(member.type, offset))
      {
        e_handler.onFatalError(
          "validateStructureLayouts: " + getMemberName(type, member.index) + " is a improperly straddled vector!");
        e_handler.onMessage("Explanation: Likely a compiler bug (DXC).");
        return false;
      }
    }

    if (is<NodeOpTypeStruct>(member.type))
    {
      if (!validateStructureLayout(member.type, rules, e_handler, is_uniform, offset))
        return false;
    }

    if (is<NodeOpTypeMatrix>(member.type))
    {
      auto matrixStride = find_member_property<PropertyMatrixStride>(type, member.index);
      if (!matrixStride)
      {
        e_handler.onFatalError("validateStructureLayouts: " + getMemberName(type, member.index) +
                               " is of a matrix type but has no matrix stride decoration.");
        e_handler.onMessage("Explanation: Likely a compiler bug (DXC).");
        return false;
      }
      if (!isAlignedTo(matrixStride->matrixStride.value, alignment))
      {
        e_handler.onFatalError("validateStructureLayouts: " + getMemberName(type, member.index) +
                               " is of matrix type with a stride of " + eastl::to_string(matrixStride->matrixStride.value) +
                               " is not aligned to " + eastl::to_string(alignment));
        e_handler.onMessage("Explanation: Likely a compiler bug (DXC).");
        return false;
      }
    }

    auto arrayType = member.type;
    auto arrayAlignment = alignment;
    while (is<NodeOpTypeArray>(arrayType) || is<NodeOpTypeRuntimeArray>(arrayType))
    {
      NodePointer<NodeTypedef> elementType;
      if (is<NodeOpTypeArray>(arrayType))
      {
        elementType = as<NodeOpTypeArray>(arrayType)->elementType;
      }
      else
      {
        elementType = as<NodeOpTypeRuntimeArray>(arrayType)->elementType;
      }
      auto arrayStrideProp = find_base_property<PropertyArrayStride>(arrayType);
      if (!arrayStrideProp)
      {
        e_handler.onFatalError(
          "validateStructureLayouts: array type of " + getMemberName(type, member.index) + " has missing array stride decorations.");
        e_handler.onMessage("Explanation: Compiler bug (DXC).");
        return false;
      }
      auto arrayStride = arrayStrideProp->arrayStride.value;
      if (!isAlignedTo(arrayStride, arrayAlignment))
      {
        e_handler.onFatalError("validateStructureLayouts: " + getMemberName(type, member.index) + " is an array with an stride of " +
                               eastl::to_string(arrayStride) + " which is not aligned to " + eastl::to_string(arrayAlignment));
        e_handler.onMessage("Explanation: This is probably because of a array of a vector with 3 "
                            "components, on Vulkan this vector is aligned as a vector with 4 "
                            "components. The structure layout has to be refactored to either be an "
                            "array of scalars or vectors with a component count different than 3.");
        return false;
      }

      size_t numElements = 0;
      if (is<NodeOpTypeArray>(arrayType))
      {
        numElements = evaluateArrayLength(arrayType);
      }
      if (numElements < 1)
        numElements = 1;

      for (size_t e = 0; e < numElements; ++e)
      {
        size_t nextOffset = e * arrayStride + offset;
        if (is<NodeOpTypeStruct>(elementType))
        {
          if (!validateStructureLayout(elementType, rules, e_handler, is_uniform, nextOffset))
          {
            return false;
          }
        }
      }

      arrayType = elementType;
      if (StructureValidationRules::SCALAR == rules)
      {
        arrayAlignment = calculateScalarAlignement(elementType);
      }
      else
      {
        arrayAlignment = calculateBaseAlignment(elementType, is_uniform, type, member.index);
      }
    }

    validOffset = offset + size;
    if (StructureValidationRules::SCALAR != rules && is_uniform &&
        (is<NodeOpTypeArray>(member.type) || is<NodeOpTypeStruct>(member.type)))
    {
      // Uniform block rules don't permit anything in the padding of a struct or array.
      validOffset = align(validOffset, alignment);
    }
  }
  return true;
}

bool validateLayout(ModuleBuilder &builder, NodePointer<NodeVariable> var, StructureValidationRules rules, ErrorHandler &e_handler)
{
  auto ptrType = as<NodeOpTypePointer>(var->resultType);
  auto valueType = as<NodeTypedef>(ptrType->type);
  // first we need to know if we validate a const buffer or a structure buffer
  auto bufferKind = get_buffer_kind(var);

  if (!is<NodeOpTypeStruct>(valueType))
  {
    // try to resolve bindless nested array once
    if (is<NodeOpTypeRuntimeArray>(valueType))
      valueType = as<NodeOpTypeRuntimeArray>(valueType)->elementType;

    if (!is<NodeOpTypeStruct>(valueType))
    {
      e_handler.onFatalError("validateLayout: buffer type pointer is not structure, probably extra nested array handling is needed");
      return false;
    }
  }

  if (BufferKind::Uniform == bufferKind)
  {
    return validateStructureLayout(valueType, rules, e_handler, true, 0);
  }
  else if (BufferKind::ReadOnlyStorage == bufferKind || BufferKind::Storage == bufferKind)
  {
    return validateStructureLayout(valueType, rules, e_handler, false, 0);
  }
  else
  {
    e_handler.onFatalError("validateStructureLayouts: Unable to determine which kind of a buffer "
                           "it is, neither use for uniform nor storage is indicated");
  }
  return true;
}
} // namespace

bool spirv::validateStructureLayouts(ModuleBuilder &builder, StructureValidationRules rules, ErrorHandler &e_handler)
{
  bool passed = true;
  e_handler.onMessage("validateStructureLayouts: Validating structure layouts for const and "
                      "structured buffers...");
  switch (rules)
  {
    case StructureValidationRules::DEFAULT:
      e_handler.onMessage("validateStructureLayouts: Validation "
                          "rule set is basic / vulkan 1.0 "
                          "without any extensions");
      break;
    case StructureValidationRules::RELAXED:
      e_handler.onMessage("validateStructureLayouts: Validation "
                          "rule set is relaxed / vulkan 1.1 or "
                          "vulkan 1.0 with relaxed block layout "
                          "extension");
      break;
    case StructureValidationRules::SCALAR:
      e_handler.onMessage("validateStructureLayouts: Validation rule set is scalar, requires "
                          "scalar block layout extension");
      break;
  }
  builder.enumerateAllGlobalVariables([&, rules](NodePointer<NodeOpVariable> var) //
    {
      if (var->storageClass == StorageClass::Uniform)
      {
        passed = passed && validateLayout(builder, var, rules, e_handler);
      }
    });
  e_handler.onMessage("validateStructureLayouts: ...finished validating structure layouts for "
                      "const and structured buffers");
  return passed;
}