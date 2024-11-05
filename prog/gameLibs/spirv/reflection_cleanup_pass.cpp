// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "module_nodes.h"
#include <spirv/module_builder.h>

using namespace spirv;

namespace
{

// error handler is daisy chain routed for emergency debugging
template <typename T>
void removeProperty(NodePointer<NodeId> &node, ErrorHandler &)
{
  eastl::vector<int> toRemove;
  for (int i = 0; i < node->properties.size(); i++)
  {
    if (is<T>(node->properties[i]))
      toRemove.push_back(i);
  }

  if (!toRemove.size())
    return;

  for (int i = toRemove.size() - 1; i >= 0; i--)
    node->properties.erase(node->properties.begin() + toRemove[i]);
}

void processNode(NodePointer<NodeId> &node, ErrorHandler &e_handler)
{
  removeProperty<PropertyUserTypeGOOGLE>(node, e_handler);
  removeProperty<PropertyHlslSemanticGOOGLE>(node, e_handler);
  removeProperty<PropertyHlslCounterBufferGOOGLE>(node, e_handler);
}

} // namespace

// purpose of this pass is to remove any trace of
//  SPV_GOOGLE_hlsl_functionality1
//  SPV_GOOGLE_user_type
//  SPV_GOOGLE_decorate_string
//    which is
//      OpDecorateStringGOOGLE
//      OpMemberDecorateStringGOOGLE
//      HlslSemanticGOOGLE
//      HlslCounterBufferGOOGLE
//      UserTypeGOOGLE

void spirv::cleanupReflectionLeftouts(ModuleBuilder &builder, ErrorHandler &e_handler)
{
  builder.enumerateAllInstructionsWithAnyProperty([=, &e_handler](NodePointer<NodeId> node) //
    { processNode(node, e_handler); });
}
