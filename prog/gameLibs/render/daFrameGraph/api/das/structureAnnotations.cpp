// Copyright (C) Gaijin Games KFT.  All rights reserved.

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

struct VrsStateRequirementsAnnotation final : das::ManagedStructureAnnotation<dafg::VrsStateRequirements>
{
  VrsStateRequirementsAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("VrsStateRequirements", ml, "dafg::VrsStateRequirements")
  {
    addField<DAS_BIND_MANAGED_FIELD(rateX)>("rateX");
    addField<DAS_BIND_MANAGED_FIELD(rateY)>("rateY");
    addField<DAS_BIND_MANAGED_FIELD(vertexCombiner)>("vertexCombiner");
    addField<DAS_BIND_MANAGED_FIELD(pixelCombiner)>("pixelCombiner");
  }
  virtual bool isLocal() const override { return true; }
};

struct ResourceDataAnnotation final : das::ManagedStructureAnnotation<dafg::ResourceData>
{
  ResourceDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResourceData", ml, "dafg::ResourceData")
  {
    addField<DAS_BIND_MANAGED_FIELD(history)>("history");
    addField<DAS_BIND_MANAGED_FIELD(type)>("resType", "type");
  }
};

struct ResourceUsageAnnotation final : das::ManagedStructureAnnotation<dafg::ResourceUsage>
{
  ResourceUsageAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResourceUsage", ml, "dafg::ResourceUsage")
  {
    addField<DAS_BIND_MANAGED_FIELD(type)>("usageType", "type");
    addField<DAS_BIND_MANAGED_FIELD(access)>("access");
    addField<DAS_BIND_MANAGED_FIELD(stage)>("stage");
  }
  bool isLocal() const override { return true; }
};

struct ResourceRequestAnnotation final : das::ManagedStructureAnnotation<dafg::ResourceRequest>
{
  ResourceRequestAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResourceRequest", ml, "dafg::ResourceRequest")
  {
    addField<DAS_BIND_MANAGED_FIELD(usage)>("usage");
    addField<DAS_BIND_MANAGED_FIELD(slotRequest)>("slotRequest");
    addField<DAS_BIND_MANAGED_FIELD(optional)>("optional");
  }
};

struct AutoResolutionDataAnnotation final : das::ManagedStructureAnnotation<dafg::AutoResolutionData>
{
  AutoResolutionDataAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AutoResolutionData", ml, "dafg::AutoResolutionData")
  {
    addField<DAS_BIND_MANAGED_FIELD(id)>("id");
    addField<DAS_BIND_MANAGED_FIELD(multiplier)>("multiplier");
  }
};

struct ShaderBlockLayersInfoAnnotation final : das::ManagedStructureAnnotation<dafg::ShaderBlockLayersInfo>
{
  ShaderBlockLayersInfoAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("ShaderBlockLayersInfo", ml, "dafg::ShaderBlockLayersInfo")
  {
    addField<DAS_BIND_MANAGED_FIELD(objectLayer)>("objectLayer");
    addField<DAS_BIND_MANAGED_FIELD(frameLayer)>("frameLayer");
    addField<DAS_BIND_MANAGED_FIELD(sceneLayer)>("sceneLayer");
  }
};

struct VirtualSubresourceRefAnnotation final : das::ManagedStructureAnnotation<dafg::VirtualSubresourceRef>
{
  VirtualSubresourceRefAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("VirtualSubresourceRef", ml, "dafg::VirtualSubresourceRef")
  {
    addField<DAS_BIND_MANAGED_FIELD(nameId)>("nameId");
    addField<DAS_BIND_MANAGED_FIELD(mipLevel)>("mipLevel");
    addField<DAS_BIND_MANAGED_FIELD(layer)>("layer");
  }
  bool isLocal() const override { return true; }
};

struct BindingAnnotation final : das::ManagedStructureAnnotation<dafg::Binding>
{
  BindingAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Binding", ml, "dafg::Binding")
  {
    addField<DAS_BIND_MANAGED_FIELD(type)>("bindType", "type");
    addField<DAS_BIND_MANAGED_FIELD(resource)>("resource");
    addField<DAS_BIND_MANAGED_FIELD(history)>("history");
  }
};


static_assert(eastl::is_trivially_copyable_v<dafg::BlobView>);
static_assert(eastl::is_trivially_destructible_v<dafg::BlobView>);
static_assert(sizeof(dafg::BlobView) <= sizeof(vec4f));

struct BlobViewAnnotation final : das::ManagedValueAnnotation<dafg::BlobView>
{
  BlobViewAnnotation(das::ModuleLibrary &ml) : ManagedValueAnnotation(ml, "BlobView", "dafg::BlobView") {}
  bool hasNonTrivialCtor() const override { return false; }
};

namespace bind_dascript
{

void setResolution(dafg::ResourceData &res, const dafg::AutoResolutionData &data) { res.resolution = data; }

void setTextureDescription(dafg::ResourceData &res, TextureResourceDescription &desc)
{
  // TODO: this is a dirty hack, we need to properly bind eastl::variant to das
  // instead of using TextureResourceDescription
  dafg::Texture2dCreateInfo info;
  info.creationFlags = desc.cFlags;
  info.mipLevels = desc.mipLevels;
  info.resolution = IPoint2{(int)desc.width, (int)desc.height};
  res.creationInfo = info;
}

void setBufferDescription(dafg::ResourceData &res, const BufferResourceDescription &desc)
{
  // TODO: this is a dirty hack, we need to properly bind eastl::variant to das
  // instead of using BufferResourceDescription
  dafg::BufferCreateInfo info;
  info.elementSize = desc.elementSizeInBytes;
  info.elementCount = desc.elementCount;
  info.flags = desc.cFlags;
  info.format = desc.viewFormat;
  res.creationInfo = info;
}

void DaFgCoreModule::addStructureAnnotations(das::ModuleLibrary &lib)
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
  addAnnotation(das::make_smart<BlobViewAnnotation>(lib));

  BIND_FUNCTION(bind_dascript::setResolution, "setResolution", das::SideEffects::modifyArgumentAndExternal);
  BIND_FUNCTION(bind_dascript::setBufferDescription, "setDescription", das::SideEffects::modifyArgumentAndExternal);
  BIND_FUNCTION(bind_dascript::setTextureDescription, "setDescription", das::SideEffects::modifyArgumentAndExternal);
}

} // namespace bind_dascript
