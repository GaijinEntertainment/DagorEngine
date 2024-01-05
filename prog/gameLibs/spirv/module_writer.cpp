// auto generated, do not modify!
#include "module_nodes.h"
#include <spirv/module_builder.h>
using namespace spirv;
struct BranchPropertyWriter
{
  WordWriter &writer;
  void operator()(PorpertyLoopMerge *m)
  {
    writer.beginInstruction(Op::OpLoopMerge,
      3 + (((m->controlMask & LoopControlMask::DependencyLength) == LoopControlMask::DependencyLength) ? 1 : 0));
    writer.writeWord(m->mergeBlock->resultId);
    writer.writeWord(m->continueBlock->resultId);
    writer.writeWord(static_cast<unsigned>(m->controlMask));
    if ((m->controlMask & LoopControlMask::DependencyLength) == LoopControlMask::DependencyLength)
      writer.writeWord(static_cast<unsigned>(m->dependencyLength.value));
    writer.endWrite();
  }
  void operator()(PropertySelectionMerge *m)
  {
    writer.beginInstruction(Op::OpSelectionMerge, 2);
    writer.writeWord(m->mergeBlock->resultId);
    writer.writeWord(static_cast<unsigned>(m->controlMask));
    writer.endWrite();
  }
  template <typename T>
  void operator()(T *)
  {}
};
struct NodeWriteVisitor
{
  WordWriter &writer;
  ModuleBuilder &builder;
  bool operator()(NodeOpNop *value)
  {
    writer.beginInstruction(Op::OpNop, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUndef *value)
  {
    writer.beginInstruction(Op::OpUndef, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpString *value)
  {
    size_t len = 1;
    len += (value->string.size() + sizeof(unsigned)) / sizeof(unsigned);
    writer.beginInstruction(Op::OpString, len);
    writer.writeWord(value->resultId);
    writer.writeString(value->string.c_str());
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLine *value)
  {
    writer.beginInstruction(Op::OpLine, 3);
    writer.writeWord(value->file->resultId);
    writer.writeWord(value->line.value);
    writer.writeWord(value->column.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeVoid *value)
  {
    writer.beginInstruction(Op::OpTypeVoid, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeBool *value)
  {
    writer.beginInstruction(Op::OpTypeBool, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeInt *value)
  {
    writer.beginInstruction(Op::OpTypeInt, 3);
    writer.writeWord(value->resultId);
    writer.writeWord(value->width.value);
    writer.writeWord(value->signedness.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeFloat *value)
  {
    writer.beginInstruction(Op::OpTypeFloat, 2);
    writer.writeWord(value->resultId);
    writer.writeWord(value->width.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeVector *value)
  {
    writer.beginInstruction(Op::OpTypeVector, 3);
    writer.writeWord(value->resultId);
    writer.writeWord(value->componentType->resultId);
    writer.writeWord(value->componentCount.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeMatrix *value)
  {
    writer.beginInstruction(Op::OpTypeMatrix, 3);
    writer.writeWord(value->resultId);
    writer.writeWord(value->columnType->resultId);
    writer.writeWord(value->columnCount.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeImage *value)
  {
    size_t len = 8;
    len += value->accessQualifier ? 1 : 0;
    writer.beginInstruction(Op::OpTypeImage, len);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledType->resultId);
    writer.writeWord(static_cast<unsigned>(value->dim));
    writer.writeWord(value->depth.value);
    writer.writeWord(value->arrayed.value);
    writer.writeWord(value->mS.value);
    writer.writeWord(value->sampled.value);
    writer.writeWord(static_cast<unsigned>(value->imageFormat));
    if (value->accessQualifier)
    {
      writer.writeWord(static_cast<unsigned>((*value->accessQualifier)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeSampler *value)
  {
    writer.beginInstruction(Op::OpTypeSampler, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeSampledImage *value)
  {
    writer.beginInstruction(Op::OpTypeSampledImage, 2);
    writer.writeWord(value->resultId);
    writer.writeWord(value->imageType->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeArray *value)
  {
    writer.beginInstruction(Op::OpTypeArray, 3);
    writer.writeWord(value->resultId);
    writer.writeWord(value->elementType->resultId);
    writer.writeWord(value->length->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeRuntimeArray *value)
  {
    writer.beginInstruction(Op::OpTypeRuntimeArray, 2);
    writer.writeWord(value->resultId);
    writer.writeWord(value->elementType->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeStruct *value)
  {
    size_t len = 1;
    len += value->param1.size();
    writer.beginInstruction(Op::OpTypeStruct, len);
    writer.writeWord(value->resultId);
    for (auto &&v : value->param1)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeOpaque *value)
  {
    size_t len = 1;
    len += (value->theNameOfTheOpaqueType.size() + sizeof(unsigned)) / sizeof(unsigned);
    writer.beginInstruction(Op::OpTypeOpaque, len);
    writer.writeWord(value->resultId);
    writer.writeString(value->theNameOfTheOpaqueType.c_str());
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypePointer *value)
  {
    writer.beginInstruction(Op::OpTypePointer, 3);
    writer.writeWord(value->resultId);
    writer.writeWord(static_cast<unsigned>(value->storageClass));
    writer.writeWord(value->type->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeFunction *value)
  {
    size_t len = 2;
    len += value->param2.size();
    writer.beginInstruction(Op::OpTypeFunction, len);
    writer.writeWord(value->resultId);
    writer.writeWord(value->returnType->resultId);
    for (auto &&v : value->param2)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeEvent *value)
  {
    writer.beginInstruction(Op::OpTypeEvent, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeDeviceEvent *value)
  {
    writer.beginInstruction(Op::OpTypeDeviceEvent, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeReserveId *value)
  {
    writer.beginInstruction(Op::OpTypeReserveId, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeQueue *value)
  {
    writer.beginInstruction(Op::OpTypeQueue, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypePipe *value)
  {
    writer.beginInstruction(Op::OpTypePipe, 2);
    writer.writeWord(value->resultId);
    writer.writeWord(static_cast<unsigned>(value->qualifier));
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConstantTrue *value)
  {
    writer.beginInstruction(Op::OpConstantTrue, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConstantFalse *value)
  {
    writer.beginInstruction(Op::OpConstantFalse, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConstant *value)
  {
    size_t len = 2;
    len += builder.getTypeSizeBits(value->resultType) > 32 ? 2 : 1;
    writer.beginInstruction(Op::OpConstant, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(static_cast<unsigned>(value->value.value));
    if (builder.getTypeSizeBits(value->resultType) > 32)
      writer.writeWord(static_cast<unsigned>(value->value.value >> 32u));
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConstantComposite *value)
  {
    size_t len = 2;
    len += value->constituents.size();
    writer.beginInstruction(Op::OpConstantComposite, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    for (auto &&v : value->constituents)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConstantSampler *value)
  {
    writer.beginInstruction(Op::OpConstantSampler, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(static_cast<unsigned>(value->samplerAddressingMode));
    writer.writeWord(value->param.value);
    writer.writeWord(static_cast<unsigned>(value->samplerFilterMode));
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConstantNull *value)
  {
    writer.beginInstruction(Op::OpConstantNull, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSpecConstantTrue *value)
  {
    writer.beginInstruction(Op::OpSpecConstantTrue, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSpecConstantFalse *value)
  {
    writer.beginInstruction(Op::OpSpecConstantFalse, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSpecConstant *value)
  {
    size_t len = 2;
    len += builder.getTypeSizeBits(value->resultType) > 32 ? 2 : 1;
    writer.beginInstruction(Op::OpSpecConstant, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(static_cast<unsigned>(value->value.value));
    if (builder.getTypeSizeBits(value->resultType) > 32)
      writer.writeWord(static_cast<unsigned>(value->value.value >> 32u));
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSpecConstantComposite *value)
  {
    size_t len = 2;
    len += value->constituents.size();
    writer.beginInstruction(Op::OpSpecConstantComposite, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    for (auto &&v : value->constituents)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSpecConstantOp *value)
  {
    auto anchor = writer.beginSpecConstantOpMode();
    writer.beginInstruction(Op::OpSpecConstantOp, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    visitNode(value->specOp, *this);
    writer.endSpecConstantOpMode(anchor);
    return true;
  }
  bool operator()(NodeOpFunction *value)
  {
    writer.beginInstruction(Op::OpFunction, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(static_cast<unsigned>(value->functionControl));
    writer.writeWord(value->functionType->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFunctionParameter *value)
  {
    writer.beginInstruction(Op::OpFunctionParameter, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFunctionCall *value)
  {
    size_t len = 3;
    len += value->parameters.size();
    writer.beginInstruction(Op::OpFunctionCall, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->function->resultId);
    for (auto &&v : value->parameters)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpVariable *value)
  {
    size_t len = 3;
    len += value->initializer ? 1 : 0;
    writer.beginInstruction(Op::OpVariable, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(static_cast<unsigned>(value->storageClass));
    if (value->initializer)
    {
      writer.writeWord((*value->initializer)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageTexelPointer *value)
  {
    writer.beginInstruction(Op::OpImageTexelPointer, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->sample->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLoad *value)
  {
    size_t len = 3;
    if (value->memoryAccess)
    {
      ++len;
      auto compare = *value->memoryAccess;
      if (MemoryAccessMask::Volatile == (compare & MemoryAccessMask::Volatile))
      {
        compare = compare ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compare & MemoryAccessMask::Aligned))
      {
        compare = compare ^ MemoryAccessMask::Aligned;
        ++len;
      }
      if (MemoryAccessMask::Nontemporal == (compare & MemoryAccessMask::Nontemporal))
      {
        compare = compare ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compare & MemoryAccessMask::MakePointerAvailable))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailable;
        ++len;
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compare & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailableKHR;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisible == (compare & MemoryAccessMask::MakePointerVisible))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisible;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compare & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisibleKHR;
        ++len;
      }
      if (MemoryAccessMask::NonPrivatePointer == (compare & MemoryAccessMask::NonPrivatePointer))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compare & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.beginInstruction(Op::OpLoad, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    if (value->memoryAccess)
    {
      writer.writeWord(static_cast<unsigned>((*value->memoryAccess)));
      auto compareWrite = (*value->memoryAccess);
      if (MemoryAccessMask::Volatile == (compareWrite & MemoryAccessMask::Volatile))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compareWrite & MemoryAccessMask::Aligned))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Aligned;
        writer.writeWord(static_cast<unsigned>(value->memoryAccessAligned.value));
      }
      if (MemoryAccessMask::Nontemporal == (compareWrite & MemoryAccessMask::Nontemporal))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compareWrite & MemoryAccessMask::MakePointerAvailable))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailable;
        writer.writeWord(value->memoryAccessMakePointerAvailable->resultId);
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compareWrite & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailableKHR;
        writer.writeWord(value->memoryAccessMakePointerAvailableKHR->resultId);
      }
      if (MemoryAccessMask::MakePointerVisible == (compareWrite & MemoryAccessMask::MakePointerVisible))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisible;
        writer.writeWord(value->memoryAccessMakePointerVisible->resultId);
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compareWrite & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisibleKHR;
        writer.writeWord(value->memoryAccessMakePointerVisibleKHR->resultId);
      }
      if (MemoryAccessMask::NonPrivatePointer == (compareWrite & MemoryAccessMask::NonPrivatePointer))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compareWrite & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpStore *value)
  {
    size_t len = 2;
    if (value->memoryAccess)
    {
      ++len;
      auto compare = *value->memoryAccess;
      if (MemoryAccessMask::Volatile == (compare & MemoryAccessMask::Volatile))
      {
        compare = compare ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compare & MemoryAccessMask::Aligned))
      {
        compare = compare ^ MemoryAccessMask::Aligned;
        ++len;
      }
      if (MemoryAccessMask::Nontemporal == (compare & MemoryAccessMask::Nontemporal))
      {
        compare = compare ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compare & MemoryAccessMask::MakePointerAvailable))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailable;
        ++len;
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compare & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailableKHR;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisible == (compare & MemoryAccessMask::MakePointerVisible))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisible;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compare & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisibleKHR;
        ++len;
      }
      if (MemoryAccessMask::NonPrivatePointer == (compare & MemoryAccessMask::NonPrivatePointer))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compare & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.beginInstruction(Op::OpStore, len);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->object->resultId);
    if (value->memoryAccess)
    {
      writer.writeWord(static_cast<unsigned>((*value->memoryAccess)));
      auto compareWrite = (*value->memoryAccess);
      if (MemoryAccessMask::Volatile == (compareWrite & MemoryAccessMask::Volatile))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compareWrite & MemoryAccessMask::Aligned))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Aligned;
        writer.writeWord(static_cast<unsigned>(value->memoryAccessAligned.value));
      }
      if (MemoryAccessMask::Nontemporal == (compareWrite & MemoryAccessMask::Nontemporal))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compareWrite & MemoryAccessMask::MakePointerAvailable))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailable;
        writer.writeWord(value->memoryAccessMakePointerAvailable->resultId);
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compareWrite & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailableKHR;
        writer.writeWord(value->memoryAccessMakePointerAvailableKHR->resultId);
      }
      if (MemoryAccessMask::MakePointerVisible == (compareWrite & MemoryAccessMask::MakePointerVisible))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisible;
        writer.writeWord(value->memoryAccessMakePointerVisible->resultId);
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compareWrite & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisibleKHR;
        writer.writeWord(value->memoryAccessMakePointerVisibleKHR->resultId);
      }
      if (MemoryAccessMask::NonPrivatePointer == (compareWrite & MemoryAccessMask::NonPrivatePointer))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compareWrite & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCopyMemory *value)
  {
    size_t len = 2;
    if (value->memoryAccess0)
    {
      ++len;
      auto compare = *value->memoryAccess0;
      if (MemoryAccessMask::Volatile == (compare & MemoryAccessMask::Volatile))
      {
        compare = compare ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compare & MemoryAccessMask::Aligned))
      {
        compare = compare ^ MemoryAccessMask::Aligned;
        ++len;
      }
      if (MemoryAccessMask::Nontemporal == (compare & MemoryAccessMask::Nontemporal))
      {
        compare = compare ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compare & MemoryAccessMask::MakePointerAvailable))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailable;
        ++len;
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compare & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailableKHR;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisible == (compare & MemoryAccessMask::MakePointerVisible))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisible;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compare & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisibleKHR;
        ++len;
      }
      if (MemoryAccessMask::NonPrivatePointer == (compare & MemoryAccessMask::NonPrivatePointer))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compare & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    if (value->memoryAccess1)
    {
      ++len;
      auto compare = *value->memoryAccess1;
      if (MemoryAccessMask::Volatile == (compare & MemoryAccessMask::Volatile))
      {
        compare = compare ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compare & MemoryAccessMask::Aligned))
      {
        compare = compare ^ MemoryAccessMask::Aligned;
        ++len;
      }
      if (MemoryAccessMask::Nontemporal == (compare & MemoryAccessMask::Nontemporal))
      {
        compare = compare ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compare & MemoryAccessMask::MakePointerAvailable))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailable;
        ++len;
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compare & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailableKHR;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisible == (compare & MemoryAccessMask::MakePointerVisible))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisible;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compare & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisibleKHR;
        ++len;
      }
      if (MemoryAccessMask::NonPrivatePointer == (compare & MemoryAccessMask::NonPrivatePointer))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compare & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.beginInstruction(Op::OpCopyMemory, len);
    writer.writeWord(value->target->resultId);
    writer.writeWord(value->source->resultId);
    if (value->memoryAccess0)
    {
      writer.writeWord(static_cast<unsigned>((*value->memoryAccess0)));
      auto compareWrite = (*value->memoryAccess0);
      if (MemoryAccessMask::Volatile == (compareWrite & MemoryAccessMask::Volatile))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compareWrite & MemoryAccessMask::Aligned))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Aligned;
        writer.writeWord(static_cast<unsigned>(value->memoryAccess0Aligned.value));
      }
      if (MemoryAccessMask::Nontemporal == (compareWrite & MemoryAccessMask::Nontemporal))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compareWrite & MemoryAccessMask::MakePointerAvailable))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailable;
        writer.writeWord(value->memoryAccess0MakePointerAvailable->resultId);
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compareWrite & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailableKHR;
        writer.writeWord(value->memoryAccess0MakePointerAvailableKHR->resultId);
      }
      if (MemoryAccessMask::MakePointerVisible == (compareWrite & MemoryAccessMask::MakePointerVisible))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisible;
        writer.writeWord(value->memoryAccess0MakePointerVisible->resultId);
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compareWrite & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisibleKHR;
        writer.writeWord(value->memoryAccess0MakePointerVisibleKHR->resultId);
      }
      if (MemoryAccessMask::NonPrivatePointer == (compareWrite & MemoryAccessMask::NonPrivatePointer))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compareWrite & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    if (value->memoryAccess1)
    {
      writer.writeWord(static_cast<unsigned>((*value->memoryAccess1)));
      auto compareWrite = (*value->memoryAccess1);
      if (MemoryAccessMask::Volatile == (compareWrite & MemoryAccessMask::Volatile))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compareWrite & MemoryAccessMask::Aligned))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Aligned;
        writer.writeWord(static_cast<unsigned>(value->memoryAccess1Aligned.value));
      }
      if (MemoryAccessMask::Nontemporal == (compareWrite & MemoryAccessMask::Nontemporal))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compareWrite & MemoryAccessMask::MakePointerAvailable))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailable;
        writer.writeWord(value->memoryAccess1MakePointerAvailable->resultId);
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compareWrite & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailableKHR;
        writer.writeWord(value->memoryAccess1MakePointerAvailableKHR->resultId);
      }
      if (MemoryAccessMask::MakePointerVisible == (compareWrite & MemoryAccessMask::MakePointerVisible))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisible;
        writer.writeWord(value->memoryAccess1MakePointerVisible->resultId);
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compareWrite & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisibleKHR;
        writer.writeWord(value->memoryAccess1MakePointerVisibleKHR->resultId);
      }
      if (MemoryAccessMask::NonPrivatePointer == (compareWrite & MemoryAccessMask::NonPrivatePointer))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compareWrite & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCopyMemorySized *value)
  {
    size_t len = 3;
    if (value->memoryAccess0)
    {
      ++len;
      auto compare = *value->memoryAccess0;
      if (MemoryAccessMask::Volatile == (compare & MemoryAccessMask::Volatile))
      {
        compare = compare ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compare & MemoryAccessMask::Aligned))
      {
        compare = compare ^ MemoryAccessMask::Aligned;
        ++len;
      }
      if (MemoryAccessMask::Nontemporal == (compare & MemoryAccessMask::Nontemporal))
      {
        compare = compare ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compare & MemoryAccessMask::MakePointerAvailable))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailable;
        ++len;
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compare & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailableKHR;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisible == (compare & MemoryAccessMask::MakePointerVisible))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisible;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compare & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisibleKHR;
        ++len;
      }
      if (MemoryAccessMask::NonPrivatePointer == (compare & MemoryAccessMask::NonPrivatePointer))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compare & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    if (value->memoryAccess1)
    {
      ++len;
      auto compare = *value->memoryAccess1;
      if (MemoryAccessMask::Volatile == (compare & MemoryAccessMask::Volatile))
      {
        compare = compare ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compare & MemoryAccessMask::Aligned))
      {
        compare = compare ^ MemoryAccessMask::Aligned;
        ++len;
      }
      if (MemoryAccessMask::Nontemporal == (compare & MemoryAccessMask::Nontemporal))
      {
        compare = compare ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compare & MemoryAccessMask::MakePointerAvailable))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailable;
        ++len;
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compare & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailableKHR;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisible == (compare & MemoryAccessMask::MakePointerVisible))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisible;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compare & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisibleKHR;
        ++len;
      }
      if (MemoryAccessMask::NonPrivatePointer == (compare & MemoryAccessMask::NonPrivatePointer))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compare & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.beginInstruction(Op::OpCopyMemorySized, len);
    writer.writeWord(value->target->resultId);
    writer.writeWord(value->source->resultId);
    writer.writeWord(value->size->resultId);
    if (value->memoryAccess0)
    {
      writer.writeWord(static_cast<unsigned>((*value->memoryAccess0)));
      auto compareWrite = (*value->memoryAccess0);
      if (MemoryAccessMask::Volatile == (compareWrite & MemoryAccessMask::Volatile))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compareWrite & MemoryAccessMask::Aligned))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Aligned;
        writer.writeWord(static_cast<unsigned>(value->memoryAccess0Aligned.value));
      }
      if (MemoryAccessMask::Nontemporal == (compareWrite & MemoryAccessMask::Nontemporal))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compareWrite & MemoryAccessMask::MakePointerAvailable))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailable;
        writer.writeWord(value->memoryAccess0MakePointerAvailable->resultId);
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compareWrite & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailableKHR;
        writer.writeWord(value->memoryAccess0MakePointerAvailableKHR->resultId);
      }
      if (MemoryAccessMask::MakePointerVisible == (compareWrite & MemoryAccessMask::MakePointerVisible))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisible;
        writer.writeWord(value->memoryAccess0MakePointerVisible->resultId);
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compareWrite & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisibleKHR;
        writer.writeWord(value->memoryAccess0MakePointerVisibleKHR->resultId);
      }
      if (MemoryAccessMask::NonPrivatePointer == (compareWrite & MemoryAccessMask::NonPrivatePointer))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compareWrite & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    if (value->memoryAccess1)
    {
      writer.writeWord(static_cast<unsigned>((*value->memoryAccess1)));
      auto compareWrite = (*value->memoryAccess1);
      if (MemoryAccessMask::Volatile == (compareWrite & MemoryAccessMask::Volatile))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compareWrite & MemoryAccessMask::Aligned))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Aligned;
        writer.writeWord(static_cast<unsigned>(value->memoryAccess1Aligned.value));
      }
      if (MemoryAccessMask::Nontemporal == (compareWrite & MemoryAccessMask::Nontemporal))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compareWrite & MemoryAccessMask::MakePointerAvailable))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailable;
        writer.writeWord(value->memoryAccess1MakePointerAvailable->resultId);
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compareWrite & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailableKHR;
        writer.writeWord(value->memoryAccess1MakePointerAvailableKHR->resultId);
      }
      if (MemoryAccessMask::MakePointerVisible == (compareWrite & MemoryAccessMask::MakePointerVisible))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisible;
        writer.writeWord(value->memoryAccess1MakePointerVisible->resultId);
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compareWrite & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisibleKHR;
        writer.writeWord(value->memoryAccess1MakePointerVisibleKHR->resultId);
      }
      if (MemoryAccessMask::NonPrivatePointer == (compareWrite & MemoryAccessMask::NonPrivatePointer))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compareWrite & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAccessChain *value)
  {
    size_t len = 3;
    len += value->indexes.size();
    writer.beginInstruction(Op::OpAccessChain, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    for (auto &&v : value->indexes)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpInBoundsAccessChain *value)
  {
    size_t len = 3;
    len += value->indexes.size();
    writer.beginInstruction(Op::OpInBoundsAccessChain, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    for (auto &&v : value->indexes)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpPtrAccessChain *value)
  {
    size_t len = 4;
    len += value->indexes.size();
    writer.beginInstruction(Op::OpPtrAccessChain, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.writeWord(value->element->resultId);
    for (auto &&v : value->indexes)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArrayLength *value)
  {
    writer.beginInstruction(Op::OpArrayLength, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->structure->resultId);
    writer.writeWord(value->arrayMember.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGenericPtrMemSemantics *value)
  {
    writer.beginInstruction(Op::OpGenericPtrMemSemantics, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpInBoundsPtrAccessChain *value)
  {
    size_t len = 4;
    len += value->indexes.size();
    writer.beginInstruction(Op::OpInBoundsPtrAccessChain, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.writeWord(value->element->resultId);
    for (auto &&v : value->indexes)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDecorationGroup *value)
  {
    writer.beginInstruction(Op::OpDecorationGroup, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpVectorExtractDynamic *value)
  {
    writer.beginInstruction(Op::OpVectorExtractDynamic, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector->resultId);
    writer.writeWord(value->index->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpVectorInsertDynamic *value)
  {
    writer.beginInstruction(Op::OpVectorInsertDynamic, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector->resultId);
    writer.writeWord(value->component->resultId);
    writer.writeWord(value->index->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpVectorShuffle *value)
  {
    size_t len = 4;
    len += value->components.size();
    writer.beginInstruction(Op::OpVectorShuffle, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    for (auto &&v : value->components)
    {
      writer.writeWord(v.value);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCompositeConstruct *value)
  {
    size_t len = 2;
    len += value->constituents.size();
    writer.beginInstruction(Op::OpCompositeConstruct, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    for (auto &&v : value->constituents)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCompositeExtract *value)
  {
    size_t len = 3;
    len += value->indexes.size();
    writer.beginInstruction(Op::OpCompositeExtract, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->composite->resultId);
    for (auto &&v : value->indexes)
    {
      writer.writeWord(v.value);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCompositeInsert *value)
  {
    size_t len = 4;
    len += value->indexes.size();
    writer.beginInstruction(Op::OpCompositeInsert, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->object->resultId);
    writer.writeWord(value->composite->resultId);
    for (auto &&v : value->indexes)
    {
      writer.writeWord(v.value);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCopyObject *value)
  {
    writer.beginInstruction(Op::OpCopyObject, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTranspose *value)
  {
    writer.beginInstruction(Op::OpTranspose, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->matrix->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSampledImage *value)
  {
    writer.beginInstruction(Op::OpSampledImage, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->sampler->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSampleImplicitLod *value)
  {
    size_t len = 4;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSampleImplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSampleExplicitLod *value)
  {
    size_t len = 5;
    auto compare = value->imageOperands;
    if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
    {
      compare = compare ^ ImageOperandsMask::Bias;
      ++len;
    }
    if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
    {
      compare = compare ^ ImageOperandsMask::Lod;
      ++len;
    }
    if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
    {
      compare = compare ^ ImageOperandsMask::Grad;
      ++len;
      ++len;
    }
    if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
    {
      compare = compare ^ ImageOperandsMask::ConstOffset;
      ++len;
    }
    if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
    {
      compare = compare ^ ImageOperandsMask::Offset;
      ++len;
    }
    if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
    {
      compare = compare ^ ImageOperandsMask::ConstOffsets;
      ++len;
    }
    if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
    {
      compare = compare ^ ImageOperandsMask::Sample;
      ++len;
    }
    if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
    {
      compare = compare ^ ImageOperandsMask::MinLod;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisible;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
      ++len;
    }
    if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
    {
      compare = compare ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
    {
      compare = compare ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
    {
      compare = compare ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
    {
      compare = compare ^ ImageOperandsMask::Offsets;
      ++len;
    }
    writer.beginInstruction(Op::OpImageSampleExplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(static_cast<unsigned>(value->imageOperands));
    auto compareWrite = value->imageOperands;
    if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Bias;
      writer.writeWord(value->imageOperandsBias->resultId);
    }
    if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Lod;
      writer.writeWord(value->imageOperandsLod->resultId);
    }
    if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Grad;
      writer.writeWord(value->imageOperandsGradX->resultId);
      writer.writeWord(value->imageOperandsGradY->resultId);
    }
    if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
      writer.writeWord(value->imageOperandsConstOffset->resultId);
    }
    if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offset;
      writer.writeWord(value->imageOperandsOffset->resultId);
    }
    if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
      writer.writeWord(value->imageOperandsConstOffsets->resultId);
    }
    if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Sample;
      writer.writeWord(value->imageOperandsSample->resultId);
    }
    if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
      writer.writeWord(value->imageOperandsMinLod->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
      writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
      writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
      writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
      writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
    }
    if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
      writer.writeWord(value->imageOperandsOffsets->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSampleDrefImplicitLod *value)
  {
    size_t len = 5;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSampleDrefImplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSampleDrefExplicitLod *value)
  {
    size_t len = 6;
    auto compare = value->imageOperands;
    if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
    {
      compare = compare ^ ImageOperandsMask::Bias;
      ++len;
    }
    if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
    {
      compare = compare ^ ImageOperandsMask::Lod;
      ++len;
    }
    if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
    {
      compare = compare ^ ImageOperandsMask::Grad;
      ++len;
      ++len;
    }
    if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
    {
      compare = compare ^ ImageOperandsMask::ConstOffset;
      ++len;
    }
    if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
    {
      compare = compare ^ ImageOperandsMask::Offset;
      ++len;
    }
    if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
    {
      compare = compare ^ ImageOperandsMask::ConstOffsets;
      ++len;
    }
    if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
    {
      compare = compare ^ ImageOperandsMask::Sample;
      ++len;
    }
    if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
    {
      compare = compare ^ ImageOperandsMask::MinLod;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisible;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
      ++len;
    }
    if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
    {
      compare = compare ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
    {
      compare = compare ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
    {
      compare = compare ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
    {
      compare = compare ^ ImageOperandsMask::Offsets;
      ++len;
    }
    writer.beginInstruction(Op::OpImageSampleDrefExplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    writer.writeWord(static_cast<unsigned>(value->imageOperands));
    auto compareWrite = value->imageOperands;
    if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Bias;
      writer.writeWord(value->imageOperandsBias->resultId);
    }
    if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Lod;
      writer.writeWord(value->imageOperandsLod->resultId);
    }
    if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Grad;
      writer.writeWord(value->imageOperandsGradX->resultId);
      writer.writeWord(value->imageOperandsGradY->resultId);
    }
    if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
      writer.writeWord(value->imageOperandsConstOffset->resultId);
    }
    if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offset;
      writer.writeWord(value->imageOperandsOffset->resultId);
    }
    if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
      writer.writeWord(value->imageOperandsConstOffsets->resultId);
    }
    if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Sample;
      writer.writeWord(value->imageOperandsSample->resultId);
    }
    if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
      writer.writeWord(value->imageOperandsMinLod->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
      writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
      writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
      writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
      writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
    }
    if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
      writer.writeWord(value->imageOperandsOffsets->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSampleProjImplicitLod *value)
  {
    size_t len = 4;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSampleProjImplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSampleProjExplicitLod *value)
  {
    size_t len = 5;
    auto compare = value->imageOperands;
    if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
    {
      compare = compare ^ ImageOperandsMask::Bias;
      ++len;
    }
    if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
    {
      compare = compare ^ ImageOperandsMask::Lod;
      ++len;
    }
    if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
    {
      compare = compare ^ ImageOperandsMask::Grad;
      ++len;
      ++len;
    }
    if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
    {
      compare = compare ^ ImageOperandsMask::ConstOffset;
      ++len;
    }
    if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
    {
      compare = compare ^ ImageOperandsMask::Offset;
      ++len;
    }
    if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
    {
      compare = compare ^ ImageOperandsMask::ConstOffsets;
      ++len;
    }
    if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
    {
      compare = compare ^ ImageOperandsMask::Sample;
      ++len;
    }
    if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
    {
      compare = compare ^ ImageOperandsMask::MinLod;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisible;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
      ++len;
    }
    if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
    {
      compare = compare ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
    {
      compare = compare ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
    {
      compare = compare ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
    {
      compare = compare ^ ImageOperandsMask::Offsets;
      ++len;
    }
    writer.beginInstruction(Op::OpImageSampleProjExplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(static_cast<unsigned>(value->imageOperands));
    auto compareWrite = value->imageOperands;
    if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Bias;
      writer.writeWord(value->imageOperandsBias->resultId);
    }
    if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Lod;
      writer.writeWord(value->imageOperandsLod->resultId);
    }
    if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Grad;
      writer.writeWord(value->imageOperandsGradX->resultId);
      writer.writeWord(value->imageOperandsGradY->resultId);
    }
    if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
      writer.writeWord(value->imageOperandsConstOffset->resultId);
    }
    if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offset;
      writer.writeWord(value->imageOperandsOffset->resultId);
    }
    if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
      writer.writeWord(value->imageOperandsConstOffsets->resultId);
    }
    if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Sample;
      writer.writeWord(value->imageOperandsSample->resultId);
    }
    if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
      writer.writeWord(value->imageOperandsMinLod->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
      writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
      writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
      writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
      writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
    }
    if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
      writer.writeWord(value->imageOperandsOffsets->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSampleProjDrefImplicitLod *value)
  {
    size_t len = 5;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSampleProjDrefImplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSampleProjDrefExplicitLod *value)
  {
    size_t len = 6;
    auto compare = value->imageOperands;
    if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
    {
      compare = compare ^ ImageOperandsMask::Bias;
      ++len;
    }
    if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
    {
      compare = compare ^ ImageOperandsMask::Lod;
      ++len;
    }
    if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
    {
      compare = compare ^ ImageOperandsMask::Grad;
      ++len;
      ++len;
    }
    if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
    {
      compare = compare ^ ImageOperandsMask::ConstOffset;
      ++len;
    }
    if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
    {
      compare = compare ^ ImageOperandsMask::Offset;
      ++len;
    }
    if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
    {
      compare = compare ^ ImageOperandsMask::ConstOffsets;
      ++len;
    }
    if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
    {
      compare = compare ^ ImageOperandsMask::Sample;
      ++len;
    }
    if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
    {
      compare = compare ^ ImageOperandsMask::MinLod;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisible;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
      ++len;
    }
    if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
    {
      compare = compare ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
    {
      compare = compare ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
    {
      compare = compare ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
    {
      compare = compare ^ ImageOperandsMask::Offsets;
      ++len;
    }
    writer.beginInstruction(Op::OpImageSampleProjDrefExplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    writer.writeWord(static_cast<unsigned>(value->imageOperands));
    auto compareWrite = value->imageOperands;
    if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Bias;
      writer.writeWord(value->imageOperandsBias->resultId);
    }
    if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Lod;
      writer.writeWord(value->imageOperandsLod->resultId);
    }
    if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Grad;
      writer.writeWord(value->imageOperandsGradX->resultId);
      writer.writeWord(value->imageOperandsGradY->resultId);
    }
    if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
      writer.writeWord(value->imageOperandsConstOffset->resultId);
    }
    if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offset;
      writer.writeWord(value->imageOperandsOffset->resultId);
    }
    if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
      writer.writeWord(value->imageOperandsConstOffsets->resultId);
    }
    if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Sample;
      writer.writeWord(value->imageOperandsSample->resultId);
    }
    if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
      writer.writeWord(value->imageOperandsMinLod->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
      writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
      writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
      writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
      writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
    }
    if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
      writer.writeWord(value->imageOperandsOffsets->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageFetch *value)
  {
    size_t len = 4;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageFetch, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageGather *value)
  {
    size_t len = 5;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageGather, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->component->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageDrefGather *value)
  {
    size_t len = 5;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageDrefGather, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageRead *value)
  {
    size_t len = 4;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageRead, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageWrite *value)
  {
    size_t len = 3;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageWrite, len);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->texel->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImage *value)
  {
    writer.beginInstruction(Op::OpImage, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageQueryFormat *value)
  {
    writer.beginInstruction(Op::OpImageQueryFormat, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageQueryOrder *value)
  {
    writer.beginInstruction(Op::OpImageQueryOrder, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageQuerySizeLod *value)
  {
    writer.beginInstruction(Op::OpImageQuerySizeLod, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->levelOfDetail->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageQuerySize *value)
  {
    writer.beginInstruction(Op::OpImageQuerySize, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageQueryLod *value)
  {
    writer.beginInstruction(Op::OpImageQueryLod, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageQueryLevels *value)
  {
    writer.beginInstruction(Op::OpImageQueryLevels, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageQuerySamples *value)
  {
    writer.beginInstruction(Op::OpImageQuerySamples, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertFToU *value)
  {
    writer.beginInstruction(Op::OpConvertFToU, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->floatValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertFToS *value)
  {
    writer.beginInstruction(Op::OpConvertFToS, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->floatValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertSToF *value)
  {
    writer.beginInstruction(Op::OpConvertSToF, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->signedValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertUToF *value)
  {
    writer.beginInstruction(Op::OpConvertUToF, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->unsignedValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUConvert *value)
  {
    writer.beginInstruction(Op::OpUConvert, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->unsignedValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSConvert *value)
  {
    writer.beginInstruction(Op::OpSConvert, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->signedValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFConvert *value)
  {
    writer.beginInstruction(Op::OpFConvert, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->floatValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpQuantizeToF16 *value)
  {
    writer.beginInstruction(Op::OpQuantizeToF16, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertPtrToU *value)
  {
    writer.beginInstruction(Op::OpConvertPtrToU, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSatConvertSToU *value)
  {
    writer.beginInstruction(Op::OpSatConvertSToU, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->signedValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSatConvertUToS *value)
  {
    writer.beginInstruction(Op::OpSatConvertUToS, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->unsignedValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertUToPtr *value)
  {
    writer.beginInstruction(Op::OpConvertUToPtr, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->integerValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpPtrCastToGeneric *value)
  {
    writer.beginInstruction(Op::OpPtrCastToGeneric, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGenericCastToPtr *value)
  {
    writer.beginInstruction(Op::OpGenericCastToPtr, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGenericCastToPtrExplicit *value)
  {
    writer.beginInstruction(Op::OpGenericCastToPtrExplicit, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(static_cast<unsigned>(value->storage));
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBitcast *value)
  {
    writer.beginInstruction(Op::OpBitcast, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSNegate *value)
  {
    writer.beginInstruction(Op::OpSNegate, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFNegate *value)
  {
    writer.beginInstruction(Op::OpFNegate, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIAdd *value)
  {
    writer.beginInstruction(Op::OpIAdd, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFAdd *value)
  {
    writer.beginInstruction(Op::OpFAdd, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpISub *value)
  {
    writer.beginInstruction(Op::OpISub, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFSub *value)
  {
    writer.beginInstruction(Op::OpFSub, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIMul *value)
  {
    writer.beginInstruction(Op::OpIMul, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFMul *value)
  {
    writer.beginInstruction(Op::OpFMul, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUDiv *value)
  {
    writer.beginInstruction(Op::OpUDiv, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSDiv *value)
  {
    writer.beginInstruction(Op::OpSDiv, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFDiv *value)
  {
    writer.beginInstruction(Op::OpFDiv, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUMod *value)
  {
    writer.beginInstruction(Op::OpUMod, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSRem *value)
  {
    writer.beginInstruction(Op::OpSRem, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSMod *value)
  {
    writer.beginInstruction(Op::OpSMod, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFRem *value)
  {
    writer.beginInstruction(Op::OpFRem, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFMod *value)
  {
    writer.beginInstruction(Op::OpFMod, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpVectorTimesScalar *value)
  {
    writer.beginInstruction(Op::OpVectorTimesScalar, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector->resultId);
    writer.writeWord(value->scalar->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpMatrixTimesScalar *value)
  {
    writer.beginInstruction(Op::OpMatrixTimesScalar, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->matrix->resultId);
    writer.writeWord(value->scalar->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpVectorTimesMatrix *value)
  {
    writer.beginInstruction(Op::OpVectorTimesMatrix, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector->resultId);
    writer.writeWord(value->matrix->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpMatrixTimesVector *value)
  {
    writer.beginInstruction(Op::OpMatrixTimesVector, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->matrix->resultId);
    writer.writeWord(value->vector->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpMatrixTimesMatrix *value)
  {
    writer.beginInstruction(Op::OpMatrixTimesMatrix, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->leftMatrix->resultId);
    writer.writeWord(value->rightMatrix->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpOuterProduct *value)
  {
    writer.beginInstruction(Op::OpOuterProduct, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDot *value)
  {
    writer.beginInstruction(Op::OpDot, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIAddCarry *value)
  {
    writer.beginInstruction(Op::OpIAddCarry, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpISubBorrow *value)
  {
    writer.beginInstruction(Op::OpISubBorrow, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUMulExtended *value)
  {
    writer.beginInstruction(Op::OpUMulExtended, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSMulExtended *value)
  {
    writer.beginInstruction(Op::OpSMulExtended, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAny *value)
  {
    writer.beginInstruction(Op::OpAny, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAll *value)
  {
    writer.beginInstruction(Op::OpAll, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIsNan *value)
  {
    writer.beginInstruction(Op::OpIsNan, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIsInf *value)
  {
    writer.beginInstruction(Op::OpIsInf, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIsFinite *value)
  {
    writer.beginInstruction(Op::OpIsFinite, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIsNormal *value)
  {
    writer.beginInstruction(Op::OpIsNormal, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSignBitSet *value)
  {
    writer.beginInstruction(Op::OpSignBitSet, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLessOrGreater *value)
  {
    writer.beginInstruction(Op::OpLessOrGreater, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpOrdered *value)
  {
    writer.beginInstruction(Op::OpOrdered, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUnordered *value)
  {
    writer.beginInstruction(Op::OpUnordered, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLogicalEqual *value)
  {
    writer.beginInstruction(Op::OpLogicalEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLogicalNotEqual *value)
  {
    writer.beginInstruction(Op::OpLogicalNotEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLogicalOr *value)
  {
    writer.beginInstruction(Op::OpLogicalOr, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLogicalAnd *value)
  {
    writer.beginInstruction(Op::OpLogicalAnd, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLogicalNot *value)
  {
    writer.beginInstruction(Op::OpLogicalNot, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSelect *value)
  {
    writer.beginInstruction(Op::OpSelect, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->condition->resultId);
    writer.writeWord(value->object1->resultId);
    writer.writeWord(value->object2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIEqual *value)
  {
    writer.beginInstruction(Op::OpIEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpINotEqual *value)
  {
    writer.beginInstruction(Op::OpINotEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUGreaterThan *value)
  {
    writer.beginInstruction(Op::OpUGreaterThan, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSGreaterThan *value)
  {
    writer.beginInstruction(Op::OpSGreaterThan, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUGreaterThanEqual *value)
  {
    writer.beginInstruction(Op::OpUGreaterThanEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSGreaterThanEqual *value)
  {
    writer.beginInstruction(Op::OpSGreaterThanEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpULessThan *value)
  {
    writer.beginInstruction(Op::OpULessThan, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSLessThan *value)
  {
    writer.beginInstruction(Op::OpSLessThan, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpULessThanEqual *value)
  {
    writer.beginInstruction(Op::OpULessThanEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSLessThanEqual *value)
  {
    writer.beginInstruction(Op::OpSLessThanEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFOrdEqual *value)
  {
    writer.beginInstruction(Op::OpFOrdEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFUnordEqual *value)
  {
    writer.beginInstruction(Op::OpFUnordEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFOrdNotEqual *value)
  {
    writer.beginInstruction(Op::OpFOrdNotEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFUnordNotEqual *value)
  {
    writer.beginInstruction(Op::OpFUnordNotEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFOrdLessThan *value)
  {
    writer.beginInstruction(Op::OpFOrdLessThan, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFUnordLessThan *value)
  {
    writer.beginInstruction(Op::OpFUnordLessThan, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFOrdGreaterThan *value)
  {
    writer.beginInstruction(Op::OpFOrdGreaterThan, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFUnordGreaterThan *value)
  {
    writer.beginInstruction(Op::OpFUnordGreaterThan, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFOrdLessThanEqual *value)
  {
    writer.beginInstruction(Op::OpFOrdLessThanEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFUnordLessThanEqual *value)
  {
    writer.beginInstruction(Op::OpFUnordLessThanEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFOrdGreaterThanEqual *value)
  {
    writer.beginInstruction(Op::OpFOrdGreaterThanEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFUnordGreaterThanEqual *value)
  {
    writer.beginInstruction(Op::OpFUnordGreaterThanEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpShiftRightLogical *value)
  {
    writer.beginInstruction(Op::OpShiftRightLogical, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.writeWord(value->shift->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpShiftRightArithmetic *value)
  {
    writer.beginInstruction(Op::OpShiftRightArithmetic, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.writeWord(value->shift->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpShiftLeftLogical *value)
  {
    writer.beginInstruction(Op::OpShiftLeftLogical, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.writeWord(value->shift->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBitwiseOr *value)
  {
    writer.beginInstruction(Op::OpBitwiseOr, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBitwiseXor *value)
  {
    writer.beginInstruction(Op::OpBitwiseXor, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBitwiseAnd *value)
  {
    writer.beginInstruction(Op::OpBitwiseAnd, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpNot *value)
  {
    writer.beginInstruction(Op::OpNot, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBitFieldInsert *value)
  {
    writer.beginInstruction(Op::OpBitFieldInsert, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.writeWord(value->insert->resultId);
    writer.writeWord(value->offset->resultId);
    writer.writeWord(value->count->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBitFieldSExtract *value)
  {
    writer.beginInstruction(Op::OpBitFieldSExtract, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.writeWord(value->offset->resultId);
    writer.writeWord(value->count->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBitFieldUExtract *value)
  {
    writer.beginInstruction(Op::OpBitFieldUExtract, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.writeWord(value->offset->resultId);
    writer.writeWord(value->count->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBitReverse *value)
  {
    writer.beginInstruction(Op::OpBitReverse, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBitCount *value)
  {
    writer.beginInstruction(Op::OpBitCount, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->base->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDPdx *value)
  {
    writer.beginInstruction(Op::OpDPdx, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDPdy *value)
  {
    writer.beginInstruction(Op::OpDPdy, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFwidth *value)
  {
    writer.beginInstruction(Op::OpFwidth, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDPdxFine *value)
  {
    writer.beginInstruction(Op::OpDPdxFine, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDPdyFine *value)
  {
    writer.beginInstruction(Op::OpDPdyFine, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFwidthFine *value)
  {
    writer.beginInstruction(Op::OpFwidthFine, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDPdxCoarse *value)
  {
    writer.beginInstruction(Op::OpDPdxCoarse, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDPdyCoarse *value)
  {
    writer.beginInstruction(Op::OpDPdyCoarse, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFwidthCoarse *value)
  {
    writer.beginInstruction(Op::OpFwidthCoarse, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpEmitVertex *value)
  {
    writer.beginInstruction(Op::OpEmitVertex, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpEndPrimitive *value)
  {
    writer.beginInstruction(Op::OpEndPrimitive, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpEmitStreamVertex *value)
  {
    writer.beginInstruction(Op::OpEmitStreamVertex, 1);
    writer.writeWord(value->stream->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpEndStreamPrimitive *value)
  {
    writer.beginInstruction(Op::OpEndStreamPrimitive, 1);
    writer.writeWord(value->stream->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpControlBarrier *value)
  {
    writer.beginInstruction(Op::OpControlBarrier, 3);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpMemoryBarrier *value)
  {
    writer.beginInstruction(Op::OpMemoryBarrier, 2);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicLoad *value)
  {
    writer.beginInstruction(Op::OpAtomicLoad, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicStore *value)
  {
    writer.beginInstruction(Op::OpAtomicStore, 4);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicExchange *value)
  {
    writer.beginInstruction(Op::OpAtomicExchange, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicCompareExchange *value)
  {
    writer.beginInstruction(Op::OpAtomicCompareExchange, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->equal->resultId);
    writer.writeWord(value->unequal->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->comparator->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicCompareExchangeWeak *value)
  {
    writer.beginInstruction(Op::OpAtomicCompareExchangeWeak, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->equal->resultId);
    writer.writeWord(value->unequal->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->comparator->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicIIncrement *value)
  {
    writer.beginInstruction(Op::OpAtomicIIncrement, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicIDecrement *value)
  {
    writer.beginInstruction(Op::OpAtomicIDecrement, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicIAdd *value)
  {
    writer.beginInstruction(Op::OpAtomicIAdd, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicISub *value)
  {
    writer.beginInstruction(Op::OpAtomicISub, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicSMin *value)
  {
    writer.beginInstruction(Op::OpAtomicSMin, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicUMin *value)
  {
    writer.beginInstruction(Op::OpAtomicUMin, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicSMax *value)
  {
    writer.beginInstruction(Op::OpAtomicSMax, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicUMax *value)
  {
    writer.beginInstruction(Op::OpAtomicUMax, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicAnd *value)
  {
    writer.beginInstruction(Op::OpAtomicAnd, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicOr *value)
  {
    writer.beginInstruction(Op::OpAtomicOr, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicXor *value)
  {
    writer.beginInstruction(Op::OpAtomicXor, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpPhi *value)
  {
    writer.beginInstruction(Op::OpPhi, 2 + value->sources.size() * 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    for (auto &&source : value->sources)
    {
      writer.writeWord(source.source->resultId);
      writer.writeWord(source.block->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLabel *value)
  {
    writer.beginInstruction(Op::OpLabel, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBranch *value)
  {
    writer.beginInstruction(Op::OpBranch, 1);
    writer.writeWord(value->targetLabel->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBranchConditional *value)
  {
    size_t len = 3;
    len += value->branchWeights.size();
    writer.beginInstruction(Op::OpBranchConditional, len);
    writer.writeWord(value->condition->resultId);
    writer.writeWord(value->trueLabel->resultId);
    writer.writeWord(value->falseLabel->resultId);
    for (auto &&v : value->branchWeights)
    {
      writer.writeWord(v.value);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSwitch *value)
  {
    writer.beginInstruction(Op::OpSwitch, 2 + 2 * value->cases.size());
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->defaultBlock->resultId);
    for (auto &&cse : value->cases)
    {
      writer.writeWord(static_cast<unsigned>(cse.value.value));
      writer.writeWord(cse.block->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpKill *value)
  {
    writer.beginInstruction(Op::OpKill, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReturn *value)
  {
    writer.beginInstruction(Op::OpReturn, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReturnValue *value)
  {
    writer.beginInstruction(Op::OpReturnValue, 1);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUnreachable *value)
  {
    writer.beginInstruction(Op::OpUnreachable, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLifetimeStart *value)
  {
    writer.beginInstruction(Op::OpLifetimeStart, 2);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->size.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpLifetimeStop *value)
  {
    writer.beginInstruction(Op::OpLifetimeStop, 2);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->size.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupAsyncCopy *value)
  {
    writer.beginInstruction(Op::OpGroupAsyncCopy, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->destination->resultId);
    writer.writeWord(value->source->resultId);
    writer.writeWord(value->numElements->resultId);
    writer.writeWord(value->stride->resultId);
    writer.writeWord(value->event->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupWaitEvents *value)
  {
    writer.beginInstruction(Op::OpGroupWaitEvents, 3);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->numEvents->resultId);
    writer.writeWord(value->eventsList->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupAll *value)
  {
    writer.beginInstruction(Op::OpGroupAll, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->predicate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupAny *value)
  {
    writer.beginInstruction(Op::OpGroupAny, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->predicate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupBroadcast *value)
  {
    writer.beginInstruction(Op::OpGroupBroadcast, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->localId->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupIAdd *value)
  {
    writer.beginInstruction(Op::OpGroupIAdd, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupFAdd *value)
  {
    writer.beginInstruction(Op::OpGroupFAdd, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupFMin *value)
  {
    writer.beginInstruction(Op::OpGroupFMin, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupUMin *value)
  {
    writer.beginInstruction(Op::OpGroupUMin, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupSMin *value)
  {
    writer.beginInstruction(Op::OpGroupSMin, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupFMax *value)
  {
    writer.beginInstruction(Op::OpGroupFMax, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupUMax *value)
  {
    writer.beginInstruction(Op::OpGroupUMax, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupSMax *value)
  {
    writer.beginInstruction(Op::OpGroupSMax, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReadPipe *value)
  {
    writer.beginInstruction(Op::OpReadPipe, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpWritePipe *value)
  {
    writer.beginInstruction(Op::OpWritePipe, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReservedReadPipe *value)
  {
    writer.beginInstruction(Op::OpReservedReadPipe, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->reserveId->resultId);
    writer.writeWord(value->index->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReservedWritePipe *value)
  {
    writer.beginInstruction(Op::OpReservedWritePipe, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->reserveId->resultId);
    writer.writeWord(value->index->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReserveReadPipePackets *value)
  {
    writer.beginInstruction(Op::OpReserveReadPipePackets, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->numPackets->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReserveWritePipePackets *value)
  {
    writer.beginInstruction(Op::OpReserveWritePipePackets, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->numPackets->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCommitReadPipe *value)
  {
    writer.beginInstruction(Op::OpCommitReadPipe, 4);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->reserveId->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCommitWritePipe *value)
  {
    writer.beginInstruction(Op::OpCommitWritePipe, 4);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->reserveId->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIsValidReserveId *value)
  {
    writer.beginInstruction(Op::OpIsValidReserveId, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->reserveId->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGetNumPipePackets *value)
  {
    writer.beginInstruction(Op::OpGetNumPipePackets, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGetMaxPipePackets *value)
  {
    writer.beginInstruction(Op::OpGetMaxPipePackets, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupReserveReadPipePackets *value)
  {
    writer.beginInstruction(Op::OpGroupReserveReadPipePackets, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->numPackets->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupReserveWritePipePackets *value)
  {
    writer.beginInstruction(Op::OpGroupReserveWritePipePackets, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->numPackets->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupCommitReadPipe *value)
  {
    writer.beginInstruction(Op::OpGroupCommitReadPipe, 5);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->reserveId->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupCommitWritePipe *value)
  {
    writer.beginInstruction(Op::OpGroupCommitWritePipe, 5);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->pipe->resultId);
    writer.writeWord(value->reserveId->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpEnqueueMarker *value)
  {
    writer.beginInstruction(Op::OpEnqueueMarker, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->queue->resultId);
    writer.writeWord(value->numEvents->resultId);
    writer.writeWord(value->waitEvents->resultId);
    writer.writeWord(value->retEvent->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpEnqueueKernel *value)
  {
    size_t len = 12;
    len += value->localSize.size();
    writer.beginInstruction(Op::OpEnqueueKernel, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->queue->resultId);
    writer.writeWord(value->flags->resultId);
    writer.writeWord(value->ndRange->resultId);
    writer.writeWord(value->numEvents->resultId);
    writer.writeWord(value->waitEvents->resultId);
    writer.writeWord(value->retEvent->resultId);
    writer.writeWord(value->invoke->resultId);
    writer.writeWord(value->param->resultId);
    writer.writeWord(value->paramSize->resultId);
    writer.writeWord(value->paramAlign->resultId);
    for (auto &&v : value->localSize)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGetKernelNDrangeSubGroupCount *value)
  {
    writer.beginInstruction(Op::OpGetKernelNDrangeSubGroupCount, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->ndRange->resultId);
    writer.writeWord(value->invoke->resultId);
    writer.writeWord(value->param->resultId);
    writer.writeWord(value->paramSize->resultId);
    writer.writeWord(value->paramAlign->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGetKernelNDrangeMaxSubGroupSize *value)
  {
    writer.beginInstruction(Op::OpGetKernelNDrangeMaxSubGroupSize, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->ndRange->resultId);
    writer.writeWord(value->invoke->resultId);
    writer.writeWord(value->param->resultId);
    writer.writeWord(value->paramSize->resultId);
    writer.writeWord(value->paramAlign->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGetKernelWorkGroupSize *value)
  {
    writer.beginInstruction(Op::OpGetKernelWorkGroupSize, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->invoke->resultId);
    writer.writeWord(value->param->resultId);
    writer.writeWord(value->paramSize->resultId);
    writer.writeWord(value->paramAlign->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGetKernelPreferredWorkGroupSizeMultiple *value)
  {
    writer.beginInstruction(Op::OpGetKernelPreferredWorkGroupSizeMultiple, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->invoke->resultId);
    writer.writeWord(value->param->resultId);
    writer.writeWord(value->paramSize->resultId);
    writer.writeWord(value->paramAlign->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRetainEvent *value)
  {
    writer.beginInstruction(Op::OpRetainEvent, 1);
    writer.writeWord(value->event->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReleaseEvent *value)
  {
    writer.beginInstruction(Op::OpReleaseEvent, 1);
    writer.writeWord(value->event->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCreateUserEvent *value)
  {
    writer.beginInstruction(Op::OpCreateUserEvent, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIsValidEvent *value)
  {
    writer.beginInstruction(Op::OpIsValidEvent, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->event->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSetUserEventStatus *value)
  {
    writer.beginInstruction(Op::OpSetUserEventStatus, 2);
    writer.writeWord(value->event->resultId);
    writer.writeWord(value->status->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCaptureEventProfilingInfo *value)
  {
    writer.beginInstruction(Op::OpCaptureEventProfilingInfo, 3);
    writer.writeWord(value->event->resultId);
    writer.writeWord(value->profilingInfo->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGetDefaultQueue *value)
  {
    writer.beginInstruction(Op::OpGetDefaultQueue, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBuildNDRange *value)
  {
    writer.beginInstruction(Op::OpBuildNDRange, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->globalWorkSize->resultId);
    writer.writeWord(value->localWorkSize->resultId);
    writer.writeWord(value->globalWorkOffset->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseSampleImplicitLod *value)
  {
    size_t len = 4;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSparseSampleImplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseSampleExplicitLod *value)
  {
    size_t len = 5;
    auto compare = value->imageOperands;
    if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
    {
      compare = compare ^ ImageOperandsMask::Bias;
      ++len;
    }
    if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
    {
      compare = compare ^ ImageOperandsMask::Lod;
      ++len;
    }
    if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
    {
      compare = compare ^ ImageOperandsMask::Grad;
      ++len;
      ++len;
    }
    if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
    {
      compare = compare ^ ImageOperandsMask::ConstOffset;
      ++len;
    }
    if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
    {
      compare = compare ^ ImageOperandsMask::Offset;
      ++len;
    }
    if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
    {
      compare = compare ^ ImageOperandsMask::ConstOffsets;
      ++len;
    }
    if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
    {
      compare = compare ^ ImageOperandsMask::Sample;
      ++len;
    }
    if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
    {
      compare = compare ^ ImageOperandsMask::MinLod;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisible;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
      ++len;
    }
    if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
    {
      compare = compare ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
    {
      compare = compare ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
    {
      compare = compare ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
    {
      compare = compare ^ ImageOperandsMask::Offsets;
      ++len;
    }
    writer.beginInstruction(Op::OpImageSparseSampleExplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(static_cast<unsigned>(value->imageOperands));
    auto compareWrite = value->imageOperands;
    if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Bias;
      writer.writeWord(value->imageOperandsBias->resultId);
    }
    if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Lod;
      writer.writeWord(value->imageOperandsLod->resultId);
    }
    if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Grad;
      writer.writeWord(value->imageOperandsGradX->resultId);
      writer.writeWord(value->imageOperandsGradY->resultId);
    }
    if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
      writer.writeWord(value->imageOperandsConstOffset->resultId);
    }
    if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offset;
      writer.writeWord(value->imageOperandsOffset->resultId);
    }
    if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
      writer.writeWord(value->imageOperandsConstOffsets->resultId);
    }
    if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Sample;
      writer.writeWord(value->imageOperandsSample->resultId);
    }
    if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
      writer.writeWord(value->imageOperandsMinLod->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
      writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
      writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
      writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
      writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
    }
    if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
      writer.writeWord(value->imageOperandsOffsets->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseSampleDrefImplicitLod *value)
  {
    size_t len = 5;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSparseSampleDrefImplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseSampleDrefExplicitLod *value)
  {
    size_t len = 6;
    auto compare = value->imageOperands;
    if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
    {
      compare = compare ^ ImageOperandsMask::Bias;
      ++len;
    }
    if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
    {
      compare = compare ^ ImageOperandsMask::Lod;
      ++len;
    }
    if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
    {
      compare = compare ^ ImageOperandsMask::Grad;
      ++len;
      ++len;
    }
    if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
    {
      compare = compare ^ ImageOperandsMask::ConstOffset;
      ++len;
    }
    if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
    {
      compare = compare ^ ImageOperandsMask::Offset;
      ++len;
    }
    if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
    {
      compare = compare ^ ImageOperandsMask::ConstOffsets;
      ++len;
    }
    if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
    {
      compare = compare ^ ImageOperandsMask::Sample;
      ++len;
    }
    if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
    {
      compare = compare ^ ImageOperandsMask::MinLod;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisible;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
      ++len;
    }
    if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
    {
      compare = compare ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
    {
      compare = compare ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
    {
      compare = compare ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
    {
      compare = compare ^ ImageOperandsMask::Offsets;
      ++len;
    }
    writer.beginInstruction(Op::OpImageSparseSampleDrefExplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    writer.writeWord(static_cast<unsigned>(value->imageOperands));
    auto compareWrite = value->imageOperands;
    if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Bias;
      writer.writeWord(value->imageOperandsBias->resultId);
    }
    if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Lod;
      writer.writeWord(value->imageOperandsLod->resultId);
    }
    if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Grad;
      writer.writeWord(value->imageOperandsGradX->resultId);
      writer.writeWord(value->imageOperandsGradY->resultId);
    }
    if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
      writer.writeWord(value->imageOperandsConstOffset->resultId);
    }
    if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offset;
      writer.writeWord(value->imageOperandsOffset->resultId);
    }
    if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
      writer.writeWord(value->imageOperandsConstOffsets->resultId);
    }
    if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Sample;
      writer.writeWord(value->imageOperandsSample->resultId);
    }
    if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
      writer.writeWord(value->imageOperandsMinLod->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
      writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
      writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
      writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
      writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
    }
    if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
      writer.writeWord(value->imageOperandsOffsets->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseSampleProjImplicitLod *value)
  {
    size_t len = 4;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSparseSampleProjImplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseSampleProjExplicitLod *value)
  {
    size_t len = 5;
    auto compare = value->imageOperands;
    if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
    {
      compare = compare ^ ImageOperandsMask::Bias;
      ++len;
    }
    if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
    {
      compare = compare ^ ImageOperandsMask::Lod;
      ++len;
    }
    if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
    {
      compare = compare ^ ImageOperandsMask::Grad;
      ++len;
      ++len;
    }
    if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
    {
      compare = compare ^ ImageOperandsMask::ConstOffset;
      ++len;
    }
    if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
    {
      compare = compare ^ ImageOperandsMask::Offset;
      ++len;
    }
    if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
    {
      compare = compare ^ ImageOperandsMask::ConstOffsets;
      ++len;
    }
    if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
    {
      compare = compare ^ ImageOperandsMask::Sample;
      ++len;
    }
    if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
    {
      compare = compare ^ ImageOperandsMask::MinLod;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisible;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
      ++len;
    }
    if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
    {
      compare = compare ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
    {
      compare = compare ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
    {
      compare = compare ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
    {
      compare = compare ^ ImageOperandsMask::Offsets;
      ++len;
    }
    writer.beginInstruction(Op::OpImageSparseSampleProjExplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(static_cast<unsigned>(value->imageOperands));
    auto compareWrite = value->imageOperands;
    if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Bias;
      writer.writeWord(value->imageOperandsBias->resultId);
    }
    if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Lod;
      writer.writeWord(value->imageOperandsLod->resultId);
    }
    if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Grad;
      writer.writeWord(value->imageOperandsGradX->resultId);
      writer.writeWord(value->imageOperandsGradY->resultId);
    }
    if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
      writer.writeWord(value->imageOperandsConstOffset->resultId);
    }
    if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offset;
      writer.writeWord(value->imageOperandsOffset->resultId);
    }
    if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
      writer.writeWord(value->imageOperandsConstOffsets->resultId);
    }
    if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Sample;
      writer.writeWord(value->imageOperandsSample->resultId);
    }
    if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
      writer.writeWord(value->imageOperandsMinLod->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
      writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
      writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
      writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
      writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
    }
    if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
      writer.writeWord(value->imageOperandsOffsets->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseSampleProjDrefImplicitLod *value)
  {
    size_t len = 5;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSparseSampleProjDrefImplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseSampleProjDrefExplicitLod *value)
  {
    size_t len = 6;
    auto compare = value->imageOperands;
    if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
    {
      compare = compare ^ ImageOperandsMask::Bias;
      ++len;
    }
    if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
    {
      compare = compare ^ ImageOperandsMask::Lod;
      ++len;
    }
    if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
    {
      compare = compare ^ ImageOperandsMask::Grad;
      ++len;
      ++len;
    }
    if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
    {
      compare = compare ^ ImageOperandsMask::ConstOffset;
      ++len;
    }
    if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
    {
      compare = compare ^ ImageOperandsMask::Offset;
      ++len;
    }
    if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
    {
      compare = compare ^ ImageOperandsMask::ConstOffsets;
      ++len;
    }
    if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
    {
      compare = compare ^ ImageOperandsMask::Sample;
      ++len;
    }
    if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
    {
      compare = compare ^ ImageOperandsMask::MinLod;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisible;
      ++len;
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
      ++len;
    }
    if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
    {
      compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
    {
      compare = compare ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
    {
      compare = compare ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
    {
      compare = compare ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
    {
      compare = compare ^ ImageOperandsMask::Offsets;
      ++len;
    }
    writer.beginInstruction(Op::OpImageSparseSampleProjDrefExplicitLod, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    writer.writeWord(static_cast<unsigned>(value->imageOperands));
    auto compareWrite = value->imageOperands;
    if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Bias;
      writer.writeWord(value->imageOperandsBias->resultId);
    }
    if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Lod;
      writer.writeWord(value->imageOperandsLod->resultId);
    }
    if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Grad;
      writer.writeWord(value->imageOperandsGradX->resultId);
      writer.writeWord(value->imageOperandsGradY->resultId);
    }
    if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
      writer.writeWord(value->imageOperandsConstOffset->resultId);
    }
    if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offset;
      writer.writeWord(value->imageOperandsOffset->resultId);
    }
    if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
      writer.writeWord(value->imageOperandsConstOffsets->resultId);
    }
    if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Sample;
      writer.writeWord(value->imageOperandsSample->resultId);
    }
    if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
      writer.writeWord(value->imageOperandsMinLod->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
      writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
    }
    if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
      writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
      writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
    }
    if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
      writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
    }
    if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
    }
    if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
    }
    if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
    }
    if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
    }
    if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
    }
    if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
    }
    if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
    }
    if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
    {
      compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
      writer.writeWord(value->imageOperandsOffsets->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseFetch *value)
  {
    size_t len = 4;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSparseFetch, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseGather *value)
  {
    size_t len = 5;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSparseGather, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->component->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseDrefGather *value)
  {
    size_t len = 5;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSparseDrefGather, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->dRef->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseTexelsResident *value)
  {
    writer.beginInstruction(Op::OpImageSparseTexelsResident, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->residentCode->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpNoLine *value)
  {
    writer.beginInstruction(Op::OpNoLine, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicFlagTestAndSet *value)
  {
    writer.beginInstruction(Op::OpAtomicFlagTestAndSet, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicFlagClear *value)
  {
    writer.beginInstruction(Op::OpAtomicFlagClear, 3);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSparseRead *value)
  {
    size_t len = 4;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSparseRead, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSizeOf *value)
  {
    writer.beginInstruction(Op::OpSizeOf, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypePipeStorage *value)
  {
    writer.beginInstruction(Op::OpTypePipeStorage, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConstantPipeStorage *value)
  {
    writer.beginInstruction(Op::OpConstantPipeStorage, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->packetSize.value);
    writer.writeWord(value->packetAlignment.value);
    writer.writeWord(value->capacity.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCreatePipeFromPipeStorage *value)
  {
    writer.beginInstruction(Op::OpCreatePipeFromPipeStorage, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pipeStorage->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGetKernelLocalSizeForSubgroupCount *value)
  {
    writer.beginInstruction(Op::OpGetKernelLocalSizeForSubgroupCount, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->subgroupCount->resultId);
    writer.writeWord(value->invoke->resultId);
    writer.writeWord(value->param->resultId);
    writer.writeWord(value->paramSize->resultId);
    writer.writeWord(value->paramAlign->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGetKernelMaxNumSubgroups *value)
  {
    writer.beginInstruction(Op::OpGetKernelMaxNumSubgroups, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->invoke->resultId);
    writer.writeWord(value->param->resultId);
    writer.writeWord(value->paramSize->resultId);
    writer.writeWord(value->paramAlign->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeNamedBarrier *value)
  {
    writer.beginInstruction(Op::OpTypeNamedBarrier, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpNamedBarrierInitialize *value)
  {
    writer.beginInstruction(Op::OpNamedBarrierInitialize, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->subgroupCount->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpMemoryNamedBarrier *value)
  {
    writer.beginInstruction(Op::OpMemoryNamedBarrier, 3);
    writer.writeWord(value->namedBarrier->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformElect *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformElect, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformAll *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformAll, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->predicate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformAny *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformAny, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->predicate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformAllEqual *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformAllEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBroadcast *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformBroadcast, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->id->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBroadcastFirst *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformBroadcastFirst, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBallot *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformBallot, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->predicate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformInverseBallot *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformInverseBallot, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBallotBitExtract *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformBallotBitExtract, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->index->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBallotBitCount *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformBallotBitCount, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBallotFindLSB *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformBallotFindLSB, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBallotFindMSB *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformBallotFindMSB, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformShuffle *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformShuffle, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->id->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformShuffleXor *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformShuffleXor, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->mask->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformShuffleUp *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformShuffleUp, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->delta->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformShuffleDown *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformShuffleDown, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->delta->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformIAdd *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformIAdd, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformFAdd *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformFAdd, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformIMul *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformIMul, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformFMul *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformFMul, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformSMin *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformSMin, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformUMin *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformUMin, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformFMin *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformFMin, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformSMax *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformSMax, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformUMax *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformUMax, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformFMax *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformFMax, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBitwiseAnd *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformBitwiseAnd, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBitwiseOr *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformBitwiseOr, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformBitwiseXor *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformBitwiseXor, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformLogicalAnd *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformLogicalAnd, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformLogicalOr *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformLogicalOr, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformLogicalXor *value)
  {
    size_t len = 5;
    len += value->clusterSize ? 1 : 0;
    writer.beginInstruction(Op::OpGroupNonUniformLogicalXor, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->value->resultId);
    if (value->clusterSize)
    {
      writer.writeWord((*value->clusterSize)->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformQuadBroadcast *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformQuadBroadcast, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->index->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformQuadSwap *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformQuadSwap, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->direction->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCopyLogical *value)
  {
    writer.beginInstruction(Op::OpCopyLogical, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpPtrEqual *value)
  {
    writer.beginInstruction(Op::OpPtrEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpPtrNotEqual *value)
  {
    writer.beginInstruction(Op::OpPtrNotEqual, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpPtrDiff *value)
  {
    writer.beginInstruction(Op::OpPtrDiff, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTerminateInvocation *value)
  {
    writer.beginInstruction(Op::OpTerminateInvocation, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupBallotKHR *value)
  {
    writer.beginInstruction(Op::OpSubgroupBallotKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->predicate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupFirstInvocationKHR *value)
  {
    writer.beginInstruction(Op::OpSubgroupFirstInvocationKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAllKHR *value)
  {
    writer.beginInstruction(Op::OpSubgroupAllKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->predicate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAnyKHR *value)
  {
    writer.beginInstruction(Op::OpSubgroupAnyKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->predicate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAllEqualKHR *value)
  {
    writer.beginInstruction(Op::OpSubgroupAllEqualKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->predicate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupReadInvocationKHR *value)
  {
    writer.beginInstruction(Op::OpSubgroupReadInvocationKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->index->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTraceRayKHR *value)
  {
    writer.beginInstruction(Op::OpTraceRayKHR, 11);
    writer.writeWord(value->accel->resultId);
    writer.writeWord(value->rayFlags->resultId);
    writer.writeWord(value->cullMask->resultId);
    writer.writeWord(value->sbtOffset->resultId);
    writer.writeWord(value->sbtStride->resultId);
    writer.writeWord(value->missIndex->resultId);
    writer.writeWord(value->rayOrigin->resultId);
    writer.writeWord(value->rayTmin->resultId);
    writer.writeWord(value->rayDirection->resultId);
    writer.writeWord(value->rayTmax->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpExecuteCallableKHR *value)
  {
    writer.beginInstruction(Op::OpExecuteCallableKHR, 2);
    writer.writeWord(value->sbtIndex->resultId);
    writer.writeWord(value->callableData->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertUToAccelerationStructureKHR *value)
  {
    writer.beginInstruction(Op::OpConvertUToAccelerationStructureKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->accel->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIgnoreIntersectionKHR *value)
  {
    writer.beginInstruction(Op::OpIgnoreIntersectionKHR, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTerminateRayKHR *value)
  {
    writer.beginInstruction(Op::OpTerminateRayKHR, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSDot *value)
  {
    size_t len = 4;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpSDot, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSDotKHR *value)
  {
    size_t len = 4;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpSDotKHR, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUDot *value)
  {
    size_t len = 4;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpUDot, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUDotKHR *value)
  {
    size_t len = 4;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpUDotKHR, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSUDot *value)
  {
    size_t len = 4;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpSUDot, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSUDotKHR *value)
  {
    size_t len = 4;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpSUDotKHR, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSDotAccSat *value)
  {
    size_t len = 5;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpSDotAccSat, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    writer.writeWord(value->accumulator->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSDotAccSatKHR *value)
  {
    size_t len = 5;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpSDotAccSatKHR, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    writer.writeWord(value->accumulator->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUDotAccSat *value)
  {
    size_t len = 5;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpUDotAccSat, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    writer.writeWord(value->accumulator->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUDotAccSatKHR *value)
  {
    size_t len = 5;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpUDotAccSatKHR, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    writer.writeWord(value->accumulator->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSUDotAccSat *value)
  {
    size_t len = 5;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpSUDotAccSat, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    writer.writeWord(value->accumulator->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSUDotAccSatKHR *value)
  {
    size_t len = 5;
    len += value->packedVectorFormat ? 1 : 0;
    writer.beginInstruction(Op::OpSUDotAccSatKHR, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->vector1->resultId);
    writer.writeWord(value->vector2->resultId);
    writer.writeWord(value->accumulator->resultId);
    if (value->packedVectorFormat)
    {
      writer.writeWord(static_cast<unsigned>((*value->packedVectorFormat)));
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeRayQueryKHR *value)
  {
    writer.beginInstruction(Op::OpTypeRayQueryKHR, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryInitializeKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryInitializeKHR, 8);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->accel->resultId);
    writer.writeWord(value->rayFlags->resultId);
    writer.writeWord(value->cullMask->resultId);
    writer.writeWord(value->rayOrigin->resultId);
    writer.writeWord(value->rayTMin->resultId);
    writer.writeWord(value->rayDirection->resultId);
    writer.writeWord(value->rayTMax->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryTerminateKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryTerminateKHR, 1);
    writer.writeWord(value->rayQuery->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGenerateIntersectionKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGenerateIntersectionKHR, 2);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->hitT->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryConfirmIntersectionKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryConfirmIntersectionKHR, 1);
    writer.writeWord(value->rayQuery->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryProceedKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryProceedKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionTypeKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionTypeKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupIAddNonUniformAMD *value)
  {
    writer.beginInstruction(Op::OpGroupIAddNonUniformAMD, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupFAddNonUniformAMD *value)
  {
    writer.beginInstruction(Op::OpGroupFAddNonUniformAMD, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupFMinNonUniformAMD *value)
  {
    writer.beginInstruction(Op::OpGroupFMinNonUniformAMD, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupUMinNonUniformAMD *value)
  {
    writer.beginInstruction(Op::OpGroupUMinNonUniformAMD, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupSMinNonUniformAMD *value)
  {
    writer.beginInstruction(Op::OpGroupSMinNonUniformAMD, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupFMaxNonUniformAMD *value)
  {
    writer.beginInstruction(Op::OpGroupFMaxNonUniformAMD, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupUMaxNonUniformAMD *value)
  {
    writer.beginInstruction(Op::OpGroupUMaxNonUniformAMD, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupSMaxNonUniformAMD *value)
  {
    writer.beginInstruction(Op::OpGroupSMaxNonUniformAMD, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(static_cast<unsigned>(value->operation));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFragmentMaskFetchAMD *value)
  {
    writer.beginInstruction(Op::OpFragmentMaskFetchAMD, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFragmentFetchAMD *value)
  {
    writer.beginInstruction(Op::OpFragmentFetchAMD, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->fragmentIndex->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReadClockKHR *value)
  {
    writer.beginInstruction(Op::OpReadClockKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->scope->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpImageSampleFootprintNV *value)
  {
    size_t len = 6;
    if (value->imageOperands)
    {
      ++len;
      auto compare = *value->imageOperands;
      if (ImageOperandsMask::Bias == (compare & ImageOperandsMask::Bias))
      {
        compare = compare ^ ImageOperandsMask::Bias;
        ++len;
      }
      if (ImageOperandsMask::Lod == (compare & ImageOperandsMask::Lod))
      {
        compare = compare ^ ImageOperandsMask::Lod;
        ++len;
      }
      if (ImageOperandsMask::Grad == (compare & ImageOperandsMask::Grad))
      {
        compare = compare ^ ImageOperandsMask::Grad;
        ++len;
        ++len;
      }
      if (ImageOperandsMask::ConstOffset == (compare & ImageOperandsMask::ConstOffset))
      {
        compare = compare ^ ImageOperandsMask::ConstOffset;
        ++len;
      }
      if (ImageOperandsMask::Offset == (compare & ImageOperandsMask::Offset))
      {
        compare = compare ^ ImageOperandsMask::Offset;
        ++len;
      }
      if (ImageOperandsMask::ConstOffsets == (compare & ImageOperandsMask::ConstOffsets))
      {
        compare = compare ^ ImageOperandsMask::ConstOffsets;
        ++len;
      }
      if (ImageOperandsMask::Sample == (compare & ImageOperandsMask::Sample))
      {
        compare = compare ^ ImageOperandsMask::Sample;
        ++len;
      }
      if (ImageOperandsMask::MinLod == (compare & ImageOperandsMask::MinLod))
      {
        compare = compare ^ ImageOperandsMask::MinLod;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compare & ImageOperandsMask::MakeTexelAvailable))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailable;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compare & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelAvailableKHR;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisible == (compare & ImageOperandsMask::MakeTexelVisible))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisible;
        ++len;
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compare & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compare = compare ^ ImageOperandsMask::MakeTexelVisibleKHR;
        ++len;
      }
      if (ImageOperandsMask::NonPrivateTexel == (compare & ImageOperandsMask::NonPrivateTexel))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compare & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compare & ImageOperandsMask::VolatileTexel))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compare & ImageOperandsMask::VolatileTexelKHR))
      {
        compare = compare ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compare & ImageOperandsMask::SignExtend))
      {
        compare = compare ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compare & ImageOperandsMask::ZeroExtend))
      {
        compare = compare ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compare & ImageOperandsMask::Nontemporal))
      {
        compare = compare ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compare & ImageOperandsMask::Offsets))
      {
        compare = compare ^ ImageOperandsMask::Offsets;
        ++len;
      }
    }
    writer.beginInstruction(Op::OpImageSampleFootprintNV, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sampledImage->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->granularity->resultId);
    writer.writeWord(value->coarse->resultId);
    if (value->imageOperands)
    {
      writer.writeWord(static_cast<unsigned>((*value->imageOperands)));
      auto compareWrite = (*value->imageOperands);
      if (ImageOperandsMask::Bias == (compareWrite & ImageOperandsMask::Bias))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Bias;
        writer.writeWord(value->imageOperandsBias->resultId);
      }
      if (ImageOperandsMask::Lod == (compareWrite & ImageOperandsMask::Lod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Lod;
        writer.writeWord(value->imageOperandsLod->resultId);
      }
      if (ImageOperandsMask::Grad == (compareWrite & ImageOperandsMask::Grad))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Grad;
        writer.writeWord(value->imageOperandsGradX->resultId);
        writer.writeWord(value->imageOperandsGradY->resultId);
      }
      if (ImageOperandsMask::ConstOffset == (compareWrite & ImageOperandsMask::ConstOffset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffset;
        writer.writeWord(value->imageOperandsConstOffset->resultId);
      }
      if (ImageOperandsMask::Offset == (compareWrite & ImageOperandsMask::Offset))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offset;
        writer.writeWord(value->imageOperandsOffset->resultId);
      }
      if (ImageOperandsMask::ConstOffsets == (compareWrite & ImageOperandsMask::ConstOffsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ConstOffsets;
        writer.writeWord(value->imageOperandsConstOffsets->resultId);
      }
      if (ImageOperandsMask::Sample == (compareWrite & ImageOperandsMask::Sample))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Sample;
        writer.writeWord(value->imageOperandsSample->resultId);
      }
      if (ImageOperandsMask::MinLod == (compareWrite & ImageOperandsMask::MinLod))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MinLod;
        writer.writeWord(value->imageOperandsMinLod->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailable == (compareWrite & ImageOperandsMask::MakeTexelAvailable))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailable;
        writer.writeWord(value->imageOperandsMakeTexelAvailable->resultId);
      }
      if (ImageOperandsMask::MakeTexelAvailableKHR == (compareWrite & ImageOperandsMask::MakeTexelAvailableKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelAvailableKHR;
        writer.writeWord(value->imageOperandsMakeTexelAvailableKHR->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisible == (compareWrite & ImageOperandsMask::MakeTexelVisible))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisible;
        writer.writeWord(value->imageOperandsMakeTexelVisible->resultId);
      }
      if (ImageOperandsMask::MakeTexelVisibleKHR == (compareWrite & ImageOperandsMask::MakeTexelVisibleKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::MakeTexelVisibleKHR;
        writer.writeWord(value->imageOperandsMakeTexelVisibleKHR->resultId);
      }
      if (ImageOperandsMask::NonPrivateTexel == (compareWrite & ImageOperandsMask::NonPrivateTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexel;
      }
      if (ImageOperandsMask::NonPrivateTexelKHR == (compareWrite & ImageOperandsMask::NonPrivateTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::NonPrivateTexelKHR;
      }
      if (ImageOperandsMask::VolatileTexel == (compareWrite & ImageOperandsMask::VolatileTexel))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexel;
      }
      if (ImageOperandsMask::VolatileTexelKHR == (compareWrite & ImageOperandsMask::VolatileTexelKHR))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::VolatileTexelKHR;
      }
      if (ImageOperandsMask::SignExtend == (compareWrite & ImageOperandsMask::SignExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::SignExtend;
      }
      if (ImageOperandsMask::ZeroExtend == (compareWrite & ImageOperandsMask::ZeroExtend))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::ZeroExtend;
      }
      if (ImageOperandsMask::Nontemporal == (compareWrite & ImageOperandsMask::Nontemporal))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Nontemporal;
      }
      if (ImageOperandsMask::Offsets == (compareWrite & ImageOperandsMask::Offsets))
      {
        compareWrite = compareWrite ^ ImageOperandsMask::Offsets;
        writer.writeWord(value->imageOperandsOffsets->resultId);
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGroupNonUniformPartitionNV *value)
  {
    writer.beginInstruction(Op::OpGroupNonUniformPartitionNV, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpWritePackedPrimitiveIndices4x8NV *value)
  {
    writer.beginInstruction(Op::OpWritePackedPrimitiveIndices4x8NV, 2);
    writer.writeWord(value->indexOffset->resultId);
    writer.writeWord(value->packedIndices->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReportIntersectionNV *value)
  {
    writer.beginInstruction(Op::OpReportIntersectionNV, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->hit->resultId);
    writer.writeWord(value->hitKind->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReportIntersectionKHR *value)
  {
    writer.beginInstruction(Op::OpReportIntersectionKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->hit->resultId);
    writer.writeWord(value->hitKind->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIgnoreIntersectionNV *value)
  {
    writer.beginInstruction(Op::OpIgnoreIntersectionNV, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTerminateRayNV *value)
  {
    writer.beginInstruction(Op::OpTerminateRayNV, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTraceNV *value)
  {
    writer.beginInstruction(Op::OpTraceNV, 11);
    writer.writeWord(value->accel->resultId);
    writer.writeWord(value->rayFlags->resultId);
    writer.writeWord(value->cullMask->resultId);
    writer.writeWord(value->sbtOffset->resultId);
    writer.writeWord(value->sbtStride->resultId);
    writer.writeWord(value->missIndex->resultId);
    writer.writeWord(value->rayOrigin->resultId);
    writer.writeWord(value->rayTmin->resultId);
    writer.writeWord(value->rayDirection->resultId);
    writer.writeWord(value->rayTmax->resultId);
    writer.writeWord(value->payloadId->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTraceMotionNV *value)
  {
    writer.beginInstruction(Op::OpTraceMotionNV, 12);
    writer.writeWord(value->accel->resultId);
    writer.writeWord(value->rayFlags->resultId);
    writer.writeWord(value->cullMask->resultId);
    writer.writeWord(value->sbtOffset->resultId);
    writer.writeWord(value->sbtStride->resultId);
    writer.writeWord(value->missIndex->resultId);
    writer.writeWord(value->rayOrigin->resultId);
    writer.writeWord(value->rayTmin->resultId);
    writer.writeWord(value->rayDirection->resultId);
    writer.writeWord(value->rayTmax->resultId);
    writer.writeWord(value->time->resultId);
    writer.writeWord(value->payloadId->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTraceRayMotionNV *value)
  {
    writer.beginInstruction(Op::OpTraceRayMotionNV, 12);
    writer.writeWord(value->accel->resultId);
    writer.writeWord(value->rayFlags->resultId);
    writer.writeWord(value->cullMask->resultId);
    writer.writeWord(value->sbtOffset->resultId);
    writer.writeWord(value->sbtStride->resultId);
    writer.writeWord(value->missIndex->resultId);
    writer.writeWord(value->rayOrigin->resultId);
    writer.writeWord(value->rayTmin->resultId);
    writer.writeWord(value->rayDirection->resultId);
    writer.writeWord(value->rayTmax->resultId);
    writer.writeWord(value->time->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAccelerationStructureNV *value)
  {
    writer.beginInstruction(Op::OpTypeAccelerationStructureNV, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAccelerationStructureKHR *value)
  {
    writer.beginInstruction(Op::OpTypeAccelerationStructureKHR, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpExecuteCallableNV *value)
  {
    writer.beginInstruction(Op::OpExecuteCallableNV, 2);
    writer.writeWord(value->sbtIndex->resultId);
    writer.writeWord(value->callableDataid->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeCooperativeMatrixNV *value)
  {
    writer.beginInstruction(Op::OpTypeCooperativeMatrixNV, 5);
    writer.writeWord(value->resultId);
    writer.writeWord(value->componentType->resultId);
    writer.writeWord(value->execution->resultId);
    writer.writeWord(value->rows->resultId);
    writer.writeWord(value->columns->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCooperativeMatrixLoadNV *value)
  {
    size_t len = 5;
    if (value->memoryAccess)
    {
      ++len;
      auto compare = *value->memoryAccess;
      if (MemoryAccessMask::Volatile == (compare & MemoryAccessMask::Volatile))
      {
        compare = compare ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compare & MemoryAccessMask::Aligned))
      {
        compare = compare ^ MemoryAccessMask::Aligned;
        ++len;
      }
      if (MemoryAccessMask::Nontemporal == (compare & MemoryAccessMask::Nontemporal))
      {
        compare = compare ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compare & MemoryAccessMask::MakePointerAvailable))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailable;
        ++len;
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compare & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailableKHR;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisible == (compare & MemoryAccessMask::MakePointerVisible))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisible;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compare & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisibleKHR;
        ++len;
      }
      if (MemoryAccessMask::NonPrivatePointer == (compare & MemoryAccessMask::NonPrivatePointer))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compare & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.beginInstruction(Op::OpCooperativeMatrixLoadNV, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->stride->resultId);
    writer.writeWord(value->columnMajor->resultId);
    if (value->memoryAccess)
    {
      writer.writeWord(static_cast<unsigned>((*value->memoryAccess)));
      auto compareWrite = (*value->memoryAccess);
      if (MemoryAccessMask::Volatile == (compareWrite & MemoryAccessMask::Volatile))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compareWrite & MemoryAccessMask::Aligned))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Aligned;
        writer.writeWord(static_cast<unsigned>(value->memoryAccessAligned.value));
      }
      if (MemoryAccessMask::Nontemporal == (compareWrite & MemoryAccessMask::Nontemporal))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compareWrite & MemoryAccessMask::MakePointerAvailable))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailable;
        writer.writeWord(value->memoryAccessMakePointerAvailable->resultId);
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compareWrite & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailableKHR;
        writer.writeWord(value->memoryAccessMakePointerAvailableKHR->resultId);
      }
      if (MemoryAccessMask::MakePointerVisible == (compareWrite & MemoryAccessMask::MakePointerVisible))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisible;
        writer.writeWord(value->memoryAccessMakePointerVisible->resultId);
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compareWrite & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisibleKHR;
        writer.writeWord(value->memoryAccessMakePointerVisibleKHR->resultId);
      }
      if (MemoryAccessMask::NonPrivatePointer == (compareWrite & MemoryAccessMask::NonPrivatePointer))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compareWrite & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCooperativeMatrixStoreNV *value)
  {
    size_t len = 4;
    if (value->memoryAccess)
    {
      ++len;
      auto compare = *value->memoryAccess;
      if (MemoryAccessMask::Volatile == (compare & MemoryAccessMask::Volatile))
      {
        compare = compare ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compare & MemoryAccessMask::Aligned))
      {
        compare = compare ^ MemoryAccessMask::Aligned;
        ++len;
      }
      if (MemoryAccessMask::Nontemporal == (compare & MemoryAccessMask::Nontemporal))
      {
        compare = compare ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compare & MemoryAccessMask::MakePointerAvailable))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailable;
        ++len;
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compare & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerAvailableKHR;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisible == (compare & MemoryAccessMask::MakePointerVisible))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisible;
        ++len;
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compare & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compare = compare ^ MemoryAccessMask::MakePointerVisibleKHR;
        ++len;
      }
      if (MemoryAccessMask::NonPrivatePointer == (compare & MemoryAccessMask::NonPrivatePointer))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compare & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compare = compare ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.beginInstruction(Op::OpCooperativeMatrixStoreNV, len);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->object->resultId);
    writer.writeWord(value->stride->resultId);
    writer.writeWord(value->columnMajor->resultId);
    if (value->memoryAccess)
    {
      writer.writeWord(static_cast<unsigned>((*value->memoryAccess)));
      auto compareWrite = (*value->memoryAccess);
      if (MemoryAccessMask::Volatile == (compareWrite & MemoryAccessMask::Volatile))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Volatile;
      }
      if (MemoryAccessMask::Aligned == (compareWrite & MemoryAccessMask::Aligned))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Aligned;
        writer.writeWord(static_cast<unsigned>(value->memoryAccessAligned.value));
      }
      if (MemoryAccessMask::Nontemporal == (compareWrite & MemoryAccessMask::Nontemporal))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::Nontemporal;
      }
      if (MemoryAccessMask::MakePointerAvailable == (compareWrite & MemoryAccessMask::MakePointerAvailable))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailable;
        writer.writeWord(value->memoryAccessMakePointerAvailable->resultId);
      }
      if (MemoryAccessMask::MakePointerAvailableKHR == (compareWrite & MemoryAccessMask::MakePointerAvailableKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerAvailableKHR;
        writer.writeWord(value->memoryAccessMakePointerAvailableKHR->resultId);
      }
      if (MemoryAccessMask::MakePointerVisible == (compareWrite & MemoryAccessMask::MakePointerVisible))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisible;
        writer.writeWord(value->memoryAccessMakePointerVisible->resultId);
      }
      if (MemoryAccessMask::MakePointerVisibleKHR == (compareWrite & MemoryAccessMask::MakePointerVisibleKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::MakePointerVisibleKHR;
        writer.writeWord(value->memoryAccessMakePointerVisibleKHR->resultId);
      }
      if (MemoryAccessMask::NonPrivatePointer == (compareWrite & MemoryAccessMask::NonPrivatePointer))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointer;
      }
      if (MemoryAccessMask::NonPrivatePointerKHR == (compareWrite & MemoryAccessMask::NonPrivatePointerKHR))
      {
        compareWrite = compareWrite ^ MemoryAccessMask::NonPrivatePointerKHR;
      }
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCooperativeMatrixMulAddNV *value)
  {
    writer.beginInstruction(Op::OpCooperativeMatrixMulAddNV, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->c->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCooperativeMatrixLengthNV *value)
  {
    writer.beginInstruction(Op::OpCooperativeMatrixLengthNV, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->type->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpBeginInvocationInterlockEXT *value)
  {
    writer.beginInstruction(Op::OpBeginInvocationInterlockEXT, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpEndInvocationInterlockEXT *value)
  {
    writer.beginInstruction(Op::OpEndInvocationInterlockEXT, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDemoteToHelperInvocation *value)
  {
    writer.beginInstruction(Op::OpDemoteToHelperInvocation, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpDemoteToHelperInvocationEXT *value)
  {
    writer.beginInstruction(Op::OpDemoteToHelperInvocationEXT, 0);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIsHelperInvocationEXT *value)
  {
    writer.beginInstruction(Op::OpIsHelperInvocationEXT, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertUToImageNV *value)
  {
    writer.beginInstruction(Op::OpConvertUToImageNV, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertUToSamplerNV *value)
  {
    writer.beginInstruction(Op::OpConvertUToSamplerNV, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertImageToUNV *value)
  {
    writer.beginInstruction(Op::OpConvertImageToUNV, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertSamplerToUNV *value)
  {
    writer.beginInstruction(Op::OpConvertSamplerToUNV, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertUToSampledImageNV *value)
  {
    writer.beginInstruction(Op::OpConvertUToSampledImageNV, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConvertSampledImageToUNV *value)
  {
    writer.beginInstruction(Op::OpConvertSampledImageToUNV, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupShuffleINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupShuffleINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->data->resultId);
    writer.writeWord(value->invocationId->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupShuffleDownINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupShuffleDownINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->current->resultId);
    writer.writeWord(value->next->resultId);
    writer.writeWord(value->delta->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupShuffleUpINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupShuffleUpINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->previous->resultId);
    writer.writeWord(value->current->resultId);
    writer.writeWord(value->delta->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupShuffleXorINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupShuffleXorINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->data->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupBlockReadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupBlockReadINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->ptr->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupBlockWriteINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupBlockWriteINTEL, 2);
    writer.writeWord(value->ptr->resultId);
    writer.writeWord(value->data->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupImageBlockReadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupImageBlockReadINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupImageBlockWriteINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupImageBlockWriteINTEL, 3);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->data->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupImageMediaBlockReadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupImageMediaBlockReadINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->width->resultId);
    writer.writeWord(value->height->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupImageMediaBlockWriteINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupImageMediaBlockWriteINTEL, 5);
    writer.writeWord(value->image->resultId);
    writer.writeWord(value->coordinate->resultId);
    writer.writeWord(value->width->resultId);
    writer.writeWord(value->height->resultId);
    writer.writeWord(value->data->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUCountLeadingZerosINTEL *value)
  {
    writer.beginInstruction(Op::OpUCountLeadingZerosINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUCountTrailingZerosINTEL *value)
  {
    writer.beginInstruction(Op::OpUCountTrailingZerosINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAbsISubINTEL *value)
  {
    writer.beginInstruction(Op::OpAbsISubINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAbsUSubINTEL *value)
  {
    writer.beginInstruction(Op::OpAbsUSubINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIAddSatINTEL *value)
  {
    writer.beginInstruction(Op::OpIAddSatINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUAddSatINTEL *value)
  {
    writer.beginInstruction(Op::OpUAddSatINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIAverageINTEL *value)
  {
    writer.beginInstruction(Op::OpIAverageINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUAverageINTEL *value)
  {
    writer.beginInstruction(Op::OpUAverageINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIAverageRoundedINTEL *value)
  {
    writer.beginInstruction(Op::OpIAverageRoundedINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUAverageRoundedINTEL *value)
  {
    writer.beginInstruction(Op::OpUAverageRoundedINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpISubSatINTEL *value)
  {
    writer.beginInstruction(Op::OpISubSatINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUSubSatINTEL *value)
  {
    writer.beginInstruction(Op::OpUSubSatINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpIMul32x16INTEL *value)
  {
    writer.beginInstruction(Op::OpIMul32x16INTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpUMul32x16INTEL *value)
  {
    writer.beginInstruction(Op::OpUMul32x16INTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->operand1->resultId);
    writer.writeWord(value->operand2->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConstantFunctionPointerINTEL *value)
  {
    writer.beginInstruction(Op::OpConstantFunctionPointerINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->function->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFunctionPointerCallINTEL *value)
  {
    size_t len = 2;
    len += value->operand1.size();
    writer.beginInstruction(Op::OpFunctionPointerCallINTEL, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    for (auto &&v : value->operand1)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAsmTargetINTEL *value)
  {
    size_t len = 2;
    len += (value->asmTarget.size() + sizeof(unsigned)) / sizeof(unsigned);
    writer.beginInstruction(Op::OpAsmTargetINTEL, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeString(value->asmTarget.c_str());
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAsmINTEL *value)
  {
    size_t len = 4;
    len += (value->asmInstructions.size() + sizeof(unsigned)) / sizeof(unsigned);
    len += (value->constraints.size() + sizeof(unsigned)) / sizeof(unsigned);
    writer.beginInstruction(Op::OpAsmINTEL, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->asmType->resultId);
    writer.writeWord(value->target->resultId);
    writer.writeString(value->asmInstructions.c_str());
    writer.writeString(value->constraints.c_str());
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAsmCallINTEL *value)
  {
    size_t len = 3;
    len += value->argument0.size();
    writer.beginInstruction(Op::OpAsmCallINTEL, len);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->not_reserved_asm->resultId);
    for (auto &&v : value->argument0)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicFMinEXT *value)
  {
    writer.beginInstruction(Op::OpAtomicFMinEXT, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicFMaxEXT *value)
  {
    writer.beginInstruction(Op::OpAtomicFMaxEXT, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAssumeTrueKHR *value)
  {
    writer.beginInstruction(Op::OpAssumeTrueKHR, 1);
    writer.writeWord(value->condition->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpExpectKHR *value)
  {
    writer.beginInstruction(Op::OpExpectKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->value->resultId);
    writer.writeWord(value->expectedValue->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpVmeImageINTEL *value)
  {
    writer.beginInstruction(Op::OpVmeImageINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->imageType->resultId);
    writer.writeWord(value->sampler->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeVmeImageINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeVmeImageINTEL, 2);
    writer.writeWord(value->resultId);
    writer.writeWord(value->imageType->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcImePayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcImePayloadINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcRefPayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcRefPayloadINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcSicPayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcSicPayloadINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcMcePayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcMcePayloadINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcMceResultINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcMceResultINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcImeResultINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcImeResultINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcImeResultSingleReferenceStreamoutINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcImeResultSingleReferenceStreamoutINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcImeResultDualReferenceStreamoutINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcImeResultDualReferenceStreamoutINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcImeSingleReferenceStreaminINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcImeSingleReferenceStreaminINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcImeDualReferenceStreaminINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcImeDualReferenceStreaminINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcRefResultINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcRefResultINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeAvcSicResultINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeAvcSicResultINTEL, 1);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sliceType->resultId);
    writer.writeWord(value->qp->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->referenceBasePenalty->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sliceType->resultId);
    writer.writeWord(value->qp->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceSetInterShapePenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceSetInterShapePenaltyINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->packedShapePenalty->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sliceType->resultId);
    writer.writeWord(value->qp->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceSetInterDirectionPenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceSetInterDirectionPenaltyINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->directionCost->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sliceType->resultId);
    writer.writeWord(value->qp->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sliceType->resultId);
    writer.writeWord(value->qp->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceSetMotionVectorCostFunctionINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceSetMotionVectorCostFunctionINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->packedCostCenterDelta->resultId);
    writer.writeWord(value->packedCostTable->resultId);
    writer.writeWord(value->costPrecision->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sliceType->resultId);
    writer.writeWord(value->qp->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceSetAcOnlyHaarINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceSetAcOnlyHaarINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->sourceFieldPolarity->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->referenceFieldPolarity->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->forwardReferenceFieldPolarity->resultId);
    writer.writeWord(value->backwardReferenceFieldPolarity->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceConvertToImePayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceConvertToImePayloadINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceConvertToImeResultINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceConvertToImeResultINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceConvertToRefPayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceConvertToRefPayloadINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceConvertToRefResultINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceConvertToRefResultINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceConvertToSicPayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceConvertToSicPayloadINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceConvertToSicResultINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceConvertToSicResultINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetMotionVectorsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetMotionVectorsINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetInterDistortionsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetInterDistortionsINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetBestInterDistortionsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetBestInterDistortionsINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetInterMajorShapeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetInterMajorShapeINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetInterMinorShapeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetInterMinorShapeINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetInterDirectionsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetInterDirectionsINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetInterMotionVectorCountINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetInterMotionVectorCountINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetInterReferenceIdsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetInterReferenceIdsINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->packedReferenceIds->resultId);
    writer.writeWord(value->packedReferenceParameterFieldPolarities->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeInitializeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeInitializeINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcCoord->resultId);
    writer.writeWord(value->partitionMask->resultId);
    writer.writeWord(value->sadAdjustment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeSetSingleReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeSetSingleReferenceINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->refOffset->resultId);
    writer.writeWord(value->searchWindowConfig->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeSetDualReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeSetDualReferenceINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->fwdRefOffset->resultId);
    writer.writeWord(value->bwdRefOffset->resultId);
    writer.writeWord(value->idSearchWindowConfig->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeRefWindowSizeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeRefWindowSizeINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->searchWindowConfig->resultId);
    writer.writeWord(value->dualRef->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeAdjustRefOffsetINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeAdjustRefOffsetINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->refOffset->resultId);
    writer.writeWord(value->srcCoord->resultId);
    writer.writeWord(value->refWindowSize->resultId);
    writer.writeWord(value->imageSize->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeConvertToMcePayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeConvertToMcePayloadINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeSetMaxMotionVectorCountINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeSetMaxMotionVectorCountINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->maxMotionVectorCount->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeSetUnidirectionalMixDisableINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeSetUnidirectionalMixDisableINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->threshold->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeSetWeightedSadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeSetWeightedSadINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->packedSadWeights->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeEvaluateWithSingleReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeEvaluateWithSingleReferenceINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->refImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeEvaluateWithDualReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeEvaluateWithDualReferenceINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->fwdRefImage->resultId);
    writer.writeWord(value->bwdRefImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->refImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->streaminComponents->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->fwdRefImage->resultId);
    writer.writeWord(value->bwdRefImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->streaminComponents->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->refImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->fwdRefImage->resultId);
    writer.writeWord(value->bwdRefImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->refImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->streaminComponents->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->fwdRefImage->resultId);
    writer.writeWord(value->bwdRefImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->streaminComponents->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeConvertToMceResultINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeConvertToMceResultINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetSingleReferenceStreaminINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetSingleReferenceStreaminINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetDualReferenceStreaminINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetDualReferenceStreaminINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeStripSingleReferenceStreamoutINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeStripSingleReferenceStreamoutINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeStripDualReferenceStreamoutINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeStripDualReferenceStreamoutINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->majorShape->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->majorShape->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->majorShape->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->majorShape->resultId);
    writer.writeWord(value->direction->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->majorShape->resultId);
    writer.writeWord(value->direction->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.writeWord(value->majorShape->resultId);
    writer.writeWord(value->direction->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetBorderReachedINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetBorderReachedINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->imageSelect->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetTruncatedSearchIndicationINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetTruncatedSearchIndicationINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcFmeInitializeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcFmeInitializeINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcCoord->resultId);
    writer.writeWord(value->motionVectors->resultId);
    writer.writeWord(value->majorShapes->resultId);
    writer.writeWord(value->minorShapes->resultId);
    writer.writeWord(value->direction->resultId);
    writer.writeWord(value->pixelResolution->resultId);
    writer.writeWord(value->sadAdjustment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcBmeInitializeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcBmeInitializeINTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcCoord->resultId);
    writer.writeWord(value->motionVectors->resultId);
    writer.writeWord(value->majorShapes->resultId);
    writer.writeWord(value->minorShapes->resultId);
    writer.writeWord(value->direction->resultId);
    writer.writeWord(value->pixelResolution->resultId);
    writer.writeWord(value->bidirectionalWeight->resultId);
    writer.writeWord(value->sadAdjustment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcRefConvertToMcePayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcRefConvertToMcePayloadINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcRefSetBidirectionalMixDisableINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcRefSetBidirectionalMixDisableINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcRefSetBilinearFilterEnableINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcRefSetBilinearFilterEnableINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcRefEvaluateWithSingleReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcRefEvaluateWithSingleReferenceINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->refImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcRefEvaluateWithDualReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcRefEvaluateWithDualReferenceINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->fwdRefImage->resultId);
    writer.writeWord(value->bwdRefImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcRefEvaluateWithMultiReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcRefEvaluateWithMultiReferenceINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->packedReferenceIds->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->packedReferenceIds->resultId);
    writer.writeWord(value->packedReferenceFieldPolarities->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcRefConvertToMceResultINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcRefConvertToMceResultINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicInitializeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicInitializeINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcCoord->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicConfigureSkcINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicConfigureSkcINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->skipBlockPartitionType->resultId);
    writer.writeWord(value->skipMotionVectorMask->resultId);
    writer.writeWord(value->motionVectors->resultId);
    writer.writeWord(value->bidirectionalWeight->resultId);
    writer.writeWord(value->sadAdjustment->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicConfigureIpeLumaINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicConfigureIpeLumaINTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->lumaIntraPartitionMask->resultId);
    writer.writeWord(value->intraNeighbourAvailabilty->resultId);
    writer.writeWord(value->leftEdgeLumaPixels->resultId);
    writer.writeWord(value->upperLeftCornerLumaPixel->resultId);
    writer.writeWord(value->upperEdgeLumaPixels->resultId);
    writer.writeWord(value->upperRightEdgeLumaPixels->resultId);
    writer.writeWord(value->sadAdjustment->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicConfigureIpeLumaChromaINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicConfigureIpeLumaChromaINTEL, 13);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->lumaIntraPartitionMask->resultId);
    writer.writeWord(value->intraNeighbourAvailabilty->resultId);
    writer.writeWord(value->leftEdgeLumaPixels->resultId);
    writer.writeWord(value->upperLeftCornerLumaPixel->resultId);
    writer.writeWord(value->upperEdgeLumaPixels->resultId);
    writer.writeWord(value->upperRightEdgeLumaPixels->resultId);
    writer.writeWord(value->leftEdgeChromaPixels->resultId);
    writer.writeWord(value->upperLeftCornerChromaPixel->resultId);
    writer.writeWord(value->upperEdgeChromaPixels->resultId);
    writer.writeWord(value->sadAdjustment->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicGetMotionVectorMaskINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicGetMotionVectorMaskINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->skipBlockPartitionType->resultId);
    writer.writeWord(value->direction->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicConvertToMcePayloadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicConvertToMcePayloadINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->packedShapePenalty->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->lumaModePenalty->resultId);
    writer.writeWord(value->lumaPackedNeighborModes->resultId);
    writer.writeWord(value->lumaPackedNonDcPenalty->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->chromaModeBasePenalty->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicSetBilinearFilterEnableINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicSetBilinearFilterEnableINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicSetSkcForwardTransformEnableINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicSetSkcForwardTransformEnableINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->packedSadCoefficients->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->blockBasedSkipType->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicEvaluateIpeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicEvaluateIpeINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicEvaluateWithSingleReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicEvaluateWithSingleReferenceINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->refImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicEvaluateWithDualReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicEvaluateWithDualReferenceINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->fwdRefImage->resultId);
    writer.writeWord(value->bwdRefImage->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicEvaluateWithMultiReferenceINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicEvaluateWithMultiReferenceINTEL, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->packedReferenceIds->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->srcImage->resultId);
    writer.writeWord(value->packedReferenceIds->resultId);
    writer.writeWord(value->packedReferenceFieldPolarities->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicConvertToMceResultINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicConvertToMceResultINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicGetIpeLumaShapeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicGetIpeLumaShapeINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicGetBestIpeLumaDistortionINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicGetBestIpeLumaDistortionINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicGetBestIpeChromaDistortionINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicGetBestIpeChromaDistortionINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicGetPackedIpeLumaModesINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicGetPackedIpeLumaModesINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicGetIpeChromaModeINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicGetIpeChromaModeINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSubgroupAvcSicGetInterRawSadsINTEL *value)
  {
    writer.beginInstruction(Op::OpSubgroupAvcSicGetInterRawSadsINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->payload->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpVariableLengthArrayINTEL *value)
  {
    writer.beginInstruction(Op::OpVariableLengthArrayINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->length->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSaveMemoryINTEL *value)
  {
    writer.beginInstruction(Op::OpSaveMemoryINTEL, 2);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRestoreMemoryINTEL *value)
  {
    writer.beginInstruction(Op::OpRestoreMemoryINTEL, 1);
    writer.writeWord(value->ptr->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatSinCosPiINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatSinCosPiINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->fromSign.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatCastINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatCastINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatCastFromIntINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatCastFromIntINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->fromSign.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatCastToIntINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatCastToIntINTEL, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatAddINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatAddINTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatSubINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatSubINTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatMulINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatMulINTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatDivINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatDivINTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatGTINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatGTINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatGEINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatGEINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatLTINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatLTINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatLEINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatLEINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatEQINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatEQINTEL, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatRecipINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatRecipINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatRSqrtINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatRSqrtINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatCbrtINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatCbrtINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatHypotINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatHypotINTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatSqrtINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatSqrtINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatLogINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatLogINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatLog2INTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatLog2INTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatLog10INTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatLog10INTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatLog1pINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatLog1pINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatExpINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatExpINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatExp2INTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatExp2INTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatExp10INTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatExp10INTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatExpm1INTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatExpm1INTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatSinINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatSinINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatCosINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatCosINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatSinCosINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatSinCosINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatSinPiINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatSinPiINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatCosPiINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatCosPiINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatASinINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatASinINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatASinPiINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatASinPiINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatACosINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatACosINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatACosPiINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatACosPiINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatATanINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatATanINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatATanPiINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatATanPiINTEL, 8);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatATan2INTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatATan2INTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatPowINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatPowINTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatPowRINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatPowRINTEL, 10);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->m2.value);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpArbitraryFloatPowNINTEL *value)
  {
    writer.beginInstruction(Op::OpArbitraryFloatPowNINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->m1.value);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->mout.value);
    writer.writeWord(value->enableSubnormals.value);
    writer.writeWord(value->roundingMode.value);
    writer.writeWord(value->roundingAccuracy.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedSqrtINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedSqrtINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedRecipINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedRecipINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedRsqrtINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedRsqrtINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedSinINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedSinINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedCosINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedCosINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedSinCosINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedSinCosINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedSinPiINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedSinPiINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedCosPiINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedCosPiINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedSinCosPiINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedSinCosPiINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedLogINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedLogINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFixedExpINTEL *value)
  {
    writer.beginInstruction(Op::OpFixedExpINTEL, 9);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->inputType->resultId);
    writer.writeWord(value->input->resultId);
    writer.writeWord(value->s.value);
    writer.writeWord(value->i.value);
    writer.writeWord(value->rI.value);
    writer.writeWord(value->q.value);
    writer.writeWord(value->o.value);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpPtrCastToCrossWorkgroupINTEL *value)
  {
    writer.beginInstruction(Op::OpPtrCastToCrossWorkgroupINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpCrossWorkgroupCastToPtrINTEL *value)
  {
    writer.beginInstruction(Op::OpCrossWorkgroupCastToPtrINTEL, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpReadPipeBlockingINTEL *value)
  {
    writer.beginInstruction(Op::OpReadPipeBlockingINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpWritePipeBlockingINTEL *value)
  {
    writer.beginInstruction(Op::OpWritePipeBlockingINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->packetSize->resultId);
    writer.writeWord(value->packetAlignment->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpFPGARegINTEL *value)
  {
    writer.beginInstruction(Op::OpFPGARegINTEL, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->result->resultId);
    writer.writeWord(value->input->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetRayTMinKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetRayTMinKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetRayFlagsKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetRayFlagsKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionTKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionTKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionInstanceCustomIndexKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionInstanceCustomIndexKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionInstanceIdKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionInstanceIdKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionGeometryIndexKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionGeometryIndexKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionPrimitiveIndexKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionPrimitiveIndexKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionBarycentricsKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionBarycentricsKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionFrontFaceKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionFrontFaceKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionCandidateAABBOpaqueKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionCandidateAABBOpaqueKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionObjectRayDirectionKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionObjectRayDirectionKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionObjectRayOriginKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionObjectRayOriginKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetWorldRayDirectionKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetWorldRayDirectionKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetWorldRayOriginKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetWorldRayOriginKHR, 3);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionObjectToWorldKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionObjectToWorldKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpRayQueryGetIntersectionWorldToObjectKHR *value)
  {
    writer.beginInstruction(Op::OpRayQueryGetIntersectionWorldToObjectKHR, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->rayQuery->resultId);
    writer.writeWord(value->intersection->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAtomicFAddEXT *value)
  {
    writer.beginInstruction(Op::OpAtomicFAddEXT, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(value->pointer->resultId);
    writer.writeWord(value->memory->resultId);
    writer.writeWord(value->semantics->resultId);
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeBufferSurfaceINTEL *value)
  {
    writer.beginInstruction(Op::OpTypeBufferSurfaceINTEL, 2);
    writer.writeWord(value->resultId);
    writer.writeWord(static_cast<unsigned>(value->accessQualifier));
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpTypeStructContinuedINTEL *value)
  {
    size_t len = 0;
    len += value->param0.size();
    writer.beginInstruction(Op::OpTypeStructContinuedINTEL, len);
    for (auto &&v : value->param0)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpConstantCompositeContinuedINTEL *value)
  {
    size_t len = 0;
    len += value->constituents.size();
    writer.beginInstruction(Op::OpConstantCompositeContinuedINTEL, len);
    for (auto &&v : value->constituents)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpSpecConstantCompositeContinuedINTEL *value)
  {
    size_t len = 0;
    len += value->constituents.size();
    writer.beginInstruction(Op::OpSpecConstantCompositeContinuedINTEL, len);
    for (auto &&v : value->constituents)
    {
      writer.writeWord(v->resultId);
    }
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Round *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450RoundEven *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Trunc *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FAbs *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450SAbs *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FSign *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450SSign *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Floor *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Ceil *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Fract *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Radians *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->degrees->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Degrees *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->radians->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Sin *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Cos *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Tan *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Asin *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Acos *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Atan *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->y_over_x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Sinh *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Cosh *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Tanh *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Asinh *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Acosh *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Atanh *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Atan2 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Pow *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Exp *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Log *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Exp2 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Log2 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Sqrt *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450InverseSqrt *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Determinant *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450MatrixInverse *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Modf *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->i->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450ModfStruct *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FMin *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450UMin *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450SMin *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FMax *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450UMax *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450SMax *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FClamp *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->minVal->resultId);
    writer.writeWord(value->maxVal->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450UClamp *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->minVal->resultId);
    writer.writeWord(value->maxVal->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450SClamp *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->minVal->resultId);
    writer.writeWord(value->maxVal->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FMix *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->a->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450IMix *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->a->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Step *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->edge->resultId);
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450SmoothStep *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->edge0->resultId);
    writer.writeWord(value->edge1->resultId);
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Fma *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->a->resultId);
    writer.writeWord(value->b->resultId);
    writer.writeWord(value->c->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Frexp *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->exp->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FrexpStruct *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Ldexp *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->exp->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450PackSnorm4x8 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->v->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450PackUnorm4x8 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->v->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450PackSnorm2x16 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->v->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450PackUnorm2x16 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->v->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450PackHalf2x16 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->v->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450PackDouble2x32 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->v->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450UnpackSnorm2x16 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450UnpackUnorm2x16 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450UnpackHalf2x16 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->v->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450UnpackSnorm4x8 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450UnpackUnorm4x8 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450UnpackDouble2x32 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->v->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Length *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Distance *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->p0->resultId);
    writer.writeWord(value->p1->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Cross *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Normalize *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FaceForward *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->n->resultId);
    writer.writeWord(value->i->resultId);
    writer.writeWord(value->nref->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Reflect *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->i->resultId);
    writer.writeWord(value->n->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450Refract *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->i->resultId);
    writer.writeWord(value->n->resultId);
    writer.writeWord(value->eta->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FindILsb *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FindSMsb *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450FindUMsb *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->value->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450InterpolateAtCentroid *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->interpolant->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450InterpolateAtSample *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->interpolant->resultId);
    writer.writeWord(value->sample->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450InterpolateAtOffset *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->interpolant->resultId);
    writer.writeWord(value->offset->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450NMin *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450NMax *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpGLSLstd450NClamp *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->minVal->resultId);
    writer.writeWord(value->maxVal->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDGcnShaderCubeFaceIndex *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDGcnShaderCubeFaceCoord *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->p->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDGcnShaderTime *value)
  {
    writer.beginInstruction(Op::OpExtInst, 4);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderBallotSwizzleInvocations *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->data->resultId);
    writer.writeWord(value->offset->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderBallotSwizzleInvocationsMasked *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->data->resultId);
    writer.writeWord(value->mask->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderBallotWriteInvocation *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->inputValue->resultId);
    writer.writeWord(value->writeValue->resultId);
    writer.writeWord(value->invocationIndex->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderBallotMbcnt *value)
  {
    writer.beginInstruction(Op::OpExtInst, 5);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->mask->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderTrinaryMinmaxFMin3 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->z->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderTrinaryMinmaxUMin3 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->z->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderTrinaryMinmaxSMin3 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->z->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderTrinaryMinmaxFMax3 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->z->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderTrinaryMinmaxUMax3 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->z->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderTrinaryMinmaxSMax3 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->z->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderTrinaryMinmaxFMid3 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->z->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderTrinaryMinmaxUMid3 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->z->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderTrinaryMinmaxSMid3 *value)
  {
    writer.beginInstruction(Op::OpExtInst, 7);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->x->resultId);
    writer.writeWord(value->y->resultId);
    writer.writeWord(value->z->resultId);
    writer.endWrite();
    return true;
  }
  bool operator()(NodeOpAMDShaderExplicitVertexParameterInterpolateAtVertex *value)
  {
    writer.beginInstruction(Op::OpExtInst, 6);
    writer.writeWord(value->resultType->resultId);
    writer.writeWord(value->resultId);
    writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));
    writer.writeWord(static_cast<unsigned>(value->extOpCode));
    writer.writeWord(value->interpolant->resultId);
    writer.writeWord(value->vertexIdx->resultId);
    writer.endWrite();
    return true;
  }
  // generic nodes do nothing, should never be called anyways
  bool operator()(Node *value) { return false; }
  bool operator()(NodeId *value) { return false; }
  bool operator()(NodeTypedef *value) { return false; }
  bool operator()(NodeOperation *value) { return false; }
  bool operator()(NodeUnaryOperation *value) { return false; }
  bool operator()(NodeBinaryOperation *value) { return false; }
  bool operator()(NodeTrinaryOperation *value) { return false; }
  bool operator()(NodeMultinaryOperation *value) { return false; }
  bool operator()(NodeAction *value) { return false; }
  bool operator()(NodeUnaryAction *value) { return false; }
  bool operator()(NodeBinaryAction *value) { return false; }
  bool operator()(NodeTrinaryAction *value) { return false; }
  bool operator()(NodeMultinaryAction *value) { return false; }
  bool operator()(NodeImageOperation *value) { return false; }
  bool operator()(NodeImageAction *value) { return false; }
  bool operator()(NodeConstant *value) { return false; }
  bool operator()(NodeConstantComposite *value) { return false; }
  bool operator()(NodeConstantSampler *value) { return false; }
  bool operator()(NodeSpecConstant *value) { return false; }
  bool operator()(NodeSpecConstantComposite *value) { return false; }
  bool operator()(NodeSpecConstantOperation *value) { return false; }
  bool operator()(NodeVariable *value) { return false; }
  bool operator()(NodeFunction *value) { return false; }
  bool operator()(NodePhi *value) { return false; }
  bool operator()(NodeBlock *value) { return false; }
  bool operator()(NodeBlockEnd *value) { return false; }
  bool operator()(NodeBranch *value) { return false; }
  bool operator()(NodeExit *value) { return false; }
  bool operator()(NodeReturn *value) { return false; }
  bool operator()(NodeSwitch *value) { return false; }
  bool operator()(NodeFunctionCall *value) { return false; }
  bool operator()(NodeScopedOperation *value) { return false; }
  bool operator()(NodeScopedAction *value) { return false; }
  bool operator()(NodeGroupedAction *value) { return false; }
  bool operator()(NodeGroupedOperation *value) { return false; }
};
struct CanWriteTypeCheckVisitor
{
  FlatSet<Id> &writtenTypes;
  FlatSet<Id> &forwardTypes;
  FlatSet<Id> &writtenConstants;
  bool &canWrite;
  template <typename T>
  bool operator()(T *value)
  {
    canWrite = true;
    value->visitRefs([&, this](auto node) //
      {
        if (writtenTypes.has(node->resultId))
          return;
        if (forwardTypes.has(node->resultId))
          return;
        if (writtenConstants.has(node->resultId))
          return;
        canWrite = false;
      });
    return true;
  }
};
void encodeHeader(ModuleBuilder &module, WordWriter &writer)
{
  writer.beginHeader();
  writer.writeWord(MagicNumber);
  writer.writeWord(module.getModuleVersion());
  writer.writeWord(module.getToolIdentifcation());
  writer.writeWord(module.getIdBounds());
  // one reserved slot that has to be 0
  writer.writeWord(0);
  writer.endWrite();
}
void encodeCapability(ModuleBuilder &module, WordWriter &writer)
{
  for (auto &&cap : module.getEnabledCapabilities())
  {
    writer.beginInstruction(Op::OpCapability, 1);
    writer.writeWord(static_cast<unsigned>(cap));
    writer.endWrite();
  }
}
void encodeExtensionEnable(ModuleBuilder &module, WordWriter &writer)
{
  module.enumerateEnabledExtensions([&writer](auto, const char *name, size_t name_len) //
    {
      writer.beginInstruction(Op::OpExtension, (name_len + sizeof(unsigned)) / sizeof(unsigned));
      writer.writeString(name);
      writer.endWrite();
    });
}
void encodeExtendedGrammarLoad(ModuleBuilder &module, WordWriter &writer)
{
  module.enumerateLoadedExtendedGrammars([&writer](Id id, auto, const char *name, size_t name_len) //
    {
      writer.beginInstruction(Op::OpExtInstImport, 1 + (name_len + sizeof(unsigned)) / sizeof(unsigned));
      writer.writeWord(id);
      writer.writeString(name);
      writer.endWrite();
    });
}
void encodeSetMemoryModel(ModuleBuilder &module, WordWriter &writer)
{
  writer.beginInstruction(Op::OpMemoryModel, 2);
  writer.writeWord(static_cast<unsigned>(module.getAddressingModel()));
  writer.writeWord(static_cast<unsigned>(module.getMemoryModel()));
  writer.endWrite();
}
void encodeEntryPoint(ModuleBuilder &module, WordWriter &writer)
{
  module.enumerateEntryPoints([&writer](auto func, auto e_model, const auto &name, const auto &interface_list) //
    {
      writer.beginInstruction(Op::OpEntryPoint, 2 + interface_list.size() + ((name.length() + sizeof(unsigned)) / sizeof(unsigned)));
      writer.writeWord(static_cast<unsigned>(e_model));
      writer.writeWord(func->resultId);
      writer.writeString(name.c_str());
      for (auto &&i : interface_list)
        writer.writeWord(i->resultId);
      writer.endWrite();
    });
}
struct ExecutionModeWriteVisitor
{
  WordWriter &target;
  NodePointer<NodeOpFunction> function;
  void operator()(const ExecutionModeInvocations *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::Invocations));
    target.writeWord(static_cast<Id>(value->numberOfInvocationInvocations.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeSpacingEqual *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SpacingEqual));
    target.endWrite();
  }
  void operator()(const ExecutionModeSpacingFractionalEven *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SpacingFractionalEven));
    target.endWrite();
  }
  void operator()(const ExecutionModeSpacingFractionalOdd *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SpacingFractionalOdd));
    target.endWrite();
  }
  void operator()(const ExecutionModeVertexOrderCw *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::VertexOrderCw));
    target.endWrite();
  }
  void operator()(const ExecutionModeVertexOrderCcw *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::VertexOrderCcw));
    target.endWrite();
  }
  void operator()(const ExecutionModePixelCenterInteger *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::PixelCenterInteger));
    target.endWrite();
  }
  void operator()(const ExecutionModeOriginUpperLeft *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::OriginUpperLeft));
    target.endWrite();
  }
  void operator()(const ExecutionModeOriginLowerLeft *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::OriginLowerLeft));
    target.endWrite();
  }
  void operator()(const ExecutionModeEarlyFragmentTests *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::EarlyFragmentTests));
    target.endWrite();
  }
  void operator()(const ExecutionModePointMode *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::PointMode));
    target.endWrite();
  }
  void operator()(const ExecutionModeXfb *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::Xfb));
    target.endWrite();
  }
  void operator()(const ExecutionModeDepthReplacing *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::DepthReplacing));
    target.endWrite();
  }
  void operator()(const ExecutionModeDepthGreater *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::DepthGreater));
    target.endWrite();
  }
  void operator()(const ExecutionModeDepthLess *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::DepthLess));
    target.endWrite();
  }
  void operator()(const ExecutionModeDepthUnchanged *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::DepthUnchanged));
    target.endWrite();
  }
  void operator()(const ExecutionModeLocalSize *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 5);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::LocalSize));
    target.writeWord(static_cast<Id>(value->xSize.value));
    target.writeWord(static_cast<Id>(value->ySize.value));
    target.writeWord(static_cast<Id>(value->zSize.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeLocalSizeHint *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 5);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::LocalSizeHint));
    target.writeWord(static_cast<Id>(value->xSize.value));
    target.writeWord(static_cast<Id>(value->ySize.value));
    target.writeWord(static_cast<Id>(value->zSize.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeInputPoints *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::InputPoints));
    target.endWrite();
  }
  void operator()(const ExecutionModeInputLines *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::InputLines));
    target.endWrite();
  }
  void operator()(const ExecutionModeInputLinesAdjacency *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::InputLinesAdjacency));
    target.endWrite();
  }
  void operator()(const ExecutionModeTriangles *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::Triangles));
    target.endWrite();
  }
  void operator()(const ExecutionModeInputTrianglesAdjacency *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::InputTrianglesAdjacency));
    target.endWrite();
  }
  void operator()(const ExecutionModeQuads *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::Quads));
    target.endWrite();
  }
  void operator()(const ExecutionModeIsolines *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::Isolines));
    target.endWrite();
  }
  void operator()(const ExecutionModeOutputVertices *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::OutputVertices));
    target.writeWord(static_cast<Id>(value->vertexCount.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeOutputPoints *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::OutputPoints));
    target.endWrite();
  }
  void operator()(const ExecutionModeOutputLineStrip *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::OutputLineStrip));
    target.endWrite();
  }
  void operator()(const ExecutionModeOutputTriangleStrip *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::OutputTriangleStrip));
    target.endWrite();
  }
  void operator()(const ExecutionModeVecTypeHint *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::VecTypeHint));
    target.writeWord(static_cast<Id>(value->vectorType.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeContractionOff *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::ContractionOff));
    target.endWrite();
  }
  void operator()(const ExecutionModeInitializer *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::Initializer));
    target.endWrite();
  }
  void operator()(const ExecutionModeFinalizer *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::Finalizer));
    target.endWrite();
  }
  void operator()(const ExecutionModeSubgroupSize *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SubgroupSize));
    target.writeWord(static_cast<Id>(value->subgroupSize.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeSubgroupsPerWorkgroup *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SubgroupsPerWorkgroup));
    target.writeWord(static_cast<Id>(value->subgroupsPerWorkgroup.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeSubgroupsPerWorkgroupId *value)
  {
    target.beginInstruction(Op::OpExecutionModeId, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SubgroupsPerWorkgroupId));
    target.writeWord(value->subgroupsPerWorkgroup->resultId);
    target.endWrite();
  }
  void operator()(const ExecutionModeLocalSizeId *value)
  {
    target.beginInstruction(Op::OpExecutionModeId, 5);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::LocalSizeId));
    target.writeWord(value->xSize->resultId);
    target.writeWord(value->ySize->resultId);
    target.writeWord(value->zSize->resultId);
    target.endWrite();
  }
  void operator()(const ExecutionModeLocalSizeHintId *value)
  {
    target.beginInstruction(Op::OpExecutionModeId, 5);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::LocalSizeHintId));
    target.writeWord(value->xSizeHint->resultId);
    target.writeWord(value->ySizeHint->resultId);
    target.writeWord(value->zSizeHint->resultId);
    target.endWrite();
  }
  void operator()(const ExecutionModeSubgroupUniformControlFlowKHR *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SubgroupUniformControlFlowKHR));
    target.endWrite();
  }
  void operator()(const ExecutionModePostDepthCoverage *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::PostDepthCoverage));
    target.endWrite();
  }
  void operator()(const ExecutionModeDenormPreserve *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::DenormPreserve));
    target.writeWord(static_cast<Id>(value->targetWidth.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeDenormFlushToZero *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::DenormFlushToZero));
    target.writeWord(static_cast<Id>(value->targetWidth.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeSignedZeroInfNanPreserve *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SignedZeroInfNanPreserve));
    target.writeWord(static_cast<Id>(value->targetWidth.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeRoundingModeRTE *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::RoundingModeRTE));
    target.writeWord(static_cast<Id>(value->targetWidth.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeRoundingModeRTZ *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::RoundingModeRTZ));
    target.writeWord(static_cast<Id>(value->targetWidth.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeStencilRefReplacingEXT *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::StencilRefReplacingEXT));
    target.endWrite();
  }
  void operator()(const ExecutionModeOutputLinesNV *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::OutputLinesNV));
    target.endWrite();
  }
  void operator()(const ExecutionModeOutputPrimitivesNV *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::OutputPrimitivesNV));
    target.writeWord(static_cast<Id>(value->primitiveCount.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeDerivativeGroupQuadsNV *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::DerivativeGroupQuadsNV));
    target.endWrite();
  }
  void operator()(const ExecutionModeDerivativeGroupLinearNV *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::DerivativeGroupLinearNV));
    target.endWrite();
  }
  void operator()(const ExecutionModeOutputTrianglesNV *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::OutputTrianglesNV));
    target.endWrite();
  }
  void operator()(const ExecutionModePixelInterlockOrderedEXT *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::PixelInterlockOrderedEXT));
    target.endWrite();
  }
  void operator()(const ExecutionModePixelInterlockUnorderedEXT *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::PixelInterlockUnorderedEXT));
    target.endWrite();
  }
  void operator()(const ExecutionModeSampleInterlockOrderedEXT *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SampleInterlockOrderedEXT));
    target.endWrite();
  }
  void operator()(const ExecutionModeSampleInterlockUnorderedEXT *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SampleInterlockUnorderedEXT));
    target.endWrite();
  }
  void operator()(const ExecutionModeShadingRateInterlockOrderedEXT *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::ShadingRateInterlockOrderedEXT));
    target.endWrite();
  }
  void operator()(const ExecutionModeShadingRateInterlockUnorderedEXT *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::ShadingRateInterlockUnorderedEXT));
    target.endWrite();
  }
  void operator()(const ExecutionModeSharedLocalMemorySizeINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SharedLocalMemorySizeINTEL));
    target.writeWord(static_cast<Id>(value->size.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeRoundingModeRTPINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::RoundingModeRTPINTEL));
    target.writeWord(static_cast<Id>(value->targetWidth.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeRoundingModeRTNINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::RoundingModeRTNINTEL));
    target.writeWord(static_cast<Id>(value->targetWidth.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeFloatingPointModeALTINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::FloatingPointModeALTINTEL));
    target.writeWord(static_cast<Id>(value->targetWidth.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeFloatingPointModeIEEEINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::FloatingPointModeIEEEINTEL));
    target.writeWord(static_cast<Id>(value->targetWidth.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeMaxWorkgroupSizeINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 5);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::MaxWorkgroupSizeINTEL));
    target.writeWord(static_cast<Id>(value->max_x_size.value));
    target.writeWord(static_cast<Id>(value->max_y_size.value));
    target.writeWord(static_cast<Id>(value->max_z_size.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeMaxWorkDimINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::MaxWorkDimINTEL));
    target.writeWord(static_cast<Id>(value->max_dimensions.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeNoGlobalOffsetINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 2);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::NoGlobalOffsetINTEL));
    target.endWrite();
  }
  void operator()(const ExecutionModeNumSIMDWorkitemsINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::NumSIMDWorkitemsINTEL));
    target.writeWord(static_cast<Id>(value->vector_width.value));
    target.endWrite();
  }
  void operator()(const ExecutionModeSchedulerTargetFmaxMhzINTEL *value)
  {
    target.beginInstruction(Op::OpExecutionMode, 3);
    target.writeWord(function->resultId);
    target.writeWord(static_cast<Id>(ExecutionMode::SchedulerTargetFmaxMhzINTEL));
    target.writeWord(static_cast<Id>(value->target_fmax.value));
    target.endWrite();
  }
};
void encodeExecutionMode(ModuleBuilder &module, WordWriter &writer)
{
  module.enumerateExecutionModes([&writer](auto func, auto em) //
    {
      executionModeVisitor(em, ExecutionModeWriteVisitor{writer, func});
    });
}
void encodeDebugString(ModuleBuilder &module, WordWriter &writer)
{
  Id emptyStringId = 0;
  module.enumerateNodeType<NodeOpString>([&writer, &emptyStringId](auto sn) //
    {
      if (sn->string.empty())
        emptyStringId = sn->resultId;
      writer.beginInstruction(Op::OpString, 1 + (sn->string.length() + sizeof(unsigned)) / sizeof(unsigned));
      writer.writeWord(sn->resultId);
      writer.writeString(sn->string.c_str());
      writer.endWrite();
    });
  if (!module.getSourceString().empty() && module.getSourceFile().value == 0 && 0 == emptyStringId)
  {
    // allocate empty string if needed
    emptyStringId = module.allocateId();
    writer.beginInstruction(Op::OpString, 2);
    writer.writeWord(emptyStringId);
    writer.writeWord(0);
    writer.endWrite();
  }
  module.enumerateSourceExtensions([&writer](const eastl::string &str) //
    {
      writer.beginInstruction(Op::OpSourceExtension, (str.length() + sizeof(unsigned)) / sizeof(unsigned));
      writer.writeString(str.c_str());
      writer.endWrite();
    });
  if (module.getSourceLanguage() != SourceLanguage::Unknown)
  {
    auto sourceStringLength = module.getSourceString().length();
    bool writeSourceString = 0 != sourceStringLength;
    auto sourceFileNameRef = module.getSourceFile();
    bool writeSourceFile = sourceFileNameRef.value != 0;
    if (writeSourceString && !writeSourceFile)
    {
      sourceFileNameRef.value = emptyStringId;
      writeSourceFile = true;
    }
    // lang id + lang ver
    size_t sourceInfoLength = 2;
    if (writeSourceFile)
      ++sourceInfoLength;
    if (writeSourceString)
      sourceInfoLength += writer.calculateStringWords(sourceStringLength);
    writer.beginInstruction(Op::OpSource, sourceInfoLength);
    writer.writeWord(static_cast<unsigned>(module.getSourceLanguage()));
    writer.writeWord(module.getSourceLanguageVersion());
    if (writeSourceFile)
      writer.writeWord(sourceFileNameRef.value);
    const char *sourceWritePtr = nullptr;
    if (writeSourceString)
      sourceWritePtr = writer.writeString(module.getSourceString().c_str());
    writer.endWrite();
    // if source code string is too long, we need to break it up into chunks of 0xffff char blocks
    while (sourceWritePtr)
    {
      auto dist = sourceWritePtr - module.getSourceString().c_str();
      writer.beginInstruction(Op::OpSourceContinued, writer.calculateStringWords(sourceStringLength - dist));
      sourceWritePtr = writer.writeString(sourceWritePtr);
      writer.endWrite();
    }
  }
}
void encodeDebugName(ModuleBuilder &module, WordWriter &writer)
{
  module.enumerateAllInstructionsWithProperty<PropertyName>([&writer](auto node, auto property) //
    { property->write(node, writer); });
}
void encodeModuleProcessingInfo(ModuleBuilder &module, WordWriter &writer)
{
  module.enumerateModuleProcessed([&writer](const char *str, size_t len) //
    {
      writer.beginInstruction(Op::OpModuleProcessed, (len + sizeof(unsigned)) / sizeof(unsigned));
      writer.writeString(str);
      writer.endWrite();
    });
}
struct DecorationWriteVisitor
{
  WordWriter &target;
  NodePointer<NodeId> node;
  // special property that is handled by the name writer
  void operator()(const PropertyName *) {}
  // property is used to determine if a pointer type needs a forward declaration
  void operator()(const PropertyForwardDeclaration *) {}
  // property used to place OpSelectionMerge at the right location
  void operator()(const PropertySelectionMerge *) {}
  // property used to place OpLoopMerge ar the right location
  void operator()(const PorpertyLoopMerge *) {}
  void operator()(const PropertyRelaxedPrecision *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::RelaxedPrecision));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::RelaxedPrecision));
    }
    target.endWrite();
  }
  void operator()(const PropertySpecId *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::SpecId));
      target.writeWord(static_cast<Id>(value->specializationConstantId.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::SpecId));
      target.writeWord(static_cast<Id>(value->specializationConstantId.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyBlock *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Block));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Block));
    }
    target.endWrite();
  }
  void operator()(const PropertyBufferBlock *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BufferBlock));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BufferBlock));
    }
    target.endWrite();
  }
  void operator()(const PropertyRowMajor *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::RowMajor));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::RowMajor));
    }
    target.endWrite();
  }
  void operator()(const PropertyColMajor *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::ColMajor));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::ColMajor));
    }
    target.endWrite();
  }
  void operator()(const PropertyArrayStride *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::ArrayStride));
      target.writeWord(static_cast<Id>(value->arrayStride.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::ArrayStride));
      target.writeWord(static_cast<Id>(value->arrayStride.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyMatrixStride *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::MatrixStride));
      target.writeWord(static_cast<Id>(value->matrixStride.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::MatrixStride));
      target.writeWord(static_cast<Id>(value->matrixStride.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyGLSLShared *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::GLSLShared));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::GLSLShared));
    }
    target.endWrite();
  }
  void operator()(const PropertyGLSLPacked *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::GLSLPacked));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::GLSLPacked));
    }
    target.endWrite();
  }
  void operator()(const PropertyCPacked *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::CPacked));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::CPacked));
    }
    target.endWrite();
  }
  void operator()(const PropertyBuiltIn *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BuiltIn));
      target.writeWord(static_cast<Id>(value->builtIn));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BuiltIn));
      target.writeWord(static_cast<Id>(value->builtIn));
    }
    target.endWrite();
  }
  void operator()(const PropertyNoPerspective *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::NoPerspective));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::NoPerspective));
    }
    target.endWrite();
  }
  void operator()(const PropertyFlat *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Flat));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Flat));
    }
    target.endWrite();
  }
  void operator()(const PropertyPatch *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Patch));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Patch));
    }
    target.endWrite();
  }
  void operator()(const PropertyCentroid *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Centroid));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Centroid));
    }
    target.endWrite();
  }
  void operator()(const PropertySample *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Sample));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Sample));
    }
    target.endWrite();
  }
  void operator()(const PropertyInvariant *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Invariant));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Invariant));
    }
    target.endWrite();
  }
  void operator()(const PropertyRestrict *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Restrict));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Restrict));
    }
    target.endWrite();
  }
  void operator()(const PropertyAliased *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Aliased));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Aliased));
    }
    target.endWrite();
  }
  void operator()(const PropertyVolatile *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Volatile));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Volatile));
    }
    target.endWrite();
  }
  void operator()(const PropertyConstant *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Constant));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Constant));
    }
    target.endWrite();
  }
  void operator()(const PropertyCoherent *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Coherent));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Coherent));
    }
    target.endWrite();
  }
  void operator()(const PropertyNonWritable *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::NonWritable));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::NonWritable));
    }
    target.endWrite();
  }
  void operator()(const PropertyNonReadable *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::NonReadable));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::NonReadable));
    }
    target.endWrite();
  }
  void operator()(const PropertyUniform *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Uniform));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Uniform));
    }
    target.endWrite();
  }
  void operator()(const PropertyUniformId *value)
  {
    target.beginInstruction(Op::OpDecorateId, 2 + 1);
    target.writeWord(node->resultId);
    target.writeWord(static_cast<Id>(Decoration::UniformId));
    target.writeWord(value->execution->resultId);
    target.endWrite();
  }
  void operator()(const PropertySaturatedConversion *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::SaturatedConversion));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::SaturatedConversion));
    }
    target.endWrite();
  }
  void operator()(const PropertyStream *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Stream));
      target.writeWord(static_cast<Id>(value->streamNumber.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Stream));
      target.writeWord(static_cast<Id>(value->streamNumber.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyLocation *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Location));
      target.writeWord(static_cast<Id>(value->location.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Location));
      target.writeWord(static_cast<Id>(value->location.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyComponent *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Component));
      target.writeWord(static_cast<Id>(value->component.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Component));
      target.writeWord(static_cast<Id>(value->component.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyIndex *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Index));
      target.writeWord(static_cast<Id>(value->index.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Index));
      target.writeWord(static_cast<Id>(value->index.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyBinding *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Binding));
      target.writeWord(static_cast<Id>(value->bindingPoint.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Binding));
      target.writeWord(static_cast<Id>(value->bindingPoint.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyDescriptorSet *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::DescriptorSet));
      target.writeWord(static_cast<Id>(value->descriptorSet.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::DescriptorSet));
      target.writeWord(static_cast<Id>(value->descriptorSet.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyOffset *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Offset));
      target.writeWord(static_cast<Id>(value->byteOffset.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Offset));
      target.writeWord(static_cast<Id>(value->byteOffset.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyXfbBuffer *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::XfbBuffer));
      target.writeWord(static_cast<Id>(value->xfbBufferNumber.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::XfbBuffer));
      target.writeWord(static_cast<Id>(value->xfbBufferNumber.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyXfbStride *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::XfbStride));
      target.writeWord(static_cast<Id>(value->xfbStride.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::XfbStride));
      target.writeWord(static_cast<Id>(value->xfbStride.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyFuncParamAttr *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::FuncParamAttr));
      target.writeWord(static_cast<Id>(value->functionParameterAttribute));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::FuncParamAttr));
      target.writeWord(static_cast<Id>(value->functionParameterAttribute));
    }
    target.endWrite();
  }
  void operator()(const PropertyFPRoundingMode *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::FPRoundingMode));
      target.writeWord(static_cast<Id>(value->floatingPointRoundingMode));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::FPRoundingMode));
      target.writeWord(static_cast<Id>(value->floatingPointRoundingMode));
    }
    target.endWrite();
  }
  void operator()(const PropertyFPFastMathMode *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::FPFastMathMode));
      target.writeWord(static_cast<Id>(value->fastMathMode));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::FPFastMathMode));
      target.writeWord(static_cast<Id>(value->fastMathMode));
    }
    target.endWrite();
  }
  void operator()(const PropertyLinkageAttributes *value)
  {
    size_t length = 0;
    length += (value->name.length() + sizeof(unsigned)) / sizeof(unsigned);
    ++length;
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + length);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::LinkageAttributes));
      target.writeString(value->name.c_str());
      target.writeWord(static_cast<Id>(value->linkageType));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + length);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::LinkageAttributes));
      target.writeString(value->name.c_str());
      target.writeWord(static_cast<Id>(value->linkageType));
    }
    target.endWrite();
  }
  void operator()(const PropertyNoContraction *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::NoContraction));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::NoContraction));
    }
    target.endWrite();
  }
  void operator()(const PropertyInputAttachmentIndex *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::InputAttachmentIndex));
      target.writeWord(static_cast<Id>(value->attachmentIndex.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::InputAttachmentIndex));
      target.writeWord(static_cast<Id>(value->attachmentIndex.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyAlignment *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::Alignment));
      target.writeWord(static_cast<Id>(value->alignment.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::Alignment));
      target.writeWord(static_cast<Id>(value->alignment.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyMaxByteOffset *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::MaxByteOffset));
      target.writeWord(static_cast<Id>(value->maxByteOffset.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::MaxByteOffset));
      target.writeWord(static_cast<Id>(value->maxByteOffset.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyAlignmentId *value)
  {
    target.beginInstruction(Op::OpDecorateId, 2 + 1);
    target.writeWord(node->resultId);
    target.writeWord(static_cast<Id>(Decoration::AlignmentId));
    target.writeWord(value->alignment->resultId);
    target.endWrite();
  }
  void operator()(const PropertyMaxByteOffsetId *value)
  {
    target.beginInstruction(Op::OpDecorateId, 2 + 1);
    target.writeWord(node->resultId);
    target.writeWord(static_cast<Id>(Decoration::MaxByteOffsetId));
    target.writeWord(value->maxByteOffset->resultId);
    target.endWrite();
  }
  void operator()(const PropertyNoSignedWrap *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::NoSignedWrap));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::NoSignedWrap));
    }
    target.endWrite();
  }
  void operator()(const PropertyNoUnsignedWrap *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::NoUnsignedWrap));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::NoUnsignedWrap));
    }
    target.endWrite();
  }
  void operator()(const PropertyExplicitInterpAMD *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::ExplicitInterpAMD));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::ExplicitInterpAMD));
    }
    target.endWrite();
  }
  void operator()(const PropertyOverrideCoverageNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::OverrideCoverageNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::OverrideCoverageNV));
    }
    target.endWrite();
  }
  void operator()(const PropertyPassthroughNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::PassthroughNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::PassthroughNV));
    }
    target.endWrite();
  }
  void operator()(const PropertyViewportRelativeNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::ViewportRelativeNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::ViewportRelativeNV));
    }
    target.endWrite();
  }
  void operator()(const PropertySecondaryViewportRelativeNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::SecondaryViewportRelativeNV));
      target.writeWord(static_cast<Id>(value->offset.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::SecondaryViewportRelativeNV));
      target.writeWord(static_cast<Id>(value->offset.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyPerPrimitiveNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::PerPrimitiveNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::PerPrimitiveNV));
    }
    target.endWrite();
  }
  void operator()(const PropertyPerViewNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::PerViewNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::PerViewNV));
    }
    target.endWrite();
  }
  void operator()(const PropertyPerTaskNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::PerTaskNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::PerTaskNV));
    }
    target.endWrite();
  }
  void operator()(const PropertyPerVertexKHR *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::PerVertexKHR));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::PerVertexKHR));
    }
    target.endWrite();
  }
  void operator()(const PropertyNonUniform *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::NonUniform));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::NonUniform));
    }
    target.endWrite();
  }
  void operator()(const PropertyRestrictPointer *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::RestrictPointer));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::RestrictPointer));
    }
    target.endWrite();
  }
  void operator()(const PropertyAliasedPointer *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::AliasedPointer));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::AliasedPointer));
    }
    target.endWrite();
  }
  void operator()(const PropertyBindlessSamplerNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BindlessSamplerNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BindlessSamplerNV));
    }
    target.endWrite();
  }
  void operator()(const PropertyBindlessImageNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BindlessImageNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BindlessImageNV));
    }
    target.endWrite();
  }
  void operator()(const PropertyBoundSamplerNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BoundSamplerNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BoundSamplerNV));
    }
    target.endWrite();
  }
  void operator()(const PropertyBoundImageNV *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BoundImageNV));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BoundImageNV));
    }
    target.endWrite();
  }
  void operator()(const PropertySIMTCallINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::SIMTCallINTEL));
      target.writeWord(static_cast<Id>(value->n.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::SIMTCallINTEL));
      target.writeWord(static_cast<Id>(value->n.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyReferencedIndirectlyINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::ReferencedIndirectlyINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::ReferencedIndirectlyINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyClobberINTEL *value)
  {
    size_t length = 0;
    length += (value->_register.length() + sizeof(unsigned)) / sizeof(unsigned);
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorateStringGOOGLE, 3 + length);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::ClobberINTEL));
      target.writeString(value->_register.c_str());
    }
    else
    {
      target.beginInstruction(Op::OpDecorateStringGOOGLE, 2 + length);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::ClobberINTEL));
      target.writeString(value->_register.c_str());
    }
    target.endWrite();
  }
  void operator()(const PropertySideEffectsINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::SideEffectsINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::SideEffectsINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyVectorComputeVariableINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::VectorComputeVariableINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::VectorComputeVariableINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyFuncParamIOKindINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::FuncParamIOKindINTEL));
      target.writeWord(static_cast<Id>(value->kind.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::FuncParamIOKindINTEL));
      target.writeWord(static_cast<Id>(value->kind.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyVectorComputeFunctionINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::VectorComputeFunctionINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::VectorComputeFunctionINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyStackCallINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::StackCallINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::StackCallINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyGlobalVariableOffsetINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::GlobalVariableOffsetINTEL));
      target.writeWord(static_cast<Id>(value->offset.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::GlobalVariableOffsetINTEL));
      target.writeWord(static_cast<Id>(value->offset.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyCounterBuffer *value)
  {
    target.beginInstruction(Op::OpDecorateId, 2 + 1);
    target.writeWord(node->resultId);
    target.writeWord(static_cast<Id>(Decoration::CounterBuffer));
    target.writeWord(value->counterBuffer->resultId);
    target.endWrite();
  }
  void operator()(const PropertyUserSemantic *value)
  {
    size_t length = 0;
    length += (value->semantic.length() + sizeof(unsigned)) / sizeof(unsigned);
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorateStringGOOGLE, 3 + length);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::UserSemantic));
      target.writeString(value->semantic.c_str());
    }
    else
    {
      target.beginInstruction(Op::OpDecorateStringGOOGLE, 2 + length);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::UserSemantic));
      target.writeString(value->semantic.c_str());
    }
    target.endWrite();
  }
  void operator()(const PropertyUserTypeGOOGLE *value)
  {
    size_t length = 0;
    length += (value->userType.length() + sizeof(unsigned)) / sizeof(unsigned);
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorateStringGOOGLE, 3 + length);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::UserTypeGOOGLE));
      target.writeString(value->userType.c_str());
    }
    else
    {
      target.beginInstruction(Op::OpDecorateStringGOOGLE, 2 + length);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::UserTypeGOOGLE));
      target.writeString(value->userType.c_str());
    }
    target.endWrite();
  }
  void operator()(const PropertyFunctionRoundingModeINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 2);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::FunctionRoundingModeINTEL));
      target.writeWord(static_cast<Id>(value->targetWidth.value));
      target.writeWord(static_cast<Id>(value->fpRoundingMode));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::FunctionRoundingModeINTEL));
      target.writeWord(static_cast<Id>(value->targetWidth.value));
      target.writeWord(static_cast<Id>(value->fpRoundingMode));
    }
    target.endWrite();
  }
  void operator()(const PropertyFunctionDenormModeINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 2);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::FunctionDenormModeINTEL));
      target.writeWord(static_cast<Id>(value->targetWidth.value));
      target.writeWord(static_cast<Id>(value->fpDenormMode));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::FunctionDenormModeINTEL));
      target.writeWord(static_cast<Id>(value->targetWidth.value));
      target.writeWord(static_cast<Id>(value->fpDenormMode));
    }
    target.endWrite();
  }
  void operator()(const PropertyRegisterINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::RegisterINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::RegisterINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyMemoryINTEL *value)
  {
    size_t length = 0;
    length += (value->memoryType.length() + sizeof(unsigned)) / sizeof(unsigned);
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorateStringGOOGLE, 3 + length);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::MemoryINTEL));
      target.writeString(value->memoryType.c_str());
    }
    else
    {
      target.beginInstruction(Op::OpDecorateStringGOOGLE, 2 + length);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::MemoryINTEL));
      target.writeString(value->memoryType.c_str());
    }
    target.endWrite();
  }
  void operator()(const PropertyNumbanksINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::NumbanksINTEL));
      target.writeWord(static_cast<Id>(value->banks.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::NumbanksINTEL));
      target.writeWord(static_cast<Id>(value->banks.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyBankwidthINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BankwidthINTEL));
      target.writeWord(static_cast<Id>(value->bankWidth.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BankwidthINTEL));
      target.writeWord(static_cast<Id>(value->bankWidth.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyMaxPrivateCopiesINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::MaxPrivateCopiesINTEL));
      target.writeWord(static_cast<Id>(value->maximumCopies.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::MaxPrivateCopiesINTEL));
      target.writeWord(static_cast<Id>(value->maximumCopies.value));
    }
    target.endWrite();
  }
  void operator()(const PropertySinglepumpINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::SinglepumpINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::SinglepumpINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyDoublepumpINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::DoublepumpINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::DoublepumpINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyMaxReplicatesINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::MaxReplicatesINTEL));
      target.writeWord(static_cast<Id>(value->maximumReplicates.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::MaxReplicatesINTEL));
      target.writeWord(static_cast<Id>(value->maximumReplicates.value));
    }
    target.endWrite();
  }
  void operator()(const PropertySimpleDualPortINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::SimpleDualPortINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::SimpleDualPortINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyMergeINTEL *value)
  {
    size_t length = 0;
    length += (value->mergeKey.length() + sizeof(unsigned)) / sizeof(unsigned);
    length += (value->mergeType.length() + sizeof(unsigned)) / sizeof(unsigned);
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + length);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::MergeINTEL));
      target.writeString(value->mergeKey.c_str());
      target.writeString(value->mergeType.c_str());
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + length);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::MergeINTEL));
      target.writeString(value->mergeKey.c_str());
      target.writeString(value->mergeType.c_str());
    }
    target.endWrite();
  }
  void operator()(const PropertyBankBitsINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BankBitsINTEL));
      target.writeWord(static_cast<Id>(value->bankBits.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BankBitsINTEL));
      target.writeWord(static_cast<Id>(value->bankBits.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyForcePow2DepthINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::ForcePow2DepthINTEL));
      target.writeWord(static_cast<Id>(value->forceKey.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::ForcePow2DepthINTEL));
      target.writeWord(static_cast<Id>(value->forceKey.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyBurstCoalesceINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BurstCoalesceINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BurstCoalesceINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyCacheSizeINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::CacheSizeINTEL));
      target.writeWord(static_cast<Id>(value->cacheSizeInBytes.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::CacheSizeINTEL));
      target.writeWord(static_cast<Id>(value->cacheSizeInBytes.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyDontStaticallyCoalesceINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::DontStaticallyCoalesceINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::DontStaticallyCoalesceINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyPrefetchINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::PrefetchINTEL));
      target.writeWord(static_cast<Id>(value->prefetcherSizeInBytes.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::PrefetchINTEL));
      target.writeWord(static_cast<Id>(value->prefetcherSizeInBytes.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyStallEnableINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::StallEnableINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::StallEnableINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyFuseLoopsInFunctionINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::FuseLoopsInFunctionINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::FuseLoopsInFunctionINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyBufferLocationINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::BufferLocationINTEL));
      target.writeWord(static_cast<Id>(value->bufferLocationId.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::BufferLocationINTEL));
      target.writeWord(static_cast<Id>(value->bufferLocationId.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyIOPipeStorageINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 1);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::IOPipeStorageINTEL));
      target.writeWord(static_cast<Id>(value->ioPipeId.value));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 1);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::IOPipeStorageINTEL));
      target.writeWord(static_cast<Id>(value->ioPipeId.value));
    }
    target.endWrite();
  }
  void operator()(const PropertyFunctionFloatingPointModeINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3 + 2);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::FunctionFloatingPointModeINTEL));
      target.writeWord(static_cast<Id>(value->targetWidth.value));
      target.writeWord(static_cast<Id>(value->fpOperationMode));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2 + 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::FunctionFloatingPointModeINTEL));
      target.writeWord(static_cast<Id>(value->targetWidth.value));
      target.writeWord(static_cast<Id>(value->fpOperationMode));
    }
    target.endWrite();
  }
  void operator()(const PropertySingleElementVectorINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::SingleElementVectorINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::SingleElementVectorINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyVectorComputeCallableFunctionINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::VectorComputeCallableFunctionINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::VectorComputeCallableFunctionINTEL));
    }
    target.endWrite();
  }
  void operator()(const PropertyMediaBlockIOINTEL *value)
  {
    if (value->memberIndex)
    {
      target.beginInstruction(Op::OpMemberDecorate, 3);
      target.writeWord(node->resultId);
      target.writeWord(*value->memberIndex);
      target.writeWord(static_cast<Id>(Decoration::MediaBlockIOINTEL));
    }
    else
    {
      target.beginInstruction(Op::OpDecorate, 2);
      target.writeWord(node->resultId);
      target.writeWord(static_cast<Id>(Decoration::MediaBlockIOINTEL));
    }
    target.endWrite();
  }
};
void encodeAnnotation(ModuleBuilder &module, WordWriter &writer)
{
  module.enumerateAllInstructionsWithAnyProperty([&writer](auto node) //
    {
      for (auto &&property : node->properties)
        visitProperty(property, DecorationWriteVisitor{writer, node});
    });
}
void encodeTypeVariableAndConstant(ModuleBuilder &module, WordWriter &writer)
{
  bool reTry = false;
  FlatSet<Id> writtenTypeIds;
  FlatSet<Id> writtenForwardIds;
  FlatSet<Id> notWrittenTypeIds;
  FlatSet<Id> notWrittenConstantIds;
  FlatSet<Id> writtenConstantIds;
  // first get the forward declarations done and gather all type ids
  module.enumerateAllTypes([&writer, &writtenForwardIds, &notWrittenTypeIds](auto node) //
    {
      notWrittenTypeIds.insert(node->resultId);
      for (auto &&p : node->properties)
      {
        if (is<PropertyForwardDeclaration>(p))
        {
          as<PropertyForwardDeclaration>(p)->write(node, writer);
          writtenForwardIds.insert(node->resultId);
        }
      }
    });
  // second get all constants that we need to write
  module.enumerateAllConstants([&notWrittenConstantIds](auto node) //
    { notWrittenConstantIds.insert(node->resultId); });
  while (!notWrittenTypeIds.empty() || !notWrittenConstantIds.empty())
  {
    bool keepWriting = false;
    do
    {
      keepWriting = false;
      for (auto &&constId : notWrittenConstantIds)
      {
        auto constant = as<NodeOperation>(module.getNode(constId));
        if (writtenTypeIds.has(constant->resultType->resultId))
        {
          visitNode(constant, NodeWriteVisitor{writer, module});
          writtenConstantIds.insert(constId);
          notWrittenConstantIds.remove(constId);
          // iterators are invalid, have to go another round for the next constants
          keepWriting = true;
          break;
        }
      }
    } while (keepWriting);
    do
    {
      keepWriting = false;
      for (auto &&typeId : notWrittenTypeIds)
      { // try write type
        auto typeDef = as<NodeTypedef>(module.getNode(typeId));
        bool canWrite = true;
        visitNode(typeDef, CanWriteTypeCheckVisitor //
          {writtenTypeIds, writtenForwardIds, writtenConstantIds, canWrite});
        if (canWrite)
        {
          visitNode(typeDef, NodeWriteVisitor{writer, module});
          writtenForwardIds.remove(typeId);
          writtenTypeIds.insert(typeId);
          notWrittenTypeIds.remove(typeId);
          // iterators are invalid, have to go another round for the next type
          keepWriting = true;
          break;
        }
      }
    } while (keepWriting);
  }
  // global undef stuff
  module.enumerateAllGlobalUndefs([&writer, &module](auto node) //
    {
      NodeWriteVisitor{writer, module}(node.get());
    });
  // variable stuff
  module.enumerateAllGlobalVariables([&writer, &module](auto node) //
    {
      NodeWriteVisitor{writer, module}(as<NodeOpVariable>(node).get());
    });
}
void encodeFunctionDeclaration(ModuleBuilder &module, WordWriter &writer)
{
  module.enumerateAllFunctions([&module, &writer](auto node) //
    {                                                        // only write functions without body here
      if (!node->body.empty())
        return;
      NodeWriteVisitor visitor{writer, module};
      auto opCodeNode = as<NodeOpFunction>(node);
      visitor(opCodeNode.get());
      for (auto &&param : opCodeNode->parameters)
        visitor(as<NodeOpFunctionParameter>(param).get());
      writer.beginInstruction(Op::OpFunctionEnd, 0);
      writer.endWrite();
    });
}
void encodeFunctionDefinition(ModuleBuilder &module, WordWriter &writer)
{
  module.enumerateAllFunctions([&module, &writer](auto node) //
    {                                                        // only write functions with body here
      if (node->body.empty())
        return;
      NodeWriteVisitor visitor{writer, module};
      auto opCodeNode = as<NodeOpFunction>(node);
      visitor(opCodeNode.get());
      for (auto &&param : opCodeNode->parameters)
        visitor(as<NodeOpFunctionParameter>(param).get());
      for (auto &&block : opCodeNode->body)
      {
        visitor(as<NodeOpLabel>(block).get());
        for (auto &&inst : as<NodeOpLabel>(block)->instructions)
          visitNode(inst, visitor);
        for (auto &&prop : as<NodeOpLabel>(block)->properties)
          visitProperty(prop.get(), BranchPropertyWriter{writer});
        visitNode(block->exit, visitor);
      }
      writer.beginInstruction(Op::OpFunctionEnd, 0);
      writer.endWrite();
    });
}
eastl::vector<unsigned> spirv::write(ModuleBuilder &module, const WriteConfig &cfg, ErrorHandler &e_handler)
{
  WordWriter writer;
  writer.errorHandler = &e_handler;
  encodeHeader(module, writer);
  encodeCapability(module, writer);
  encodeExtensionEnable(module, writer);
  encodeExtendedGrammarLoad(module, writer);
  encodeSetMemoryModel(module, writer);
  encodeEntryPoint(module, writer);
  encodeExecutionMode(module, writer);
  encodeDebugString(module, writer);
  encodeDebugName(module, writer);
  encodeModuleProcessingInfo(module, writer);
  encodeAnnotation(module, writer);
  encodeTypeVariableAndConstant(module, writer);
  encodeFunctionDeclaration(module, writer);
  encodeFunctionDefinition(module, writer);
  return eastl::move(writer.words);
}
