//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/externalResources.h>

#include <3d/dag_resPtr.h>
#include <3d/dag_resizableTex.h>

#include <EASTL/string.h>

template <typename UniqueResType>
class FGExternalUniqueRes : public UniqueResType
{
protected:
  dafg::NodeHandle registerResourceNode{};
  using ResType = typename UniqueResType::resource_t;
  using BaseType = ManagedRes<ResType>;
  using ResViewType = ManagedResView<BaseType>;

  void swap(FGExternalUniqueRes &other)
  {
    UniqueResType::swap(other);
    eastl::swap(registerResourceNode, other.registerResourceNode);
  }

  void registerInFg(const char *daframegraph_name, dafg::NameSpace ns = dafg::root(),
    dafg::multiplexing::Mode multiplexing_mode = dafg::multiplexing::Mode::FullMultiplex)
  {
    if (!daframegraph_name)
      daframegraph_name = (*this)->getName();
    const eastl::string node_name{eastl::string::CtorSprintf{}, "register_%s", daframegraph_name};
    registerResourceNode = ns.registerNode(node_name.c_str(), DAFG_PP_NODE_SRC,
      [view = ResViewType{*this}, daframegraph_name, multiplexing_mode](dafg::Registry registry) {
        registry.multiplex(multiplexing_mode);
        if constexpr (eastl::is_same_v<ResViewType, ManagedTexView>)
          registry.registerTexture(daframegraph_name, [=](const dafg::multiplexing::Index &) { return view; });
        else
          registry.registerBuffer(daframegraph_name, [=](const dafg::multiplexing::Index &) { return view; });
      });
  }

public:
  FGExternalUniqueRes() = default;
  FGExternalUniqueRes(FGExternalUniqueRes &&other) noexcept { this->swap(other); }

  FGExternalUniqueRes &operator=(FGExternalUniqueRes &&other)
  {
    FGExternalUniqueRes(eastl::move(other)).swap(*this);
    return *this;
  }

  FGExternalUniqueRes(UniqueResType &&res, const char *daframegraph_name, dafg::NameSpace ns = dafg::root(),
    dafg::multiplexing::Mode multiplexing_mode = dafg::multiplexing::Mode::FullMultiplex) :
    UniqueResType(eastl::move(res))
  {
    registerInFg(daframegraph_name, ns, multiplexing_mode);
  }

  void changeResource(ResPtr<ResType> &&res) = delete;
};

using FGExternalUniqueTex = FGExternalUniqueRes<UniqueTex>;
using FGExternalUniqueBuf = FGExternalUniqueRes<UniqueBuf>;
using FGExternalUniqueTexHolder = FGExternalUniqueRes<UniqueTexHolder>;
using FGExternalUniqueBufHolder = FGExternalUniqueRes<UniqueBufHolder>;

template <typename ResizableResType>
class FGExternalResizableRes : public FGExternalUniqueRes<ResizableResType>
{
  const char *resourceName{};
  dafg::NameSpace resourceNS = dafg::root();
  dafg::multiplexing::Mode multiplexingMode = dafg::multiplexing::Mode::FullMultiplex;

  void swap(FGExternalResizableRes &other)
  {
    BaseWrapper::swap(other);
    eastl::swap(resourceName, other.resourceName);
    eastl::swap(resourceNS, other.resourceNS);
  }

  using BaseWrapper = FGExternalUniqueRes<ResizableResType>;

public:
  FGExternalResizableRes() = default;
  FGExternalResizableRes(FGExternalResizableRes &&other) noexcept { this->swap(other); }

  FGExternalResizableRes &operator=(FGExternalResizableRes &&other)
  {
    FGExternalResizableRes(eastl::move(other)).swap(*this);
    return *this;
  }

  FGExternalResizableRes(ResizableResType &&res, const char *daframegraph_name, dafg::NameSpace ns = dafg::root(),
    dafg::multiplexing::Mode multiplexing_mode = dafg::multiplexing::Mode::FullMultiplex) :
    BaseWrapper{eastl::move(res), daframegraph_name, ns, multiplexing_mode},
    resourceName{daframegraph_name},
    resourceNS{ns},
    multiplexingMode(multiplexing_mode)
  {}

  void resize(int width, int height, int d = 1)
  {
    ResizableResType::resize(width, height, d);
    this->registerInFg(resourceName, resourceNS, multiplexingMode);
  }
};

using FGExternalResizableTex = FGExternalResizableRes<ResizableTex>;
using FGExternalResizableTexHolder = FGExternalResizableRes<ResizableTexHolder>;
