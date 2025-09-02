// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/detail/dispatchRequestBase.h>
#include <frontend/internalRegistry.h>

namespace dafg::detail
{

template <DispatchRequestBase::ArgType arg_type, typename ArgT>
static void set_arg(dafg::NodeData &node, const ArgT &value)
{
  auto dispatchRequirements = eastl::get_if<DispatchRequirements>(&node.executeRequirements);
  G_ASSERT_RETURN(dispatchRequirements, );

  eastl::visit(
    [&](auto &args) {
      using T = eastl::remove_cvref_t<decltype(args)>;

      static constexpr bool hasDirectArgs = eastl::is_base_of_v<DispatchRequirements::DirectArgs, T>;
      static constexpr bool hasIndirectArgs = eastl::is_base_of_v<DispatchRequirements::IndirectArgs, T>;
      static constexpr bool hasMeshIndirectArgs = eastl::is_base_of_v<DispatchRequirements::MeshIndirectArgs, T>;
      static constexpr bool hasMeshIndirectCountArgs = eastl::is_base_of_v<DispatchRequirements::MeshIndirectCountArgs, T>;

      if constexpr (arg_type == DispatchRequestBase::ArgType::X && hasDirectArgs)
        args.x = value;
      else if constexpr (arg_type == DispatchRequestBase::ArgType::Y && hasDirectArgs)
        args.y = value;
      else if constexpr (arg_type == DispatchRequestBase::ArgType::Z && hasDirectArgs)
        args.z = value;
      else if constexpr (arg_type == DispatchRequestBase::ArgType::Offset && hasIndirectArgs)
        args.offset = value;
      else if constexpr (arg_type == DispatchRequestBase::ArgType::Stride && hasMeshIndirectArgs)
        args.stride = value;
      else if constexpr (arg_type == DispatchRequestBase::ArgType::Count && hasMeshIndirectArgs)
        args.count = value;
      else if constexpr (arg_type == DispatchRequestBase::ArgType::CountOffset && hasMeshIndirectCountArgs)
        args.countOffset = value;
      else if constexpr (arg_type == DispatchRequestBase::ArgType::MaxCount && hasMeshIndirectCountArgs)
        args.maxCount = value;
      else
        static_assert(true, "Invalid argument type");
    },
    dispatchRequirements->arguments);
}

template <typename ArgsT>
static ArgsT &init_dispatch_requirements(dafg::NodeData &node, ShaderNameId shaderId, const char *node_name)
{
  if (DAGOR_UNLIKELY(node.executeRequirements.index() != 0))
    logerr("daFG: Execute requirements already set for node %s.", node_name);

  node.executeRequirements = DispatchRequirements{shaderId, ArgsT{}};
  return eastl::get<ArgsT>(eastl::get<DispatchRequirements>(node.executeRequirements).arguments);
}

void DispatchRequestBase::threads()
{
  init_dispatch_requirements<DispatchRequirements::Threads>(registry->nodes[nodeId], shaderId, registry->knownNames.getName(nodeId));
}

void DispatchRequestBase::groups()
{
  init_dispatch_requirements<DispatchRequirements::Groups>(registry->nodes[nodeId], shaderId, registry->knownNames.getName(nodeId));
}

void DispatchRequestBase::indirect(const char *buffer)
{
  auto &node = registry->nodes[nodeId];
  auto &args = init_dispatch_requirements<DispatchRequirements::IndirectArgs>(node, shaderId, registry->knownNames.getName(nodeId));

  const auto nodeNsId = registry->knownNames.getParent(nodeId);
  const auto resNameId = registry->knownNames.addNameId<ResNameId>(nodeNsId, buffer);

  node.readResources.insert(resNameId);
  node.resourceRequests.emplace(resNameId, ResourceRequest{ResourceUsage{Usage::INDIRECTION_BUFFER, Access::READ_ONLY}});

  args.res = resNameId;
}

void DispatchRequestBase::meshThreads()
{
  init_dispatch_requirements<DispatchRequirements::MeshThreads>(registry->nodes[nodeId], shaderId,
    registry->knownNames.getName(nodeId));
}

void DispatchRequestBase::meshGroups()
{
  init_dispatch_requirements<DispatchRequirements::MeshGroups>(registry->nodes[nodeId], shaderId,
    registry->knownNames.getName(nodeId));
}

void DispatchRequestBase::meshIndirect(const char *buffer)
{
  indirect(buffer);

  G_ASSERT_RETURN(eastl::holds_alternative<DispatchRequirements>(registry->nodes[nodeId].executeRequirements), );
  auto &requirements = eastl::get<DispatchRequirements>(registry->nodes[nodeId].executeRequirements);

  G_ASSERT_RETURN(eastl::holds_alternative<DispatchRequirements::IndirectArgs>(requirements.arguments), );
  auto &args = eastl::get<DispatchRequirements::IndirectArgs>(requirements.arguments);

  requirements.arguments = DispatchRequirements::MeshIndirectArgs{args, 0};
}

void DispatchRequestBase::meshIndirectCount(const char *buffer)
{
  auto &node = registry->nodes[nodeId];
  G_ASSERT_RETURN(eastl::holds_alternative<DispatchRequirements>(node.executeRequirements), );
  auto &requirements = eastl::get<DispatchRequirements>(node.executeRequirements);

  G_ASSERT_RETURN(eastl::holds_alternative<DispatchRequirements::MeshIndirectArgs>(requirements.arguments), );
  auto &args = eastl::get<DispatchRequirements::MeshIndirectArgs>(requirements.arguments);

  const auto nodeNsId = registry->knownNames.getParent(nodeId);
  const auto resNameId = registry->knownNames.addNameId<ResNameId>(nodeNsId, buffer);

  node.readResources.insert(resNameId);
  node.resourceRequests.emplace(resNameId, ResourceRequest{ResourceUsage{Usage::INDIRECTION_BUFFER, Access::READ_ONLY}});

  requirements.arguments = DispatchRequirements::MeshIndirectCountArgs{args, resNameId, 0, 0};
}

template <DispatchRequestBase::ArgType arg_type>
void DispatchRequestBase::setArg(uint32_t value)
{
  set_arg<arg_type>(registry->nodes[nodeId], value);
}

template void DispatchRequestBase::setArg<DispatchRequestBase::ArgType::X>(uint32_t);
template void DispatchRequestBase::setArg<DispatchRequestBase::ArgType::Y>(uint32_t);
template void DispatchRequestBase::setArg<DispatchRequestBase::ArgType::Z>(uint32_t);
template void DispatchRequestBase::setArg<DispatchRequestBase::ArgType::Offset>(uint32_t);
template void DispatchRequestBase::setArg<DispatchRequestBase::ArgType::Stride>(uint32_t);
template void DispatchRequestBase::setArg<DispatchRequestBase::ArgType::Count>(uint32_t);
template void DispatchRequestBase::setArg<DispatchRequestBase::ArgType::CountOffset>(uint32_t);
template void DispatchRequestBase::setArg<DispatchRequestBase::ArgType::MaxCount>(uint32_t);

template <DispatchRequestBase::ArgType arg_type>
void DispatchRequestBase::setBlobArg(VirtualResourceRequestBase request, TypeErasedProjector projector)
{
  auto &node = registry->nodes[nodeId];

  const auto resNameId = request.resUid.resId;
  G_ASSERT_RETURN(node.resourceRequests.find(resNameId) != node.resourceRequests.end(), );

  set_arg<arg_type>(node, BlobArg{resNameId, projector});
}

template void DispatchRequestBase::setBlobArg<DispatchRequestBase::ArgType::X>(VirtualResourceRequestBase, TypeErasedProjector);
template void DispatchRequestBase::setBlobArg<DispatchRequestBase::ArgType::Y>(VirtualResourceRequestBase, TypeErasedProjector);
template void DispatchRequestBase::setBlobArg<DispatchRequestBase::ArgType::Z>(VirtualResourceRequestBase, TypeErasedProjector);
template void DispatchRequestBase::setBlobArg<DispatchRequestBase::ArgType::Offset>(VirtualResourceRequestBase, TypeErasedProjector);
template void DispatchRequestBase::setBlobArg<DispatchRequestBase::ArgType::Stride>(VirtualResourceRequestBase, TypeErasedProjector);
template void DispatchRequestBase::setBlobArg<DispatchRequestBase::ArgType::Count>(VirtualResourceRequestBase, TypeErasedProjector);
template void DispatchRequestBase::setBlobArg<DispatchRequestBase::ArgType::CountOffset>(VirtualResourceRequestBase,
  TypeErasedProjector);
template void DispatchRequestBase::setBlobArg<DispatchRequestBase::ArgType::MaxCount>(VirtualResourceRequestBase, TypeErasedProjector);

template <DispatchRequestBase::ArgType arg_type>
void DispatchRequestBase::setAutoResolutionArg(AutoResTypeNameId resolution, float multiplier, TypeErasedProjector projector)
{
  set_arg<arg_type>(registry->nodes[nodeId], DispatchAutoResolutionArg{resolution, multiplier, projector});
}

template void DispatchRequestBase::setAutoResolutionArg<DispatchRequestBase::ArgType::X>(AutoResTypeNameId, float,
  TypeErasedProjector);
template void DispatchRequestBase::setAutoResolutionArg<DispatchRequestBase::ArgType::Y>(AutoResTypeNameId, float,
  TypeErasedProjector);
template void DispatchRequestBase::setAutoResolutionArg<DispatchRequestBase::ArgType::Z>(AutoResTypeNameId, float,
  TypeErasedProjector);

template <DispatchRequestBase::ArgType arg_type>
void DispatchRequestBase::setTextureResolutionArg(VirtualResourceRequestBase request, TypeErasedProjector projector)
{
  auto &node = registry->nodes[nodeId];

  const auto resNameId = request.resUid.resId;
  G_ASSERT_RETURN(node.resourceRequests.find(resNameId) != node.resourceRequests.end(), );

  set_arg<arg_type>(node, DispatchTextureResolutionArg{resNameId, projector});
}

template void DispatchRequestBase::setTextureResolutionArg<DispatchRequestBase::ArgType::X>(VirtualResourceRequestBase,
  TypeErasedProjector);
template void DispatchRequestBase::setTextureResolutionArg<DispatchRequestBase::ArgType::Y>(VirtualResourceRequestBase,
  TypeErasedProjector);
template void DispatchRequestBase::setTextureResolutionArg<DispatchRequestBase::ArgType::Z>(VirtualResourceRequestBase,
  TypeErasedProjector);

} // namespace dafg::detail
