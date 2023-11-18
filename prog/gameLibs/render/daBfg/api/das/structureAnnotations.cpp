#include <api/das/frameGraphModule.h>
#include <api/das/bindingHelper.h>

struct TextureResourceAnnotation final : das::ManagedStructureAnnotation<TextureResourceDescription>
{
  TextureResourceAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("TextureResourceDescription", ml, "TextureResourceDescription")
  {
    addField<DAS_BIND_MANAGED_FIELD(mipLevels)>("mipLevels");
    addField<DAS_BIND_MANAGED_FIELD(cFlags)>("cFlags");
    addField<DAS_BIND_MANAGED_FIELD(activation)>("activation");
    addField<DAS_BIND_MANAGED_FIELD(width)>("width");
    addField<DAS_BIND_MANAGED_FIELD(height)>("height");
  }
};

struct BufferResourceDescriptionAnnotation final : das::ManagedStructureAnnotation<BufferResourceDescription>
{
  BufferResourceDescriptionAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("BufferResourceDescription", ml, "BufferResourceDescription")
  {
    addField<DAS_BIND_MANAGED_FIELD(elementCount)>("elementCount");
    addField<DAS_BIND_MANAGED_FIELD(elementSizeInBytes)>("elementSizeInBytes");
    addField<DAS_BIND_MANAGED_FIELD(viewFormat)>("viewFormat");
    addField<DAS_BIND_MANAGED_FIELD(cFlags)>("cFlags");
    addField<DAS_BIND_MANAGED_FIELD(activation)>("activation");
  }
};

struct VolTextureResourceAnnotation final : das::ManagedStructureAnnotation<VolTextureResourceDescription>
{
  VolTextureResourceAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("VolTextureResourceDescription", ml, "VolTextureResourceDescription")
  {}
};

struct ArrayTextureResourceAnnotation final : das::ManagedStructureAnnotation<ArrayTextureResourceDescription>
{
  ArrayTextureResourceAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("ArrayTextureResourceDescription", ml, "ArrayTextureResourceDescription")
  {}
};

struct CubeTextureResourceAnnotation final : das::ManagedStructureAnnotation<CubeTextureResourceDescription>
{
  CubeTextureResourceAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("CubeTextureResourceDescription", ml, "CubeTextureResourceDescription")
  {}
};

struct ArrayCubeTextureResourceAnnotation final : das::ManagedStructureAnnotation<ArrayCubeTextureResourceDescription>
{
  ArrayCubeTextureResourceAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("ArrayCubeTextureResourceDescription", ml, "ArrayCubeTextureResourceDescription")
  {}
};

struct VrsStateRequirementsAnnotation final : das::ManagedStructureAnnotation<dabfg::VrsStateRequirements>
{
  VrsStateRequirementsAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("VrsStateRequirements", ml, "dabfg::VrsStateRequirements")
  {
    addField<DAS_BIND_MANAGED_FIELD(rateX)>("rateX");
    addField<DAS_BIND_MANAGED_FIELD(rateY)>("rateY");
    addField<DAS_BIND_MANAGED_FIELD(rateTextureResId)>("rateTextureResId");
    addField<DAS_BIND_MANAGED_FIELD(vertexCombiner)>("vertexCombiner");
    addField<DAS_BIND_MANAGED_FIELD(pixelCombiner)>("pixelCombiner");
  }
  virtual bool isLocal() const override { return true; }
};

struct ResourceDataAnnotation final : das::ManagedStructureAnnotation<dabfg::ResourceData>
{
  ResourceDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResourceData", ml, "dabfg::ResourceData")
  {
    addField<DAS_BIND_MANAGED_FIELD(history)>("history");
    addField<DAS_BIND_MANAGED_FIELD(type)>("resType", "type");
  }
};

struct ResourceUsageAnnotation final : das::ManagedStructureAnnotation<dabfg::ResourceUsage>
{
  ResourceUsageAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResourceUsage", ml, "dabfg::ResourceUsage")
  {
    addField<DAS_BIND_MANAGED_FIELD(access)>("access");
    addField<DAS_BIND_MANAGED_FIELD(type)>("usageType", "type");
    addField<DAS_BIND_MANAGED_FIELD(stage)>("stage");
  }
  bool isLocal() const override { return true; }
};

struct ResourceRequestAnnotation final : das::ManagedStructureAnnotation<dabfg::ResourceRequest>
{
  ResourceRequestAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResourceRequest", ml, "dabfg::ResourceRequest")
  {
    addField<DAS_BIND_MANAGED_FIELD(usage)>("usage");
    addField<DAS_BIND_MANAGED_FIELD(slotRequest)>("slotRequest");
    addField<DAS_BIND_MANAGED_FIELD(optional)>("optional");
  }
};

