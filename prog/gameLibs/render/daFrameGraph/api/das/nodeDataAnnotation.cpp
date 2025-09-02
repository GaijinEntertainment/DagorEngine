// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <api/das/frameGraphModule.h>
#include <api/das/bindingHelper.h>


struct NodeStateRequirementsAnnotation final : das::ManagedStructureAnnotation<dafg::NodeStateRequirements>
{
  NodeStateRequirementsAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("NodeStateRequirements", ml, "dafg::NodeStateRequirements")
  {
    addField<DAS_BIND_MANAGED_FIELD(supportsWireframe)>("supportsWireframe");
    addField<DAS_BIND_MANAGED_FIELD(vrsState)>("vrsState");
    addField<DAS_BIND_MANAGED_FIELD(pipelineStateOverride)>("pipelineStateOverride");
  }
};

struct VirtualPassRequirementsAnnotation final : das::ManagedStructureAnnotation<dafg::VirtualPassRequirements>
{
  VirtualPassRequirementsAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("VirtualPassRequirements", ml, "dafg::VirtualPassRequirements")
  {
    addField<DAS_BIND_MANAGED_FIELD(colorAttachments)>("colorAttachments");
    addField<DAS_BIND_MANAGED_FIELD(depthAttachment)>("depthAttachment");
    addField<DAS_BIND_MANAGED_FIELD(depthReadOnly)>("depthReadOnly");
    addField<DAS_BIND_MANAGED_FIELD(vrsRateAttachment)>("vrsRateAttachment");
  }
};

struct NodeDataAnnotation final : das::ManagedStructureAnnotation<dafg::NodeData>
{
  NodeDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NodeData", ml, "dafg::NodeData")
  {
    addField<DAS_BIND_MANAGED_FIELD(followingNodeIds)>("followingNodeIds");
    addField<DAS_BIND_MANAGED_FIELD(precedingNodeIds)>("precedingNodeIds");
    addField<DAS_BIND_MANAGED_FIELD(priority)>("priority");
    addField<DAS_BIND_MANAGED_FIELD(multiplexingMode)>("multiplexingMode");
    addField<DAS_BIND_MANAGED_FIELD(sideEffect)>("sideEffect");
    addField<DAS_BIND_MANAGED_FIELD(createdResources)>("createdResources");
    addField<DAS_BIND_MANAGED_FIELD(resourceRequests)>("resourceRequests");
    addField<DAS_BIND_MANAGED_FIELD(readResources)>("readResources");
    addField<DAS_BIND_MANAGED_FIELD(renamedResources)>("renamedResources");
    addField<DAS_BIND_MANAGED_FIELD(historyResourceReadRequests)>("historyResourceReadRequests");
    addField<DAS_BIND_MANAGED_FIELD(modifiedResources)>("modifiedResources");
    addField<DAS_BIND_MANAGED_FIELD(generation)>("generation");
    addField<DAS_BIND_MANAGED_FIELD(nodeSource)>("nodeSource");
    addField<DAS_BIND_MANAGED_FIELD(shaderBlockLayers)>("shaderBlockLayers");
    addField<DAS_BIND_MANAGED_FIELD(stateRequirements)>("stateRequirements");
    addField<DAS_BIND_MANAGED_FIELD(renderingRequirements)>("renderingRequirements");
    addField<DAS_BIND_MANAGED_FIELD(bindings)>("bindings");
  }
};

namespace bind_dascript
{

void setNodeSource(dafg::NodeData &node, const char *source) { node.nodeSource = source; }

void DaFgCoreModule::addNodeDataAnnotation(das::ModuleLibrary &lib)
{
  addAnnotation(das::make_smart<NodeStateRequirementsAnnotation>(lib));
  addAnnotation(das::make_smart<VirtualPassRequirementsAnnotation>(lib));
  addAnnotation(das::make_smart<NodeDataAnnotation>(lib));

  auto bindingMapType = das::makeType<dafg::BindingsMap>(lib);
  bindingMapType->alias = "BindingsMap";
  addAlias(bindingMapType);

  BIND_FUNCTION(bind_dascript::setNodeSource, "setNodeSource", das::SideEffects::modifyArgumentAndExternal);
}

} // namespace bind_dascript
