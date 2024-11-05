//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daBfg/usage.h>
#include <render/daBfg/stage.h>
#include <render/daBfg/resourceCreation.h>

#include <render/daBfg/detail/projectors.h>
#include <render/daBfg/detail/resUid.h>
#include <render/daBfg/detail/nodeNameId.h>
#include <render/daBfg/detail/blob.h>


namespace dabfg
{
struct InternalRegistry;
struct ResourceProvider;
struct ResourceRequest;
} // namespace dabfg


namespace dabfg::detail
{

// Non-template base class for VirtualResource(Semi)Request.
// Actual work happens here. This is a trick that allows us not to
// expose the InternalRegistry headers in the API.
struct VirtualResourceRequestBase
{
  void texture(const Texture2dCreateInfo &info);
  void texture(const Texture3dCreateInfo &info);
  void buffer(const BufferCreateInfo &info);
  void blob(const BlobDescription &desc);

  void markWithTag(ResourceSubtypeTag tag);

  void optional();
  void bindToShaderVar(const char *shader_var_name, ResourceSubtypeTag projectedTag, TypeErasedProjector projector);
  void bindAsView(ResourceSubtypeTag projectedTag, TypeErasedProjector projector);
  void bindAsProj(ResourceSubtypeTag projectedTag, TypeErasedProjector projector);
  void atStage(Stage stage);
  void useAs(Usage type);
  const ResourceProvider *provider();
  ResourceRequest &thisRequest();

  ResUid resUid;
  NodeNameId nodeId;
  InternalRegistry *registry;
};

} // namespace dabfg::detail
