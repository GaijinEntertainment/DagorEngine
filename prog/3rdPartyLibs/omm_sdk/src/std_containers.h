/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <streambuf>
#include <ostream>
#include <istream>

#include "omm.h"

#include "std_allocator.h"

namespace omm
{
    template<class TKey, class TVal>
    using hash_map_allocator = StdAllocator<std::pair<const TKey, TVal>>;

    template<class TKey, class TVal>
    using hash_map = std::unordered_map<TKey, TVal, std::hash<TKey>, std::equal_to<TKey>, hash_map_allocator<const TKey, TVal>>;

    template<class TKey, class TVal>
    using map_allocator = StdAllocator<std::pair<const TKey, TVal>>;

    template<class TKey, class TVal>
    using map = std::map<TKey, TVal, std::less<TKey>, map_allocator<const TKey, TVal>>;

    using string_allocator = StdAllocator<char>;

    using string = std::basic_string<char, std::char_traits<char>, StdAllocator<char>>;

    template<class TKey>
    using set_allocator = StdAllocator<TKey>;

    template<class TKey>
    using set = std::set<TKey, std::less<TKey>, set_allocator<TKey>>;

    template<class TVal>
    using vector = std::vector<TVal, StdAllocator<TVal>>;

    template<class TVal>
    using list = std::list<TVal, StdAllocator<TVal>>;
}