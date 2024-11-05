//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daBfg/detail/resUid.h>
#include <render/daBfg/detail/blob.h>


namespace dabfg
{
struct ResourceProvider;
}

namespace dabfg::detail
{

// Non-template base class for VirtualResourceHandle.
// Actual work happens here. This is a trick that allows us not to
// expose the ResourceProvider headers in the API.
struct VirtualResourceHandleBase
{
  // T can only be ManagedTexView, ManagedBufView or BlobView
  template <class T>
  T getResourceView() const;

  ResUid resUid;
  const ResourceProvider *provider;
};

} // namespace dabfg::detail
