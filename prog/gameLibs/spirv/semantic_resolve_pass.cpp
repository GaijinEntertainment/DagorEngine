// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "module_nodes.h"
#include <spirv/module_builder.h>

using namespace spirv;

namespace
{
void patchSemantic(NodePointer<NodeOpVariable> var, const SemanticInfo *input_semantics, unsigned input_smentics_count,
  const SemanticInfo *output_smenaitcs, unsigned output_smenaitcs_count, ErrorHandler &e_handler)
{
  bool isOut = var->storageClass == StorageClass::Output;
  bool isIn = var->storageClass == StorageClass::Input;
  // if not in or out, return ok and do nothing
  if (!isOut && !isIn)
    return;

  // interface vars are always pointers
  auto ptrType = as<NodeOpTypePointer>(var->resultType);
  auto varType = ptrType->type;

  if (is<NodeOpTypeStruct>(varType))
  {
    // should never be reached, DXC flattens in/out structs
    e_handler.onFatalError("resolveSemantics: in/out variable was a struct, not supported yet!");
    return;
  }

  auto src = isOut ? output_smenaitcs : input_semantics;
  auto srcCount = isOut ? output_smenaitcs_count : input_smentics_count;

  PropertyLocation *locationProperty = nullptr;
  PropertyHlslSemanticGOOGLE *semanticProperty = nullptr;
  PropertyBuiltIn *builtInProperty = nullptr;
  for (auto &&prop : var->properties)
  {
    if (is<PropertyLocation>(prop))
      locationProperty = as<PropertyLocation>(prop);
    else if (is<PropertyHlslSemanticGOOGLE>(prop))
      semanticProperty = as<PropertyHlslSemanticGOOGLE>(prop);
    else if (is<PropertyBuiltIn>(prop))
      builtInProperty = as<PropertyBuiltIn>(prop);
  }

  if (!semanticProperty)
  {
    e_handler.onWarning("resolveSemantics: in/out variable has no semantic property!");
    return;
  }

  if (!builtInProperty)
  {
    if (srcCount > 0)
    {
      PropertyLocation *newLocation = locationProperty;
      if (!newLocation)
      {
        newLocation = new PropertyLocation;
        var->properties.emplace_back(newLocation);
        newLocation->location = 0;
      }

      auto mapRef = eastl::find_if(src, src + srcCount, [=](const auto &ent) { return ent.name == semanticProperty->semantic; });
      if (mapRef == src + srcCount)
      {
        eastl::string logBuf("resolveSemantics: no matching semantic found for ");
        logBuf += semanticProperty->semantic;
        e_handler.onWarning(logBuf.c_str());
        return;
      }

      newLocation->location = mapRef->location;
      newLocation->memberIndex = semanticProperty->memberIndex;
    }
  }
  else if (locationProperty)
  {
    e_handler.onWarning("resolveSemantics: variable has builtIn property and location property!");
    // remove location property
    var->properties.erase(eastl::find_if(begin(var->properties), end(var->properties),
      [=](const auto &prop) //
      { return (const void *)prop.get() == (const void *)locationProperty; }));
  }

  // remove semantic property
  var->properties.erase(eastl::find_if(begin(var->properties), end(var->properties),
    [=](const auto &prop) { return as<PropertyHlslSemanticGOOGLE>(prop) == semanticProperty; }));
}
} // namespace

// A very simple pass
// Loop over all globals, if they are either input or output then check if they have a
// PropertyHlslSemanticGOOGLE property If so look it up, add new location property with its location
// and remove the old PropertyHlslSemanticGOOGLE from the variable
// TODO: can be optimized to loop over interface variables of each entry point
void spirv::resolveSemantics(ModuleBuilder &builder, const SemanticInfo *input_semantics, unsigned input_smentics_count,
  const SemanticInfo *output_smenaitcs, unsigned output_smenaitcs_count, ErrorHandler &e_handler)
{
  builder.enumerateAllGlobalVariables([=, &e_handler](auto var) //
    { patchSemantic(var, input_semantics, input_smentics_count, output_smenaitcs, output_smenaitcs_count, e_handler); });
}