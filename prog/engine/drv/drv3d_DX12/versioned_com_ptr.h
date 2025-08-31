// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>
#include <EASTL/type_traits.h>

template <typename... Ts>
struct TypePack
{};

template <typename T, typename P>
struct TypeIndexInTypePack;

template <typename T, typename... Ts>
struct TypeIndexInTypePack<T, TypePack<T, Ts...>>
{
  static constexpr size_t value = 0;
};

template <typename T, typename O, typename... Ts>
struct TypeIndexInTypePack<T, TypePack<O, Ts...>>
{
  static constexpr size_t value = 1 + TypeIndexInTypePack<T, TypePack<Ts...>>::value;
};

template <typename T, typename A>
struct AppendToTypePack;

template <typename T, typename... Ts>
struct AppendToTypePack<TypePack<Ts...>, T>
{
  using Type = TypePack<Ts..., T>;
};

template <typename Base, typename... Versions>
class VersionedComPtr;

template <typename Base, typename... Versions>
class VersionedPtr
{
  using TypePackSet = TypePack<Base, Versions...>;
  using PointerTypePackSet = TypePack<eastl::add_pointer_t<Base>, eastl::add_pointer_t<Versions>...>;
  Base *pointer = nullptr;
  size_t versionIndex = 0;

  template <typename T>
  static bool canCast(size_t from)
  {
    static constexpr bool table[] = //
      {eastl::is_convertible_v<eastl::add_pointer_t<Base>, eastl::add_pointer_t<T>>,
        (eastl::is_convertible_v<eastl::add_pointer_t<Versions>, eastl::add_pointer_t<T>>)...};
    return table[from];
  }

public:
  VersionedPtr() = default;
  ~VersionedPtr() = default;
  VersionedPtr(const VersionedComPtr<Base, Versions...> &other);
  VersionedPtr(VersionedComPtr<Base, Versions...> &other);
  template <typename T>
  VersionedPtr(T *ptr) : pointer{ptr}, versionIndex{TypeIndexInTypePack<T, TypePackSet>::value}
  {}
  VersionedPtr(const VersionedPtr &other) = default;
  VersionedPtr &operator=(const VersionedPtr &other) = default;

  template <typename T>
  constexpr bool is() const
  {
    return pointer ? canCast<T>(versionIndex) : true;
  }

  template <typename T>
  T *as()
  {
    G_ASSERTF(is<T>(), "this can not be safely cast to T");
    return static_cast<T *>(pointer);
  }

  template <typename T>
  const T *as() const
  {
    G_ASSERTF(is<T>(), "this can not be safely cast to T");
    return static_cast<const T *>(pointer);
  }

  template <typename T>
  void reset(T *ptr)
  {
    pointer = ptr;
    versionIndex = TypeIndexInTypePack<T, TypePackSet>::value;
  }

  Base *get() { return pointer; }

  const Base *get() const { return pointer; }

  Base *operator->() { return pointer; }
  const Base *operator->() const { return pointer; }

  void reset() { pointer = nullptr; }

  explicit operator bool() const { return nullptr != pointer; }
};

// Optimized version that does not need to track type, as only the base exists
template <typename Base>
class VersionedPtr<Base>
{
  Base *pointer = nullptr;

public:
  VersionedPtr() = default;
  ~VersionedPtr() = default;
  VersionedPtr(const VersionedComPtr<Base> &other);
  VersionedPtr(VersionedComPtr<Base> &other);
  template <typename T>
  VersionedPtr(Base *ptr) : pointer{ptr}
  {}
  VersionedPtr(const VersionedPtr &other) = default;
  VersionedPtr &operator=(const VersionedPtr &other) = default;

  template <typename T>
  static constexpr bool is()
  {
    return eastl::is_convertible_v<eastl::add_pointer_t<Base>, eastl::add_pointer_t<T>>;
  }

  template <typename T>
  eastl::enable_if_t<is<T>(), T *> as()
  {
    return static_cast<T *>(pointer);
  }

