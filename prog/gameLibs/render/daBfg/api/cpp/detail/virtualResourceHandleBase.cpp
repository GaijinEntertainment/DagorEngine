#include <render/daBfg/detail/virtualResourceHandleBase.h>
#include <frontend/resourceProvider.h>


namespace dabfg::detail
{

template <class T>
T VirtualResourceHandleBase::getResourceView() const
{
  auto &storage = resUid.history ? provider->providedHistoryResources : provider->providedResources;
  if (auto it = storage.find(resUid.resId); it != storage.end())
    return eastl::get<T>(it->second);
  else
    return {};
}

template ManagedTexView VirtualResourceHandleBase::getResourceView<ManagedTexView>() const;
template ManagedBufView VirtualResourceHandleBase::getResourceView<ManagedBufView>() const;
template BlobView VirtualResourceHandleBase::getResourceView<BlobView>() const;

} // namespace dabfg::detail
