// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/detail/virtualResourceHandleBase.h>
#include <frontend/resourceProvider.h>


namespace dabfg::detail
{

template <class T>
T VirtualResourceHandleBase::getResourceView() const
{
  auto &storage = resUid.history ? provider->providedHistoryResources : provider->providedResources;
  if (auto it = storage.find(resUid.resId); it != storage.end())
  {
    if (eastl::holds_alternative<MissingOptionalResource>(it->second))
      return {};
    else
      return eastl::get<T>(it->second);
  }
  else
  {
    logerr("daBfg: Attempted to get a view to a resource which is banned from being accessed! "
           "This is likely the back buffer, which you cannot access directly, only render to it through pass requests!");
    return {};
  }
}

template ManagedTexView VirtualResourceHandleBase::getResourceView<ManagedTexView>() const;
template ManagedBufView VirtualResourceHandleBase::getResourceView<ManagedBufView>() const;
template BlobView VirtualResourceHandleBase::getResourceView<BlobView>() const;

} // namespace dabfg::detail
