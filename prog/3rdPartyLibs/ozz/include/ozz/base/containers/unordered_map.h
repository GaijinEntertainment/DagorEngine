//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#ifndef OZZ_OZZ_BASE_CONTAINERS_UNORDERED_MAP_H_
#define OZZ_OZZ_BASE_CONTAINERS_UNORDERED_MAP_H_

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)  // warning C4702: unreachable code
#endif                           // _MSC_VER

#include <unordered_map>

#ifdef _MSC_VER
#pragma warning(pop)
#endif  // _MSC_VER

#include "ozz/base/containers/std_allocator.h"

namespace ozz {

// Redirects std::unordered_map to ozz::unordered_map in order to replace std
// default allocator by ozz::StdAllocator.
template <class _Key, class _Ty, class _Hash = std::hash<_Key>,
          class _KeyEqual = std::equal_to<_Key>,
          class _Allocator = ozz::StdAllocator<std::pair<const _Key, _Ty>>>
using unordered_map =
    std::unordered_map<_Key, _Ty, _Hash, _KeyEqual, _Allocator>;

// Redirects std::unordered_multimap to ozz::UnorderedMultiMap in order to
// replace std default allocator by ozz::StdAllocator.
template <class _Key, class _Ty, class _Hash = std::hash<_Key>,
          class _KeyEqual = std::equal_to<_Key>,
          class _Allocator = ozz::StdAllocator<std::pair<const _Key, _Ty>>>
using unordered_multimap =
    std::unordered_multimap<_Key, _Ty, _Hash, _KeyEqual, _Allocator>;
}  // namespace ozz
#endif  // OZZ_OZZ_BASE_CONTAINERS_UNORDERED_MAP_H_
