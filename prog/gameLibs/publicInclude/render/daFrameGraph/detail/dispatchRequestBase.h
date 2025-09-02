//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/shaderNameId.h>
#include <render/daFrameGraph/detail/projectors.h>
#include <render/daFrameGraph/detail/resourceType.h>
#include <render/daFrameGraph/detail/virtualResourceRequestBase.h>

struct InternalRegistry;

namespace dafg::detail
{

struct DispatchRequestBase
{
  void threads();
  void groups();
  void indirect(const char *buffer);

  void meshThreads();
  void meshGroups();
  void meshIndirect(const char *buffer);
  void meshIndirectCount(const char *buffer);

  enum class ArgType : uint8_t
  {
    X,
    Y,
    Z,
    Offset,
    Stride,
    Count,
    CountOffset,
    MaxCount,
  };

  template <ArgType arg_type>
  void setArg(uint32_t value);

  template <ArgType arg_type>
  void setBlobArg(VirtualResourceRequestBase request, TypeErasedProjector projector);

  template <ArgType arg_type, int D>
  void setAutoResolutionArg(AutoResolutionRequest<D> request, TypeErasedProjector projector)
  {
    setAutoResolutionArg<arg_type>(request.autoResTypeId, request.multiplier, projector);
  }

  template <ArgType arg_type>
  void setTextureResolutionArg(VirtualResourceRequestBase request, TypeErasedProjector projector);

  InternalRegistry *registry;
  NodeNameId nodeId;
  ShaderNameId shaderId;

private:
  template <ArgType arg_type>
  void setAutoResolutionArg(AutoResTypeNameId resolution, float multiplier, TypeErasedProjector projector);
};

} // namespace dafg::detail
