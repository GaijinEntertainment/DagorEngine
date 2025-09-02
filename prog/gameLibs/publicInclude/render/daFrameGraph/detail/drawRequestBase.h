//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/shaderNameId.h>
#include <render/daFrameGraph/detail/projectors.h>
#include <render/daFrameGraph/detail/virtualResourceRequestBase.h>
#include <render/daFrameGraph/detail/drawRequestPolicy.h>
#include <render/daFrameGraph/primitive.h>

struct InternalRegistry;

namespace dafg::detail
{

struct DrawRequestBase
{
  void draw(DrawPrimitive primitive);
  void drawIndexed(DrawPrimitive primitive);
  void indirect(const char *buffer);

  enum class ArgType : uint8_t
  {
    StartVertex,
    BaseVertex,
    StartIndex,
    PrimitiveCount,
    StartInstance,
    InstanceCount,
    Offset,
    Stride,
    Count,
  };

  template <ArgType arg_type>
  void setArg(uint32_t value);

  template <ArgType arg_type>
  void setBlobArg(VirtualResourceRequestBase request, TypeErasedProjector projector);

  InternalRegistry *registry;
  NodeNameId nodeId;
  ShaderNameId shaderId;
};

} // namespace dafg::detail
