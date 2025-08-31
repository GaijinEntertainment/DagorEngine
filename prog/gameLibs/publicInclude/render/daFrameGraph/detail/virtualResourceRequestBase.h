//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/usage.h>
#include <render/daFrameGraph/stage.h>
#include <render/daFrameGraph/resourceCreation.h>

#include <render/daFrameGraph/detail/projectors.h>
#include <render/daFrameGraph/detail/resUid.h>
#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/blob.h>
#include <render/daFrameGraph/detail/rtti.h>


union ResourceClearValue;

namespace dafg
{
struct InternalRegistry;
struct ResourceProvider;
struct ResourceRequest;
} // namespace dafg


namespace dafg::detail
{

// Non-template base class for VirtualResource(Semi)Request.
// Actual work happens here. This is a trick that allows us not to
// expose the InternalRegistry headers in the API.
struct VirtualResourceRequestBase
{
  void texture(const Texture2dCreateInfo &info);
  void texture(const Texture3dCreateInfo &info);
  void buffer(const BufferCreateInfo &info);
  void blob(BlobDescription &&desc, detail::RTTI &&rtti);

  void markWithTag(ResourceSubtypeTag tag);

  void optional();
  void bindToShaderVar(const char *shader_var_name, ResourceSubtypeTag projectedTag, TypeErasedProjector projector);
  void bindAsView(ResourceSubtypeTag projectedTag, TypeErasedProjector projector);
  void bindAsProj(ResourceSubtypeTag projectedTag, TypeErasedProjector projector);
  void bindAsVertexBuffer(uint32_t stream, uint32_t stride);
  void bindAsIndexBuffer();

  void atStage(Stage stage);
  void useAs(Usage type);
  void clear(const ResourceClearValue &clear_value);
  void clear(ResourceSubtypeTag projectee, const char *blob_name, detail::TypeErasedProjector projector);
  const ResourceProvider *provider();
  ResourceRequest &thisRequest();

  ResUid resUid;
  NodeNameId nodeId;
  InternalRegistry *registry;
};

} // namespace dafg::detail
