#include "module_nodes.h"
#include <spirv/module_builder.h>

using namespace spirv;

struct BufferAndCounterPair
{
  NodePointer<NodeOpVariable> counter;
  NodePointer<NodeOpVariable> buffer;
};

static NodePointer<NodeOpTypePointer> make_pointer_type_of(ModuleBuilder &builder, NodePointer<NodeTypedef> type, StorageClass sc)
{
  NodePointer<NodeOpTypePointer> result;

  builder.enumerateAllTypes([&](auto type) //
    {
      if (!result)
      {
        if (is<NodeOpTypePointer>(type))
        {
          auto ptr = as<NodeOpTypePointer>(type);
          if (ptr->storageClass == sc && ptr->type == type)
          {
            result = ptr;
          }
        }
      }
    });

  if (!result)
    result = builder.newNode<NodeOpTypePointer>(0, sc, type);

  return result;
}

// gets constant + 1
static NodePointer<NodeOpConstant> get_next_value(ModuleBuilder &builder, NodePointer<NodeOpConstant> constant)
{
  auto nextValue = constant->value;
  nextValue.value++;

  // first try to find match
  NodePointer<NodeOpConstant> nextConstant;
  builder.enumerateAllConstants([&](auto constant) //
    {
      if (!nextConstant)
      {
        if (is<NodeOpConstant>(constant))
        {
          auto cnst = as<NodeOpConstant>(constant);
          if (cnst->resultType == constant->resultType && cnst->value.value == nextValue.value)
          {
            nextConstant = cnst;
          }
        }
      }
    });

  if (!nextConstant)
    nextConstant = builder.newNode<NodeOpConstant>(0, constant->resultType, nextValue);

  return nextConstant;
}

// find or create int 32 type
static NodePointer<NodeOpTypeInt> get_counter_type(ModuleBuilder &builder)
{
  NodePointer<NodeOpTypeInt> result;
  builder.enumerateAllTypes([&](auto type_def) //
    {
      if (!result)
      {
        if (is<NodeOpTypeInt>(type_def))
        {
          auto iType = as<NodeOpTypeInt>(type_def);
          if (iType->width.value == 32 && iType->signedness.value == 1)
          {
            result = iType;
          }
        }
      }
    });
  if (!result)
  {
    result = builder.newNode<NodeOpTypeInt>(0, 32, 1);
  }
  return result;
}

static unsigned get_offset(NodePointer<NodeOpTypeStruct> structure, unsigned member)
{
  for (auto &&prop : structure->properties)
  {
    if (is<PropertyOffset>(prop))
    {
      auto ofs = as<PropertyOffset>(prop);
      if (ofs->memberIndex && *ofs->memberIndex == member)
      {
        return ofs->byteOffset.value;
      }
    }
  }
  return 0;
}

static bool has_offset(NodePointer<NodeOpTypeStruct> structure, unsigned member, unsigned offset)
{
  for (auto &&prop : structure->properties)
  {
    if (is<PropertyOffset>(prop))
    {
      auto ofs = as<PropertyOffset>(prop);
      if (ofs->memberIndex && *ofs->memberIndex == member)
      {
        return ofs->byteOffset.value == offset;
      }
    }
  }
  return false;
}

struct CopyAndAjustPropertyVisitor
{
  CastableUniquePointer<Property> operator()(const PorpertyLoopMerge *p) { return p->clone(); }
  CastableUniquePointer<Property> operator()(const PropertySelectionMerge *p) { return p->clone(); }
  CastableUniquePointer<Property> operator()(const PropertyForwardDeclaration *p) { return p->clone(); }
  CastableUniquePointer<Property> operator()(const PropertyOffset *p)
  {
    auto v = p->clone();
    if (v->memberIndex)
      ++(*v->memberIndex);
    v->byteOffset.value += 256;
    return v;
  }

