#pragma once

#include <EASTL/utility.h>


// Very simple wrapper to make a non thread safe container thread safe with the help of a paired mutex.
// Access is grated with a AccessToken type, which grants access to the containers interface.
template <typename T, typename MTX>
class ContainerMutexWrapper
{
  MTX mtx;
  T container;

  void lock() { mtx.lock(); }

  void unlock() { mtx.unlock(); }

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

    T &operator*() { return parent->data(); }
    T *operator->() { return &parent->data(); }
  };

  AccessToken access() { return {*this}; }
};
