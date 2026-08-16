#pragma once

#include <type_traits>

namespace rc {
namespace compat {

#if __cpp_lib_is_invocable >= 201703
template <typename Fn, typename ...Args>
using return_type = typename std::invoke_result<Fn,Args...>;
#else
template <typename Fn, typename ...Args>
using return_type = typename std::result_of<Fn(Args...)>;
#endif

}
}