  template <typename T>
  CastableUniquePointer<Property> operator()(const T *p)
  {
    auto v = p->clone();
    if (v->memberIndex)
      ++(*v->memberIndex);
    return v;
  }
};

static NodePointer<NodeOpTypeStruct> embed_counter_into_structure(ModuleBuilder &builder, NodePointer<NodeOpTypeStruct> base)
{
  NodePointer<NodeOpTypeStruct> result;
  // first try to find a struct that matches the one we want
  builder.enumerateAllTypes([&](auto type_def) //
    {
      if (!result)
      {
        if (is<NodeOpTypeStruct>(type_def))
        {
          auto structType = as<NodeOpTypeStruct>(type_def);
          if (base->param1.size() + 1 == base->param1.size() && has_offset(structType, 0, 0) && has_offset(structType, 1, 256))
          {
            unsigned i;
            for (i = 0; i < base->param1.size(); ++i)
            {
              if (base->param1[i] != structType->param1[i + 1])
                break;
              if ((get_offset(base, i) + 256) != get_offset(structType, i + 1))
                break;
            }
            if (i == base->param1.size())
            {
              result = structType;
            }
          }
        }
      }
    });

  if (!result)
  {
    auto newStruct = builder.newNode<NodeOpTypeStruct>(0, nullptr, 0);
    newStruct->param1.reserve(base->param1.size() + 1);
    newStruct->param1.push_back(get_counter_type(builder));
    newStruct->param1.insert(end(newStruct->param1), begin(base->param1), end(base->param1));
    // offset for first one
    newStruct->properties.emplace_back(new PropertyOffset);
    as<PropertyOffset>(newStruct->properties.back())->memberIndex = 0;
    as<PropertyOffset>(newStruct->properties.back())->byteOffset.value = 0;
    for (auto &&prop : base->properties)
      newStruct->properties.push_back(visitProperty(prop, CopyAndAjustPropertyVisitor{}));
    result = newStruct;
  }

  return result;
}

static bool has_any_consumers(ModuleBuilder &builder, NodePointer<NodeId> node)
{
  unsigned count = 0;
  builder.enumerateConsumersOf(node, [&](auto, auto &) { ++count; });
  return count > 0;
}

static void patch_struct_consumer(ModuleBuilder &builder, NodePointer<NodeOperation> consumer)
{
  // patch first index to offset by one
  if (is<NodeOpAccessChain>(consumer))
  {
    auto ac = as<NodeOpAccessChain>(consumer);
    builder.updateRef(ac, ac->indexes[0], get_next_value(builder, ac->indexes[0]));
  }
  else if (is<NodeOpInBoundsAccessChain>(consumer))
  {
    auto ac = as<NodeOpInBoundsAccessChain>(consumer);
    builder.updateRef(ac, ac->indexes[0], get_next_value(builder, ac->indexes[0]));
  }
  else if (is<NodeOpPtrAccessChain>(consumer))
  {
    auto ac = as<NodeOpPtrAccessChain>(consumer);
    builder.updateRef(ac, ac->element, get_next_value(builder, ac->element));
  }
  else if (is<NodeOpInBoundsPtrAccessChain>(consumer))
  {
    auto ac = as<NodeOpInBoundsPtrAccessChain>(consumer);
    builder.updateRef(ac, ac->element, get_next_value(builder, ac->element));
  }
}

static void patch_counter_consumer(ModuleBuilder &builder, NodePointer<NodeOperation> consumer, NodePointer<NodeOpVariable> buffer)
{
  // patch target variable, index is kept
  if (is<NodeOpAccessChain>(consumer))
  {
    auto ac = as<NodeOpAccessChain>(consumer);
    builder.updateRef(ac, ac->base, buffer);
  }
  else if (is<NodeOpInBoundsAccessChain>(consumer))
  {
    auto ac = as<NodeOpInBoundsAccessChain>(consumer);
    builder.updateRef(ac, ac->base, buffer);
  }
  else if (is<NodeOpPtrAccessChain>(consumer))
  {
    auto ac = as<NodeOpPtrAccessChain>(consumer);
    builder.updateRef(ac, ac->base, buffer);
  }
  else if (is<NodeOpInBoundsPtrAccessChain>(consumer))
  {
    auto ac = as<NodeOpInBoundsPtrAccessChain>(consumer);
    builder.updateRef(ac, ac->base, buffer);
  }
}