  template <typename T>
  eastl::enable_if_t<is<T>(), const T *> as() const
  {
    return static_cast<const T *>(pointer);
  }

  template <typename T>
  eastl::enable_if_t<eastl::is_same_v<T, Base>> reset(T *ptr)
  {
    pointer = ptr;
  }

  Base *get() { return pointer; }

  const Base *get() const { return pointer; }

  Base *operator->() { return pointer; }
  const Base *operator->() const { return pointer; }

  void reset() { pointer = nullptr; }

  explicit operator bool() const { return nullptr != pointer; }
};

/**
 * Special smart com pointer to support versioned interfaces like
 * ID3D12Device, ID3d12Device1 and so on.
 * It keeps track of which version it stores and handles all the logistics
 * of accessing interfaces of each type.
 * Requirements:
 * Base is a common bas of any entry in Versions.
 * Base interface contains Release() to release references and AddRef() to increase references.
 */
template <typename Base, typename... Versions>
class VersionedComPtr
{
  using TypePackSet = TypePack<Base, Versions...>;
  using PointerTypePackSet = TypePack<eastl::add_pointer_t<Base>, eastl::add_pointer_t<Versions>...>;
  Base *pointer = nullptr;
  size_t versionIndex = 0;
  void tidy()
  {
    decRef();
    pointer = nullptr;
  }
  void decRef()
  {
    if (pointer)
      pointer->Release();
  }
  void incRef()
  {
    if (pointer)
      pointer->AddRef();
  }

  template <typename T>
  static bool canCast(size_t from)
  {
    static constexpr bool table[] = //
      {eastl::is_convertible_v<eastl::add_pointer_t<Base>, eastl::add_pointer_t<T>>,
        (eastl::is_convertible_v<eastl::add_pointer_t<Versions>, eastl::add_pointer_t<T>>)...};
    return table[from];
  }

  template <typename T, typename U>
  bool tryQuery(T clb)
  {
    if (clb(__uuidof(U), reinterpret_cast<void **>(&pointer)))
    {
      versionIndex = TypeIndexInTypePack<U, TypePackSet>::value;
      return true;
    }
    pointer = nullptr;
    return false;
  }

  template <typename T, typename U, typename O, typename... Os>
  bool tryQuery(T clb)
  {
    // gow from last to first entries
    if (tryQuery<T, O, Os...>(clb))
      return true;
    if (clb(__uuidof(U), reinterpret_cast<void **>(&pointer)))
    {
      versionIndex = TypeIndexInTypePack<U, TypePackSet>::value;
      return true;
    }
    return false;
  }

public:
  VersionedComPtr() = default;
  ~VersionedComPtr() { decRef(); }
  template <typename T>
  VersionedComPtr(T *ptr) : pointer{ptr}, versionIndex{TypeIndexInTypePack<T, TypePackSet>::value}
  {}
  VersionedComPtr(const VersionedComPtr &other) : pointer{other.pointer}, versionIndex{other.versionIndex} { incRef(); }
  VersionedComPtr &operator=(const VersionedComPtr &other)
  {
    decRef();
    pointer = other.pointer;
    versionIndex = other.versionIndex;
    incRef();
    return *this;
  }

  VersionedComPtr(VersionedComPtr &&other) : pointer{other.release()}, versionIndex{other.versionIndex} {}
  VersionedComPtr &operator=(VersionedComPtr &&other)
  {
    decRef();
    pointer = other.release();
    versionIndex = other.versionIndex;
    return *this;
  }

  Base *release()
  {
    auto ptr = pointer;
    pointer = nullptr;
    return ptr;
  }

  template <typename T>
  constexpr bool is() const
  {
    return pointer ? canCast<T>(versionIndex) : true;
  }

  template <typename T>
  T *as()
  {
    G_ASSERTF(is<T>(), "this can not be safely cast to T");
    return static_cast<T *>(pointer);
  }

