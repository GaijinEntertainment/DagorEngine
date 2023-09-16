#pragma once

#include <EASTL/type_traits.h>

namespace ecs
{

template <typename T>
struct HasShouldProcess
{
  struct Fallback
  {
    void should_process();
  };
  struct Derived : T, Fallback
  {
    Derived() {}
  };

  template <typename U>
  static eastl::false_type test(decltype(&U::should_process) *);

  template <typename U>
  static eastl::true_type test(...);

  static bool const value = eastl::is_same<decltype(test<Derived>(0)), eastl::true_type>::value;
};

template <class ClassName, int shouldProcess = HasShouldProcess<ClassName>::value>
struct CheckShouldProcess;

template <class ClassName>
struct CheckShouldProcess<ClassName, 1>
{
  template <typename T>
  bool operator()(const T &t)
  {
    return ClassName::should_process(t);
  }
};

template <class ClassName>
struct CheckShouldProcess<ClassName, 0>
{
  template <typename T>
  bool operator()(const T &)
  {
    return true;
  }
};


template <class ClassName, class T>
bool optional_should_process(const T &t)
{
  return CheckShouldProcess<ClassName>()(t);
}

}; // namespace ecs