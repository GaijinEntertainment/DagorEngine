#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include "module_nodes.h"
#include <spirv/module_builder.h>

using namespace spirv;
using namespace eastl;

struct PropertyCloneVisitor
{
  template <typename T>
  CastableUniquePointer<Property> operator()(const T *p)
  {
    return p->clone();
  }
};

vector<CastableUniquePointer<Property>> duplicate_properties_with_new_name(vector<CastableUniquePointer<Property>> &properties,
  int32_t newIndex)
{
  vector<CastableUniquePointer<Property>> result;
  for (auto &prop : properties)
  {
    auto newProp = visitProperty(prop.get(), PropertyCloneVisitor{});
    if (is<PropertyName>(newProp))
      as<PropertyName>(newProp)->name += to_string(newIndex);
    result.emplace_back(move(newProp));
  }
  return result;
};

struct BufferPtrRefs
{
  NodeOpTypePointer *ptrType;
  vector<NodePointer<NodeOpVariable>> vars;
};

void add_buffer_var_ref(vector<BufferPtrRefs> &bufferPtrRefs, NodeOpTypePointer *ptrType, NodePointer<NodeOpVariable> var)
{
  auto found = find_if(bufferPtrRefs.begin(), bufferPtrRefs.end(), [ptrType](const BufferPtrRefs &e) { return ptrType == e.ptrType; });
  BufferPtrRefs &bufferRefs = (found == bufferPtrRefs.end()) ? bufferPtrRefs.emplace_back(BufferPtrRefs{ptrType, {}}) : *found;
  bufferRefs.vars.emplace_back(var);
}

void spirv::make_buffer_pointer_type_unique_per_buffer(ModuleBuilder &builder)
{
  vector<BufferPtrRefs> bufferPtrRefs;
  // enumerate all global variables to find all buffer blocks and collect them in the `bufferPtrRefs` with their pointers.
  // actually, `bufferPtrRefs` is logically a map<PTR, vector<VAR>>, but we use a vector to get the reproducible output.
  builder.enumerateAllGlobalVariables([&builder, &bufferPtrRefs](NodePointer<NodeOpVariable> var) {
    if (var->storageClass == StorageClass::Uniform)
    {
      auto ptrType = as<NodeOpTypePointer>(var->resultType);
      auto valueType = as<NodeTypedef>(ptrType->type);

      for (const auto &prop : valueType->properties)
      {
        if (is<PropertyBufferBlock>(prop))
        {
          // add buffer to container
          add_buffer_var_ref(bufferPtrRefs, ptrType.get(), var);
          break;
        }
      }
    }
  });

  // process collected buffer pointers
  for (auto &ptrRefs : bufferPtrRefs)
  {
    // skip buffers that already has unique pointer types
    if (ptrRefs.vars.size() < 2)
      continue;

    // process only struct pointer
    if (!is<NodeOpTypeStruct>(ptrRefs.ptrType->type))
      continue;

    auto typeStruct = as<NodeOpTypeStruct>(ptrRefs.ptrType->type);

    auto it = ptrRefs.vars.begin() + 1u;
    int32_t newIndex = 1;
    while (it != ptrRefs.vars.end())
    {
      // declare a new struct type with the same properties, except `PropertyName`. we change it by adding index at the end.
      NodePointer<NodeOpTypeStruct> newType =
        builder.newNode<NodeOpTypeStruct>(typeStruct->resultId, typeStruct->param1.data(), typeStruct->param1.size());
      newType->properties = duplicate_properties_with_new_name(typeStruct->properties, newIndex);

      // declare a new pointer type with the same properties, plus changed `PropertyName`.
      NodePointer<NodeOpTypePointer> newPtr =
        builder.newNode<NodeOpTypePointer>(ptrRefs.ptrType->resultId, ptrRefs.ptrType->storageClass, as<NodeId>(newType));
      newPtr->properties = duplicate_properties_with_new_name(ptrRefs.ptrType->properties, newIndex);

      // replace pointer type in buffer with a new one
      (*it)->resultType = newPtr;
      ++it;
      ++newIndex;
    }
  }

  re_index(builder);
}