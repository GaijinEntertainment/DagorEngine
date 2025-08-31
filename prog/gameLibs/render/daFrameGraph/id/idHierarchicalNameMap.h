// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/fixed_string.h>
#include <id/idNameMap.h>
#include <id/idSparseNameMap.h>
#include <id/idIndexedMapping.h>
#include <dag/dag_vectorMap.h>


namespace detail
{

template <class T>
struct PerFolderMemoizedChildren;

template <class FolderT, class T>
struct PerTypeData;

template <class... Ts>
struct FirstOf;
template <class T, class... Ts>
struct FirstOf<T, Ts...>
{
  using Type = T;
};

template <class T>
struct ShortNameIdImpl
{
  enum class Type : eastl::underlying_type_t<T>
  {
    Invalid = eastl::to_underlying(T::Invalid)
  };
};

} // namespace detail

/// Enum class based ID type for a short name. Used to speed up name lookups.
template <class T>
using ShortNameId = typename detail::ShortNameIdImpl<T>::Type;

/**
 * \brief A type-safe hierarchical name map.
 * Conceptually, it is a tree-like structure, where nodes are elements
 * of the first type in \p Ts and leafs are elements of any other types
 * in \p Ts.
 * Note that for every type in \p Ts, the indexes used for leafs of that
 * type are sequential and hence not unique across all of \p Ts. This
 * allows one to make effective leaf type -> data mappings by using
 * good ol' arrays.
 * Also note that removal of elements is not supported.
 * The implementation is sub-optimal, feel free to try and improve it.
 */
template <class... Ts>
class IdHierarchicalNameMap : private detail::PerTypeData<typename detail::FirstOf<Ts...>::Type, Ts>...
{
  static_assert((eastl::is_enum_v<Ts> && ...), "All Ts must be enum classes!");

  using FolderT = typename detail::FirstOf<Ts...>::Type;

public:
  IdHierarchicalNameMap();

  /// Returns the root node of the tree
  FolderT root() const { return rootFolder; }

  /// Returns the parent node for \p id
  template <class T>
  FolderT getParent(T id) const;

  /// Adds a new node of type \p T to \p folder with name \p name
  template <class T>
  T addNameId(FolderT folder, const char *name);

  /// Gets an existing ID with name \p name in folder \p folder of type \p T if it exists and Invalid otherwise
  template <class T>
  T getNameId(FolderT folder, const char *name) const;

  /// Gets an existing ID with short name id \p child in folder \p folder of type \p T if it exists and Invalid otherwise
  template <class T>
  T getNameId(FolderT folder, ShortNameId<T> child) const;

  /// Returns the full name of the node, as in "/a/b/c/node"
  template <class T>
  const char *getName(T id) const;

  /// Returns the short name of the node, as in "node" for "/a/b/c/node"
  template <class T>
  const char *getShortName(T id) const;

  template <class T>
  ShortNameId<T> getShortNameId(T id) const;

  template <class T>
  T getNameId(eastl::string_view full_name) const;

  template <class T>
  uint32_t nameCount() const;

  template <class T, class UnaryFunc>
  void iterateSubTree(FolderT folder, const UnaryFunc &f) const;

  template <class T>
  size_t getChildCount(FolderT folder) const;

private:
  FolderT rootFolder;

  struct PerFolderData : detail::PerFolderMemoizedChildren<Ts>...
  {};

  IdIndexedMapping<FolderT, PerFolderData> folders;
};

template <class T>
struct detail::PerFolderMemoizedChildren
{
  IdSparseNameMap<T> children;
  dag::VectorMap<ShortNameId<T>, T> childrenByShortName;
};

template <class FolderT, class T>
struct detail::PerTypeData
{
  IdNameMap<T> fullPaths;
  IdNameMap<ShortNameId<T>> shortNameIds;
  IdIndexedMapping<T, ShortNameId<T>> shortNameIdByNameId;
  IdIndexedMapping<T, FolderT> parents;
};

template <class... Ts>
IdHierarchicalNameMap<Ts...>::IdHierarchicalNameMap()
{
  using Data = detail::PerTypeData<FolderT, FolderT>;
  rootFolder = Data::fullPaths.addNameId("");
  Data::parents.set(rootFolder, rootFolder);
  folders.expandMapping(rootFolder);
}

