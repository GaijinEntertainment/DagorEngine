//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/ecsHash.h>
#include <daECS/core/component.h>
#include <debug/dag_assert.h>
#include <daECS/core/internal/asserts.h>
#include <EASTL/vector_map.h>
#include <generic/dag_smallTab.h>

namespace ecs
{

class Object
{
public:
  // it is possible to optimize layout. our keys are immutable strings, i.e. we never change them
  // so we don't need capacity. actually, SimpleString (const char*) would be sufficient, but then there is no SSO
  // so optimal value_type would be 64/32-sizeof(ChildComponent) == 12/44 bytes of SSO buffer.
  // Most keys are smaller than 44, but some bigger than 12.
  typedef eastl::pair<eastl::string, ecs::ChildComponent> value_type;

  // also, size and capacity of hash_container_t and container_t is always same, so instead of 24*2 = 48 bytes we can have 32 bytes of
  // data.
  typedef dag::Vector<value_type> container_t;
  typedef dag::Vector<ecs::hash_str_t> hash_container_t;
  typedef container_t::iterator iterator;
  typedef container_t::const_iterator const_iterator;

  bool operator==(const Object &a) const;
  bool operator!=(const Object &a) const { return !(*this == a); }
  uint32_t size() const { return uint32_t(container.size()); }
  bool empty() const { return size() == 0; }
  iterator begin()
  {
    changeGen();
    return container.begin();
  }
  iterator end() { return container.end(); }
  const_iterator begin() const { return container.begin(); }
  const_iterator end() const { return container.end(); }

  __forceinline const_iterator find_as(const HashedConstString str) const
  {
    auto hashIt = eastl::lower_bound(hashContainer.begin(), hashContainer.end(), str.hash);
    if (hashIt == hashContainer.end() || *hashIt != str.hash)
      return end();

    if (DAGOR_LIKELY(noCollisions))
    {
      const_iterator it = container.begin() + (hashIt - hashContainer.begin());
      // Assume that no collisions in searched / instance string
      // return (!str.str || strcmp(it->first.c_str(), str.str) == 0) ? it : container.end();
      DAECS_EXT_ASSERTF(!str.str || !strcmp(it->first.c_str(), str.str), "Search key hash collision %#x: '%s' != '%s'", str.hash,
        str.str, it->first.c_str());
      return it;
    }
    else
      return findAsWithCollision(hashIt, str);
  }

  iterator find_as(const HashedConstString s)
  {
    auto it = const_cast<iterator>(((const Object *)this)->find_as(s));
    if (it != end())
      changeGen();
    return it;
  }

  iterator find_as(const char *s) { return find_as(ECS_HASH_SLOW(s)); }
  const_iterator find_as(const char *s) const { return find_as(ECS_HASH_SLOW(s)); }

  const_iterator find(const HashedConstString s) const { return find_as(s); }
  const_iterator find(const char *s) const { return find_as(s); }

  iterator erase(const_iterator pos)
  {
    if (pos == container.end())
      return container.end();
    changeGen();
    auto hashIt = hashContainer.begin() + (pos - container.begin());
    hashContainer.erase(hashIt);
    return container.erase(pos);
  }

  void clear()
  {
    if (empty())
      return;
    validateExtensive();
    changeGen();
    container.clear();
    hashContainer.clear();
    noCollisions = true;
  }

  template <typename T>
  const T *getNullable(const HashedConstString &name) const
  {
    auto it = find_as(name);
    return it == end() ? nullptr : it->second.getNullable<T>();
  }

  template <typename T>
  T *getNullableRW(const HashedConstString &name)
  {
    auto it = find_as(name);
    return it == end() ? nullptr : it->second.getNullable<T>();
  }

  const char *getMemberOr(const HashedConstString &name, const char *def) const;
  template <typename T, typename U = ECS_MAYBE_VALUE_T(T)>
  U getMemberOr(const HashedConstString &name, const T &def) const
  {
    auto it = find_as(name);
    return it == end() ? def : it->second.getOr(def);
  }
  template <typename T, typename U = ECS_MAYBE_VALUE_T(T)>
  U getMemberOr(const char *name, const T &def) const
  {
    return getMemberOr(ECS_HASH_SLOW(name), def);
  }
  bool hasMember(const HashedConstString name) const { return find_as(name) != end(); }
  bool hasMember(const char *name) const { return hasMember(ECS_HASH_SLOW(name)); }

