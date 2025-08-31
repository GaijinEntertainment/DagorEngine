// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "module_nodes.h"
#include <spirv/module_builder.h>

using namespace spirv;

// dxc has some minor bugs right now that can be fixed with some pre passes
void spirv::fixDXCBugs(ModuleBuilder &builder, ErrorHandler &e_handler)
{
  // put in here stuff to fix some problems with DXC

  // push constant annotation and bind globals gives binding for push constant, while it should not
  // remove this element from $Globals or workaround anyhow,
  // so offset of push constant is not breaking global const buffer layout

  // we may add 1 node (int type decl) inside enumeration, reserve to avoid resize issues
  builder.reserveMemoryForNodes(1);

  builder.enumerateAllInstructionsWithAnyProperty([&builder, &e_handler](NodePointer<NodeId> node) //
    {
      if (!is<NodeOpTypeStruct>(node))
        return;

      bool globStruct = false;
      bool immDwords = false;
      Id immDwordMember = -1;
      for (auto &&prop : node->properties)
      {
        if (is<PropertyName>(prop))
        {
          auto nameProp = as<PropertyName>(prop);
          if (!nameProp->memberIndex.has_value() && nameProp->name == "type.$Globals")
            globStruct = true;
          if (nameProp->memberIndex.has_value() && nameProp->name == "imm_dwords")
          {
            immDwords = true;
            immDwordMember = nameProp->memberIndex.value();
          }

          if (globStruct && immDwords)
            break;
        }
      }

      if (!globStruct || !immDwords)
        return;

      auto globStructNode = as<NodeOpTypeStruct>(node);
      if (globStructNode->param1.empty())
      {
        e_handler.onFatalError("Global struct node is empty, yet fields was declared");
        return;
      }

      auto targetNode = globStructNode->param1[immDwordMember];
      if (!is<NodeOpTypeStruct>(targetNode))
      {
        e_handler.onFatalError("Imm dword field is not struct, while it should be struct");
        return;
      }

      auto targetNodeStruct = as<NodeOpTypeStruct>(targetNode);
      bool isTargetNodeValid = false;
      for (auto &&prop : targetNodeStruct->properties)
      {
        if (is<PropertyName>(prop))
        {
          auto nameProp = as<PropertyName>(prop);
          if (!nameProp->memberIndex.has_value() && nameProp->name == "ImmDwords")
          {
            isTargetNodeValid = true;
            break;
          }
        }
      }
      if (!isTargetNodeValid)
      {
        e_handler.onFatalError("Field that expected to be ImmDwords, was not it, verify patching code");
        return;
      }

      // parameter is last in struct, just cut it out
      if (immDwordMember == (globStructNode->param1.size() - 1))
      {
        globStructNode->param1.pop_back();

        eastl::vector<int> toRemove;
        for (auto &&prop : node->properties)
        {
          if (is<PropertyName>(prop))
          {
            auto nameProp = as<PropertyName>(prop);
            if (nameProp->memberIndex.has_value() && nameProp->memberIndex == immDwordMember)
              toRemove.push_back(&prop - node->properties.begin());
          }
          else if (is<PropertyOffset>(prop))
          {
            auto offsetProp = as<PropertyOffset>(prop);
            if (offsetProp->memberIndex.has_value() && offsetProp->memberIndex == immDwordMember)
              toRemove.push_back(&prop - node->properties.begin());
          }
        }

        for (int i = toRemove.size() - 1; i >= 0; i--)
          node->properties.erase(node->properties.begin() + toRemove[i]);
      }
      // otherwise replace with fake int
      else
      {
        NodePointer<NodeOpTypeInt> intTypeNode;

        builder.enumerateAllTypes([&intTypeNode](auto type) //
          {
            if (!intTypeNode)
            {
              if (is<NodeOpTypeInt>(type))
              {
                auto ptr = as<NodeOpTypeInt>(type);
                if (ptr->width.value == 32 && ptr->signedness.value == 0)
                {
                  intTypeNode = ptr;
                }
              }
            }
          });

        if (!intTypeNode)
          intTypeNode = builder.newNode<NodeOpTypeInt>(0, 32, 0);

        globStructNode->param1[immDwordMember] = intTypeNode;
        re_index(builder);
      }
    });
}