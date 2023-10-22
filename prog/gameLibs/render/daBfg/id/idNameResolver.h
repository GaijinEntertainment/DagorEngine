#pragma once

#include <id/idHierarchicalNameMap.h>
#include <id/idIndexedFlags.h>
#include <id/idRange.h>


namespace detail
{
template <class T>
struct PerTypeResolver
{
  IdIndexedMapping<T, T> resolved;
};
} // namespace detail

template <class FolderT, class... Ts>
class IdNameResolver : private detail::PerTypeResolver<Ts>...
{
public:
  IdNameResolver() {}

  template <class A>
  void rebuild(const IdHierarchicalNameMap<FolderT, Ts...> &name_map, const IdIndexedFlags<Ts, A> &...validity_flags_pack)
  {
    // This is very strong template magic.
    (
      [&]() {
        // The entire lambda is an expression that has two packs mentioned
        // inside of it, Ts and validity_flags_pack. Below, we immediately
        // invoke the lambda and use an operator comma fold expression to
        // run this lambda for every type in Ts.
        using T = Ts;
        const auto &validity_flags = validity_flags_pack;

        // All ids we get down below are then guaranteed to be present in the validity flags.
        G_ASSERT(validity_flags.size() == name_map.template nameCount<T>());

        auto &res = detail::PerTypeResolver<T>::resolved;
        res.resize(name_map.template nameCount<T>());

        for (auto [from, to] : res.enumerate())
        {
          const auto targetShortName = name_map.getShortName(from);

          auto currFolder = name_map.getParent(from);
          uint32_t stepUpCounter = 0;
          static constexpr uint32_t INFINITE_LOOP_THRESHOLD = 100;
          for (; stepUpCounter < INFINITE_LOOP_THRESHOLD; ++stepUpCounter)
          {
            const auto candidate = name_map.template getNameId<T>(currFolder, targetShortName);
            if (candidate != T::Invalid && validity_flags[candidate])
            {
              to = candidate;
              break;
            }

            if (currFolder == name_map.root())
            {
              to = T::Invalid;
              break;
            }

            currFolder = name_map.getParent(currFolder);
          }
          if (stepUpCounter == INFINITE_LOOP_THRESHOLD)
            logerr("Went up by %d levels of nested namespaces, probably an infinite loop in dabfg backend!", stepUpCounter);
        }
      }(),
      ...);
  }

  template <class T>
  T resolve(T id) const
  {
    const auto &res = detail::PerTypeResolver<T>::resolved;
    G_ASSERT(res.isMapped(id));
    return res[id];
  }
};
