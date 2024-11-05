// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <api/das/frameGraphModule.h>


DAS_BASE_BIND_ENUM_IMPL(dabfg::NameSpaceNameId, NameSpaceNameId, //-V1008
  Invalid);

DAS_BASE_BIND_ENUM_IMPL(dabfg::NodeNameId, NodeNameId, //-V1008
  Invalid);

DAS_BASE_BIND_ENUM_IMPL(dabfg::ResNameId, ResNameId, //-V1008
  Invalid);

DAS_BASE_BIND_ENUM_IMPL(dabfg::AutoResTypeNameId, AutoResTypeNameId, //-V1008
  Invalid);

DAS_BASE_BIND_ENUM_IMPL(dabfg::multiplexing::Mode, MultiplexingMode, None, SuperSampling, SubSampling, Viewport, FullMultiplex);

DAS_BASE_BIND_ENUM_IMPL(dabfg::SideEffects, SideEffect, None, Internal, External);


DAS_BASE_BIND_ENUM_IMPL(dabfg::Access, Access, UNKNOWN, READ_ONLY, READ_WRITE);
DAS_BASE_BIND_ENUM_IMPL(dabfg::Usage, Usage, UNKNOWN, COLOR_ATTACHMENT, INPUT_ATTACHMENT, DEPTH_ATTACHMENT,
  DEPTH_ATTACHMENT_AND_SHADER_RESOURCE, RESOLVE_ATTACHMENT, SHADER_RESOURCE, CONSTANT_BUFFER, INDEX_BUFFER, VERTEX_BUFFER, COPY, BLIT,
  INDIRECTION_BUFFER, VRS_RATE_TEXTURE);
DAS_BASE_BIND_ENUM_IMPL(dabfg::Stage, Stage, UNKNOWN, PRE_RASTER, POST_RASTER, COMPUTE, TRANSFER, RAYTRACE, ALL_GRAPHICS,
  ALL_INDIRECTION);


DAS_BASE_BIND_ENUM_IMPL(dabfg::History, History, No, ClearZeroOnFirstFrame, DiscardOnFirstFrame);

DAS_BASE_BIND_ENUM_IMPL(ResourceActivationAction, ResourceActivationAction, REWRITE_AS_COPY_DESTINATION, REWRITE_AS_UAV,
  REWRITE_AS_RTV_DSV, CLEAR_F_AS_UAV, CLEAR_I_AS_UAV, CLEAR_AS_RTV_DSV, DISCARD_AS_UAV, DISCARD_AS_RTV_DSV);


DAS_BASE_BIND_ENUM_IMPL(dabfg::ResourceType, ResourceType, Invalid, Texture, Buffer, Blob);

DAS_BASE_BIND_ENUM_IMPL(VariableRateShadingCombiner, VariableRateShadingCombiner, VRS_PASSTHROUGH, VRS_OVERRIDE, VRS_MIN, VRS_MAX,
  VRS_SUM);

DAS_BASE_BIND_ENUM_IMPL(dabfg::BindingType, BindingType, ShaderVar, ViewMatrix, ProjMatrix, Invalid);

namespace bind_dascript
{

void DaBfgCoreModule::addEnumerations(das::ModuleLibrary &)
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
