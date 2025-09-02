// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/detail/drawRequestBase.h>
#include <frontend/internalRegistry.h>

namespace dafg::detail
{

void DrawRequestBase::draw(DrawPrimitive primitive)
{
  auto &node = registry->nodes[nodeId];

  if (DAGOR_UNLIKELY(!eastl::holds_alternative<DrawRequirements>(node.executeRequirements)))
    logerr("daFG: Node %s has already specified execute requirements", registry->knownNames.getName(nodeId));

  node.executeRequirements = DrawRequirements{shaderId, DrawRequirements::DirectNonIndexedArgs{primitive}};
}

void DrawRequestBase::drawIndexed(DrawPrimitive primitive)
{
  auto &node = registry->nodes[nodeId];

  if (DAGOR_UNLIKELY(!eastl::holds_alternative<DrawRequirements>(node.executeRequirements)))
    logerr("daFG: Node %s has already specified execute requirements", registry->knownNames.getName(nodeId));

  node.executeRequirements = DrawRequirements{shaderId, DrawRequirements::DirectIndexedArgs{primitive}};
}

void DrawRequestBase::indirect(const char *buffer)
{
  auto &node = registry->nodes[nodeId];

  auto drawRequirements = eastl::get_if<DrawRequirements>(&node.executeRequirements);
  G_ASSERT_RETURN(drawRequirements, );

  const auto nodeNsId = registry->knownNames.getParent(nodeId);
  const auto resNameId = registry->knownNames.addNameId<ResNameId>(nodeNsId, buffer);

  registry->nodes[nodeId].readResources.insert(resNameId);
  registry->nodes[nodeId].resourceRequests.emplace(resNameId,
    ResourceRequest{ResourceUsage{Usage::INDIRECTION_BUFFER, Access::READ_ONLY}});

  eastl::visit(
    [&](auto &args) {
      using T = eastl::remove_cvref_t<decltype(args)>;
      if constexpr (eastl::is_same_v<DrawRequirements::DirectNonIndexedArgs, T>)
        drawRequirements->arguments =
          DrawRequirements::IndirectNonIndexedArgs{DrawRequirements::IndirectArgs{args.primitive, resNameId}};
      else if constexpr (eastl::is_same_v<DrawRequirements::DirectIndexedArgs, T>)
        drawRequirements->arguments = DrawRequirements::IndirectIndexedArgs{DrawRequirements::IndirectArgs{args.primitive, resNameId}};
    },
    drawRequirements->arguments);
}

template <DrawRequestBase::ArgType arg_type>
static DrawRequirements *update_draw_requirements(dafg::NodeData &node)
{
  auto drawRequirements = eastl::get_if<DrawRequirements>(&node.executeRequirements);
  if (DAGOR_UNLIKELY(!drawRequirements))
    return nullptr;

  // Upgrade indirect to multiIndirect if needed.
  eastl::visit(
    [&](auto &args) {
      using T = eastl::remove_cvref_t<decltype(args)>;
      static constexpr bool isMultiDrawArg =
        arg_type == DrawRequestBase::ArgType::Stride || arg_type == DrawRequestBase::ArgType::Count;
      if constexpr (isMultiDrawArg && eastl::is_same_v<DrawRequirements::IndirectNonIndexedArgs, T>)
        drawRequirements->arguments = DrawRequirements::MultiIndirectNonIndexedArgs{args};
      else if constexpr (!isMultiDrawArg && eastl::is_same_v<DrawRequirements::IndirectIndexedArgs, T>)
        drawRequirements->arguments = DrawRequirements::MultiIndirectIndexedArgs{args};
    },
    drawRequirements->arguments);

  return drawRequirements;
}

template <DrawRequestBase::ArgType arg_type, typename ArgT>
static void set_arg(dafg::NodeData &node, const ArgT &value)
{
  auto drawRequirements = update_draw_requirements<arg_type>(node);
  G_ASSERT_RETURN(drawRequirements, );

  eastl::visit(
    [&](auto &args) {
      using T = eastl::remove_cvref_t<decltype(args)>;
      static constexpr bool hasDirectArgs = eastl::is_base_of_v<DrawRequirements::DirectArgs, T>;
      static constexpr bool hasDirectNonIndexedArgs = eastl::is_same_v<DrawRequirements::DirectNonIndexedArgs, T>;
      static constexpr bool hasDirectIndexedArgs = eastl::is_same_v<DrawRequirements::DirectIndexedArgs, T>;
      static constexpr bool hasIndirectArgs = eastl::is_base_of_v<DrawRequirements::IndirectArgs, T>;
      static constexpr bool hasMulltiIndirectArgs = eastl::is_base_of_v<DrawRequirements::MultiIndirectArgs, T>;

      if constexpr (arg_type == DrawRequestBase::ArgType::BaseVertex && hasDirectIndexedArgs)
        args.baseVertex = value;
      else if constexpr (arg_type == DrawRequestBase::ArgType::StartVertex && hasDirectNonIndexedArgs)
        args.startVertex = value;
      else if constexpr (arg_type == DrawRequestBase::ArgType::StartIndex && hasDirectIndexedArgs)
        args.startIndex = value;
      else if constexpr (arg_type == DrawRequestBase::ArgType::PrimitiveCount && hasDirectArgs)
        args.primitiveCount = value;
      else if constexpr (arg_type == DrawRequestBase::ArgType::StartInstance && hasDirectArgs)
        args.startInstance = value;
      else if constexpr (arg_type == DrawRequestBase::ArgType::InstanceCount && hasDirectArgs)
        args.instanceCount = value;
      else if constexpr (arg_type == DrawRequestBase::ArgType::Offset && hasIndirectArgs)
        args.offset = value;
      else if constexpr (arg_type == DrawRequestBase::ArgType::Stride && hasMulltiIndirectArgs)
        args.stride = value;
      else if constexpr (arg_type == DrawRequestBase::ArgType::Count && hasMulltiIndirectArgs)
        args.drawCount = value;
    },
    drawRequirements->arguments);
}

template <DrawRequestBase::ArgType arg_type>
void DrawRequestBase::setArg(uint32_t value)
{
  set_arg<arg_type>(registry->nodes[nodeId], value);
}

template void DrawRequestBase::setArg<DrawRequestBase::ArgType::BaseVertex>(uint32_t);
template void DrawRequestBase::setArg<DrawRequestBase::ArgType::StartVertex>(uint32_t);
template void DrawRequestBase::setArg<DrawRequestBase::ArgType::StartIndex>(uint32_t);
template void DrawRequestBase::setArg<DrawRequestBase::ArgType::StartInstance>(uint32_t);
template void DrawRequestBase::setArg<DrawRequestBase::ArgType::InstanceCount>(uint32_t);
template void DrawRequestBase::setArg<DrawRequestBase::ArgType::PrimitiveCount>(uint32_t);
template void DrawRequestBase::setArg<DrawRequestBase::ArgType::Offset>(uint32_t);
template void DrawRequestBase::setArg<DrawRequestBase::ArgType::Stride>(uint32_t);
template void DrawRequestBase::setArg<DrawRequestBase::ArgType::Count>(uint32_t);

template <DrawRequestBase::ArgType arg_type>
void DrawRequestBase::setBlobArg(VirtualResourceRequestBase request, TypeErasedProjector projector)
{
  auto &node = registry->nodes[nodeId];

  const auto resNameId = request.resUid.resId;
  G_ASSERT_RETURN(node.resourceRequests.find(resNameId) != node.resourceRequests.end(), );

  set_arg<arg_type>(node, BlobArg{resNameId, projector});
}

template void DrawRequestBase::setBlobArg<DrawRequestBase::ArgType::BaseVertex>(VirtualResourceRequestBase, TypeErasedProjector);
template void DrawRequestBase::setBlobArg<DrawRequestBase::ArgType::StartVertex>(VirtualResourceRequestBase, TypeErasedProjector);
template void DrawRequestBase::setBlobArg<DrawRequestBase::ArgType::StartIndex>(VirtualResourceRequestBase, TypeErasedProjector);
template void DrawRequestBase::setBlobArg<DrawRequestBase::ArgType::StartInstance>(VirtualResourceRequestBase, TypeErasedProjector);
template void DrawRequestBase::setBlobArg<DrawRequestBase::ArgType::InstanceCount>(VirtualResourceRequestBase, TypeErasedProjector);
template void DrawRequestBase::setBlobArg<DrawRequestBase::ArgType::PrimitiveCount>(VirtualResourceRequestBase, TypeErasedProjector);
template void DrawRequestBase::setBlobArg<DrawRequestBase::ArgType::Offset>(VirtualResourceRequestBase, TypeErasedProjector);
template void DrawRequestBase::setBlobArg<DrawRequestBase::ArgType::Stride>(VirtualResourceRequestBase, TypeErasedProjector);
template void DrawRequestBase::setBlobArg<DrawRequestBase::ArgType::Count>(VirtualResourceRequestBase, TypeErasedProjector);

} // namespace dafg::detail