template <class... Ts>
template <class T>
auto IdHierarchicalNameMap<Ts...>::getParent(T id) const -> FolderT
{
  using Data = detail::PerTypeData<FolderT, T>;
  return Data::parents[id];
}

template <class... Ts>
template <class T>
T IdHierarchicalNameMap<Ts...>::addNameId(FolderT folder, const char *name)
{
  G_ASSERTF(name != nullptr && *name != '\0', "Empty names are not allowed!");
  G_ASSERTF(strchr(name, '/') == nullptr, "Names cannot contain slashes!");

  using ChildData = detail::PerFolderMemoizedChildren<T>;
  auto &folderChildren = folders[folder].ChildData::children;

  if (const auto childId = folderChildren.get(name); childId != T::Invalid)
    return childId;

  eastl::fixed_string<char, 256> fullPath;
  fullPath.append(getName(folder));
  fullPath.append("/");
  fullPath.append(name);

  using Data = detail::PerTypeData<FolderT, T>;
  const auto childId = Data::fullPaths.addNameId(fullPath.c_str());
  Data::parents.set(childId, folder);
  folderChildren.add(name, childId);
  const auto shortNameId = Data::shortNameIds.addNameId(name);
  Data::shortNameIdByNameId.set(childId, shortNameId);
  folders[folder].ChildData::childrenByShortName.emplace(shortNameId, childId);
  if constexpr (eastl::is_same_v<T, FolderT>)
    folders.expandMapping(childId);

  return childId;
}

template <class... Ts>
template <class T>
T IdHierarchicalNameMap<Ts...>::getNameId(FolderT folder, const char *name) const
{
  using ChildData = detail::PerFolderMemoizedChildren<T>;
  const auto &folderChildren = folders[folder].ChildData::children;
  return folderChildren.get(name);
}

template <class... Ts>
template <class T>
T IdHierarchicalNameMap<Ts...>::getNameId(FolderT folder, ShortNameId<T> child) const
{
  using ChildData = detail::PerFolderMemoizedChildren<T>;
  const auto &childrenByShortName = folders[folder].ChildData::childrenByShortName;
  auto it = childrenByShortName.find(child);
  return it != childrenByShortName.end() ? it->second : T::Invalid;
}

template <class... Ts>
template <class T>
const char *IdHierarchicalNameMap<Ts...>::getName(T id) const
{
  using Data = detail::PerTypeData<FolderT, T>;
  return Data::fullPaths.getNameCstr(id);
}

template <class... Ts>
template <class T>
const char *IdHierarchicalNameMap<Ts...>::getShortName(T id) const
{
  // Fun hack :)
  const auto fullName = getName<T>(id);
  const auto lastSlash = strrchr(fullName, '/');
  G_ASSERT_RETURN(lastSlash != nullptr, fullName); // sanity check
  return lastSlash + 1;
}

template <class... Ts>
template <class T>
ShortNameId<T> IdHierarchicalNameMap<Ts...>::getShortNameId(T id) const
{
  using Data = detail::PerTypeData<FolderT, T>;
  return Data::shortNameIdByNameId[id];
}

template <class... Ts>
template <class T>
T IdHierarchicalNameMap<Ts...>::getNameId(eastl::string_view full_name) const
{
  using Data = detail::PerTypeData<FolderT, T>;
  return Data::fullPaths.getNameId(full_name);
}

template <class... Ts>
template <class T>
uint32_t IdHierarchicalNameMap<Ts...>::nameCount() const
{
  return detail::PerTypeData<FolderT, T>::fullPaths.nameCount();
}

template <class... Ts>
template <class T, class UnaryFunc>
void IdHierarchicalNameMap<Ts...>::iterateSubTree(FolderT folder, const UnaryFunc &f) const
{
  using ChildFolders = detail::PerFolderMemoizedChildren<FolderT>;
  for (const auto &childFolderId : folders[folder].ChildFolders::children.getIds())
    iterateSubTree<T>(childFolderId, f);

  using ChildData = detail::PerFolderMemoizedChildren<T>;
  const auto childIds = folders[folder].ChildData::children.getIds();
  eastl::for_each(childIds.cbegin(), childIds.cend(), f);
}

template <class... Ts>
template <class T>
size_t IdHierarchicalNameMap<Ts...>::getChildCount(FolderT folder) const
{
  using ChildData = detail::PerFolderMemoizedChildren<T>;
  return folders[folder].ChildData::children.size();
}
