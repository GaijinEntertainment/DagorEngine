#pragma once
template <typename T, typename A = eastl::allocator, typename C = uint32_t>
using SmallTab = dag::Vector<T, A, false, C>;
