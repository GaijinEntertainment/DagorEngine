#include <api/dasModules/frameGraphModule.h>

#define DAS_BIND_ENUM_BOTH(enum_name, das_enum_name, ...) \
  DAS_BASE_BIND_ENUM_BOTH(DAS_BIND_ENUM_QUALIFIED_HELPER, enum_name, das_enum_name, __VA_ARGS__)

DAS_BIND_ENUM_BOTH(dabfg::NameSpaceNameId, NameSpaceNameId, //-V1008
  Invalid);

DAS_BIND_ENUM_BOTH(dabfg::NodeNameId, NodeNameId, //-V1008
  Invalid);

DAS_BIND_ENUM_BOTH(dabfg::ResNameId, ResNameId, //-V1008
  Invalid);

DAS_BIND_ENUM_BOTH(dabfg::AutoResTypeNameId, AutoResTypeNameId, //-V1008
  Invalid);

DAS_BIND_ENUM_BOTH(dabfg::multiplexing::Mode, MultiplexingMode, None, SuperSampling, SubSampling, Viewport, FullMultiplex);

DAS_BIND_ENUM_BOTH(dabfg::SideEffects, SideEffect, None, Internal, External);


DAS_BIND_ENUM_BOTH(dabfg::Access, Access, UNKNOWN, READ_ONLY, READ_WRITE);
DAS_BIND_ENUM_BOTH(dabfg::Usage, Usage, UNKNOWN, COLOR_ATTACHMENT, INPUT_ATTACHMENT, DEPTH_ATTACHMENT,
  DEPTH_ATTACHMENT_AND_SHADER_RESOURCE, RESOLVE_ATTACHMENT, SHADER_RESOURCE, CONSTANT_BUFFER, INDEX_BUFFER, VERTEX_BUFFER, COPY, BLIT,
  INDIRECTION_BUFFER, VRS_RATE_TEXTURE);
DAS_BIND_ENUM_BOTH(dabfg::Stage, Stage, UNKNOWN, PRE_RASTER, POST_RASTER, COMPUTE, TRANSFER, RAYTRACE, ALL_GRAPHICS, ALL_INDIRECTION);


DAS_BIND_ENUM_BOTH(dabfg::History, History, No, ClearZeroOnFirstFrame, DiscardOnFirstFrame);

DAS_BIND_ENUM_BOTH(ResourceActivationAction, ResourceActivationAction, REWRITE_AS_COPY_DESTINATION, REWRITE_AS_UAV, REWRITE_AS_RTV_DSV,
  CLEAR_F_AS_UAV, CLEAR_I_AS_UAV, CLEAR_AS_RTV_DSV, DISCARD_AS_UAV, DISCARD_AS_RTV_DSV);


DAS_BIND_ENUM_BOTH(dabfg::ResourceType, ResourceType, Invalid, Texture, Buffer, Blob);

DAS_BIND_ENUM_BOTH(VariableRateShadingCombiner, VariableRateShadingCombiner, VRS_PASSTHROUGH, VRS_OVERRIDE, VRS_MIN, VRS_MAX, VRS_SUM);

DAS_BIND_ENUM_BOTH(dabfg::BindingType, BindingType, ShaderVar, ViewMatrix, ProjMatrix, Invalid);

#undef DAS_BIND_ENUM_BOTH

namespace bind_dascript
{

void DaBfgModule::addEnumerations(das::ModuleLibrary &)
{
  addEnumeration(das::make_smart<EnumerationNameSpaceNameId>());
  addEnumeration(das::make_smart<EnumerationNodeNameId>());
  addEnumeration(das::make_smart<EnumerationResNameId>());
  addEnumeration(das::make_smart<EnumerationHistory>());
  addEnumeration(das::make_smart<EnumerationResourceActivationAction>());
  addEnumeration(das::make_smart<EnumerationMultiplexingMode>());
  addEnumeration(das::make_smart<EnumerationSideEffect>());

  addEnumeration(das::make_smart<EnumerationAccess>());
  addEnumeration(das::make_smart<EnumerationUsage>());
  addEnumeration(das::make_smart<EnumerationStage>());

  addEnumeration(das::make_smart<EnumerationResourceType>());
  addEnumeration(das::make_smart<EnumerationAutoResTypeNameId>());
  addEnumeration(das::make_smart<EnumerationVariableRateShadingCombiner>());
  addEnumeration(das::make_smart<EnumerationBindingType>());
}

} // namespace bind_dascript
