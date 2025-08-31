// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <api/das/frameGraphModule.h>


DAS_BASE_BIND_ENUM_IMPL(dafg::NameSpaceNameId, NameSpaceNameId, //-V1008
  Invalid);

DAS_BASE_BIND_ENUM_IMPL(dafg::NodeNameId, NodeNameId, //-V1008
  Invalid);

DAS_BASE_BIND_ENUM_IMPL(dafg::ResNameId, ResNameId, //-V1008
  Invalid);

DAS_BASE_BIND_ENUM_IMPL(dafg::AutoResTypeNameId, AutoResTypeNameId, //-V1008
  Invalid);

DAS_BASE_BIND_ENUM_IMPL(dafg::multiplexing::Mode, MultiplexingMode, None, SuperSampling, SubSampling, Viewport, FullMultiplex);

DAS_BASE_BIND_ENUM_IMPL(dafg::SideEffects, SideEffect, None, Internal, External);


DAS_BASE_BIND_ENUM_IMPL(dafg::Access, Access, UNKNOWN, READ_ONLY, READ_WRITE);
DAS_BASE_BIND_ENUM_IMPL(dafg::Usage, Usage, UNKNOWN, COLOR_ATTACHMENT, DEPTH_ATTACHMENT, RESOLVE_ATTACHMENT, SHADER_RESOURCE,
  CONSTANT_BUFFER, INDEX_BUFFER, VERTEX_BUFFER, COPY, BLIT, INDIRECTION_BUFFER, VRS_RATE_TEXTURE, INPUT_ATTACHMENT,
  DEPTH_ATTACHMENT_AND_SHADER_RESOURCE);
DAS_BASE_BIND_ENUM_IMPL(dafg::Stage, Stage, UNKNOWN, PRE_RASTER, POST_RASTER, COMPUTE, TRANSFER, RAYTRACE, ALL_GRAPHICS,
  ALL_INDIRECTION);


DAS_BASE_BIND_ENUM_IMPL(dafg::History, History, No, ClearZeroOnFirstFrame, DiscardOnFirstFrame);

DAS_BASE_BIND_ENUM_IMPL(ResourceActivationAction, ResourceActivationAction, REWRITE_AS_COPY_DESTINATION, REWRITE_AS_UAV,
  REWRITE_AS_RTV_DSV, CLEAR_F_AS_UAV, CLEAR_I_AS_UAV, CLEAR_AS_RTV_DSV, DISCARD_AS_UAV, DISCARD_AS_RTV_DSV);


DAS_BASE_BIND_ENUM_IMPL(dafg::ResourceType, ResourceType, Invalid, Texture, Buffer, Blob);

DAS_BASE_BIND_ENUM_IMPL(VariableRateShadingCombiner, VariableRateShadingCombiner, VRS_PASSTHROUGH, VRS_OVERRIDE, VRS_MIN, VRS_MAX,
  VRS_SUM);

DAS_BASE_BIND_ENUM_IMPL(dafg::BindingType, BindingType, ShaderVar, ViewMatrix, ProjMatrix, Invalid);

namespace bind_dascript
{

void DaFgCoreModule::addEnumerations(das::ModuleLibrary &lib)
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
  das::addEnumFlagOps<dafg::Usage>(*this, lib, "dafg::Usage");
  addEnumeration(das::make_smart<EnumerationStage>());

  addEnumeration(das::make_smart<EnumerationResourceType>());
  addEnumeration(das::make_smart<EnumerationAutoResTypeNameId>());
  addEnumeration(das::make_smart<EnumerationVariableRateShadingCombiner>());
  addEnumeration(das::make_smart<EnumerationBindingType>());
}

} // namespace bind_dascript
