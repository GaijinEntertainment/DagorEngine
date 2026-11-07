// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/utility.h>
#include <osApiWrappers/dag_threadSafety.h>


// Very simple wrapper to make a non thread safe container thread safe with the help of a paired mutex.
// Access is grated with a AccessToken type, which grants access to the containers interface.
template <typename T, typename MTX>
class ContainerMutexWrapper
{
  MTX mtx;
  T container;

  // NOTE: this class is automatically always thread safe via the type system, no need for static analysis.

  void lock() DAG_TS_NO_THREAD_SAFETY_ANALYSIS { mtx.lock(); }

  void unlock() DAG_TS_NO_THREAD_SAFETY_ANALYSIS { mtx.unlock(); }

  T &data() { return container; }

public:
  ContainerMutexWrapper() = default;
  ~ContainerMutexWrapper() = default;

  ContainerMutexWrapper(const ContainerMutexWrapper &) = delete;
  ContainerMutexWrapper &operator=(const ContainerMutexWrapper &) = delete;

  ContainerMutexWrapper(ContainerMutexWrapper &&) = delete;
  ContainerMutexWrapper &operator=(ContainerMutexWrapper &&) = delete;

  class AccessToken
  {
    ContainerMutexWrapper *parent = nullptr;

  public:
    AccessToken() = default;
    ~AccessToken()
    {
      if (parent)
      {
        parent->unlock();
      }
    }

    AccessToken(ContainerMutexWrapper &p) : parent{&p} { parent->lock(); }

    AccessToken(const AccessToken &) = delete;
    AccessToken &operator=(const AccessToken &) = delete;

    AccessToken(AccessToken &&other) : parent{other.parent} { other.parent = nullptr; }
    AccessToken &operator=(AccessToken &&other)
    {
      eastl::swap(parent, other.parent);
      return *this;
    }

    T &operator*() const { return parent->data(); }
    T *operator->() const { return &parent->data(); }
  };

  AccessToken access() { return {*this}; }
};

template <typename T, typename MTX>
class ContainerReadWriteMutexWrapper
{
  MTX mtx;
  T container;

  // NOTE: this class is automatically always thread safe via the type system, no need for static analysis.

  void lockRead() DAG_TS_NO_THREAD_SAFETY_ANALYSIS { mtx.lockRead(); }
  void lockWrite() DAG_TS_NO_THREAD_SAFETY_ANALYSIS { mtx.lockWrite(); }

  void unlockRead() DAG_TS_NO_THREAD_SAFETY_ANALYSIS { mtx.unlockRead(); }
  void unlockWrite() DAG_TS_NO_THREAD_SAFETY_ANALYSIS { mtx.unlockWrite(); }

  T &data() { return container; }

public:
  ContainerReadWriteMutexWrapper() = default;
  ~ContainerReadWriteMutexWrapper() = default;

  ContainerReadWriteMutexWrapper(const ContainerReadWriteMutexWrapper &) = delete;
  ContainerReadWriteMutexWrapper &operator=(const ContainerReadWriteMutexWrapper &) = delete;

  ContainerReadWriteMutexWrapper(ContainerReadWriteMutexWrapper &&) = delete;
  ContainerReadWriteMutexWrapper &operator=(ContainerReadWriteMutexWrapper &&) = delete;

  class ReadAccessToken
  {
    ContainerReadWriteMutexWrapper *parent = nullptr;

  public:
    ReadAccessToken() = default;
    ~ReadAccessToken()
    {
      if (parent)
      {
        parent->unlockRead();
      }
    }

    ReadAccessToken(ContainerReadWriteMutexWrapper &p) : parent{&p} { parent->lockRead(); }

    ReadAccessToken(const ReadAccessToken &) = delete;
    ReadAccessToken &operator=(const ReadAccessToken &) = delete;

    ReadAccessToken(ReadAccessToken &&other) : parent{other.parent} { other.parent = nullptr; }
    ReadAccessToken &operator=(ReadAccessToken &&other)
    {
      eastl::swap(parent, other.parent);
      return *this;
    }

    const T &operator*() const { return parent->data(); }
    const T *operator->() const { return &parent->data(); }
  };

  class WriteAccessToken
  {
    ContainerReadWriteMutexWrapper *parent = nullptr;

  public:
    WriteAccessToken() = default;
    ~WriteAccessToken()
    {
      if (parent)
      {
        parent->unlockWrite();
      }
    }

    WriteAccessToken(ContainerReadWriteMutexWrapper &p) : parent{&p} { parent->lockWrite(); }

    WriteAccessToken(const WriteAccessToken &) = delete;
    WriteAccessToken &operator=(const WriteAccessToken &) = delete;

    WriteAccessToken(WriteAccessToken &&other) : parent{other.parent} { other.parent = nullptr; }
    WriteAccessToken &operator=(WriteAccessToken &&other)
    {
      eastl::swap(parent, other.parent);
      return *this;
    }

    T &operator*() const { return parent->data(); }
    T *operator->() const { return &parent->data(); }
  };

  ReadAccessToken read() { return {*this}; }
  WriteAccessToken write() { return {*this}; }
};