  ChildComponent &insert(const HashedConstString str)
  {
    changeGen();
    auto hashIt = eastl::lower_bound(hashContainer.begin(), hashContainer.end(), str.hash); //
    if (noCollisions) // todo: optimize it, use one lower_bound and then insert!
    {
      auto it = hashIt != hashContainer.end() ? container.begin() + (hashIt - hashContainer.begin()) : container.end();
      if (it != container.end() && *hashIt == str.hash)
      {
        if (it->first != str.str)
          noCollisions = false; // we found collision!
        else
          return it->second; // already inserted key
      }
      else
      {
        hashContainer.emplace(hashIt, str.hash);
        return container.emplace(it, value_type{str.str, ecs::ChildComponent{}})->second;
      }
    }
    return insertWithCollision(hashIt, str);
  }
  ChildComponent &insert(const char *name) { return insert(ECS_HASH_SLOW(name)); }
  const ChildComponent &operator[](const HashedConstString name) const
  {
    auto it = find_as(name);
    return it == end() ? emptyAttrRO : it->second;
  }
  bool replicateCompare(const Object &o)
  {
    // we can also add hash of content and do full comparison only if hash is same (but we would have to keep flag if hash is
    // calculated)
    if (gen == o.gen)
    {
#if DAECS_EXTENSIVE_CHECKS
      if (*this != o)
        logerr("Object differs, while it's generation is same");
      else
        return false;
#else
      return false;
#endif
    }
    if (*this != o)
    {
      *this = o;
      gen = o.gen;
      return true;
    }
    gen = o.gen;
    return false;
  }
  // bool operator ==(const Object &o) const {return (gen == o.gen);}
  // comment to avoid careless use of slow operation
  // const ChildComponent& operator[](const char* name) const { return this->operator[](ECS_HASH_SLOW(name));}
  // ChildComponent& operator[](const char* name) { return this->operator[](ECS_HASH_SLOW(name));}
  // ecs20 compatibility:
  template <typename T, typename = eastl::enable_if_t<eastl::is_rvalue_reference<T &&>::value, void>>
  void addMember(const HashedConstString name, T &&val)
  {
    insert(name) = eastl::move(val);
  }
  template <typename T, typename = eastl::enable_if_t<eastl::is_rvalue_reference<T &&>::value, void>>
  void addMember(const char *name, T &&val)
  {
    addMember(ECS_HASH_SLOW(name), eastl::move(val));
  }
  template <typename T>
  void addMember(const HashedConstString name, const T &val)
  {
    insert(name) = val;
  }
  template <typename T>
  void addMember(const char *name, const T &val)
  {
    addMember(ECS_HASH_SLOW(name), val);
  }

  Object() = default;
  Object(const Object &a) : container(a.container), noCollisions(a.noCollisions), hashContainer(a.hashContainer) { changeGen(); }
  Object(Object &&a) : container(eastl::move(a.container)), hashContainer(eastl::move(a.hashContainer)), noCollisions(a.noCollisions)
  {
    changeGen();
  }
  inline Object &operator=(const Object &a)
  {
    container = a.container;
    hashContainer = a.hashContainer;
    noCollisions = a.noCollisions;
    changeGen();
    return *this;
  }
  inline Object &operator=(Object &&a)
  {
    eastl::swap(container, a.container);
    eastl::swap(hashContainer, a.hashContainer);
    noCollisions = a.noCollisions;
    changeGen();
    return *this;
  }

#if DAECS_EXTENSIVE_CHECKS
  ~Object() { validateExtensive(); }
#endif
  void reserve(uint32_t sz)
  {
    container.reserve(sz);
    hashContainer.reserve(sz);
  }
  void shrink_to_fit()
  {
    container.shrink_to_fit();
    hashContainer.shrink_to_fit();
  }

protected:
  static ChildComponent emptyAttrRO;
  void changeGen() { gen++; }
  container_t container;
  hash_container_t hashContainer;
  // TODO: remove this in release (i.e. assume that there is no collisions) with validation in dev
  bool noCollisions = true;
  uint32_t gen = 0;

  bool slowIsEqual(const ecs::Object &a) const;
  ChildComponent &insertWithCollision(hash_container_t::const_iterator, const ecs::HashedConstString str);
  const_iterator findAsWithCollision(hash_container_t::const_iterator, const ecs::HashedConstString str) const;
  void validate() const;
#if DAECS_EXTENSIVE_CHECKS && DAGOR_DBGLEVEL > 1
  void validateExtensive() const { validate(); }
#else
  void validateExtensive() const {}
#endif
};

inline eastl::string &get_key_string(const ecs::Object::iterator &i) { return i->first; }
inline const eastl::string &get_key_string(const ecs::Object::const_iterator &i) { return i->first; }

inline eastl::string &get_key_string(eastl::string &i) { return i; }
inline const eastl::string &get_key_string(const eastl::string &i) { return i; }

inline bool Object::operator==(const Object &a) const
{
  if (size() != a.size() || noCollisions != a.noCollisions)
    return false;
  if (DAGOR_LIKELY(noCollisions))
  {
    auto ahi = a.hashContainer.begin();
    auto bhi = hashContainer.begin();
    for (auto ai = a.begin(), bi = begin(), be = end(); bi != be; ++bi, ++ai, ++ahi, ++bhi)
      if (*ahi != *bhi || ai->second != bi->second)
        return false;
    return true;
  }
  else
    return slowIsEqual(a);
}

}; // namespace ecs
