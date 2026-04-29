//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/resUid.h>
#include <render/daFrameGraph/detail/blob.h>


namespace dafg
{
struct ResourceProvider;
}

namespace dafg::detail
{

// Non-template base class for VirtualResourceHandle.
// Actual work happens here. This is a trick that allows us not to
// expose the ResourceProvider headers in the API.
struct VirtualResourceHandleBase
{
  // T can only be BaseTexture*, Sbuffer* or BlobView
  template <class T>
  T getResourceData() const;

  ResUid resUid;
  const ResourceProvider *provider;
};

} // namespace dafg::detail