  template <typename T>
  const T *as() const
  {
    G_ASSERTF(is<T>(), "this can not be safely cast to T");
    return static_cast<const T *>(pointer);
  }

  template <typename T>
  bool autoQuery(T clb)
  {
    decRef();
    tryQuery<T, Base, Versions...>(clb);
    return pointer != nullptr;
  }

  template <typename T>
  void reset(T *ptr)
  {
    decRef();
    pointer = ptr;
    versionIndex = TypeIndexInTypePack<T, TypePackSet>::value;
  }

  Base *get() { return pointer; }

  const Base *get() const { return pointer; }

  Base *operator->() { return pointer; }
  const Base *operator->() const { return pointer; }

  void reset() { tidy(); }

  size_t getVersionIndex() const { return versionIndex; }

  explicit operator bool() const { return nullptr != pointer; }
};

// Optimized version that does not need to track type, as only the base exists
template <typename Base>
class VersionedComPtr<Base>
{
  Base *pointer = nullptr;
  void tidy()
  {
    decRef();
    pointer = nullptr;
  }
  void decRef()
  {
    if (pointer)
      pointer->Release();
  }
  void incRef()
  {
    if (pointer)
      pointer->AddRef();
  }

public:
  VersionedComPtr() = default;
  ~VersionedComPtr() { decRef(); }
  VersionedComPtr(Base *ptr) : pointer{ptr} {}
  VersionedComPtr(const VersionedComPtr &other) : pointer{other.pointer} { incRef(); }
  VersionedComPtr &operator=(const VersionedComPtr &other)
  {
    decRef();
    pointer = other.pointer;
    incRef();
    return *this;
  }

  VersionedComPtr(VersionedComPtr &&other) : pointer{other.release()} {}
  VersionedComPtr &operator=(VersionedComPtr &&other)
  {
    decRef();
    pointer = other.release();
    return *this;
  }

  Base *release()
  {
    auto ptr = pointer;
    pointer = nullptr;
    return ptr;
  }

  template <typename T>
  static constexpr bool is()
  {
    return eastl::is_convertible_v<eastl::add_pointer_t<Base>, eastl::add_pointer_t<T>>;
  }

  template <typename T>
  eastl::enable_if_t<is<T>(), T *> as()
  {
    return static_cast<T *>(pointer);
  }

  template <typename T>
  eastl::enable_if_t<is<T>(), const T *> as() const
  {
    return static_cast<const T *>(pointer);
  }

  template <typename T>
  bool autoQuery(T clb)
  {
    decRef();
    clb(__uuidof(Base), reinterpret_cast<void **>(&pointer));
    return pointer != nullptr;
  }

  template <typename T>
  eastl::enable_if_t<eastl::is_same_v<T, Base>> reset(Base *ptr)
  {
    decRef();
    pointer = ptr;
  }

  Base *get() { return pointer; }

  const Base *get() const { return pointer; }

  Base *operator->() { return pointer; }
  const Base *operator->() const { return pointer; }

  void reset() { tidy(); }

  size_t getVersionIndex() const { return 0; }

  explicit operator bool() const { return nullptr != pointer; }
};

template <typename Base, typename... Versions>
inline VersionedPtr<Base, Versions...>::VersionedPtr(const VersionedComPtr<Base, Versions...> &other) :
  pointer{other.get()}, versionIndex{other.getVersionIndex()}
{}

template <typename Base, typename... Versions>
inline VersionedPtr<Base, Versions...>::VersionedPtr(VersionedComPtr<Base, Versions...> &other) :
  pointer{other.get()}, versionIndex{other.getVersionIndex()}
{}

template <typename Base>
inline VersionedPtr<Base>::VersionedPtr(const VersionedComPtr<Base> &other) : pointer{other.get()}
{}

template <typename Base>
inline VersionedPtr<Base>::VersionedPtr(VersionedComPtr<Base> &other) : pointer{other.get()}
{}