struct AutoResolutionDataAnnotation final : das::ManagedStructureAnnotation<dabfg::AutoResolutionData>
{
  AutoResolutionDataAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AutoResolutionData", ml, "dabfg::AutoResolutionData")
  {
    addField<DAS_BIND_MANAGED_FIELD(id)>("id");
    addField<DAS_BIND_MANAGED_FIELD(multiplier)>("multiplier");
  }
};

struct ShaderBlockLayersInfoAnnotation final : das::ManagedStructureAnnotation<dabfg::ShaderBlockLayersInfo>
{
  ShaderBlockLayersInfoAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("ShaderBlockLayersInfo", ml, "dabfg::ShaderBlockLayersInfo")
  {
    addField<DAS_BIND_MANAGED_FIELD(objectLayer)>("objectLayer");
    addField<DAS_BIND_MANAGED_FIELD(frameLayer)>("frameLayer");
    addField<DAS_BIND_MANAGED_FIELD(sceneLayer)>("sceneLayer");
  }
};

struct VirtualSubresourceRefAnnotation final : das::ManagedStructureAnnotation<dabfg::VirtualSubresourceRef>
{
  VirtualSubresourceRefAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("VirtualSubresourceRef", ml, "dabfg::VirtualSubresourceRef")
  {
    addField<DAS_BIND_MANAGED_FIELD(nameId)>("nameId");
    addField<DAS_BIND_MANAGED_FIELD(mipLevel)>("mipLevel");
    addField<DAS_BIND_MANAGED_FIELD(layer)>("layer");
  }
  bool isLocal() const override { return true; }
};

struct BindingAnnotation final : das::ManagedStructureAnnotation<dabfg::Binding>
{
  BindingAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Binding", ml, "dabfg::Binding")
  {
    addField<DAS_BIND_MANAGED_FIELD(type)>("bindType", "type");
    addField<DAS_BIND_MANAGED_FIELD(resource)>("resource");
    addField<DAS_BIND_MANAGED_FIELD(history)>("history");
  }
};

namespace bind_dascript
{

void setTextureInfo(dabfg::ResourceData &res, TextureResourceDescription &desc)
{
  desc.clearValue = make_clear_value(0u, 0u, 0u, 0u);
  res.creationInfo = ResourceDescription{desc};
}

void setResolution(dabfg::ResourceData &res, const dabfg::AutoResolutionData &data) { res.resolution = data; }

void set_description(dabfg::ResourceData &res, const BufferResourceDescription &desc) { res.creationInfo = desc; }

void DaBfgModule::addStructureAnnotations(das::ModuleLibrary &lib)
{
  addAnnotation(das::make_smart<TextureResourceAnnotation>(lib));
  addAnnotation(das::make_smart<VolTextureResourceAnnotation>(lib));
  addAnnotation(das::make_smart<ArrayTextureResourceAnnotation>(lib));
  addAnnotation(das::make_smart<CubeTextureResourceAnnotation>(lib));
  addAnnotation(das::make_smart<ArrayCubeTextureResourceAnnotation>(lib));
  addAnnotation(das::make_smart<ResourceDataAnnotation>(lib));
  addAnnotation(das::make_smart<AutoResolutionDataAnnotation>(lib));
  addAnnotation(das::make_smart<ShaderBlockLayersInfoAnnotation>(lib));
  addAnnotation(das::make_smart<VrsStateRequirementsAnnotation>(lib));
  addAnnotation(das::make_smart<VirtualSubresourceRefAnnotation>(lib));
  addAnnotation(das::make_smart<BindingAnnotation>(lib));
  addAnnotation(das::make_smart<ResourceUsageAnnotation>(lib));
  addAnnotation(das::make_smart<ResourceRequestAnnotation>(lib));
  addAnnotation(das::make_smart<BufferResourceDescriptionAnnotation>(lib));

  BIND_FUNCTION(bind_dascript::set_description, "setDescription", das::SideEffects::modifyArgumentAndExternal);
  BIND_FUNCTION(bind_dascript::setResolution, "setResolution", das::SideEffects::modifyArgumentAndExternal);
  BIND_FUNCTION(bind_dascript::setTextureInfo, "setTextureInfo", das::SideEffects::modifyArgumentAndExternal);
}

} // namespace bind_dascript
