#pragma once

#include "rapidcheck/fn/Common.h"
#include "rapidcheck/shrinkable/Create.h"
#include "rapidcheck/Compat.h"

namespace rc {
namespace gen {

template <typename T>
Gen<Decay<T>> just(T &&value) {
  return fn::constant(shrinkable::just(std::forward<T>(value)));
}

template <typename Callable>
Gen<typename rc::compat::return_type<Callable>::type::ValueType>
lazy(Callable &&callable) {
  return
      [=](const Random &random, int size) { return callable()(random, size); };
}

} // namespace gen
} // namespace rc
