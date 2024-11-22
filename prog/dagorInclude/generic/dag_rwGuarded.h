//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_rwSpinLock.h>
#include <EASTL/utility.h>


namespace dag
{
/// @brief Thread safe container
///
/// @tparam T Type of object
/// @tparam ReadWriteLock Lock type - should have methods lockRead(), lockWrite(),
///   unlockRead() and unlockWrite()
///
/// Idea of this container: you will get pointer to object
/// after lock for read or write.
/// It allows programming of inside class without thinking about
/// syncronization and easily moving lock upper in the call stack.
///
/// @code{.cpp}
///   RWGuarded<T> container;
///
///   {
///     auto lockedRW = container.lockWrite();
///     lockedRW->someNotConstMethod();
///   }
///
///   {
///     auto lockedRO = container.lockRead();
///     lockedRO->someConstMethod();
///   }
/// @endcode
template <typename T, typename ReadWriteLock = SpinLockReadWriteLock>
struct RWGuarded
{
  using ValueType = T;
  using RWLock = ReadWriteLock;
  using This = RWGuarded<ValueType, RWLock>;
  struct LockedRW;
  struct LockedRO;

  template <typename... Args>
  RWGuarded(Args &&...args) : value(eastl::forward<Args>(args)...)
  {}
  ~RWGuarded() = default;
  RWGuarded(const This &) = delete;
  This &operator=(const This &) = delete;
  RWGuarded(This &&) = delete;
  This &operator=(This &&) = delete;

  /// @brief Lock object for read/write
  LockedRW lockWrite() { return LockedRW{*this}; }

  /// @brief Lock object for read only
  LockedRO lockRead() const { return LockedRO{*this}; }

  struct LockedRW
  {
    LockedRW() = default;
    explicit LockedRW(This &owner_) DAG_TS_NO_THREAD_SAFETY_ANALYSIS : owner(&owner_) { owner->rwLock.lockWrite(); }
    ~LockedRW() { reset(); }
    void reset() DAG_TS_NO_THREAD_SAFETY_ANALYSIS
    {
      if (!owner)
        return;

      owner->rwLock.unlockWrite();
      owner = nullptr;
    }
    LockedRW(const LockedRW &) = delete;
    LockedRW &operator=(const LockedRW &) = delete;
    LockedRW(LockedRW &&other) : owner(other.owner) { other.owner = nullptr; }
    LockedRW &operator=(LockedRW &&other)
    {
      reset();
      eastl::swap(owner, other.owner);
      return *this;
    }

    /// @brief Dereference
    ValueType *operator->() const
    {
      G_ASSERT(owner);
      return &owner->value;
    }

    /// @brief Dereference
    ValueType &operator*() const
    {
      G_ASSERT(owner);
      return owner->value;
    }

    /// @brief Get pointer to value
    ///
    /// @return pointer to value or nullptr, if it wasn't locked
    ValueType *ptr() const { return owner ? &owner->value : nullptr; }

    /// @brief Check if object was locked
    explicit operator bool() const { return owner; }

  private:
    This *owner = nullptr;
  };

  struct LockedRO
  {
    LockedRO() = default;
    explicit LockedRO(const This &owner_) DAG_TS_NO_THREAD_SAFETY_ANALYSIS : owner(&owner_) { owner->rwLock.lockRead(); }
    ~LockedRO() { reset(); }
    void reset() DAG_TS_NO_THREAD_SAFETY_ANALYSIS
    {
      if (!owner)
        return;

      owner->rwLock.unlockRead();
      owner = nullptr;
    }
    LockedRO(const LockedRW &) = delete;
    LockedRO &operator=(const LockedRO &) = delete;
    LockedRO(LockedRO &&other) : owner(other.owner) { other.owner = nullptr; }
    LockedRO &operator=(LockedRO &&other)
    {
      reset();
      eastl::swap(owner, other.owner);
      return *this;
    }

    /// @brief Dereference
    const ValueType *operator->() const
    {
      G_ASSERT(owner);
      return &owner->value;
    }

    /// @brief Dereference
    const ValueType &operator*() const
    {
      G_ASSERT(owner);
      return owner->value;
    }

    /// @brief Get pointer to value
    ///
    /// @return pointer to value or nullptr, if it wasn't locked
    const ValueType *ptr() const { return owner ? &owner->value : nullptr; }

    /// @brief Check if object was locked
    explicit operator bool() const { return owner; }

  private:
    const This *owner = nullptr;
  };

protected:
  ValueType value;
  mutable RWLock rwLock;
};
} // namespace dag