static void embed_counter_buffer(ModuleBuilder &builder, NodePointer<NodeOpVariable> buffer, NodePointer<NodeOpVariable> counter)
{
  // get struct type
  auto ptrType = as<NodeOpTypePointer>(buffer->resultType);
  auto structType = as<NodeOpTypeStruct>(ptrType->type);
  // build new patched type
  auto newStructType = embed_counter_into_structure(builder, structType);
  auto newStructPointerType = make_pointer_type_of(builder, newStructType, ptrType->storageClass);
  // patch buffer variable
  builder.updateRef(buffer, buffer->resultType, newStructPointerType);
  // patch buffer variable access
  builder.enumerateConsumersOf(buffer, [&](auto node, auto &) //
    { patch_struct_consumer(builder, node); });
  // patch counter variable access
  builder.enumerateConsumersOf(counter, [&](auto node, auto &) //
    { patch_counter_consumer(builder, node, buffer); });

  auto counterPtr = as<NodeOpTypePointer>(counter->resultType);
  // struct with one int
  auto counterStruct = as<NodeOpTypeStruct>(counterPtr->type);
  // try to remove old types
  if (!has_any_consumers(builder, ptrType))
  {
    builder.removeNode(ptrType);
  }
  if (!has_any_consumers(builder, structType))
  {
    builder.removeNode(structType);
  }
  if (!has_any_consumers(builder, counter))
  {
    builder.removeNode(counter);
  }
  if (!has_any_consumers(builder, counterPtr))
  {
    builder.removeNode(counterPtr);
  }
  if (!has_any_consumers(builder, counterStruct))
  {
    builder.removeNode(counterStruct);
  }
}

eastl::vector<NodePointer<NodeOpVariable>> spirv::resolveAtomicBuffers(ModuleBuilder &builder, AtomicResolveMode mode)
{

  // phase one, collect all buffers and buffers with counters pairs,
  // also removes the property that links them together
  eastl::vector<BufferAndCounterPair> buffersToPatch;
  builder.enumerateAllGlobalVariables([&](auto var) //
    {
      // can only have one
      auto property = eastl::find_if(begin(var->properties), end(var->properties),
        [](auto &prop) //
        { return is<PropertyHlslCounterBufferGOOGLE>(prop); });
      if (property != end(var->properties))
      {
        BufferAndCounterPair pair;
        pair.buffer = var;
        pair.counter = as<PropertyHlslCounterBufferGOOGLE>(*property)->counterBuffer;
        buffersToPatch.push_back(pair);
        // drop it right away
        var->properties.erase(property);
      }
    });

  eastl::vector<NodePointer<NodeOpVariable>> buffersWithCounterList;
  if (!buffersToPatch.empty())
  {
    buffersWithCounterList.reserve(buffersToPatch.size());
    if (AtomicResolveMode::SeparateBuffer == mode)
    {
      for (auto &&pair : buffersToPatch)
      {
        // just need to copy over binding point and put the counter into the counter list
        auto counterBinding = find_property<PropertyBinding>(pair.counter);
        auto bufferBinding = find_property<PropertyBinding>(pair.buffer);
        counterBinding->bindingPoint = bufferBinding->bindingPoint;

        buffersWithCounterList.push_back(pair.counter);
      }
    }
    else if (AtomicResolveMode::Embed == mode)
    {
      for (auto &&pair : buffersToPatch)
      {
        embed_counter_buffer(builder, pair.buffer, pair.counter);
        buffersWithCounterList.push_back(pair.buffer);
      }
    }
  }

  return buffersWithCounterList;
}